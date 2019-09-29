CC = gcc
CFLAGS = -g -Wall -pthread
LDFLAGS =
LIBS += -lm
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
	$(CC) $(CFLAGS) code/server.c code/config.h code/shwrapper.h \
		./code/shm_queue.h $(OUT) -o bin/server $(LIBS)

client:
	$(CC) $(CFLAGS) code/client.c code/config.h code/shwrapper.h \
		./code/shm_queue.h $(OUT) -o bin/client $(LIBS)

clean:
	@rm code/*.o bin/*
