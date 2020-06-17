#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include "hiredis/hiredis.h"

af_client_t *af_new_client(void) {
  af_client_t *client = malloc(sizeof(af_client_t));
  return client;
}

af_client_result_t * af_init_client(af_client_t * client, char * host, int port) {
    af_client_result_t *result = malloc(sizeof(af_client_result_t));
    client->redisHost = host;
    client->redisPort = port;
    client->redisContext = redisConnect(client->redisHost, client->redisPort);

    if (client->redisContext == NULL || client->redisContext->err) {
      result->status = AF_CLIENT_ERROR;
      if (client->redisContext) {
        sprintf(result->msg, "Fail to connect to redis: %s\n", client->redisContext->errstr);
      } else {
          sprintf(result->msg, "Fail to allocate redis context");
      }
      free(client);
    } else {
      result->status = AF_CLIENT_OK;
    }
    return result;
}

void af_free_client(af_client_t * client) {
    redisFree(client->redisContext);
    free(client);
}

af_client_result_t *af_client_schedule(af_client_t *client, char *worker, char *job, size_t job_size) {
  af_client_result_t *result = malloc(sizeof(af_client_result_t));

  redisReply *reply = redisCommand(client->redisContext, "RPUSH %s %b",
                                   AF_CLIENT_JOBS_KEY, job, job_size);

  if (reply->type == REDIS_REPLY_ERROR) {
    result->status = AF_CLIENT_ERROR;
    sprintf(result->msg, "Fail to schedule job: %.*s\n", (int)reply->len,
            reply->str);
  } else {
    result->status = AF_CLIENT_OK;
  }

  return result;
}

af_client_result_t *af_client_store_failure(af_client_t *client, char *worker, char *job, size_t job_size) {
  af_client_result_t *result = malloc(sizeof(af_client_result_t));

  redisReply *reply = redisCommand(client->redisContext, "RPUSH %s %b", AF_CLIENT_FAILURE_KEY, job, job_size);

  if (reply->type == REDIS_REPLY_ERROR) {
    result->status = AF_CLIENT_ERROR;
    sprintf(result->msg, "Fail to store failure job: %.*s\n", (int)reply->len,
            reply->str);
  } else {
    result->status = AF_CLIENT_OK;
  }

  return result;
}

af_client_result_t *af_client_fetch_job(af_client_t *client, char **job, size_t *job_size) {
  af_client_result_t *result = malloc(sizeof(af_client_result_t));

  redisReply *reply;
  reply = redisCommand(client->redisContext, "LPOP %s", AF_CLIENT_JOBS_KEY);

  if (reply->type == REDIS_REPLY_ERROR) {
    result->status = AF_CLIENT_ERROR;
    sprintf(result->msg, "Fail to fetch job: %s\n", reply->str);
  } else if (reply->type == REDIS_REPLY_NIL) {
    result->status = AF_CLIENT_EMPTY;
    sprintf(result->msg, "Job list is empty\n");
  } else {
    result->status = AF_CLIENT_OK;
    *job = malloc(reply->len);
    memcpy(*job, reply->str, reply->len);
    *job_size = reply->len;
  }

  freeReplyObject(reply);

  return result;
}
