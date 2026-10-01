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

typedef struct	s_sim
{
	t_world		w;
	int			*obj_body;
	int			nobjs;
	float		time;
	int			kind;
	int			wind_obj;
	float		wind;
}				t_sim;

int				sim_setup(t_sim *s, const char *name, t_gpu_mesh *mesh);
void			sim_advance(t_sim *s, float dt);
void			sim_apply(t_sim *s, const uint32_t *tri_obj,
					const float *rest_pos, const t_gpu_tri *rest_tris,
					float *pos, t_gpu_tri *tris, size_t n);
void			mesh_detach(t_gpu_mesh *mesh);

#endif
