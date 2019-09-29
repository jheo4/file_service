#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "shm_queue.h"
#include "config.h"
#include "shwrapper.h"

int pid;
shmQueue_t reqQueue;
message_t reqMessage;

int main(){
  pid = getpid();
  createQueue(4, 512, REQ_QUEUE_INDEX, &reqQueue, sizeof(shmQueue_t));

  reqMessage.content = (void*)malloc(reqQueue.sizePerSeg);

  printf("nimi...");
  reqMessage.id = pid;
  strcpy(reqMessage.fn, "kimchiman");
  memcpy(reqMessage.content, "nikimi... ZOTO", reqQueue.sizePerSeg);

  while(1){
    enqueue(&reqQueue, &reqMessage);
    usleep(500000);
  }

  free(reqMessage.content);

  cleanQueue(&reqQueue);
  return 0;
}
