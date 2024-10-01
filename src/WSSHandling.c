#include "WSSHandling.h"
#include "JSONParsing.h"
#include "PCQueue.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <libwebsockets.h>
#include <time.h>




// Main callback function of the WSS connection
int lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
                 void *in, size_t len){
  //printf("Reason: %d\n",reason);
  // The json parser context.
  static struct lejp_ctx json_ctx;
  int return_code=0;


  switch(reason){
    // INITIAL CONNECTION
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf("Connection established\n");
      // Construct the parser
      construct_parser(&api_queue, &json_ctx);
      // Subscribe to everything configured
      if(subscribe_to_symbols(wsi)!=0){
        // If there was error in subscribing, try again...
        printf("Error in subscription, exiting...\n");
        connection_closed=true;
        lejp_destruct(&json_ctx);
        return -1;
      }
      break;
    // AT EACH RECEPTION OF DATA
    case LWS_CALLBACK_CLIENT_RECEIVE:
      //printf("%.*s\n",(int)len,(char*)in);
      // Exclusive access to producer end for whole parse.
      pthread_mutex_lock(api_queue.producer_lock);
      // Parse the received stream.
      return_code=lejp_parse(&json_ctx,(unsigned char*)in,len);
      // Check if stream was successful
      if(return_code<0 && return_code!=LEJP_CONTINUE){
        printf("Error in stream parsing: %s\n",
               lejp_error_to_string(return_code));
      }
      // Unlock the mutex for minute events.
      pthread_mutex_unlock(api_queue.producer_lock);
      // Reset parser
      construct_parser(&api_queue, &json_ctx);
      break;
    // IF CONNECTION WAD CLOSED
    case LWS_CALLBACK_CLIENT_CLOSED:
      printf("Connection closing...\n");
      // Assert flag
      connection_closed=true;
      // If it was intended, exit gracefully
      if(api_queue.exit_flag==1){
        printf("Signaling all threads\n");
        pthread_cond_signal(api_queue.not_empty);
        lws_close_reason(wsi,LWS_CLOSE_STATUS_NORMAL,NULL,0);
      }
      lejp_destruct(&json_ctx);
      return -1;
      break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      printf("Connection error\n");
      connection_closed=true;
      break;
    default:
      break;
  }
  return 0;
}

// If any structure wasn't created correctly, return NULL
WSS_Objects finnhub_connection_setup(char* api_key){
  // Context/connection info 
  struct lws_context_creation_info ctx_info;
  struct lws_client_connect_info conn_info;
  // The result
  WSS_Objects result;
  result.ctx=NULL;
  result.wsi=NULL;
  // Protocol for WSS connetion
  static struct lws_protocols protocols[]={
      {"finnhub-protocol",lws_callback,0,0,0,NULL,0},
      {NULL,NULL,0,0,0,NULL,0}
    };
  // Buffer for api key configuration 
  static char api_key_buffer[TEMP_BUFFER_LENGTH+1];


  // Initialize context
  memset(&ctx_info, 0, sizeof(ctx_info));
  ctx_info.port=CONTEXT_PORT_NO_LISTEN;
  ctx_info.protocols=protocols;
  ctx_info.options=LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  ctx_info.uid=-1;
  ctx_info.gid=-1;
  ctx_info.ssl_ca_filepath="./ca-certificates.crt";
  struct lws_context *ctx=lws_create_context(&ctx_info); 
  if(ctx==NULL){
    printf("Error in context creation\n");
    return result;
  }

  // Initialize connection info
  memset(&conn_info,0,sizeof(conn_info));
  conn_info.context=ctx;
  conn_info.ssl_connection=LCCSCF_USE_SSL;
  conn_info.address="ws.finnhub.io";
  conn_info.port=443; // Default WSS port
  // Copy API key to path
  snprintf(api_key_buffer,TEMP_BUFFER_LENGTH,"/?token=%s",api_key);
  conn_info.path=api_key_buffer;
  conn_info.host=conn_info.address;
  conn_info.origin=conn_info.address;
  conn_info.protocol=protocols[0].name;
  conn_info.ietf_version_or_minus_one = -1;
  struct lws *wsi=lws_client_connect_via_info(&conn_info);
  if(wsi==NULL){
    printf("Connection failed\n");
    // Deallocate the context that was created.
    lws_context_destroy(result.ctx);
    result.ctx=NULL;
    return result;
  }
  // Bind interrupt to exit flag switch
  signal(SIGINT,close_connection_interrupt); 
  // Return the ctx for the lws_service routine
  result.ctx=ctx;
  result.wsi=wsi;
  return result; 
}

int subscribe_to_symbols(struct lws *wsi){
  char buffer[TEMP_BUFFER_LENGTH];
  // For each symbol in symbols_list
  for(int i=0;strlen(symbols_list[i])>0;i++){
    // Create subscribe message
    snprintf(buffer, TEMP_BUFFER_LENGTH,
             "{\"type\":\"subscribe\",\"symbol\":\"%s\"}",
             symbols_list[i]); 
    printf("Subsribing to: %s\n",symbols_list[i]); 
    if(write_to_server(wsi,buffer)!=0){
      printf("Error while subscribing to %s\n",symbols_list[i]);
      return -1;
    }
  }
  return 0;
}

int write_to_server(struct lws *wsi, char *msg){
  size_t len=strlen(msg);
  unsigned char buf[LWS_PRE+len];
  // Pre pad the message for the transmission.
  memcpy(&buf[LWS_PRE],msg,len);
  int return_code;
  return_code=lws_write(wsi,&buf[LWS_PRE],len,LWS_WRITE_TEXT);
  if(return_code<0){
    printf("Error on lws_write\n");
    return return_code;
  }
  return 0;
}

void close_connection_interrupt(int sig){
  printf("\nProgram interrupted...\n");
  if(sig==SIGINT){
    // Access the queue
    pthread_mutex_lock(api_queue.mut);
    // Activate the exit flag of the queue
    api_queue.exit_flag=1;
    //pthread_cond_signal(api_queue.not_empty);
    // Release the queue
    pthread_mutex_unlock(api_queue.mut);
  }
  return;
}

void setup_timer(){
  struct itimerval timer;
  struct timeval current_time;
  // Setup alarm sig to routine
  if (signal(SIGALRM, send_directive_to_queue) == SIG_ERR) {
        perror("Error installing signal handler");
        exit(EXIT_FAILURE);
    }
  // Calculate initial delay 
  gettimeofday(&current_time,NULL);
  timer.it_value.tv_sec=59-current_time.tv_sec%60; 
  timer.it_value.tv_usec=(int)1e6-current_time.tv_usec;
  // Set up interval
  timer.it_interval.tv_sec=60;
  timer.it_interval.tv_usec=0;
  
  // Init timer
  if(setitimer(ITIMER_REAL,&timer,NULL)==-1){
    perror("Error in timer setup: ");
    exit(1);
  }
  return;
}

// Function that executes at every minute
void send_directive_to_queue(int signum){
  static int minute_counter=0;
  static int hour_counter=0;
  // Count to 48 hours
  minute_counter++;
  hour_counter+=(minute_counter/60);
  minute_counter=minute_counter%60;
  if(minute_counter==1){
    printf("Hour %d\n",hour_counter);
  }

  struct timeval current_time;
  gettimeofday(&current_time,NULL);

  // Prepare directive
  WorkItem directive_item;
  directive_item.event_time=current_time;
  directive_item.trade.t=current_time.tv_sec/60-1;
  directive_item.trade.v=DIRECTIVE_CALCULATE_MINUTE;

  // Get queue access 
  pthread_mutex_lock(api_queue.producer_lock);
  // Add directive
  queue_add(&api_queue,&directive_item);
  // If time limit was reached, exit
  if(hour_counter==PROGRAM_MAX_HOUR_LIMIT){
    printf("Hour limit was reached..\n");
    api_queue.exit_flag=true;
  }
  // Give access back
  pthread_mutex_unlock(api_queue.producer_lock);
  return;
}















