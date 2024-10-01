/**
 * Definition of the PCQueue (Producer-Consumer Queue) Class. 
 * QUEUE_SIZE is hardcoded and the methods are self explanatory.
*/
#ifndef PCQUEUE_H
#define PCQUEUE_H 

#include <libwebsockets.h>
#include <pthread.h>
#include <sys/time.h>

#include "TradeProcessing.h"

#define QUEUE_SIZE 2000

/**
 * @brief Represents the basic element of the queue.
 */
typedef struct{
  Trade trade; //< The trade that's being processed
  struct timeval event_time; //< The arrival time of the element
} WorkItem;

/**
 * @brief Represents the whole Producer-COnsumer Queue structure.
 */
typedef struct{
  // Standard Prod-Cons Queue Members
  WorkItem buffer[QUEUE_SIZE]; //< Standard buffer
  int head,tail; //< Standard head tail
  int full,empty; //< Bools for empty and full queue states
  pthread_mutex_t *mut; //< For mutual exclusion of queue access
  pthread_cond_t *not_full,*not_empty; //< For waking up producers/consumers
  
  // Additional flags/locks
  int exit_flag; // When asserted, changes return value of queue_remove
  pthread_mutex_t *producer_lock; // For exclusive access to production end
} PCQueue;

/**
 * @brief Initializes a new queue.
 *
 * @return a new PCQueue struct.
 */
PCQueue queue_init();

/**
 * @brief Destroys a queue. 
 *
 * @param[in] queue   Pointer to queue to be destroyed.
 */
void queue_destory(PCQueue *queue);

/**
 * @brief Adds new item to queue.
 *
 * Copys full item by value.
 * Exclusive access to queue is embedded in the function.
 *
 * @param[in] queue  Pointer to queue.
 * @param[in] item   Pointer to item to be added.
 * 
 * @return 0 on success.
 */
int queue_add(PCQueue *queue, WorkItem *item);


/**
 * @brief Removes an item from the queue.
 *
 * Copys full item by value.
 * Exclusive access to queue is embedded in the function.
 *
 * @param[in]  queue Pointer to queue.
 * @param[out] item  Pointer to output item location. 
 *
 * @return 0 on success, -1 when given the order to exit the queue.
 */
int queue_remove(PCQueue *queue, WorkItem *item);


#endif 
