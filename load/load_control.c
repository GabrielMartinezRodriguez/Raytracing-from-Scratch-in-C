/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_control.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:31:49 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 21:11:53 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "load.h"

void			processstring(t_scene *scene, char **words)
{
	if (ft_strncmp("R", words[0], ft_strlen(words[0])) == 0)
		loadresolution(scene, words);
	else if (ft_strncmp("A", words[0], ft_strlen(words[0])) == 0)
		loadambientlight(scene, words);
	else if (ft_strncmp("c", words[0], ft_strlen(words[0])) == 0)
		loadcamera(scene, words);
	else if (ft_strncmp("l", words[0], ft_strlen(words[0])) == 0)
		loadlight(scene, words);
	else if (ft_strncmp("sp", words[0], ft_strlen(words[0])) == 0)
		loadsphere(scene, words);
	else if (ft_strncmp("pl", words[0], ft_strlen(words[0])) == 0)
		loadplane(scene, words);
	else if (ft_strncmp("sq", words[0], ft_strlen(words[0])) == 0)
		loadsquare(scene, words);
	else if (ft_strncmp("cy", words[0], ft_strlen(words[0])) == 0)
		loadcylinder(scene, words);
	else if (ft_strncmp("tr", words[0], ft_strlen(words[0])) == 0)
		loadtriangle(scene, words);
	else
		error("no se ha reconocido un identificador");
}

int				loadscene(t_scene *scene, char *filename)
{
	char	*line;
	char	**words;
	int		fd;
	int		end;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		error("no se ha podido abrir el fichero indicado");
	end = 0;
	while (!end)
	{
		if (get_next_line(fd, &line) == 0)
			end = 1;
		words = ft_split(line, ' ');
		if (countwords(words) > 0)
			processstring(scene, words);
		freespace(words);
		free(line);
	}
	if (fundamentalsobjects("") == 0)
		error("Faltan objetos necesarios");
	inicamera(scene->camera, scene->resolution);
	close(fd);
	return (0);
}
