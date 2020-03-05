/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:17:37 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 21:06:59 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "header.h"

int				main(int arg_n, char **arg_s)
{
	t_scene		scene;
	t_arg		args;
	t_libx		libx;

	
	ini_libx(&libx, &scene);
	args = process_args(arg_n, arg_s);
	loadscene(&scene, args.file_load);
	generateimage(scene, &libx);
	showscene(&libx, &args, &scene);
	return (0);
}
