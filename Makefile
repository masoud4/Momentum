CC = clang
SRC = Momentum.c
PKG =  pkg-config --cflags --libs x11 xft xinerama xfixes gl glx xext
DIS = -o MomentumWm
install:
		${CC} -Wall-g -Wextra ${SRC} -lm `${PKG}` ${DIS}
	##	${CC} -Wall -g -Wextra element.c -lm `${PKG}` -o menu
run:
	bash ./startServer

build:install run
