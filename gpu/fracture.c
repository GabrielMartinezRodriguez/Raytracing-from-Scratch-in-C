/*
** Fractura de Voronoi de un objeto hueco (jarron), desde cero.
**
** Cada triangulo se recorta exactamente con los planos que separan las
** celdas (mediatrices entre semillas), asi los bordes de cada trozo son
** rectos y no en dientes de sierra. Donde un plano corta la pared se
** construye la cara de rotura: una tira que une la pared exterior con la
** interior, con el material de ceramica sin esmaltar.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "fracture.h"

#define MAXV 32
#define MAX_THICK 0.015f

typedef struct	s_pv
{
	simd_float3	p;
	simd_float2	uv;
	simd_float3	n;
}				t_pv;

static simd_float3	unpack(unsigned packed)
{
	if (packed == 0)
		return (0);
	float x = (short)(packed & 0xffff) / 32767.0f;
	float y = (short)(packed >> 16) / 32767.0f;
	simd_float3 n = simd_make_float3(x, y, 1 - fabsf(x) - fabsf(y));
	if (n.z < 0)
	{
		float ox = n.x;
		n.x = (1 - fabsf(n.y)) * (ox >= 0 ? 1 : -1);
		n.y = (1 - fabsf(ox)) * (n.y >= 0 ? 1 : -1);
	}
	return (simd_normalize(n));
}

static t_pv	lerp_pv(t_pv a, t_pv b, float t)
{
	t_pv	r;

	r.p = a.p + (b.p - a.p) * t;
	r.uv = a.uv + (b.uv - a.uv) * t;
	r.n = a.n + (b.n - a.n) * t;
	return (r);
}

/*
** Sutherland-Hodgman: se queda con la parte del poligono donde
** dot(p - o, n) <= 0.
*/

static int	clip(t_pv *in, int n, simd_float3 o, simd_float3 nrm, t_pv *out)
{
	int	m = 0;

	for (int i = 0; i < n; i++)
	{
		t_pv a = in[i];
		t_pv b = in[(i + 1) % n];
		float da = simd_dot(a.p - o, nrm);
		float db = simd_dot(b.p - o, nrm);
		if (da <= 0 && m < MAXV)
			out[m++] = a;
		if ((da < 0 && db > 0) || (da > 0 && db < 0))
			if (m < MAXV)
				out[m++] = lerp_pv(a, b, da / (da - db));
	}
	return (m);
}

typedef struct	s_out
{
	float		*pos;
	t_gpu_tri	*tris;
	uint32_t	*obj;
	size_t		n;
	size_t		cap;
}				t_out;

static void	push(t_out *o, simd_float3 *p, simd_float2 *uv, unsigned *n,
				unsigned material, uint32_t obj)
{
	if (o->n == o->cap)
	{
		o->cap = o->cap ? o->cap * 2 : 4096;
		o->pos = realloc(o->pos, sizeof(float) * 9 * o->cap);
		o->tris = realloc(o->tris, sizeof(t_gpu_tri) * o->cap);
		o->obj = realloc(o->obj, sizeof(uint32_t) * o->cap);
	}
	for (int k = 0; k < 3; k++)
	{
		o->pos[o->n * 9 + k * 3] = p[k].x;
		o->pos[o->n * 9 + k * 3 + 1] = p[k].y;
		o->pos[o->n * 9 + k * 3 + 2] = p[k].z;
		o->tris[o->n].uv[k * 2] = uv[k].x;
		o->tris[o->n].uv[k * 2 + 1] = uv[k].y;
		o->tris[o->n].n[k] = n[k];
	}
	o->tris[o->n].material = material;
	o->obj[o->n++] = obj;
}

/*
** Distancia hasta la otra cara de la pared: rayo desde p hacia dentro del
** material (dir) contra los triangulos originales del objeto.
*/

static float	wall_depth(const float *tris, int ntris, simd_float3 p,
					simd_float3 dir)
{
	float	best = MAX_THICK;

	for (int i = 0; i < ntris; i++)
	{
		const float *v = tris + i * 9;
		simd_float3 a = simd_make_float3(v[0], v[1], v[2]);
		simd_float3 e1 = simd_make_float3(v[3], v[4], v[5]) - a;
		simd_float3 e2 = simd_make_float3(v[6], v[7], v[8]) - a;
		simd_float3 pv = simd_cross(dir, e2);
		float det = simd_dot(e1, pv);
		if (fabsf(det) < 1e-12f)
			continue ;
		simd_float3 tv = p - a;
		float u = simd_dot(tv, pv) / det;
		if (u < 0 || u > 1)
			continue ;
		simd_float3 qv = simd_cross(tv, e1);
		float w = simd_dot(dir, qv) / det;
		if (w < 0 || u + w > 1)
			continue ;
		float t = simd_dot(e2, qv) / det;
		if (t > 2e-4f && t < best)
			best = t;
	}
	return (best);
}

/*
** Cara de rotura bajo el borde (a, b), que esta en el plano de corte de
** normal pn (hacia fuera del trozo). La tira baja hacia dentro de la pared,
** por dentro del plano, hasta la otra cara.
*/

static void	crack_strip(t_out *o, const float *orig, int norig, t_pv a,
				t_pv b, simd_float3 ng, simd_float3 pn, unsigned mat,
				uint32_t obj)
{
	simd_float3 m = ng - pn * simd_dot(ng, pn);
	if (simd_length(m) < 0.2f)
		return ;
	m = simd_normalize(m);
	float ta = wall_depth(orig, norig, a.p, -m);
	float tb = wall_depth(orig, norig, b.p, -m);
	if (ta >= MAX_THICK || tb >= MAX_THICK)
		return ;
	simd_float3 a2 = a.p - m * ta;
	simd_float3 b2 = b.p - m * tb;
	unsigned pk = pack_normal(pn);
	unsigned nn[3] = {pk, pk, pk};
	simd_float2 uv[3] = {a.uv, b.uv, b.uv};
	simd_float3 t1[3] = {a.p, b.p, b2};
	simd_float3 t2[3] = {a.p, b2, a2};
	if (simd_dot(simd_cross(b.p - a.p, b2 - a.p), pn) < 0)
	{
		t1[1] = b2;
		t1[2] = b.p;
		t2[1] = a2;
		t2[2] = b2;
	}
	push(o, t1, uv, nn, mat, obj);
	uv[1] = a.uv;
	push(o, t2, uv, nn, mat, obj);
}

int			fracture_object(t_gpu_mesh *m, int obj, const simd_float3 *seeds,
				int nseeds, const int *shard_obj, unsigned crack_mat)
{
	t_out	o;
	int		norig = 0;
	float	*orig;
	int		cracks = 0;

	memset(&o, 0, sizeof(o));
	for (size_t i = 0; i < m->ntris; i++)
		norig += m->tri_obj[i] == (uint32_t)obj;
	orig = malloc(sizeof(float) * 9 * (norig + 1));
	norig = 0;
	for (size_t i = 0; i < m->ntris; i++)
		if (m->tri_obj[i] == (uint32_t)obj)
			memcpy(orig + 9 * norig++, m->pos + 9 * i, sizeof(float) * 9);
	for (size_t i = 0; i < m->ntris; i++)
	{
		if (m->tri_obj[i] != (uint32_t)obj)
		{
			simd_float3 p[3];
			simd_float2 uv[3];
			for (int k = 0; k < 3; k++)
			{
				p[k] = simd_make_float3(m->pos[i * 9 + k * 3],
					m->pos[i * 9 + k * 3 + 1], m->pos[i * 9 + k * 3 + 2]);
				uv[k] = simd_make_float2(m->tris[i].uv[k * 2],
					m->tris[i].uv[k * 2 + 1]);
			}
			push(&o, p, uv, m->tris[i].n, m->tris[i].material, m->tri_obj[i]);
			continue ;
		}
		t_pv tri[3];
		int has_n = m->tris[i].n[0] != 0;
		for (int k = 0; k < 3; k++)
		{
			tri[k].p = simd_make_float3(m->pos[i * 9 + k * 3],
				m->pos[i * 9 + k * 3 + 1], m->pos[i * 9 + k * 3 + 2]);
			tri[k].uv = simd_make_float2(m->tris[i].uv[k * 2],
				m->tris[i].uv[k * 2 + 1]);
			tri[k].n = unpack(m->tris[i].n[k]);
		}
		simd_float3 ng = simd_normalize(simd_cross(tri[1].p - tri[0].p,
			tri[2].p - tri[0].p));
		for (int c = 0; c < nseeds; c++)
		{
			t_pv buf[2][MAXV];
			int n = 3;
			int cur = 0;
			memcpy(buf[0], tri, sizeof(tri));
			for (int j = 0; j < nseeds && n > 0; j++)
			{
				if (j == c)
					continue ;
				simd_float3 pn = simd_normalize(seeds[j] - seeds[c]);
				simd_float3 mid = (seeds[j] + seeds[c]) * 0.5f;
				n = clip(buf[cur], n, mid, pn, buf[1 - cur]);
				cur = 1 - cur;
			}
			if (n < 3)
				continue ;
			t_pv *poly = buf[cur];
			for (int k = 1; k + 1 < n; k++)
			{
				simd_float3 p[3] = {poly[0].p, poly[k].p, poly[k + 1].p};
				simd_float2 uv[3] = {poly[0].uv, poly[k].uv, poly[k + 1].uv};
				unsigned nn[3] = {0, 0, 0};
				if (has_n)
				{
					nn[0] = pack_normal(poly[0].n);
					nn[1] = pack_normal(poly[k].n);
					nn[2] = pack_normal(poly[k + 1].n);
				}
				push(&o, p, uv, nn, m->tris[i].material, shard_obj[c]);
			}
			/*
			** Bordes del poligono que estan sobre un plano de corte: ahi
			** va la cara de rotura.
			*/
			for (int k = 0; k < n; k++)
			{
				t_pv a = poly[k];
				t_pv b = poly[(k + 1) % n];
				for (int j = 0; j < nseeds; j++)
				{
					if (j == c)
						continue ;
					simd_float3 pn = simd_normalize(seeds[j] - seeds[c]);
					simd_float3 mid = (seeds[j] + seeds[c]) * 0.5f;
					if (fabsf(simd_dot(a.p - mid, pn)) < 2e-6f
						&& fabsf(simd_dot(b.p - mid, pn)) < 2e-6f
						&& simd_distance(a.p, b.p) > 1e-7f)
					{
						crack_strip(&o, orig, norig, a, b, ng, pn, crack_mat,
							shard_obj[c]);
						cracks++;
						break ;
					}
				}
			}
		}
	}
	free(orig);
	free(m->pos);
	free(m->tris);
	free(m->tri_obj);
	m->pos = o.pos;
	m->tris = o.tris;
	m->tri_obj = o.obj;
	m->ntris = o.n;
	m->cap = o.cap;
	return (cracks);
}
