#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int getShm(int, int);

struct regInfo{
  int curIndex;
  int assignedId;
};

int getShm(int key, int size){
  int retID = shmget((key_t)key, size, 0666 | IPC_CREAT);
  if(retID == -1){
    perror("shmget failed...\n");
    exit(1);
  }
  return retID;
}

