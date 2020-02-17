/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strmapi.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/11 19:28:38 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/12 17:44:28 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char	*ft_strmapi(char *s, char (*f)(unsigned int, char))
{
	int		len;
	char	*aux;
	int		i;

	if (s == NULL)
		return (NULL);
	len = ft_strlen(s);
	aux = malloc(len + 1);
	if (aux == NULL)
		return (NULL);
	i = 0;
	while (i++ < len)
		aux[i - 1] = f(i - 1, s[i - 1]);
	aux[i - 1] = '\0';
	return (aux);
}
