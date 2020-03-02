#include "header.h"

t_bmp_file iniBMP(char *name, unsigned short int bitPerPixel, t_resolution resolution)
{
    t_bmp_file image;

    image.file_name = name;
    image.magicFile[0] = 0x42;
    image.magicFile[1] = 0x4D;
    image.reserved[0] = 0x0;
    image.reserved[1] = 0x0;
    image.pixelData = 0x36;
    image.header_size = 0x28;
    image.resolution.x = resolution.x;
    image.resolution.y = resolution.y;
    image.planesColor = 0x1;
    image.bitsPerPixel = bitPerPixel;
    image.comprension = 0x0;
    image.pixelsPerMeterX = 0x0;
    image.pixelsPerMeterY = 0x0;
    image.totalColors = 0x0;
    image.importantColors = 0x0;
    image.fileSize = image.bitsPerPixel * image.resolution.x
    * image.resolution.y + image.pixelData;
    image.imageSize = image.bitsPerPixel * image.resolution.x
    * image.resolution.y;
    return (image);
}
void writeBMP(int fd, char *image, t_bmp_file file)
{
    int x;
    int y;
    long int aux;
    write(fd, &file.magicFile, sizeof(file.magicFile));
    write(fd, &file.fileSize, sizeof(file.fileSize));
    write(fd, &file.reserved, sizeof(file.reserved));
    write(fd, &file.pixelData, sizeof(file.pixelData));
    write(fd, &file.header_size, sizeof(file.header_size));
    write(fd, &file.resolution, sizeof(file.resolution));
    write(fd, &file.planesColor, sizeof(file.planesColor));
    write(fd, &file.bitsPerPixel, sizeof(file.bitsPerPixel));
    write(fd, &file.comprension, sizeof(file.comprension));
    write(fd, &file.imageSize, sizeof(file.imageSize));
    write(fd, &file.pixelsPerMeterX, sizeof(file.pixelsPerMeterX));
    write(fd, &file.pixelsPerMeterY, sizeof(file.pixelsPerMeterY));
    write(fd, &file.totalColors, sizeof(file.totalColors));
    write(fd, &file.importantColors, sizeof(file.importantColors));
    y = 0;
    while(y < file.resolution.y)
    {
        x = 0;
        while(x < file.resolution.x)
        {
            write(fd, &image[4 * file.resolution.x * (file.resolution.y - y - 1) + x * 4], 3);
            x++;
        }
        y++;
    }
}
void imageToFile(char *rgbMatrix, char *name, t_resolution resolution)
{
    int fd;
    t_bmp_file image;

    fd = open(name, O_WRONLY|O_CREAT|O_TRUNC,0777);
    if(fd < 0)
        error("NO SE HA PODIDO CREAR LA IMAGEN");
    image = iniBMP(name, 0x18, resolution);
    writeBMP(fd, rgbMatrix, image);
    close(fd);
}