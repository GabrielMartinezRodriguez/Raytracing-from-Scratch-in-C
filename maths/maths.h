/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   maths.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:47:04 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 23:29:58 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MATHS_H
# define MATHS_H
# include "../header.h"
# include "matrixs/matrix.h"
# include "vectors/vectors.h"

t_tupla			solveequation(t_tupla equation, double equal);
t_tupla			solveequation2(t_tupla equation, double equal);
t_tupla			solveequation1(t_tupla equation, double equal);
t_tupla			notableproduct(double first, double second);
t_tupla			addtupla(t_tupla first, t_tupla second);
t_tupla			multupla(t_tupla tupla, double number);
t_tupla			newtupla(double first, double second);
double			lowernumber(t_tupla tupla);
double			absoluto(double abs);
#endif
