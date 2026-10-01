#include <metal_stdlib>
using namespace metal;
#ifdef HW_RT
# include <metal_raytracing>
using namespace raytracing;
# define ACCEL_DECL primitive_acceleration_structure accel, intersection_function_table<> table
# define ACCEL_ARGS accel, table
#else
# define ACCEL_DECL device const t_gpu_node *nodes
# define ACCEL_ARGS nodes
#endif

#include "shared.h"

/*
** Port a Metal de generateImage/shading.c: mismas intersecciones y mismo
** sombreado (Blinn-Phong + sombras + reflejos + supersampling + gamma).
** Cada hilo de la GPU calcula un pixel.
*/

constant int	MAX_DEPTH = 5;
constant float	SPECULAR = 0.4;
constant float	SHININESS = 60;
constant float	GAMMA = 2.2;
constant float	EPSILON = 1e-3;
constant float	SHADOW_MIN_WEIGHT = 0.25;
constant float	TMIN = 1e-3;
constant float	NOHIT = 1e30f;

struct			Hit
{
	float		t;
	float3		normal;
	int			id;
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
	float h = b * b - (dot(oc, oc) - r * r);

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
		t = hit_object(o, d, objs[i], normal);
		if (t < hit.t)
		{
			hit = {t, normal, i};
			if (any)
				return (hit);
		}
	}
	if (f.nobjects == f.nplanes)
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

static float3	trace(float3 o, float3 d, constant t_gpu_frame &f,
					device const t_gpu_object *objs,
					device const t_gpu_light *lights,
					ACCEL_DECL)
{
	float3 color = float3(0);
	float3 weight = float3(1);

	for (int depth = 0; depth <= MAX_DEPTH; depth++)
	{
		Hit hit = intersect(o, d, f, objs, ACCEL_ARGS, NOHIT, false);
		if (hit.id < 0)
			break ;
		device const t_gpu_object &obj = objs[hit.id];
		float3 n = normalize(hit.normal);
		if (dot(n, d) > 0)
			n = -n;
		float3 p = o + hit.t * d + n * EPSILON;
		float3 albedo = obj.color.xyz;
		float3 local = albedo * f.ambient.xyz;
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
				local += lc * SPECULAR * pow(ndh, SHININESS);
		}
		if (obj.reflect > 0 && depth < MAX_DEPTH)
		{
			color += weight * (1 - obj.reflect) * local;
			weight *= obj.reflect;
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

kernel void		render(texture2d<float, access::write> out [[texture(0)]],
					constant t_gpu_frame &f [[buffer(0)]],
					device const t_gpu_object *objs [[buffer(1)]],
					device const t_gpu_light *lights [[buffer(2)]],
#ifdef HW_RT
					primitive_acceleration_structure accel [[buffer(3)]],
					intersection_function_table<> table [[buffer(4)]],
#else
					device const t_gpu_node *nodes [[buffer(3)]],
#endif
					uint2 gid [[thread_position_in_grid]])
{
	float w = out.get_width();
	float h = out.get_height();

	if (gid.x >= w || gid.y >= h)
		return ;
	float depth = w / (2 * tan(f.fov * M_PI_F / 360));
	float3 color = float3(0);
	for (int sy = 0; sy < f.samples; sy++)
		for (int sx = 0; sx < f.samples; sx++)
		{
			float px = gid.x + (sx + 0.5) / f.samples - 0.5;
			float py = gid.y + (sy + 0.5) / f.samples - 0.5;
			float3 d = normalize(f.forward.xyz * depth
				+ f.right.xyz * (px - w / 2) + f.up.xyz * (h / 2 - py));
			color += trace(f.origin.xyz, d, f, objs, lights, ACCEL_ARGS);
		}
	color /= f.samples * f.samples;
	out.write(float4(pow(clamp(color, 0.0, 1.0), 1 / GAMMA), 1), gid);
}
