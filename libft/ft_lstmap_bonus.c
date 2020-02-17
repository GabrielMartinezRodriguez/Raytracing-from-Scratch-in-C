/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstmap.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/12 16:37:28 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/12 17:35:03 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

t_list		*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *))
{
	t_list		*lst1;
	t_list		*first;

	(void)(*del);
	if (f == NULL)
		return (NULL);
	if (lst == NULL)
		return (NULL);
	lst1 = ft_lstnew(f(lst->content));
	if (lst1 == NULL)
		return (NULL);
	first = lst1;
	while (lst->next != NULL)
	{
		lst = lst->next;
		lst1->next = ft_lstnew(f(lst->content));
		if (lst1->next == NULL)
			return (NULL);
		lst1 = lst1->next;
	}
	return (first);
}
