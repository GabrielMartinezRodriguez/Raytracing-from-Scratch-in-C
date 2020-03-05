/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   triangle.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:48:24 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 22:42:47 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "objects.h"

void				initriangle(t_triangle *triangle)
{
	triangle->vect1 = distancevector(triangle->point2, triangle->point1);
	triangle->vect2 = distancevector(triangle->point3, triangle->point1);
	triangle->normal = crossproduct(triangle->vect1, triangle->vect2);
}

t_vect3				trinormal(t_triangle *tri, t_rayo ray)
{
	if (angletwovectors(tri->normal, ray.vector) > 0)
		return (mulvector(tri->normal, -1));
	else
		return (tri->normal);
}

t_intersection		*trianglecollision(t_rayo ray, t_triangle *tri)
{
	double				lambda;
	t_intersection		*returned;
	t_tupla				solution;
	t_vect3				pointinter;

	initriangle(tri);
	lambda = pseudointersection(ray, tri->point1, tri->normal);
	if (lambda <= 0)
		return (NULL);
	pointinter = pointray(ray, lambda);
	solution = vectorialspacesolution(tri->point1, tri->vect1, tri->vect2, pointinter);
	if (solution.tamaño <= 0)
		return (NULL);
	if (!(solution.tupla[0] >= 0 && solution.tupla[1] >= 0 && (solution.tupla[0] + solution.tupla[1] <= 1)))
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->lambda = lambda;
	returned->color = tri->color;
	returned->object = tri;
	returned->normal = trinormal(tri, ray);
	return (returned);
}
