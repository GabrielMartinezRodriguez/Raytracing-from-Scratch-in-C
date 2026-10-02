#ifndef FLUID_H
# define FLUID_H
# include "physics.h"

/*
** Liquido con Position Based Fluids (Macklin y Muller, 2013), desde cero:
** particulas que intentan mantener su densidad de reposo (incompresible),
** viscosidad XSPH y colision con el entorno estatico del motor de fisica.
** La superficie se reconstruye cada imagen con marching tetrahedra sobre un
** campo de densidad.
*/

typedef struct	s_fluid
{
	int			n;
	int			cap;
	simd_float3	*x;
	simd_float3	*p;
	simd_float3	*v;
	simd_float3	*dp;
	simd_float3	*hitn;
	float		*lambda;
	int			*nbr;
	int			*nnbr;
	float		d0;
	float		h;
	float		rho0;
	float		eps;
	simd_float3	emit_pos;
	simd_float3	emit_vel;
	float		emit_radius;
	float		emit_until;
	float		emit_acc;
	float		time;
	float		*tri;
	unsigned	*tri_n;
	int			ntri;
	int			tri_cap;
	float		*wet;
	int			wet_n;
	float		wet_cell;
	simd_float2	wet_lo;
}				t_fluid;

void			fluid_init(t_fluid *f, float spacing, int cap);
void			fluid_step(t_fluid *f, t_world *w, float dt);
void			fluid_surface(t_fluid *f);

#endif
