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
}				t_gpu_frame;

#endif
