#ifndef OBJ_LOADER_H
# define OBJ_LOADER_H
# include "shared.h"

typedef struct	s_mesh_material
{
	char		name[128];
	char		texture[1024];
	simd_float4	kd;
}				t_mesh_material;

/*
** Malla de triangulos sin indexar: pos guarda 9 floats por triangulo (sus
** 3 vertices) en el formato que piden las unidades RT; tris, sus atributos.
** El material 0 es el de por defecto.
*/

typedef struct	s_gpu_mesh
{
	float			*pos;
	t_gpu_tri		*tris;
	size_t			ntris;
	size_t			cap;
	t_mesh_material	*mats;
	int				nmats;
	int				capmats;
	void			*map;
	size_t			map_size;
	simd_float3		center;
	simd_float3		size;
}				t_gpu_mesh;

/*
** Carga un .obj (con su .mtl) en mesh. Devuelve el numero de triangulos,
** o -1 si no se puede abrir.
*/

long			load_obj(t_gpu_mesh *mesh, const char *path);
unsigned int	pack_normal(simd_float3 n);

/*
** Cache binaria junto al .obj (modelo.obj.rtcache): load_obj_cached la usa
** si coincide con el .obj (tamano y fecha) y si no, la genera.
*/

long			load_obj_cached(t_gpu_mesh *mesh, const char *path);
void			free_mesh_data(t_gpu_mesh *mesh);

#endif
