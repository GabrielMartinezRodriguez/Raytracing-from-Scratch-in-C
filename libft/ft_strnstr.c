/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strnstr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/16 17:34:55 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/16 17:34:58 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t needle_length;

	if (*needle == '\0')
		return ((char *)haystack);
	needle_length = ft_strlen(needle);
	while ((*haystack != '\0') && (len-- >= needle_length))
	{
		if (ft_toupper(*haystack) == ft_toupper(*needle))
			if (ft_strncmp(haystack, needle, needle_length) == 0)
				return ((char *)haystack);
		haystack++;
	}
	return (NULL);
}
