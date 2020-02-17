#include "header.h"

t_intersection  *sphereCollision(t_rayo rayo, t_esfera *esfera)
{
    t_tupla productoX;
    t_tupla productoY;
    t_tupla productoZ;
    t_tupla equation;
    t_intersection *returned;

    productoX = notableProduct(rayo.vector.x, rayo.punto.x - esfera->punto.x);
    productoY = notableProduct(rayo.vector.y, rayo.punto.y - esfera->punto.y);
    productoZ = notableProduct(rayo.vector.z, rayo.punto.z - esfera->punto.z);
    equation = addTupla(productoX, productoY);
    equation = addTupla(equation, productoZ);
    equation = solveEquation(equation, pow(esfera->radio, 2));
    if(equation.tamaño <= 0)
        return (NULL);
    returned = ft_calloc(1, sizeof(t_intersection));
    returned->object = esfera;
    returned->type = sphere;
    returned->lambda = lowerNumber(equation);
    returned->normal = distanceVector(esfera->punto, pointRay(rayo, returned->lambda));
    return (returned);
}
