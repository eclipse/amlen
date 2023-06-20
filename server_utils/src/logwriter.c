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
/************************************************************************/
/*                                                                      */
/* Module Name: logwriter.c                                             */
/*                                                                      */
/* Description:                                                         */
/*        Contains implementations for log writer.  This logs to either */
/*      a file or syslog, and writes to the trace.                      */
/*                                                                      */
/************************************************************************/
#define TRACE_COMP Util
#include <ismutil.h>
#include <log.h>
#include <syslog.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void ism_common_checkTimezone(ism_ts_t * ts);

//
/*Default port for Bedrock syslog-ng*/
static  ism_ts_t * g_ts = NULL;
static  int        g_ts_hour = 0;
static  int        g_isProxy;


extern char         g_procname[20];
extern pthread_mutex_t * sharedProcessLock;

extern ism_logWriter_t * g_logwriter[LOGGER_Max+1];
static ismSyslogConnection_t * syslogConnection = NULL;

int ism_log_closeSysLogConnection(ismSyslogConnection_t * syslog_conn);

/*
 * Initial value of global log level
 */
ISM_LOGLEV g_loglevel = ISM_LOGLEV_INFO;

#define NANOS_IN_HOUR 3600000000000L

/*
 * Reset the timestamp used for trace
 */
void ism_common_resetLogTimestamp(void) {
    g_ts_hour = 0;
}

/*
 * Build log message for trace and log
 */
static int build_log_msg(const ismLogOut_t * logout, int pri, const char * msgf, char * msg, int msg_size) {
    int        len;
    int        mlen;
    char tbuf[64];

    if (!g_ts) {
        g_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        g_ts_hour = ism_common_currentTimeNanos() / NANOS_IN_HOUR;
    } else {
        /*
         * The timezone can change every hour.  While this is unlikely, we
         * check it here.
         */
        int hour = (int)(logout->timestamp/NANOS_IN_HOUR);
        if (hour != g_ts_hour) {
            ism_common_checkTimezone(g_ts);
            g_ts_hour = (int)(logout->timestamp / NANOS_IN_HOUR);
        }
    }
    ism_common_setTimestamp(g_ts, logout->timestamp);
    ism_common_formatTimestamp(g_ts, tbuf, 64, 7, ISM_TFF_ISO8601);

    if (pri != 0) {
        //RFC 5424 format with structure elements
        len = (int)snprintf(msg, msg_size, "<%u>1 %s %s %s %i %s %s ",
                pri, tbuf, ism_common_getHostnameInfo(), g_procname,
                getpid(), logout->msgid, logout->sdelements);
    } else {
        len = (int)snprintf(msg, msg_size, "%s %-12s %-10s %s %s: ", tbuf,
                ism_log_getCategoryName(logout->category), logout->threadName, logout->msgid,
                ism_log_getLogLevelCode(logout->loglevel));
    }
    if (len < 0 || len > msg_size) {
        return msg_size+logout->size+512;
    }
    mlen = ism_common_formatMessage(msg+len, msg_size-len-1, msgf, logout->repl, logout->count);
    return len+mlen;
}

static int info_trace = 6;

/*
 * Tell logging this is the proxy
 */
void ism_log_setExeType(int type) {
    if (type)
        info_trace = 5;
    g_isProxy = type;
}

int ism_common_isServer(void) {
    return g_isProxy == 0;
}

int ism_common_isProxy(void) {
    return g_isProxy == 1;
}

int ism_common_isBridge(void) {
    return g_isProxy == 2;
}

/*
 * Get ISM Trace Level from the ISM Log level
 */
int ism_log_getTraceLevelForLog(int level) {
    switch(level) {
    case ISM_LOGLEV_CRIT:
    case ISM_LOGLEV_ERROR:
        return 1;
    case ISM_LOGLEV_WARN:
        return 4;
    case ISM_LOGLEV_NOTICE:
        return 5;
    case ISM_LOGLEV_INFO:
        return info_trace;
    default:
        return 7;
    }
}


/*
 * Log into ISM Trace component
 */
void ism_log_toTrace(const ismLogOut_t * le) {
    char * buf;
    int    buflen = le->size + 256;
    int    inheap = 0;
    int    level = ism_log_getTraceLevelForLog(le->loglevel);
    int    bytes_needed;

    if (level <= ism_defaultTrace->trcComponentLevels[TRACECOMP_System]) {
        if (buflen < 8192) {
            buf = alloca(buflen);
        } else {
            buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,142),buflen);
            inheap = 1;
        }
        bytes_needed = build_log_msg(le, 0, le->msgf, buf, buflen);
        if (bytes_needed > buflen) {
            if (inheap)
                buf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_log,143),buf, bytes_needed);
            else {
                if (bytes_needed < 8192 - buflen) {
                    buf = alloca(bytes_needed);
                } else {
                    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,144),bytes_needed);
                    inheap = 1;
                }
            }
            buflen = bytes_needed;
            build_log_msg(le, 0, le->msgf, buf, buflen);
        }
        TRACE(6, "Log logid=%d from %s at %s:%d\n", le->msgnum, le->func, le->location, le->lineno);
        traceFunction(6, 0, le->location, le->lineno, "%s\n", buf);
        if (inheap)
            ism_common_free(ism_memory_utils_log,buf);
    }
}


/*****************************************************************/
/* Log to File                                                    */
/*****************************************************************/

/*
 * Used for CUnit test
 */
int ism_log_setWriterCallback(ism_logWriter_t * lw, void (*callback)(const char *) ) {
    lw->desttype = DESTTYPE_CALLBACK;
    lw->file = (FILE *)callback;
    return 0;
}

/*
 * Set the destination for the log writer
 */
int ism_log_setWriterDestination(ism_logWriter_t * lw, const char * destination) {
    FILE * f;
    int    isfile = 0;
    int    rc = 0;

    lw->desttype = 2;
    /* If the destination names match, do nothing */
    if (!lw->destination || strcmp(destination, lw->destination)) {

        /*
         * Open the file with special processing for stdout and stderr.
         */
        if (!strcmp(destination, "stdout")) f = stdout;
        else if (!strcmp(destination, "stderr")) f = stderr;
        else {
            f = fopen(destination, lw->overwrite ? "wb" : "ab");
            isfile = 1;              /* We opened the file so we must close it */
        }
        if (!f) {
            rc = errno;
        } else {
            FILE * oldFile = lw->file;
            char * oldDestination = lw->destination;
            int oldIsFile = lw->isfile;

            lw->file = f;
            lw->isfile = isfile;
            lw->destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_log,1000),destination);

            if (oldIsFile) {
                fclose(oldFile);
                ism_common_free(ism_memory_utils_log,oldDestination);
            }
        }
    }
    return rc;
}

/*
 * Close the log writer
 */
void ism_log_closeWriter(ism_logWriter_t * lw) {
    if (lw) {
        ism_common_free(ism_memory_utils_log,lw);
        lw = NULL;
    }
}

/**
 * Get Syslog Level from ISM Log Level
 */
static int getSysLogSeverity(ISM_LOGLEV level) {
    switch(level){
    case ISM_LOGLEV_CRIT:
        return LOG_CRIT;
    case ISM_LOGLEV_ERROR:
        return LOG_ERR;
    case ISM_LOGLEV_WARN:
        return LOG_WARNING;
    case ISM_LOGLEV_NOTICE:
        return LOG_NOTICE;
    case ISM_LOGLEV_INFO:
    default:
        return LOG_INFO;

    }
}


/*
 * Log into Log File or SysLog
 */
void ism_log_toISMLogger(ism_logWriter_t * lw, int kind, ismLogOut_t * logout) {
    char msgx[1024];
    const char * msgf;
    char * buf;
    int    buflen = logout->size + 256;
    int    inheap = 0;
    int    mlen;
    int    bytes_needed;
    int    rc;

    /*Perform Filtering*/
    if (ism_log_filter(lw->level+kind, logout->loglevel, logout->category, logout->msgnum)==0) {
        return;
    }

    /*
     * Get the message
     */
    msgf = ism_common_getMessage(logout->msgid, msgx, sizeof(msgx), &mlen);
    if (msgf) {
        buflen += strlen(msgf);
    } else {
        msgf = logout->msgf;
    }
    if (lw->desttype == DESTTYPE_SYSLOG && buflen > 6000)
        buflen = 6000;

    /*
     * Create the buffer and build the log message
     */
    if (buflen < 8192) {
        buf = alloca(buflen);
    } else {
        buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,148),buflen);
        inheap = 1;
    }
    if (lw->desttype == DESTTYPE_FILE) {
        bytes_needed = build_log_msg(logout, 0, msgf, buf, buflen);
        if (bytes_needed > buflen) {
            if (inheap)
                buf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_log,149),buf, bytes_needed);
            else {
                if (bytes_needed < 8192 - buflen) {
                    buf = alloca(bytes_needed);
                } else {
                    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,150),bytes_needed);
                    inheap = 1;
                }
            }
            buflen = bytes_needed;
            build_log_msg(logout, 0, msgf, buf, buflen);
        }

        /* sharedProcessLock might not be set in cunit */
        if (sharedProcessLock) {
            rc = pthread_mutex_lock(sharedProcessLock);
            if (rc == EOWNERDEAD) {
                pthread_mutex_consistent(sharedProcessLock);
            }
        }
        fprintf(lw->file, "%s\n", buf);
        fflush(lw->file);
        if (sharedProcessLock) {
            pthread_mutex_unlock(sharedProcessLock);
        }
    } else if (lw->desttype == DESTTYPE_SYSLOG) {
        int pri = lw->facility * 8 + getSysLogSeverity(logout->loglevel);
        bytes_needed = build_log_msg(logout, pri, msgf, buf, buflen);
        if (bytes_needed > buflen) {
            if (inheap)
                buf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_log,151),buf, bytes_needed);
            else {
                if (bytes_needed < 8192 - buflen) {
                    buf = alloca(bytes_needed);
                } else {
                    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,152),bytes_needed);
                    inheap = 1;
                }
            }
            buflen = bytes_needed;
            build_log_msg(logout, pri, msgf, buf, buflen);
        }
        ism_log_logSysLog(lw, logout, buf);
    } else if (lw->desttype == DESTTYPE_CALLBACK) {
        void (*callback)(const char * msg) = (void (*)(const char * msg))lw->file;
        build_log_msg(logout, 0, msgf, buf, buflen);
        callback(buf);
    }
    if (inheap)
        ism_common_free(ism_memory_utils_log,buf);
}

/*
 * Filter the log event based on the log writer filter.
 * The log formatter has already filtered before we get here.
 * The log writer can have several filter for different dispsitions such
 * as log or monitor.
 */
static int shouldLog(ism_logFilter_t * lw, ISM_LOGLEV level, uint32_t category, uint32_t msg) {
    int  i;

    if (level > (int)lw->level) {
        /*
         * If the log point level is greater the writer level,
         * Check if any added categories that allow this category
         * and log level to log.
         */
        for (i=0; i<(int)lw->addCatCount; i++) {
            if (category == lw->addCatFilterItem[i].category &&
                    level <= lw->addCatFilterItem[i].level)
                return 1;
        }
        for (i=0; i<(int)lw->addMsgCount; i++) {
            if (msg == lw->addMsgFilterItem[i].msgid)
                return 1;
        }
        return 0;

    } else {
        /*
         * If There are ADDED CATEGORIES, then EXCLUDES all other categories
         */
        if (lw->addCatCount>0){
            for (i=0; i<(int)lw->addCatCount; i++) {
                if (category == lw->addCatFilterItem[i].category &&
                        level <= lw->addCatFilterItem[i].level)
                    return 1;
            }
            for (i=0; i<(int)lw->addMsgCount; i++) {
                if (msg == lw->addMsgFilterItem[i].msgid)
                    return 1;
            }
            return 0;
        } else if (lw->delCatCount>0) {
            /*
             * If there are DELETED CATEGORIES and NO Added categories.
             * Assume to INCLUDE all other categories.
             */
            for (i=0; i<(int)lw->delCatCount; i++) {
                if (category == lw->delCatFilterItem[i].category&&
                        level >= lw->delCatFilterItem[i].level)
                    return 0;
            }
            for (i=0; i<(int)lw->delMsgCount; i++) {
                if (msg == lw->delCatFilterItem[i].msgid)
                    return 0;
            }
            return 1;
        } else {
            for (i=0; i<(int)lw->delMsgCount; i++) {
                if (msg == lw->delCatFilterItem[i].msgid)
                    return 0;
            }
            return 1;
        }

        /*If no filter, return true to log*/
        return 1;
    }

}

/*
 * Get the next token in the filter string.g
 * Tokens are separated by any number of spaces and commas.
 * Tokens cannot be more than 31 characters long.
 */
static const char * nexttoken(const char * str, char * token) {
    int len = 1;
    char * tp = token;
    while (*str && (*str==' ' || *str==','))
        str++;
    while (*str && *str!=' ' && *str!=',') {
        if (len++ < 32)
            *tp++ = *str;
        str++;
    }
    *tp = 0;
    return str;
}

/*
 * Filter the log entry
 */
int ism_log_filter(ism_logFilter_t * lw, const ISM_LOGLEV level, int32_t category, int32_t msgnum) {
    /*
     * If it should be logged, it passes the filter
     */
    if (shouldLog(lw, level, category, msgnum)) {
        return 8;
    }

    return 0;
}




/*
 * Open the socket for syslog logging
 * TODO: other destinations
 */
int  ism_log_openSysLogConnection(ismSyslogConnection_t * syslog_conn) {
    int  rc;
    struct addrinfo hints;
    struct addrinfo * result = NULL;
    struct addrinfo * rp;
    char portstr[64];

    if (!syslog_conn || syslog_conn->ip == NULL) {
        TRACE(7, "Cannot establish syslog connection - syslog parameters were not specified");
        return -1;
    }

    memset(&hints, 0, sizeof hints);

    if (strchr(syslog_conn->ip, ':')) {
        syslog_conn->ipv6 = 1;
        hints.ai_family = AF_INET6;

    }else{
        syslog_conn->ipv6 = 0;
        hints.ai_family = AF_INET;

    }

    if (syslog_conn->tcp){
        hints.ai_socktype= SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

    }else{
        hints.ai_socktype= SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
    }
    memset(&portstr, 0, 64);
    sprintf((char *)&portstr,"%d", syslog_conn->port);
    rc = getaddrinfo(syslog_conn->ip,(char *) &portstr, &hints, &result);
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        syslog_conn->sock = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
        if (syslog_conn->sock == -1)
            continue;

        if (!syslog_conn->tcp && rp->ai_protocol == IPPROTO_UDP) {
            /* No Need to call connect for UDP. Just return on socket. */
            if (rp->ai_family == AF_INET) {
                memcpy(&syslog_conn->ipv4addr.sin_addr, &((struct sockaddr_in *)rp->ai_addr)->sin_addr, 4);
                syslog_conn->ipv4addr.sin_port = (in_port_t)ntohs((uint16_t)syslog_conn->port);
                syslog_conn->ipv4addr.sin_family = AF_INET;
            } else if (rp->ai_family == AF_INET6) {

                memcpy(&(syslog_conn->ipv6addr.sin6_addr), &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr, 16);
                syslog_conn->ipv6addr.sin6_port = (in_port_t)ntohs((uint16_t)syslog_conn->port);
                syslog_conn->ipv6addr.sin6_family = AF_INET6;
            }
            break;

        }
        if (syslog_conn->tcp && connect(syslog_conn->sock, rp->ai_addr, rp->ai_addrlen) == 0){
            syslog_conn->isconnected=1;
            break;
        }
        syslog_conn->isconnected=0;
        close(syslog_conn->sock);
    }

    freeaddrinfo (result);

    if (rc < 0 || rp==NULL) {
        TRACE(1, "Unable to open syslog location: %s, port %d. rc=%d. Error: %s\n",
                syslog_conn->ip, syslog_conn->port, rc, strerror(errno));
        return -1;
    }

    int count=0;
    for (count=0; count<LOGGER_Max; count++) {
        if (g_logwriter[count] != NULL && g_logwriter[count]->desttype==DESTTYPE_SYSLOG) {
            g_logwriter[count]->syslog_conn = syslog_conn;
        }
    }
    return 0;
}

/**
 * Initialize syslog configuration from configuration properties.
 *
 * @param props  Configuration properties.
 * @return A return code: 0=good.
 */
int ism_log_initSyslog(ism_prop_t * props) {
    int rc = ISMRC_OK;
    const char * host;
    const char * protocol;
    int port;
    int tcp;

    int isEnabled = ism_common_getIntProperty(props, "Syslog.Enabled.Syslog", 0);
    if (!isEnabled) {
    	// We cannot close the connection is there are syslog log locations
        int count=0;
        for (count=0; count<LOGGER_Max; count++) {
            if (g_logwriter[count] != NULL && g_logwriter[count]->desttype == DESTTYPE_SYSLOG) {
            	return ISMRC_SyslogInUse;
            }
        }

    	if (syslogConnection && syslogConnection->isconnected) {
    		ism_log_closeSysLogConnection(syslogConnection);
    	}
    	return rc;
    }

    host = ism_common_getStringProperty(props, "Syslog.Host.Syslog");
    port = ism_common_getIntProperty(props, "Syslog.Port.Syslog", -1);
    protocol = ism_common_getStringProperty(props, "Syslog.Protocol.Syslog");
    tcp = -1;

    if (protocol) {
        if (!strcmp(protocol, "tcp")) {
            tcp = 1;
        } else {
            tcp = 0;
        }
    }

    // Try to establish syslog connection using specified configuration
    // If successful, update syslogConnection object
    ismSyslogConnection_t * tempConnection = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_log,154),1, sizeof(ismSyslogConnection_t));
    if (host) {
        tempConnection->ip = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_log,1000),host);
    } else if (syslogConnection && syslogConnection->ip) {
        tempConnection->ip = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_log,1000),syslogConnection->ip);
    } else {
        tempConnection->ip = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_log,1000),DUMMY_LINKLOCAL_ADDR);
    }

    if (port != -1) {
        tempConnection->port = port;
    } else if (syslogConnection) {
        tempConnection->port = syslogConnection->port;
    } else {
        tempConnection->port = ISM_SYSLOG_PORT;
    }
    if (tcp != -1) {
        tempConnection->tcp = tcp;
    } else if (syslogConnection) {
        tempConnection->tcp = syslogConnection->tcp;
    } else {
        tempConnection->tcp = 1;
    }

    rc = ism_log_openSysLogConnection(tempConnection);
    if (rc) {
        ism_log_closeSysLogConnection(tempConnection);
        ism_common_free(ism_memory_utils_log,(char*)tempConnection->ip);
        ism_common_free(ism_memory_utils_log,tempConnection);

        return rc;
    }


    if (!syslogConnection && !rc) {
        syslogConnection = tempConnection;

        return rc;
    } else if (!rc
            || (strcmp(tempConnection->ip, syslogConnection->ip))
            || (tempConnection->port != syslogConnection->port)
            || (tempConnection->tcp != syslogConnection->tcp)) {
        // If data has changed, close the connection and free up memory
        ism_log_closeSysLogConnection(syslogConnection);
        syslogConnection->isconnected = 0;

        if (strcmp(tempConnection->ip, syslogConnection->ip)) {
            ism_common_free(ism_memory_utils_log,(char*)syslogConnection->ip);
            syslogConnection->ip = tempConnection->ip;
        }
        syslogConnection->port = tempConnection->port;
        syslogConnection->tcp = tempConnection->tcp;

        rc = ism_log_openSysLogConnection(syslogConnection);
    }

    ism_common_free(ism_memory_utils_log,tempConnection);

    return rc;
}

/*
 * Open the socket for syslog logging
 * TODO: other destinations
 */
int  ism_log_openSysLog(ism_logWriter_t * lw, int which, int facility) {
    int rc = ISMRC_OK;
    if (!syslogConnection) {
        return ISMRC_NotConnected;
    }

    if (!syslogConnection->isconnected) {
        rc = ism_log_openSysLogConnection(syslogConnection);
        if (!rc) {
            sleep(5);
        }
    }

    lw->syslog_conn = syslogConnection;
    lw->desttype = DESTTYPE_SYSLOG;
    lw->facility = facility;
    return rc;
}


/*
 * Insert a filter item
 */
static ismFilterItem_t * insertFilterItem(uint32_t value, uint32_t level, uint32_t * count, uint32_t * max, ismFilterItem_t * vals, int kind) {
    int i;

    if (vals != NULL){
        for (i=0; i < *count; i++) {
            if (kind==FilterItem_Category){
                if (vals[i].category == value)
                    return vals;
            } else {
                if (vals[i].msgid == value)
                    return vals;
            }
        }
    }
    if ((vals != NULL) && (*count < *max)) {
        vals[(*count)].kind = kind;
        vals[(*count)].level = level;
        if (kind == FilterItem_Category) {
            vals[(*count)].category = value;
        } else {
            vals[(*count)].msgid = value;
        }
        (*count)++;
    } else {
        if (*max == 0)
            *max = 32;
        else
            *max = *max * 4;
        vals = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_log,159),vals, *max * sizeof(ismFilterItem_t));

        vals[(*count)].level = level;
        vals[(*count)].kind = kind;
        if (kind == FilterItem_Category) {
            vals[(*count)].category = value;
        } else {
            vals[(*count)].msgid = value;
        }
        (*count)++;
    }

    return vals;
}

/*
 * Create a log filter
 */
int ism_log_createFilter(ism_logFilter_t * lw, const char * filter) {
    char token [32];
    const char * str;
    char *   tokp;
    char     neg = 0;
    char *   eos;
    uint32_t msg;
    int     rc = 0;
    ISM_LOGLEV ismlevel = 5;
    int xrc=0;

    if (!filter)
        return 0;
    str = nexttoken(filter, token);
    while (*token) {
        tokp = token;
        if (*tokp == '-' || *tokp == '+') {
            neg = *tokp;
            tokp++;
        }
        if (*tokp >= '0' && *tokp <= '9') {
            msg = strtoul(tokp, &eos, 10);
            if (msg<1 || msg>9999 || *eos) {
                TRACE(2, "Invalid log filter: %s\n", token);
                rc = -1;
            } else {
                if (neg == '-') {
                    lw->delMsgFilterItem= insertFilterItem(msg,ismlevel, &lw->delMsgCount, &lw->delMsgAlloc, lw->delMsgFilterItem,FilterItem_MsgId);
                } else {
                    lw->addMsgFilterItem= insertFilterItem(msg,ismlevel, &lw->addMsgCount, &lw->addMsgAlloc, lw->addMsgFilterItem,FilterItem_MsgId );
                }
            }
        } else {
            /* Get the category and level. Format category:level */
            char * cattoken = NULL;
            char * leveltoken = NULL;
            cattoken = (char *)ism_common_getToken(tokp, ":", ":", &leveltoken);

            if (leveltoken != NULL) {
                xrc = ism_log_getISMLogLevel((const char *)leveltoken, &ismlevel);
                if (xrc!=0){
                    ismlevel= lw->level;
                }
            } else {
                ismlevel= lw->level;
            }

            msg = ism_log_getCategoryID(cattoken);

            if (msg == 0) {
                TRACE(2, "Invalid log filter category: %s\n", token);
                rc = -1;
            } else {
                if (neg == '-') {
                    lw->delCatFilterItem= insertFilterItem(msg,ismlevel, &lw->delCatCount, &lw->delCatAlloc, lw->delCatFilterItem,FilterItem_Category);
                    lw->level = 5;
                } else {
                    lw->addCatFilterItem= insertFilterItem(msg,ismlevel, &lw->addCatCount, &lw->addCatAlloc, lw->addCatFilterItem,FilterItem_Category);
                    lw->level = 0;
                }
            }
        }
        str = nexttoken(str, token);
    }
    return rc;
}

/*
 * Close the socket for syslog logging
 */
int ism_log_closeSysLogConnection(ismSyslogConnection_t * syslog_conn) {
    if (syslog_conn->sock != -1){
        if (close(syslog_conn->sock)<0 ){
            if (syslog_conn->isconnected) {
                TRACE(2, "Failed to close the socket of syslog connection.\n");
                syslog_conn->isconnected = 0;
                return -1;
            }
        }
    }
    return 0;
}

/*
 * Log to Syslog
 * TODO: other destinations
 */
void ism_log_logSysLog(ism_logWriter_t * lw, ismLogOut_t * logout, char * message) {
    int  len;
    int  tries = 0;
    int  packtsent  = 0;
    char * pos;

    if (!lw->syslog_conn) {
        TRACE(7, "Syslog connection has not been established yet\n");
    }

    /*
     * Fix up the message to make sure it is not greater than 8192 bytes, does not contain
     * a newline, and ends with a newline.
     */
    len = strlen(message);
    if (len > 8190)
        ism_commmon_truncateUTF8(message, 8190);

    if (message[len-1] == '\n') {
        message[--len] = 0;
    }
    pos = strchr(message, '\n');
    if (pos) {
        while (*pos) {
            if (*pos == '\n')
                *pos = '\r';
            pos++;
        }
    }
    message[len++] = '\n';
    do {
        if (syslogConnection->tcp && !syslogConnection->isconnected){
            while(!syslogConnection->isconnected){
                ism_log_openSysLogConnection(syslogConnection);
                sleep(5);
            }
        }
        if (syslogConnection->ipv6) {
            packtsent = sendto (syslogConnection->sock, message, len, MSG_NOSIGNAL,
                    (struct sockaddr *) &syslogConnection->ipv6addr, sizeof(syslogConnection->ipv6addr));

            if (packtsent<0 && syslogConnection->tcp) {
                ism_log_closeSysLogConnection(syslogConnection);
                syslogConnection->isconnected=0;
            }
        } else {
            packtsent = sendto (syslogConnection->sock, message, len, MSG_NOSIGNAL,
                    (struct sockaddr *) &syslogConnection->ipv4addr, sizeof(syslogConnection->ipv4addr));
            if (packtsent<0 && syslogConnection->tcp){
                /* Connection Error. Retry to get the connection and try again. */
                ism_log_closeSysLogConnection(syslogConnection);
                syslogConnection->isconnected=0;
            }
        }
    } while (packtsent < 0 && tries++<10);

    if (packtsent < 0) {
        TRACE(2, "Error sending message to syslog: errno=%d. Error: %s\n ", errno, strerror(errno));
        fprintf(stdout, "%s\n", message);
        fflush(stdout);
    } else if (packtsent != len) {
        TRACE(2, "Error sending message to syslog: Sent bytes: %d != length bytes %d.\n", (int)packtsent, (int)len );
        fprintf(stdout, "%s\n", message);
        fflush(stdout);
    }
}

/**
 * Check if the buffer (not NULL-terminated!) contains
 * a valid message and find a corresponding logger.
 *
 * If not a valid message, or a logger is not file-based, return NULL
 * to allow the caller to deal with the buffer.
 *
 * @param buffer A pointer to the buffer with a potential message
 * @param count  The length of the text in the buffer
 *
 * @return FILE* of the logger or NULL, if the message is not valid
 */
FILE * ism_log_findLogWriter(const char * buffer, int count) {
	/* First, extract the component
     * 2015-10-11T12:19:13.624Z Admin        imaserver  CWLNA6013 N: Administration service is initialized: RunMode=0.
	 *
	 * Then find and return the logger
	 */
	char *loglevelStr;
	int len;
	char *ptr;
	int32_t msgId;
	char *msgIdPtr;
	char *endPtr;
	char *catToken;
	ISM_LOGLEV logLevel;
	int catId;
	int i;

	msgIdPtr = (char *)memmem(buffer, count, "CWLN", 4);
	if (!msgIdPtr) {
		return NULL;	// Not a Message Gatway message
	}

	// Message ID is 9 characters long
	ptr = (char *)memchr(msgIdPtr, ' ', count - (msgIdPtr - buffer));
	if (!ptr || (ptr - msgIdPtr) != 9) {
		return NULL;
	}

	msgId = atoi(msgIdPtr + 5);	// Skip CWLNx and get the number

	// Extract log level
	ptr++;
	endPtr = (char *)memchr(ptr, ':', count - (ptr - buffer));
	if (!endPtr) {
		return NULL;
	}

	len = endPtr - ptr;
	loglevelStr = alloca(len + 1);
	memcpy(loglevelStr, ptr, len);
	loglevelStr[len] = 0;

	ism_log_getISMLogLevel((const char *)loglevelStr, &logLevel);

	// Category is 12 characters after the first space.
	// Limit search to the place where message ID starts.
	ptr = (char *)memchr(buffer, ' ', (msgIdPtr - buffer));
	if (ptr) {
		ptr++; 							// Bypass the space
	} else {
		return NULL;	// Bad format, ignore
	}

	if (ptr > msgIdPtr) {
		return NULL;	// Bad format, ignore
	}

	endPtr = (char *)memchr(ptr, ' ', (msgIdPtr - ptr));
	if (!endPtr) {
		return NULL; // Bad format, ignore
	}

	len = endPtr - ptr;
	catToken = alloca(len + 1);

	memcpy(catToken, ptr, len);
	catToken[len] = 0;

	catId = ism_log_getCategoryID(catToken);
    for (i = LOGGER_SysLog; i < LOGGER_Max; i++) {
    	ism_logWriter_t * lw = g_logwriter[i];

    	// If the destination is not a file or the destination
    	// is an actual file, not a stdout or stderr, skip the logger
    	if (lw->desttype != DESTTYPE_FILE || lw->isfile == 1) {
    		continue;
    	}

    	int kind = TRACE_DOMAIN->logLevel[i];
        if (ism_log_filter(lw->level+kind, logLevel, catId, msgId) == 0) {
        	continue;
        }

        return lw->file;
    }

	// Incorrect logger, ignore
	return NULL;
}
