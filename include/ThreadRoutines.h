/**
 * Definition of all thread routines for each thread role:
 * - WSS Connector: connects to Finnhub API, parses incoming json stream and
 *   adds trade items to the api_queue.
 * - Writer: Consumes the api_queues item, writes them to a buffer and passes 
 *   the item to the candlestick/average calculators.
 * - Calculator: Receives trades from writers and calculates in real time the
 *   candlesticks of each symbol. At certain trade configurations the 
 *   calculator writes the results to the corresponding files.
*/
#ifndef THREAD_ROUTINES_H
#define THREAD_ROUTINES_H 

#include "PCQueue.h"
#include "TradeProcessing.h"
#include <stdbool.h>

// Symbol list that's defined concretely in main.c
extern const char symbols_list[][SYMBOLS_MAX_LENGTH];


/**
 * @brief Represents all of the WSSClient's arguments.
 */
typedef struct{
  char* api_key; //< API key of user
  bool* connection_closed_flag; //< Flag for connection closed.
} WSSClientArgs;

/**
 * @brief Represents all of the Writer's arguments.
 */
typedef struct{
  PCQueue *api_queue; //< 1st stage pipeline queue.
  PCQueue *calculation_queue; //< 2nd stage pipeline queue.
  FILE **transaction_files; //< File handlers for trade logging.
  FILE *delay_log_file; //< File handler for the delay log.
  pthread_mutex_t *transaction_file_mutexes; //< Mutex array for the files.
  int symbol_count; //< Number of symbols.
} WriterArgs;


/**
 * @brief Represents all of the Calculator's arguments.
 */
typedef struct{
  PCQueue *calculation_queue; //< 2nd stage pipeline queue.
  FILE **candlestick_files; //< File handlers for candlestick logging.
  FILE **avg_files; //< File handlers for moving average logging.
  FILE *delay_log_file; //< File handler for the delay log.
  CalculatorBuffer *calc_buffers; //< Array of buffers for each symbol.
  int symbol_count; //< Number of symbols.
} CalculatorArgs;


/**
 * @brief The routine for the WSS Client.
 *
 * Connects to Finnhub's API using LWS, and adds 
 * all trades received to the 1st pipeline stage queue.
 *
 * @param[in] arg Pointer to the thread's arguments.
 */
void* WSSClient(void* arg);

/**
 * @brief The routine for the Writer role.
 *
 * Consumes the 1st pipeline stage's queue elements,
 * Writes the trades to the log files,
 * Passes the data to the 2nd pipeline stage queue for further processing.
 *
 * @param[in] arg Pointer to the thread's arguments.
 */
void* Writer(void* arg);

/**
 * @brief The routine for the Calculator role.
 *
 * Consumes data from the 2nd pipeline stage queue.
 * Creates candlesticks for each minute.
 * When directed by special consumer items, writes 
 * all necassary data to files and resets.
 *
 * @param[in] arg Pointer to the thread's arguments.
 */
void* Calculator(void* arg);


#endif
