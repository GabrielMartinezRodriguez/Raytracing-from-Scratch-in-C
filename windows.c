#include "header.h"

double FOV_H_GR = 120;
double FOV_V_RAD;
int RES_H = 1920;
int RES_V = 1080;

int main()
{
    int fd;
    double FOV_H_RAD;
    void *ptr = mlx_init();
    void *win_ptr = mlx_new_window(ptr, 1920, 1080, "prueba");

    t_rayo ray;
    t_esfera sphere;
    t_cylinder cyl;
    t_square sq;
    t_plane pl;
    t_triangle tri;
    t_intersection *inter1;
    t_intersection *inter2;
    t_intersection *inter;

    t_tupla *tupla;
    ray.punto.x = 0;
    ray.punto.y = 0;
    ray.punto.z = 0;
    ray.vector.x = 960;
    ray.vector.y = -540;
    ray.vector.z = 960;
    sphere.punto.x = 0;
    sphere.punto.y = 0;
    sphere.punto.z = 500;
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
    pl.normal.x = 1;
    pl.normal.z = 0;
    pl.normal.y = 0;

    FOV_H_RAD = (FOV_H_GR/360) * 2 * M_PI;
    ray.vector.z = RES_H / (2 * tan(FOV_H_RAD/2));
    for(int j = 0; j < RES_V; j++)
    {
        for(int i = 0; i < RES_H; i++)
        {
            ray.vector.x = i - RES_H/2;
            ray.vector.y = RES_V/2 - j;
            inter1 = sphereCollision(ray, &sphere);
            inter2 = triangleCollision(ray, &tri);
            inter = returnNear(inter1, inter2);
            if(inter != NULL)
            {
                if(inter1 == inter)
                    mlx_pixel_put(ptr, win_ptr, i, j, 0x00ff0000)                           ;
                else if(inter2)
                    mlx_pixel_put(ptr, win_ptr, i, j, 0x0000ff00);
            }
        }
    }
    mlx_loop(ptr);
    return 0;
}

int main()
{
    t_scene     scene;

    load_scene(scene, );
}