/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_objects.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 23:31:54 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 23:34:29 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "load.h"

void			loadsphere(t_scene *scene, char **words)
{
	t_list_obj		*lista;
	t_esfera		*sp;

	if (countwords(words) != 4 && countwords(words) != 5)
		error("mal formato de esfera");
	lista = ft_calloc(1, sizeof(t_list_obj));
	lista->type = sphere;
	sp = ft_calloc(1, sizeof(t_esfera));
	sp->punto = chargepoint(words[1]);
	sp->radio = ft_atoi_double(words[2]) / 2;
	sp->color = chargecolor(words[3]);
	lista->object = sp;
	lista->functioncoll = &spherecollision;
	loadreflect(lista, words, 4);
	addobject(scene, lista);
}

void			loadplane(t_scene *scene, char **words)
{
	t_list_obj		*lista;
	t_plane			*pl;

	if (countwords(words) != 4 && countwords(words) != 5)
		error("mal formato de plano");
	lista = ft_calloc(1, sizeof(t_list_obj));
	lista->type = plane;
	pl = ft_calloc(1, sizeof(t_plane));
	pl->point = chargepoint(words[1]);
	pl->normal = chargenormalizedvector(words[2]);
	pl->color = chargecolor(words[3]);
	lista->object = pl;
	lista->functioncoll = &planecollision;
	iniplane(pl);
	loadreflect(lista, words, 4);
	addobject(scene, lista);
}

void			loadsquare(t_scene *scene, char **words)
{
	t_list_obj		*lista;
	t_square		*sq;

	if (countwords(words) != 5 && countwords(words) != 6)
		error("mal formato de cuadrado");
	lista = ft_calloc(1, sizeof(t_list_obj));
	lista->type = square;
	sq = ft_calloc(1, sizeof(t_square));
	sq->point = chargepoint(words[1]);
	sq->normal = chargenormalizedvector(words[2]);
	sq->height = ft_atoi_double(words[3]);
	sq->color = chargecolor(words[4]);
	lista->object = sq;
	lista->functioncoll = &squarecollision;
	inisquare(sq);
	loadreflect(lista, words, 5);
	addobject(scene, lista);
}

void			loadcylinder(t_scene *scene, char **words)
{
	t_list_obj		*lista;
	t_cylinder		*cyl;

	if (countwords(words) != 6 && countwords(words) != 7)
		error("mal formato de cilindro");
	lista = ft_calloc(1, sizeof(t_list_obj));
	lista->type = cylinder;
	cyl = ft_calloc(1, sizeof(t_cylinder));
	cyl->point = chargepoint(words[1]);
	cyl->normal = chargenormalizedvector(words[2]);
	cyl->color = chargecolor(words[3]);
	cyl->diameter = ft_atoi_double(words[4]);
	cyl->height = ft_atoi_double(words[5]);
	lista->object = cyl;
	lista->functioncoll = &cylindercollision;
	inicylinder(cyl);
	ini_tops(cyl);
	loadreflect(lista, words, 6);
	addobject(scene, lista);
}

void			loadtriangle(t_scene *scene, char **words)
{
	t_list_obj		*lista;
	t_triangle		*tri;

	if (countwords(words) != 5 && countwords(words) != 6)
		error("mal formato de triangulo");
	lista = ft_calloc(1, sizeof(t_list_obj));
	lista->type = triangle;
	tri = ft_calloc(1, sizeof(t_triangle));
	tri->point1 = chargepoint(words[1]);
	tri->point2 = chargepoint(words[2]);
	tri->point3 = chargepoint(words[3]);
	tri->color = chargecolor(words[4]);
	lista->object = tri;
	lista->functioncoll = &trianglecollision;
	initriangle(tri);
	loadreflect(lista, words, 5);
	addobject(scene, lista);
}

void			loadreflect(t_list_obj *lista, char **words, int index)
{
	if (countwords(words) <= index)
		return ;
	lista->reflect = ft_atoi_double(words[index]);
	if (lista->reflect < 0 || lista->reflect > 1)
		error("la reflectividad debe estar en el rango [0,1]");
}
