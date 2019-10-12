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

unsigned int maxSeg = 0;
unsigned int sizePerSeg = 0;
unsigned int segID;
segInfo_t *seg;

unsigned int pid;

shmQueue_t queue[MAX_QUEUE];
message_t queueMessage[2];

unsigned int clientRegID;
unsigned int serverRegID;
struct registryInfo *clientReg;
struct registryInfo *serverReg;
unsigned int requestClient;
unsigned int servingClient;

void *bindClient();
void *workComp();


int main(int argc, char *argv[]){
  enum{
    ARG_PROG_NAME,
    ARG_SEG_NUM_INFO,
    ARG_SEG_NUM,
    ARG_SEG_SIZE_INFO,
    ARG_SEG_SIZE,
  };

  pid = SERVER_IDENTIFIER;
  pthread_t bindThread;
  pthread_t compThread;

  maxSeg = atoi(argv[ARG_SEG_NUM]);
  sizePerSeg = atoi(argv[ARG_SEG_SIZE]);
  segID = getShm(SEG_KEY, sizeof(segInfo_t));
  seg = shmat(segID, (void*)0, 0);
  seg->sizePerSeg = sizePerSeg;
  seg->maxSeg = maxSeg;

  clientReg = getRegistry(&clientRegID, CLIENT_REG_KEY);
  serverReg = getRegistry(&serverRegID, SERVER_REG_KEY);
  clientReg->registry = 0;
  serverReg->registry = 0;
  printf("reginfo in main: %d\n", clientReg->registry);
  printf("reginfo in main: %d\n", serverReg->registry);

  pthread_create(&bindThread, NULL, bindClient, NULL);
  pthread_create(&compThread, NULL, workComp, NULL);
  pthread_join(bindThread, NULL);
  pthread_join(compThread, NULL);

  shmdt(&seg);
  shmctl(segID, IPC_RMID, NULL);
  shmdt(clientReg);
  shmdt(serverReg);
  shmctl(clientRegID, IPC_RMID, NULL);
  shmctl(serverRegID, IPC_RMID, NULL);

  /*
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
  */
  return 0;
}


void *bindClient(){
  for(int i = 0; i < 1000000; i++){
    pthread_mutex_lock(&clientReg->lock);
    requestClient = clientReg->registry;
    pthread_mutex_unlock(&clientReg->lock);

    pthread_mutex_lock(&serverReg->lock);
    servingClient = serverReg->registry;
    pthread_mutex_unlock(&serverReg->lock);

    for(; servingClient < requestClient; servingClient++){
      // create queue pair RX/TX
      createQueue(maxSeg, sizePerSeg, servingClient,
          &queue[servingClient], sizeof(shmQueue_t));
      initQueue(&queue[servingClient]);

      printQueueInfo(&queue[servingClient]);
    }
    pthread_mutex_lock(&serverReg->lock);
    serverReg->registry = servingClient;
    pthread_mutex_unlock(&serverReg->lock);
    sleep(5);
  }
}


void *workComp(){

  queueMessage[0].content = (void*)malloc(queue[0].sizePerSeg);
  queueMessage[1].content = (void*)malloc(queue[1].sizePerSeg);
  queueMessage[1].id = pid;

  char buf[100];
  while(1){
    for(int i = 0; i < servingClient; i+=2){
      if(!isEmpty(&queue[i])){
        dequeue(&queue[i], &queueMessage[0]);
        printQueueInfo(&queue[i]);

        strcpy(queueMessage[1].fn, "respond");
        sprintf(buf, "%s compressed", (char*)queueMessage[0].content);
        memcpy(queueMessage[1].content, buf, queue[i+1].sizePerSeg);
        enqueue(&queue[i+1], &queueMessage[1]);
        printQueueInfo(&queue[i+1]);
        sleep(3);
      }
    }
  }
}
