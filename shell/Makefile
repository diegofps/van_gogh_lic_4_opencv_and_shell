CC=g++
CCD=g++
LIBS=`pkg-config opencv4 --libs` `pkg-config --cflags opencv4`
FLAGS=-Wall -std=c++11 -I../lib

all:
	$(CC) main.cpp -o vglic -O3 $(FLAGS) $(LIBS)

debug:
	$(CCD) main.cpp -o vglic -g $(FLAGS) $(LIBS)
	gdb vglic
