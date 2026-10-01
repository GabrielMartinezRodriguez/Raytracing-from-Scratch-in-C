#include <metal_stdlib>
using namespace metal;
#ifdef HW_RT
# include <metal_raytracing>
using namespace raytracing;
# define ACCEL_DECL primitive_acceleration_structure accel, \
	intersection_function_table<> table, \
	primitive_acceleration_structure mesh_accel, \
	intersection_function_table<triangle_data> mesh_table, \
	device const t_gpu_tri *tris, device const GpuMaterial *mats, \
	device const packed_float3 *mesh_pos
# define ACCEL_ARGS accel, table, mesh_accel, mesh_table, tris, mats, mesh_pos
#else
# define ACCEL_DECL device const t_gpu_node *nodes
# define ACCEL_ARGS nodes
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
constant int	AO_RAYS = 8;
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

static float2	tri_uv(device const t_gpu_tri &t, float2 b)
{
	return (float2(t.uv[0], t.uv[1]) * (1 - b.x - b.y)
		+ float2(t.uv[2], t.uv[3]) * b.x + float2(t.uv[4], t.uv[5]) * b.y);
}

/*
** Hojas y demas materiales con transparencia: el hardware pregunta si el
** punto tocado del triangulo es opaco segun el canal alfa de la textura.
*/

[[intersection(triangle, triangle_data)]]
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
		intersector<triangle_data> mi;
		mi.assume_geometry_type(geometry_type::triangle);
		mi.accept_any_intersection(any);
		ray mr(o, d, 0.0f, hit.t);
		intersection_result<triangle_data> mres = mi.intersect(mr, mesh_accel,
			mesh_table);
		if (mres.type == intersection_type::triangle)
		{
			uint tri = (mres.geometry_id == 0 && f.nopaque > 0)
				? mres.primitive_id : f.nopaque + mres.primitive_id;
			hit = {mres.distance, float3(0), (int)tri,
				mres.triangle_barycentric_coord, true};
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

static float3	sky(float3 d)
{
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
** Oclusion ambiental: fraccion de AO_RAYS rayos cortos (AO_RADIUS) hacia
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

	for (int i = 0; i < AO_RAYS; i++)
	{
		float u = fract(r0 + i * 0.618034f);
		float v = fract(r1 + (i + 0.5f) / AO_RAYS);
		float r = sqrt(v);
		float phi = 2 * M_PI_F * u;
		float3 dir = t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(1 - v);
		open += intersect(p, dir, f, objs, ACCEL_ARGS, AO_RADIUS, true).id < 0;
	}
	return ((float)open / AO_RAYS);
}

static float3	trace(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					device const t_gpu_light *lights,
					ACCEL_DECL, uint seed)
{
	float3 color = float3(0);
	float3 weight = float3(1);
	float cone = 0;

	for (int depth = 0; depth <= MAX_DEPTH; depth++)
	{
		Hit hit = intersect(o, d, f, objs, ACCEL_ARGS, NOHIT, false);
		if (hit.id < 0)
		{
			if (f.ntris > 0)
				color += weight * sky(d);
			break ;
		}
		float3 n;
		float3 ng;
		float3 albedo;
		float refl;
		float spec;
		float3 p = o + hit.t * d;
		cone += hit.t;
#ifdef HW_RT
		if (hit.mesh)
		{
			device const t_gpu_tri &t = tris[hit.id];
			float3 p0 = mesh_pos[hit.id * 3];
			float3 p1 = mesh_pos[hit.id * 3 + 1];
			float3 p2 = mesh_pos[hit.id * 3 + 2];
			float3 cr = cross(p1 - p0, p2 - p0);
			ng = normalize(cr);
			if (dot(ng, d) > 0)
				ng = -ng;
			float2 b = hit.bary;
			float3 ns = unpack_normal(t.n[0]) * (1 - b.x - b.y)
				+ unpack_normal(t.n[1]) * b.x + unpack_normal(t.n[2]) * b.y;
			n = length_squared(ns) > 1e-4f ? normalize(ns) : ng;
			if (dot(n, ng) < 0)
				n = -n;
			device const GpuMaterial &m = mats[t.material];
			albedo = m.kd.rgb;
			if (m.flags & MAT_TEXTURE)
			{
				/*
				** Nivel de mipmap por "cono de rayo": cuanto mas lejos y mas
				** de canto, mas grande es la huella del pixel en la textura.
				*/
				float2 e1 = float2(t.uv[2] - t.uv[0], t.uv[3] - t.uv[1]);
				float2 e2 = float2(t.uv[4] - t.uv[0], t.uv[5] - t.uv[1]);
				float uv_area = fabs(e1.x * e2.y - e2.x * e1.y)
					* m.diffuse.get_width() * m.diffuse.get_height();
				float lod = 0.5f * log2(uv_area / max(length(cr), 1e-12f))
					+ log2(cone * f.spread / max(fabs(dot(ng, d)), 0.1f));
				albedo *= m.diffuse.sample(tex_sampler, tri_uv(t, b),
					level(max(lod, 0.0f))).rgb;
			}
			refl = 0;
			spec = MESH_SPECULAR;
		}
		else
#endif
		{
			device const t_gpu_object &obj = objs[hit.id];
			n = normalize(hit.normal);
			if (dot(n, d) > 0)
				n = -n;
			ng = n;
			albedo = obj.color.xyz;
			refl = obj.reflect;
			spec = SPECULAR;
		}
		/*
		** El error de coma flotante crece con la distancia al origen: un
		** desplazamiento fijo deja "acne" (puntos negros) en escenas grandes.
		*/
		p += ng * EPSILON * max(1.0f, length(p) * 0.1f);
		float3 local = albedo * f.ambient.xyz;
		if (f.ntris > 0 && depth == 0)
			local *= ambient_occlusion(p, n, seed, f, objs, ACCEL_ARGS);
		bool shadows = max3(weight.x, weight.y, weight.z) >= SHADOW_MIN_WEIGHT;
		for (int i = 0; i < f.nlights; i++)
		{
			float3 tolight = lights[i].position.xyz - p;
			float dist = length(tolight);
			float3 l = tolight / dist;
			float ndl = dot(n, l);
			if (ndl <= 0 || (shadows && intersect(p, l, f, objs, ACCEL_ARGS, dist, true).id >= 0))
				continue ;
			float3 lc = lights[i].color.xyz;
			local += albedo * lc * ndl;
			float ndh = dot(n, normalize(l - d));
			if (ndh > 0)
				local += lc * spec * pow(ndh, SHININESS);
		}
		if (refl > 0 && depth < MAX_DEPTH)
		{
			color += weight * (1 - refl) * local;
			weight *= refl;
			o = p;
			d = reflect(d, n);
		}
		else
		{
			color += weight * local;
			break ;
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
					constant t_gpu_frame &f [[buffer(0)]],
					device const t_gpu_object *objs [[buffer(1)]],
					device const t_gpu_light *lights [[buffer(2)]],
#ifdef HW_RT
					primitive_acceleration_structure accel [[buffer(3)]],
					intersection_function_table<> table [[buffer(4)]],
					primitive_acceleration_structure mesh_accel [[buffer(8)]],
					intersection_function_table<triangle_data> mesh_table [[buffer(9)]],
					device const t_gpu_tri *tris [[buffer(10)]],
					device const GpuMaterial *mats [[buffer(11)]],
					device const packed_float3 *mesh_pos [[buffer(12)]],
#else
					device const t_gpu_node *nodes [[buffer(3)]],
#endif
					uint2 gid [[thread_position_in_grid]])
{
	float w = out.get_width();
	float h = out.get_height();

	if (gid.x >= w || gid.y >= h)
		return ;
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
			float3 c = trace(f.origin.xyz, d, f, objs, lights, ACCEL_ARGS,
				hash(gid.x * 1973 + gid.y * 9277 + (sy * samples + sx) * 26699));
			color += f.ntris > 0 ? tonemap(c * EXPOSURE) : c;
		}
	color /= samples * samples;
	out.write(float4(pow(clamp(color, 0.0, 1.0), 1 / GAMMA), 1), gid);
}
