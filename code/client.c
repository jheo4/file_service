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

unsigned int maxSeg = 4;
unsigned int sizePerSeg = 512;
unsigned int pid;

shmQueue_t queue[2];
message_t queueMessage[2];

unsigned int clientRegID;
unsigned int serverRegID;
struct registryInfo *clientReg;
struct registryInfo *serverReg;
unsigned int requestClient;
unsigned int servingClient;

typedef void (*callBack)();
void getResponse();
void *asyncRequestServer();
void *syncRequestServer();


int main(){
  int isSync = 0;
  pthread_t compThread;

  clientReg = getRegistry(&clientRegID, CLIENT_REG_KEY);
  serverReg = getRegistry(&serverRegID, SERVER_REG_KEY);
  pthread_mutex_lock(&clientReg->lock);
  pid = clientReg->registry;
  clientReg->registry = pid + 2;
  printf("pid %d, registry %d\n", pid, clientReg->registry);
  pthread_mutex_unlock(&clientReg->lock);

  while(serverReg->registry < pid+2){
    printf("waiting for the server to register...\n");
    sleep(1);
  }
  printf("registered..!\n");
  shmdt(clientReg);
  shmdt(serverReg);

  for(int i = 0; i < 2; i++){
    createQueue(maxSeg, sizePerSeg, pid+i, &queue[i], sizeof(shmQueue_t));
    queueMessage[i].id = pid;
    queueMessage[i].content = (void*)malloc(queue[i].sizePerSeg);
  }


  if(isSync){
    pthread_create(&compThread, NULL, syncRequestServer, NULL);
  }
  else{
    pthread_create(&compThread, NULL, asyncRequestServer, NULL);
  }

  pthread_join(compThread, NULL);


  for(int i = 0; i < 2; i++){
    free(queueMessage[i].content);
    cleanQueue(&queue[i]);
  }
  return 0;
}


void getResponse(){
  dequeue(&queue[1], &queueMessage[1]);
  printf("respond %s\n", (char*)queueMessage[1].content);
}


void *asyncRequestServer(){
  callBack resFunc = getResponse;
  char buf[100] = "message: ";

  while(1){
    strcpy(queueMessage[0].fn, "request");
    sprintf(buf, "YaRae YaRae... %d ", pid);
    memcpy(queueMessage[0].content, buf, queue[0].sizePerSeg);

    if(queue[0].meta->curSeg != maxSeg){
      enqueue(&queue[0], &queueMessage[0]);
    }
    // callback
    if(queue[1].meta->curSeg != 0){
      resFunc();
    }
  }
}

void *syncRequestServer(){
  char buf[100] = "message: ";
  while(1){
    strcpy(queueMessage[0].fn, "request");
    sprintf(buf, "NIKIMI!!! %d ", pid);
    memcpy(queueMessage[0].content, buf, queue[0].sizePerSeg);

    if(queue[0].meta->curSeg != maxSeg){
      enqueue(&queue[0], &queueMessage[0]);
      while(queue[1].meta->curSeg == 0){ }  // blocking
      dequeue(&queue[1], &queueMessage[1]);
      printf("respond %s\n", (char*)queueMessage[1].content);
    }

    printf("I'm synchronous...\n");
  }
}
