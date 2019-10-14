CC = gcc
CFLAGS = -g -Wall -pthread
LDFLAGS = -Llibcyaml/build/release -Lsnappy-c
LIBS += -lm -lcyaml -lyaml -lsnappyc
INC=-Ilibcyaml/include -Isnappy-c -Icode
SRC = code/ShwRapper.c code/ShmQueue.c code/OutputGenerator.c
OBJ = $(SRC:.c=.o)

OUT = bin/libshm.a

.c.o:
	$(CC) $(CFLGAS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

setup:
	mkdir bin

server:
	$(CC) $(CFLAGS) $(INC) code/Server.c $(OUT) -o bin/server $(LDFLAGS) $(LIBS)

client:
	$(CC) $(CFLAGS) $(INC) code/Client.c $(OUT) -o bin/client $(LDFLAGS) $(LIBS)

clean:
	@rm code/*.o bin/*

