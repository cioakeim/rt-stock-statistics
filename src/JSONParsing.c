#include "JSONParsing.h"
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "TradeProcessing.h"
#include <inttypes.h>

// Paths that are recognized by the parser
const char my_paths[][JSON_PATHS_MAX_LENGTH]={
  "data[]",
  "data[].s",
  "data[].p",
  "data[].t",
  "data[].v"
};

void construct_parser(PCQueue *api_queue,struct lejp_ctx *ctx){
  // Pass the api_queue to the user pointer
  void *user=(void*)api_queue;
  // Make the paths argument passable
  static const char *path_pointers[]={
    my_paths[0],
    my_paths[1],
    my_paths[2],
    my_paths[3],
    my_paths[4]
  };
  lejp_construct(ctx,json_callback,user,path_pointers,
                 (char)LWS_ARRAY_SIZE(my_paths));
  return;
}


signed char json_callback(struct lejp_ctx *ctx, char reason){
  // This is the 1st stage queue of the implementation 
  PCQueue *api_queue=(PCQueue*)ctx->user;

  // Object that tracks each incoming object
  static WorkItem current_work_item;
  // These handle str->number conversions.
  static bool trade_is_valid=true;
  char *conversion_ptr;


  switch(reason){
  // Started parsing, get timestamp
  case LEJPCB_START:
    gettimeofday(&current_work_item.event_time,NULL);
    break;
  // Found an object of the data array, so assume it'll be valid
  case LEJPCB_OBJECT_START:
    if(strcmp(ctx->path,"data[]")==0){
      trade_is_valid=true;
    }
    break;
  // Found the symbol
  case LEJPCB_VAL_STR_END:
    if(strcmp(ctx->path,"data[].s")==0){
      // Find which item it corresponds to
      for(int i=0;symbols_list[i][0]>0;i++){
        if(strcmp(symbols_list[i],ctx->buf)==0){
          current_work_item.trade.s_index=i;
          break;
        }
      }
    }
    break;
  // Integer found (check all possible fields)
  case LEJPCB_VAL_NUM_INT:
    if(strcmp(ctx->path,"data[].t")==0){
      current_work_item.trade.t=strtoull(ctx->buf,&conversion_ptr,10);
      if(*conversion_ptr!='\0'){
        printf("Conversion problem\n");
        trade_is_valid=false;
      }
    }
    else if(strcmp(ctx->path,"data[].v")==0){
      current_work_item.trade.v=strtod(ctx->buf,&conversion_ptr);   
      if(*conversion_ptr!='\0'){
        printf("False v\n");
        trade_is_valid=false;
      }
    }
    else if(strcmp(ctx->path,"data[].p")==0){
      current_work_item.trade.p=strtod(ctx->buf,&conversion_ptr);   
      if(*conversion_ptr!='\0'){
        trade_is_valid=false;
      }
    }
    break;
  // Float found (check all possible fields)
  case LEJPCB_VAL_NUM_FLOAT:
    if(strcmp(ctx->path,"data[].p")==0){
      current_work_item.trade.p=strtod(ctx->buf,&conversion_ptr);   
      if(*conversion_ptr!='\0'){
        printf("False p (float)\n");
        trade_is_valid=false;
      }
    }
    else if(strcmp(ctx->path,"data[].v")==0){
      current_work_item.trade.v=strtod(ctx->buf,&conversion_ptr);   
      if(*conversion_ptr!='\0'){
        printf("False v\n");
        trade_is_valid=false;
      }
    }
    break;
  // Object fully scanned
  case LEJPCB_OBJECT_END:
    // If object was on data array it's a trade, add if valid
    if(trade_is_valid && strcmp(ctx->path,"data[]")==0){
      queue_add(api_queue,&current_work_item);
    }
    break;
  default:
    break;
  }
  return 0;
}
