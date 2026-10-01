#ifndef SIM_H
# define SIM_H
# include "obj_loader.h"
# include "physics.h"

/*
** Escenarios animados sobre un modelo .obj: que objetos son cuerpos
** rigidos, como empiezan y que pasa con el tiempo. sim_setup puede anadir
** triangulos al modelo (copias de objetos, tela, liquido) antes de subirlo
** a la GPU; sim_apply mueve los vertices de cada imagen.
*/

/*
** Tela: rejilla de particulas con restricciones de distancia (estructura,
** cizalla y flexion), resuelta con Position Based Dynamics.
*/

typedef struct	s_cloth
{
	int			n;
	simd_float3	*x;
	simd_float3	*p;
	simd_float3	*v;
	int			(*cons)[2];
	float		*rest;
	float		*stiff;
	int			ncons;
	simd_float3	*nrm;
	simd_float3	*hitn;
	int			obj;
	float		size;
}				t_cloth;

typedef struct	s_sim
{
	t_world		w;
	int			*obj_body;
	int			nobjs;
	float		time;
	int			kind;
	int			wind_obj;
	float		wind;
	simd_float3	wind_base;
	float		wind_height;
	t_cloth		cloth;
	int			vase_body;
	int			shard_obj[32];
	int			nshards;
	int			broken;
	simd_float3	vase_vel;
	float		**shard_tris;
	int			*shard_ntris;
}				t_sim;

int				sim_setup(t_sim *s, const char *name, t_gpu_mesh *mesh);
void			sim_advance(t_sim *s, float dt);
void			sim_apply(t_sim *s, const uint32_t *tri_obj,
					const float *rest_pos, const t_gpu_tri *rest_tris,
					float *pos, t_gpu_tri *tris, size_t n);
void			mesh_detach(t_gpu_mesh *mesh);

#endif
