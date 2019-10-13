CC = gcc
CFLAGS = -g -Wall -pthread
LDFLAGS = -Llibcyaml/build/release -Lsnappy-c
LIBS += -lm -lcyaml -lyaml -lsnappyc
INC=-Ilibcyaml/include -Isnappy-c
SRC = code/shwrapper.c code/shm_queue.c
OBJ = $(SRC:.c=.o)

OUT = bin/libshm.a

.c.o:
	$(CC) $(CFLGAS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

setup:
	mkdir bin

server:
	$(CC) $(CFLAGS) $(INC) code/server.c code/config.h code/shwrapper.h \
		./code/shm_queue.h $(OUT) -o bin/server $(LDFLAGS) $(LIBS)

client:
	$(CC) $(CFLAGS) $(INC) code/client.c code/config.h code/shwrapper.h \
		./code/shm_queue.h $(OUT) -o bin/client $(LDFLAGS) $(LIBS)

clean:
	@rm code/*.o bin/*

