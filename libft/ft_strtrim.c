/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strtrim.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/09 13:28:24 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/14 18:12:28 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static int		ft_chrcontainstr(const char c, char const *str)
{
	int flag;
	int i;

	flag = 0;
	i = 0;
	while (str[i] != '\0' && flag == 0)
	{
		if (str[i] == c)
			flag = 1;
		i++;
	}
	return (flag);
}

char			*ft_strtrim(char const *s1, char const *set)
{
	int		len;
	int		i1;
	int		i2;
	char	*a;

	if (s1 == NULL || set == NULL)
		return (NULL);
	len = ft_strlen(s1);
	i1 = 0;
	while (ft_chrcontainstr(s1[i1], set) && i1 < len)
		i1++;
	i2 = len - 1;
	while (i2 >= 0 && ft_chrcontainstr(s1[i2], set))
		i2--;
	if (i2 >= i1)
		a = ft_substr(s1, i1, i2 - i1 + 1);
	else
	{
		return (ft_strdup(""));
	}
	return (a);
}
