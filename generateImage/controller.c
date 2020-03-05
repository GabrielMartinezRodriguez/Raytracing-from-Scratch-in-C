/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   controller.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:46:28 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 21:31:40 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

t_color			calccolor(t_intersection *intersection, t_rayo ray, double intesity, t_color colorlight)
{
	t_color color;

	color = reflectedcolor(intersection->color, colorlight);
	color = anglecolor(color, intersection->normal, ray.vector);
	color = intensitycolor(color, intesity);
	return (color);
}

t_color			ambientlight(t_scene *scene, t_intersection *intersection)
{
	t_color	new_color;

	ft_bzero(&new_color, sizeof(new_color));
	new_color = reflectedcolor(intersection->color, scene->env_light.color);
	new_color = intensitycolor(new_color, scene->env_light.intensity);
	return (new_color);
}

void			writepixelimage(t_libx *libx, t_color color, int x, int y)
{
	int bitpixel;
	int size_line;
	int endian;

	libx->img_addr = (int *)mlx_get_data_addr(libx->img_ptr, &bitpixel, &size_line, &endian);
	libx->img_addr[(size_line / 4) * y + x] = colortoint(color);
}

void			showscene(t_libx *libx, t_arg *args, t_scene *scene)
{
	if (args->file_save == NULL)
	{
		mlx_put_image_to_window(libx->ptr, libx->win_ptr, libx->img_ptr, 0, 0);
		mlx_loop(libx->ptr);
	}
	else
		imagetofile((char *)libx->img_addr, args->file_save, scene->resolution);
	mlx_destroy_image(libx->ptr, libx->img_ptr);
}

void			generateimage(t_scene scene, t_libx *libx)
{
	int					i;
	int					j;
	t_intersection		*intersection;
	t_colors_reflected	colors;
	t_rayo				ray;

	libx->img_ptr = mlx_new_image(libx->ptr, scene.resolution.x, scene.resolution.y);
	ft_bzero(&colors, sizeof(t_colors_reflected));
	i = 0;
	j = 0;
	while (i < scene.resolution.y)
	{
		while (j < scene.resolution.x)
		{
			intersection = primaryray(&scene, &ray, j, i);
			if (intersection != NULL)
			{
				colors.color_lights = secondaryray(&scene, ray, intersection);
				colors.color_ambient = ambientlight(&scene, intersection);
				writepixelimage(libx, fusion_colors(colors), j, i);
			}
			if (intersection != NULL)
				free(intersection);
			j++;
		}
		i++;
		j = 0;
	}
}
