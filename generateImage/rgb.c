/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rgb.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/01 13:45:00 by gmartine          #+#    #+#             */
/*   Updated: 2026/10/01 13:45:00 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "generateimage.h"

t_rgb			newrgb(double r, double g, double b)
{
	t_rgb color;

	color.r = r;
	color.g = g;
	color.b = b;
	return (color);
}

t_rgb			tolinear(t_color color, double intensity)
{
	return (newrgb(pow(color.red / 255.0, GAMMA) * intensity,
		pow(color.green / 255.0, GAMMA) * intensity,
		pow(color.blue / 255.0, GAMMA) * intensity));
}

t_rgb			addrgb(t_rgb first, t_rgb second)
{
	return (newrgb(first.r + second.r, first.g + second.g,
		first.b + second.b));
}

t_rgb			mulrgb(t_rgb first, t_rgb second)
{
	return (newrgb(first.r * second.r, first.g * second.g,
		first.b * second.b));
}

t_rgb			scalergb(t_rgb color, double factor)
{
	return (newrgb(color.r * factor, color.g * factor, color.b * factor));
}

static int		tochannel(double value)
{
	if (value > 1)
		value = 1;
	if (value < 0)
		value = 0;
	return ((int)(pow(value, 1 / GAMMA) * 255 + 0.5));
}

unsigned int	rgbtoint(t_rgb color)
{
	return (tochannel(color.r) << 16 | tochannel(color.g) << 8
		| tochannel(color.b));
}
