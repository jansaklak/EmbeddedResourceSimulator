CC=g++
CFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter
all: main

main: main.o  Graf.o Hardware.o ConfigParser.o  Times.o COM.o  Edge.o Cost_List_refining.o Cost_List_subtaskHandler.o Cost_List_scheduler.o Cost_List_normalizing.o Cost_List_getters.o Cost_List_creating.o  Cost_List_printing.o SubTasks.o Instance.o Cost_List.o
	$(CC) $(CFLAGS) main.o  Graf.o Hardware.o ConfigParser.o  Times.o COM.o  Cost_List_subtaskHandler.o Edge.o Cost_List_refining.o Cost_List_scheduler.o Cost_List_normalizing.o Cost_List_getters.o Cost_List_creating.o  Cost_List_printing.o SubTasks.o Instance.o Cost_List.o -o main

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

Graf.o: Graf.cpp
	$(CC) $(CFLAGS) -c Graf.cpp

ConfigParser.o: ConfigParser.cpp
	$(CC) $(CFLAGS) -c ConfigParser.cpp

SubTasks.o: SubTasks.cpp
	$(CC) $(CFLAGS) -c SubTasks.cpp

Costs.o: Cost_List.cpp
	$(CC) $(CFLAGS) -c Cost_List.cpp

Hardware.o: Hardware.cpp
	$(CC) $(CFLAGS) -c Hardware.cpp

Times.o: Times.cpp
	$(CC) $(CFLAGS) -c Times.cpp

Cost_List_subtaskHandler.o: Cost_List_subtaskHandler.cpp
	$(CC) $(CFLAGS) -c Cost_List_subtaskHandler.cpp

COM.o: COM.cpp
	$(CC) $(CFLAGS) -c COM.cpp

Edge.o: Edge.cpp
	$(CC) $(CFLAGS) -c Edge.cpp

Instance.o: Instance.cpp
	$(CC) $(CFLAGS) -c Instance.cpp

Cost_List_normalizing.o: Cost_List_normalizing.cpp
	$(CC) $(CFLAGS) -c Cost_List_normalizing.cpp

Cost_List_scheduler.o: Cost_List_scheduler.cpp
	$(CC) $(CFLAGS) -c Cost_List_scheduler.cpp

Cost_List_refining.o: Cost_List_refining.cpp
	$(CC) $(CFLAGS) -c Cost_List_refining.cpp


Cost_List_printing.o: Cost_List_printing.cpp
	$(CC) $(CFLAGS) -c Cost_List_printing.cpp

Cost_List_creating.o: Cost_List_creating.cpp
	$(CC) $(CFLAGS) -c Cost_List_creating.cpp

Cost_List_.o: Cost_List_getters.cpp
	$(CC) $(CFLAGS) -c Cost_List_getters.cpp



run: main
	./main

debug: main
	gdb main
	run

clean:
	rm -rf *.o main *.Identifier data/*.temp

with_GDB:
	gdb main
