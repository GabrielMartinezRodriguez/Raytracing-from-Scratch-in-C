/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shading.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 13:45:00 by gmartine          #+#    #+#             */
/*   Updated: 2026/10/01 13:45:00 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

static double	dot(t_vect3 first, t_vect3 second)
{
	return (first.x * second.x + first.y * second.y + first.z * second.z);
}

static int		inshadow(t_scene *scene, t_vect3 point, t_vect3 light)
{
	t_rayo			ray;
	t_intersection	*obstacle;
	int				shadow;

	ray.punto = point;
	ray.vector = distancevector(light, point);
	obstacle = raycollision(scene->objets, ray);
	shadow = obstacle != NULL && obstacle->lambda < 1;
	free(obstacle);
	return (shadow);
}

/*
** Blinn-Phong: componente difusa (color del objeto) + brillo especular
** (color de la luz) por cada luz que no este tapada por otro objeto.
*/

static t_rgb	directlight(t_scene *scene, t_hit *hit, t_rgb albedo)
{
	t_list_light	*light;
	t_vect3			tolight;
	t_vect3			half;
	t_rgb			lightcolor;
	t_rgb			color;

	color = newrgb(0, 0, 0);
	light = scene->lights;
	while (light != NULL)
	{
		tolight = changelenght(distancevector(light->point.point,
			hit->point), 1);
		if (dot(hit->normal, tolight) > 0
			&& !inshadow(scene, hit->point, light->point.point))
		{
			lightcolor = tolinear(light->point.color, light->point.intensity);
			color = addrgb(color, scalergb(mulrgb(albedo, lightcolor),
				dot(hit->normal, tolight)));
			half = changelenght(distancevector(tolight, hit->dir), 1);
			if (dot(hit->normal, half) > 0)
				color = addrgb(color, scalergb(lightcolor, SPECULAR
					* pow(dot(hit->normal, half), SHININESS)));
		}
		light = light->next;
	}
	return (color);
}

static t_hit	buildhit(t_rayo ray, t_intersection *intersection)
{
	t_hit hit;

	hit.dir = changelenght(ray.vector, 1);
	hit.normal = changelenght(intersection->normal, 1);
	if (dot(hit.normal, hit.dir) > 0)
		hit.normal = mulvector(hit.normal, -1);
	hit.point = pointvectorpoint(pointray(ray, intersection->lambda),
		hit.normal, EPSILON);
	return (hit);
}

t_rgb			trace(t_scene *scene, t_rayo ray, int depth)
{
	t_intersection	*intersection;
	t_hit			hit;
	t_rgb			albedo;
	t_rgb			color;
	t_rayo			reflected;

	intersection = raycollision(scene->objets, ray);
	if (intersection == NULL)
		return (newrgb(0, 0, 0));
	hit = buildhit(ray, intersection);
	albedo = tolinear(intersection->color, 1);
	color = mulrgb(albedo, tolinear(scene->env_light.color,
		scene->env_light.intensity));
	color = addrgb(color, directlight(scene, &hit, albedo));
	if (intersection->reflect > 0 && depth < MAX_DEPTH)
	{
		reflected.punto = hit.point;
		reflected.vector = pointvectorpoint(hit.dir, hit.normal,
			-2 * dot(hit.dir, hit.normal));
		color = addrgb(scalergb(color, 1 - intersection->reflect),
			scalergb(trace(scene, reflected, depth + 1),
			intersection->reflect));
	}
	free(intersection);
	return (color);
}
