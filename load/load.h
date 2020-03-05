/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:34:27 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 16:23:26 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOAD_H
# define LOAD_H
# include "../header.h"

int				loadscene(t_scene *scene, char *filename);
void			processstring(t_scene *scene, char **words);
void			loadtriangle(t_scene *scene, char **words);
void			loadcylinder(t_scene *scene, char **words);
void			loadsquare(t_scene *scene, char **words);
void			loadplane(t_scene *scene, char **words);
void			loadsphere(t_scene *scene, char **words);
void			loadlight(t_scene *scene, char **words);
void			loadcamera(t_scene *scene, char **words);
void			loadresolution(t_scene *scene, char **words);
void			loadambientlight(t_scene *scene, char **words);
t_vect3			chargenormalizedvector(char *vector);
t_color			chargecolor(char *rgb);
t_vect3			chargepoint(char *point);
void			addobject(t_scene *scene, void *object);
int				countwords(char **words);
void			freespace(char **words);
void			error(char *s);
int             fundamentalsobjects(char *str);
#endif
