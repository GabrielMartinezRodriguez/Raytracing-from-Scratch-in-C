/*
** Puente entre el cargador de escenas original (load/) y la GPU: carga el
** .rt con loadscene() y aplana las listas enlazadas en arrays contiguos
** que se suben tal cual a buffers de Metal.
*/

#include "../header.h"
#include "scene_export.h"

static t_vec4	v4(t_vect3 v)
{
	return ((t_vec4){v.x, v.y, v.z, 0});
}

static t_vec4	unit(t_vect3 v)
{
	return (v4(changelenght(v, 1)));
}

static t_vec4	linear(t_color c, double intensity)
{
	return ((t_vec4){pow(c.red / 255.0, 2.2) * intensity,
		pow(c.green / 255.0, 2.2) * intensity,
		pow(c.blue / 255.0, 2.2) * intensity, 0});
}

static void		export_object(t_list_obj *node, t_gpu_object *out)
{
	t_esfera	*sp;
	t_plane		*pl;
	t_square	*sq;
	t_cylinder	*cy;
	t_triangle	*tr;

	ft_bzero(out, sizeof(t_gpu_object));
	out->reflect = node->reflect;
	if (node->type == sphere && (sp = node->object))
	{
		*out = (t_gpu_object){v4(sp->punto), {0}, {0}, linear(sp->color, 1),
			sp->radio, 0, node->reflect, GPU_SPHERE};
	}
	else if (node->type == plane && (pl = node->object))
	{
		*out = (t_gpu_object){v4(pl->point), unit(pl->normal), {0},
			linear(pl->color, 1), 0, 0, node->reflect, GPU_PLANE};
	}
	else if (node->type == square && (sq = node->object))
	{
		*out = (t_gpu_object){v4(sq->point), unit(sq->normal),
			unit(sq->vect1), linear(sq->color, 1), 0, sq->height,
			node->reflect, GPU_SQUARE};
	}
	else if (node->type == cylinder && (cy = node->object))
	{
		*out = (t_gpu_object){v4(cy->point), unit(cy->normal), {0},
			linear(cy->color, 1), cy->diameter / 2, cy->height,
			node->reflect, GPU_CYLINDER};
	}
	else if (node->type == triangle && (tr = node->object))
	{
		*out = (t_gpu_object){v4(tr->point1), v4(tr->point2),
			v4(tr->point3), linear(tr->color, 1), 0, 0, node->reflect,
			GPU_TRIANGLE};
	}
}

static int		count_objects(t_scene *scene, int *nlights, int *ncameras)
{
	t_list_obj		*obj;
	t_list_light	*light;
	t_camera		*camera;
	int				count;

	count = 0;
	obj = scene->objets;
	while (obj && ++count)
		obj = obj->next;
	*nlights = 0;
	light = scene->lights;
	while (light && ++*nlights)
		light = light->next;
	*ncameras = 0;
	camera = scene->camera;
	while (camera && ++*ncameras)
		camera = camera->next;
	return (count);
}

void			export_scene(char *path, t_gpu_scene *out)
{
	t_scene			scene;
	t_list_obj		*obj;
	t_list_light	*light;
	t_camera		*camera;
	int				i;

	ft_bzero(&scene, sizeof(t_scene));
	loadscene(&scene, path);
	out->nobjects = count_objects(&scene, &out->nlights, &out->ncameras);
	out->objects = ft_calloc(out->nobjects + 1, sizeof(t_gpu_object));
	out->lights = ft_calloc(out->nlights + 1, sizeof(t_gpu_light));
	out->cameras = ft_calloc(out->ncameras + 1, sizeof(t_gpu_camera));
	out->ambient = linear(scene.env_light.color, scene.env_light.intensity);
	out->width = scene.resolution.x;
	out->height = scene.resolution.y;
	i = 0;
	obj = scene.objets;
	while (obj && (export_object(obj, &out->objects[i++]), 1))
		obj = obj->next;
	i = 0;
	light = scene.lights;
	while (light)
	{
		out->lights[i].position = v4(light->point.point);
		out->lights[i++].color = linear(light->point.color,
			light->point.intensity);
		light = light->next;
	}
	i = 0;
	camera = scene.camera;
	while (camera)
	{
		out->cameras[i].origin = v4(camera->origin);
		out->cameras[i].direction = v4(camera->direction);
		out->cameras[i++].fov = camera->fov_h;
		camera = camera->next;
	}
}
