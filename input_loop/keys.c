/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keys.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/06 21:33:43 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/06 21:33:54 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input.h"

int		key_pressed(int key, t_data_struct *data)
{
	int changed;

	changed = 0;
	if (key == ESC)
		exit(0);
	if (key == 123 || key == 124)
	{
		if (key == 123)
			changed = changecamera(&data->scene, 0);
		else if (key == 124)
			changed = changecamera(&data->scene, 1);
		if (changed == 1)
		{
			generateimage(data->scene, &data->libx);
			showscene(&data->libx, &data->args, &data->scene);
		}
	}
	return (0);
}
