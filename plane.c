#include "header.h"

t_plane *iniPlane(t_plane *plane)
{
	double number;

	number = 0;
	number += plane->point.x * plane->normal.x;
	number += plane->point.y * plane->normal.y;
	number += plane->point.z * plane->normal.z;
	plane->hight = number;
	return (plane);
}

t_intersection  *planeCollision(t_rayo ray, t_plane *pl)
{
	t_tupla				equation;
	t_tupla				productoX;
	t_tupla				productoY;
	t_tupla				productoZ;
	t_intersection		*returned;

	iniPlane(pl);
	productoX = newTupla(ray.vector.x ,ray.punto.x);
	productoY = newTupla(ray.vector.y, ray.punto.y);
	productoZ = newTupla(ray.vector.z, ray.punto.z);
	productoX = mulTupla(productoX, pl->normal.x);
	productoY = mulTupla(productoY, pl->normal.y);
	productoZ = mulTupla(productoZ, pl->normal.z);
	equation = addTupla(productoX, productoY);
	equation = addTupla(equation, productoZ);
	equation = solveEquation(equation, pl->hight);
	if (equation.tamaño <= 0)
		return (NULL);
	returned = ft_calloc(1, sizeof(t_intersection));
	returned->object = pl;
	returned->type = plane;
	returned->lambda = lowerNumber(equation);
	returned->normal = pl->normal;
	return (returned);
}

double pseudoIntersection(t_rayo ray, t_vect3 point, t_vect3 normal)
{
    t_plane     pseudoPlane;
    t_intersection *returned;
    double      lambda;

    pseudoPlane.normal = normal;
    pseudoPlane.point = point;
    iniPlane(&pseudoPlane);
    returned = planeCollision(ray, &pseudoPlane);
	if(returned == NULL)
		return (-1);
    lambda = returned->lambda;
    free(returned);
    return(lambda);
}