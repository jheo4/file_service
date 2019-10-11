#include <pthread.h>

typedef struct registryInfo{
  pthread_mutex_t lock;
  unsigned int registry;
}registryInfo_t;

registryInfo_t* getRegistry(unsigned int *regID, unsigned int keyVal);
int getShm(int key, int size);
int setSharedLock(pthread_mutex_t *inputLock);
