/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memchr.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 17:33:49 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/19 16:08:52 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	*ft_memchr(const void *s, int c, size_t n)
{
	const char *p;

	if (n != 0)
	{
		p = s;
		while (n-- != 0)
		{
			if (*p++ == (char)c)
				return ((void *)(p - 1));
		}
	}
	return (NULL);
}
