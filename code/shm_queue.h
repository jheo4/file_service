#include <pthread.h>
#include "config.h"

//typedef struct shmQueue shmQueue_t;
typedef struct shmQueueMeta{
  pthread_mutex_t lock;
  unsigned int readIndex;
  unsigned int writeIndex;
  unsigned int curSeg;
  int empty;
  int full;
}shmQueueMeta_t;


typedef struct shmQueue{
  unsigned int metaID;
  shmQueueMeta_t *meta;
  unsigned int shmIDs[MAX_QUEUE_ELEM];
  unsigned int maxSeg;
  unsigned int sizePerSeg;
  unsigned int sizeOfMsg;
  unsigned int queueIndex;
}shmQueue_t;

typedef struct message{
  int id;
  char fn[20];
  void *content;
}message_t;


void createQueue(unsigned int _maxSeg, unsigned int _sizePerSeg,
    unsigned int _queueIndex, shmQueue_t* newQueue, int size);

void initQueue(shmQueue_t* queue);
void cleanQueue(shmQueue_t* queue);
void destroyQueue(shmQueue_t* queue);

int enqueue(shmQueue_t* queue, message_t* data);
int dequeue(shmQueue_t* queue, message_t* data);

int isFull(shmQueue_t*);
int isEmpty(shmQueue_t*);

