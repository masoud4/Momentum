CC = clang
SRC = Momentum.c
UTL = util.c
PKG =  pkg-config --cflags --libs x11 xft xinerama xfixes gl glx xext xrandr
MDK_FLAGS = -I ../MDK/include  ../MDK/build/mdk.o  
DIS = -o MomentumWm
install:
		${CC} -Wall -g -Wextra ${UTL} -lm `${PKG}` -c 
		# ${CC} -Wall -g -Wextra ${SRC} util.o ${MDK_FLAGS} -lm `${PKG}` ${DIS}
		${CC} -Wall -g -Wextra ${SRC} util.o  -lm `${PKG}` ${DIS}
	##	${CC} -Wall -g -Wextra element.c -lm `${PKG}` -o menu
run:
	bash ./startServer

build:install run

