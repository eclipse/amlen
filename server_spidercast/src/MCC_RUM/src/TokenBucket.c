/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */


/* when setting TB_THREAD to 1 make sure the task is removed */
#define TB_THREAD 0

#define TOKEN_BUCKET_MINRATE_BYTE_MILLI 100 

static token_bucket *init_bucket(uint32_t trans_rate, int min_sleep_time);
static int destroy_bucket(token_bucket *bucket);
static int update_bucket_rate(token_bucket * bucket, int factor);
static int get_tokens(token_bucket * bucket, int tokens);
static int credit_get_tokens(token_bucket * bucket, int tokens);
#if TB_THREAD
static THREAD_RETURN_TYPE bucket_update_thread(void * param);
#endif
/**************************************************************/
token_bucket *init_bucket(uint32_t trans_rate, int sleep_time)
{
  token_bucket *bucket;
  
  if ( (bucket = malloc(sizeof(token_bucket))) == NULL ) return NULL; 
  if ( pthread_mutex_init(&bucket->mutex, NULL) != 0 ||
    pthread_cond_init(&bucket->waiting,NULL) != 0 )
  {
    rmm_fprintf(stderr, "(init_bucket) failed to initialize mutex!\n");
    free(bucket);
    return NULL;
  }
  bucket->status = THREAD_INIT;
  bucket->tokens_per_milli = trans_rate / 1000;
  bucket->tokens_per_milli_max = bucket->tokens_per_milli;
  bucket->tokens_per_milli_min = TOKEN_BUCKET_MINRATE_BYTE_MILLI;
  if ( bucket->tokens_per_milli == 0 ) 
  {
    rmm_fprintf(stderr, "Token Bucket transmission rate error\n");
    free(bucket);
    return NULL; 
  }
  bucket->sleep_time = sleep_time;
  bucket->size = rmmMax(2*bucket->tokens_per_milli*(bucket->sleep_time+10), MAX_PACKET_SIZE);
  bucket->tokens = bucket->size;

#if TB_THREAD
  if ( pthread_create(&bucket->updater,NULL,bucket_update_thread, (void *)bucket) != 0)  /* start the updater thread */
  {
    rmm_fprintf(stderr, "(init_bucket) failed to create token bucket updater thread!\n");
    free(bucket);
    return NULL;
  }
#endif
  
  return bucket;
}

/**************************************************************/
int destroy_bucket(token_bucket *bucket)
{
  int rc = RMM_SUCCESS;
#if TB_THREAD
  int n = 10;
#endif
  
  if ( bucket == NULL ) return RMM_FAILURE;

  bucket->status = THREAD_KILL;

#if TB_THREAD
  while ( n > 0 )
  {
    if ( bucket->status == THREAD_EXIT ) break;
    Sleep(bucket->sleep_time); 
    n--;
  }
  if ( n <= 0 ) 
  {
    pthread_cancel(bucket->updater);
    pthread_join(bucket->updater, NULL);
    rc = RMM_FAILURE;
  }
#endif
  
  pthread_mutex_destroy(&bucket->mutex);
  pthread_cond_destroy(&bucket->waiting);
  free(bucket);
  
  return rc;
}

/**************************************************************/
int update_bucket_rate(token_bucket * bucket, int percent)
{
  int rate_add=0;

  if ( bucket == NULL ) return RMM_FAILURE;

  if ( percent > 100 && (bucket->tokens_per_milli >=  bucket->tokens_per_milli_max) )
    return RMM_FAILURE;

  if ( percent < 100 && (bucket->tokens_per_milli <=  bucket->tokens_per_milli_min) )
    return RMM_FAILURE;

  pthread_mutex_lock(&bucket->mutex );
  
  if ( percent > 100 ) rate_add = TOKEN_BUCKET_MINRATE_BYTE_MILLI;
  else rate_add = 0;
  
  bucket->tokens_per_milli = (percent*bucket->tokens_per_milli/100) + rate_add;
  
  if ( bucket->tokens_per_milli > bucket->tokens_per_milli_max )
    bucket->tokens_per_milli = bucket->tokens_per_milli_max;
  
  if ( bucket->tokens_per_milli < bucket->tokens_per_milli_min )
    bucket->tokens_per_milli = bucket->tokens_per_milli_min;
  
  bucket->size = rmmMax(2*bucket->tokens_per_milli*(bucket->sleep_time+10), MAX_PACKET_SIZE);
  
  pthread_mutex_unlock(&bucket->mutex );
  
  return RMM_SUCCESS;
}

/**************************************************************/
/* Returns 1 if tokens received, 0 if the amount requested is larger than bucket size.
Waits indefinitely until the right amount is available.          */
int get_tokens(token_bucket * bucket, int tokens)
{
  if ( bucket == NULL ) return RMM_FAILURE;

  if (tokens > bucket->size )
    return RMM_FAILURE;
  pthread_mutex_lock(&bucket->mutex );
  while (bucket->status == THREAD_RUNNING && bucket->tokens < tokens)  /* BEAM suppression: infinite loop */
  {
    pthread_cond_wait(&bucket->waiting, &bucket->mutex);
  }
  bucket->tokens -= tokens;
  pthread_mutex_unlock(&bucket->mutex );
  
  return RMM_SUCCESS;
}

/**************************************************************/
int credit_get_tokens(token_bucket * bucket, int tokens)
{
  if ( bucket == NULL ) return RMM_FAILURE;

  pthread_mutex_lock(&bucket->mutex);
  if ( bucket->tokens > tokens )
    bucket->tokens -= tokens;
  else 
    bucket->tokens = 0;
  pthread_mutex_unlock(&bucket->mutex);
  
  return RMM_SUCCESS;
}


/**************************************************************/
/**************************************************************/
#if TB_THREAD

THREAD_RETURN_TYPE bucket_update_thread(void * param)
{
  token_bucket *bucket = (token_bucket *)param;
  rmm_uint64  prev_time, cur_time, period_time;
  int time_elapsed, period_length, actual_sleep;
  int update_cycle;
  
  prev_time = rmmTime(NULL,NULL);
  period_time = prev_time;
  period_length = 0;
  actual_sleep = bucket->sleep_time ;
  update_cycle = rmmMax(1, 100/bucket->sleep_time);
  
  bucket->status = THREAD_RUNNING;
  
  while ( bucket->status != THREAD_KILL )
  {
    Sleep(bucket->sleep_time);
    
    cur_time = rmmTime(NULL,NULL);
    if ( cur_time < prev_time )
      continue;
    time_elapsed = (int)(cur_time - prev_time);
    prev_time = cur_time;
    
    /* update tokens */
    pthread_mutex_lock(&bucket->mutex);
    bucket->tokens += bucket->tokens_per_milli * time_elapsed;
    if (bucket->tokens > bucket->size)
      bucket->tokens = bucket->size ;
    pthread_mutex_unlock(&bucket->mutex);
    
    pthread_cond_broadcast(&bucket->waiting);
    
  }  /* while */
  
  bucket->status = THREAD_EXIT;
  rmm_thread_exit();
}
#endif
