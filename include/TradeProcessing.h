/**
 * All structures and methods for processing and gathering
 * trades and their statistics are implemented here.
 */
#ifndef TRADE_PROCESSING_HPP
#define TRADE_PROCESSING_HPP 

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#define SYMBOLS_MAX_LENGTH 20
#define CANDLESTICK_IS_EMPTY -1
#define DIRECTIVE_CALCULATE_MINUTE -1 // Is assigned to v member of trade 

// List of symbols defined concretely in main.c
extern const char symbols_list[][SYMBOLS_MAX_LENGTH];


/**
 * @brief Represents all info regarding 1 trade.
 */
typedef struct{
  double p; //< Last price of trade.
  char s_index; //< Pointer to the symbol of the traded stock (on symbols_list)
  uint64_t t; //< Timestamp of trade (ms since Epoch)
  double v; //< Volume traded.
} Trade;


/**
 * @brief Represents all data for a single candlestick.
 *
 * Each candlestick is created for a time inverval of 1min.
 */
typedef struct{
  double max; //< Max price of interval.
  double min; //< Min price of interval.
  double open; //< Opening price of interval.
  double close; //< Closing price of interval.
  double volume; //< Total volume traded on interval.
  double weighted_price; //< Sum of (p[i]*v[i]) of all trades in interval.
} Candlestick;


/**
 * @brief Represents all info regarding the 15minute moving average.
 *
 * It's a queue like structure with only a head (oldest_index),
 * and the head just overrides the oldest entry @ each minute. 
 */
typedef struct{
  // For total volume
  double total_15min_volume; //< Volume of the past 15 minutes
  double minute_volumes[15]; //< Each minute volume separate
  // For weighted price
  double total_15min_weighted_price; //< Weighted price of the past 15 minutes
  double weighted_prices[15]; //< Each minute weighted price separate
  // Queue head 
  int oldest_index; //< Pointer to the oldest element of the 15 
} MovingAverageInfo;

/**
 * @brief Represents all data needing to be calculated for a single symbol. 
 */
typedef struct{
  Candlestick candlestick; //< The latest minute's candlestick.
  MovingAverageInfo avg_info; //< The moving avg info of the past 15 minutes. 
} CalculatorBuffer;


/**
 * @brief Writes a given trade to it's corresponding file. 
 *
 * Mutual file exclusion is embedded in the function.
 * Each trade is recorded in the form: t,p,v,delay_us
 * where delay (us) counts from the arrival of the json object.
 *
 * @param[in] trade Pointer to trade structure to be logged.
 * @param[in] handlers Array of csv file handlers (one per symbol).
 * @param[in] file_mutexes Array of mutex vars (one per file).
 * @param[in] event_time Time of json objet's arrival.
 * @param[in] delay_file File handler for delay log of writer.
 */
void write_trade_to_file(Trade *trade,FILE** handlers,
                         pthread_mutex_t *file_mutexes,
                         struct timeval event_time,
                         FILE* delay_file);


/**
 * @brief Initializes a CalculatorBuffer for each symbol. 
 *
 * Each field is 0 and each candlestick's opening price is 
 * CANDLESTICK_IS_EMPTY, to indicate that the structure needs to be treated as 
 * empty.
 *
 * @param[in] buffers The array of buffers 
 * @param[in] symbol_count The number of symbols
 */
void init_calculator_buffers(CalculatorBuffer *buffers,int symbol_count);


/**
 * @brief Adds all data of the new trade to the current minute info.
 *
 * @param[in]   trade Pointer to the trade that is added.
 * @param[out]  buffers The array of CalculatorBuffers that is modified.
 */
void add_trade_to_buffers(Trade *trade,
                          CalculatorBuffer *buffers);


/**
 * @brief Stores the CalculatorBuffer data and resets for new minute.
 *
 * At each file also writes delay (us) at the end of each entry.
 *
 * @param[in] timestamp_minutes Minutes since Epoch of the minute to be stored.
 * @param[in] event_time Timestamp of minute event arrival.
 * @param[in] symbol_count Number of symbols.
 * @param[in] candlestick_files The array of file handlers for the candlestick 
 * entries.
 * @param[in] avg_files The array of file handlers for the movign avg entries.
 * @param[in] delay_file File handler for the delay log of calculator.
 * @param[in/out] buffers The array of CalculatorBuffers that are stored and 
 * reset.
 */
void write_and_reset_buffers(uint64_t timestamp_minutes,
                             struct timeval event_time,int symbol_count,
                             FILE **candlestick_files, FILE **avg_files,
                             FILE *delay_file,
                             CalculatorBuffer *buffers);

#endif 
