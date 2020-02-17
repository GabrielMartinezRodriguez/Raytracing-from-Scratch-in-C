/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstclear_bonus.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/11 23:50:00 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/14 17:55:42 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	ft_lstclear(t_list **lst, void (*del)(void *))
{
	t_list *aux;
	t_list *lst1;

	lst1 = *lst;
	if (*lst || lst)
	{
		while (lst1->next != NULL)
		{
			aux = lst1->next;
			del(lst1->content);
			free(lst1);
			lst1 = aux;
		}
		del(lst1->content);
		free(lst1);
		*lst = NULL;
	}
}
