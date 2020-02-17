#include "header.h"

int     loadScene(t_scene *scene)
{
    scene->lights = ft_calloc(1, sizeof(t_list_light));
    scene->lights->point.color.blue = 255;
    scene->lights->point.color.green = 255;
    scene->lights->point.color.red = 255;
    scene->lights->point.intensity = 1;
    scene->lights->point.point = newVector(0, 0, 0);
}

