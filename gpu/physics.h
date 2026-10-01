#ifndef PHYSICS_H
# define PHYSICS_H
# include <simd/simd.h>

/*
** Motor de fisica de solidos rigidos, escrito desde cero.
**
** Cada cuerpo es un grupo de esferas (en coordenadas locales respecto a su
** centro de masas). El entorno estatico (mesa, suelo...) son triangulos
** reales indexados en una rejilla. Las colisiones se resuelven con impulsos
** secuenciales: rebote, rozamiento y correccion de penetracion.
*/

typedef struct	s_psphere
{
	simd_float3	c;
	float		r;
}				t_psphere;

typedef struct	s_body
{
	simd_float3		pos;
	simd_quatf		rot;
	simd_float3		vel;
	simd_float3		ang;
	float			inv_mass;
	simd_float3x3	inv_inertia;
	int				first;
	int				count;
	float			restitution;
	float			friction;
	float			idle;
	int				sleeping;
	int				active;
	int				touching;
	simd_float3		rest_com;
}				t_body;

typedef struct	s_contact
{
	int			a;
	int			b;
	simd_float3	p;
	simd_float3	n;
	float		depth;
	float		jn;
	float		jt;
	float		bias;
	float		mass_n;
}				t_contact;

typedef struct	s_world
{
	t_body		*bodies;
	int			nbodies;
	int			capbodies;
	t_psphere	*local;
	simd_float3	*wpos;
	int			*owner;
	int			nspheres;
	int			capspheres;
	float		*tris;
	int			ntris;
	float		cell;
	simd_float3	gmin;
	int			gdim[3];
	int			*cell_start;
	int			*cell_items;
	int			*stamp;
	int			stamp_id;
	t_contact	*contacts;
	int			ncontacts;
	int			capcontacts;
	simd_float3	gravity;
	int			has_ground;
	float		ground_y;
	int			iterations;
	int			substeps;
}				t_world;

void			world_init(t_world *w);
void			world_set_static(t_world *w, const float *tris, int ntris,
					float cell);
int				world_add_body(t_world *w, const t_psphere *spheres, int n,
					float density, simd_float3 rest_com);
void			world_step(t_world *w, float dt);
void			world_settle(t_world *w);
float			world_push_point(t_world *w, simd_float3 *p, float r,
					simd_float3 *normal);
int				build_cluster(const float *tris, int ntris, int res,
					t_psphere **out, simd_float3 *com);
int				fit_sphere(const float *tris, int ntris, t_psphere **out,
					simd_float3 *com);
simd_float3		body_point(const t_body *b, simd_float3 rest);
simd_float3		body_dir(const t_body *b, simd_float3 v);
void			body_wake(t_body *b);

#endif
