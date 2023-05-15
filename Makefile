CC = clang
SRC = Momentum.c
PKG =  pkg-config --cflags --libs x11 xft xinerama
DIS = -o MomentumWm
install:
		${CC} ${SRC} -lm `${PKG}` ${DIS}
run:
	bash ./startServer

build:install run
