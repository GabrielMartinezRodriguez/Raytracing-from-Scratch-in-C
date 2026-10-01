NAME = Raytracing

LIBX = libmlx.a

SRCS_LIB = $(wildcard libft/*.c)

SRCS_GNL = $(wildcard get_next_line/*.c)

SRCS_ARG = $(wildcard arguments/*.c)

SRCS_CLR = $(wildcard color/*.c)

SRCS_GNI = $(wildcard generateImage/*.c)

SRCS_BMP = $(wildcard generateImage/bmp/*.c)

SRCS_LD = $(wildcard load/*.c)

SRCS_MATHS = $(wildcard maths/*.c)

SRCS_MATRIX = $(wildcard maths/matrixs/*.c)

SRCS_VECTOR = $(wildcard maths/vectors/*.c)

SRCS_OBJ = $(wildcard objects/*.c)

SRCS_LIBX = $(wildcard libx/*.c)

SRCS_INP = $(wildcard input_loop/*.c)

SRCS =  $(SRCS_RT) $(SRCS_LIB) $(SRCS_GNL) $(SRCS_ARG) $(SRCS_CLR) $(SRCS_GNI) $(SRCS_BMP) \
		$(SRCS_LD) $(SRCS_MATHS) $(SRCS_MATRIX) $(SRCS_VECTOR) $(SRCS_OBJ) $(SRCS_LIBX) $(SRCS_INP) main.c

COMPILER = clang -arch x86_64 -O2

FLAGS = -Wall -Wextra -Werror -Wno-unused-but-set-parameter -Wno-unused-but-set-variable

FLAGS_LIBX = -L. -lmlx -lpthread -framework OpenGL -framework AppKit -lz

$(NAME):
		$(COMPILER) $(FLAGS) $(FLAGS_LIBX) $(SRCS) -o $(NAME)

all: $(NAME)

fclean:
		rm -f $(NAME)

re: fclean all
