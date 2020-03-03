/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rayo.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:59 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/03 20:42:25 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

t_rayo			cordToRay(int i, int j, t_scene *scene)
{
	t_rayo		ray;
	t_vect3		vectX;
	t_vect3		vectY;
	t_vect3		vectZ;

	ray.punto = scene->camera.origin;
	vectZ =  mulVector(scene->camera.direction, scene->camera.depth);
	vectX = mulVector(scene->camera.vectorX, i - scene->resolution.x / 2);
	vectY = mulVector(scene->camera.vectorY, scene->resolution.y / 2 - j);
	ray.vector = addVector(vectX, vectY);
	ray.vector = addVector(ray.vector, vectZ);
	return (ray);
}

t_rayo			rayToLight(t_intersection *intersection, t_rayo primary, t_light_point light)
{
	t_rayo		secondary;
	t_vect3		originPoint;

	originPoint = pointRay(primary, intersection->lambda - 0.0001);
	secondary.punto = originPoint;
	secondary.vector = distanceVector(light.point, originPoint);
	return (secondary);
}
