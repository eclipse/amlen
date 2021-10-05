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
#include <ismutil.h>
#endif

#ifndef NO_ICU
#include "unicode/udat.h"
#include "unicode/ustring.h"
#endif


#define NOTIMEZONE 0x7fffffff
#define BILLION  1000000000
#define TENMILLION 10000000
#define EOS 0x01

extern pthread_mutex_t     g_utillock;

/*
 * Time zone option.
 */
enum TimeZoneFormat {
    TZ_NONE            = 0,    /**< Do not format a time zone */
    TZ_ISO8601         = 1,    /**< Format in ISO8601 if the offset is timezone */
    TZ_FORCE_ISO8601   = 2,    /**< Format in ISO8601 even if the timezone is not defined */
    TZ_RFC822          = 3,    /**< Format in RFC822 if the timezone is defined */
    TZ_FORCE_RFC822    = 4,    /**< Format in RFC822 even if the timezone is not defined */
    TZ_ISO8601_2       = 5,    /**< Format in ISO8601 if the offset is timezone */
    TZ_FORCE_ISO8601_2 = 6     /**< Format in ISO8601 even if the timezone is not defined */
};

static int  parseDate(ism_ts_t * ts, const char * s);
static int  parseN(ism_ts_t * ts, int maxdigit, int minvalue, int maxvalue);
static char *  formatTime(ism_ts_t * ts, char * obuf, int olen, int level, int tz, int useT);
static void putN(ism_ts_t * ts, int val, int count, int level, char sep);
static ism_time_t getTimeNanos(ism_ts_t * ts);
static void setCurrentTime(ism_ts_t * ts, int loctime);
static void setTimeNanos(ism_ts_t * ts, ism_time_t nanos);

struct ism_ts_t {
    int    year;       /**< Year (4 digit) */
    int    month;      /**< Month in year (1 to 12) */
    int    day;        /**< Day in month (1 to 31) */
    int    hour;       /**< Hour in day (0 to 23) */
    int    minute;     /**< Minute in hour (0 to 59) */
    int    second;     /**< Second in minute (0 to 60) */
    int    millis;     /**< Milliseconds in second (0 to 999) */
    int    tzoff;      /**< Timezone offset in minutes (negative is west) */
    int    dow;        /**< Day of week */

    ism_time_t tstamp; /**< Timestamp in mampsecpmds since 1970-01-10T00:00Z */
    ism_time_t tshour; /**< Timestamp for the next hour */
    char   tsvalid;    /**< Timestamp valid 0=not, 1=tstamp, 2=tstamp and tshour */
    char   lenient;    /**< Allow lenient parse */
    char   tzset;      /**< Timezone has been set */
    char   level;      /**< Level of information supplied */
    char   fmtlevel;   /**< Level for this format */
    char   sep;        /**< Separator during parse */
    short  pos;        /**< Position during parse */
    short  posx;       /**< Position during format */
    short  digits;     /**< Digits in parse */
    char   buf[68];    /**< Temporary buffer */
    int    nanos;      /**< Nanoseconds */
    int64_t tstampn;   /**< Nanosecond timestamp */
};

/*
 * Convert from an ISM time to a Java timestamp
 * @param ts   An ISM timestamp
 * @return A Java timestamp (milliseconds since 1970-01-01T00:00).
 */
int64_t ism_common_convertTimeToJTime(ism_time_t ts) {
    return (int64_t)ts/1000000;
}


/*
 * Convert from a Java timestamp to an ISM timestamp
 * @param jts   A Java timestamp (milliseconds since 1970-01-01T00:00).
 * @return An ISM timestamp
 */
ism_time_t ism_common_convertJTimeToTime(int64_t jts) {
    return (ism_time_t)(jts*1000000);
}


/*
 * Reset all value to the default
 */
static void resetValues(ism_ts_t * ts) {
    ts->year    = 1970;
    ts->month   = 1;
    ts->day     = 1;
    ts->hour    = 0;
    ts->minute  = 0;
    ts->second  = 0;
    ts->tzoff   = 0;
    ts->tzset   = 0;
    ts->pos     = 0;
    ts->dow     = 7;                                             /* Not set*/
    ts->nanos   = 0;
    ts->tsvalid = 0;
}


#ifndef _WIN32

/* *********************
 *
 * Unix specific
 *
 ********************* */

#include <sys/time.h>

/*
 * Get the current time in nanoseconds.
 * This is the fastest clock source in 64 bit Linux as it is kept in mapped memory by the kernel.
 * It is only slightly slower than directly reading the TSC.
 *
 * On modern processors and 3.x kernels the resolution is near nanosecond.
 */
uint64_t ism_common_currentTimeNanos(void) {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    return (int64_t)tv.tv_sec * BILLION + tv.tv_nsec;
}

/* TSC timestamp storage type */
typedef union
{
	double   d;
	uint32_t i[2];		/* i[0] is the low TSC register, i[1] is the high TSC register */
	uint64_t l;
} ism_tsc_t;

#ifdef __x86_64__
/* Read TSC instruction on x86 */
#define rdtsc(low,high)  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
#elif __PPC64__
#define rdtsc(low,high) {uint64_t ppc64_timebase =  __builtin_ppc_get_timebase(); low = ppc64_timebase & 0xFFFFFF; high = ppc64_timebase >>32;}
#endif
static double tscScale = 0.0;
static ism_tsc_t startTSC = {0};
static double startTime = 0.0;
static int useTSC = 0;
extern int g_MHz;

/*
 * Initialize the TSC
 *
 * There is no way to get the TSC rate other than to compare it to the monotonic clock.
 * Tests show that we resolve to the correct 5 digits of resolution in 100 milliseconds.
 */
static void initTSC_int(int wait) {
    struct timespec t1, t2;
    ism_tsc_t       tsc1, tsc2;
    double freq = 0.0;

    /*
     * Compute the rate
     */
    clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    rdtsc(tsc1.i[0],tsc1.i[1]);
    usleep(100000);         /* two tenths of a second */
    clock_gettime(CLOCK_MONOTONIC_RAW, &t2);
    rdtsc(tsc2.i[0],tsc2.i[1]);
    freq = ((tsc2.l-tsc1.l)*1000000000.0)/((t2.tv_sec-t1.tv_sec)*1000000000.0+(t2.tv_nsec-t1.tv_nsec));

    /*
     * Set the start time
     */
    startTSC = tsc1;
    startTime = t1.tv_sec + t1.tv_nsec *1e-9;
    tscScale = 1.0/freq;
    g_MHz = (int) (freq*1e-6);    /* Show the processor rate */
    useTSC = 1;
}

/*
 * Initialize the TSC.
 */
void ism_common_initTSC(void) {
    initTSC_int(100000);   /* 100 milliseconds */
}

/*
 * This function reads "The System Clock" (TSC) using the rdtsc assembly instruction.
 * This is the highest resolution clock on Intel systems and can be referenced at a
 * rate of 1e8 times/second on Westmere systems.
 *
 * Returns time since an arbitrary time (normally when the system was booted) in units of seconds.
 * The maximum precision that can realistically be claimed with the TSC is +/- 10 nanoseconds.
 *
 * The TSC does not work correctly on all systems but it does on our target systems.
 * On older systems the TSC is not synchronized across CPU sockets.
 *
 * On systems where Linux does not use the tsc as a clocksource, we use the MONOTONIC_RAW
 * clock.
 */
HOT double ism_common_readTSC(void) {
    if (useTSC) {
        /* There is a tsc clocksource */
        ism_tsc_t t;
	    rdtsc(t.i[0],t.i[1]);
	    return (tscScale*(t.l-startTSC.l) + startTime);
    } else {
        /* No tsc clocksource.  Use the Linux clock support */
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
        return (double)tp.tv_sec + ((double)tp.tv_nsec * 1e-9);
    }
}

/*
 * This function takes a tsc value as return from isn_common_TSC() and
 * converts to a very approx ism_time_t (number of nanos since unix epoch)
 *
 * Note that the TSC *must* be from this execution of the program and not
 * persisted somewhere.
 */
ism_time_t ism_common_convertTSCToApproxTime(double normalisedTSC)
{
    double tscNow = ism_common_readTSC();

    struct timespec tp_now;
    clock_gettime(CLOCK_REALTIME, &tp_now);

    double timeDelta = tscNow - normalisedTSC;

    double inputTimeSecs = (double)tp_now.tv_sec + ((double)tp_now.tv_nsec * 1e-9) - timeDelta;

    return (uint64_t)(inputTimeSecs * BILLION);
}

/*
 * Set the current time
 */
static void setCurrentTime(ism_ts_t * ts, int loctime) {
    struct timeval tv;
    struct tm  ltime;
    gettimeofday(&tv, NULL);

    if (loctime) {
        localtime_r(&tv.tv_sec, &ltime);
#if defined(AIX) || defined(SOLARIS) || defined(__hpux__)|| defined(HPUX)
        ts->tzoff = -(timezone/60);
        if (ltime.tm_isdst == 1)
            ts->tzoff += 60;
#else
        ts->tzoff = ltime.tm_gmtoff/60;    /* Linux */
#endif
        ts->tzset = 1;
    } else {
        gmtime_r(&tv.tv_sec, &ltime);
        ts->tzoff = 0;
        ts->tzset = 1;
    }
    ts->year   = ltime.tm_year+1900;
    ts->month  = ltime.tm_mon+1;
    ts->day    = ltime.tm_mday;
    ts->hour   = ltime.tm_hour;
    ts->minute = ltime.tm_min;
    ts->second = ltime.tm_sec;
    ts->nanos  = tv.tv_usec * 1000;
    ts->dow    = ltime.tm_wday;
}

/*
 * Set the timezone offset
 */
void ism_common_setTimezoneOffset(ism_ts_t * ts, int offset) {
    ts->tzoff = offset;
    ts->tzset = 1;
    ts->tsvalid = 0;
}

/*
 * Set the time in an ISO8601 timestamp based on a specified (unix)
 */
static void setTimeNanos(ism_ts_t * ts, uint64_t nanos) {
    time_t     tim;
    struct tm  ltime;
    uint64_t   hour;

    if (ts->tsvalid == 2) {
        if (nanos >= ts->tstamp && nanos < ts->tshour) {
            int64_t seconds;
            ts->tstamp = nanos;
            ts->nanos  = nanos % BILLION;
            seconds = nanos / BILLION;
            ts->second = seconds % 60;
            ts->minute = (seconds % 3600) / 60;
            return;
        }
    }

    ts->tstamp = nanos;
    tim = nanos/BILLION + (ts->tzoff * 60);
    hour = nanos/3600000000000ULL;
    gmtime_r(&tim, &ltime);
    ts->year   = ltime.tm_year+1900;
    ts->month  = ltime.tm_mon+1;
    ts->day    = ltime.tm_mday;
    ts->hour   = ltime.tm_hour;
    ts->minute = ltime.tm_min;
    ts->second = ltime.tm_sec;
    ts->nanos  = nanos % BILLION;
    ts->dow    = ltime.tm_wday;
    ts->tshour = (hour+1)*3600000000000ULL;
    ts->tsvalid = 2;
}


/*
 * Get time in nanos (Unix)
 */
static ism_time_t getTimeNanos(ism_ts_t * ts) {
    time_t tim;
    struct tm ltime;
    if (ts->tsvalid)
        return ts->tstamp;
    memset(&ltime, 0, sizeof(ltime));
    ltime.tm_year = ts->year-1900;
    ltime.tm_mon  = ts->month-1;
    ltime.tm_mday = ts->day;
    ltime.tm_hour = ts->hour;
    ltime.tm_min  = ts->minute;
    ltime.tm_sec  = ts->second;
    tim = mktime(&ltime) - timezone;           /* Correct for Unix timezone*/
    ts->tstamp = (((int64_t)tim) - (ts->tzoff * 60))*BILLION + ts->nanos;
    ts->tsvalid = 1;
    return ts->tstamp;
}


#else

/* *********************
 *
 * Windows specific
 *
 ********************* */

#define FT2INT64(ft) (*(int64_t *)(&(ft)))

static double initTimeBase(void);
static double TimeBase;
static int64_t TimeBase64;
static int    TimeBaseSet = 0;
const static double timemul = 0.0001;
const static double nanomul = 100.0;

/*
 * Static initialization of the time base (1970-01-01T00:00)
 */
static double initTimeBase(void) {
    SYSTEMTIME systime;
    FILETIME   filetime;
    memset(&systime, 0, sizeof(SYSTEMTIME));
    systime.wYear  = 1970;
    systime.wMonth = 1;
    systime.wDay   = 1;
    SystemTimeToFileTime(&systime, &filetime);
    TimeBase64 = FT2INT64(filetime);
    return (double) TimeBase64;
}

/*
 * Returns the current time in nanoseconds into the Unix era.
 */
ism_time_t  ism_common_currentTimeNanos(void) {
    FILETIME   filetime;

    if (!TimeBaseSet) {
        TimeBase = initTimeBase();
        TimeBaseSet = 1;
    }
    GetSystemTimeAsFileTime(&filetime);
    return (int64_t)(FT2INT64(filetime) - TimeBase64) * 100;
}


/*
 * Set the time in nanoseconds
 */
void setTimeNanos(ism_ts_t * ts, uint64_t nanos) {
    FILETIME   filetime;
    SYSTEMTIME systime;
    int64_t tm;
    uint64_t   deltans;

    if (!TimeBaseSet) {
        TimeBase = initTimeBase();
        TimeBaseSet = 1;
    }

    if (ts->tsvalid == 2) {
        if (nanos >= ts->tstamp && nanos < ts->tshour) {
            int64_t seconds;
            ts->tstamp = nanos;
            ts->nanos  = nanos % BILLION;
            seconds = nanos / BILLION;
            ts->second = seconds % 60;
            ts->minute = (seconds % 3600) / 60;
            return;
        }
    }
    ts->tstamp = nanos;
    nanos += (ts->tzoff * 60 * BILLION);
    tm = nanos / 100 + TimeBase64;
    *(int64_t *)(&filetime) = tm;
    FileTimeToSystemTime(&filetime, &systime);
    ts->year   = systime.wYear;
    ts->month  = systime.wMonth;
    ts->day    = systime.wDay;
    ts->hour   = systime.wHour;
    ts->minute = systime.wMinute;
    ts->second = systime.wSecond;
    ts->nanos  = nanos%BILLION;
    ts->dow    = systime.wDayOfWeek;
    deltans = (((ts->minute * 60) + ts->second) * BILLION) + ts->nanos;
    ts->tshour = ts->tstamp + (3600LL*BILLION - deltans);
    ts->tsvalid = 2;
}


/*
 * Get the time in milliseconds
 */
ism_time_t getTimeNanos(ism_ts_t * ts) {
    FILETIME   filetime;
    SYSTEMTIME systime;
    uint64_t    nanos;

    if (ts->tsvalid)
        return ts->tstamp;

    if (!TimeBaseSet) {
        TimeBase = initTimeBase();
        TimeBaseSet = 1;
    }
    systime.wYear         = ts->year;
    systime.wMonth        = ts->month;
    systime.wDay          = ts->day;
    systime.wHour         = ts->hour;
    systime.wMinute       = ts->minute;
    systime.wSecond       = ts->second;
    systime.wMilliseconds = ts->nanos / 1000000;
    SystemTimeToFileTime(&systime, &filetime);
    nanos = (int64_t)(FT2INT64(filetime) - TimeBase64) * 100;
    nanos -= (ts->tzoff * 60 * BILLION);  /* minutes to milliseconds */
    ts->tstamp = nanos;
    ts->tsvalid = 1;
    return nanos;
}


void setCurrentTime(ism_ts_t * ts, int loctime) {
    SYSTEMTIME systime;
    TIME_ZONE_INFORMATION tzi;

    if (loctime) {
        int dls = GetTimeZoneInformation(&tzi);
        ts->tzoff = -tzi.Bias;
        if (dls == TIME_ZONE_ID_DAYLIGHT)
            ts->tzoff -= tzi.DaylightBias;
        ts->tzset = 1;
        GetLocalTime(&systime);
    } else {
        GetSystemTime(&systime);
        ts->tzoff = 0;
        ts->tzset = 1;
    }

    ts->year   = systime.wYear;
    ts->month  = systime.wMonth;
    ts->day    = systime.wDay;
    ts->hour   = systime.wHour;
    ts->minute = systime.wMinute;
    ts->second = systime.wSecond;
    ts->nanos  = systime.wMilliseconds * 1000000;
    ts->dow    = systime.wDayOfWeek;
}

#endif


/*
 * Open a timestamp object.
 * As there is some cost associated with creating the timezone object, it is best to
 * create one and change the time when required.
 * @param  tzflag  The timezone flags (see ISM_TZF_*)
 * @return The timestamp object.
 */
ism_ts_t * ism_common_openTimestamp(int tzflag) {
    ism_ts_t * iso;

    iso = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,50),1, sizeof(ism_ts_t));
    resetValues(iso);
    switch (tzflag) {
    case ISM_TZF_UNDEF:
        break;
    case ISM_TZF_UTC:
        setCurrentTime(iso, 0);
        iso->level = 7;
        break;
    case ISM_TZF_LOCAL:
        setCurrentTime(iso, 1);
        iso->level = 7;
        break;
    default:
        ism_common_free(ism_memory_utils_misc,iso);
        iso = NULL;
        break;
    }
    return iso;
}


/*
 * Close the timestamp object and free resources.
 * @param The timestamp object.
 */
void ism_common_closeTimestamp(ism_ts_t * ts) {
    if (ts)
        ism_common_free(ism_memory_utils_misc,ts);
}



/*
 * Set the timestamp object from an ISM timestamp.
 * @param ts   The timestamp object.
 * @param time The ISM timestamp.
 */
void ism_common_setTimestamp(ism_ts_t * ts, ism_time_t time) {
    setTimeNanos(ts, time);
    ts->level = 7;
}


/*
 * Set the timestamp object from an ISO8601 string
 * @param ts       The timestamp object.
 * @param datetime The array of time values.
 * @return A return code.  If this is not zero, use ism_getLastError() to determine the cause.
 */
int ism_common_setTimestampString(ism_ts_t * ts, const char * datetime) {
    int  len;
    int  save_tzset = ts->tzset;
    int  save_tzoff = ts->tzoff;
    ism_timeval_t  vals;

    ts->lenient = 1;
    resetValues(ts);
    ism_common_getTimestampValues(ts, &vals);

    len = (int)strlen(datetime);
    if (len > 65)
        return 0;
    strcpy(ts->buf, datetime);
    ts->buf[len] = EOS;
    ts->buf[len+1] = 0;
    ts->level = parseDate(ts, datetime);
    if (ts->level > 0)
        ts->pos = 0;
    if (!ts->tzset) {
        ts->tzset = save_tzset;
        ts->tzoff = save_tzoff;
    }
    if (ts->level <= 0) {
        return -1;
    }
    if (ts->level > 8) {
        vals.hour     = ts->hour;
        vals.minute   = ts->minute;
        vals.second   = ts->second;
        vals.nanos    = ts->nanos;
        vals.tzoffset = ts->tzoff;
        ism_common_setTimestampValues(ts, &vals, 0);
    } else {
        ts->tsvalid = 0;
    }
    return 0;
}

static void setDayOfWeek(ism_ts_t * ts){
    struct tm  ltime;
    time_t tim = ism_common_convertTimeToJTime(getTimeNanos(ts));
	tim /= 1000000;
	gmtime_r(&tim, &ltime);
	ts->dow = ltime.tm_wday;
}

/*
 * Set the timestamp object from a set of time values.
 * @param ts    The timestamp object.
 * @param vals  The array of time values.
 * @return A return code.  If this is not zero, use ism_getLastError() to determine the cause.
 */
int ism_common_setTimestampValues(ism_ts_t * ts, ism_timeval_t * vals, int settz) {
    if (settz) {
        if (vals->tzoffset == NOTIMEZONE) {
            ts->tzoff = 0;
            ts->tzset = 0;
        } else if (ts->tzset == 0) {
            ts->tzset = 1;
            ts->tzoff = vals->tzoffset;
        }
    }
    ts->tsvalid = 0;
    ts->year   = vals->year;
    ts->month  = vals->month;
    ts->day    = vals->day;
    ts->hour   = vals->hour;
    ts->minute = vals->minute;
    ts->second = vals->second;
    ts->nanos  = vals->nanos;
    if ((vals->dayofweek >= 0) && (vals->dayofweek < 7))
    	ts->dow    = vals->dayofweek;
    else
    	setDayOfWeek(ts);
    return 0;
}


/*
 * Get the time from a timestamp object.
 * @param ts    The timestamp object.
 * @return The time from the timestamp object as an ISM timestamp
 */
ism_time_t ism_common_getTimestamp(ism_ts_t * ts) {
    return getTimeNanos(ts);
}

/*
 * Use an epoch of 2000-01-01T0 for expiration
 */
static time_t expire_delta = (30*365+7) * (24*60*60);   /* Delta seconds to 2000-01-01T00 */

/*
 * Get the current time to compare for expiration.  Round up.
 */
uint32_t ism_common_nowExpire(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_usec > 0)
        tv.tv_sec++;
    return (uint32_t)(tv.tv_sec - expire_delta);
}


/*
 * Convert a time to an expiration time
 */
uint32_t ism_common_getExpire(ism_time_t ntime) {
    if (ntime == 0)
        return 0;
    int64_t dtime = (ntime / BILLION) - (int64_t)expire_delta;
    if (dtime & ~0xffffffffL)
        dtime = 0xffffffff;
    return (uint32_t)dtime;
}

/*
 * Convert a time to an expiration time.
 */
uint32_t ism_common_javaTimeToExpirationTime(ism_time_t jtime){
  if (jtime == 0)
      return 0;
  int64_t dtime = (jtime / 1000) - (int64_t)expire_delta;
  dtime &= 0xffffffffL;
  return (uint32_t)dtime;
}


/*
 * Convert an expiration time to a time
 */
ism_time_t ism_common_convertExpireToTime(uint32_t dtime)
{
    if (dtime == 0)
        return 0;
    ism_time_t ntime = ((ism_time_t)dtime + expire_delta) * 1000000000;
    return ntime;
}


/*
 * Get the time values from a timestamp object.
 * @param ts    The timestamp object.
 * @param vals  The array of time values.
 * @return The address of the time value array.
 */
ism_timeval_t * ism_common_getTimestampValues(ism_ts_t * ts, ism_timeval_t * values) {
    values->year      = ts->year;
    values->month     = ts->month;
    values->day       = ts->day;
    values->hour      = ts->hour;
    values->minute    = ts->minute;
    values->second    = ts->second;
    values->tzoffset  = ts->tzoff;
    values->nanos     = ts->nanos;
    if ((ts->dow < 0) || (ts->dow > 6))
    	setDayOfWeek(ts);
    values->dayofweek = ts->dow;
    return values;
}


/*
 * Used to adjust timezone if required.
 * Not for common use.
 */
void ism_common_checkTimezone(ism_ts_t * ts) {
    ism_ts_t * newts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    if (newts->tzoff != ts->tzoff) {
        pthread_mutex_lock(&g_utillock);
        ts->tzoff = newts->tzoff;
        ts->tsvalid = 0;
        pthread_mutex_unlock(&g_utillock);
    }
    ism_common_closeTimestamp(newts);
}

/*
 * Format the time from a timestamp object.
 * @param ts    The timestamp object.
 * @param buf   The buffer to return the time string.
 * @param len   The length of the output buffer
 * @param level The level of
 * @param flags Option flags
 * @return The address of the time buffer.
 */
char * ism_common_formatTimestamp(ism_ts_t * ts, char * buf, int len, int level, int flags) {
    int  nanoflag = 0;
    int  tzflag   = 0;

    /*
     * Convert flags to nanosecond level
     */
    if (flags & ISM_TFF_NANOS)
        nanoflag = 2;
    else if (flags & ISM_TFF_MICROS)
        nanoflag = 1;
    if (nanoflag && (flags & ISM_TFF_NANOSEP))
        nanoflag += 2;

    /*
     * Convert flags to timezone flags
     */
    if (flags & ISM_TFF_ISO8601) {
        tzflag = TZ_ISO8601;
        if ((flags & ISM_TFF_ISO8601_2) == ISM_TFF_ISO8601_2) {
            tzflag = TZ_ISO8601_2;
        }
    } else if (flags & ISM_TFF_RFC822) {
        tzflag = TZ_RFC822;
    }
    if (tzflag && (flags & ISM_TFF_FORCETZ))
        tzflag += 1;

    formatTime(ts, buf, len, nanoflag*100+level, tzflag, !(flags&ISM_TFF_SPACE));
    return buf;
}

/*
 * Private parse the time and date
 */
static int parseDate(ism_ts_t * ts, const char * s) {
    char savesep;
    int tz;
    int level = 0;
    ts->pos = 0;
    if (ts->lenient && (s[1]==':' || s[2]==':')) {
        ts->pos = 0;
        level = 4;
    } else if (s[0]=='T') {
        ts->pos = 1;
        level = 4;
    } else {
        ts->year = parseN(ts, 4, 0, 9999);
        if (ts->sep != '-') {
            return (ts->sep == EOS) ? 1 : 0;
        }
        ts->month = parseN(ts, 2, 1, 12);
        if (ts->sep != '-') {
            return (ts->sep == EOS) ? 2 : 0;
        }
        ts->day = parseN(ts, 2, 1, 31);
        if ((ts->sep != 'T') && (ts->sep != ' ' || !ts->lenient)) {
            return (ts->sep == EOS) ? 3 : 0;
        }
    }
    ts->hour = parseN(ts, 2, 0, 23);
    if (ts->sep != ':') {
        return (ts->sep == EOS) ? level+4 : 0;
    }
    ts->minute = parseN(ts, 2, 0, 59);
    if (ts->sep !=':' && ts->sep != '+' && ts->sep != '-' && ts->sep != 'Z') {
        return (ts->sep == EOS) ? level+5 : 0;
    }
    level += 5;
    if (ts->sep == ':') {
        ts->second = parseN(ts, 2, 0, 60);
        if (ts->sep !='.' && ts->sep != '+' && ts->sep != '-' && ts->sep != 'Z') {
            return (ts->sep == EOS) ? level+1 : 0;
        }
        level++;
        if (ts->sep == '.') {
            /* Get milliseconds */
            ts->nanos = parseN(ts, 0, 0, 999999999);
            if (ts->digits > 9)
                ts->digits = 9;
            switch (ts->digits) {
            case 1: ts->nanos *= 100000000; break;
            case 2: ts->nanos *= 10000000;  break;
            case 3: ts->nanos *= 1000000;   break;
            case 4: ts->nanos *= 100000;    break;
            case 5: ts->nanos *= 10000;     break;
            case 6: ts->nanos *= 1000;      break;
            case 7: ts->nanos *= 100;       break;
            case 8: ts->nanos *= 10;        break;
            }
            if (ts->sep != '+' && ts->sep != '-' && ts->sep != 'Z') {
                return (ts->sep == EOS) ? level+1 : 0;
            }
            level++;
        }
    }
    if (ts->sep == 'Z') {
        ts->tzoff = 0;
        ts->tzset = 1;
        parseN(ts, 0, 0, 0);
        return (ts->sep == EOS) ? level : 0;
    }
    savesep = ts->sep;
    tz = parseN(ts, 4, 0, 1400);
    if (ts->sep == EOS) {
        if (ts->digits > 2) {         /* Allow the Java ISO 831 date form */
            if (tz%100 > 59)
                return 0;
            ts->tzoff = (tz%100) + (tz/100)*60;
            ts->tzset = 1;
        } else {
            if (tz > 14)
                return 0;
            ts->tzoff = tz*60;
            ts->tzset = 1;
        }
        if (savesep == '-')
            ts->tzoff = -(ts->tzoff);
        return  level;
    }
    if (ts->sep == ':') {
        if (ts->digits > 2 || tz>14)
            return 0;
        ts->tzoff = (tz * 60) + parseN(ts, 2, 0, 59);
        ts->tzset = 1;
        if (savesep == '-')
            ts->tzoff = -(ts->tzoff);
    } else {
        return 0;
    }
    parseN(ts, 0, 0, 0);
    return (ts->sep == EOS) ? level : 0;
}


/*
 * private parse of an integer value
 */
static int parseN(ism_ts_t * ts, int maxdigit, int minvalue, int maxvalue) {
    int value = 0;
    ts->digits = 0;
    for (;;) {
        char ch = ts->buf[ts->pos++];
        if (ch >= '0' && ch <='9') {
            ts->digits++;
            value = (value*10) + (ch-'0');
        } else {
            if (!maxdigit && ch==',') {
            } else {
                ts->sep = ch;
                if (ch == EOS)
                    ts->pos--;
                break;
            }
        }
    }
    if (value < minvalue || value > maxvalue ||
       (maxdigit!=0 && (ts->digits>maxdigit || (!ts->lenient && ts->digits<maxdigit)))) {
        ts->sep = '\x02';
        value = 0;
    }
    return value;
}

/*
 * Format an ism_ts_t date structure specifying the level and options
 */
static char *  formatTime(ism_ts_t * ts, char * obuf, int olen, int level, int tz, int useT) {
    int tzoff = ts->tzoff;
    int nanos = level/100;
    if (nanos)
        level = level%100;
    ts->posx = 0;
    if (level==0)
        level = ts->level;
    ts->fmtlevel = level;
    if (level <= 7) {
        putN(ts, ts->year,  4, 1, '-');
        putN(ts, ts->month, 2, 2, '-');
        putN(ts, ts->day,   2, 3, useT ? 'T' : ' ');
    } else {
        level -=4;
        if (useT)
            ts->buf[ts->posx++] = 'T';
    }
    ts->fmtlevel = level;
    putN(ts, ts->hour,   2, 4, ':');
    putN(ts, ts->minute, 2, 5, ':');
    putN(ts, ts->second, 2, 6, '.');
    switch (nanos) {
    case 0:      /* Milliseconds */
        putN(ts, ts->nanos/1000000, 3, 7, '~');
        break;
    case 1:      /* Microseconds, no separator */
        putN(ts, ts->nanos/1000000, 3, 7, '~');
        putN(ts, (ts->nanos/1000)%1000, 3, 7, '~');
        break;
    case 2:      /* Nanoseconds, no separator */
        putN(ts, ts->nanos/1000000, 3, 7, '~');
        putN(ts, (ts->nanos/1000)%1000, 3, 7, '~');
        putN(ts, ts->nanos%1000, 3, 7, '~');
        break;
    case 3:      /* Microseconds, comma separator */
        putN(ts, ts->nanos/1000000, 3, 7, ',');
        putN(ts, (ts->nanos/1000)%1000, 3, 7, '~');
        break;
    case 4:      /* Nanoseconds, comma separators */
        putN(ts, ts->nanos/1000000, 3, 7, ',');
        putN(ts, (ts->nanos/1000)%1000, 3, 7, ',');
        putN(ts, ts->nanos%1000, 3, 7, '~');
        break;
    }

    /* Use ISO-8601 timezone format */
    if ((tz == TZ_ISO8601 && ts->tzset) || tz==TZ_FORCE_ISO8601 ||
        (tz == TZ_ISO8601_2 && ts->tzset) || tz==TZ_FORCE_ISO8601_2) {
        if (tzoff == 0) {
            ts->buf[ts->posx++] = 'Z';
        } else {
            if (tzoff < 0) {
                ts->buf[ts->posx++] = '-';
                tzoff = -tzoff;
            } else {
                ts->buf[ts->posx++] = '+';
            }
            if (tz == TZ_ISO8601 || tz == TZ_FORCE_ISO8601 || tzoff%60) {
                putN(ts, tzoff/60, 2, 0, ':');
                putN(ts, tzoff%60, 2, 0, '~');
            } else {
                putN(ts, tzoff/60, 2, 0, '~');
            }
        }
    }

    /* Use old RFC 822 timezone format */
    if ((tz == TZ_RFC822 && ts->tzset) || tz==TZ_FORCE_RFC822) {
        if (tzoff < 0) {
            ts->buf[ts->posx++] = '-';
            tzoff = -tzoff;
        } else {
            ts->buf[ts->posx++] = '+';
        }
        putN(ts, tzoff/60 * 100 + tzoff%60, 4, 0, '~');
    }
    ts->buf[ts->posx] = 0;
    strncpy(obuf, ts->buf, olen);
    if (olen > 0)
        obuf[olen-1] = 0;
    return ts->buf;
}


/*
 * Private function to put in integer
 */
static void putN(ism_ts_t * ts, int val, int count, int level, char sep) {
    if (level > ts->fmtlevel)
        return;
    if (count>3)
        ts->buf[ts->posx++] = (char)( (val/1000)%10 + '0' );
    if (count>2)
        ts->buf[ts->posx++] = (char)( (val/100)%10 + '0' );
    if (count>1)
        ts->buf[ts->posx++] = (char)( (val/10)%10 + '0' );
    ts->buf[ts->posx++] = (char) (val%10 + '0');
    if (level != ts->fmtlevel && sep != '~') {
        ts->buf[ts->posx++] = sep;
    }
}

#define UUID_Epoch 12219292800L  /* Delta in seconds from 1582-10-15 to 1970-01-01 */

/*
 * Return a UUID epoch timestamp
 * This is in hundreds of nanoseconds since 1582-10-15T00
 */
uint64_t ism_common_getUUIDtime(void) {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    return ((uint64_t)tv.tv_sec+UUID_Epoch) * TENMILLION + (tv.tv_nsec/100);
}

/*
 * Convert a UUID time to a nanosecond timestamp
 * This code does not handle timestamps outside of the encoding range for nanosecond timestamps
 * @param uuid_time A UUID timestamp
 * @return a nanosecond timestamp
 */
ism_time_t ism_common_convertUTimeToTime(uint64_t uuid_time) {
    if (!uuid_time)
        return 0;
    return (uuid_time-(UUID_Epoch*TENMILLION)) * 100;
}

/*
 * Convert a timestamp to a UUID time
 * @param time  A timestamp in nanosecond resolution
 * @return A UUID timestamp
 */
XAPI uint64_t ism_common_convertTimeToUTime(ism_time_t time) {
    if (!time)
        return 0;
    return (time/100)+(UUID_Epoch*TENMILLION);
}

#ifndef NO_ICU
/**
 * Format the date and time based on the locale.
 * @param locale	the locale
 * @param dateToFormat	the date and time as the number of milliseconds since 1970-01-01T00Z.
 * @param buf   The buffer to return the time string.  This should be 256 bytes long.
 * @return The total buffer size needed; if greater than input buffer length, the output was truncated.
 */
int32_t ism_common_formatMillisTimestampByLocale(const char * locale, int64_t timestamp, char * buf, int len) {
	UErrorCode status = U_ZERO_ERROR;
	UChar result[256];
	int32_t  needed;
	int32_t  used;

	UDateFormat* dfmt = udat_open(UDAT_LONG, UDAT_SHORT, locale, NULL, -1, NULL, -1, &status);
	needed = udat_format(dfmt, timestamp, result, 256, NULL, &status);
    udat_close (dfmt);

	if (U_FAILURE(status)) {
	    used = 0;
        TRACE(6, "Failed to format the date. StatusCode: %d.\n", status);
    } else {
        u_strToUTF8(buf, len, &used, result, needed, &status);
        if (U_FAILURE(status)) {
            used = 0;
        } else {
            if (used >= len) {
                if (len)
                    buf[len-1] = 0;
            } else {
                buf[used] = 0;
            }
        }
    }

	return used;
}

/**
 * Format the ISM timestamp based on the locale.
 * @param locale	the locale
 * @param timestamp	The time in nanaseconds since 1970-01-01T00Z ignoring leap seconds.
 * @param buf  		The buffer to return the time string.
 * @return The total buffer size needed; if greater than input buffer length, the output was truncated.
 */
int32_t ism_common_formatTimestampByLocale(const char * locale, ism_time_t timestamp, char * buf, int len) {
	int64_t timeInMilli = ism_common_convertTimeToJTime(timestamp);
	return ism_common_formatMillisTimestampByLocale(locale, timeInMilli, buf, len);
}
#endif
