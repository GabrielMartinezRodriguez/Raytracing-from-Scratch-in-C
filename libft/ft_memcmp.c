/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memcmp.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 17:33:57 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/16 17:34:02 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

int		ft_memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *p1;
	const unsigned char *p2;

	if (n != 0)
	{
		p1 = s1;
		p2 = s2;
		while (n-- != 0)
		{
			if (*p1++ != *p2++)
				return ((int)(*(--p1) - *(--p2)));
		}
	}
	return (0);
}
