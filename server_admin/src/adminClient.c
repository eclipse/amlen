/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#ifndef TRACE_COMP
#define TRACE_COMP Admin
#endif
#include <adminInternal.h>
#include <assert.h>
#include <pthread.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>


extern int ism_admin_mqc_receive(int correlationID, int rc, char * buf, int buflen);
extern int ism_config_addInitialMQCConfiguration(concat_alloc_t *buf);
extern int ism_config_json_updateFile(int getLock);
extern int ism_config_json_delMQCObject(int objID, char *objName);
extern int ism_config_json_addMQCObject(char *sentBuffer);
extern pthread_spinlock_t configSpinLock;
extern int mqcPausedSignaled;

static const char * traceFolder = IMA_SVR_DATA_PATH "/diag/logs";
static ismHashMap * mqcaRequestsMap = NULL;
static int mqcaCorrelationID = 1;
static char mqcTaskSet[8192]={0};

typedef struct mqcAdminRequest_t {
    ism_http_t *http;
    ism_rest_api_cb callback;
    int persistData;
    int objID;
    char *objName;
    char *bufSent;
} mqcAdminRequest_t;

typedef ism_transport_t * (*newOutgoing_f)(ism_endpoint_t * endpoint, int fromPool);
newOutgoing_f newOutgoingFptr = NULL;

typedef int (*connect_f)(ism_transport_t * transport, ism_transport_t * ctransport,
        const char * server, int port, struct ssl_ctx_st * tlsCTX);
connect_f connectFptr = NULL;

static int startMqcAdminChannel(uint32_t sqn);
static ism_transport_t * getMQCAdminChannel(void);
static void freeMQCAdminChannel(void);
static volatile int mqcTerminated = 1;

int mqcAdminPauseState = ISM_ADMIN_STATE_PAUSE;
static pthread_mutex_t mqcAdminLock;
static pthread_cond_t  mqcAdminCond;

int mqcAdminPauseInited = 0;

#define     CHANNEL_CONNECTION_CLOSED       0
#define     CHANNEL_CONNECTION_IN_PROCESS   1
#define     CHANNEL_CONNECTED               2

typedef struct mqcaChannel_t {
    ism_transport_t *   transport;
    pthread_spinlock_t  lock;
    uint16_t            state;
    uint16_t            useCount;
}mqcaChannel_t;

static mqcaChannel_t mqcAdminChannel = {0};
static const char * mqcServer = "/tmp/.com.ibm.ima.mqcAdmin";
/*
 * Dummy endpoint so we do not segfault
 */
static ism_endstat_t    mqcStat;
static ism_endpoint_t mqcaEndpoint = {
    .name        = "!MQCAdmin",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_AnyInternal,
    .stats       = &mqcStat,
};

static uint32_t         mqcPiSqn = 0;
typedef struct externalProcessInfo_t {
    pthread_barrier_t   barrier;
    pthread_mutex_t     lock;
    pthread_t           thread;
    pid_t               pid;
    uint32_t            sqn;
    ism_timer_t         timer;
    volatile uint8_t    terminated;
} externalProcessInfo_t;

static externalProcessInfo_t * mqcProcInfo = NULL;

static externalProcessInfo_t * initExternalProcInfo(void) {
    externalProcessInfo_t * result = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,557),1,sizeof(externalProcessInfo_t));

    pthread_barrier_init(&result->barrier, NULL, 2);
    pthread_mutex_init(&result->lock, NULL);
    return result;
}

static void destroyExternalProcInfo(externalProcessInfo_t * info) {
    pthread_mutex_destroy(&info->lock);
    pthread_barrier_destroy(&info->barrier);
    if(info->timer)
        ism_common_cancelTimer(info->timer);
    ism_common_free(ism_memory_admin_misc,info);
}


static void killMQCProcess(void) {
    pthread_spin_lock(&mqcAdminChannel.lock);
    if(mqcProcInfo) {
        pthread_mutex_lock(&mqcProcInfo->lock);
        if(mqcProcInfo->pid) {
            kill(mqcProcInfo->pid, SIGKILL);
        }
        pthread_mutex_unlock(&mqcProcInfo->lock);
    }
    pthread_spin_unlock(&mqcAdminChannel.lock);
}

static int mqcStartControlChannelTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
/*
 *
 */
static void handleControlChannelClose(ism_time_t delay) {
    pthread_spin_lock(&mqcAdminChannel.lock);
    if (mqcTerminated) {
        pthread_spin_unlock(&mqcAdminChannel.lock);
        return;
    }
    if(mqcProcInfo)
        mqcProcInfo->timer = ism_common_setTimerOnce(ISM_TIMER_HIGH, mqcStartControlChannelTimer,
                (void*)((uintptr_t)mqcProcInfo->sqn), delay);
    pthread_spin_unlock(&mqcAdminChannel.lock);
}

static void clearRequestsMap(void) {
    ism_common_HashMapLock(mqcaRequestsMap);
    if(ism_common_getHashMapNumElements(mqcaRequestsMap)) {
        int i = 0;
        ismHashMapEntry ** requests = ism_common_getHashMapEntriesArray(mqcaRequestsMap);
        while(requests[i] != ((void*)-1)) {
            mqcAdminRequest_t * request = (mqcAdminRequest_t*) requests[i]->value;
            TRACE(3, "Warning: Clear MQConnectivity admin request map for object name %s. MQC process terminate state: %d\n", request->objName?request->objName:"", mqcTerminated);
            // ism_common_setError(ISMRC_MQCProcessError);
            ism_confg_rest_createErrMsg(request->http, ISMRC_MQCProcessError, NULL, 0);
            if ( request->callback )
                request->callback(request->http, ISMRC_MQCProcessError);
            ism_common_removeHashMapElement(mqcaRequestsMap, requests[i]->key, sizeof(int));
            i++;
        }
        ism_common_freeHashMapEntriesArray(requests);

    }
    ism_common_HashMapUnlock(mqcaRequestsMap);
}

/*
 */
static int mqcClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    pthread_spin_lock(&mqcAdminChannel.lock);
    TRACE(5, "mqcClosing: transport=%p rc=%d(%s)  useCount=%d state=%d\n",
            transport, rc, reason, mqcAdminChannel.useCount, mqcAdminChannel.state);
    if (mqcAdminChannel.state == CHANNEL_CONNECTION_CLOSED) {
        pthread_spin_unlock(&mqcAdminChannel.lock);
        return 0;
    }
    if (mqcAdminChannel.state == CHANNEL_CONNECTION_IN_PROCESS) {
        int closed = 0;
        mqcAdminChannel.state = CHANNEL_CONNECTION_CLOSED;
        if (mqcTerminated) {
            mqcAdminChannel.transport = NULL;
            closed = 1;
        }
        pthread_spin_unlock(&mqcAdminChannel.lock);
        if (closed)
            transport->closed(transport);
        return 0;
    }
    mqcAdminChannel.state = CHANNEL_CONNECTION_CLOSED;
    mqcAdminChannel.useCount--;
    if (mqcAdminChannel.useCount) {
        pthread_spin_unlock(&mqcAdminChannel.lock);
        return 0;
    }
    mqcAdminChannel.transport = NULL;
    pthread_spin_unlock(&mqcAdminChannel.lock);
    transport->closed(transport);
    clearRequestsMap();
    handleControlChannelClose(3000000000);
    return 0;
}

static void onAdminChannelOpen(void) {
    char xbuf[8192];
    concat_alloc_t buf = {xbuf, sizeof(xbuf), 8};
    const char * locale = ism_common_getLocale();
    uint8_t len = (uint8_t) strlen(locale);
    buf.buf[buf.used++] = len;
    ism_common_allocBufferCopyLen(&buf, locale, len);

    /* Add initial configuration */
    ism_config_addInitialMQCConfiguration(&buf);

    ism_transport_t * transport = getMQCAdminChannel();
    TRACE(5, "onAdminChannelOpen: transport=%p\n", transport);
    if(transport) {
        if(transport->send(transport, buf.buf+8, buf.used-8, 0, SFLAG_FRAMESPACE) == SRETURN_BAD_STATE) {
            killMQCProcess();
            transport->close(transport, ISMRC_ClosedOnSend, 0, "Send data failed");
        }
        freeMQCAdminChannel();
    }
    ism_common_freeAllocBuffer(&buf);
}

/*
 *
 */
static int mqcChannelConnected(ism_transport_t * transport, int rc) {
    int isOpen = 0;
    int oldState, newState;
    pthread_spin_lock(&mqcAdminChannel.lock);
    oldState = mqcAdminChannel.state;
    if ((rc == 0) && (mqcAdminChannel.state == CHANNEL_CONNECTION_IN_PROCESS)) {
        mqcAdminChannel.state = CHANNEL_CONNECTED;
        mqcAdminChannel.useCount = 1;
        transport->ready = 1;
        isOpen = 1;
    } else {
        transport = mqcAdminChannel.transport;
        mqcAdminChannel.transport = NULL;
        mqcAdminChannel.state = CHANNEL_CONNECTION_CLOSED;
    }
    newState = mqcAdminChannel.state;
    pthread_spin_unlock(&mqcAdminChannel.lock);
    TRACE(5, "mqcChannelConnected: transport=%p rc=%d isOpen=%d oldState=%d newState=%d\n",
            transport, rc, isOpen, oldState, newState);
    if (isOpen) {
        onAdminChannelOpen();
    } else {
        if (transport) {
            transport->closed(transport);
            handleControlChannelClose(3000000000);
        }
    }
    return 0;
}



static int adminReplyReceive(ism_transport_t * transport, char * buf, int buflen, int corID) {
    int rc = (int) ntohl(*((int*)buf));
    buf += 4;
    ism_admin_mqc_receive(corID, rc, buf,buflen);
    return 0;
}

/*
 *
 */
static int mqcInitChannelTransport(ism_transport_t * transport) {
    transport->actionname = NULL;
    transport->protocol = "adminClient"; /* Make the string constant */
    transport->protocol_family = "admin"; /* Constant string */
    transport->endpoint_name = "MQCAdmin";
    transport->client_addr="";
    transport->server_addr=mqcServer;
    transport->clientport = 0;
    transport->serverport = 0;
    transport->originated = 2;      /* Allow us to set clientport and tid */
    transport->receive = adminReplyReceive;
    transport->name = "mqcAdmin";
    transport->tid = 0;
    transport->connected = mqcChannelConnected;
    transport->closing = mqcClosing;
    return 0;
}

/*
 *
 */
static int mqcStartControlChannelTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    uint32_t sqn = (uint32_t)((uintptr_t)userdata);
    startMqcAdminChannel(sqn);
    ism_common_cancelTimer(timer);
    return 0;
}


/*
 * Start a connection to MQ Connectivity process
 */
static int startMqcAdminChannel(uint32_t sqn) {
    ism_transport_t * transport = NULL;
    pthread_spin_lock(&mqcAdminChannel.lock);
    if (mqcTerminated) {
        pthread_spin_unlock(&mqcAdminChannel.lock);
        return 0;
    }
    if(mqcProcInfo && mqcProcInfo->timer && (mqcProcInfo->sqn == sqn)) {
        transport = newOutgoingFptr(&mqcaEndpoint, 1);
        mqcInitChannelTransport(transport);
        mqcAdminChannel.transport = transport;
        mqcAdminChannel.state = CHANNEL_CONNECTION_IN_PROCESS;
        mqcProcInfo->timer = NULL;
    }
    pthread_spin_unlock(&mqcAdminChannel.lock);
    if(transport) {
        /* Start the control connection to the plug-in process */
        TRACE(5, "Start outgoing control connection with MQ Connectivity process: transport=%p\n", transport);
        int rc = connectFptr(transport, NULL, mqcServer, 0, NULL);
        if(rc) {
            TRACE(4, "Creation of outgoing control connection with MQ Connectivity process failed: transport=%p rc=%d\n", transport, rc);
        }
    }
    return 0;
}

static ism_transport_t * getMQCAdminChannel(void) {
    ism_transport_t * transport = NULL;
    int useCount;
    pthread_spin_lock(&mqcAdminChannel.lock);
    if(LIKELY(mqcAdminChannel.transport && (mqcAdminChannel.state == CHANNEL_CONNECTED))) {
        transport = mqcAdminChannel.transport;
        mqcAdminChannel.useCount++;
    }
    useCount = mqcAdminChannel.useCount;
    pthread_spin_unlock(&mqcAdminChannel.lock);
    TRACE(5, "getMQCAdminChannel: transport=%p useCount=%d\n",transport, useCount);
    return transport;
}

static void freeMQCAdminChannel(void) {
    ism_transport_t * transport = NULL;
    ism_transport_t * t1 = NULL;
    int useCount;
    pthread_spin_lock(&mqcAdminChannel.lock);
    mqcAdminChannel.useCount--;
    t1 = mqcAdminChannel.transport;
    if(mqcAdminChannel.useCount == 0) {
        transport = mqcAdminChannel.transport;
        mqcAdminChannel.transport = NULL;
    }
    useCount = mqcAdminChannel.useCount;
    pthread_spin_unlock(&mqcAdminChannel.lock);
    TRACE(5, "freeMQCAdminChannel: transport=%p useCount=%d\n",t1, useCount);
    if(UNLIKELY(transport != NULL)) {
        TRACE(4, "freeMQCAdminChannel: complete transport closing for mqcAdminChannel transport=%p connection=%u\n", transport, transport->index);
        transport->closed(transport);
        clearRequestsMap();
        handleControlChannelClose(3000000000);
    }
}


/*
 * Initialize the channel to MQ Connectivity process
 */
int ism_admin_init_mqc_channel(void) {
    int  rc = ISMRC_OK;
    const char * tf = ism_common_getStringConfig("TraceFolder");
    if(tf)
        traceFolder = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tf);
    newOutgoingFptr = (newOutgoing_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_newOutgoing_fnptr", 0L);
    connectFptr = (connect_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_connect_fnptr", 0L);

    if((newOutgoingFptr == NULL) || (connectFptr == NULL))
        return ISMRC_Error;

    pthread_spin_init(&mqcAdminChannel.lock, 0);
    TRACE(4, "Initialize MQC Admin Channel\n");
    mqcaRequestsMap = ism_common_createHashMap(256, HASH_INT32);

    return rc;
}

static pid_t  createExternalProcess(externalProcessInfo_t * procInfo, const char * cmd) {
    char * argv[64] = {NULL};
    char * env[64] = {NULL};
    /* char affMap[CPU_SETSIZE] = {0}; */
    char * value;
    int err;
    /* int affLen; */
    const char * cfg;
    int index = 1;
    int varindex = 0;
    pid_t pid;
    TRACE(5, "createExternalProcess - entry: procInfo=%p cmd=%s\n", procInfo, cmd);
    argv[0] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cmd);
    cfg = ism_common_getStringConfig("ConfigDir");
    if(cfg) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"-c");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cfg);
    }
    if (mqcTaskSet[0]){
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"-s");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),mqcTaskSet);
    }

    // Make sure the external process has some specific environment variables
    value = getenv("HOME");
    if (value) {
        char buffer[strlen(value)+strlen("HOME")+2];
        sprintf(buffer, "HOME=%s", value);
        env[varindex++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buffer);
    }
    value = getenv("LANG");
    if (value) {
        char buffer[strlen(value)+strlen("LANG")+2];
        sprintf(buffer, "LANG=%s", value);
        env[varindex++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buffer);
    }
    value = getenv("QUALIFY_SHARED");
    if (value) {
        char buffer[strlen(value)+strlen("QUALIFY_SHARED")+2];
        sprintf(buffer, "QUALIFY_SHARED=%s", value);
        env[varindex++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),buffer);
    }
    pid = vfork();
    if(!pid) {
        // Child process
        char logFile[256];
        sprintf(logFile, "%s/mqcStartup.log", traceFolder);
        int fd = open(logFile, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        execve(argv[0], argv, env);
        _exit(errno);
    }
    err = errno;
    if(pid < 0) {
        ism_common_setErrorData(ISMRC_SysCallFailed, "%s%d%s", "vfork", err, strerror(err));
    }
    for(--index; index >= 0; --index) {
        if(argv[index])
            ism_common_free(ism_memory_admin_misc,argv[index]);
    }
    for(--varindex; varindex >= 0; --varindex) {
        ism_common_free(ism_memory_admin_misc,env[varindex]);
    }
    TRACE(5, "createExternalProcess - exit: procInfo=%p pid=%d\n", procInfo, pid);
    return pid;
}


static int removeSocketFile(void) {
    struct stat sbuf;
    int rc = stat(mqcServer, &sbuf);
    if(rc == 0) {
        if((sbuf.st_mode & S_IFMT) == S_IFSOCK) {
            TRACE(6, "MQC Admin Socket file %s exists\n", mqcServer);
            rc = unlink(mqcServer);
            if (rc) {
                TRACE(3, "Unable to delete MQC Admin socket file: %s. Error: %d (%s) \n",mqcServer, rc, strerror(rc));
                return ISMRC_PortInUse;
            }
        } else {
            TRACE(3, "MQC Admin Socket file %s already exists with non-socket mode %u\n", mqcServer, sbuf.st_mode);
            return ISMRC_PortInUse;
        }
    }
    return 0;
}

static void * externalProcessorMonitor(void * parm, void * context, int value) {
    externalProcessInfo_t * procInfo = (externalProcessInfo_t*)parm;
    const char * cmd = (const char *) context ;
    int shouldNotify = 1;
    if(removeSocketFile()) {
        TRACE(5, "externalProcessorMonitor - before pthread_barrier_wait: procInfo=%p\n", procInfo);
        pthread_barrier_wait(&procInfo->barrier);
        TRACE(5, "externalProcessorMonitor - after pthread_barrier_wait: procInfo=%p\n", procInfo);
        return NULL;
    }
//    procInfo->thread = pthread_self();
    TRACE(5, "externalProcessorMonitor: procInfo=%p terminated=%d\n", procInfo, procInfo->terminated);
    pthread_mutex_lock(&procInfo->lock);
    while(!procInfo->terminated) {
    	pid_t pid = createExternalProcess(procInfo, cmd);
        if(shouldNotify) {
            TRACE(5, "externalProcessorMonitor - before pthread_barrier_wait: procInfo=%p\n", procInfo);
            pthread_barrier_wait(&procInfo->barrier);
            TRACE(5, "externalProcessorMonitor - after pthread_barrier_wait: procInfo=%p\n", procInfo);
        }
        shouldNotify = 0;
        if(pid > 0) {
            int status = 0;
            int i;
            procInfo->pid = pid;
            pthread_mutex_unlock(&procInfo->lock);
            waitpid(pid, &status, 0);
            pthread_mutex_lock(&procInfo->lock);
            TRACE(5, "externalProcessorMonitor - external process terminated: procInfo=%p pid=%d status=%d\n", procInfo, pid, status);
            procInfo->pid = 0;
            for(i = 0; i < 10; i++) {
                usleep(100000);
                if(removeSocketFile())
                    continue;
                break;
            }
            continue;
        }
        break;
    }
    if(shouldNotify) {
        TRACE(5, "externalProcessorMonitor - before pthread_barrier_wait: procInfo=%p\n", procInfo);
        pthread_barrier_wait(&procInfo->barrier);
        TRACE(5, "externalProcessorMonitor - after pthread_barrier_wait: procInfo=%p\n", procInfo);
    }
    pthread_mutex_unlock(&procInfo->lock);
    TRACE(5, "externalProcessorMonitor - exit: procInfo=%p terminated=%d\n", procInfo, procInfo->terminated);
    return NULL;
}

/*
 * Start the MQ Connectivity admin channel
 */
int ism_admin_start_mqc_channel(void) {
    TRACE(5, "ism_admin_start_mqc_channel: mqcTerminated=%d\n", mqcTerminated);
    pthread_spin_lock(&mqcAdminChannel.lock);
    if(mqcTerminated) {
        mqcTerminated = 0;
        assert(mqcProcInfo == NULL);
        mqcProcInfo = initExternalProcInfo();
        mqcProcInfo->sqn = mqcPiSqn++;
        ism_common_startThread(&mqcProcInfo->thread, externalProcessorMonitor, mqcProcInfo,
                IMA_SVR_INSTALL_PATH "/bin/startMQCBridge.sh", 0, ISM_TUSAGE_NORMAL, 0, "MQCMon", "MQC Bridge Process Monitor");
        pthread_barrier_wait(&mqcProcInfo->barrier);
        mqcProcInfo->timer =
                ism_common_setTimerOnce(ISM_TIMER_HIGH, mqcStartControlChannelTimer, (void*)((uintptr_t)mqcProcInfo->sqn), 1000000000);  /* 1 second */
    }
    pthread_spin_unlock(&mqcAdminChannel.lock);
    return 0;
}

int ism_admin_stop_mqc_channel(void) {
    TRACE(5, "ism_admin_stop_mqc_channel: mqcTerminated=%d\n", mqcTerminated);
    pthread_spin_lock(&mqcAdminChannel.lock);
    if(!mqcTerminated) {
        mqcTerminated = 1;
        if(mqcProcInfo)
            mqcProcInfo->terminated = 1;
        pthread_spin_unlock(&mqcAdminChannel.lock);
        ism_transport_t * transport = getMQCAdminChannel();
        if(transport) {
            transport->close(transport, ISMRC_OK, 0,"MQ Connectivity was terminated");
            freeMQCAdminChannel();
        }
        pthread_spin_lock(&mqcAdminChannel.lock);
        if(mqcProcInfo) {
            void * result = NULL;
            pthread_mutex_lock(&mqcProcInfo->lock);
            mqcProcInfo->terminated = 1;
            if(mqcProcInfo->pid) {
                kill(mqcProcInfo->pid, SIGKILL);
            }
            pthread_mutex_unlock(&mqcProcInfo->lock);
            pthread_join(mqcProcInfo->thread, &result);
        }
        destroyExternalProcInfo(mqcProcInfo);
        mqcProcInfo = NULL;
    }
    pthread_spin_unlock(&mqcAdminChannel.lock);
    return 0;
}

/* Send a message to MQC process */
int ism_admin_mqc_send(char * buff, int length, ism_http_t * http, ism_rest_api_cb callback, int persistData, int objID, char *objName) {
    ism_transport_t * transport = getMQCAdminChannel();
    if(transport) {
        int correlationID;
        char xbuf[8192];
        concat_alloc_t buf = {xbuf, sizeof(xbuf), 8};
        char *bufSent = NULL;
        const char * locale = http->locale;
        if ( !locale ) locale = "en_US";
        uint8_t len = (uint8_t) strlen(locale);
        buf.buf[buf.used++] = len;
        ism_common_allocBufferCopyLen(&buf, locale, len);
        ism_common_allocBufferCopyLen(&buf, buff, length);

        mqcAdminRequest_t * request = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,561),sizeof(mqcAdminRequest_t));
        if(request == NULL) {
            freeMQCAdminChannel();
            ism_common_freeAllocBuffer(&buf);
            return ISMRC_AllocateError;
        }
        request->http = http;
        request->callback = callback;
        request->persistData = persistData;
        request->objID = objID;
        request->objName = NULL;

        if ( objName )  {
            request->objName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),objName);
        } else {
            if ( persistData == 1 ) {
                bufSent = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,562),length+1);
                memcpy(bufSent, buff, length);
                bufSent[length] = 0;
                request->bufSent = bufSent;
            }
        }
        ism_common_HashMapLock(mqcaRequestsMap);
        correlationID = mqcaCorrelationID++;
        ism_common_putHashMapElement(mqcaRequestsMap, &correlationID, sizeof(int), request, NULL);
        ism_common_HashMapUnlock(mqcaRequestsMap);

        // TRACE(9, "Send msg to mqc: len=%d\n", buf.used-8);

        if(transport->send(transport, buf.buf+8, buf.used-8, correlationID, SFLAG_FRAMESPACE) == SRETURN_BAD_STATE) {
            killMQCProcess();
            transport->close(transport, ISMRC_ClosedOnSend, 0, "Send data failed");
        }
        freeMQCAdminChannel();
        ism_common_freeAllocBuffer(&buf);
        return ISMRC_OK;
    }

    return ISMRC_MQCProcessError;
}

/* Receive response of the message sent to MQC process using ism_admin_mqc_send() */
int ism_admin_mqc_receive(int correlationID, int rc, char * buf, int buflen) {

    mqcAdminRequest_t * request = (mqcAdminRequest_t*)ism_common_removeHashMapElementLock(mqcaRequestsMap, &correlationID, sizeof(int));

    if (request) {
        int rc1 = ISMRC_OK;

        ism_http_t *http = request->http;
        http->outbuf.used = 0;
        memset(http->outbuf.buf, 0, http->outbuf.len);

        if ( rc == ISMRC_OK && request->persistData == 1 ) {
            /* check if object needs to be deleted */
            if ( request->objName ) {
                rc1 = ism_config_json_delMQCObject(request->objID, request->objName);
                ism_common_free(ism_memory_admin_misc,request->objName);
            } else {
                rc1 = ism_config_json_addMQCObject(request->bufSent);
                ism_common_free(ism_memory_admin_misc,request->bufSent);
            }
            int getLock = 1;
            if ( rc1 == ISMRC_OK ) {
                ism_config_json_updateFile(getLock);
            }
        }

        if ( request->callback ) {
			if ( rc1 != ISMRC_OK ) {
				/* need to update return message before invoking http callback */
				const char * repl[1];
				int replSize = 0;
				ism_confg_rest_createErrMsg(http, rc1, repl, replSize);
				rc = rc1;
			} else {
				ism_common_allocBufferCopyLen(&request->http->outbuf, buf, buflen);
			}

            request->callback(request->http, rc);
        }

        ism_common_free(ism_memory_admin_misc,request);
    }

    return 0;
}

int ism_admin_getMQCStatus(void) {
    return ((ism_config_getMQConnEnabled()) && (mqcAdminChannel.transport != NULL) && (mqcAdminChannel.state == CHANNEL_CONNECTED));
}


/**
 * Initialize mqc admin lock and condition variables
 */
void ism_admin_mqc_init(void) {
    char affMap[CPU_SETSIZE] = {0};
    int affLen = ism_config_autotune_getaffinity("mqcBridge",affMap);
    if (affLen){
        int j;
        for(j = 0; j < affLen; j++) {
            if(affMap[j]){
                char cpu[16];
                sprintf(cpu,"%d,",j);
                strcat(mqcTaskSet,cpu);
            }
        }
        mqcTaskSet[strlen(mqcTaskSet)-1] = '\0';
    }

    pthread_mutex_init(&mqcAdminLock, 0);
    pthread_cond_init(&mqcAdminCond, 0);
}

/**
 * To make mqc admin wait to receive a signal to manage mqc server administrative
 * functions.
 */
int ism_admin_mqc_pause(void) {
    int32_t rc = ISMRC_OK;
    TRACE(3, "Initialize ism_admin_mqc_pause()\n");

    if ( mqcPausedSignaled == 1 ) return rc;

    for (;;) {
        mqcAdminPauseInited = 1;

        pthread_mutex_lock(&mqcAdminLock);
        pthread_cond_wait(&mqcAdminCond, &mqcAdminLock);

        if ( mqcAdminPauseState == ISM_ADMIN_STATE_CONTINUE ) {
            TRACE(5, "ism_admin_mqc_pause: state is ADMIN_STATE_CONTINUE.\n");
            /* This state is set when mqcbridge process successfully connected to
             * imaserver server using domain sockets based channel for admin work.
             * Set next state to ADMIN_STATE_PAUSE - required so that we can
             * handle imaserver stop.
             */
            mqcAdminPauseState = ISM_ADMIN_STATE_PAUSE;
            pthread_mutex_unlock(&mqcAdminLock);
            break;
        }

        if ( mqcAdminPauseState == ISM_ADMIN_STATE_STOP ) {
            TRACE(2, "ism_admin_mqc_pause: initial configuration is transferred.\n");
            rc = ISMRC_Closed;
            pthread_mutex_unlock(&mqcAdminLock);
            break;
        }

        TRACE(5, "ism_admin_mqc_pause is signaled. Pause state %d is not processed.\n", mqcAdminPauseState);
        pthread_mutex_unlock(&mqcAdminLock);
    }
    mqcAdminPauseInited = 0;
    return rc;
}

/**
 * Send a signal to continue or stop mqc process
 */
int ism_admin_mqc_send_signal(int type, int mode) {
    int rc = ISMRC_Error;
    if ( type == ISM_ADMIN_STATE_STOP ) {
        rc = ISMRC_OK;
        pthread_mutex_lock(&mqcAdminLock);
        mqcAdminPauseState = mode;
        pthread_cond_signal(&mqcAdminCond);
        pthread_mutex_unlock(&mqcAdminLock);
    } else if ( type == ISM_ADMIN_STATE_CONTINUE) {
        rc = ISMRC_OK;
        pthread_mutex_lock(&mqcAdminLock);
        mqcAdminPauseState = ISM_ADMIN_STATE_CONTINUE;
        pthread_cond_signal(&mqcAdminCond);
        pthread_mutex_unlock(&mqcAdminLock);
    }
    return rc;
}
