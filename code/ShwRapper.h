#include <pthread.h>
#include "config.h"

typedef struct poolInfo{
  pthread_mutex_t lock;
  unsigned int pool[MAX_CLIENT];
}poolInfo_t;


poolInfo_t* getPoolInfo(unsigned int *poolID);
int getShm(int key, int size);
int setSharedLock(pthread_mutex_t *inputLock);
