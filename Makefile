CC = clang
SRC = Momentum.c
PKG =  pkg-config --cflags --libs x11 xft xinerama xfixes gl glx
DIS = -o MomentumWm
install:
		${CC} -Wall -Wextra ${SRC} -lm `${PKG}` ${DIS}
run:
	bash ./startServer

build:install run
