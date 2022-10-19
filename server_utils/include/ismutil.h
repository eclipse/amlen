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

#ifndef __ISMUTIL_DEFINED
#define __ISMUTIL_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismuints.h>
#include <tracecomponent.h>
#include <commonMemHandler.h>

#ifdef _WIN32
#include <ismutil_win.h>
#else
#include <stddef.h>

#ifndef XAPI
    #define XAPI extern
#endif
#ifndef xUNUSED
    #if defined(__GNUC__)
        #define xUNUSED __attribute__ (( unused ))
    #else
        #define xUNUSED
    #endif
#endif

#define XINLINE __inline__

#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#define strcmpi strcasecmp
#define strncmpi strncasecmp
typedef int SOCKET;
#include <dlfcn.h>
#define SOEXP ".so"
#define SOPRE "lib"
#define DLLOAD_OPT ,RTLD_LAZY
#define LIBTYPE void *
#define dlerror_string(str, len) ism_common_strlcpy((str), dlerror(), (len));
#include <signal.h>
#endif
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <ismrc.h>
#ifdef DMALLOC
#include "dmalloc.h"
#endif


/*
 * Define a platform independent thread handle
 */
typedef uintptr_t ism_threadh_t;

#ifdef _WIN32
typedef uintptr_t ism_threadkey_t;
#else
typedef pthread_key_t ism_threadkey_t;
#endif

#ifndef CONCAT_ALLOC_T
#define CONCAT_ALLOC_T

/**
 * Structure for allocated memory for a result buffer.
 * This is designed so that the buffer can be put on the stack, but if it overflows
 * it will be reallocated on the heap.
 */
typedef struct concat_alloc_t {
    char * buf;                /**< The buffer which can be reallocated    */
    int    len;                /**< The allocated length                   */
    int    used;               /**< The used length of the buffer          */
    int    pos;                /**< Buffering options                      */
    char   inheap;             /**< buf is in the heap and must be freed   */
    char   compact;            /**< json compact: 0=normal, 1=compact, 3=very compact */
    char   resv[2];
} concat_alloc_t;
#endif

/**
 * The time values structure
 */
typedef struct ism_timeval_t {
    int  year;         /**< Year: 1700-2240           */
    int  month;        /**< Month in year: 1-12       */
    int  day;          /**< Day in month: 1-31        */
    int  hour;         /**< Hour:    0-23             */
    int  minute;       /**< Minutes: 0-59             */
    int  second;       /**< Seconds: 0-60                 */
    int  nanos;        /**< Nanoseconds: 0-999999999      */
    int  dayofweek;    /**< 0=Sunday, 6=Saturday          */
    int  tzoffset;     /**< Timezone offset in minutes. >0 = east*/
} ism_timeval_t;

/**
 * Variable type enumeration
 */
enum ism_VariableTypes {
    VT_Null         = 0,       /**< Null value                    */
    VT_String       = 1,       /**< Type of String                */
    VT_ByteArray    = 2,       /**< Type of Byte Array            */
    VT_Boolean      = 3,       /**< Type of boolean               */
    VT_Byte         = 4,       /**< Signed 8 bit value            */
    VT_UByte        = 5,       /**< Unsigned 8 bit value          */
    VT_Short        = 6,       /**< Signed 16 bit value           */
    VT_UShort       = 7,       /**< Unsigned 16 bit value         */
    VT_Integer      = 8,       /**< Type of Integer               */
    VT_UInt         = 9,       /**< Type of Integer               */
    VT_Long         = 10,      /**< Type of Long Integer          */
    VT_ULong        = 11,      /**< Type of Long Integer          */
    VT_Float        = 12,      /**< Type of Float                 */
    VT_Double       = 13,      /**< Type of Double                */
    VT_Name         = 14,      /**< Name string                   */
    VT_NameIndex    = 15,      /**< Name integer                  */
    VT_Char         = 16,      /**< Character value               */
    VT_Map          = 17,      /**< Value is a map                */
    VT_Unset        = 18,      /**< Value not set                 */
    VT_Xid          = 19,      /**< XID value                     */
};


/**
 * Define a union type to allow any type to be contained
 */
typedef union  {
    int32_t  i;          /**< Integer value (int, byte, short) */
    uint32_t u;          /**< Unsigned int                     */
    int64_t  l;          /**< Long value                       */
    float    f;          /**< Float value                      */
    double   d;          /**< Double value                     */
    char *   s;          /**< Pointer value                    */
} anytype_t;             /**< Union of value types             */

/*
 * Field types
 */
typedef struct ism_field_t {
    enum ism_VariableTypes type;       /**< The type of the variable      */
    uint32_t  len;                     /**< Length of byte array          */
    anytype_t val;                     /**< The value of the field        */
} ism_field_t;


/**
 * XA tranaction ID
 */
#define XID_DATASIZE  128
typedef struct ism_xid_t {
    int32_t   formatID;          /* Format identifier    */
    int32_t   gtrid_length;      /* Value from 1-64      */
    int32_t   bqual_length;      /* Value from 1-64      */
    uint8_t   data[XID_DATASIZE]; /* gtrid and bqual data */
} ism_xid_t;
#define XID_HDRSIZE   12

/**
 * Encapsulated timestamp object
 */
typedef struct ism_ts_t ism_ts_t;
typedef struct ism_timezone_t ism_timezone_t;

/**
 * Define the common time object
 */
typedef uint64_t ism_time_t;


/**
 * Typedef for the thread start function
 */
typedef void * (* ism_thread_func_t)(void * parm, void * context, int value);

/**
 * Typedef for the thread cleanup function
 */
typedef void (* ism_thread_cleanup_func_t)(void * cleanup_parm);

typedef struct ism_thread_properties_t {
    int affLen;
    int priority;
} ism_thread_properties_t;

/*
 * Thread parameter structure
 */
typedef struct ism_thread_parm_t {
	ism_thread_func_t method;
	void * data;
    void * context;
    int    value;
    int    resv;
    char   tname[16];
    ism_thread_properties_t *properties;
} ism_thread_parm_t;


/**
 * Typedef for generic properties object
 */
typedef struct ism_prop_t ism_prop_t;

/**
 * Property entry with name and type
 */
typedef struct ism_propent_t {
    const char * name;
    ism_field_t   f;
} ism_propent_t;


/*
 * Domain based trace and log settings.
 * For each domain there is one for normal and one for selected.
 */
typedef struct ism_trclevel_t {
    struct ism_domain_t * domain;
    uint8_t        selected;
    uint8_t        trcLevel;
    uint8_t        resv [2];
    int            trcMsgData;
    uint8_t        trcComponentLevels[TRACECOMP_MAX];
    uint8_t        logLevel[8];
} ism_trclevel_t;

/*
 * Comparison values to select
 */
typedef struct ism_trcSel_t {
    char *         client_addr;    /* Comparison for client address with wildcard, or null */
    char *         clientid;       /* Comparison for clientID with wildcard, or null       */
    char *         userid;         /* Comparison for userID with wildcard, or null         */
} ism_trcSel_t;

/*
 * Domain object
 */
typedef struct ism_domain_t {
    char           struct_id [4];  /* Contains the string "DOM"  */
    uint32_t       id;             /* The ID of the domain (constrained to 16 bit) */
    const char *   name;           /* The name of the domain                       */
    ism_trclevel_t trace;          /* The normal trace settings                    */
    ism_trclevel_t selected;       /* The selected trace settings                  */
    ism_trcSel_t   select_connect; /* Selector strings, or null if there are no connection selectors */
} ism_domain_t;

/**
 * An array of enumeration names and values.
 *
 * The first entry in the array has the name of the enumeration as the name, and the count
 * of entries as the value.
 */
typedef struct ism_enumList {
    char *  name;
    int32_t value;
} ism_enumList;
#define INVALID_ENUM -999

/**
 * Look up the enum value given a name.
 * @param enumlist  A enumerated value list
 * @param value     A name to search for.
 * @return  The value matching the specified name, or INVALID_ENUM to indicate not found.
 */
XAPI int32_t ism_common_enumValue(ism_enumList * enumlist, const char * value);

/**
 * Look up the enum name given the value.
 * @param enumlist  An enumerated value list
 * @param value     A value to search for.  The name of the first matching value is returned.
 * @return  The name matching the specified value, or NULL to indicate not found.
 */
XAPI const char * ism_common_enumName(ism_enumList * enumlist, int32_t value);


/* *********************************************************************
 * Branch Prediction macros (only used for GCC)
 * These should not be overused, a study in the linux kernel found them
 * overused to the extent that turning many of them off made the code
 * faster.
 * http://lwn.net/Articles/420019/
 *
 */
#ifdef __GNUC__
#define UNLIKELY(a) __builtin_expect((a), 0)
#define LIKELY(a)   __builtin_expect((a), 1)
#define HOT         __attribute__((hot))
#define COLD        __attribute__((cold))
#define MALLOC      __attribute__((malloc))
#define NO_NULL(arg1...) __attribute__((nonnull(arg1)))
// Variables that are only used for debugging (e.g. assert calls) should be
// qualified with 'DEBUG_ONLY' to stop the compiler complaining when asserts
// are disabled (NDEBUG is set)
#ifdef NDEBUG
#define DEBUG_ONLY __attribute__((unused))
#else
#define DEBUG_ONLY
#endif
#else
#define UNLIKELY(a) (a)
#define LIKELY(a)   (a)
#define HOT
#define COLD
#define MALLOC
#define NO_NULL(arg1...)
#define DEBUG_ONLY
#endif

#define ISM_PROTYPE_SERVER  0
#define ISM_PROTYPE_MQC     1
#define ISM_PROTYPE_TRACE   2
#define ISM_PROTYPE_SNMP    3
#define ISM_PROTYPE_PLUGIN  4

/*
 * Special value for use on ism_engine_threadInit call to indicate
 * that no store stream should be opened. This should only be used by store
 * threads that know it is safe to do so, most threads are expected to have
 * either a non-critical (0) or critical (non-zero) store stream associated
 * with them.
 */
#define ISM_ENGINE_NO_STORE_STREAM 0xff

/* ********************************************************************
 *
 * Log and trace
 *
 **********************************************************************/

#define TOPT_TIME     0x01     /* Show timestamp       */
#define TOPT_THREAD   0x02     /* Show thread name     */
#define TOPT_WHERE    0x04     /* Show source location */
#define TOPT_ALLOPT   0x07     /* All base options     */
#define TOPT_NOGLOBAL 0x08     /* Do not combine with global options */
#define TOPT_CALLSTK6 0x10     /* Short call stack */
#define TOPT_CALLSTK  0x30     /* Show extended call stack */
#define TOPT_APPEND   0x80     /* Append */

extern ism_domain_t ism_defaultDomain;
extern ism_trclevel_t * ism_defaultTrace;


/*
 * Source modules which wish to use a different location for the trace level object
 * should make sure TRACE_DOMAIN is set to the object address before invoking TRACE.
 * This check allows it to be set before including ismutil.h.  If included later use
 * an #undef before defining it again.
 */
#ifndef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace
#endif


#ifndef _WIN32
/*
 * Type for trace function.
 * The standard trace function is ism_common_trace, but it can be replaced.
 * @param  level  The trace level 0-9
 * @param  opt    The trace options override
 * @param  file   The source file of the trace point.  Any leading path will be stripped.
 * @param  lineno The line number of the trace point.
 * @param  fmt    The trace format in printf syntax followed by a variable number of replacement values.
 */
typedef __attribute__ ((format (printf, 5, 6))) void (* ism_common_TraceFunction_t)(
        int level, int opt, const char * file, int lineno, const char * fmt, ...);

/*
 * Type for the trace data function.
 * The standard trace function is ism_common_traceData, but it can be repalced.
 * <p>
 * The actual number of bytes written is the smaller of the buflen and maxlen.
 * @param  label  A label to put before the data trace
 * @param  option The trace options override
 * @param  file   The source file of the trace point.  Any leading path will be stripped.
 * @param  where  The line number of the trace point.
 * @param  vbuf   The binary data to trace.
 * @param  buflen The length of the data to trace.  This length is itself traced.
 * @param  maxlen The maximum number of bytes to trace.  This allows the data to be constained while
 *                still logging how big the object actually is.
 *
 */
typedef void (*ism_common_TraceDataFunction_t)(const char * label, int option, const char * file,
                                           int where, const void * vbuf, int buflen, int maxlen);

/*
 * Type for the set error function.
 * The standard set error function is ism_common_setError(), but it can be replaced.
 * @param  rc     The return code
 * @param  file   The source file of the trace point.  Any leading path will be stripped.
 * @param  where  The line number of the trace point.
 */
typedef void (* ism_common_SetErrorFunction_t)(ism_rc_t rc, const char * filename, int where);

/*
 * Type for the set error with data function.
 * The standard set error with data function is ism_common_setErrorData(), but this can be replaced.
 * @param  rc     The return code
 * @param  file   The source file of the trace point.  Any leading path will be stripped.
 * @param  where  The line number of the trace point.
 * @param  fmt    The trace format in printf syntax followed by a variable number of replacement values.
 *
 */
typedef void (* __attribute__ ((format (printf, 4, 5)))ism_common_SetErrorDataFunction_t)(
        ism_rc_t rc, const char * filename, int where, const char * fmt, ...);

/*
 * Initialize a trace module.
 * @param props  Properties sent to the trace module
 * @param errorBuffer
 */
typedef int (* ism_common_TraceInitModule_t)(ism_prop_t * props, char * errorBuffer, int errorBuffSize, int * trclevel);

/*
 * Update the configuration of a trace module.
 * @param props  Properties sent to the trace module.
 */
typedef void (* ism_common_TraceModuleCfgUpdate_t)(ism_prop_t * props);

/*
 * External entry points for trace
 */
extern ism_common_TraceFunction_t traceFunction;
extern ism_common_TraceDataFunction_t traceDataFunction;
extern ism_common_SetErrorFunction_t setErrorFunction;
extern ism_common_SetErrorDataFunction_t setErrorDataFunction;


#endif

/*
 * Trace implementation.  This should be invoked using the TRACE command
 */
XAPI void ism_common_trace(int level, int opt, const char * file, int lineno, const char * fmt, ...);

/*
 * Trace data.
 *
 * This function is designed to be used internally by the TRACEDATA macro
 */
XAPI void ism_common_traceData(const char * label, int option, const char * file,
        int where, const void * vbuf, int buflen, int maxlen);

/* Function that update the utils in memory trace buffer */
//NB: NOT YET IMPLEMENTED - Currently ignore value for in memory trace and uses normal trace
#define VTRACE(inmemval, level, fmt...) TRACE(level, fmt)


// Constants for trace levels

// Component entry/exit
#define ISM_CEI_TRACE          7

// Function entry/exit for selected functions
#define ISM_FNC_TRACE          8

// Functions called so often they should require a higher trace level
#define ISM_HIFREQ_FNC_TRACE   9

// Server ought to be stopping, everything is going wrong
#define ISM_SEVERE_ERROR_TRACE 2

// It's gone wrong (e.g. for one queue)...but the server won't come down
#define ISM_ERROR_TRACE        4

// Something looks suspicious
#define ISM_WORRYING_TRACE     5

// Something a little unusual happened
#define ISM_UNUSUAL_TRACE      7

// Something generally interesting happened
#define ISM_NORMAL_TRACE       8

// Something interesting which happens with a high frequency
#define ISM_HIGH_TRACE         9

// First failure data capture
#define ISM_FFDC_TRACE         2

// Something interesting happened - such as last server timestamp
#define ISM_INTERESTING_TRACE  5

// When diagnosing perf problems these trace points may help....
#define ISM_PERFDIAG_TRACE     6

// To diagnose shutdown problems, we might need a higher level of trace.
#define ISM_SHUTDOWN_DIAG_TRACE ISM_INTERESTING_TRACE

/**
 * Create a trace entry as a printf format.
 *
 * The levels of trace allow more detailed trace to be turned off.
 * <ul>
 * <li> 1 = Trace events which normally occur only once Events which happen at init, start, and stop.
 *          This level can also be used when reporting errors but these are commonly logged as well.
 * <li> 2 = Configuration information and details of startup and changes to server state.
 *          This level can also be used when reporting external warnigs, but these are commonly logged as well.
 * <li> 3 = Infrequent actions as well as rejection of connection and client errors.
 *          This level can also be used when reporting external informational conditions and security issues
 *          but these are commonly logged as well.  We put these in the trace as well since the user will
 *          commonly disable this level of log.
 * <li> 4 = Details of infrequent action including connections, statistics, and actions which change states
 *          but are normal.
 * <li> 5 = Frequent actions which do not commonly occur at a per-message rate.
 * <li> 7 = Trace events which commonly occur on a per-message basis and call/return from external functions
 *          which are not called on a per-message basis.
 * <li> 8 = Call/return from internal functions
 * <li> 9 = Detailed trace used to diagnose a problem.
 * </ul>
 * Use of trace levels 8 or 9 might have a noticable impact on performance.  Use of trace levels of 4 and
 * below should have no impact on performance.
 * @param level A trace level indicator from 0 to 9.
 * @param fmt   A printf format
 * @param ...   The printf arguments
 */
#ifndef _WIN32
#define TRACE(level, fmt...) if (UNLIKELY((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])) \
    traceFunction((level), 0, __FILE__, __LINE__, fmt)
#define TRACEX(level, comp, opt, fmt...) if ((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(comp)]) \
    traceFunction((level), (opt), __FILE__, __LINE__, fmt)
#define TRACEL(level, lvl, fmt...) if (UNLIKELY((level) <= (lvl)->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])) \
    traceFunction((level), 0, __FILE__, __LINE__, fmt)
#define TRACEDATA(level, label, opt, data, buflen, maxlen) \
    if ((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)]) \
         traceDataFunction((label), (opt), __FILE__, __LINE__, (data), (buflen), (maxlen))
#define SHOULD_TRACE(level) UNLIKELY((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])
#define SHOULD_TRACEC(level, comp) UNLIKELY((level) <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(comp)])
#define SHOULD_TRACEL(level, lvl) UNLIKELY((level) <= (lvl)->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])
#define TRACE_LEVEL TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)]
#define TRACE_LEVELX(comp) TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(comp)]

#else
#define TRACE ism_common_trace
#endif

/**
 * Set the trace level to show.
 *
 * * DEPRECATED *
 * This method is depreciated because it only deals with the single trace level and
 * does not deal with trace components, shared tenancy, or selected objects.
 * <p>
 * Any trace entries at or the specified level or lower will be shown.
 * <p>
 * Normally the trace level is set up using the TraceLevel configuration value.
 * <p>
 * This sets all of the component trace levels for the default domain.
 *
 * @param level A level from 0 to 9.  A value of 0 disables all trace.
 * @return The previous trace level.
 */
XAPI int ism_common_setTraceLevel(int level);


/**
 * Get the trace level.
 *
 * * DEPRECATED *
 * This method is depreciated because it only deals with the single trace level and
 * does not deal with trace components, shared tenancy, or selected objects.
 * <p>
 * Any trace entries at or the specified level or lower will be shown.
 * @return The current trace level 0 to 9.  A value of 0 means that trace is disabled.
 */
XAPI int ism_common_getTraceLevel(void);

/**
 * Set the trace backup mode.
 * @param traceBackup The backup mode:
 * 		0 - 1 backup file, _prev.log suffix
 * 		1 - multiple compressed backup files
 * 		2 - offload compressed backup files
 */
XAPI void ism_common_setTraceBackup(int traceBackup);

/**
 * Get the trace backup mode.
 * @return The backup mode:
 * 		0 - 1 backup file, _prev.log suffix
 * 		1 - multiple compressed backup files
 * 		2 - offload compressed backup files
 */
XAPI int ism_common_getTraceBackup(void);

/**
 * Set the trace backup max file count. This value is used
 * only when the trace backup mode is 1.
 *
 * @param count The max number of trace backup files
 */
XAPI void ism_common_setTraceBackupCount(int count);

/**
 * Get the trace backup max file count.
 *
 * @return The current max number of trace backup files
 */
XAPI int ism_common_getTraceBackupCount(void);

/**
 * Set the remote destination for trace backup files. This value is used
 * only when the trace backup mode is 2.
 *
 * Only ftp:// and scp:// destinations are valid.
 *
 * @param destination The remote location for trace backup files.
 */
XAPI void ism_common_setTraceBackupDestination(const char * destination);

/**
 * Get the remote destination for trace backup files.
 *
 * @return The remote location for trace backup files.
 */
XAPI char * ism_common_getTraceBackupDestination(void);

/**
 * Set trace connection configuration.
 *
 * The selection string is a set of comma and/or space separated object=match options.
 * <br>The objects are: Endpoint, ClientID, or ClientAddr.
 * <br>The match string is a set of characters which can contain an asterisk to match 0 or more characters.
 * @param trconn A trace connection selection string
 * @return A return code, 0=good
 *
 */
int ism_common_setTraceConnection(const char * trcconn);

/**
 * Trace connection select by clientID.
 * @param clientID  The clientID of the connection to match
 * @return A return code 0=not selected, 1=selected
 */
XAPI int  ism_common_traceSelectClientID(const char * clientID);

/**
 * Trace connection select by clientID.
 * @param clientID  The client address of the connection to match
 * @return A return code 0=not selected, 1=selected
 */
XAPI int  ism_common_traceSelectClientAddr(const char * clientaddr);

/**
 * Trace connection select by endpoint.
 * @param clientID  The endpoint of the connection to match
 * @return A return code 0=not selected, 1=selected
 */
XAPI int  ism_common_traceSelectEndpoint(const char * endpoint);

/**
 * Set the trace level.
 *
 * Any trace entries at or the specified level or lower will be shown.
 * <p>
 * Normally the trace level is set up using the TraceLevel configuration value.
 * <p>
 * This sets all of the component trace levels for the default domain.
 *
 * @oaran trclevel  A trace level object
 * @param level  A level string
 * @return a return code: 0=good
 */
XAPI int ism_common_setTraceLevelX(ism_trclevel_t * trclevel, const char * level);


/**
 * Set the trace file name.
 * Close any existing trace file and start writing to the specified file.
 * <p>
 * Normally the trace file is set up using the TraceFile configuration value.
 *
 * @param  file  The trace file name
 * @param  append If non-zero, append to an existing file if is exists
 * @return A return code: 0=good
 */
XAPI int ism_common_setTraceFile(const char * file, int append);


/**
 * Set the trace options.
 *
 * The trace options are a space, tab, or comma separated list which can contain the following
 * enumerated values in any case: file, thread, or where.
 * @param str  The options string
 * @return A return code: 0=good
 */
XAPI int ism_common_setTraceOptions(const char * str);

/**
 * Return the max size of message data to trace.
 *
 * @return The max size of message data to trace
 */
XAPI int ism_common_getTraceMsgData(void);

/*
 * Set the max size of message data to trace.
 * A negative value is treated as zero.
 * @param msgdata  the max size of message data to trace
 */
XAPI void ism_common_setTraceMsgData(int msgdata);


/**
 * Set the trace filter.
 * * DEPRECATED *
 * This method is
 *
 * The trace filter are a space, tab, or comma separated list which can contain
 * The key=value pair for the trace component and trace level (0-9).
 * In addition, the several trace options can be set as keyword=value pairs where
 * the keyword is the option. Keywords can be in any case.
 * <p>
 * You can also colon as a separator for keyword and value.  This form must be used when
 * sending the TraceFilter from CLI as the CLI does not allow an equals (=) in the value
 * of a property.
 * <p>
 * The components which can be traced are: system, engine, http, jms, log, mqtt, protocol,
 * security, store, tcp, transport, util, monitoring.
 * <p>
 * The following options are allows:
 * <dl>
 * <dt>time = 0/1<dd>Show timestamps in the trace
 * <dt>where = 0/1<dd>Show location and file:line in the trace
 * <dt>thread = 0/1<dd>Show the thread name in the trace
 * <dt>msgdata = size<dd>The maximum size for user message data.
 * Set this to zero (the default) to not trace any user data in messages.  This can also be used to limit the
 * size of trace when using large messages.  If this is set non-zero then properties and message body might
 * be traced.  If the messages contain confidential data this should be set to 0.
 * </dl>
 * <p>
 * See the components defined in tracecomponents.h
 * @param str  The options string
 * @return A return code: 0=good
 */
XAPI int ism_common_setTraceFilter(const char * str);

/*
 * Get the trace filter.
 *
 * Get the last successful trace filter and place in the supplied buffer.
 * @param buf    The buffer to return the data
 * @param len    The length of the buffer
 * @return The length the the filter which might be larger than the supplied buffer
 */
XAPI int  ism_common_getTraceFilter(char * buf, int len);

/**
 * Set the trace max size.
 * @param tracemax   The maximum size to set.  This must be greater than zero.
 */
XAPI void ism_common_setTraceMax(uint64_t tracemax);

/**
 * Get the trace component ID given the component name.
 *
 * The search is done case independent.
 * @param component  The component name
 * @return The component ID or -1 to indicate that the name does not match any component
 */
XAPI int ism_common_getTraceComponentID(const char * component);

/**
 * Return the trace component name given the ID.
 * @param id  The component ID
 * @return The name of the component or "unknown" to indicate that the component is unknown.
 */
XAPI const char * ism_common_getTraceComponentName(int id);


/*
 * Initial portion of transport object used by other components.
 * This includes the various identifying fields.
 *
 * Any character stings in this object should be either constants or allocated using the
 * ism_transport_putString or ism_transport_allocBytes() methods.  If allocated in any other
 * way once set they must not be freed until the transport object is freed.
 *
 * The strings protocol, protocol_family,endpont_name, clientID, and name should always be set
 * to a non-null value (with might be a zero length string).
 *
 * This structure must be maintained to match ism_transport_t in transport.h and pxtransport.h.
 */
typedef struct ima_transport_info_t {
    const char *      protocol;          /**< The name of the protocol                       */
    const char *      protocol_family;   /**< The family of the protocol as: mqtt, jms, admin, mq  */
    const char *      client_addr;       /**< Client IP address as a string                  */
    const char *      client_host;       /**< Client hostname  (commonly not set)            */
    const char *      server_addr;       /**< Server IP address as a string                  */
    uint16_t          clientport;        /**< Port on the client                             */
    uint16_t          serverport;        /**< Port of the server                             */
    uint8_t           crtChckStatus;     /**< Status of client certificate check             */
    uint8_t           adminCloseConn;    /**< Flag if connection is closed administratively  */
    uint8_t           usePSK;
    uint8_t           originated;
    ism_domain_t *    domain;            /**< Domain object                                  */
    ism_trclevel_t *  trclevel;          /**< Trace level object                             */
    const char * 	  endpoint_name;
    struct ssl_st *   ssl;               /**< openssl SSL object if secure                   */
    const char *      userid;            /**< Primary userid                                 */
    char *            cert_name;         /**< Name from key/certificate                      */
    const char *      clientID;          /**< The client ID                                  */
    const char *      name;              /**< Connection name.  This is used for trace.  The clientID is commonly used    */
    uint32_t          index;             /**< The index of this transport entry              */
    int               monitor_id;        /**< The ID of this transport for monitoring        */
    uint32_t          tlsReadBytes;      /**< The size of the TLS headers read               */
    uint32_t          tlsWriteBytes;     /**< The size of the TLS headers written            */
} ima_transport_info_t;

/**
 * Protocol IDs
 *
 * All protocol IDs below 100 represent native protocols.
 * All protocol IDs 100 or above represent plug-in protocols.
 */
enum ism_protocol_id_e {
    PROTOCOL_ID_SHARED = 1,       /** The client is shared (internally used only)    */
    PROTOCOL_ID_MQTT   = 2,       /** The client is MQTT                             */
    PROTOCOL_ID_JMS    = 3,       /** The client is JMS                              */
    PROTOCOL_ID_MQ     = 4,       /** The client is MQ Connectivity                  */
    PROTOCOL_ID_FWD    = 5,       /** The client is a Remote Server forwarder        */
    PROTOCOL_ID_RMSG   = 6,       /** The native rest message                        */
    PROTOCOL_ID_HTTP   = 7,       /** The client is HTTP                             */
    PROTOCOL_ID_ENGINE = 8,       /** The client is an internal engine client        */
    PROTOCOL_ID_PLUGIN = 100      /** The client uses a plugin protocol              */
};

/*
 * Enumeration for expected message rate
 */
typedef enum {
    EXPECTEDMSGRATE_UNDEFINED = 0,
    EXPECTEDMSGRATE_LOW = 10,
    EXPECTEDMSGRATE_DEFAULT = 50,
    EXPECTEDMSGRATE_HIGH = 80,
    EXPECTEDMSGRATE_MAX = 100
} ExpectedMsgRate_t;


/* ********************************************************************
 *
 * Buffer functions
 *
 **********************************************************************/

/**
 * Free an allocated buffer.
 * @param buf   An initialized allocation buffer
 */
XAPI void ism_common_freeAllocBuffer(concat_alloc_t * buf);


/**
 * Copy a buffer into the allocated buffer object
 * @param buf     The allocation buffer
 * @param newbuf  The data to add
 * @param len     The length of the new data
 * @return the location of the copied data
 */
XAPI char * ism_common_allocBufferCopyLen(concat_alloc_t * buf, const char * newbuf, int len);

/**
 * Copy a buffer into the allocated buffer object
 * @param buf     The allocation buffer
 * @param newbuf  The string to add
 * @return the location of the copied data
 */
XAPI char * ism_common_allocBufferCopy(concat_alloc_t * buf, const char * newbuf);

/**
 * Allocate space in an allocation buffer
 * @param buf   The allocaton buffer
 * @param int   The length needed
 * @param align 0=1 byte alignment !0=8 byte alignment
 * @return the address of the allocated space
 */
XAPI char * ism_common_allocAllocBuffer(concat_alloc_t * buf, int len, int align);

/**
 * Allocate space in an allocation buffer directly on the heap
 * if the buffer has already been allocated then drop down to standard allocAllocBuffer
 * @param buf the allocation buffer
 * @param int the lenth needed
 */
XAPI char * ism_common_allocAllocBufferOnHeap(concat_alloc_t * buf, int len);


/* ********************************************************************
 *
 * Thread functions
 *
 **********************************************************************/

/**
 * Flags for start thread.
 * These can be ORd together.
 */
enum thead_flags_e {
    ISM_TFLAG_NONE      = 0,  /**< No flags                    */
    ISM_TFLAG_DETACHED  = 1,  /**< Start the thread detached   */
    ISM_TFLAG_SUSPENDED = 2,  /**< Start the thread suspended  */
};


/**
 * Usage for start thread.
 * The usage of a thread can be used to automatically set performance characteristics
 * for the thread.
 */
enum thread_usage_e {
    ISM_TUSAGE_NORMAL   = 0,  /**< This is a a normal thread, use default settings */
    ISM_TUSAGE_WORK     = 1,
    ISM_TUSAGE_LOW      = 2,  /**< This is a thread doing low priority work */
    ISM_TUSAGE_CLEANUP  = 3,  /**< This is a thread doing periodic maintenance */
    ISM_TUSAGE_TIMER    = 4,  /**< This is a timer thread */
    ISM_TUSAGE_ADMIN    = 5,  /**< This thread is used for administration */
    ISM_TUSAGE_MONITOR  = 6,  /**< This thread is used for monitoring */
    ISM_TUSAGE_IOREADER = 7,  /**< IO reader thread    */
    ISM_TUSAGE_IOWRITER = 8   /**< IO writer thread */
};


/**
 * Start a thread.
 * A thread is configured and started.
 * Three parameters are passed to the thread.  These are called data, context, and value, but what is
 * passed is not defined.
 *
 * @param  thread   The returned thread handle
 * @param  addr     The address of the function to execute
 * @param  data     The data parameter to pass to the thread.
 * @param  context  The context parameter to pass to the thread.
 * @param  value    The value parameter to pass to the thread.
 * @param  usage    A indication of the usage of the thread which can be used to set other performance values
 * @param  flags    A set of flags for this thread.  Use 0 if no flags are required.
 * @param  name     The name of the thread to create.  This name is used for debugging and should be 16 characters
 *                  or less.  The name is not required to be unique, but it is recommended that it be so.
 *                  If muliple threads are created which do the same processing, an counter can be appended to
 *                  the end of the name.
 * @param  description  The description of the thread for problem determination.  This does not need to be unique
 *                  if the name is unique.
 * @return A return code: 0=good
 */
XAPI int   ism_common_startThread(ism_threadh_t * handle, ism_thread_func_t addr, void * data, void * context, int value,
                 enum thread_usage_e usage, int flags, const char * name, const char * description);


/**
 * Return the thread handle for the current thread
 * @return The thread handle for the current thread
 */
XAPI ism_threadh_t ism_common_threadSelf(void);

/**
 * End this thread.
 * When a thread is ended it can return a value which is kept until the join is done by th3e parent.
 * If this thread is detached, the return value is not retained.
 * <p>
 * This call does not return.
 *
 * @param retval  The return value.  This is normally a scalar cast to a void *.  It can be a pointer
 *                and if it is care should be taken that the memory is kept until the thread is joined.
 *
 */
XAPI void  ism_common_endThread(void * retval);

/**
 * Get the name of the thread.
 * @param  buf  The buffer to return the thread name
 * @param  len  The length of the buffer to return the thread name
 * @return A length of the thread name, or 0 to indicate that the thread name is not available
 */
XAPI int ism_common_getThreadName(char * buf, int len);

/**
 * Wait for a thread to terminate and capture its return value.
 * @param  thread  The thread handle to joing
 * @param  retval  The location to capture the return value of the thread.
 * @return A return code 0=good
 */
XAPI int   ism_common_joinThread(ism_threadh_t thread, void * * retval);


/*
 * Wait for a thread to terminate for a given number of nanoseconds and
 * capture it's return value.
 * @param  thread  The thread handle to join
 * @param  retval  The location to capture the return value of the thread.
 * @param  timeout The number of nanoseconds to wait for the thread to join.
 * @return A return code 0=good
 */
XAPI int ism_common_joinThreadTimed(ism_threadh_t thread, void ** retvalptr, ism_time_t timeout);

/**
 * Cancel a thread.
 * This is a request for a thread to terminate itself.
 * @param  thread  The thread to cancel.
 * @return A return code: 0=good
 */
XAPI int   ism_common_cancelThread(ism_threadh_t thread);


/**
 * Detach a thread.
 * This is used after a thread is started and at a point where the thread will no longer be joined
 * in the case of a failure.  This can be done in either the parent or child thread.
 */
XAPI int   ism_common_detachThread(ism_threadh_t thread);


/**
 * Set thread affinity.
 * For each byte in the processor map, if the byte is non-zero the associated processor
 * is available for scheduling this thread.
 * @param thread  The thread to modify
 * @param map     An array of bytes where each byte represents a processor thread
 * @param len     The length of the map
 */
XAPI int   ism_common_setAffinity(ism_threadh_t thread, char * map, int len);

/**
 * Converts the thread affinity from 64 bit number to 64 bytes array
 * @param char - affinity as a string
 * @param char[64] - affinity as byte array
 * @return index of last non-zero byte in the array
 */
XAPI int ism_common_parseThreadAffinity(const char *affStr, char affMap[CPU_SETSIZE]);

/**
 * Sets a function for thread cleanup. This method needs to be called
 * from the thread to be cleaned up, NOT from the thread that called ism_common_startThread.
 * @param cleanup_function A function to be called at thread exit time.
 * @param cleanup_parm - A parameter to be passed to the cleanup routine.
 */
XAPI void ism_common_setThreadCleanup(ism_thread_cleanup_func_t cleanup_function, void * cleanup_parm);


/**
 * Make the thread-local storage for the current thread. Only needed when the thread is not created
 * using ism_common_startThread (e.g. threads coming in from the JNI)
 */
XAPI void ism_common_makeTLS(int need, const char* name);

/**
 * Free TLS memory for the current thread. Should be used to clean up
 * thread-local storage only if the thread was not created using ism_common_startThread
 */
XAPI void ism_common_freeTLS(void);

/**
 * Sleep for a time in microseconds.
 * Control should not return until this amount of time has elapsed, but it may be
 * longer before control is returned.
 * @param  micros  The time in microseconds to sleep.
 */
XAPI  void      ism_common_sleep(int micros);


/**
 * Method put in delay loop with various strategies.
 * @param stategy: 0=poll, 1=yield, 2=sleep;
 */
XAPI void ism_common_delay(int strategy);

/**
 * Returns a pointer to a (const) string that we use for
 * temporary data that is possibly (but not guarenteed) to be cleared
 * before our next start
 */
XAPI const char *ism_common_getRuntimeTempDir(void);

/**
 * Expands variable in path for a Unix Domain Socket
 */
XAPI int ism_common_expandUDSPathVars(char *expandedString, int maxSize, const char *inString);


 /**
  * Emergency shutdown
  *
  * This function does not return but will immediately shut down ths process without any cleanup
  * @param core   Force a core dump
  */
#define ism_common_shutdown(core) ism_common_shutdown_int(__FILE__, __LINE__, core);
 XAPI void  ism_common_shutdown_int(const char * file, int line, int core);

/* ********************************************************************
 *
 * String functions
 *
 **********************************************************************/

/**
 * Get a token from a string.
 * Note that the input string is modified by this call by placing terminator characters into the string.
 * @param from     The input string
 * @param leading  A string of characters which must be single byte characters.
 *                 If any of these characters exists at the beginning of the input string they are ignored.
 * @param trailing A string of characters which must be single byte characters.
 *                 If any of these characters is found in parsing the token, the token is ended.
 *                 This character in the input string is replaced with a 0x00 byte.
 * @param more     The return location within the input string following any trailing characters.
 * @return The found token or NULL to indicate that there are no characters left in the input string.
 */
XAPI char * ism_common_getToken(char * from, const char * leading, const char * trailing, char * * more);

/**
 * Return the count of tokens in a string.
 * @param str   The string to look for tokens
 * @param delim The set of allowed delimiters
 * @return the count of tokens
 */
int ism_common_countTokens(const char * str, const char * delim);

/**
 * Copy a length limited string guaranteeing a null terminator.
 *
 * There will always be a null terminator if the size if not zero.
 * This function does not guarantee truncation on a character boundary for UTF-8 strings.
 * @param dst   The destination buffer
 * @param stc   The source string
 * @param size  The size of the destination buffer
 * @return The size of the source string, which might be larger than the size.
 */
XAPI size_t ism_common_strlcpy(char *dst, const char *src, size_t size);


/**
 * Concatenate a length limited string guaranteeing a null terminator.
 *
 * There will always be a null terminator if the size if not zero.
 * This function does not guarantee truncation on a character boundary for UTF-8 strings.
 * @param dst   The destination buffer
 * @param stc   The source string
 * @param size  The size of the destination buffer
 * @return The size the resulting string would be, which might be larger than the size.
 */
XAPI size_t ism_common_strlcat(char *dst, const char *src, size_t size);

/**
 * Do a generic match using an asterisk wildcard which matches 0 or more characters
 *
 * @param str    The string to match
 * @param match  The match string
 * @return 0=does not match, 1=matches
 */
int ism_common_match(const char * str, const char * match);

/*
 * Return codes for ism_common_matchWithVars
 */
typedef enum {
    MWVRC_Matched            = 0,  /* Successful match */
    MWVRC_WildcardMismatch   = 1,  /* Match failed after wildcard */
    MWVRC_LiteralMismatch    = 2,  /* Match failed in literal text */
    MWVRC_MalformedVariable  = 3,  /* A non-terminated variable was found */
    MWVRC_UnknownVariable    = 4,  /* An unknown variable was found */
    MWVRC_NoUserID           = 5,  /* The connection does not have a user ID */
    MWVRC_NoClientID         = 6,  /* The connection does not have a client ID */
    MWVRC_NoCommonName       = 7,  /* The connection does not have a common name */
    MWVRC_UserIDMismatch     = 8,  /* Match failed in the userID */
    MWVRC_ClientIDMismatch   = 9,  /* Match failed in the ClientID */
    MWVRC_CommonNameMismatch = 10, /* Match failed in the CommonName */
    MWVRC_NoGroupID          = 11, /* Group ID is not set */
    MWVRC_GroupIDMismatch    = 12, /* Match failed in the groupID */
    MWVRC_MaxGroupID         = 13, /* Exceeded groupID limit */
} mwwrc_enum;

#define MAX_GROUPIDS 10

/*
 * Match a string with replacement values from a connection.
 *
 * The following variables are allowed in the match string:
 * <dl>
 * <dt>${ClientID}    <dd>Match the clientID from the connection
 * <dt>${CommonName}  <dd>Match the certificate common name from the connection
 * <dt>${UserID}      <dd>Match the userID from the connection
 * <dt>${GroupID}     <dd>Match the next groupID passed as an argument
 * <dt>${GroupID#}    <dd>Match the specific groupID passed as an argument
 * <dt>${*}           <dd>Match a literal asterisk
 * <dt>${$}           <dd>Match a dollar sign
 * </dl>
 *
 * @param str        The source string to compare
 * @param match      The match string which can contain variables
 * @param transport  The transport object
 * @param groupID    Array of group IDs
 * @param nogroups   Number of groups (max 10)
 * @param noCaseCheck  Match groups as case independent.  This only works before the first asterisk in the match string.
 * @return A return code from mwvrc_enum. 0=match
 *
 * If more than one ${GroupID} is in the match string, they are used in order starting with entry 0 in the group list
 */
int ism_common_matchWithVars(const char * str, const char * match, ima_transport_info_t * transport, char * groupID[], int noGroups, int noCaseCheck);

/*
 * Compute the CRC for an array of bytes.
 *
 * This is the CRC used by zip.
 *
 * This is the CRC used by zip and others.
 * It is used in Kafka message format 0 and 1.
 * @param crc   Starting CRC
 * @param buf   The array to process
 * @param len   The length of the buffer
 * @return  A CRC value
 */
uint32_t ism_common_crc(uint32_t crc, char * buf, int len);

/*
 * Compute the CRC32C for an array of bytes.
 *
 * This is the CRC implemented by Intel in the chip support and used in iSCSI.
 * The algorithm is the same as the zip CRC but the polynomial is different.
 *
 * It is used in Kafka message format 2.
 * @param crc   Starting CRC
 * @param buf   The array to process
 * @param len   The length of the buffer
 * @return  A CRC value
 */
uint32_t ism_common_crc32c(uint32_t crc, char * buf, int len);


/*
 * Fast 32 bit hash for a string
 * @param in  The input string
 * @return The fnv1a hash
 */
XAPI uint32_t ism_strhash_fnv1a_32(const char * in);

/*
 * Fast 32 bit hash for a string continued
 * @param in  The input string
 * @param hash The hast returned from a previous call
 * @return The fnv1a hash
 */
XAPI uint32_t ism_strhash_fnv1a_32_more(const char * in, uint32_t hash);

/*
 * Fast 32 bit hash for a byte array
 * @param in  The input array
 * @param len The length of the array
 * @return The fnv1a hash
 */
XAPI uint32_t ism_memhash_fnv1a_32(const void * in, size_t len);

/*
 * Fast 32 bit hash for a byte array continue
 * @param in  The input byte array
 * @param len The length of the array
 * @return The fnv1a hash
 */
XAPI uint32_t ism_memhash_fnv1a_32_more(const void * in, size_t len, uint32_t hash);

/* ********************************************************************
 *
 * Time functions
 *
 **********************************************************************/

/**
 * Return the current time in nanoseconds into the Unix era.
 * @return the current time in nanoseconds since 1970-01-01H00 ignoring leap seconds.
 */
XAPI ism_time_t ism_common_currentTimeNanos(void);

/**
 * Read and normalize the TSC.
 * Reading the TSC is fast but is unreliable as a timer source.  It can be used
 * when the restrictions are understood.  Since the epoch is unknown, this can
 * only be used to compare two timestamps on the same machine.
 * @return A time value is seconds relative to an arbitrary epoch.
 */
HOT XAPI double ism_common_readTSC( void );

/*
 * This function takes a tsc value as returned from isn_common_TSC() and
 * converts to a very approx ism_time_t (number of nanos since unix epoch)
 *
 * Note that the TSC *must* be from this execution of the program and not
 * persisted somewhere.
 */
ism_time_t ism_common_convertTSCToApproxTime(double normalisedTSC);

/**
 * Convert from an ISM time to a Java timestamp
 * @param ts   An ISM timestamp
 * @return A Java timestamp (milliseconds since 1970-01-01T00:00).
 */
XAPI int64_t ism_common_convertTimeToJTime(ism_time_t ts);

/**
 * Convert from a Java timestamp to an ISM timestamp
 * @param jts   A Java timestamp (milliseconds since 1970-01-01T00:00).
 * @return An ISM timestamp
 */
XAPI ism_time_t ism_common_convertJTimeToTime(int64_t jts);

/**
 * Flags for which kind of timestamp to open
 */
#define ISM_TZF_UNDEF      0  /**< UTC time is undefined             */
#define ISM_TZF_UTC        1  /**< UTC current time                  */
#define ISM_TZF_LOCAL      2  /**< Local timezone, time is undefined */

/**
 * Open a timestamp object.
 * As there is some cost associated with creating the timezone object, it is best to
 * create one and change the time when required.
 * @param  tzflag  The timezone flag (see ISM_TZF_*)
 * @return The timestamp object.
 */
XAPI ism_ts_t * ism_common_openTimestamp(int tzflag);

/**
 * Close the timestamp object and free its resources.
 * @param ts The timestamp object.
 */
XAPI void ism_common_closeTimestamp(ism_ts_t * ts);

/**
 * Set the timestamp object from an ISM timestamp.
 * @param ts   The timestamp object to set
 * @param time The ISM timestamp containing the timestamp
 */
XAPI void ism_common_setTimestamp(ism_ts_t * ts, ism_time_t time);

/**
 * Set the timestamp object from a set of time values.
 * The day of week field is never used when setting a timezone.
 * @param ts    The timestamp object.
 * @param vals  The array of time values.
 * @param settz If non-zero, set the timezone from the timestamp object.
 * @return A return code.  0=good.
 */
XAPI int ism_common_setTimestampValues(ism_ts_t * ts, ism_timeval_t * vals, int settz);

/**
 * Set the timestamp object from an ISO8601 string
 * @param ts       The timestamp object.
 * @param datetime The array of time values.
 * @return A return code.
 */
XAPI int ism_common_setTimestampString(ism_ts_t * ts, const char * datetime);

/**
 * Get the time from a timestamp object.
 * @param ts    The timestamp object.
 * @return The time from the timestamp object as an ISM timestamp
 */
XAPI ism_time_t ism_common_getTimestamp(ism_ts_t * ts);

/**
 * Get the time values from a timestamp object.
 * @param ts    The timestamp object.
 * @param values  The array of time values.
 * @return The address of the time value array.
 */
XAPI ism_timeval_t * ism_common_getTimestampValues(ism_ts_t * ts, ism_timeval_t * values);

/**
 * Flags for formatting options
 */
#define ISM_TFF_SPACE     1   /**< Use space as the separator between date and time  */
#define ISM_TFF_ISO8601   2   /**< Use ISO8601 timezone format (2:2)                 */
#define ISM_TFF_RFC822    4   /**< Use RFC822 timezone format like Java (4 digits)   */
#define ISM_TFF_FORCETZ   8   /**< Put out timezone even if not set                  */
#define ISM_TFF_MICROS   16   /**< Show 6 digits beyond second                       */
#define ISM_TFF_NANOS    32   /**< Show 9 digits behond second                       */
#define ISM_TFF_NANOSEP  64   /**< Use comma triad separator in nanoseconds          */
#define ISM_TFF_ISO8601_2 130 /**< Use the two digit ISO8601 format when possible    */

/**
 * Format the time from a timestamp object.
 * @param ts    The timestamp object.
 * @param buf   The buffer to return the time string.
 * @param len   The length of the output buffer
 * @param level The number of components of the timestamp to format
 * <br>     1 = yyyy
 * <br>     2 = yyyy-mm
 * <br>     3 = yyyy-mm-dd
 * <br>     4 = yyyy-mm-dd hh
 * <br>     5 = yyyy-mm-dd hh:mm
 * <br>     6 = yyyy-mm-dd hh:mm:ss
 * <br>     7 = yyyy-mm-dd hh:mm:ss.mmm
 * <br>     8 = hh
 * <br>     9 = hh:mm
 * <br>    10 = hh:mm:ss
 * <br>    11 = hh:mm:ss.mmm
 * @param flags Option flags
 * @return The address of the time buffer.
 */
XAPI char * ism_common_formatTimestamp(ism_ts_t * ts, char * buf, int len, int level, int flags);

/*
 * Set the timezone offset
 * @param The timestamp object
 * @param The timezone offset in minutes
 */
XAPI void ism_common_setTimezoneOffset(ism_ts_t * ts, int offset);

/*
 * Get a timezone object.
 * One copy of a timezone object is loaded for each time zone name.
 */
XAPI ism_timezone_t * ism_common_getTimeZone(const char * name);

/*
 * Get the name of a timezone object.
 */
XAPI const char * ism_common_getTimeZoneName(ism_timezone_t * timezone);

/*
 * Check a timezone object to account for changes in the offset
 */
XAPI int  ism_common_checkTimeZone(ism_timezone_t * timezone, ism_time_t now, ism_time_t * valid_untl);

/*
 * Get the current time to compare for expiration.
 * Round the time up to the next second.
 */
XAPI uint32_t ism_common_nowExpire(void);

/*
 * Convert a time to an expiration time.
 */
XAPI uint32_t ism_common_getExpire(ism_time_t ntime);

/*
 * Convert a time to an expiration time.
 */
XAPI uint32_t ism_common_javaTimeToExpirationTime(ism_time_t jtime);

/*
 * Convert an expiration time to a time
 */
XAPI ism_time_t ism_common_convertExpireToTime(uint32_t dtime);

/**
 * Format the date and time based on the locale.
 * @param locale	the locale
 * @param tmiestamp The time as milliseconds since 1970-01-01T00Z ignoring leap seconds
 * @param buf   The buffer to return the time string.
 * @return The total buffer size needed; if greater than input buffer length, the output was truncated.
 */
XAPI int32_t ism_common_formatMillisTimestampByLocale(const char * locale, int64_t timestamp, char * buf, int len);

/**
 * Format the nanosecond timestamp based on the locale.
 * @param locale	the locale
 * @param timestamp	The time as the number of nanaseconds since 1970-01-01T00Z ignoring leap seconds
 * @param buf  		The buffer to return the time string.
 * @return The total buffer size needed; if greater than input buffer length, the output was truncated.
 */
XAPI int32_t ism_common_formatTimestampByLocale(const char * locale, ism_time_t timestamp, char * buf, int len);


/**
 * Return the thread timestamp object with the current time set
 *
 * The returned timestamp object can be used to format the current time.  This object must only be used
 * in the current thread and only before a trace entry. This handles the issue of time zones changing
 * at hour boundaries (summer, winter).
 *
 * @param The time to set.  If 0 use the current time.
 * @return A thread local timestamp object set to the current time
 */
XAPI ism_ts_t * ism_common_getThreadTimestamp(void);

/*
 * Return a UUID epoch timestamp
 * The resolution is hundreds of nanoseconds, and the epoch is 1582-10-15T00Z ignoring leap seconds.
 * This is the timestamp used in version 1 (time based) UUIDs.
 * @return the UUID timestamp
 */
XAPI uint64_t ism_common_getUUIDtime(void);

/*
 * Convert a UUID time to a nanosecond timestamp
 * This code does not handle timestamps outside of the encoding range for nanosecond timestamps
 * @param uuid_time A UUID timestamp
 * @return a nanosecond timestamp
 */
XAPI ism_time_t ism_common_convertUTimeToTime(uint64_t uuid_time);

/*
 * Convert a timestamp to a UUID time
 * @param time  A timestamp in nanosecond resolution
 * @return A UUID timestamp
 */
XAPI uint64_t ism_common_convertTimeToUTime(ism_time_t time);

/*
 * Extract the UUID time from a string form type 1 UUID.
 * @param uuid  The UUID
 * @return the timestamp, or 0 if the UUID is not type 1 or is invalid.
 */
XAPI uint64_t ism_common_extractUUIDtime(const char * uuid);

/*
 * Construct a UUID
 * If the buffer length is 16 bytes, then a binary UUID is returned.
 * If the buffer length is greater than 36, then a string UUID is returned.
 *
 * @param buf  A buffer to put the put the UUID.
 * @param len  The length of the buffer.  This must be more than 36 or exactly 16.
 * @param time A timestamp for the UUID.  This is normally set to 0 which indicates that the
 *             current time should be used.  This is not used for type 4.
 * @param type 1=time based with MAC, 4=random, 17=time based with random node
 */
XAPI const char * ism_common_newUUID(char * buf, int len, int type, uint64_t time);

/*
 * Convert a binary UUID to a string UUID
 * @param uuid_bin  A binary UUID
 * @param buf       The output buffer
 * @param len       The length of the output buffer which must be at least 57 bytes
 * @return A string UUID or NULL in the case of an error
 */
XAPI const char * ism_common_UUIDtoString(const char * uuid_bin, char * buf, int len);

/*
 * Convert a string UUID to binary
 * This code just looks for 32 hex digits and allows any number of hyphen separators
 * @param uuid_str  The string format of the UUID
 * @param buf       The return buffer.  This must be at least 16 bytes
 * @return the buffer with the binary UUID or NULL to indicate an error
 */
XAPI const char * ism_common_UUIDtoBinary(const char * uuid_str, char * buf);


/* ********************************************************************
 *
 * Interface mapping functions
 *
 **********************************************************************/
#define IFMAP_TO_SYSTEM   0    /**< Map from virtual name to system name */
#define IFMAP_FROM_SYSTEM 1    /**< Map from system name to virtual name */
/**
 * Map an interface between the virtual interface name and the system interface name.
 *
 * The ISM server allows the names of network interfaces to be virtually renamed
 * and the renamed values are used in external configuration.  However, to use the
 * Linux APIs you need to know the interface name as known to the system.
 *
 * All tracing should be done using the virtual names.
 *
 * If the specified name is unknown, return the input map name.
 * @param ifname  The interface name to map
 * @param fromsystem  0=virtual to system, 1=system to virtual
 * @return The mapped name
 */
XAPI const char * ism_common_ifmap(const char * ifname, int fromsystem);


/*
 * Return if a interface name is known.
 *
 * @param ifname  The interface name to map
 * @param fromsystem  0=virtual to system, 1=system to virtual
 * @return A return code 1=known, 0=not known
 */
int ism_common_ifmap_known(const char * ifname, int fromsystem);


/* ********************************************************************
 *
 * Domain functions
 *
 **********************************************************************/

/**
 * Get the domain object by ID.
 * @param id   The ID of the domain
 * @return a domain object or NULL to indicate the ID is not a valid domain ID.
 */
ism_domain_t * ism_common_getDomainID(uint32_t id);

/**
 * Get the domain object by name.
 * @param name The name of the domain
 * @return a domain object or NULL to indicate the name is not known.
 */
ism_domain_t * ism_common_getDomain(const char * name);

/* ********************************************************************
 *
 * Config functions
 *
 **********************************************************************/

/**
 * Get a configuration property as a string
 * @param name   The name of the config item
 * @return the value of the configuration property, or NULL if it is not set
 */
XAPI const char * ism_common_getStringConfig(const char * name);

/**
 * Get a configuration property as an integer
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as an integer, or the default value if the config property
 *         is not set or cannot be converted to an integer.
 */
XAPI int          ism_common_getIntConfig(const char * name, int defval);

/**
 * Get a configuration property as a long
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as a long (64 bit) integer, or the default value if the config
 *         property is not set of cannot be converted to a boolean.
 */
XAPI int64_t      ism_common_getLongConfig(const char * name, int64_t defval);

/**
 * Get a configuration property as a boolean
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as a boolean (0=false, 1=true), or the default value if the config
 *         property is not set of cannot be converted to a boolean.
 */
XAPI int          ism_common_getBooleanConfig(const char * name, int defval);

/**
 * Get the configuration property object
 * @return the configuration properties object
 */
XAPI ism_prop_t * ism_common_getConfigProperties(void);

/*
 * Get the name of the server
 * @return The name of the server which is NULL if ot set
 */
XAPI const char * ism_common_getServerName(void);

/*
 * Get the unique ID of the server
 * @return The unique ID of the server (or NULL if it is not set)
 */
XAPI const char * ism_common_getServerUID(void);

/*
 * This is called to generate the server UID.
 * If the ServerUID is currently set, this will add a generation to it.
 * If the ServerUID is not set, this will create a new ServerUID.
 *
 * The ServerUID can only be modified before messaging is started.
 * @return The unique ID of the server
 */
XAPI const char * ism_common_generateServerUID(void);

/*
 * Get the name of the process.
 *
 * This should be the short form of the process name (without path)
 * @return The name of the process
 */
XAPI const char * ism_common_getProcessName(void);

/**
 * Return a string giving information about the platform.
 * This information is designed to aid in problem determination.
 * @return An information string about the platform
 */
XAPI const char * ism_common_getPlatformInfo(void);

/**
 * Return a string giving information about the platform.
 * This information is designed to aid in problem determination.
 * @return An information string about the processor
 */
XAPI const char * ism_common_getProcessorInfo(void);


/**
 * Return a string giving information about the platform.
 * This information is designed to aid in problem determination.
 * @return An information string about the operating system kernel
 */
XAPI const char * ism_common_getKernelInfo(void);


/**
 * Return the total memory available to MessageSight.
 * @return the total memory available to MessageSight in MBs
 */
XAPI uint64_t ism_common_getTotalMemory(void);


/**
 * Return a string giving information about the platform.
 * This information is designed to aid in problem determination.
 * @return An information string about the hostname
 */
XAPI const char * ism_common_getHostnameInfo(void);

/**
 * Get the build version.
 *
 * This might be zero length for unofficial builds
 * @return The version string set in the build
 */
XAPI const char * ism_common_getVersion(void);


/**
 * Get the build label.
 *
 * This might be zero length for unofficial builds
 * @return The build label set in the build
 */
XAPI const char * ism_common_getBuildLabel(void);

/* 
 * Get info to uniquely identify level of source used in the build.
 * For example a git commit hash or tag.
 *
 * May be 0-length if IMA_SOURCELEVEL_INFO env var not set during
 * build.
 *
 * @return string with source level identifier.
 */
XAPI const char * ism_common_getSourceLevel(void);

/**
 * Get the date and time of the build.
 *
 * This date and time are local to where the build was done and do not contain a timezone.
 * This is the time the component is built an can vary between components of the same build.
 * This is set for all builds.
 * @return The build date and time in YY-MM-DD HH;MM format
 */
XAPI const char * ism_common_getBuildTime(void);


//Types of CGroup we know (and care) about
typedef enum tag_ism_cgrouptype_t {
    ISM_CGROUP_UNIFIED = 0, //cgroup in v2 of cgroups (used in Fedora 31+) that has memory,cpu info (and all other cgroup info)
    ISM_CGROUP_CPU     = 2, //cgroup in v1 of cgroups (used in RHEL 7 and 8) that has memory info
    ISM_CGROUP_MEMORY  = 8, //cgroup in v1 of cgroups (used in RHEL 7 and 8) that has memory info
} ism_cgrouptype_t;

XAPI int ism_common_getCGroupPath(ism_cgrouptype_t groupType,
                                  char *buf,
                                  int buflen,
                                  char **outPath,
                                  int *pToplevelCGroup);

/**
 * Convert a config property name to its canonical name.
 *
 * The name is converted in place which requires that the canonical name be the same length or
 * shorter that the original name.  In almost all cases, the canonical name is used only to
 * allow any case of a property to be specified.
 * <p>
 * It the property name is not known it is still allowed, but the case cannot be corrected.
 *
 * @param name  The property name to convert.
 * @return A return code: 0=property is known, 1=property is unknown
 */
XAPI int ism_common_canonicalName(char * name);

/* ********************************************************************
 *
 * Properties
 *
 **********************************************************************/

/**
 * Simple properties object constructor.
 * There are several properties implementations in ISM, but they have the same basic
 * syntax.  A property is a name,type,value triplet.  These properties are used
 * as the properties on configuration and messaging objects.  Other protocols such
 * as JMS also define similar properties but with different constraints.
 * <p>
 * The properties object is no synchronized, and it is the responsibility of the
 * invoker to do so if required.
 * @param The initial length to allocate in bytes
 * @return A properties object
 */
XAPI ism_prop_t * ism_common_newProperties(int len);

/**
 * Properties object constructor which will not check for duplicate properties.
 */
XAPI ism_prop_t * ism_common_newAllowDupsProperties(int len);

/**
 * Free a properties object.
 * @param props  A properties object
 */
XAPI void ism_common_freeProperties(ism_prop_t * props);

/**
 * Get property count.
 * @param props A properties object
 */
XAPI int  ism_common_getPropertyCount(ism_prop_t * props);

/**
 * Get a property as a field.
 * @param props A properties object
 * @param name  The name of the property to get
 * @param field The field in which to return the property
 */
XAPI int  ism_common_getProperty(ism_prop_t * props, const char * name, ism_field_t * field);

/**
 * Get a property as a string.
 * If the object is not a string, it is converted as required to a string.
 * The memory is associated with the properties object and has the lifetime of
 * the properties object.
 * @param props  A properties object
 * @param name   The name of the object to return.
 */
XAPI const char * ism_common_getStringProperty(ism_prop_t * props, const char * name);

/**
 * Get a property as an integer.
 * If the object is not an integer, it is converted to an integer.  If it does not
 * exist or cannot be converted to an integer, the default value is returned.
 * @param props   The properties object
 * @param name    The name of the property
 * @param deflt   The default value of the property
 * @return The integer value of the property, or the default value if the property is missing
 *     or cannot be converted to integer.
 */
XAPI int  ism_common_getIntProperty(ism_prop_t * props, const char * name, int deflt);

/**
 * Get a property as an unsigned integer.
 * If the object is not an unsigned integer, it is converted to an integer.  If it does not
 * exist or cannot be converted to an unsigned integer, the default value is returned.
 * @param props   The properties object
 * @param name    The name of the property
 * @param deflt   The default value of the property
 * @return The unsigned integer value of the property, or the default value if the property is missing
 *     or cannot be converted to an unsigned integer.
 */
XAPI uint32_t ism_common_getUintProperty(ism_prop_t * props, const char * name, uint32_t deflt);

/**
 * Get a property as a long.
 * If the object is not a long integer, it is converted to a long.  If it does not
 * exist or cannot be converted to an integer, the default value is returned.
 * @param props   The properties object
 * @param name    The name of the property
 * @param deflt   The default value of the property
 * @return The integer value of the property, or the default value if the property is missing
 *     or cannot be converted to long.
 */
int64_t  ism_common_getLongProperty(ism_prop_t * props, const char * name, int64_t deflt);

/**
 * Get a property as an boolean.
 * If the object is not an boolean, it is converted to a boolean.  If it does not
 * exist or cannot be converted to a boolean, the default value is returned.
 * @param props   The properties object
 * @param name    The name of the property
 * @param deflt   The default value of the property
 * @return The integer value of the property, or the default value if the property is missing
 *     or cannot be converted to boolean.
 */
XAPI int  ism_common_getBooleanProperty(ism_prop_t * props, const char * name, int deflt);

/**
 * Set a property.
 * Properties objects are not synchronized, and thus it is the responsibility of the
 * invoker to gurantee that the set and get of properties do not happen at the same time.
 * @param props  A properties object
 * @param name   The name of the property
 * @param field  The field containing the value to set
 */
XAPI int  ism_common_setProperty(ism_prop_t * props, const char * name, ism_field_t * field);

/**
 * Add a property without checking for duplicates.
 * Properties objects are not synchronized, and thus it is the responsibility of the
 * invoker to gurantee that the set and get of properties do not happen at the same time.
 * @param props  A properties object
 * @param name   The name of the property
 * @param field  The field containing the value to set
 */
XAPI int ism_common_addProperty(ism_prop_t * props, const char * name, ism_field_t * field);

/**
 * Get a property name by index.
 * Properties have no implied order, and the order or index can change whenever the
 * properties object is modified.  This method allows for iteration thru a list of
 * properties.  Properties should not be modified while doing this iteration.
 * @param  props  A properties object
 * @param  index  An index into the set of properties, starting with 0
 * @param  name   The returned name of the property
 * @return A return code: 0=good  1=does not exist
 */
XAPI int  ism_common_getPropertyIndex(ism_prop_t * props, int index, const char * * name);

/*
 * Clear all properties
 * @param  props  A properties object
 * @return A return code: 0=good
 */
int ism_common_clearProperties(ism_prop_t * props);

/**
 * Convert a field to a string.
 * @param props  A properties object
 * @param field  The field to convert
 * @return  A string
 */
XAPI const char * ism_common_convertToString(ism_prop_t * props, ism_field_t * field);

/*
 * Convert a field to a string to return the value
 * @param f     The field
 * @param tbuf  A 64 byte temporary buffer
 * @return A string or NULL to indicate the field cannot be converted to a string
 */
XAPI const char * ism_common_fieldToString(ism_field_t * f, char tbuf[64]);

/**
 * Convert a properties object to a serialized properties map.
 *
 * Properties are written to the buffer at the used position in the buffer which is updated.
 *
 * The properties are written to a content object in serialized form.  The resulting buffer is a byte array and
 * can be moved as desired.  You only need the concat buffer over it to read or write it.
 *
 * @param props   The properties object to serialize
 * @param mapbuf  The buffer to place the serialized properties.
 */
XAPI int ism_common_serializeProperties(ism_prop_t * props, concat_alloc_t * mapbuf);

/**
 * Convert serialized properties to a properties object.
 *
 * Put all named fields in the specified buffer (from pos to used) into the properties object.
 * The properties object must be created before calling this function.
 *
 * @param  mapbuf The serialized properties
 * @param  props  The properties object
 * @return An ISMRC return code: 0=good, ISMRC_PropertiesNotValid if bad
 */
XAPI int ism_common_deserializeProperties(concat_alloc_t * mapbuf, ism_prop_t * props);

/**
 * Find a property by name in serialized properties.
 *
 * Search a set of properties for the named property, and return a field containing its value.
 * If the named property is not found return a non-zero return code and set the field to
 * a null field.
 * <p>
 * The position in the map buffer is not modified and the search is done from position 0
 * in the buffer.
 * @param mapbuf  The serialized properties buffer
 * @param name    The property name to search for
 * @param field   The field to return
 * @return A return code 0=found, 1=not found
 */
XAPI int ism_common_findPropertyName(concat_alloc_t * mapbuf, const char * name, ism_field_t * field);

/**
 * Find a property by id in serialized properties.
 *
 * Search a set of properties for the id property, and return a field containing its value.
 * If the named property is not found return a non-zero return code and set the field to
 * a null field.
 * <p>
 * The position in the map buffer is not modified and the search is done from position 0
 * in the buffer.
 * @param mapbuf  The serialized properties buffer
 * @param id      The property id to search for
 * @param field   The field to return
 * @return A return code 0=found, 1=not found
 */
XAPI int ism_common_findPropertyID(concat_alloc_t * mapbuf, int id, ism_field_t * field);

/**
 * Put a property into concise form.
 * @param mapbuf  The serialized properties buffer
 * @param name    The name of the property
 * @param field   The value of the property;
 */
XAPI void ism_common_putProperty(concat_alloc_t * mapbuf, const char * name, ism_field_t * field);

/**
 * Put an id property into concise form with an
 * @param mapbuf  The serialized properties buffer
 * @param id      The id of the property
 * @param field   The value of the property;
 */
XAPI void ism_common_putPropertyID(concat_alloc_t * mapbuf, int id, ism_field_t * field);

/*
 * The other values are in imacontent.h
 */
#define IDX_Topic 9  /* The publish topic name */

/* typedefs for register */
typedef int (* ism_serprop_f)(ism_prop_t * props, concat_alloc_t * mapbuf);
typedef int (* ism_deserprop_f)(concat_alloc_t * mapbuf, ism_prop_t * props);
typedef int (* ism_findpropname_f)(concat_alloc_t * mapbuf, const char * name, ism_field_t * field);
typedef void (* ism_putprop_f)(concat_alloc_t * mapbuf, const char * name, ism_field_t * field);

/* ********************************************************************
 *
 * Server initialization and termination
 *
 **********************************************************************/


/**
 * Start the ISM server.
 * All initialization of the server is done in this function, but it is not run.
 * If this method returns a good return code the server will be run.
 * @return A return code, 0=good.
 */
int	ism_common_initServer(void);

/**
 * Run the ISM server.
 * This call starts the server, and should return to the invoker when the server is started.
 * If this method returns a bad return code, the server will be immediately terminated.
 * @return A return code, 0=good.
 */
int  ism_common_startServer(void);

/**
 * Terminate the ISM server.
 * @return A return code, 0=good.
 */
int  ism_common_termServer(void);

/**
 * Initialize the ISM utilities.
 * This is done before the server initialization
 */
XAPI void  ism_common_initUtil(void);
XAPI void  ism_common_initUtilMain(void);


/* ********************************************************
 *                                                        *
 *       Hash map 					                      *
 *                                                        *
 ******************************************************** */
XAPI uint32_t ism_common_computeHashCode(const char * ptr, size_t length);

/**
 * The element object of a linked list
 */
typedef struct ismHashMapEntry_
{
 uint32_t                  hash_code;   /* the hash code of that element. Used for re-hashing    */
 int                       key_len;
 char                      *key;
 void                      *value;
 struct ismHashMapEntry_   *next;
} ismHashMapEntry;

/**
 * The hash map object
 */
typedef struct ismHashMap_t ismHashMap;

typedef enum ismHashFunctionType_t {
    HASH_INT32,
    HASH_INT64,
    HASH_STRING,
    HASH_BYTE_ARRAY
}ismHashFunctionType_t;
/**
 * Create HashMap object
 * Call ism_common_destroyHashMap to delete the HashMap object which return by
 * this function.
 * @param capacity initial capacity for the hash map
 * @return   ismHashMap object
 */
XAPI ismHashMap *      ism_common_createHashMap(uint32_t capacity, ismHashFunctionType_t hashType);

/**
 * Destroy HashMap object
 * @param hash_map the HashMap object
 */
XAPI void   ism_common_destroyHashMap(ismHashMap * hash_map);

typedef void (*ism_freeValueObject)(void *);

/**
 * Enumeration callback function.
 * @param entry   The hash map entry
 * @param context The returned context
 * @return 0=continue, !0=stop enumeration
 */
typedef int (* hash_enum_f)(ismHashMapEntry * entry, void * context);

/**
 * Destroy HashMap object and free values
 * @param hash_map the HashMap object
 * @param freeCB callback to free value object
 */
XAPI void   ism_common_destroyHashMapAndFreeValues(ismHashMap * hash_map, ism_freeValueObject freeCB);

/**
 * Lock HashMap object
 * @param hash_map the HashMap object
 */
XAPI void   ism_common_HashMapLock(ismHashMap * hash_map);
/**
 * Unlock HashMap object
 * @param hash_map the HashMap object
 */
XAPI void   ism_common_HashMapUnlock(ismHashMap * hash_map);

/**
 * Put an element into the map
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len length for the key
 * @param retvalue the previous value for the key if any.
 * @return  A return code, 0=good.
 */
XAPI int ism_common_putHashMapElement(ismHashMap * hash_map, const void * key, int key_len, void *value, void **retvalue);

/**
 * Get an element from the hash map base on the input key
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len length for the key
 * @return the element
 */
XAPI void *  ism_common_getHashMapElement(ismHashMap *hash_map, const void *key, int key_len);

/**
 * Remove the HashMap element based on the key.
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len length for the key
 * @return the element
 */
XAPI void *  ism_common_removeHashMapElement(ismHashMap * hash_map, const void * key, int key_len);

/**
 * Put an element into the map.
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @param retvalue the previous value for the key if any.
 * @return  A return code, 0=good.
 */
XAPI int ism_common_putHashMapElementLock(ismHashMap *hash_map, const void *key, int key_len, void *value, void **retvalue);

/**
 * Get an element from the hash map base on the input key with lock.
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @return the element
 */
XAPI void *  ism_common_getHashMapElementLock(ismHashMap * hash_map, const void *key, int key_len);

/**
 * Remove the hash map element based on the key with lock.
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @return the element
 */
XAPI void *  ism_common_removeHashMapElementLock(ismHashMap * hash_map, const void *key, int key_len);

/**
 * Get hash map elements number
 * @param hash_map the HashMap object
 * @return the number of the elements in the map
 */
XAPI int ism_common_getHashMapNumElements(const ismHashMap * hash_map);

/**
 * Get hash map Size.
 * @param hash_map the HashMap object
 * @return the size of the hashmap
 */
XAPI int ism_common_getHashMapSize(const ismHashMap * hash_map);

/**
 * Get hash map entries.
 * @param hash_map the HashMap object
 * @return the array with all entries in the map. Last element of the array is (void*)-1;
 */
XAPI ismHashMapEntry ** ism_common_getHashMapEntriesArray(ismHashMap * hash_map);

/**
 * Enumerate a hash map
 * @param hash_map   The HashMap object
 * @param callback   The function to call for each entry
 * @param context    A value to send to the callback
 * @return The return code from a callback or 0 to indicate all callbacks returned 0
 */
XAPI int ism_common_enumerateHashMap(ismHashMap * hash_map, hash_enum_f callback, void * context);

/**
 * Free hash map entries array returned by ism_common_getHashMapEntriesArray.
 * @param array
 */
XAPI void ism_common_freeHashMapEntriesArray(ismHashMapEntry **array);

XAPI void ism_hashMapInit();

/* ********************************************************************
 *
 * Data conversion functions
 *
 **********************************************************************/

/**
 * Convert a signed integer to ASCII.
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 */
XAPI char * ism_common_itoa(int32_t ival, char * buf);

/**
 * Convert a signed 64 bit integer to ASCII.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 24 bytes long
 */
XAPI char * ism_common_ltoa(int64_t lval, char * buf);

/**
 * Convert an unsigned 32 bit integer to hex
 * @param uval  The unsigned integer value.
 * @param shorten Flag of whether to remove leading 0 bytes
 * @param buf   The output buffer.
 */
XAPI char * ism_common_uitox(uint32_t uval, int shorten, char * buf);


/**
 * Convert an unsigned 32 bit integer to hex
 * @param uval  The unsigned integer value.
 * @param shorten Flag of whether to remove leading 0 bytes
 * @param buf   The output buffer.
 */
XAPI char * ism_common_ultox(uint64_t uval, int shorten, char * buf);

/**
 * Convert an unsigned 32 bit integer to ASCII.
 * @param  uval  The unsigned integer value.
 * @param  buf   The output buffer.  This should be at least 12 bytes long.
 */
XAPI char * ism_common_uitoa(uint32_t uval, char * buf);

/**
 * Convert an unsigned 64 bit unsigned integer to ASCII.
 * @param ulval  The unsigned long value
 * @param buf    The output buffer.  This should be at least 32 bytes long.
 */
XAPI char * ism_common_ultoa(uint64_t ulval, char * buf);

/**
 * Convert an unsigned 64 bit integer to ASCII with separators.
 * @param lval Unsigned long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 32 bytes long
 */
XAPI char * ism_common_ultoa_ts(uint64_t lval, char * buf, char triad);

/**
 * Convert signed integer to ASCII wit separators.
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 */
XAPI char * ism_common_itoa_ts(int32_t ival, char * buf, char triad);

/**
 * Convert a signed 64 bit integer to ASCII with separators.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 24 bytes long
 */
XAPI char * ism_common_ltoa_ts(int64_t lval, char * buf, char triad);

/**
 * Convert a float to ASCII
 * @param flt  The float value (this is passed as a double in C)
 * @param buf  The output buffer.  This should be at least 32 bytes long.
 */
XAPI char * ism_common_ftoa(double flt, char * buf);

/**
 * Convert a double to ASCII
 * @param dbl  The double value.
 * @param buf  The output buffer.  This should be at least 32 bytes long.
 */
XAPI char * ism_common_dtoa(double dbl, char * buf);

/**
 * Get a buffer size.
 * The buffer size is a string containing an optional suffix of K or M to indicate
 * thousands or millions.
 * @param cfgname  A configuration parameter name
 * @param ssize    A string representation of the parameter value
 * @param dsize    A string representation of the alternative parameter value
 * @return An integer value corresponding to ssize (or dsize, if ssize is NULL),
 *         if input is valid. Otherwise 0.
 */
XAPI int ism_common_getBuffSize(const char * cfgname, const char * ssize, const char * dsize);



/*
 * Format an integer to ASCII based on the locale
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
XAPI int32_t ism_common_formatInt(const char *locale, int32_t ival, char * buf) ;

/*
 * Format 64 bit integer to ASCII based on the locale.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 24 bytes long
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
XAPI int32_t ism_common_formatInt64(const char *locale, int64_t lval, char * buf);

/*
 * Convert a float or double to ASCII based on the locale
 * @param locale	the locale
 * @param flt  		The float value (this is passed as a double in C)
 * @param buf  		The output buffer.  This should be at least 32 bytes long.
 * @return The total buffer size needed; if greater than size of buf, the output was truncated.
 */
XAPI int32_t ism_common_formatDouble(const char * locale, double flt, char * buf);

/*
 * Format an unformatted number from a string to the decimal and based on the locale
 * @param locale	the locale
 * @param number	The number to format.
 * @param length	The length of the input number, or -1 if the input is nul-terminated.
 * @param buf  		The output buffer.  This should be at least 32 bytes long.
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
XAPI int32_t ism_common_formatDecimal(const char * locale, const char * number, int32_t length, char * buf);

/* ********************************************************************
 *
 * Timer functions
 *
 **********************************************************************/

#define ISM_PRIORITY_CLASS_COUNT	2

typedef enum ism_priority_class_e {
    ISM_TIMER_HIGH = 0,
	ISM_TIMER_LOW  = 1,
} ism_priority_class_e;

typedef struct TimerTask_t * ism_timer_t;

/**
 * Timer unit type enumeration
 */
enum ism_timer_e {
    TS_SECONDS      = 0,        /**< Period specified in seconds */
    TS_MILLISECONDS = 1,        /**< Period specified in milliseconds */
    TS_MICROSECONDS = 2,        /**< Period specified in microseconds */
    TS_NANOSECONDS  = 4         /**< Period specified in nanoseconds */
};

/**
 * Typedef for the callback function executing the task
 * @return 0 if periodic timer should not be scheduled any more
 */
typedef int (* ism_attime_t)(ism_timer_t key, ism_time_t timestamp, void * userdata);

/**
 * Schedule a task to be executed once.
 * @param  timer    The timer type (ISM_TIMER_HIGH/ISM_TIMER_LOW).
 * @param  attime   The address of the scheduled function to execute.
 * @param  userdata The pointer to the user data to be passed to the scheduled function.
 * @param  delay     The delay (in nanoseconds) before the function has to be executed.
 * @return The identifier for the scheduled task.
 * NOTE: Timer object has to be destroyed by invoking ism_common_cancelTimer method.
 */
XAPI ism_timer_t ism_common_setTimerOnceInt(ism_priority_class_e timer, ism_attime_t attime, void * userdata,
                                         ism_time_t delay, const char * file, int line);

#define ism_common_setTimerOnce(timer, attime, userdata, delay) \
    ism_common_setTimerOnceInt((timer), (attime), (userdata), (delay), __FILE__, __LINE__)

/**
 * Schedule a task to be executed repeatedly.
 * @param  timer    The timer type (ISM_TIMER_HIGH/ISM_TIMER_LOW).
 * @param  attime   The address of the scheduled function to execute.
 * @param  userdata The pointer to the user data to be passed to the scheduled function.
 * @param  delay    The delay before the first execution of the function, in units as specified by units parameter.
 * @param  period   The length of the interval between successive executions of the scheduled function,
 *                  in units as specified by units parameter.
 * @param  units    Time units used for delay and period parameters.
 * @return The identifier for the scheduled task.
 * NOTE: Timer object has to be destroyed by invoking ism_common_cancelTimer method.
 */
XAPI ism_timer_t ism_common_setTimerRateInt(ism_priority_class_e timer, ism_attime_t attime, void * userdata,
		                                 uint64_t delay, uint64_t period, enum ism_timer_e units, const char * file, int line);

#define ism_common_setTimerRate(timer, attime, userdata, delay, period, units) \
        ism_common_setTimerRateInt((timer), (attime), (userdata), (delay), (period), (units), __FILE__, __LINE__)

/**
 * Cancel a scheduled task.
 * @param  timer      An identifier for the task to be canceled.
 * @return A return code: 0=found and canceled, -1=not found.
 */
XAPI int   ism_common_cancelTimerInt(ism_timer_t timer, const char * file, int line);

#define ism_common_cancelTimer(timer) \
        ism_common_cancelTimerInt((timer), __FILE__, __LINE__)

/**
 * Monitor task schedule and invoke tasks accordingly.
 * @param  param   A pointer to the timer priority class.
 */
XAPI void * ism_common_timerListener(void * param, void * context, int value);

/**
 * Initialize timers for high- and low-priority events.
 */
XAPI void ism_common_initTimers(void);

/**
 * Stop timers for high- and low-priority events.
 */
XAPI void ism_common_stopTimers(void);

/**
 * Typedef for the callback function executing the task
 *
 * To remove the user handler, return -1.  All other return values will
 * leave the handler in place.
 */
typedef int (* ism_handler_f)(void * userdata);

typedef void * ism_handler_t;

/*
 * Add a handler for user signals.
 *
 * As there can be multiple users of the user signal, the handler must
 * check that the waited for event is complete.
 *
 * @param callback The callback method
 * @param userdata The userdata to send as data to the callback
 */
ism_handler_t ism_common_addUserHandler(ism_handler_f callback, void * userdata);

/*
 * Remove a user handler.
 *
 * This cannot be called from within a handler callback.  To remove a handler
 * while running a callback, return -1 from it.
 */
void ism_common_removeUserHandler(ism_handler_t handler);

/*
 * Get the signal number to use for the user signal
 */
int ism_common_userSignal(void);

/*
 * Run all handlers.
 * This is called when the signal is received
 */
void ism_common_runUserHandlers(void);


/* ********************************************************************
 *
 * Logger functions
 *
 **********************************************************************/

/**
 * Define the log levels.
 */
typedef enum ISM_LOGLEV {
	ISM_LOGLEV_CRIT				= 1,     /**< E = Critical 		*/
    ISM_LOGLEV_ERROR			= 2,     /**< E = Error 		*/
    ISM_LOGLEV_WARN				= 3,     /**< W = Warning 		*/
    ISM_LOGLEV_NOTICE			= 4,     /**< N = Notice 		*/
    ISM_LOGLEV_INFO				= 5,     /**< I = Info	 		*/
} ISM_LOGLEV;


#define ISM_LOGLEV_S_CRIT 	"C"
#define ISM_LOGLEV_S_ERROR 	"E"
#define ISM_LOGLEV_S_WARN 	"W"
#define ISM_LOGLEV_S_NOTICE	"N"
#define ISM_LOGLEV_S_INFO	"I"
#define ISM_LOGLEVELS "EWNIC"

/*Global ISM Message Prefix. This prefix is registered with IBM Software Group*/
#define ISM_MSG_PREFIX "CWLNA"
#define ISM_MSG_PREFIX0 "CWLNA0"
#define ISM_MSG_ID(ID)  ((ID>1000)?ISM_MSG_PREFIX#ID:ISM_MSG_PREFIX0#ID)
#define ISM_MSG_CAT(CAT) LOGCAT_##CAT
#define ISM_LOGLEV(SEV) ISM_LOGLEV_##SEV
typedef int32_t ism_logCategory_t;

#include <logcategory.h>

typedef struct ism_log_context_ {
    const char *topicFilter;
    const char *clientId;
    const char *resourceSetId;
} ism_common_log_context;

/**
 * Format a log message in external format
 * The log message is formatted and sent to the log writer. Normally, this API is not called
 * directly, instead the LOGnF macros are used.  This API must be used if this is a externalized
 * event and any replacement data exists.

 * @param severity   The logging level of this message. (Invalid Log levels won't get logged)
 * @param category   The logging category of the message
 * @param component  The component id for the product
 * @param msgnum     The message number without the prefix or the severity.
 * @param types      A string of length n, where n = the number of replacement items in the message.
 *
 * @param fmt        A format string in java MessageFormat style.  This may be null if the format
 *                   is available in the external properties file.
 * @param ...        The substitution values.  If the first entry of the types is '[', then
 *                   the substitution values are an array of uint64_t.
 */


#ifdef _WIN32
XAPI void ism_common_logInvoke(ism_common_log_context *context, const ISM_LOGLEV level, int msgnum, int32_t category, ism_trclevel_t trclvl,
                const char * method, const char * file, int line, const char * types, const char * fmts, ...);
#define LOG(sev, cat, msgnum, types, fmt,...) \
    ism_common_logInvoke(NULL, (ISM_LOGLEV_##sev), msgnum, (LOGCAT_##cat), TRACE_DOMAIN, __FUNCTION__, __FILE__, __LINE__, types, fmt, __VA_ARGS__);
#define LOGCTX(ctx, sev, cat, msgnum, types, fmt, ...)\
    ism_common_logInvoke(ctx, (ISM_LOGLEV_##sev), msgnum, (LOGCAT_##cat), TRACE_DOMAIN, __FUNCTION__, __FILE__, __LINE__, types, fmt, __VA_ARGS__);
#else
XAPI __attribute__ ((format (printf, 10, 12))) void ism_common_logInvoke(ism_common_log_context *context, const ISM_LOGLEV level, int msgnum, const char * msgID,
        int32_t category, ism_trclevel_t * trclvl, const char * method, const char * file, int line, const char * types, const char * fmts, ...);
#define LOG(sev, cat, msgnum, types, fmt...) \
	ism_common_logInvoke(NULL, (ISM_LOGLEV_##sev), msgnum, ISM_MSG_ID(msgnum),(LOGCAT_##cat), TRACE_DOMAIN, __FUNCTION__, __FILE__, __LINE__, types, fmt);
#define LOGCTX(ctx, sev, cat, msgnum, types, fmt...) \
    ism_common_logInvoke(ctx, (ISM_LOGLEV_##sev), msgnum, ISM_MSG_ID(msgnum),(LOGCAT_##cat), TRACE_DOMAIN, __FUNCTION__, __FILE__, __LINE__, types, fmt);
#endif


/* ********************************************************************
 *
 * Linked list functions
 *
 **********************************************************************/

/**
 * Linked list node structure
 */
typedef struct ism_list_node_ {
	void	*data;
	struct	ism_list_node_ *prev;
	struct	ism_list_node_ *next;
} ism_common_list_node;

/**
 * Linked list structure
 */
typedef struct ism_list_ {
	ism_common_list_node	*head;
	ism_common_list_node	*tail;
	void					(*destroy)(void *data);
	pthread_spinlock_t 		*lock;
	int						size;
	int						rsrv;
} ism_common_list;

/**
 * ism_list iterator structure, used to traverse linked lists
 */
typedef struct ism_listIterator_ {
	ism_common_list			*list;
	ism_common_list_node	*currNode;
	ism_common_list_node	*lastNode;
} ism_common_listIterator;

/********************************************************
*                                                       *
* ism_list functions                                    *
*                                                       *
********************************************************/

/**
 * This data structure thread-safe depends on synchronized flag passed during init time.
 * If false - all synchronization must be provided by caller. In particular, note that the iterator structure
 * becomes invalid if the current node is removed from the list. Caller
 * is also responsible for memory management of ism_list and ism_listIterator
 * structures and stored data.
 */

/**
 * Initialize a linked list.
 * ism_lists must be initialized before any other functions are called.
 * @param list      A pointer to space allocated for a ism_list
 * @param destroy   The function to use when destroying the list to free space allocated for data.
 * If this is null, no function will be called to free memory allocated for data.
 * @return 0 on success, -1 if memory could not be allocated
 */
XAPI int ism_common_list_init(ism_common_list * list, int synchronized, void (*destroy)(void * data));

/**
 * Destroys a linked list.
 * Removes all nodes from the list and frees space allocated for them. Applies the list's
 * destroy function to the stored data, if a destroy function has been specified. Does not free
 * the list. ism_common_list must be initialized again before use.
 * @param list      A pointer to the ism_common_list to be destroyed
 */
XAPI void ism_common_list_destroy(ism_common_list * list);

/**
 * Inserts data into a linked list as the first element.
 * Inserts data in a new node after the specified node in the list.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @return 0 on success, -1 if memory could not be allocated for new node
 */

XAPI int ism_common_list_insert_head(ism_common_list * list, const void * data);
/**
 * Inserts data into a linked list as the last element.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @return 0 on success, -1 if memory could not be allocated for new node
 */
XAPI int ism_common_list_insert_tail(ism_common_list * list, const void * data);

/**
 * Move elements from one list to another.
 * @param list1   A pointer to the list to be merged in
 * @param list2   A pointer to the list to be inserted
 * @return 0 on success, -1 if memory could not be allocated for new node
 */
XAPI int ism_common_list_merge_lists(ism_common_list * list1, ism_common_list * list2);

/**
 * Inserts data into an ordered linked list.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @param comparator
 * @return 0 on success, -1 if memory could not be allocated for new node
 */

XAPI int ism_common_list_insert_ordered(ism_common_list * list, const void * data, int (*comparator)(const void * data1, const void * data2));

/**
 * Removes first element from a linked.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be returned
 * @return 0 on success, -2 if list is empty
 */

XAPI int ism_common_list_remove_head(ism_common_list * list, void * * data);
/**
 * Removes last element from a linked.
 * @param list    A pointer to the list
 * @param data	 A pointer to the data to be returned
 * @return 0 on success, -2 if list is empty
 */

XAPI int ism_common_list_remove_tail(ism_common_list * list, void * * data);

/**
 * Returns the number of elements in the list
 * @param list      A pointer to the list
 * @return the number of elements in the list
 */
#define ism_common_list_size(list) ((list)->size)

/**
 * Makes an array of pointers to data from the list. The array parameter should be a pointer
 * to an unallocated pointer to pointer to void. Note that this function allocates memory to
 * the pointer to pointer to void. The length of the array is returned by the function.
 * Usage:
 *        void **array;
 *        int length = list_to_array(list, &array);
 * @param list      A pointer to the list
 * @param array     A pointer to the pointer to the array
 * @return length of array on success, -1 otherwise
 */
XAPI int ism_common_list_to_array(ism_common_list * list, void * * * array);

/**
 * Fills a list from an array of pointers to data. The list must be initialized and empty.
 * @param list      A pointer to the list
 * @param array     An array of void pointers (void **)
 * @param size		Length of the array
 * @return 0 on success, -1 if memory for array could not be allocated
 */
XAPI int ism_common_list_from_array(ism_common_list * list, void * * array, int size);

/**
 * Frees an array allocated during a list_to_array function call.
 * @param array     A pointer to the array
 */
XAPI void ism_common_list_array_free(void * * array);

/********************************************************
*                                                       *
* ism_common_listIterator functions                            *
*                                                       *
********************************************************/

/**
 * Initializes an iterator for a list.
 * If list is synchronized it will take the lock.
 * @param iter      A pointer to the iterator to initialize
 * @param list      A pointer to the list the iterator should traverse
 */
XAPI void ism_common_list_iter_init(ism_common_listIterator * iter, ism_common_list * list);
/**
 * Initializes an iterator for a list. If list is synchronized it'll free the lock
 * @param iter      A pointer to the iterator to initialize
 */
XAPI void ism_common_list_iter_destroy(ism_common_listIterator * iter);

/**
 * Resets the iterator to the beginning of its list.
 * @param iter      A pointer to the iterator to reset
 */
XAPI void ism_common_list_iter_reset(ism_common_listIterator * iter);

/**
 * Returns a pointer to the next node in the list and moves the iterator to the subsequent node.
 * @param iter      A pointer to the iterator
 * @return A pointer to the next node in the list or NULL if the next node
 * does not exist
 */
XAPI ism_common_list_node * ism_common_list_iter_next(ism_common_listIterator * iter);

/**
 * Tests whether more nodes exist in the list.
 * @param iter      A pointer to the iterator
 * @return 1 if more nodes exist, 0 otherwise
 */
XAPI int ism_common_list_iter_hasNext(ism_common_listIterator * iter);

/**
 *
 * Removes from the underlying linked list the last element returned by the
 * ism_common_list_iter_next.  This method can be called only once per
 * call to ism_common_list_iter_next.
 * @param list      A pointer to the list
 * @param node      A pointer to the iterator. If NULL, removes from the head of the list.
 * @param data	   A pointer to direct at the removed data
 * @return 0 on success, -3 if the ism_common_list_iter_next method has not
 *		  yet been called, or this method has already been called after the
 *		  last call to the ism_common_list_iter_next method, or if NULL was returned by the
 *		  last call  to the ism_common_list_iter_next.
 */
XAPI int ism_common_list_remove(ism_common_list * list, ism_common_listIterator * iter, void * * data);

/**
 * Removes all nodes from a linked list.
 * @param list      A pointer to the list
 */
XAPI void ism_common_list_clear(ism_common_list * list);

/**
 * Get the number of entries in a list
 * @param list   A list object
 */
XAPI int ism_common_list_getSize(ism_common_list *list);


/********************************************************
*                                                       *
* Thread local storage functions                        *
*                                                       *
********************************************************/

/**
 * Create a key for storage of thread-specific data.
 *
 * @param keyAddr pointer to key that will be filled in upon completion of function
 * @return integer return code. <0 on error.
 */
XAPI int ism_common_createThreadKey(ism_threadkey_t * keyAddr);

/**
 * Set thread-specific data associated with the supplied key
 *
 * @param key Key created using ism_common_createThreadKey
 * @param data Data to associate with this key
 * @return integer return code. <0 on error.
 */
XAPI int ism_common_setThreadKey(ism_threadkey_t key, void * data);

/*
 * Get thread-specific data associate with the supplied key
 *
 * @param key Key created using ism_common_createThreadKey
 * @param rc Pointer to integer that will be filled in upon completion
 * @return Thread-specific data.  Can be NULL if an error occurred or if data
 * was set to NULL.  Thus check the return code if NULL.
 */
XAPI void * ism_common_getThreadKey(ism_threadkey_t key, int * rc);

/*
 * Destroy the key for storage of thread-specific data.
 *
 * @param key Key created using ism_common_createThreadKey
 * @return integer return code. <0 on error.
 */

XAPI int ism_common_destroyThreadKey(ism_threadkey_t key);


/* *******************************************************
 *                                                       *
 * ism_common_bufferPool functions                       *
 *                                                       *
 ********************************************************/

typedef struct ism_byte_buffer_t * ism_byteBuffer;
typedef struct ism_byteBufferPool_t * ism_byteBufferPool;

typedef struct ism_byteBufferPool_t {
    ism_byteBuffer          head;
    pthread_mutex_t         mutex;
	pthread_spinlock_t     	lock;
	int 					allocated;
	int 					free;
	int 					bufSize;
	int 					minPoolSize;
	int 					maxPoolSize;
} ism_byteBufferPool_t;

/**
 *
 */
typedef struct ism_byte_buffer_t {
    struct ism_byte_buffer_t *  next;
	ism_byteBufferPool 			pool;
	char * 						buf;
	uint32_t					allocated;
	uint32_t					used;
	char * 						getPtr;
	char * 						putPtr;
	int							inuse;
} ism_byte_buffer_t;

/**
 * Allocate a buffer from the pool.
 *
 * @paream bufSize  The size to allocate
 * @return A buffer
 */
XAPI ism_byteBuffer ism_allocateByteBuffer(int bufSize);

/**
 * Free a buffer.
 *
 * @param bb   The buffer to free
 */
XAPI void ism_freeByteBuffer(ism_byteBuffer bb);

/**
 * Create a buffer pool
 */
XAPI ism_byteBufferPool ism_common_createBufferPool(int bufSize, int minPoolSize, int maxPoolSize, const char * name);

/**
 * Destroy a buffer pool
 */
XAPI void ism_common_destroyBufferPool(ism_byteBufferPool pool);

/**
 * Get information about a buffer pool.
 */
XAPI void ism_common_getBufferPoolInfo(ism_byteBufferPool pool, int *minSize, int *maxSize, int *allocated, int *free);

/**
 * Get a buffer from a pool
 */
XAPI ism_byteBuffer ism_common_getBuffer(ism_byteBufferPool pool, int force);

/**
 * Get a list of buffers from a pool
 */
XAPI ism_byteBuffer ism_common_getBuffersList(ism_byteBufferPool pool, int count, int force);

/**
 * Return a buffer to the pool
 */
XAPI void ism_common_returnBuffer(ism_byteBuffer bb, const char * file, int where);

/**
 * Return a list of buffers to the pool
 */
XAPI void ism_common_returnBuffersList(ism_byteBuffer head, ism_byteBuffer tail, int count);

XAPI void ism_bufferPoolInit();

struct ism_json_t;
XAPI void ism_utils_addBufferPoolsDiagnostics(struct ism_json_t * jobj, const char * name);
XAPI void ism_utils_traceBufferPoolsDiagnostics(int32_t traceLevel);

/********************************************************
*                                                       *
* ism monotonic cond_timedwait functions                *
*                                                       *
********************************************************/

/**
 * @Return the monotonic time in nanoseconds since some unspecified starting point.
 */
XAPI ism_time_t ism_common_monotonicTimeNanos(void);

/**
 * Init a pthread_cond_t object to use CLOCK_MONOTONIC
 * for time measurements in pthread_cond_timedwait()
 * @return 0 on success and pthread error o.w.
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
XAPI int ism_common_cond_init_monotonic(pthread_cond_t * restrict cond);
#else
XAPI int ism_common_cond_init_monotonic(pthread_cond_t * cond);
#endif

/**
 * A wraper function around pthread_cond_timedwait for
 * pthread_cond_t objects that were initialyzed to use
 * CLOCK_MONOTONIC for timing.
 * @param  fRelative = 0 => wtime contains the target absolute monotonic time
 *                   = 1 => wtime contains a relative time to wait
 * @return 0 on success and pthread error o.w.
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
XAPI int ism_common_cond_timedwait(pthread_cond_t * restrict cond, pthread_mutex_t * restrict mutex, const struct timespec * restrict wtime, uint8_t fRelative);
#else
XAPI int ism_common_cond_timedwait(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * wtime, uint8_t fRelative);
#endif

/********************************************************
*                                                       *
* ism globalization functions                           *
*                                                       *
********************************************************/

/**
 * Scan a UTF-8 string for validity and return the number of characters.
 *
 * This code checks that the byte string strictly conforms to the UTF-8 specification.
 * This requires that all characters be in canonical representation (shortest form).
 *
 * @param str  The string of bytes to check
 * @param len  The length of the string
 * @return A count of the unicode characters in the string or a negative value to
 *         indicate that the string is not a valid UTF-8 string.
 */
XAPI int ism_common_validUTF8(const char * str, int len);

/**
 * Scan a UTF-8 string for validity allowing no control codepoints except CR, LF, and HT.
 *
 * This is used primarily to distinguish between text strings and binary data.
 *
 * This code checks that the byte string strictly conforms to the UTF-8 specification.
 * This requires that all characters be in canonical representation (shortest form).
 *
 * @param str  The string of bytes to check
 * @param len  The length of the string
 * @return A count of the unicode characters in the string or a negative value to
 *         indicate that the string is not a valid UTF-8 string, or contains
 *         prohibited control characters.
 */
XAPI int ism_common_validUTF8NoCC(const char * str, int len);

/**
 * Lowercase a UTF-8 string returning an allocated string.
 *
 * @param  dest  The lower case of the of string which must be freed by the invoker
 * @param  src   A null-terminated string.
 * @return  An error if no bytes were written to the destination string or null destination.
 */
XAPI int ism_common_lowerCaseUTF8(char ** dest, const char * src);

/*
 * Lowercase a UTF-8 string into a buffer.
 * @param buf     The output buffer
 * @param buflen  The length of the output buffer
 * @param src     The source UTF-8 string
 * @param srclen  The number of bytes to convert or -1 for a null terminated string
 * @return A pointer to the lowercased string or to the src if the lower case failed
 */
XAPI const char * ism_common_lowerUTF8(char * buf, int buflen, const char * src, int srclen);

/*
 * Restriction values for ism_common_validUTF8Restrict
 * These values may be ORed together
 */
enum ism_UTF8Restrict {
    UR_None            = 0x0000,    /* No restrictions */
    UR_NoNull          = 0x0001,    /* Null not allowed */
    UR_NoC0Line        = 0x0002,    /* CR, LF, and HT not allowed */
    UR_NoC0Other       = 0x0004,    /* C0 controls other than CR, LF, and HT are not allowed */
    UR_NoC0            = 0x0007,    /* No C0 controls (0x0000 - 0x001F) are allowed */
    UR_NoC1            = 0x0008,    /* No C1 controls (0x0080 - 0x009F) are allowed */
    UR_NoControl       = 0x000F,    /* No C0 or C1 controls */
    UR_NoNonchar       = 0x0010,    /* No noncharacters are allowed */
    UR_NoPlus          = 0x0020,    /* Do not allow a plus */
    UR_NoHash          = 0x0040,    /* Do not allow a hash */
    UR_NoWildcard      = 0x0060,    /* Do not allow # or + */
    UR_NoSpace         = 0x0080,    /* Do not allow space  */
    UR_NoSlash         = 0x0100     /* Do not allow a slash */
};

/**
 * Scan a UTF-8 string for validity checking for restricted codepoints.
 *
 * This code checks that the byte string strictly conforms to the UTF-8 specification.
 * This requires that all characters be in canonical representation (shortest form).
 *
 * @param str  The string of bytes to check
 * @param len  The length of the string
 * @param notallowed  A mask of bits indicating what characters are not allowed
 * @return A count of the unicode characters in the string or a negative value to
 *         indicate that the string is not a valid UTF-8 string, or contains
 *         prohibited codepoints.  Note that if there are multiple problems, the
 *         return code only indicates the first problem.  In general all negative
 *         return values should be considered the same, but are separate for unit
 *         testing.
 * <br>    -1 = Contains a null
 * <br>    -2 = Contains a non-null C0 control
 * <br>    -3 = Contains a C1 control
 * <br>    -4 = Contains a non-character
 * <br>    -5 = Contains an invalid codepoint sequence
 * <br>    -6 = Contains a wildcard character
 * <br>    -7 = Contains a space character
 * <br>    -8 = Contains a slash
 *
 */
XAPI int ism_common_validUTF8Restrict(const char * str, int len, int notallowed);


/*
 * Replace any invalid bytes in a UTF-8 string.
 *
 * If the string is valid according to the constraints specified, the output string should
 * exactly match the input string.
 *
 * It is the responsibility of the invoker to allocate enough space in the output string
 * and a trailing null.  If the codepoint is outside ASCII-7, then it will take multiple bytes
 * to represent and this must be accounted for in the output buffer size.
 *
 * @param str  The input string
 * @param len  The length of the input string
 * @param out  The output string.  If this is NULL, the length is returned by no string is returned.
 * @param notallowed  A mask of bits indicating what characters are not allowed
 * @param repl The replacement character as a unicode codepoint.  If this is zero, no character will be replaced.
 *
 * @return a count of the bytes in the output sting.
 */
XAPI int ism_common_validUTF8Replace(const char * str, int len, char * out, int notallowed, uint32_t repl);

/**
 * Truncate a UTF-8 string with the specified number of characters.
 * The actual string will be at most one byte less to leave room for the
 * null terminator.
 * <p>
 * This should be called to truncate a UTF-8 string at a character boundary after
 * a function such as strlcpy() has been used.
 *
 * @param str     The string to truncate
 * @param maxlen  The maximum length of the string, including a byte for the null terminator
 */
XAPI void ism_commmon_truncateUTF8(char * str, int maxlen);

/**
 * Get the base sever locale name.
 *
 * @return The string for the base name of the server locale.
 */
XAPI const char * ism_common_getLocale(void);

/**
 * Get the ISM resource bundle for a locale.
 *
 * If the locale is NULL, use the server default locale.
 * @param locale the base locale name
 * @return A resource bundle for the locale, which might be NULL if resources for this locale are not found.
 */
XAPI struct UResourceBundle *  ism_common_getMessageCatalog(const char * locale);

/**
 * Get the ISM resource bundle from a specified path for a locale.
 *
 * If the locale is NULL, use the server default locale.
 * @param locale the base locale name
 * @return A resource bundle for the locale, which might be NULL if resources for this locale are not found.
 */
XAPI struct UResourceBundle *  ism_common_getMessageCatalogFromCatalogPath(const char *path, const char * locale);

/**
 * Get a message for the server default locale.
 *
 * The message is returned in UTF-8.  If the messages is truncated (xlen>=len) the message might not be valid.
 * @param msgid  The message ID used as the key in the resource
 * @param buf    The location to return the message
 * @param len    The length of the buffer
 * @param xlen   The returned length of the message.
 */
XAPI const char * ism_common_getMessage(const char * msgid, char * buf, int len, int * xlen);

/**
 * Get a message for a specified locale.
 *
 * The message is returned in UTF-8.  If the messages is truncated (xlen>=len) the message might not be valid.
 * @param msgid  The message ID used as the key in the resource
 * @param locale The locale name
 * @param buf    The location to return the message
 * @param len    The length of the buffer
 * @param xlen   The returned length of the message.
 */
XAPI const char * ism_common_getMessageByLocale(const char * msgid, const char * locale, char * buf, int len, int * xlen);

/**
 * Get a message for a specified locale and catalog dir.
 *
 * The message is returned in UTF-8.  If the messages is truncated (xlen>=len) the message might not be valid.
 * @param path   The Catalog directory path
 * @param msgid  The message ID used as the key in the resource
 * @param locale The locale name
 * @param buf    The location to return the message
 * @param len    The length of the buffer
 * @param xlen   The returned length of the message.
 */
XAPI const char * ism_common_getMessageByLocaleAndCatalogPath(const char *path, const char * msgid, const char * locale, char * buf, int len, int * xlen);

/**
 * Get a message for the specified locale.
 *
 * The message is returned in UTF-8.  If the messages is truncated (xlen>=len) the message might not be valid.
 * @param locale The name of a locale
 * @param msgid  The message ID used as the key in the resource
 * @param buf    The location to return the message
 * @param len    The length of the buffer
 * @param xlen   The returned length of the message.
 */
XAPI const char * ism_common_getLocaleMessage(const char * locale, const char * msgid, char * buf, int len, int * xlen);


/**
 * Format a message
 * @param msgBuff 		the message buffer
 * @param msgBuffSize 	the size of the message buffer
 * @param msgFormat		the message format
 * @param replData		the replacement data
 * @param replDataSize	the replacement data size
 */
XAPI size_t ism_common_formatMessage(char * msgBuff, size_t msgBuffSize, const char * msgFormat,
        const char * * replData, int replDataSize);

/**
 * Set the request locale into the thread local storage
 * @param 	localekey		the pthread key for locale
 * @param 	locale 		the locale
 * @return 	If successful, the function shall return zero; otherwise,
 *          an error number shall be returned to indicate the error.
 */
XAPI int ism_common_setRequestLocale(pthread_key_t localekey, const char * locale) ;

/**
 * Get the request locale from the thread local storage
 * @param 	localekey		the pthread key for locale
 * @return 	the locale
 *
 */
const char * ism_common_getRequestLocale(pthread_key_t localekey) ;

/********************************************************
*                                                       *
* ism return code functions  	                        *
*                                                       *
********************************************************/

/**
 * Return the base error string for a return code.
 *
 * This error string is in message format and may have place holders for replacement data.
 * @param code   The error code
 * @param buffer The input buffer to contain the formatted string
 * @param len    The length of the buffer
 * @return The length of the data to return.  This might be longer than the available length of the buffer.
 */
XAPI const char * ism_common_getErrorString(int code, char * buffer, int len);

/**
 * Return the text name for a return code.
 *
 * This error string is in message format and may have place holders for replacement data.
 * @param code   The error code
 * @param buffer The input buffer to contain the formatted string
 * @param len    The length of the buffer
 * @return The length of the data to return.  This might be longer than the available length of the buffer.
 */
XAPI const char * ism_common_getErrorName(ism_rc_t code, char * buffer, int len);

/**
 * Return the error string by locale.
 *
 * This error string is in message format and may have place holders for replacement data.
 * @param code   The error code
 * @param locale The locale name to use.  If not found, the system will choose a locale.
 * @param buffer The input buffer to contain the string
 * @param len    The length of the input buffer
 * @return The length of the data to return.  This might be longer than the available length of the buffer.
 */
XAPI const char * ism_common_getErrorStringByLocale(ism_rc_t code, const char *locale, char * buffer, int len);

/**
 * Return the last error for this thread.
 * Normally only errors are recorded, but if ism_common_setError() is called with a zero return code,
 * the error is reset and zero will be returned until another error is found.
 * @return The error code
 */
XAPI int  ism_common_getLastError(void) ;

/**
 * Get the Last Error replace data if any.
 * @param inbuf the concat_alloc_t object which contain the buffer of the replacement data
 * @return the size of the replace data
 */
XAPI int ism_common_getLastErrorReplData(concat_alloc_t * inbuf);

/**
 * Get the replace data string from replace data buffer.
 * @param buf 	replacement data buffer.
 * @return the replacement string.
 */
XAPI const char * ism_common_getReplacementDataString(concat_alloc_t * buf);

/*
 * Set a simple error without replacement data.
 *
 * If the return code is zero, the last error for this thread is reset.
 * @param rc     The return code value
 */
#ifdef DOXYGEN
void ism_common_setError(ism_rc_t rc);
#else
XAPI void ism_common_setError_int(ism_rc_t rc, const char * filename, int where);
#ifndef ism_common_setError
#define ism_common_setError(rc) setErrorFunction((rc), __FILE__, __LINE__)
#endif
#endif

/*
 * Set an error with replacement data.
 *
 * If the return code is zero, the last error for this thread is reset.
 * @param rc     The return code value
 * @param types  The types of the replacement data in
 * @parem ...    The replacement data as variable arguments
 */
#ifdef DOXYGEN
void ism_common_setErrorData(ism_rc_t rc, types, ...);
#else
#ifdef _WIN32
XAPI void ism_common_setErrorData_int(ism_rc_t rc, const char * filename, int where, const char * types, ...) ;
#define ism_common_setErrorData(rc, types, ...) setErrorDataFunction((rc), __FILE__, __LINE__, types, __VA_ARGS__)
#else
XAPI __attribute__ ((format (printf, 4, 5))) void ism_common_setErrorData_int(ism_rc_t rc, const char * filename, int where, const char * types, ...) ;
#ifndef ism_common_setErrorData
#define ism_common_setErrorData(rc, types...) setErrorDataFunction((rc), __FILE__, __LINE__, types)
#endif
#endif
#endif

/**
 * Format the last error in this thread.
 *
 * @param buffer The buffer which contain the formatted string
 * @param len    The length of the buffer.
 * @Return The length of the data to return.  This might be longer than the available length of the buffer.
 *         If there is no error, return 0.
 */
XAPI int ism_common_formatLastError(char * buffer, int len);

/**
 * Format the last error code in this thread by locale
 * @param locale the input locale
 * @param buffer the buffer which contain the formatted string
 * @param len buffer len
 * @Return The length of the data to return.  This might be longer than the available length of the buffer.
 *         If there is no error, return 0.
 */
XAPI int ism_common_formatLastErrorByLocale(const char * locale, char * buffer, int len);


/*********************************************************
 *                                                       *
 * SSL/TLS functions  	                                 *
 *                                                       *
 *********************************************************/
typedef struct x509_store_ctx_st X509StoreCtx;
typedef int (*ism_verifySSLCert)(int, X509StoreCtx *);
typedef int (*ism_pem_pw_t)(char * buf, int size, int rwflag, void * userdata);

/**
 * Create a TLS context for an endpoint
 *
 * Any of the string parameters can be NULL to indicate that the default should be used.
 */
XAPI struct ssl_ctx_st * ism_common_create_ssl_ctx(const char * objname, const char * methodName, const char * ciphers,
        const char * certFile, const char * keyFile, int isServer, int sslopt, int useClientCert, const char * profileName,
        ism_verifySSLCert verifyCallback, const char * serverName);

XAPI struct ssl_ctx_st * ism_common_create_ssl_ctxpw(const char * objname, const char * methodName, const char * ciphers,
        const char * certFile, const char * keyFile, int isServer, int sslopt, int useClientCert, const char * profileName,
        ism_verifySSLCert verifyCallback, const char * serverName, ism_pem_pw_t getkeypw, void * userdata);

/**
 * Destroy a security context
 */
XAPI void ism_common_destroy_ssl_ctx(struct ssl_ctx_st * ctx);

/**
 * Validate PEM certificate
 *
 * @certFileName File to validate
 */
XAPI int ism_common_validate_PEM(const char * certFileName);

/**
 * Update CRL for security context
 */
XAPI int ism_common_ssl_update_crl(struct ssl_ctx_st * ctx, const char * objname, const char * profileName);

/**
 * Create an SNI context and index it by name
 */
XAPI int ism_ssl_setSNIConfig(const char * name, const char * serverCert, const char * serverKey,
        const char * keyPassword, const char * trustCerts, int requireClientCert,
        ism_verifySSLCert verifyCallback);

/*
 * Remove an SNI context
 */
XAPI int ism_ssl_removeSNIConfig(const char * name);

/*
 * Add certs from memory to the TLS context
 * @param ctx  A TLS context
 * @param objname  The name of the object (use for logging)
 * @param trustCerts  A set of certs in PEM format as a string in memory
 */
int  ism_common_addTrustCerts(struct ssl_ctx_st * ctx, const char * objname, const char * trustCerts);

/**
 * Validate a PSK file.
 *
 * Validate that the PSK file is good.  The PSK file must be a valid csv file with two items on each line.
 * <p>
 * The first item is an identity which must be a valid UTF-8 string and not contain C0 control characters
 * (codepoints less than space).  It must be less than 256 bytes in length.
 * <p>
 * The second item is a hex string which must have an even number of hex digits,
 * and not more than 512 hex digits (the resulting binary key must be 0 to 256 bytes in length).  A zero length
 * key disables the use of a PSK cipher for this identity and will result in a disconnect if a PSK cipher is
 * selected.
 * <p>
 * If an error occurs, the line number in the file (starting with 1) is returned and validation fails.
 * If an invalid file is used in apply, any invalid lines will be ignored.
 *
 * @param name   The name of the PSK file
 * @param line   (output) the line number at which an error occurred or # of good lines.
 * @return An ISMRC return code
 */
XAPI int ism_ssl_validatePSKfile(const char * name, int32_t * line, int32_t *goodEntries);

/**
 * Apply PSK file.
 *
 * Set the identity to PSK mappings from a file.  Any invalid entries will be
 * ignored and thus any checking should be done before calling this method.
 *
 * @param name   The name of the PSK file, and # of psk entries if already validated
 * @return An ISMRC return code
 */
XAPI int ism_ssl_applyPSKfile(const char * name, int mapSize);

/**
 * Callback for PSK file modification.
 *
 * This is normally used to update the SSL Contexts connected to endpoints when
 * the PSK file is updated.
 *
 * @param  count  The number of PSK entries.  If this is zero, PSK processing is disabled.
 */
typedef void (* ism_psk_notify_t)(int count);

/*
 * Set a method to call when the PSK file is applied.
 *
 * Only one callback is allowed to be set, and this will replace any existing callback.
 * The purpose of this callback is so that we can notify all SSL contexts of the change.
 * The transport component knows about which SSL contexts are in use by endpoints, but
 * the utilities do not.  Normally it is only significant when the PSK file is empty
 * before or after the apply.
 *
 * @param psknotifier  The method to call when the PSK file is applied.
 */
XAPI void ism_common_setPSKnotify(ism_psk_notify_t psknotifer);

/**
 * Update an SSL context when the PSK file is changed.
 * @param ctx    A server SSL context
 * @param enable Zero if PSK handling should be disabled, non-zero if it should be enabled.
 */
XAPI int ism_ssl_update_psk(struct ssl_ctx_st * ctx, int enable);

/**
 * Compute SHA1
 * Low level API call to digest SHA1 forbidden in FIPS mode!
 * So we are going to use high level digest APIs.
 */
XAPI uint32_t ism_ssl_SHA1(const void *key, size_t keyLen, uint8_t * mdBuf);

struct ssl_st;

/**
 * Re-validates the client's certificate
 * @param ssl - SSL handle for the connection
 * @return - 0 if validation failed, 1 if succeeded
 */

XAPI int ism_ssl_revalidateCert(struct ssl_st * ssl);

/*
 * Get the set if Subject Alternate Names
 * The names are returned as a null separated list in a buffer
 * @param ssl   The SSL object
 * @param buf   The output buffer
 * @return The count of names returned
 */
XAPI int ism_ssl_getSubjectAltNames(struct ssl_st * ssl, concat_alloc_t * buf);

/**
 * Returns peer certificate info for a connection
 * Returned buffer should be freed by the caller using BUF_MEM_free(BUF_MEM * result)
 */
XAPI struct buf_mem_st * ism_ssl_getPeerCertInfo(struct ssl_st * ssl, int full, int isServer);

/**
 * Callback for openSSL info.
 * This should only be used as the ssl info callback within an SSL object which is set using SSL_set_info_callback().
 * The transport object must be set as the app data in the SSL object using SSL_set_app_data().
 * The parameters are filled in by openSSL.
 */
XAPI void ism_common_sslInfoCallback(const struct ssl_st * ssl, int where, int ret);

/**
 * Callback for the openSSL message debug.
 * This should only be used as the openSSL message debug in the SSL object which is set using SSL_set_msg_callback().
 * The transport object must be set as the arg data in the SSL object using SSL_set_msg_callback_arg().
 * The parameters are filled in by openSSL.
 */
XAPI void ism_common_sslProtocolDebugCallback(int direction, int version, int contentType, const void * buf, size_t len,
        struct ssl_st * ssl, void * arg);

/**
 * Get the cipher id for the current transport.
 * @param transport   The transport object
 * @return the cipher ID or -1 to indicate there is no cipher
 */
XAPI int ism_common_getCipher(ima_transport_info_t * transport);

/**
 * Get the cipher object for the current transport.
 * @param transport  The transport object
 * @return the cipher object or NULL to indicate there is no cipher object
 */
XAPI struct ssl_cipher_st * ism_common_getCipherObject(ima_transport_info_t * transport);

/**
 * Get the cipher name given its index.
 * The name is is the TLS specification syntax and not openSSL syntax.
 * @param id  The index of the cipher.
 * @return the naem of the cipher in spec format
 */
XAPI const char * ism_common_getCipherName(int id);

/**
 * Get the cipher id given its name.
 * The name is is the TLS specification syntax and not openSSL syntax.
 * @param name  The name of the cipher in spec format
 * @return the index of the cipher or -1 to indicate no cipher or -2 to indicate unknown cipher
 */
XAPI int ism_common_getCipherId(const char * name);

/**
 *  Initialize a map of pre-certified client certificates
 *  @param profileName
 */
XAPI ismHashMap * ism_ssl_initAllowedClientCerts(const char * profileName);
/**
 *  Clean up and destory a map of pre-certified client certificates
 *  @param allowedClientsMap
 */
XAPI void ism_ssl_cleanAllowedClientCerts(ismHashMap *allowedClientsMap);

/**
 * Call back to get the map of pre-certified client certificates from the transport object
 */
typedef ismHashMap * (*ism_getAllowedClientsMap)(ima_transport_info_t *transport);

/**
 * Check whether connecting client certificate is pre-certified
 * @param preverify_ok - indicates, whether the verification of the certificate in question was passed (preverify_ok=1) or not (preverify_ok=0) (provided by openSSL to the verify_callback)
 * @param storeCTX - a pointer to the complete context used for the certificate chain verification. (provided by openSSL to the verify_callback)
 * @param getAllowedClients Call back to get the map of pre-certified client certificates from the transport object
 */
XAPI int ism_ssl_checkPreverifiedClient(int preverify_ok, struct x509_store_ctx_st *storeCTX, ism_getAllowedClientsMap getAllowedClients);

/*********************************************************
 *                                                       *
 * Trace Module Functions                                *
 *                                                       *
 *********************************************************/


/**
 * Reset trace routines so instead of using a trace module we use the inbuilt trace
 */

XAPI void ism_common_TraceModuleClear(void);

/**
 * Loads the trace module in response to changes in dynamic configuration
 * of the trace module.
 *
 * @param props The modified properties
 */
XAPI void ism_common_TraceModuleDynamicUpdate(ism_prop_t * props);

/**
 * Inform the trace module of changes to the server configuration.
 *
 * This is called when any change is made to the Server section of the dynamic
 * configuration.
 * @param props The modified server configuration properties
 */
XAPI void ism_common_TraceModuleConfigUpdate(ism_prop_t * props);


/********************************************************
 *                                                      *
 * XA ID (XID) functions                                *
 *                                                      *
 ********************************************************/

#define ISM_FWD_XID 0x00667764

/**
 * Convert a VT_Xid field to a XID structure.
 *
 * @param field  A field which must be of type VT_Xid
 * @param xid    A output XID structure allocated by the caller
 * @return A return code, 0=good
 */
int  ism_common_toXid(ism_field_t * field, ism_xid_t * xid);

/**
 * Return the buffer size need to convert a XID to VT_Xid
 * @param xid  The input XID structure
 * @return The length in bytes of the allocation required for the XID of zero for error
 */
int  ism_common_xidFieldLen(ism_xid_t * xid);

/**
 * Convert a XID structure to VT_Xid field.
 *
 * It is acceptable to use a XID structure as the buffer, and you can use the same
 * XID structure as the input, but in that case the XID structure is no longer usable.
 * The buffer must have the same lifetime as the field.  If you are converting to put this
 * into a property, setting the property will copy the buffer.
 *
 * @param xid   The input XID structure
 * @param field The output field allocated by the caller
 * @param buf   The output buffer for the XID which must be at least as large as specified by calling
 *              ism_protocol_xidFieldLen() with this xid.
 */
int  ism_common_fromXid(ism_xid_t * xid, ism_field_t * field, char * buf);

/**
 * Convert string to XID.
 *
 * @param xidStr input string
 * @param xid   The XID structure
 * @return 0 if OK
 */
int ism_common_StringToXid(const char * xidStr, ism_xid_t * xid);

/**
 * Convert a XID to a printable string
 *
 * @param xid   The XID structure
 * @param buf   The output buffer
 * @param len   The length of the buffer (should be 278 byes for max size)
 * @return The buffer with the xid filled in
 */
char * ism_common_xidToString(const ism_xid_t * xid, char * buf, int len);

/*
 * Convert from a base256 string to a base64 string.
 * @param from  The input string
 * @param to    The output location.  This should be allocated at least (fromlen+2)/3*4+1 bytes
 * @oaran fromlen The length of the input string
 */
XAPI int ism_common_toBase64(char * from, char * to, int fromlen);

/*
 * Convert from base64 to base 256.
 *
 * @param from  The input string in base64 which must be a multiple of 4 bytes
 * @param to    The output string which must be large enough to hold the result.
 *              This is commonly allocated by using a length equal to the input length.
 * @return The size of the output, or a negative value to indicate an error
 *
 */
XAPI int ism_common_fromBase64(char * from, char * to, int fromlen);

/**
 * Convert a byte array to a hex string.
 *
 * The resulting string is null terminated.
 *
 * The invoker must give a large enough buffer for the output.  This can be
 * determined as fromlen*2+1.
 *
 * @param  from  The input byte array
 * @param  to    The output location
 * @param  fromlen  The length of the from byte array
 * @return  The length of the output not including the null terminator
 */
XAPI int ism_common_toHexString(const char * from, char * to, int fromlen);

/**
 * Convert a hex string to a binary array.
 *
 * The invoker must give a large enough buffer for the output.
 * The string must be made up of only hex digits which can be in any case.
 * There must be an even number of hex digits.
 *
 * @param from A null terminated string containing an even number of hex digits
 * @param to   An output location for the byte array. This can be null to only
 *             check the hex string and find its length.
 * @return The length of the output in bytes or -1 to indicate an error
 */
XAPI int ism_common_fromHexString(const char * from, char * to);

/**
 * Platform Types
 *
 */
typedef enum ism_platformType_t {
    PLATFORM_TYPE_9005       = 0,       /** Real appliance - 9005 systems */
    PLATFORM_TYPE_APPLIANCE  = 1,       /** Real appliance with nvDIMM    */
    PLATFORM_TYPE_CCIVM      = 2,       /** SoftLayer virtual server      */
    PLATFORM_TYPE_CCIBM      = 3,       /** SoftLayer Baremetal with      */
    PLATFORM_TYPE_VMWARE     = 4,       /** VMware                        */
    PLATFORM_TYPE_XEN        = 5,       /** XEN                           */
    PLATFORM_TYPE_VIRTUALBOX = 6,       /** Virtual Box                   */
    PLATFORM_TYPE_KVM        = 7,       /** KVM                           */
    PLATFORM_TYPE_AZURE      = 8,       /** Microsoft AZURE               */
    PLATFORM_TYPE_EC2        = 9,       /** Amazone EC2                   */
    PLATFORM_TYPE_DOCKER     = 10,      /** Docker container              */
    PLATFORM_TYPE_UNKNOWN    = 11,      /** Unknown                       */
} ism_platformType_t;

/**
 * Accepted License Type
 *
 */
typedef enum ism_licenseType_t {
    PLATFORM_LICENSE_UNKNOWN      = 0,       /** Unknown     */
    PLATFORM_LICENSE_DEVELOPMENT  = 1,       /** Development */
    PLATFORM_LICENSE_TEST         = 2,       /** Test        */
    PLATFORM_LICENSE_PRODUCTION   = 3,       /** Production  */
    PLATFORM_LICENSE_STANDBY      = 4,       /** Standby     */
} ism_licenseType_t;


/**
 * Initialize platform data file.
 * Platform data file contains platform specific data such as
 * isVM, platform serial number, type etc.
 * @return ISMRC_OK or ISMRC_*
 */
int ism_common_initPlatformDataFile(void);

/**
 * Updates cached accepted License type
 * based on information stored in accepted.json file set
 * during license acceptance process and returns the cached value
 */
ism_platformType_t ism_common_updatePlatformLicenseType(void);

/**
 * Returns platform type.
 * This information is used to invoke specific action
 * to customize or set various configuration options.
 * @return Platform type
 */
XAPI ism_platformType_t ism_common_getPlatformType(void);

/**
 * Returns 1 if platform is VM.
 * This information is used to invoke specific action
 * to customize or set various configuration options.
 * @return Platform type
 */
XAPI int ism_common_getPlatformIsVM(void);

/**
 * Returns accepted license type - Development, Test or Production
 * This information is used to invoke specific action
 * to customize or set various configuration options.
 * @return Platform type
 */
XAPI ism_licenseType_t ism_common_getPlatformLicenseType(void);

/**
 * Sets the accepted license type - Development, Test or Production
 * This information is used to invoke specific action
 * to customize or set various configuration options.
 * @param licensedUsage
 * @return 0 = good
 */
XAPI int ism_common_setPlatformLicenseType(const char *licensedUsage);

/*
 * Returns last 7 characters of platform serial number.
 * Serial number is used for UUID generation.
 */
XAPI const char * ism_common_getPlatformSerialNumber(void);

/*
 * Validate a server UID.
 * A valid serverUID is 1 to 16 bytes containing only alpahnumerics, ',', and '-'.
 * Our generated serverUID has 8 bytes of alphanumeric with an optional '.'
 * followed by an integer.
 * @param  serverUID  The server UID
 * @return 1=valid, 0=not valid
 */
XAPI int ism_common_validServerUID(const char * serverUID);

/**
 * Retrieve the IP Address of specified interface with an index
 *
 ** Note: Caller need to free the return value.
 *
 * @param ifmapname  	The interface name with index.
 * 						If there is index associate with the
 * 						interface name, only that one IP return.
 * 						Otherwise, all IPs will be returned.
 * @return 	IP or a list of IPs will be returned. NULL if there
 * 			is no IP associated with the interface or index.
 */
XAPI char * ism_common_ifmapip(const char * ifmapname);

/**
 * Retrieve the interface name with index for an IP.
 * The format of the return is interfacename_index.
 * For example: eth6_1
 *
 * Note: Caller need to free the return value.
 *
 * @param ip the IP address
 * @param admin_state the AdminState value (0 - disabled, 1 - enabled)
 * @return the interface name with the index associate with the ip.
 */
XAPI char * ism_common_ifmapname(const char * ip, int * admin_state);

/**
 * Check and increase the count which this message ID had been logged.
 * @param msgCode	the message id
 * @param clientID	the client ID
 * @param sourceIP	the source IP address
 * @param reason 	the reason
 * @return the last count which this message ID had been logged.
 */
int ism_common_conditionallyLogged(ism_common_log_context *context, const ISM_LOGLEV level, ism_logCategory_t category, int msgCode, ism_trclevel_t * trclvl, const char * clientID, const char * sourceIP, const char * reason);

/**
 * Read content of the file
 * @param fileName name of the file to read
 * @param pBuff pointer to the buffer with data to be returned. (If NULL only file length will be calculated)
 * @param pLen  pinter to file length to be returned
 * @return 0 if OK, error code otherwise
 */
XAPI int ism_common_readFile(const char * fileName, char ** pBuff, int *pLen);


/* Simple server */
typedef struct _simpleServer_t * ism_simpleServer_t;
typedef void (*ism_simpleServer_connect_cb)(void);
typedef void (*ism_simpleServer_disconnect_cb)(int rc);
typedef int (*ism_simpleServer_request_cb)(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int *rc);
XAPI ism_simpleServer_t ism_common_simpleServer_start(const char * serverAddress, ism_simpleServer_request_cb requestCB,
        ism_simpleServer_connect_cb connectCB, ism_simpleServer_disconnect_cb disconnectCB);
XAPI void ism_common_simpleServer_stop(ism_simpleServer_t simpleServer);
XAPI int ism_common_simpleServer_waitForConnection(ism_simpleServer_t simpleServer);

/*
 * Dynamically tune the server configuration based on available resources and empirical data from performance testing.
 * Any and all of the configuration parameters set here can be overridden by the user in the server static configuration file.
 */
XAPI void ism_config_autotune(void);

/* Create a hex string representation of a CPU affinity map */
XAPI void ism_common_affMaskToStr(char affMask[CPU_SETSIZE], int len, char * buff);

/*
 * Pass back the user provided (static configuration) thread affinity for thread named by thrName, or if not provided
 * pass back the computed thread affinity mask.
 *
 * thrName (input) - name of the thread to return a CPU affinity map for
 * affMap (input/output) - memory allocated by caller, return the CPU map filled in with CPUs
 *                         that thread named by affStr is to be assigned to
 * return - the index of the last CPU which is set in affMap
 */
XAPI int ism_config_autotune_getaffinity(const char *thrName, char affMap[CPU_SETSIZE]);

typedef struct ismUtilsArray_t * ismArray_t;
XAPI void ism_ArrayInit(void);

/*
 * Create Array object
 * Call ism_common_destroyArray to delete the HashMap object which return by
 * this function.
 * @param capacity - capacity for the list
 * @return   ismArray_t object
 */
XAPI ismArray_t ism_common_createArray(uint32_t capacity);
/*
 * Destroy Array object
 * @param array the Array object
 */
XAPI void ism_common_destroyArray(ismArray_t array);
XAPI void ism_common_destroyArrayAndFreeValues(ismArray_t array, ism_freeValueObject freeCB);
/*
 * Put an object into the list
 * @param array - Array to put object to
 * @param object
 * @return  A index of array entry where object was put to. 0=failure (array is full).
 */
XAPI uint32_t ism_common_addToArray(ismArray_t array, void * object);
/*
 * Put an object into the list. Take lock first
 * @param array - Array to put object to
 * @param object
 * @return  A index of array entry where object was put to. 0=failure (array is full).
 */
XAPI uint32_t ism_common_addToArrayLock(ismArray_t array, void * object);
/*
 * Get an element from the array
 * @param array
 * @param index
 * @return the element
 */
XAPI void * ism_common_getArrayElement(ismArray_t array, uint32_t index);
/*
* Get an element from the array. Take lock first.
* @parma array
* @param index
* @return the element
*/
XAPI void * ism_common_getArrayElementLock(ismArray_t array, uint32_t index);
/*
 * Remove an element from the array
 * @param array
 * @param index
 * @return the element
 */
XAPI void * ism_common_removeArrayElement(ismArray_t array, uint32_t index);
/*
* Get an element from the array. Take lock first.
 * @param array
* @param index
* @return the element
*/
XAPI void * ism_common_removeArrayElementLock(ismArray_t array, uint32_t index);
/*
 * Lock Array object
 * @param array - Array object
 */
XAPI void ism_common_ArrayLock(ismArray_t array);
/*
 * Unlock Array object
 * @param array - Array object
 */
XAPI void ism_common_ArrayUnlock(ismArray_t array);
/*
 * Get number of elements in array
 * @param array - Array object
 * @return number of elements in array
 */
XAPI int ism_common_getArrayNumElements(const ismArray_t array);

/*
 * Do the openSSL trust store hash for one directory
 *
 * @param dirpath The path to the directory
 * @param leave_links Leave existing links
 * @param verbose Bitmask how noisy to be: 0=none, 1=trace, 2=print
 * @return 0=good 1=unable to open directory
 *
 * Using leave_links=1 can leave stale links in the directory, but is not harmful otherwise.
 * This is commonly used when the trust store is updated at run time.
 */
int  ism_common_hashTrustDirectory(const char * dirpath, int leave_links, int verbose);


/*----  start stuff for thread health monitoring  -----*/
XAPI int  ism_common_stack_trace(int tmp);
XAPI void ism_common_going2work(void);
XAPI void ism_common_backHome(void);
XAPI int  ism_common_add_my_health(char *myName);
XAPI int  ism_common_del_my_health(void);
XAPI int  ism_common_check_health(void);
XAPI char *ism_common_show_mutex_owner(pthread_mutex_t *mutex, char *res, size_t len);
/*----  end   stuff for thread health monitoring  -----*/

/*
 * Dump First Failure Data Capture to trace
 *
 * @param function - calling function name
 * @param seqId - a sequence ID
 * @param core - take core dump (will shutdown)
 * @param filename - calling function's file name
 * @param lineNo - calling function's line number
 * @param label - a label
 * @param recode - the return code
 * @param ... data to print
 *
 * see ism_common_convertCoreDumpEnumToBool for a way to optionally
 * produce core dumps based on debug builds
 */
void ism_common_ffdc( const char *function
                    , uint32_t seqId
                    , bool core
                    , const char *filename
                    , uint32_t lineNo
                    , char *label
                    , uint32_t retcode
                    , ... );

/*
 * Get the number of non fatal FFDCs that have occurred
 * Used to check that the memory monitoring is healthy and
 * not leaking memory.
 *
 * Returned along with the memory statistics in the proxy and
 * the gateway memory statistics
 */
uint64_t ism_common_get_ffdc_count(void);

#define ISM_FFDC(_seqId, _core, _label, _retcode...) ism_common_ffdc(__func__, _seqId, _core, __FILE__, __LINE__, _label, _retcode)

/*********************************************************************/
/* FFDC PROBE values                                                 */
/*********************************************************************/
#define ismCommonFFDCPROBE_001 1
#define ismCommonFFDCPROBE_002 2
#define ismCommonFFDCPROBE_003 3
#define ismCommonFFDCPROBE_004 4
#define ismCommonFFDCPROBE_005 5
#define ismCommonFFDCPROBE_006 6
#define ismCommonFFDCPROBE_007 7
#define ismCommonFFDCPROBE_008 8
#define ismCommonFFDCPROBE_009 9
#define ismCommonFFDCPROBE_010 10
#define ismCommonFFDCPROBE_011 11
#define ismCommonFFDCPROBE_012 12
#define ismCommonFFDCPROBE_013 13
#define ismCommonFFDCPROBE_014 14
#define ismCommonFFDCPROBE_015 15
#define ismCommonFFDCPROBE_016 16
#define ismCommonFFDCPROBE_017 17
#define ismCommonFFDCPROBE_018 18
#define ismCommonFFDCPROBE_019 19
#define ismCommonFFDCPROBE_020 20
#define ismCommonFFDCPROBE_021 21
#define ismCommonFFDCPROBE_022 22
#define ismCommonFFDCPROBE_023 23
#define ismCommonFFDCPROBE_024 24
#define ismCommonFFDCPROBE_025 25
#define ismCommonFFDCPROBE_026 26
#define ismCommonFFDCPROBE_027 27
#define ismCommonFFDCPROBE_028 28
#define ismCommonFFDCPROBE_029 29
#define ismCommonFFDCPROBE_030 30
#define ismCommonFFDCPROBE_031 31
#define ismCommonFFDCPROBE_032 32
#define ismCommonFFDCPROBE_033 33
#define ismCommonFFDCPROBE_034 34
#define ismCommonFFDCPROBE_035 35
#define ismCommonFFDCPROBE_036 36
#define ismCommonFFDCPROBE_037 37
#define ismCommonFFDCPROBE_038 38
#define ismCommonFFDCPROBE_039 39
#define ismCommonFFDCPROBE_040 40
#define ismCommonFFDCPROBE_041 41
#define ismCommonFFDCPROBE_042 42
#define ismCommonFFDCPROBE_043 43
#define ismCommonFFDCPROBE_044 44
#define ismCommonFFDCPROBE_045 45
#define ismCommonFFDCPROBE_046 46
#define ismCommonFFDCPROBE_047 47
#define ismCommonFFDCPROBE_048 48
#define ismCommonFFDCPROBE_049 49


/*----- start stuff for processing structure identifiers -----*/
#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif
static inline bool ism_common_compareStructId(const char *structID, const char *expectedStructID)
{
    return (*((uint32_t *)structID) == *((uint32_t *)expectedStructID ))?true:false;
}

enum ffdc_coreDump {
    CORE_DUMP_SOMETIMES, // depending on NDEBUG
    CORE_DUMP_ALWAYS,
    CORE_DUMP_NEVER,
};

/*
 * Take a ffdc_coreDump option and map it to a bool based on ndebug
 *
 * @param coreDump - setting to map
 * @Return should take a core dump
 *
 */
static inline bool ism_common_convertCoreDumpEnumToBool( enum ffdc_coreDump coreDump)
{
    bool core = false;
    switch(coreDump)
    {
    case CORE_DUMP_SOMETIMES:
#ifndef NDEBUG
        core = true;
#endif
        break;
    case CORE_DUMP_ALWAYS:
        core = true;
        break;
    case CORE_DUMP_NEVER:
        core = false;
        break;
    }
    return core;
}

//Check the 4-byte struct ID is as expected
#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif

/*
 * Check if a common_malloc_eyecatcher has the correct struct ID
 *
 * @param structID - struct ID to check
 * @param expectedStructID - expected ID
 * @param coreDump - option to select if a core dump should be taken
 * @param func - calling function's name
 * @param seqId - a sequence ID
 * @param file - calling function's file name
 * @param line - calling function's line number
 *
 */
static inline bool ism_common_checkStructIdLocation(const char *structID, const char *expectedStructID, enum ffdc_coreDump coreDump, const char *func, uint32_t seqId, const char *file, int line)
{
    if(UNLIKELY(*((uint32_t *)structID)   != *((uint32_t *)expectedStructID )))
    {
        char ErrorString[256];
        snprintf(ErrorString, 256, "file: %s line: %u - Expected StructId %.*s got: %.*s\n",
                file, (unsigned int)line, 4, expectedStructID, 4, structID);
        bool core = ism_common_convertCoreDumpEnumToBool(coreDump);

        ism_common_ffdc(func, seqId, core, file, line, ErrorString, ISMRC_Error,
                             "Received StructId", structID, 4,
                             "Expected StructId", expectedStructID, 4,
                             NULL);
        return false;
    }
    return true;
}

#define ism_common_checkStructId(structID, expectedStructId,coreDump, seqId) ism_common_checkStructIdLocation(structID, expectedStructId, coreDump, __FUNCTION__, seqId, __FILE__, __LINE__)

/*
 * Check if a common_malloc_eyecatcher has the correct location
 *
 * @param structID - ID to check
 * @param expectedStructID - expected ID
 * @param extraDebug - some extra data to include in the trace
 * @param coreDump - option to select if a core dump should be taken
 * @param func - calling function's name
 * @param seqId - a sequence ID
 * @param file - calling function's file name
 * @param line - calling function's line number
 *
 */
static inline bool ism_common_checkIdLocation(uint32_t id, uint32_t expectedId, uint32_t extraDebug, enum ffdc_coreDump coreDump, const char *func, uint32_t seqId, const char *file, int line)
{
    if(UNLIKELY(id != expectedId))
    {
        char ErrorString[256];
        snprintf(ErrorString, 256, "file: %s line: %u - Expected id %d got: %d\n",
                file, (unsigned int)line, expectedId, id);
        bool core = ism_common_convertCoreDumpEnumToBool(coreDump);

        ism_common_ffdc(func, seqId, core, file, line, ErrorString, ISMRC_Error,
                             "Received Id:" , &id , sizeof(id),
                             "Expected Id:", &expectedId, sizeof(expectedId),
                             "Debug:", &extraDebug, sizeof(extraDebug),
                             NULL);
        return false;
    }
    return true;
}

#define ism_common_checkId(structID, expectedStructId, debug, coreDump, seqId) ism_common_checkIdLocation(structID, expectedStructId, debug, coreDump, __FUNCTION__, seqId, __FILE__, __LINE__)

// Set the 4-byte struct ID
#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif

/*
 * Set a struct ID
 *
 * @param structID - struct ID to update
 * @param newStructID - the new value
 *
 */
static inline void ism_common_setStructId(char *structID, const char *newStructID)
{
    *((uint32_t *)structID) = *((uint32_t *)newStructID);
}

/*
 * Invalidate a struct ID by placing ? at the front
 *
 * @param structID - struct ID to update
 *
 */
static inline void ism_common_invalidateStructId(char *structID)
{
    *structID = '?';
}


/* Default paths if they are not set using the path_parser.py/paths.properties 
   mechanism - define FALLBACK_DEFAULTPATHS if that is needed*/
#ifdef FALLBACK_DEFAULTPATHS
#ifndef IMA_SVR_INSTALL_PATH
#define IMA_SVR_INSTALL_PATH "/opt/ibm/imaserver"
#endif
#ifndef IMA_SVR_DATA_PATH
#define IMA_SVR_DATA_PATH "/var/messagesight"
#endif
#ifndef IMA_BRIDGE_INSTALL_PATH
#define IMA_BRIDGE_INSTALL_PATH "/opt/ibm/imabridge"
#endif
#ifndef IMA_BRIDGE_DATA_PATH
#define IMA_BRIDGE_DATA_PATH "/var/imabridge"
#endif
#ifndef IMA_SVR_COMPONENT_NAME_FULL
#define IMA_SVR_COMPONENT_NAME_FULL "Message Gateway Server"
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
