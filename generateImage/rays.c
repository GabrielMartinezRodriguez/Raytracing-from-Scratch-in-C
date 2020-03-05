/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rays.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:59 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 20:08:23 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

t_rayo			cordtoray(int i, int j, t_scene *scene)
{
	t_rayo		ray;
	t_vect3		vectx;
	t_vect3		vecty;
	t_vect3		vectz;

	ray.punto = scene->camera->origin;
	vectz = mulvector(scene->camera->direction, scene->camera->depth);
	vectx = mulvector(scene->camera->vectorx, i - scene->resolution.x / 2);
	vecty = mulvector(scene->camera->vectory, scene->resolution.y / 2 - j);
	ray.vector = addvector(vectx, vecty);
	ray.vector = addvector(ray.vector, vectz);
	return (ray);
}

t_rayo			raytolight(t_intersection *intersection, t_rayo primary, t_light_point light)
{
	t_rayo		secondary;
	t_vect3		originpoint;

	originpoint = pointray(primary, intersection->lambda - 0.00001);
	secondary.punto = originpoint;
	secondary.vector = distancevector(light.point, originpoint);
	return (secondary);
}

t_intersection	*raycollision(t_list_obj *objects, t_rayo ray)
{
	t_intersection	*(*collision)(t_rayo, void *);
	t_intersection	*morenear;
	t_intersection	*actualintersection;
	t_list_obj		*actualobject;

	actualobject = objects;
	morenear = NULL;
	while (actualobject != NULL)
	{
		collision = actualobject->functioncoll;
		actualintersection = (*collision)(ray, actualobject->object);
		morenear = returnnear(morenear, actualintersection);
		actualobject = actualobject->next;
	}
	return (morenear);
}

t_intersection	*primaryray(t_scene *scene, t_rayo *ray, int pixelx, int pixelj)
{
	t_intersection *intersection;

	*ray = cordtoray(pixelx, pixelj, scene);
	intersection = raycollision(scene->objets, *ray);
	return (intersection);
}

t_color			secondaryray(t_scene *scene, t_rayo ray, t_intersection *intersectionobject)
{
	t_list_light	*light;
	t_intersection	*lightobstacle;
	double			distance[2];
	t_color			color;

	light = scene->lights;
	ft_bzero(&color, sizeof(t_color));
	while (light != NULL)
	{
		lightobstacle = NULL;
		ray = raytolight(intersectionobject, ray, light->point);
		distance[0] = distancepoint(ray.punto, light->point.point);
		lightobstacle = raycollision(scene->objets, ray);
		if (lightobstacle != NULL)
			distance[1] = distancepoint(pointray(ray, lightobstacle->lambda), ray.punto);
		if (distance[0] < distance[1] || lightobstacle == NULL)
			color = addcolor(calccolor(intersectionobject, ray, light->point.intensity, light->point.color), color);
		light = light->next;
		if (lightobstacle != NULL)
			free(lightobstacle);
	}
	return (color);
}
