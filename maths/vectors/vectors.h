/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vectors.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:40:50 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 23:26:01 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VECTORS_H
# define VECTORS_H
# include "../maths.h"

t_vect3				newvector(double x, double y, double z);
t_vect3				crossproduct(t_vect3 first, t_vect3 second);
double				vector_index(t_vect3 vect, int i);
t_vect3				changelenght(t_vect3 vector, double module);
t_vect3				mulvector(t_vect3 vector, double number);
t_vect3				perpendicular_rand_vect(t_vect3 vector);
double				modulevector(t_vect3 vector);
t_vect3				pointray(t_rayo ray, double lamda);
t_vect3				distancevector(t_vect3 point1, t_vect3 point2);
t_vect3				addvector(t_vect3 vect1, t_vect3 vect2);
double				distancepoint(t_vect3 point1, t_vect3 point2);
t_vect3				pointvectorpoint(t_vect3 point, t_vect3 vect, double lambda);
int					checkequation(double origin, double vect1, double vect2, double point, t_tupla *despeje);
t_tupla				vectorialspacesolution(t_vect3 origin, t_vect3 vect1, t_vect3 vect2, t_vect3 point);
double				angletwovectors(t_vect3 vect1, t_vect3 vect2);
#endif
