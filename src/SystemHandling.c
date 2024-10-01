#include "SystemHandling.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int open_csv_batch(const char *folder_path,int symbols_count,
                   FILE** file_handlers){
  char buffer[FILEPATH_BUFFER_LENGTH];
  // Make sure path exists
  if(ensure_directory_exists(folder_path)!=0){
    printf("Error in creating directory: %s\n",folder_path);
    exit(-1);
  }
  // Open file handlers
  for(int i=0;i<symbols_count;i++){
    snprintf(buffer,FILEPATH_BUFFER_LENGTH-1,"%s/%s.csv",folder_path,
             symbols_list[i]);
    file_handlers[i]=fopen(buffer,"a");
    // For any error return -1
    if(file_handlers==NULL){
      printf("Error in opening file: %s\n",buffer);
      return -1;
    }
  }
  return 0;
}


int close_csv_batch(FILE** handlers,int number_of_symbols){
  for(int i=0;i<number_of_symbols;i++){
    fclose(handlers[i]);
  }
  return 0;
}


int open_delay_files(const char *folder_path,int writers_count,
                     FILE** delay_writers_logs,FILE** delay_calc_logs){
  char buffer[FILEPATH_BUFFER_LENGTH];
  // Make sure directory exists
  if(ensure_directory_exists("./delays")!=0){
    printf("Error in delay folder creation...\n");
    exit(-1);
  }
  // Open writer logs
  for(int i=0;i<writers_count;i++){
    snprintf(buffer,FILEPATH_BUFFER_LENGTH,"%s/writer_%d.csv",folder_path,i);
    delay_writers_logs[i]=fopen(buffer,"a");
    if(delay_writers_logs[i]==NULL){
      printf("Error in opening: %s\n",buffer);
      return -1;
    }
  }
  // Open calc logs
  snprintf(buffer,FILEPATH_BUFFER_LENGTH,"%s/calculator.csv",folder_path);
  *delay_calc_logs=fopen(buffer,"a");
  if(*delay_calc_logs==NULL){
    printf("Error in opening: %s\n",buffer);
    return -1;
  }
  return 0;
}

int close_delay_files(FILE **delay_writers_logs,FILE **delay_calc_logs,
                      int writers_count){
  for(int i=0;i<writers_count;i++){
    fclose(delay_writers_logs[i]);
  }
  fclose(*delay_calc_logs);
  return 0;
}


int ensure_directory_exists(const char *dir_name){
  if(access(dir_name, F_OK)!=0){
    // Directory doesn't exist, create
    if(mkdir(dir_name,0755)!=0){
      // If failure in directory creation exit
      return -1;
    }
  }
  return 0;
}
