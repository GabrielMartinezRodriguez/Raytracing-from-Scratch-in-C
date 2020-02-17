#include "header.h"

double FOV_H_GR = 120;
double FOV_V_RAD;
int RES_H = 1920;
int RES_V = 1080;

double absoluto(double abs)
{
    if(abs < 0)
        return -abs;
    return abs;
}

int main()
{
    int fd;
    double FOV_H_RAD;
    void *ptr = mlx_init();
    void *win_ptr = mlx_new_window(ptr, 1920, 1080, "prueba");

    unsigned int color1 = 0x00ff0000;
    unsigned int color2 = 0x0000ff00;


    t_rayo ray;
    t_esfera sphere;
    t_cylinder cyl;
    t_square sq;
    t_plane pl;
    t_triangle tri;
    t_intersection *inter1;
    t_intersection *inter2;
    t_intersection *inter;

    t_intersection *shader;
    t_intersection *shader1;
    t_intersection *shader2;
    t_tupla *tupla;
    ray.punto.x = 0;
    ray.punto.y = 0;
    ray.punto.z = 0;
    ray.vector.x = 960;
    ray.vector.y = -540;
    ray.vector.z = 960;
    sphere.punto.x = 0;
    sphere.punto.y = 0;
    sphere.punto.z = 30;
    sphere.radio = 10;


    tri.point1.x = 0;
    tri.point1.y = 0;
    tri.point1.z = 70;
    tri.point2.x = 0;
    tri.point2.y = 50;
    tri.point2.z = 500;
    tri.point3.x = 60;
    tri.point3.y = 0;
    tri.point3.z = 70;


    sq.point.x = 10;
    sq.point.y = 10;
    sq.point.z = 30;
    sq.height = 10;
    sq.normal.x = 0;
    sq.normal.y = 0;
    sq.normal.z = 1;

    cyl.diameter = 20;
    cyl.height = 30;
    cyl.point.y = 0;
    cyl.point.x = 0;
    cyl.point.z = 30;
    cyl.normal.x = 1;
    cyl.normal.y = 0;
    cyl.normal.z = -1;

    pl.point.x = 0;
    pl.point.y = 0;
    pl.point.z = 30;
    pl.normal.x = 0;
    pl.normal.z = -1;
    pl.normal.y = 0;

    t_scene     scene;

    load_scene(&scene);
    scene.objets = ft_calloc(1, sizeof(t_list_obj));
    scene.objets->object = &pl;
    scene.objets->type = plane;
    scene.objets->next = ft_calloc(1, sizeof(t_list_obj));
    scene.objets->next->object = &sphere;

    FOV_H_RAD = (FOV_H_GR/360) * 2 * M_PI;
    
    for(int j = 0; j < RES_V; j++)
    {
        for(int i = 0; i < RES_H; i++)
        {
            ray.vector.x = i - RES_H/2;
            ray.vector.y = RES_V/2 - j;
            ray.vector.z = RES_H / (2 * tan(FOV_H_RAD/2));
            ray.punto.x = 0;
            ray.punto.y = 0;
            ray.punto.z = 0;
            inter1 = sphereCollision(ray, &sphere);
            inter2 = planeCollision(ray, &pl);
            inter = returnNear(inter1, inter2);

            if(inter1 != NULL)
                i = i;
            if(inter != NULL)
            {
                shader1 = NULL;
                shader2 = NULL;
                ray.punto = addVector(pointRay(ray, inter->lambda), changeLenght(inter->normal, 0.001));
                ray.vector = distanceVector(scene.lights->point.point, ray.punto);
                shader1 = sphereCollision(ray, &sphere);
                shader2 = planeCollision(ray, &pl);
                shader = returnNear(shader1, shader2);
                if(inter1 == inter && shader != NULL)
                    mlx_pixel_put(ptr, win_ptr, i, j, 0xE6ff0000);
                else if(inter1 == inter)
                    i = i;
                if(inter2 == inter && shader != NULL)
                    mlx_pixel_put(ptr, win_ptr, i, j, 0xE600ff00);
                if(!shader)
                {
                    color1 = ((1 - (absoluto(angleTwoVectors(inter->normal, ray.vector)))) * 255);
                    if(color1 < 25)
                        color1 = 0;
                    else
                        color1 -= 25;
                    if(color1 > 230)
                        i = i;
                    color1 *= pow(256, 3);
                    if(inter1 == inter)
                        mlx_pixel_put(ptr, win_ptr, i, j, color1 + 0x00ff0000);
                    else if(inter2)
                        mlx_pixel_put(ptr, win_ptr, i, j, 0x0000ff00 + color1);
                }
            }
        }
    }
    mlx_loop(ptr);
    return 0;
}

