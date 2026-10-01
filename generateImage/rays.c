/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rays.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:59 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:34:38 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

t_rayo			cordtoray(t_scene *scene, double i, double j)
{
	t_rayo		ray;
	t_vect3		vectx;
	t_vect3		vecty;
	t_vect3		vectz;

	ray.punto = scene->camera->origin;
	vectz = mulvector(scene->camera->direction, scene->camera->depth);
	vectx = mulvector(scene->camera->vectorx, i - scene->resolution.x / 2.0);
	vecty = mulvector(scene->camera->vectory, scene->resolution.y / 2.0 - j);
	ray.vector = addvector(vectx, vecty);
	ray.vector = addvector(ray.vector, vectz);
	return (ray);
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
		if (actualintersection != NULL)
			actualintersection->reflect = actualobject->reflect;
		morenear = returnnear(morenear, actualintersection);
		actualobject = actualobject->next;
	}
	return (morenear);
}
