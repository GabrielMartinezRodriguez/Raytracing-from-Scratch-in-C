/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   plane.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:56 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 21:36:46 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "objects.h"

t_plane			*iniplane(t_plane *plane)
{
	double number;

	number = 0;
	number += plane->point.x * plane->normal.x;
	number += plane->point.y * plane->normal.y;
	number += plane->point.z * plane->normal.z;
	plane->hight = number;
	return (plane);
}

t_vect3			planenormal(t_plane *pl, t_rayo ray)
{
	if (angletwovectors(pl->normal, ray.vector) > 0)
		return (mulvector(pl->normal, -1));
	else
		return (pl->normal);
}

t_intersection	*planecollision(t_rayo ray, t_plane *pl)
{
	t_tupla				equation;
	t_tupla				productox;
	t_tupla				productoy;
	t_tupla				productoz;
	t_intersection		*returned;

	iniplane(pl);
	productox = newtupla(ray.vector.x, ray.punto.x);
	productoy = newtupla(ray.vector.y, ray.punto.y);
	productoz = newtupla(ray.vector.z, ray.punto.z);
	productox = multupla(productox, pl->normal.x);
	productoy = multupla(productoy, pl->normal.y);
	productoz = multupla(productoz, pl->normal.z);
	equation = addtupla(productox, productoy);
	equation = addtupla(equation, productoz);
	equation = solveequation(equation, pl->hight);
	if (equation.tamaño <= 0)
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->object = pl;
	returned->color = pl->color;
	returned->lambda = lowernumber(equation);
	returned->normal = planenormal(pl, ray);
	return (returned);
}

double			pseudointersection(t_rayo ray, t_vect3 point, t_vect3 normal)
{
	t_plane				pseudoplane;
	t_intersection		*returned;
	double				lambda;

	pseudoplane.normal = normal;
	pseudoplane.point = point;
	iniplane(&pseudoplane);
	returned = planecollision(ray, &pseudoplane);
	if (returned == NULL)
		return (-1);
	lambda = returned->lambda;
	free(returned);
	return (lambda);
}
