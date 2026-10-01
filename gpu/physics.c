/*
** Motor de fisica de solidos rigidos, desde cero.
**
** Paso de simulacion (world_step), repetido en subpasos:
**   1. gravedad y amortiguamiento sobre las velocidades
**   2. posicion de cada esfera en el mundo
**   3. contactos: esfera contra triangulos estaticos (rejilla uniforme) y
**      esfera contra esfera de otros cuerpos (tabla hash espacial)
**   4. impulsos secuenciales: normal (con rebote y correccion de
**      penetracion) y rozamiento de Coulomb, varias iteraciones
**   5. integracion de posiciones y orientaciones (cuaterniones)
**   6. cuerpos casi quietos se duermen para no temblar
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "physics.h"

#define SLOP 0.001f
#define BAUMGARTE 0.25f
#define SLEEP_SPEED 0.04f
#define SLEEP_TIME 0.6f
#define ROLLING 3.0f

static simd_float3	v3(const float *p)
{
	return (simd_make_float3(p[0], p[1], p[2]));
}

void			world_init(t_world *w)
{
	memset(w, 0, sizeof(*w));
	w->gravity = simd_make_float3(0, -9.81f, 0);
	w->iterations = 12;
	w->substeps = 10;
}

/*
** Triangulos estaticos en una rejilla uniforme (formato compacto: para cada
** celda, el rango de indices de triangulo que la tocan).
*/

void			world_set_static(t_world *w, const float *tris, int ntris,
					float cell)
{
	simd_float3	lo = simd_make_float3(1e30f, 1e30f, 1e30f);
	simd_float3	hi = -lo;

	w->tris = malloc(sizeof(float) * 9 * ntris);
	memcpy(w->tris, tris, sizeof(float) * 9 * ntris);
	w->ntris = ntris;
	for (int i = 0; i < ntris * 3; i++)
	{
		lo = simd_min(lo, v3(tris + i * 3));
		hi = simd_max(hi, v3(tris + i * 3));
	}
	lo -= cell;
	hi += cell;
	for (int k = 0; k < 3; k++)
	{
		float ext = hi[k] - lo[k];
		if (ext / cell > 400)
			cell = ext / 400;
	}
	w->cell = cell;
	w->gmin = lo;
	for (int k = 0; k < 3; k++)
		w->gdim[k] = (int)ceilf((hi[k] - lo[k]) / cell) + 1;
	int ncells = w->gdim[0] * w->gdim[1] * w->gdim[2];
	int *count = calloc(ncells + 1, sizeof(int));
	for (int pass = 0; pass < 2; pass++)
	{
		for (int t = 0; t < ntris; t++)
		{
			simd_float3 a = v3(tris + t * 9), b = v3(tris + t * 9 + 3);
			simd_float3 c = v3(tris + t * 9 + 6);
			simd_float3 mn = simd_min(simd_min(a, b), c);
			simd_float3 mx = simd_max(simd_max(a, b), c);
			int i0[3], i1[3];
			for (int k = 0; k < 3; k++)
			{
				i0[k] = (int)((mn[k] - lo[k]) / cell);
				i1[k] = (int)((mx[k] - lo[k]) / cell);
			}
			for (int z = i0[2]; z <= i1[2]; z++)
				for (int y = i0[1]; y <= i1[1]; y++)
					for (int x = i0[0]; x <= i1[0]; x++)
					{
						int id = (z * w->gdim[1] + y) * w->gdim[0] + x;
						if (pass == 0)
							count[id + 1]++;
						else
							w->cell_items[w->cell_start[id] + count[id]++] = t;
					}
		}
		if (pass == 0)
		{
			for (int i = 0; i < ncells; i++)
				count[i + 1] += count[i];
			w->cell_start = malloc(sizeof(int) * (ncells + 1));
			memcpy(w->cell_start, count, sizeof(int) * (ncells + 1));
			w->cell_items = malloc(sizeof(int) * (count[ncells] + 1));
			memset(count, 0, sizeof(int) * (ncells + 1));
		}
	}
	free(count);
	w->stamp = calloc(ntris + 1, sizeof(int));
}

int				world_add_body(t_world *w, const t_psphere *spheres, int n,
					float density, simd_float3 rest_com)
{
	t_body			*b;
	float			mass = 0;
	simd_float3x3	inertia = {0};

	if (w->nbodies == w->capbodies)
	{
		w->capbodies = w->capbodies ? w->capbodies * 2 : 64;
		w->bodies = realloc(w->bodies, sizeof(t_body) * w->capbodies);
	}
	while (w->nspheres + n > w->capspheres)
	{
		w->capspheres = w->capspheres ? w->capspheres * 2 : 1024;
		w->local = realloc(w->local, sizeof(t_psphere) * w->capspheres);
		w->wpos = realloc(w->wpos, sizeof(simd_float3) * w->capspheres);
		w->owner = realloc(w->owner, sizeof(int) * w->capspheres);
	}
	b = &w->bodies[w->nbodies];
	memset(b, 0, sizeof(*b));
	b->first = w->nspheres;
	b->count = n;
	for (int i = 0; i < n; i++)
	{
		simd_float3 d = spheres[i].c - rest_com;
		float r = spheres[i].r;
		float m = density * 4.0f / 3.0f * (float)M_PI * r * r * r;
		mass += m;
		/*
		** Inercia de una esfera maciza + teorema de Steiner.
		*/
		float dd = simd_dot(d, d);
		simd_float3x3 sphere = simd_diagonal_matrix(simd_make_float3(
			0.4f * m * r * r + m * dd, 0.4f * m * r * r + m * dd,
			0.4f * m * r * r + m * dd));
		simd_float3x3 outer = simd_matrix(d * d.x, d * d.y, d * d.z);
		inertia = simd_add(inertia, simd_sub(sphere,
			simd_mul(m, outer)));
		w->local[w->nspheres] = (t_psphere){d, r};
		w->owner[w->nspheres] = w->nbodies;
		w->nspheres++;
	}
	b->inv_mass = mass > 0 ? 1 / mass : 0;
	b->inv_inertia = simd_inverse(inertia);
	b->pos = rest_com;
	b->rest_com = rest_com;
	b->rot = simd_quaternion(0.0f, simd_make_float3(0, 1, 0));
	b->restitution = 0.25f;
	b->friction = 0.5f;
	b->active = 1;
	return (w->nbodies++);
}

simd_float3		body_point(const t_body *b, simd_float3 rest)
{
	return (b->pos + simd_act(b->rot, rest - b->rest_com));
}

simd_float3		body_dir(const t_body *b, simd_float3 v)
{
	return (simd_act(b->rot, v));
}

void			body_wake(t_body *b)
{
	b->sleeping = 0;
	b->idle = 0;
}

/*
** Punto del triangulo (a, b, c) mas cercano a p (Ericson, Real-Time
** Collision Detection, 5.1.5).
*/

static simd_float3	closest_on_tri(simd_float3 p, simd_float3 a, simd_float3 b,
						simd_float3 c)
{
	simd_float3	ab = b - a, ac = c - a, ap = p - a;
	float		d1 = simd_dot(ab, ap), d2 = simd_dot(ac, ap);

	if (d1 <= 0 && d2 <= 0)
		return (a);
	simd_float3 bp = p - b;
	float d3 = simd_dot(ab, bp), d4 = simd_dot(ac, bp);
	if (d3 >= 0 && d4 <= d3)
		return (b);
	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0 && d1 >= 0 && d3 <= 0)
		return (a + ab * (d1 / (d1 - d3)));
	simd_float3 cp = p - c;
	float d5 = simd_dot(ab, cp), d6 = simd_dot(ac, cp);
	if (d6 >= 0 && d5 <= d6)
		return (c);
	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0 && d2 >= 0 && d6 <= 0)
		return (a + ac * (d2 / (d2 - d6)));
	float va = d3 * d6 - d5 * d4;
	if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0)
		return (b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6))));
	float denom = 1 / (va + vb + vc);
	return (a + ab * (vb * denom) + ac * (vc * denom));
}

static void		add_contact(t_world *w, int a, int b, simd_float3 p,
					simd_float3 n, float depth)
{
	if (w->ncontacts == w->capcontacts)
	{
		w->capcontacts = w->capcontacts ? w->capcontacts * 2 : 4096;
		w->contacts = realloc(w->contacts, sizeof(t_contact)
			* w->capcontacts);
	}
	w->contacts[w->ncontacts++] = (t_contact){a, b, p, n, depth, 0, 0, 0, 0};
}

static void		static_contacts(t_world *w, int s)
{
	simd_float3	c = w->wpos[s];
	float		r = w->local[s].r;
	int			i0[3], i1[3];

	/*
	** Suelo infinito (opcional): lo que rueda fuera del suelo visible no
	** cae al vacio.
	*/
	if (w->has_ground && c.y - r < w->ground_y)
		add_contact(w, w->owner[s], -1, simd_make_float3(c.x, w->ground_y,
			c.z), simd_make_float3(0, 1, 0), w->ground_y - (c.y - r));
	if (!w->ntris)
		return ;
	for (int k = 0; k < 3; k++)
	{
		i0[k] = (int)floorf((c[k] - r - w->gmin[k]) / w->cell);
		i1[k] = (int)floorf((c[k] + r - w->gmin[k]) / w->cell);
		if (i1[k] < 0 || i0[k] >= w->gdim[k])
			return ;
		i0[k] = i0[k] < 0 ? 0 : i0[k];
		i1[k] = i1[k] >= w->gdim[k] ? w->gdim[k] - 1 : i1[k];
	}
	w->stamp_id++;
	for (int z = i0[2]; z <= i1[2]; z++)
		for (int y = i0[1]; y <= i1[1]; y++)
			for (int x = i0[0]; x <= i1[0]; x++)
			{
				int id = (z * w->gdim[1] + y) * w->gdim[0] + x;
				for (int k = w->cell_start[id]; k < w->cell_start[id + 1]; k++)
				{
					int t = w->cell_items[k];
					if (w->stamp[t] == w->stamp_id)
						continue ;
					w->stamp[t] = w->stamp_id;
					const float *tp = w->tris + t * 9;
					simd_float3 q = closest_on_tri(c, v3(tp), v3(tp + 3),
						v3(tp + 6));
					simd_float3 d = c - q;
					float dist = simd_length(d);
					if (dist >= r)
						continue ;
					simd_float3 fn = simd_normalize(simd_cross(v3(tp + 3)
						- v3(tp), v3(tp + 6) - v3(tp)));
					/*
					** Si el centro ya esta detras de la cara (ha atravesado la
					** superficie), se empuja hacia fuera segun la normal de la
					** cara; empujar desde el punto mas cercano lo hundiria mas.
					*/
					if (simd_dot(c - v3(tp), fn) < 0)
						add_contact(w, w->owner[s], -1, q, fn, r + dist);
					else
						add_contact(w, w->owner[s], -1, q, dist > 1e-6f
							? d / dist : fn, r - dist);
				}
			}
}

/*
** Pares de esferas cercanas de cuerpos distintos con una tabla hash
** espacial (celda = diametro de la esfera mas grande).
*/

static unsigned	hcell(int x, int y, int z, unsigned size)
{
	return (((unsigned)x * 73856093u ^ (unsigned)y * 19349663u
		^ (unsigned)z * 83492791u) % size);
}

static void		body_contacts(t_world *w)
{
	float		maxr = 0;
	unsigned	size = (unsigned)w->nspheres * 2 + 1;
	int			*head = malloc(sizeof(int) * size);
	int			*next = malloc(sizeof(int) * w->nspheres);

	for (int i = 0; i < w->nspheres; i++)
		maxr = fmaxf(maxr, w->local[i].r);
	float cell = maxr * 2;
	memset(head, -1, sizeof(int) * size);
	for (int i = 0; i < w->nspheres; i++)
	{
		if (!w->bodies[w->owner[i]].active)
			continue ;
		simd_float3 c = w->wpos[i] / cell;
		unsigned h = hcell((int)floorf(c.x), (int)floorf(c.y),
			(int)floorf(c.z), size);
		next[i] = head[h];
		head[h] = i;
	}
	for (int i = 0; i < w->nspheres; i++)
	{
		int bi = w->owner[i];
		if (!w->bodies[bi].active)
			continue ;
		simd_float3 c = w->wpos[i] / cell;
		int cx = (int)floorf(c.x), cy = (int)floorf(c.y), cz = (int)floorf(c.z);
		for (int dz = -1; dz <= 1; dz++)
			for (int dy = -1; dy <= 1; dy++)
				for (int dx = -1; dx <= 1; dx++)
					for (int j = head[hcell(cx + dx, cy + dy, cz + dz, size)];
						j >= 0; j = next[j])
					{
						int bj = w->owner[j];
						if (j <= i || bj == bi)
							continue ;
						if (w->bodies[bi].sleeping && w->bodies[bj].sleeping)
							continue ;
						simd_float3 d = w->wpos[i] - w->wpos[j];
						float rr = w->local[i].r + w->local[j].r;
						float dist2 = simd_dot(d, d);
						if (dist2 >= rr * rr)
							continue ;
						float dist = sqrtf(dist2);
						simd_float3 n = dist > 1e-6f ? d / dist
							: simd_make_float3(0, 1, 0);
						add_contact(w, bi, bj, w->wpos[j] + n * w->local[j].r,
							n, rr - dist);
					}
	}
	free(head);
	free(next);
}

static simd_float3x3	world_inv_inertia(const t_body *b)
{
	simd_float3x3 r = simd_matrix3x3(b->rot);

	return (simd_mul(simd_mul(r, b->inv_inertia), simd_transpose(r)));
}

static simd_float3	point_vel(const t_body *b, simd_float3 r)
{
	return (b->vel + simd_cross(b->ang, r));
}

static void		apply(t_body *b, simd_float3x3 ii, simd_float3 r, simd_float3 p)
{
	b->vel += p * b->inv_mass;
	b->ang += simd_mul(ii, simd_cross(r, p));
}

/*
** Impulsos secuenciales sobre todos los contactos (Catto, "Iterative
** Dynamics with Temporal Coherence").
*/

static void		solve(t_world *w, float dt)
{
	static t_body	still;
	simd_float3x3	*iia = malloc(sizeof(simd_float3x3) * (w->ncontacts + 1));
	simd_float3x3	*iib = malloc(sizeof(simd_float3x3) * (w->ncontacts + 1));

	memset(&still, 0, sizeof(still));
	for (int i = 0; i < w->ncontacts; i++)
	{
		t_contact *c = &w->contacts[i];
		t_body *a = &w->bodies[c->a];
		t_body *b = c->b >= 0 ? &w->bodies[c->b] : &still;
		if (c->b >= 0 && b->sleeping && !a->sleeping)
			body_wake(b);
		if (a->sleeping && c->b >= 0 && !b->sleeping)
			body_wake(a);
		iia[i] = world_inv_inertia(a);
		iib[i] = c->b >= 0 ? world_inv_inertia(b) : (simd_float3x3){0};
		simd_float3 ra = c->p - a->pos, rb = c->p - b->pos;
		simd_float3 ca = simd_cross(simd_mul(iia[i], simd_cross(ra, c->n)), ra);
		simd_float3 cb = simd_cross(simd_mul(iib[i], simd_cross(rb, c->n)), rb);
		float k = a->inv_mass + (c->b >= 0 ? b->inv_mass : 0)
			+ simd_dot(c->n, ca + cb);
		c->mass_n = k > 0 ? 1 / k : 0;
		float vn = simd_dot(point_vel(a, ra) - (c->b >= 0
			? point_vel(b, rb) : simd_make_float3(0, 0, 0)), c->n);
		float e = fmaxf(a->restitution, c->b >= 0 ? b->restitution : 0);
		c->bias = (vn < -0.8f ? -e * vn : 0)
			+ BAUMGARTE / dt * fmaxf(c->depth - SLOP, 0);
	}
	for (int it = 0; it < w->iterations; it++)
		for (int i = 0; i < w->ncontacts; i++)
		{
			t_contact *c = &w->contacts[i];
			t_body *a = &w->bodies[c->a];
			t_body *b = c->b >= 0 ? &w->bodies[c->b] : &still;
			if (a->sleeping && (c->b < 0 || b->sleeping))
				continue ;
			simd_float3 ra = c->p - a->pos, rb = c->p - b->pos;
			simd_float3 vr = point_vel(a, ra) - (c->b >= 0
				? point_vel(b, rb) : simd_make_float3(0, 0, 0));
			float vn = simd_dot(vr, c->n);
			float jn = c->mass_n * (-vn + c->bias);
			float old = c->jn;
			c->jn = fmaxf(old + jn, 0);
			jn = c->jn - old;
			simd_float3 p = c->n * jn;
			apply(a, iia[i], ra, p);
			if (c->b >= 0)
				apply(b, iib[i], rb, -p);
			/*
			** Rozamiento: contra la velocidad tangencial, como mucho
			** coeficiente * impulso normal acumulado.
			*/
			vr = point_vel(a, ra) - (c->b >= 0 ? point_vel(b, rb) : simd_make_float3(0, 0, 0));
			simd_float3 vt = vr - c->n * simd_dot(vr, c->n);
			float lt = simd_length(vt);
			if (lt < 1e-6f)
				continue ;
			simd_float3 t = vt / lt;
			simd_float3 ca = simd_cross(simd_mul(iia[i], simd_cross(ra, t)), ra);
			simd_float3 cb = simd_cross(simd_mul(iib[i], simd_cross(rb, t)), rb);
			float kt = a->inv_mass + (c->b >= 0 ? b->inv_mass : 0)
				+ simd_dot(t, ca + cb);
			float mu = sqrtf(a->friction * (c->b >= 0 ? b->friction : 0.6f));
			float jt = fminf(lt / kt, mu * c->jn);
			apply(a, iia[i], ra, -t * jt);
			if (c->b >= 0)
				apply(b, iib[i], rb, t * jt);
		}
	free(iia);
	free(iib);
}

void			world_step(t_world *w, float dt)
{
	float h = dt / w->substeps;

	if (dt <= 0)
		return ;
	for (int s = 0; s < w->substeps; s++)
	{
		for (int i = 0; i < w->nbodies; i++)
		{
			t_body *b = &w->bodies[i];
			if (!b->active || b->sleeping || b->inv_mass == 0)
				continue ;
			b->vel += w->gravity * h;
			b->vel *= 1 - 0.02f * h;
			b->ang *= 1 - 0.3f * h;
		}
		for (int i = 0; i < w->nspheres; i++)
			w->wpos[i] = body_point(&w->bodies[w->owner[i]],
				w->bodies[w->owner[i]].rest_com + w->local[i].c);
		w->ncontacts = 0;
		for (int i = 0; i < w->nbodies; i++)
			w->bodies[i].touching = 0;
		for (int i = 0; i < w->nspheres; i++)
		{
			t_body *b = &w->bodies[w->owner[i]];
			if (b->active && !b->sleeping)
				static_contacts(w, i);
		}
		body_contacts(w);
		for (int i = 0; i < w->ncontacts; i++)
		{
			w->bodies[w->contacts[i].a].touching = 1;
			if (w->contacts[i].b >= 0)
				w->bodies[w->contacts[i].b].touching = 1;
		}
		solve(w, h);
		for (int i = 0; i < w->nbodies; i++)
		{
			t_body *b = &w->bodies[i];
			if (!b->active || b->sleeping || b->inv_mass == 0)
				continue ;
			/*
			** Resistencia a la rodadura: un cuerpo apoyado pierde giro y
			** velocidad poco a poco (la fruta se deforma un poco contra el
			** suelo); sin esto rodaria para siempre.
			*/
			if (b->touching)
			{
				b->ang *= 1 - ROLLING * h;
				b->vel *= 1 - ROLLING * 0.4f * h;
			}
			b->pos += b->vel * h;
			simd_quatf dq = simd_quaternion(b->ang.x * 0.5f * h,
				b->ang.y * 0.5f * h, b->ang.z * 0.5f * h, 0.0f);
			b->rot = simd_normalize(simd_add(b->rot, simd_mul(dq, b->rot)));
			float speed = simd_length(b->vel) + simd_length(b->ang) * 0.1f;
			b->idle = speed < SLEEP_SPEED ? b->idle + h : 0;
			if (b->idle > SLEEP_TIME)
			{
				b->sleeping = 1;
				b->vel = 0;
				b->ang = 0;
			}
		}
	}
}

/*
** Penetracion maxima de un cuerpo contra el entorno estatico y los demas
** cuerpos (sin resolver nada).
*/

static float	body_overlap(t_world *w, int bi)
{
	t_body	*b = &w->bodies[bi];
	float	worst = 0;
	int		saved = w->ncontacts;

	for (int i = b->first; i < b->first + b->count; i++)
		w->wpos[i] = body_point(b, b->rest_com + w->local[i].c);
	for (int i = b->first; i < b->first + b->count; i++)
	{
		int before = w->ncontacts;
		static_contacts(w, i);
		for (int k = before; k < w->ncontacts; k++)
			worst = fmaxf(worst, w->contacts[k].depth);
		w->ncontacts = before;
		for (int j = 0; j < w->nspheres; j++)
		{
			int bj = w->owner[j];
			if (bj == bi || bj > bi)
				continue ;
			float rr = w->local[i].r + w->local[j].r;
			float d = simd_distance(w->wpos[i], w->wpos[j]);
			if (d < rr)
				worst = fmaxf(worst, rr - d);
		}
	}
	w->ncontacts = saved;
	return (worst);
}

/*
** Sube cada cuerpo lo justo para que no empiece metido en la mesa ni en los
** cuerpos colocados antes (si no, saltaria al primer paso).
*/

void			world_settle(t_world *w)
{
	for (int i = 0; i < w->nspheres; i++)
		w->wpos[i] = body_point(&w->bodies[w->owner[i]],
			w->bodies[w->owner[i]].rest_com + w->local[i].c);
	for (int bi = 0; bi < w->nbodies; bi++)
		for (int it = 0; it < 60; it++)
		{
			float d = body_overlap(w, bi);
			if (d < 0.0005f)
				break ;
			w->bodies[bi].pos.y += d + 0.0005f;
		}
}

/*
** Cuantos triangulos corta un rayo que sale de p en la direccion del eje
** "axis" (Moller-Trumbore). Impar = p esta dentro de la malla.
*/

static int		ray_hits(const float *tris, int ntris, simd_float3 p, int axis)
{
	simd_float3	d = simd_make_float3(axis == 0, axis == 1, axis == 2);
	int			hits = 0;

	for (int t = 0; t < ntris; t++)
	{
		simd_float3 a = v3(tris + t * 9);
		simd_float3 e1 = v3(tris + t * 9 + 3) - a;
		simd_float3 e2 = v3(tris + t * 9 + 6) - a;
		simd_float3 pv = simd_cross(d, e2);
		float det = simd_dot(e1, pv);
		if (fabsf(det) < 1e-12f)
			continue ;
		simd_float3 tv = p - a;
		float u = simd_dot(tv, pv) / det;
		if (u < 0 || u > 1)
			continue ;
		simd_float3 qv = simd_cross(tv, e1);
		float v = simd_dot(d, qv) / det;
		if (v < 0 || u + v > 1)
			continue ;
		hits += simd_dot(e2, qv) / det > 0;
	}
	return (hits);
}

/*
** Grupo de esferas que aproxima una malla: esferas en los puntos de una
** rejilla que quedan dentro (voto de 3 rayos) y, para las partes finas
** (paredes de una copa, una tabla), esferas pegadas a la superficie.
*/

int				build_cluster(const float *tris, int ntris, int res,
					t_psphere **out, simd_float3 *com)
{
	simd_float3	lo = simd_make_float3(1e30f, 1e30f, 1e30f);
	simd_float3	hi = -lo;
	int			n = 0;
	int			cap = 256;
	t_psphere	*s = malloc(sizeof(t_psphere) * cap);

	for (int i = 0; i < ntris * 3; i++)
	{
		lo = simd_min(lo, v3(tris + i * 3));
		hi = simd_max(hi, v3(tris + i * 3));
	}
	simd_float3 ext = hi - lo;
	float h = fmaxf(fmaxf(ext.x, ext.y), ext.z) / res;
	int dim[3];
	for (int k = 0; k < 3; k++)
		dim[k] = (int)ceilf(ext[k] / h);
	for (int z = 0; z < dim[2]; z++)
		for (int y = 0; y < dim[1]; y++)
			for (int x = 0; x < dim[0]; x++)
			{
				simd_float3 p = lo + simd_make_float3(x + 0.5f, y + 0.5f,
					z + 0.5f) * h;
				int votes = (ray_hits(tris, ntris, p, 0) & 1)
					+ (ray_hits(tris, ntris, p, 1) & 1)
					+ (ray_hits(tris, ntris, p, 2) & 1);
				if (votes < 2)
					continue ;
				if (n == cap)
					s = realloc(s, sizeof(t_psphere) * (cap *= 2));
				s[n++] = (t_psphere){p, h * 0.6f};
			}
	/*
	** Superficie: una esfera por celda fina (h/2) que contenga muestras de
	** los triangulos, colocada en la media de esas muestras.
	*/
	float hs = h * 0.5f;
	int sd[3];
	for (int k = 0; k < 3; k++)
		sd[k] = (int)ceilf(ext[k] / hs) + 1;
	int ncell = sd[0] * sd[1] * sd[2];
	simd_float3 *acc = calloc(ncell, sizeof(simd_float3));
	simd_float3 *nrm = calloc(ncell, sizeof(simd_float3));
	int *cnt = calloc(ncell, sizeof(int));
	for (int t = 0; t < ntris; t++)
		for (int k = 0; k < 4; k++)
		{
			simd_float3 a = v3(tris + t * 9), b = v3(tris + t * 9 + 3);
			simd_float3 c = v3(tris + t * 9 + 6);
			simd_float3 fn = simd_cross(b - a, c - a);
			simd_float3 p = k == 3 ? (a + b + c) / 3 : (k == 0 ? a : k == 1
				? b : c);
			int ix = (int)((p.x - lo.x) / hs), iy = (int)((p.y - lo.y) / hs);
			int iz = (int)((p.z - lo.z) / hs);
			int id = (iz * sd[1] + iy) * sd[0] + ix;
			if (id < 0 || id >= ncell)
				continue ;
			acc[id] += p;
			nrm[id] += fn;
			cnt[id]++;
		}
	/*
	** La esfera se mete hacia dentro su propio radio (segun la normal media
	** de la celda): asi su borde coincide con la superficie de la malla y el
	** objeto no flota sobre la mesa.
	*/
	float rs = hs * 0.55f;
	for (int i = 0; i < ncell; i++)
	{
		if (!cnt[i])
			continue ;
		if (n == cap)
			s = realloc(s, sizeof(t_psphere) * (cap *= 2));
		simd_float3 c = acc[i] / cnt[i];
		if (simd_length(nrm[i]) > 1e-12f)
			c -= simd_normalize(nrm[i]) * rs;
		s[n++] = (t_psphere){c, rs};
	}
	free(acc);
	free(nrm);
	free(cnt);
	simd_float3 sum = 0;
	float vol = 0;
	for (int i = 0; i < n; i++)
	{
		float v = s[i].r * s[i].r * s[i].r;
		sum += s[i].c * v;
		vol += v;
	}
	*com = vol > 0 ? sum / vol : (lo + hi) * 0.5f;
	*out = s;
	return (n);
}

/*
** Una sola esfera (fruta): centro = centroide de los vertices, radio =
** distancia media a ellos. Rueda como una fruta de verdad.
*/

int				fit_sphere(const float *tris, int ntris, t_psphere **out,
					simd_float3 *com)
{
	simd_float3	c = 0;
	float		r = 0;

	for (int i = 0; i < ntris * 3; i++)
		c += v3(tris + i * 3);
	c /= (float)(ntris * 3);
	for (int i = 0; i < ntris * 3; i++)
		r += simd_distance(c, v3(tris + i * 3));
	r /= (float)(ntris * 3);
	*out = malloc(sizeof(t_psphere));
	(*out)[0] = (t_psphere){c, r};
	*com = c;
	return (1);
}
