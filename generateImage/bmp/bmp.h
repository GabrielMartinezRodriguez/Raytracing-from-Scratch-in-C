/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bmp.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 20:31:34 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 00:08:17 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BMP_H
# define BMP_H
# include "../generateimage.h"

t_bmp_file		inibmp(char *name, unsigned short int bitperpixel, t_resolution resolution);
void			writebmp(int fd, char *image, t_bmp_file file);
void			imagetofile(char *rgbmatrix, char *name, t_resolution resolution);
#endif
