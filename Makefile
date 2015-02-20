CC = g++
FLAGS = -g -o

INCLUDE = -I/usr/X11R6/include -I/usr/include/GL -I/usr/include -I.
LIBDIR = -L/usr/X11R6/lib -L/usr/local/lib
SOURCES = main.cpp *.h *.hpp
LIBS = -lGLEW -lGL -lGLU -lglut -lm

EXENAME = texture

all: $(SOURCES)
	$(CC) $(FLAGS) $(EXENAME) $(INCLUDE) $(LIBDIR) $(SOURCES) $(LIBS)

clean:
	rm -f *.o $(EXENAME)

test: clean all
	./texture data/strawberry.bmp data/bunny.obj

.PHONY: all clean
