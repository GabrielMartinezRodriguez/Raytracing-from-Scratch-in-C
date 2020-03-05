/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scene.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmartine <gmartine@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/04 21:58:14 by gmartine          #+#    #+#             */
/*   Updated: 2020/03/05 21:11:18 by gmartine         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SCENE_H
# define SCENE_H

typedef struct			s_arg
{
	char	*file_save;
	char	*file_load;
}						t_arg;

typedef struct			s_libx
{
	void	*ptr;
	void	*win_ptr;
	void	*img_ptr;
	int		*img_addr;
}						t_libx;

typedef struct			s_tupla
{
	int		tamaño;
	double	tupla[3];
}						t_tupla;

typedef struct			s_color
{
	unsigned char	intensity;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
}						t_color;

typedef struct			s_colors_reflected
{
	t_color		color_lights;
	t_color		color_ambient;
}						t_colors_reflected;

typedef struct			s_vect3
{
	double	x;
	double	y;
	double	z;
}						t_vect3;

typedef struct			s_matrix3
{
	t_vect3		vect1;
	t_vect3		vect2;
	t_vect3		vect3;
}						t_matrix3;

typedef struct			s_rayo
{
	t_vect3		punto;
	t_vect3		vector;
}						t_rayo;

typedef struct			s_esfera
{
	t_vect3		punto;
	double		radio;
	t_color		color;
}						t_esfera;

typedef struct			s_plane
{
	t_vect3		normal;
	t_vect3		point;
	t_color		color;
	double		hight;
}						t_plane;

typedef struct			s_circle
{
	t_vect3		center;
	t_vect3		normal;
	t_color		color;
	double		radius;
}						t_circle;

typedef struct			s_cylinder
{
	t_vect3		point;
	t_vect3		normal;
	double		diameter;
	double		height;
	t_color		color;
	t_vect3		vect1;
	t_vect3		vect2;
	t_circle	upper_top;
	t_circle	lower_top;
	t_matrix3	inversebase;
	t_matrix3	base;
}						t_cylinder;

typedef struct			s_square
{
	t_vect3		point;
	t_vect3		normal;
	double		height;
	t_vect3		corner;
	t_vect3		vect1;
	t_vect3		vect2;
	t_color		color;
	void		*funcollider;
}						t_square;

typedef struct			s_triangle
{
	t_vect3		point1;
	t_vect3		point2;
	t_vect3		point3;
	t_color		color;
	t_vect3		vect1;
	t_vect3		vect2;
	t_vect3		normal;
}						t_triangle;

typedef enum			e_object
{
	sphere,
	square,
	plane,
	cylinder,
	triangle,
	circle
}						t_object;

typedef struct			s_list_obj
{
	void				*object;
	void				*functioncoll;
	t_object			type;
	struct s_list_obj	*next;
}						t_list_obj;

typedef struct			s_light_environmental
{
	double			intensity;
	t_color			color;
}						t_light_environmental;

typedef struct			s_light_point
{
	t_vect3		point;
	double		intensity;
	t_color		color;
}						t_light_point;

typedef struct			s_list_light
{
	t_light_point			point;
	struct s_list_light		*next;
}						t_list_light;

typedef struct			s_camera
{
	t_vect3				origin;
	t_vect3				direction;
	t_vect3				vectorx;
	t_vect3				vectory;
	double				depth;
	double				fov_h;
	double				fov_h_rad;
	struct	s_camera	*next;
	struct	s_camera	*back;
}						t_camera;

typedef struct			s_resolution
{
	int		x;
	int		y;
}						t_resolution;

typedef struct			s_scene
{
	t_list_obj				*objets;
	t_list_light			*lights;
	t_light_environmental	env_light;
	t_camera				*camera;
	t_resolution			resolution;
}						t_scene;

typedef struct			s_intersection
{
	double					lambda;
	void					*object;
	t_vect3					normal;
	t_color					color;
}						t_intersection;

typedef struct			s_bmp_file
{
	char					*file_name;
	char					magicfile[2];
	unsigned int			filesize;
	unsigned short int		reserved[2];
	t_resolution			resolution;
	unsigned int			header_size;
	unsigned short int		planescolor;
	unsigned short int		bitsperpixel;
	unsigned int			comprension;
	unsigned int			imagesize;
	unsigned int			pixelspermeterx;
	unsigned int			pixelspermetery;
	unsigned int			totalcolors;
	unsigned int			importantcolors;
	unsigned int			pixeldata;
}						t_bmp_file;
#endif
