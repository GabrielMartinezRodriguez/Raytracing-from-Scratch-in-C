/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_atoi.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 17:21:48 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/16 17:21:53 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

int			ft_atoi(const char *str)
{
	int		sign;
	long	nbr;

	sign = 1;
	nbr = 0;
	if (*str == '\0')
		return (0);
	while (ft_isspace(*str))
		str++;
	if (*str == '-' || *str == '+')
		if (*(str++) == '-')
			sign = -1;
	while (*str >= '0' && *str <= '9')
	{
		nbr = (nbr * 10) + (*(str++) - '0');
		if ((sign * nbr) > 2147483647)
			return (-1);
		if ((sign * nbr) < -2147483648)
			return (0);
	}
	return (int)(nbr * sign);
}
