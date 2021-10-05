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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <poll.h>

#include <store_unit_test.h>
#include <store.h>
#include <ha.h>

#define RECORD_ATTRIBUTE   0xabcd
#define RECORD_STATE       0xbcde
#define TEST_DELDISKGEN         0

extern testParameters_t test_params;

double t0;
volatile int goOn;
volatile int initialized=0;

/*
 * Array that carries all simple store tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_store_tests[] = {
        { "Initialization", testInitialization },
        { "CreateMsgs1Thread", testCreateMsgs1Thread },
        { "CreateMsgs4Threads", testCreateMsgs4Threads },
        { "CreateStateObjects", testCreateStates },
        { "DeleteStateObjects", testDeleteStates },
        { "Statistics", testStatistics },
        { "RefsStatistics", testRefsStatistics },
        { "StreamReservation", testReservation },
        { "MemoryAlerts", testAlerts },
        { "OwnerLimit", testOwnerLimit },
        { "CompactGeneration", testCompactGeneration },
        CU_TEST_INFO_NULL
};

/*
 * Array that carries all performance store tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_store_perf_tests[] = {
        { "PruneReferencesPerf", testPruneReferencesPerf },
        CU_TEST_INFO_NULL
};

/*
 * Array that carries the High-Availability store tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_storeHA_tests[] = {
        { "CreateMsgs1ThreadHA", testCreateMsgs1ThreadHA },
        { "ReadMsgs1ThreadHA", testReadMsgs1ThreadHA },
        { "CreateMsgs4ThreadsHA", testCreateMsgs4ThreadsHA },
        { "CreateStateObjectsHA", testCreateStatesHA },
        { "SendAdminMsgsHA", testSendAdminMsgsHA },
        { "OrphanChunksHA", testOrphanChunksHA },
        { "CompactGenerationHA", testCompactGenerationHA },
        CU_TEST_INFO_NULL
};

typedef struct
{
   ismStore_Record_t  sr[1];
   ismStore_Handle_t  handle;
   void              *ctxt;
} StoreRec;

typedef struct
{
   uint64_t nMsgs;
   uint64_t nBytes;
   uint64_t nStates;
   int      fd;
} w2fCtx;

typedef struct
{
   uint32_t refsCount;
   uint32_t statesCount;
} orphanCtxt;

typedef struct msgEntry_
{
  struct msgEntry_ *next;
  char             *msg_buf;
  size_t            msg_off;
  size_t            msg_len;
} MsgEntry;

typedef struct threadInfo_
{
  pthread_t           tid;
  pthread_mutex_t     lock[1];
  pthread_cond_t      cond[1];
  MsgEntry           *first;
  MsgEntry           *last;
  size_t              size;
  int                 objType;
  int                 compactGen;
} ThreadInfo ;

typedef struct addRefsThreadInfo_
{
  pthread_t           tid;
  pthread_mutex_t    *lock;
  pthread_cond_t     *cond;
  int                 nRefs;
  uint8_t             fInit;
} AddRefsThreadInfo ;

static double sysTime(void)
{
   struct timeval tv[1];
   gettimeofday(tv,NULL);
   return (double)tv->tv_sec + (double)tv->tv_usec/1e6 - t0;
}

static double u01(double *seed)
{
   static double mult =  742938285e0;
   static double modu = 2147483647e0;
   double s = *seed;

   if (s <= 0e0)
   {
      s = sysTime() + (double)pthread_self();
   }
   s = fmod(s*mult, modu);
   *seed = s;
   return (s / modu);
}

static void diff(const char *srcFilename, const char *dstFilename)
{
   FILE *fp;
   char cmd[1024], line[1024];

   /* Open the command for reading. */
   sprintf(cmd, "diff %s %s", srcFilename, dstFilename);
   fp = popen(cmd, "r");
   if (fp == NULL)
   {
      TRACE(1, "%s_%d: failed to execute %s command\n", __FUNCTION__, __LINE__, cmd);
      CU_ASSERT(0);
      return;
    }

    /* Read the output a line at a time - output it. */
    memset(line, 0, sizeof(line));
    if (fgets(line, sizeof(line)-1, fp) != NULL)
    {
       TRACE(1, "%s_%d: the source file %s, and destination file %s differs. (%s)", __FUNCTION__, __LINE__, srcFilename, dstFilename, line);
       CU_ASSERT(0);
    }

    /* close */
    pclose(fp);
}

static int write2file(ismStore_DumpData_t *pDumpData, void *pContext)
{
   ismStore_DumpData_t *dt = pDumpData;
   w2fCtx *w2f = (w2fCtx *)pContext;
   int fd, ec, i;
   uint64_t bytesRem;

   fd = w2f->fd;

   switch (dt->dataType)
   {
      case ISM_STORE_DUMP_GENID :
         break;
      case ISM_STORE_DUMP_REC4TYPE :
         CU_ASSERT(pDumpData->pRecord->Attribute == RECORD_ATTRIBUTE);
         CU_ASSERT(pDumpData->pRecord->State == RECORD_STATE);
         break;
      case ISM_STORE_DUMP_REF4OWNER :
         dt->readRefHandle = 1;
         break;
      case ISM_STORE_DUMP_REF :
         dt->readRefHandle = 1;
         break;
      case ISM_STORE_DUMP_MSG :
      {
         off_t off, rc;
         off = (off_t)dt->pRecord->Attribute;
         rc = lseek(fd, off, SEEK_SET);
         if (rc != off)
         {
            ec = errno;
            TRACE(1, "%s_%d: lseek failed, off= %lu, errno = %d (%s)\n", __FUNCTION__, __LINE__, off, ec, strerror(ec));
            CU_ASSERT(0);
            return ISMRC_Error;
         }
         bytesRem = dt->pRecord->DataLength;
         for (i=0; i < dt->pRecord->FragsCount && bytesRem > 0; i++)
         {
            uint64_t wbytes;

            off = dt->pRecord->pFragsLengths[0];
            wbytes = (off > bytesRem ? bytesRem : off);
            rc = write(fd, dt->pRecord->pFrags[0], wbytes);
            if (rc != wbytes)
            {
               ec = errno;
               TRACE(1, "%s_%d: write failed, len= %lu, errno = %d (%s)\n", __FUNCTION__, __LINE__, off, ec, strerror(ec));
               CU_ASSERT(0);
               return ISMRC_Error;
            }
         }
         w2f->nMsgs++;
         w2f->nBytes += dt->pRecord->DataLength;
         break;
      }
      case ISM_STORE_DUMP_PROP :
         break;
      case ISM_STORE_DUMP_STATE4OWNER :
      {
         int rc = write(fd, &dt->pState->Value, sizeof(dt->pState->Value)) ;
         if ( rc != sizeof(dt->pState->Value) )
         {
            ec = errno;
            TRACE(1, "%s_%d: write failed, len= %lu, errno = %d (%s)\n", __FUNCTION__, __LINE__, sizeof(dt->pState->Value), ec, strerror(ec));
            CU_ASSERT(0);
            return ISMRC_Error;
          }
          w2f->nStates++;
          w2f->nBytes += sizeof(dt->pState->Value);
          break ;
      }
      default :
         break;
   }
   return ISMRC_OK;
}

static int validateOrphan(ismStore_DumpData_t *pDumpData, void *pContext)
{
   ismStore_DumpData_t *dt = pDumpData;
   orphanCtxt *pOrphanCtxt = (orphanCtxt *)pContext;

   if (dt->dataType == ISM_STORE_DUMP_REF4OWNER)
   {
      CU_ASSERT_FATAL(dt->pReference->Value >= 3000);
      pOrphanCtxt->refsCount++;
   }
   else if (dt->dataType == ISM_STORE_DUMP_STATE4OWNER)
   {
      CU_ASSERT_FATAL(dt->pState->Value >= 3000);
      pOrphanCtxt->statesCount++;
   }

   return ISMRC_OK;
}

static int initRecord(ismStore_Record_t *pRecord)
{
   if ( (pRecord->pFrags = (char **)malloc(sizeof(char *))) == NULL)
   {
      return 0;
   }
   memset(pRecord->pFrags, 0, sizeof(char *));
   if ( (pRecord->pFragsLengths = (uint32_t *)malloc(sizeof(uint32_t))) == NULL)
   {
      return 0;
   }
   memset(pRecord->pFragsLengths, 0, sizeof(uint32_t));
   pRecord->FragsCount = 1;
   return 1;
}

static void freeRecord(ismStore_Record_t *pRecord)
{
   int i;

   if (pRecord->pFrags)
   {
      for (i=0; i < pRecord->FragsCount; i++)
      {
         if (pRecord->pFrags[i])
         {
            free(pRecord->pFrags[i]);
         }
      }
      free(pRecord->pFrags);
   }
   if (pRecord->pFragsLengths)
   {
      free(pRecord->pFragsLengths);
   }
}

static void initTestEnv(void)
{
   if (initialized)
      return;

   t0 = sysTime();
   initialized = 1;
}

static void prepareStore(uint8_t fRecovery, uint8_t fHAEnabled)
{
   ism_field_t f;
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = (fRecovery ? 0 : 2);
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UByte;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_CACHEFLUSH_MODE, &f);
   f.type = VT_String;
   f.val.s = test_params.dir_name;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ROOT_PATH, &f);
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_PERSIST_PATH, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   f.type = VT_UInt;
   f.val.u = (1<<16);
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_BLOCK_SIZE, &f);
   f.type = VT_Boolean;
   f.val.i = !fRecovery;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_CLEAR, &f);
   f.type = VT_UInt;
   f.val.u = (1<<10);
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_RECOVERY_MEMSIZE_MB, &f);
   f.type = VT_UByte;
   f.val.i = test_params.mem_type;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MEMTYPE, &f);
   f.type = VT_ULong;
   f.val.l = 0x1080000000;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_NVRAM_OFFSET, &f);
   f.type = VT_Boolean;
   f.val.i = test_params.enable_persist;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ENABLEPERSIST, &f);

   if (fHAEnabled)
   {
      f.type = VT_Boolean;
      f.val.i = 1;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_ENABLEHA, &f);
      f.type = VT_String;
      f.val.s = test_params.ha_startup;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_STARTUPMODE, &f);
      f.type = VT_UByte;
      f.val.i = test_params.ha_protocol;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_REPLICATIONPROTOCOL, &f);
      f.type = VT_String;
      f.val.s = test_params.remote_disc_nic;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_REMOTEDISCOVERYNIC, &f);
      f.type = VT_String;
      f.val.s = test_params.local_rep_nic;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_LOCALREPLICATIONNIC, &f);
      f.type = VT_String;
      f.val.s = test_params.local_disc_nic;
      ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_LOCALDISCOVERYNIC, &f);
      if (test_params.ha_port > 0)
      {
         f.type = VT_UInt;
         f.val.u = test_params.ha_port;
         ism_common_setProperty(ism_common_getConfigProperties(), ismHA_CFG_PORT, &f);
      }
   }

   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
}

#if TEST_DELDISKGEN
#define TEMP_MSGS_COUNT      500
#define TEMP_MSG_SIZE    1048576
static void deleteDiskGeneration(ismStore_StreamHandle_t sh)
{
   ismStore_Record_t msgt[1];
   ismStore_Handle_t msgth[TEMP_MSGS_COUNT];
   int i;

   TRACE(1, "******** Adding %d temporary large messages to fill the in-mem generations ****\n", TEMP_MSGS_COUNT);
   char *pbufft = malloc(TEMP_MSG_SIZE);
   memset(pbufft, 0, TEMP_MSG_SIZE);
   initRecord(msgt);
   msgt->Type = ISM_STORE_RECTYPE_MSG;
   msgt->pFrags[0]  = pbufft;
   msgt->pFragsLengths[0] = msgt->DataLength = TEMP_MSG_SIZE;
   for (i=0; i < TEMP_MSGS_COUNT;)
   {
      if ((rc = ism_store_createRecord(sh, msgt, &msgth[i])) == ISMRC_OK)
      {
         rc = ism_store_commit(sh);
         i++;
      }
   }
   TRACE(5, "%d temporary messages have been added\n", TEMP_MSGS_COUNT);

   for (i=0; i < TEMP_MSGS_COUNT; i++)
   {
      if ((rc = ism_store_deleteRecord(sh, msgth[i])) != ISMRC_OK)
      {
         CU_ASSERT_FATAL(0);
      }
      rc = ism_store_commit(sh);
   }
   TRACE(5, "%d temporary messages have been deleted\n", TEMP_MSGS_COUNT);
   TRACE(1, "Press any key to continue\n\n");
   getchar();
}
#endif

static void *fillStoreThread(void *arg, void* _context, int _value)
{
   ThreadInfo *ti = (ThreadInfo *)arg;
   MsgEntry   *me;
   ismStore_StreamHandle_t sh;
   StoreRec Queue[1];
   ismStore_Record_t msg[1];
   ismStore_Reference_t ref[1];
   ismStore_Handle_t rh, dmh[100000];
   ismStore_StateObject_t state[1];
   char dummy_msg_buff[10];
   uint64_t oid;
   int dm_index = 0, rc;

   if ( (rc = ism_store_openStream(&sh, 1)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_openStream() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
      pthread_exit(NULL);
   }

#if TEST_DELDISKGEN
   deleteDiskGeneration(sh);
#endif

   if (ti->compactGen)
   {
      memset(dmh, 0, sizeof(dmh));
      memset(dummy_msg_buff, 0, sizeof(dummy_msg_buff));
      dm_index = 0;
   }

   memset(Queue->sr,0,sizeof(ismStore_Record_t)) ;
   if ( !initRecord(Queue->sr) )
   {
      TRACE(1, "%s_%d: failed to initRecord.\n", __FUNCTION__, __LINE__);
      CU_ASSERT_FATAL(0);
      pthread_exit(NULL);
   }
   Queue->sr->Type = (ti->objType == 0 ? ISM_STORE_RECTYPE_QUEUE : ISM_STORE_RECTYPE_CLIENT);
   Queue->sr->Attribute = RECORD_ATTRIBUTE;
   Queue->sr->State = RECORD_STATE;
   Queue->sr->pFragsLengths[0] = Queue->sr->DataLength = 54321;
   if ( !(Queue->sr->pFrags[0] = malloc(Queue->sr->DataLength)) )
   {
      TRACE(1, "%s_%d: failed to allocate memory. %s\n", __FUNCTION__, __LINE__, strerror(errno));
      CU_ASSERT_FATAL(0);
      pthread_exit(NULL);
   }

   if (ti->objType == 0)
      snprintf(Queue->sr->pFrags[0], Queue->sr->DataLength," Queue for messages of thread: %lu",ti->tid);
   else
      snprintf(Queue->sr->pFrags[0], Queue->sr->DataLength," Client for states of thread: %lu",ti->tid);

   if ( (rc = ism_store_createRecord(sh, Queue->sr, &Queue->handle)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_createRecord() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
      pthread_exit(NULL);
   }

   if ( (rc = ism_store_commit(sh)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_commit() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
      pthread_exit(NULL);
   }

   if (ti->objType == 0)
   {
      if ( (rc = ism_store_openReferenceContext(Queue->handle, NULL, &Queue->ctxt)) != ISMRC_OK )
      {
         TRACE(1, "%s_%d: ism_store_openReferenceContext() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
         CU_ASSERT_FATAL(0);
         pthread_exit(NULL);
      }
      memset(msg,0,sizeof(ismStore_Record_t));
      memset(ref,0,sizeof(ismStore_Reference_t));
      initRecord(msg);
      msg->Type = ISM_STORE_RECTYPE_MSG;
      oid = 0;
      TRACE(5, "%s_%lx_%d: Start reading queue and inserting msgs into store for Queue %s ; time= %f\n",
            __FUNCTION__, ti->tid, __LINE__, (char *)Queue->sr->pFrags[0], sysTime());
   }
   else
   {
      if ( (rc = ism_store_openStateContext(Queue->handle, &Queue->ctxt)) != ISMRC_OK )
      {
         TRACE(1, "%s_%d: ism_store_openStateContext() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
         CU_ASSERT_FATAL(0);
         pthread_exit(NULL);
      }
      memset(state,0,sizeof(ismStore_StateObject_t));
      TRACE(5, "%s_%lx_%d: Start reading queue and inserting states into store for Client %s ; time= %f\n",
            __FUNCTION__, ti->tid, __LINE__, (char *)Queue->sr->pFrags[0], sysTime());
   }

   for(;;)
   {
      pthread_mutex_lock(ti->lock);
      while ( !ti->first )
      {
        if ( !goOn )
           break ;
        pthread_cond_wait(ti->cond, ti->lock);
      }
      me = ti->first;
      if ( !me )
         break;
      if ( !(ti->first = me->next) )
         ti->last = NULL;
      ti->size--;
      pthread_mutex_unlock(ti->lock);

      if (ti->objType == 0)
      {
         msg->pFrags[0]  = me->msg_buf ;
         msg->pFragsLengths[0] = msg->DataLength = me->msg_len ;
         msg->Attribute = me->msg_off ;
         if ( (rc = ism_store_createRecord(sh, msg, &ref->hRefHandle)) != ISMRC_OK )
         {
            TRACE(1, "%s_%d: ism_store_createRecord() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
            CU_ASSERT_FATAL(0);
            pthread_exit(NULL);
         }
         ref->OrderId = oid++;
         if ( (rc = ism_store_createReference(sh, Queue->ctxt, ref, 0, &rh)) != ISMRC_OK )
         {
            TRACE(1, "%s_%d: ism_store_createReference() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
            CU_ASSERT_FATAL(0);
            pthread_exit(NULL);
         }

         // Create a dummy message which is used for compaction
         if (ti->compactGen)
         {
            if (dmh[dm_index])
            {
               if ( (rc = ism_store_deleteRecord(sh, dmh[dm_index])) != ISMRC_OK )
               {
                  TRACE(1, "%s_%d: ism_store_deleteRecord() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
                  CU_ASSERT_FATAL(0);
                  pthread_exit(NULL);
               }
               dmh[dm_index] = 0;
            }
            msg->pFrags[0]  = dummy_msg_buff;
            msg->pFragsLengths[0] = msg->DataLength = sizeof(dummy_msg_buff);
            msg->Attribute = 0x0;
            if ( (rc = ism_store_createRecord(sh, msg, &dmh[dm_index])) != ISMRC_OK )
            {
               TRACE(1, "%s_%d: ism_store_createRecord() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
               CU_ASSERT_FATAL(0);
               pthread_exit(NULL);
            }
            if (++dm_index >= 100000) { dm_index = 0; }
         }
      }
      else
      {
         memcpy((char *)&state->Value, me->msg_buf, sizeof(state->Value));
         if ( (rc = ism_store_createState(sh, Queue->ctxt, state, &rh)) != ISMRC_OK )
         {
            TRACE(1, "%s_%d: ism_store_createState() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
            CU_ASSERT_FATAL(0);
            pthread_exit(NULL);
         }
      }
      free(me->msg_buf);
      free(me);

      if ( (rc = ism_store_commit(sh)) != ISMRC_OK )
      {
         TRACE(1, "%s_%d: ism_store_commit() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
         CU_ASSERT_FATAL(0);
         pthread_exit(NULL);
      }
   }
   pthread_mutex_unlock(ti->lock);

   if (ti->compactGen)
   {
      sleep(1);
      for (dm_index=0; dm_index < 100000; dm_index++)
      {
         if (dmh[dm_index])
         {
            rc = ism_store_deleteRecord(sh, dmh[dm_index]);
            rc = ism_store_commit(sh);
            dmh[dm_index] = 0;
         }
      }
      sleep(1);
   }

   if ( (rc = ism_store_closeStream(sh)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_closeStream() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }

   freeRecord(Queue->sr);
   pthread_exit(NULL);
}

void fillStore(const char *filename, int nThreads, int objType, int compactGen)
{
   int fd, rc, i;
   size_t offset, ask, get;
   StoreRec Server[1];
   ismStore_StreamHandle_t sh;
   struct utsname sys_info;
   ThreadInfo *ti;
   MsgEntry *me;
   ismStore_StateObject_t *state;
   double seed=0e0;
   uint64_t oid;

   if ( (fd = open(filename, O_RDONLY, 0444)) < 0 )
   {
      TRACE(1, "%s_%d: failed to open input file %s. %s\n", __FUNCTION__, __LINE__, filename, strerror(errno));
      CU_ASSERT_FATAL(0);
   }

   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);
   memset(Server,0,sizeof(StoreRec)) ;
   if ( !initRecord(Server->sr) )
   {
      TRACE(1, "%s_%d: failed to initRecord.\n", __FUNCTION__, __LINE__);
      CU_ASSERT_FATAL(0);
   }

   Server->sr->Type = ISM_STORE_RECTYPE_SERVER;
   Server->sr->Attribute = RECORD_ATTRIBUTE;
   Server->sr->State = RECORD_STATE;
   Server->sr->pFragsLengths[0] = Server->sr->DataLength = 12345;
   if ( !(Server->sr->pFrags[0] = malloc(Server->sr->DataLength)) )
   {
      TRACE(1, "%s_%d: failed to allocate memory. %s\n", __FUNCTION__, __LINE__, strerror(errno));
      CU_ASSERT_FATAL(0);
   }

   if ( uname(&sys_info) != -1 )
   {
      snprintf(Server->sr->pFrags[0], Server->sr->pFragsLengths[0]," Niagra Incredible Server running on: sysname: %s , nodename: %s , release: %s , version: %s , machine: %s",
               sys_info.sysname,sys_info.nodename,sys_info.release,sys_info.version,sys_info.machine);
   }
   else
      strcpy(Server->sr->pFrags[0], "Niagra Incredible Server");

   if ( (rc = ism_store_createRecord(sh, Server->sr, &Server->handle)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_createRecord() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }

   if ( (rc = ism_store_commit(sh)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_commit() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }

   if ( !(ti = malloc(nThreads * sizeof(ThreadInfo))) )
   {
      TRACE(1, "%s_%d: failed to allocate memory. %s\n", __FUNCTION__, __LINE__, strerror(errno));
      CU_ASSERT_FATAL(0);
   }
   memset(ti, 0, nThreads * sizeof(ThreadInfo));

   goOn = 1;
   for ( i=0 ; i < nThreads ; i++ )
   {
      ti[i].objType = objType;
      ti[i].compactGen = compactGen;
      if ( (rc = pthread_mutex_init(ti[i].lock,NULL)) )
      {
         TRACE(1, "%s_%d: pthread_mutex_init() failed. %s\n", __FUNCTION__, __LINE__, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
      if ( (rc = pthread_cond_init(ti[i].cond,NULL)) )
      {
         TRACE(1, "%s_%d: pthread_cond_init() failed. %s\n", __FUNCTION__, __LINE__, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
      rc = ism_common_startThread(&ti[i].tid,fillStoreThread, &ti[i], NULL, 0,
              ISM_TUSAGE_NORMAL, 0, "fillStoreThread", "fillStoreThread");

      if ( rc )
      {
         TRACE(1, "%s_%d: pthread_create() failed. %s\n", __FUNCTION__, __LINE__, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
   }

   offset = 0;
   oid = 0;
   TRACE(5, "%s_%d: Start reading file and inserting msgs into store... time= %f\n", __FUNCTION__, __LINE__, sysTime());
   for(;;)
   {
      if ( !(me = malloc(sizeof(MsgEntry))) )
      {
         TRACE(1, "%s_%d: failed to allocate memory. %s\n", __FUNCTION__, __LINE__, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
      memset(me,0,sizeof(MsgEntry));
      if (objType == 0)
         ask = 16 + (size_t)(u01(&seed) * 16384);
      else
         ask = sizeof(state->Value);

      if ( !(me->msg_buf = malloc(ask)) )
      {
         TRACE(1, "%s_%d: failed to allocate memory. %s\n", __FUNCTION__, __LINE__, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
      get = read(fd, me->msg_buf, ask);
      if ( get < 0 )
      {
         TRACE(1, "%s_%d: failed to read %lu. %s\n", __FUNCTION__, __LINE__, ask, strerror(errno));
         CU_ASSERT_FATAL(0);
      }
      if ( get == 0 )
      {
         free(me->msg_buf);
         free(me);
         break;
      }
      me->msg_off = offset;
      me->msg_len = get;
      offset += get;
      i = (oid++) % nThreads;
      pthread_mutex_lock(ti[i].lock);
      while( ti[i].size > 512 )
      {
         pthread_mutex_unlock(ti[i].lock);
         poll(0,0,1);
         pthread_mutex_lock(ti[i].lock);
      }
      if ( ti[i].last )
         ti[i].last->next = me;
      else
         ti[i].first = me;

      ti[i].last = me;
      ti[i].size++ ;
      if (!((oid/nThreads)&255) )
         pthread_cond_signal(ti[i].cond) ;
      pthread_mutex_unlock(ti[i].lock) ;
   }
   close(fd);
   goOn = 0;
   for ( i=0 ; i < nThreads ; i++ )
   {
      pthread_mutex_lock(ti[i].lock);
      pthread_cond_signal(ti[i].cond);
      pthread_mutex_unlock(ti[i].lock);
   }
   for ( i=0 ; i < nThreads ; i++ )
      pthread_join(ti[i].tid, NULL);

   TRACE(5, "%s_%d: Finish inserting msgs into store: nmsgs= %lu, nbytes= %lu, time= %f\n",
         __FUNCTION__, __LINE__, oid, offset, sysTime());

   if ( (rc = ism_store_closeStream(sh)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_closeStream() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }

   if ( (rc = ism_store_term()) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_term() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }

   freeRecord(Server->sr);
   free(ti);
}

void readStore(const char *filename)
{
   int fd, rc;
   w2fCtx w2f[1];

   if ( (fd = open(filename, O_WRONLY|O_CREAT, 0644)) < 0 )
   {
      TRACE(1, "%s_%d: failed to open output file %s. %s\n", __FUNCTION__, __LINE__, filename, strerror(errno));
      CU_ASSERT_FATAL(0);
   }

   rc = ftruncate(fd, 0);
   TRACE(5, "%s_%d: Start reading store data and writing msgs to file... time= %f\n", __FUNCTION__, __LINE__, sysTime());

   memset(w2f,0,sizeof(w2fCtx));
   w2f->fd = fd;
   if ( (rc = ism_store_dumpCB(write2file, w2f)) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_dumpCB() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }
   close(fd);
   TRACE(5, "%s_%d: Finish reading store data and writing msgs to file: nmsgs= %lu, nstates= %lu, nbytes= %lu, time= %f\n",
         __FUNCTION__, __LINE__, w2f->nMsgs, w2f->nStates, w2f->nBytes, sysTime());

   if ( (rc = ism_store_recoveryCompleted()) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_recoveryCompleted() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }
   if ( (rc = ism_store_term()) != ISMRC_OK )
   {
      TRACE(1, "%s_%d: ism_store_term() failed. error code %d.\n", __FUNCTION__, __LINE__, rc);
      CU_ASSERT_FATAL(0);
   }
}

/**
 * Test store initialization
 */
void testInitialization(void)
{
   ism_field_t f;
   ismStore_Statistics_t statistics;
   ismStore_StreamHandle_t hStream;
   int32_t rc, i;

   initTestEnv();

   // Start before init
   ism_common_clearProperties(ism_common_getConfigProperties());
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_StoreNotAvailable);

   // Term before init
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_StoreNotAvailable);

   // RecoveryCompleted before init
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_StoreNotAvailable);

   // GetStataistics before init
   CU_ASSERT_FATAL(ism_store_getStatistics(&statistics) == ISMRC_StoreNotAvailable);

   // Drop before init
   CU_ASSERT_FATAL(ism_store_drop() == ISMRC_StoreNotAvailable);

   // OpenStream before init
   CU_ASSERT_FATAL(ism_store_openStream(&hStream, 1) == ISMRC_StoreNotAvailable);

   // Default values with ColdStart + TotalMemSize
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);

   // RecoveryCompleted before start
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_StoreNotAvailable);

   // OpenStream before start
   CU_ASSERT_FATAL(ism_store_openStream(&hStream, 1) == ISMRC_StoreNotAvailable);

   // Term just after init
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   // Full cycle: init + start + recovery + term
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   // Wrong MemoryType value
   f.type = VT_UByte;
   f.val.i = ismSTORE_MEMTYPE_NVRAM + 2;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MEMTYPE, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong CacheFlushMode
   f.type = VT_UByte;
   f.val.i = ismSTORE_CACHEFLUSH_ADR + 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_CACHEFLUSH_MODE, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong combination of RestoreFromDisk and ColdStart
   f.type = VT_UByte;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_RESTORE_DISK, &f);
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong InMemGensCount
   f.type = VT_UInt;
   f.val.i = 5;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_INMEM_GENS_COUNT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong memory size for management generation
   for (i=0; i < 120; i+=10)
   {
      f.type = VT_UInt;
      f.val.i = i;
      ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_MEM_PCT, &f);
      f.type = VT_UInt;
      f.val.i = 0;
      ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_STORETRANS_RSRV_OPS, &f);
      f.type = VT_UInt;
      f.val.u = test_params.store_memsize_mb;
      ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
      rc = ism_store_init();

      if (i < 5 || i > 95)
      {
         CU_ASSERT_FATAL(rc == ISMRC_BadPropertyValue);
      }
      else
      {
         CU_ASSERT_FATAL(rc == ISMRC_OK);

         CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
      }
      ism_common_clearProperties(ism_common_getConfigProperties());
   }

   // Wrong memory size of a generation
   f.type = VT_UInt;
   f.val.i = 100;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   f.type = VT_UInt;
   f.val.i = 90;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_MEM_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong management pool alert on/off percent
   f.type = VT_UInt;
   f.val.i = 101;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTON_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 30;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTON_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 101;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTOFF_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 30;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTOFF_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 70;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTON_PCT, &f);
   f.type = VT_UInt;
   f.val.i = 75;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTOFF_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong generation pool alert on percent
   f.type = VT_UInt;
   f.val.i = 101;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_GEN_ALERTON_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 30;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_GEN_ALERTON_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong disk space alert on/off percent
   f.type = VT_UInt;
   f.val.i = 101;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ALERTON_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 101;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ALERTOFF_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   f.type = VT_UInt;
   f.val.i = 70;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ALERTON_PCT, &f);
   f.type = VT_UInt;
   f.val.i = 75;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ALERTOFF_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong stream cache percent
   f.type = VT_Double;
   f.val.d = 30;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_STREAMS_CACHE_PCT, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());

   // Wrong Store.StoreTransactionRsrvOps
   f.type = VT_UInt;
   f.val.u = 16384;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   f.type = VT_UInt;
   f.val.i = 1e8;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_STORETRANS_RSRV_OPS, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_BadPropertyValue);
   ism_common_clearProperties(ism_common_getConfigProperties());
}

void testCreateMsgs1Thread(void)
{
   initTestEnv();

   prepareStore(0, 0);
   fillStore(test_params.large_filename, 1, 0, 0);

   prepareStore(1, 0);
   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCreateMsgs1ThreadHA(void)
{
   ismHA_View_t view;

   initTestEnv();

   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);
   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");
      fillStore(test_params.large_filename, 1, 0, 0);
      prepareStore(1, 0);
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
      getchar();
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testReadMsgs1ThreadHA(void)
{
   ismHA_View_t view;

   initTestEnv();

   prepareStore(1, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);
   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
      getchar();
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCreateMsgs4Threads(void)
{
   initTestEnv();

   prepareStore(0, 0);
   fillStore(test_params.large_filename, 4, 0, 0);

   prepareStore(1, 0);
   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCreateMsgs4ThreadsHA(void)
{
   ismHA_View_t view;

   initTestEnv();

   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);

   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");
      fillStore(test_params.large_filename, 4, 0, 0);
      prepareStore(1, 0);
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
      getchar();
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCreateStates(void)
{
   initTestEnv();

   prepareStore(0, 0);
   fillStore(test_params.small_filename, 1, 1, 0);

   prepareStore(1, 0);
   readStore(test_params.output_filename);
   //diff(test_params.small_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCreateStatesHA(void)
{
   ismHA_View_t view;

   initTestEnv();

   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);

   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");
      fillStore(test_params.small_filename, 1, 1, 0);
      prepareStore(1, 0);
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   readStore(test_params.output_filename);
   //diff(test_params.small_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testDeleteStates(void)
{
   ism_field_t f;
   ismStore_StreamHandle_t sh;
   ismStore_Record_t record;
   ismStore_Handle_t ownerHandle, handle, *stateHandle;
   ismStore_IteratorHandle pIterator;
   ismStore_StateObject_t state;
   void *stateCtxt;
   int32_t rc;
   int i, numStates=4096;

   stateHandle = (ismStore_Handle_t *)malloc(numStates * sizeof(ismStore_Handle_t));
   CU_ASSERT_FATAL(stateHandle != NULL);
   memset(stateHandle, 0, numStates * sizeof(ismStore_Handle_t));

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

   memset(&record, 0, sizeof(record));
   initRecord(&record);
   record.Type = ISM_STORE_RECTYPE_CLIENT;
   record.DataLength = 5;
   record.pFrags[0] = "States";

   CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &ownerHandle) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_openStateContext(ownerHandle, &stateCtxt) == ISMRC_OK);

   // Add states object
   memset(&state, 0, sizeof(state));
   for (i=0; i < numStates; i++)
   {
      state.Value = i;
      CU_ASSERT_FATAL(ism_store_createState(sh, stateCtxt, &state, &stateHandle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   // Delete all the states with odd values
   for (i=1; i < numStates; i += 2)
   {
      CU_ASSERT_FATAL(ism_store_deleteState(sh, stateCtxt, stateHandle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   CU_ASSERT_FATAL(ism_store_closeStateContext(stateCtxt) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeStream(sh) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   // Restart the store and verify that only the states with even values exist
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);

   for (i=0, pIterator = NULL;;i++)
   {
      rc = ism_store_getNextStateForOwner(&pIterator, ownerHandle, &handle, &state);
      if (rc == ISMRC_StoreNoMoreEntries)
      {
         break;
      }
      CU_ASSERT_FATAL(rc == ISMRC_OK);
      CU_ASSERT_FATAL(state.Value < numStates);
      CU_ASSERT_FATAL(state.Value % 2 == 0);
      CU_ASSERT_FATAL(stateHandle[state.Value] == handle);
   }
   CU_ASSERT_FATAL(i == (numStates / 2));

   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStateContext(ownerHandle, &stateCtxt) == ISMRC_OK);

   // Delete all the states one by one
   for (i=0; i < numStates; i += 2)
   {
      CU_ASSERT_FATAL(ism_store_deleteState(sh, stateCtxt, stateHandle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   // Restart the store and verify that there are no state objects
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);

   pIterator = NULL;
   CU_ASSERT_FATAL(ism_store_getNextStateForOwner(&pIterator, ownerHandle, &handle, &state) == ISMRC_StoreNoMoreEntries);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   free(stateHandle);
}

void testStatistics(void)
{
   ism_field_t f;
   ismStore_Statistics_t statistics;

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_getStatistics(&statistics) == ISMRC_OK);
   CU_ASSERT(statistics.ActiveGenId == 2);
   CU_ASSERT(statistics.GenerationsCount == 3);
   CU_ASSERT(statistics.StreamsCount == 0);

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

void testRefsStatistics(void)
{
   ism_field_t f;
   ismStore_StreamHandle_t sh;
   ismStore_Handle_t handle, refHandle;
   ismStore_ReferenceStatistics_t refStats;
   ismStore_Record_t queue;
   ismStore_Reference_t reference;
   uint64_t minActiveOrderId;
   void *queueCtxt;
   int i;

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

   memset(&queue, 0, sizeof(queue));
   queue.Type = ISM_STORE_RECTYPE_QUEUE;
   CU_ASSERT_FATAL(ism_store_createRecord(sh, &queue, &handle) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

   // Test #1: New owner - the statistics should be empty
   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, &refStats, &queueCtxt) == ISMRC_OK);

   CU_ASSERT(refStats.MinimumActiveOrderId == 0);
   CU_ASSERT(refStats.LowestGenId == 0);
   CU_ASSERT(refStats.HighestGenId == 0);
   CU_ASSERT(refStats.HighestOrderId == 0);

   // Test #2: Adding 10 references (0-9)
   memset(&reference, 0, sizeof(reference));
   for (i=0; i < 10; i++)
   {
      reference.OrderId = i;
      reference.hRefHandle = 0x1;

      CU_ASSERT_FATAL(ism_store_createReference(sh, queueCtxt, &reference, 0, &refHandle) == ISMRC_OK);
   }

   CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeReferenceContext(queueCtxt) == ISMRC_OK);

   // Trying to open a reference context with pRefStats=NULL (should be OK)
   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, NULL, &queueCtxt) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeReferenceContext(queueCtxt) == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, &refStats, &queueCtxt) == ISMRC_OK);

   CU_ASSERT(refStats.MinimumActiveOrderId == 0);
   CU_ASSERT(refStats.LowestGenId == 2);
   CU_ASSERT(refStats.HighestGenId == 2);
   CU_ASSERT(refStats.HighestOrderId > 0);

   // Test #3: Adding reference with OrderId 120 and set the MinimumActiveOrderId to 5
   memset(&reference, 0, sizeof(reference));
   reference.OrderId = 120;
   reference.hRefHandle = 0x1;
   minActiveOrderId = 5;
   ism_store_createReference(sh, queueCtxt, &reference, minActiveOrderId, &refHandle);

   CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeReferenceContext(queueCtxt) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, &refStats, &queueCtxt) == ISMRC_OK);

   CU_ASSERT(refStats.MinimumActiveOrderId <= minActiveOrderId);
   CU_ASSERT(refStats.LowestGenId == 2);
   CU_ASSERT(refStats.HighestGenId == 2);
   CU_ASSERT(refStats.HighestOrderId >= minActiveOrderId);

   // Test #4: Restarting the store
   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);

   // Make sure that the statistics is valid after recovery
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, &refStats, &queueCtxt) == ISMRC_OK);

   CU_ASSERT(refStats.MinimumActiveOrderId <= minActiveOrderId);
   CU_ASSERT(refStats.LowestGenId == 2);
   CU_ASSERT(refStats.HighestGenId == 2);
   CU_ASSERT(refStats.HighestOrderId >= minActiveOrderId);

   // Test #5: Remove all the reference chunks from the store by setting MinimumActiveOrderId to 5000
   minActiveOrderId = 5000;
   CU_ASSERT_FATAL(ism_store_pruneReferences(queueCtxt, minActiveOrderId) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeReferenceContext(queueCtxt) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, &refStats, &queueCtxt) == ISMRC_OK);

   CU_ASSERT(refStats.MinimumActiveOrderId == minActiveOrderId);
   CU_ASSERT(refStats.LowestGenId == 2);
   CU_ASSERT(refStats.HighestGenId == 2);
   CU_ASSERT(refStats.HighestOrderId == minActiveOrderId);

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

#define PRUNE_NTHREADS          5
#define PRUNE_NQUEUES        1000
#define PRUNE_NREFS     100000000
static void *addRefThread(void *arg)
{
   AddRefsThreadInfo *ti = (AddRefsThreadInfo *)arg;
   ismStore_StreamHandle_t sh;
   ismStore_Record_t queue;
   ismStore_Handle_t handle, refHandle;
   ismStore_Reference_t reference;
   uint64_t minActiveOrderId;
   void **queueCtxt;
   int i, j;

   CU_ASSERT_FATAL((queueCtxt = (void **)malloc(PRUNE_NQUEUES * sizeof(void *))) != NULL);
   memset(queueCtxt, 0, PRUNE_NQUEUES * sizeof(void *));
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);
   for (i=0; i < PRUNE_NQUEUES; i++)
   {
      memset(&queue, 0, sizeof(queue));
      queue.Type = ISM_STORE_RECTYPE_QUEUE;
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &queue, &handle) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_openReferenceContext(handle, NULL, &queueCtxt[i]) == ISMRC_OK);
   }

   TRACE(5, "Thread %lu has been initialized\n", ti->tid);
   pthread_mutex_lock(ti->lock);
   ti->fInit = 1;
   pthread_cond_wait(ti->cond, ti->lock);
   pthread_mutex_unlock(ti->lock);
   TRACE(5, "Thread %lu has been started\n", ti->tid);

   memset(&reference, 0, sizeof(reference));
   for (j=0; j < ti->nRefs; j++)
   {
      for (i=0; i < PRUNE_NQUEUES; i++)
      {
         reference.OrderId = j;
         reference.hRefHandle = 0x1;
         minActiveOrderId = (j / test_params.oid_interval) * test_params.oid_interval;

         CU_ASSERT_FATAL(ism_store_createReference(sh, queueCtxt[i], &reference, minActiveOrderId, &refHandle) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }
   }

   CU_ASSERT_FATAL(ism_store_closeStream(sh) == ISMRC_OK);
   free(queueCtxt);
   TRACE(5, "Thread %lu has been completed\n", ti->tid);
   pthread_exit(NULL);
}

void testPruneReferencesPerf(void)
{
   AddRefsThreadInfo ti[PRUNE_NTHREADS];
   pthread_mutex_t mutex;
   pthread_cond_t cond;
   double time0, time1;
   int i;

   CU_ASSERT_FATAL(pthread_mutex_init(&mutex, NULL) == 0);
   CU_ASSERT_FATAL(pthread_cond_init(&cond, NULL) == 0);
   initTestEnv();

   prepareStore(0,0);
   if (test_params.oid_interval == 0) { test_params.oid_interval = 1; }

   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);

   for (i=0; i < PRUNE_NTHREADS; i++)
   {
      memset(&ti[i], 0, sizeof(AddRefsThreadInfo));
      ti[i].lock = &mutex;
      ti[i].cond = &cond;
      ti[i].nRefs = PRUNE_NREFS / (PRUNE_NTHREADS * PRUNE_NQUEUES);
      CU_ASSERT_FATAL(pthread_create(&ti[i].tid, NULL, addRefThread, &ti[i]) >= 0);
   }

   pthread_mutex_lock(&mutex);
   for (i=0; i < PRUNE_NTHREADS; i++)
   {
      while (ti[i].fInit == 0)
      {
         pthread_mutex_unlock(&mutex);
         sleep(1);
         pthread_mutex_lock(&mutex);
      }
   }
   pthread_cond_broadcast(&cond);
   pthread_mutex_unlock(&mutex);

   time0 = sysTime();
   for (i=0 ; i < PRUNE_NTHREADS; i++)
   {
      pthread_join(ti[i].tid, NULL);
   }
   time1 = sysTime();

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
   TRACE(1, "PruneReferencesPerf: MinActiveOrderIdInterval = %u, Time = %f\n", test_params.oid_interval, (time1 - time0));
}

void testReservation(void)
{
   ism_field_t f;
   ismStore_StreamHandle_t sh;
   ismStore_Reservation_t rsrv;
   ismStore_Record_t record;
   ismStore_Handle_t handle;

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = test_params.store_memsize_mb;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

   // Null reservation
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, NULL) == ISMRC_NullArgument);

   // Empty reservation
   memset(&rsrv, 0, sizeof(rsrv));
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);

   // Small reservation
   memset(&rsrv, 0, sizeof(rsrv));
   rsrv.DataLength = 1024;
   rsrv.RecordsCount = 1;
   rsrv.RefsCount = 1;
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);

   // Large reservation
   memset(&rsrv, 0, sizeof(rsrv));
   rsrv.DataLength = 65535;
   rsrv.RecordsCount = 10;
   rsrv.RefsCount = 1000;
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_OK);

   // Multiple reservation
   memset(&rsrv, 0, sizeof(rsrv));
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_StoreTransActive);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);

   // Too large reservation
   memset(&rsrv, 0, sizeof(rsrv));
   rsrv.DataLength = 65535;
   rsrv.RecordsCount = 1e7;
   rsrv.RefsCount = 1e7;
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_StoreAllocateError);

   // Reservation during active store-transaction
   memset(&record, 0, sizeof(record));
   record.Type = ISM_STORE_RECTYPE_SERVER;
   CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &handle) == ISMRC_OK);

   memset(&rsrv, 0, sizeof(rsrv));
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_StoreTransActive);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);

   memset(&rsrv, 0, sizeof(rsrv));
   CU_ASSERT_FATAL(ism_store_reserveStreamResources(sh, &rsrv) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_closeStream(sh) == ISMRC_OK);

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

int testOnEvent(ismStore_EventType_t eventType, void *pContext)
{
   TRACE(5, "Store event %d\n", eventType);
   return ISMRC_OK;
}

void testAlerts(void)
{
   ism_field_t f;
   ismStore_StreamHandle_t sh;
   ismStore_Handle_t handle[50000];
   ismStore_Record_t record;
   int i;

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = 50;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   f.type = VT_UInt;
   f.val.u = 256;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_SMALL_GRANULE, &f);
   f.type = VT_UInt;
   f.val.u = 4096;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_GRANULE_SIZE, &f);
   f.type = VT_UInt;
   f.val.u = 95;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_OWNER_LIMIT_PCT, &f);
   f.type = VT_UInt;
   f.val.i = 60;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTON_PCT, &f);
   f.type = VT_UInt;
   f.val.i = 51;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MGMT_ALERTOFF_PCT, &f);

   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_registerEventCallback(testOnEvent, NULL) == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

   // Adding records into the management small pool
   memset(handle, 0, sizeof(handle));
   for (i=0; i < 50000; i++)
   {
      memset(&record, 0, sizeof(record));
      record.Type = ISM_STORE_RECTYPE_QUEUE;
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &handle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   sleep(1);
   // Deleting the records
   for (i=0; i < 50000; i++)
   {
      CU_ASSERT_FATAL(ism_store_deleteRecord(sh, handle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   // Adding records into the management large pool
   memset(handle, 0, sizeof(handle));
   for (i=0; i < 1500; i++)
   {
      memset(&record, 0, sizeof(record));
      record.Type = ISM_STORE_RECTYPE_SERVER;
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &handle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   sleep(1);
   // Deleting the records
   for (i=0; i < 1500; i++)
   {
      CU_ASSERT_FATAL(ism_store_deleteRecord(sh, handle[i]) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

void testOwnerLimit(void)
{
   ism_field_t f;
   ismStore_StreamHandle_t sh;
   ismStore_Handle_t handle;
   ismStore_Record_t record;
   ismStore_Statistics_t statistics;
   int ownerLimit, numQ, i;

   initTestEnv();

   // Default values with ColdStart + TotalMemSize
   ism_common_clearProperties(ism_common_getConfigProperties());
   f.type = VT_UInt;
   f.val.i = 1;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_COLD_START, &f);
   f.type = VT_UInt;
   f.val.u = 50;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
   f.type = VT_UInt;
   f.val.u = 50;
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_OWNER_LIMIT_PCT, &f);

   CU_ASSERT_FATAL(ism_store_init() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_start() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

   // GetStataistics read the TotalOwnerRecordsLimit
   CU_ASSERT_FATAL(ism_store_getStatistics(&statistics) == ISMRC_OK);
   //ownerLimit = statistics.OwnerCount.TotalOwnerRecordsLimit;
   ownerLimit = (int)(statistics.MemStats.Pool1RecordsLimitBytes/statistics.MemStats.RecordSize);

   // Adding records into the management small pool
   memset(&handle, 0, sizeof(handle));
   for (i=0; i < ownerLimit; i++)
   {
      memset(&record, 0, sizeof(record));
      record.Type = ISM_STORE_RECTYPE_QUEUE;
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &handle) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
   }

   // GetStataistics before OwnerLimit
   CU_ASSERT_FATAL(ism_store_getStatistics(&statistics) == ISMRC_OK);
   //CU_ASSERT_FATAL(statistics.OwnerCount.NumOfQueueRecords == ownerLimit);
   numQ = (int)(statistics.MemStats.QueuesBytes / statistics.MemStats.RecordSize);
   CU_ASSERT_FATAL(numQ == ownerLimit);

   memset(&record, 0, sizeof(record));
   record.Type = ISM_STORE_RECTYPE_QUEUE;
   CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &handle) == ISMRC_StoreOwnerLimit);
   CU_ASSERT_FATAL(ism_store_rollback(sh) == ISMRC_OK);

   // GetStataistics after rollback
   CU_ASSERT_FATAL(ism_store_getStatistics(&statistics) == ISMRC_OK);
   //CU_ASSERT_FATAL(statistics.OwnerCount.NumOfQueueRecords == ownerLimit);
   numQ = (int)(statistics.MemStats.QueuesBytes / statistics.MemStats.RecordSize);
   CU_ASSERT_FATAL(numQ == ownerLimit);

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

void testCompactGeneration(void)
{
   initTestEnv();

   prepareStore(0, 0);
   fillStore(test_params.large_filename, 1, 0, 1);

   prepareStore(1, 0);
   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testCompactGenerationHA(void)
{
   ismHA_View_t view;

   initTestEnv();

   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);
   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");
      fillStore(test_params.large_filename, 1, 0, 1);
      prepareStore(1, 0);
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
      getchar();
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   readStore(test_params.output_filename);
   diff(test_params.large_filename, test_params.output_filename);
   unlink(test_params.output_filename);
}

void testSendAdminMsgsHA(void)
{
   ismHA_View_t view;
   ismHA_AdminMessage_t msg;
   char dataBuffer[1024], resBuffer[ismHA_MAX_ADMIN_RESPONSE_LENGTH];
   int rc, i;

   initTestEnv();

   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);

   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");

      memset(&msg, 0, sizeof(msg));
      memset(dataBuffer, 0, sizeof(dataBuffer));
      memset(resBuffer, 0, sizeof(resBuffer));
      msg.pData = dataBuffer;
      msg.pResBuffer = resBuffer;
      msg.ResBufferLength = ismHA_MAX_ADMIN_RESPONSE_LENGTH;
      for (i=0; i < 10; i++)
      {
         msg.DataLength = sprintf(dataBuffer, "This is the data buffer of the Admin message %d which is used by Idan Zach for testing", i);
         TRACE(5, "Admin message %d (DataLength %u) is being sent to the Standby node\n", i, msg.DataLength);
         rc = ism_ha_store_send_admin_msg(&msg);
         CU_ASSERT(rc == ISMRC_OK);
         TRACE(5, "Admin message %d has been sent successfully to the Standby node. ResponseLength %u, ResponseBuffer %s, rc %d\n", i, msg.ResLength, msg.pResBuffer, rc);
      }

      TRACE(5, "Transfer the Admin file %s to the Standby disk\n", test_params.admin_filename);
      rc = ism_ha_store_transfer_file(test_params.dir_name, test_params.admin_filename);
      CU_ASSERT(rc == ISMRC_OK);
      TRACE(5, "The admin file %s has been transferred to the Standby disk. rc %d\n", test_params.admin_filename, rc);
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}

void testOrphanChunksHA(void)
{
   ismHA_View_t view;
   ismStore_StreamHandle_t sh;
   ismStore_Record_t record;
   ismStore_Reference_t ref;
   ismStore_StateObject_t state;
   ismStore_Handle_t ownerHandle1, ownerHandle2, msgHandle, handle;
   void *stateCtxt, *refCtxt;
   orphanCtxt oCtxt;
   int i;

   initTestEnv();

   test_params.store_memsize_mb = 60;
   prepareStore(0, 1);
   CU_ASSERT_FATAL(ism_ha_store_get_view(&view) == ISMRC_OK);
   TRACE(5, "High-Availaibility View: NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u\n",
         view.NewRole, view.OldRole, view.ActiveNodesCount, view.SyncNodesCount);

   if (view.NewRole == ISM_HA_ROLE_PRIMARY)
   {
      TRACE(5, "The node has been selected to act as the Primary node\n");

      CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_openStream(&sh, 1) == ISMRC_OK);

      memset(&record, 0, sizeof(record));
      initRecord(&record);
      record.Type = ISM_STORE_RECTYPE_CLIENT;
      record.pFragsLengths[0] = record.DataLength = 25;
      record.pFrags[0] = "Client with orphan chunks";

      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &ownerHandle1) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

      CU_ASSERT_FATAL(ism_store_openReferenceContext(ownerHandle1, NULL, &refCtxt) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_openStateContext(ownerHandle1, &stateCtxt) == ISMRC_OK);

      // Add a message
      record.Type = ISM_STORE_RECTYPE_MSG;
      record.pFragsLengths[0] = record.DataLength = 7;
      record.pFrags[0] = "Message";
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &msgHandle) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

      // Add 1024 references
      memset(&ref, 0, sizeof(ref));
      for (i=0; i < 1024; i++)
      {
         ref.OrderId = i;
         ref.hRefHandle = msgHandle;
         ref.Value = i;
         CU_ASSERT_FATAL(ism_store_createReference(sh, refCtxt, &ref, 0, &handle) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }

      // Add 1024 states object
      memset(&state, 0, sizeof(state));
      for (i=0; i < 1024; i++)
      {
         state.Value = i;
         CU_ASSERT_FATAL(ism_store_createState(sh, stateCtxt, &state, &handle) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }

      // Delete the owner
      CU_ASSERT_FATAL(ism_store_closeReferenceContext(refCtxt) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_closeStateContext(stateCtxt) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_deleteRecord(sh, ownerHandle1) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

      // Allocate a new owner with the same handle
      for (i=0; i < 100000; i++)
      {
         record.Type = ISM_STORE_RECTYPE_CLIENT;
         record.pFragsLengths[0] = record.DataLength = 10;
         record.pFrags[0] = "New Client";
         CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &ownerHandle2) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
         if (ownerHandle1 == ownerHandle2) { break; }
         CU_ASSERT_FATAL(ism_store_deleteRecord(sh, ownerHandle2) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }

      CU_ASSERT_FATAL(ownerHandle1 == ownerHandle2);

      CU_ASSERT_FATAL(ism_store_openReferenceContext(ownerHandle2, NULL, &refCtxt) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_openStateContext(ownerHandle2, &stateCtxt) == ISMRC_OK);

      // Add a message
      record.Type = ISM_STORE_RECTYPE_MSG;
      record.pFragsLengths[0] = record.DataLength = 7;
      record.pFrags[0] = "Message";
      CU_ASSERT_FATAL(ism_store_createRecord(sh, &record, &msgHandle) == ISMRC_OK);
      CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);

      // Add 10 references to the new owner
      memset(&ref, 0, sizeof(ref));
      for (i=0; i < 10; i++)
      {
         ref.OrderId = 20 + i;
         ref.hRefHandle = msgHandle;
         ref.Value = 3000 + i;
         CU_ASSERT_FATAL(ism_store_createReference(sh, refCtxt, &ref, 0, &handle) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }

      // Add 10 states object to the new owner
      memset(&state, 0, sizeof(state));
      for (i=0; i < 10; i++)
      {
         state.Value = 3000 + i;
         CU_ASSERT_FATAL(ism_store_createState(sh, stateCtxt, &state, &handle) == ISMRC_OK);
         CU_ASSERT_FATAL(ism_store_commit(sh) == ISMRC_OK);
      }
   }
   else if (view.NewRole == ISM_HA_ROLE_STANDBY)
   {
      TRACE(5, "The node has been selected to act as the Standby node\n");
      // Wait until the node becomes the Primary
      while (view.NewRole != ISM_HA_ROLE_PRIMARY)
      {
         sleep(1);
         ism_ha_store_get_view(&view);
      }
      TRACE(5, "The node is now acting as the Primary node\n");
      getchar();
      memset(&oCtxt, 0, sizeof(oCtxt));
      CU_ASSERT_FATAL(ism_store_dumpCB(validateOrphan, &oCtxt) == ISMRC_OK);
      CU_ASSERT_FATAL(oCtxt.refsCount == 10);
      CU_ASSERT_FATAL(oCtxt.statesCount == 10);
      CU_ASSERT_FATAL(ism_store_recoveryCompleted() == ISMRC_OK);
   }
   else
   {
      TRACE(1, "The High-Availability role (%u) is not valid\n", view.NewRole);
      CU_ASSERT_FATAL(0);
   }

   CU_ASSERT_FATAL(ism_store_term() == ISMRC_OK);
}
