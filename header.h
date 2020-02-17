#ifndef HEADER_H
# define HEADER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "libx/mlx.h"
#include "libft/libft.h"
#include "scene.h"

int     loadScene(t_scene *scene);
t_intersection  *sphereCollision(t_rayo rayo, t_esfera *esfera);
t_intersection *cylinderCollisionTransformed(t_rayo ray, t_cylinder cyl, t_matrix3 matrix);
t_intersection *squareCollision(t_rayo ray, t_square *sq);
t_tupla notableProduct(double first, double second);
t_tupla addTupla(t_tupla first, t_tupla second);
t_tupla newTupla(double first, double second);
t_tupla mulTupla(t_tupla tupla, double number);

t_tupla solveEquation(t_tupla equation, double equal);
t_tupla solveEquation2(t_tupla equation, double equal);
t_tupla solveEquation1(t_tupla equation, double equal);


t_plane         *iniPlane(t_plane *plane);
t_intersection  *planeCollision(t_rayo ray, t_plane *pl);
double pseudoIntersection(t_rayo ray, t_vect3 point, t_vect3 normal);

t_vect3 crossProduct(t_vect3 first, t_vect3 second);
t_vect3 changeLenght(t_vect3 vector, double module);
t_vect3 perpendicular_rand_vect(t_vect3 vector);

t_intersection *cylinderCollision(t_rayo ray, t_cylinder *cyl);

double distancePoint(t_vect3 point1, t_vect3 point2);

t_intersection *circleCollision(t_rayo ray, t_circle *cir);
t_vect3 pointVectorPoint(t_vect3 point, t_vect3 vect, double lambda);
t_vect3 distanceVector(t_vect3 point1, t_vect3 point2);
t_intersection *returnNear(t_intersection *first, t_intersection *second);

t_tupla vectorialSpaceSolution(t_vect3 origin, t_vect3 vect1, t_vect3 vect2, t_vect3 point);
t_intersection *triangleCollision(t_rayo ray, t_triangle *tri);
void iniTriangle(t_triangle *triangle);

t_vect3 rotateVector(t_matrix3 matrix, t_vect3 vector);
t_matrix3 inverseMatrix(t_matrix3 matrix);
double detMatrix(t_matrix3 matrix);
t_matrix3 attachedMatrix(t_matrix3 matrix);
double subMatrixDet(t_matrix3 matrix, int i, int j);
double **t_matrixTomatrix(t_matrix3 matrix);
void set_index_matrix(t_matrix3 *matrix, int i, int j,double value);
double get_index_matrix(t_matrix3 matrix, int i, int j);
t_matrix3 transposeMatrix(t_matrix3 matrix);
t_matrix3 iniMatrix(t_vect3 first, t_vect3 second, t_vect3 third);
double moduleVector(t_vect3 vector);
t_vect3 pointRay(t_rayo ray, double lamda);
double lowerNumber(t_tupla tupla);
t_vect3 newVector(double x, double y, double z);
t_vect3 addVector(t_vect3 vect1, t_vect3 vect2);
double angleTwoVectors(t_vect3 vect1, t_vect3 vect2);
#endif