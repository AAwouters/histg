CC = gcc
CFLAGS = -Wall -std=c99 -Iinclude -O3
LIBS = -lm

BIN = ./bin/
SRC = ./src/
INC = ./include/

histg: dir $(BIN)histg.o $(BIN)histg_lib.o $(BIN)spanning_tree.o $(BIN)timer.o $(BIN)kirchhoff.o $(BIN)adjlist.o
	$(CC) $(BIN)histg.o $(BIN)histg_lib.o $(BIN)spanning_tree.o $(BIN)timer.o $(BIN)kirchhoff.o $(BIN)adjlist.o \
	-o $(BIN)histg $(LIBS)

dir: $(BIN)



$(BIN)histg.o: $(SRC)histg.c $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)histg.c -o $@

$(BIN)histg_lib.o: $(SRC)histg_lib.c $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)histg_lib.c -o $@

$(BIN)spanning_tree.o: $(SRC)spanning_tree.c $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)spanning_tree.c -o $@

$(BIN)timer.o: $(SRC)timer.c $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)timer.c -o $@

$(BIN)kirchhoff.o: $(SRC)kirchhoff.c $(INC)kirchhoff.h $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)kirchhoff.c -o $@

$(BIN)adjlist.o: $(SRC)adjlist.c $(INC)adjlist.h $(INC)histg_lib.h
	$(CC) $(CFLAGS) -c $(SRC)adjlist.c -o $@

clean:
	rm -f */*.o *.out

winter: $(BIN)winter.o $(BIN)histg_lib.o $(BIN)spanning_tree.o $(BIN)timer.o $(BIN)adjlist.o
	$(CC) $(CFLAGS) $(BIN)winter.o $(BIN)histg_lib.o $(BIN)spanning_tree.o $(BIN)timer.o $(BIN)adjlist.o \
		-o $(BIN)winter $(LIBS)

$(BIN)winter.o: $(SRC)winter.c
	$(CC) $(CFLAGS) -c $(SRC)winter.c -o $@

$(info $(shell mkdir -p $(BIN)))