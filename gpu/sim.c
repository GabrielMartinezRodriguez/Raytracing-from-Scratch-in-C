/*
** Escenarios animados del bodegon (y de cualquier .obj): convierte objetos
** del modelo en cuerpos rigidos del motor de fisica, les da su estado
** inicial y, en cada imagen, mueve los vertices de la malla segun la
** posicion y orientacion de su cuerpo.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

enum { SIM_NONE, SIM_FRUTAS, SIM_LLUVIA };

static float	frand(unsigned *seed)
{
	*seed = *seed * 1664525u + 1013904223u;
	return ((*seed >> 8) / 16777216.0f);
}

static int		find_obj(t_gpu_mesh *m, const char *name)
{
	for (int i = 0; i < m->nobjs; i++)
		if (strcmp(m->obj_names[i], name) == 0)
			return (i);
	return (-1);
}

static int		starts(const char *s, const char *prefix)
{
	return (strncmp(s, prefix, strlen(prefix)) == 0);
}

/*
** La malla de la cache esta mapeada en solo lectura: para anadir o cambiar
** triangulos se copia a memoria propia.
*/

void			mesh_detach(t_gpu_mesh *m)
{
	if (!m->map)
		return ;
	float *pos = malloc(sizeof(float) * 9 * m->ntris);
	t_gpu_tri *tris = malloc(sizeof(t_gpu_tri) * m->ntris);
	uint32_t *obj = malloc(sizeof(uint32_t) * m->ntris);
	memcpy(pos, m->pos, sizeof(float) * 9 * m->ntris);
	memcpy(tris, m->tris, sizeof(t_gpu_tri) * m->ntris);
	memcpy(obj, m->tri_obj, sizeof(uint32_t) * m->ntris);
	free_mesh_data(m);
	m->pos = pos;
	m->tris = tris;
	m->tri_obj = obj;
	m->cap = m->ntris;
}

/*
** Triangulos de un objeto en un array compacto (9 floats por triangulo).
*/

static float	*object_tris(t_gpu_mesh *m, int obj, int *n)
{
	int		count = 0;
	float	*out;

	for (size_t i = 0; i < m->ntris; i++)
		count += m->tri_obj[i] == (uint32_t)obj;
	out = malloc(sizeof(float) * 9 * (count + 1));
	count = 0;
	for (size_t i = 0; i < m->ntris; i++)
		if (m->tri_obj[i] == (uint32_t)obj)
			memcpy(out + 9 * count++, m->pos + 9 * i, sizeof(float) * 9);
	*n = count;
	return (out);
}

/*
** Copia un objeto (sus triangulos) como objeto nuevo; devuelve su indice.
*/

static int		clone_object(t_gpu_mesh *m, int obj)
{
	int		id = m->nobjs;
	size_t	n0 = m->ntris;
	size_t	add = 0;

	for (size_t i = 0; i < n0; i++)
		add += m->tri_obj[i] == (uint32_t)obj;
	if (m->ntris + add > m->cap)
	{
		m->cap = (m->ntris + add) * 2;
		m->pos = realloc(m->pos, sizeof(float) * 9 * m->cap);
		m->tris = realloc(m->tris, sizeof(t_gpu_tri) * m->cap);
		m->tri_obj = realloc(m->tri_obj, sizeof(uint32_t) * m->cap);
	}
	for (size_t i = 0; i < n0; i++)
		if (m->tri_obj[i] == (uint32_t)obj)
		{
			memcpy(m->pos + 9 * m->ntris, m->pos + 9 * i, sizeof(float) * 9);
			m->tris[m->ntris] = m->tris[i];
			m->tri_obj[m->ntris++] = id;
		}
	m->obj_names = realloc(m->obj_names, 64 * (m->nobjs + 1));
	snprintf(m->obj_names[id], 64, "%s_copia", m->obj_names[obj]);
	m->nobjs++;
	return (id);
}

typedef struct	s_kind
{
	const char	*prefix;
	float		mass;
	float		restitution;
	float		friction;
	int			round;
}				t_kind;

/*
** Material fisico de cada objeto del bodegon segun su nombre.
*/

/*
** Masa real aproximada de cada objeto (kg): las esferas rellenan tambien
** partes huecas, asi que la densidad se ajusta para que la masa cuadre.
*/

static const t_kind	g_kinds[] = {
	{"food_apple", 0.2f, 0.35f, 0.6f, 1},
	{"food_pomegranate", 0.3f, 0.3f, 0.6f, 1},
	{"lemon", 0.12f, 0.35f, 0.6f, 1},
	{"croissant", 0.06f, 0.2f, 0.7f, 0},
	{"wooden_cutting_board", 1.0f, 0.3f, 0.5f, 0},
	{"ceramic_vase", 1.2f, 0.2f, 0.4f, 0},
	{"brass_goblet", 0.35f, 0.35f, 0.35f, 0},
	{NULL, 0, 0, 0, 0}
};

static const t_kind	*kind_of(const char *name)
{
	for (int i = 0; g_kinds[i].prefix; i++)
		if (starts(name, g_kinds[i].prefix))
			return (&g_kinds[i]);
	return (NULL);
}

static int		make_body(t_sim *s, t_gpu_mesh *m, int obj, const t_kind *k)
{
	int			n;
	float		*tris = object_tris(m, obj, &n);
	t_psphere	*sp;
	simd_float3	com;
	int			ns = k->round ? fit_sphere(tris, n, &sp, &com)
		: build_cluster(tris, n, 9, &sp, &com);
	float		vol = 0;

	for (int i = 0; i < ns; i++)
		vol += 4.0f / 3.0f * (float)M_PI * sp[i].r * sp[i].r * sp[i].r;
	int			b = world_add_body(&s->w, sp, ns, k->mass / vol, com);

	s->w.bodies[b].restitution = k->restitution;
	s->w.bodies[b].friction = k->friction;
	s->obj_body[obj] = b;
	printf("cuerpo %-24s %4d triangulos -> %3d esferas, %.2f kg\n",
		m->obj_names[obj], n, ns, 1 / s->w.bodies[b].inv_mass);
	free(tris);
	free(sp);
	return (b);
}

/*
** Velocidad para lanzar un cuerpo desde su posicion y que llegue a "to" en
** "t" segundos (tiro parabolico).
*/

static simd_float3	aim(simd_float3 from, simd_float3 to, float t,
						simd_float3 g)
{
	return ((to - from) / t - g * (0.5f * t));
}

static void		static_world(t_sim *s, t_gpu_mesh *m)
{
	size_t	n = 0;
	float	*tris = malloc(sizeof(float) * 9 * (m->ntris + 1));

	for (size_t i = 0; i < m->ntris; i++)
	{
		int o = m->tri_obj[i];
		if (s->obj_body[o] >= 0 || starts(m->obj_names[o], "potted_plant_02_leaves"))
			continue ;
		memcpy(tris + 9 * n++, m->pos + 9 * i, sizeof(float) * 9);
	}
	world_set_static(&s->w, tris, (int)n, 0.08f);
	int ground = find_obj(m, "Plane");
	if (ground >= 0)
	{
		s->w.has_ground = 1;
		s->w.ground_y = 1e30f;
		for (size_t i = 0; i < m->ntris; i++)
			if (m->tri_obj[i] == (uint32_t)ground)
				for (int k = 0; k < 3; k++)
					s->w.ground_y = fminf(s->w.ground_y, m->pos[i * 9 + k * 3 + 1]);
	}
	printf("entorno estatico: %zu triangulos\n", n);
	free(tris);
}

int				sim_setup(t_sim *s, const char *name, t_gpu_mesh *m)
{
	unsigned	seed = 12345;

	memset(s, 0, sizeof(*s));
	world_init(&s->w);
	mesh_detach(m);
	s->kind = strcmp(name, "frutas") == 0 ? SIM_FRUTAS
		: strcmp(name, "lluvia") == 0 ? SIM_LLUVIA : SIM_NONE;
	if (s->kind == SIM_NONE)
	{
		fprintf(stderr, "error: escenario desconocido '%s' (frutas, lluvia)\n",
			name);
		return (-1);
	}
	if (s->kind == SIM_LLUVIA)
	{
		const char *fruits[3] = {"food_apple_01", "lemon", "food_pomegranate_01"};
		int base[3];
		for (int k = 0; k < 3; k++)
			base[k] = find_obj(m, fruits[k]);
		for (int i = 0; i < 180; i++)
			if (base[i % 3] >= 0)
				clone_object(m, base[i % 3]);
	}
	s->nobjs = m->nobjs;
	s->obj_body = malloc(sizeof(int) * m->nobjs);
	for (int i = 0; i < m->nobjs; i++)
		s->obj_body[i] = -1;
	for (int i = 0; i < m->nobjs; i++)
	{
		const t_kind *k = kind_of(m->obj_names[i]);
		if (k)
			make_body(s, m, i, k);
	}
	static_world(s, m);
	world_settle(&s->w);
	t_world *w = &s->w;
	if (s->kind == SIM_FRUTAS)
	{
		/*
		** El limon rueda hacia el borde y cae; la granada cae sobre la
		** tabla; la manzana sale lanzada contra una copa.
		*/
		int lemon = s->obj_body[find_obj(m, "lemon")];
		int pom = s->obj_body[find_obj(m, "food_pomegranate_01")];
		int apple = s->obj_body[find_obj(m, "food_apple_01")];
		int gob = s->obj_body[find_obj(m, "brass_goblet_01")];
		if (lemon >= 0)
		{
			w->bodies[lemon].vel = simd_make_float3(0.55f, 0, 0.75f);
			w->bodies[lemon].ang = simd_make_float3(8, 0, -6);
		}
		if (pom >= 0)
		{
			w->bodies[pom].pos += simd_make_float3(0.05f, 0.45f, -0.15f);
			w->bodies[pom].ang = simd_make_float3(2, 1, 0);
		}
		if (apple >= 0 && gob >= 0)
		{
			t_body *a = &w->bodies[apple];
			a->pos = simd_make_float3(0.55f, 1.25f, -0.35f);
			a->vel = aim(a->pos, w->bodies[gob].pos + simd_make_float3(0, 0.1f,
				0), 0.75f, w->gravity);
			a->ang = simd_make_float3(0, 5, 10);
		}
	}
	else
	{
		/*
		** Lluvia: cada copia empieza a una altura distinta (asi caen
		** escalonadas), girada al azar y con algo de velocidad lateral.
		*/
		for (int i = 0; i < m->nobjs; i++)
		{
			int b = s->obj_body[i];
			if (b < 0 || !strstr(m->obj_names[i], "_copia"))
				continue ;
			t_body *bd = &w->bodies[b];
			bd->pos = simd_make_float3((frand(&seed) - 0.5f) * 1.3f,
				1.6f + frand(&seed) * 4.5f, (frand(&seed) - 0.5f) * 1.3f);
			bd->rot = simd_normalize(simd_quaternion(frand(&seed) * 6.28f,
				simd_normalize(simd_make_float3(frand(&seed) - 0.5f,
				frand(&seed) - 0.5f, frand(&seed) - 0.5f))));
			bd->vel = simd_make_float3((frand(&seed) - 0.5f) * 0.6f, -1,
				(frand(&seed) - 0.5f) * 0.6f);
			bd->ang = simd_make_float3(frand(&seed) - 0.5f, frand(&seed) - 0.5f,
				frand(&seed) - 0.5f) * 8;
		}
	}
	s->wind_obj = find_obj(m, "potted_plant_02_leaves");
	return (0);
}

void			sim_advance(t_sim *s, float dt)
{
	world_step(&s->w, dt);
	s->time += dt;
}

/*
** Gira una normal comprimida (octaedro, 2 x 16 bits) con el cuaternion.
*/

static unsigned	rotate_normal(unsigned packed, simd_quatf q)
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
	return (pack_normal(simd_act(q, simd_normalize(n))));
}

void			sim_apply(t_sim *s, const uint32_t *tri_obj,
					const float *rest_pos, const t_gpu_tri *rest_tris,
					float *pos, t_gpu_tri *tris, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		uint32_t o = tri_obj[i];
		int b = (int)o < s->nobjs ? s->obj_body[o] : -1;
		if (b < 0)
			continue ;
		t_body *bd = &s->w.bodies[b];
		for (int k = 0; k < 3; k++)
		{
			simd_float3 p = simd_make_float3(rest_pos[i * 9 + k * 3],
				rest_pos[i * 9 + k * 3 + 1], rest_pos[i * 9 + k * 3 + 2]);
			p = body_point(bd, p);
			pos[i * 9 + k * 3] = p.x;
			pos[i * 9 + k * 3 + 1] = p.y;
			pos[i * 9 + k * 3 + 2] = p.z;
			tris[i].n[k] = rotate_normal(rest_tris[i].n[k], bd->rot);
		}
	}
}
