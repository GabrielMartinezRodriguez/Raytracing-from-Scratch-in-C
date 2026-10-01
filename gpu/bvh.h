#ifndef BVH_H
# define BVH_H
# include "scene_export.h"

/*
** Reordena scene->objects: primero los planos (infinitos, fuera del arbol)
** y despues el resto en el orden de las hojas del BVH.
*/

t_gpu_node		*build_bvh(t_gpu_scene *scene, int *nnodes, int *nplanes);

void			object_bounds(t_gpu_object *o, float *min, float *max);

#endif
