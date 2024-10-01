#include "PCQueue.h"
#include <pthread.h>

PCQueue queue_init(){
  PCQueue queue;
  queue.head=queue.tail=0;
  queue.empty=1;
  queue.full=0;
  queue.mut=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queue.mut,NULL);
  queue.not_full=(pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(queue.not_full,NULL);
  queue.not_empty=(pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(queue.not_full,NULL);
  queue.producer_lock=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queue.producer_lock,NULL);
  queue.exit_flag=0;

  return queue;
}

void queue_destory(PCQueue *queue){
  pthread_mutex_destroy(queue->mut);
  free(queue->mut);
  pthread_cond_destroy(queue->not_full);
  free(queue->not_full);
  pthread_cond_destroy(queue->not_empty);
  free(queue->not_empty);
  pthread_mutex_destroy(queue->producer_lock);
  free(queue->producer_lock);
  return;
}

int queue_add(PCQueue *queue, WorkItem *item){
  // Get queue access
  pthread_mutex_lock(queue->mut);
  // If exit order was given skip all inserts.
  if(queue->exit_flag==1){
    pthread_mutex_unlock(queue->mut);
    return -1;
  }
  // Wait until there is availability
  while(queue->full){
    pthread_cond_wait(queue->not_full,queue->mut);
  }
  // Add item
  queue->buffer[queue->tail]=*item;
  queue->tail=(queue->tail+1)%QUEUE_SIZE;
  // Assert full signal if needed
  if(queue->tail==queue->head)
    queue->full=1;
  queue->empty=0;
  // Give queue access back 
  pthread_mutex_unlock(queue->mut);
  pthread_cond_signal(queue->not_empty);
  return 0;
}

int queue_remove(PCQueue *queue, WorkItem *item){
  // Get queue access 
  pthread_mutex_lock(queue->mut);
  // Wait until there is availability
  while(queue->empty){
    // If given order to exit, consume nothing
    if(queue->exit_flag==1){
      // Found empty queue and exit flag indicator so give queue access back
      pthread_mutex_unlock(queue->mut);
      // Let other consumers know that it's time to exit
      pthread_cond_signal(queue->not_empty);
      // Return exit value
      return -1;
    }
    pthread_cond_wait(queue->not_empty,queue->mut);
  }
  // Remove item
  *item=queue->buffer[queue->head];
  queue->head=(queue->head+1)%QUEUE_SIZE;
  // Assert empty signal if needed
  if(queue->tail==queue->head){
    queue->empty=1;
  }
  queue->full=0;
  // Give queue access back 
  pthread_mutex_unlock(queue->mut);
  pthread_cond_signal(queue->not_full);
  return 0;
}
