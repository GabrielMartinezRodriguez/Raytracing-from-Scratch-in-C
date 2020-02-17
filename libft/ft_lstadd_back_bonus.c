/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstadd_back.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/11 22:59:05 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/12 22:41:13 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	ft_lstadd_back(t_list **alst, t_list *new)
{
	t_list *aux;

	aux = *alst;
	if (aux == NULL)
	{
		*alst = new;
		new->next = NULL;
	}
	else
	{
		while (aux->next != NULL)
			aux = aux->next;
		aux->next = new;
		new->next = NULL;
	}
}
