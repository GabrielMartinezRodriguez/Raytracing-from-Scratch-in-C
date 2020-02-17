#include "header.h"
//corregir primer lado de cuadrado

t_cylinder *iniCylinder(t_cylinder *cyl)
{
    cyl->vect1 = perpendicular_rand_vect(cyl->normal);
    cyl->vect2 = crossProduct(cyl->vect1, cyl->normal);
    cyl->normal = changeLenght(cyl->normal, 1);
    cyl->vect1 = changeLenght(cyl->vect1, 1);
    cyl->vect2 = changeLenght(cyl->vect2, 1);
    return (cyl);
}

static void rotateCollision(t_rayo *ray, t_cylinder *cyl, t_matrix3 base)
{
    t_matrix3 inverseBase;

    inverseBase = inverseMatrix(base);
    ray->punto = rotateVector(inverseBase, ray->punto);
    ray->vector = rotateVector(inverseBase, ray->vector);
    cyl->point = rotateVector(inverseBase, cyl->point);
    cyl->normal = rotateVector(inverseBase, cyl->normal);
    cyl->vect1 = rotateVector(inverseBase, cyl->vect1);
    cyl->vect2 = rotateVector(inverseBase, cyl->vect2);
}

static void ini_tops(t_cylinder *cyl)
{
    cyl->upper_top.center = pointVectorPoint(cyl->point, cyl->normal, cyl->height/2);
    cyl->lower_top.center = pointVectorPoint(cyl->point, cyl->normal, -cyl->height/2);
    cyl->lower_top.normal = cyl->normal;
    cyl->upper_top.normal = cyl->normal;
    cyl->upper_top.radius = cyl->diameter/2;
    cyl->lower_top.radius = cyl->diameter/2;
}
t_intersection *cylinderCollision(t_rayo ray, t_cylinder *cyl)
{
    t_matrix3 base;
    t_intersection **intersection;
    t_intersection *returned;

    intersection = ft_calloc(3, sizeof(t_intersection *));
    iniCylinder(cyl);
    ini_tops(cyl);
    base = iniMatrix(cyl->normal, cyl->vect1, cyl->vect2);
    intersection[0] = cylinderCollisionTransformed(ray, *cyl, base);
    intersection[1] = circleCollision(ray, &(cyl->upper_top));
    intersection[2] = circleCollision(ray, &(cyl->lower_top));
    returned = returnNear(intersection[0], intersection[1]);
    returned = returnNear(returned, intersection[2]);
    if(returned == NULL)
        return (NULL);
    returned->object = cyl;
    returned->type = cylinder;
    return(returned);
}

t_intersection *cylinderCollisionTransformed(t_rayo ray, t_cylinder cyl, t_matrix3 matrix)
{
    t_tupla productos[3];
    t_intersection *intersection;
    t_tupla solution;
    t_vect3 point;
    double menor;

    rotateCollision(&ray, &cyl, matrix);
    productos[0] = notableProduct(ray.vector.y, ray.punto.y - cyl.point.y);
    productos[1]= notableProduct(ray.vector.z, ray.punto.z - cyl.point.z);
    productos[2] = addTupla(productos[0], productos[1]);
    solution = solveEquation(productos[2], pow(cyl.diameter/2, 2));
    if(solution.tamaño <= 0)
        return NULL;
    menor = lowerNumber(solution);
    point = pointRay(ray, menor);
    if (pow(point.x - cyl.point.x, 2) > pow(cyl.height/2, 2))
        return (NULL);
    intersection = ft_calloc(1, sizeof(t_intersection));
    intersection->lambda = menor;
    intersection->type = cylinder;
    intersection->normal = point;
    intersection->normal.x = 0;
    intersection->normal = rotateVector(matrix, intersection->normal);
    return (intersection);
}