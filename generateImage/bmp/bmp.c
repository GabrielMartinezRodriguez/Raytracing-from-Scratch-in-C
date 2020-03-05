/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bmp.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/03 17:45:47 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/04 22:18:23 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bmp.h"

t_bmp_file		inibmp(char *name, unsigned short int bitperpixel, t_resolution resolution)
{
	t_bmp_file image;

	image.file_name = name;
	image.magicfile[0] = 0x42;
	image.magicfile[1] = 0x4d;
	image.reserved[0] = 0x0;
	image.reserved[1] = 0x0;
	image.pixeldata = 0x36;
	image.header_size = 0x28;
	image.resolution.x = resolution.x;
	image.resolution.y = resolution.y;
	image.planescolor = 0x1;
	image.bitsperpixel = bitperpixel;
	image.comprension = 0x0;
	image.pixelspermeterx = 0x0;
	image.pixelspermetery = 0x0;
	image.totalcolors = 0x0;
	image.importantcolors = 0x0;
	image.filesize = image.bitsperpixel * image.resolution.x
	* image.resolution.y + image.pixeldata;
	image.imagesize = image.bitsperpixel * image.resolution.x
	* image.resolution.y;
	return (image);
}

void			writebmp(int fd, char *image, t_bmp_file file)
{
	int x;
	int y;

	write(fd, &file.magicfile, sizeof(file.magicfile));
	write(fd, &file.filesize, sizeof(file.filesize));
	write(fd, &file.reserved, sizeof(file.reserved));
	write(fd, &file.pixeldata, sizeof(file.pixeldata));
	write(fd, &file.header_size, sizeof(file.header_size));
	write(fd, &file.resolution, sizeof(file.resolution));
	write(fd, &file.planescolor, sizeof(file.planescolor));
	write(fd, &file.bitsperpixel, sizeof(file.bitsperpixel));
	write(fd, &file.comprension, sizeof(file.comprension));
	write(fd, &file.imagesize, sizeof(file.imagesize));
	write(fd, &file.pixelspermeterx, sizeof(file.pixelspermeterx));
	write(fd, &file.pixelspermetery, sizeof(file.pixelspermetery));
	write(fd, &file.totalcolors, sizeof(file.totalcolors));
	write(fd, &file.importantcolors, sizeof(file.importantcolors));
	y = 0;
	while (y < file.resolution.y)
	{
		x = 0;
		while (x < file.resolution.x)
		{
			write(fd, &image[4 * file.resolution.x * (file.resolution.y - y - 1) + x * 4], 3);
			x++;
		}
		y++;
	}
}

void			imagetofile(char *rgbmatrix, char *name, t_resolution resolution)
{
	int				fd;
	t_bmp_file		image;

	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
		error("no se ha podido crear la imagen");
	image = inibmp(name, 0x18, resolution);
	writebmp(fd, rgbmatrix, image);
	close(fd);
}
