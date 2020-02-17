#include "header.h"

t_square *iniSquare(t_square *square)
{
    /*DIRECTION OF ONE SIDE*/
    square->vect1 = perpendicular_rand_vect(square->normal);
    square->vect2 = crossProduct(square->normal, square->vect1);
    square->vect1 = changeLenght(square->vect1, square->height);
    square->vect2 = changeLenght(square->vect2, square->height);
    
    square->corner.x = square->point.x + square->vect1.x/2 + square->vect2.x/2;
    square->corner.y = square->point.y + square->vect1.y/2 + square->vect2.y/2;
    square->corner.z = square->point.z + square->vect1.z/2 + square->vect2.z/2;
    return (square);
}

/*FIND THE PLANE TO WHICH THE SQUARE BELONGS, AND LATER TEST IF THE POINT IS IN THE SQUARE*/
t_intersection *squareCollision(t_rayo ray, t_square *sq)
{
    t_intersection  *returned;
    t_tupla         equation[4];
    t_tupla         solution;
    t_vect3         puntoInter;
    double          lambda;

    iniSquare(sq);
    lambda = pseudoIntersection(ray, sq->point, sq->normal);
    if (lambda <= 0)
        return (NULL);
    puntoInter = pointRay(ray, lambda);
    solution = vectorialSpaceSolution(sq->corner, sq->vect1, sq->vect2, puntoInter);
    if (solution.tamaño <= 0)
        return (NULL);
    if (!(solution.tupla[0] >= -1 && solution.tupla[0] <= 0 && solution.tupla[1] >= -1 && solution.tupla[1] <= 0)) //negado
        return (NULL);
    returned = ft_calloc(1, sizeof(t_intersection));
    returned->lambda = lambda;
    returned->type = square;
    returned->object = sq;
    returned->normal = sq->normal;
    return (returned);
}

