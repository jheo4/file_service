#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "/home/jin/github/file_service/snappy-c/snappy.h"
//#include <snappy.h>
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
    ARG_STATE,
    ARG_SYNC,
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

  pthread_create(&bindThread, NULL, bindClient, NULL);
  pthread_create(&compThread, NULL, workComp, NULL);
  pthread_join(bindThread, NULL);
  pthread_join(compThread, NULL);

  free(queueMessage[0].content);
  free(queueMessage[1].content);
  for(int i = 0; i < serverReg->registry; i++)
    destroyQueue(&queue[i]);

  shmdt(&seg);
  shmctl(segID, IPC_RMID, NULL);
  shmdt(clientReg);
  shmdt(serverReg);
  shmctl(clientRegID, IPC_RMID, NULL);
  shmctl(serverRegID, IPC_RMID, NULL);
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
    }
    pthread_mutex_lock(&serverReg->lock);
    if(serverReg->registry != servingClient){
      serverReg->registry = servingClient;
      printf("Client is registered...\n");
    }
    pthread_mutex_unlock(&serverReg->lock);
    sleep(1);
  }
}


void *workComp(){
  struct snappy_env env;
  size_t compressed_size;
  snappy_init_env(&env);

  queueMessage[0].content = (void*)malloc(queue[0].sizePerSeg);
  queueMessage[1].content = (void*)malloc(queue[1].sizePerSeg);
  queueMessage[1].id = pid;

  printf("Compressor works...\n");
  while(1){
    for(int i = 0; i < serverReg->registry; i+=2){
      if(queue[i].meta->curSeg != 0 &&
          queue[i+1].meta->curSeg < queue[i+1].maxSeg){
        dequeue(&queue[i], &queueMessage[0]);
        snappy_compress(&env, queueMessage[0].content,
            queueMessage[0].contentSize, queueMessage[1].content,
            &queueMessage[1].contentSize);
        strcpy(queueMessage[1].fn, queueMessage[0].fn);

        enqueue(&queue[i+1], &queueMessage[1]);
      }
    }
  }
}
