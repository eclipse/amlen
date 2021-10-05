/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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

#ifndef XAPI 
#define XAPI
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/vfs.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <math.h>
#include "store.h"
#include "storeInternal.h"
#include "storeMemory.h"

#include "stubFuncs.c"

static const char *recName(ismStore_RecordType_t type)
{
  if ( type == ISM_STORE_RECTYPE_SERVER ) return "Server" ; 
  if ( type == ISM_STORE_RECTYPE_CLIENT ) return "Client" ; 
  if ( type == ISM_STORE_RECTYPE_QUEUE  ) return "Queue" ; 
  if ( type == ISM_STORE_RECTYPE_TOPIC  ) return "Topic" ; 
  if ( type == ISM_STORE_RECTYPE_SUBSC  ) return "Subscriber" ; 
  if ( type == ISM_STORE_RECTYPE_TRANS  ) return "Transaction" ; 
  if ( type == ISM_STORE_RECTYPE_BMGR   ) return "Bridge queue manager" ;
  if ( type == ISM_STORE_RECTYPE_REMSRV ) return "Remote Server" ;
  if ( type == ISM_STORE_RECTYPE_MSG    ) return "Message" ; 
  if ( type == ISM_STORE_RECTYPE_PROP   ) return "Property" ; 
  if ( type == ISM_STORE_RECTYPE_CPROP  ) return "Client Property" ; 
  if ( type == ISM_STORE_RECTYPE_QPROP  ) return "Queue Property" ; 
  if ( type == ISM_STORE_RECTYPE_TPROP  ) return "Topic Property" ; 
  if ( type == ISM_STORE_RECTYPE_SPROP  ) return "Subscriber Property" ; 
  if ( type == ISM_STORE_RECTYPE_BXR    ) return "Bridge XID" ; 
  if ( type == ISM_STORE_RECTYPE_RPROP  ) return "Remote Server Property" ; 
  return "Unknown" ; 
}


static char *print_record(ismStore_Record_t *pR, char *str, int len)
{
  int i,k=0, j=0 ; 
  char *p;
  memset(str,0,len);
  k += snprintf(str+k,len-k,"Type=%x, Attr=%lx, State=%lx, dLen=%u, ",
       pR->Type, pR->Attribute, pR->State, pR->DataLength);
  for ( i=0, p=pR->pFrags[0] ; i<pR->DataLength && k<len ; i++, p++ )
  {
    if ( isprint(*p) )
    {
      if ( j )
      {
        str[k++] = '|' ; 
        j = 0 ; 
      }
      str[k++] = *p ; 
    }
    else
    {
      unsigned char c;
      if ( !j )
      {
        str[k++] = '|' ; 
        j = 1 ; 
      }
      c = (*p)>>4 ;  
      if ( c < 10 )
        str[k++] = '0' + c ; 
      else
        str[k++] = 'a' + c-10 ; 
      c = (*p)&0xf ; 
      if ( c < 10 )
        str[k++] = '0' + c ; 
      else
        str[k++] = 'a' + c-10 ; 
    }
  }
  return str ; 
}

static size_t dump_count[ISM_STORE_DUMP_PROP+1];

static int dump2count(ismStore_DumpData_t *pDumpData, void *pContext)
{
  ismStore_DumpData_t *dt = pDumpData ; 
  FILE *fp = (FILE *)pContext ; 
  if (!(dt && fp) ) return ISMRC_NullArgument ;

  if ( dt->dataType >= ISM_STORE_DUMP_GENID && 
       dt->dataType <= ISM_STORE_DUMP_PROP ) 
  {
    __sync_fetch_and_add(dump_count+dt->dataType, (size_t)1) ; 
    dt->readRefHandle = 1 ; 
  }
  else
  {
    fprintf(fp,"Unrecognized DUMP dataType: %d.\n",dt->dataType) ; 
  }
  return ISMRC_OK ;
}

static int dump2file(ismStore_DumpData_t *pDumpData, void *pContext)
{
  char str[4096] ; 
  ismStore_DumpData_t *dt = pDumpData ; 
  FILE *fp = (FILE *)pContext ; 
  if (!(dt && fp) ) return ISMRC_NullArgument ;

  switch (dt->dataType)
  {
   case ISM_STORE_DUMP_GENID :
    fprintf(fp," Data for generation %hu\n",dt->genId);
    break ; 
   case ISM_STORE_DUMP_REC4TYPE :
   {
    ismStore_ReferenceStatistics_t rs[1] ; 
    if ( ismSTORE_IS_SPLITITEM(dt->recType) && ism_store_getReferenceStatistics(dt->handle,rs) == ISMRC_OK )
      fprintf(fp,"\tHandle %p: %s , min_act_oid %lu, max_oid %lu ; definition: %s\n",(void *)dt->handle, recName(dt->recType),
              rs->MinimumActiveOrderId,rs->HighestOrderId,print_record(dt->pRecord,str,sizeof(str)));
    else
      fprintf(fp,"\tHandle %p: %s definition: %s\n",(void *)dt->handle, recName(dt->recType),print_record(dt->pRecord,str,sizeof(str)));
    break ; 
   }
   case ISM_STORE_DUMP_REF4OWNER :
    fprintf(fp,"\t\tReference for %s record at handle %p in generation %hu: ",recName(dt->recType),(void *)dt->owner, dt->genId); 
    fprintf(fp," Reference %p, referring to %p, orderId %lu, value %u, state %d.\n", (void *)dt->handle, (void *)dt->pReference->hRefHandle, dt->pReference->OrderId, dt->pReference->Value, (int)dt->pReference->State);
    dt->readRefHandle = ( ismSTORE_EXTRACT_GENID(dt->pReference->hRefHandle) != ismSTORE_MGMT_GEN_ID ) ; 
    break ; 
   case ISM_STORE_DUMP_STATE4OWNER :
    fprintf(fp,"\t\tState for %s record at handle %p in generation %hu: ",recName(dt->recType),(void *)dt->owner, dt->genId); 
    fprintf(fp," State %p, value %u.\n", (void *)dt->handle, dt->pState->Value);
    break ; 
   case ISM_STORE_DUMP_REF :
    dt->readRefHandle = 1 ; 
    break ; 
   case ISM_STORE_DUMP_MSG :
    fprintf(fp,"\t\t\t\tMessage - length %u, attr %lu, state %lu.\n", dt->pRecord->DataLength,dt->pRecord->Attribute,dt->pRecord->State); 
    break ; 
   case ISM_STORE_DUMP_PROP :
    fprintf(fp,"\t\tProperty at handle %p, for %s owner %p in generation %hu: %s\n",(void *)dt->handle,recName(dt->recType),(void *)dt->owner, dt->genId,print_record(dt->pRecord,str,sizeof(str))); 
    break ; 
   default :
    fprintf(fp,"Unrecognized DUMP dataType: %d.\n",dt->dataType) ; 
    break ; 
  }
  return ISMRC_OK ;
}

int main(int argc, char **argv)
{
  int i,rc, tl=0, raw=0, h=0, shm=0, bkup=0, rstr=0, mem=0, prst=0, cnt=0, v12=0;
  uint64_t ssMB=(1<<14) ; 
  char *rp="/store/com.ibm.ism" ; 
  char *pp="/store/persist" ; 
  char *of ; 
  char *tf="stdout";

  for ( i=2 ; i<argc ; i++ )
  {
    if (!strcasecmp(argv[i],"-rp") )
    {
       rp = argv[++i] ; 
    }
    else if (!strcasecmp(argv[i],"-pp") )
    {
       pp = argv[++i] ; 
    }
    else if (!strcasecmp(argv[i],"-shm") )
    {
       shm = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-mem") )
    {
       mem = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-v12") )
    {
       v12 = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-ss") )
    {
       ssMB = atoi(argv[++i]) ; 
    }
    else if (!strcasecmp(argv[i],"-raw") )
    {
       raw = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-cnt") )
    {
       cnt = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-prst") )
    {
       prst = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-bkup") )
    {
       bkup = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-rstr") )
    {
       rstr = 1 ; 
    }
    else if (!strcasecmp(argv[i],"-tl") )
    {
       tl = atoi(argv[++i]) ; 
    }
    else if (!strcasecmp(argv[i],"-tf") )
    {
       tf = argv[++i] ; 
    }
    else if (!strcasecmp(argv[i],"-h") )
    {
       h = 1 ; 
    }
  } 

  if ( argc < 2 || h )
  {
    printf("Usage:  %s <outputFile> [options]\n",argv[0]);
    printf("\t Where options is any of the following:\n");
    printf("\t\t -rp Path  : Root path of the store (defaule: %s)\n",rp);
    printf("\t\t -pp Path  : Persist path of the store (defaule: %s)\n",pp);
    printf("\t\t -shm      : Use SHM store memory type (default: nvDIMM)\n");
    printf("\t\t -mem      : Use MEM store memory type (default: nvDIMM)\n");
    printf("\t\t -v12      : Use RecoverFromV12 (default: 0)\n");
    printf("\t\t -ss Size  : Total in memory Store Size in MB (default: %lu)\n",ssMB);
    printf("\t\t -raw      : Use Raw read mode (default: logic read mode)\n");
    printf("\t\t -cnt      : Use Count read mode (default: logic read mode)\n");
    printf("\t\t -prst     : Enable Disk Persistence (default Do Not)\n");
    printf("\t\t -bkup     : Perform BackupToDisk of in mem gens (default Do Not)\n");
    printf("\t\t -rstr     : Perform RestoreFromDisk of in mem gens (default Do Not)\n");
    printf("\t\t -tl Level : Trace level (default: %d)\n",tl);
    printf("\t\t -tf File  : Trace file (default: %s)\n",tf);
    printf("\t\t -h        : Print this help\n");
    exit(-1);
  }
  of = argv[1] ; 


  ism_common_initUtil();
  ism_common_setTraceLevel(tl);
  ism_common_setTraceOptions("time,thread,where");
  if (strcasecmp(tf,"stdout"))
    ism_common_setTraceFile(tf,0);
  if ( getenv("TraceMax") )
  {
    size_t s = strtol(getenv("TraceMax"),NULL,0);
    ism_common_setTraceMax(s);
  }

  {
    ism_field_t f;
    ism_prop_t * props = ism_common_getConfigProperties() ;
    f.type = VT_Boolean;
    f.val.i = 0 ;
    ism_common_setProperty( props, ismSTORE_CFG_COLD_START, &f);
    f.type = VT_String ;
    f.val.s = rp;
    ism_common_setProperty( props, ismSTORE_CFG_DISK_ROOT_PATH, &f);
    f.type = VT_String ;
    f.val.s = pp;
    ism_common_setProperty( props, ismSTORE_CFG_DISK_PERSIST_PATH, &f);
    f.type = VT_UInt ;
    f.val.u = (1<<16);
    ism_common_setProperty( props, ismSTORE_CFG_DISK_BLOCK_SIZE, &f);
    f.type = VT_Boolean;
    f.val.i = 0;
    ism_common_setProperty( props, ismSTORE_CFG_DISK_CLEAR, &f);
    f.type = VT_UInt ;
    f.val.u = (1<<14);
    ism_common_setProperty( props, ismSTORE_CFG_RECOVERY_MEMSIZE_MB, &f);
    f.type = VT_UInt ;
    f.val.u = mem ? ismSTORE_MEMTYPE_MEM : (shm ? ismSTORE_MEMTYPE_SHM : ismSTORE_MEMTYPE_NVRAM) ;
    ism_common_setProperty( props, ismSTORE_CFG_MEMTYPE, &f);
    f.type = VT_ULong;
    f.val.l = (mem|shm) ? 0 : 0x10800 ; 
    f.val.l <<= 20 ;  
    ism_common_setProperty( props, ismSTORE_CFG_NVRAM_OFFSET, &f);
    f.type = VT_UInt ;
    f.val.u = ssMB;
    ism_common_setProperty( props, ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
    f.type = VT_UInt ;
    f.val.u = 0 ; 
    ism_common_setProperty( props, ismSTORE_CFG_CACHEFLUSH_MODE, &f);
    f.type = VT_Boolean;
    f.val.i = prst ? 1 : 0 ;
    ism_common_setProperty( props, ismSTORE_CFG_DISK_ENABLEPERSIST, &f);
    f.type = VT_Boolean;
    f.val.i = bkup ? 1 : 0 ;
    ism_common_setProperty( props, ismSTORE_CFG_BACKUP_DISK, &f);
    f.type = VT_Boolean;
    f.val.i = rstr ? 1 : 0 ;
    ism_common_setProperty( props, ismSTORE_CFG_RESTORE_DISK, &f);
    f.type = VT_Boolean;
    f.val.i = v12 ;
    ism_common_setProperty( props, ismSTORE_CFG_RECOVER_FROM_V12, &f);
    if ( getenv("PersistRecoveryFlags") )
    {
      f.type = VT_UInt ;
      f.val.u = strtol(getenv("PersistRecoveryFlags"),NULL,0);
      ism_common_setProperty( props, ismSTORE_CFG_PERSISTRECOVERYFLAGS, &f);
    }
  }


  if ( (rc = ism_store_init()) != ISMRC_OK )
  {
    printf("_%s_%d: ism_store_init() failed.  rc= %d\n",argv[0],__LINE__,rc);
    exit(-1);
  }
  if ( (rc = ism_store_start()) != ISMRC_OK )
  {
    printf("_%s_%d: ism_store_start() failed.  rc= %d\n",argv[0],__LINE__,rc);
    exit(-1);
  }

  if ( raw )
    rc = ism_store_dumpRaw(of) ; 
  else
  {
    FILE *fp ;
    int32_t need_close=0 ; 
  
    if (!of || !strcmp(of,"stdout") )
      fp = stdout ; 
    else
    if (!strcmp(of,"stderr") )
      fp = stderr ; 
    else
    if ( !(fp = fopen(of,"w")) ) 
      fp = stdout ; 
    else
      need_close = 1 ; 
  
    if ( cnt )
    {
      rc = ism_store_dumpCB(dump2count, fp) ;
      fprintf(fp,"Count for ISM_STORE_DUMP_GENID      : %lu\n",dump_count[ISM_STORE_DUMP_GENID      ]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_REC4TYPE   : %lu\n",dump_count[ISM_STORE_DUMP_REC4TYPE   ]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_REF4OWNER  : %lu\n",dump_count[ISM_STORE_DUMP_REF4OWNER  ]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_STATE4OWNER: %lu\n",dump_count[ISM_STORE_DUMP_STATE4OWNER]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_REF        : %lu\n",dump_count[ISM_STORE_DUMP_REF        ]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_MSG        : %lu\n",dump_count[ISM_STORE_DUMP_MSG        ]) ; 
      fprintf(fp,"Count for ISM_STORE_DUMP_PROP       : %lu\n",dump_count[ISM_STORE_DUMP_PROP       ]) ; 
    }
    else
    {
      rc = ism_store_dumpCB(dump2file, fp) ;
    }
    if ( need_close ) fclose(fp) ; 
    //rc = ism_store_dump   (of) ; 
  }
  if ( rc )
  {
    if ( raw )
      printf("_%s_%d: ism_store_dumpRaw() failed.  rc= %d\n",argv[0],__LINE__,rc);
    else
      printf("_%s_%d: ism_store_dump() failed.  rc= %d\n",argv[0],__LINE__,rc);
    exit(-1);
  }

  if ( (rc = ism_store_term()) != ISMRC_OK )
  {
    printf("_%s_%d: ism_store_term() failed.  rc= %d\n",argv[0],__LINE__,rc);
    exit(-1);
  }
}

