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

//****************************************************************************
/// @file  test_utils_monitoring.h
/// @brief Utility functions for testing monitoring
//****************************************************************************
#ifndef __ISM_TEST_UTILS_MONITORING_H_DEFINED
#define __ISM_TEST_UTILS_MONITORING_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include "engine.h"
//#include "security.h"
//#include "transport.h"

#define TEST_ACTION_SUBSCRIPTION "Subscription"
#define TEST_ACTION_QUEUE        "Queue"
#define TEST_ACTION_TOPIC        "Topic"
#define TEST_ACTION_MQTTCLIENT   "MQTTClient"

#define TEST_NAME_STATTYPE      "\"StatType\""
#define TEST_NAME_RESULTCOUNT   "\"ResultCount\""
#define TEST_NAME_SUBNAME       "\""ismENGINE_MONITOR_FILTER_SUBNAME"\""
#define TEST_NAME_CLIENTID      "\""ismENGINE_MONITOR_FILTER_CLIENTID"\""
#define TEST_NAME_TOPICSTRING   "\""ismENGINE_MONITOR_FILTER_TOPICSTRING"\""
#define TEST_NAME_SUBTYPE       "\""ismENGINE_MONITOR_FILTER_SUBTYPE"\""
#define TEST_NAME_QUEUENAME     "\""ismENGINE_MONITOR_FILTER_QUEUENAME"\""

#define TEST_VALUE_BUFFEREDMSGSHIGHEST     "\"BufferedMsgsHighest\""
#define TEST_VALUE_BUFFEREDMSGSLOWEST      "\"BufferedMsgsLowest\""
#define TEST_VALUE_BUFFEREDPERCENTHIGHEST  "\"BufferedPercentHighest\""
#define TEST_VALUE_BUFFEREDPERCENTLOWEST   "\"BufferedPercentLowest\""
#define TEST_VALUE_REJECTEDMSGSHIGHEST     "\"RejectedMsgsHighest\""
#define TEST_VALUE_REJECTEDMSGSLOWEST      "\"RejectedMsgsLowest\""
#define TEST_VALUE_LASTCONNECTEDTIMEOLDEST "\"LastConnectedTimeOldest\""
#define TEST_VALUE_PUBLISHEDMSGSHIGHEST    "\"PublishedMsgsHighest\""
#define TEST_VALUE_PUBLISHEDMSGSLOWEST     "\"PublishedMsgsLowest\""
#define TEST_VALUE_SUBSCRIPTIONSHIGHEST    "\"SubscriptionsHighest\""
#define TEST_VALUE_SUBSCRIPTIONSLOWEST     "\"SubscriptionsLowest\""
#define TEST_VALUE_FAILEDPUBLISHESHIGHEST  "\"FailedPublishesHighest\""
#define TEST_VALUE_FAILEDPUBLISHESLOWEST   "\"FailedPublishesLowest\""
#define TEST_VALUE_PRODUCERSHIGHEST        "\"ProducersHighest\""
#define TEST_VALUE_PRODUCERSLOWEST         "\"ProducersLowest\""
#define TEST_VALUE_CONSUMERSHIGHEST        "\"ConsumersHighest\""
#define TEST_VALUE_CONSUMERSLOWEST         "\"ConsumersLowest\""
#define TEST_VALUE_CONSUMEDMSGSHIGHEST     "\"ConsumedMsgsHighest\""
#define TEST_VALUE_CONSUMEDMSGSLOWEST      "\"ConsumedMsgsLowest\""
#define TEST_VALUE_PRODUCEDMSGSHIGHEST     "\"ProducedMsgsHighest\""
#define TEST_VALUE_PRODUCEDMSGSLOWEST      "\"ProducedMsgsLowest\""

#define TEST_VALUE_BUFFEREDMSGSHIGHEST_SHORT     "\"BufferedMsgs\""
#define TEST_VALUE_BUFFEREDMSGSLOWEST_SHORT      "\"-BufferedMsgs\""
#define TEST_VALUE_BUFFEREDPERCENTHIGHEST_SHORT  "\"BufferedPercent\""
#define TEST_VALUE_BUFFEREDPERCENTLOWEST_SHORT   "\"-BufferedPercent\""
#define TEST_VALUE_REJECTEDMSGSHIGHEST_SHORT     "\"RejectedMsgs\""
#define TEST_VALUE_REJECTEDMSGSLOWEST_SHORT      "\"-RejectedMsgs\""
#define TEST_VALUE_PUBLISHEDMSGSHIGHEST_SHORT    "\"PublishedMsgs\""
#define TEST_VALUE_PUBLISHEDMSGSLOWEST_SHORT     "\"-PublishedMsgs\""

#define TEST_VALUE_ALL                  "\"All\""
#define TEST_VALUE_ASTERISK             "\"*\""

#define TEST_DEFAULT_SUBSCRIPTION_FILTER TEST_NAME_SUBNAME":"TEST_VALUE_ASTERISK","\
                                         TEST_NAME_CLIENTID":"TEST_VALUE_ASTERISK","\
                                         TEST_NAME_TOPICSTRING":"TEST_VALUE_ASTERISK","\
                                         TEST_NAME_SUBTYPE":"TEST_VALUE_ALL
#define TEST_DEFAULT_TOPIC_FILTER        TEST_NAME_TOPICSTRING":"TEST_VALUE_ASTERISK
#define TEST_DEFAULT_QUEUE_FILTER        TEST_NAME_QUEUENAME":"TEST_VALUE_ASTERISK
#define TEST_DEFAULT_MQTTCLIENT_FILTER   TEST_NAME_CLIENTID":"TEST_VALUE_ASTERISK

//****************************************************************************
/// @brief  Make a call to ism_monitoring_getEngineStats with an input JSON string
///
/// @param[in]     action       Which engine stat action is required,
///                             "Subscription", "Topic", "Queue" or "MQTTClient"
/// @param[in]     inputJSON    JSON string to be parsed and passed to
///                             ism_monitoring_getEngineStats
/// @param[out]    outputBuffer output buffer for the results of the request
///
/// @remark If the JSON string need not include the 'Action' or 'User' fields,
///         the Action is assumed as 'set' and user is only used by the admin
///         component that wrappers this function.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_getEngineStats(char *action,
                            const char *inputJSON,
                            concat_alloc_t *outputBuffer);

#endif //end ifndef __ISM_TEST_UTILS_MONITORING_H_DEFINED
