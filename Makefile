CC = clang
SRC = Momentum.c
PKG =  pkg-config --cflags --libs x11 xft xinerama xfixes gl glx xext
DIS = -o MomentumWm-test
EXTERA =-Wextra -Wall
install:
		${CC}  ${SRC} -lm `${PKG}` ${DIS}
run:
	bash ./startServer

build:install run
