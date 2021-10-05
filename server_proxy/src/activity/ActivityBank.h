/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
 * ActivityBank.h
 *
 *  Created on: Nov 7, 2017
 */

#ifndef ACTIVITYBANK_H_
#define ACTIVITYBANK_H_

#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <set>
#include <list>

#include "pxactivity.h"
#include "ActivityUtils.h"

namespace ism_proxy
{

class ActivityBank
{
public:

	/**
	 *
	 * Eviction algorithm nth_element-deque is the fastest, hence the default.
	 *
	 * @param num identifier
	 * @param memoryLimitBytes memory limit in bytes, 0=unlimited
	 * @param evictionAlgo 0=quickselect, 1=max-heap, 2=nth_element-deque, 3=quickselect_deque
	 */
	ActivityBank(int num, std::size_t memoryLimitBytes = 0, int evictionAlgo = 2);

	virtual ~ActivityBank();

	void registerActivityNotify(ActivityNotify* actNotify);

	void setEnable(bool enable);

	bool isEnabled() const;

	void clear();

	void setClientTypePolicy(PXACT_CLIENT_TYPE_t client_type, int policy);

    int clientActivity(
    		const ismPXACT_Client_t* pClient);

    int clientConnectivity(
    		const ismPXACT_Client_t* pClient,
    		const ismPXACT_ConnectivityInfo_t* pConnInfo);

    int getUpdateBatch(
			ClientUpdateVector& updateVec,
			uint32_t& numUpdates);



    std::size_t getNumClients() const;
    std::size_t getSizeBytes() const;
    std::size_t getUpdateQueueSize() const;

    void setMemoryLimitBytes(std::size_t limit);
    std::size_t getMemoryLimitBytes() const;

    /**
     * Reduce the amount of memory below the limit.
     * Try to throw away 5% of the LRU clients.
     * @param algorithm 1=quickselect, otherwise=max-heap
     * @return
     */
    int manageMemory();

    /**
     * Evict (remove) k LRU clients, which are clean.
     * @param k number of clients to evict
     * @param algorithm 1=quickselect, otherwise=max-heap
     * @return number actually evicted.
     */
    std::size_t evictClients(const std::size_t k);

    //=========================================================================

    /**
     * Get stats.
     *
     * @return
     */
    ActivityStats getActivityStats();


	/**
	 * Call by periodic task that updates the timestamp by reading TSC clock.
	 * Call approx. every 10ms.
	 */
    void updateLazyTimestamp();

private:

    const int num_;

    mutable std::mutex mutex_;

    bool enabled_;

    using ClientUpdateMap = std::unordered_map<const char*, ClientUpdateRecord_SPtr, CStringMurmur3Hash, CStringEquals>;
    ClientUpdateMap client_update_map_;

    using UpdateList = std::list<ClientUpdateRecord_SPtr>;

    using UpdateQueue = std::deque<ClientUpdateRecord_SPtr>;
    UpdateQueue update_queue_;

    UpdateQueue lru_deque_;
    int evictionAlgo_;

    const double lazyTimestamp_First_;
    double lazyTimestamp_Last_;
    double lazyTimestamp_;

    std::size_t totalBytes_;

    ActivityNotify* actNotify_;

    ActivityStats stats_;
    double sum_conflation_delay_sec_;
    std::size_t sum_conflation_factor_;
    std::size_t num_conflation_ops_;


    double averageQueueSize_;

    std::size_t memoryLimitBytes_;

    int clientTypePolicy_[PXACT_CLIENT_TYPE_OTHER+1] = {0};

    using LastActive = std::pair<double,const char*>;
    using LRU_Heap = std::priority_queue<LastActive>;

    static bool updateMetadata(const ismPXACT_Client_t* pClient, const ismPXACT_ConnectivityInfo_t* pConnInfo, ClientUpdateRecord_SPtr& record);
    static bool updateGatewayID(const char* pGwID, std::string& gatewayID);

    void notifyWorker();

    /**
     * A cheap but (very) inaccurate monotonically increasing time-stamp.
     * The timestamp starts at 0 when the object is constructed.
     * Based on reading the TSC clock every 10K updates, and a periodic task that updates the timestamp by reading TSC clock.
     *
     * This method is not synchronized, it is always read when the lock is already taken.
     * @return
     */
    double getLazyTimestamp();

    std::size_t evictClients_heap(const std::size_t k);
    std::size_t evictClients_quickselect(const std::size_t k);
    std::size_t evictClients_nth_element_deque(const std::size_t k);
    std::size_t evictClients_quickselect_deque(const std::size_t k);

};

using ActivityBankSPtr = std::shared_ptr<ActivityBank>;
using ActivityBanksVector = std::vector<ActivityBankSPtr>;

} /* namespace ism_proxy */

#endif /* ACTIVITYBANK_H_ */
