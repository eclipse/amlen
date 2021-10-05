const static char sccsid[] = "%Z% %W% %I% %E% %U%";
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

/*
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

/* The functions in this module provide a thin layer above MQ to load either the
   server or client bindings library and to store pointers to the MQ API entry
   points in the structure ETM_dynamic_MQ_entries. The functions in this module are
   then called which call the real MQ API function cia the function pointers.
   The MQ DLL must be explicitly loaded before any MQ calls are made via a call to
   cphMQSplitterCheckMQLoaded(), specifying on the call whether the
   client or server library is required.

   The code in this module is based on an old cat II SupportPac, which provided a
   Lotus Script interface to MQ, written by Stephen Todd.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(AMQ_UNIX)
   #include <sys/types.h>

   #include <malloc.h>
   #include <errno.h>

   #include <dlfcn.h>      /* dynamic load stuff */
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * Now include MQ header files                                        *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <cmqc.h>

#include "cphMQSplitter.h"


/*********************************************************************/
/*   Global variables                                                */
/*********************************************************************/

mq_epList ETM_dynamic_MQ_entries;      /* the list of real entry points */

unsigned long ETM_DLL_found = 0;    /* 0 means dll not found*/
unsigned long ETM_Tried = 0;        /* 0 means haven't tried establish*/


/*********************************************************************/
/*   The Code                                                        */
/*********************************************************************/



/**********************************************************************************/
/*                                                                                */
/* We have all the standard entry points as copied from cmqc.h - these            */
/* directly pass the incoming call to the dynamic entry address obtained during   */
/* initialization.                                                                */
/*                                                                                */
/**********************************************************************************/



 /*********************************************************************/
 /*  MQBACK Function -- Back Out Changes                              */
 /*********************************************************************/

 void MQENTRY MQBACK (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqback)
  {
    (ETM_dynamic_MQ_entries.mqback)(Hconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQCLOSE Function -- Close Object                                 */
 /*********************************************************************/

 void MQENTRY MQCLOSE (
   MQHCONN  Hconn,      /* Connection handle */
   PMQHOBJ  pHobj,      /* Object handle */
   MQLONG   Options,    /* Options that control the action of MQCLOSE */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqclose )
  {
    (ETM_dynamic_MQ_entries.mqclose)(Hconn,pHobj,Options,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQCMIT Function -- Commit Changes                                */
 /*********************************************************************/

 void MQENTRY MQCMIT (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqcmit )
  {
    (ETM_dynamic_MQ_entries.mqcmit)(Hconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQCONN Function -- Connect Queue Manager                         */
 /*********************************************************************/

 void MQENTRY MQCONN (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqconn )
  {
     (ETM_dynamic_MQ_entries.mqconn)(pName,pHconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQCONNX Function -- Connect Queue Manager                         */
 /*  Note: Extra function here compared to rest of calls - this allows*/
 /*        retry aftre 6000 without bring notes down                  */
 /*********************************************************************/

 void MQENTRY MQCONNX (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQCNO    pConnectOpts, /* Connect options */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqconnx )
  {
     (ETM_dynamic_MQ_entries.mqconnx)(pName,pConnectOpts,pHconn,pCompCode,pReason);

  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQDISC Function -- Disconnect Queue Manager                      */
 /*********************************************************************/

 void MQENTRY MQDISC (
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqdisc)
  {
    (ETM_dynamic_MQ_entries.mqdisc)(pHconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQGET Function -- Get Message                                    */
 /*********************************************************************/

 void MQENTRY MQGET (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pGetMsgOpts,   /* Options that control the action of
                              MQGET */
   MQLONG   BufferLength,  /* Length in bytes of the Buffer area */
   PMQVOID  pBuffer,       /* Area to contain the message data */
   PMQLONG  pDataLength,   /* Length of the message */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqget)
  {
    (ETM_dynamic_MQ_entries.mqget)(Hconn,Hobj,pMsgDesc,pGetMsgOpts,BufferLength,
                                 pBuffer,pDataLength,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQINQ Function -- Inquire Object Attributes                      */
 /*********************************************************************/

 void MQENTRY MQINQ (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason)         /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqinq)
  {
    (ETM_dynamic_MQ_entries.mqinq)(Hconn,Hobj,SelectorCount,pSelectors,IntAttrCount,
                                 pIntAttrs,CharAttrLength,pCharAttrs,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQOPEN Function -- Open Object                                   */
 /*********************************************************************/

 void MQENTRY MQOPEN (
   MQHCONN  Hconn,      /* Connection handle */
   PMQVOID  pObjDesc,   /* Object descriptor */
   MQLONG   Options,    /* Options that control the action of MQOPEN */
   PMQHOBJ  pHobj,      /* Object handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  /*  ------- Changes for defect 22444 end here -----  */

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqopen)
  {
    (ETM_dynamic_MQ_entries.mqopen)(Hconn,pObjDesc,Options,pHobj,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQPUT Function -- Put Message                                    */
 /*********************************************************************/

 void MQENTRY MQPUT (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqput)
  {
    (ETM_dynamic_MQ_entries.mqput)(Hconn,Hobj,pMsgDesc,pPutMsgOpts,BufferLength,
                                 pBuffer,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQPUT1 Function -- Put One Message                               */
 /*********************************************************************/

 void MQENTRY MQPUT1 (
   MQHCONN  Hconn,         /* Connection handle */
   PMQVOID  pObjDesc,      /* Object descriptor */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT1 */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqput1)
  {
    (ETM_dynamic_MQ_entries.mqput1)(Hconn,pObjDesc,pMsgDesc,pPutMsgOpts,BufferLength,
                                 pBuffer,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
//    *pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQSET Function -- Set Object Attributes                          */
 /*********************************************************************/

 void MQENTRY MQSET (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason)         /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqset)
  {
    (ETM_dynamic_MQ_entries.mqset)(Hconn,Hobj,SelectorCount,pSelectors,IntAttrCount,
                                 pIntAttrs,CharAttrLength,pCharAttrs,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}




 /****************************************************************/
 /*  MQBUFMH Function -- Buffer To Message Handle                */
 /****************************************************************/


 void MQENTRY MQBUFMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pBufMsgHOpts,  /* I: Options that control the action of
                               MQBUFMH */
   PMQVOID   pMsgDesc,      /* IO: Message descriptor */
   MQLONG    BufferLength,  /* IL: Length in bytes of the Buffer area */
   PMQVOID   pBuffer,       /* OB: Area to contain the message buffer */
   PMQLONG   pDataLength,   /* O: Length of the output buffer */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqbufmh)
  {
    (ETM_dynamic_MQ_entries.mqbufmh)(Hconn,Hmsg,pBufMsgHOpts,pMsgDesc,BufferLength,pBuffer,pDataLength,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}

 /****************************************************************/
 /*  MQCRTMH Function -- Create Message Handle                   */
 /****************************************************************/


 void MQENTRY MQCRTMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQVOID   pCrtMsgHOpts,  /* IO: Options that control the action of
                               MQCRTMH */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqcrtmh)
  {
    (ETM_dynamic_MQ_entries.mqcrtmh)(Hconn,pCrtMsgHOpts,pHmsg,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /****************************************************************/
 /*  MQDLTMH Function -- Delete Message Handle                   */
 /****************************************************************/


 void MQENTRY MQDLTMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQVOID   pDltMsgHOpts,  /* I: Options that control the action of
                               MQDLTMH */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqdltmh)
  {
    (ETM_dynamic_MQ_entries.mqdltmh)(Hconn,pHmsg,pDltMsgHOpts,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}



 /*********************************************************************/
 /*  MQSUBSCRIBE Function -- Subscribe to a given destination         */
 /*********************************************************************/

 void MQENTRY MQSUB (

   MQHCONN  Hconn,      /* I: Connection handle */
   PMQVOID  pSubDesc,   /* IO: Subscription descriptor */
   PMQHOBJ  pHobj,      /* IO: Object handle for queue */
   PMQHOBJ  pHsub,      /* O: Subscription object handle */
   PMQLONG  pCompCode,  /* OC: Completion code */
   PMQLONG  pReason)    /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqsub)
  {
    (ETM_dynamic_MQ_entries.mqsub)(Hconn,pSubDesc,pHobj,pHsub,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}

/*
** Method: cphMQSplitterCheckMQLoaded
**
** This method is called (by cphConnectionParseArgs) to check whether a previous attempt has been made to load the client or server
** MQM dll/shared library. If no attempt has been made, cphMQSplitterLoadMQ is called which will then try to open it and store the
** entry points for the MQ API functions in the ETM_dynamic_MQ_entries structure.
**
** Input Parameters: pLog - pointer to the CPH_LOG structure
**                   isClient - flag to denote whether the client or server library should be loaded. If set to zero, the server
**                   library is loaded. If set to any other value the client library is loaded.
**
** Returns: 0 on successful execution, -1 otherwise
**
*/
int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient) {

  /* If this is the first time this method has been called then attempt to load the client/server DLL/shared lib */
  if (ETM_Tried==0) {
   ETM_DLL_found = cphMQSplitterLoadMQ(pLog, &ETM_dynamic_MQ_entries, isClient);
   ETM_Tried = 1;
  }

  if (!ETM_DLL_found)  return(-1);
  return(0);
 }


#if defined(WIN32)

/*
** Method: cphMQSplitterLoadMQ
**
** Attempts to load the client or server MQM DLL/shared library. LoadLibraryA is used on Windows to open the DLL and GetProcAddress
** to retrieve the individual entry points. On Linux, dlopen is used to open the shared library and dlsym to retrieve the entry
** points.
**
** Input Parameters: pLog - pointer to the CPH_LOG structure
**                   isClient - flag to denote whether the client or server library should be loaded. If set to zero, the server
**                   library is loaded. If set to any other value the client library is loaded.
**
** Returns: TRUE if the DLL loaded successfully and FALSE otherwise
**
*/
long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient)
{
  EPTYPE    pLibrary=0L;
  long      rc = FALSE;         /* note that FALSE = FAILURE */
  char      Failure[128]="";
  char      DLL_name[80];
  char      msg[200];


  /*~~~~~~~~~~~~~~~~~~~~~~~*/
  #if defined(ITS_WINDOWS)
   DWORD    error=0;
   #ifdef WIN16
   unsigned int semrc=0;
   unsigned int suppress_error_window = SEM_NOOPENFILEERRORBOX;
   #endif
  #endif
  /*~~~~~~~~~~~~~~~~~~~~~~~*/

  /*~~~~~~~~~~~~~~~~~~~~~~~*/
  #if defined(AMQ_OS2)
   SHORT    error;
  #endif
  /*~~~~~~~~~~~~~~~~~~~~~~~*/


  /* first we looksee if user is telling us to load a specific library */
  if (0 == isClient)
      strcpy(DLL_name, MQLOCAL_NAME);
  else
      strcpy(DLL_name, MQREMOTE_NAME);

 #if defined (ITS_WINDOWS)

  pLibrary = LoadLibraryA(DLL_name);

#endif


  if (pLibrary)   /* did we find a DLL? */
  {

    /* tell the world what we found */
    sprintf(msg, "DLL %s loaded ok", DLL_name);
    cphLogPrintLn(pLog, LOGINFO, msg);

   /**********************************************************/
   /* For the present we will assume that having found a dll */
   /* it will have the things we need !!!!                   */
   /* If it hasn't then code in gmqdyn0a.c will raise error  */
   /**********************************************************/

   #if defined(ITS_WINDOWS)
    ep->mqconn   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQCONN");
    //ep->mqconn   = (pMQCONN) GetProcAddress(pLibrary,"MQCONN");
    ep->mqconnx  = (MQFUNCPTR) GetProcAddress(pLibrary,"MQCONNX");
    ep->mqput    = (MQFUNCPTR) GetProcAddress(pLibrary,"MQPUT");
    ep->mqget    = (MQFUNCPTR) GetProcAddress(pLibrary,"MQGET");
    ep->mqopen   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQOPEN");
    ep->mqclose  = (MQFUNCPTR) GetProcAddress(pLibrary,"MQCLOSE");
    ep->mqdisc   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQDISC");
    ep->mqset    = (MQFUNCPTR) GetProcAddress(pLibrary,"MQSET");
    ep->mqput1   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQPUT1");
    ep->mqback   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQBACK");
    ep->mqcmit   = (MQFUNCPTR) GetProcAddress(pLibrary,"MQCMIT");
    ep->mqinq    = (MQFUNCPTR) GetProcAddress(pLibrary,"MQINQ");
#ifndef CPH_WMQV6
    ep->mqsub    = (MQFUNCPTR) GetProcAddress(pLibrary,"MQSUB");
    ep->mqbufmh  = (MQFUNCPTR) GetProcAddress(pLibrary,"MQBUFMH");
    ep->mqcrtmh  = (MQFUNCPTR) GetProcAddress(pLibrary,"MQCRTMH");
    ep->mqdltmh  = (MQFUNCPTR) GetProcAddress(pLibrary,"MQDLTMH");
#endif
  #else
    error = DosQueryProcAddr(pLibrary,0L,"MQCONN",&(ep->mqconn));
    error = DosQueryProcAddr(pLibrary,0L,"MQPUT",&(ep->mqput));
    error = DosQueryProcAddr(pLibrary,0L,"MQGET",&(ep->mqget));
    error = DosQueryProcAddr(pLibrary,0L,"MQOPEN",&(ep->mqopen));
    error = DosQueryProcAddr(pLibrary,0L,"MQCLOSE",&(ep->mqclose));
    error = DosQueryProcAddr(pLibrary,0L,"MQDISC",&(ep->mqdisc));
    error = DosQueryProcAddr(pLibrary,0L,"MQSET",&(ep->mqset));
    error = DosQueryProcAddr(pLibrary,0L,"MQPUT1",&(ep->mqput1));
    error = DosQueryProcAddr(pLibrary,0L,"MQBACK",&(ep->mqback));
    error = DosQueryProcAddr(pLibrary,0L,"MQCMIT",&(ep->mqcmit));
    error = DosQueryProcAddr(pLibrary,0L,"MQINQ",&(ep->mqinq));
#ifndef CPH_WMQV6
    error = DosQueryProcAddr(pLibrary,0L,"MQSUB",&(ep->mqsub));
    error = DosQueryProcAddr(pLibrary,0L,"MQBUFMH",&(ep->mqbufmh));
    error = DosQueryProcAddr(pLibrary,0L,"MQCRTMH",&ep->mqcrtmh));
    error = DosQueryProcAddr(pLibrary,0L,"MQDLTMH",&ep->mqdltmh));
#endif
   #endif

   rc = TRUE;     /* TRUE means it worked - dll was found */
  }
  else
  {
    /* couldn't find a dll !!! */

    #if defined(ITS_WINDOWS)
     #ifdef AMQ_NT    /* Win16 doesn't have GetLastError */
     error = GetLastError();
     #endif
    #endif

    sprintf(msg, "Cannot find lib - %s", DLL_name);
    cphLogPrintLn(pLog, LOGERROR, msg);

    rc = FALSE;
  }

return rc;
}

#elif defined(AMQ_UNIX)


long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient)
{
  EPTYPE    pLibrary=0L;
  long      rc = 0;  /* 0 means that we failed */
  char      DLL_name[80];
  char     *ErrorText;     /* Text of error from dlxxx()           */
  char buff[200];

   if (0 == isClient)
      strcpy(DLL_name, MQLOCAL_NAME);
   else
      strcpy(DLL_name, MQREMOTE_NAME);

    pLibrary = (void *)dlopen(DLL_name,RTLD_NOW
#if defined(_AIX)
                             | RTLD_MEMBER
#endif
                             );
    if (!pLibrary) ErrorText = dlerror();

 if ( pLibrary == NULL )
 {
    /* lets try and tell the world what happened */
     sprintf(buff, "On trying to load %s ........", DLL_name);
     cphLogPrintLn(pLog, LOGERROR, buff);
     cphLogPrintLn(pLog, LOGERROR, ErrorText);
 }

  if (pLibrary)   /* did we find a DLL? */
  {

   {  /* tell the world what we found */

    sprintf(buff, "Shared library %s loaded ok", DLL_name);
    cphLogPrintLn(pLog, LOGINFO, buff);
   }

   /**********************************************************/
   /* For the present we will assume that having found a dll */
   /* it will have the things we need !!!!                   */
   /**********************************************************/

       ep->mqconn   = (MQFUNCPTR)  dlsym(pLibrary, "MQCONN");
       ep->mqconnx  = (MQFUNCPTR)  dlsym(pLibrary, "MQCONNX");
       ep->mqdisc   = (MQFUNCPTR)  dlsym(pLibrary, "MQDISC");
       ep->mqopen   = (MQFUNCPTR)  dlsym(pLibrary, "MQOPEN");
       ep->mqclose  = (MQFUNCPTR)  dlsym(pLibrary, "MQCLOSE");
       ep->mqget    = (MQFUNCPTR)  dlsym(pLibrary, "MQGET");
       ep->mqput    = (MQFUNCPTR)  dlsym(pLibrary, "MQPUT");
       ep->mqset    = (MQFUNCPTR)  dlsym(pLibrary, "MQSET");
       ep->mqput1   = (MQFUNCPTR)  dlsym(pLibrary, "MQPUT1");
       ep->mqback   = (MQFUNCPTR)  dlsym(pLibrary, "MQBACK");
       ep->mqcmit   = (MQFUNCPTR)  dlsym(pLibrary, "MQCMIT");
       ep->mqinq    = (MQFUNCPTR)  dlsym(pLibrary, "MQINQ");
#ifndef CPH_WMQV6
       ep->mqsub    = (MQFUNCPTR)  dlsym(pLibrary,"MQSUB");
       ep->mqbufmh  = (MQFUNCPTR)  dlsym(pLibrary,"MQBUFMH");
       ep->mqcrtmh  = (MQFUNCPTR)  dlsym(pLibrary,"MQCRTMH");
       ep->mqdltmh  = (MQFUNCPTR ) dlsym(pLibrary,"MQDLTMH");
#endif
       rc = 1; /* to show it worked */
  }
  else
  {
    /* couldn't find a dll !!! */
    sprintf(buff, "Cannot find lib - %s!", DLL_name);
    cphLogPrintLn(pLog, LOGERROR, buff);
  }

  return rc;
}

#endif
