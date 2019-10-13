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
#include "shm_queue.h"
#include "config.h"
#include "shwrapper.h"
//#include <cyaml/cyaml.h>
#include "/home/jin/github/file_service/libcyaml/include/cyaml/cyaml.h"
#include "yamlParser.h"

typedef void (*callBack)();
typedef struct printInfo{
  char info[20][30];
  int curInfo;
} printInfo_t;

unsigned int maxSeg = 0;
unsigned int sizePerSeg = 0;
unsigned int segID;
segInfo_t *seg;

unsigned int pid;
int isSync = 0;
callBack resFunc;
requestInfo_t *reqInfo;
printInfo_t pInfo;

shmQueue_t queue[2];
message_t queueMessage[2];

unsigned int clientRegID;
unsigned int serverRegID;
struct registryInfo *clientReg;
struct registryInfo *serverReg;
unsigned int requestClient;
unsigned int servingClient;

void getResponse();
void *requestServer();
void testRoundtrip(FILE *fp, int iteration, char *fn,
    struct timeval *tripTime);
void printTotalInfo(int cur, int tot, int curIter, int totIter);

int main(int argc, char *argv[]){
  enum{
    ARG_PROG_NAME, ARG_CONF, ARG_PATH_IN,
    ARG_STATE, ARG_SYNC,
  };
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

  printf("FileList...\n");
  for(int i = 0; i < reqInfo->requests_count; i++){
    printf("  %d: %s\n", i, reqInfo->requests[i].fileType);
  } printf("\n");


  clientReg = getRegistry(&clientRegID, CLIENT_REG_KEY);
  serverReg = getRegistry(&serverRegID, SERVER_REG_KEY);
  pthread_mutex_lock(&clientReg->lock);
  pid = clientReg->registry;
  clientReg->registry = pid + 2;
  printf("pid %d, registry %d\n", pid, clientReg->registry);
  pthread_mutex_unlock(&clientReg->lock);

  while(serverReg->registry < pid+2){
    printf("waiting for the server to register...\n");
    sleep(2);
  }
  printf("registered..!\n");
  shmdt(clientReg);
  shmdt(serverReg);

  for(int i = 0; i < 2; i++){
    createQueue(maxSeg, sizePerSeg, pid+i, &queue[i], sizeof(shmQueue_t));
    queueMessage[i].id = pid;
    queueMessage[i].content = (void*)malloc(queue[i].sizePerSeg);
  }

  pthread_create(&compThread, NULL, requestServer, NULL);
  pthread_join(compThread, NULL);

  cyaml_free(&config, &requestSchema, reqInfo, 0);
  for(int i = 0; i < 2; i++){
    free(queueMessage[i].content);
    cleanQueue(&queue[i]);
  }
  return 0;
}


void getResponse(){
  dequeue(&queue[1], &queueMessage[1]);
}


void *requestServer(){
  FILE *fp_in;
  struct timeval simple, stress;
  if(!isSync)

  // simple test
  strcpy(pInfo.info[0], "Simple Test");
  for(int i = 0; i < reqInfo->requests_count; i++){
    pInfo.curInfo = i+1;
    strcpy(pInfo.info[i+1], reqInfo->requests[i].fileType);
    testRoundtrip(fp_in, 1, reqInfo->requests[i].fileType, &simple);
  }

  // stress testing
  strcpy(pInfo.info[0], "Stress Test");
  for(int i = 0; i < reqInfo->requests_count; i++){
    pInfo.curInfo = i+1;
    strcpy(pInfo.info[i+1], reqInfo->requests[i].fileType);
    testRoundtrip(fp_in, 100, reqInfo->requests[i].fileType, &stress);
  }

  printf("Simple execution time: %lds %03ld\n", simple.tv_sec, simple.tv_usec);
  printf("Stress execution time: %lds %03ld\n", stress.tv_sec, stress.tv_usec);
}

void testRoundtrip(FILE *fp, int iteration, char *fn,
    struct timeval *tripTime){
  struct timeval totalTime, temp;
  fp = fopen(fn, "r");
  if(fp == NULL){
    printf("file open error...\n");
    return;
  }
  
  fseek(fp, 0, SEEK_END);
  unsigned int totalSize = ftell(fp);
  unsigned int readSize;
  int curProgress, tempInfo = pInfo.curInfo++;

  int packets = (totalSize / queue[0].sizePerSeg) + 1;
  gettimeofday(&totalTime, NULL);
  for(int i = 0; i < iteration; i++){
    rewind(fp);
    readSize = 0;
    curProgress = 0;
    for(int j = 0; j < packets; j++){
      readSize = fread(queueMessage[0].content, 1, queue[0].sizePerSeg, fp);
      queueMessage[0].contentSize = readSize;
      strcpy(queueMessage[0].fn, fn);

      if(isSync){
        if(queue[0].meta->curSeg != maxSeg){
          enqueue(&queue[0], &queueMessage[0]);
          while(queue[1].meta->curSeg == 0){ }  // blocking
          dequeue(&queue[1], &queueMessage[1]);
          curProgress++;
          printTotalInfo(curProgress, packets, i, iteration);
        }
      }
      else{
        if(queue[0].meta->curSeg != maxSeg){
          enqueue(&queue[0], &queueMessage[0]);
        }
        if(queue[1].meta->curSeg != 0){
          resFunc();
          curProgress++;
          printTotalInfo(curProgress, packets, i, iteration);
        }
      }
    }
    while(queue[1].meta->curSeg != 0){
      dequeue(&queue[1], &queueMessage[1]);
      curProgress++;
      printTotalInfo(curProgress, packets, i, iteration);
    }
  }
  gettimeofday(&temp, NULL);
  timersub(&temp, &totalTime, &totalTime);
  timeradd(&totalTime, tripTime, tripTime);
  sprintf(pInfo.info[tempInfo], "%s is done", fn);
  pInfo.curInfo = tempInfo;
  printTotalInfo(-1, -1, -1, -1);

  fclose(fp);
}


void printTotalInfo(int cur, int tot, int curIter, int totIter){
  clear();
  for(int i = 0; i < pInfo.curInfo; i++){
    printf("%s\n", pInfo.info[i]);
  }
  if(cur > 0 && tot > 0){
    printf("(%d/%d) Iteration Progress (%d/%d)\n", curIter, totIter,
        cur, tot);
  }
}
