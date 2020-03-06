/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matrix.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:44:14 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:28:11 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MATRIX_H
# define MATRIX_H
# include "../maths.h"

t_vect3			rotatevector(t_matrix3 matrix, t_vect3 vector);
t_matrix3		inversematrix(t_matrix3 matrix);
double			detmatrix(t_matrix3 matrix);
t_matrix3		attachedmatrix(t_matrix3 matrix);
double			submatrixdet(t_matrix3 matrix, int i, int j);
double			**t_matrixtomatrix(t_matrix3 matrix);
void			set_index_matrix(t_matrix3 *matrix, int i, int j, double value);
double			get_index_matrix(t_matrix3 matrix, int i, int j);
t_matrix3		transposematrix(t_matrix3 matrix);
t_matrix3		inimatrix(t_vect3 first, t_vect3 second, t_vect3 third);
void			freematrix(double **matrix);
#endif
