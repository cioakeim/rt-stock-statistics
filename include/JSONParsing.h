/**
 * Implementation of the methods used for JSON Stream parsing 
 * of the API's trades.
*/
#ifndef JSONPARSING_H
#define JSONPARSING_H 

#include <libwebsockets.h>
#include "PCQueue.h"

#define JSON_PATHS_MAX_LENGTH 10

/**
 * @brief Constructs a json parser's context.
 *
 * Also passes the 1st stage's PCQueue pointer to ctx->user,
 * for the parse to be able to add items to the 1st stage.
 *
 * @param[in]  api_queue Pointer to the queue of the 1st stage pipeline.
 * @param[out] ctx The JSON Parser context that is initialized.
 */
void construct_parser(PCQueue *api_queue,struct lejp_ctx *ctx);


/**
 * @brief Main callback function of the parse procedure.
 *
 * As the stream is parsed, each trade object found is added
 * to the 1st stage pipeline queue.
 *
 * @param[in] ctx The JSON Parser ctx.
 * @param[in] reason The reason the callback function was called.
 */
signed char json_callback(struct lejp_ctx *ctx, char reason);



#endif
