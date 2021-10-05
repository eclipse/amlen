/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

#include <ismutil.h>
#include "unicode/udat.h"
#include "unicode/basictz.h"
using namespace icu;

/*
 * posix and glibc do not define the use of multiple timezones in a process.
 * We therefore use ICU to get the timezone offsets.  However the timezone
 * methods in ICU are only exposed in C++, therefore we have this small
 * C++ file.
 */
extern "C" {

static int getTimeZoneOffsetUntil(BasicTimeZone * tz, ism_time_t timens, ism_time_t * untilns);

struct ism_timezone_t {
    struct ism_timezone_t * next;
    char       name [32];
#ifndef NO_ICU
    BasicTimeZone * icutz;
#endif
    pthread_mutex_t lock;
    ism_time_t valid_until;
    int        offset;
    uint8_t    flags[4];
};

static ism_timezone_t * g_utc_tz = NULL;
static ism_timezone_t * g_tzlist = NULL;
static pthread_mutex_t tzlock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Initialize the timezone by always creating a UTC timezone
 */
void initTimezone(void) {
    pthread_mutex_lock(&tzlock);
    if (g_utc_tz == NULL) {
        ism_timezone_t * timezone = new ism_timezone_t();
        strcpy(timezone->name, "UTC");
#ifndef NO_ICU
        timezone->icutz = (BasicTimeZone *)BasicTimeZone::createTimeZone("UTC");
#endif
        timezone->offset = 0;
        timezone->valid_until = 0x7ffffffffffffLL;   /* vaalid forever */
        pthread_mutex_init(&timezone->lock, 0);
        g_utc_tz = timezone;
    }
    pthread_mutex_unlock(&tzlock);
}

/*
 * Return the timezone name if the timezone exists
 */
const char * ism_common_getTimeZoneName(ism_timezone_t * timezone) {
    if (timezone)
        return timezone->name;
    return NULL;
}

/*
 * This is called when the valitity time has expired for a timezone offset
 */
int  ism_common_checkTimeZone(ism_timezone_t * timezone, ism_time_t now, ism_time_t * valid_until) {
    int ret = 0;
    if (!timezone)
        return 0;
    pthread_mutex_lock(&timezone->lock);
    if (now >= timezone->valid_until) {
#ifndef NO_ICU
        timezone->offset = getTimeZoneOffsetUntil(timezone->icutz, ism_common_currentTimeNanos(), &timezone->valid_until);
#endif
    }
    if (valid_until)
        *valid_until = timezone->valid_until;
    ret = timezone->offset;
    pthread_mutex_unlock(&timezone->lock);
    return ret;
}


/*
 * Get a timezone object
 */
ism_timezone_t * ism_common_getTimeZone(const char * name) {
    ism_timezone_t * ret;
    if (g_utc_tz == NULL)
        initTimezone();

    if (!name || !strcmp(name, "UTC") || strlen(name) > 31)
        return g_utc_tz;

    pthread_mutex_lock(&tzlock);
    ret = g_tzlist;
    while (ret) {
        if (!strcmp(name, ret->name)) {
            pthread_mutex_unlock(&tzlock);
            printf("found timezone: %s\n", ret->name);
            return ret;
        }
        ret = ret->next;
    }

    ism_timezone_t * timezone = new ism_timezone_t();
    strcpy(timezone->name, name);
#ifndef NO_ICU
    timezone->icutz = (BasicTimeZone *)TimeZone::createTimeZone(name);
    timezone->offset = getTimeZoneOffsetUntil(timezone->icutz,
            ism_common_currentTimeNanos(), &timezone->valid_until);
#else
    timezone->offset = 0;
    timezone->valid_until = 0x7ffffffffffffLL;   /* vaalid forever */
#endif
    pthread_mutex_init(&timezone->lock, 0);
    timezone->next = g_tzlist;
    g_tzlist = timezone;
    pthread_mutex_unlock(&tzlock);
    // printf("timezone=%s offset=%d\n", timezone->name, timezone->offset);
    return timezone;
}

#ifndef NO_ICU
#define ADD_YEAR   365*24*80*60*1000LL
#define ADD_3MONTH  90*24*80*60*1000LL
#define ADD_2WEEK   14*24*60*60*1000LL
#define ADD_DAY        24*60*60*1000LL
#define ADD_HOUR          60*60*1000LL

/*
 * Get the offset for the specified time good until a specified time
 *
 * ICU will not tell us how long the offset is valid, but will tell us if it changes
 * within a range we ask for.
 *
 * TODO: The validity checking stuff does not work, fix it
 */
static int getTimeZoneOffsetUntil(BasicTimeZone * tz, ism_time_t timens, ism_time_t * untilns) {
    int64_t ms = timens/1000000;
    int64_t basems = ms - (ms % 3600000);
    UDate timems = (UDate)ms;
    BasicTimeZone * timez = (BasicTimeZone *)tz;
    int32_t tzoffset;
    int32_t dstoffset;
    int32_t tzo2;
    int32_t dst2;
    UErrorCode success = U_ZERO_ERROR;
    timez->getOffset(timems, false, tzoffset, dstoffset, success);
    timez->getOffset(timems+ADD_3MONTH, false, tzo2, dst2, success);
    if (dstoffset == dst2) {
        int32_t dst3;
        int32_t dst4;
        timez->getOffset(timems+ADD_3MONTH+ADD_3MONTH, false, tzo2, dst3, success);
        timez->getOffset(timems+ADD_3MONTH+ADD_3MONTH+ADD_3MONTH, false, tzo2, dst4, success);
        if (dst3==dstoffset && dst4==dstoffset) {
            // printf("valid for a year\n");
            *untilns = (basems + ADD_YEAR) * 1000000;
        } else {
            // printf("valid for 3 months\n");
            *untilns = (basems + ADD_3MONTH) * 1000000;
        }
    } else {
        timez->getOffset(timems+ADD_2WEEK, false, tzo2, dst2, success);
        if (dstoffset == dst2) {
            // printf("valid for 2 weeks\n");
            *untilns = (basems + ADD_2WEEK) * 1000000;
        } else {
            timez->getOffset(timems+ADD_DAY, false, tzo2, dst2, success);
            if (dstoffset == dst2) {
                // printf("valid for day\n");
                *untilns = (basems+ADD_DAY) * 1000000;
            } else {
                // printf("valid for hour\n");
                *untilns = (basems+ADD_HOUR) * 1000000;
            }
        }
    }
    return (tzoffset+dstoffset)/60000;
}
#endif

}
