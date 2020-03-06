/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   camera.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 20:01:38 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:35:39 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

void		inicamera(t_camera *camera, t_resolution resolution)
{
	camera->vectorx = perpendicular_rand_vect(camera->direction);
	camera->vectory = crossproduct(camera->vectorx, camera->direction);
	camera->vectorx = changelenght(camera->vectorx, 1);
	camera->vectory = changelenght(camera->vectory, 1);
	camera->direction = changelenght(camera->direction, 1);
	camera->fov_h_rad = (camera->fov_h / 360) * 2 * M_PI;
	camera->depth = resolution.x / (2 * tan(camera->fov_h_rad / 2));
}

int			changecamera(t_scene *scene, int next)
{
	int		change;

	change = 0;
	if (next == 1)
	{
		if (scene->camera->next != NULL)
		{
			change = 1;
			write(1, "PROCESANDO...", 14);
			scene->camera = scene->camera->next;
		}
		else
			write(1, "HA LLEGADO A LA ULTIMA CAMARA, NO QUEDAN MAS CAMARAS\n", 54);
	}
	else
	{
		if (scene->camera->back != NULL)
		{
			change = 1;
			write(1, "PROCESANDO...", 14);
			scene->camera = scene->camera->back;
		}
		else
			write(1, "ESTAN EN LA PRIMERA CAMARA, AVANCE SI QUIERE CAMBIAR DE CAMARA\n", 64);
	}
	return (change);
}
