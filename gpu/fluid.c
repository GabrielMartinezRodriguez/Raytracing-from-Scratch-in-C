/*
** Position Based Fluids + reconstruccion de superficie, desde cero.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "fluid.h"
#include "obj_loader.h"

#define MAX_NBR 64
#define ITERS 3
#define SUBSTEPS 4
#define XSPH 0.02f
#define COHESION 10.0f
#define WALL_FRICTION 0.15f
#define SURF_SCALE 1.5f
#define SURF_ISO 0.3f

static float	poly6(float r2, float h)
{
	float	h2 = h * h;

	if (r2 >= h2)
		return (0);
	float d = h2 - r2;
	return (315.0f / (64.0f * (float)M_PI * powf(h, 9)) * d * d * d);
}

static simd_float3	spiky_grad(simd_float3 r, float h)
{
	float	len = simd_length(r);

	if (len >= h || len < 1e-9f)
		return (0);
	float k = -45.0f / ((float)M_PI * powf(h, 6)) * (h - len) * (h - len);
	return (r / len * k);
}

/*
** Densidad de reposo y constante de relajacion a partir de una particula
** rodeada de vecinas en una red cubica de separacion d0.
*/

void			fluid_init(t_fluid *f, float spacing, int cap)
{
	memset(f, 0, sizeof(*f));
	f->d0 = spacing;
	f->h = spacing * 2.0f;
	f->cap = cap;
	f->x = malloc(sizeof(simd_float3) * cap);
	f->p = malloc(sizeof(simd_float3) * cap);
	f->v = malloc(sizeof(simd_float3) * cap);
	f->dp = malloc(sizeof(simd_float3) * cap);
	f->hitn = malloc(sizeof(simd_float3) * cap);
	f->lambda = malloc(sizeof(float) * cap);
	f->nbr = malloc(sizeof(int) * cap * MAX_NBR);
	f->nnbr = malloc(sizeof(int) * cap);
	float rho = 0;
	simd_float3 gsum = 0;
	float g2 = 0;
	for (int z = -3; z <= 3; z++)
		for (int y = -3; y <= 3; y++)
			for (int x = -3; x <= 3; x++)
			{
				simd_float3 r = simd_make_float3(x, y, z) * spacing;
				rho += poly6(simd_dot(r, r), f->h);
				simd_float3 g = spiky_grad(r, f->h);
				gsum += g;
				g2 += simd_dot(g, g);
			}
	f->rho0 = rho;
	f->eps = 0.01f * (g2 / (rho * rho));
}

/*
** Chorro: cada vez que el liquido ha avanzado una separacion d0, se anade
** una capa de particulas en un disco (rejilla con un poco de ruido).
*/

static void		emit(t_fluid *f, float dt)
{
	static unsigned	seed = 99;

	if (f->time > f->emit_until)
		return ;
	f->emit_acc += simd_length(f->emit_vel) * dt;
	while (f->emit_acc >= f->d0)
	{
		f->emit_acc -= f->d0;
		int r = (int)(f->emit_radius / f->d0);
		for (int j = -r; j <= r; j++)
			for (int i = -r; i <= r; i++)
			{
				if ((i * i + j * j) * f->d0 * f->d0 > f->emit_radius
					* f->emit_radius || f->n >= f->cap)
					continue ;
				seed = seed * 1664525u + 1013904223u;
				float jit = ((seed >> 8) / 16777216.0f - 0.5f) * 0.1f * f->d0;
				f->x[f->n] = f->emit_pos + simd_make_float3(i * f->d0 + jit, 0,
					j * f->d0 - jit);
				f->v[f->n] = f->emit_vel;
				f->n++;
			}
	}
}

static unsigned	hcell(int x, int y, int z, unsigned size)
{
	return (((unsigned)x * 73856093u ^ (unsigned)y * 19349663u
		^ (unsigned)z * 83492791u) % size);
}

static void		neighbors(t_fluid *f)
{
	unsigned	size = (unsigned)f->n * 2 + 1;
	int			*head = malloc(sizeof(int) * size);
	int			*next = malloc(sizeof(int) * (f->n + 1));
	float		h2 = f->h * f->h;

	memset(head, -1, sizeof(int) * size);
	for (int i = 0; i < f->n; i++)
	{
		simd_float3 c = f->p[i] / f->h;
		unsigned k = hcell((int)floorf(c.x), (int)floorf(c.y), (int)floorf(c.z),
			size);
		next[i] = head[k];
		head[k] = i;
	}
	for (int i = 0; i < f->n; i++)
	{
		simd_float3 c = f->p[i] / f->h;
		int cx = (int)floorf(c.x), cy = (int)floorf(c.y), cz = (int)floorf(c.z);
		int cnt = 0;
		for (int dz = -1; dz <= 1; dz++)
			for (int dy = -1; dy <= 1; dy++)
				for (int dx = -1; dx <= 1; dx++)
					for (int j = head[hcell(cx + dx, cy + dy, cz + dz, size)];
						j >= 0 && cnt < MAX_NBR; j = next[j])
						if (j != i && simd_distance_squared(f->p[i], f->p[j]) < h2)
							f->nbr[i * MAX_NBR + cnt++] = j;
		f->nnbr[i] = cnt;
	}
	free(head);
	free(next);
}

void			fluid_step(t_fluid *f, t_world *w, float dt)
{
	float	h = dt / SUBSTEPS;

	for (int st = 0; st < SUBSTEPS; st++)
	{
		emit(f, h);
		for (int i = 0; i < f->n; i++)
		{
			f->v[i] += w->gravity * h;
			f->p[i] = f->x[i] + f->v[i] * h;
			/*
			** Colision barrida: si el camino del paso corta una superficie,
			** la particula se queda justo delante (el agua rapida no
			** atraviesa el tablero de la mesa).
			*/
			float tt;
			simd_float3 nrm;
			if (world_segment(w, f->x[i], f->p[i], &tt, &nrm))
			{
				f->p[i] = f->x[i] + (f->p[i] - f->x[i]) * tt + nrm * f->d0 * 0.5f;
				float vn = simd_dot(f->v[i], nrm);
				if (vn < 0)
					f->v[i] -= nrm * vn;
			}
		}
		neighbors(f);
		for (int it = 0; it < ITERS; it++)
		{
			/*
			** Restriccion de densidad C = rho / rho0 - 1 (solo cuando hay
			** compresion, para no apelotonar la superficie) y su
			** multiplicador lambda.
			*/
			for (int i = 0; i < f->n; i++)
			{
				float rho = poly6(0, f->h);
				simd_float3 gi = 0;
				float sum = 0;
				for (int k = 0; k < f->nnbr[i]; k++)
				{
					int j = f->nbr[i * MAX_NBR + k];
					simd_float3 r = f->p[i] - f->p[j];
					rho += poly6(simd_dot(r, r), f->h);
					simd_float3 g = spiky_grad(r, f->h) / f->rho0;
					gi += g;
					sum += simd_dot(g, g);
				}
				float c = fmaxf(rho / f->rho0 - 1, 0);
				f->lambda[i] = -c / (sum + simd_dot(gi, gi) + f->eps);
			}
			for (int i = 0; i < f->n; i++)
			{
				simd_float3 d = 0;
				for (int k = 0; k < f->nnbr[i]; k++)
				{
					int j = f->nbr[i * MAX_NBR + k];
					d += (f->lambda[i] + f->lambda[j])
						* spiky_grad(f->p[i] - f->p[j], f->h);
				}
				f->dp[i] = d / f->rho0;
			}
			for (int i = 0; i < f->n; i++)
			{
				f->p[i] += f->dp[i];
				simd_float3 nrm;
				f->hitn[i] = 0;
				if (world_push_point(w, &f->p[i], f->d0 * 0.5f, &nrm) > 0)
					f->hitn[i] = nrm;
			}
		}
		for (int i = 0; i < f->n; i++)
		{
			f->v[i] = (f->p[i] - f->x[i]) / h;
			float vn = simd_dot(f->v[i], f->hitn[i]);
			if (vn > 0)
				f->v[i] -= f->hitn[i] * vn;
			/*
			** Rozamiento con la mesa: el agua "moja" la madera y no
			** resbala como si fuera hielo.
			*/
			if (simd_length_squared(f->hitn[i]) > 0)
			{
				simd_float3 vt = f->v[i] - f->hitn[i] * simd_dot(f->v[i],
					f->hitn[i]);
				f->v[i] -= vt * WALL_FRICTION;
			}
		}
		/*
		** Viscosidad XSPH: cada particula se acerca a la velocidad media de
		** sus vecinas (el agua se mueve "en bloque").
		*/
		for (int i = 0; i < f->n; i++)
		{
			simd_float3 dv = 0;
			for (int k = 0; k < f->nnbr[i]; k++)
			{
				int j = f->nbr[i * MAX_NBR + k];
				dv += (f->v[j] - f->v[i]) * (poly6(simd_distance_squared(
					f->p[i], f->p[j]), f->h) / f->rho0);
			}
			/*
			** Cohesion (tension superficial, al estilo Akinci): las vecinas
			** se atraen un poco a media distancia, asi el agua forma
			** charcos y chorros continuos en vez de gotitas sueltas.
			*/
			simd_float3 coh = 0;
			for (int k = 0; k < f->nnbr[i]; k++)
			{
				int j = f->nbr[i * MAX_NBR + k];
				simd_float3 r = f->p[j] - f->p[i];
				float len = simd_length(r);
				if (len < 1e-9f)
					continue ;
				float q = len / f->h;
				coh += r / len * (q * (1 - q) * (1 - q));
			}
			f->dp[i] = dv * XSPH + coh * (COHESION * h);
		}
		for (int i = 0; i < f->n; i++)
		{
			f->v[i] += f->dp[i];
			f->x[i] = f->p[i];
		}
		f->time += h;
	}
}

/*
** Superficie: campo de densidad normalizado (1 = agua en reposo) en una
** rejilla y la isosuperficie 0,5 con marching tetrahedra: cada cubo se parte
** en 6 tetraedros que comparten la diagonal 0-7, y cada tetraedro da 0, 1 o
** 2 triangulos segun que esquinas quedan dentro. No hace falta ninguna tabla.
*/

/*
** Densidad de una particula rodeada de agua en reposo con el nucleo de la
** superficie (para que el campo valga 1 dentro del agua).
*/

static float	surf_rest(float d0, float hs)
{
	float	rho = 0;
	int		n = (int)ceilf(hs / d0);

	for (int z = -n; z <= n; z++)
		for (int y = -n; y <= n; y++)
			for (int x = -n; x <= n; x++)
				rho += poly6(d0 * d0 * (x * x + y * y + z * z), hs);
	return (rho);
}

static const int	g_tets[6][4] = {{0, 1, 3, 7}, {0, 1, 5, 7}, {0, 2, 3, 7},
	{0, 2, 6, 7}, {0, 4, 5, 7}, {0, 4, 6, 7}};

typedef struct	s_grid
{
	simd_float3	lo;
	float		c;
	int			nx;
	int			ny;
	int			nz;
	float		*phi;
}				t_grid;

static float	gat(t_grid *g, int x, int y, int z)
{
	x = x < 0 ? 0 : x >= g->nx ? g->nx - 1 : x;
	y = y < 0 ? 0 : y >= g->ny ? g->ny - 1 : y;
	z = z < 0 ? 0 : z >= g->nz ? g->nz - 1 : z;
	return (g->phi[(z * g->ny + y) * g->nx + x]);
}

static simd_float3	grad_at(t_grid *g, int x, int y, int z)
{
	return (simd_make_float3(gat(g, x + 1, y, z) - gat(g, x - 1, y, z),
		gat(g, x, y + 1, z) - gat(g, x, y - 1, z),
		gat(g, x, y, z + 1) - gat(g, x, y, z - 1)));
}

static void		emit_tri(t_fluid *f, simd_float3 *v, simd_float3 *n,
					simd_float3 out)
{
	if (f->ntri == f->tri_cap)
		return ;
	simd_float3 fn = simd_cross(v[1] - v[0], v[2] - v[0]);
	int order[3] = {0, 1, 2};
	if (simd_dot(fn, out) < 0)
	{
		order[1] = 2;
		order[2] = 1;
	}
	for (int k = 0; k < 3; k++)
	{
		simd_float3 p = v[order[k]];
		f->tri[f->ntri * 9 + k * 3] = p.x;
		f->tri[f->ntri * 9 + k * 3 + 1] = p.y;
		f->tri[f->ntri * 9 + k * 3 + 2] = p.z;
		simd_float3 nn = n[order[k]];
		f->tri_n[f->ntri * 3 + k] = pack_normal(simd_length(nn) > 1e-12f
			? simd_normalize(nn) : out);
	}
	f->ntri++;
}

void			fluid_surface(t_fluid *f)
{
	t_grid	g;
	float	iso = SURF_ISO;

	f->ntri = 0;
	if (f->n == 0)
		return ;
	simd_float3 lo = f->x[0], hi = f->x[0];
	for (int i = 1; i < f->n; i++)
	{
		lo = simd_min(lo, f->x[i]);
		hi = simd_max(hi, f->x[i]);
	}
	g.c = f->d0 * 0.7f;
	g.lo = lo - f->h * SURF_SCALE * 1.5f;
	hi += f->h * SURF_SCALE * 1.5f;
	/*
	** Si el agua ocupa mucho (charcos en el suelo), la rejilla se hace mas
	** gruesa para no pasar de ~40 millones de celdas.
	*/
	simd_float3 ext = hi - g.lo;
	float cells = ext.x * ext.y * ext.z / (g.c * g.c * g.c);
	if (cells > 4e7f)
		g.c *= cbrtf(cells / 4e7f);
	g.nx = (int)ceilf((hi.x - g.lo.x) / g.c) + 1;
	g.ny = (int)ceilf((hi.y - g.lo.y) / g.c) + 1;
	g.nz = (int)ceilf((hi.z - g.lo.z) / g.c) + 1;
	if ((long)g.nx * g.ny * g.nz > 60000000L)
		return ;
	g.phi = calloc((size_t)g.nx * g.ny * g.nz, sizeof(float));
	/*
	** Para la superficie se usa un nucleo mas ancho que en la simulacion:
	** una lamina fina de agua (una o dos particulas de grosor) sigue dando
	** una capa continua en vez de gotas sueltas.
	*/
	float hs = f->h * SURF_SCALE;
	float rho_s = surf_rest(f->d0, hs);
	int rr = (int)ceilf(hs / g.c);
	for (int i = 0; i < f->n; i++)
	{
		simd_float3 q = (f->x[i] - g.lo) / g.c;
		int cx = (int)q.x, cy = (int)q.y, cz = (int)q.z;
		for (int z = cz - rr; z <= cz + rr + 1; z++)
			for (int y = cy - rr; y <= cy + rr + 1; y++)
				for (int x = cx - rr; x <= cx + rr + 1; x++)
				{
					if (x < 0 || y < 0 || z < 0 || x >= g.nx || y >= g.ny
						|| z >= g.nz)
						continue ;
					simd_float3 pos = g.lo + simd_make_float3(x, y, z) * g.c;
					float w = poly6(simd_distance_squared(pos, f->x[i]), hs);
					g.phi[(z * g.ny + y) * g.nx + x] += w / rho_s;
				}
	}
	for (int z = 0; z < g.nz - 1; z++)
		for (int y = 0; y < g.ny - 1; y++)
			for (int x = 0; x < g.nx - 1; x++)
			{
				float val[8];
				int any_in = 0, any_out = 0;
				for (int k = 0; k < 8; k++)
				{
					val[k] = gat(&g, x + (k & 1), y + ((k >> 1) & 1),
						z + ((k >> 2) & 1));
					any_in |= val[k] > iso;
					any_out |= val[k] <= iso;
				}
				if (!any_in || !any_out)
					continue ;
				for (int t = 0; t < 6; t++)
				{
					int in[4], out[4], ni = 0, no = 0;
					for (int k = 0; k < 4; k++)
					{
						int c = g_tets[t][k];
						if (val[c] > iso)
							in[ni++] = c;
						else
							out[no++] = c;
					}
					if (ni == 0 || no == 0)
						continue ;
					/*
					** Punto de corte en cada arista dentro-fuera, con su
					** normal (menos el gradiente: hacia donde baja el campo).
					*/
					simd_float3 pts[4], nrm[4], cin = 0, cout = 0;
					int np = 0;
					for (int a = 0; a < ni; a++)
						for (int b = 0; b < no; b++)
						{
							int ca = in[a], cb = out[b];
							float tt = (iso - val[ca]) / (val[cb] - val[ca]);
							simd_float3 pa = simd_make_float3(x + (ca & 1),
								y + ((ca >> 1) & 1), z + ((ca >> 2) & 1));
							simd_float3 pb = simd_make_float3(x + (cb & 1),
								y + ((cb >> 1) & 1), z + ((cb >> 2) & 1));
							pts[np] = g.lo + (pa + (pb - pa) * tt) * g.c;
							simd_float3 ga = grad_at(&g, (int)pa.x, (int)pa.y,
								(int)pa.z);
							simd_float3 gb = grad_at(&g, (int)pb.x, (int)pb.y,
								(int)pb.z);
							nrm[np++] = -(ga + (gb - ga) * tt);
						}
					for (int a = 0; a < ni; a++)
						cin += simd_make_float3(in[a] & 1, (in[a] >> 1) & 1,
							(in[a] >> 2) & 1);
					for (int b = 0; b < no; b++)
						cout += simd_make_float3(out[b] & 1, (out[b] >> 1) & 1,
							(out[b] >> 2) & 1);
					simd_float3 outward = cout / no - cin / ni;
					if (np == 3)
						emit_tri(f, pts, nrm, outward);
					else
					{
						/*
						** Dos esquinas dentro y dos fuera: cuatro cortes que
						** forman un quad (a0b0, a0b1, a1b1, a1b0).
						*/
						simd_float3 q1[3] = {pts[0], pts[1], pts[3]};
						simd_float3 n1[3] = {nrm[0], nrm[1], nrm[3]};
						simd_float3 q2[3] = {pts[0], pts[3], pts[2]};
						simd_float3 n2[3] = {nrm[0], nrm[3], nrm[2]};
						emit_tri(f, q1, n1, outward);
						emit_tri(f, q2, n2, outward);
					}
				}
			}
	free(g.phi);
}
