/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: twolockqueue.c                                       */
/*                                                                   */
/* Description: Simple toy queue with separate putter and getter     */
/*              locks. We do NOT plan to use this code or algorithm  */
/*              (no easy way to extend for atomic commits).          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/***************************************************************/
/*Set of defines for playing with the behaviour of the toy     */
/*program                                                      */
/***************************************************************/

//Should the queue struct we carefully spaced to avoid false sharing
#define CACHELINE_SEPARATION 1
//Need to have locks unless there's and most 1putter and 1 getter
#define LOCKING 1
//If we want a single thread that does putting and getting undef this:
#define MULTITHREADED 1


#ifdef CACHELINE_SEPARATION
  #define CACHELINE_SIZE 64
  #define CACHELINE_ALIGNED __attribute__ ((aligned (CACHELINE_SIZE)))
#else
  #define CACHELINE_ALIGNED
#endif


typedef struct tag_ismPTLQMessage {
   char buffer[100];
} ismPTLQMessage;

typedef struct tag_ismPTLQQueueNode {
   struct tag_ismPTLQQueueNode *volatile next;
   ismPTLQMessage *msg;
} ismPTLQQueueNode;


typedef struct tagismPTLQQueue {
   ismPTLQQueueNode *volatile dummyhead CACHELINE_ALIGNED;
   pthread_spinlock_t getlock CACHELINE_ALIGNED;
   ismPTLQQueueNode *volatile tail CACHELINE_ALIGNED;
   pthread_spinlock_t putlock CACHELINE_ALIGNED; 
} ismPTLQueue;

typedef struct tagTmpTestThreadInfo {
   pthread_t thread_id;
   ismPTLQueue *q;
   int numMsgs;
   volatile int numMsgsDone;
} TmpTestThreadInfo;


void handle_error(const char *errmsg) {
   printf("%s\n",errmsg);  
}


int ismPTLQinitialiseQ(ismPTLQueue **Q) {
   int rc=0;
   ismPTLQQueueNode *dummy;
   
   *Q = (ismPTLQueue *)malloc(sizeof(ismPTLQueue));
   
   if(*Q == NULL) {
      rc = 10;
      goto mod_exit;  
   }
   
   //set up the locks
   rc = pthread_spin_init(&((*Q)->getlock), PTHREAD_PROCESS_PRIVATE);
   if(rc != 0 ) {
      handle_error("Create getlock\n");
      goto mod_exit;  
   }
   rc = pthread_spin_init(&((*Q)->putlock), PTHREAD_PROCESS_PRIVATE);
   if(rc != 0 ) {
      handle_error("Create putlock\n");
      goto mod_exit;  
   }
   
   //We always have one dummy node on the queue
   dummy = (ismPTLQQueueNode *)calloc(1, sizeof(ismPTLQQueueNode));
    
   if(dummy == NULL) {
      rc = 20;
      goto mod_exit;  
   }
   
   dummy->next = NULL;
   dummy->msg  = NULL;
   
   (*Q)->dummyhead = dummy;
   (*Q)->tail = dummy;
  
mod_exit:
   return rc;    
}

int ismPTLQ_put(ismPTLQueue *Q, ismPTLQMessage *inmsg) {
   ismPTLQQueueNode *node;
   ismPTLQMessage   *msg;
   int rc = 0;

   /* Copy the incoming message - we can't "steal" the memory */
   msg = (ismPTLQMessage *)malloc(sizeof(ismPTLQMessage));
   
   if(msg == NULL) {
      rc = 10;
      goto mod_exit;  
   }
   
   memcpy(msg, inmsg, sizeof(ismPTLQMessage));
   
   node = (ismPTLQQueueNode *)malloc(sizeof(ismPTLQQueueNode));
    
   if(node == NULL) {
      rc = 30;
      goto mod_exit;  
   }
   
   node->next = NULL;
   node->msg  = msg;

#ifdef LOCKING   
   //Get the putting lock
   rc = pthread_spin_lock(&(Q->putlock));
         
   if(rc != 0) {
     handle_error("Fail to get producer lock");
     goto mod_exit; 
   }
#endif
   Q->tail->next = node;
   
   //Think about a crash here: We'll have a valid message chain but tail won't point to the end
   //                          We'll need to push tail forward on restart
   Q->tail = node;

#ifdef LOCKING   
   //Release the putting lock
   rc = pthread_spin_unlock(&(Q->putlock));
         
   if(rc != 0) {
     handle_error("Fail to release producer lock");
     goto mod_exit; 
   }
#endif
   
mod_exit:   
   return rc;
}

int ismPTLQ_get(ismPTLQueue *Q, ismPTLQMessage **outmsg) {
   int rc = 0;
   ismPTLQQueueNode *dummyNode; 
   ismPTLQQueueNode *nextNode;
   
   ismPTLQMessage *msg;
#ifdef LOCKING    
   //Get the getting lock
   rc = pthread_spin_lock(&(Q->getlock));
         
   if(rc != 0) {
     handle_error("Fail to get consumer lock");
     goto mod_exit; 
   }
#endif
   
   dummyNode = Q->dummyhead;
   nextNode  = dummyNode->next;
   
   if(nextNode != (ismPTLQQueueNode *)NULL) {
      msg = nextNode->msg;
      
      //turn nextNode into the new dummyNode
      nextNode->msg = NULL;
      Q->dummyhead = nextNode; 
#ifdef LOCKING      
      //Release the getting lock
      rc = pthread_spin_unlock(&(Q->getlock));
         
      if(rc != 0) {
        handle_error("Fail to release consumer lock");
        goto mod_exit; 
      }
#endif
      
      //Now (outside the lock) we can move the message back from "special" memory
      //At the moment we can just return a pointer...
      *outmsg = msg;
      
      //free the old dummynode
      free(dummyNode);
   } else {
#ifdef LOCKING
      //Release the getting lock
      rc = pthread_spin_unlock(&(Q->getlock));
         
      if(rc != 0) {
        handle_error("Fail to release consumer lock");
        goto mod_exit; 
      }
#endif
      
      *outmsg = NULL;  
   }
   
mod_exit:
   return rc;  
}

void ismPTLQ_freeMessage(ismPTLQMessage **msg) {
   free(*msg);
   *msg = NULL;
}

void ismPTLQ_freeQ(ismPTLQueue **Q) {
  int rc;
  
  while( ((*Q)->dummyhead) != NULL ) {
     ismPTLQQueueNode *temp = (*Q)->dummyhead;
     (*Q)->dummyhead = (*Q)->dummyhead->next;
     
     if(temp->msg != NULL) {
       ismPTLQ_freeMessage(&(temp->msg));  
     }
     
     free(temp);
  }
  
   //set up the locks
   rc = pthread_spin_destroy(&((*Q)->getlock));
   if(rc != 0 ) {
      handle_error("destroy getlock\n");
   }
   rc = pthread_spin_destroy(&((*Q)->putlock));
   if(rc != 0 ) {
      handle_error("destroy putlock\n");
   }
  
  
  free(*Q);
  *Q = NULL;
}

void *TmpTestPutterThread(void *args) {
   TmpTestThreadInfo *threadargs = (TmpTestThreadInfo *)args;
   ismPTLQMessage putmsg;
   
   strcpy(putmsg.buffer,"A Test Message");
   
   for(threadargs->numMsgsDone =0 ; threadargs->numMsgsDone < threadargs->numMsgs; threadargs->numMsgsDone++) {
      ismPTLQ_put(threadargs->q, &putmsg);
   }
   return NULL;
}

void *TmpTestGetterThread(void *args) {
   TmpTestThreadInfo *threadargs = (TmpTestThreadInfo *)args;
   ismPTLQMessage *gotmsg;
   
   do {
     ismPTLQ_get(threadargs->q, &gotmsg);
     if( gotmsg != NULL ) {
        //printf("%d: %s\n",(threadargs->numMsgsDone)+1,gotmsg->buffer);
          //if( 0 == (threadargs->numMsgsDone % 32768)) printf("%d: done %d\n",threadargs->thread_id, threadargs->numMsgsDone);
          //if(gotmsg->buffer[8] == 'v') { printf("It's a v\n");}
        ismPTLQ_freeMessage(&gotmsg);
        threadargs->numMsgsDone++;
     } else {
        //printf("Got no msg\n");
     }
   } while( threadargs->numMsgsDone < threadargs->numMsgs);

   return NULL;
}

//A single thread that does putting and getting...not usually used

void *TmpTestPutterGetterThread(void *args) {
   TmpTestThreadInfo *threadargs = (TmpTestThreadInfo *)args;
   ismPTLQMessage putmsg;
   ismPTLQMessage *gotmsg;
   
   strcpy(putmsg.buffer,"A Test Message");
   
   for(threadargs->numMsgsDone =0 ; threadargs->numMsgsDone < threadargs->numMsgs; threadargs->numMsgsDone++) {
      ismPTLQ_put(threadargs->q, &putmsg);
        
        ismPTLQ_get(threadargs->q, &gotmsg);
      if( gotmsg != NULL ) {
        //printf("%d: %s\n",(threadargs->numMsgsDone)+1,gotmsg->buffer);
        ismPTLQ_freeMessage(&gotmsg);
        threadargs->numMsgsDone++;
      } else {
        printf("Got no msg\n");
      }
   }
   return NULL;
}

int main(int argc, char **argv) {
   int rc = 0;
   int msgs_per_thread=20000000;
   int num_threads=1; //Our toy harness creates this many putters and this many getters
   int i;
   ismPTLQueue *Q;
   TmpTestThreadInfo *putthreadinfo;
   TmpTestThreadInfo *getthreadinfo;
   
   rc = ismPTLQinitialiseQ(&Q);
   
   if( rc != 0 ) {
     goto mod_exit;  
   }

#ifdef MULTITHREADED  
   putthreadinfo = calloc(num_threads, sizeof(TmpTestThreadInfo));
   if (putthreadinfo == NULL) {
      handle_error("calloc TmpTestThreadInfo - put");
      rc = 10;
      goto mod_exit;
   }
   getthreadinfo = calloc(num_threads, sizeof(TmpTestThreadInfo));
   if (putthreadinfo == NULL) {
      handle_error("calloc TmpTestThreadInfo - get");
      rc = 10;
      goto mod_exit;
   }
   
   for(i = 0; i < num_threads; i++) {
      putthreadinfo[i].numMsgs=msgs_per_thread;
      putthreadinfo[i].q = Q;

      rc = test_task_startThread(&(putthreadinfo[i].thread_id),TmpTestPutterThread, (void *)&(putthreadinfo[i]),"TmpTestPutterThread");

      if( rc != 0) {
         handle_error("pthread_create - put");
         goto mod_exit;  
      }

      getthreadinfo[i].numMsgs=msgs_per_thread;
      getthreadinfo[i].q = Q;

      rc = test_task_startThread(&(getthreadinfo[i].thread_id),TmpTestGetterThread, (void *)&(getthreadinfo[i]),"TmpTestGetterThread");

      if( rc != 0) {
         handle_error("pthread_create- get");
         goto mod_exit;  
      }
   }
 
   //Cleanup the putters
   for(i = 0; i < num_threads; i++) {
      rc = pthread_join(putthreadinfo[i].thread_id, NULL);
      if( rc != 0) {
         handle_error("pthread_join - putters");
         goto mod_exit;  
      }
   }
   
   //Cleanup the getters
   for(i = 0; i < num_threads; i++) {
      rc = pthread_join(getthreadinfo[i].thread_id, NULL);
      if( rc != 0) {
         handle_error("pthread_join - getters");
         goto mod_exit;  
      }
   }
#else
   putthreadinfo = calloc(1, sizeof(TmpTestThreadInfo));
   if (putthreadinfo == NULL) {
      handle_error("calloc TmpTestThreadInfo - single");
      rc = 10;
      goto mod_exit;
   }

   putthreadinfo[i].numMsgs=msgs_per_thread;
   putthreadinfo[i].q = Q;

   rc = test_task_startThread(&(putthreadinfo->thread_id),TmpTestPutterGetterThread, (void *)putthreadinfo,"TmpTestPutterGetterThread");

   if( rc != 0) {
      handle_error("pthread_create - single");
      goto mod_exit;  
   }

   rc = pthread_join(putthreadinfo->thread_id, NULL);
   if( rc != 0) {
      handle_error("pthread_join - single");
      goto mod_exit;  
   }
#endif 
   
   ismPTLQ_freeQ(&Q);
      
mod_exit:
   return rc;
}
