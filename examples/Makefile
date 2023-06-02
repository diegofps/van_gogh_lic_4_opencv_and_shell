CC=g++
CCD=g++
LIBS=`pkg-config opencv4 --libs` `pkg-config --cflags opencv4`
FLAGS=-Wall -std=c++11 -I../lib

all:
	$(CC) main.cpp -o main -O3 $(FLAGS) $(LIBS)

debug:
	$(CCD) main.cpp -o main -g $(FLAGS) $(LIBS)
	gdb main
