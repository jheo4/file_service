#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <cyaml/cyaml.h>
#include "ShmQueue.h"
#include "config.h"
#include "ShwRapper.h"
#include "YamlParser.h"
#include "OutputGenerator.h"

typedef void (*callBack)();
typedef struct printInfo{
  char info[20][30];
  double timeEachFile[10];
  int curInfo;
} printInfo_t;

unsigned int maxSeg = 0;
unsigned int sizePerSeg = 0;
unsigned int segID;
segInfo_t *seg;

unsigned int pid=-1;
int isSync = 0;
callBack resFunc;
requestInfo_t *reqInfo;
printInfo_t pInfo;

shmQueue_t queue[2];
message_t queueMessage[2];
unsigned int queuePoolID;
poolInfo_t *queuePool;

void getResponse(){ dequeue(&queue[1], &queueMessage[1]); }
void *requestServer();
void testRoundtrip(int iteration, char *fn, struct timeval *tripTime);
void printTotalInfo(int cur, int tot, int curIter, int totIter);
void INThandler(int);

int main(int argc, char *argv[]){
  enum{
    ARG_PROG_NAME, ARG_CONF, ARG_PATH_IN,
    ARG_STATE, ARG_SYNC,
  };
  signal(SIGINT, INThandler);

  pInfo.curInfo = 0;
  pthread_t compThread;
  cyaml_err_t err;

  segID = getShm(SEG_KEY, sizeof(segInfo_t));
  seg = shmat(segID, (void*)0, 0);
  sizePerSeg = seg->sizePerSeg;
  maxSeg = seg->maxSeg;
  printf("maxSeg %d, sizePerSeg %d\n", maxSeg, sizePerSeg);

  printf("Sync Mode...\n");
  if(!strcmp(argv[ARG_SYNC], "ASYNC") || !strcmp(argv[ARG_SYNC], "async")){
    printf("  ASYNC\n\n");
    isSync = 0;
    resFunc = getResponse;
  }
  else{
    printf("  SYNC\n\n");
    isSync = 1;
  }

  err = cyaml_load_file(argv[ARG_PATH_IN], &config, &requestSchema,
      (void**) &reqInfo, NULL);
  if (err != CYAML_OK) {
    fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
    return EXIT_FAILURE;
  }
;
  printf("FileList...\n");
  for(int i = 0; i < reqInfo->requests_count; i++){
    printf("  %d: %s\n", i, reqInfo->requests[i].fileType);
  } printf("\n");

  queuePool = getPoolInfo(&queuePoolID);
  while(pid == -1){
    printf("waiting for the server to register...\n");
    pthread_mutex_lock(&queuePool->lock);
    for(int i = 0; i < MAX_CLIENT; i++){
      if(queuePool->pool[i] == AVAILABLE){
        pid = i;
        queuePool->pool[i] = REQUESTED;
        printf("queue %d is requested...\n", pid);
        pthread_mutex_unlock(&queuePool->lock);
        break;
      }
    }
    pthread_mutex_unlock(&queuePool->lock);
    sleep(1);
  }

  while(queuePool->pool[pid] != USED){}
  printf("queue %d registered..!\n", pid);

  printf("queue Create...");
  for(int i = 0; i < 2; i++){
    createQueue(maxSeg, sizePerSeg+10, (pid*2)+i, &queue[i],
        sizeof(shmQueue_t));
    queueMessage[i].id = pid;
    queueMessage[i].content = (void*)malloc(queue[i].sizePerSeg+10);
  }
  printf("    done\n");

  printf("start thread...\n");
  pthread_create(&compThread, NULL, requestServer, NULL);
  pthread_join(compThread, NULL);

  cyaml_free(&config, &requestSchema, reqInfo, 0);
  for(int i = 0; i < 2; i++){
    free(queueMessage[i].content);
    cleanQueue(&queue[i]);
  }
  shmdt(&seg);
  pthread_mutex_lock(&queuePool->lock);
  queuePool->pool[pid] = AVAILABLE;
  pthread_mutex_unlock(&queuePool->lock);
  shmdt(queuePool);

  return 0;
}


void *requestServer(){
  struct timeval simple, stress;

  // simple test
  printf("start simple test...\n");
  strcpy(pInfo.info[0], "Simple Test");
  for(int i = 0; i < reqInfo->requests_count; i++){
    pInfo.curInfo = i+1;
    strcpy(pInfo.info[i+1], reqInfo->requests[i].fileType);
    testRoundtrip(1, reqInfo->requests[i].fileType, &simple);
  }

  // stress testing
  printf("start stress test...\n");
  strcpy(pInfo.info[0], "Stress Test");
  for(int i = 0; i < reqInfo->requests_count; i++){
    pInfo.curInfo = i+1;
    strcpy(pInfo.info[i+1], reqInfo->requests[i].fileType);
    testRoundtrip(100, reqInfo->requests[i].fileType, &stress);
  }

  resultInfo_t res;
  res.pid = pid;
  res.maxSeg = queue[0].maxSeg;
  res.sizePerSeg = queue[0].sizePerSeg-10;
  res.simple = (double)simple.tv_sec + ((double)simple.tv_usec / 1000000);
  res.stress = (double)stress.tv_sec + ((double)stress.tv_usec / 1000000);
  res.isSync = isSync;
  generateOutput(&res);

  printf("Simple execution time: %lds %ldms\n", simple.tv_sec,
      (simple.tv_usec/1000));
  printf("Stress execution time: %lds %ldms\n", stress.tv_sec,
      (stress.tv_usec/1000));
}

void testRoundtrip(int iteration, char *fn,
    struct timeval *tripTime){
  struct timeval totalTime, temp;
  FILE *fp = fopen(fn, "r");
  if(fp == NULL){
    printf("file open error...\n");
    return;
  }

  fseek(fp, 0, SEEK_END);
  unsigned int totalSize = ftell(fp);
  unsigned int readSize;
  int curProgress, tempInfo = pInfo.curInfo++;

  int packets = (totalSize / sizePerSeg) + 1;
  gettimeofday(&totalTime, NULL);
  printf("total packets: %d\n", packets);
  for(int i = 0; i < iteration; i++){
    rewind(fp);
    readSize = 0;
    for(curProgress = 0; curProgress < packets;){
      readSize = fread(queueMessage[0].content, 1, sizePerSeg, fp);
      queueMessage[0].contentSize = readSize;
      strcpy(queueMessage[0].fn, fn);
      if(isSync){
        if(queue[0].meta->curSeg < maxSeg){
          enqueue(&queue[0], &queueMessage[0]);
          while(queue[1].meta->curSeg < 1){ }  // blocking
          dequeue(&queue[1], &queueMessage[1]);
          curProgress++;
          printTotalInfo(curProgress, packets, i, iteration);
        }
      }
      else{
        if(queue[0].meta->curSeg < maxSeg){
          enqueue(&queue[0], &queueMessage[0]);
        }
        if(queue[1].meta->curSeg > 0){
          resFunc();
          curProgress++;
          printTotalInfo(curProgress, packets, i, iteration);
        }
      }
    }
  }
  gettimeofday(&temp, NULL);
  timersub(&temp, &totalTime, &totalTime);
  pInfo.timeEachFile[tempInfo] = (double)totalTime.tv_sec +
    ((double)totalTime.tv_usec/1000000);
  timeradd(&totalTime, tripTime, tripTime);
  sprintf(pInfo.info[tempInfo], "%s is done", fn);
  pInfo.curInfo = tempInfo;
  printTotalInfo(-1, -1, -1, -1);

  fclose(fp);
}


void printTotalInfo(int cur, int tot, int curIter, int totIter){
  clear();
  printf("%s\n", pInfo.info[0]);
  for(int i = 1; i < pInfo.curInfo; i++){
    printf("%s (%lf) \n", pInfo.info[i], pInfo.timeEachFile[i]);
  }
  if(cur > 0 && tot > 0){
    printf("(%d/%d) Iteration Progress (%d/%d)\n", curIter, totIter,
        cur, tot);
  }
}


void INThandler(int sig){
  for(int i = 0; i < 2; i++){
    cleanQueue(&queue[i]);
  }
  shmdt(&seg);
  pthread_mutex_lock(&queuePool->lock);
  queuePool->pool[pid] = AVAILABLE;
  pthread_mutex_unlock(&queuePool->lock);
  shmdt(queuePool);
}
