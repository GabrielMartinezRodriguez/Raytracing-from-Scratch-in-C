#ifndef HEADER_H
# define HEADER_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "libx/mlx.h"
#include "libft/libft.h"
#include "get_next_line/get_next_line.h"
#include "scene.h"

typedef struct s_arg
{
    char *file_save;
    char *file_load;
}              t_arg;

typedef struct s_libx
{
    void *ptr;
    void *win_ptr;
    void *img_ptr;
    int *img_addr;
}              t_libx;

t_cylinder *iniCylinder(t_cylinder *cyl);
void iniCamera(t_camera *camera, t_resolution resolution);
t_color angleColor(t_color color, t_vect3 normal, t_vect3 rayDir);
t_color reflectedColor(t_color colorObject, t_color colorLight);
t_color intensityColor(t_color color, double intensity);
t_color addColor(t_color color1, t_color color2);
unsigned int colorToint(t_color color);
double absoluto(double abs);
t_rayo cordToRay(int i, int j, t_scene *scene);
t_rayo rayToLight(t_intersection *intersection, t_rayo primary, t_light_point light);
void error(char *s);
int loadScene(t_scene *scene, char *fileName);
t_intersection  *sphereCollision(t_rayo rayo, t_esfera *esfera);
t_intersection *cylinderCollisionTransformed(t_rayo ray, t_cylinder cyl);
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
t_vect3 mulVector(t_vect3 vector, double number);
double angleTwoVectors(t_vect3 vect1, t_vect3 vect2);
#endif