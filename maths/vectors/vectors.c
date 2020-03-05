/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vectors.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 15:54:41 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 16:30:23 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vectors.h"

double				distancepoint(t_vect3 point1, t_vect3 point2)
{
	double distance;

	distance = 0;
	distance += pow(point1.x - point2.x, 2);
	distance += pow(point1.y - point2.y, 2);
	distance += pow(point1.z - point2.z, 2);
	distance = sqrt(distance);
	return (distance);
}

t_vect3				pointvectorpoint(t_vect3 point, t_vect3 vect, double lambda)
{
	t_vect3 returned;

	returned.x = point.x + lambda * vect.x;
	returned.y = point.y + lambda * vect.y;
	returned.z = point.z + lambda * vect.z;
	return (returned);
}

int					checkequation(double origin, double vect1, double vect2, double point, t_tupla *despeje)
{
	t_tupla equation;
	t_tupla solution;

	if (vect1 == 0)
		return (0);
	if (vect2 == 0)
	{
		equation = newtupla(vect1, origin);
		solution = solveequation(equation, point);
		*despeje = solution;
		return (1);
	}
	equation = newtupla(-(vect2 / vect1), (point - origin) / vect1);
	*despeje = equation;
	return (2);
}

t_tupla				vectorialspacesolution(t_vect3 origin, t_vect3 vect1, t_vect3 vect2, t_vect3 point)
{
	int			i;
	int			j;
	int			flag;
	t_tupla		despeje;
	t_tupla		equation[4];

	flag = 0;
	i = 0;
	while (i < 3 && flag == 0)
	{
		flag = checkequation(vector_index(origin, i), vector_index(vect1, i), vector_index(vect2, i), vector_index(point, i), &despeje);
		i++;
	}
	i--;
	j = 0;
	while (j < 3 && !(j != i && vector_index(vect2, j) != 0 && (vector_index(vect1, j) != vector_index(vect1, i) || vector_index(vect2, j) != vector_index(vect2, i)) && (vector_index(vect1, j) != -vector_index(vect1, i) || vector_index(vect2, j) != -vector_index(vect2, i))))
		j++;
	if (flag == 1)
	{
		equation[0] = newtupla(vector_index(vect2, j), despeje.tupla[0] * vector_index(vect1, j));
		equation[1] = solveequation(equation[0], vector_index(point, j) - vector_index(origin, j));
		equation[1].tamaño++;
		equation[1].tupla[1] = despeje.tupla[0];
		return (equation[1]);
	}
	equation[0] = multupla(despeje, vector_index(vect1, j));
	equation[1] = newtupla(vector_index(vect2, j), vector_index(origin, j));
	equation[2] = addtupla(equation[0], equation[1]);
	equation[3] = solveequation(equation[2], vector_index(point, j));
	equation[3].tamaño++;
	equation[3].tupla[1] = despeje.tupla[0] * equation[3].tupla[0] + despeje.tupla[1];
	return (equation[3]);
}

double				angletwovectors(t_vect3 vect1, t_vect3 vect2)
{
	double	scalarproduct;
	double	cos;

	scalarproduct = vect1.x * vect2.x + vect1.y * vect2.y + vect1.z * vect2.z;
	cos = scalarproduct / (modulevector(vect1) * modulevector(vect2));
	return (cos);
}
