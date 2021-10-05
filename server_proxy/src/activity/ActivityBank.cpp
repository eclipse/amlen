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
 * ActivityBank.cpp
 *
 *  Created on: Nov 7, 2017
 */

#include <chrono>
#include <algorithm>
#include <climits>
#include "ActivityBank.h"

namespace ism_proxy
{

ActivityBank::ActivityBank(int num, std::size_t memoryLimitBytes, int evictionAlgo) :
		num_(num),
		enabled_(true),
		evictionAlgo_(evictionAlgo),
		lazyTimestamp_First_(ism_common_readTSC()),
		lazyTimestamp_Last_(0),
		lazyTimestamp_(lazyTimestamp_Last_),
		totalBytes_(0),
		actNotify_(NULL),
		sum_conflation_delay_sec_(0),
		sum_conflation_factor_(0),
		num_conflation_ops_(0),
		averageQueueSize_(0),
		memoryLimitBytes_(memoryLimitBytes)


{
	TRACE(5, "%s B%d this=%p\n", __FUNCTION__, num_, this);
}

ActivityBank::~ActivityBank()
{
	TRACE(7, "%s B%d\n", __FUNCTION__, num_);
	client_update_map_.clear();
	update_queue_.clear();
	lru_deque_.clear();
}

void ActivityBank::registerActivityNotify(ActivityNotify* actNotify)
{
	TRACE(7, "%s B%d, actNotify=%p\n", __FUNCTION__, num_, actNotify);
	LockGuard lock(mutex_);
	actNotify_ = actNotify;
}

void ActivityBank::setEnable(bool enable)
{
    TRACE(7, "%s B%d, enable=%s\n", __FUNCTION__, num_, (enable ? "T" : "F"));
    LockGuard lock(mutex_);
    enabled_ = enable;
}

bool ActivityBank::isEnabled() const
{
    LockGuard lock(mutex_);
    return enabled_;
}

void ActivityBank::clear()
{
    TRACE(5, "%s B%d \n", __FUNCTION__, num_);
    LockGuard lock(mutex_);
    client_update_map_.clear();
    update_queue_.clear();
    lru_deque_.clear();
    totalBytes_ = 0;
    stats_.avg_conflation_delay_ms = 0;
    stats_.avg_conflation_factor = 0;
    stats_.avg_db_read_latency_ms = 0;
    stats_.avg_db_update_latency_ms = 0;
    stats_.avg_db_bulk_update_latency_ms = 0;
    stats_.memory_bytes = 0;
    stats_.num_clients = 0;
}

void ActivityBank::setClientTypePolicy(PXACT_CLIENT_TYPE_t client_type, int policy)
{
    TRACE(7, "%s B%d, client_type=%d policy=%d\n", __FUNCTION__, num_, client_type, policy);
    LockGuard lock(mutex_);
    clientTypePolicy_[client_type] = policy;
}

int ActivityBank::clientActivity(
		const ismPXACT_Client_t* pClient)
{
    PXACT_ACTIVITY_TYPE_t activity_type = pClient->activityType;

	TRACE(9, "%s Entry: B%d, client=%s, act_type=%d\n", __FUNCTION__, num_, toString(pClient).c_str(), activity_type);

	LockGuard lock(mutex_);

	if (!enabled_)
	{
	    TRACE(9,"%s Exit: B%d, disabled. rc=%d.\n", __FUNCTION__, num_, ISMRC_Closed);
	    return ISMRC_Closed;
	}

	const int monPolicy = clientTypePolicy_[pClient->clientType];
	if (monPolicy < 0)
	{
        TRACE(9,"%s Exit: client_type %d monitoring disabled, ignoring; rc=%d.\n",
                __FUNCTION__, ISMRC_OK, pClient->clientType);
	    return ISMRC_OK;
	}

	ClientUpdateMap::iterator pos = client_update_map_.find(pClient->pClientID);
	if (pos != client_update_map_.end())
	{
		TRACE(9,"%s Record before: %s/n", __FUNCTION__, pos->second->toString().c_str());
		if (strcmp(pClient->pOrgName, pos->second->org->c_str()) != 0)
		{
			TRACE(1, "Error: %s OrgName changed, arg=%s, map=%s; rc=%d\n",
					__FUNCTION__, pClient->pOrgName, pos->second->org->c_str(), ISMRC_Error);
			return ISMRC_Error;
		}

		pos->second->last_act_type = activity_type;
		uint8_t dirty_flags_before = pos->second->dirty_flags;
		pos->second->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
		double now_ts = getLazyTimestamp();
		if (monPolicy > 0 && (now_ts - pos->second->update_timestamp) >  monPolicy)
		{
		    TRACE(9,"%s Record expired in cache, possibly in DB, now=%f, update=%f, monPolicy=%d, setting DIRTY_FLAG_NEW_RECORD: %s/n",
		            __FUNCTION__, now_ts, pos->second->update_timestamp, monPolicy, pos->second->toString().c_str());
		    //It expired in the DB, so treat it as new record, will trigger an insert rather than an update
		    pos->second->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
		}
		pos->second->update_timestamp = now_ts;
		pos->second->num_updates++;

		if (updateMetadata(pClient, NULL, pos->second))
		{
			TRACE(9,"%s Metadata changed, record after: %s\n", __FUNCTION__, pos->second->toString().c_str());
		}

		if (dirty_flags_before == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
		{
			update_queue_.push_back(pos->second);
			pos->second->queuing_timestamp = pos->second->update_timestamp;
			notifyWorker();
			TRACE(9,"%s Clean to dirty, pushed to updateQueue\n", __FUNCTION__);
		}
	}
	else
	{
		//New record
		ClientUpdateRecord_SPtr record(
				new ClientUpdateRecord(
						pClient->pClientID,
						pClient->pOrgName,
						pClient->pDeviceType,
						pClient->pDeviceID,
						pClient->clientType,
						pClient->pGateWayID));
		record->last_act_type = activity_type;
		record->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
		record->update_timestamp = getLazyTimestamp();
		record->num_updates++;

		//The key c-string is a pointer to the value client_id, be careful when erasing
		client_update_map_[record->client_id->c_str()]= record;
		update_queue_.push_back(record);
		record->queuing_timestamp = record->update_timestamp;
		if (evictionAlgo_>= 2)
		{
			lru_deque_.push_back(record);
		}
		notifyWorker();
		//key-pointer+ recored-size + record-shared-pointer
		totalBytes_ += record->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr);
		TRACE(9,"%s New record, pushed to updateQueue: %s/n", __FUNCTION__, record->toString().c_str());
		stats_.num_new_clients++;
		stats_.num_clients++;
	}

	stats_.num_activity++;

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, ISMRC_OK);
	return ISMRC_OK;
}

int ActivityBank::clientConnectivity(
		const ismPXACT_Client_t* pClient,
		const ismPXACT_ConnectivityInfo_t* pConnInfo)
{
	TRACE(9, "%s Entry: B%d, client=%s\n", __FUNCTION__, num_, toString(pClient).c_str());

    PXACT_ACTIVITY_TYPE_t act_type = pClient->activityType;
	bool connect;
	if (act_type >= PXACT_ACTIVITY_TYPE_CONN && act_type <= PXACT_ACTIVITY_TYPE_CONN_END )
	{
		connect = true;
	}
	else if (act_type >= PXACT_ACTIVITY_TYPE_DISCONN && act_type <= PXACT_ACTIVITY_TYPE_DISCONN_END)
	{
		connect = false;
	}
	else
	{
		TRACE(1, "Error: %s Illegal activity type=%d; rc=%d\n",	__FUNCTION__, act_type, ISMRC_Error);
		return ISMRC_Error;
	}

	LockGuard lock(mutex_);

	if (!enabled_)
	{
	    TRACE(9,"%s Exit: B%d, disabled. rc=%d.\n", __FUNCTION__, num_, ISMRC_Closed);
	    return ISMRC_Closed;
	}

	const int monPolicy = clientTypePolicy_[pClient->clientType];
    if (monPolicy < 0)
    {
        TRACE(9,"%s Exit: client_type %d monitoring disabled, ignoring; rc=%d.\n",
                __FUNCTION__, ISMRC_OK, pClient->clientType);
        return ISMRC_OK;
    }

	ClientUpdateMap::iterator pos = client_update_map_.find(pClient->pClientID);
	if (pos != client_update_map_.end())
	{
		TRACE(9,"%s Record before: %s\n", __FUNCTION__, pos->second->toString().c_str());
		if (strcmp(pClient->pOrgName, pos->second->org->c_str()) != 0)
		{
			TRACE(1, "Error: %s OrgName changed, arg=%s, map=%s; rc=%d\n",
					__FUNCTION__, pClient->pOrgName, pos->second->org->c_str(), ISMRC_Error);
			return ISMRC_Error;
		}

        uint8_t dirty_flags_before = pos->second->dirty_flags;

        //Always only connect / disconnect event comes into this function
        if (connect)
        {
        	pos->second->conn_ev_type = act_type;
        	pos->second->disconn_ev_type = PXACT_ACTIVITY_TYPE_NONE;
        }
        else
        {
        	pos->second->disconn_ev_type = act_type;
        	pos->second->conn_ev_type = PXACT_ACTIVITY_TYPE_NONE;
        }

		//connect events as well as explicit client disconnect count also as activity
		if (connect || act_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT)
		{
		    pos->second->last_act_type = act_type;
		    pos->second->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
		}
		pos->second->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT;
		if (connect) //If it is connect event get time from protocol
		{
			pos->second->connect_time = pConnInfo->connectTime/1.0e6; //in milliseconds
		}
		else  //For disconnect event get the current time
		{
			pos->second->disconnect_time = ism_common_currentTimeNanos()/1.0e6;//in milliseconds
		}
		double now_ts = getLazyTimestamp();
        if (monPolicy > 0 && (now_ts - pos->second->update_timestamp) >  monPolicy)
        {
            //It expired in the DB, so treat it as new record, will trigger an insert rather than an update
            TRACE(9,"%s Record expired in cache, possibly in DB, now=%f, update=%f, monPolicy=%d, setting DIRTY_FLAG_NEW_RECORD: %s\n",
                    __FUNCTION__, now_ts, pos->second->update_timestamp, monPolicy, pos->second->toString().c_str());
            pos->second->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
        }
        pos->second->update_timestamp = now_ts;
        pos->second->num_updates++;

		if (ActivityBank::updateMetadata(pClient, pConnInfo, pos->second))
		{
			TRACE(9,"%s Metadata changed, record after: %s\n", __FUNCTION__, pos->second->toString().c_str());
		}

		TRACE(9,"%s %s Dirty flags before %x now %x\n", __FUNCTION__, (connect ? "Connect" : "Disconnect"), dirty_flags_before, pos->second->dirty_flags);

		if (dirty_flags_before == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
		{
			update_queue_.push_back(pos->second);
			pos->second->queuing_timestamp = pos->second->update_timestamp;
			notifyWorker();
			//TRACE(9,"%s %s Clean to dirty, pushed to updateQueue\n", __FUNCTION__, (connect ? "Connect" : "Disconnect"));
			TRACE(9,"%s %s Pushed to updateQueue\n", __FUNCTION__, (connect ? "Connect" : "Disconnect"));
		}
	}
	else
	{
		//New record
		ClientUpdateRecord_SPtr record(
				new ClientUpdateRecord(
						pClient->pClientID,
						pClient->pOrgName,
						pClient->pDeviceType,
						pClient->pDeviceID,
						pClient->clientType,
						pClient->pGateWayID));
		if (connect)
		{
			record->conn_ev_type = act_type;
		}
		else
		{
			record->disconn_ev_type = act_type;
		}
		record->clean_s = pConnInfo->cleanS;
		record->protocol = pConnInfo->protocol;
		record->reason_code = pConnInfo->reasonCode;
		record->expiry_sec = static_cast<int32_t>(pConnInfo->expiryIntervalSec);
		record->target_server = pConnInfo->targetServerType;
		record->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT;
		//connect events as well as explicit client disconnect count also as activity
        if (connect || act_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT)
        {
            record->last_act_type = act_type;
            record->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
        }
        if (connect) //If it is connect event get time from protocol
        {
        	record->connect_time = pConnInfo->connectTime/1.0e6; //in milliseconds
        }
        else  //disconnect event get the current time
        {
        	record->disconnect_time = ism_common_currentTimeNanos()/1.0e6; //in milliseconds
        }

		record->update_timestamp = getLazyTimestamp();
		record->num_updates++;

		//The key c-string is a pointer to the value client_id, be careful when erasing
		client_update_map_[record->client_id->c_str()]= record;
		update_queue_.push_back(record);
		record->queuing_timestamp = record->update_timestamp;
		if (evictionAlgo_>= 2)
		{
			lru_deque_.push_back(record);
		}
		notifyWorker();
		//key-pointer+ recored-size + record-shared-pointer
		totalBytes_ += (record->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr));
		TRACE(9,"%s %s New record, pushed to updateQueue: %s\n", __FUNCTION__, (connect ? "Connect" : "Disconnect"), record->toString().c_str());
		stats_.num_new_clients++;
		stats_.num_clients++;
	}

	stats_.num_connectivity++;

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, ISMRC_OK);
	return ISMRC_OK;

}

/*
 * Pre-condition: no holes
 * Post-condition: no holes
 */
int ActivityBank::getUpdateBatch(
			ClientUpdateVector& updateVec,
			uint32_t& numUpdates)
{
	TRACE(8, "%s Entry: vec.size=%ld\n", __FUNCTION__, updateVec.size());
	int rc = ISMRC_OK;

	{
		std::unique_lock<std::mutex> ulock(mutex_);
		uint32_t index = 0;
		uint32_t num = 0;
		bool done = false;

		while (!done && (num < updateVec.size()) && !update_queue_.empty() && (index < update_queue_.size()))
		{
		    ClientUpdateRecord_SPtr record;
		    if (index == 0)
		    {
		        record = update_queue_.front();
		    }
		    else
		    {
		        record = update_queue_[index];
		    }

			if (record)
			{
			    bool is_conn_new = (record->dirty_flags & (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD));
			    if (num == 0)
			    {
			        *updateVec[num] = *record; //copy
			        record->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CLEAN;
			        update_queue_.pop_front();
			        sum_conflation_delay_sec_ += (getLazyTimestamp() - record->queuing_timestamp);
			        num_conflation_ops_++;
			        sum_conflation_factor_ += record->num_updates;
			        record->num_updates = 0;

			        ++num;
			        if (is_conn_new)
			        {
			            done = true;
			        }
			    }
			    else
			    {
			        if (is_conn_new)
			        {
			            ++index; //skip it
			        }
			        else
			        {
			            *updateVec[num] = *record; //copy
			            record->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CLEAN;
			            if (index == 0)
			            {
			                update_queue_.pop_front();
			            }
			            else
			            {
			                update_queue_[index].reset();
			                ++index;
			            }
			            sum_conflation_delay_sec_ += (getLazyTimestamp() - record->queuing_timestamp);
			            num_conflation_ops_++;
			            sum_conflation_factor_ += record->num_updates;
			            record->num_updates = 0;

			            ++num;
			        }
			    }

			    TRACE(8,"%s done=%d, num=%d, index=%d, record=%s\n",__FUNCTION__, done, num, index, record->toString().c_str());
			}
			else if (index == 0)
			{
			    update_queue_.pop_front();
			    TRACE(8,"%s done=%d, num=%d, index=%d, skipping empty record at front.\n",__FUNCTION__, done, num, index);
			}
			else
			{
			    rc = ISMRC_Error;
			    TRACE(1,"Error: %s empty record not at front, done=%d, num=%d, index=%d, \n",__FUNCTION__, done, num, index);
			    break;
			}
		}//while

		//eliminate holes
		if (rc == ISMRC_OK)
		{
		    numUpdates = num;
		    ClientUpdateVector tmpVec;
            for (unsigned i=0; i < index; ++i)
            {
                if (!update_queue_.empty())
                {
                    ClientUpdateRecord_SPtr record = update_queue_.front();
                    update_queue_.pop_front();
                    if (record)
                    {
                        tmpVec.push_back(record);
                    }
                }
            }

            while (!tmpVec.empty())
            {
                ClientUpdateRecord_SPtr record = tmpVec.back();
                tmpVec.pop_back();
                update_queue_.push_front(record);
            }
		}

	}

	TRACE(8, "%s Exit: numUpdates=%d, rc=%d\n", __FUNCTION__, numUpdates, rc);
	return rc;
}

int ActivityBank::manageMemory()
{
    std::size_t limit = getMemoryLimitBytes();
	if ( limit > 0)
	{
		std::size_t total_before = getSizeBytes();

		if (total_before > limit)
		{
		    std::size_t num_clients = getNumClients();
		    if (num_clients > 0)
		    {
		        std::size_t mem_per_clients = total_before/num_clients;
		        std::size_t num2evict = (total_before - limit*19/20)/mem_per_clients;
		        std::size_t num_evicted = evictClients(num2evict);
		        std::size_t total_after = getSizeBytes();
		        TRACE(7, "%s B%d, evicting clients (algo=%d): #clients-before=%ld, #requested=%ld, #actual=%ld; mem-before=%ld, cleared=%ld Bytes.\n",
		                __FUNCTION__, num_, evictionAlgo_, num_clients, num2evict, num_evicted, total_before, (total_before-total_after));
		    }
		}
	}
	return ISMRC_OK;
}

std::size_t ActivityBank::getNumClients() const
{
	LockGuard lock(mutex_);
	return client_update_map_.size();
}

std::size_t ActivityBank::getSizeBytes() const
{
	LockGuard lock(mutex_);
	return totalBytes_;
}

std::size_t ActivityBank::getUpdateQueueSize() const
{
	LockGuard lock(mutex_);
	return update_queue_.size();
}


void ActivityBank::setMemoryLimitBytes(std::size_t limit)
{
    LockGuard lock(mutex_);
    memoryLimitBytes_ = limit;
}


std::size_t ActivityBank::getMemoryLimitBytes() const
{
    LockGuard lock(mutex_);
	return memoryLimitBytes_;
}

std::size_t ActivityBank::evictClients(const std::size_t k)
{
    if (k==0)
        return ISMRC_OK;

    switch (evictionAlgo_)
	{
	case 1:
	{
		return evictClients_heap(k);
	}

	case 2:
	{
		return evictClients_nth_element_deque(k);
	}

	case 3:
	{
		return evictClients_quickselect_deque(k);
	}

	default:
		return evictClients_quickselect(k);
	}
}

std::size_t ActivityBank::evictClients_heap(const std::size_t k)
{
	using namespace std;

	TRACE(8,"%s Entry: B%d k=%ld \n", __FUNCTION__, num_, k);

	LRU_Heap heap;
	size_t map_size = 0;

	auto start = chrono::system_clock::now();
	{
		LockGuard lock(mutex_);
		map_size = client_update_map_.size();
		for (ClientUpdateMap::const_iterator it = client_update_map_.begin(); it != client_update_map_.end(); ++it)
		{
			if ((*it).second->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
			{
				if (heap.size() < k)
				{
					heap.push(make_pair((*it).second->update_timestamp, (*it).first));
				}
				else
				{
					LRU_Heap::const_reference top = heap.top();
					if ((*it).second->update_timestamp <= top.first )
					{
						heap.push(make_pair((*it).second->update_timestamp, (*it).first));
						heap.pop();
					}
				}
			}
		}
	}

	if (!heap.empty())
	{
		LastActive k_th = heap.top();
		TRACE(7, "%s B%d The %ld-th smallest item of %ld is (%s), removing all items with timestamp <= (%f) \n",
				__FUNCTION__, num_, k, map_size, (k_th.second ? k_th.second : "NULL)"), k_th.first);
	}

	auto mid = chrono::system_clock::now();
	size_t removed = 0;

	//Either remove by name, or remove by iteration and comparison to heap.top.first
	while (!heap.empty())
	{
		LRU_Heap::const_reference top = heap.top();
		{
			LockGuard lock(mutex_);
			ClientUpdateMap::iterator pos = client_update_map_.find(top.second);
			if (pos != client_update_map_.end())
			{
				ClientUpdateRecord_SPtr rec = pos->second;
				if (rec->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN && rec->update_timestamp <= top.first)
				{
					client_update_map_.erase(pos);
					removed++;
					stats_.num_evict_clients++;
					stats_.num_clients--;
					//key-pointer+ recored-size + record-shared-pointer
					totalBytes_ -= (rec->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr));
				}
			}
		}
		heap.pop();
	}

	auto stop = chrono::system_clock::now();

	TRACE(8,"%s Exit: B%d removed=%ld, total=%fs, find=%fs, erase=%fs \n",
			__FUNCTION__, num_, removed,
			chrono::duration<double>(stop-start).count(),
			chrono::duration<double>(mid-start).count(),
			chrono::duration<double>(stop-mid).count());

	return removed;
}

std::size_t ActivityBank::evictClients_quickselect(const std::size_t k)
{
	using namespace std;

	TRACE(9,"%s Entry: B%d k=%ld \n", __FUNCTION__, num_, k);

	auto start = chrono::system_clock::now();
	vector<LastActive> array;
	array.reserve(max(k,getNumClients()-getUpdateQueueSize()));

	{
		LockGuard lock(mutex_);
		for (ClientUpdateMap::const_iterator it = client_update_map_.begin(); it != client_update_map_.end(); ++it)
		{
			if ((*it).second->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
			{
				array.push_back(make_pair((*it).second->update_timestamp, (*it).first));
			}
		}
	}

	auto copy = chrono::system_clock::now();

	if (array.empty())
	{
		TRACE(8,"%s Exit: B%d nothing to evict \n", __FUNCTION__, num_);
		return 0;
	}

	LastActive k_th = ism_proxy::quickselect<LastActive>(array,k);

	TRACE(7, "%s B%d The %ld-th smallest item of %ld is (%s), removing all items with timestamp <= (%f) \n",
			__FUNCTION__, num_, k, array.size(), (k_th.second ? k_th.second : "NULL)"), k_th.first);

	auto select = chrono::system_clock::now();

	size_t removed = 0;

	for (unsigned j=0; j<k; ++j)
	{
		{
			LockGuard lock(mutex_);
			ClientUpdateMap::iterator pos = client_update_map_.find(array[j].second);
			if (pos != client_update_map_.end())
			{
				ClientUpdateRecord_SPtr rec = pos->second;
				if (rec->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN && rec->update_timestamp <= array[j].first)
				{
					client_update_map_.erase(pos);
					removed++;
					stats_.num_evict_clients++;
					stats_.num_clients--;
					//key-pointer+ recored-size + record-shared-pointer
					totalBytes_ -= (rec->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr));
				}
			}
		}
	}

	auto stop = chrono::system_clock::now();

	TRACE(8,"%s Exit: B%d removed=%ld, total=%fs, copy=%fs, select=%fs, find=%fS, erase=%fS \n",
				__FUNCTION__, num_, removed,
				chrono::duration<double>(stop-start).count(),
				chrono::duration<double>(copy-start).count(),
				chrono::duration<double>(select-copy).count(),
				chrono::duration<double>(select-start).count(),
				chrono::duration<double>(stop-select).count());

	return removed;
}

std::size_t ActivityBank::evictClients_nth_element_deque(const std::size_t k)
{
	using namespace std;

	TRACE(9, "%s Entry: B%d k=%ld \n", __FUNCTION__, num_, k);

	auto start = chrono::system_clock::now();

	size_t N;
	const char* kth_key = NULL;
	double kth_timestamp;
	{
		LockGuard lock(mutex_);
		N = lru_deque_.size();
		UpdateQueue::iterator k_th = lru_deque_.begin() + k - 1;
		nth_element(lru_deque_.begin(), k_th, lru_deque_.end(), compare_ts_less);
		kth_key = (*k_th)->client_id->c_str();
		kth_timestamp = (*k_th)->update_timestamp;
	}

	auto select = chrono::system_clock::now();

	TRACE(7, "%s B%d The %ld-th smallest item of %ld is (%s), removing all items with timestamp <= (%f) \n",
			__FUNCTION__, num_, k, N, (kth_key ? kth_key : "NULL"), kth_timestamp);

	size_t removed = 0;
	for (auto i = k; i > 0; --i)
	{
		{
			LockGuard lock(mutex_);
			ClientUpdateRecord_SPtr rec = lru_deque_.front();
			lru_deque_.pop_front();
			if (rec->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN && rec->update_timestamp <= kth_timestamp)
			{
				client_update_map_.erase(rec->client_id->c_str());
				removed++;
				stats_.num_evict_clients++;
				stats_.num_clients--;
				//key-pointer+ recored-size + record-shared-pointer
				totalBytes_ -= (rec->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr));
			}
			else
			{
				lru_deque_.push_back(rec);
			}
		}
	}

	auto stop = chrono::system_clock::now();

	TRACE(8, "%s Exit: B%d removed=%ld, total=%fs, select=%fs, erase=%fS \n", __FUNCTION__, num_, removed,
			chrono::duration<double>(stop - start).count(),
			chrono::duration<double>(select - start).count(),
			chrono::duration<double>(stop - select).count());

	return removed;
}


std::size_t ActivityBank::evictClients_quickselect_deque(const std::size_t k)
{
	using namespace std;

	TRACE(9, "%s B%d Entry: k=%ld \n", __FUNCTION__, num_, k);

	auto start = chrono::system_clock::now();

	size_t N;
	const char* kth_key;
	double kth_timestamp;
	{
		LockGuard lock(mutex_);
		N = lru_deque_.size();
		ClientUpdateRecord_SPtr kth_rec  = quickselect<ClientUpdateRecord_SPtr, UpdateQueue, Compare_TS_Less>(lru_deque_,k);
		kth_key = kth_rec->client_id->c_str();
		kth_timestamp = kth_rec->update_timestamp;
	}

	auto select = chrono::system_clock::now();

	TRACE(7, "%s B%d The %ld-th smallest item of %ld is (%s), removing all items with timestamp <= (%f) \n",
			__FUNCTION__, num_, k, N, (kth_key ? kth_key : "NULL)"), kth_timestamp);

	size_t removed = 0;
	for (auto i = k; i > 0; --i)
	{
		{
			LockGuard lock(mutex_);
			ClientUpdateRecord_SPtr rec = lru_deque_.front();
			lru_deque_.pop_front();
			if (rec->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN && rec->update_timestamp <= kth_timestamp)
			{
				client_update_map_.erase(rec->client_id->c_str());
				removed++;
				stats_.num_evict_clients++;
				stats_.num_clients--;
				//key-pointer+ recored-size + record-shared-pointer
				totalBytes_ -= (rec->getSizeBytes() + 8 + sizeof(ClientUpdateRecord_SPtr));
			}
			else
			{
				lru_deque_.push_back(rec);
			}
		}
	}

	auto stop = chrono::system_clock::now();

	TRACE(8, "%s Exit: B%d removed=%ld, total=%fs, select=%fs, erase=%fS \n", __FUNCTION__, num_, removed,
			chrono::duration<double>(stop - start).count(),
			chrono::duration<double>(select - start).count(),
			chrono::duration<double>(stop - select).count());

	return removed;
}


bool ActivityBank::updateMetadata(
		const ismPXACT_Client_t* pClient,
		const ismPXACT_ConnectivityInfo_t* pConnInfo,
		ClientUpdateRecord_SPtr& record)
{
	bool changed = false;

	if (pClient->clientType != record->client_type)
	{
		record->client_type = pClient->clientType;
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (pConnInfo && (pConnInfo->protocol != record->protocol))
	{
		record->protocol = pConnInfo->protocol;
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (pConnInfo && (pConnInfo->cleanS != record->clean_s))
	{
		record->clean_s = pConnInfo->cleanS;
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (pConnInfo && (pConnInfo->targetServerType != record->target_server))
	{
		record->target_server = pConnInfo->targetServerType;
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (pConnInfo && (pConnInfo->expiryIntervalSec != static_cast<uint32_t>(record->expiry_sec)))
	{
		record->expiry_sec = static_cast<int32_t>(pConnInfo->expiryIntervalSec);
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (pConnInfo && (pConnInfo->reasonCode != static_cast<uint32_t>(record->reason_code)))
	{
		record->reason_code = static_cast<int32_t>(pConnInfo->reasonCode);
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	if (updateGatewayID(pClient->pGateWayID, *(record->gateway_id)))
	{
		record->dirty_flags |= record->DIRTY_FLAG_METADATA;
		changed = true;
	}

	return changed;
}

bool ActivityBank::updateGatewayID(const char* pGwID, std::string& gatewayID)
{
	if (pGwID)
	{
		if (strcmp(pGwID, gatewayID.c_str()) != 0)
		{
			gatewayID = pGwID;
			return true;
		}
	}
	else if (!gatewayID.empty())
	{
		gatewayID.clear();
		return true;
	}

	return false;
}

double ActivityBank::getLazyTimestamp()
{
	static const double delta = 0.000001; //1uS
	static const double gap = 0.01; //10mS

	lazyTimestamp_ += delta;

	if (lazyTimestamp_ > lazyTimestamp_Last_ + gap)
	{
		lazyTimestamp_ = std::max(lazyTimestamp_, ism_common_readTSC()-lazyTimestamp_First_);
		lazyTimestamp_Last_ = lazyTimestamp_;
	}
	return lazyTimestamp_;
}

void ActivityBank::updateLazyTimestamp()
{
	LockGuard lock(mutex_);
	lazyTimestamp_ = std::max(lazyTimestamp_, ism_common_readTSC()-lazyTimestamp_First_);
	lazyTimestamp_Last_ = lazyTimestamp_;
}

void ActivityBank::notifyWorker()
{
	TRACE(8, "%s Entry: B%d.\n", __FUNCTION__, num_);
	if (actNotify_)
	{
		actNotify_->notifyWork();
	}
	else
	{
		TRACE(3, "%s Exit: B%d notifyWork callback is NULL.\n", __FUNCTION__, num_);
	}
	TRACE(8, "%s Exit: B%d.\n", __FUNCTION__, num_);
}

ActivityStats ActivityBank::getActivityStats()
{
	LockGuard lock(mutex_);
	stats_.memory_bytes = totalBytes_;
	if (num_conflation_ops_>0)
	{
	    stats_.avg_conflation_delay_ms = static_cast<int64_t>(1000*sum_conflation_delay_sec_/num_conflation_ops_);
	    stats_.avg_conflation_factor = sum_conflation_factor_/num_conflation_ops_;
	}
	else
	{
	    stats_.avg_conflation_delay_ms = 0;
	    stats_.avg_conflation_factor = 0;
	}
	sum_conflation_delay_sec_ = 0;
	sum_conflation_factor_ = 0;
	num_conflation_ops_ = 0;

	return stats_;
}

} /* namespace ism_proxy */
