all: build run

build:
	gcc -framework OpenGL -lpng -lglut glexp.c -o glexp

run:
	./glexp