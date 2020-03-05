/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sphere.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:48:14 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 21:36:53 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "objects.h"

t_intersection			*spherecollision(t_rayo rayo, t_esfera *esfera)
{
	t_tupla			productox;
	t_tupla			productoy;
	t_tupla			productoz;
	t_tupla			equation;
	t_intersection	*returned;

	productox = notableproduct(rayo.vector.x, rayo.punto.x - esfera->punto.x);
	productoy = notableproduct(rayo.vector.y, rayo.punto.y - esfera->punto.y);
	productoz = notableproduct(rayo.vector.z, rayo.punto.z - esfera->punto.z);
	equation = addtupla(productox, productoy);
	equation = addtupla(equation, productoz);
	equation = solveequation(equation, pow(esfera->radio, 2));
	if (equation.tamaño <= 0)
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->object = esfera;
	returned->color = esfera->color;
	returned->lambda = lowernumber(equation);
	returned->normal = distancevector(pointray(rayo, returned->lambda), esfera->punto);
	return (returned);
}
