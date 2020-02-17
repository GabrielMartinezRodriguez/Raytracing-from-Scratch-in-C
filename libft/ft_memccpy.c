/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memccpy.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eligero <eligero@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/07 15:41:45 by eligero           #+#    #+#             */
/*   Updated: 2019/11/07 20:24:28 by eligero          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	*ft_memccpy(void *dst, const void *src, int c, size_t n)
{
	unsigned char		*mdst;
	const unsigned char	*msrc;

	mdst = (unsigned char *)dst;
	msrc = (const unsigned char *)src;
	if (n)
	{
		while (n-- != 0)
			if ((*mdst++ = *msrc++) == (unsigned char)c)
				return (mdst);
	}
	return (NULL);
}
