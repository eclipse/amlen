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
 * ActivityWorker.h
 *  Created on: Nov 13, 2017
 */

#ifndef ACTIVITYWORKER_H_
#define ACTIVITYWORKER_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "ActivityUtils.h"
#include "ActivityBank.h"
#include "ActivityDB.h"

namespace ism_proxy
{

class ActivityWorker : public ActivityNotify
{
public:
	ActivityWorker(int num, int numBanksPerWorker, int batchSize, ActivityBanksVector& banksVec, ActivityDB& activityDB, FailureNotify* failureNotify);
	virtual ~ActivityWorker();

	int start();

	void run();

	int stop();

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

	/**
	 * @see ActivityNotify
	 */
	virtual void notifyWork();



	ActivityStats getActivityStats();

private:
	const int num_;
	char thread_name_[20] = {0};
    ActivityBanksVector myBanksVec_;
	ActivityDB& activityDB_;

	ClientUpdateVector updateVec_;
	ActivityDBClient_SPtr dbClient_;
	std::string proxy_uid_;
	std::thread thread_;
	mutable std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> work_;
	bool started_;
	bool finish_;
	bool error_;

	std::unique_ptr<TokenBucket> tokenBucket_;

	ClientStateRecord client_state_record_;

	ActivityStats stats_;
	double sum_read_latency_sec_;
	std::size_t num_read_latency_;
	double sum_update_latency_sec_;
	std::size_t num_update_latency_;
	double sum_bulk_update_latency_sec_;
	std::size_t num_bulk_update_latency_;

	FailureNotify* failureNotify_;

	bool isFinish();
	bool isWork();

	int write_new_record(ClientUpdateRecord_SPtr& update);
	int write_conn_ev(ClientUpdateRecord_SPtr& update);
	int rmw_conn_ev(ClientUpdateRecord_SPtr& update);

	int report_potential_connection_stealing(ClientUpdateRecord_SPtr& update);
};


} /* namespace ism_proxy */

#endif /* ACTIVITYWORKER_H_ */
