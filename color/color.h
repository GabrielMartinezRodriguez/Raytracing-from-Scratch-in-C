/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   color.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:21:56 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 00:20:38 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COLOR_H
# define COLOR_H
# include "../header.h"

t_color			anglecolor(t_color color, t_vect3 normal, t_vect3 raydir);
t_color			reflectedcolor(t_color colorobject, t_color colorlight);
t_color			intensitycolor(t_color color, double intensity);
unsigned int	colortoint(t_color color);
t_color			addcolor(t_color color1, t_color color2);
t_color			fusion_colors(t_colors_reflected colors);
#endif
