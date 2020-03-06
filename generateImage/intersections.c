/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   intersections.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:29 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:34:49 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

t_intersection		*returnnear(t_intersection *first, t_intersection *second)
{
	t_intersection *returned;

	first = infront(first);
	second = infront(second);
	if (first != NULL && second != NULL)
	{
		if (first->lambda > second->lambda)
			returned = second;
		else
			returned = first;
	}
	if (first == NULL)
		returned = second;
	else if (second == NULL)
		returned = first;
	if (returned != first && first != NULL)
		free(first);
	if (returned != second && second != NULL)
		free(second);
	return (returned);
}

t_intersection		*infront(t_intersection *intersection)
{
	if (intersection == NULL)
		return (NULL);
	if (intersection->lambda <= 0)
	{
		free(intersection);
		return (NULL);
	}
	return (intersection);
}
