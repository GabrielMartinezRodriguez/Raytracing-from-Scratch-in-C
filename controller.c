#include "header.h"

t_arg process_args(int arg_n, char **arg_s)
{
    t_arg arg;

    if(arg_n <= 2)
    {
        if(ft_strncmp(&arg_s[1][ft_strlen(arg_s[1])], ".rt", 3) == 0)
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
                if(arg_s > 4)
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
t_intersection *primaryCollision(t_scene *scene, t_rayo ray, int pixelX, int pixelJ)
{
    t_intersection *(collision)(t_rayo, void *);
    t_intersection *moreNear;
    t_intersection *actualIntersection;
    t_list_obj     *actualObject;


    ray = cordToRay(pixelX, pixelJ, scene);
    actualObject = scene->objets;
    while(actualObject != NULL)
    {
        collision = actualObject->functionColl;
        actualIntersection = collision(ray, actualObject->object);
        moreNear = returnNear(moreNear, actualIntersection);
        actualObject = actualObject->next;
    }
    return (moreNear);
}


void generateImage(t_arg arg, t_scene scene, t_libx *libx)
{
    int i;
    int j;
    t_rayo ray;

    libx->ptr = mlx_init();
    libx->img_ptr = mlx_new_image(libx->ptr, scene.resolution.x, scene.resolution.y);
    i = 0;
    j = 0;
    while(i < scene.resolution.y)
    {
        while(j < scene.resolution.x)
        {
            
            j++;
        }
        i++;
        j = 0;
    }
}

int main(int arg_n, char **arg_s)
{
    t_scene scene;
    t_arg args;
    t_libx libx;


    args = process_args(arg_n, arg_s);
    loadScene(&scene, args.file_load);
    generateImage(args, scene, &libx);
    return (0);
}
