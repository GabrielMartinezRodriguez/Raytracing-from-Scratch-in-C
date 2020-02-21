#include "header.h"

void iniCamera(t_camera *camera, t_resolution resolution)
{
    camera->vectorX = perpendicular_rand_vect(camera->direction);
    camera->vectorY = crossProduct(camera->vectorX, camera->direction);
    camera->vectorX = changeLenght(camera->vectorX, 1);
    camera->vectorY = changeLenght(camera->vectorY, 1);
    camera->direction = changeLenght(camera->direction, 1);
    camera->FOV_H_RAD = (camera->FOV_H/360) * 2 * M_PI;
    camera->depth = resolution.x / (2 * tan(camera->FOV_H_RAD/2));
}