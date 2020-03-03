/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vectors.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 15:54:41 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/03 20:38:20 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

t_vect3				newVector(double x, double y, double z)
{
	t_vect3 vector;

	vector.x = x;
	vector.y = y;
	vector.z = z;
	return (vector);
}

t_vect3				crossProduct(t_vect3 first, t_vect3 second)
{
	t_vect3 result;

	result.x = first.y * second.z - first.z * second.y;
	result.y = first.z * second.x - first.x * second.z;
	result.z = first.x * second.y - first.y * second.x;
	return (result);
}

double				vector_index(t_vect3 vect, int i)
{
	if (i == 0)
		return (vect.x);
	else if (i == 1)
		return (vect.y);
	else if (i == 2)
		return (vect.z);
	return (0);
}

t_vect3				changeLenght(t_vect3 vector, double module)
{
	double		moduleOriginal;
	double		factor;

	moduleOriginal = sqrt(pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2));
	factor = moduleOriginal / module;
	vector.x /= factor;
	vector.y /= factor;
	vector.z /= factor;
	return (vector);
}

t_vect3				mulVector(t_vect3 vector, double number)
{
	vector.x *= number;
	vector.y *= number;
	vector.z *= number;
	return (vector);
}

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
		solution = solveEquation(equation, 0);
		new_vector.x = solution.tupla[0];
	}
	else if (vector.y != 0)
	{
		equation.tupla[1] = vector.x + vector.z;
		equation.tupla[0] = vector.y;
		solution = solveEquation(equation, 0);
		new_vector.y = solution.tupla[0];
	}
	else
	{
		equation.tupla[1] = vector.x + vector.y;
		equation.tupla[0] = vector.z;
		solution = solveEquation(equation, 0);
		new_vector.z = solution.tupla[0];
	}
	return (new_vector);
}

double				moduleVector(t_vect3 vector)
{
	double module;

	module = 0;
	module += pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2);
	module = sqrt(module);
	return (module);
}

t_vect3				pointRay(t_rayo ray, double lamda)
{
	t_vect3 point;

	point.x = ray.punto.x + lamda * ray.vector.x;
	point.y = ray.punto.y + lamda * ray.vector.y;
	point.z = ray.punto.z + lamda * ray.vector.z;
	return (point);
}

t_vect3				distanceVector(t_vect3 point1, t_vect3 point2)
{
	t_vect3 vect;

	vect.x = point1.x - point2.x;
	vect.y = point1.y - point2.y;
	vect.z = point1.z - point2.z;
	return (vect);
}

t_vect3				addVector(t_vect3 vect1, t_vect3 vect2)
{
	t_vect3 vector;

	vector.x = vect1.x + vect2.x;
	vector.y = vect1.y + vect2.y;
	vector.z = vect1.z + vect2.z;
	return (vector);
}

double				distancePoint(t_vect3 point1, t_vect3 point2)
{
	double distance;

	distance = 0;
	distance += pow(point1.x - point2.x, 2);
	distance += pow(point1.y - point2.y, 2);
	distance += pow(point1.z - point2.z, 2);
	distance = sqrt(distance);
	return (distance);
}

t_vect3				pointVectorPoint(t_vect3 point, t_vect3 vect, double lambda)
{
	t_vect3 returned;

	returned.x = point.x + lambda * vect.x;
	returned.y = point.y + lambda * vect.y;
	returned.z = point.z + lambda * vect.z;
	return (returned);
}

static int			checkEquation(double origin, double vect1, double vect2, double point, t_tupla *despeje)
{
	t_tupla equation;
	t_tupla solution;

	if (vect1 == 0)
		return (0);
	if (vect2 == 0)
	{
		equation = newTupla(vect1, origin);
		solution = solveEquation(equation, point);
		*despeje = solution;
		return (1);
	}
	equation = newTupla(-(vect2 / vect1), (point - origin) / vect1);
	*despeje = equation;
	return (2);
}

t_tupla				vectorialSpaceSolution(t_vect3 origin, t_vect3 vect1, t_vect3 vect2, t_vect3 point)
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
		flag = checkEquation(vector_index(origin, i), vector_index(vect1, i), vector_index(vect2, i), vector_index(point, i), &despeje);
		i++;
	}
	i--;
	j = 0;
	while (j < 3 && !(j != i && vector_index(vect2, j) != 0 && (vector_index(vect1, j) != vector_index(vect1, i) || vector_index(vect2, j) != vector_index(vect2, i)) && (vector_index(vect1, j) != -vector_index(vect1, i) || vector_index(vect2, j) != -vector_index(vect2, i))))
		j++;
	if (flag == 1)
	{
		equation[0] = newTupla(vector_index(vect2, j), despeje.tupla[0] * vector_index(vect1, j));
		equation[1] = solveEquation(equation[0], vector_index(point, j) - vector_index(origin, j));
		equation[1].tamaño++;
		equation[1].tupla[1] = despeje.tupla[0];
		return (equation[1]);
	}
	equation[0] = mulTupla(despeje, vector_index(vect1, j));
	equation[1] = newTupla(vector_index(vect2, j), vector_index(origin, j));
	equation[2] = addTupla(equation[0], equation[1]);
	equation[3] = solveEquation(equation[2], vector_index(point, j)); 
	equation[3].tamaño++;
	equation[3].tupla[1] = despeje.tupla[0] * equation[3].tupla[0] + despeje.tupla[1];
	return (equation[3]);
}

double				angleTwoVectors(t_vect3 vect1, t_vect3 vect2)
{
	double	scalarProduct;
	double	cos;

	scalarProduct = vect1.x * vect2.x + vect1.y * vect2.y + vect1.z * vect2.z;
	cos = scalarProduct / (moduleVector(vect1) * moduleVector(vect2));
	return (cos);
}
