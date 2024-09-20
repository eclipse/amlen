/*
 * Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
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

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include "llmLogUtil.h"


#include <locale.h>
#define STRDUP(x) strdup((x))
#define STRCPY(dest, src, destSize) strcpy((dest),(src))
#define STRCAT(dst,dstSize,src) strcat(dst,src)
#define SPRINTF1(buff,buffLength,format,value) sprintf(buff,format,value)
#if defined(SOLARIS) || defined(AIX) || defined(HPUX)
#include <alloca.h>
#endif

#include "thread.c"
#define MAX_NUM_OF_FILTERS  64
/*
This structure defines an entry in the list of instance configurations
*/
typedef struct CFG_LIST_ENTRY{
    char*                       instanceName;
    llmInstanceLogCfg_t*        filters[64];
    int                         numOfFilters;
    struct CFG_LIST_ENTRY*      prev;
    struct CFG_LIST_ENTRY*      next;
}llmInstanceLogCfgListEntry_t;

typedef void    (*la_task_proc_t)(void * param);
/*
This structure defines an entry in the logging tasks list
*/
typedef struct LogAnnouncerTask_t{
    la_task_proc_t  task_proc;  //Task processing method
    void*   taskParam;                  //Task parameter
    struct LogAnnouncerTask_t*  next;
}LogAnnouncerTask_t;

typedef     char    TRACE_LINE[1024];
#define MAX_REPL_DATA_SIZE  512
#define MAX_NUM_OF_REPL_DATA 16

typedef char replacementData_t[MAX_REPL_DATA_SIZE];

typedef struct LOG_CALLBACK {
    llm_on_log_event_t      onLogEvent;
    void*                   user;
    uint64_t                timestamp;
} LogCallback_t;
/*
    Trace buffer structure
*/
typedef struct TraceBuffer_t {
    TCHandle                tcHandle;
    LogCallback_t           callbacks[MAX_NUM_OF_FILTERS];
    int                     numOfCallbacks;
    int                     componentId;
    COMPONENT_NAME          componentName;
    int                     msgKey;
    int                     logLevel;
    TRACE_LINE              traceLine;
    size_t                  traceLineLength;
    int                     replDataSize;
    replacementData_t       replData[MAX_NUM_OF_REPL_DATA];
    uintptr_t               threadId;
    int64_t                 timestamp;
    const char*             instanceName;
    struct TraceBuffer_t*   next;
    struct TraceBuffer_t*   last;
}TraceBuffer_t;

/*
    This structure contains the current information on log announcer thread
*/
typedef struct LogAnnoncerThreadInfo_t{
    fmd_threadh_t           threadId;
    fmd_cond_t              condVar;
    int                     isRunning;
    TBHandle                traceBufferToReturn;/* Handle to the trace buffer that has to be returned to the pool.
                                                   We use it to avoid additional lock acquiring.
                                                */
    LogAnnouncerTask_t*     taskListHead;
    LogAnnouncerTask_t*     taskListTail;
    int                     taskListSize;
}LogAnnoncerThreadInfo_t;


/*
    This structure contains an information on registered trace components
*/
typedef struct TraceComponent_t{
    char*                                   instanceName;
    int                                     componentId;
    COMPONENT_NAME                          componentName;
    const llmInstanceLogCfgListEntry_t*     instanceCfg;
    int                                     inUseCounter;
    struct TraceComponent_t*                prev;
    struct TraceComponent_t*                next;
}TraceComponent_t;




static int  createLogAnnouncerThread(void);
static void stopLogAnnouncerThread(int isFini);
static int invokeLog(TBHandle tbHandle);
static int finalizeInstance(void);


//List of the instance configurations
static llmInstanceLogCfgListEntry_t* llmInstanceLogCfgListHead = NULL;
//List of registered components
static TraceComponent_t*    TCListHead = NULL;
static LogAnnouncerTask_t*  tasksPool = NULL;
static int                  tasksPoolSize = 0;
#define MAX_POOL_SIZE 1024
#define MAX_NUM_OF_TB 10240
static TBHandle tbPool = NULL;
static int      tbPoolSize = 0;
static int      numOfTraceBuffersAllocated = 0;

/*
 * Lock for the Log message structure
 */
static fmd_lock_t                   llmLogUtilLock;

//Current log announcer thread. Created when first trace component is registered. Stopped when there are no more registered components
static LogAnnoncerThreadInfo_t*     llmLogAnnouncerThread;

//This is an information on a space currently allocated for log events
typedef struct LOG_EVENT_DATA{
    llmLogEvent_t               logEvent;
    char*                       formatedMessage;
    size_t                      formatedMessageSize;
    void**                      eventParams;
    int                         eventParamsSize;
}logEventData_t;

static logEventData_t           logEventData;

#include <sys/time.h>

static uint64_t getCurrentTimeMillis(void* user){
    struct      timeval tv;
    uint64_t    result = 1000;
    gettimeofday(&tv, NULL);
    result *= tv.tv_sec;
    result += tv.tv_usec/1000;
    return result;
}

static void freePools(void){
    //Free trace buffer and tasks pools
    while(tbPool != NULL){
        TBHandle next = tbPool->next;
        free(tbPool);
        tbPool = next;
    }
    tbPoolSize = 0;
    while(tasksPool != NULL){
        LogAnnouncerTask_t* next = tasksPool->next;
        free(tasksPool);
        tasksPool = next;
    }
    tasksPoolSize = 0;
    numOfTraceBuffersAllocated = 0;
}

/*
 * Initialization of the llmLogUtil component
 */
void llmLogUtilsInit(void) {
    //Init global lock
    fmd_lockinit(&llmLogUtilLock);
    fmd_lock(&llmLogUtilLock);

    llmLogAnnouncerThread = NULL;
    //Allocate space for log events
    logEventData.formatedMessage = (char*)malloc(1024*sizeof(char));
    logEventData.formatedMessageSize = 1024;
    logEventData.eventParams = (void**) malloc (128*sizeof(void*));
    logEventData.eventParamsSize = 128;
    logEventData.logEvent.event_params = logEventData.eventParams;

    fmd_unlock(&llmLogUtilLock);
}
void llmLogUtilsTerm(void) {
    fmd_lock(&llmLogUtilLock);
    stopLogAnnouncerThread(1);//Ensure that log announcer thread is stopped
    freePools();
    while(TCListHead != NULL){//Verify that list of registered components is empty
        TCHandle next = TCListHead->next;
        free(TCListHead);
        TCListHead = next;
    }
    while(llmInstanceLogCfgListHead != NULL){//Verify that list of instance configurations is empty
        llmInstanceLogCfgListEntry_t* next = llmInstanceLogCfgListHead->next;
        free(llmInstanceLogCfgListHead);
        llmInstanceLogCfgListHead = next;
    }
    free(logEventData.formatedMessage);
    free(logEventData.eventParams);
    fmd_unlock(&llmLogUtilLock);
    fmd_lockclose(&llmLogUtilLock);
}


static void updateAllTraceComponents(const char* instanceName, const llmInstanceLogCfgListEntry_t *config);

//Remove components with log level set to default (-1) from the structure
static void removeDefaultComponents(llmInstanceLogCfg_t *config){
    int i,j;
    llmComponentLogCfg_t tmpArray[MAX_NUMBER_OF_COMPONENTS];
    if(config->numOfComponentCfgs == 0) return;
    memset(tmpArray,0,sizeof(tmpArray));
    //Copy all non default components to temporary array
    for(i = 0, j = 0; i < config->numOfComponentCfgs; i++){
        if(config->componentCfgs[i].allowedLogLevel == -1) continue;
        //Log level os not default
        tmpArray[j].allowedLogLevel = config->componentCfgs[i].allowedLogLevel;
        tmpArray[j].componentId = config->componentCfgs[i].componentId;
        j++;
    }

    if(config->numOfComponentCfgs != j){//If some components were removed
        memcpy(config->componentCfgs,tmpArray,sizeof(tmpArray));
        config->numOfComponentCfgs = j;
    }
}

//Clone the instance logging configuration
static llmInstanceLogCfg_t* cloneInstanceLogCfg(const llmInstanceLogCfg_t* cfg){
    llmInstanceLogCfg_t* result = (llmInstanceLogCfg_t*)malloc(sizeof(llmInstanceLogCfg_t));
    if(result == NULL){
        return NULL;
    }
    memcpy(result,cfg,sizeof(llmInstanceLogCfg_t));
    result->instanceName = strdup(cfg->instanceName);
    /*Set the get_time callback*/
    if(cfg->get_time!=NULL){
            result->get_time = cfg->get_time;
            result->get_time_param = cfg->get_time_param;
    }else{
            result->get_time = getCurrentTimeMillis;
            result->get_time_param=NULL;
    }
    removeDefaultComponents(result);
    return result;
}

static void freeInstanceLogCfg(llmInstanceLogCfg_t* cfg){
    if(cfg == NULL) return;
    free(cfg->instanceName);
    free(cfg);
}

//Find instance logging configuration in the list
static llmInstanceLogCfgListEntry_t* findLogCfgListEntry(const char* instanceName){
    llmInstanceLogCfgListEntry_t* curr = llmInstanceLogCfgListHead;
    while(curr != NULL){
        if(strcmp(instanceName,curr->instanceName) == 0){
            break;
        }
        curr = curr->next;
    }
    return curr;
}

static int updateExistingConfig(llmInstanceLogCfg_t** pCurrCfg, const llmInstanceLogCfg_t* newCfg, int* errorCode){
    finalizeInstance();
    freeInstanceLogCfg(*pCurrCfg);
    *pCurrCfg = cloneInstanceLogCfg(newCfg);
    if(*pCurrCfg == NULL) {
        *errorCode = NOT_ENOUGH_MEMORY;
        return 1;
    }
    return 0;
}

static int setFilter(llmInstanceLogCfgListEntry_t* listEntry, const llmInstanceLogCfg_t* newCfg, int updateExisting, int* errorCode){
    int i;
    for(i = 0; i < listEntry->numOfFilters; i++){
        if(listEntry->filters[i]->filterID == newCfg->filterID){
            if(!updateExisting){
                *errorCode = INSTANCE_ALREADY_EXISTS;
                return 1;
            }
            return updateExistingConfig(&(listEntry->filters[i]),newCfg,errorCode);
        }
    }

    if(updateExisting){
        *errorCode = INSTANCE_UNKNOWN;
        return 1;
    }
    if(listEntry->numOfFilters == MAX_NUM_OF_FILTERS){
        *errorCode = TOO_MANY_FILTERS_DEFINED;
        return 1;
    }
    listEntry->filters[i] = cloneInstanceLogCfg(newCfg);
    if(listEntry->filters[i] == NULL) {
        *errorCode = NOT_ENOUGH_MEMORY;
        return 1;
    }
    listEntry->numOfFilters++;
    return 0;
}
static int removeFilter(llmInstanceLogCfgListEntry_t* listEntry, int filterID, int* errorCode){
    int i;
    for(i = 0; i < listEntry->numOfFilters; i++){
        if(listEntry->filters[i]->filterID == filterID){
            break;
        }
    }
    if(i == listEntry->numOfFilters){
        //Not found
        *errorCode = INSTANCE_UNKNOWN;
        return 1;
    }
    freeInstanceLogCfg(listEntry->filters[i]);
    for(i = i+1; i < listEntry->numOfFilters; i++){
        listEntry->filters[i-1] = listEntry->filters[i];
    }
    listEntry->filters[i-1] = NULL;
    listEntry->numOfFilters--;
    return 0;
}

/*Get instance name of the trace component*/
LLM_API const char * llmGetTCInstanceName(const TCHandle tcHandle){
  return tcHandle->instanceName;
}

//Methods for logging configuration
LLM_API int llmSetInstanceLogConfig(const llmInstanceLogCfg_t *config, int updateExisting, int* ec){
    llmInstanceLogCfgListEntry_t* listEntry = NULL;
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);
    int rc = 1;


    if(config == NULL){
        *errorCode = INVALID_CONFIG_PARAM;
        return rc;
    }
    *errorCode = 0;
    fmd_lock(&llmLogUtilLock);
    do {
        //Check whether instance is already registered
        listEntry = findLogCfgListEntry(config->instanceName);
        if(listEntry == NULL){
            //Clone instnce configuration and put it to the head of the list
            listEntry = (llmInstanceLogCfgListEntry_t*)malloc(sizeof(llmInstanceLogCfgListEntry_t));
            if(listEntry == NULL){
                *errorCode = NOT_ENOUGH_MEMORY;
                break;
            }
            memset(listEntry,0,sizeof(llmInstanceLogCfgListEntry_t));
            rc = setFilter(listEntry,config,updateExisting,errorCode);
            if(rc != 0){
                free(listEntry);
                break;
            }
            listEntry->instanceName = strdup(config->instanceName);
            //Update the list. Put new config to the head of the list
            listEntry->next = llmInstanceLogCfgListHead;
            llmInstanceLogCfgListHead = listEntry;
            if(listEntry->next != NULL){
                listEntry->next->prev = listEntry;
            }
            //Update registered trace components
            updateAllTraceComponents(listEntry->instanceName,listEntry);
        }else{
            rc = setFilter(listEntry,config,updateExisting,errorCode);
        }
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}

LLM_API int llmGetInstanceLogConfig(llmInstanceLogCfg_t *config, int* ec){
    llmInstanceLogCfgListEntry_t* listEntry = NULL;
    int rc = 1;
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);

    if(config == NULL){
        *errorCode = INVALID_CONFIG_PARAM;
        return rc;
    }
    fmd_lock(&llmLogUtilLock);
    do{
        int i;
        listEntry = findLogCfgListEntry(config->instanceName);
        if(listEntry == NULL){
            //Instance is unknown
            *errorCode = INSTANCE_UNKNOWN;
            break;
        }
        for(i = 0; i < listEntry->numOfFilters; i++){
            if(listEntry->filters[i]->filterID == config->filterID){
                char* name = config->instanceName;
                memcpy(config,listEntry->filters[i],sizeof(llmInstanceLogCfg_t));
                config->instanceName = name;
                if(config->get_time == getCurrentTimeMillis)
                    config->get_time = NULL;
                rc = 0;
                break;
            }
        }
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}

LLM_API int llmRemoveInstanceLogConfig(const char* instanceName, int filterID, int* ec){
    llmInstanceLogCfgListEntry_t* listEntry = NULL;
    int rc = 1;
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);

    if(instanceName == NULL){
        *errorCode = INVALID_CONFIG_PARAM;
        return rc;
    }
    fmd_lock(&llmLogUtilLock);
    do{
        listEntry = findLogCfgListEntry(instanceName);
        if(listEntry == NULL){
            //Instance is unknown
            *errorCode = INSTANCE_UNKNOWN;
            break;
        }
        finalizeInstance();
        rc = removeFilter(listEntry,filterID,errorCode);
        if(rc != 0){
            break;
        }
        if(listEntry->numOfFilters > 0){
            break;
        }
        //Update the list
        if(listEntry->next != NULL){
            listEntry->next->prev = listEntry->prev;
        }
        if(listEntry->prev != NULL){
            listEntry->prev->next = listEntry->next;
        }else{
            llmInstanceLogCfgListHead = listEntry->next;
        }
        free(listEntry->instanceName);
        free(listEntry);
        listEntry = findLogCfgListEntry("");
        //Update trace components related to this instance. There should not be components like that, but ...
        if(listEntry == NULL){
            updateAllTraceComponents(instanceName,NULL);
        }else{
            updateAllTraceComponents(instanceName,listEntry);
        }
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}


LLM_API int llmChangeComponentLogConfig(const char* instanceName, int filterID, const llmComponentLogCfg_t *config, int* ec){
    llmInstanceLogCfgListEntry_t* listEntry = NULL;
    int rc = 1;
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);

    if((instanceName == NULL) || (config == NULL) ){
        *errorCode = INVALID_CONFIG_PARAM;
        return rc;
    }
    fmd_lock(&llmLogUtilLock);
    do{
        int i;
        llmInstanceLogCfg_t* cfg = NULL;
        listEntry = findLogCfgListEntry(instanceName);
        if(listEntry == NULL){
            //Instance is unknown
            *errorCode = INSTANCE_UNKNOWN;
            break;
        }
        for(i = 0; i < listEntry->numOfFilters; i++){
            if(listEntry->filters[i]->filterID == filterID){
                cfg = listEntry->filters[i];
                break;
            }
        }
        if(cfg == NULL){
            //Instance is unknown
            *errorCode = INSTANCE_UNKNOWN;
            break;
        }
        for(i = 0; i < cfg->numOfComponentCfgs; i++){
            if(cfg->componentCfgs[i].componentId == config->componentId){
                cfg->componentCfgs[i].allowedLogLevel = config->allowedLogLevel;
                break;
            }
        }
        if(i == cfg->numOfComponentCfgs){//Component had a default configuration
            if(cfg->numOfComponentCfgs == MAX_NUMBER_OF_COMPONENTS){
                *errorCode = MAX_NUMBER_OF_COMPONENTS_EXEEDED;
                break;
            }
            cfg->componentCfgs[i].allowedLogLevel = config->allowedLogLevel;
            cfg->componentCfgs[i].componentId = config->componentId;
            cfg->numOfComponentCfgs++;
        }
        removeDefaultComponents(cfg);
        rc = 0;
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}

//Update information for registered component
static void updateTC(TraceComponent_t* pTC,const llmInstanceLogCfgListEntry_t* cfg){
    pTC->instanceCfg = cfg;
}

LLM_API int llmRegisterTraceComponent(const char* instanceName, int componentId, const COMPONENT_NAME componentName,
                                      TCHandle* pHandle, int *ec){
    llmInstanceLogCfgListEntry_t* listEntry = NULL;
    int rc = 1;
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);

    if((instanceName == NULL) || (pHandle == NULL)){
        *errorCode = INVALID_CONFIG_PARAM;
        return rc;
    }
    fmd_lock(&llmLogUtilLock);
    do{
        TCHandle result = TCListHead;
        while(result != NULL){
            if((strcmp(result->instanceName,instanceName) == 0) && (result->componentId == componentId)){
                break;
            }
            result = result->next;
        }
        if(result != NULL){
            *pHandle = result;
            result->inUseCounter++;
            rc = 0;
        }else{
            result = (TraceComponent_t*)malloc(sizeof(TraceComponent_t));
            if(result == NULL){
                *errorCode = NOT_ENOUGH_MEMORY;
                break;
            }
            memset(result,0,sizeof(TraceComponent_t));
            result->instanceName = strdup(instanceName);
            result->componentId = componentId;
            if(componentName[0] == 0){
                SPRINTF1(result->componentName,sizeof(COMPONENT_NAME),"Component.%d",componentId);
            }else{
                STRCPY(result->componentName,componentName,sizeof(COMPONENT_NAME));
            }
            result->inUseCounter = 1;
            listEntry = findLogCfgListEntry(instanceName);
            if(listEntry == NULL){
                listEntry = findLogCfgListEntry("");
            }
            if(listEntry != NULL){
                updateTC(result,listEntry);
            }
            *pHandle = result;
            result->next = TCListHead;
            TCListHead = result;
            if(result->next != NULL){
                result->next->prev = result;
            }else{
                createLogAnnouncerThread();
            }
            rc = 0;
        }
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}



LLM_API int llmUnregisterTraceComponent(TCHandle tcHandle, int *ec){
    int tmp;
    int *errorCode = ((ec != NULL) ? ec : &tmp);
    if((tcHandle == NULL) || (tcHandle->inUseCounter == 0)){
        *errorCode = (tcHandle == NULL) ? INVALID_CONFIG_PARAM : COMPONENT_NOT_REGISTERED;
        return 1;
    }
    fmd_lock(&llmLogUtilLock);
    tcHandle->inUseCounter--;
    if(tcHandle->inUseCounter == 0){
        if(tcHandle->next != NULL){
            tcHandle->next->prev = tcHandle->prev;
        }
        if(tcHandle->prev != NULL){
            tcHandle->prev->next = tcHandle->next;
        }else{
            TCListHead = tcHandle->next;
            if (TCListHead == NULL) {
                stopLogAnnouncerThread(0);
                freePools();
            }
        }
        free(tcHandle->instanceName);
        free(tcHandle);
    }
    fmd_unlock(&llmLogUtilLock);
    return 0;
}

static int isFiltered(const llmInstanceLogCfg_t* cfg, int componentID, unsigned int logLevel, int msgId){
    int i;
    unsigned int allowedLogLevel = cfg->allowedLogLevelDefault;
    if(cfg->onLogEvent == NULL) return 1;
    for(i = 0; i < cfg->numOfComponentCfgs; i++){
        if(cfg->componentCfgs[i].componentId == componentID){
            allowedLogLevel = cfg->componentCfgs[i].allowedLogLevel;
            break;
        }
    }
    if(allowedLogLevel < logLevel){
        for(i = 0; i < cfg->numOfAllowedKeys; i++){
            if(cfg->allowedKeys[i] == msgId){
                return 0;
            }
        }
        return 1;
    }else{
        for(i = 0; i < cfg->numOfRestrictedKeys; i++){
            if(cfg->restrictedKeys[i] == msgId){
                return 1;
            }
        }
        return 0;
    }
}

static int isTraceAllowed(TCHandle tcHandle, unsigned int logLevel, int msgId){
    int i;
    const llmInstanceLogCfgListEntry_t* instanceCfg = tcHandle->instanceCfg;
    if(instanceCfg == NULL){
        return 0;
    }
    for(i = 0; i < instanceCfg->numOfFilters; i++){
        if(!isFiltered(instanceCfg->filters[i],tcHandle->componentId,logLevel,msgId)){
            return 1;
        }
    }
    return 0;
}

static void getAllowedCallbacks(TBHandle tbHandle){
    const llmInstanceLogCfgListEntry_t* instanceCfg = tbHandle->tcHandle->instanceCfg;
    tbHandle->numOfCallbacks = 0;
    if(instanceCfg != NULL){
        int i;
        for(i = 0; i < instanceCfg->numOfFilters; i++){
            if(isFiltered(instanceCfg->filters[i],tbHandle->componentId,tbHandle->logLevel,tbHandle->msgKey)){
                continue;
            }
            tbHandle->callbacks[tbHandle->numOfCallbacks].onLogEvent = instanceCfg->filters[i]->onLogEvent;
            tbHandle->callbacks[tbHandle->numOfCallbacks].user = instanceCfg->filters[i]->user;
            tbHandle->callbacks[tbHandle->numOfCallbacks].timestamp = instanceCfg->filters[i]->get_time(instanceCfg->filters[i]->get_time_param);


            tbHandle->numOfCallbacks++;
        }
    }
}
//Checks if specific log entry is allowed
LLM_API int llmIsTraceAllowed(TCHandle tcHandle, unsigned int logLevel, int msgId){
    int rc;
    if((tcHandle == NULL) || (tcHandle->inUseCounter == 0)){
        return 0;
    }
    fmd_lock(&llmLogUtilLock);
    rc = isTraceAllowed(tcHandle, logLevel,msgId);
    fmd_unlock(&llmLogUtilLock);
    return rc;
}

//Update all registered trace components related to the instance
static void updateAllTraceComponents(const char* instanceName, const llmInstanceLogCfgListEntry_t *config){
    TCHandle current = TCListHead;
    int isDefaultConfigUpdated = (strlen(instanceName) == 0);
    while(current != NULL){
        if(
            (strcmp(current->instanceName,instanceName) == 0) //Component uses instance configuration
            ||
            (isDefaultConfigUpdated && (findLogCfgListEntry(current->instanceName) == NULL)) //Component uses default configuration
        ){
            updateTC(current,config);
        }
        current = current->next;
    }
}


/*
 * Put char into the byte array, adding the '\0'.
 * @param value  The char value
 * @param buf    The output buffer.
 */
static size_t llm_ctoa(char value, replacementData_t buff){
    buff[0] = value;
    buff[1] = 0;
    return 1;
}

/*
 * Copies string into the byte array, cutting it if there is no enough space.
 * @param value  The string value
 * @param buf    The output buffer.
 */
static size_t llm_stoa(char* value, replacementData_t buff){
    int maxLength = sizeof(replacementData_t)-1;
    int i=0;
    if(value == NULL){
        STRCPY(buff,"(null)",sizeof(replacementData_t));
        return 6;
    }
    while(value[i]){
        buff[i] = value[i];
        i++;
        if(i == maxLength) break;
    };
    buff[i] = 0;
    return i;
}

/*
 * Convert a signed 64 bit integer to ASCII with separators.
 * @param lval value (64) bit integer value
 * @param buff  The output buffer.  This should be at least 24 bytes long
 */
static size_t llm_itoa(int64_t value, replacementData_t buff){
    char sValue[64];
    char* curr = sValue+sizeof(sValue)-1;
    char* result = buff;
    size_t length;

    if(value < 0){
        *result = '-';
        result++;
        value = -value;
    }
    do{
        int digit = (int)(value % 10);
        value = value / 10;
        *curr = '0' + digit;
        curr--;
    }while(value);
    curr++;
    length = sizeof(sValue)-(curr-sValue);
    memcpy(result, curr, length);
    result[length] = 0;
    return ((&(result[length]))-buff);
}

/*
 * Convert an unsigned 64 bit unsigned integer to ASCII.
 * @param ulval  The unsigned long value
 * @param buf    The output buffer.  This should be at least 24 bytes long.
 * @param radix  10 or 16
 */
static size_t llm_uitoa(uint64_t value, replacementData_t buff, int radix){
    char sValue[32];
    char* curr = sValue+sizeof(sValue)-1;
    char* result = buff;
    size_t length;

    if(radix == 16){
        result[0] = '0';
        result[1] = 'x';
        result += 2;
    }
    do{
        int digit = (int)(value % radix);
        value = value/radix;
        if(digit < 10){
            *curr = '0' + digit;
        }else{
            *curr = 'A' + digit - 10;
        }
        curr--;
    }while(value);
    curr++;
    length = sizeof(sValue)-(curr-sValue);
    memcpy(result, curr, length);
    result[length] = 0;
    return ((&(result[length]))-buff);
}

//Remove character from the end of the string
#define TRIM_CHAR(buff, c) {            \
    char* pChar = buff+strlen(buff);    \
    while (pChar[-1] == c){             \
        pChar--;                        \
    }                                   \
    *pChar = 0;                         \
}

/*
 * Convert a double to ASCII
 * @param dbl  The double value.
 * @param buf  The output buffer.  This should be at least 32 bytes long.
 */
static size_t llm_dtoa(double dbl, replacementData_t buff) {
    char *      cp;
    char *      outp = buff;
    double      d2;
    int         x;
    size_t      maxLength = sizeof(replacementData_t);
    union{uint64_t l; double d;} ud;

    if (dbl<0.0) {
        *buff++ = '-';
        dbl = -dbl;
        maxLength--;
    }

    /*
     * Check for NaN and Infinity
     */
    ud.d = dbl;
    if ((ud.l&0x7ff0000000000000LL) == 0x7ff0000000000000LL) {
        if ((ud.l&0xfffffffffffffLL) == 0) {
            memcpy(buff, "Infinity", 9);
        } else {
            memcpy(outp, "NaN", 4);                      /* Ignore the sign*/
        }
        return strlen(buff);
    }

    /*
     * For very large or small values or for NaN or infinity use E notation
     */
    if (dbl < 1E-5 || dbl>=1E10) {
        if (dbl == 0) {
            memcpy(buff, "0.0", 4);
        } else {
            SPRINTF1(buff, maxLength,"%.8g", dbl);
            TRIM_CHAR(buff,' ');
        }
        return strlen(buff);
    }

    /*
     * Handle integer
    */
    if (floor(dbl)==dbl && dbl<1E9) {
        size_t length = llm_uitoa((int64_t)dbl, buff,10);
        STRCAT(buff, maxLength,".0");
        return (length+2);
    }
   /* Handle up to 4 decimal digits */
    d2 = dbl*10000.0;
    if (floor(d2) == d2) {
        llm_uitoa((uint64_t)dbl, buff,10);
        x = (int)(((uint64_t)d2)%10000);
        cp = buff + strlen(buff);
        cp[0] = '.';
        if(x != 0){
            cp[1] = (char)(x/1000 + '0');
            cp[2] = (char)(x/100%10 + '0');
            cp[3] = (char)(x/10%10 + '0');
            cp[4] = (char)(x%10 + '0');
            cp[5] = 0;
            TRIM_CHAR(buff,'0');
        }else{
            cp[1] = '0';
            cp[2] = 0;
        }
        return strlen(buff);
    }

    SPRINTF1(buff,maxLength, "%.9f", dbl);
    TRIM_CHAR(buff,'0');
    return strlen(buff);
}

/*
    Parse replacement data and converts it to strings
    Supported replacement data types are:
        %c - char
        %d - signed integer
        %u - unsigned integer (dec)
        %x - unsigned integer (hex)
        %e - double
        %lld - signed 64 bit integer
        %llu - unsigned 64 bit integer (dec)
        %llx - unsigned 64 bit integer (hex)
*/
static int parseReplacementData(const char* replDataTypes,replacementData_t* replData,int replDataSize, va_list args){
    int i;
    char *nextToken = NULL;
    char *token = NULL;
  int len = strlen(replDataTypes)+1;
    char* types = alloca(len);
    if(types == NULL) return 0;
  memcpy(types,replDataTypes,len);
    for(i = 0, token = strtok_r(types,"%",&nextToken); token != NULL; i++, token = strtok_r(NULL,"%",&nextToken)){
        char c = *token;
        int error = 0;
        if(i >= replDataSize){
            break;
        }
        memset(&replData[i],0,sizeof(replacementData_t));
        switch(c){
            case 's':
                {
                    char* value = (char*)va_arg(args, char*);
                    llm_stoa(value,replData[i]);
                }
                break;
            case 'd':
                {
                    int value = (int)va_arg(args, int);
                    llm_itoa(value,replData[i]);
                }
                break;
            case 'u':
                {
                    unsigned int value = (unsigned int)va_arg(args, unsigned int);
                    llm_uitoa(value,replData[i],10);
                }
                break;
            case 'x':
                {
                    unsigned int value = (unsigned int)va_arg(args, unsigned int);
                    llm_uitoa(value,replData[i],16);
                }
                break;
            case 'l':
                if((strlen(token) != 3) || (token[1] != 'l')){
                    error = 1;
                }else{
                    switch(token[2]){
                        case 'd':
                            {
                                int64_t value = (int64_t)va_arg(args, int64_t);
                                llm_itoa(value,replData[i]);
                            }
                            break;
                        case 'u':
                            {
                                uint64_t value = (uint64_t)va_arg(args, uint64_t);
                                llm_uitoa(value,replData[i],10);
                            }
                            break;
                        case 'x':
                            {
                                uint64_t value = (uint64_t)va_arg(args, uint64_t);
                                llm_uitoa(value,replData[i],16);
                            }
                            break;
                        default:
                            error = 1;
                            break;
                    }
                }
                break;
            case 'e':
                {
                    double value = (double)va_arg(args, double);
                    llm_dtoa(value,replData[i]);
                }
                break;
            case 'c':
                {
                    char value = (char)va_arg(args, int);
                    llm_ctoa(value,replData[i]);
                }
                break;
            default:
                error = 1;
                break;
        }
        if(error){
            break;
        }
    }
    return i;
}

/*
 * Format a message from a java message definition.
*/
static size_t formatMessage(char * msgBuff, size_t msgBuffSize, const char * msgFormat,
                            const replacementData_t* replData,int replDataSize) {
    char * mp;               /* Output location in the message */
    char * xp;               /* Output location in the format info fields */
    char   formatNum[64];    /* Replacement number, Nulled for safety */
    char   formatType[64];   /* Replacement type  (Currently we just parse it, but don't use it) */
    char   formatExt[64];    /* Replacement extension (Currently we just parse it, but don't use it)*/
    size_t left,  xleft;     /* Bytes left */
    int    inQuote = 0;      /* Inside a quote */
    int    braceStack = 0;   /* Brace count in a format specification */
    int    part;             /* Which part of the format */
    size_t    replen;        /* Replacement length */
    const char * repstr;     /* Replacement string */

    mp = msgBuff;
    left = msgBuffSize-1;
    while (*msgFormat) {
        char ch = *msgFormat++;
        if (ch == '\'') {                    /* Start of quote */
            if (*msgFormat == '\'') {              /* Two quotes makes a single quote */
                if (left > 0)                /* Put out the quote char */
                    *mp++ = ch;
                left--;
            } else {
                inQuote = !inQuote;          /* Invert quote state */
            }
            continue;
        }
        if (inQuote || ch != '{') {   /* If not start of format spec */
            if (left > 0)                    /* Put out the char */
                *mp++ = ch;
            left--;
            continue;
        }
        /*
         * Parse the format string
         */
        part = 0;
        xp = formatNum;                  /* The first field is the format number */
        xleft = 63;
        braceStack = 1;                  /* We are in a format */
        while (*msgFormat && braceStack) {
            ch = *msgFormat++;
            switch (ch) {
            case ',':                    /* End of a section */
                *xp = 0;
                switch (part) {
                case 0:
                    xp = formatType;     /* Switch to type */
                    xleft = 63;
                    part = 1;
                    break;
                case 1:
                    xp = formatExt;      /* Switch to extension */
                    xleft = 63;
                    part = 2;
                    break;
                default:                 /* Just put the comma into the extension */
                    part++;
                    if (xleft > 0){
                        *xp++ = ch;
                        xleft--;
                    }
                }
                break;

            case '{':                    /* Keep an even number of braces */
                braceStack++;
                if (xleft > 0){
                    *xp++ = ch;
                    xleft--;
                }
                break;

            case '}':
                braceStack--;
            /*
             * Replace the variable
             */
                if (braceStack == 0) {
                    int which;
                    *xp = 0;
                    which = atoi(formatNum);
                    if (which <0 || which >= replDataSize) {
                        repstr = "*UNKNOWN*";
                    } else {
                        repstr = replData[which];
                    }

                    /* Put the string in the output buffer */
                    replen = strlen(repstr);
                    if (replen < left) {
                        memcpy(mp, repstr, replen);
                        mp += replen;
                        left -= replen;
                    }
                    memset(formatNum, 0, 64);
                    memset(formatType, 0, 64);
                    memset(formatExt, 0, 64);
                }else{
                    if (xleft > 0){
                        *xp++ = ch;
                        xleft--;
                    }
                }
                break;
            default:                    /* Put the character into the current format string */
                if (xleft > 0){
                    *xp++ = ch;
                    xleft--;
                }
                break;

            }

        }
    }

    /*
     * Return the size the message would have been (it could be larger than the buffer)
     */
    msgBuffSize -= left;
    *mp = 0;
    return msgBuffSize;
}


/*Get TraceBuffer from the pool. Create new buffer if pool is empty*/
static TBHandle getTraceBuffer(int block){
    TraceBuffer_t* traceBuffer = NULL;
    while(block && (numOfTraceBuffersAllocated >= MAX_NUM_OF_TB)){/* BEAM suppression: infinite loop */
        fmd_unlock(&llmLogUtilLock);
        fmd_sleep(1000);
        fmd_lock(&llmLogUtilLock);
    }
    if(tbPool != NULL){
        traceBuffer = tbPool;
        tbPool = tbPool->next;
        tbPoolSize--;
        traceBuffer->next = NULL;
    }else{
        traceBuffer = (TraceBuffer_t*) malloc (sizeof(TraceBuffer_t));
        memset(traceBuffer,0,sizeof(TraceBuffer_t));
        numOfTraceBuffersAllocated++;
    }
    return traceBuffer;
}

//Return TraceBuffer to the pool. Free buffer if pool is full
static void returnTraceBuffer(TBHandle tbHandle){
    while(tbHandle != NULL){
        TBHandle next = tbHandle->next;
        if(tbPoolSize < MAX_POOL_SIZE){
            memset(tbHandle,0,sizeof(TraceBuffer_t));
            tbHandle->next = tbPool;
            tbPool = tbHandle;
            tbPoolSize++;
        }else{
            free(tbHandle);
            numOfTraceBuffersAllocated--;
        }
        tbHandle = next;
    }
}

static void initTraceBuffer(TBHandle tbHandle, TCHandle tcHandle, int logLevel, int msgId){
    tbHandle->tcHandle = tcHandle;
    tbHandle->componentId = tcHandle->componentId;
    STRCPY(tbHandle->componentName,tcHandle->componentName,sizeof(COMPONENT_NAME));
    tbHandle->threadId = fmd_threadSelf();
    //tbHandle->timestamp = getCurrentTimeMillis();
    tbHandle->logLevel = logLevel;
    tbHandle->msgKey = msgId;
}

LLM_API TBHandle llmCreateTraceBuffer(TCHandle tcHandle, unsigned int logLevel, int msgId){
    TBHandle result = NULL;
    fmd_lock(&llmLogUtilLock);
    do{
        if((tcHandle == NULL) || (tcHandle->inUseCounter == 0)) break;
        if(!isTraceAllowed(tcHandle,logLevel,msgId)) break;
        result = getTraceBuffer(1);
        if(result == NULL) break;
        initTraceBuffer(result,tcHandle,logLevel,msgId);
        result->instanceName = tcHandle->instanceCfg->instanceName;
        result->last = result;
    }while(0);
    fmd_unlock(&llmLogUtilLock);
    return result;
}

LLM_API int llmSimpleTraceInvoke(TCHandle tcHandle, unsigned int logLevel, int msgId, const char* replacementDataTypes, const char* msgFormat, ...){
    va_list args;
    TraceBuffer_t* traceBuffer = NULL;
    fmd_lock(&llmLogUtilLock);
    if((tcHandle == NULL) || (tcHandle->inUseCounter == 0)){
      fmd_unlock(&llmLogUtilLock);
      return -1;
    }
    if(!isTraceAllowed(tcHandle,logLevel,msgId)){
      fmd_unlock(&llmLogUtilLock);
      return -1;
    }
    //Get buffer from the pool
    traceBuffer = getTraceBuffer(1);
    if(traceBuffer == NULL){
      fmd_unlock(&llmLogUtilLock);
      return -1;
    }
    initTraceBuffer(traceBuffer,tcHandle,logLevel,msgId);
    traceBuffer->instanceName = tcHandle->instanceCfg->instanceName;
    fmd_unlock(&llmLogUtilLock);
    va_start(args,msgFormat);
    traceBuffer->replDataSize = parseReplacementData(replacementDataTypes,traceBuffer->replData,MAX_NUM_OF_REPL_DATA,args);
    va_end(args);
    traceBuffer->traceLineLength = formatMessage(traceBuffer->traceLine,sizeof(TRACE_LINE),msgFormat,(const replacementData_t*)(&(traceBuffer->replData[0])),traceBuffer->replDataSize);
    fmd_lock(&llmLogUtilLock);
    invokeLog(traceBuffer);
    fmd_unlock(&llmLogUtilLock);
    return 0;
}

//Invoke the log entry (Will combine all data in Tracebuffer to one log entry)
LLM_API int llmCompositeTraceInvoke(TBHandle tbHandle){
    if(tbHandle == NULL) return -1;
    fmd_lock(&llmLogUtilLock);
    invokeLog(tbHandle);
    fmd_unlock(&llmLogUtilLock);
    return 0;
}


LLM_API int llmAddTraceData(TBHandle tbHandle, const char* replacementDataTypes, const char* msgFormat, ...){
    va_list args;
    TraceBuffer_t* traceBuffer = NULL;
    if(tbHandle == NULL){
      return -1;
    }
    if((tbHandle->last->traceLineLength == 0) && (tbHandle->last->replDataSize == 0)){ //The last trace buffer is empty
      traceBuffer = tbHandle->last;
    }else{
      //Get new buffer from the pool
      fmd_lock(&llmLogUtilLock);
      traceBuffer = getTraceBuffer(0);
      fmd_unlock(&llmLogUtilLock);
      if(traceBuffer == NULL){
        //This can happen only if we are out of memory
        return -1;
      }
      tbHandle->last->next = traceBuffer;
      tbHandle->last = traceBuffer;
    }
    //Format the logging message
    va_start(args,msgFormat);
    traceBuffer->replDataSize = parseReplacementData(replacementDataTypes,traceBuffer->replData,MAX_NUM_OF_REPL_DATA,args);
    va_end(args);
    traceBuffer->traceLineLength = formatMessage(traceBuffer->traceLine,sizeof(TRACE_LINE),msgFormat,(const replacementData_t*)(&(traceBuffer->replData[0])),traceBuffer->replDataSize);
    
    return 0;
}

//Get TraceBuffer from the pool. Create new buffer if pool is empty
static LogAnnouncerTask_t* getTaskFromThePool(void){
    LogAnnouncerTask_t* task = NULL;
    if(tasksPool != NULL){
        task = tasksPool;
        tasksPool = tasksPool->next;
        tasksPoolSize--;
        task->next = NULL;
    }else{
        task = (LogAnnouncerTask_t*) malloc (sizeof(LogAnnouncerTask_t));
        memset(task,0,sizeof(LogAnnouncerTask_t));
    }
    return task;
}

//Return TraceBuffer to the pool. Free buffer if pool is full
static void returnTaskToThePool(LogAnnouncerTask_t* task){
    if(tasksPoolSize < MAX_POOL_SIZE){
        memset(task,0,sizeof(LogAnnouncerTask_t));
        task->next = tasksPool;
        tasksPool = task;
        tasksPoolSize++;
    }else{
        free(task);
    }
}

LLM_API int llmReleaseTraceBuffer(TBHandle tbHandle){
    if(tbHandle == NULL){
        return 0;
    }
    fmd_lock(&(llmLogUtilLock));
    returnTraceBuffer(tbHandle);
    fmd_unlock(&(llmLogUtilLock));
    return 0;
}


/*
    Log announcer processing method
*/
static void* llmLogAnnouncerThreadProc(void* param){
LogAnnoncerThreadInfo_t *pThreadInfo = (LogAnnoncerThreadInfo_t*)param;
 #if USE_PRCTL
  {
    char tn[16] ; 
    rmm_strlcpy(tn,__FUNCTION__,16);
    prctl( PR_SET_NAME,(uintptr_t)tn);
  }
 #endif
    while(1){
        LogAnnouncerTask_t* currTask = NULL;
        la_task_proc_t      taskProc;
        void*               taskParam;
        fmd_lock(&(llmLogUtilLock));
        //Return last used trace buffer to the ppol
        returnTraceBuffer(pThreadInfo->traceBufferToReturn);
        pThreadInfo->traceBufferToReturn = NULL;
        while((pThreadInfo->isRunning) && (pThreadInfo->taskListSize == 0)){/* BEAM suppression: infinite loop */
            //No events in the queue - wait
            fmd_cond_wait(&(pThreadInfo->condVar),&(llmLogUtilLock));
        }
        if(pThreadInfo->taskListSize == 0){
            //Termination request was received - stop the tread
            llmLogAnnouncerThread = NULL;
            fmd_unlock(&(llmLogUtilLock));
            break;
        }
        //Remove first task from the queue
        currTask = pThreadInfo->taskListHead;
        //SA_ASSUME(currTask != NULL);
        pThreadInfo->taskListHead = currTask->next;
        if(pThreadInfo->taskListHead == NULL){
            //The task list is empty
            pThreadInfo->taskListTail = NULL;
        }
        pThreadInfo->taskListSize--;
        taskProc = currTask->task_proc;
        taskParam = currTask->taskParam;
        currTask->next = NULL;
        returnTaskToThePool(currTask);
        fmd_unlock(&(llmLogUtilLock));
        //Process the current task. Note that we don't hold the lock at this point
        taskProc(taskParam);
    }
    fmd_cond_destroy(&(pThreadInfo->condVar));
    free(pThreadInfo);
    fmd_endThread(NULL);
    return NULL;
}
/*
Create the log announcer thread
*/
static int  createLogAnnouncerThread(void){
    int rc;
    if(llmLogAnnouncerThread != NULL) return -1;
    llmLogAnnouncerThread = (LogAnnoncerThreadInfo_t*)malloc(sizeof(LogAnnoncerThreadInfo_t));
    if(llmLogAnnouncerThread == NULL) return -1;
    memset(llmLogAnnouncerThread,0,sizeof(LogAnnoncerThreadInfo_t));
    do {
        rc = fmd_cond_init(&(llmLogAnnouncerThread->condVar));
        if(rc != 0) break;
        llmLogAnnouncerThread->isRunning = 1;
        rc = fmd_startThread(&(llmLogAnnouncerThread->threadId),llmLogAnnouncerThreadProc,llmLogAnnouncerThread);
        if(rc != 0) {
            fmd_cond_destroy(&(llmLogAnnouncerThread->condVar));
            llmLogAnnouncerThread->isRunning = 0;
            break;
        }
    }while(0);
    if(rc != 0){
        free(llmLogAnnouncerThread);
        llmLogAnnouncerThread = NULL;
    }
    return rc;
}

/*
Stop the log announcer thread
*/
static void stopLogAnnouncerThread(int isFini){
    void* rc;
    int i = 0;
    fmd_threadh_t   threadId;
    while(llmLogAnnouncerThread != NULL){/* BEAM suppression: infinite loop */
        llmLogAnnouncerThread->isRunning = 0;//Mark thread as not running
        fmd_cond_signal(&(llmLogAnnouncerThread->condVar));
        threadId = llmLogAnnouncerThread->threadId;
        fmd_unlock(&(llmLogUtilLock));
        if(isFini){
            fmd_sleep(10000);
            if(i == 0) fmd_detachThread(threadId);
            i++;
            if(i==10){
                fmd_lock(&(llmLogUtilLock));
                break;
            }
        }else{
            fmd_joinThread(threadId,&rc);
            fmd_lock(&(llmLogUtilLock));
            break;
        }
        fmd_lock(&(llmLogUtilLock));
    }
    llmLogAnnouncerThread = NULL;
}

//Log notification task. It converts trace buffer to log event and executes the callback
static void notifyLogEventTask(void * param){
    TBHandle    traceBuffer = (TBHandle)param;
    int i;
    char*   formatedMessage;
    int numOfReplData = 1;
    size_t formatedMsgLength = 1;
    llmLogEvent_t* pLogEvent = &(logEventData.logEvent);
    pLogEvent->log_level = traceBuffer->logLevel;
    pLogEvent->module = traceBuffer->componentName;
    pLogEvent->msg_key = traceBuffer->msgKey;
    pLogEvent->threadId = traceBuffer->threadId;
    //pLogEvent->timestamp = traceBuffer->timestamp;
    pLogEvent->instanceName = traceBuffer->instanceName;
    //Calculate formated log message length and number of replacement data strings
    do {
        numOfReplData += traceBuffer->replDataSize;
        formatedMsgLength += (traceBuffer->traceLineLength+1);
        traceBuffer = traceBuffer->next;
    }while(traceBuffer != NULL);
    //We do not copy replacement data for log level higher than (INFO)
    if(pLogEvent->log_level <= LLM_LOGLEV_INFO){
        pLogEvent->nparams = numOfReplData;
        //Ensure that we have enough space for replacement data
        if(numOfReplData > logEventData.eventParamsSize){
            void ** tmp = (void**)malloc(numOfReplData*sizeof(void*));
            if(!tmp)
              goto task_complete;
            free(logEventData.eventParams);
            logEventData.eventParams = tmp;
            logEventData.eventParamsSize = numOfReplData;
            logEventData.logEvent.event_params = logEventData.eventParams;
        }
    }else{
        pLogEvent->nparams = 1;
    }
    //Ensure that we have enough space for formated message
    if(formatedMsgLength > logEventData.formatedMessageSize){
        char * tmp = (char*)malloc(formatedMsgLength*sizeof(char));
        if(!tmp)
          goto task_complete;
        free(logEventData.formatedMessage);
        logEventData.formatedMessage = tmp;
        logEventData.formatedMessageSize = formatedMsgLength;
    }
    formatedMessage = logEventData.formatedMessage;
    formatedMessage[0] = 0;
    traceBuffer = (TBHandle)param;
    numOfReplData = 1;
    do {
        //Combine all formated messages into one
        STRCAT(formatedMessage,logEventData.formatedMessageSize,traceBuffer->traceLine);
        formatedMsgLength -= traceBuffer->traceLineLength;
        if(pLogEvent->log_level <= LLM_LOGLEV_INFO){
            //Copy replacement data
            for(i = 0; i < traceBuffer->replDataSize; i++){
                pLogEvent->event_params[numOfReplData++] = traceBuffer->replData[i];
            }
        }
        traceBuffer = traceBuffer->next;
    }while(traceBuffer != NULL);
    pLogEvent->event_params[0] = formatedMessage;
    traceBuffer = (TBHandle)param;
    for(i = 0; i < traceBuffer->numOfCallbacks; i++){
        llm_on_log_event_t callback = traceBuffer->callbacks[i].onLogEvent;
        void* user = traceBuffer->callbacks[i].user;
        pLogEvent->timestamp = traceBuffer->callbacks[i].timestamp;
        callback(pLogEvent,user);//Invoke callback
    }
task_complete:
    llmLogAnnouncerThread->traceBufferToReturn = (TBHandle)param;//It will be return to the pool in the next processing cycle
}

//Add log annoncer task to the queue. Should be called withing synchronized block.
static int addLogAnnouncerTask(LogAnnouncerTask_t* task){
    if(llmLogAnnouncerThread == NULL){
        return -1;
    }
    task->next = NULL;
    if(llmLogAnnouncerThread->taskListSize == 0){
        llmLogAnnouncerThread->taskListHead = llmLogAnnouncerThread->taskListTail = task;
    }else{
        llmLogAnnouncerThread->taskListTail->next = task;
        llmLogAnnouncerThread->taskListTail = task;
    }
    llmLogAnnouncerThread->taskListSize++;
    fmd_cond_signal(&(llmLogAnnouncerThread->condVar));
    return 0;
}

//Create new log announcer task and put it to the processor queue. Should be called withing synchronized block.
static int invokeLog(TBHandle tbHandle){
    int rc;
    LogAnnouncerTask_t* task;
    getAllowedCallbacks(tbHandle);
    if(tbHandle->numOfCallbacks == 0) return 0;
    task = getTaskFromThePool();
    if(task == NULL) return -1;
    task->next = NULL;
    task->task_proc = notifyLogEventTask;
    task->taskParam = tbHandle;
    rc = addLogAnnouncerTask(task);
    if(rc != 0){
        returnTaskToThePool(task);
    }
    return rc;
}
typedef struct FTParam {
  fmd_cond_t* condVar;
  int         done;
} FTParam;
static void finalizeTask(void* param){
  FTParam* ftParam = (FTParam*)param;
  fmd_cond_t* condVar = ftParam->condVar;
  fmd_lock(&llmLogUtilLock);
  ftParam->done = 1;
  fmd_cond_broadcast(condVar);
  fmd_unlock(&llmLogUtilLock);
}

//Create a finalizer task and put it to the processor queue. Should be called withing synchronized block.
static int finalizeInstance(void){
    int rc;
    fmd_cond_t condVar;
    FTParam ftParam;
    LogAnnouncerTask_t* task = getTaskFromThePool();
    if(task == NULL) return -1;
    task->next = NULL;
    task->task_proc = finalizeTask;
    ftParam.condVar = &condVar;
    ftParam.done = 0;
    task->taskParam = &ftParam;
    fmd_cond_init(&condVar);
    rc = addLogAnnouncerTask(task);
    if(rc != 0){
      returnTaskToThePool(task);
    }else{
      while(!ftParam.done){
        fmd_cond_wait(&condVar,&llmLogUtilLock);
      }
    }
    fmd_cond_destroy(&condVar);
    return rc;
}

LLM_API FILE* llm_fopen(const char * fname, const char * mode, int *err){
  FILE* result = NULL;
  char tm[32];
  if(err) *err = 0;
  //if((result=fopen(u_fname, mode)) == NULL){
 #ifdef O_CLOEXEC
  rmm_snprintf(tm,32,"%se",mode);
 #else
  rmm_snprintf(tm,32,"%s",mode);
 #endif
  if((result=fopen(fname, tm)) == NULL){
    if(err) *err = errno;
  }
  return result;
}


