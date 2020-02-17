/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/09 13:53:10 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/16 18:22:08 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static void		odionorminette(char **aux, int *ini, int *seeking, int *i)
{
	aux[0] = NULL;
	*ini = 0;
	*seeking = 1;
	*i = 0;
}

static char		*ft_strndup(const char *str1, int n, char c)
{
	char	*aux;
	int		i1;
	int		i2;

	i1 = 0;
	if (str1[n + 1] == '\0' && str1[n] != c)
		n++;
	while (str1[i1] != '\0')
		i1++;
	aux = malloc(i1 + 1);
	i2 = 0;
	while (i2 < i1 && i2 < n)
	{
		aux[i2] = str1[i2];
		i2++;
	}
	aux[i2] = '\0';
	return (aux);
}

char			**ft_split(char const *s, char c)
{
	int		i;
	char	**aux;
	char	*ptr;
	int		ini;
	int		seeking;

	aux = malloc(sizeof(char *));
	if (aux == NULL || s == NULL)
		return (NULL);
	odionorminette(aux, &ini, &seeking, &i);
	while (s[i++] != '\0')
	{
		if (seeking && s[i - 1] != c)
		{
			ini = i - 1;
			seeking = 0;
		}
		else if (seeking == 0 && (s[i - 1] == c || s[i] == '\0'))
		{
			seeking = 1;
			ptr = ft_strndup(&s[ini], i - ini - 1, c);
			aux = ft_insert_string(aux, ptr);
		}
	}
	return (aux);
}
