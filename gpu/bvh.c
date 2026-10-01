/*
** BVH construido en la CPU con SAH por cubetas (binned SAH): en cada nodo
** se prueban 12 cortes por eje y se elige el que minimiza
** area_izq * n_izq + area_der * n_der.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "bvh.h"

#define BINS 12
#define LEAF_SIZE 2

typedef struct	s_box
{
	simd_float3	min;
	simd_float3	max;
}				t_box;

typedef struct	s_build
{
	t_gpu_object	*objs;
	t_box			*boxes;
	simd_float3		*centers;
	t_gpu_node		*nodes;
	int				used;
}				t_build;

static t_box	empty_box(void)
{
	return ((t_box){simd_make_float3(INFINITY, INFINITY, INFINITY),
		simd_make_float3(-INFINITY, -INFINITY, -INFINITY)});
}

static t_box	grow(t_box b, t_box o)
{
	return ((t_box){simd_min(b.min, o.min), simd_max(b.max, o.max)});
}

static float	area(t_box b)
{
	simd_float3 e = b.max - b.min;

	if (e.x < 0)
		return (0);
	return (e.x * e.y + e.y * e.z + e.z * e.x);
}

static t_box	object_box(t_gpu_object *o)
{
	simd_float3	a = o->a.xyz;
	simd_float3	ext;

	if (o->type == GPU_SPHERE)
		ext = simd_make_float3(o->radius, o->radius, o->radius);
	else if (o->type == GPU_CYLINDER)
	{
		simd_float3 ax = o->b.xyz;
		ext = simd_abs(ax) * (o->height / 2) + o->radius
			* simd_make_float3(sqrtf(fmaxf(0, 1 - ax.x * ax.x)),
			sqrtf(fmaxf(0, 1 - ax.y * ax.y)), sqrtf(fmaxf(0, 1 - ax.z * ax.z)));
	}
	else if (o->type == GPU_SQUARE)
		ext = (simd_abs(o->c.xyz) + simd_abs(simd_cross(o->b.xyz, o->c.xyz)))
			* (o->height / 2);
	else
	{
		t_box b = {simd_min(simd_min(a, o->b.xyz), o->c.xyz),
			simd_max(simd_max(a, o->b.xyz), o->c.xyz)};
		return (b);
	}
	return ((t_box){a - ext, a + ext});
}

static void		set_node(t_gpu_node *n, t_box b, int first, int count)
{
	*n = (t_gpu_node){b.min.x, b.min.y, b.min.z, first,
		b.max.x, b.max.y, b.max.z, count};
}

static void		swap(t_build *s, int i, int j)
{
	t_gpu_object	o = s->objs[i];
	t_box			b = s->boxes[i];
	simd_float3		c = s->centers[i];

	s->objs[i] = s->objs[j];
	s->objs[j] = o;
	s->boxes[i] = s->boxes[j];
	s->boxes[j] = b;
	s->centers[i] = s->centers[j];
	s->centers[j] = c;
}

static int		best_split(t_build *s, int first, int count, t_box cb,
					int *axis_out, float *pos_out)
{
	float	best = INFINITY;
	int		axis;
	int		i;

	for (axis = 0; axis < 3; axis++)
	{
		float lo = cb.min[axis], hi = cb.max[axis];
		if (hi - lo < 1e-6f)
			continue ;
		t_box	bins[BINS];
		int		cnt[BINS] = {0};
		for (i = 0; i < BINS; i++)
			bins[i] = empty_box();
		float scale = BINS / (hi - lo);
		for (i = first; i < first + count; i++)
		{
			int b = (int)((s->centers[i][axis] - lo) * scale);
			b = b >= BINS ? BINS - 1 : b;
			cnt[b]++;
			bins[b] = grow(bins[b], s->boxes[i]);
		}
		float	left_area[BINS - 1];
		int		left_cnt[BINS - 1];
		t_box	acc = empty_box();
		int		n = 0;
		for (i = 0; i < BINS - 1; i++)
		{
			acc = grow(acc, bins[i]);
			n += cnt[i];
			left_area[i] = area(acc);
			left_cnt[i] = n;
		}
		acc = empty_box();
		n = 0;
		for (i = BINS - 1; i > 0; i--)
		{
			acc = grow(acc, bins[i]);
			n += cnt[i];
			float cost = left_area[i - 1] * left_cnt[i - 1] + area(acc) * n;
			if (left_cnt[i - 1] > 0 && n > 0 && cost < best)
			{
				best = cost;
				*axis_out = axis;
				*pos_out = lo + i / scale;
			}
		}
	}
	return (best < INFINITY);
}

static void		build(t_build *s, int index, int first, int count)
{
	t_box	b = empty_box();
	t_box	cb = empty_box();
	int		axis;
	float	pos;
	int		i;
	int		j;

	for (i = first; i < first + count; i++)
	{
		b = grow(b, s->boxes[i]);
		cb = grow(cb, (t_box){s->centers[i], s->centers[i]});
	}
	if (count <= LEAF_SIZE || !best_split(s, first, count, cb, &axis, &pos))
	{
		set_node(&s->nodes[index], b, first, count);
		return ;
	}
	i = first;
	j = first + count - 1;
	while (i <= j)
	{
		if (s->centers[i][axis] < pos)
			i++;
		else
			swap(s, i, j--);
	}
	if (i == first || i == first + count)
	{
		set_node(&s->nodes[index], b, first, count);
		return ;
	}
	int left = s->used;
	s->used += 2;
	set_node(&s->nodes[index], b, left, 0);
	build(s, left, first, i - first);
	build(s, left + 1, i, first + count - i);
}

int				partition_planes(t_gpu_scene *scene)
{
	int p = 0;

	for (int i = 0; i < scene->nobjects; i++)
		if (scene->objects[i].type == GPU_PLANE)
		{
			t_gpu_object tmp = scene->objects[p];
			scene->objects[p++] = scene->objects[i];
			scene->objects[i] = tmp;
		}
	return (p);
}

t_gpu_node		*build_bvh(t_gpu_scene *scene, int *nnodes, int *nplanes)
{
	t_build	s;
	int		n = scene->nobjects;
	int		p = partition_planes(scene);
	int		i;

	*nplanes = p;
	s.objs = scene->objects + p;
	n -= p;
	s.boxes = malloc(sizeof(t_box) * (n + 1));
	s.centers = malloc(sizeof(simd_float3) * (n + 1));
	s.nodes = calloc(2 * n + 2, sizeof(t_gpu_node));
	for (i = 0; i < n; i++)
	{
		s.boxes[i] = object_box(&s.objs[i]);
		s.centers[i] = (s.boxes[i].min + s.boxes[i].max) * 0.5f;
	}
	s.used = 1;
	if (n > 0)
		build(&s, 0, 0, n);
	for (i = 0; i < s.used; i++)
		if (s.nodes[i].count > 0)
			s.nodes[i].first += p;
	*nnodes = s.used;
	free(s.boxes);
	free(s.centers);
	return (s.nodes);
}

void			object_bounds(t_gpu_object *o, float *min, float *max)
{
	t_box b = object_box(o);

	min[0] = b.min.x;
	min[1] = b.min.y;
	min[2] = b.min.z;
	max[0] = b.max.x;
	max[1] = b.max.y;
	max[2] = b.max.z;
}
