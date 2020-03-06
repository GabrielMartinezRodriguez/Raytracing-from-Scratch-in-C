/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libx.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/05 20:52:01 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:32:22 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libx.h"

int		mouse_hook(int button, int x, int y, t_libx *param)
{
	printf("button pulsado %d\n", button);
	printf("pixel selecionado =  %d x %d\n", x, y);
	printf("direcion a win ptr %p\n", param->win_ptr);
	return (0);
}

void	ini_libx(t_data_struct *data)
{
	data->libx.ptr = mlx_init();
	data->libx.win_ptr = mlx_new_window(data->libx.ptr, data->scene.resolution.x, data->scene.resolution.y, "raytracing majestuoso");
	mlx_do_key_autorepeatoff(data->libx.ptr);
	mlx_hook(data->libx.win_ptr, 2, 1, key_pressed, &data->scene);
	mlx_mouse_hook(data->libx.win_ptr, &mouse_hook, &data->libx);
}
