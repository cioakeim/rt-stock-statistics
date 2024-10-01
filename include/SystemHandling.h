/*
* This module contains methods for creating file systems (mainly csv batches)
* for each thread routine. Also contains methods for measuring cpu usage.
*/
#ifndef SYSTEM_HANDLING_H
#define SYSTEM_HANDLING_H 

#include "TradeProcessing.h"

#define FILEPATH_BUFFER_LENGTH 100

// List of symbols defined concretely in main.c
extern const char symbols_list[][SYMBOLS_MAX_LENGTH];

/**
 * @brief Creates a file batch of csv files for each symbol.
 *
 * Given the symbol list above, for each symbol X, opens 
 * (and creates if non-existant) folder_path/X.csv. Every
 * open is in append mode.
 *
 * @param[in]  folder_path    Path to the batch folder.
 * @param[in]  symbols_count  Number of symbols. 
 * @param[out] file_handlers  Array of file handlers, one 
 * for each symbol.
 *
 * @return 0 on success, -1 on any failed open.
 */
int open_csv_batch(const char *folder_path,int symbols_count,
                   FILE** file_handlers);

/**
 * @brief Given a file_handler array, closes all files. 
 *
 * @param[in] handlers      Array of files that will be closed.
 * @param[in] symbols_count Number of symbols. 
 *
 * @return 0 on success.
 */
int close_csv_batch(FILE** handlers,int symbols_count);

/**
 * @brief Creates the delay log files for the writers and the calculator.
 *
 * @param[in]  folder_path Where the files will be created.
 * @param[in]  writers_count The number of writers.
 * @param[out] delay_writers_logs The file handlers of the writers.
 * @param[out] delay_calc_logs The file handler of the calculator.
 */
int open_delay_files(const char *folder_path,int writers_count,
                     FILE** delay_writers_logs,FILE** delay_calc_logs);

/**
 * @brief Closes the delay files.
 *
 * @param[in] delay_writer_logs The file handlers of the writers.
 * @param[in] delay_calc_logs The file handler of the calculator.
 * @param[in] writers_count The number of writers.
 */
int close_delay_files(FILE **delay_writers_logs,FILE **delay_calc_logs,
                      int writers_count);

/**
* @brief Ensures a directory exists given a path.
*
* @param[in]  path  Full/relative path to directory 
* e.g /path/to/final_folder
*
* @returns 0 on success, -1 on failure to create.
*/
int ensure_directory_exists(const char *path);


#endif
