#include "TradeProcessing.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>


void write_trade_to_file(Trade *trade,FILE** handlers,
                         pthread_mutex_t *file_mutexes,
                         struct timeval event_time,
                         FILE *delay_file){
  struct timeval current_time;
  double delay_us;
  int i=trade->s_index;
  // Get file access 
  pthread_mutex_lock(&file_mutexes[i]);
  // Write to file
  // Format: timestamp,p,v
  fprintf(handlers[i],"%" PRIu64 ",%f,%f\n",trade->t,trade->p,
          trade->v);
  // Get time delay
  gettimeofday(&current_time,NULL);
  delay_us=(current_time.tv_sec-event_time.tv_sec)*1e6
          +(current_time.tv_usec-event_time.tv_usec);
  // Write to delay log file.
  fprintf(delay_file,"%f\n",delay_us);
  // Give up file acess
  pthread_mutex_unlock(&file_mutexes[i]);
  return;
}

// Calculator methods


// Initializes the calculator buffers
void init_calculator_buffers(CalculatorBuffer *buffers,int symbol_count){
  for(int i=0;i<symbol_count;i++){
    // Init volume buffers to 0
    memset(&buffers[i].avg_info,0,sizeof(CalculatorBuffer));
    buffers[i].candlestick.open=CANDLESTICK_IS_EMPTY;
    buffers[i].candlestick.close=CANDLESTICK_IS_EMPTY;
  }
  return;
}

// Add all trade info to the buffers structure
void add_trade_to_buffers(Trade *trade,
                          CalculatorBuffer *buffers){
  //printf("Adding to %s\n",trade.s);
  int i=trade->s_index;  
  // If candlestick isn't empty 
  if(buffers[i].candlestick.open>CANDLESTICK_IS_EMPTY){
    if(trade->p>buffers[i].candlestick.max)
      buffers[i].candlestick.max=trade->p;
    else if(trade->p<buffers[i].candlestick.min)
      buffers[i].candlestick.min=trade->p;
    // Store last trade.
    buffers[i].candlestick.close=trade->p;
  }
  // Else, candlestick if empty (so first trade)
  else{
    buffers[i].candlestick.open=buffers[i].candlestick.close=trade->p;
    buffers[i].candlestick.max=buffers[i].candlestick.min=trade->p;
  }
  // In any case add the volume to the counter
  buffers[i].candlestick.volume+=trade->v;
  // For the weighted average, handle case of 0 volume for forex trading
  if(trade->v==0){
    buffers[i].candlestick.weighted_price+=1*trade->p;
  }
  else{
    buffers[i].candlestick.weighted_price+=trade->v*trade->p;
  }
  return;
}


void write_and_reset_buffers(uint64_t timestamp_minutes,
                             struct timeval event_time,int symbol_count,
                             FILE **candlestick_files, FILE **avg_files,
                             FILE *delay_file,
                             CalculatorBuffer *buffers){
  // For each symbol 
  Candlestick *candlestick;
  MovingAverageInfo *avg;
  double moving_average;
  double delay_us;
  struct timeval current_time;
  for(int i=0;i<symbol_count;i++){
    // For clarity, assign these pointers.
    candlestick=&buffers[i].candlestick;
    avg=&buffers[i].avg_info;
    // Handle total volume
    avg->total_15min_volume=avg->total_15min_volume
                                      -avg->minute_volumes[avg->oldest_index]
                                      +candlestick->volume;
    avg->minute_volumes[avg->oldest_index]=candlestick->volume;
    // Handle weighted_price
    avg->total_15min_weighted_price=avg->total_15min_weighted_price 
                                      -avg->weighted_prices[avg->oldest_index]
                                      +candlestick->weighted_price;
    avg->weighted_prices[avg->oldest_index]=candlestick->weighted_price;
    // Change index 
    avg->oldest_index=(avg->oldest_index+1)%15;
    // If there was no volume, set the moving average to close price 
    // of candlestick for continuity
    if(avg->total_15min_volume==0){
      moving_average=candlestick->close;
    }
    else{
      moving_average=avg->total_15min_weighted_price/avg->total_15min_volume;
    }
    // Format: timestamp_minutes,moving_average,total_volume,delay_us
    fprintf(avg_files[i],"%" PRIu64 ",%f,%f\n",timestamp_minutes,
            moving_average,
            avg->total_15min_volume);
    // Get delay from event timestamp
    gettimeofday(&current_time,NULL);
    delay_us=(current_time.tv_sec-event_time.tv_sec)*1e6
            +(current_time.tv_usec-event_time.tv_usec);
    fprintf(delay_file,"%f\n",delay_us);
    // Handle candlestick
    // Format of file is timestamp(min),open,high,low,close,volume
    // If candlestick isn't empty 
    if(candlestick->open>CANDLESTICK_IS_EMPTY){
      fprintf(candlestick_files[i],"%" PRIu64 ",%f,%f,%f,%f,%f\n",
              timestamp_minutes,
              candlestick->open,candlestick->max,
              candlestick->min,candlestick->close,candlestick->volume);
    }
    // If candlestick is empty get the close price from last minute, but
    // if close price is also -1 just skip the whole entry 
    // (there wasn't any trades since start)
    else if(candlestick->close>CANDLESTICK_IS_EMPTY){
      fprintf(candlestick_files[i],"%" PRIu64 ",%f,%f,%f,%f,%f\n",
              timestamp_minutes,
              candlestick->close,candlestick->close,
              candlestick->close,candlestick->close,candlestick->volume);
    }
    // Get delay from event timestamp
    gettimeofday(&current_time,NULL);
    delay_us=(current_time.tv_sec-event_time.tv_sec)*1e6
            +(current_time.tv_usec-event_time.tv_usec);
    fprintf(delay_file,"%f\n",delay_us);
    // Reset the candlestick
    candlestick->open=CANDLESTICK_IS_EMPTY;
    candlestick->volume=0;
  }
  return;
}







