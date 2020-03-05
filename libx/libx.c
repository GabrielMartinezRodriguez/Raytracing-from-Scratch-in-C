/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libx.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/05 20:52:01 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 21:05:49 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libx.h"

void ini_libx(t_libx *libx, t_scene *scene)
{
    	libx->ptr = mlx_init();
	    libx->win_ptr = mlx_new_window(libx->ptr, scene->resolution.x, scene->resolution.y, "raytracing majestuoso");
        mlx_hook(libx->win_ptr, 2, 1, key_pressed, scene);
}