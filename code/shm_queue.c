#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "shwrapper.h"
#include "shm_queue.h"
#include "config.h"

void createQueue(unsigned int _maxSeg, unsigned int _sizePerSeg,
    unsigned int _queueIndex, shmQueue_t* newQueue, int size){
  newQueue->metaID = getShm(_queueIndex + META_ELEM_SECTION,
      sizeof(shmQueueMeta_t));
  newQueue->maxSeg = _maxSeg;
  newQueue->sizePerSeg = _sizePerSeg;
  newQueue->sizeOfMsg = _sizePerSeg + MSG_HEADER_SIZE;
  newQueue->queueIndex = (_queueIndex * _maxSeg) + QUEUE_ELEM_SECTION;
  int shmSize = sizeof(int)*_maxSeg;
  for(int i=newQueue->queueIndex, j=0; i < newQueue->queueIndex + _maxSeg; i++,
      j++){
    newQueue->shmIDs[j] = getShm(i, newQueue->sizeOfMsg);
  }
  newQueue->meta = shmat(newQueue->metaID, (void*) 0, 0);
  setSharedLock(&newQueue->meta->lock);
}


void initQueue(shmQueue_t* queue){
  queue->meta->readIndex = 0;
  queue->meta->writeIndex = 0;
  queue->meta->empty = 1;
  queue->meta->full = 0;
  queue->meta->curSeg = 0;
}


void cleanQueue(shmQueue_t* queue){
  shmdt(queue->meta);
}


void destroyQueue(shmQueue_t* queue){
  shmdt(queue->meta);
  shmctl(queue->metaID, IPC_RMID, NULL);
  for(int i = 0; i < queue->maxSeg; i++)
    shmctl(queue->shmIDs[i], IPC_RMID, NULL);
}


int enqueue(shmQueue_t* queue, struct message* data){
  void *shm;
  if(queue->meta->curSeg == queue->maxSeg){
    printf("queue is full... %d\n", queue->meta->curSeg);
    return 0;
  }

  pthread_mutex_lock(&queue->meta->lock);

  shm = (void *) shmat(queue->shmIDs[queue->meta->writeIndex], (void*) 0, 0);
  memcpy(shm, data, MSG_HEADER_SIZE);
  memcpy(shm+MSG_HEADER_SIZE, data->content, queue->sizePerSeg);
  shmdt(shm);

  printf("Enqueue idx(%d) cur(%d) RI(%d) WI(%d)\n", queue->queueIndex,
      queue->meta->curSeg, queue->meta->readIndex, queue->meta->writeIndex);
  queue->meta->curSeg+=1;
  queue->meta->writeIndex = (queue->meta->writeIndex + 1) % queue->maxSeg;

  pthread_mutex_unlock(&queue->meta->lock);

  return 1;
}


int dequeue(shmQueue_t* queue, struct message* data){
  void *shm;
  if(queue->meta->curSeg == 0){
    printf("queue is empty... %d\n", queue->meta->curSeg);
    return 0;
  }

  pthread_mutex_lock(&queue->meta->lock);

  printf("dequeue...\n");
  shm = (void *) shmat(queue->shmIDs[queue->meta->readIndex], (void*) 0, 0);
  memcpy(data, shm, MSG_HEADER_SIZE);
  memcpy(data->content, shm+MSG_HEADER_SIZE, queue->sizePerSeg);
  shmdt(shm);

  queue->meta->curSeg-=1;
  queue->meta->readIndex = (queue->meta->readIndex + 1) % queue->maxSeg;

  pthread_mutex_unlock(&queue->meta->lock);

  return 1;
}


int isEmpty(shmQueue_t* queue){
  if(queue->meta->curSeg == 0)
    return 1;
  return 0;
}


int isFull(shmQueue_t* queue){
  if(queue->meta->curSeg == queue->maxSeg)
    return 1;
  return 0;
}

void printQueueInfo(shmQueue_t* queue){
  printf("qIdx(%d), #seg(%d), segSize(%d), ", queue->queueIndex, queue->maxSeg,
      queue->sizePerSeg);
  printf("curSeg(%d), R/W(%d/%d), empty(%d)\n", queue->meta->curSeg,
      queue->meta->readIndex, queue->meta->writeIndex, queue->meta->empty);
}

