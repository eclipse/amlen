/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

/*
 * CleanupWorker.h
 *  Created on: Mar 4, 2018
 */

#ifndef CLEANUPWORKER_H_
#define CLEANUPWORKER_H_

#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "ActivityUtils.h"
#include "ActivityDB.h"

namespace ism_proxy
{

class CleanupWorker
{
public:
    CleanupWorker(int batchSize, ActivityDB& activityDB, FailureNotify* failureNotify);
    virtual ~CleanupWorker();

    int start();

    void run();

    int stop();

    ActivityStats getActivityStats();

    void addProxy(const std::string& proxy_uid);

    /**
     *
     * @param rate_iops Valid >=1, 0=unlimited
     * @param refill_interval_ms
     * @return
     */
    int initRateLimit(uint32_t rate_iops, uint32_t max_rate_iops, uint32_t refill_interval_ms);

    /**
     *
     * @param rate_iops Valid >=1, 0=unlimited
     * @param refill_interval_ms
     * @return
     */
    int updateRateLimit(uint32_t rate_iops);

    int tokenBucketRefill();


private:
    const char* thread_name_ = "pxact.clean";
    int batchSize_;
    ActivityDB& activityDB_;

    ActivityDBClient_SPtr dbClient_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> work_;
    bool started_;
    bool finish_;

    std::unique_ptr<TokenBucket> tokenBucket_; //TODO

    ActivityStats stats_;
    double sum_read_latency_sec_;
    std::size_t num_read_latency_;

    FailureNotify* failureNotify_;

    StringSet proxies_high_priority_;
    StringSet proxies_low_priority_;

    bool isWork() const;

    bool isFinish() const;

    int cleanClientProxy();

    int cleanProxy();

    static std::string randSelect(const StringSet& set);
};

} /* namespace ism_proxy */

#endif /* CLEANUPWORKER_H_ */
