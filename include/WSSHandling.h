/**
 * Implementation of WSS Connection handling using libwebsockets.
 * All configuration of the connection is done here.
*/
#ifndef WSS_HANDLING_HPP
#define WSS_HANDLING_HPP

#include <libwebsockets.h>
#include <stdbool.h>
#include "PCQueue.h"
#include "TradeProcessing.h"

// Used for snprintf on statically allocated buffers.
#define TEMP_BUFFER_LENGTH 256

#define PROGRAM_MAX_HOUR_LIMIT 48

// List of symbols defined concretely in main.c
extern const char symbols_list[][SYMBOLS_MAX_LENGTH];

// The 1st pipeline stage PCQueue, defined in main.c
extern PCQueue api_queue;

// Global flags for connection status defined in main.c
extern bool exit_wss_connection; // When asserted, exit gracefully.
extern bool connection_closed; // When asserted, connection to WSS was closed.


/**
 * @brief Represents all objects that result from the WSS connection.
 */
typedef struct{
  struct lws_context *ctx; //< The connection's context.
  struct lws *wsi; //< The connection's pointer.
} WSS_Objects;


/**
 * @brief Sets up the connection to Finnhub's API using LWS's WSS Client.
 *
 * @param[in] api_key The API user's key needed for authentication.
 *
 * @return On success, WSS_Objects struct with context and connection pointer.
 * If connection was unsuccessful, returns NULL pointers.
 */
WSS_Objects finnhub_connection_setup(char* api_key);


/**
 * @brief Sends subscribe messages for each symbol on symbols_list 
 *
 * @param[in] wsi Pointer to the connection that is accessed.
 *
 * @return 0 on success, -1 on failure.
 */
int subscribe_to_symbols(struct lws *wsi);


/**
 * @brief Sends a message to the connected server.
 *
 * @param[in] wsi Pointer to connection that is being accessed.
 * @param[in] msg Message that is being sent.
 *
 * @return 0 on success, error value of lws_write on failure.
 */
int write_to_server(struct lws *wsi, char *msg);


/**
 * @brief Main callback function of WSS connection.
 *
 * Handles connection establishment, data reception, connection errors,
 * and connection cut-offs.
 *
 * @param[in] wsi Pointer to connection.
 * @param[in] reason Reason of callback.
 * @param[in] user User pointer for custom use (not used here).
 * @param[in] in Pointer to incoming data if data arrived.
 * @param[in] len Size of in (in bytes).
 *
 * @return 0 on success, -1 on exit.
 */
int lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, 
                 void *in, size_t len);


/**
 * @brief The interrupt routine that starts the program exit.
 * 
 * It's mapped to SIGINT and asserts the exit_flag for graceful exit.
 *
 * @param sig Signal number to verify it's SIGINT
 */
void close_connection_interrupt(int sig);


/**
 * @brief Sets up timer to send a SIGALRM at each minute start.
 *
 * Not starting from program execution, sends at XX.00 minutes.
 */
void setup_timer();


/**
 * @brief Sends a directive to the api_queue to calculate minute.
 *
 * It's bound to SIGALRM and every minute adds a special directive 
 * item to the 1st stage queue. (Directive item is differentiated by v<0)
 *
 * At PROGRAM_MAX_HOUR_LIMIT, asserts the exit flag for graceful exit.
 */
void send_directive_to_queue(int signum);


#endif
