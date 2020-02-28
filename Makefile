NAME = Raytracing

LIBX = libmlx.a

SRCS_RT = $(wildcard *.c)

SRCS_LIB = $(wildcard libft/*.c)

SRCS_GNL = $(wildcard get_next_line/*.c)

OBJ = *.o

COMPILER = gcc

FLAGS = -Wall -Wextra -Werror

FLAGS_LIBX = -lmlx -framework OpenGL -framework AppKit -lz

$(NAME): header.h scene.h
		$(COMPILER) $(FLAGS) $(FLAGS_LIBX) $(SRCS_RT) $(SRCS_GNL) $(SRCS_LIB) $(LIBX) -o $(NAME)

all: $(NAME)

clean:
		rm -f $(OBJ)

fclean: clean
		rm -f $(NAME)

re: fclean all
