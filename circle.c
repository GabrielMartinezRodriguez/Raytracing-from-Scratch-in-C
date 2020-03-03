/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   circle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:46:01 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/03 20:02:37 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

t_intersection		*circleCollision(t_rayo ray, t_circle *cir)
{
	double			lambda;
	double			distance;
	t_vect3			intersection_point;
	t_intersection	*returned;

	lambda = pseudoIntersection(ray, cir->center, cir->normal);
	if (lambda < 0)
		return (NULL);
	intersection_point = pointRay(ray, lambda);
	distance = distancePoint(cir->center, intersection_point);
	if (distance > cir->radius)
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->lambda = lambda;
	returned->object = cir;
	returned->color = cir->color;
	returned->normal = cir->normal;
	return (returned);
}
