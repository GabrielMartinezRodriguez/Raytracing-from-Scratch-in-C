/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/09 11:42:44 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/19 18:38:27 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char			*ft_strjoin(char const *s1, char const *s2)
{
	char	*aux;
	int		i;
	int		t_s[2];

	if (s1 == NULL || s2 == NULL)
		return (NULL);
	t_s[0] = ft_strlen(s2);
	t_s[1] = ft_strlen(s1);
	aux = malloc(t_s[1] + t_s[0] + 1);
	if (aux == NULL)
		return (NULL);
	i = 0;
	while (i < t_s[1])
	{
		aux[i] = s1[i];
		i++;
	}
	i = 0;
	while (i < t_s[0])
	{
		aux[i + t_s[1]] = s2[i];
		i++;
	}
	aux[i + t_s[1]] = '\0';
	return (aux);
}
