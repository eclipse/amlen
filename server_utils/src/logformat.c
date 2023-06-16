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
/*                                                                   	*/
/* Module Name: logformat.c                      	                   	*/
/*                                                                   	*/
/* Description: 														*/
/*		Contains implementations for both logging and Return Codes 		*/
/*                                                                   	*/
/************************************************************************/
#define TRACE_COMP Util
#include <ismutil.h>
#include <locale.h>
#include <stdarg.h>
#include <sys/time.h>
#include <log.h>
#include <config.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

static void ism_log_iotGetSDElements(concat_alloc_t *buf, int32_t category, ism_common_log_context *context);
static void ism_log_defaultGetSDElements(concat_alloc_t *buf, int32_t category, ism_common_log_context *context);

typedef void (* ism_log_getSDElements_t)(concat_alloc_t *, int32_t, ism_common_log_context *);

ism_log_getSDElements_t ism_log_getSDElements = ism_log_defaultGetSDElements;

ISM_LOGLEV ism_g_loglevel;
char * ism_g_syslog_ip=DUMMY_LINKLOCAL_ADDR;
int ism_g_syslog_port=ISM_SYSLOG_PORT;
ism_logWriter_t * g_logwriter[LOGGER_Max+1] = {NULL};

/*
 * Current log entries in backlog
 */
static int g_logEntCount = 0;
static int g_logEntLost  = 0;
#define LOG_ENT_MAX    400000
#define LOG_ENT_RESUME 250000


void * ism_log_announcer(void * param, void *, int value);

#define BILLION        1000000000L
static int logCleanUpTime = 1; //in Minutes. Default 60 Minutes
static int logObjectTTLTime=60; //In Minutes. Default is 60 minutes
static ism_time_t logObjectTTLTimeNano = 1*60*BILLION;
static ism_time_t logCleanUpTimeInNano = 10*60*BILLION;
static int logTableInited = 0;
static int logCleanUpTaskStarted=0;
ism_timer_t  logTableTimer = NULL;
static int summarizeLogsEnable=0;
static int summarizeLogsInterval=5;
static ism_time_t summarizeLogsIntervalInNano=10*60*BILLION;
static int ism_log_initLogTable(void);
static int ism_log_termLogTable(void);
static int logTableCleanUpTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata);
static int getDefaultFacility(int which);
static pthread_mutex_t *getSharedProcessLock();

extern int ism_log_setWriterDestination(ism_logWriter_t * lw, const char * destination);
extern int ism_log_closeSysLogConnection(ismSyslogConnection_t * syslog_conn);
int ism_common_isBridge(void);
int ism_common_isProxy(void);
void ism_log_setExeType(int type);

/*
 * Aux Logger Settings ISM enum list
 */
ism_enumList enum_auxlogger_settings [] = {
    { "Setting",    3,                 	},
    { "Min",     	AuxLogSetting_Min,   	},
    { "Normal",     AuxLogSetting_Normal,   },
    { "Max",   		AuxLogSetting_Max, 		},

};

/*
 * Lock for the category structure
 */
static pthread_mutex_t    logLock;
static pthread_cond_t     logCond;
static ism_threadh_t      logThread;
static ismLogEvent_t *    logHead;
static ismLogEvent_t *    logTail;

pthread_mutex_t    * sharedProcessLock = NULL;
static const char SHARED_MEMORY_LOG_LOCK[] = "/" IMA_PRODUCT_SHM_PREFIX "::shared_log_lock";
static const char SHARED_MEMORY_LOG_LOCK_BRIDGE[] = "/" IMA_PRODUCT_SHM_PREFIX ".imabridge::shared_log_lock";
static const char SHARED_MEMORY_LOG_LOCK_PROXY[] = "/" IMA_PRODUCT_SHM_PREFIX ".imaproxy::shared_log_lock";

static pthread_mutex_t    g_logtableLock;
static ismHashMap * g_logtable;


/*
 * Set the logging level.
 */
void ism_log_setLevel(ISM_LOGLEV level) {
    /*
     * If an invalid log level is specified, leave the log level.
     */
    if (level < 1 || level > ISM_LOGLEV_INFO)
        return;
    ism_g_loglevel = level;
}


/*
 * Uset the logcategory.h header file to procude the name to id category mapping
 */
#define LOGCAT_SPECIAL
int ism_log_getCategoryID(const char * category) {
    if (!strcmp(category, ".")) return 0;
#undef  LOGCAT_
#define LOGCAT_(name, val) else if (!strcmpi(category, #name)) return LOGCAT_##name;
#include <logcategory.h>
    return 0;
}


/*
 * Get a logging category name given the id.
 * This is used for the name returned by logging so it has default values
 * for zero or unknown categories.
 * @param id    The category id.
 * @return The category name.  If the category is zero return an asterisk (*).
 * If the category is unknown, return a dot (.).
 */
const char * ism_log_getCategoryName(int32_t id) {
    switch (id) {
#undef  LOGCAT_
#define LOGCAT_(name, val) case LOGCAT_##name: return #name;
#include <logcategory.h>
    default:
        return ".";
    }
}
#undef LOGCAT_
#undef LOGCAT_SPECIAL

/*
 * External references
 */
int ism_common_initLocale(ism_prop_t * props);
/* We might make these public later */
extern char ism_common_getNumericSeparator(void);
extern char ism_common_getNumericPoint(void);

int stopWork;
static int inited = 0;
/*
 * Initialize a log formatter instance.
 * The log formatter will be initialized to default values with no handler.
 *
 * The memory for the formatter object is input to this routine.
 */
int ism_log_init(void) {

    if (!inited) {
        ism_prop_t *configProps = ism_common_getConfigProperties();
        inited = 1;
        stopWork=0;
        ism_common_initLocale(configProps);
        pthread_mutex_init(&logLock, 0);
        pthread_cond_init(&logCond, 0);

        // In IoT, produce IoT structure elements
        if (ism_common_getBooleanProperty(configProps, "Protocol.AllowMqttProxyProtocol", 0)) {
            ism_log_getSDElements = ism_log_iotGetSDElements;
        } else {
            assert(ism_log_getSDElements == ism_log_defaultGetSDElements);
        }

        sharedProcessLock = getSharedProcessLock();

        ism_common_startThread(&logThread, ism_log_announcer,
                NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, "logger", "The log announcer thread");

        ism_common_getMessageCatalog(NULL);

        ism_log_initLogTable();

    }
    return 0;
}

/*
 * Provide a combine log_init and setExeType so that the exe type is known in log_init
 */
int ism_log_init2(int exetype) {
    ism_log_setExeType(exetype);
    return ism_log_init();
}

/*
 * Cancel a log formatter instance.
 * This is used only in test
 */
int ism_log_term(void) {

    if (inited) {
    	stopWork=1;
    	pthread_mutex_lock(&logLock);
    	pthread_cond_signal(&logCond);
    	pthread_mutex_unlock(&logLock);

		ism_common_joinThread(logThread, NULL);

		pthread_mutex_destroy(&logLock);
		pthread_cond_destroy(&logCond);

		ism_log_termLogTable();

		inited=0;
    }

    return 0;
}


/*
 * Put a length and string to the concat buffer
 */
static void putString(concat_alloc_t * buf, const char * str) {
    int32_t len = (int32_t)strlen(str);
    char * loc = ism_common_allocAllocBuffer(buf, len+5, 0);
    memcpy(loc, &len, 4);
    memcpy(loc+4, str, len+1);
}


/*
 * Get a string from the buffer.
 */
static const char * getString(concat_alloc_t * buf) {
    int32_t len;
    const char * ret;

    if (buf->pos + 4 > buf->used)
        return "";
    memcpy(&len, buf->buf+buf->pos, 4);
    len++;
    buf->pos += 4;
    if (len<0 || len+buf->pos > buf->used) {
        buf->pos = buf->used;
        return "";
    }
    ret = buf->buf + buf->pos;
    buf->pos += len;
    return ret;
}


/*
 * Calculate the extra size to escape
 */
int escapeExtra(const char * str) {
    int extra = 0;
    while (*str) {
        char ch = *str++;
        if (ch == '\n' || ch == '\r' || ch == '\t' || ch == '"')
            extra++;
        else if (ch == '\\') {
            if (*str == 'n' || *str == 'r' || *str == 't' || *str == '"' || *str == '\\')
                extra++;
        }
    }
    return extra;
}


/*
 * Do the quote escape used with replacement data
 */
void escapeQuote(char * out, const char * str) {
    while (*str) {
        char ch = *str++;
        switch(ch) {
        case '\n':    *out++ = '\\'; *out++ = 'n';   break;
        case '\r':    *out++ = '\\'; *out++ = 'r';   break;
        case '\t':    *out++ = '\\'; *out++ = 't';   break;
        case '"':     *out++ = '\\'; *out++ = '"';   break;
        case '\\':
            *out++ = '\\';
            if (*str == 'n' || *str == 'r' || *str == 't' || *str == '"' || *str == '\\') {
                *out++ = '\\';   break;
            }
            break;
        default:
            *out++ = ch;
            break;
        }
        *out = 0;
    }
}

/*
 * Add an individual field to an existing SD element
 */
static void ism_log_addSDElementField(concat_alloc_t *buf, const char *name, const char *value, int checkEscaping) {
    if (value) {
        size_t nameLen = strlen(name);
        size_t valueLen = strlen(value);
        /* if escaping is possible, allocate double the length of the value (and rewind unused) */
        int allocLen = (int)((nameLen+valueLen+4)+(checkEscaping ? valueLen : 0));

        char *field = ism_common_allocAllocBuffer(buf, allocLen, 0);

        *field++ = ' ';
        memcpy(field, name, nameLen);
        field += nameLen;
        *field++ = '=';
        *field++ = '\"';
        if (checkEscaping) {
            const char *valuePos = value;
            char charVal;
            size_t escapesAdded = 0;
            for(; (charVal = *valuePos) != '\0'; valuePos++) {
                /* Escape " \ ] with a leading backslash */
                if (charVal == '\"' || charVal == '\\' || charVal == ']') {
                    *field++ = '\\';
                    escapesAdded++;
                }
                *field++ = charVal;
            }
            /* rewind the part of the buffer we didn't use */
            buf->used -= (int)(valueLen-escapesAdded);
        } else {
            memcpy(field, value, valueLen);
            field += valueLen;
        }
        *field++ = '\"';
    }
}

/*
 * Get the default SD Elements added to syslog message
 */
static void ism_log_defaultGetSDElements(concat_alloc_t *buf, int32_t category, ism_common_log_context *context)
{
    const char *categoryName = ism_log_getCategoryName(category);

    ism_common_allocBufferCopyLen(buf, "[",1);
    ism_common_allocBufferCopyLen(buf, ismLOG_DEFAULT_SDID, strlen(ismLOG_DEFAULT_SDID));
    ism_log_addSDElementField(buf, "cat", categoryName, 0);
    ism_common_allocBufferCopyLen(buf, "]", 1);
}

/*
 * Get the SD elements to include in IoT
 */
static void ism_log_iotGetSDElements(concat_alloc_t *buf, int32_t category, ism_common_log_context *context)
{
    /* include the default elements */
    ism_log_defaultGetSDElements(buf, category, context);

    /* Using the context add an additional SD containing the values found */
    if (context) {
        int valuesFound = 0;
        const char *resourceSetId = context->resourceSetId;
        const char *clientId = context->clientId;

        if (clientId) valuesFound++;
        if (resourceSetId) valuesFound++;

        if (valuesFound) {
            ism_common_allocBufferCopyLen(buf, "[", 1);
            ism_common_allocBufferCopyLen(buf, ismLOG_IOT_SDID, strlen(ismLOG_IOT_SDID));

            ism_log_addSDElementField(buf, "organization", resourceSetId, 1);
            ism_log_addSDElementField(buf, "clientid", clientId, 1);

            ism_common_allocBufferCopyLen(buf, "]", 1);
        }
    }
}

/*
 * Put the SD elements string into the specified buffer
 */
static void ism_log_putSDElements(concat_alloc_t *buf, int32_t category, ism_common_log_context *context)
{
    char xbuf[4096];
    concat_alloc_t sdbuf = {0};
    sdbuf.buf = xbuf;
    sdbuf.len = sizeof(xbuf);

    /* Get the SD elements appropriate for the environment */
    ism_log_getSDElements(&sdbuf, category, context);

    /* NULL terminate and add string to the buffer */
    ism_common_allocBufferCopy(&sdbuf, "");
    putString(buf, sdbuf.buf);
    ism_common_freeAllocBuffer(&sdbuf);
}

/*
 * Parse replacement data and converts it to strings
 * Supported replacement data types are:
 * %c - char
 * %d - signed integer
 * %e - double
 * %p - pointer in hex
 * %u - unsigned integer (dec)
 * %x - unsigned integer (hex)
 * %lld or %ld - signed 64 bit integer
 * %llu or %lu - unsigned 64 bit integer (dec)
 * %llx or %lx - unsigned 64 bit integer (hex)
 */
int ism_log_parseReplacementData(concat_alloc_t * buf, const char * types, va_list args) {
    int      count;
    int      extra;
    const char * tp;
    char *   svalue;
    int      slen;
    int32_t  value;
    uint32_t len;
    uint64_t lvalue;
    double   dvalue;
    char     cbuf[64];

    if (types == NULL)
        return 0;
    tp = types;
    count = 0;

    tp = types;
    for (tp = types; *tp; tp++) {
        char ch;
        if (*tp != '%')
            continue;

        ch = *++tp;

        if (ch == '-') {
            if (*++tp == 's') {
                svalue = (char *)va_arg(args, uintptr_t);
                if (svalue == NULL)
                    svalue = "(null)";
                slen = (int)strlen(svalue);
                extra = escapeExtra(svalue);
                char * loc = ism_common_allocAllocBuffer(buf, slen+extra+7, 0);
                len = slen+extra+2;
                memcpy(loc, &len, 4);
                loc[4] = '"';
                if (extra == 0)
                    memcpy(loc+5, svalue, slen);
                else
                    escapeQuote(loc+5, svalue);
                loc[slen+extra+5] = '"';
                loc[slen+extra+6] = 0;
                count++;
            }
        } else if (ch == 's') {
            svalue = (char *)va_arg(args, uintptr_t);
            if (svalue == NULL)
                svalue = "(null)";
            putString(buf, svalue);
            count++;
        } else {
            switch (ch) {
            case 'd':
                value = (int)va_arg(args, int32_t);
                ism_common_itoa(value, cbuf);
                break;

            case 'u':
                value = (int32_t)va_arg(args, int32_t);
                ism_common_uitoa(value, cbuf);
                break;

            case 'x':
                value = (int32_t)va_arg(args, int32_t);
                ism_common_uitox(value, 1, cbuf);
                break;

            case 'l':
                if (!*++tp)
                    return count;
                if (*tp == 'l')
                    tp++;
                switch (*tp) {
                case 'd':
                    lvalue = (int64_t) va_arg(args, int64_t);
                    ism_common_ltoa(lvalue, cbuf);
                    break;
                case 'u':
                    lvalue = (int64_t) va_arg(args, int64_t);
                    ism_common_ultoa_ts(lvalue, cbuf, ism_common_getNumericSeparator());
                    break;
                case 'x':
                    lvalue = (int64_t) va_arg(args, int64_t);
                    ism_common_ultox(lvalue, 1, cbuf);
                    break;
                default:
                    return count;
                }
                break;

            case 'e':
                dvalue = (double)va_arg(args, double);
                ism_common_dtoa(dvalue, cbuf);
                break;

            case 'c':
                cbuf[0] = (char)va_arg(args, int);
                cbuf[1] = 0;
                break;

            case 'p':
                lvalue = (uintptr_t)va_arg(args, uintptr_t);
                ism_common_ultox(lvalue, 1, cbuf);
                break;

            default:
                return count;
            }
            putString(buf, cbuf);
            count++;
        }
    }
    return count;
}


/*
 * Format a message from a java message definition.
 */
XAPI size_t ism_common_formatMessage(char * msgBuff, size_t msgBuffSize, const char * msgFormat,
        const char * * replData, int replDataSize) {
    char * mp;           /* Output location in the message */
    char * xp;           /* Output location in the format info fields */
    char formatNum[64];  /* Replacement number, Nulled for safety */
    char formatType[64]; /* Replacement type  (Currently we just parse it, but don't use it) */
    char formatExt[64];  /* Replacement extension (Currently we just parse it, but don't use it)*/
    size_t left, xleft;  /* Bytes left */
    int inQuote = 0;     /* Inside a quote */
    int braceStack = 0;  /* Brace count in a format specification */
    int part;            /* Which part of the format */
    size_t replen;       /* Replacement length */
    const char * repstr; /* Replacement string */

    mp = msgBuff;
    left = msgBuffSize - 1;
    while (*msgFormat) {
        char ch = *msgFormat++;
        if (ch == '\'') { /* Start of quote */
            if (*msgFormat == '\'') { /* Two quotes makes a single quote */
                if (left > 0) {         /* Put out the quote char */
                    *mp++ = ch;
                    msgFormat++;		/* Advance within the pattern */
                }
                left--;
            } else {
                inQuote = !inQuote;   /* Invert quote state */
            }
            continue;
        }
        if (inQuote || ch != '{') {   /* If not start of format spec */
            if (left > 0)             /* Put out the char */
                *mp++ = ch;
            left--;
            continue;
        }
        /*
         * Parse the format string
         */
        part = 0;
        xp = formatNum;               /* The first field is the format number */
        xleft = 63;
        braceStack = 1;               /* We are in a format */
        while (*msgFormat && braceStack) {
            ch = *msgFormat++;
            switch (ch) {
            case ',':                 /* End of a section */
                *xp = 0;
                switch (part) {
                case 0:
                    xp = formatType;  /* Switch to type */
                    xleft = 63;
                    part = 1;
                    break;
                case 1:
                    xp = formatExt;   /* Switch to extension */
                    xleft = 63;
                    part = 2;
                    break;
                default:              /* Just put the comma into the extension */
                    part++;
                    if (xleft > 0)
                        *xp++ = ch;
                    xleft--;
                    break;
                }
                break;

            case '{':                 /* Keep an even number of braces */
                braceStack++;
                if (xleft > 0) {
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
                    if (which < 0 || which >= replDataSize) {
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
                    } else {
                    	msgBuffSize += (replen+1);
                    }
                    memset(formatNum, 0, 64);
                    memset(formatType, 0, 64);
                    memset(formatExt, 0, 64);
                } else {
                    if (xleft > 0) {
                        *xp++ = ch;
                        xleft--;
                    }
                }
                break;
            default: /* Put the character into the current format string */
                if (xleft > 0) {
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


/*
 * Log announcer processing method
 */
void * ism_log_announcer(void * param, void * context, int value) {
    ismLogEvent_t * logent;
    ismLogOut_t     logout;
    concat_alloc_t  buf = {0};
    const char *    repl[64];
    int        i, which;

    while (!stopWork) {
        pthread_mutex_lock(&logLock);

        /* Wait for work */
        while (logHead == NULL) {      /* BEAM suppression: infinite loop */
            pthread_cond_wait(&logCond, &logLock);
            if (stopWork) {
            	pthread_mutex_unlock(&logLock);
            	return NULL;
            }
        }
        g_logEntCount--;

        /* Select the first item */
        logent = logHead;
        SA_ASSUME(logent != NULL);
        logHead = logent->next;
        if (logHead == NULL)
            logTail = NULL;
        pthread_mutex_unlock(&logLock);

        /* Create the log output structure */
        memset(&logout, 0, sizeof(logout));
        logout.timestamp = logent->timestamp;
        strcpy(logout.threadName, logent->threadName);
        logout.category = logent->category;
        logout.loglevel = logent->log_level;
        logout.size     = logent->size;
        logout.msgnum   = logent->msgnum;
        logout.lineno   = logent->lineno;
        strcpy(logout.msgid, logent->msgid);
        buf.buf = (char *)(logent+1);
        buf.used = logout.size;
        buf.pos = 0;

        logout.func = getString(&buf);           /* Get the function */
        logout.location = getString(&buf);       /* Get the location */
        logout.sdelements = getString(&buf);     /* Get the structured data elements */
        logout.msgf = getString(&buf);           /* Get the message format */

        /* Get the replacement */
        logout.count = logent->count;
        if (logout.count > 64)
            logout.count = 64;                  /* Max count for formatMessage */
        for (i=0; i<logout.count; i++) {
            repl[i] = getString(&buf);
        }
        logout.repl = repl;

        /* Sent the entry as desired */
        ism_log_toTrace(&logout);

        /* Log to System Syslog or other facilities  */
        for (which=0; which<LOGGER_Max; which++) {
        	if (g_logwriter[which] != NULL) {
        		ism_log_toISMLogger(g_logwriter[which], logent->levels[which], &logout);
        	}
        }

        ism_common_free(ism_memory_utils_log,logent);
    }
    return NULL;
}


/*
 * This function will create the log entry and queue it up for logging.
 */
XAPI void ism_common_logInvoke(ism_common_log_context *context, const ISM_LOGLEV level, int msgnum, const char * msgID, int32_t category,
        ism_trclevel_t * trclvl, const char * func, const char * file, int line,
        const char * types, const char * fmts, ...) {
    va_list args;
    ismLogEvent_t * logent;
    int count;
    concat_alloc_t buf = {0};
    char     xbuf[4096];
    char *   fp;

    buf.buf = xbuf;
    buf.len = sizeof(xbuf);

    /* Put out the function name */
    if (!func)
        func = "";
    putString(&buf, func);

    /* Put out the source file name, but not the path */
    if (file) {
        fp = (char *)file + strlen(file);
        while (fp > file && fp[-1] != '/' && fp[-1] != '\\')
            fp--;
    } else {
        fp = "";
    }
    putString(&buf, fp);

    /* Put out the structured data elements */
    ism_log_putSDElements(&buf, category, context);

    /* Put out the */
    putString(&buf, fmts);
    va_start(args, fmts);
    count = ism_log_parseReplacementData(&buf, types, args);
    va_end(args);

    /*
     * Construct the log entry
     */
    logent = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,161),sizeof(ismLogEvent_t) + buf.used);
    if (logent) {
        logent->timestamp = ism_common_currentTimeNanos();
        logent->category  = category;
        logent->log_level = level;
        memcpy(logent->levels, trclvl->logLevel, sizeof logent->levels);
        // printf("log msg=%d cat=%d lvl=%d %d %d %d\n", msgnum, category, logent->levels[0],logent->levels[1], logent->levels[2], logent->levels[3]);
        logent->lineno    = line;
        logent->next      = NULL;
        logent->msgnum    = msgnum;
        ism_common_strlcpy(logent->msgid, msgID, sizeof(logent->msgid));
        logent->count     = count;
        logent->size      = buf.used;
        logent->next      = NULL;
        ism_common_getThreadName(logent->threadName, sizeof(logent->threadName));
        if (logent->threadName[0] == 0)
            strcpy(logent->threadName, ".");
        memcpy((char *)(logent+1), buf.buf, buf.used);
    }

    /*
     * Insert the log item at the tail of the log queue
     */
    pthread_mutex_lock(&logLock);
    /* If the log queue to too full, throw the log entry away */
    if (!logent || g_logEntCount > LOG_ENT_MAX || (g_logEntLost && g_logEntCount > LOG_ENT_RESUME)) {
        g_logEntLost++;
        if (logent) {     /* If we made a logent, free it since we are not enqueuing it */
            ism_common_free(ism_memory_utils_log,logent);
        }
    } else {
        g_logEntCount++;
        if (logent && g_logEntLost) {
        	int lostCount = g_logEntLost;
            g_logEntLost = 0;
            /* We cannot log with the log lock held */
            pthread_mutex_unlock(&logLock);
            LOG(ERROR, Server, 901, "%u", "Log entries lost due to delay in writing the log: count={0}", lostCount);
            pthread_mutex_lock(&logLock);
        }
        if (logTail) {
            logTail->next = logent;
            logTail = logent;
        } else {
            logTail = logent;
            logHead = logent;
        }
        pthread_cond_signal(&logCond);
    }

    pthread_mutex_unlock(&logLock);


    if (buf.inheap)
    {
        ism_common_freeAllocBuffer(&buf);
    }

}


/*
 * Get log level string.
 */
char * ism_log_getLogLevelCode(ISM_LOGLEV level) {
    switch (level) {
    case ISM_LOGLEV_CRIT:
        return ISM_LOGLEV_S_CRIT;
    case ISM_LOGLEV_ERROR:
        return ISM_LOGLEV_S_ERROR;
    case ISM_LOGLEV_WARN:
        return ISM_LOGLEV_S_WARN;
    case ISM_LOGLEV_NOTICE:
        return ISM_LOGLEV_S_NOTICE;
    case ISM_LOGLEV_INFO:
    default:
        return ISM_LOGLEV_S_INFO;
    }
}

/*
 * Get ISM Log Level
 */
int ism_log_getISMLogLevel(const char * level, ISM_LOGLEV * ismlevel) {
    if (level != NULL && ismlevel != NULL) {
        if (!strcmpi("INFO", level) || !strcmpi("I", level))
            *ismlevel = ISM_LOGLEV_INFO;
        else if (!strcmpi("WARN", level) || !strcmpi("WARNING", level) || !strcmpi("W", level))
            *ismlevel = ISM_LOGLEV_WARN;
        else if (!strcmpi("NOTICE", level) || !strcmpi("N", level))
            *ismlevel = ISM_LOGLEV_NOTICE;
        else if (!strcmpi("ERROR", level) || !strcmp("ERR", level)|| !strcmpi("E", level))
            *ismlevel = ISM_LOGLEV_ERROR;
        else if (!strcmpi("CRIT", level) || !strcmpi("C", level))
            *ismlevel = ISM_LOGLEV_CRIT;
        else if (!strcmpi("NORMAL", level) || !strcmpi("NORM", level))
            *ismlevel = ISM_LOGLEV_NOTICE;
        else if (!strcmpi("MINIMUM", level) || !strcmpi("MIN", level))
            *ismlevel = ISM_LOGLEV_ERROR;
        else if (!strcmpi("MAXIMUM", level) || !strcmpi("MAX", level))
            *ismlevel = ISM_LOGLEV_INFO;
        else
            return -1;
    } else {
        return -1;
    }
    return 0;
}

/*
 * Create a single logger for all
 */
int ism_log_createLoggerSingle(ism_prop_t * props) {
    int   rc = 0;
    const char * type = ism_common_getStringProperty(props, "LogDestinationType");
    const char * destination = ism_common_getStringProperty(props, "LogDestination");
    ism_logWriter_t * lw = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_log,163),1, sizeof(ism_logWriter_t));

    pthread_mutex_lock(&logLock);

    if (type && !strcmp(type, "syslog")) {
        ism_log_initSyslog(props);
        int facility = atoi(destination);
        if (!facility) {
            facility = getDefaultFacility(LOGGER_SysLog);
        }

        if (lw != NULL) {
            rc = ism_log_openSysLog(lw, LOGGER_SysLog, facility);
        }
    } else if (destination) {
        rc = ism_log_setWriterDestination(lw, destination);
    } else {
        rc = ISMRC_BadPropertyValue;
    }

    /* Default to stdout */
    if (rc) {
        TRACE(5, "Due to error, default stdout logger created");
        destination = "stdout";
        type = "file";
        rc = ism_log_setWriterDestination(lw, destination);
    }
    TRACE(8,"Init logger: type=%s dest=%s rc=%d\n", type, destination, rc);
    g_logwriter[LOGGER_SysLog] = (rc == 0)?lw:NULL;
    if (!g_logwriter[LOGGER_SysLog]) {
        TRACE(2, "The destination for log is not valid: %s\n", destination);
    } else {
        lw->level[0].level = ISM_LOGLEV_NOTICE;
        ism_log_createFilter(lw->level+0, "1111,900,902");
        lw->level[AuxLogSetting_Min].level = ISM_LOGLEV_WARN;
        ism_log_createFilter(lw->level+AuxLogSetting_Min, "1111,900,902");
        lw->level[AuxLogSetting_Normal].level = ISM_LOGLEV_NOTICE;
        ism_log_createFilter(lw->level+AuxLogSetting_Normal, "1111,900,902");
        lw->level[AuxLogSetting_Max].level = ISM_LOGLEV_INFO;
    }

    pthread_mutex_unlock(&logLock);
    return rc;
}


/*
 * Create a logger.
 */
XAPI int ism_log_createLogger(int which, ism_prop_t * props) {
    int rc = 0;

    assert(which >= LOGGER_SysLog && which <= LOGGER_Max);

    const char * type;
    const char * destination;

    switch (which) {
    case LOGGER_Security:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.SecurityLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.SecurityLog");
        break;
    case LOGGER_Admin:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.AdminLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.AdminLog");
        break;
    case LOGGER_Connection:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.ConnectionLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.ConnectionLog");
        break;
    case LOGGER_MQConnectivity:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.MQConnectivityLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.MQConnectivityLog");
    	break;
    case LOGGER_SysLog:
    default:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.DefaultLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.DefaultLog");
        break;
    }

	TRACE(5, "Creating logger %s:%s\n", type, destination);

    ism_logWriter_t * lw = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_log,164),1, sizeof(ism_logWriter_t));

    pthread_mutex_lock(&logLock);

    if (type && !strcmp(type, "syslog")) {
    	int facility = atoi(destination);
    	if (!facility) {
            facility = getDefaultFacility(which);
    	}

        if (lw != NULL) {
        	rc = ism_log_openSysLog(lw, which, facility);
        }
    } else if (destination) {
        rc = ism_log_setWriterDestination(lw, destination);
    } else {
    	rc = ISMRC_BadPropertyValue;
    }

    if (rc) {
    	// If file could not be opened or syslog connection could not be established,
    	// send messages to stdout
        TRACE(5, "Due to error, default stdout logger created: which=%d rc=%d\n", which, rc);
    	destination = "stdout";
    	type = "file";
    	rc = ism_log_setWriterDestination(lw, destination);
    }

    TRACE(8,"Init %s logger: which=%d dest=%s rc=%d\n", type, which, destination, rc);
    g_logwriter[which] = (rc == 0)?lw:NULL;
    if (!g_logwriter[which]) {
        TRACE(2, "The destination for log %d is not valid: %s\n", which, destination);
    } else {
        ism_log_createFilter(g_logwriter[which]->level+0,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Normal));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Min,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Min));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Normal,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Normal));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Max,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Max));
    }

    pthread_mutex_unlock(&logLock);
    return rc;
}


/*
 * Update a logger.
 */
XAPI int ism_log_updateLogger(int which, ism_prop_t * props) {
    int rc = 0;

    if (g_logwriter[which] == NULL) {
    	return ism_log_createLogger(which, props);
    }

    assert(which >= LOGGER_SysLog && which <= LOGGER_Max);

    const char * type;
    const char * destination;

    switch (which) {
    case LOGGER_Security:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.SecurityLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.SecurityLog");
        break;
    case LOGGER_Admin:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.AdminLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.AdminLog");
        break;
    case LOGGER_Connection:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.ConnectionLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.ConnectionLog");
        break;
    case LOGGER_MQConnectivity:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.MQConnectivityLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.MQConnectivityLog");
    	break;
    case LOGGER_SysLog:
    default:
        type = ism_common_getStringProperty(props, "LogLocation.LocationType.DefaultLog");
        destination = ism_common_getStringProperty(props, "LogLocation.Destination.DefaultLog");
        break;
    }

	TRACE(5, "Creating logger %s:%s\n", type, destination);

    pthread_mutex_lock(&logLock);
    ism_logWriter_t * lw = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_log,165),sizeof(ism_logWriter_t));
    memcpy(lw, g_logwriter[which], sizeof(ism_logWriter_t));
    if (g_logwriter[which]->destination && g_logwriter[which]->isfile) {
    	lw->destination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_log,1000),g_logwriter[which]->destination);
    }

    int changed = 0;

    if (type && !strcmp(type, "syslog")) {
    	// Syslog - check for changes
    	int facility = destination?atoi(destination):lw->facility;
    	if (lw->desttype == DESTTYPE_SYSLOG) {
    		lw->facility = facility;
    	} else {
    		// Switch from file to syslog, set facility
    		if (!destination) {
    			facility = getDefaultFacility(which);
    		}

    		rc = ism_log_openSysLog(lw, which, facility);

    		changed = 1;
    	}
   	} else {
   		// File - check for changes
   		if (lw->desttype == DESTTYPE_FILE && strcmp(destination, lw->destination) != 0) {
   			changed = 1;
   		} else if (lw->desttype == DESTTYPE_SYSLOG) {
   			changed = 1;
   		}
		rc = ism_log_setWriterDestination(lw, destination);
   	}

    // If the only error is that syslog connection was not established, proceed normally
    if (rc) {
    	if (type && strcmp(type, "syslog")) {
    		TRACE(1, "The file for log %d (%s) could not be opened for writing: %d\n",
    				which, destination, rc);

    		rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", "Destination", destination);
    	} else {
    		TRACE(1, "The connection to syslog server for log %d (%s) was not established: %d\n",
    				which, destination, rc);
    	}
		ism_log_closeWriter(lw);
    } else if (changed) {
    	ism_logWriter_t * oldlw = g_logwriter[which];
    	g_logwriter[which] = lw;

    	if (oldlw->desttype == DESTTYPE_FILE) {
    		// If the destination type was changed from file to syslog, clean up
    		// file handle and destination pointer
    		if (oldlw->isfile && lw->desttype == DESTTYPE_SYSLOG) {
    			fclose(oldlw->file);
    			ism_common_free(ism_memory_utils_log,oldlw->destination);

    			g_logwriter[which]->isfile = 0;
    			g_logwriter[which]->destination = NULL;
    			g_logwriter[which]->file = NULL;
    		}
    	} else {
    		if (oldlw->isconnected) {
    			ism_log_closeSysLogConnection(oldlw->syslog_conn);
    		}
    		g_logwriter[which]->isconnected = 0;
    	}
    	ism_log_closeWriter(oldlw);
    }

    TRACE(5, "Re-init %s logger: which=%d dest=%s rc=%d\n", type, which, destination, rc);
    if (rc) {
        TRACE(2, "The destination for log %d is not valid: %s\n", which, destination);
    } else {
        ism_log_createFilter(g_logwriter[which]->level+0,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Normal));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Min,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Min));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Normal,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Normal));
        ism_log_createFilter(g_logwriter[which]->level+AuxLogSetting_Max,
                ism_log_getLogFilterFromAuxLoggerSetting(which, AuxLogSetting_Max));
    }

    pthread_mutex_unlock(&logLock);
    return rc;
}

/*
 * Terminate the ISM Logger
 */
XAPI int ism_log_termLogger(int which) {
	int rc = ISMRC_OK;

    assert(which >= LOGGER_SysLog && which <= LOGGER_Max);
    pthread_mutex_lock(&logLock);
    if (g_logwriter[which]) {
        ism_log_closeWriter(g_logwriter[which]);
        g_logwriter[which] = NULL;
    } else {
    	rc = ISMRC_NotFound;
    }
    pthread_mutex_unlock(&logLock);
    return rc;
}


/*
 * get log level base on the Aux logger setting
 */
const char * ism_log_getLogLevelFromAuxLoggerSetting(int32_t setting) {
	switch(setting){
    case AuxLogSetting_Min:
			return "WARN";
	case AuxLogSetting_Max:
			return "INFO";
	case AuxLogSetting_Normal:
	default:
	 	return "NOTICE";
	};
}


/*
 * get log filter base on the Aux logger setting
 */
const char * ism_log_getLogFilterFromAuxLoggerSetting(int which, int32_t setting) {
	if (which==LOGGER_Connection) {
		switch (setting){
		case AuxLogSetting_Min: 	  return CONNECTION_SYSLOG_MIN_FILTER;
		case AuxLogSetting_Max:		  return CONNECTION_SYSLOG_MAX_FILTER;
		default:
		case AuxLogSetting_Normal:    return CONNECTION_SYSLOG_NORMAL_FILTER;
		};
	} else if (which==LOGGER_Security) {
		switch (setting){
		case AuxLogSetting_Min:       return SECURITY_SYSLOG_MIN_FILTER;
		case AuxLogSetting_Max:   	  return SECURITY_SYSLOG_MAX_FILTER;
		default:
		case AuxLogSetting_Normal:    return SECURITY_SYSLOG_NORMAL_FILTER;
		};
	} else if (which==LOGGER_Admin) {
		switch(setting){
		case AuxLogSetting_Min:       return ADMIN_SYSLOG_MIN_FILTER;
		case AuxLogSetting_Max:       return ADMIN_SYSLOG_MAX_FILTER;
		default:
		case AuxLogSetting_Normal:    return ADMIN_SYSLOG_NORMAL_FILTER;
		};
    } else if (which==LOGGER_MQConnectivity) {
		switch(setting){
		case AuxLogSetting_Min:       return MQCONN_SYSLOG_MIN_FILTER;
		case AuxLogSetting_Max:       return MQCONN_SYSLOG_MAX_FILTER;
		default:
		case AuxLogSetting_Normal:    return MQCONN_SYSLOG_NORMAL_FILTER;
		};
	} else if (which==LOGGER_SysLog) {
		return BASE_SYSLOG_FILTER;
	} else {
		return NULL;
	}
}



/*
 * Destroy clientLogObj object
 */
static int destroyClientLogObj(ismClientLogObj * clientLogObj, char *keyStr)
{
	int i=0;
	int removedCount=0;


	if (clientLogObj == NULL){
		return removedCount;
	}

	if (keyStr == NULL) {
	    keyStr = "NULL";
	}

	ism_log_checkStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);

	TRACE(7,"destroyClientLogObj: keyStr=%s\n", keyStr);

	ismHashMapEntry **dataEntries =  ism_common_getHashMapEntriesArray(clientLogObj->msglogtable);

	while(dataEntries[i] != ((void*)-1)){

		ismHashMapEntry * dataEntry = dataEntries[i];
		ismLogObj * logObj = (ismLogObj *)dataEntry->value;
		ism_common_removeHashMapElement(clientLogObj->msglogtable, dataEntry->key, dataEntry->key_len);
		int msgcode=0;
		if (logObj!=NULL){
			ism_log_checkStructId(logObj->structId, ismLOG_LOGOBJ_STRUCTID);
			msgcode=logObj->msgcode;
			ism_log_invalidateStructId(logObj->structId);
			ism_common_free(ism_memory_utils_log,logObj);
			logObj=NULL;
		}
		removedCount++;
		TRACE(7,"destroyClientLogObj: removed log object from the client log table: keyStr=%s msgcode=%d totalremoved=%d\n", keyStr, msgcode, removedCount );


		i++;
	}

	ism_common_destroyHashMap(clientLogObj->msglogtable);
	clientLogObj->msglogtable=NULL;

	ism_log_invalidateStructId(clientLogObj->structId);
	ism_common_free(ism_memory_utils_log,clientLogObj);

	ism_common_freeHashMapEntriesArray(dataEntries);

	TRACE(7,"destroyClientLogObj: removed_count:%d keyStr=%s\n", removedCount, keyStr );


	return removedCount;
}

/*
 * create the key for the message id based on the message ID, client ID, and source IP.
 */
static int makeLogTableKey(int msgCode, const char * clientID, const char * sourceIP, const char * reason, concat_alloc_t *keyBuff)
{
	char buf[512];

	if (msgCode!=-1){
		sprintf(buf,"%d",msgCode);
		ism_common_allocBufferCopyLen(keyBuff, buf, strlen(buf));
	}
	if (clientID){
		ism_common_allocBufferCopyLen(keyBuff, clientID, strlen(clientID));
	}

	if (sourceIP){
		ism_common_allocBufferCopyLen(keyBuff, sourceIP, strlen(sourceIP));
	}

	if (reason){
			ism_common_allocBufferCopyLen(keyBuff, reason, strlen(reason));
	}

	if (keyBuff->used<=0){
		/*All empty. Create default key*/
		ism_common_allocBufferCopyLen(keyBuff, "default_key", 11  );
	}

	return keyBuff->used;

}


/*
 * Check and increase the count which this message ID had been logged.
 * @param msgCode	the message id
 * @param category	the message category
 * @param clientID	the client ID
 * @param sourceIP	the source IP address
 * @return the last count which this message ID had been logged.
 */
int ism_common_conditionallyLogged(ism_common_log_context *context, const ISM_LOGLEV level, ism_logCategory_t category, int msgCode, ism_trclevel_t * trclvl, const char * clientID, const char * sourceIP, const char * reason)
{

	int keysize=0;
	int rcount=0;
	int which=0;
    char tbuf[2048];
    void * clientLogItem = NULL;
    ismLogObj * logObj=NULL;
    ismClientLogObj * clientLogObj=NULL;

    /* Conditional logging is not used in the bridge */
    if (ism_common_isBridge())
        return 0;

    if (!logTableInited) return 0;



    concat_alloc_t keyBuffer = { tbuf, sizeof(tbuf), 0, 0 };
    keysize=makeLogTableKey(-1, clientID, sourceIP, NULL, &keyBuffer);
    char *key = alloca(keysize+1);
    memcpy(key, keyBuffer.buf, keysize);
    key[keysize++]='\0';
    ism_common_freeAllocBuffer(&keyBuffer);

    ism_time_t ctime= ism_common_currentTimeNanos();

    pthread_mutex_lock(&g_logtableLock);

    clientLogItem = ism_common_getHashMapElement(g_logtable, key, keysize);

    if (clientLogItem==NULL) {
    	/*Initialize the clientLog Object*/
    	clientLogObj= ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_log,169),1,sizeof(ismClientLogObj));
    	ism_log_setStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);
    	clientLogObj->msglogtable = ism_common_createHashMap(128,HASH_INT32);
	    ism_common_putHashMapElement(g_logtable, key, keysize, clientLogObj, NULL);
    } else {
     	clientLogObj = (ismClientLogObj *)clientLogItem;
     	ism_log_checkStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);
    }


    void * logItem = ism_common_getHashMapElement(clientLogObj->msglogtable, &msgCode, (int) sizeof(int));

    if (logItem==NULL){

    	logObj= ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_log,170),1,sizeof(ismLogObj));
    	ism_log_setStructId(logObj->structId, ismLOG_LOGOBJ_STRUCTID);
		logObj->msgcode = msgCode;
		logObj->timestamp_last_logged=ctime;

		ism_common_putHashMapElement(clientLogObj->msglogtable, &msgCode, sizeof(int), logObj, NULL);
		for (which=0; which<LOGGER_Max; which++) {
			if (g_logwriter[which] != NULL) {
				int kind = trclvl->logLevel[which];
				if (ism_log_filter(g_logwriter[which]->level+kind, level, category, msgCode)){
					logObj->shouldLogRepeated=1;
					break;
				}
			}
		}

    } else {
    	logObj = (ismLogObj *)logItem;
    	ism_log_checkStructId(logObj->structId, ismLOG_LOGOBJ_STRUCTID);
    }


    logObj->timestamp = ctime;

    //rcount=logObj->logcount++;
    logObj->logcount++;

    rcount = logObj->logcount_last;
    if (summarizeLogsEnable){

    	ism_time_t last_logged_time=logObj->timestamp_last_logged ;

		logObj->logcount_last++;

		//TRACE(8, "msgID: %d. rcount=%d. ctime=%llu. last_logged_time=%llu.summarize_logs_expire_time=%llu.\n", msgCode, rcount,(ULL)ctime,(ULL)last_logged_time,(ULL)summarize_logs_expire_time);
		if (rcount>0 && ctime - last_logged_time >= summarizeLogsIntervalInNano ){
			/*Log N times it happed*/
			if (logObj->shouldLogRepeated){
				char msgidBuf[32];
				sprintf(msgidBuf, "CWLNA%04d", msgCode%10000);
				//LOG(NOTICE, Server, 945, "%d%d%-s%-s%-s", "Repeating log entry - {0} events in last {1} minutes: ClientID={2}. ClientIP={3}. MessageID={4}.", logObj->logcount_last,summarizeLogsInterval, clientID, sourceIP, msgidBuf);
				ism_common_logInvoke(context, ISM_LOGLEV_NOTICE, 945, ISM_MSG_ID(945), category ,
								TRACE_DOMAIN, __FUNCTION__, __FILE__, __LINE__,
									"%d%d%-s%-s%-s%-s",
									"Repeating log entry - {0} events in last {1} minutes: ClientID={2}. ClientIP={3}. MessageID={4}. Reason={5}",
									logObj->logcount_last,summarizeLogsInterval, clientID, sourceIP, msgidBuf , (reason!=NULL)?reason:"");
			}
			logObj->logcount_last=0;
			logObj->timestamp_last_logged=ctime;
			rcount = 0; //Set this zero so the actual log line can be printed after the repeated log
		}
    }

    pthread_mutex_unlock(&g_logtableLock);
	return rcount;
}



/*
 * Get the count which the message ID had been logged.
 * @param msgCode 	the message ID
 * @param clientID	Client ID
 * @param sourceIP	source IP address
 * @return the count which this message ID had been logged.
 */
int ism_common_getLoggedCount(int msgCode, const char * clientID, const char * sourceIP, const char * reason)
{
	int rcount=0;
	int keysize=0;
	char tbuf[2048];
	void * logItem = NULL;
	void * logClientitem = NULL;
	ismLogObj * logObj=NULL;
	ismClientLogObj * clientLogObj=NULL;

	if (!logTableInited)
	    return 0;

	concat_alloc_t keyBuffer = { tbuf, sizeof(tbuf), 0, 0 };

	keysize=makeLogTableKey(-1, clientID, sourceIP, NULL, &keyBuffer);

	char *key = alloca(keysize+1);
	memcpy(key, keyBuffer.buf, keysize);
	key[keysize++]='\0';
	ism_common_freeAllocBuffer(&keyBuffer);

	pthread_mutex_lock(&g_logtableLock);
	logClientitem=ism_common_getHashMapElement(g_logtable, key, keysize);
	if (logClientitem!=NULL){

		clientLogObj = (ismClientLogObj *)logClientitem;
		ism_log_checkStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);
	    logItem = ism_common_getHashMapElement(clientLogObj->msglogtable, &msgCode, (int)sizeof(int));
	    if (logItem!=NULL){
	    	logObj =(ismLogObj *)logItem;
	    	ism_log_checkStructId(logObj->structId, ismLOG_LOGOBJ_STRUCTID);
	    	rcount= logObj->logcount;
	    }


	}
	pthread_mutex_unlock(&g_logtableLock);

	TRACE(7,"ism_common_getLoggedCount: count=%d msgcode=%d clientID=%s sourceIP=%s\n", rcount, msgCode, clientID, sourceIP );
	return rcount;


}

/*
 * Clean Up Task for Log Table
 * The clean up time is based on the configuration LogTableCleanUpInterval
 */
static int logTableCleanUpTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	ismLogObj * logObj=NULL;
	ismClientLogObj * clientLogObj=NULL;
	int removedCount=0;

	if (!logTableInited) {
		ism_common_cancelTimer(key);
		return 0;
	}

	pthread_mutex_lock(&g_logtableLock);

	ismHashMapEntry **dataEntries =  ism_common_getHashMapEntriesArray(g_logtable);
	int i=0;
	TRACE(7,"logTableCleanUpTimerTask: global table: totalcount:%d\n",ism_common_getHashMapNumElements(g_logtable) );
	while(dataEntries[i] != ((void*)-1)){
		clientLogObj = (ismClientLogObj *)dataEntries[i]->value;

		int clientMsgLogTableNum =0;
		int msgLogRemovedCount=0;

		char * keyStr = dataEntries[i]->key;
		assert(keyStr[dataEntries[i]->key_len-1] == '\0');

		if (clientLogObj!=NULL){
			ism_log_checkStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);

			clientMsgLogTableNum = ism_common_getHashMapNumElements(clientLogObj->msglogtable);

			ismHashMapEntry **dataMsgLogEntries =  ism_common_getHashMapEntriesArray(clientLogObj->msglogtable);
			int msgloccount=0;
			while(dataMsgLogEntries[msgloccount] != ((void*)-1)){
				logObj = (ismLogObj *)dataMsgLogEntries[msgloccount]->value;
				ism_log_checkStructId(logObj->structId, ismLOG_LOGOBJ_STRUCTID);

				ism_time_t logTimeStamp = logObj->timestamp;

				if ( timestamp >= ((logTimeStamp) + logObjectTTLTimeNano)) {
					int msgcode = logObj->msgcode;
					//Remove from the table.
					ism_common_removeHashMapElement(clientLogObj->msglogtable, dataMsgLogEntries[msgloccount]->key, dataMsgLogEntries[msgloccount]->key_len);
					ism_log_invalidateStructId(logObj->structId);
					ism_common_free(ism_memory_utils_log,logObj);

					msgLogRemovedCount++;
					TRACE(7,"logTableCleanUpTimerTask: removed log object from the client log table: key=%s msgcode=%d totalremoved=%d\n", keyStr,msgcode, msgLogRemovedCount);
				}
				msgloccount++;
			}
			ism_common_freeHashMapEntriesArray(dataMsgLogEntries);

			TRACE(7,"logTableCleanUpTimerTask: total log entries removed  the client log table: key=%s totalcount:%d\n", keyStr, msgLogRemovedCount);
		}

		if (msgLogRemovedCount==clientMsgLogTableNum){
			TRACE(7,"logTableCleanUpTimerTask: removing client log object from global log table: key=%s count=%d\n", keyStr, msgLogRemovedCount);
			destroyClientLogObj(clientLogObj, keyStr);
			ism_common_removeHashMapElement(g_logtable, dataEntries[i]->key,dataEntries[i]->key_len);

			removedCount++;
		}

		i++;
	}

	TRACE(7,"logTableCleanUpTimerTask removed client log entries: totalcount:%d\n",removedCount);

	ism_common_freeHashMapEntriesArray(dataEntries);

	pthread_mutex_unlock(&g_logtableLock);

    return 1;
}

/*
 * Set Summarize log Enable property
 */
int ism_log_setSummarizeLogsEnable(int enabled)
{
	summarizeLogsEnable=enabled;
	return summarizeLogsEnable;
}

/*
 * Set Summarize Log Interval
 */
int ism_log_setSummarizeLogsInterval(int interval)
{
	summarizeLogsInterval = interval;
	summarizeLogsIntervalInNano = summarizeLogsInterval* 60 * BILLION;
	return summarizeLogsInterval;
}


/*
 * Set Summarize Log Cleanup Interval
 */
int ism_log_setLogTableCleanUpInterval(int interval)
{
	logCleanUpTime=interval;
	logCleanUpTimeInNano =  logCleanUpTime * 60 * BILLION; //Minutes to Nanoseconds
	return logCleanUpTime;
}

/*
 * Call this SPI after timers are inited.
 * @param cleanup_time	time in secs
 */
int ism_log_startLogTableCleanUpTimerTask(void)
{

	if (logTableTimer && logCleanUpTaskStarted){
		TRACE(3,"Canceling LogTableCleanUpTimerTask. started=%d. Previous logCleanUpTimeInNano=%llu.\n", logCleanUpTaskStarted, (ULL)logCleanUpTimeInNano);
		ism_common_cancelTimer(logTableTimer);
		logTableTimer = NULL;
		logCleanUpTaskStarted=0;
	}

	TRACE(3,"Start LogTableCleanUpTimerTask. started=%d. logCleanUpTimeInNano=%llu. summarizeLogsIntervalInNano=%llu.\n", logCleanUpTaskStarted, (ULL)logCleanUpTimeInNano,(ULL)summarizeLogsIntervalInNano );
	if (logCleanUpTaskStarted==0){
		logCleanUpTaskStarted=1;
		logTableTimer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t)logTableCleanUpTimerTask, NULL, logCleanUpTimeInNano, logCleanUpTimeInNano, TS_NANOSECONDS);
		TRACE(3,"LogTable Clean Up Timer Task is set. CleanUpInterval: %d\n",logCleanUpTime);
	}

	return 0;
}


/*
 * Call this init after logInit
 */
int ism_log_initLogTable(void)
{

	if (!logTableInited){
		/*Init Log Table*/

		pthread_mutex_init(&g_logtableLock, 0);
		g_logtable = ism_common_createHashMap(128,HASH_STRING);

		/*Get the rate or time from a config*/
		logCleanUpTime = ism_common_getIntConfig("LogTableCleanUpInterval", 1);
		TRACE(3,"LogTableCleanUpTime Configuration: %d\n", logCleanUpTime);
		summarizeLogsEnable = ism_common_getIntConfig("SummarizeLogs.Enabled", 0);
		TRACE(3,"SummarizeLogsEnabled Configuration: %d\n", summarizeLogsEnable);
		summarizeLogsInterval = ism_common_getIntConfig("SummarizeLogs.Interval", 15);
		TRACE(3,"SummarizeLogsInterval Configuration: %d\n", summarizeLogsInterval);

		summarizeLogsIntervalInNano = summarizeLogsInterval* 60 * BILLION;
		logCleanUpTimeInNano =  logCleanUpTime * 60 * BILLION; //Minutes to Nanoseconds

		logObjectTTLTime = ism_common_getIntConfig("SummarizeLogObjectTTL", 60);
		logObjectTTLTimeNano = logObjectTTLTime * 60 * BILLION; //Minutes to Nanoseconds
		TRACE(3,"SummarizeLogObjectTTL Configuration: %d\n", logObjectTTLTime);


		logTableInited=1;
		TRACE(3,"LogTable is initialized.\n");
	}
	return 0;
}


/*
 * Terminate the log table for conditional logging
 */
int ism_log_termLogTable(void)
{
	if (logTableInited){
		logTableInited=0;
		pthread_mutex_lock(&g_logtableLock);

		ismHashMapEntry **array = ism_common_getHashMapEntriesArray(g_logtable);
		int i=0;
		ismClientLogObj * clientLogObj=NULL;
		while(array[i] != ((void*)-1)){
			clientLogObj = (ismClientLogObj *)array[i]->value;
			destroyClientLogObj(clientLogObj, array[i]->key);
			i++;
		}
		ism_common_freeHashMapEntriesArray(array);

		ism_common_destroyHashMap(g_logtable);
		g_logtable=NULL;

		pthread_mutex_unlock(&g_logtableLock);

		pthread_mutex_destroy(&g_logtableLock);
		TRACE(3,"LogTable is terminated.\n");

	}

	return 0;
}


int ism_log_removeConditionallyLoggedEntries(const char * clientID, const char * sourceIP)
{
	char tbuf[2048];
	int keysize=0;
	ismClientLogObj * clientLogObj=NULL;

	if (!logTableInited) return 0;

	pthread_mutex_lock(&g_logtableLock);


	if (ism_common_getHashMapNumElements(g_logtable)<1){
		pthread_mutex_unlock(&g_logtableLock);
		return 0;
	}
	TRACE(7,"ism_log_removeConditionallyLoggedEntries: clientID=%s sourceIP=%s\n", clientID, sourceIP);

	concat_alloc_t keyBuffer = { tbuf, sizeof(tbuf), 0, 0 };

	keysize=makeLogTableKey(-1, clientID, sourceIP, NULL, &keyBuffer);
	char *key = alloca(keysize+1);
	memcpy(key, keyBuffer.buf, keysize);
	key[keysize++]='\0';

	ism_common_freeAllocBuffer(&keyBuffer);

	void * clientLogItem = ism_common_removeHashMapElement(g_logtable, key, keysize);

	if (clientLogItem==NULL){
		pthread_mutex_unlock(&g_logtableLock);
		return 0;
	}

	clientLogObj = (ismClientLogObj *)clientLogItem;
	ism_log_checkStructId(clientLogObj->structId, ismLOG_CLIENTLOGOBJ_STRUCTID);
	int numItems = destroyClientLogObj(clientLogObj, key);

	pthread_mutex_unlock(&g_logtableLock);

	TRACE(7,"ism_log_removeConditionallyLoggedEntries: clientID=%s sourceIP=%s totalremovecount=%d\n", clientID, sourceIP, numItems);

	return numItems;

}

/*
 * Set Summarize Log Object Time To Live
 */
int ism_log_setSummarizeLogObjectTTL(int ttl)
{
	if (ttl<1)
		ttl=1;

	logObjectTTLTime = ttl;
	logObjectTTLTimeNano = logObjectTTLTime * 60 * BILLION; //Minutes to Nano

	TRACE(5, "SummarizeLogObjectTTL Configuration: ObjectTTL=%d. ObjectTTLInNanos=%llu\n",logObjectTTLTime, (ULL)logObjectTTLTimeNano);
	return 0;

}

/*
 * Get the default facility for a given logger.
 *
 * @param which A logger index.
 *
 * @return The syslog facility
 */
int getDefaultFacility(int which) {
	int facility;

	switch(which){
	case LOGGER_Security:
		facility = SECURITY_SYSLOG_FACILITY;
		break;
	case LOGGER_Admin:
		facility = ADMIN_SYSLOG_FACILITY;
		break;
	case LOGGER_Connection:
		facility = CONNECTION_SYSLOG_FACILITY;
		break;
	case LOGGER_MQConnectivity:
		facility = MQCONN_SYSLOG_FACILITY;
		break;
	case LOGGER_SysLog:
	default:
		facility = BASE_SYSLOG_FACILITY;
		break;
	}

	return facility;
}

/*
 * Get a shared process lock
 */
static pthread_mutex_t * getSharedProcessLock(void) {
    int isNew = 0;
    pthread_mutex_t * lock = NULL;
    int fd;
    void *addr;
    const char * shm_lock;
    size_t lockNameLength;

    if (ism_common_isBridge())
        shm_lock = SHARED_MEMORY_LOG_LOCK_BRIDGE;
    else if (ism_common_isProxy())
        shm_lock = SHARED_MEMORY_LOG_LOCK_PROXY;
    else
        shm_lock = SHARED_MEMORY_LOG_LOCK;

    lockNameLength = strlen(shm_lock)+1;

    if (sharedProcessLock) {
        return sharedProcessLock;
    }

    // We might need to qualify the name of this lock (e.g. for unit tests)
    char *qualifyShared = getenv("QUALIFY_SHARED");

    if (qualifyShared != NULL)
    {
        lockNameLength += strlen(qualifyShared) + 1; // +1 for colon
    }

    char lockName[lockNameLength];

    strcpy(lockName, shm_lock);

    if (qualifyShared != NULL)
    {
        strcat(lockName, ":");
        strcat(lockName, qualifyShared);
    }

    // Try to attach to an existing Posix shared memory object.
    fd = shm_open(lockName, O_RDWR, S_IRWXU);
    if (fd == -1) {
        // Try to create it instead. It's initially zero bytes long.
    	isNew = 1;
    	fd = shm_open(lockName, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
    	if (fd == -1) {
    		TRACE(1, "Failed to open shared memory object \"%s\" - errno %d.\n", lockName, errno);
    		return NULL;
    	}

    	if (ftruncate(fd, sizeof(pthread_mutex_t)) == -1) {
    		TRACE(1, "Failed to allocate memory in shared memory object \"%s\" - errno %d.\n", SHARED_MEMORY_LOG_LOCK, errno);
    		return NULL;
    	}
    }

    addr = mmap(NULL, sizeof(pthread_mutex_t),
           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
    	TRACE(1, "Failed to mmap shared memory object \"%s\" - errno %d.\n", SHARED_MEMORY_LOG_LOCK, errno);
    	return NULL;
    }

    lock = (pthread_mutex_t *)addr;

    if (isNew) {
		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
		pthread_mutex_init(lock, &mattr);
		pthread_mutexattr_destroy(&mattr);
    }

    return lock;
}

