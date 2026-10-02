#include <metal_stdlib>
using namespace metal;
#ifdef HW_RT
# include <metal_raytracing>
using namespace raytracing;
/*
** Con una sola copia del modelo (MESH_INST sin definir) la malla va en una
** estructura primitiva directa: sin el nivel de instancias, que cuesta ~30%.
*/
# ifdef MESH_INST
#  define MESH_ACCEL instance_acceleration_structure
#  define MESH_TAGS triangle_data, instancing
# else
#  define MESH_ACCEL primitive_acceleration_structure
#  define MESH_TAGS triangle_data
# endif
# define ACCEL_DECL primitive_acceleration_structure accel, \
	intersection_function_table<> table, \
	MESH_ACCEL mesh_accel, \
	intersection_function_table<MESH_TAGS> mesh_table, \
	device const t_gpu_tri *tris, device const GpuMaterial *mats, \
	device const packed_float3 *mesh_pos, device const float4 *insts, \
	texture2d<float> env_map
# define ACCEL_ARGS accel, table, mesh_accel, mesh_table, tris, mats, \
	mesh_pos, insts, env_map
#else
# define ACCEL_DECL device const t_gpu_node *nodes, texture2d<float> env_map
# define ACCEL_ARGS nodes, env_map
#endif

#include "shared.h"

#ifdef HW_RT

/*
** Material de malla. La textura es una referencia sin enlazar (Metal 3):
** el host escribe su gpuResourceID en el mismo sitio (32 bytes en total).
*/

struct			GpuMaterial
{
	float4				kd;
	texture2d<float>	diffuse;
	texture2d<float>	normal;
	texture2d<float>	rough;
	texture2d<float>	metal;
	float				roughness;
	float				metallic;
	uint				flags;
	uint				pad;
};

constexpr sampler	tex_sampler(address::repeat, filter::linear,
						mip_filter::linear);
#endif

/*
** Port a Metal de generateImage/shading.c: mismas intersecciones y mismo
** sombreado (Blinn-Phong + sombras + reflejos + supersampling + gamma).
** Cada hilo de la GPU calcula un pixel.
*/

constant int	MAX_DEPTH = 5;
constant float	SPECULAR = 0.4;
constant float	MESH_SPECULAR = 0.05;
constant float	SHININESS = 60;
constant float	GAMMA = 2.2;
constant float	EPSILON = 1e-3;
constant float	EXPOSURE = 1.0;
constant int	GI_BOUNCES = 2;
constant int	MAX_EVENTS = 12;
constant float	INDIRECT_MAX = 6.0f;
constant float3	WATER_SIGMA = float3(0.9f, 0.25f, 0.12f);
constant int	GI_SHADOW_BOUNCES = 3;
constant float	BOUNCE_LOD_BIAS = 3.0f;
constant float	CLIP_NEAR = 0.05f;
constant float	CLIP_FAR = 1000.0f;
constant float	SKY_LIGHT = 2.0;
constant float	GI_EXPOSURE = 2.2;
constant float	AO_RADIUS = 1.5;
constant float	EDGE = 0.1;
constant float	SHADOW_MIN_WEIGHT = 0.25;
constant float	TMIN = 1e-3;
constant float	NOHIT = 1e30f;

struct			Hit
{
	float		t;
	float3		normal;
	int			id;
	float2		bary;
	bool		mesh;
	int			inst;
};

static float	hit_plane(float3 o, float3 d, float3 p, float3 n)
{
	float den = dot(d, n);

	if (fabs(den) < 1e-8)
		return (NOHIT);
	float t = dot(p - o, n) / den;
	return (t > TMIN ? t : NOHIT);
}

static float	hit_sphere(float3 o, float3 d, float3 c, float r)
{
	float3 oc = o - c;
	float b = dot(oc, d);
	/*
	** Forma estable (Ray Tracing Gems, cap. 7): en vez de b^2 - |oc|^2,
	** que resta dos numeros enormes lejos del origen, se usa la distancia
	** del centro al rayo.
	*/
	float3 qc = oc - b * d;
	float h = r * r - dot(qc, qc);

	if (h < 0)
		return (NOHIT);
	h = sqrt(h);
	if (-b - h > TMIN)
		return (-b - h);
	if (-b + h > TMIN)
		return (-b + h);
	return (NOHIT);
}

static float	hit_square(float3 o, float3 d, device const t_gpu_object &obj)
{
	float3 n = obj.b.xyz;
	float t = hit_plane(o, d, obj.a.xyz, n);

	if (t == NOHIT)
		return (NOHIT);
	float3 p = o + t * d - obj.a.xyz;
	float3 u = obj.c.xyz;
	float3 v = cross(n, u);
	float half_side = obj.height / 2;
	if (fabs(dot(p, u)) > half_side || fabs(dot(p, v)) > half_side)
		return (NOHIT);
	return (t);
}

static float	hit_triangle(float3 o, float3 d, device const t_gpu_object &obj)
{
	float3 e1 = obj.b.xyz - obj.a.xyz;
	float3 e2 = obj.c.xyz - obj.a.xyz;
	float3 pv = cross(d, e2);
	float det = dot(e1, pv);

	if (fabs(det) < 1e-8)
		return (NOHIT);
	float3 tv = o - obj.a.xyz;
	float u = dot(tv, pv) / det;
	if (u < 0 || u > 1)
		return (NOHIT);
	float3 qv = cross(tv, e1);
	float v = dot(d, qv) / det;
	if (v < 0 || u + v > 1)
		return (NOHIT);
	float t = dot(e2, qv) / det;
	return (t > TMIN ? t : NOHIT);
}

/*
** Cilindro finito con tapas: lateral (raices de la cuadratica dentro de la
** altura) y dos discos en +-altura/2.
*/

static float	hit_cylinder(float3 o, float3 d,
					device const t_gpu_object &obj, thread float3 &normal)
{
	float3 c = obj.a.xyz;
	float3 axis = obj.b.xyz;
	float r = obj.radius;
	float hh = obj.height / 2;
	float best = NOHIT;
	float3 oc = o - c;
	float da = dot(d, axis);
	float oa = dot(oc, axis);
	float3 dp = d - da * axis;
	float3 op = oc - oa * axis;
	float qa = dot(dp, dp);
	float qb = 2 * dot(dp, op);
	float disc = qb * qb - 4 * qa * (dot(op, op) - r * r);

	if (qa > 1e-8 && disc >= 0)
	{
		float sq = sqrt(disc);
		float roots[2] = {(-qb - sq) / (2 * qa), (-qb + sq) / (2 * qa)};
		for (int i = 0; i < 2; i++)
		{
			float t = roots[i];
			if (t > TMIN && fabs(oa + t * da) <= hh)
			{
				best = t;
				normal = (op + t * dp) / r;
				break ;
			}
		}
	}
	for (int s = -1; s <= 1; s += 2)
	{
		float3 cap = c + s * hh * axis;
		float t = hit_plane(o, d, cap, axis);
		if (t < best && length(o + t * d - cap) <= r)
		{
			best = t;
			normal = s * axis;
		}
	}
	return (best);
}

static float		hit_object(float3 o, float3 d, device const t_gpu_object &obj,
					thread float3 &normal)
{
	float t;

	switch (obj.type)
	{
		case GPU_SPHERE:
			t = hit_sphere(o, d, obj.a.xyz, obj.radius);
			normal = o + t * d - obj.a.xyz;
			return (t);
		case GPU_PLANE:
			normal = obj.b.xyz;
			return (hit_plane(o, d, obj.a.xyz, obj.b.xyz));
		case GPU_SQUARE:
			normal = obj.b.xyz;
			return (hit_square(o, d, obj));
		case GPU_TRIANGLE:
			normal = cross(obj.b.xyz - obj.a.xyz, obj.c.xyz - obj.a.xyz);
			return (hit_triangle(o, d, obj));
		default:
			return (hit_cylinder(o, d, obj, normal));
	}
}

#ifdef HW_RT

/*
** Version con las unidades de ray tracing de la GPU: el hardware recorre
** el arbol de cajas y llama a shape_hit para la interseccion exacta.
** Los objetos del arbol empiezan despues de los planos (de ahi el offset
** del buffer en la tabla de funciones).
*/

static float3	unpack_normal(uint v)
{
	if (v == 0)
		return (float3(0));
	float2 e = unpack_snorm2x16_to_float(v);
	float3 n = float3(e.x, e.y, 1 - fabs(e.x) - fabs(e.y));
	if (n.z < 0)
		n.xy = (1 - fabs(n.yx)) * select(float2(-1), float2(1), n.xy >= 0);
	return (normalize(n));
}

static float3	inst_rotate(float3 v, float4 it)
{
	return (float3(it.x * v.x + it.y * v.z, v.y, -it.y * v.x + it.x * v.z));
}

static float3	inst_point(float3 p, float4 it)
{
	return (inst_rotate(p, it) + float3(it.z, 0, it.w));
}

static float2	tri_uv(device const t_gpu_tri &t, float2 b)
{
	return (float2(t.uv[0], t.uv[1]) * (1 - b.x - b.y)
		+ float2(t.uv[2], t.uv[3]) * b.x + float2(t.uv[4], t.uv[5]) * b.y);
}

/*
** Hojas y demas materiales con transparencia: el hardware pregunta si el
** punto tocado del triangulo es opaco segun el canal alfa de la textura.
*/

#ifdef MESH_INST
[[intersection(triangle, triangle_data, instancing)]]
#else
[[intersection(triangle, triangle_data)]]
#endif
bool			alpha_test(uint pid [[primitive_id]],
					float2 bary [[barycentric_coord]],
					device const t_gpu_tri *tris [[buffer(0)]],
					device const GpuMaterial *mats [[buffer(1)]],
					constant uint &base [[buffer(2)]])
{
	device const t_gpu_tri &t = tris[base + pid];

	return (mats[t.material].diffuse.sample(tex_sampler, tri_uv(t, bary),
		level(0)).a >= 0.5f);
}

struct			BoxResult
{
	bool		accept [[accept_intersection]];
	float		distance [[distance]];
};

[[intersection(bounding_box)]]
BoxResult		shape_hit(float3 origin [[origin]],
					float3 direction [[direction]],
					float min_distance [[min_distance]],
					float max_distance [[max_distance]],
					uint id [[primitive_id]],
					device const t_gpu_object *objs [[buffer(0)]])
{
	float3	normal;
	float	t = hit_object(origin, direction, objs[id], normal);

	BoxResult result = {t < NOHIT && t >= min_distance && t <= max_distance, t};

	return (result);
}

static Hit		intersect(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					ACCEL_DECL, float tmax, bool any)
{
	Hit		hit = {tmax, float3(0), -1};
	float3	normal;
	float	t;

	for (int i = 0; i < f.nplanes; i++)
	{
		/*
		** Rayo de sombra que no cruza el plano: el plano no puede taparlo.
		*/
		if (any && dot(objs[i].b.xyz, o - objs[i].a.xyz)
			* dot(objs[i].b.xyz, o + d * tmax - objs[i].a.xyz) > 0)
			continue ;
		t = hit_object(o, d, objs[i], normal);
		if (t < hit.t)
		{
			hit = {t, normal, i};
			if (any)
				return (hit);
		}
	}
	if (f.ntris > 0)
	{
		intersector<MESH_TAGS> mi;
		mi.assume_geometry_type(geometry_type::triangle);
		mi.accept_any_intersection(any);
		ray mr(o, d, 0.0f, hit.t);
		intersection_result<MESH_TAGS> mres = mi.intersect(mr, mesh_accel,
			mesh_table);
		if (mres.type == intersection_type::triangle)
		{
			uint tri = (mres.geometry_id == 0 && f.nopaque > 0)
				? mres.primitive_id : f.nopaque + mres.primitive_id;
#ifdef MESH_INST
			int inst = (int)mres.instance_id;
#else
			int inst = 0;
#endif
			hit = {mres.distance, float3(0), (int)tri,
				mres.triangle_barycentric_coord, true, inst};
			if (any)
				return (hit);
		}
	}
	if (f.nobjects == f.nplanes)
		return (hit);
	/*
	** Si el rayo no toca la caja que envuelve todos los objetos, ni se
	** llama al hardware (p. ej. reflejos del suelo que suben al cielo).
	*/
	float3 inv = 1.0f / d;
	float3 t0 = (f.scene_min.xyz - o) * inv;
	float3 t1 = (f.scene_max.xyz - o) * inv;
	float3 lo = min(t0, t1);
	float3 hi = max(t0, t1);
	if (max(max(lo.x, lo.y), max(lo.z, 0.0f)) > min(min(hi.x, hi.y), min(hi.z, hit.t)))
		return (hit);
	intersector<> isect;
	isect.assume_geometry_type(geometry_type::bounding_box);
	isect.accept_any_intersection(any);
	ray r(o, d, 0.0f, hit.t);
	intersection_result<> res = isect.intersect(r, accel, table);
	if (res.type == intersection_type::bounding_box)
	{
		int i = res.primitive_id + f.nplanes;
		hit_object(o, d, objs[i], normal);
		hit = {res.distance, normal, i};
	}
	return (hit);
}

#else

/*
** Slab test: distancia de entrada a la caja, o NOHIT si no la toca antes
** de tmax.
*/

static float	hit_box(float3 o, float3 inv, device const t_gpu_node &n,
					float tmax)
{
	float3 t0 = (float3(n.minx, n.miny, n.minz) - o) * inv;
	float3 t1 = (float3(n.maxx, n.maxy, n.maxz) - o) * inv;
	float3 lo = min(t0, t1);
	float3 hi = max(t0, t1);
	float tin = max(max(lo.x, lo.y), max(lo.z, 0.0f));
	float tout = min(min(hi.x, hi.y), min(hi.z, tmax));
	return (tin <= tout ? tin : NOHIT);
}

#define STACK_SIZE 48

/*
** any = true: rayo de sombra, vale con encontrar cualquier objeto antes
** de tmax (no hace falta el mas cercano).
*/

static Hit		intersect(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					ACCEL_DECL, float tmax, bool any)
{
	Hit		hit = {tmax, float3(0), -1};
	float3	normal;
	float3	inv = 1.0f / d;
	int		stack[STACK_SIZE];
	int		sp = 0;
	float	t;

	for (int i = 0; i < f.nplanes; i++)
	{
		t = hit_object(o, d, objs[i], normal);
		if (t < hit.t)
		{
			hit = {t, normal, i};
			if (any)
				return (hit);
		}
	}
	if (f.nobjects == f.nplanes || hit_box(o, inv, nodes[0], hit.t) == NOHIT)
		return (hit);
	stack[sp++] = 0;
	while (sp > 0)
	{
		device const t_gpu_node &node = nodes[stack[--sp]];
		if (node.count > 0)
		{
			for (int i = node.first; i < node.first + node.count; i++)
			{
				t = hit_object(o, d, objs[i], normal);
				if (t < hit.t)
				{
					hit = {t, normal, i};
					if (any)
						return (hit);
				}
			}
			continue ;
		}
		float tl = hit_box(o, inv, nodes[node.first], hit.t);
		float tr = hit_box(o, inv, nodes[node.first + 1], hit.t);
		int near = node.first;
		int far = node.first + 1;
		if (tr < tl)
		{
			float tmp = tl;
			tl = tr;
			tr = tmp;
			near = far;
			far = node.first;
		}
		if (tr != NOHIT && sp < STACK_SIZE)
			stack[sp++] = far;
		if (tl != NOHIT && sp < STACK_SIZE)
			stack[sp++] = near;
	}
	return (hit);
}

#endif

/*
** Cielo de las escenas con modelo: degradado de horizonte a cenit. En las
** escenas .rt clasicas el fondo sigue siendo negro.
*/

constexpr sampler	env_sampler(address::repeat, filter::linear);

/*
** Cielo: con HDRI (f.env.y > 0) se lee la foto de 360 grados en proyeccion
** equirectangular, girada f.env.z radianes; sin HDRI, un degradado.
*/

static float3	sky(float3 d, constant t_gpu_frame &f, texture2d<float> env_map)
{
	if (f.env.y > 0)
	{
		float c = cos(f.env.z);
		float sn = sin(f.env.z);
		float3 r = float3(c * d.x + sn * d.z, d.y, -sn * d.x + c * d.z);
		float2 uv = float2(atan2(r.x, -r.z) / (2 * M_PI_F) + 0.5f,
			acos(clamp(r.y, -1.0f, 1.0f)) / M_PI_F);
		return (env_map.sample(env_sampler, uv, level(0)).rgb * f.env.x);
	}
	float	up = clamp(d.y, 0.0f, 1.0f);

	return (mix(float3(0.85f, 0.9f, 1.0f), float3(0.32f, 0.5f, 0.85f),
		pow(up, 0.6f)) * 1.2f);
}

/*
** Curva ACES (aproximacion de Narkowicz): comprime las luces altas como una
** camara en vez de recortarlas a blanco.
*/

static float3	tonemap(float3 x)
{
	return (clamp((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f)
		+ 0.14f), 0.0f, 1.0f));
}

static uint		hash(uint x)
{
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	x ^= x >> 16;
	return (x);
}

/*
** Oclusion ambiental: fraccion de f.ao_rays rayos cortos (AO_RADIUS) hacia
** el hemisferio de la normal que no chocan con nada. Distribucion coseno
** con una rotacion aleatoria por pixel.
*/

static float	ambient_occlusion(float3 p, float3 n, uint seed,
					constant t_gpu_frame &f, device const t_gpu_object *objs,
					ACCEL_DECL)
{
	float3	t = normalize(fabs(n.x) > 0.5f ? cross(n, float3(0, 1, 0))
		: cross(n, float3(1, 0, 0)));
	float3	b = cross(n, t);
	float	r0 = (hash(seed) & 0xffff) / 65536.0f;
	float	r1 = (hash(seed * 7 + 3) & 0xffff) / 65536.0f;
	int		open = 0;

	for (int i = 0; i < f.ao_rays; i++)
	{
		float u = fract(r0 + i * 0.618034f);
		float v = fract(r1 + (i + 0.5f) / f.ao_rays);
		float r = sqrt(v);
		float phi = 2 * M_PI_F * u;
		float3 dir = t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(1 - v);
		open += intersect(p, dir, f, objs, ACCEL_ARGS, AO_RADIUS, true).id < 0;
	}
	return ((float)open / f.ao_rays);
}

struct			Surf
{
	float3		p;
	float3		n;
	float3		ng;
	float3		albedo;
	float		refl;
	float		spec;
	float		rough;
	float		metal;
	bool		pbr;
	bool		glass;
	float3		ng_raw;
};

/*
** Datos de la superficie tocada: punto (ya desplazado para no chocar
** consigo mismo), normal de sombreado, normal geometrica y material.
*/

static Surf		surface(Hit hit, float3 o, float3 d, float cone,
					constant t_gpu_frame &f, device const t_gpu_object *objs,
					ACCEL_DECL, float lod_bias = 0)
{
	Surf	s;

	s.p = o + hit.t * d;
#ifdef HW_RT
	if (hit.mesh)
	{
		device const t_gpu_tri &t = tris[hit.id];
		/*
		** Instancia: giro alrededor de Y (cos, sin) y desplazamiento en X/Z.
		** Las posiciones del buffer estan en el espacio del modelo.
		*/
		float4 it = insts[hit.inst];
		float3 p0 = inst_point(mesh_pos[hit.id * 3], it);
		float3 p1 = inst_point(mesh_pos[hit.id * 3 + 1], it);
		float3 p2 = inst_point(mesh_pos[hit.id * 3 + 2], it);
		float3 cr = cross(p1 - p0, p2 - p0);
		s.ng = normalize(cr);
		s.ng_raw = s.ng;
		if (dot(s.ng, d) > 0)
			s.ng = -s.ng;
		float2 b = hit.bary;
		float3 ns = inst_rotate(unpack_normal(t.n[0]) * (1 - b.x - b.y)
			+ unpack_normal(t.n[1]) * b.x + unpack_normal(t.n[2]) * b.y, it);
		s.n = length_squared(ns) > 1e-4f ? normalize(ns) : s.ng;
		if (dot(s.n, s.ng) < 0)
			s.n = -s.n;
		device const GpuMaterial &m = mats[t.material];
		s.albedo = m.kd.rgb;
		s.glass = (m.flags & MAT_GLASS) != 0;
		s.rough = m.roughness;
		s.metal = m.metallic;
		s.pbr = true;
		/*
		** Nivel de mipmap por "cono de rayo": cuanto mas lejos y mas de
		** canto, mas grande es la huella del pixel en la textura. lod0 es
		** para una textura de 1x1; cada mapa suma su propio tamano.
		*/
		float2 e1 = float2(t.uv[2] - t.uv[0], t.uv[3] - t.uv[1]);
		float2 e2 = float2(t.uv[4] - t.uv[0], t.uv[5] - t.uv[1]);
		float uv_det = e1.x * e2.y - e2.x * e1.y;
		float lod0 = 0.5f * log2(max(fabs(uv_det), 1e-20f)
			/ max(length(cr), 1e-12f))
			+ log2(cone * f.spread / max(fabs(dot(s.ng, d)), 0.1f)) + lod_bias;
		float2 uv = tri_uv(t, b);
		if (m.flags & MAT_TEXTURE)
			s.albedo *= m.diffuse.sample(tex_sampler, uv, level(max(lod0
				+ 0.5f * log2((float)m.diffuse.get_width()
				* m.diffuse.get_height()), 0.0f))).rgb;
		if (m.flags & MAT_ROUGH)
			s.rough = m.rough.sample(tex_sampler, uv, level(max(lod0 + 0.5f
				* log2((float)m.rough.get_width() * m.rough.get_height()),
				0.0f))).r;
		if (m.flags & MAT_METAL)
			s.metal = m.metal.sample(tex_sampler, uv, level(max(lod0 + 0.5f
				* log2((float)m.metal.get_width() * m.metal.get_height()),
				0.0f))).r;
		if ((m.flags & MAT_NORMAL) && fabs(uv_det) > 1e-12f)
		{
			/*
			** Mapa de normales (convencion OpenGL): base tangente sacada de
			** las posiciones y coordenadas de textura del triangulo.
			*/
			float3 tn = m.normal.sample(tex_sampler, uv, level(max(lod0
				+ 0.5f * log2((float)m.normal.get_width()
				* m.normal.get_height()), 0.0f))).rgb * 2 - 1;
			float3 tg = ((p1 - p0) * e2.y - (p2 - p0) * e1.y) / uv_det;
			float3 bt = ((p2 - p0) * e1.x - (p1 - p0) * e2.x) / uv_det;
			tg = tg - s.n * dot(s.n, tg);
			if (length_squared(tg) > 1e-12f)
			{
				tg = normalize(tg);
				float3 bn = cross(s.n, tg) * (dot(cross(s.n, tg), bt) < 0
					? -1.0f : 1.0f);
				float3 nm = normalize(tg * tn.x + bn * tn.y + s.n * max(tn.z,
					0.05f));
				if (dot(nm, s.ng) > 0.02f)
					s.n = nm;
			}
		}
		s.rough = clamp(s.rough, 0.03f, 1.0f);
		s.metal = clamp(s.metal, 0.0f, 1.0f);
		s.refl = 0;
		s.spec = MESH_SPECULAR;
	}
	else
#endif
	{
		device const t_gpu_object &obj = objs[hit.id];
		s.n = normalize(hit.normal);
		if (dot(s.n, d) > 0)
			s.n = -s.n;
		s.ng = s.n;
		s.albedo = obj.color.xyz;
		s.refl = obj.reflect;
		s.spec = SPECULAR;
		s.rough = 1;
		s.metal = 0;
		s.pbr = false;
		s.glass = false;
		s.ng_raw = s.n;
	}
	/*
	** El error de coma flotante crece con la distancia al origen: un
	** desplazamiento fijo deja "acne" (puntos negros) en escenas grandes.
	*/
	s.p += s.ng * EPSILON * max(1.0f, length(s.p) * 0.1f);
	return (s);
}

/*
** Modelo de material fisico: difuso de Lambert + reflexion especular GGX
** (microfacetas) con Fresnel de Schlick y sombreado de Smith. Las luces van
** en unidades "pi" (difuso = albedo * luz * coseno), asi que el especular se
** multiplica por pi para mantener la proporcion.
*/

static float3	fresnel(float3 f0, float c)
{
	return (f0 + (1 - f0) * pow(clamp(1 - c, 0.0f, 1.0f), 5.0f));
}

static float	ggx_d(float nh, float a)
{
	float a2 = a * a;
	float k = nh * nh * (a2 - 1) + 1;

	return (a2 / (M_PI_F * k * k));
}

static float	smith_g1(float c, float a)
{
	float a2 = a * a;

	return (2 * c / (c + sqrt(a2 + (1 - a2) * c * c)));
}

static float3	brdf_pi(Surf s, float3 v, float3 l)
{
	float3	h = normalize(v + l);
	float	nl = max(dot(s.n, l), 0.0f);
	float	nv = max(dot(s.n, v), 1e-4f);
	float	a = s.rough * s.rough;
	float3	f0 = mix(float3(0.04f), s.albedo, s.metal);
	float3	F = fresnel(f0, max(dot(v, h), 0.0f));
	float3	spec = ggx_d(max(dot(s.n, h), 0.0f), a) * smith_g1(nl, a)
		* smith_g1(nv, a) / (4 * nv * max(nl, 1e-4f)) * F * M_PI_F;

	return ((1 - F) * (1 - s.metal) * s.albedo + spec);
}

static float	rand01(uint seed)
{
	return ((hash(seed) & 0xffffff) / 16777216.0f);
}

/*
** Caminos que rebotan en algo difuso y luego atraviesan el agua (causticas)
** dan puntos muy brillantes y raros: se recortan para que no salgan
** "chispas" en la imagen.
*/

static float3	clip_caustic(float3 c, bool caustic)
{
	return (caustic ? min(c, float3(1.5f)) : c);
}

/*
** Recorte de luciernagas: una sola muestra de luz rebotada no puede valer
** mas que INDIRECT_MAX (se escala manteniendo el color).
*/

static float3	clip_indirect(float3 c, int depth)
{
	float m = max3(c.x, c.y, c.z);
	return (depth > 0 && m > INDIRECT_MAX ? c * (INDIRECT_MAX / m) : c);
}

/*
** Sombra con superficies transparentes (agua): el rayo hacia la luz
** atraviesa el material dielectrico perdiendo un poco en cada cara y solo
** lo para algo opaco. Sin agua en la escena, un rayo "cualquier impacto".
*/

static float3	shadow_trans(float3 p, float3 l, float dist,
					constant t_gpu_frame &f, device const t_gpu_object *objs,
					ACCEL_DECL)
{
	if (f.extra.y <= 0)
		return (intersect(p, l, f, objs, ACCEL_ARGS, dist, true).id < 0
			? float3(1) : float3(0));
	float3 tr = float3(1);
	for (int k = 0; k < 8; k++)
	{
		Hit h = intersect(p, l, f, objs, ACCEL_ARGS, dist, false);
		if (h.id < 0)
			return (tr);
#ifdef HW_RT
		if (!h.mesh || !(mats[tris[h.id].material].flags & MAT_GLASS))
			return (float3(0));
#else
		return (float3(0));
#endif
		tr *= 0.92f;
		float adv = h.t + 1e-3f;
		p += l * adv;
		dist -= adv;
	}
	return (tr);
}

/*
** Luz directa: lamparas del .rt y, si hay HDRI, el sol como un disco con su
** tamano real: cada rayo de sombra apunta a un punto al azar del disco, y
** de ahi salen las sombras con penumbra.
*/

static float3	direct_light(Surf s, float3 d, bool shadows,
					constant t_gpu_frame &f, device const t_gpu_object *objs,
					device const t_gpu_light *lights, ACCEL_DECL,
					uint seed = 0)
{
	float3	local = float3(0);
	float3	v = -d;

	for (int i = 0; i < f.nlights; i++)
	{
		float3 tolight = lights[i].position.xyz - s.p;
		float dist = length(tolight);
		float3 l = tolight / dist;
		float ndl = dot(s.n, l);
		if (ndl <= 0)
			continue ;
		float3 vis = shadows ? shadow_trans(s.p, l, dist, f, objs, ACCEL_ARGS)
			: float3(1);
		if (max3(vis.x, vis.y, vis.z) <= 0)
			continue ;
		float3 lc = lights[i].color.xyz * vis;
		if (s.pbr)
		{
			local += lc * ndl * brdf_pi(s, v, l);
			continue ;
		}
		local += s.albedo * lc * ndl;
		float ndh = dot(s.n, normalize(l - d));
		if (ndh > 0)
			local += lc * s.spec * pow(ndh, SHININESS);
	}
	if (f.sun_color.w > 0)
	{
		float3	w = f.sun_dir.xyz;
		float3	t = normalize(fabs(w.y) < 0.9f ? cross(w, float3(0, 1, 0))
			: cross(w, float3(1, 0, 0)));
		float3	b = cross(w, t);
		float	u1 = rand01(seed * 3 + 11);
		float	u2 = rand01(seed * 5 + 29);
		float	ct = 1 - u1 * (1 - f.sun_dir.w);
		float	st = sqrt(max(0.0f, 1 - ct * ct));
		float3	l = normalize(w * ct + t * (st * cos(2 * M_PI_F * u2))
			+ b * (st * sin(2 * M_PI_F * u2)));
		float	ndl = dot(s.n, l);
		if (ndl > 0 && dot(s.ng, l) > 0)
		{
			float3 vis = shadows ? shadow_trans(s.p, l, NOHIT, f, objs,
				ACCEL_ARGS) : float3(1);
			local += f.sun_color.rgb * vis * ndl * (s.pbr ? brdf_pi(s, v, l)
				: s.albedo);
		}
	}
	return (local);
}

static float3	trace(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					device const t_gpu_light *lights,
					ACCEL_DECL, uint seed, thread float &first_t)
{
	float3 color = float3(0);
	first_t = NOHIT;
	float3 weight = float3(1);
	float cone = 0;

	for (int depth = 0; depth <= MAX_DEPTH; depth++)
	{
		Hit hit = intersect(o, d, f, objs, ACCEL_ARGS, NOHIT, false);
		if (hit.id < 0)
		{
			if (f.ntris > 0)
				color += weight * sky(d, f, env_map);
			break ;
		}
		cone += hit.t;
		if (depth == 0)
			first_t = hit.t;
		Surf s = surface(hit, o, d, cone, f, objs, ACCEL_ARGS);
		float3 local = s.albedo * f.ambient.xyz;
		if (f.ntris > 0 && depth == 0)
			local *= ambient_occlusion(s.p, s.n, seed, f, objs, ACCEL_ARGS);
		bool shadows = max3(weight.x, weight.y, weight.z) >= SHADOW_MIN_WEIGHT;
		local += direct_light(s, d, shadows, f, objs, lights, ACCEL_ARGS);
		if (s.refl > 0 && depth < MAX_DEPTH)
		{
			color += weight * (1 - s.refl) * local;
			weight *= s.refl;
			o = s.p;
			d = reflect(d, s.n);
		}
		else
		{
			color += weight * local;
			break ;
		}
	}
	return (color);
}

static float3	cosine_dir(float3 n, uint seed)
{
	float3	t = normalize(fabs(n.x) > 0.5f ? cross(n, float3(0, 1, 0))
		: cross(n, float3(1, 0, 0)));
	float3	b = cross(n, t);
	float	u = (hash(seed) & 0xffffff) / 16777216.0f;
	float	v = (hash(seed * 31 + 17) & 0xffffff) / 16777216.0f;
	float	r = sqrt(v);
	float	phi = 2 * M_PI_F * u;

	return (t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(1 - v));
}

/*
** Direccion reflejada muestreando la distribucion GGX (microfaceta al azar
** alrededor de la normal segun la rugosidad).
*/

static float3	ggx_dir(float3 n, float3 v, float a, uint seed,
					thread float3 &h)
{
	float	u1 = rand01(seed * 13 + 7);
	float	u2 = rand01(seed * 17 + 3);
	float	ct = sqrt((1 - u1) / (1 + (a * a - 1) * u1));
	float	st = sqrt(max(0.0f, 1 - ct * ct));
	float3	t = normalize(fabs(n.x) > 0.5f ? cross(n, float3(0, 1, 0))
		: cross(n, float3(1, 0, 0)));
	float3	b = cross(n, t);

	h = normalize(t * (st * cos(2 * M_PI_F * u2)) + b * (st * sin(2 * M_PI_F
		* u2)) + n * ct);
	return (reflect(-v, h));
}

/*
** Iluminacion global: luz directa (lamparas, sol con penumbra) + GI_BOUNCES
** rebotes. En cada rebote se elige entre la parte difusa (direccion coseno)
** y la especular (direccion GGX), con mas probabilidad de especular cuanto
** mas metalico y liso es el material. Devuelve tambien las guias del
** denoiser del primer impacto.
*/

static float3	trace_gi(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					device const t_gpu_light *lights, ACCEL_DECL, uint seed,
					thread float &first_t, thread float3 &first_albedo,
					thread float3 &first_normal, thread float3 &first_spec,
					thread float &first_rough)
{
	float3	color = float3(0);
	float3	weight = float3(1);
	float	cone = 0;

	first_t = NOHIT;
	first_albedo = float3(1);
	first_normal = float3(0, 0, 1);
	first_spec = float3(0.04f);
	first_rough = 1;
	int		depth = 0;
	bool	inside = false;
	bool	caustic = false;
	/*
	** "depth" cuenta solo los rebotes en superficies normales; atravesar
	** agua (entrar, salir, reflejarse en ella) no gasta rebotes, hasta un
	** maximo de eventos.
	*/
	for (int event = 0; event < MAX_EVENTS && depth <= GI_BOUNCES; event++)
	{
		Hit hit = intersect(o, d, f, objs, ACCEL_ARGS, NOHIT, false);
		if (hit.id < 0)
		{
			color += clip_indirect(clip_caustic(weight * sky(d, f, env_map)
				* (depth == 0 || f.env.y > 0 ? 1.0f : SKY_LIGHT), caustic), depth);
			break ;
		}
		if (inside)
			weight *= exp(-WATER_SIGMA * hit.t);
		cone += hit.t;
		/*
		** La luz rebotada no necesita texturas nitidas: en los rebotes se lee
		** un nivel de mipmap mas borroso (menos memoria).
		*/
		Surf s = surface(hit, o, d, cone, f, objs, ACCEL_ARGS,
			depth > 0 ? BOUNCE_LOD_BIAS : 0.0f);
		float3 f0 = mix(float3(0.04f), s.albedo, s.metal);
		if (event == 0)
		{
			first_t = hit.t;
			first_albedo = s.albedo * (1 - s.metal);
			first_normal = s.n;
			first_spec = f0;
			first_rough = s.rough;
		}
		if (s.glass)
		{
			/*
			** Agua: brillo del sol en la superficie y, segun Fresnel,
			** reflexion o refraccion (indice 1,33). Dentro, la luz se
			** absorbe un poco (Beer-Lambert, algo mas el rojo).
			*/
			float3 hp = o + hit.t * d;
			float sc = EPSILON * max(1.0f, length(hp) * 0.1f);
			bool entering = dot(d, s.ng_raw) < 0;
			float3 nn = entering ? s.ng_raw : -s.ng_raw;
			float3 ns = dot(s.n, nn) > 0 ? s.n : -s.n;
			if (!inside)
			{
				Surf sp = s;
				sp.albedo = float3(0);
				sp.metal = 0;
				sp.rough = 0.04f;
				sp.pbr = true;
				sp.n = ns;
				sp.p = hp + nn * sc;
				color += clip_caustic(weight * direct_light(sp, d, true, f, objs,
					lights, ACCEL_ARGS, seed + event * 7919), caustic);
			}
			caustic = caustic || depth > 0;
			float cosi = clamp(-dot(d, ns), 0.0f, 1.0f);
			float fr = 0.02f + 0.98f * pow(1 - cosi, 5.0f);
			float3 rd = refract(d, ns, entering ? 1 / 1.33f : 1.33f);
			if (length_squared(rd) < 1e-8f
				|| rand01(seed * 23 + event * 41 + 9) < fr)
			{
				d = reflect(d, ns);
				o = hp + nn * sc;
			}
			else
			{
				d = normalize(rd);
				o = hp - nn * sc;
				inside = entering;
			}
			continue ;
		}
		color += clip_indirect(clip_caustic(weight * direct_light(s, d, true, f,
			objs, lights, ACCEL_ARGS, seed + event * 7919), caustic), depth);
		if (depth == GI_BOUNCES)
			break ;
		depth++;
		float3 v = -d;
		/*
		** Probabilidad de seguir el reflejo segun cuanto aporta de verdad:
		** Fresnel al angulo de vision frente a lo que aporta el difuso. Una
		** madera oscura vista de canto refleja mucho; vista de frente, casi
		** nada. Asi cada camino lleva un peso parecido y hay menos grano.
		*/
		float ps = 0;
		if (s.pbr)
		{
			float3 fv = fresnel(f0, max(dot(s.n, v), 0.0f));
			float es = dot(fv, float3(0.2126f, 0.7152f, 0.0722f));
			float ed = dot(s.albedo * (1 - s.metal) * (1 - fv),
				float3(0.2126f, 0.7152f, 0.0722f));
			ps = clamp(es / max(es + ed, 1e-4f), 0.05f, 0.95f);
		}
		o = s.p;
		if (rand01(seed * 19 + event * 31 + 5) < ps)
		{
			float3 h;
			float a = s.rough * s.rough;
			d = ggx_dir(s.n, v, a, seed + event * 4513, h);
			float nl = dot(s.n, d);
			float nv = max(dot(s.n, v), 1e-4f);
			if (nl <= 0 || dot(d, s.ng) <= 0)
				break ;
			float vh = max(dot(v, h), 1e-4f);
			weight *= fresnel(f0, vh) * smith_g1(nl, a) * smith_g1(nv, a) * vh
				/ (nv * max(dot(s.n, h), 1e-4f)) / ps;
		}
		else
		{
			d = cosine_dir(s.n, seed + event * 7919);
			if (dot(d, s.ng) <= 0)
				break ;
			weight *= s.albedo * (1 - s.metal) * (s.pbr ? 1 - fresnel(f0,
				max(dot(s.n, v), 0.0f)) : float3(1)) / (1 - ps);
		}
	}
	return (color);
}

static float	channel_diff(float4 a, float4 b)
{
	float3 d = fabs(a.rgb - b.rgb);

	return (max3(d.x, d.y, d.z));
}

/*
** pass 0: normal (f.samples x f.samples rayos por pixel).
** pass 1 y 2: antialiasing adaptativo. El 1 calcula 1 rayo por pixel; el
** 2 solo repite con f.samples x f.samples los pixeles que difieren de algun
** vecino (bordes); el resto se copia tal cual.
*/

kernel void		render(texture2d<float, access::write> out [[texture(0)]],
					texture2d<float, access::read> base [[texture(1)]],
					texture2d<float, access::write> depth_out [[texture(2)]],
					texture2d<float, access::write> motion_out [[texture(3)]],
					texture2d<float, access::write> albedo_out [[texture(4)]],
					texture2d<float, access::write> normal_out [[texture(5)]],
					texture2d<float, access::write> rough_out [[texture(6)]],
					texture2d<float, access::write> spec_out [[texture(7)]],
					constant t_gpu_frame &f [[buffer(0)]],
					device const t_gpu_object *objs [[buffer(1)]],
					device const t_gpu_light *lights [[buffer(2)]],
#ifdef HW_RT
					primitive_acceleration_structure accel [[buffer(3)]],
					intersection_function_table<> table [[buffer(4)]],
					MESH_ACCEL mesh_accel [[buffer(8)]],
					intersection_function_table<MESH_TAGS> mesh_table [[buffer(9)]],
					device const t_gpu_tri *tris [[buffer(10)]],
					device const GpuMaterial *mats [[buffer(11)]],
					device const packed_float3 *mesh_pos [[buffer(12)]],
					device const float4 *insts [[buffer(13)]],
#endif
					texture2d<float> env_map [[texture(8)]],
#ifdef HW_RT
#else
					device const t_gpu_node *nodes [[buffer(3)]],
#endif
					uint2 gid [[thread_position_in_grid]])
{
	float w = out.get_width();
	float h = out.get_height();

	if (gid.x >= w || gid.y >= h)
		return ;
	if (f.pass == 5)
	{
		/*
		** Salida del denoiser (luz lineal) -> tonemapping + gamma.
		*/
		float3 c = tonemap(base.read(gid).rgb * (f.extra.x > 0 ? f.extra.x
			: 1.0f));
		out.write(float4(pow(c, 1 / GAMMA), 1), gid);
		return ;
	}
	if (f.pass == 4)
	{
		/*
		** Iluminacion global para el denoiser de MetalFX: luz lineal con
		** grano mas las guias (color de superficie, normal, rugosidad,
		** profundidad de recorte y movimiento).
		*/
		float depth = w / (2 * tan(f.fov * M_PI_F / 360));
		float px = gid.x + f.jitter.x;
		float py = gid.y + f.jitter.y;
		float3 d = normalize(f.forward.xyz * depth
			+ f.right.xyz * (px - w / 2) + f.up.xyz * (h / 2 - py));
		float first_t;
		float3 alb;
		float3 nrm;
		float3 spc;
		float rgh;
		float3 o = f.origin.xyz;
		if (f.extra.w > 0)
		{
			/*
			** Lente fina: el rayo sale de un punto al azar de la apertura y
			** pasa por el punto de enfoque; lo que no esta a esa distancia
			** sale desenfocado, como con una camara de verdad.
			*/
			uint ls = hash(gid.x * 7919 + gid.y * 104723 + f.frame * 15485863);
			float r = sqrt(rand01(ls)) * f.extra.w;
			float a = 2 * M_PI_F * rand01(ls * 7 + 3);
			float3 fw = normalize(f.forward.xyz);
			float3 fp = o + d * (f.extra.z / dot(d, fw));
			o += normalize(f.right.xyz) * (r * cos(a))
				+ normalize(f.up.xyz) * (r * sin(a));
			d = normalize(fp - o);
		}
		float3 c = trace_gi(o, d, f, objs, lights, ACCEL_ARGS,
			hash(gid.x * 1973 + gid.y * 9277 + f.frame * 104729), first_t,
			alb, nrm, spc, rgh);
		float3 lit = c * (f.env.y > 0 ? f.env.w : GI_EXPOSURE);
		/*
		** Modo acumulacion (video de calidad sin denoiser): se suma la
		** muestra a lo acumulado (base) y se escribe en out (ping-pong).
		*/
		if (f.accum > 0)
		{
			float4 prev = f.accum == 2 ? base.read(gid) : float4(0);
			out.write(prev + float4(lit, 1), gid);
			return ;
		}
		out.write(float4(lit, 1), gid);
		albedo_out.write(float4(alb, 1), gid);
		normal_out.write(float4(nrm, 0), gid);
		rough_out.write(float4(rgh), gid);
		spec_out.write(float4(spc, 1), gid);
		float3 world = f.origin.xyz + d * min(first_t, 1e5f);
		float3 v = world - f.prev_origin.xyz;
		float z = dot(v, f.prev_forward.xyz);
		float2 prev = float2(dot(v, f.prev_right.xyz), -dot(v, f.prev_up.xyz))
			* (depth / max(z, 1e-4f)) + float2(w / 2, h / 2);
		float2 motion = z > 1e-4f ? prev - float2(px, py) : float2(0);
		motion_out.write(float4(motion / float2(w, h), 0, 0), gid);
		float vz = min(first_t, 1e5f) * dot(d, f.forward.xyz);
		depth_out.write(float4(first_t < NOHIT ? clamp(CLIP_FAR * (vz - CLIP_NEAR)
			/ ((CLIP_FAR - CLIP_NEAR) * vz), 0.0f, 1.0f) : 1.0f), gid);
		return ;
	}
	if (f.pass == 3)
	{
		/*
		** Entrada del reescalado temporal (MetalFX): 1 rayo desplazado por
		** el jitter de esta imagen, mas la profundidad (Z invertida) y el
		** vector de movimiento: donde estaba este punto en la imagen anterior.
		*/
		float depth = w / (2 * tan(f.fov * M_PI_F / 360));
		float px = gid.x + f.jitter.x;
		float py = gid.y + f.jitter.y;
		float3 d = normalize(f.forward.xyz * depth
			+ f.right.xyz * (px - w / 2) + f.up.xyz * (h / 2 - py));
		float first_t;
		float3 c = trace(f.origin.xyz, d, f, objs, lights, ACCEL_ARGS,
			hash(gid.x * 1973 + gid.y * 9277 + f.frame * 104729), first_t);
		c = f.ntris > 0 ? tonemap(c * EXPOSURE) : c;
		out.write(float4(pow(clamp(c, 0.0, 1.0), 1 / GAMMA), 1), gid);
		float3 world = f.origin.xyz + d * min(first_t, 1e5f);
		float3 v = world - f.prev_origin.xyz;
		float z = dot(v, f.prev_forward.xyz);
		float2 prev = float2(dot(v, f.prev_right.xyz), -dot(v, f.prev_up.xyz))
			* (depth / max(z, 1e-4f)) + float2(w / 2, h / 2);
		float2 motion = z > 1e-4f ? prev - float2(px, py) + f.jitter.zw
			: float2(0);
		motion_out.write(float4(motion / float2(w, h), 0, 0), gid);
		depth_out.write(float4(first_t < NOHIT ? clamp(0.05f / first_t, 0.0f,
			1.0f) : 0.0f), gid);
		return ;
	}
	if (f.pass == 2)
	{
		float4 c = base.read(gid);
		uint2 hi = uint2(w - 1, h - 1);
		float diff = 0;
		diff = max(diff, channel_diff(c, base.read(uint2(max((int)gid.x - 1, 0), gid.y))));
		diff = max(diff, channel_diff(c, base.read(uint2(min(gid.x + 1, hi.x), gid.y))));
		diff = max(diff, channel_diff(c, base.read(uint2(gid.x, max((int)gid.y - 1, 0)))));
		diff = max(diff, channel_diff(c, base.read(uint2(gid.x, min(gid.y + 1, hi.y)))));
		if (diff < EDGE)
		{
			out.write(c, gid);
			return ;
		}
	}
	int samples = f.pass == 1 ? 1 : f.samples;
	float depth = w / (2 * tan(f.fov * M_PI_F / 360));
	float3 color = float3(0);
	/*
	** En el refinado con un numero impar de muestras, la central coincide
	** con el rayo de la primera pasada: se reutiliza en vez de repetirlo.
	*/
	int reuse = f.pass == 2 && samples % 2 == 1 ? samples / 2 : -1;
	if (reuse >= 0)
		color = pow(base.read(gid).rgb, GAMMA);
	for (int sy = 0; sy < samples; sy++)
		for (int sx = 0; sx < samples; sx++)
		{
			if (sx == reuse && sy == reuse)
				continue ;
			float px = gid.x + (sx + 0.5) / samples - 0.5;
			float py = gid.y + (sy + 0.5) / samples - 0.5;
			float3 d = normalize(f.forward.xyz * depth
				+ f.right.xyz * (px - w / 2) + f.up.xyz * (h / 2 - py));
			float first_t;
			float3 c = trace(f.origin.xyz, d, f, objs, lights, ACCEL_ARGS,
				hash(gid.x * 1973 + gid.y * 9277 + (sy * samples + sx) * 26699
				+ f.frame * 104729), first_t);
			color += f.ntris > 0 ? tonemap(c * EXPOSURE) : c;
		}
	color /= samples * samples;
	out.write(float4(pow(clamp(color, 0.0, 1.0), 1 / GAMMA), 1), gid);
}
