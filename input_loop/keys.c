# include "input.h"

int key_pressed(int key, t_scene *scene)
{
    if(key == ESC)
        exit(0);
    if(key == 37 || key == 39)
    {
        if(key == 37)
            changecamera(scene, 0);
        else if(key == 39)
            changecamera(scene, 1);
    }
    return (0);
}