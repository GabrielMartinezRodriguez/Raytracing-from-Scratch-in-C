/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memmove.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 17:34:16 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/16 17:34:18 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	*ft_memmove(void *dst, const void *src, size_t len)
{
	const char	*f;
	char		*t;

	f = src;
	t = dst;
	if ((f != NULL || t != NULL) && len != 0)
	{
		if (f < t)
		{
			f += len;
			t += len;
			while (len-- > 0)
				*(--t) = *(--f);
		}
		else
			while (len-- > 0)
				*t++ = *f++;
	}
	return (dst);
}
