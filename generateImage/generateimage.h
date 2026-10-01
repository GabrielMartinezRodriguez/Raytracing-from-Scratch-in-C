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

# include <pthread.h>

# define AA_SAMPLES 3
# define THREADS 14
# define MAX_DEPTH 5
# define GAMMA 2.2
# define SPECULAR 0.4
# define SHININESS 60
# define EPSILON 0.0001

typedef struct	s_hit
{
	t_vect3		point;
	t_vect3		normal;
	t_vect3		dir;
}				t_hit;

typedef struct	s_worker
{
	t_scene		*scene;
	int			*pixels;
	int			line;
	int			index;
}				t_worker;

void			generateimage(t_scene scene, t_libx *libx);
void			showscene(t_libx *libx, t_arg *args, t_scene *scene);
void			inicamera(t_camera *camera, t_resolution resolution);
t_intersection	*raycollision(t_list_obj *objects, t_rayo ray);
t_rayo			cordtoray(t_scene *scene, double i, double j);
t_rgb			trace(t_scene *scene, t_rayo ray, int depth);
t_rgb			newrgb(double r, double g, double b);
t_rgb			tolinear(t_color color, double intensity);
t_rgb			addrgb(t_rgb first, t_rgb second);
t_rgb			mulrgb(t_rgb first, t_rgb second);
t_rgb			scalergb(t_rgb color, double factor);
unsigned int	rgbtoint(t_rgb color);
t_intersection	*returnnear(t_intersection *first, t_intersection *second);
t_intersection	*infront(t_intersection *intersection);
int				changecamera(t_scene *scene, int next);
int				esc_hook(int button, void *x);
#endif
