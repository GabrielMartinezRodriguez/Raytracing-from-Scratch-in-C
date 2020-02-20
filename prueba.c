#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "libx/mlx.h"

/*int main()
{
    void *ptr;
    void *image;
    int *dir_img;
    void *window;

    int pixel_line;
    int bit_pixel;
    int endian;

    ptr = mlx_init();
    image = mlx_new_image(ptr, 1920, 1080);
    dir_img = (int *)mlx_get_data_addr(image, &bit_pixel, &pixel_line, &endian);
    
    printf("%i %i", bit_pixel, pixel_line);
    for(int j = 0; j < 10; j++)
    {
        for(int i =0; i < 1920; i++)
            dir_img[j * 1920 + i] = 0x00ffffff;
    }
        
    window = mlx_new_window(ptr, 1920, 1080, "PRUEBA");
    mlx_put_image_to_window(ptr, window, image, 00, 0);
    mlx_loop(ptr);
    return 0;
}*/