CC = g++
CFLAGS = -std=c++17 -g -pedantic -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -I. -Isrc/models -Isrc/simulator

MODEL_SRCS = $(wildcard src/models/*.cpp)
MODEL_OBJS = $(MODEL_SRCS:.cpp=.o)

SIMULATOR_SRCS = $(wildcard src/simulator/*.cpp)
SIMULATOR_OBJS = $(SIMULATOR_SRCS:.cpp=.o)

OBJS = main.o $(MODEL_OBJS) $(SIMULATOR_OBJS)

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o main

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

run: main
	./main

gui: main
	./run_gui.sh

debug: main
	gdb main

clean:
	rm -rf main main.o *.o *.Identifier data/*.temp src/models/*.o src/simulator/*.o

.PHONY: all run gui debug clean
