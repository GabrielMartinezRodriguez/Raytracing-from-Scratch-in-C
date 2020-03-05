/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matrix.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:47:47 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 22:53:48 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "matrix.h"

double			**t_matrixtomatrix(t_matrix3 matrix)
{
	double		**newmatrix;

	newmatrix = ft_calloc(3, sizeof(double *));
	newmatrix[0] = ft_calloc(3, sizeof(double));
	newmatrix[1] = ft_calloc(3, sizeof(double));
	newmatrix[2] = ft_calloc(3, sizeof(double));
	newmatrix[0][0] = matrix.vect1.x;
	newmatrix[1][0] = matrix.vect1.y;
	newmatrix[2][0] = matrix.vect1.z;
	newmatrix[0][1] = matrix.vect2.x;
	newmatrix[1][1] = matrix.vect2.y;
	newmatrix[2][1] = matrix.vect2.z;
	newmatrix[0][2] = matrix.vect3.x;
	newmatrix[1][2] = matrix.vect3.y;
	newmatrix[2][2] = matrix.vect3.z;
	return (newmatrix);
}

void			set_index_matrix(t_matrix3 *matrix, int i, int j, double value)
{
	if (i == 0)
	{
		if (j == 0)
			matrix->vect1.x = value;
		else if (j == 1)
			matrix->vect1.y = value;
		else
			matrix->vect1.z = value;
	}
	else if (i == 1)
	{
		if (j == 0)
			matrix->vect2.x = value;
		else if (j == 1)
			matrix->vect2.y = value;
		else
			matrix->vect2.z = value;
	}
	else
	{
		if (j == 0)
			matrix->vect3.x = value;
		else if (j == 1)
			matrix->vect3.y = value;
		else
			matrix->vect3.z = value;
	}
}

double			get_index_matrix(t_matrix3 matrix, int i, int j)
{
	double value;

	if (i == 0)
	{
		if (j == 0)
			value = matrix.vect1.x;
		else if (j == 1)
			value = matrix.vect1.y;
		else
			value = matrix.vect1.z;
	}
	else if (i == 1)
	{
		if (j == 0)
			value = matrix.vect2.x;
		else if (j == 1)
			value = matrix.vect2.y;
		else
			value = matrix.vect2.z;
	}
	else
	{
		if (j == 0)
			value = matrix.vect3.x;
		else if (j == 1)
			value = matrix.vect3.y;
		else
			value = matrix.vect3.z;
	}
	return (value);
}

t_matrix3		transposematrix(t_matrix3 matrix)
{
	double aux;

	aux = matrix.vect1.y;
	matrix.vect1.y = matrix.vect2.x;
	matrix.vect2.x = aux;
	aux = matrix.vect1.z;
	matrix.vect1.z = matrix.vect3.x;
	matrix.vect3.x = aux;
	aux = matrix.vect2.z;
	matrix.vect2.z = matrix.vect3.y;
	matrix.vect3.y = aux;
	return (matrix);
}

t_matrix3		inimatrix(t_vect3 first, t_vect3 second, t_vect3 third)
{
	t_matrix3 new;

	new.vect1 = first;
	new.vect2 = second;
	new.vect3 = third;
	return (new);
}
