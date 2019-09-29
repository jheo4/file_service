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

int pid ;
shmQueue_t reqQueue;
message_t reqMessage;

int main(){
  pid = getpid();

  createQueue(4, 512, REQ_QUEUE_INDEX, &reqQueue, sizeof(shmQueue_t));
  initQueue(&reqQueue);
  reqMessage.content = (void*)malloc(reqQueue.sizePerSeg);
  printf("after setting...\n%d %d\n", reqQueue.maxSeg, reqQueue.sizePerSeg);

  while(1){
    dequeue(&reqQueue, &reqMessage);
    printf("%d %s\n%s\n", reqMessage.id, reqMessage.fn,
        (char*)reqMessage.content);
    sleep(1);
  }

  free(reqMessage.content);

  destroyQueue(&reqQueue);
  //destroyQueue(resQueue);

  return 0;
}

