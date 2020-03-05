/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vectors2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:22:53 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 16:30:55 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vectors.h"

t_vect3				perpendicular_rand_vect(t_vect3 vector)
{
	t_tupla		equation;
	t_tupla		solution;
	t_vect3		new_vector;

	equation.tamaño = 2;
	new_vector.x = 1;
	new_vector.y = 1;
	new_vector.z = 1;
	if (vector.x != 0)
	{
		equation.tupla[1] = vector.y + vector.z;
		equation.tupla[0] = vector.x;
		solution = solveequation(equation, 0);
		new_vector.x = solution.tupla[0];
	}
	else if (vector.y != 0)
	{
		equation.tupla[1] = vector.x + vector.z;
		equation.tupla[0] = vector.y;
		solution = solveequation(equation, 0);
		new_vector.y = solution.tupla[0];
	}
	else
	{
		equation.tupla[1] = vector.x + vector.y;
		equation.tupla[0] = vector.z;
		solution = solveequation(equation, 0);
		new_vector.z = solution.tupla[0];
	}
	return (new_vector);
}

double				modulevector(t_vect3 vector)
{
	double module;

	module = 0;
	module += pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2);
	module = sqrt(module);
	return (module);
}

t_vect3				pointray(t_rayo ray, double lamda)
{
	t_vect3 point;

	point.x = ray.punto.x + lamda * ray.vector.x;
	point.y = ray.punto.y + lamda * ray.vector.y;
	point.z = ray.punto.z + lamda * ray.vector.z;
	return (point);
}

t_vect3				distancevector(t_vect3 point1, t_vect3 point2)
{
	t_vect3 vect;

	vect.x = point1.x - point2.x;
	vect.y = point1.y - point2.y;
	vect.z = point1.z - point2.z;
	return (vect);
}

t_vect3				addvector(t_vect3 vect1, t_vect3 vect2)
{
	t_vect3 vector;

	vector.x = vect1.x + vect2.x;
	vector.y = vect1.y + vect2.y;
	vector.z = vect1.z + vect2.z;
	return (vector);
}
