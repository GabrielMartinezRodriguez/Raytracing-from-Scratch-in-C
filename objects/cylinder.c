/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cylinder.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:21:03 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:26:59 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "objects.h"

t_intersection		*cylindercollision(t_rayo ray, t_cylinder *cyl)
{
	t_intersection	**intersection;
	t_intersection	*returned;

	intersection = ft_calloc(3, sizeof(t_intersection *));
	ini_tops(cyl);
	intersection[0] = cylindercollisiontransformed(ray, *cyl);
	intersection[1] = circlecollision(ray, &(cyl->upper_top));
	intersection[2] = circlecollision(ray, &(cyl->lower_top));
	returned = returnnear(intersection[0], intersection[1]);
	returned = returnnear(returned, intersection[2]);
	free(intersection);
	if (returned == NULL)
		return (NULL);
	returned->object = cyl;
	returned->color = cyl->color;
	return (returned);
}

t_cylinder			*inicylinder(t_cylinder *cyl)
{
	cyl->vect1 = perpendicular_rand_vect(cyl->normal);
	cyl->vect2 = crossproduct(cyl->vect1, cyl->normal);
	cyl->normal = changelenght(cyl->normal, 1);
	cyl->vect1 = changelenght(cyl->vect1, 1);
	cyl->vect2 = changelenght(cyl->vect2, 1);
	cyl->base = inimatrix(cyl->normal, cyl->vect1, cyl->vect2);
	cyl->inversebase = inversematrix(cyl->base);
	return (cyl);
}

void				rotatecollision(t_rayo *ray, t_cylinder *cyl)
{
	ray->punto = rotatevector(cyl->inversebase, ray->punto);
	ray->vector = rotatevector(cyl->inversebase, ray->vector);
	cyl->point = rotatevector(cyl->inversebase, cyl->point);
	cyl->normal = rotatevector(cyl->inversebase, cyl->normal);
	cyl->vect1 = rotatevector(cyl->inversebase, cyl->vect1);
	cyl->vect2 = rotatevector(cyl->inversebase, cyl->vect2);
}

void				ini_tops(t_cylinder *cyl)
{
	cyl->upper_top.center = pointvectorpoint(cyl->point, cyl->normal, cyl->height / 2);
	cyl->lower_top.center = pointvectorpoint(cyl->point, cyl->normal, -cyl->height / 2);
	cyl->lower_top.normal = mulvector(cyl->normal, -1);
	cyl->upper_top.normal = cyl->normal;
	cyl->upper_top.radius = cyl->diameter / 2;
	cyl->lower_top.radius = cyl->diameter / 2;
	cyl->lower_top.color = cyl->color;
	cyl->upper_top.color = cyl->color;
}

t_vect3				calcnormalcyl(t_cylinder cyl, t_vect3 intersectionpoint, t_matrix3 matrix)
{
	t_vect3	inpoint;
	t_vect3	normal;

	inpoint = cyl.point;
	inpoint.x = intersectionpoint.x;
	normal = distancevector(intersectionpoint, inpoint);
	normal = rotatevector(matrix, normal);
	return (normal);
}

t_intersection		*cylindercollisiontransformed(t_rayo ray, t_cylinder cyl)
{
	t_tupla			productos[3];
	t_intersection	*intersection;
	t_tupla			solution;
	t_vect3			point;
	double			menor;

	rotatecollision(&ray, &cyl);
	productos[0] = notableproduct(ray.vector.y, ray.punto.y - cyl.point.y);
	productos[1] = notableproduct(ray.vector.z, ray.punto.z - cyl.point.z);
	productos[2] = addtupla(productos[0], productos[1]);
	solution = solveequation(productos[2], pow(cyl.diameter / 2, 2));
	if (solution.tamaño <= 0)
		return (NULL);
	menor = lowernumber(solution);
	point = pointray(ray, menor);
	if (pow(point.x - cyl.point.x, 2) > pow(cyl.height / 2, 2))
		return (NULL);
	intersection = ft_calloc(1, sizeof(t_intersection));
	intersection->lambda = menor;
	intersection->color = cyl.color;
	intersection->normal = calcnormalcyl(cyl, point, cyl.base);
	return (intersection);
}
