#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include "shwrapper.h"



int getShm(int key, int size){
  int retID = shmget((key_t)key, size, 0666 | IPC_CREAT);
  if(retID == -1){
    perror("shmget failed...\n");
    exit(1);
  }
  return retID;
}


int setSharedLock(pthread_mutex_t *inputLock){
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(inputLock, &attr);
  pthread_mutexattr_destroy(&attr);
}


registryInfo_t* getRegistry(unsigned int *regID, unsigned int keyVal){
  *regID = getShm(keyVal, sizeof(registryInfo_t));
  registryInfo_t* tempReg;
  tempReg = shmat(*regID, (void*)0, 0);
  setSharedLock(&tempReg->lock);
  return tempReg;
}

