#ifndef SCENE_EXPORT_H
# define SCENE_EXPORT_H
# include "shared.h"

typedef struct	s_gpu_camera
{
	t_vec4		origin;
	t_vec4		direction;
	float		fov;
}				t_gpu_camera;

typedef struct	s_gpu_scene
{
	t_gpu_object	*objects;
	int				nobjects;
	t_gpu_light		*lights;
	int				nlights;
	t_gpu_camera	*cameras;
	int				ncameras;
	t_vec4			ambient;
	int				width;
	int				height;
}				t_gpu_scene;

void			export_scene(char *path, t_gpu_scene *out);

#endif
