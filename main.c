/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:17:37 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:26:16 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

int				main(int arg_n, char **arg_s)
{
	t_data_struct data;

	data.args = process_args(arg_n, arg_s);
	loadscene(&data.scene, data.args.file_load);
	ini_libx(&data);
	generateimage(data.scene, &data.libx);
	showscene(&data.libx, &data.args, &data.scene);
	if (data.args.file_save == NULL)
		mlx_loop(data.libx.ptr);
	return (0);
}
