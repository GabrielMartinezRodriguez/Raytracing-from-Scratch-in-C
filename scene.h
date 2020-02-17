#ifndef SCENE_H
# define SCENE_H

# include "header.h"

typedef struct s_tupla
{
        int     tamaño;
        double  tupla[3];
}              t_tupla;


typedef struct s_color
{
        char    red;
        char    green;
        char    blue;
}              t_color;

typedef struct s_vect3
{
        double x;
        double y;
        double z;
}              t_vect3;

typedef struct s_matrix3
{
        t_vect3 vect1;
        t_vect3 vect2;
        t_vect3 vect3;
}              t_matrix3;


typedef struct s_rayo
{
        t_vect3 punto;
        t_vect3 vector;
}              t_rayo;

/*OBJECTS*/                                             
typedef struct s_esfera
{
        t_vect3 punto;
        double  radio;
        t_color color;
}              t_esfera;

typedef struct s_plane
{
        t_vect3 normal;
        t_vect3 point;
        t_color color;
        double  hight;
}               t_plane;

typedef struct s_circle
{
        t_vect3 center;
        t_vect3 normal;
        double radius;
}              t_circle;

typedef struct s_cylinder
{
        t_vect3 point;
        t_vect3 normal;
        double diameter;
        double height;
        t_color color;
        t_vect3 vect1;
        t_vect3 vect2;
        t_circle upper_top;
        t_circle lower_top;
}               t_cylinder;

typedef struct  s_square
{
        t_vect3 point;
        t_vect3 normal;
        double  height;
        t_vect3 corner;
        t_vect3 vect1;
        t_vect3 vect2;
        t_color color;
}               t_square;

typedef struct  s_triangle
{
        t_vect3 point1;
        t_vect3 point2;
        t_vect3 point3;
        t_color s_color;
        t_vect3 vect1;
        t_vect3 vect2;
        t_vect3 normal;
}               t_triangle;

/*OBJECT LIST*/
typedef enum e_object
{
        sphere,
        square,
        plane,
        cylinder,
        triangle,
        circle
}             t_object;

typedef struct s_list_obj
{
        void            *object;
        t_object        type;
        struct s_list_obj      *next;
}               t_list_obj;

/*LIGHTS*/

typedef struct s_light_environmental
{
    double intensity;
    t_color color;
}              t_light_environmental;

typedef struct s_light_point
{
    t_vect3 point;
    double intensity;
    t_color color;
}              t_light_point;

/*LIGHT LIST*/

typedef struct  s_list_light
{
    t_light_point          point;
    struct s_list_light    *next;
}               t_list_light;

typedef struct s_camera
{
        t_vect3 origin;
        t_vect3 direction;
        double  FOV_H;
}               t_camera;

typedef struct s_scene
{
        t_list_obj      *objets;
        t_list_light    *lights;
        t_light_environmental env_light;
        t_camera        camera;
}               t_scene;

/*INTERSECTION WITH OBJECT*/

typedef struct   s_intersection
{
    double      lambda;
    void        *object;
    t_vect3     normal;
    t_object    type;
}               t_intersection;

#endif