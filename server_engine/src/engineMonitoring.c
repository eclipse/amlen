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
/// @file  engineMonitoring.c
/// @brief Engine component monitoring functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "queueCommon.h"          // ieq functions & constants
#include "queueNamespace.h"       // queue namespace
#include "clientState.h"
#include "engineMonitoring.h"
#include "remoteServers.h"
#include "engineUtils.h"
#include "resourceSetStats.h"    // iere functions & constants

//****************************************************************************
/// @brief  Various functions to compare two Subscription Monitor records
//****************************************************************************
static int32_t iemn_highestSubscriptionMonitorBufferedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                           const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.BufferedMsgs > smonB->stats.BufferedMsgs) return 1;
    if (smonA->stats.BufferedMsgs < smonB->stats.BufferedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorBufferedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                          const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.BufferedMsgs < smonB->stats.BufferedMsgs) return 1;
    if (smonA->stats.BufferedMsgs > smonB->stats.BufferedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestSubscriptionMonitorBufferedPercent(const ismEngine_SubscriptionMonitor_t *smonA,
                                                              const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.BufferedPercent > smonB->stats.BufferedPercent) return 1;
    if (smonA->stats.BufferedPercent < smonB->stats.BufferedPercent) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorBufferedPercent(const ismEngine_SubscriptionMonitor_t *smonA,
                                                             const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.BufferedPercent < smonB->stats.BufferedPercent) return 1;
    if (smonA->stats.BufferedPercent > smonB->stats.BufferedPercent) return -1;
    return 0;
}

static int32_t iemn_highestSubscriptionMonitorPublishedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                            const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.ProducedMsgs > smonB->stats.ProducedMsgs) return 1;
    if (smonA->stats.ProducedMsgs < smonB->stats.ProducedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorPublishedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                           const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.ProducedMsgs < smonB->stats.ProducedMsgs) return 1;
    if (smonA->stats.ProducedMsgs > smonB->stats.ProducedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestSubscriptionMonitorRejectedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                           const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.RejectedMsgs > smonB->stats.RejectedMsgs) return 1;
    if (smonA->stats.RejectedMsgs < smonB->stats.RejectedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorRejectedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                          const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.RejectedMsgs < smonB->stats.RejectedMsgs) return 1;
    if (smonA->stats.RejectedMsgs > smonB->stats.RejectedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestSubscriptionMonitorBufferedHWMPercent(const ismEngine_SubscriptionMonitor_t *smonA,
                                                                 const ismEngine_SubscriptionMonitor_t *smonB)
{
  if (smonA->stats.BufferedHWMPercent > smonB->stats.BufferedHWMPercent) return 1;
  if (smonA->stats.BufferedHWMPercent < smonB->stats.BufferedHWMPercent) return -1;
  return 0;
}

static int32_t iemn_lowestSubscriptionMonitorBufferedHWMPercent(const ismEngine_SubscriptionMonitor_t *smonA,
                                                                const ismEngine_SubscriptionMonitor_t *smonB)
{
  if (smonA->stats.BufferedHWMPercent < smonB->stats.BufferedHWMPercent) return 1;
  if (smonA->stats.BufferedHWMPercent > smonB->stats.BufferedHWMPercent) return -1;
  return 0;
}

static int32_t iemn_highestSubscriptionMonitorExpiredMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                          const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.ExpiredMsgs > smonB->stats.ExpiredMsgs) return 1;
    if (smonA->stats.ExpiredMsgs < smonB->stats.ExpiredMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorExpiredMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                         const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.ExpiredMsgs < smonB->stats.ExpiredMsgs) return 1;
    if (smonA->stats.ExpiredMsgs > smonB->stats.ExpiredMsgs) return -1;
    return 0;
}

static int32_t iemn_highestSubscriptionMonitorDiscardedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                            const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.DiscardedMsgs > smonB->stats.DiscardedMsgs) return 1;
    if (smonA->stats.DiscardedMsgs < smonB->stats.DiscardedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestSubscriptionMonitorDiscardedMsgs(const ismEngine_SubscriptionMonitor_t *smonA,
                                                           const ismEngine_SubscriptionMonitor_t *smonB)
{
    if (smonA->stats.DiscardedMsgs < smonB->stats.DiscardedMsgs) return 1;
    if (smonA->stats.DiscardedMsgs > smonB->stats.DiscardedMsgs) return -1;
    return 0;
}

//****************************************************************************
/// @brief  Various functions to compare two Topic Monitor records
//****************************************************************************
static int32_t iemn_highestTopicMonitorPublishedMsgs(const ismEngine_TopicMonitor_t *tmonA,
                                                     const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.PublishedMsgs > tmonB->stats.PublishedMsgs) return 1;
    if (tmonA->stats.PublishedMsgs < tmonB->stats.PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestTopicMonitorPublishedMsgs(const ismEngine_TopicMonitor_t *tmonA,
                                                    const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.PublishedMsgs < tmonB->stats.PublishedMsgs) return 1;
    if (tmonA->stats.PublishedMsgs > tmonB->stats.PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestTopicMonitorSubscriptions(const ismEngine_TopicMonitor_t *tmonA,
                                                     const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->subscriptions > tmonB->subscriptions) return 1;
    if (tmonA->subscriptions < tmonB->subscriptions) return -1;
    return 0;
}

static int32_t iemn_lowestTopicMonitorSubscriptions(const ismEngine_TopicMonitor_t *tmonA,
                                                    const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->subscriptions < tmonB->subscriptions) return 1;
    if (tmonA->subscriptions > tmonB->subscriptions) return -1;
    return 0;
}

static int32_t iemn_highestTopicMonitorRejectedMsgs(const ismEngine_TopicMonitor_t *tmonA,
                                                    const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.RejectedMsgs > tmonB->stats.RejectedMsgs) return 1;
    if (tmonA->stats.RejectedMsgs < tmonB->stats.RejectedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestTopicMonitorRejectedMsgs(const ismEngine_TopicMonitor_t *tmonA,
                                                   const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.RejectedMsgs < tmonB->stats.RejectedMsgs) return 1;
    if (tmonA->stats.RejectedMsgs > tmonB->stats.RejectedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestTopicMonitorFailedPublishes(const ismEngine_TopicMonitor_t *tmonA,
                                                       const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.FailedPublishes > tmonB->stats.FailedPublishes) return 1;
    if (tmonA->stats.FailedPublishes < tmonB->stats.FailedPublishes) return -1;
    return 0;
}

static int32_t iemn_lowestTopicMonitorFailedPublishes(const ismEngine_TopicMonitor_t *tmonA,
                                                      const ismEngine_TopicMonitor_t *tmonB)
{
    if (tmonA->stats.FailedPublishes < tmonB->stats.FailedPublishes) return 1;
    if (tmonA->stats.FailedPublishes > tmonB->stats.FailedPublishes) return -1;
    return 0;
}

//****************************************************************************
/// @brief  Various functions to compare two Queue Monitor records
//****************************************************************************
static int32_t iemn_highestQueueMonitorBufferedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                    const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedMsgs > qmonB->stats.BufferedMsgs) return 1;
    if (qmonA->stats.BufferedMsgs < qmonB->stats.BufferedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorBufferedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                   const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedMsgs < qmonB->stats.BufferedMsgs) return 1;
    if (qmonA->stats.BufferedMsgs > qmonB->stats.BufferedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorBufferedPercent(const ismEngine_QueueMonitor_t *qmonA,
                                                       const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedPercent > qmonB->stats.BufferedPercent) return 1;
    if (qmonA->stats.BufferedPercent < qmonB->stats.BufferedPercent) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorBufferedPercent(const ismEngine_QueueMonitor_t *qmonA,
                                                      const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedPercent < qmonB->stats.BufferedPercent) return 1;
    if (qmonA->stats.BufferedPercent > qmonB->stats.BufferedPercent) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorProducedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                    const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ProducedMsgs > qmonB->stats.ProducedMsgs) return 1;
    if (qmonA->stats.ProducedMsgs < qmonB->stats.ProducedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorProducedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                   const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ProducedMsgs < qmonB->stats.ProducedMsgs) return 1;
    if (qmonA->stats.ProducedMsgs > qmonB->stats.ProducedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorConsumedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                    const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ConsumedMsgs > qmonB->stats.ConsumedMsgs) return 1;
    if (qmonA->stats.ConsumedMsgs < qmonB->stats.ConsumedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorConsumedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                   const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ConsumedMsgs < qmonB->stats.ConsumedMsgs) return 1;
    if (qmonA->stats.ConsumedMsgs > qmonB->stats.ConsumedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorProducers(const ismEngine_QueueMonitor_t *qmonA,
                                                 const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->producers > qmonB->producers) return 1;
    if (qmonA->producers < qmonB->producers) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorProducers(const ismEngine_QueueMonitor_t *qmonA,
                                                const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->producers < qmonB->producers) return 1;
    if (qmonA->producers > qmonB->producers) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorConsumers(const ismEngine_QueueMonitor_t *qmonA,
                                                 const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->consumers > qmonB->consumers) return 1;
    if (qmonA->consumers < qmonB->consumers) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorConsumers(const ismEngine_QueueMonitor_t *qmonA,
                                                const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->consumers < qmonB->consumers) return 1;
    if (qmonA->consumers > qmonB->consumers) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorBufferedHWMPercent(const ismEngine_QueueMonitor_t *qmonA,
                                                          const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedHWMPercent > qmonB->stats.BufferedHWMPercent) return 1;
    if (qmonA->stats.BufferedHWMPercent < qmonB->stats.BufferedHWMPercent) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorBufferedHWMPercent(const ismEngine_QueueMonitor_t *qmonA,
                                                         const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.BufferedHWMPercent < qmonB->stats.BufferedHWMPercent) return 1;
    if (qmonA->stats.BufferedHWMPercent > qmonB->stats.BufferedHWMPercent) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorExpiredMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                   const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ExpiredMsgs > qmonB->stats.ExpiredMsgs) return 1;
    if (qmonA->stats.ExpiredMsgs < qmonB->stats.ExpiredMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorExpiredMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                  const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.ExpiredMsgs < qmonB->stats.ExpiredMsgs) return 1;
    if (qmonA->stats.ExpiredMsgs > qmonB->stats.ExpiredMsgs) return -1;
    return 0;
}

static int32_t iemn_highestQueueMonitorDiscardedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                     const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.DiscardedMsgs > qmonB->stats.DiscardedMsgs) return 1;
    if (qmonA->stats.DiscardedMsgs < qmonB->stats.DiscardedMsgs) return -1;
    return 0;
}

static int32_t iemn_lowestQueueMonitorDiscardedMsgs(const ismEngine_QueueMonitor_t *qmonA,
                                                    const ismEngine_QueueMonitor_t *qmonB)
{
    if (qmonA->stats.DiscardedMsgs < qmonB->stats.DiscardedMsgs) return 1;
    if (qmonA->stats.DiscardedMsgs > qmonB->stats.DiscardedMsgs) return -1;
    return 0;
}

//****************************************************************************
/// @brief  Various functions to compare two Transaction Monitor records
//****************************************************************************
static int32_t iemn_oldestTransactionStateChange(const ismEngine_TransactionMonitor_t *tmonA,
                                                 const ismEngine_TransactionMonitor_t *tmonB)
{
  if (tmonA->stateChangedTime < tmonB->stateChangedTime) return 1;
  if (tmonA->stateChangedTime > tmonB->stateChangedTime) return -1;
  return 0;
}

//****************************************************************************
/// @brief  Various functions to compare two ResourceSet Monitor records
//****************************************************************************
static int32_t iemn_highestResourceSetMonitorTotalMemoryBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                              const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.TotalMemoryBytes > rmonB->stats.TotalMemoryBytes) return 1;
    if (rmonA->stats.TotalMemoryBytes < rmonB->stats.TotalMemoryBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorSubscriptions(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.Subscriptions > rmonB->stats.Subscriptions) return 1;
    if (rmonA->stats.Subscriptions < rmonB->stats.Subscriptions) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPersistentNonSharedSubscriptions(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                              const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PersistentNonSharedSubscriptions > rmonB->stats.PersistentNonSharedSubscriptions) return 1;
    if (rmonA->stats.PersistentNonSharedSubscriptions < rmonB->stats.PersistentNonSharedSubscriptions) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorNonpersistentNonSharedSubscriptions(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                                 const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.NonpersistentNonSharedSubscriptions > rmonB->stats.NonpersistentNonSharedSubscriptions) return 1;
    if (rmonA->stats.NonpersistentNonSharedSubscriptions < rmonB->stats.NonpersistentNonSharedSubscriptions) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPersistentSharedSubscriptions(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PersistentSharedSubscriptions > rmonB->stats.PersistentSharedSubscriptions) return 1;
    if (rmonA->stats.PersistentSharedSubscriptions < rmonB->stats.PersistentSharedSubscriptions) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorNonpersistentSharedSubscriptions(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                              const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.NonpersistentSharedSubscriptions > rmonB->stats.NonpersistentSharedSubscriptions) return 1;
    if (rmonA->stats.NonpersistentSharedSubscriptions < rmonB->stats.NonpersistentSharedSubscriptions) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorBufferedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                          const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.BufferedMsgs > rmonB->stats.BufferedMsgs) return 1;
    if (rmonA->stats.BufferedMsgs < rmonB->stats.BufferedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorDiscardedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.DiscardedMsgs > rmonB->stats.DiscardedMsgs) return 1;
    if (rmonA->stats.DiscardedMsgs < rmonB->stats.DiscardedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorRejectedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                          const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.RejectedMsgs > rmonB->stats.RejectedMsgs) return 1;
    if (rmonA->stats.RejectedMsgs < rmonB->stats.RejectedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorRetainedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                          const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.RetainedMsgs > rmonB->stats.RetainedMsgs) return 1;
    if (rmonA->stats.RetainedMsgs < rmonB->stats.RetainedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorWillMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                      const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.WillMsgs > rmonB->stats.WillMsgs) return 1;
    if (rmonA->stats.WillMsgs < rmonB->stats.WillMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorBufferedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                              const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.BufferedMsgBytes > rmonB->stats.BufferedMsgBytes) return 1;
    if (rmonA->stats.BufferedMsgBytes < rmonB->stats.BufferedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPersistentBufferedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                        const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PersistentBufferedMsgBytes > rmonB->stats.PersistentBufferedMsgBytes) return 1;
    if (rmonA->stats.PersistentBufferedMsgBytes < rmonB->stats.PersistentBufferedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorNonpersistentBufferedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.NonpersistentBufferedMsgBytes > rmonB->stats.NonpersistentBufferedMsgBytes) return 1;
    if (rmonA->stats.NonpersistentBufferedMsgBytes < rmonB->stats.NonpersistentBufferedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorRetainedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                              const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.RetainedMsgBytes > rmonB->stats.RetainedMsgBytes) return 1;
    if (rmonA->stats.RetainedMsgBytes < rmonB->stats.RetainedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorWillMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                          const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.WillMsgBytes > rmonB->stats.WillMsgBytes) return 1;
    if (rmonA->stats.WillMsgBytes < rmonB->stats.WillMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPersistentWillMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                    const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PersistentWillMsgBytes > rmonB->stats.PersistentWillMsgBytes) return 1;
    if (rmonA->stats.PersistentWillMsgBytes < rmonB->stats.PersistentWillMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorNonpersistentWillMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                       const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.NonpersistentWillMsgBytes > rmonB->stats.NonpersistentWillMsgBytes) return 1;
    if (rmonA->stats.NonpersistentWillMsgBytes < rmonB->stats.NonpersistentWillMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPublishedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PublishedMsgs > rmonB->stats.PublishedMsgs) return 1;
    if (rmonA->stats.PublishedMsgs < rmonB->stats.PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS0PublishedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                               const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS0PublishedMsgs > rmonB->stats.QoS0PublishedMsgs) return 1;
    if (rmonA->stats.QoS0PublishedMsgs < rmonB->stats.QoS0PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS1PublishedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                               const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS1PublishedMsgs > rmonB->stats.QoS1PublishedMsgs) return 1;
    if (rmonA->stats.QoS1PublishedMsgs < rmonB->stats.QoS1PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS2PublishedMsgs(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                               const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS2PublishedMsgs > rmonB->stats.QoS2PublishedMsgs) return 1;
    if (rmonA->stats.QoS2PublishedMsgs < rmonB->stats.QoS2PublishedMsgs) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPublishedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                               const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PublishedMsgBytes > rmonB->stats.PublishedMsgBytes) return 1;
    if (rmonA->stats.PublishedMsgBytes < rmonB->stats.PublishedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS0PublishedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                   const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS0PublishedMsgBytes > rmonB->stats.QoS0PublishedMsgBytes) return 1;
    if (rmonA->stats.QoS0PublishedMsgBytes < rmonB->stats.QoS0PublishedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS1PublishedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                   const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS1PublishedMsgBytes > rmonB->stats.QoS1PublishedMsgBytes) return 1;
    if (rmonA->stats.QoS1PublishedMsgBytes < rmonB->stats.QoS1PublishedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorQoS2PublishedMsgBytes(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                   const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.QoS2PublishedMsgBytes > rmonB->stats.QoS2PublishedMsgBytes) return 1;
    if (rmonA->stats.QoS2PublishedMsgBytes < rmonB->stats.QoS2PublishedMsgBytes) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorMaxPublishRecipients(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                  const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.MaxPublishRecipients > rmonB->stats.MaxPublishRecipients) return 1;
    if (rmonA->stats.MaxPublishRecipients < rmonB->stats.MaxPublishRecipients) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorConnections(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                         const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.Connections > rmonB->stats.Connections) return 1;
    if (rmonA->stats.Connections < rmonB->stats.Connections) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorActiveClients(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                           const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.ActiveClients > rmonB->stats.ActiveClients) return 1;
    if (rmonA->stats.ActiveClients < rmonB->stats.ActiveClients) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorActivePersistentClients(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                     const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.ActivePersistentClients > rmonB->stats.ActivePersistentClients) return 1;
    if (rmonA->stats.ActivePersistentClients < rmonB->stats.ActivePersistentClients) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorActiveNonpersistentClients(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                        const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.ActiveNonpersistentClients > rmonB->stats.ActiveNonpersistentClients) return 1;
    if (rmonA->stats.ActiveNonpersistentClients < rmonB->stats.ActiveNonpersistentClients) return -1;
    return 0;
}

static int32_t iemn_highestResourceSetMonitorPersistentClientStates(const ismEngine_ResourceSetMonitor_t *rmonA,
                                                                    const ismEngine_ResourceSetMonitor_t *rmonB)
{
    if (rmonA->stats.PersistentClientStates > rmonB->stats.PersistentClientStates) return 1;
    if (rmonA->stats.PersistentClientStates < rmonB->stats.PersistentClientStates) return -1;
    return 0;
}

//****************************************************************************
/// @brief  Match the specified subscription with the specified mask
///
/// @param[in]     subscription      The subscription to check
/// @param[in]     policyInfo        The policy information structure to check
/// @param[in]     filters           The filters to apply to the subscription
///
/// @return true if the subscription matches, otherwise false.
//****************************************************************************
static bool iemn_matchSubscriptionFilters(ismEngine_Subscription_t *subscription,
                                          iepiPolicyInfo_t *policyInfo,
                                          const iemnSubscriptionFilters_t *filters)
{
    bool match = false;

    // Check the subscription options
    if ((filters->subOptionsMask != ismENGINE_SUBSCRIPTION_OPTION_NONE) &&
        ((subscription->subOptions & filters->subOptionsMask) != filters->subOptionsValue))
        goto mod_exit;

    // Check the subscription internal attributes
    if ((filters->internalAttrsMask != iettSUBATTR_NONE) &&
        ((subscription->internalAttrs & filters->internalAttrsMask) != filters->internalAttrsValue))
        goto mod_exit;

    // Check the clientId
    if (NULL != filters->clientId)
    {
        if (NULL == subscription->clientId)
            goto mod_exit;

        if (ism_common_match(subscription->clientId, filters->clientId) == 0)
            goto mod_exit;
    }

    // Check the regular expression clientId
    if (NULL != filters->regexClientId)
    {
        if (NULL == subscription->clientId)
            goto mod_exit;

        if (ism_regex_match(filters->regexClientId, subscription->clientId) != 0)
            goto mod_exit;
    }

    // Check for it being an admin subscription
    if (filters->anonymousSharersMask != iettNO_ANONYMOUS_SHARER)
    {
        assert((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0);

        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

        if (NULL == sharedSubData ||
            (sharedSubData->anonymousSharers & filters->anonymousSharersMask) != filters->anonymousSharersValue)
        {
            goto mod_exit;
        }
    }

    // Check the subscription name
    if (NULL != filters->subName)
    {
        if (NULL == subscription->subName)
            goto mod_exit;

        if (ism_common_match(subscription->subName, filters->subName) == 0)
            goto mod_exit;
    }

    // Check that the resource Set identifier matches if specified.
    if (NULL != filters->resourceSetIdentifier)
    {
        const char *resourceSetIdentifier = iere_getResourceSetIdentifier(subscription->resourceSet);

        if (NULL == resourceSetIdentifier)
            goto mod_exit;

        if (ism_common_match(resourceSetIdentifier, filters->resourceSetIdentifier) == 0)
            goto mod_exit;
    }

    // Check the topic string
    if (NULL != filters->topicString)
    {
        assert(NULL != subscription->node->topicString);

        if (ism_common_match(subscription->node->topicString, filters->topicString) == 0)
            goto mod_exit;
    }

    // Check the messaging policy name
    if (NULL != filters->messagingPolicyName)
    {
        // Deleted policy / unique policy
        if (NULL == policyInfo->name)
            goto mod_exit;

        if (ism_common_match(policyInfo->name, filters->messagingPolicyName) == 0)
            goto mod_exit;
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}

//****************************************************************************
/// @brief  Match the specified topic (subsNode) with the specified mask
///
/// @param[in]     node     Subscription node (topic)
/// @param[in]     filters  The filters to apply
///
/// @return true if the subscription matches, otherwise false.
//****************************************************************************
static bool iemn_matchTopicFilters(iettSubsNode_t *node,
                                   iemnTopicFilters_t *filters)
{
    bool match = false;

    // Check the topic string matches this node's topic string.
    if (NULL != filters->topicString)
    {
        assert((node->nodeFlags & iettNODE_FLAG_TYPE_MASK) == iettNODE_FLAG_MULTICARD);
        assert(node->topicString != NULL);

        if (ism_common_match(node->topicString, filters->topicString) == 0)
            goto mod_exit;
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}


//****************************************************************************
/// @brief  Match the specified client with the specified filters
///
/// @param[in]     pClient  ClientState
/// @param[in]     filters  The filters to apply
///
/// @return true if the client state matches, otherwise false.
//****************************************************************************
static bool iemn_matchClientStateFilters(ismEngine_ClientState_t *pClient,
                                         iemnClientStateFilters_t *filters)
{
    bool match = false;

    // Check the clientId matches this client ID
    if (NULL != filters->clientId)
    {
        if (ism_common_match(pClient->pClientId, filters->clientId) == 0)
            goto mod_exit;
    }

    // Check the clientId matches this client ID using a RegEx
    if (NULL != filters->regexClientId)
    {
        if (ism_regex_match(filters->regexClientId, pClient->pClientId) != 0)
            goto mod_exit;
    }

    // Check the resourceSet ID matches this client's resource set
    if (NULL != filters->resourceSetIdentifier)
    {
        const char *resourceSetIdentifier = iere_getResourceSetIdentifier(pClient->resourceSet);

        if (NULL == resourceSetIdentifier)
            goto mod_exit;

        if (ism_common_match(resourceSetIdentifier, filters->resourceSetIdentifier) == 0)
            goto mod_exit;
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}

//****************************************************************************
/// @brief  Match the specified resource set with the specified filters
///
/// @param[in]     pResourceSet  Resource set
/// @param[in]     filters       The filters to apply
///
/// @return true if the resource set matches, otherwise false.
//****************************************************************************
static bool iemn_matchResourceSetFilters(iereResourceSet_t *pResourceSet,
                                         iemnResourceSetFilters_t *filters)
{
    bool match = false;

    // Check the resource set matches this resource set's identifier
    if (NULL != filters->resourceSetId)
    {
        if (ism_common_match(pResourceSet->stats.resourceSetIdentifier, filters->resourceSetId) == 0)
            goto mod_exit;
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for subscriptions
///
/// @param[out]    results           The returned list of subscription monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeSubscriptionMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getSubscriptionMonitor(
                                                ismEngine_SubscriptionMonitor_t **results
                                              , uint32_t *resultCount
                                              , ismEngineMonitorType_t type
                                              , uint32_t maxResults
                                              , ism_prop_t *filterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u filterProperties=%p\n", __func__,
               type, maxResults, filterProperties);

    assert(results != NULL);
    assert(resultCount != NULL);

    uint32_t checkedCount = 0;
    uint32_t localResultCount = 0;
    ismEngine_SubscriptionMonitor_t *localResults = NULL;
    uint32_t pos;
    ismEngine_SubscriptionMonitor_t localSubscriptionMonitor = {0};

    iemnSubscriptionFilters_t filters = iemnSUBSCRIPTION_FILTERS_DEFAULT;

    // For 'all unsorted' case we reallocate as needed, we ignore maxResults passed in
    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        maxResults = 0;
    }
    else
    {
        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        localResults = iemem_calloc(pThreadData,
                                    IEMEM_PROBE(iemem_monitoringData, 1), 1,
                                    (maxResults+1) * sizeof(ismEngine_SubscriptionMonitor_t));

        if (NULL == localResults)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // Deal with filtering of results
    bool applyFilters = false;

    // Some filter properties were specified, analyse these now
    if (NULL != filterProperties)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on subscription name
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_SUBNAME);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.subName = tempFilter;
            applyFilters = true;
        }

        // Check for a non-trivial filter on client Id
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_CLIENTID);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.clientId = tempFilter;
            applyFilters = true;
        }

        // Check for filter on regular expression client ID
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_REGEX_CLIENTID);
        if (tempFilter != NULL)
        {
            int regex_rc = ism_regex_compile(&filters.regexClientId, tempFilter);
            if (regex_rc != 0)
            {
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit;
            }
            applyFilters = true;
        }

        // Check for a non-trivial filter on topicString
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_TOPICSTRING);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.topicString = tempFilter;
            applyFilters = true;
        }

        // Check for non-trivial filter on messaging policy name
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.messagingPolicyName = tempFilter;
            applyFilters = true;
        }

        // Check for non-trivial filter on Resource Set Identifier
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            if (iere_isResourceSetTrackingEnabled() == true)
            {
                filters.resourceSetIdentifier = tempFilter;
                applyFilters = true;
            }
            else
            {
                ieutTRACEL(pThreadData, tempFilter, ENGINE_INTERESTING_TRACE,
                           FUNCTION_IDENT "Ignoring ResourceSetID filter '%s' because resource sets not tracked\n",
                           __func__, tempFilter);
            }
        }

        // Check for a non-trivial filter on subscription type (Durable/Nondurable, Shared/Nonshared)
        // Note: That this is a comma separated list
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_SUBTYPE);
        if (NULL != tempFilter && strncasecmp(tempFilter, ismENGINE_MONITOR_FILTER_SUBTYPE_ALL, 3))
        {
            const char *filterPos = tempFilter;
            while(filterPos != NULL)
            {
                size_t filterLen;
                char *endPos = strchr(filterPos, ',');

                if (endPos != NULL)
                {
                    filterLen = (size_t)(endPos-filterPos);
                    endPos += 1;
                }
                else
                {
                    filterLen = strlen(filterPos);
                }

                // Skip over leading spaces
                while(*filterPos == ' ')
                {
                    filterPos++;
                    filterLen--;
                }

                // Ignore trailing spaces
                while(filterLen > 0 && filterPos[filterLen-1] == ' ')
                {
                    filterLen--;
                }

                // Check this filter.

                // We look at the PERSISTENCE of the subscription when asked about DURABILITY,
                // because for non-shared subscription there is a 1:1 mapping between persistence
                // and durability, and for shared subscriptions there is too, unless it's a
                // subscription with mixed durability consumers, in which case we make it persistent
                // too.
                if (filterLen == strlen(ismENGINE_MONITOR_FILTER_SUBTYPE_DURABLE) &&
                    !strncasecmp(filterPos,
                                 ismENGINE_MONITOR_FILTER_SUBTYPE_DURABLE,
                                 filterLen))
                {
                    filters.internalAttrsMask |= iettSUBATTR_PERSISTENT;
                    filters.internalAttrsValue |= iettSUBATTR_PERSISTENT;
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_SUBTYPE_NONDURABLE) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_SUBTYPE_NONDURABLE,
                                      filterLen))
                {
                    filters.internalAttrsMask |= iettSUBATTR_PERSISTENT;
                    filters.internalAttrsValue &= ~iettSUBATTR_PERSISTENT;
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_SUBTYPE_SHARED) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_SUBTYPE_SHARED,
                                      filterLen))
                {
                    filters.subOptionsMask |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                    filters.subOptionsValue |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_SUBTYPE_NONSHARED) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_SUBTYPE_NONSHARED,
                                      filterLen))
                {
                    filters.subOptionsMask |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                    filters.subOptionsValue &= ~ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_SUBTYPE_ADMIN) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_SUBTYPE_ADMIN,
                                      filterLen))
                {
                    filters.subOptionsMask |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                    filters.subOptionsValue |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
                    filters.anonymousSharersMask |= iettANONYMOUS_SHARER_ADMIN;
                    filters.anonymousSharersValue |= iettANONYMOUS_SHARER_ADMIN;
                }
                else
                {
                    ieutTRACEL(pThreadData, tempFilter, ENGINE_ERROR_TRACE, "Invalid SubType Filter '%s'\n", tempFilter);
                    rc = ISMRC_InvalidParameter;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                filterPos = endPos;
            }

            applyFilters = true;
        }
    }

    // Determine which comparison function(s) to use for this request
    iemnCompSubscriptionMonitorFunction_t comparisonFunction;

    switch(type)
    {
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
            comparisonFunction = iemn_highestSubscriptionMonitorBufferedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS:
            comparisonFunction = iemn_lowestSubscriptionMonitorBufferedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT:
            comparisonFunction = iemn_highestSubscriptionMonitorBufferedPercent;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT:
            comparisonFunction = iemn_lowestSubscriptionMonitorBufferedPercent;
            break;
        case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
            comparisonFunction = iemn_highestSubscriptionMonitorPublishedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS:
            comparisonFunction = iemn_lowestSubscriptionMonitorPublishedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
            comparisonFunction = iemn_highestSubscriptionMonitorRejectedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_REJECTEDMSGS:
            comparisonFunction = iemn_lowestSubscriptionMonitorRejectedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT:
            comparisonFunction = iemn_highestSubscriptionMonitorBufferedHWMPercent;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT:
            comparisonFunction = iemn_lowestSubscriptionMonitorBufferedHWMPercent;
            break;
        case ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS:
            comparisonFunction = iemn_highestSubscriptionMonitorExpiredMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_EXPIREDMSGS:
            comparisonFunction = iemn_lowestSubscriptionMonitorExpiredMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
            comparisonFunction = iemn_highestSubscriptionMonitorDiscardedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS:
            comparisonFunction = iemn_lowestSubscriptionMonitorDiscardedMsgs;
            break;
        case ismENGINE_MONITOR_ALL_UNSORTED:
            comparisonFunction = NULL;
            break;
        default:
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
    }

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Lock the topic tree for read access
    ismEngine_getRWLockForRead(&tree->subsLock);

    // Start with the first subscription
    ismEngine_Subscription_t *curSubscription = tree->subscriptionHead;
    ismEngine_Subscription_t *heldSubscription = NULL;

    // There are no subscriptions unlock the tree and return
    if (curSubscription == NULL)
    {
        ismEngine_unlockRWLock(&tree->subsLock);
        goto mod_exit;
    }

    // There are subscriptions, go through them
    uint32_t localResultCountAtLastBatch = 0;
    while(curSubscription != NULL)
    {
        iepiPolicyInfo_t *curPolicyInfo = ieq_getPolicyInfo(curSubscription->queueHandle);

        // Include only subscriptions not flagged as logically deleted
        if ((curSubscription->internalAttrs & iettSUBATTR_DELETED) == 0)
        {
            ieq_getStats(pThreadData, curSubscription->queueHandle, &localSubscriptionMonitor.stats);

            if (type == ismENGINE_MONITOR_ALL_UNSORTED)
            {
                if (!applyFilters || iemn_matchSubscriptionFilters(curSubscription, curPolicyInfo, &filters))
                {
                    if (localResultCount == maxResults)
                    {
                        uint32_t newMaxResults = maxResults + 1000;
                        ismEngine_SubscriptionMonitor_t *newLocalResults;

                        newLocalResults = iemem_realloc(pThreadData,
                                                        IEMEM_PROBE(iemem_monitoringData, 17), localResults,
                                                        (newMaxResults+1) * sizeof(ismEngine_SubscriptionMonitor_t));

                        if (newLocalResults == NULL)
                        {
                            ismEngine_unlockRWLock(&tree->subsLock);
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            goto mod_exit;
                        }

                        // Make sure new entries are empty
                        memset(&newLocalResults[localResultCount],
                               0,
                               ((newMaxResults+1)-maxResults) * sizeof(ismEngine_SubscriptionMonitor_t));

                        localResults = newLocalResults;
                        maxResults = newMaxResults;
                    }

                    localSubscriptionMonitor.topicString = NULL;
                    localSubscriptionMonitor.subscription = curSubscription;
                    localSubscriptionMonitor.policyInfo = curPolicyInfo;

                    localResults[localResultCount++] = localSubscriptionMonitor;

                    assert(localResults[localResultCount].subscription == NULL); // Check the sentinel
                }
            }
            else
            {
                // Initially, fill the return array with the first entries found, in order.
                if (localResultCount < maxResults)
                {
                    if (!applyFilters || iemn_matchSubscriptionFilters(curSubscription, curPolicyInfo, &filters))
                    {
                        for(pos=0; pos<localResultCount; pos++)
                        {
                            if (comparisonFunction(&localResults[pos], &localSubscriptionMonitor) > 0)
                            {
                                memmove(&localResults[pos+1],
                                        &localResults[pos],
                                        (localResultCount-pos)*sizeof(ismEngine_SubscriptionMonitor_t));
                                break;
                            }
                        }

                        localResultCount++;

                        localSubscriptionMonitor.topicString = NULL;
                        localSubscriptionMonitor.subscription = curSubscription;
                        localSubscriptionMonitor.policyInfo = curPolicyInfo;

                        localResults[pos] = localSubscriptionMonitor;

                        assert(localResults[localResultCount].subscription == NULL); // Check the sentinel
                    }
                }
                // Once we have an initial set, we are only interested in adding values
                // that are an improvement on the current set.
                else if ((comparisonFunction(&localSubscriptionMonitor, &localResults[0]) > 0) &&
                        (!applyFilters || iemn_matchSubscriptionFilters(curSubscription, curPolicyInfo, &filters)))
                {
                    int32_t start=0;
                    int32_t end=maxResults;

                    // The set is sorted, so we use a binary split search
                    while(start != end)
                    {
                        int32_t mid = start+((end-start)/2);
                        int32_t result = comparisonFunction(&localResults[mid], &localSubscriptionMonitor);

                        if (result == 0)
                        {
                            end = start = mid;
                        }
                        else if (result > 0)
                        {
                            end = mid;
                        }
                        else
                        {
                            start = mid + 1;
                        }
                    }

                    // We are about to remove an entry from the list, free any memory
                    // that we allocated last time we released and retook the lock for
                    // this subscriber
                    if (localResults[0].topicString != NULL) iemem_free(pThreadData, iemem_monitoringData, localResults[0].topicString);

                    pos = start-1;

                    memmove(&localResults[0], &localResults[1], pos*sizeof(ismEngine_SubscriptionMonitor_t));

                    localSubscriptionMonitor.topicString = NULL;
                    localSubscriptionMonitor.subscription = curSubscription;
                    localSubscriptionMonitor.policyInfo = curPolicyInfo;

                    localResults[pos] = localSubscriptionMonitor;

                    assert(localResults[localResultCount].subscription == NULL); // Check the sentinel
                }
            }
        }

        ismEngine_Subscription_t *nextSubscription = curSubscription->next;

        // If we have checked a 'batch' subscriptions we drop and retake the subscription lock.
        // If this is the case, or there are no more subscriptions, we have to gather
        // information about the current results.
        if ((++checkedCount % ismENGINE_MONITOR_SUBSCRIPTION_BATCHSIZE) == 0 || nextSubscription == NULL)
        {
            // Go through the new results, grabbing strings where necessary
            ismEngine_SubscriptionMonitor_t *curSubMonitor;

            if (type == ismENGINE_MONITOR_ALL_UNSORTED)
            {
                // If we have more results since the last batch, we only need to update the
                // set we added
                if (localResultCount != localResultCountAtLastBatch)
                {
                    curSubMonitor = &localResults[localResultCountAtLastBatch];
                    localResultCountAtLastBatch = localResultCount;
                }
                // Otherwise, force the while loop to end immediately
                else
                {
                    localSubscriptionMonitor.subscription = NULL;
                    curSubMonitor = &localSubscriptionMonitor;
                }
            }
            else
            {
                // Any of the entries could have changed
                curSubMonitor = &localResults[0];

            }

            while(NULL != curSubMonitor->subscription)
            {
                ismEngine_Subscription_t *updateSubscription = curSubMonitor->subscription;
                iepiPolicyInfo_t *updatePolicyInfo = curSubMonitor->policyInfo;

                // All of the strings associated with this subscription are allocated in a single
                // block off of topicString - if we don't have one, this is a new subscription added
                // this batch - so we need to pick up it's subscription attributes / information now
                // since after we release the lock the subscription may get destroyed.
                //
                // We keep the subscription pointer as the indication that this slot in the results
                // is in use (and for debug information), but once the lock is released we cannot
                // refer to it.
                if (NULL == curSubMonitor->topicString)
                {
                    size_t clientIdLength, subNameLength, topicStringLength, policyNameLength;
                    char *messagingPolicyName = updatePolicyInfo->name;

                    topicStringLength= strlen(updateSubscription->node->topicString);
                    clientIdLength = updateSubscription->clientId ? strlen(updateSubscription->clientId) : 0;
                    subNameLength = updateSubscription->subName ? strlen(updateSubscription->subName) : 0;
                    policyNameLength = messagingPolicyName ? strlen(messagingPolicyName) : 0;

                    char *stringPos = iemem_malloc(pThreadData,
                                                   IEMEM_PROBE(iemem_monitoringData, 2),
                                                   topicStringLength+1+
                                                   clientIdLength+1+
                                                   subNameLength+1+
                                                   policyNameLength+1);

                    if (stringPos == NULL)
                    {
                        ismEngine_unlockRWLock(&tree->subsLock);
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    topicStringLength++; // include null
                    curSubMonitor->topicString = stringPos;
                    memcpy(curSubMonitor->topicString,
                           updateSubscription->node->topicString,
                           topicStringLength);
                    stringPos += topicStringLength;

                    if (clientIdLength != 0)
                    {
                        clientIdLength++; // include null
                        curSubMonitor->clientId = stringPos;
                        memcpy(curSubMonitor->clientId,
                               updateSubscription->clientId,
                               clientIdLength);
                        stringPos += clientIdLength;
                    }
                    else
                    {
                        curSubMonitor->clientId = NULL;
                    }

                    if (subNameLength != 0)
                    {
                        subNameLength++; // include null
                        curSubMonitor->subName = stringPos;
                        memcpy(curSubMonitor->subName,
                               updateSubscription->subName,
                               subNameLength);
                        stringPos += subNameLength;
                    }
                    else
                    {
                        curSubMonitor->subName = NULL;
                    }

                    if (policyNameLength != 0)
                    {
                        policyNameLength++; // include null
                        curSubMonitor->messagingPolicyName = stringPos;
                        memcpy(curSubMonitor->messagingPolicyName,
                               messagingPolicyName,
                               policyNameLength);
                        stringPos += policyNameLength;
                    }
                    else
                    {
                        curSubMonitor->messagingPolicyName = NULL;
                    }

                    curSubMonitor->policyType = (uint32_t)(updatePolicyInfo->policyType);
                    curSubMonitor->internalAttrs = updateSubscription->internalAttrs;
                    curSubMonitor->consumers = updateSubscription->consumerCount;
                    curSubMonitor->durable = !!(updateSubscription->internalAttrs & iettSUBATTR_PERSISTENT);
                    curSubMonitor->shared = !!(updateSubscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED);
                    curSubMonitor->resourceSetId = iere_getResourceSetIdentifier(updateSubscription->resourceSet);
                    curSubMonitor->configType = iett_getAdminSubscriptionType(updateSubscription);
                }

                curSubMonitor++;
            }

            // We need to make sure the subscription we are about to move to does not disappear
            if (nextSubscription != NULL)
            {
                iett_acquireSubscriptionReference(nextSubscription);
            }

            // Unlock the topic tree
            ismEngine_unlockRWLock(&tree->subsLock);

            // We previously held a subscription, so now release it
            if (heldSubscription != NULL)
            {
                (void)iett_releaseSubscription(pThreadData, heldSubscription, false);
            }

            // If we have a nextSubscription it is now the held one, else held should be NULL
            heldSubscription = nextSubscription;

            // We have more subscriptions to analyse, so get the lock back
            if (nextSubscription != NULL)
            {
                ismEngine_getRWLockForRead(&tree->subsLock);
            }
        }

        curSubscription = nextSubscription;
    }

    assert(heldSubscription == NULL);

mod_exit:

    if (filters.regexClientId != NULL) ism_regex_free(filters.regexClientId);

    // All done, update the caller's results
    if (rc == OK && localResultCount != 0)
    {
        if (type != ismENGINE_MONITOR_ALL_UNSORTED)
        {
            uint32_t swap_pos = localResultCount-1;

            // The results are in the counter-intuitive order for the request, so reverse them.
            for(pos=0; pos<swap_pos; pos++, swap_pos--)
            {
                localSubscriptionMonitor = localResults[pos];
                localResults[pos] = localResults[swap_pos];
                localResults[swap_pos] = localSubscriptionMonitor;
            }
        }

        *resultCount = localResultCount;
        *results = localResults;
    }
    else
    {
        ism_engine_freeSubscriptionMonitor(localResults);

        *resultCount = 0;
        *results = NULL;
    }

    ieutTRACEL(pThreadData, *results, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, checkedCount=%u, resultCount=%u, results=%p\n", __func__,
               rc, checkedCount, *resultCount, *results);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Free a subscription monitor list
///
/// @param[in]     list              The subscription monitor list to free
//****************************************************************************
XAPI void ism_engine_freeSubscriptionMonitor(ismEngine_SubscriptionMonitor_t *list)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, list,  ENGINE_CEI_TRACE, FUNCTION_IDENT "list=%p\n", __func__, list);

    // If the list is NULL, nothing to do.
    if (list != NULL)
    {
        uint32_t loop = 0;

        ismEngine_SubscriptionMonitor_t *curSubMonitor = &list[loop++];

        // Go though the entries until we hit one with a NULL subscription
        // NOTE: The subscription pointer should not be dereferenced as the subscription
        //       to which it originally pointed may no longer be available.
        while(NULL != curSubMonitor->subscription)
        {
            if (curSubMonitor->topicString != NULL) iemem_free(pThreadData, iemem_monitoringData, curSubMonitor->topicString);
            curSubMonitor = &list[loop++];
        }

        iemem_free(pThreadData, iemem_monitoringData, list);
    }

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Start topic monitoring on the subtree below a specified topic
///         string
///
/// @param[in]     topicString         The topic string which ends in a multi-level
///                                    wildcard but contains no other wildcards.
/// @param[in]     resetActiveMonitor  Whether to reset the stats for an already
///                                    active topic monitor.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_startTopicMonitor(
                                          const char *topicString,
                                          bool resetActiveMonitor)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, topicString,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    if (!iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_TOPICMONITOR))
    {
        rc = ISMRC_InvalidTopicMonitor;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Activate stats on the subtree node
    rc = iett_activateSubsNodeStats(pThreadData, topicString, resetActiveMonitor);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Stop topic monitoring on the subtree below a specified topic
///         string
///
/// @param[in]     topicString  The topic string which ends in a multi-level
///                             wildcard but contains no other wildcards.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_stopTopicMonitor( const char *topicString )
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, topicString,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    if (!iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_TOPICMONITOR))
    {
        rc = ISMRC_InvalidTopicMonitor;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = iett_deactivateSubsNodeStats(pThreadData, topicString);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for subscriptions
///
/// @param[out]    results           The returned list of subscription monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark If result type is ismENGINE_MONITOR_ALL_UNSORTED then maxResults is
///         ignored and all of the active topic monitor results are returned in
///         an unsorted list.
///
/// @remark ism_engine_freeTopicMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getTopicMonitor(
                                         ismEngine_TopicMonitor_t **results
                                       , uint32_t *resultCount
                                       , ismEngineMonitorType_t type
                                       , uint32_t maxResults
                                       , ism_prop_t *filterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u filterProperties=%p\n", __func__,
               type, maxResults, filterProperties);

    assert(results != NULL);
    assert(resultCount != NULL);

    uint32_t localResultCount = 0;
    ismEngine_TopicMonitor_t *localResults = NULL;
    uint32_t pos;
    ismEngine_TopicMonitor_t localTopicMonitor;

    // Deal with filtering of results
    iemnTopicFilters_t filters = iemnTOPIC_FILTERS_DEFAULT;

    bool applyFilters = false;

    // Some filter properties were specified, analyse these now
    if (NULL != filterProperties)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on topicString
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_TOPICSTRING);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.topicString = tempFilter;
            applyFilters = true;
        }
    }

    iettSubsNodeStats_t *curSubsNodeStats;
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // If request is for all results, unsorted we ignore maxResults and just return
    // all of the values on active nodes
    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        ismEngine_TopicMonitor_t * newLocalResults;

        // We ignore whatever came in for maxResults
        maxResults = 0;

        // Lock the subscription tree for read if there are any active nodes
        if (tree->activeSubNodeStats != 0)
        {
            ismEngine_getRWLockForRead(&tree->subsLock);
        }
        else
        {
            goto mod_exit_no_release;
        }

        // Start with the first subscription
        curSubsNodeStats = tree->subNodeStatsHead;

        while(curSubsNodeStats != NULL)
        {
            if ((curSubsNodeStats->topicStats.ResetTime != 0) &&
                (!applyFilters || iemn_matchTopicFilters(curSubsNodeStats->node, &filters)))
            {
                if (localResultCount == maxResults)
                {
                    // Re-allocate for 100 more results
                    uint32_t newMaxResults = maxResults + 100;

                    newLocalResults = iemem_realloc(pThreadData,
                                                    IEMEM_PROBE(iemem_monitoringData, 3),
                                                    localResults,
                                                    (newMaxResults+1) * sizeof(ismEngine_TopicMonitor_t));

                    if (NULL == newLocalResults)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    localResults = newLocalResults;
                    maxResults = newMaxResults;
                }

                localResults[localResultCount].stats = curSubsNodeStats->topicStats;
                localResults[localResultCount].subscriptions = curSubsNodeStats->node->parent->totalSubsCount;
                localResults[localResultCount].topicString = NULL;
                localResults[localResultCount].node = curSubsNodeStats->node;

                localResultCount++;
            }

            curSubsNodeStats = curSubsNodeStats->next;
        }

        // Make sure the list terminated with an entry point to a NULL node
        if (localResults != NULL) localResults[localResultCount].node = NULL;
    }
    // For any other request type, we care about maxResults and perform selection
    else
    {
        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit_no_release;
        }

        // Determine which comparison function(s) to use for this request
        iemnCompTopicMonitorFunction_t comparisonFunction;

        switch(type)
        {
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
                comparisonFunction = iemn_highestTopicMonitorPublishedMsgs;
                break;
            case ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS:
                comparisonFunction = iemn_lowestTopicMonitorPublishedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS:
                comparisonFunction = iemn_highestTopicMonitorSubscriptions;
                break;
            case ismENGINE_MONITOR_LOWEST_SUBSCRIPTIONS:
                comparisonFunction = iemn_lowestTopicMonitorSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
                comparisonFunction = iemn_highestTopicMonitorRejectedMsgs;
                break;
            case ismENGINE_MONITOR_LOWEST_REJECTEDMSGS:
                comparisonFunction = iemn_lowestTopicMonitorRejectedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES:
                comparisonFunction = iemn_highestTopicMonitorFailedPublishes;
                break;
            case ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES:
                comparisonFunction = iemn_lowestTopicMonitorFailedPublishes;
                break;
            default:
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit_no_release;
        }

        // Allocate space for results and lock the subscription tree for read if
        // there are any active nodes
        if (tree->activeSubNodeStats != 0)
        {
            localResults = iemem_calloc(pThreadData,
                                        IEMEM_PROBE(iemem_monitoringData, 5), 1,
                                        (maxResults+1) * sizeof(ismEngine_TopicMonitor_t));

            if (NULL == localResults)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit_no_release;
            }

            ismEngine_getRWLockForRead(&tree->subsLock);
        }
        else
        {
            goto mod_exit_no_release;
        }

        // Start with the first subscription
        curSubsNodeStats = tree->subNodeStatsHead;

        // Go through the subscriptions
        while(curSubsNodeStats != NULL)
        {
            // We are only interested in stats that are actively being gathered
            if (curSubsNodeStats->topicStats.ResetTime != 0)
            {
                localTopicMonitor.stats = curSubsNodeStats->topicStats;
                localTopicMonitor.subscriptions = curSubsNodeStats->node->parent->totalSubsCount;

                // Initially, fill the return array with the first entries found, in order.
                if (localResultCount < maxResults)
                {
                    if (!applyFilters || iemn_matchTopicFilters(curSubsNodeStats->node, &filters))
                    {
                        for(pos=0; pos<localResultCount; pos++)
                        {
                            if (comparisonFunction(&localResults[pos], &localTopicMonitor) > 0)
                            {
                                memmove(&localResults[pos+1],
                                        &localResults[pos],
                                        (localResultCount-pos)*sizeof(ismEngine_TopicMonitor_t));
                                break;
                            }
                        }

                        localResultCount++;

                        localTopicMonitor.topicString = NULL;
                        localTopicMonitor.node = curSubsNodeStats->node;

                        localResults[pos] = localTopicMonitor;

                        assert(localResults[localResultCount].node == NULL); // Check the sentinel
                    }
                }
                // Once we have an initial set, we are only interested in adding values
                // that are an improvement on the current set.
                else if ((comparisonFunction(&localTopicMonitor, &localResults[0]) > 0) &&
                         (!applyFilters || iemn_matchTopicFilters(curSubsNodeStats->node, &filters)))
                {
                    int32_t start=0;
                    int32_t end=maxResults;

                    // The set is sorted, so we use a binary split search
                    while(start != end)
                    {
                        int32_t mid = start+((end-start)/2);
                        int32_t result = comparisonFunction(&localResults[mid], &localTopicMonitor);

                        if (result == 0)
                        {
                            end = start = mid;
                        }
                        else if (result > 0)
                        {
                            end = mid;
                        }
                        else
                        {
                            start = mid + 1;
                        }
                    }

                    pos = start-1;

                    memmove(&localResults[0], &localResults[1], pos*sizeof(ismEngine_TopicMonitor_t));

                    localTopicMonitor.topicString = NULL;
                    localTopicMonitor.node = curSubsNodeStats->node;

                    localResults[pos] = localTopicMonitor;

                    assert(localResults[localResultCount].node == NULL); // Check the sentinel
                }
            }

            curSubsNodeStats = curSubsNodeStats->next;
        }
    }

    if (localResults != NULL)
    {
        // Go through the results grabbing strings where necessary
        ismEngine_TopicMonitor_t *curTopicMonitor = &localResults[0];

        while(NULL != curTopicMonitor->node)
        {
            const iettSubsNode_t *updateSubsNode = curTopicMonitor->node;

            assert(updateSubsNode != NULL);
            assert((updateSubsNode->nodeFlags & iettNODE_FLAG_TYPE_MASK) == iettNODE_FLAG_MULTICARD);

            size_t topicStringLength = strlen(updateSubsNode->topicString) + 1;

            curTopicMonitor->topicString = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_monitoringData, 6), topicStringLength);

            if (curTopicMonitor->topicString == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                break;
            }

            memcpy(curTopicMonitor->topicString, updateSubsNode->topicString, topicStringLength);

            curTopicMonitor++;
        }
    }

mod_exit:

    // Unlock the topic tree
    ismEngine_unlockRWLock(&tree->subsLock);

mod_exit_no_release:

    // All done, update the caller's results
    if (rc == OK && localResultCount != 0)
    {
        // If this is a sorted request, the results are in the opposite
        // order for the request, so reverse them.
        if (type != ismENGINE_MONITOR_ALL_UNSORTED)
        {
            uint32_t swap_pos = localResultCount-1;

            for(pos=0; pos<swap_pos; pos++, swap_pos--)
            {
                localTopicMonitor = localResults[pos];
                localResults[pos] = localResults[swap_pos];
                localResults[swap_pos] = localTopicMonitor;
            }
        }

        *resultCount = localResultCount;
        *results = localResults;
    }
    else
    {
        ism_engine_freeTopicMonitor(localResults);

        *resultCount = 0;
        *results = NULL;
    }

    ieutTRACEL(pThreadData, *results, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, results=%p\n", __func__,
               rc, *resultCount, *results);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Free a topic monitor list
///
/// @param[in]     list  The topic monitor list to free
//****************************************************************************
XAPI void ism_engine_freeTopicMonitor(ismEngine_TopicMonitor_t *list)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, list,  ENGINE_CEI_TRACE, FUNCTION_IDENT "list=%p\n", __func__, list);

    // If the list is NULL, nothing to do.
    if (list != NULL)
    {
        uint32_t loop = 0;

        ismEngine_TopicMonitor_t *curTopicMonitor = &list[loop++];

        // Go though the entries until we hit one with a NULL subsNode
        // NOTE: The subsNodeStats pointer should not be dereferenced as it may
        //       have been deallocated
        while(NULL != curTopicMonitor->node)
        {
            if (curTopicMonitor->topicString != NULL) iemem_free(pThreadData, iemem_monitoringData, curTopicMonitor->topicString);
            curTopicMonitor = &list[loop++];
        }

        iemem_free(pThreadData, iemem_monitoringData, list);
    }

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Match the specified queue with the specified mask
///
/// @param[in]     queue             The queue to check
/// @param[in]     filters           The filters to apply to the queue
///
/// @return true if the queue matches, otherwise false.
//****************************************************************************
static bool iemn_matchQueueFilters(ieqnQueue_t *queue,
                                   iemnQueueFilters_t *filters)
{
    bool match = false;

    // Check the queue scope
    if (filters->QueueScope == ismQueueScope_Client)
    {
        if (queue->temporary == false)
            goto mod_exit;
    }
    else if (filters->QueueScope == ismQueueScope_Server)
    {
        if (queue->temporary == true)
            goto mod_exit;
    }

    // Check the queue name
    if (NULL != filters->queueName)
    {
        if (ism_common_match(queue->queueName, filters->queueName) == 0)
            goto mod_exit;
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}

//****************************************************************************
/// @brief  Callback for a queue in the queue table
///
/// @param[in]     key               Queue name
/// @param[in]     keyHash           Hash of the queue name
/// @param[in]     value             ieqnQueue_t of the named queue
/// @param[in]     context           iemnGetQueueMonitorContext_t being used
//****************************************************************************
static void iemn_getQueueMonitor(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    uint32_t pos;
    ismEngine_QueueMonitor_t localQueueMonitor;
    iemnGetQueueMonitorContext_t *pContext = (iemnGetQueueMonitorContext_t *)context;
    ieqnQueue_t *namedQueue = (ieqnQueue_t *)value;

    // If we had an earlier failure skip this one.
    if (pContext->rc != OK) goto mod_exit;

    // Get Statistics for this queue
    ieq_getStats(pThreadData, namedQueue->queueHandle, &localQueueMonitor.stats);
    localQueueMonitor.producers = namedQueue->producerCount;
    localQueueMonitor.consumers = namedQueue->consumerCount;

    if (pContext->type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        if (!pContext->applyFilters || iemn_matchQueueFilters(namedQueue, pContext->pFilters))
        {
            if (pContext->localResultCount == pContext->maxResults)
            {
                uint32_t newMaxResults = pContext->maxResults + 1000;
                ismEngine_QueueMonitor_t *newLocalResults;

                newLocalResults = iemem_realloc(pThreadData,
                                                IEMEM_PROBE(iemem_monitoringData, 18), pContext->localResults,
                                                (newMaxResults+1) * sizeof(ismEngine_QueueMonitor_t));

                if (newLocalResults == NULL)
                {
                    pContext->rc = ISMRC_AllocateError;
                    ism_common_setError(pContext->rc);
                    goto mod_exit;
                }

                // Make sure new entries are empty
                memset(&newLocalResults[pContext->localResultCount],
                       0,
                       ((newMaxResults+1)-pContext->maxResults) * sizeof(ismEngine_QueueMonitor_t));

                pContext->localResults = newLocalResults;
                pContext->maxResults = newMaxResults;
            }

            localQueueMonitor.queue = namedQueue;
            localQueueMonitor.queueName = NULL;
            localQueueMonitor.scope = (namedQueue->temporary) ? ismQueueScope_Client : ismQueueScope_Server;

            pContext->localResults[pContext->localResultCount++] = localQueueMonitor;
        }
    }
    else
    {
        // Initially, fill the return array with the first entries found, in order.
        if (pContext->localResultCount < pContext->maxResults)
        {
            if (!pContext->applyFilters || iemn_matchQueueFilters(namedQueue, pContext->pFilters))
            {
                for(pos=0; pos<pContext->localResultCount; pos++)
                {
                    if (pContext->comparisonFunction(&pContext->localResults[pos], &localQueueMonitor) > 0)
                    {
                        memmove(&pContext->localResults[pos+1],
                                &pContext->localResults[pos],
                                (pContext->localResultCount-pos)*sizeof(ismEngine_QueueMonitor_t));
                        break;
                    }
                }

                localQueueMonitor.queue = namedQueue;
                localQueueMonitor.queueName = NULL;
                localQueueMonitor.scope = (namedQueue->temporary) ? ismQueueScope_Client : ismQueueScope_Server;

                pContext->localResults[pos] = localQueueMonitor;
                pContext->localResultCount++;
            }
        }
        // Once we have an initial set, we are only interested in adding values
        // that are an improvement on the current set.
        else if ((pContext->comparisonFunction(&localQueueMonitor, &pContext->localResults[0]) > 0) &&
                 (!pContext->applyFilters || iemn_matchQueueFilters(namedQueue, pContext->pFilters)))
        {
            int32_t start=0;
            int32_t end=pContext->maxResults;

            // The set is sorted, so we use a binary split search
            while(start != end)
            {
                int32_t mid = start+((end-start)/2);
                int32_t result = pContext->comparisonFunction(&pContext->localResults[mid], &localQueueMonitor);

                if (result == 0)
                {
                    end = start = mid;
                }
                else if (result > 0)
                {
                    end = mid;
                }
                else
                {
                    start = mid + 1;
                }
            }

            assert(pContext->localResults[0].queueName == NULL);

            pos = start-1;

            memmove(&pContext->localResults[0], &pContext->localResults[1], pos*sizeof(ismEngine_QueueMonitor_t));

            localQueueMonitor.queue = namedQueue;
            localQueueMonitor.queueName = NULL;
            localQueueMonitor.scope = (namedQueue->temporary) ? ismQueueScope_Client : ismQueueScope_Server;

            pContext->localResults[pos] = localQueueMonitor;
        }
    }

mod_exit:

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for queues
///
/// @param[out]    results           The returned list of queue monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeQueueMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getQueueMonitor(
                                         ismEngine_QueueMonitor_t **results
                                       , uint32_t *resultCount
                                       , ismEngineMonitorType_t type
                                       , uint32_t maxResults
                                       , ism_prop_t *filterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    iemnGetQueueMonitorContext_t qmonContext = { 0 };

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u filterProperties=%p\n", __func__,
               type, maxResults, filterProperties);

    assert(results != NULL);
    assert(resultCount != NULL);

    // Deal with filtering of results
    iemnQueueFilters_t filters = iemnQUEUE_FILTERS_DEFAULT;
    qmonContext.pFilters = &filters;
    qmonContext.applyFilters = false;
    qmonContext.maxResults = maxResults;
    qmonContext.localResultCount = 0;
    qmonContext.localResults = NULL;
    qmonContext.type = type;

    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        qmonContext.maxResults = 0;
    }
    else
    {
        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        qmonContext.localResults = iemem_calloc(pThreadData,
                                                IEMEM_PROBE(iemem_monitoringData, 7), 1,
                                                (maxResults+1) * sizeof(ismEngine_QueueMonitor_t));

        if (NULL == qmonContext.localResults)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // Some filter properties were specified, analyse these now
    if (NULL != filterProperties)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on subscription name
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_QUEUENAME);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.queueName = tempFilter;
            qmonContext.applyFilters = true;
        }

        // Check for a non-trivial filter on queue type - never actually exposed so we can change in the future
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_SCOPE);
        if (NULL != tempFilter && strncasecmp(tempFilter, ismENGINE_MONITOR_FILTER_SCOPE_ALL, 3))
        {
            if (!strncasecmp(tempFilter, ismENGINE_MONITOR_FILTER_SCOPE_SERVER, 9))
            {
                filters.QueueScope = ismQueueScope_Server;
            }
            else if (!strncasecmp(tempFilter, ismENGINE_MONITOR_FILTER_SCOPE_CLIENT, 9))
            {
                filters.QueueScope = ismQueueScope_Client;
            }
            else
            {
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit;
            }

            qmonContext.applyFilters = true;
        }
    }

    switch(type)
    {
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorBufferedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorBufferedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorBufferedPercent;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorBufferedPercent;
            break;
        case ismENGINE_MONITOR_HIGHEST_PRODUCERS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorProducers;
            break;
        case ismENGINE_MONITOR_LOWEST_PRODUCERS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorProducers;
            break;
        case ismENGINE_MONITOR_HIGHEST_CONSUMERS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorConsumers;
            break;
        case ismENGINE_MONITOR_LOWEST_CONSUMERS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorConsumers;
            break;
        case ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorConsumedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorConsumedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorProducedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorProducedMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorBufferedHWMPercent;
            break;
        case ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorBufferedHWMPercent;
            break;
        case ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorExpiredMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_EXPIREDMSGS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorExpiredMsgs;
            break;
        case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
            qmonContext.comparisonFunction = iemn_highestQueueMonitorDiscardedMsgs;
            break;
        case ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS:
            qmonContext.comparisonFunction = iemn_lowestQueueMonitorDiscardedMsgs;
            break;
        case ismENGINE_MONITOR_ALL_UNSORTED:
            qmonContext.comparisonFunction = NULL;
            break;
        default:
            assert(false);
            break;
    }


    // Lock the queue namespace for read
    ismEngine_getRWLockForRead(&ismEngine_serverGlobal.queues->namesLock);

    // Go through the queues
    ieut_traverseHashTable(pThreadData,
                           ismEngine_serverGlobal.queues->names,
                           &iemn_getQueueMonitor,
                           &qmonContext);

    rc = qmonContext.rc;

    if (rc == OK && qmonContext.localResultCount > 0)
    {
        // Before releasing the lock, make a copy of the queue names which we plan to return
        ismEngine_QueueMonitor_t *curQueueMonitor = &qmonContext.localResults[0];

        while(NULL != curQueueMonitor->queue)
        {
            const ieqnQueue_t *updateQueue = curQueueMonitor->queue;

            curQueueMonitor->queueName = iemem_malloc(pThreadData,
                                                      IEMEM_PROBE(iemem_monitoringData, 8),
                                                      strlen(updateQueue->queueName)+1);

            if (curQueueMonitor->queueName == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                break;
            }

            strcpy(curQueueMonitor->queueName, updateQueue->queueName);

            curQueueMonitor->scope = updateQueue->temporary ? ismQueueScope_Client : ismQueueScope_Server;

            curQueueMonitor++;
        }
    }

    // Unlock the queue namespace
    ismEngine_unlockRWLock(&ismEngine_serverGlobal.queues->namesLock);

    rc = qmonContext.rc;

mod_exit:

    // All done, update the caller's results
    if (rc == OK && qmonContext.localResultCount != 0)
    {
        if (type != ismENGINE_MONITOR_ALL_UNSORTED)
        {
            uint32_t pos=0, swap_pos=qmonContext.localResultCount-1;

            // The results are in the counter-intuitive order for the request, so reverse them.
            for(; pos<swap_pos; pos++, swap_pos--)
            {
                ismEngine_QueueMonitor_t localQueueMonitor = qmonContext.localResults[pos];
                qmonContext.localResults[pos] = qmonContext.localResults[swap_pos];
                qmonContext.localResults[swap_pos] = localQueueMonitor;
            }
        }

        *resultCount = qmonContext.localResultCount;
        *results = qmonContext.localResults;
    }
    else
    {
        ism_engine_freeQueueMonitor(qmonContext.localResults);

        *resultCount = 0;
        *results = NULL;
    }

    ieutTRACEL(pThreadData, *results, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, results=%p\n", __func__,
               rc, *resultCount, *results);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @brief  Free a queue monitor list
///
/// @param[in]     list              The queue monitor list to free
//****************************************************************************
XAPI void ism_engine_freeQueueMonitor(ismEngine_QueueMonitor_t *list)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, list,  ENGINE_CEI_TRACE, FUNCTION_IDENT "list=%p\n", __func__, list);

    // If the list is NULL, nothing to do.
    if (list)
    {
        uint32_t loop = 0;

        ismEngine_QueueMonitor_t *curQueueMonitor = &list[loop++];

        // Go though the entries until we hit one with a NULL queue
        // NOTE: The queue pointer should not be dereferenced as the
        //       queue to which it originally pointed may no longer be available
        while(NULL != curQueueMonitor->queue)
        {
            if (curQueueMonitor->queueName != NULL) iemem_free(pThreadData, iemem_monitoringData, curQueueMonitor->queueName);
            curQueueMonitor = &list[loop++];
        }

        iemem_free(pThreadData, iemem_monitoringData, list);
    }

    ieut_leavingEngine(pThreadData);
}


static bool iemn_getClientStateMonitor(ieutThreadData_t *pThreadData,
                                       ismEngine_ClientState_t *pClient,
                                       void *context)
{
    iemnGetClientStateMonitorContext_t *pContext = (iemnGetClientStateMonitorContext_t *)context;

    // We skip clients whose state we're not interested in
    if (pContext->requestedOpState[pClient->OpState] == false)
    {
        goto mod_exit;
    }

    assert(pClient->pClientId != NULL);

    // We need to determine whether this client state is from a protocol we're
    // interested in - we lump all plugin protocols together and return them if
    // PROTOCOL_ID_PLUGIN was requested.
    uint32_t testProtocolId = pClient->protocolId;

    if (testProtocolId > PROTOCOL_ID_PLUGIN) testProtocolId = PROTOCOL_ID_PLUGIN;

    // Ignore clients whose protocols were not requested
    if (pContext->requestedProtocol[testProtocolId] == false)
    {
        goto mod_exit;
    }

    // We consider it connected if it is to the left of being a Zombie...
    bool fIsConnected = (pClient->OpState < iecsOpStateZombie);
    bool fIsDurable = (pClient->Durability == iecsDurable);

    // For all unsorted requests, we want to include all that match the filters
    if (pContext->type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        if (!pContext->applyFilters || iemn_matchClientStateFilters(pClient, pContext->pFilters))
        {
            if (pContext->resultCount == pContext->maxResults)
            {
                uint32_t newMaxResults = pContext->maxResults + 1000;
                ismEngine_ClientStateMonitor_t *pNewResults;

                pNewResults = iemem_realloc(pThreadData,
                                            IEMEM_PROBE(iemem_monitoringData, 13), pContext->pResults,
                                            (newMaxResults+1) * sizeof(ismEngine_ClientStateMonitor_t));

                if (pNewResults == NULL)
                {
                    pContext->rc = ISMRC_AllocateError;
                    goto mod_exit;
                }

                // Make sure the new entries are empty - the free routine relies on a sentinel
                // entry with a NULL clientId pointer.
                memset(&pNewResults[pContext->resultCount],
                       0,
                       ((newMaxResults+1)-pContext->maxResults)*sizeof(ismEngine_ClientStateMonitor_t));

                pContext->pResults = pNewResults;
                pContext->maxResults = newMaxResults;
            }

            char *pClientIdCopy = iemem_malloc(pThreadData,
                                               IEMEM_PROBE(iemem_monitoringData, 14),
                                               strlen(pClient->pClientId)+1);
            if (pClientIdCopy == NULL)
            {
                pContext->rc = ISMRC_AllocateError;
                goto mod_exit;
            }

            uint32_t pos = pContext->resultCount++;

            strcpy(pClientIdCopy, pClient->pClientId);
            pContext->pResults[pos].ClientId = pClientIdCopy;
            pContext->pResults[pos].ProtocolId = pClient->protocolId;
            pContext->pResults[pos].fIsConnected = fIsConnected;
            pContext->pResults[pos].fIsDurable = fIsDurable;
            pContext->pResults[pos].LastConnectedTime = pClient->LastConnectedTime;
            pContext->pResults[pos].ExpiryTime = pClient->ExpiryTime;
            pContext->pResults[pos].WillDelayExpiryTime = pClient->WillDelayExpiryTime;
            pContext->pResults[pos].ResourceSetId = iere_getResourceSetIdentifier(pClient->resourceSet);
        }
    }
    else
    {
        const int32_t limit = pContext->maxResults - 1;

        assert(pContext->maxResults > 0);

        // Initially, fill the return array with the first entries found, in order.
        if (pContext->resultCount < pContext->maxResults)
        {
            if (!pContext->applyFilters || iemn_matchClientStateFilters(pClient, pContext->pFilters))
            {
                // The list is sorted into the desired order of the query.
                // For LastConnectedTimeOldest, the oldest ones are at the front of the array.
                // The variable 'pos' is used for the desired position to insert the result
                uint32_t pos;
                if (pContext->resultCount == 0)
                {
                    pos = 0;
                }
                else
                {
                    for (pos = pContext->resultCount; pos > 0; pos--)
                    {
                        if (pContext->pResults[pos-1].LastConnectedTime <= pClient->LastConnectedTime)
                        {
                            break;
                        }
                    }
                }

                char *pClientIdCopy = iemem_malloc(pThreadData,
                                                   IEMEM_PROBE(iemem_monitoringData, 9),
                                                   strlen(pClient->pClientId)+1);
                if (pClientIdCopy == NULL)
                {
                    pContext->rc = ISMRC_AllocateError;
                    goto mod_exit;
                }

                if (pos != pContext->resultCount)
                {
                    memmove(&pContext->pResults[pos + 1],
                            &pContext->pResults[pos],
                            (pContext->resultCount - pos) * sizeof(ismEngine_ClientStateMonitor_t));
                }

                pContext->resultCount++;

                strcpy(pClientIdCopy, pClient->pClientId);
                pContext->pResults[pos].ClientId = pClientIdCopy;
                pContext->pResults[pos].ProtocolId = pClient->protocolId;
                pContext->pResults[pos].fIsConnected = fIsConnected;
                pContext->pResults[pos].fIsDurable = fIsDurable;
                pContext->pResults[pos].LastConnectedTime = pClient->LastConnectedTime;
                pContext->pResults[pos].ExpiryTime = pClient->ExpiryTime;
                pContext->pResults[pos].WillDelayExpiryTime = pClient->WillDelayExpiryTime;
                pContext->pResults[pos].ResourceSetId = iere_getResourceSetIdentifier(pClient->resourceSet);
            }
        }
        // Once we have an initial set, we are only interested in adding values
        // that are an improvement on the current set.
        else if ((pContext->pResults[limit].LastConnectedTime > pClient->LastConnectedTime) &&
                 (!pContext->applyFilters || iemn_matchClientStateFilters(pClient, pContext->pFilters)))
        {
            // The list is sorted into the desired order of the query.
            // For LastConnectedTimeOldest, the oldest ones are at the front of the array.
            // The variable 'pos' is used for the desired position to insert the result
            uint32_t pos;
            for (pos = limit; pos > 0; pos--)
            {
                if (pContext->pResults[pos-1].LastConnectedTime <= pClient->LastConnectedTime)
                {
                    break;
                }
            }

            char *pClientIdCopy = iemem_malloc(pThreadData,
                                               IEMEM_PROBE(iemem_monitoringData, 10),
                                               strlen(pClient->pClientId)+1);
            if (pClientIdCopy == NULL)
            {
                pContext->rc = ISMRC_AllocateError;
                goto mod_exit;
            }

            // We are about to remove an entry from the list, free any memory associated with it
            if (pContext->pResults[limit].ClientId != NULL)
            {
                iemem_free(pThreadData, iemem_monitoringData, (char *)pContext->pResults[limit].ClientId);
            }

            if (pos != limit)
            {
                memmove(&pContext->pResults[pos + 1],
                        &pContext->pResults[pos],
                        (limit - pos) * sizeof(ismEngine_ClientStateMonitor_t));
            }

            strcpy(pClientIdCopy, pClient->pClientId);
            pContext->pResults[pos].ClientId = pClientIdCopy;
            pContext->pResults[pos].ProtocolId = pClient->protocolId;
            pContext->pResults[pos].fIsConnected = fIsConnected;
            pContext->pResults[pos].fIsDurable = fIsDurable;
            pContext->pResults[pos].LastConnectedTime = pClient->LastConnectedTime;
            pContext->pResults[pos].ExpiryTime = pClient->ExpiryTime;
            pContext->pResults[pos].WillDelayExpiryTime = pClient->WillDelayExpiryTime;
            pContext->pResults[pos].ResourceSetId = iere_getResourceSetIdentifier(pClient->resourceSet);
        }
    }

mod_exit:
    return (pContext->rc == OK) ? true : false;
}

//****************************************************************************
/// @internal
///
/// @brief  Get monitoring data for client-states
///
/// @param[out]    ppMonitor         The returned list of client-state monitors
/// @param[out]    pResultCount      Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     pFilterProperties Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeClientStateMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getClientStateMonitor(
    ismEngine_ClientStateMonitor_t ** ppMonitor,
    uint32_t *                        pResultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ismEngine_ClientStateMonitor_t *pMonitor = NULL;
    uint32_t resultCount = 0;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u pFilterProperties=%p\n", __func__,
               type, maxResults, pFilterProperties);

    assert(ppMonitor != NULL);
    assert(pResultCount != NULL);
    assert(type == ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME || type == ismENGINE_MONITOR_ALL_UNSORTED);

    iemnClientStateFilters_t filters = iemnCLIENT_STATE_FILTERS_DEFAULT;

    // For the 'all unsorted' case we reallocate as needed, we ignore maxResults passed in
    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        maxResults = 0;
    }
    else
    {
        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        pMonitor = iemem_calloc(pThreadData,
                                IEMEM_PROBE(iemem_monitoringData, 11), 1,
                                (maxResults + 1) * sizeof(ismEngine_ClientStateMonitor_t));
        if (pMonitor == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    iemnGetClientStateMonitorContext_t context;
    context.type = type;
    context.rc = OK;
    context.applyFilters = false;
    context.resultsOverflow = false;
    context.pFilters = &filters;
    context.maxResults = maxResults;
    context.resultCount = 0;
    context.pResults = pMonitor;

    memset(context.requestedOpState, false, sizeof(context.requestedOpState));
    context.requestedOpState[iecsOpStateZombie] = true;

    memset(context.requestedProtocol, false, sizeof(context.requestedProtocol));
    context.requestedProtocol[PROTOCOL_ID_MQTT] = true;
    context.requestedProtocol[PROTOCOL_ID_HTTP] = true;

    // Some filter properties were specified, analyse these now
    if (pFilterProperties != NULL)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on string client ID
        tempFilter = ism_common_getStringProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_CLIENTID);
        if ((tempFilter != NULL) && ((strlen(tempFilter) > 1) || (tempFilter[0] != '*')))
        {
            filters.clientId = tempFilter;
            context.applyFilters = true;
        }

        // Check for filter on regular expression client ID
        tempFilter = ism_common_getStringProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_REGEX_CLIENTID);
        if (tempFilter != NULL)
        {
            int regex_rc = ism_regex_compile(&filters.regexClientId, tempFilter);
            if (regex_rc != OK)
            {
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit;
            }
            context.applyFilters = true;
        }

        // Check for non-trivial filter on Resource Set Identifier
        tempFilter = ism_common_getStringProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            if (iere_isResourceSetTrackingEnabled() == true)
            {
                filters.resourceSetIdentifier = tempFilter;
                context.applyFilters = true;
            }
            else
            {
                ieutTRACEL(pThreadData, tempFilter, ENGINE_INTERESTING_TRACE,
                           FUNCTION_IDENT "Ignoring ResourceSetID filter '%s' because resource sets not tracked\n",
                           __func__, tempFilter);
            }
        }

        // Check for selection of specific connection states
        tempFilter = ism_common_getStringProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_CONNECTIONSTATE);
        if (tempFilter != NULL && strcasecmp(tempFilter, ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED))
        {
            memset(context.requestedOpState, false, sizeof(context.requestedOpState));

            const char *filterPos = tempFilter;
            while(filterPos != NULL)
            {
                size_t filterLen;
                char *endPos = strchr(filterPos, ',');

                if (endPos != NULL)
                {
                    filterLen = (size_t)(endPos-filterPos);
                    endPos += 1;
                }
                else
                {
                    filterLen = strlen(filterPos);
                }

                // Skip over leading spaces
                while(*filterPos == ' ')
                {
                    filterPos++;
                    filterLen--;
                }

                // Ignore trailing spaces
                while(filterLen > 0 && filterPos[filterLen-1] == ' ')
                {
                    filterLen--;
                }

                // Check this filter
                if (filterLen == strlen(ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_ALL) &&
                    !strncasecmp(filterPos,
                                 ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_ALL,
                                 filterLen))
                {
                    // Include all but states in the returned set
                    memset(context.requestedOpState, true, sizeof(context.requestedOpState));
                    break; // Don't bother looking for other values, they asked for 'All'
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_CONNECTED) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_CONNECTED,
                                      filterLen))
                {
                    context.requestedOpState[iecsOpStateActive] = true;
                }
                else if (filterLen == strlen(ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED) &&
                         !strncasecmp(filterPos,
                                      ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED,
                                      filterLen))
                {
                    context.requestedOpState[iecsOpStateZombie] = true;
                }
                else
                {
                    // See if it's a numeric value ID
                    char testNumeric[filterLen+1];

                    memcpy(testNumeric, filterPos, filterLen);
                    testNumeric[filterLen] = '\0';

                    char *endPtr = NULL;
                    iecsOpState_t opState = (iecsOpState_t)strtoul(testNumeric, &endPtr, 10);
                    assert(endPtr != NULL);

                    if (*endPtr == '\0' && (opState >= 0 && opState < iecsOpStateLAST))
                    {
                        context.requestedOpState[opState] = true;
                    }
                    else
                    {
                        ieutTRACEL(pThreadData, tempFilter, ENGINE_ERROR_TRACE, "Invalid ConnectionState Filter '%s'\n", tempFilter);
                        rc = ISMRC_InvalidParameter;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                }

                filterPos = endPos;
            }
        }

        // Check for selection of specific protocol IDs
        tempFilter = ism_common_getStringProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_PROTOCOLID);
        if (tempFilter != NULL)
        {
            memset(context.requestedProtocol, false, sizeof(context.requestedProtocol));

            const char *filterPos = tempFilter;
            while(filterPos != NULL)
            {
                size_t filterLen;
                char *endPos = strchr(filterPos, ',');

                if (endPos != NULL)
                {
                    filterLen = (size_t)(endPos-filterPos);
                    endPos += 1;
                }
                else
                {
                    filterLen = strlen(filterPos);
                }

                // Skip over leading spaces
                while(*filterPos == ' ')
                {
                    filterPos++;
                    filterLen--;
                }

                // Ignore trailing spaces
                while(filterLen > 0 && filterPos[filterLen-1] == ' ')
                {
                    filterLen--;
                }

                // Check this filter
                if (filterLen == 3 && !strncasecmp(filterPos,
                                                   ismENGINE_MONITOR_FILTER_PROTOCOLID_ALL,
                                                   filterLen))
                {
                    // Include all but PROTOCOL_ID_SHARED in the returned set
                    memset(context.requestedProtocol, true, sizeof(context.requestedProtocol));
                    context.requestedProtocol[PROTOCOL_ID_SHARED] = false;
                    break; // Don't bother looking for other values, they asked for 'All'
                }
                else if (filterLen == 10 && !strncasecmp(filterPos,
                                                         ismENGINE_MONITOR_FILTER_PROTOCOLID_DEVICELIKE,
                                                         filterLen))
                {
                    // "DeviceLike" is what was previously "All", i.e. MQTT, HTTP & PLUG-IN protocols
                    context.requestedProtocol[PROTOCOL_ID_MQTT] = true;
                    context.requestedProtocol[PROTOCOL_ID_HTTP] = true;
                    context.requestedProtocol[PROTOCOL_ID_PLUGIN] = true;
                }
                else if (filterLen == 4 && !strncasecmp(filterPos,
                                                        ismENGINE_MONITOR_FILTER_PROTOCOLID_MQTT,
                                                        filterLen))
                {
                    context.requestedProtocol[PROTOCOL_ID_MQTT] = true;
                }
                else if (filterLen == 4 && !strncasecmp(filterPos,
                                                        ismENGINE_MONITOR_FILTER_PROTOCOLID_HTTP,
                                                        filterLen))
                {
                    context.requestedProtocol[PROTOCOL_ID_HTTP] = true;
                }
                else if (filterLen == 6 && !strncasecmp(filterPos,
                                                        ismENGINE_MONITOR_FILTER_PROTOCOLID_PLUGIN,
                                                        filterLen))
                {
                    context.requestedProtocol[PROTOCOL_ID_PLUGIN] = true;
                }
                else
                {
                    // See if it's a numeric protocol ID
                    char testNumeric[filterLen+1];

                    memcpy(testNumeric, filterPos, filterLen);
                    testNumeric[filterLen] = '\0';

                    char *endPtr = NULL;
                    uint64_t protocolId = strtoul(testNumeric, &endPtr, 10);
                    assert(endPtr != NULL);

                    if (*endPtr == '\0' && (protocolId > 0 && protocolId <= PROTOCOL_ID_PLUGIN))
                    {
                        context.requestedProtocol[protocolId] = true;
                    }
                    else
                    {
                        ieutTRACEL(pThreadData, tempFilter, ENGINE_ERROR_TRACE, "Invalid ProtocolID Filter '%s'\n", tempFilter);
                        rc = ISMRC_InvalidParameter;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                }

                filterPos = endPos;
            }
        }
    }

    // Now get the results, by visiting all client states in the table
    (void)iecs_traverseClientStateTable(pThreadData,
                                        NULL, 0, 0, NULL,
                                        iemn_getClientStateMonitor,
                                        &context);

    pMonitor = context.pResults; // May have been reallocated
    rc = context.rc;
    resultCount = context.resultCount;

mod_exit:

    if (filters.regexClientId != NULL) ism_regex_free(filters.regexClientId);

    if (((rc != OK) || (resultCount == 0)) && (pMonitor != NULL))
    {
        ism_engine_freeClientStateMonitor(pMonitor);
        pMonitor = NULL;
    }

    if (rc == OK)
    {
        *ppMonitor = pMonitor;
        *pResultCount = resultCount;
    }

    ieutTRACEL(pThreadData, pMonitor, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, pMonitor=%p\n", __func__,
               rc, resultCount, pMonitor);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @brief  Free monitoring data for client-states
///
/// @param[in]     pMonitor          The monitoring information to free
//****************************************************************************
XAPI void ism_engine_freeClientStateMonitor(ismEngine_ClientStateMonitor_t *pMonitor)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, pMonitor,  ENGINE_CEI_TRACE, FUNCTION_IDENT "pMonitor=%p\n", __func__, pMonitor);

    // If the list is NULL, nothing to do.
    if (pMonitor != NULL)
    {
        ismEngine_ClientStateMonitor_t *pCurrent = pMonitor;

        while (pCurrent->ClientId != NULL)
        {
            iemem_free(pThreadData, iemem_monitoringData, (char *)pCurrent->ClientId);
            pCurrent++;
        }

        iemem_free(pThreadData, iemem_monitoringData, pMonitor);
    }

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Match the specified transaction with the specified filters
///
/// @param[in]     transaction       The transaction to check
/// @param[in]     filters           The filters to apply to the queue
///
/// @return true if the transaction matches, otherwise false.
//****************************************************************************
#define iemnMQCONNECTIVITY_TXN_FORMATID 0x494D514D
static bool iemn_matchTransactionFilters(ismEngine_Transaction_t *transaction,
                                         iemnTransactionFilters_t *filters)
{
    bool match = false;

    // Not interested in this transaction state
    if (filters->includeState[transaction->TranState] == false)
    {
        goto mod_exit;
    }

    // Excluding MQ Connectivity transactions, and this is one of them
    if (filters->excludeMQConnectivity &&
        transaction->pXID->formatID == iemnMQCONNECTIVITY_TXN_FORMATID)
    {
        goto mod_exit;
    }

    // Looking for specific XIDs
    if (filters->xidString != NULL)
    {
        char XIDBuffer[300];
        const char *XIDString;

        // Generate the string representation of this transaction's XID
        XIDString = ism_common_xidToString(transaction->pXID, XIDBuffer, sizeof(XIDBuffer));

        assert(XIDString == XIDBuffer);

        if (ism_common_match(XIDString, filters->xidString) == 0)
        {
            goto mod_exit;
        }
    }

    // Passed all specified filters
    match = true;

mod_exit:

    return match;
}

//****************************************************************************
/// @brief  Deal with an individual transaction and see if it fits our set
//****************************************************************************
static void iemn_getTransactionMonitor(ieutThreadData_t *pThreadData,
                                       char *key,
                                       uint32_t keyHash,
                                       void *value,
                                       void *context)
{
    iemnGetTransactionMonitorContext_t *pContext = (iemnGetTransactionMonitorContext_t *)context;
    ismEngine_Transaction_t *transaction = (ismEngine_Transaction_t *)value;
    uint8_t tranState = transaction->TranState;

    assert(tranState <= ietrMAX_TRANSACTION_STATE_VALUE);

    // If we are applying filters see if we can quickly discount this transaction
    if (pContext->applyFilters && iemn_matchTransactionFilters(transaction, pContext->pFilters) == false)
    {
        goto mod_exit;
    }

    uint32_t pos;
    ismEngine_TransactionMonitor_t localTransactionMonitor;

    // NULL comparison function means we are getting all results unsorted
    if (pContext->comparisonFunction == NULL)
    {
        if (pContext->localResultCount == pContext->maxResults)
        {
            // Re-allocate for 100 more results
            uint32_t newMaxResults = pContext->maxResults + 100;

            ismEngine_TransactionMonitor_t *newLocalResults = iemem_realloc(pThreadData,
                                                                            IEMEM_PROBE(iemem_monitoringData, 4),
                                                                            pContext->localResults,
                                                                            (newMaxResults+1) * sizeof(ismEngine_TransactionMonitor_t));

            if (NULL == newLocalResults)
            {
                pContext->rc = ISMRC_AllocateError;
                ism_common_setError(pContext->rc);
                goto mod_exit;
            }

            pContext->localResults = newLocalResults;
            pContext->maxResults = newMaxResults;
        }

        pos = pContext->localResultCount;

        memcpy(&pContext->localResults[pos].XID, transaction->pXID, sizeof(ism_xid_t));
        pContext->localResults[pos].state = tranState;
        pContext->localResults[pos].stateChangedTime = transaction->StateChangedTime;

        pContext->localResultCount++;
    }
    else
    {
        memcpy(&localTransactionMonitor.XID, transaction->pXID, sizeof(ism_xid_t));
        localTransactionMonitor.state = tranState;
        localTransactionMonitor.stateChangedTime = transaction->StateChangedTime;

        // Initially, fill the return array with the first entries found, in order.
        if (pContext->localResultCount < pContext->maxResults)
        {
            for(pos=0; pos<pContext->localResultCount; pos++)
            {
                if (pContext->comparisonFunction(&pContext->localResults[pos], &localTransactionMonitor) > 0)
                {
                    memmove(&pContext->localResults[pos+1],
                            &pContext->localResults[pos],
                            (pContext->localResultCount-pos)*sizeof(ismEngine_TransactionMonitor_t));
                    break;
                }
            }

            pContext->localResults[pos] = localTransactionMonitor;
            pContext->localResultCount++;
        }
        // Once we have an initial set, we are only interested in adding values
        // that are an improvement on the current set.
        else if (pContext->comparisonFunction(&localTransactionMonitor, &pContext->localResults[0]) > 0)
        {
            int32_t start=0;
            int32_t end=pContext->maxResults;

            // The set is sorted, so we use a binary split search
            while(start != end)
            {
                int32_t mid = start+((end-start)/2);
                int32_t result = pContext->comparisonFunction(&pContext->localResults[mid], &localTransactionMonitor);

                if (result == 0)
                {
                    end = start = mid;
                }
                else if (result > 0)
                {
                    end = mid;
                }
                else
                {
                    start = mid + 1;
                }
            }

            pos = start-1;

            memmove(&pContext->localResults[0],
                    &pContext->localResults[1],
                    pos*sizeof(ismEngine_TransactionMonitor_t));

            pContext->localResults[pos] = localTransactionMonitor;
        }
    }

mod_exit:

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for transactions
///
/// @param[out]    results           The returned list of transaction monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeTransactionMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getTransactionMonitor(
                                               ismEngine_TransactionMonitor_t **results
                                             , uint32_t *resultCount
                                             , ismEngineMonitorType_t type
                                             , uint32_t maxResults
                                             , ism_prop_t *filterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    iemnGetTransactionMonitorContext_t tmonContext = { 0 };
    iemnTransactionFilters_t filters = iemnTRANSACTION_FILTERS_DEFAULT;

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u filterProperties=%p\n", __func__,
               type, maxResults, filterProperties);

    assert(results != NULL);
    assert(resultCount != NULL);

    // The default filters do need to be applied.
    tmonContext.applyFilters = true;
    tmonContext.pFilters = &filters;

    // Filtering is being overridden
    if (filterProperties != NULL)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on XID
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_XID);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            filters.xidString = tempFilter;

            // If there is a request for specific XID/XIDs *INCLUDE* MQConnectivity XIDs in the result
            filters.excludeMQConnectivity = false;
        }

        // Check for a filter on transaction state that doesn't match the default
        // Note: That this is a comma separated list
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_TRANSTATE);
        if (NULL != tempFilter && strncasecmp(tempFilter, ismENGINE_MONITOR_FILTER_TRANSTATE_ALL, 3))
        {
            // Start out by resetting the defaults to false
            memset(&filters.includeState, 0, sizeof(filters.includeState));

            // Go through each state in the list setting appropriate booleans in our array
            const char *filterPos = tempFilter;
            while(filterPos != NULL)
            {
                size_t filterLen;
                char *endPos = strchr(filterPos, ',');

                if (endPos != NULL)
                {
                    filterLen = (size_t)(endPos-filterPos);
                    endPos += 1;
                }
                else
                {
                    filterLen = strlen(filterPos);
                }

                // Skip over leading spaces
                while(*filterPos == ' ')
                {
                    filterPos++;
                    filterLen--;
                }

                // Ignore trailing spaces
                while(filterLen > 0 && filterPos[filterLen-1] == ' ')
                {
                    filterLen--;
                }

                // Check this filter and translate it into a boolean
                if (filterLen == 3 && !strncasecmp(filterPos,
                                                   ismENGINE_MONITOR_FILTER_TRANSTATE_ANY,
                                                   filterLen))
                {
                    memset(&filters.includeState, true, sizeof(filters.includeState));

                    assert(filters.includeState[ismTRANSACTION_STATE_NONE] == true);
                }
                else if (filterLen == 8)
                {
                    if (!strncasecmp(filterPos,
                                     ismENGINE_MONITOR_FILTER_TRANSTATE_PREPARED,
                                     filterLen))
                    {
                        filters.includeState[ismTRANSACTION_STATE_PREPARED] = true;
                    }
                    else if (!strncasecmp(filterPos,
                                          ismENGINE_MONITOR_FILTER_TRANSTATE_IN_FLIGHT,
                                          filterLen))
                    {
                        filters.includeState[ismTRANSACTION_STATE_IN_FLIGHT] = true;
                    }
                    else
                    {
                        ieutTRACEL(pThreadData, tempFilter, ENGINE_ERROR_TRACE, "Invalid TranState Filter '%s'\n", tempFilter);
                        rc = ISMRC_InvalidParameter;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                }
                else if (filterLen == 9 && !strncasecmp(filterPos,
                                                        ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC,
                                                        filterLen))
                {
                    filters.includeState[ismTRANSACTION_STATE_HEURISTIC_COMMIT] = true;
                    filters.includeState[ismTRANSACTION_STATE_HEURISTIC_ROLLBACK] = true;
                }
                else if (filterLen == 10 && !strncasecmp(filterPos,
                                                         ismENGINE_MONITOR_FILTER_TRANSTATE_COMMIT_ONLY,
                                                         filterLen))
                {
                    filters.includeState[ismTRANSACTION_STATE_COMMIT_ONLY] = true;
                }
                else if (filterLen == 12 && !strncasecmp(filterPos,
                                                         ismENGINE_MONITOR_FILTER_TRANSTATE_ROLLBACK_ONLY,
                                                         filterLen))
                {
                    filters.includeState[ismTRANSACTION_STATE_ROLLBACK_ONLY] = true;
                }
                else if (filterLen == 15 && !strncasecmp(filterPos,
                                                         ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_COMMIT,
                                                         filterLen))
                {
                    filters.includeState[ismTRANSACTION_STATE_HEURISTIC_COMMIT] = true;
                }
                else if (filterLen == 17 && !strncasecmp(filterPos,
                                                         ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_ROLLBACK,
                                                         filterLen))
                {
                    filters.includeState[ismTRANSACTION_STATE_HEURISTIC_ROLLBACK] = true;
                }
                else
                {
                    ieutTRACEL(pThreadData, tempFilter, ENGINE_ERROR_TRACE, "Invalid TranState Filter '%s'\n", tempFilter);
                    rc = ISMRC_InvalidParameter;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                filterPos = endPos;
            }
        }
    }

    tmonContext.localResultCount = 0;
    tmonContext.localResults = NULL;

    // If all transactions are requested, we just ignore maxResults
    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        tmonContext.maxResults = 0;
    }
    else
    {
        tmonContext.maxResults = maxResults;

        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        tmonContext.localResults = iemem_calloc(pThreadData,
                                                IEMEM_PROBE(iemem_monitoringData, 12), 1,
                                                (maxResults+1) * sizeof(ismEngine_TransactionMonitor_t));

        if (NULL == tmonContext.localResults)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        switch(type)
        {
            case ismENGINE_MONITOR_OLDEST_STATECHANGE:
                tmonContext.comparisonFunction = iemn_oldestTransactionStateChange;
                break;
            default:
                assert(false);
                break;
        }

    }

    // Lock the global transaction table for read
    ismEngine_getRWLockForRead(&ismEngine_serverGlobal.TranControl->GlobalTranLock);

    // Go through the transactions
    ieut_traverseHashTable(pThreadData,
                           ismEngine_serverGlobal.TranControl->GlobalTranTable,
                           &iemn_getTransactionMonitor,
                           &tmonContext);

    rc = tmonContext.rc;

    // Unlock the global transaction table
    ismEngine_unlockRWLock(&ismEngine_serverGlobal.TranControl->GlobalTranLock);

mod_exit:

    // All done, update the caller's results
    if (rc == OK && tmonContext.localResultCount != 0)
    {
        // The results are in the counter-intuitive order for the request, so reverse them.
        if (type != ismENGINE_MONITOR_ALL_UNSORTED)
        {
            uint32_t pos=0, swap_pos=tmonContext.localResultCount-1;

            for(; pos<swap_pos; pos++, swap_pos--)
            {
                ismEngine_TransactionMonitor_t localTransactionMonitor = tmonContext.localResults[pos];
                tmonContext.localResults[pos] = tmonContext.localResults[swap_pos];
                tmonContext.localResults[swap_pos] = localTransactionMonitor;
            }
        }

        *resultCount = tmonContext.localResultCount;
        *results = tmonContext.localResults;
    }
    else
    {
        ism_engine_freeTransactionMonitor(tmonContext.localResults);

        *resultCount = 0;
        *results = NULL;
    }

    ieutTRACEL(pThreadData, *results, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, results=%p\n", __func__,
               rc, *resultCount, *results);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @brief  Free a transaction monitor list
///
/// @param[in]     list    The transaction monitor list to free
//****************************************************************************
XAPI void ism_engine_freeTransactionMonitor(ismEngine_TransactionMonitor_t *list)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, list,  ENGINE_CEI_TRACE, FUNCTION_IDENT "list=%p\n", __func__, list);

    // If the list is NULL, nothing to do.
    if (list)
    {
        iemem_free(pThreadData, iemem_monitoringData, list);
    }

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Update the stats in a ResourceSetMonitor from a resource set
///
/// @param[in,out] resourceSetMonitor  Resource set monitor to update
/// @param[in]     pResourceSet        Resource set to update from
/// @param[in]     resetTime           Time when this set was reset
/// @param[in]     reportTime          Time when this report is being generated
//****************************************************************************
static inline void iemn_updateResourceSetMonitor(ismEngine_ResourceSetMonitor_t *resourceSetMonitor,
                                                 iereResourceSet_t *pResourceSet,
                                                 ism_time_t resetTime,
                                                 ism_time_t reportTime)
{
    int64_t thisValue;

    const iereResourceSetStats_t localStats = pResourceSet->stats;
    ismEngine_ResourceSetStatistics_t *resMonStats = &resourceSetMonitor->stats;

    resourceSetMonitor->resourceSet = pResourceSet;
    resourceSetMonitor->resourceSetId = localStats.resourceSetIdentifier;
    resourceSetMonitor->resetTime = resetTime;
    resourceSetMonitor->reportTime = reportTime;
    assert(reportTime >= resetTime);
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_MEMORY];
    resMonStats->TotalMemoryBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SUBSCRIPTIONS];
    resMonStats->PersistentNonSharedSubscriptions = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SUBSCRIPTIONS];
    resMonStats->NonpersistentNonSharedSubscriptions = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SHARED_SUBSCRIPTIONS];
    resMonStats->PersistentSharedSubscriptions = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SHARED_SUBSCRIPTIONS];
    resMonStats->NonpersistentSharedSubscriptions = thisValue > 0 ? (uint64_t)thisValue : 0;
    resMonStats->Subscriptions = resMonStats->NonpersistentNonSharedSubscriptions +
                                 resMonStats->NonpersistentSharedSubscriptions +
                                 resMonStats->PersistentNonSharedSubscriptions +
                                 resMonStats->PersistentSharedSubscriptions;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS];
    resMonStats->BufferedMsgs = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_BUFFEREDMSG_BYTES];
    resMonStats->PersistentBufferedMsgBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_BUFFEREDMSG_BYTES];
    resMonStats->NonpersistentBufferedMsgBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    resMonStats->BufferedMsgBytes = resMonStats->PersistentBufferedMsgBytes +
                                    resMonStats->NonpersistentBufferedMsgBytes;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS];
    resMonStats->RetainedMsgs = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES];
    resMonStats->RetainedMsgBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_WILLMSGS];
    resMonStats->WillMsgs = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_WILLMSG_BYTES];
    resMonStats->PersistentWillMsgBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_WILLMSG_BYTES];
    resMonStats->NonpersistentWillMsgBytes = thisValue > 0 ? (uint64_t)thisValue : 0;
    resMonStats->WillMsgBytes = resMonStats->PersistentWillMsgBytes +
                                resMonStats->NonpersistentWillMsgBytes;
    resMonStats->QoS0PublishedMsgs = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED]);
    resMonStats->QoS1PublishedMsgs = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED]);
    resMonStats->QoS2PublishedMsgs = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED]);
    resMonStats->PublishedMsgs = resMonStats->QoS0PublishedMsgs +
                                 resMonStats->QoS1PublishedMsgs +
                                 resMonStats->QoS2PublishedMsgs;
    resMonStats->PublishedMsgsSinceRestart = resMonStats->PublishedMsgs +
                                             (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED_TO_LASTRESET]) +
                                             (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED_TO_LASTRESET]) +
                                             (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED_TO_LASTRESET]);
    resMonStats->QoS0PublishedMsgBytes = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED]);
    resMonStats->QoS1PublishedMsgBytes = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED]);
    resMonStats->QoS2PublishedMsgBytes = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED]);
    resMonStats->PublishedMsgBytes = resMonStats->QoS0PublishedMsgBytes +
                                     resMonStats->QoS1PublishedMsgBytes +
                                     resMonStats->QoS2PublishedMsgBytes;
    resMonStats->PublishedMsgBytesSinceRestart = resMonStats->PublishedMsgBytes +
                                                 (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED_TO_LASTRESET]) +
                                                 (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED_TO_LASTRESET]) +
                                                 (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED_TO_LASTRESET]);
    resMonStats->DiscardedMsgs = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS]);
    resMonStats->DiscardedMsgsSinceRestart = resMonStats->DiscardedMsgs +
                                             (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS_TO_LASTRESET]);
    resMonStats->RejectedMsgs = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS]);
    resMonStats->RejectedMsgsSinceRestart = resMonStats->RejectedMsgs +
                                            (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS_TO_LASTRESET]);
    resMonStats->Connections = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS]);
    resMonStats->ConnectionsSinceRestart = resMonStats->Connections +
                                           (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS_TO_LASTRESET]);
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_PERSISTENT_CLIENTS];
    resMonStats->ActivePersistentClients = thisValue > 0 ? (uint64_t)thisValue : 0;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_NONPERSISTENT_CLIENTS];
    resMonStats->ActiveNonpersistentClients = thisValue > 0 ? (uint64_t)thisValue : 0;
    resMonStats->ActiveClients = resMonStats->ActivePersistentClients +
                                 resMonStats->ActiveNonpersistentClients;
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES];
    resMonStats->PersistentClientStates = thisValue > 0 ? (uint64_t)thisValue : 0;
    resMonStats->MaxPublishRecipients = (uint64_t)(localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS]);
    thisValue = localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS_TO_LASTRESET];
    if (resMonStats->MaxPublishRecipients > (uint64_t)thisValue)
    {
        resMonStats->MaxPublishRecipientsSinceRestart = resMonStats->MaxPublishRecipients;
    }
    else
    {
        resMonStats->MaxPublishRecipientsSinceRestart = (uint64_t)thisValue;
    }
}

//****************************************************************************
/// @brief  Callback for a resource set in the resource set table
///
/// @param[in]     pResourceSet      iereResourceSet_t being enumerated
/// @param[in]     context           iemnGetResourceMonitorContext_t
//****************************************************************************
static void iemn_resourceSetCallback(ieutThreadData_t *pThreadData,
                                     iereResourceSet_t *pResourceSet,
                                     ism_time_t resetTime,
                                     void *context)
{
    uint32_t pos;
    ismEngine_ResourceSetMonitor_t localResourceSetMonitor;
    iemnGetResourceSetMonitorContext_t *pContext = (iemnGetResourceSetMonitorContext_t *)context;

    if (!pContext->applyFilters || iemn_matchResourceSetFilters(pResourceSet, pContext->pFilters))
    {
        // If we haven't set the reportTime yet, set it now. We do this here because we are now
        // inside the lock which needs to be taken for the resetTime to be altered, so we know this
        // is a valid time AFTER the resetTime.
        if (pContext->reportTime == 0) pContext->reportTime = ism_common_currentTimeNanos();

        if (pContext->type == ismENGINE_MONITOR_ALL_UNSORTED)
        {
            if (pContext->localResultCount == pContext->maxResults)
            {
                uint32_t newMaxResults = pContext->maxResults + 1000;
                ismEngine_ResourceSetMonitor_t *pNewResults;

                pNewResults = iemem_realloc(pThreadData,
                                            IEMEM_PROBE(iemem_monitoringData, 16), pContext->localResults,
                                            (newMaxResults+1) * sizeof(ismEngine_ResourceSetMonitor_t));

                if (pNewResults == NULL)
                {
                    pContext->rc = ISMRC_AllocateError;
                    goto mod_exit;
                }

                pContext->localResults = pNewResults;
                pContext->maxResults = newMaxResults;
            }

            pos = pContext->localResultCount++;

            iemn_updateResourceSetMonitor(&pContext->localResults[pos], pResourceSet, resetTime, pContext->reportTime);
        }
        else
        {
            iemn_updateResourceSetMonitor(&localResourceSetMonitor, pResourceSet, resetTime, pContext->reportTime);

            // Initially, fill the return array with the first entries found, in order.
            if (pContext->localResultCount < pContext->maxResults)
            {
                for(pos=0; pos<pContext->localResultCount; pos++)
                {
                    if (pContext->comparisonFunction(&pContext->localResults[pos], &localResourceSetMonitor) > 0)
                    {
                        memmove(&pContext->localResults[pos+1],
                                &pContext->localResults[pos],
                                (pContext->localResultCount-pos)*sizeof(ismEngine_ResourceSetMonitor_t));
                        break;
                    }
                }

                pContext->localResults[pos] = localResourceSetMonitor;
                pContext->localResultCount++;
            }
            else
            {
                ismEngine_ResourceSetStatistics_t *pOtherStats = &pContext->otherStats;

                // Once we have an initial set, we are only interested in adding values that are an improvement on the current set.
                if (pContext->comparisonFunction(&localResourceSetMonitor, &pContext->localResults[0]) > 0)
                {
                    int32_t start=0;
                    int32_t end=pContext->maxResults;

                    // The set is sorted, so we use a binary split search
                    while(start != end)
                    {
                        int32_t mid = start+((end-start)/2);
                        int32_t result = pContext->comparisonFunction(&pContext->localResults[mid], &localResourceSetMonitor);

                        if (result == 0)
                        {
                            end = start = mid;
                        }
                        else if (result > 0)
                        {
                            end = mid;
                        }
                        else
                        {
                            start = mid + 1;
                        }
                    }

                    pos = start-1;

                    ismEngine_ResourceSetMonitor_t removedResourceSetMonitor = pContext->localResults[0];

                    memmove(&pContext->localResults[0], &pContext->localResults[1], pos*sizeof(ismEngine_ResourceSetMonitor_t));

                    pContext->localResults[pos] = localResourceSetMonitor;

                    // Set up to update the otherStats...
                    localResourceSetMonitor = removedResourceSetMonitor;
                }

                // At this point localResourceSetMonitor contains either this new one, or the one removed from
                // the set we're going to return -- either way, update the stats for other resource sets with
                // the values from it.
                pOtherStats->TotalMemoryBytes += localResourceSetMonitor.stats.TotalMemoryBytes;
                pOtherStats->Subscriptions += localResourceSetMonitor.stats.Subscriptions;
                pOtherStats->PersistentNonSharedSubscriptions += localResourceSetMonitor.stats.PersistentNonSharedSubscriptions;
                pOtherStats->NonpersistentNonSharedSubscriptions += localResourceSetMonitor.stats.NonpersistentNonSharedSubscriptions;
                pOtherStats->PersistentSharedSubscriptions += localResourceSetMonitor.stats.PersistentSharedSubscriptions;
                pOtherStats->NonpersistentSharedSubscriptions += localResourceSetMonitor.stats.NonpersistentSharedSubscriptions;
                pOtherStats->BufferedMsgs += localResourceSetMonitor.stats.BufferedMsgs;
                pOtherStats->DiscardedMsgs += localResourceSetMonitor.stats.DiscardedMsgs;
                pOtherStats->RejectedMsgs += localResourceSetMonitor.stats.RejectedMsgs;
                pOtherStats->RetainedMsgs += localResourceSetMonitor.stats.RetainedMsgs;
                pOtherStats->WillMsgs += localResourceSetMonitor.stats.WillMsgs;
                pOtherStats->BufferedMsgBytes += localResourceSetMonitor.stats.BufferedMsgBytes;
                pOtherStats->PersistentBufferedMsgBytes += localResourceSetMonitor.stats.PersistentBufferedMsgBytes;
                pOtherStats->NonpersistentBufferedMsgBytes += localResourceSetMonitor.stats.NonpersistentBufferedMsgBytes;
                pOtherStats->RetainedMsgBytes += localResourceSetMonitor.stats.RetainedMsgBytes;
                pOtherStats->WillMsgBytes += localResourceSetMonitor.stats.WillMsgBytes;
                pOtherStats->PersistentWillMsgBytes += localResourceSetMonitor.stats.PersistentWillMsgBytes;
                pOtherStats->NonpersistentWillMsgBytes += localResourceSetMonitor.stats.NonpersistentWillMsgBytes;
                pOtherStats->PublishedMsgs += localResourceSetMonitor.stats.PublishedMsgs;
                pOtherStats->QoS0PublishedMsgs += localResourceSetMonitor.stats.QoS0PublishedMsgs;
                pOtherStats->QoS1PublishedMsgs += localResourceSetMonitor.stats.QoS1PublishedMsgs;
                pOtherStats->QoS2PublishedMsgs += localResourceSetMonitor.stats.QoS2PublishedMsgs;
                pOtherStats->PublishedMsgBytes += localResourceSetMonitor.stats.PublishedMsgBytes;
                pOtherStats->QoS0PublishedMsgBytes += localResourceSetMonitor.stats.QoS0PublishedMsgBytes;
                pOtherStats->QoS1PublishedMsgBytes += localResourceSetMonitor.stats.QoS1PublishedMsgBytes;
                pOtherStats->QoS2PublishedMsgBytes += localResourceSetMonitor.stats.QoS2PublishedMsgBytes;
                pOtherStats->Connections += localResourceSetMonitor.stats.Connections;
                pOtherStats->ActiveClients += localResourceSetMonitor.stats.ActiveClients;
                pOtherStats->ActivePersistentClients += localResourceSetMonitor.stats.ActivePersistentClients;
                pOtherStats->ActiveNonpersistentClients += localResourceSetMonitor.stats.ActiveNonpersistentClients;
                pOtherStats->PersistentClientStates += localResourceSetMonitor.stats.PersistentClientStates;

                // Non-cumulative values...
                if (localResourceSetMonitor.stats.MaxPublishRecipients > pOtherStats->MaxPublishRecipients)
                {
                    pOtherStats->MaxPublishRecipients = localResourceSetMonitor.stats.MaxPublishRecipients;
                }

                // Sanity check that the stats structure hasn't changed... If it has, we need to include new
                // stats above.
                assert(sizeof(*pOtherStats) ==
                       (offsetof(ismEngine_ResourceSetStatistics_t, PersistentClientStates)+
                        sizeof(localResourceSetMonitor.stats.PersistentClientStates)));

            }
        }
    }

mod_exit:

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for resource sets
///
/// @param[out]    results           The returned list of resource set monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in,out] otherSets         The monitor values for sets that are not
///                                  included in the results.
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeResourceSetMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t iemn_getResourceSetMonitor( ieutThreadData_t *pThreadData
                                  , ismEngine_ResourceSetMonitor_t **results
                                  , uint32_t *resultCount
                                  , ismEngineMonitorType_t type
                                  , uint32_t maxResults
                                  , ismEngine_ResourceSetMonitor_t *otherSets
                                  , ism_prop_t *filterProperties)
{
    int32_t rc;
    uint32_t localResultCount = 0;
    ismEngine_ResourceSetMonitor_t *localResults = NULL;
    uint32_t pos;

    ieutTRACEL(pThreadData, type, ENGINE_FNC_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u otherSets=%p\n", __func__,
               type, maxResults, otherSets);

    // Deal with filtering of results
    iemnResourceSetFilters_t filters = iemnRESOURCESET_FILTERS_DEFAULT;

    iemnGetResourceSetMonitorContext_t context;
    context.type = type;
    context.rc = OK;
    // Gets set when we find a matching set, to ensure that it is after resetTime.
    context.reportTime = 0;

    // For 'all unsorted' we reallocate as needed and ignore maxResults passed in
    if (type == ismENGINE_MONITOR_ALL_UNSORTED)
    {
        maxResults = 0;
        context.comparisonFunction = NULL;

        // Cannot support otherSets if we're returning all sets.
        if (otherSets != NULL)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }
    else
    {
        memset(&context.otherStats, 0, sizeof(context.otherStats));

        if (maxResults == 0)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        switch(type)
        {
            case ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorTotalMemoryBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorPersistentNonSharedSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorNonpersistentNonSharedSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorPersistentSharedSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorNonpersistentSharedSubscriptions;
                break;
            case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorBufferedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorDiscardedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorRejectedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorRetainedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_WILLMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorWillMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorBufferedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorPersistentBufferedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorNonpersistentBufferedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorRetainedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorWillMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorPersistentWillMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorNonpersistentWillMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorPublishedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS0PublishedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS1PublishedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS2PublishedMsgs;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorPublishedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS0PublishedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS1PublishedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES:
                context.comparisonFunction = iemn_highestResourceSetMonitorQoS2PublishedMsgBytes;
                break;
            case ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS:
                context.comparisonFunction = iemn_highestResourceSetMonitorMaxPublishRecipients;
                break;
            case ismENGINE_MONITOR_HIGHEST_CONNECTIONS:
                context.comparisonFunction = iemn_highestResourceSetMonitorConnections;
                break;
            case ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS:
                context.comparisonFunction = iemn_highestResourceSetMonitorActiveClients;
                break;
            case ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS:
                context.comparisonFunction = iemn_highestResourceSetMonitorActivePersistentClients;
                break;
            case ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS:
                context.comparisonFunction = iemn_highestResourceSetMonitorActiveNonpersistentClients;
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES:
                context.comparisonFunction = iemn_highestResourceSetMonitorPersistentClientStates;
                break;
            default:
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit;
        }

        localResults = iemem_calloc(pThreadData,
                                    IEMEM_PROBE(iemem_monitoringData, 15), 1,
                                    (maxResults+1) * sizeof(ismEngine_ResourceSetMonitor_t));

        if (NULL == localResults)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    context.applyFilters = false;
    context.pFilters = &filters;
    context.maxResults = maxResults;
    context.localResultCount = 0;
    context.localResults = localResults;

    const char *specificResourceSetId = NULL;

    // Some filter properties were specified, analyse these now
    if (NULL != filterProperties)
    {
        const char *tempFilter = NULL;

        // Check for a non-trivial filter on resourceSet identifier
        tempFilter = ism_common_getStringProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID);
        if (NULL != tempFilter && (strlen(tempFilter) > 1 || tempFilter[0] != '*'))
        {
            // No wildcard specified -- So the caller is interested in a specific ResourceSetID
            if (strchr(tempFilter, '*') == NULL)
            {
                specificResourceSetId = tempFilter;
            }
            else
            {
                filters.resourceSetId = tempFilter;
                context.applyFilters = true;
            }
        }
    }

    // We've been called for a specific resource set, so go find it directly.
    if (specificResourceSetId != NULL)
    {
        iere_enumerateSingleResourceSet(pThreadData, specificResourceSetId, iemn_resourceSetCallback, &context);
    }
    else
    {
        iere_enumerateResourceSets(pThreadData, iemn_resourceSetCallback, &context);
    }

    localResults = context.localResults;
    localResultCount = context.localResultCount;
    rc = context.rc;

mod_exit:

    // All done, update the caller's results
    if (rc == OK && localResultCount != 0)
    {
        if (type != ismENGINE_MONITOR_ALL_UNSORTED)
        {
            ismEngine_ResourceSetMonitor_t localResourceSetMonitor = {0};

            uint32_t swap_pos = localResultCount-1;

            // The results are in the counter-intuitive order for the request, so reverse them.
            for(pos=0; pos<swap_pos; pos++, swap_pos--)
            {
                localResourceSetMonitor = localResults[pos];
                localResults[pos] = localResults[swap_pos];
                localResults[swap_pos] = localResourceSetMonitor;
            }

            // We need to fill in the otherSets if it was provided.
            if (otherSets != NULL)
            {
                memset(otherSets, 0, sizeof(*otherSets));

                otherSets->resetTime = localResults[0].resetTime;
                otherSets->stats = context.otherStats;
            }
        }
        else
        {
            assert(otherSets == NULL);
        }

        *resultCount = localResultCount;
        *results = localResults;
    }
    else
    {
        ism_engine_freeResourceSetMonitor(localResults);

        *resultCount = 0;
        *results = NULL;
    }

    ieutTRACEL(pThreadData, *results, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, results=%p\n", __func__,
               rc, *resultCount, *results);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Get engine monitoring data for resource sets
///
/// @param[out]    results           The returned list of resource set monitors
/// @param[out]    resultCount       Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     filterProperties  Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeResourceSetMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getResourceSetMonitor(
                                                ismEngine_ResourceSetMonitor_t **results
                                              , uint32_t *resultCount
                                              , ismEngineMonitorType_t type
                                              , uint32_t maxResults
                                              , ism_prop_t *filterProperties)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, type, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d maxResults=%u filterProperties=%p\n", __func__,
               type, maxResults, filterProperties);

    assert(results != NULL);
    assert(resultCount != NULL);

    rc = iemn_getResourceSetMonitor(pThreadData, results, resultCount, type, maxResults, NULL, filterProperties);

    ieutTRACEL(pThreadData, *results, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultCount=%u, results=%p\n", __func__,
               rc, *resultCount, *results);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Free a resource set monitor list
///
/// @param[in]     list              The resource set monitor list to free
//****************************************************************************
XAPI void ism_engine_freeResourceSetMonitor(ismEngine_ResourceSetMonitor_t *list)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, list,  ENGINE_CEI_TRACE, FUNCTION_IDENT "list=%p\n", __func__, list);

    // If the list is NULL, nothing to do.
    if (list != NULL)
    {
        iemem_free(pThreadData, iemem_monitoringData, list);
    }

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Add statistics from the given thread data structure
///
/// @param[in]     pThreadData   Thread data structure to use
/// @param[in]     context       ismEngine_MessageStatistics_t to update
//****************************************************************************
typedef struct tag_iemnCountThreadStatsCallbackContext_t
{
    uint64_t DroppedMessages;
    uint64_t ExpiredMessages;
    int64_t BufferedMessages;
    int64_t InternalRetainedMessages;
    int64_t ExternalRetainedMessages;
    int64_t BufferedMessagesWithExpirySet;
    int64_t RetainedMessagesWithExpirySet;
    int64_t RemoteServerBufferedMessageBytes;
    uint64_t FromForwarderRetainedMessages;
    uint64_t FromForwarderMessages;
    uint64_t FromForwarderNoRecipientMessages;
    uint64_t ExpiredClientStates;
    int64_t ZombieClientStatesWithExpirySet;
    int64_t ResoureSetMemBytes;
} iemnCountThreadStatsCallbackContext_t;

void iemn_countThreadStatsCallback(ieutThreadData_t *pThreadData,
                                   void *context)
{
    iemnCountThreadStatsCallbackContext_t *pContext = (iemnCountThreadStatsCallbackContext_t *)context;

    pContext->DroppedMessages += pThreadData->stats.droppedMsgCount;
    pContext->ExpiredMessages += pThreadData->stats.expiredMsgCount;
    pContext->BufferedMessages += pThreadData->stats.bufferedMsgCount;
    pContext->InternalRetainedMessages += pThreadData->stats.internalRetainedMsgCount;
    pContext->ExternalRetainedMessages += pThreadData->stats.externalRetainedMsgCount;
    pContext->BufferedMessagesWithExpirySet += pThreadData->stats.bufferedExpiryMsgCount;
    pContext->RetainedMessagesWithExpirySet += pThreadData->stats.retainedExpiryMsgCount;
    pContext->RemoteServerBufferedMessageBytes += pThreadData->stats.remoteServerBufferedMsgBytes;
    pContext->FromForwarderRetainedMessages += pThreadData->stats.fromForwarderRetainedMsgCount;
    pContext->FromForwarderMessages += pThreadData->stats.fromForwarderMsgCount;
    pContext->FromForwarderNoRecipientMessages += pThreadData->stats.fromForwarderNoRecipientMsgCount;
    pContext->ExpiredClientStates += pThreadData->stats.expiredClientStates;
    pContext->ZombieClientStatesWithExpirySet += pThreadData->stats.zombieSetToExpireCount;
    pContext->ResoureSetMemBytes += pThreadData->stats.resourceSetMemBytes;
}

//****************************************************************************
/// @internal
///
/// @brief  Get all per-thread statistics
///
/// @remark Engine-wide statistics are collected in per-thread stats
/// structures to avoid taking a lock - so we actually run the chain of thread data
/// structures updating statistics for each.
///
/// @return Nothing.
//****************************************************************************
void iemn_getThreadStats(ieutThreadData_t *pThreadData,
                         iemnCountThreadStatsCallbackContext_t *context)
{
    context->DroppedMessages = ismEngine_serverGlobal.endedThreadStats.droppedMsgCount;
    context->ExpiredMessages = ismEngine_serverGlobal.endedThreadStats.expiredMsgCount;
    context->BufferedMessages = ismEngine_serverGlobal.endedThreadStats.bufferedMsgCount;
    context->InternalRetainedMessages = ismEngine_serverGlobal.endedThreadStats.internalRetainedMsgCount;
    context->ExternalRetainedMessages = ismEngine_serverGlobal.endedThreadStats.externalRetainedMsgCount;
    context->BufferedMessagesWithExpirySet = ismEngine_serverGlobal.endedThreadStats.bufferedExpiryMsgCount;
    context->RetainedMessagesWithExpirySet = ismEngine_serverGlobal.endedThreadStats.retainedExpiryMsgCount;
    context->RemoteServerBufferedMessageBytes = ismEngine_serverGlobal.endedThreadStats.remoteServerBufferedMsgBytes;
    context->FromForwarderRetainedMessages = ismEngine_serverGlobal.endedThreadStats.fromForwarderRetainedMsgCount;
    context->FromForwarderMessages = ismEngine_serverGlobal.endedThreadStats.fromForwarderMsgCount;
    context->FromForwarderNoRecipientMessages = ismEngine_serverGlobal.endedThreadStats.fromForwarderNoRecipientMsgCount;
    context->ExpiredClientStates = ismEngine_serverGlobal.endedThreadStats.expiredClientStates;
    context->ZombieClientStatesWithExpirySet = ismEngine_serverGlobal.endedThreadStats.zombieSetToExpireCount;
    context->ResoureSetMemBytes = ismEngine_serverGlobal.endedThreadStats.resourceSetMemBytes;

    // Add all live individual counts
    ieut_enumerateThreadData(iemn_countThreadStatsCallback, context);
}

//****************************************************************************
/// @internal
///
/// @brief  Get all messaging statistics (internal and external)
///
/// @param[in]  pThreadData    thread data of the calling thread [OPTIONAL]
/// @param[out] pStatistics    iemnMessagingStatistics_t to fill in
///
/// @remark In order to be able to call this function from a reduce memory
/// callback which currently has no access to a pThreadData, we make it optional,
/// an alternative would be to alter the memHandler timer task / callback mechanism
/// to pass a pThreadData it's callbacks.
///
/// As it happens, this means we can reduce the tracing in the reduce memory
/// callbacks - which is probably a good thing.
///
/// @return Nothing.
//****************************************************************************
void iemn_getMessagingStatistics(ieutThreadData_t *pThreadData,
                                 iemnMessagingStatistics_t *pStatistics)
{
    if (pThreadData != NULL)
    {
        ieutTRACEL(pThreadData, pStatistics,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pStatistics=%p\n", __func__, pStatistics);
    }

    // Values that don't come from the per-thread statistics
    pStatistics->externalStats.ServerShutdownTime = ism_common_convertExpireToTime(ismEngine_serverGlobal.ServerShutdownTimestamp);

    iemnCountThreadStatsCallbackContext_t context;

    iemn_getThreadStats(pThreadData, &context);

    // Update the values with sensible (non-negative) results
    pStatistics->externalStats.DroppedMessages = context.DroppedMessages;
    pStatistics->externalStats.ExpiredMessages = context.ExpiredMessages;
    pStatistics->externalStats.BufferedMessages = (context.BufferedMessages < 0) ?
                                                      0 : (uint64_t)context.BufferedMessages;
    pStatistics->externalStats.RetainedMessages = (context.ExternalRetainedMessages < 0) ?
                                                      0 : (uint64_t)context.ExternalRetainedMessages;
    pStatistics->externalStats.ClientStates = ismEngine_serverGlobal.totalClientStatesCount;
    pStatistics->externalStats.ExpiredClientStates = (context.ExpiredClientStates < 0) ?
                                                         0 : (uint64_t)context.ExpiredClientStates;
    pStatistics->externalStats.Subscriptions = ismEngine_serverGlobal.totalSubsCount;

    pStatistics->InternalRetainedMessages = (context.InternalRetainedMessages < 0) ?
                                                0 : (uint64_t)context.InternalRetainedMessages;
    pStatistics->BufferedMessagesWithExpirySet = (context.BufferedMessagesWithExpirySet < 0) ?
                                                     0 : (uint64_t)context.BufferedMessagesWithExpirySet;
    pStatistics->RetainedMessagesWithExpirySet = (context.RetainedMessagesWithExpirySet < 0) ?
                                                     0 : (uint64_t)context.RetainedMessagesWithExpirySet;
    pStatistics->RemoteServerBufferedMessageBytes = (context.RemoteServerBufferedMessageBytes < 0) ?
                                                        0 : (uint64_t)context.RemoteServerBufferedMessageBytes;
    pStatistics->FromForwarderRetainedMessages = context.FromForwarderRetainedMessages;
    pStatistics->FromForwarderMessages = context.FromForwarderMessages;
    pStatistics->FromForwarderNoRecipientMessages = context.FromForwarderNoRecipientMessages;
    pStatistics->ResourceSetMemoryBytes = (context.ResoureSetMemBytes < 0) ?
                                              0 : (uint64_t)context.ResoureSetMemBytes;

    if (pThreadData != NULL)
    {
        ieutTRACEL(pThreadData, pStatistics, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Get all clientState statistics
///
/// @param[in]  pThreadData    thread data of the calling thread [OPTIONAL]
/// @param[out] pStatistics    iemnClientStateStatistics_t to fill in
///
/// @return Nothing.
//****************************************************************************
void iemn_getClientStateStatistics(ieutThreadData_t *pThreadData,
                                   iemnClientStateStatistics_t *pStatistics)
{
    ieutTRACEL(pThreadData, pStatistics,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pStatistics=%p\n", __func__, pStatistics);

    iemnCountThreadStatsCallbackContext_t context;

    iemn_getThreadStats(pThreadData, &context);

    // Update the values with sensible (non-negative) results
    pStatistics->ExpiredClientStates = context.ExpiredClientStates;
    pStatistics->ZombieClientStatesWithExpirySet = (context.ZombieClientStatesWithExpirySet < 0) ?
                                                       0 : (uint64_t)context.ZombieClientStatesWithExpirySet;

    ieutTRACEL(pThreadData, pStatistics, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @internal
///
/// @brief  Get messaging statistics
///
/// @param[out] pStatistics    ismEngine_MessagingStatistics_t to fill in
///
/// @remark The function returns engine-wide statistics about work that it
/// has been monitoring. This is *loosely* messaging related, but, for
/// instance, one statistic returned is the number of ClientStates that
/// have been expired.
///
/// @return Nothing.
//****************************************************************************
XAPI void ism_engine_getMessagingStatistics(ismEngine_MessagingStatistics_t *pStatistics)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, pStatistics,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "pStatistics=%p\n", __func__, pStatistics);

    iemnMessagingStatistics_t internalStats;

    iemn_getMessagingStatistics(pThreadData, &internalStats);

    *pStatistics = internalStats.externalStats; // Copy the external portion of the stats

    ieutTRACEL(pThreadData, pStatistics, ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);
    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Get the engine's statistic for the specified remote server Handle
///
/// @param[in]  remoteServerHandle  The engine's handle representing the remote
///                                 server.
/// @param[out] pStatistics         ismEngine_RemoteServerStatistics_t to fill in
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getRemoteServerStatistics(
                                                  ismEngine_RemoteServerHandle_t remoteServerHandle,
                                                  ismEngine_RemoteServerStatistics_t *pStatistics)
{
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ismEngine_RemoteServer_t *remoteServer = (ismEngine_RemoteServer_t *)remoteServerHandle;

    ieutTRACEL(pThreadData, remoteServerHandle, ENGINE_CEI_TRACE, FUNCTION_ENTRY "remoteServerHandle=%p pStatistics=%p\n",
               __func__, remoteServerHandle, pStatistics);

    if (remoteServerHandle == NULL || pStatistics == NULL)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    iers_acquireServerReference(remoteServer);

    ism_time_t outOfSyncTime = 0;

    assert(strlen(remoteServer->serverUID) <= sizeof(pStatistics->serverUID));
    assert(remoteServer->lowQoSQueueHandle != NULL);
    assert(remoteServer->highQoSQueueHandle != NULL);

    strncpy(pStatistics->serverUID, remoteServer->serverUID, sizeof(pStatistics->serverUID)-1);
    pStatistics->serverUID[sizeof(pStatistics->serverUID)-1] = '\0';

    ieq_getStats(pThreadData, remoteServer->lowQoSQueueHandle, &pStatistics->q0);
    ieq_getStats(pThreadData, remoteServer->highQoSQueueHandle, &pStatistics->q1);

    // If there is no entry for this server in the out-of-sync list, consider ourselves in-sync
    // with this server.
    pStatistics->retainedSync = (iers_getOutOfSyncTime(pThreadData,
                                                       remoteServer->serverUID,
                                                       &outOfSyncTime) == ISMRC_NotFound);

    iers_releaseServer(pThreadData, remoteServer);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "outOfSyncTime=%lu, rc=%d\n", __func__,
               outOfSyncTime, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of engineMonitoring.c                                         */
/*                                                                   */
/*********************************************************************/
