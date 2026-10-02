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
#include "fracture.h"

enum { SIM_NONE, SIM_FRUTAS, SIM_LLUVIA, SIM_VIENTO, SIM_TELA, SIM_ROTURA,
	SIM_LIQUIDO };

#define FLUID_TRIS 450000

#define SHARDS 20

#define CLOTH_N 100
#define CLOTH_THICK 0.006f
#define CLOTH_DRAG 4.0f

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

static void		cloth_setup(t_sim *s, t_gpu_mesh *m);

/*
** Agua que cae en chorro sobre la mesa: material dielectrico (kd.w = 1) y
** FLUID_TRIS triangulos reservados en la malla para su superficie, que se
** rellenan cada imagen (los que sobran quedan degenerados, sin area).
*/

static void		fluid_setup(t_sim *s, t_gpu_mesh *m)
{
	t_fluid	*f = &s->fluid;

	fluid_init(f, 0.012f, 30000);
	f->emit_pos = simd_make_float3(0.15f, 1.6f, -0.05f);
	f->emit_vel = simd_make_float3(0, -1.2f, 0);
	f->emit_radius = 0.04f;
	f->emit_until = 5.0f;
	f->tri_cap = FLUID_TRIS;
	f->tri = malloc(sizeof(float) * 9 * f->tri_cap);
	f->tri_n = malloc(sizeof(unsigned) * 3 * f->tri_cap);
	m->mats = realloc(m->mats, sizeof(t_mesh_material) * (m->nmats + 1));
	t_mesh_material *mat = &m->mats[m->nmats];
	memset(mat, 0, sizeof(*mat));
	snprintf(mat->name, sizeof(mat->name), "agua");
	mat->kd = simd_make_float4(1, 1, 1, 1);
	mat->roughness = 0.03f;
	int material = m->nmats++;
	s->fluid_obj = m->nobjs;
	m->obj_names = realloc(m->obj_names, 64 * (m->nobjs + 1));
	snprintf(m->obj_names[m->nobjs++], 64, "agua");
	if (m->ntris + FLUID_TRIS > m->cap)
	{
		m->cap = m->ntris + FLUID_TRIS;
		m->pos = realloc(m->pos, sizeof(float) * 9 * m->cap);
		m->tris = realloc(m->tris, sizeof(t_gpu_tri) * m->cap);
		m->tri_obj = realloc(m->tri_obj, sizeof(uint32_t) * m->cap);
	}
	for (int i = 0; i < FLUID_TRIS; i++)
	{
		for (int k = 0; k < 9; k++)
			m->pos[m->ntris * 9 + k] = k % 3 == 1 ? -50.0f : 0.0f;
		memset(&m->tris[m->ntris], 0, sizeof(t_gpu_tri));
		m->tris[m->ntris].material = material;
		m->tri_obj[m->ntris++] = s->fluid_obj;
	}
	printf("agua: densidad de reposo %.0f, hasta %d particulas y %d "
		"triangulos de superficie\n", f->rho0, f->cap, FLUID_TRIS);
}

/*
** Prefractura del jarron: SHARDS puntos al azar dentro de su caja y cada
** triangulo va al punto mas cercano (diagrama de Voronoi). Cada trozo pasa a
** ser un objeto propio de la malla; mientras el jarron esta entero, todos
** se mueven con el mismo cuerpo.
*/

static void		prefracture(t_sim *s, t_gpu_mesh *m)
{
	int			vase = find_obj(m, "ceramic_vase_01");
	unsigned	seed = 777;
	simd_float3	lo = simd_make_float3(1e30f, 1e30f, 1e30f), hi = -lo;
	simd_float3	seeds[SHARDS];

	if (vase < 0)
		return ;
	for (size_t i = 0; i < m->ntris; i++)
		if (m->tri_obj[i] == (uint32_t)vase)
			for (int k = 0; k < 3; k++)
			{
				simd_float3 p = simd_make_float3(m->pos[i * 9 + k * 3],
					m->pos[i * 9 + k * 3 + 1], m->pos[i * 9 + k * 3 + 2]);
				lo = simd_min(lo, p);
				hi = simd_max(hi, p);
			}
	for (int k = 0; k < SHARDS; k++)
		seeds[k] = lo + (hi - lo) * simd_make_float3(frand(&seed),
			frand(&seed), frand(&seed));
	m->obj_names = realloc(m->obj_names, 64 * (m->nobjs + SHARDS));
	for (int k = 0; k < SHARDS; k++)
	{
		s->shard_obj[k] = m->nobjs;
		snprintf(m->obj_names[m->nobjs++], 64, "vasija_trozo_%02d", k);
	}
	s->nshards = SHARDS;
	/*
	** Cara de rotura: ceramica sin esmaltar, blanca rota y mate.
	*/
	m->mats = realloc(m->mats, sizeof(t_mesh_material) * (m->nmats + 1));
	t_mesh_material *mat = &m->mats[m->nmats];
	memset(mat, 0, sizeof(*mat));
	snprintf(mat->name, sizeof(mat->name), "ceramica_rota");
	mat->kd = simd_make_float4(0.80f, 0.76f, 0.70f, 0);
	mat->roughness = 0.9f;
	int cracks = fracture_object(m, vase, seeds, SHARDS, s->shard_obj,
		m->nmats++);
	printf("jarron: %d trozos, %d bordes de rotura\n", SHARDS, cracks);
	s->shard_tris = malloc(sizeof(float *) * SHARDS);
	s->shard_ntris = malloc(sizeof(int) * SHARDS);
	for (int k = 0; k < SHARDS; k++)
		s->shard_tris[k] = object_tris(m, s->shard_obj[k], &s->shard_ntris[k]);
}

/*
** Jarron entero: un cuerpo con todos los triangulos de los trozos; se
** suelta desde 1,4 m sobre el suelo, junto a la mesa, girando.
*/

static void		vase_setup(t_sim *s, t_gpu_mesh *m)
{
	int			total = 0;
	float		*all;
	t_psphere	*sp;
	simd_float3	com;

	for (int k = 0; k < s->nshards; k++)
		total += s->shard_ntris[k];
	all = malloc(sizeof(float) * 9 * (total + 1));
	total = 0;
	for (int k = 0; k < s->nshards; k++)
	{
		memcpy(all + 9 * total, s->shard_tris[k], sizeof(float) * 9
			* s->shard_ntris[k]);
		total += s->shard_ntris[k];
	}
	int ns = build_cluster(all, total, 9, &sp, &com);
	float vol = 0;
	for (int i = 0; i < ns; i++)
		vol += 4.0f / 3.0f * (float)M_PI * sp[i].r * sp[i].r * sp[i].r;
	s->vase_body = world_add_body(&s->w, sp, ns, 1.2f / vol, com);
	t_body *b = &s->w.bodies[s->vase_body];
	b->restitution = 0.1f;
	b->pos = simd_make_float3(0.75f, 1.45f, 0.45f);
	b->rot = simd_normalize(simd_quaternion(0.5f, simd_normalize(
		simd_make_float3(1, 0, 0.4f))));
	b->ang = simd_make_float3(1.5f, 0.5f, -2.0f);
	for (int k = 0; k < s->nshards; k++)
		s->obj_body[s->shard_obj[k]] = s->vase_body;
	free(all);
	free(sp);
	(void)m;
}

/*
** Rotura: si el jarron entero pierde de golpe mucha velocidad (impacto),
** cada trozo pasa a ser un cuerpo propio con la velocidad que tenia ese
** punto del jarron mas un pequeno empuje hacia fuera.
*/

static void		maybe_break(t_sim *s)
{
	t_body		*v = &s->w.bodies[s->vase_body];
	unsigned	seed = 4242;

	if (s->broken || simd_length(s->vase_vel) - simd_length(v->vel) < 1.2f)
		return ;
	s->broken = 1;
	simd_float3 pre = s->vase_vel;
	simd_float3 vcenter = v->pos;
	float total_area = 0;
	float area[32] = {0};
	for (int k = 0; k < s->nshards; k++)
	{
		for (int t = 0; t < s->shard_ntris[k]; t++)
		{
			const float *p = s->shard_tris[k] + t * 9;
			simd_float3 a = simd_make_float3(p[0], p[1], p[2]);
			simd_float3 e1 = simd_make_float3(p[3], p[4], p[5]) - a;
			simd_float3 e2 = simd_make_float3(p[6], p[7], p[8]) - a;
			area[k] += 0.5f * simd_length(simd_cross(e1, e2));
		}
		total_area += area[k];
	}
	for (int k = 0; k < s->nshards; k++)
	{
		t_psphere *sp;
		simd_float3 com;
		if (s->shard_ntris[k] == 0)
			continue ;
		int ns = build_cluster(s->shard_tris[k], s->shard_ntris[k], 6, &sp,
			&com);
		float vol = 0;
		for (int i = 0; i < ns; i++)
			vol += 4.0f / 3.0f * (float)M_PI * sp[i].r * sp[i].r * sp[i].r;
		/*
		** Masa de cada trozo proporcional a su superficie (pared de grosor
		** uniforme).
		*/
		int b = world_add_body(&s->w, sp, ns, 1.2f * area[k] / total_area
			/ vol, com);
		v = &s->w.bodies[s->vase_body];
		t_body *sh = &s->w.bodies[b];
		sh->rot = v->rot;
		sh->pos = body_point(v, com);
		simd_float3 out = simd_normalize(sh->pos - vcenter
			+ simd_make_float3(0, 0.02f, 0));
		sh->vel = pre * 0.45f + simd_cross(v->ang, sh->pos - v->pos)
			+ out * (0.3f + frand(&seed) * 0.6f);
		sh->ang = simd_make_float3(frand(&seed) - 0.5f, frand(&seed) - 0.5f,
			frand(&seed) - 0.5f) * 9;
		sh->restitution = 0.15f;
		sh->friction = 0.6f;
		s->obj_body[s->shard_obj[k]] = b;
		free(sp);
	}
	s->w.bodies[s->vase_body].active = 0;
	printf("\njarron roto en %d trozos a %.1f m/s\n", s->nshards,
		simd_length(pre));
}

int				sim_setup(t_sim *s, const char *name, t_gpu_mesh *m)
{
	unsigned	seed = 12345;

	memset(s, 0, sizeof(*s));
	world_init(&s->w);
	mesh_detach(m);
	s->kind = strcmp(name, "frutas") == 0 ? SIM_FRUTAS
		: strcmp(name, "lluvia") == 0 ? SIM_LLUVIA
		: strcmp(name, "viento") == 0 ? SIM_VIENTO
		: strcmp(name, "tela") == 0 ? SIM_TELA
		: strcmp(name, "rotura") == 0 ? SIM_ROTURA
		: strcmp(name, "liquido") == 0 ? SIM_LIQUIDO : SIM_NONE;
	if (s->kind == SIM_NONE)
	{
		fprintf(stderr, "error: escenario desconocido '%s' (frutas, lluvia, "
			"viento, tela, rotura, liquido)\n", name);
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
	if (s->kind == SIM_ROTURA)
		prefracture(s, m);
	s->nobjs = m->nobjs;
	s->obj_body = malloc(sizeof(int) * m->nobjs);
	for (int i = 0; i < m->nobjs; i++)
		s->obj_body[i] = -1;
	for (int i = 0; i < m->nobjs; i++)
	{
		const t_kind *k = kind_of(m->obj_names[i]);
		if (s->kind == SIM_ROTURA && starts(m->obj_names[i], "vasija_trozo"))
			continue ;
		if (k && s->kind != SIM_VIENTO && s->kind != SIM_TELA
			&& s->kind != SIM_LIQUIDO)
			make_body(s, m, i, k);
	}
	static_world(s, m);
	world_settle(&s->w);
	t_world *w = &s->w;
	if (s->kind == SIM_ROTURA)
		vase_setup(s, m);
	if (s->kind == SIM_LIQUIDO)
		fluid_setup(s, m);
	if (s->kind == SIM_VIENTO)
		s->wind = 1.0f;
	else if (s->kind == SIM_TELA)
		cloth_setup(s, m);
	else if (s->kind == SIM_FRUTAS)
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
	/*
	** Viento sobre las hojas: base y altura de la planta (sus vertices). En
	** los demas escenarios sopla una brisa suave.
	*/
	s->wind_obj = find_obj(m, "potted_plant_02_leaves");
	if (s->wind == 0)
		s->wind = 0.35f;
	if (s->wind_obj >= 0)
	{
		simd_float3 lo = simd_make_float3(1e30f, 1e30f, 1e30f), hi = -lo;
		for (size_t i = 0; i < m->ntris; i++)
			if (m->tri_obj[i] == (uint32_t)s->wind_obj)
				for (int k = 0; k < 3; k++)
				{
					simd_float3 p = simd_make_float3(m->pos[i * 9 + k * 3],
						m->pos[i * 9 + k * 3 + 1], m->pos[i * 9 + k * 3 + 2]);
					lo = simd_min(lo, p);
					hi = simd_max(hi, p);
				}
		s->wind_base = simd_make_float3((lo.x + hi.x) / 2, lo.y,
			(lo.z + hi.z) / 2);
		s->wind_height = hi.y - lo.y;
	}
	return (0);
}

/*
** Mantel de cuadros: material nuevo (textura de Poly Haven junto al .obj)
** y una rejilla de CLOTH_N x CLOTH_N particulas colgando sobre la mesa,
** girada 30 grados para que las esquinas caigan por los lados.
*/

static void		cloth_setup(t_sim *s, t_gpu_mesh *m)
{
	t_cloth		*c = &s->cloth;
	int			n = CLOTH_N;
	char		dir[1024];
	const char	*slash;

	slash = strrchr(m->mats[1].texture, '/');
	snprintf(dir, sizeof(dir), "%.*s", slash ? (int)(slash - m->mats[1].texture)
		: 1, slash ? m->mats[1].texture : ".");
	m->mats = realloc(m->mats, sizeof(t_mesh_material) * (m->nmats + 1));
	t_mesh_material *mat = &m->mats[m->nmats];
	memset(mat, 0, sizeof(*mat));
	snprintf(mat->name, sizeof(mat->name), "mantel");
	snprintf(mat->texture, sizeof(mat->texture), "%s/fabric_pattern_07_col_1_2k.jpg", dir);
	snprintf(mat->normal_map, sizeof(mat->normal_map), "%s/fabric_pattern_07_nor_gl_2k.png", dir);
	snprintf(mat->rough_map, sizeof(mat->rough_map), "%s/fabric_pattern_07_rough_2k.jpg", dir);
	mat->kd = simd_make_float4(1, 1, 1, 0);
	mat->roughness = 0.9f;
	int material = m->nmats++;
	c->n = n;
	c->size = 1.9f;
	c->x = malloc(sizeof(simd_float3) * n * n);
	c->p = malloc(sizeof(simd_float3) * n * n);
	c->v = calloc(n * n, sizeof(simd_float3));
	c->nrm = malloc(sizeof(simd_float3) * n * n);
	c->hitn = calloc(n * n, sizeof(simd_float3));
	float ang = 0.52f;
	for (int j = 0; j < n; j++)
		for (int i = 0; i < n; i++)
		{
			float u = ((float)i / (n - 1) - 0.5f) * c->size;
			float w = ((float)j / (n - 1) - 0.5f) * c->size;
			c->x[j * n + i] = simd_make_float3(u * cosf(ang) - w * sinf(ang),
				1.55f + 0.002f * sinf(i * 0.7f) * cosf(j * 0.9f),
				u * sinf(ang) + w * cosf(ang));
		}
	/*
	** Restricciones: vecinos (estructura), diagonales (cizalla) y a dos
	** pasos (flexion, blanda para que caiga en pliegues).
	*/
	int cap = n * n * 6;
	c->cons = malloc(sizeof(int[2]) * cap);
	c->rest = malloc(sizeof(float) * cap);
	c->stiff = malloc(sizeof(float) * cap);
	int offs[6][3] = {{1, 0, 100}, {0, 1, 100}, {1, 1, 80}, {1, -1, 80},
		{2, 0, 10}, {0, 2, 10}};
	for (int j = 0; j < n; j++)
		for (int i = 0; i < n; i++)
			for (int k = 0; k < 6; k++)
			{
				int i2 = i + offs[k][0], j2 = j + offs[k][1];
				if (i2 < 0 || j2 < 0 || i2 >= n || j2 >= n)
					continue ;
				c->cons[c->ncons][0] = j * n + i;
				c->cons[c->ncons][1] = j2 * n + i2;
				c->rest[c->ncons] = simd_distance(c->x[j * n + i],
					c->x[j2 * n + i2]);
				c->stiff[c->ncons++] = offs[k][2] / 100.0f;
			}
	/*
	** Triangulos de la tela en la malla (posiciones de partida; cada imagen
	** sim_apply los recoloca).
	*/
	c->obj = m->nobjs;
	m->obj_names = realloc(m->obj_names, 64 * (m->nobjs + 1));
	snprintf(m->obj_names[m->nobjs++], 64, "mantel");
	size_t add = (size_t)(n - 1) * (n - 1) * 2;
	if (m->ntris + add > m->cap)
	{
		m->cap = m->ntris + add;
		m->pos = realloc(m->pos, sizeof(float) * 9 * m->cap);
		m->tris = realloc(m->tris, sizeof(t_gpu_tri) * m->cap);
		m->tri_obj = realloc(m->tri_obj, sizeof(uint32_t) * m->cap);
	}
	float tile = c->size / 0.4f;
	for (int j = 0; j < n - 1; j++)
		for (int i = 0; i < n - 1; i++)
			for (int h = 0; h < 2; h++)
			{
				/*
				** Cada quad son dos triangulos: (i,j)(i+1,j)(i,j+1) y
				** (i+1,j)(i+1,j+1)(i,j+1).
				*/
				int q[3][2] = {{i + h, j}, {i + 1, j + h}, {i, j + 1}};
				t_gpu_tri *t = &m->tris[m->ntris];
				memset(t, 0, sizeof(*t));
				for (int k = 0; k < 3; k++)
				{
					simd_float3 p = c->x[q[k][1] * n + q[k][0]];
					m->pos[m->ntris * 9 + k * 3] = p.x;
					m->pos[m->ntris * 9 + k * 3 + 1] = p.y;
					m->pos[m->ntris * 9 + k * 3 + 2] = p.z;
					t->uv[k * 2] = (float)q[k][0] / (n - 1) * tile;
					t->uv[k * 2 + 1] = (float)q[k][1] / (n - 1) * tile;
					t->n[k] = pack_normal(simd_make_float3(0, 1, 0));
				}
				t->material = material;
				m->tri_obj[m->ntris++] = c->obj;
			}
	printf("mantel: %d particulas, %d restricciones, %zu triangulos\n",
		n * n, c->ncons, add);
}

/*
** Un paso de Position Based Dynamics (Muller et al. 2007): predecir con la
** velocidad, corregir posiciones para cumplir las restricciones y las
** colisiones, y sacar la velocidad de lo que se ha movido.
*/

static void		cloth_step(t_sim *s, float dt)
{
	t_cloth	*c = &s->cloth;
	int		np = c->n * c->n;
	int		sub = 30;
	float	h = dt / sub;

	for (int st = 0; st < sub; st++)
	{
		for (int i = 0; i < np; i++)
		{
			c->v[i] += s->w.gravity * h;
			/*
			** Rozamiento con el aire: una tela tiene mucha superficie para
			** su masa y deja de oscilar enseguida.
			*/
			c->v[i] *= 1 - CLOTH_DRAG * h;
			c->p[i] = c->x[i] + c->v[i] * h;
		}
		/*
		** Muchos subpasos cortos con pocas iteraciones cada uno ("Small
		** Steps in Physics Simulation", Macklin et al. 2019): la tela se
		** estira mucho menos que con pocos pasos largos.
		*/
		for (int it = 0; it < 2; it++)
		{
			for (int k = 0; k < c->ncons; k++)
			{
				int a = c->cons[k][0], b = c->cons[k][1];
				simd_float3 d = c->p[b] - c->p[a];
				float len = simd_length(d);
				if (len < 1e-9f)
					continue ;
				simd_float3 corr = d * (0.5f * c->stiff[k] * (len - c->rest[k])
					/ len);
				c->p[a] += corr;
				c->p[b] -= corr;
			}
		}
		{
			for (int i = 0; i < np; i++)
			{
				simd_float3 nrm;
				c->hitn[i] = 0;
				if (world_push_point(&s->w, &c->p[i], CLOTH_THICK, &nrm) > 0)
				{
					c->hitn[i] = nrm;
					/*
					** Rozamiento: el desplazamiento tangencial respecto a la
					** posicion anterior se frena.
					*/
					simd_float3 dp = c->p[i] - c->x[i];
					simd_float3 tn = dp - nrm * simd_dot(dp, nrm);
					c->p[i] -= tn * 0.4f;
				}
			}
		}
		for (int i = 0; i < np; i++)
		{
			c->v[i] = (c->p[i] - c->x[i]) / h;
			/*
			** El empujon de la colision no es velocidad real: si la
			** particula ha chocado, se quita la componente que sale de la
			** superficie (contacto inelastico, como la tela de verdad).
			*/
			float vn = simd_dot(c->v[i], c->hitn[i]);
			if (vn > 0)
				c->v[i] -= c->hitn[i] * vn;
			c->x[i] = c->p[i];
		}
	}
	/*
	** Normales de vertice: suma de las normales de los quads vecinos.
	*/
	int n = c->n;
	memset(c->nrm, 0, sizeof(simd_float3) * np);
	for (int j = 0; j < n - 1; j++)
		for (int i = 0; i < n - 1; i++)
		{
			simd_float3 a = c->x[j * n + i], b = c->x[j * n + i + 1];
			simd_float3 d = c->x[(j + 1) * n + i];
			simd_float3 fn = simd_cross(d - a, b - a);
			c->nrm[j * n + i] += fn;
			c->nrm[j * n + i + 1] += fn;
			c->nrm[(j + 1) * n + i] += fn;
			c->nrm[(j + 1) * n + i + 1] += fn;
		}
}

void			sim_advance(t_sim *s, float dt)
{
	if (dt <= 0)
		return ;
	if (s->nshards && !s->broken)
		s->vase_vel = s->w.bodies[s->vase_body].vel;
	world_step(&s->w, dt);
	if (s->nshards)
		maybe_break(s);
	if (s->cloth.n)
		cloth_step(s, dt);
	if (s->fluid.cap)
	{
		fluid_step(&s->fluid, &s->w, dt);
		fluid_surface(&s->fluid);
	}
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

/*
** Desplazamiento de una hoja por el viento: crece con la altura (la base
** no se mueve), una onda que recorre la planta, rachas lentas y un temblor
** rapido de cada hoja.
*/

static simd_float3	wind_offset(t_sim *s, simd_float3 p)
{
	float	h = fmaxf(0, (p.y - s->wind_base.y) / s->wind_height);
	float	t = s->time;
	float	gust = 0.55f + 0.45f * sinf(t * 0.9f) * sinf(t * 0.37f + 1.3f);
	float	phase = p.x * 7 + p.z * 5 - t * 2.2f;
	float	sway = 0.045f * h * h * gust * s->wind;
	float	flutter = 0.006f * h * s->wind * sinf(t * 13 + p.x * 41 + p.y * 37
		+ p.z * 29);

	return (simd_make_float3(sway * (0.8f + 0.4f * sinf(phase)) + flutter,
		flutter * 0.6f, sway * 0.35f * sinf(phase * 1.3f + 0.7f) + flutter));
}

void			sim_apply(t_sim *s, const uint32_t *tri_obj,
					const float *rest_pos, const t_gpu_tri *rest_tris,
					float *pos, t_gpu_tri *tris, size_t n)
{
	size_t	cloth_tri = 0;
	int		fluid_tri = 0;

	for (size_t i = 0; i < n; i++)
	{
		uint32_t o = tri_obj[i];
		if (s->fluid.cap && (int)o == s->fluid_obj)
		{
			t_fluid *f = &s->fluid;
			int k = fluid_tri++;
			for (int c = 0; c < 9; c++)
				pos[i * 9 + c] = k < f->ntri ? f->tri[k * 9 + c]
					: (c % 3 == 1 ? -50.0f : 0.0f);
			for (int c = 0; c < 3; c++)
				tris[i].n[c] = k < f->ntri ? f->tri_n[k * 3 + c] : 0;
			continue ;
		}
		if (s->cloth.n && (int)o == s->cloth.obj)
		{
			/*
			** Los triangulos de la tela conservan su orden relativo en la
			** GPU: el k-esimo es la mitad k % 2 del quad k / 2.
			*/
			t_cloth *c = &s->cloth;
			int q = (int)(cloth_tri / 2), hh = (int)(cloth_tri % 2);
			int qi = q % (c->n - 1), qj = q / (c->n - 1);
			int idx[3][2] = {{qi + hh, qj}, {qi + 1, qj + hh}, {qi, qj + 1}};
			for (int k = 0; k < 3; k++)
			{
				int v = idx[k][1] * c->n + idx[k][0];
				pos[i * 9 + k * 3] = c->x[v].x;
				pos[i * 9 + k * 3 + 1] = c->x[v].y;
				pos[i * 9 + k * 3 + 2] = c->x[v].z;
				tris[i].n[k] = pack_normal(simd_length(c->nrm[v]) > 1e-12f
					? simd_normalize(c->nrm[v]) : simd_make_float3(0, 1, 0));
			}
			cloth_tri++;
			continue ;
		}
		if ((int)o == s->wind_obj && s->wind > 0)
		{
			for (int k = 0; k < 3; k++)
			{
				simd_float3 p = simd_make_float3(rest_pos[i * 9 + k * 3],
					rest_pos[i * 9 + k * 3 + 1], rest_pos[i * 9 + k * 3 + 2]);
				p += wind_offset(s, p);
				pos[i * 9 + k * 3] = p.x;
				pos[i * 9 + k * 3 + 1] = p.y;
				pos[i * 9 + k * 3 + 2] = p.z;
			}
			continue ;
		}
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
