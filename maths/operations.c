/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operations.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:51 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 21:37:22 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "maths.h"

t_tupla			notableproduct(double first, double second)
{
	t_tupla	tupla;

	tupla.tamaño = 3;
	tupla.tupla[0] = pow(first, 2);
	tupla.tupla[1] = 2 * first * second;
	tupla.tupla[2] = pow(second, 2);
	return (tupla);
}

t_tupla			addtupla(t_tupla first, t_tupla second)
{
	t_tupla		tupla;
	int			i;

	i = 0;
	if (first.tamaño > second.tamaño)
		tupla.tamaño = first.tamaño;
	else
		tupla.tamaño = second.tamaño;
	while (i < 3)
	{
		tupla.tupla[i] = first.tupla[i] + second.tupla[i];
		i++;
	}
	return (tupla);
}

t_tupla			multupla(t_tupla tupla, double number)
{
	t_tupla		mul;
	int			i;

	mul.tamaño = tupla.tamaño;
	i = 0;
	while (i < tupla.tamaño)
	{
		mul.tupla[i] = tupla.tupla[i] * number;
		i++;
	}
	return (mul);
}

t_tupla			newtupla(double first, double second)
{
	t_tupla tupla;

	tupla.tamaño = 2;
	tupla.tupla[0] = first;
	tupla.tupla[1] = second;
	return (tupla);
}

double			lowernumber(t_tupla tupla)
{
	int			i;
	double		lower;

	lower = tupla.tupla[0];
	i = 1;
	while (i < tupla.tamaño)
	{
		if (tupla.tupla[i] < lower && tupla.tupla[i] >= 0)
			lower = tupla.tupla[i];
		i++;
	}
	return (lower);
}

double			absoluto(double abs)
{
	if (abs < 0)
		return (-abs);
	return (abs);
}
