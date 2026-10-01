/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   controller.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:46:28 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:18:21 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

void			showscene(t_libx *libx, t_arg *args, t_scene *scene)
{
	if (args->file_save == NULL)
		mlx_put_image_to_window(libx->ptr, libx->win_ptr, libx->img_ptr, 0, 0);
	else
		imagetofile((char *)libx->img_addr, args->file_save, scene->resolution);
	mlx_destroy_image(libx->ptr, libx->img_ptr);
}

/*
** Supersampling: AA_SAMPLES x AA_SAMPLES rayos por pixel, repartidos en
** rejilla, y se promedia el color en espacio lineal.
*/

static t_rgb	renderpixel(t_scene *scene, int x, int y)
{
	t_rgb	color;
	int		sx;
	int		sy;

	color = newrgb(0, 0, 0);
	sy = 0;
	while (sy < AA_SAMPLES)
	{
		sx = 0;
		while (sx < AA_SAMPLES)
		{
			color = addrgb(color, trace(scene, cordtoray(scene,
				x + (sx + 0.5) / AA_SAMPLES - 0.5,
				y + (sy + 0.5) / AA_SAMPLES - 0.5), 0));
			sx++;
		}
		sy++;
	}
	return (scalergb(color, 1.0 / (AA_SAMPLES * AA_SAMPLES)));
}

static void		*renderrows(void *arg)
{
	t_worker	*worker;
	int			x;
	int			y;

	worker = arg;
	y = worker->index;
	while (y < worker->scene->resolution.y)
	{
		x = 0;
		while (x < worker->scene->resolution.x)
		{
			worker->pixels[worker->line * y + x] =
				rgbtoint(renderpixel(worker->scene, x, y));
			x++;
		}
		y += THREADS;
	}
	return (NULL);
}

void			generateimage(t_scene scene, t_libx *libx)
{
	pthread_t	threads[THREADS];
	t_worker	workers[THREADS];
	int			bitpixel;
	int			size_line;
	int			endian;
	int			i;

	libx->img_ptr = mlx_new_image(libx->ptr, scene.resolution.x, scene.resolution.y);
	libx->img_addr = (int *)mlx_get_data_addr(libx->img_ptr, &bitpixel, &size_line, &endian);
	i = -1;
	while (++i < THREADS)
	{
		workers[i].scene = &scene;
		workers[i].pixels = libx->img_addr;
		workers[i].line = size_line / 4;
		workers[i].index = i;
		pthread_create(&threads[i], NULL, renderrows, &workers[i]);
	}
	i = -1;
	while (++i < THREADS)
		pthread_join(threads[i], NULL);
}
