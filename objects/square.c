/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   square.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:48:19 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:19:29 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "objects.h"

t_square			*inisquare(t_square *square)
{
	square->vect1 = perpendicular_rand_vect(square->normal);
	square->vect2 = crossproduct(square->normal, square->vect1);
	square->vect1 = changelenght(square->vect1, square->height);
	square->vect2 = changelenght(square->vect2, square->height);
	square->corner.x =
			square->point.x + square->vect1.x / 2 + square->vect2.x / 2;
	square->corner.y =
			square->point.y + square->vect1.y / 2 + square->vect2.y / 2;
	square->corner.z =
			square->point.z + square->vect1.z / 2 + square->vect2.z / 2;
	return (square);
}

t_vect3				squarenormal(t_square *sq, t_rayo ray)
{
	if (angletwovectors(sq->normal, ray.vector) > 0)
		return (mulvector(sq->normal, -1));
	else
		return (sq->normal);
}

t_intersection		*squarecollision(t_rayo ray, t_square *sq)
{
	t_intersection	*returned;
	t_tupla			solution;
	t_vect3			puntointer;
	double			lambda;

	inisquare(sq);
	lambda = pseudointersection(ray, sq->point, sq->normal);
	if (lambda <= 0)
		return (NULL);
	puntointer = pointray(ray, lambda);
	solution = vectorialspacesolution(sq->corner, sq->vect1, sq->vect2, puntointer);
	if (solution.tamaño <= 0)
		return (NULL);
	if (!(solution.tupla[0] >= -1 && solution.tupla[0] <= 0 && solution.tupla[1] >= -1 && solution.tupla[1] <= 0))
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->lambda = lambda;
	returned->color = sq->color;
	returned->object = sq;
	returned->normal = squarenormal(sq, ray);
	return (returned);
}
