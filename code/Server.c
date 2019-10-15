#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <snappy.h>
#include "ShmQueue.h"
#include "config.h"
#include "ShwRapper.h"

unsigned int maxSeg = 0;
unsigned int sizePerSeg = 0;
unsigned int segID;
segInfo_t *seg;

unsigned int pid;
int isSync;
int synchedClient;
typedef void (*callBack)();
callBack reqFunc;

shmQueue_t queue[MAX_QUEUE];
message_t queueMessage[2];

unsigned int queuePoolID;
poolInfo_t *queuePool;
unsigned int requestClient;
unsigned int servingClient;

void *bindClient();
void *workComp();
void INThandler(int);
void getRequest(int idx){ dequeue(&queue[idx], &queueMessage[0]); }

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
  signal(SIGINT, INThandler);

  pid = SERVER_IDENTIFIER;
  pthread_t bindThread;
  pthread_t compThread;

  maxSeg = atoi(argv[ARG_SEG_NUM]);
  sizePerSeg = atoi(argv[ARG_SEG_SIZE]);

  reqFunc = getRequest;
  if(!strcmp(argv[ARG_SYNC], "ASYNC") || !strcmp(argv[ARG_SYNC], "async")){
    printf("  ASYNC\n\n");
    isSync = 0;
  }
  else{
    printf("  SYNC\n\n");
    isSync = 1;
    synchedClient = -1;
  }

  segID = getShm(SEG_KEY, sizeof(segInfo_t));
  seg = shmat(segID, (void*)0, 0);
  seg->sizePerSeg = sizePerSeg;
  seg->maxSeg = maxSeg;

  printf("set pool...");
  queuePool = getPoolInfo(&queuePoolID);
  for(int i = 0; i < MAX_CLIENT; i++)
    queuePool->pool[i] = AVAILABLE;
  printf("    done\n");

  printf("create queue...");
  for(int i = 0; i < MAX_QUEUE; i++)
    createQueue(maxSeg, sizePerSeg+10, i, &queue[i], sizeof(shmQueue_t));
  printf("    done\n");


  printf("start threads...\n");
  pthread_create(&bindThread, NULL, bindClient, NULL);
  pthread_create(&compThread, NULL, workComp, NULL);
  pthread_join(bindThread, NULL);
  pthread_join(compThread, NULL);

  free(queueMessage[0].content);
  free(queueMessage[1].content);
  for(int i = 0; i < MAX_QUEUE; i++)
    destroyQueue(&queue[i]);

  shmdt(&seg);
  shmctl(segID, IPC_RMID, NULL);
  shmdt(queuePool);
  shmctl(queuePoolID, IPC_RMID, NULL);
  return 0;
}


void *bindClient(){
  int curQueue;
  int clearFlag;
  for(int i = 0; i < 1000000; i++){
    printf("Queue Usage...\n");
    pthread_mutex_lock(&queuePool->lock);

    clearFlag = true;
    for(int i = 0; i < MAX_CLIENT; i++)
      if(queuePool->pool[i] == USED)
        clearFlag = false;

    if(clearFlag == true) synchedClient = -1;

    for(int i = 0; i < MAX_CLIENT; i++){
      if(queuePool->pool[i] == REQUESTED){
        if(isSync){
          if(synchedClient == -1){
            queuePool->pool[i] = USED;
            synchedClient = i;
            curQueue = i * 2;
            initQueue(&queue[curQueue]);
            initQueue(&queue[curQueue+1]);
          }
          else if(queuePool->pool[synchedClient] == AVAILABLE){
            synchedClient = -1;
          }
        }
        else{
          queuePool->pool[i] = USED;
          curQueue = i * 2;
          initQueue(&queue[curQueue]);
          initQueue(&queue[curQueue+1]);
        }
      }
      printf("%d ", queuePool->pool[i]);
    }
    printf("\n");
    if(isSync) printf("synched client %d\n", synchedClient);
    pthread_mutex_unlock(&queuePool->lock);
    usleep(500000);
  }
}


void *workComp(){
  struct snappy_env env;
  int snappyRes, curQueue;
  size_t compressed_size;
  snappy_init_env(&env);

  queueMessage[0].content = (void*)malloc(queue[0].sizePerSeg+10);
  queueMessage[1].content = (void*)malloc(queue[1].sizePerSeg+10);

  printf("Compressor works...   ");
  if(isSync) printf("in SYNC mode\n");
  else printf("in ASYNC mode\n");

  while(1){
    for(int i=0; i < MAX_CLIENT; i++){
      curQueue = -1;
      if(isSync){
        if(synchedClient != -1){
          pthread_mutex_lock(&queuePool->lock);
          if(queuePool->pool[i] == USED) curQueue = (i*2);
          pthread_mutex_unlock(&queuePool->lock);
        }
      }
      else{
        pthread_mutex_lock(&queuePool->lock);
        if(queuePool->pool[i] == USED) curQueue = (i*2);
        pthread_mutex_unlock(&queuePool->lock);
      }

      if(curQueue > -1){
        if(queue[curQueue].meta->curSeg > 0 &&
            queue[curQueue+1].meta->curSeg < queue[curQueue+1].maxSeg){
          dequeue(&queue[curQueue], &queueMessage[0]);
          snappyRes = snappy_compress(&env,
              (const char *)queueMessage[0].content,
              queueMessage[0].contentSize, queueMessage[1].content,
              &queueMessage[1].contentSize);
          queueMessage[1].id = pid;
          enqueue(&queue[curQueue+1], &queueMessage[1]);
        }
      }
    }
  }
}


void INThandler(int sig){
  for(int i = 0; i < MAX_QUEUE; i++)
    destroyQueue(&queue[i]);
  shmdt(&seg);
  shmctl(segID, IPC_RMID, NULL);
  shmdt(queuePool);
  shmctl(queuePoolID, IPC_RMID, NULL);
}
