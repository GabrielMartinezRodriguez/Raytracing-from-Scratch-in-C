/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   generateimage.h                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:25:05 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:35:16 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GENERATEIMAGE_H
# define GENERATEIMAGE_H
# include "../header.h"
# include "bmp/bmp.h"

void			generateimage(t_scene scene, t_libx *libx);
void			showscene(t_libx *libx, t_arg *args, t_scene *scene);
void			inicamera(t_camera *camera, t_resolution resolution);
t_color			calccolor(t_intersection *intersection, t_rayo ray, double intesity, t_color colorlight);
t_intersection	*raycollision(t_list_obj *objects, t_rayo ray);
t_intersection	*primaryray(t_scene *scene, t_rayo *ray, int pixelx, int pixelj);
t_color			secondaryray(t_scene *scene, t_rayo ray, t_intersection *intersectionobject);
t_color			ambientlight(t_scene *scene, t_intersection *intersection);
void			writepixelimage(t_libx *libx, t_color color, int x, int y);
t_rayo			cordtoray(int i, int j, t_scene *scene);
t_rayo			raytolight(t_intersection *intersection, t_rayo primary, t_light_point light);
t_intersection	*returnnear(t_intersection *first, t_intersection *second);
t_intersection	*infront(t_intersection *intersection);
int				changecamera(t_scene *scene, int next);
int				esc_hook(int button, void *x);
#endif
