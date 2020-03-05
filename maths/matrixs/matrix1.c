/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matrix1.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:23:09 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 16:29:13 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "matrix.h"

t_vect3			rotatevector(t_matrix3 matrix, t_vect3 vector)
{
	t_vect3		rotatedvector;

	rotatedvector.x = matrix.vect1.x * vector.x + matrix.vect2.x * vector.y + matrix.vect3.x * vector.z;
	rotatedvector.y = matrix.vect1.y * vector.x + matrix.vect2.y * vector.y + matrix.vect3.y * vector.z;
	rotatedvector.z = matrix.vect1.z * vector.x + matrix.vect2.z * vector.y + matrix.vect3.z * vector.z;
	return (rotatedvector);
}

t_matrix3		inversematrix(t_matrix3 matrix)
{
	int			i;
	int			j;
	t_matrix3	inverse;
	double		det;

	det = detmatrix(matrix);
	matrix = attachedmatrix(matrix);
	i = 0;
	j = 0;
	while (i < 3)
	{
		while (j < 3)
		{
			set_index_matrix(&inverse, i, j, get_index_matrix(matrix, i, j) / det);
			j++;
		}
		j = 0;
		i++;
	}
	inverse = transposematrix(inverse);
	return (inverse);
}

double			detmatrix(t_matrix3 matrix)
{
	double det;

	det = 0;
	det += matrix.vect1.x * matrix.vect2.y * matrix.vect3.z + matrix.vect1.y * matrix.vect2.z * matrix.vect3.x + matrix.vect1.z * matrix.vect2.x * matrix.vect3.y;
	det -= matrix.vect3.x * matrix.vect2.y * matrix.vect1.z + matrix.vect2.x * matrix.vect1.y * matrix.vect3.z + matrix.vect3.y * matrix.vect2.z * matrix.vect1.x;
	return (det);
}

t_matrix3		attachedmatrix(t_matrix3 matrix)
{
	t_matrix3	attached;
	int			i;
	int			j;

	i = 0;
	j = 0;
	while (i < 3)
	{
		while (j < 3)
		{
			set_index_matrix(&attached, i, j, submatrixdet(matrix, i, j));
			j++;
		}
		j = 0;
		i++;
	}
	return (attached);
}

double			submatrixdet(t_matrix3 matrix, int i, int j)
{
	int			inifila;
	int			inicol;
	double		det;
	double		**newmatrix;

	newmatrix = t_matrixtomatrix(matrix);
	if (i > 0)
		inicol = 0;
	else
		inicol = 1;
	if (j > 0)
		inifila = 0;
	else
		inifila = 1;
	if (inicol + 1 != i && inifila + 1 != j)
		det = newmatrix[inifila][inicol] * newmatrix[inifila + 1][inicol + 1] - newmatrix[inifila + 1][inicol] * newmatrix[inifila][inicol + 1];
	else if (inicol + 2 != i && inifila + 1 != j)
		det = newmatrix[inifila][inicol] * newmatrix[inifila + 1][inicol + 2] - newmatrix[inifila + 1][inicol] * newmatrix[inifila][inicol + 2];
	else if (inicol + 1 != i && inifila + 2 != j)
		det = newmatrix[inifila][inicol] * newmatrix[inifila + 2][inicol + 1] - newmatrix[inifila + 2][inicol] * newmatrix[inifila][inicol + 1];
	else if (inicol + 2 != i && inifila + 2 != j)
		det = newmatrix[inifila][inicol] * newmatrix[inifila + 2][inicol + 2] - newmatrix[inifila + 2][inicol] * newmatrix[inifila][inicol + 2];
	if ((i + j) % 2 != 0)
		det = -det;
	freematrix(newmatrix);
	return (det);
}

void			freematrix(double **matrix)
{
	free(matrix[0]);
	free(matrix[1]);
	free(matrix[2]);
	free(matrix);
}
