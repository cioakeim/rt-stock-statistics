/**
 * This main function connnects all the submodules and contains all the
 * configuration parameters for establishing a connection with finnhub's
 * WSS endpoint, and subscribing to a list of stock symbols.
 * 
 * Configuration:
 * Hardcoded configuration parameters include the API Key of the user 
 * and the list of symbols that are tracked by the estimator.
*/
#include <bits/types/struct_rusage.h>
#include <openssl/evp.h>
#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <libwebsockets.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

#include "PCQueue.h"
#include "ThreadRoutines.h"
#include "TradeProcessing.h"
#include "WSSHandling.h"
#include "SystemHandling.h"


// CONFIGURATION HARDCODED PARAMETERS

#define WRITERS_COUNT 2
#define SYMBOL_COUNT LWS_ARRAY_SIZE(symbols_list)-1
#define API_KEY "XXXXXXXX"

// Array of symbols for subscription (null terminated)
const char symbols_list[][SYMBOLS_MAX_LENGTH]={
  "AAPL",
  "NIO",
  "INTC",
  "AMZN",
  "NVDA",
  "TSLA",
  "GOOGL",
  "AMD",
  "MSFT",
  "META",
  "BRK.B",
  "BINANCE:BTCUSDT",
  "BINANCE:ETHUSDT",
  "BINANCE:BNBUSDT",
  "BINANCE:SOLUSDT",
  "BINANCE:DOGEUSDT",
  "OANDA:EUR_USD",
  "OANDA:USD_JPY",
  "OANDA:GBP_USD",
  "OANDA:AUD_USD",
  "OANDA:USD_CAD",
  ""
};
// The API key for Finnhub
char api_key[60];


// Flag used for exiting gracefully from the WSS connection
bool exit_wss_connection=false;
bool connection_closed=false;


// The api queue
PCQueue api_queue;
// The exit flag 
int exit_flag;

int main(int argc, char** argv){
  struct timeval program_start,program_end;
  double program_elapsed_time;
  gettimeofday(&program_start,NULL);

  // Handle api key configuration
  if(argc==2){
    strcpy(api_key,argv[1]);
  }
  else{
    strcpy(api_key,API_KEY);
  }
  printf("Api_key: %s\n",api_key);

  // Init queues
  api_queue=queue_init();
  PCQueue calculation_queue=queue_init();

  // Prepare WSS Client
  pthread_t wss_client;
  WSSClientArgs wss_connector_args;
  wss_connector_args.api_key=api_key;
  wss_connector_args.connection_closed_flag=&connection_closed;


  // Prepare delay log files
  FILE *delay_writer_logs[WRITERS_COUNT];
  FILE *delay_calculator_logs;
  if(open_delay_files("./delays",WRITERS_COUNT,delay_writer_logs,
                      &delay_calculator_logs)!=0){
    printf("Error in delay file creation.\n");
    exit(-1);
  }


  // Prepare Writers
  // Create file systems
  FILE *transaction_files[SYMBOL_COUNT];
  if(open_csv_batch("./trade_logs",SYMBOL_COUNT,transaction_files)!=0){
    printf("Error in opening csv batch\n");
    exit(-1);
  }
  // Create file mutexes for correct file access
  pthread_mutex_t writing_mutexes[SYMBOL_COUNT];
  for(int i=0;i<SYMBOL_COUNT;i++){
    pthread_mutex_init(&writing_mutexes[i],NULL);
  }
  pthread_t writer[WRITERS_COUNT];
  WriterArgs writer_args[WRITERS_COUNT];
  for(int i=0;i<WRITERS_COUNT;i++){
    writer_args[i].api_queue=&api_queue;
    writer_args[i].transaction_files=transaction_files;
    writer_args[i].symbol_count=SYMBOL_COUNT;
    writer_args[i].transaction_file_mutexes=writing_mutexes;
    writer_args[i].calculation_queue=&calculation_queue;
    writer_args[i].delay_log_file=delay_writer_logs[i];
  }

  // Initialize Calculator 
  // Create file systems
  FILE *candlestick_files[SYMBOL_COUNT];
  if(open_csv_batch("./candlesticks",SYMBOL_COUNT,candlestick_files)!=0){
    printf("Error in opening csv batch\n");
    exit(-1);
  }
  FILE *avg_files[SYMBOL_COUNT];
  if(open_csv_batch("./moving_avg",SYMBOL_COUNT,avg_files)!=0){
    printf("Error in opening csv batch\n");
    exit(-1);
  }
  pthread_t calculator;
  CalculatorArgs calculator_args;
  CalculatorBuffer calculator_buffers[SYMBOL_COUNT];
  calculator_args.calculation_queue=&calculation_queue;
  calculator_args.symbol_count=SYMBOL_COUNT;
  calculator_args.candlestick_files=candlestick_files;
  calculator_args.avg_files=avg_files;
  calculator_args.calc_buffers=calculator_buffers;
  calculator_args.delay_log_file=delay_calculator_logs;

  // Start threads
  pthread_create(&wss_client, NULL, WSSClient, (void*)&wss_connector_args);
  for(int i=0;i<WRITERS_COUNT;i++)
    pthread_create(&writer[i], NULL, Writer, (void*)&writer_args[i]);
  pthread_create(&calculator,NULL,Calculator,(void*)&calculator_args);
  setup_timer();

  pthread_join(wss_client, NULL);
  for(int i=0;i<WRITERS_COUNT;i++)
    pthread_join(writer[i],NULL);
  pthread_join(calculator,NULL);
  printf("Threads complete\n");


  // Cleanup

  // Close files
  close_csv_batch(transaction_files,SYMBOL_COUNT);
  close_csv_batch(candlestick_files,SYMBOL_COUNT);
  close_csv_batch(avg_files,SYMBOL_COUNT);
  close_delay_files(delay_writer_logs,&delay_calculator_logs,WRITERS_COUNT);

  // Destroy mutexes
  for(int i=0;i<SYMBOL_COUNT;i++){
    pthread_mutex_destroy(&writing_mutexes[i]);
  }

  // Get final time
  gettimeofday(&program_end,NULL);
  program_elapsed_time=(program_end.tv_sec-program_start.tv_sec)
                      +(program_end.tv_usec-program_start.tv_usec)/1e6;
  // Get CPU% usage
  struct rusage usage;
  double total_cpu_time,user_cpu_time,system_cpu_time;
  double cpu_percentage;
  getrusage(RUSAGE_SELF,&usage);
  user_cpu_time=usage.ru_utime.tv_sec+usage.ru_utime.tv_usec/1e6;
  system_cpu_time=usage.ru_stime.tv_sec+usage.ru_stime.tv_usec/1e6;
  total_cpu_time=user_cpu_time+system_cpu_time;
  cpu_percentage=(total_cpu_time/program_elapsed_time)*100;

  printf("Time elapsed: %f s\n",program_elapsed_time);
  printf("User cpu time: %f s\n",user_cpu_time);
  printf("System cpu time: %f s\n",system_cpu_time);
  printf("Total cpu time: %f s\n",total_cpu_time);
  printf("CPU %% usage: %f %%\n",cpu_percentage);

  return 0;
}




