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
/* Module Name: log.h		                      	                   	*/
/*                                                                   	*/
/* Description: 														*/
/*		Internal methods for log implementation.  These methods are     */
/*      not used outside of logging.                                    */
/*                                                                   	*/
/************************************************************************/
#ifndef __ISMLOG_DEFINED
#define __ISMLOG_DEFINED

#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>

/**
  * Log event information
  *
  * This structure contains information which is passed to the application for each
  * log entry.
  */
typedef struct ismLogEvent_t {
    struct ismLogEvent_t * next;
    int             log_level ;              /* Log level of msg (enum ISM_LOGLEV) */
    int             msgnum;
    char            msgid[24];
    int             count;                   /* Number of event parameters         */
    int             lineno;                  /* Location line number               */
    char            threadName[16];          /* Originating thread id              */
    ism_time_t      timestamp;               /* Timestamp when event was created   */
    int32_t 		category;
    uint32_t        size;
    uint8_t         levels [8];
} ismLogEvent_t;


/**
 * Log output information.
 *
 * This structure is used with the log announcer thread
 */
typedef struct ismLogOut_t  {
    ism_time_t      timestamp;
    char            threadName[16];
    char            msgid [24];
    int             msgnum;
    int             loglevel;
    int             category;
    int             count;
    int             lineno;
    int             size;
    const char * *  repl;
    const char *    msgf;
    const char *    func;
    const char *    location;
    const char *    sdelements;
} ismLogOut_t;

/*Log object*/
typedef struct ismLogObj_t {
#ifndef REMOVE_EXTRA_CONSISTENCY_CHECKING
		char structId[4];
#endif
		int msgcode;
		ism_time_t timestamp; //original or first timestamp
		ism_time_t timestamp_last_logged;
		int logcount_last;
		int logcount;
		int shouldLogRepeated;
} ismLogObj;

#define ismLOG_LOGOBJ_STRUCTID "LOGO"

/*Client Log object*/
typedef struct ismClientLogObj_t {
#ifndef REMOVE_EXTRA_CONSISTENCY_CHECKING
		char structId[4];
#endif
		ismHashMap * msglogtable; /*this contains the key, and ismLogObj as value*/
} ismClientLogObj;

#define ismLOG_CLIENTLOGOBJ_STRUCTID "CLOG"

#ifndef REMOVE_EXTRA_CONSISTENCY_CHECKING
static inline void ism_log_setStructId(char *structID, const char *newStructID) {
    *((uint32_t *)structID) = *((uint32_t *)newStructID);
}

static inline void ism_log_invalidateStructId(char *structID) {
	structID[0] = 'X';
}

static inline void ism_log_checkStructId(const char *structID, const char *expectedStructID){
    if(UNLIKELY(*((uint32_t *)structID) != *((uint32_t *)expectedStructID ))) {
        abort();
    }
}
#else
#define ism_log_setStructId(__structID, __newStructID)
#define ism_log_invalidateStructId(__structID)
#define ism_log_checkStructId(__structID, __expectedStructID)
#endif

/* RFC 5424 Structured data identifiers */
#define ismLOG_DEFAULT_SDID "ismsd"
// MessageSight's full assigned ID is 1.3.6.1.4.1.2.1.
//
// '1.3.6.1.4.1' comes from the issuing authority (iso.org.dod.internet.private.enterprise).
// '2' is IBM's Private Enterprise Number.
// '1' is the SubID assigned to MessageSight.
//
// From section 7.2.2 of the RFC document (http://www.rfc-base.org/txt/rfc-5424.txt), we need
// to qualify any structured data with the IANA-assigned private enterprise number and our
// sub-identifiers only, so for MessageSight this means "2.1".
#define ismLOG_MESSAGESIGHT_ENTERPRISE_ID "2.1"
#define ismLOG_IOT_SDID     "wiotp@"ismLOG_MESSAGESIGHT_ENTERPRISE_ID

typedef struct ism_logWriter_t ism_logWriter_t;

/*
 * Set of loggers
 */
enum AuxLogger {
    LOGGER_SysLog      = 0,    /* Base system log */
    LOGGER_Connection  = 1,    /* Connection log  */
    LOGGER_Security    = 2,    /* Security log     */
    LOGGER_Admin       = 3,    /* Administration log       */
    LOGGER_MQConnectivity = 4, /* MQConnectivity log     */
    LOGGER_Max         = 5     /* Max value (not a log)  */
};

enum FilterItemKind {
    FilterItem_Category      = 0,
    FilterItem_MsgId		 = 1,
};

typedef struct ismFilterItem_t{
	int kind;
	uint32_t category;
	uint32_t msgid;
	uint32_t level;
}ismFilterItem_t;



typedef struct ismSyslogConnection_t{
	int isconnected;
	SOCKET      sock;
	uint8_t     tcp;
	int         port;
	struct sockaddr_in6 ipv6addr;
	struct sockaddr_in  ipv4addr;
	const char * ip;
	uint8_t     ipv6;
}ismSyslogConnection_t;

/*
 * Binary form of log filter
 */
typedef struct ism_logFilter_t {
    uint32_t    level;                    /* Overall level                 */
    uint32_t    addCatCount;              /* Count of added categories     */
    uint32_t    delCatCount;              /* Count of deleted categories   */
    uint32_t    addMsgCount;              /* Count of added messages       */
    uint32_t    delMsgCount;              /* Count of deleted messages     */
    uint32_t    addCatAlloc;              /* Count of added categories     */
    uint32_t    delCatAlloc;              /* Count of deleted categories   */
    uint32_t    addMsgAlloc;              /* Count of added messages       */
    uint32_t    delMsgAlloc;              /* Count of deleted messages     */
    ismFilterItem_t *  addCatFilterItem;  /* Added categories              */
    ismFilterItem_t *  delCatFilterItem;  /* Deleted categories            */
    ismFilterItem_t *  addMsgFilterItem;  /* Added messages                */
    ismFilterItem_t *  delMsgFilterItem;  /* Deleted messages              */
} ism_logFilter_t;

/*
 * The logWriter structure is encapsulated.
 */
struct ism_logWriter_t {
    char *      destination;            /* Base file name                */
    uint32_t    currentCount;           /* Count of records              */
    uint32_t    fileSize;
    uint8_t     desttype;               /* Type of monitor 0=none, 3=syslog, 2=file, 4=callback */
    uint8_t     isfile;                 /* Do we need to close the file  */
    uint8_t     tcp;
    uint8_t     isconnected;
    uint8_t     overwrite;
    uint8_t     resv[2];
    int         facility;               /* Syslog facility               */
    FILE *      file;                   /* Open file structure           */
    ismSyslogConnection_t * syslog_conn;
    ism_logFilter_t  level[4];
};

#define DESTTYPE_FILE   2
#define DESTTYPE_SYSLOG 3
#define DESTTYPE_CALLBACK 4

#define SLOGTYPE_UDP   0
#define SLOGTYPE_TCP   1
#define SLOGTYPE_PIPE  2
#define SLOGTYPE_UNIX  3

#ifndef SA_ASSUME
#define SA_ASSUME(expression)
#endif


/* This is the link-local address of syslog server*/
#define DUMMY_LINKLOCAL_ADDR "127.0.0.1"

#define ISM_SYSLOG_PORT 514

/*Aux Log Settings*/
enum ism_auxlog_Settings {
    AuxLogSetting_Min   	  = 1,
    AuxLogSetting_Normal      = 2,
    AuxLogSetting_Max		  = 3,

};

#define BASE_SYSLOG_FACILITY			1   	/* Syslog User Facility          */
#define CONNECTION_SYSLOG_FACILITY		16		/* Syslog Local Use 1 Facility   */
#define SECURITY_SYSLOG_FACILITY		10		/* Syslog Security Facility      */
#define ADMIN_SYSLOG_FACILITY			15  	/* Syslog Local Use 0 Facility   */
#define MQCONN_SYSLOG_FACILITY          17      /* Syslog Local Use 2 Facility   */

#define BASE_SYSLOG_FILTER 					"900,-Connection:warn,-Security:warn,-Admin:notice,-MQConnectivity:error"

#define CONNECTION_SYSLOG_MIN_FILTER 		"Connection:warn,1111,900"
#define CONNECTION_SYSLOG_NORMAL_FILTER 	"Connection:notice,900"
#define CONNECTION_SYSLOG_MAX_FILTER 		"Connection:info,900"

#define SECURITY_SYSLOG_MIN_FILTER			"Security:warn"
#define SECURITY_SYSLOG_NORMAL_FILTER		"Security:notice"
#define SECURITY_SYSLOG_MAX_FILTER			"Security:info"

#define ADMIN_SYSLOG_MIN_FILTER				"Admin:warn"
#define ADMIN_SYSLOG_NORMAL_FILTER			"Admin:notice"
#define ADMIN_SYSLOG_MAX_FILTER				"Admin:info"

#define MQCONN_SYSLOG_MIN_FILTER            "MQConnectivity:warn"
#define MQCONN_SYSLOG_NORMAL_FILTER         "MQConnectivity:notice"
#define MQCONN_SYSLOG_MAX_FILTER            "MQConnectivity:info"


/**
 * Initialize loggers.
 *
 * This configures the loggers and must be called before using any log points.
 * This will call ism_log_init if it has not already been called.
 *
 * @param mqonly  If this is set to 1, only configure syslog and MQConnectivity logs.
 */
extern void ism_common_initLoggers(ism_prop_t * props, int mqonly);


/**
 * Initialize logging.
 * This is called early in the process to initialize logging locks.
 * After this is called you can invoke the other log functions, but no log points
 * can be used until after the logs are configured.
 *
 * @return A return code: 0=good
 */
extern int ism_log_init(void);

/**
 * Terminating logging.
 *
 * @return A return code: 0=good
 */
extern int ism_log_term(void) ;

/**
 * Set up a logger.
 *
 * @param which The logger index
 * @param props Configuration properties
 *
 * @return A return code: 0=good
 */
extern int ism_log_createLogger(int which, ism_prop_t * props);

/**
 * Update a logger.
 *
 * @param which The logger index
 * @param props Configuration properties
 *
 * @return A return code: 0=good
 */
extern int ism_log_updateLogger(int which, ism_prop_t * props);

/**
 * Write a log entry using the log writer.
 * @param logWriter A log writer instance.
 * @param logmsg  The log output structure to write.
 */
extern void ism_log_logWriter(void * logWriter, ismLogOut_t * logmsg);

/**
 * Write a log entry to the trace
 * @param logmsg  The log output structure
 */
extern void ism_log_toTrace(const ismLogOut_t * logmsg);

/**
 * Get the ID of a category by name.
 * It is assumed that this is not a frequent activity as it does a sequential match.
 */
int ism_log_getCategoryID(const char * category);

/**
 * Get the name of a category by ID.
 * @param id   The log id
 */
extern const char * ism_log_getCategoryName(int32_t id);

/**
 * Set the global logging level.
 *
 * This level applies to the system log.
 * @param level the new logging level
 */
extern void ism_log_setLevel(ISM_LOGLEV level);

/**
 * Get log level one character name
 * @param level the log level
 * @return the log level string
 */
extern char * ism_log_getLogLevelCode(ISM_LOGLEV level);

/**
 * Get the log level from the name
 * @param level  The level to get
 * TODO: change this
 */
extern int ism_log_getISMLogLevel(const char * level, ISM_LOGLEV * ismlevel);

/**
 * Get the trace level associated with a log level
 * @param level  The log level
 * @return The trace level
 */
extern int ism_log_getTraceLevelForLog(int level);

/**
 * Open a log writer
 *
 * @param destination  The destination file
 * @param which			logger type
 */
extern ism_logWriter_t * ism_log_openWriter(int which, const char * destination);

/**
 * Log to an ISM logger (file or syslog)
 */
extern void ism_log_toISMLogger(ism_logWriter_t * lw, int kind, ismLogOut_t * logout);


extern void ism_log_closeWriter(ism_logWriter_t * lw);

/**
 * Open the syslog
 */
extern int  ism_log_openSysLog(ism_logWriter_t * lw, int which, int facility);

/**
 * Close syslog
 */
extern int ism_log_closeSysLog(ism_logWriter_t * lw);

/**
 * Log to syslog
 */
extern void ism_log_logSysLog(ism_logWriter_t * lw, ismLogOut_t * logout, char * message);

/*
 * Create a log filter
 */
extern int ism_log_createFilter(ism_logFilter_t * lw, const char * filter);

/*
 * Filter the message based on the logwriter
 */
extern int ism_log_filter(ism_logFilter_t * lw, ISM_LOGLEV level, int32_t category, int32_t msgnum) ;

/*
 * Get Log level from the aux logger setting value
 */
extern const char * ism_log_getLogLevelFromAuxLoggerSetting(int32_t setting);

/*
 * Get Log Filter for the logger
 */
extern const char * ism_log_getLogFilterFromAuxLoggerSetting(int which, int32_t setting);

/**
 * Initialize syslog configuration from configuration properties.
 *
 * @param props  Configuration properties.
 * @return A return code: 0=good.
 */
extern int ism_log_initSyslog(ism_prop_t * props);

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
extern FILE * ism_log_findLogWriter(const char * buffer, int count);

#endif
