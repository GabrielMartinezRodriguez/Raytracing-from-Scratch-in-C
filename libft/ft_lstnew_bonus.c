/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstnew.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/11 22:38:33 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/11 23:37:42 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

t_list		*ft_lstnew(void const *content)
{
	t_list *ptr;

	ptr = malloc(sizeof(t_list));
	if (ptr != NULL)
	{
		ptr->content = (void *)content;
		ptr->next = NULL;
	}
	return (ptr);
}
