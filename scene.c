#include "header.h"

int     load_scene(t_scene *scene)
{
    scene->lights = ft_calloc(1, sizeof(t_list_light));
    scene->lights->point.color.blue = 255;
    scene->lights->point.color.green = 255;
    scene->lights->point.color.red = 255;
    scene->lights->point.intensity = 1;
    scene->lights->point.point = newVector(-30, 0, 5);
    scene->env_light.intensity = 0.01;
    scene->env_light.color.blue = 255;
    scene->env_light.color.red = 255;
    scene->env_light.color.green = 255;
    return (1);
}

