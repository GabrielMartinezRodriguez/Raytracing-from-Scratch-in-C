#include "header.h"

t_arg process_args(int arg_n, char **arg_s)
{
    t_arg arg;

    ft_bzero(&arg, sizeof(t_arg));
    if(arg_n >= 2)
    {
        if(ft_strncmp(&arg_s[1][ft_strlen(arg_s[1]) - 3], ".rt", 3) == 0)
        {
            arg.file_load = ft_strdup(arg_s[1]);
            if(arg_n < 3)
                return(arg);
            if(ft_strncmp(arg_s[2], "--save", ft_strlen(arg_s[2])) == 0)
            {
                if(arg_n == 3)
                    arg.file_save = ft_strdup("scene.bmp");
                else
                    arg.file_save = ft_strjoin(arg_s[3], ".bmp");
                if(arg_n > 4)
                    error("Ha excedido la cantidad maxima de argumentos");
            }
        }
        else
            error("El fichero de escena debe ser de extension .rt");
    }
    else
        error("Es necesario indicar el fichero desde el cual se cargara la escena");
    return(arg);

}

t_color calcColor(t_intersection *intersection, t_rayo ray, double intesity, t_color colorLight)
{
    t_color color;

    color = reflectedColor(intersection->color, colorLight);
    color = angleColor(color, intersection->normal, ray.vector);
    color = intensityColor(color, intesity);
    return (color);
}
t_intersection *rayCollision(t_list_obj *objects, t_rayo ray)
{
    t_intersection *(*collision)(t_rayo, void *);
    t_intersection *moreNear;
    t_intersection *actualIntersection;
    t_list_obj     *actualObject;

    actualObject = objects;
    moreNear = NULL;
    while(actualObject != NULL)
    {
        collision = actualObject->functionColl;
        actualIntersection = (*collision)(ray, actualObject->object);
        moreNear = returnNear(moreNear, actualIntersection);
        actualObject = actualObject->next;
    }
    return (moreNear);
}
t_intersection *primaryCollision(t_scene *scene, t_rayo *ray,int pixelX, int pixelJ)
{
    t_intersection *intersection;

    *ray = cordToRay(pixelX, pixelJ, scene);
    intersection = rayCollision(scene->objets, *ray);
    return (intersection);
}

t_color     secondaryCollision(t_scene *scene, t_rayo ray, t_intersection *intersectionObject)
{
    t_list_light *light;
    t_intersection  *lightObstacle;
    double          distance[2];
    t_color         color;

    light = scene->lights;
    ft_bzero(&color, sizeof(t_color));
    while (light != NULL)
    {
        lightObstacle = NULL;
        ray = rayToLight(intersectionObject, ray, light->point);
        distance[0] = distancePoint(ray.punto, light->point.point);
        lightObstacle = rayCollision(scene->objets, ray);
        if(lightObstacle != NULL)
            distance[1] = distancePoint(pointRay(ray, lightObstacle->lambda), ray.punto);
        if(distance[0] < distance[1] || lightObstacle == NULL)
            color = addColor(calcColor(intersectionObject, ray, light->point.intensity, light->point.color), color);
        light = light->next;
    }
    return (color);
}
t_color     ambientLight(t_scene *scene, t_intersection *intersection)
{
    t_color new_color;

    ft_bzero(&new_color, sizeof(new_color));
    new_color = reflectedColor(intersection->color, scene->env_light.color);
    new_color = intensityColor(new_color, scene->env_light.intensity);
    return (new_color);
}

t_color fusion_colors(t_colors_reflected colors)
{
    t_color color;

    color = addColor(colors.color_ambient, colors.color_lights);
    return (color);
}

void writePixelImage(t_libx *libx, t_color color, int x, int y)
{
    int bitPixel;
    int size_line;
    int endian;

    libx->img_addr = (int *)mlx_get_data_addr(libx->img_ptr, &bitPixel, &size_line, &endian);
    libx->img_addr[(size_line/4) * y + x] = colorToint(color);
}

void showScene(t_libx *libx, t_arg *args, t_scene *scene)
{
    if(args->file_save == NULL)
        mlx_put_image_to_window(libx->ptr, libx->win_ptr, libx->img_ptr, 0, 0);
    else
        imageToFile((char *)libx->img_addr, args->file_save, scene->resolution);
    mlx_destroy_image(libx->ptr, libx->img_ptr);
}

void generateImage(t_scene scene, t_libx *libx)
{
    int i;
    int j;
    t_intersection *intersection;
    t_colors_reflected       colors;
    t_rayo           ray;

    libx->ptr = mlx_init();
    libx->win_ptr = mlx_new_window(libx->ptr, scene.resolution.x, scene.resolution.y, "RAYTRACING MAJESTUOSO");
    libx->img_ptr = mlx_new_image(libx->ptr, scene.resolution.x, scene.resolution.y);
    ft_bzero(&colors, sizeof(t_colors_reflected));
    i = 0;
    j = 0;
    while(i < scene.resolution.y)
    {
        while(j < scene.resolution.x)
        {
            intersection = primaryCollision(&scene, &ray, j, i);
            if(intersection != NULL)
            {
                colors.color_lights = secondaryCollision(&scene, ray, intersection);
                colors.color_ambient = ambientLight(&scene, intersection);
                writePixelImage(libx, fusion_colors(colors), j, i);
            }
            if(intersection != NULL)
                free(intersection);
            j++;
        }
        i++;
        j = 0;
    }
}
 int size_line;
int                 mouse_hook(int button, int x, int y, t_libx *param)
{
    printf("button pulsado %d\n", button);
    printf("pixel selecionado =  %d x %d\n", x, y);
    printf("direcion a win ptr %p\n", param->win_ptr);
    printf("%d", size_line/4);
    return 0;
}

int                esc_hook(int button)
{
    if(button == ESC)
        exit(1);
    return 0;
}

int main(int arg_n, char **arg_s)
{
    t_scene scene;
    t_arg args;
    t_libx libx;
    args = process_args(arg_n, arg_s);
    loadScene(&scene, args.file_load);
    generateImage(scene, &libx);
    showScene(&libx, &args, &scene);
    mlx_key_hook(libx.win_ptr, &esc_hook, &libx);
    mlx_loop(libx.ptr);
    return (0);
}
