#ifndef HDRI_H
# define HDRI_H

/*
** Cielo HDRI (.hdr Radiance, proyeccion equirectangular) en floats RGB.
** extract_sun busca el sol (la zona mas brillante), devuelve su direccion,
** su tamano angular y la luz que aporta, y lo borra del mapa para que no se
** cuente dos veces.
*/

typedef struct	s_hdri
{
	float		*rgb;
	int			w;
	int			h;
}				t_hdri;

int				load_hdr(const char *path, t_hdri *img);
void			extract_sun(t_hdri *img, float dir[3], float color[3],
					float *cos_radius);

#endif
