#include "ThreadRoutines.h"
#include "PCQueue.h"
#include "TradeProcessing.h"
#include "WSSHandling.h"
#include <string.h>

void* WSSClient(void* arg){
  // Decode args.
  WSSClientArgs *args=(WSSClientArgs*)arg;
  bool* connection_closed_flag=args->connection_closed_flag;
  char* api_key=args->api_key;
  // Object for connection to API
  WSS_Objects wss;
  
  // Exit only of the exit flag is asserted manually
  while(api_queue.exit_flag==0){
    // Try to setup the connection
    printf("Attempting to connect...\n");
    wss=finnhub_connection_setup(api_key);
    // If the initialization failed, try again after a second
    if(wss.ctx==NULL || wss.wsi==NULL){
      sleep(1);
      continue;
    }
    // Start connection
    *connection_closed_flag=false;
    while(*connection_closed_flag==false){
      // Service normally
      if(lws_service(wss.ctx,0)<0){
        printf("Error in lws_service\n");
      }
      // Break if exiting gracefully
      if(api_queue.exit_flag==1){
        break;  
      }
    }
    // Cleanup
    lws_context_destroy(wss.ctx);
  }
  // If the queue is empty, signal consumers.
  pthread_mutex_lock(api_queue.mut);
  if(api_queue.empty){
    pthread_cond_signal(api_queue.not_empty);
  }
  pthread_mutex_unlock(api_queue.mut);

  printf("WSSClient returning..\n");
  return NULL;
}

void* Writer(void* arg){
  // Decode args
  WriterArgs *args=(WriterArgs*)arg;
  PCQueue *api_queue=args->api_queue;
  PCQueue *calculation_queue=args->calculation_queue;
  FILE **transaction_files=args->transaction_files;
  FILE *delay_log_file=args->delay_log_file;
  pthread_mutex_t *file_mutexes=args->transaction_file_mutexes;
  int symbol_count=args->symbol_count;

  
  WorkItem current_work_item;
  while(true){
    // Get trade
    if(queue_remove(api_queue,&current_work_item)==-1){
      // If you were instructed to exit the 1st queue, pass the signal to next stage
      calculation_queue->exit_flag=1;
      pthread_cond_signal(calculation_queue->not_empty);
      break; 
    } 
    // If it's an actual trade and not a directive
    if(current_work_item.trade.v>DIRECTIVE_CALCULATE_MINUTE){
      // Write the trade to the file
      write_trade_to_file(&current_work_item.trade,transaction_files,
                          file_mutexes,current_work_item.event_time,
                          delay_log_file);
    }
    // Add work item to calculation queue 
    queue_add(calculation_queue,&current_work_item);
  }


  printf("Writer returning..\n");
  return NULL;
}

void* Calculator(void* arg){
  // Decode arguments
  CalculatorArgs *args=(CalculatorArgs*)arg;
  PCQueue *calculation_queue=args->calculation_queue;
  FILE **candlestick_files=args->candlestick_files;
  FILE **avg_files=args->avg_files;
  FILE *delay_log_file=args->delay_log_file;
  CalculatorBuffer *buffers=args->calc_buffers;
  int symbol_count=args->symbol_count;
  // Initialize calculation buffers 
  init_calculator_buffers(buffers,symbol_count);
  
  WorkItem current_work_item;
  while(true){
    // Get item (or exit if flag is set)
    if(queue_remove(calculation_queue,&current_work_item)==-1){
      // If returned -1 queue is empty so break
      break;
    }
    // Check if item is actual trade of a directive 
    // Actual trade 
    if(current_work_item.trade.v>DIRECTIVE_CALCULATE_MINUTE){
      add_trade_to_buffers(&current_work_item.trade,buffers);
    }
    // Else calculate minute
    else{
      write_and_reset_buffers(current_work_item.trade.t,
                              current_work_item.event_time,symbol_count,
                              candlestick_files,avg_files,
                              delay_log_file,buffers);
    }
  }
  printf("Calculator returning..\n");
  return NULL;
}
