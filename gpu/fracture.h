#ifndef FRACTURE_H
# define FRACTURE_H
# include "obj_loader.h"

/*
** Parte el objeto obj en celdas de Voronoi (una por semilla): recorta sus
** triangulos con los planos de corte, los asigna a shard_obj[celda] y
** anade las caras de rotura (material crack_mat). Devuelve cuantos bordes
** de rotura ha encontrado.
*/

int		fracture_object(t_gpu_mesh *m, int obj, const simd_float3 *seeds,
			int nseeds, const int *shard_obj, unsigned crack_mat);

#endif
