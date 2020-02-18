/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/19 16:07:01 by gmartine          #+#    #+#             */
/*   Updated: 2019/11/20 13:52:50 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

static void		sub_loop(char **str, char *line[2])
{
	char *aux;

	if ((aux = ft_memchr(str[0], '\n', ft_strlen(str[0]))))
	{
		aux[0] = '\0';
		line[0] = ft_strjoin(line[0], str[0]);
		free(line[1]);
		str[0] = ft_strdup(aux + 1);
		free(str[1]);
	}
	else
	{
		line[0] = ft_strjoin(line[0], str[0]);
		free(line[1]);
		str[0] = NULL;
		free(str[1]);
	}
	str[1] = line[0];
}

static int		test(int fd, char **memory, char **line)
{
	if (fd >= 0 && read(fd, memory[fd], 0) == 0 &&
	line != NULL && BUFFER_SIZE > 0)
		return (1);
	return (0);
}

static void		ini(int fd, char **memory, char **str)
{
	if (memory[fd] == NULL)
		str[0] = ft_strdup("");
	else
	{
		str[0] = ft_strjoin("", memory[fd]);
		free(memory[fd]);
	}
	str[1] = str[0];
}

static int		loop(int fd, char **str, char *buffer)
{
	int		read_history;
	char	*line[2];

	read_history = 1;
	line[0] = ft_strdup("");
	line[1] = line[0];
	while (!ft_memchr(str[0], '\n', ft_strlen(str[0])) &&
	(read_history = read(fd, buffer, BUFFER_SIZE)) > 0)
	{
		buffer[read_history] = '\0';
		line[0] = ft_strjoin(line[0], str[0]);
		free(line[1]);
		line[1] = line[0];
		str[0] = ft_strdup(buffer);
		free(str[1]);
		str[1] = str[0];
	}
	sub_loop(str, line);
	return (read_history);
}

int				get_next_line(int fd, char **line)
{
	static char		*memory[4096];
	char			buffer[BUFFER_SIZE + 1];
	char			*str[2];
	int				read_history;

	if (!test(fd, memory, line))
		return (-1);
	ini(fd, memory, str);
	read_history = loop(fd, str, buffer);
	if (read_history < 0)
		return (-1);
	memory[fd] = str[0];
	*line = str[1];
	if (read_history > 0)
		return (1);
	return (0);
}
