/*
** Tipos compartidos entre la CPU (C / Objective-C) y la GPU (Metal).
** Solo se usan vec4 y escalares de 4 bytes para que el layout de memoria
** sea identico en los dos lados.
*/

#ifndef SHARED_H
# define SHARED_H

# ifdef __METAL_VERSION__

typedef float4	t_vec4;

# else
#  include <simd/simd.h>

typedef simd_float4	t_vec4;

# endif

# define GPU_SPHERE 0
# define GPU_PLANE 1
# define GPU_SQUARE 2
# define GPU_CYLINDER 3
# define GPU_TRIANGLE 4

/*
** a: centro (esfera, cuadrado, cilindro), punto (plano) o vertice 1
** b: normal (plano, cuadrado), eje (cilindro) o vertice 2
** c: eje u del cuadrado o vertice 3
** color: rgb lineal (ya corregido de gamma)
*/

typedef struct	s_gpu_object
{
	t_vec4		a;
	t_vec4		b;
	t_vec4		c;
	t_vec4		color;
	float		radius;
	float		height;
	float		reflect;
	int			type;
}				t_gpu_object;

/*
** Nodo del BVH (32 bytes). Si count > 0 es una hoja con los objetos
** [first, first + count); si no, sus hijos son first y first + 1.
*/

typedef struct	s_gpu_node
{
	float		minx;
	float		miny;
	float		minz;
	int			first;
	float		maxx;
	float		maxy;
	float		maxz;
	int			count;
}				t_gpu_node;

/*
** Atributos de un triangulo de malla (.obj), 40 bytes: coordenadas de
** textura de sus 3 vertices, normales de vertice comprimidas (octaedro,
** 2 x 16 bits) e indice de material.
*/

typedef struct	s_gpu_tri
{
	float		uv[6];
	unsigned int	n[3];
	unsigned int	material;
}				t_gpu_tri;

# define MAT_TEXTURE 1
# define MAT_ALPHA 2
# define MAT_NORMAL 4
# define MAT_ROUGH 8
# define MAT_METAL 16
# define MAT_GLASS 32

typedef struct	s_gpu_light
{
	t_vec4		position;
	t_vec4		color;
}				t_gpu_light;

typedef struct	s_gpu_frame
{
	t_vec4		origin;
	t_vec4		forward;
	t_vec4		right;
	t_vec4		up;
	t_vec4		ambient;
	float		fov;
	int			nobjects;
	int			nlights;
	int			samples;
	int			nplanes;
	int			pass;
	int			ntris;
	int			nopaque;
	float		spread;
	int			ao_rays;
	unsigned int	frame;
	int			accum;
	t_vec4		scene_min;
	t_vec4		scene_max;
	t_vec4		prev_origin;
	t_vec4		prev_forward;
	t_vec4		prev_right;
	t_vec4		prev_up;
	t_vec4		jitter;
	t_vec4		sun_dir;
	t_vec4		sun_color;
	t_vec4		env;
	t_vec4		extra;
}				t_gpu_frame;

#endif
