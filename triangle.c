#include "header.h"

void iniTriangle(t_triangle *triangle)
{
    triangle->vect1 =  distanceVector(triangle->point2, triangle->point1);
    triangle->vect2 = distanceVector(triangle->point3, triangle->point1);
    triangle->normal = crossProduct(triangle->vect1, triangle->vect2);
}

t_intersection *triangleCollision(t_rayo ray, t_triangle *tri)
{
    double lambda;
    t_intersection *returned;
    t_tupla solution;
    t_tupla equation[4];
    t_vect3 pointInter;

    iniTriangle(tri);
    lambda = pseudoIntersection(ray, tri->point1, tri->normal);
    if(lambda <= 0)
        return (NULL);
    pointInter = pointRay(ray, lambda);
    solution = vectorialSpaceSolution(tri->point1, tri->vect1, tri->vect2, pointInter);
    if (solution.tamaño  <= 0)
        return (NULL);
    if (!(solution.tupla[0] >= 0 && solution.tupla[1] >= 0 && (solution.tupla[0] + solution.tupla[1] <= 1))) //negado
        return (NULL);
    returned = ft_calloc(1, sizeof(t_intersection));
    returned->lambda = lambda;
    returned->type = triangle;
    returned->object  = tri;
    returned->normal = tri->normal;
    return(returned);
}