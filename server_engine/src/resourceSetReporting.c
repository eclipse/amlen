/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

//****************************************************************************
/// @file  resourceSetReporting.c
/// @brief Produce regular reports of resourceSets
//****************************************************************************
#define TRACE_COMP Engine

#include <sys/time.h>
#include <ctype.h>

#include "engineInternal.h"
#include "memHandler.h"
#include "resourceSetStatsInternal.h"
#include "engineMonitoring.h"
#include "engineUtils.h"

void *iere_reportingThread(void *arg, void * context, int value);

//****************************************************************************
/// @brief Generate a hash value for a specified string.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
static uint32_t iere_generateHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}

//****************************************************************************
/// @brief Start resourceSetStats reporting thread if enabled.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iere_startResourceSetReporting(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (resourceSetStatsControl != NULL)
    {
        iereReportingControl_t *reportingControl = &resourceSetStatsControl->reporting;

        assert(reportingControl->threadHandle == 0);

        const char *defaultWeeklyReportStatType;
        const char *defaultDailyReportStatType;
        const char *defaultHourlyReportStatType;

        // Get properties controlling regular resourceSet reporting which default differently in IoT.
        if (ismEngine_serverGlobal.isIoTEnvironment == true)
        {
            defaultWeeklyReportStatType = ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE;
            reportingControl->weeklyReportDay = ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK;
            reportingControl->weeklyReportHour = ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY;
            reportingControl->weeklyReportMaxResults = ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS;
            reportingControl->weeklyReportResetStats = ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS;

            defaultDailyReportStatType = ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_STATTYPE;
            reportingControl->dailyReportHour = ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY;
            reportingControl->dailyReportMaxResults = ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS;
            reportingControl->dailyReportResetStats = ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS;

            defaultHourlyReportStatType = ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE;
            reportingControl->hourlyReportMaxResults = ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS;
            reportingControl->hourlyReportResetStats = ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS;

            reportingControl->minutesPast = ismENGINE_IOTDFLT_RESOURCESET_STATS_MINUTES_PAST_HOUR;
        }
        else
        {
            defaultWeeklyReportStatType = ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE;
            reportingControl->weeklyReportDay = ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK;
            reportingControl->weeklyReportHour = ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY;
            reportingControl->weeklyReportMaxResults = ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS;
            reportingControl->weeklyReportResetStats = ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS;

            defaultDailyReportStatType = ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_STATTYPE;
            reportingControl->dailyReportHour = ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY;
            reportingControl->dailyReportMaxResults = ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS;
            reportingControl->dailyReportResetStats = ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS;

            defaultHourlyReportStatType = ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE;
            reportingControl->hourlyReportMaxResults = ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS;
            reportingControl->hourlyReportResetStats = ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS;

            reportingControl->minutesPast = ismENGINE_DEFAULT_RESOURCESET_STATS_MINUTES_PAST_HOUR;
        }

        reportingControl->requestedReportMonitorType = ismENGINE_MONITOR_NONE;

        // Get any overridden weekly values from the static config
        const char *statType = ism_common_getStringConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE);
        if (statType == NULL) statType = defaultWeeklyReportStatType;
        assert(statType != NULL);

        reportingControl->weeklyReportMonitorType = iere_mapStatTypeToMonitorType(pThreadData, statType, false);

        if (reportingControl->weeklyReportMonitorType != ismENGINE_MONITOR_NONE)
        {
            reportingControl->weeklyReportDay =
                ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK,
                                        reportingControl->weeklyReportDay) % 7;
            reportingControl->weeklyReportHour =
                ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY,
                                        reportingControl->weeklyReportHour) % 24;
            reportingControl->weeklyReportMaxResults =
                (uint32_t)ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS,
                                                  reportingControl->weeklyReportMaxResults);
            reportingControl->weeklyReportResetStats =
                ism_common_getBooleanConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS,
                                            reportingControl->weeklyReportResetStats);
        }

        // Get any overridden daily values from the static config
        statType = ism_common_getStringConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_STATTYPE);
        if (statType == NULL) statType = defaultDailyReportStatType;
        assert(statType != NULL);

        reportingControl->dailyReportMonitorType = iere_mapStatTypeToMonitorType(pThreadData, statType, false);

        if (reportingControl->dailyReportMonitorType != ismENGINE_MONITOR_NONE)
        {
            reportingControl->dailyReportHour =
                ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY,
                                        reportingControl->dailyReportHour) % 24;
            reportingControl->dailyReportMaxResults =
                (uint32_t)ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS,
                                                  reportingControl->dailyReportMaxResults);
            reportingControl->dailyReportResetStats =
                ism_common_getBooleanConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS,
                                            reportingControl->dailyReportResetStats);
        }

        // Get any overridden hourly values from the static config
        statType = ism_common_getStringConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE);
        if (statType == NULL) statType = defaultHourlyReportStatType;
        assert(statType != NULL);

        reportingControl->hourlyReportMonitorType = iere_mapStatTypeToMonitorType(pThreadData, statType, false);

        if (reportingControl->hourlyReportMonitorType != ismENGINE_MONITOR_NONE)
        {
            reportingControl->hourlyReportMaxResults =
                (uint32_t)ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS,
                                                  reportingControl->hourlyReportMaxResults);
            reportingControl->hourlyReportResetStats =
                ism_common_getBooleanConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS,
                                            reportingControl->hourlyReportResetStats);
        }

        // See when past the hour reporting cycles should start
        reportingControl->minutesPast = ism_common_getIntConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_MINUTES_PAST_HOUR,
                                                                reportingControl->minutesPast);
        if (reportingControl->minutesPast >= 0)
        {
            reportingControl->minutesPast = reportingControl->minutesPast % 60;
        }
        else if (reportingControl->minutesPast < iereMINUTES_PAST_UNSPECIFIED)
        {
            reportingControl->minutesPast = iereMINUTES_PAST_UNSPECIFIED;
        }

        // Set the minutes past to something specific if any regular monitoring is needed
        if ((reportingControl->weeklyReportMonitorType != ismENGINE_MONITOR_NONE) ||
            (reportingControl->dailyReportMonitorType != ismENGINE_MONITOR_NONE) ||
            (reportingControl->hourlyReportMonitorType != ismENGINE_MONITOR_NONE))
        {
            if (reportingControl->minutesPast == iereMINUTES_PAST_UNSPECIFIED)
            {
                // Use the serverUID to give us a number of minutes past the hour between 15 and 45
                // meaning that multiple servers running on the same machine will report at different
                // times, but the same server (in an HA pair) will report at the same time.
                const char *serverUID = (const char *)ism_common_getServerUID();

                reportingControl->minutesPast = 15;

                // Modify when we're going to wake up based on serverUID
                if (serverUID != NULL)
                {
                    uint32_t serverUIDHash = iere_generateHash(serverUID);

                    // Set the minutes past to between 15 and 45.
                    reportingControl->minutesPast += (int32_t)(serverUIDHash % 31);
                }
            }
        }
        else
        {
            ieutTRACEL(pThreadData, reportingControl, ENGINE_INTERESTING_TRACE, "Timed resourceSet reporting not enabled\n");
            reportingControl->minutesPast = iereMINUTES_PAST_UNSPECIFIED;
        }

        // Start by setting up the wake-up mechanism
        pthread_condattr_t attr;

        int os_rc = pthread_condattr_init(&attr);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_condattr_init failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        os_rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_002, true, "pthread_condattr_setclock failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        os_rc = pthread_cond_init(&(reportingControl->cond_wakeup), &attr);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_003, true, "pthread_cond_init failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        os_rc = pthread_condattr_destroy(&attr);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_004, true, "pthread_condattr_destroy failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        os_rc = pthread_mutex_init(&(reportingControl->mutex_wakeup), NULL);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_005, true, "pthread_mutex_init failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        // Now start the thread...
        int startRc = ism_common_startThread(&reportingControl->threadHandle,
                                             iere_reportingThread,
                                             NULL, reportingControl, 0,
                                             ISM_TUSAGE_NORMAL,
                                             0,
                                             "resSetReporter",
                                             "Report_Resource_Set_Stats");

        if (startRc != 0)
        {
            ieutTRACEL(pThreadData, startRc, ENGINE_ERROR_TRACE, "ism_common_startThread for resSetreporter failed with %d\n", startRc);
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }
        else
        {
            assert(reportingControl->threadHandle != 0);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

static inline void iere_lockReportingWakeupMutex(iereReportingControl_t *reportingControl)
{
    ismEngine_lockMutex(&(reportingControl->mutex_wakeup));
}

static inline void iere_unlockReportingWakeupMutex(iereReportingControl_t *reportingControl)
{
    ismEngine_unlockMutex(&(reportingControl->mutex_wakeup));
}

//****************************************************************************
/// @brief Wake up the reporting thread
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
static void iere_wakeResourceSetReportingThread(ieutThreadData_t *pThreadData)
{
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (resourceSetStatsControl != NULL && resourceSetStatsControl->reporting.threadHandle != 0)
    {
        iereReportingControl_t *reportingControl = &resourceSetStatsControl->reporting;

        iere_lockReportingWakeupMutex(reportingControl);

        int os_rc = pthread_cond_broadcast(&(reportingControl->cond_wakeup));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "broadcast failed!", ISMRC_Error
                          , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        iere_unlockReportingWakeupMutex(reportingControl);
    }
}

//****************************************************************************
/// @brief Start resourceSetStats reporting thread if enabled.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void iere_stopResourceSetReporting( ieutThreadData_t *pThreadData )
{
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (resourceSetStatsControl != NULL && resourceSetStatsControl->reporting.threadHandle != 0)
    {
        iereReportingControl_t *reportingControl = &resourceSetStatsControl->reporting;

        void *retVal = NULL;

        // Request the reporting thread to end, and wait for it to do so
        reportingControl->endRequested = true;

        // Wake up the reporting thread
        iere_wakeResourceSetReportingThread(pThreadData);

        // Wait for the thread to actually end
        ieut_waitForThread(pThreadData,
                           resourceSetStatsControl->reporting.threadHandle,
                           &retVal,
                           IERE_MAXIMUM_SHUTDOWN_TIMEOUT_SECONDS);

        // Don't expect anything to be returned
        assert(retVal == NULL);

        resourceSetStatsControl->reporting.threadHandle = 0;
    }

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Request a resource set report
///
/// @param[in]     pThreadData      Thread data to use
/// @param[in]     monitorType      Type of request to perform to produce results
/// @param[in]     maxResults       Maximum number of results (ignored if ismENGINE_MONITOR_ALL_UNSORTED)
/// @param[in]     resetStats       Whether to reset the stats with this request.
///
/// @return OK or an ISMRC_ return code indicating an error
//****************************************************************************
int32_t iere_requestResourceSetReport(ieutThreadData_t *pThreadData,
                                      ismEngineMonitorType_t monitorType,
                                      uint32_t maxResults,
                                      bool resetStats)
{
    int32_t rc = OK;

    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "monitorType=%d maxResults=%u resetStats=%d\n",
               __func__, monitorType, maxResults, (int)resetStats);

    if (monitorType == ismENGINE_MONITOR_NONE)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }
    // We do allow the specification ofone of the iereENGINE_MONITOR_FAKE* values so that
    // you can explicitly prompt one of those reports (and so we can test them).
    else if (monitorType > ismENGINE_MONITOR_INTENAL_FAKEWEEKLY)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }

    if (rc != OK) goto mod_exit;

    if (resourceSetStatsControl != NULL && resourceSetStatsControl->reporting.threadHandle != 0)
    {
        iereReportingControl_t *reportingControl = &resourceSetStatsControl->reporting;

        iere_lockReportingWakeupMutex(reportingControl);

        if (reportingControl->requestedReportMonitorType != ismENGINE_MONITOR_NONE)
        {
            rc = ISMRC_ExistingKey;
            ism_common_setError(rc);
        }
        else
        {
            reportingControl->requestedReportMonitorType = monitorType;
            reportingControl->requestedReportMaxResults = maxResults;
            reportingControl->requestedReportResetStats = resetStats;
        }

        if (rc == OK)
        {
            int os_rc = pthread_cond_broadcast(&(reportingControl->cond_wakeup));

            if (UNLIKELY(os_rc != 0))
            {
                ieutTRACE_FFDC( ieutPROBE_001, true, "broadcast failed!", ISMRC_Error
                              , "resourceSetStatsControl", resourceSetStatsControl, sizeof(*resourceSetStatsControl)
                              , "os_rc", &os_rc, sizeof(os_rc)
                              , NULL);
            }
        }

        iere_unlockReportingWakeupMutex(reportingControl);
    }
    else
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Report the results for a single resource set monitor
///
/// @param[in]     pThreadData               Thread data to use
/// @param[in[     monitorType               The type of monitoring requested
/// @param[in]     statType                  String version of the monitoring requested
/// @param[in]     includeSinceRestartValues Whether to include the 'SinceRestart' values
///                                          in the log message.
/// @param[in]     resetTimeString           String version of the reset time
/// @param[in]     reportTimeString          String version of the report time
/// @param[in]     resultPos                 Where we are in the list of results
/// @param[in]     resultCount               How many results there are
/// @param[in]     result                    The result (stats) to report.
//****************************************************************************
void iere_reportResourceSetMonitor(ieutThreadData_t *pThreadData,
                                   ismEngineMonitorType_t monitorType, const char *statType, bool includeSinceRestartValues,
                                   const char *resetTimeString, const char *reportTimeString,
                                   uint32_t resultPos, uint32_t resultCount,
                                   ismEngine_ResourceSetMonitor_t *result)
{
    char *resourceSetID = result->resourceSetId == NULL ? iereOTHER_RESOURCESETS_ID : result->resourceSetId;

    // Sanity check that there are no more stats to report (might need to add more to the JSON).
    assert(sizeof(result->stats) == offsetof(ismEngine_ResourceSetStatistics_t,PersistentClientStates) +
                                    sizeof(result->stats.PersistentClientStates));

    // Produce our output JSON
    char xbuf[4096];
    ieutJSONBuffer_t JSONBuffer = {true, {xbuf, sizeof(xbuf)}};

    ieut_jsonStartObject(&JSONBuffer, NULL);
    ieut_jsonAddString(&JSONBuffer, "ResourceSetID", resourceSetID);
    ieut_jsonAddString(&JSONBuffer, "ResetTime", (char *)resetTimeString);
    ieut_jsonAddString(&JSONBuffer, "ReportTime", (char *)reportTimeString);
    ieut_jsonStartObject(&JSONBuffer, "Stats");
    ieut_jsonAddUInt64(&JSONBuffer, "TotalMemoryBytes", result->stats.TotalMemoryBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "Subscriptions", result->stats.Subscriptions);
    ieut_jsonAddUInt64(&JSONBuffer, "PersistentNonSharedSubscriptions", result->stats.PersistentNonSharedSubscriptions);
    ieut_jsonAddUInt64(&JSONBuffer, "NonpersistentNonSharedSubscriptions", result->stats.NonpersistentNonSharedSubscriptions);
    ieut_jsonAddUInt64(&JSONBuffer, "PersistentSharedSubscriptions", result->stats.PersistentSharedSubscriptions);
    ieut_jsonAddUInt64(&JSONBuffer, "NonpersistentSharedSubscriptions", result->stats.NonpersistentSharedSubscriptions);
    ieut_jsonAddUInt64(&JSONBuffer, "BufferedMsgs", result->stats.BufferedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "DiscardedMsgs", result->stats.DiscardedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "RejectedMsgs", result->stats.RejectedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "RetainedMsgs", result->stats.RetainedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "WillMsgs", result->stats.WillMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "BufferedMsgBytes", result->stats.BufferedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "PersistentBufferedMsgBytes", result->stats.PersistentBufferedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "NonpersistentBufferedMsgBytes", result->stats.NonpersistentBufferedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "RetainedMsgBytes", result->stats.RetainedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "WillMsgBytes", result->stats.WillMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "PersistentWillMsgBytes", result->stats.PersistentWillMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "NonpersistentWillMsgBytes", result->stats.NonpersistentWillMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "PublishedMsgs", result->stats.PublishedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS0PublishedMsgs", result->stats.QoS0PublishedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS1PublishedMsgs", result->stats.QoS1PublishedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS2PublishedMsgs", result->stats.QoS2PublishedMsgs);
    ieut_jsonAddUInt64(&JSONBuffer, "PublishedMsgBytes", result->stats.PublishedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS0PublishedMsgBytes", result->stats.QoS0PublishedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS1PublishedMsgBytes", result->stats.QoS1PublishedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "QoS2PublishedMsgBytes", result->stats.QoS2PublishedMsgBytes);
    ieut_jsonAddUInt64(&JSONBuffer, "MaxPublishRecipients", result->stats.MaxPublishRecipients);
    ieut_jsonAddUInt64(&JSONBuffer, "Connections", result->stats.Connections);
    ieut_jsonAddUInt64(&JSONBuffer, "ActiveClients", result->stats.ActiveClients);
    ieut_jsonAddUInt64(&JSONBuffer, "ActivePersistentClients", result->stats.ActivePersistentClients);
    ieut_jsonAddUInt64(&JSONBuffer, "ActiveNonpersistentClients", result->stats.ActiveNonpersistentClients);
    ieut_jsonAddUInt64(&JSONBuffer, "PersistentClientStates", result->stats.PersistentClientStates);
    if (includeSinceRestartValues == true)
    {
        ieut_jsonAddUInt64(&JSONBuffer, "DiscardedMsgsSinceRestart", result->stats.DiscardedMsgsSinceRestart);
        ieut_jsonAddUInt64(&JSONBuffer, "PublishedMsgsSinceRestart", result->stats.PublishedMsgsSinceRestart);
        ieut_jsonAddUInt64(&JSONBuffer, "PublishedMsgBytesSinceRestart", result->stats.PublishedMsgBytesSinceRestart);
        ieut_jsonAddUInt64(&JSONBuffer, "MaxPublishRecipientsSinceRestart", result->stats.MaxPublishRecipientsSinceRestart);
        ieut_jsonAddUInt64(&JSONBuffer, "ConnectionsSinceRestart", result->stats.ConnectionsSinceRestart);
    }
    ieut_jsonEndObject(&JSONBuffer);
    ieut_jsonEndObject(&JSONBuffer);
    ieut_jsonNullTerminateJSONBuffer(&JSONBuffer);

    ism_common_log_context logContext = {0};
    logContext.resourceSetId = resourceSetID;

    // For AllUnsorted, or the '__OtherResourceSets' set we put out a message which doesn't indicate
    // the position in the list.
    if (monitorType == ismENGINE_MONITOR_ALL_UNSORTED || result->resourceSetId == NULL)
    {
        LOGCTX(&logContext, INFO, Messaging, 3023, "%-s%lu%s",
               "ResourceSetID {0} has TotalMemoryBytes {1}. Full statistics: {2}",
               resourceSetID, result->stats.TotalMemoryBytes, JSONBuffer.buffer.buf);
    }
    else
    {
        LOGCTX(&logContext, INFO, Messaging, 3022, "%-s%u%u%s%s",
               "ResourceSetID {0} is set {1} of {2} sorted by StatType {3}. Full statistics: {4}",
               resourceSetID, resultPos, resultCount, statType, JSONBuffer.buffer.buf);
    }

    // The resultPos might be bigger than resultCount (for other resource sets)
    if (resultPos <= resultCount)
    {
        // TODO: Might want a delay in here after, say doing 1000 results to give the logger a break?
    }

    ieut_jsonReleaseJSONBuffer(&JSONBuffer);
}

//****************************************************************************
/// @brief The reporting thread
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void *iere_reportingThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    iereReportingControl_t *reportingControl = (iereReportingControl_t *)context;

    // Make sure we're thread-initialised.
    ism_engine_threadInit(0);

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, reportingControl, ENGINE_INTERESTING_TRACE,
               FUNCTION_ENTRY "Started thread %s with control %p [Weekly: Type=%u, Max=%u, Reset=%d, Day=%d, Time=%02d:%02d] [Daily: Type=%u, Max=%u, Reset=%d, Time=%02d:%02d] [Hourly: Type=%u, Max=%u, Reset=%d, Time=xx:%02d]\n",
               __func__, threadName, reportingControl,
               // Weekly
               reportingControl->weeklyReportMonitorType, reportingControl->weeklyReportMaxResults,
               (int32_t)reportingControl->weeklyReportResetStats,
               reportingControl->weeklyReportDay, reportingControl->weeklyReportHour, reportingControl->minutesPast,
               // Daily
               reportingControl->dailyReportMonitorType, reportingControl->dailyReportMaxResults,
               (int32_t)reportingControl->dailyReportResetStats,
               reportingControl->dailyReportHour, reportingControl->minutesPast,
               // Hourly
               reportingControl->hourlyReportMonitorType, reportingControl->hourlyReportMaxResults,
               (int32_t)reportingControl->hourlyReportResetStats,
               reportingControl->minutesPast);

    bool mutexLocked = false;
    while(1)
    {
        iere_lockReportingWakeupMutex(reportingControl);
        mutexLocked = true;

        time_t now_time;
        struct tm now_tm;
        struct tm *pNow_tm;
        int32_t os_rc;

        if (reportingControl->endRequested == false &&
            reportingControl->requestedReportMonitorType == ismENGINE_MONITOR_NONE)
        {
            // Ensure that anything waiting for this thread to free memory doesn't wait now
            ieut_leavingEngine(pThreadData);

            if (reportingControl->minutesPast != iereMINUTES_PAST_UNSPECIFIED)
            {
                assert(reportingControl->minutesPast >= 0 && reportingControl->minutesPast < 60);

                now_time = time(NULL);
                pNow_tm = localtime_r(&now_time, &now_tm);

                uint64_t deltaSecs;

                // Can't find the time -- just report in an hour.
                if (pNow_tm == NULL) now_tm.tm_min = reportingControl->minutesPast;

                if (reportingControl->minutesPast > now_tm.tm_min)
                {
                    deltaSecs = 60 * (uint64_t)(reportingControl->minutesPast - now_tm.tm_min);
                }
                else
                {
                    deltaSecs = 60 * (uint64_t)(60 - (now_tm.tm_min - reportingControl->minutesPast));
                }

                struct timespec waituntil;
                clock_gettime(CLOCK_MONOTONIC, &waituntil);

                waituntil.tv_sec += (__time_t)deltaSecs;

                os_rc = pthread_cond_timedwait( &(reportingControl->cond_wakeup)
                                              , &(reportingControl->mutex_wakeup)
                                              , &waituntil);
            }
            else
            {
                os_rc = pthread_cond_wait( &(reportingControl->cond_wakeup)
                                         , &(reportingControl->mutex_wakeup));
            }

            // We are back...
            ieut_enteringEngine(NULL);
        }
        else
        {
            os_rc = OK;
        }

        if (reportingControl->endRequested == true) break;

        if (os_rc != OK && os_rc != ETIMEDOUT)
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_cond_timedwait failed!", ISMRC_Error
                          , "reportingControl", reportingControl, sizeof(*reportingControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        // Allow a requested report to emulate one of the Weekly, Hourly or Daily reports...
        if (reportingControl->requestedReportMonitorType > ismENGINE_MONITOR_NONE)
        {
            // We are pretending to have timed out.
            os_rc = ETIMEDOUT;
            pNow_tm = &now_tm;

            // Out-of-bounds values so we fall through as expected
            pNow_tm->tm_wday = 7;
            pNow_tm->tm_hour = 24;

            if (reportingControl->requestedReportMonitorType == ismENGINE_MONITOR_INTENAL_FAKEWEEKLY)
            {
                pNow_tm->tm_wday = reportingControl->weeklyReportDay;
                pNow_tm->tm_hour = reportingControl->weeklyReportHour;
            }
            else if (reportingControl->requestedReportMonitorType == ismENGINE_MONITOR_INTERNAL_FAKEDAILY)
            {
                pNow_tm->tm_hour = reportingControl->dailyReportHour;
            }
            else
            {
                assert(reportingControl->requestedReportMonitorType == ismENGINE_MONITOR_INTERNAL_FAKEHOURLY);
            }

            reportingControl->requestedReportMonitorType = ismENGINE_MONITOR_NONE;
        }
        else
        {
            now_time = time(NULL);
            pNow_tm = localtime_r(&now_time, &now_tm);
        }

        ismEngine_ResourceSetMonitor_t others;
        ismEngine_ResourceSetMonitor_t *results = NULL;
        ismEngine_ResourceSetMonitor_t *otherSets;
        ismEngineMonitorType_t monitorType;
        bool includeSinceRestartValues;
        uint32_t resultCount = 0;
        uint32_t maxResults;
        bool resetStats;

        // The timer popped -- so we need to do one of the regular reports...
        if (os_rc == ETIMEDOUT)
        {
            // Weekly report (all unsorted)
            if ((reportingControl->weeklyReportMonitorType != ismENGINE_MONITOR_NONE) &&
                ((pNow_tm != NULL) &&
                 (pNow_tm->tm_wday == reportingControl->weeklyReportDay) &&
                 (pNow_tm->tm_hour == reportingControl->weeklyReportHour)))
            {
                monitorType = reportingControl->weeklyReportMonitorType;
                maxResults = reportingControl->weeklyReportMaxResults;
                resetStats = reportingControl->weeklyReportResetStats;
                includeSinceRestartValues = true;
            }
            // Daily report
            else if ((reportingControl->dailyReportMonitorType != ismENGINE_MONITOR_NONE) &&
                     ((pNow_tm != NULL) &&
                      (pNow_tm->tm_hour == reportingControl->dailyReportHour)))
            {
                monitorType = reportingControl->dailyReportMonitorType;
                maxResults = reportingControl->dailyReportMaxResults;
                resetStats = reportingControl->dailyReportResetStats;
                includeSinceRestartValues = false;
            }
            // Hourly report
            else if (reportingControl->hourlyReportMonitorType != ismENGINE_MONITOR_NONE)
            {
                monitorType = reportingControl->hourlyReportMonitorType;
                maxResults = reportingControl->hourlyReportMaxResults;
                resetStats = reportingControl->hourlyReportResetStats;
                includeSinceRestartValues = false;
            }
            // No report
            else
            {
                monitorType = ismENGINE_MONITOR_NONE;
                maxResults = 0;
                resetStats = false;
                includeSinceRestartValues = false;
            }
        }
        // The timer didn't pop -- has a report been requested?
        else
        {
            if (reportingControl->requestedReportMonitorType != ismENGINE_MONITOR_NONE)
            {
                monitorType = reportingControl->requestedReportMonitorType;
                maxResults = reportingControl->requestedReportMaxResults;
                resetStats = reportingControl->requestedReportResetStats;
                includeSinceRestartValues = true;

                // Don't want to do another one...
                reportingControl->requestedReportMonitorType = ismENGINE_MONITOR_NONE;
            }
            // No report
            else
            {
                monitorType = ismENGINE_MONITOR_NONE;
                maxResults = 0;
                resetStats = false;
                includeSinceRestartValues = false;
            }
        }

        iere_unlockReportingWakeupMutex(reportingControl);
        mutexLocked = false;

        ieutTRACEL(pThreadData, monitorType, ENGINE_INTERESTING_TRACE,
                   FUNCTION_IDENT "os_rc=%d monitorType=%d, maxResults=%u, resetStats=%s\n",
                   __func__, os_rc, monitorType, maxResults, resetStats ? "true" : "false");

        // If we have a monitorType, then we should now go get the results and report them.
        if (monitorType != ismENGINE_MONITOR_NONE)
        {
            otherSets = monitorType == ismENGINE_MONITOR_ALL_UNSORTED ? NULL : &others;

            int32_t rc = iemn_getResourceSetMonitor(pThreadData,
                                                    &results,
                                                    &resultCount,
                                                    monitorType,
                                                    maxResults,
                                                    otherSets,
                                                    NULL);

            if (rc != OK)
            {
                ieutTRACEL(pThreadData, monitorType, ENGINE_WORRYING_TRACE,
                           FUNCTION_IDENT "monitorType=%d, maxResults=%u, otherSets=%p, rc=%d\n",
                           __func__, monitorType, maxResults, otherSets, rc);

                ism_common_setError(rc);
            }
            else if (resultCount != 0)
            {
                // If we should reset stats, do it now
                if (resetStats == true) iere_resetResourceSetStats(pThreadData);

                // Get the time strings to report
                ism_time_t resetTimeNanos = results[0].resetTime;
                ism_time_t reportTimeNanos = ism_common_currentTimeNanos();

                char resetTimeString[64];
                char reportTimeString[64];

                ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);

                if (ts != NULL)
                {
                    ism_common_setTimestamp(ts, resetTimeNanos);
                    ism_common_formatTimestamp(ts, resetTimeString, sizeof(resetTimeString), 7, ISM_TFF_ISO8601);
                    ism_common_setTimestamp(ts, reportTimeNanos);
                    ism_common_formatTimestamp(ts, reportTimeString, sizeof(reportTimeString), 7, ISM_TFF_ISO8601);
                    ism_common_closeTimestamp(ts);

                }
                // Cannot format the time strings -- put out the raw numbers!
                else
                {
                    sprintf(resetTimeString, "%lu", resetTimeNanos);
                    sprintf(reportTimeString, "%lu", reportTimeNanos);
                }

                const char *statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);

                for(uint32_t i=0; i<resultCount; i++)
                {
                    assert(results[i].resetTime == resetTimeNanos); // Sanity check they all have the same reset time

                    iere_reportResourceSetMonitor(pThreadData,
                                                  monitorType, statType, includeSinceRestartValues,
                                                  resetTimeString, reportTimeString,
                                                  i+1, resultCount,
                                                  &results[i]);
                }

                // Include the 'all other sets' with a position of 1 past the resultCount.
                if (otherSets != NULL)
                {
                    assert(otherSets->resetTime == resetTimeNanos);

                    iere_reportResourceSetMonitor(pThreadData,
                                                  monitorType, statType, includeSinceRestartValues,
                                                  resetTimeString, reportTimeString,
                                                  resultCount+1, resultCount,
                                                  otherSets);
                }

                // Free the results.
                ism_engine_freeResourceSetMonitor(results);
            }
        }

        // Increase the count of reporting cycles completed...
        __sync_add_and_fetch(&reportingControl->reportingCyclesCompleted, 1);
    }

    if (mutexLocked) iere_unlockReportingWakeupMutex(reportingControl);

    ieutTRACEL(pThreadData, reportingControl, ENGINE_INTERESTING_TRACE, FUNCTION_EXIT "Ending thread %s with control %p (after %u cycles)\n",
               __func__, threadName, reportingControl, reportingControl->reportingCyclesCompleted);
    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}

//****************************************************************************
/// @brief Map the StatType string to an engine monitor type
///
/// @param[in]     pThreadData      Thread data to use
/// @param[in]     statType         StatType string to map
/// @param[in]     allowFakeReports Whether to permit the specification of a
///                                 statType representing fake hourly/daily/weekly
///                                 reports.
///
/// @remark NOTE: Because this comes from a user input, we allow it to be case
/// insensitive -- purely because the rest of the API is.
///
/// @remark If StatType is a number, we return it as the monitorType with only
/// a rudimentary check that it is a valid ismENGINE_MONITOR_ number, not that
/// it is valid for ResourceSet monitoring...
///
/// @return ismEngineMonitorType_t that statType maps to ismENGINE_MONITOR_NONE
/// if there is no mapping.
//****************************************************************************
ismEngineMonitorType_t iere_mapStatTypeToMonitorType(ieutThreadData_t *pThreadData,
                                                     const char *statType,
                                                     bool allowFakeReports)
{
    ismEngineMonitorType_t monitorType;

    char firstChar = toupper(statType[0]);

    // Check the most likely set first...
    if (firstChar == ismENGINE_MONITOR_STATTYPE_NONE[0] &&
        strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_NONE) == 0)
    {
        monitorType = ismENGINE_MONITOR_NONE;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_ALLUNSORTED[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_ALLUNSORTED) == 0)
    {
        monitorType = ismENGINE_MONITOR_ALL_UNSORTED;
    }
    // Check for a numeric specification...
    else if (firstChar == '-' || (firstChar >= '0' && firstChar <= '9'))
    {
        int64_t value = strtol(statType, NULL, 10);

        if (value < 0 || ((allowFakeReports == true && value > ismENGINE_MONITOR_INTENAL_FAKEWEEKLY) ||
                          (allowFakeReports == false && value > ismENGINE_MONITOR_MAX)))
        {
            monitorType = ismENGINE_MONITOR_NONE;
            ieutTRACEL(pThreadData, monitorType, ENGINE_ERROR_TRACE,
                       FUNCTION_IDENT "Invalid numeric statType specified %ld ('%s')\n", __func__, value, statType);
        }
        else
        {
            monitorType = (ismEngineMonitorType_t)value;
        }
    }
    // Now check for the less likely set...
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_WILLMSGS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_CONNECTIONS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS;
    }
    else if (firstChar == ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST) == 0)
    {
        monitorType = ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES;
    }
    else if (allowFakeReports == true && firstChar == ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY) == 0)
    {
        monitorType = ismENGINE_MONITOR_INTERNAL_FAKEHOURLY;
    }
    else if (allowFakeReports == true && firstChar == ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY) == 0)
    {
        monitorType = ismENGINE_MONITOR_INTERNAL_FAKEDAILY;
    }
    else if (allowFakeReports == true && firstChar == ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY[0] &&
             strcasecmp(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY) == 0)
    {
        monitorType = ismENGINE_MONITOR_INTENAL_FAKEWEEKLY;
    }
    else
    {
        monitorType = ismENGINE_MONITOR_NONE;
        ieutTRACEL(pThreadData, monitorType, ENGINE_ERROR_TRACE,
                   FUNCTION_IDENT "Unexpected statType '%s'\n", __func__, statType);
    }

    return monitorType;
}

//****************************************************************************
/// @brief Map a monitorType to the StatType string it represents
///
/// @param[in]     pThreadData      Thread data to use
/// @param[in]     monitorType      MonitorType to map
///
/// @remark If MonitorType is not one of the set that we expect for a resourceSet
/// monitor request the string 'None' is returned.
///
/// @return String representation of the StatType for the monitorType.
//****************************************************************************
const char *iere_mapMonitorTypeToStatType(ieutThreadData_t *pThreadData, ismEngineMonitorType_t monitorType)
{
    const char *statType;

    switch(monitorType)
    {
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS:
            statType = ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_ALL_UNSORTED:
            statType = ismENGINE_MONITOR_STATTYPE_ALLUNSORTED;
            break;
        case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_WILLMSGS:
            statType = ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES:
            statType = ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_CONNECTIONS:
            statType = ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS:
            statType = ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS:
            statType = ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS:
            statType = ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST;
            break;
        case ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES:
            statType = ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST;
            break;
        case ismENGINE_MONITOR_INTERNAL_FAKEHOURLY:
            statType = ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY;
            break;
        case ismENGINE_MONITOR_INTERNAL_FAKEDAILY:
            statType = ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY;
            break;
        case ismENGINE_MONITOR_INTENAL_FAKEWEEKLY:
            statType = ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY;
            break;
        case ismENGINE_MONITOR_NONE:
        default:
            statType = ismENGINE_MONITOR_STATTYPE_NONE;
            break;
    }

    return statType;
}
