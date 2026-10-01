#ifndef OBJ_LOADER_H
# define OBJ_LOADER_H
# include "scene_export.h"

/*
** Anade a la escena los triangulos del .obj. Devuelve cuantos, o -1.
*/

int				load_obj(t_gpu_scene *scene, const char *path);

#endif
