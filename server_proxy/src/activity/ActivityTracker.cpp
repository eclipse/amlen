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
 * ActivityTracker.cpp
 *  Created on: Oct 18, 2017
 */

#include <set>
#include <map>

#include "ActivityTracker.h"
#include "ActivityUtils.h"

namespace ism_proxy
{

ActivityTracker::ActivityTracker() :
				shutdownHandler_(NULL),
				state_(PXACT_STATE_TYPE_INIT),
				started_lock_free_(false),
				start_messaging_(false),
				term_drain_(false),
				timestamp_First_(ism_common_readTSC()),
				error_close_done_(false),
				heartbeat_task_(NULL),
				heartbeat_missed_(0),
				heartbeat_monitor_task_(NULL),
				tokenbucket_task_(NULL),
				lazy_clock_task_(NULL),
				numBanks_(ActivityDBConfig::NUM_BANKS_PER_WORKER_DEF * ActivityDBConfig::NUM_WORKER_THREADS_DEF),
				sum_read_latency_sec_(0),
				num_read_latency_(0),
				sum_update_latency_sec_(0),
				num_update_latency_(0),
				sum_bulk_update_latency_sec_(0),
				num_bulk_update_latency_(0),
				auto_restart_finish_(true)
{
	TRACE(5,"%s Entry: this=%p\n", __FUNCTION__, this);

    for (int i=0; i<MAX_BANKS; ++i)
    {
        ActivityBankSPtr bank = std::shared_ptr<ActivityBank>(new ActivityBank(i, 0));
        bank->setEnable(false);
        banksVec_.push_back(bank);
    }
}

ActivityTracker::~ActivityTracker()
{
}

int ActivityTracker::registerShutdownHandler(
		ismPXActivity_ShutdownHandler_t handler)
{
	TRACE(8,"%s Entry: handler=%p\n", __FUNCTION__, (void*)handler);
	int rc = ISMRC_OK;

	LockGuardRecur lock(mutex_);

	if (state_ != PXACT_STATE_TYPE_TERM)
	{
		shutdownHandler_ = handler;
	}
	else
	{
		rc = ISMRC_Closed;
	}

	return rc;
}

int ActivityTracker::configure(
		const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB)
{
	TRACE(8,"%s Entry: \n", __FUNCTION__);

	if (!pConfigActivityDB)
	{
		TRACE(1,"Error: %s, NULL pConfigActivityDB, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}

	int rc = ISMRC_OK;

	LockGuardRecur lock(mutex_);

	switch (state_)
    {
    case PXACT_STATE_TYPE_INIT:
    case PXACT_STATE_TYPE_CONFIG:
    {
         rc = configure_first(pConfigActivityDB);
    }
        break;

    case PXACT_STATE_TYPE_START:
    {
        rc = configure_during_start(pConfigActivityDB);
        TRACE(4,"Config: %s, reconfigured; rc=%d\n", __FUNCTION__, rc);
    }
        break;

    case PXACT_STATE_TYPE_STOP:
    {
        rc = configure_during_stop(pConfigActivityDB);
        TRACE(4,"Config: %s, reconfigured; rc=%d\n", __FUNCTION__, rc);
    }
        break;

    case PXACT_STATE_TYPE_TERM:
    {
        rc = ISMRC_Closed;
        TRACE(1,"Error: %s, already closed, rc=%d\n", __FUNCTION__, rc);
    }
        break;

    case PXACT_STATE_TYPE_ERROR:
    {
        if (error_close_done_)
        {
            rc = configure_during_stop(pConfigActivityDB);
        }
        else
        {
            rc = ISMRC_WouldBlock;
        }

        if (rc == ISMRC_OK)
        {
            TRACE(4,"Config: %s, reconfigured during error state; rc=%d\n", __FUNCTION__, rc);
        }
        else
        {
            TRACE(1,"Error: %s, failed to reconfigure during error state; rc=%d\n", __FUNCTION__, rc);
        }
    }
        break;

    default:
        rc = ISMRC_Error;
        TRACE(1, "Error: %s, Invalid state: %d, rc=%d\n", __FUNCTION__, state_, rc);
    }

	TRACE(9, "%s Exit: rc=%d \n", __FUNCTION__, rc);
	return rc;
}

int ActivityTracker::configure_first(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB)
{
    TRACE(9,"Entry: %s config=%s\n",__FUNCTION__, toString(pConfigActivityDB).c_str());

    ActivityDBConfig_SPtr prev_config;
    if (state_ == PXACT_STATE_TYPE_CONFIG)
    {
        prev_config = config_;
        TRACE(4,"%s Config: overriding previous config=%s\n",__FUNCTION__, prev_config->toString().c_str());
    }

    config_.reset(new ActivityDBConfig(pConfigActivityDB));
    TRACE(4,"%s Config: config=%s\n",__FUNCTION__, config_->toString().c_str());

    if (!(config_->numWorkerThreads>0 && config_->numBanksPerWorkerThread>0))
    {   //This can never happen because ActivityDBConfig() sets defaults for all values <=0.
        TRACE(1,"Error: %s, bad configuration: numWorkerThreads=%d, numBanksPerWorkerThread=%d, rc=%d\n",
                __FUNCTION__, config_->numWorkerThreads, config_->numBanksPerWorkerThread, ISMRC_Error);
        return ISMRC_Error;
    }

    if (config_->numWorkerThreads > MAX_WORKERS)
    {
        config_->numWorkerThreads = MAX_WORKERS;
        TRACE(4,"Config: %s, numWorkerThreads=%d > MAX_WORKERS=%d; set to MAX_WORKERS.\n",
                __FUNCTION__, config_->numWorkerThreads, MAX_WORKERS);
    }

    if (!start_messaging_) //Can change the number of banks only before start messaging
    {
        numBanks_ = config_->numWorkerThreads * config_->numBanksPerWorkerThread;
        if (numBanks_ > MAX_BANKS)
        {
            TRACE(4,"Config: %s, numWorkerThreads*numBanksPerWorkerThread=%d > MAX_BANKS=%d; adjusting numBanksPerWorkerThread.\n",
                    __FUNCTION__, numBanks_, MAX_BANKS);
            config_->numBanksPerWorkerThread = MAX_BANKS / config_->numWorkerThreads;
            numBanks_ = config_->numWorkerThreads * config_->numBanksPerWorkerThread;
            TRACE(4,"Config: %s, adjusted: numBanksPerWorkerThread=%d, numBanks=%d <= MAX_BANKS=%d.\n",
                    __FUNCTION__, config_->numBanksPerWorkerThread, numBanks_, MAX_BANKS);
        }
    }
    else
    {   //make sure number of threads & banks-per-thread is still consistent
        const int nb = config_->numWorkerThreads * config_->numBanksPerWorkerThread;
        if (numBanks_ != nb)
        {
            TRACE(4, "Config: %s, numWorkerThreads*numBanksPerWorkerThread=%d != numBanks=%d; re-adjusting.\n",
                    __FUNCTION__, nb, numBanks_);
            for (int t = 10; t > 0; --t)
            {
                int b = numBanks_ / t;
                if (numBanks_ == b * t) //will always be true when t==1
                {
                    config_->numWorkerThreads = t;
                    config_->numBanksPerWorkerThread = b;
                    break;
                }
            }
            TRACE(4, "Config: %s, adjusted: numWorkerThreads=%d, numBanksPerWorkerThread=%d.\n",
                    __FUNCTION__, config_->numWorkerThreads, config_->numBanksPerWorkerThread);
        }
    }

    uint64_t memLimitPerBank_Bytes = 0;
    if (config_->memoryLimitPercentOfTotal > 0 && config_->memoryLimitPercentOfTotal < 100)
    {
        std::size_t total = ism_common_getTotalMemory();
        memLimitPerBank_Bytes = config_->memoryLimitPercentOfTotal* static_cast<double>(total) * 1024 * 1024 / 100 / numBanks_;
        TRACE(4,"%s Config: setting memory limit: %f%s of %ldMB = %ld per bank, x%d banks.\n",
                __FUNCTION__, config_->memoryLimitPercentOfTotal, "%", total, memLimitPerBank_Bytes, numBanks_);
    }
    else
    {
        TRACE(4,"%s Config: setting memory limit: disabled.\n", __FUNCTION__);
    }

    for (int i=0; i<numBanks_; ++i)
    {
        ActivityBankSPtr& bank = banksVec_[i];
        bank->setMemoryLimitBytes(memLimitPerBank_Bytes);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_DEVICE, config_->monitoringPolicyDeviceClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_GATEWAY, config_->monitoringPolicyGatewayClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_APP, config_->monitoringPolicyAppClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_APP_SCALE, config_->monitoringPolicyAppScaleClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_CONNECTOR, config_->monitoringPolicyConnectorClientClass);
        bank->setEnable(false);
    }

    workerVec_.clear(); //In case it is a reconfiguration
    for (int i=0; i<config_->numWorkerThreads; ++i)
    {
        workerVec_.push_back(
                std::shared_ptr<ActivityWorker>(
                        new ActivityWorker( i,
                                config_->numBanksPerWorkerThread,
                                config_->bulkWriteSize,
                                banksVec_, activityDB_, this)));
        workerVec_.back()->initRateLimit(
                config_->dbIOPSRateLimit/config_->numWorkerThreads,
                TOKEN_BUCKET_MAX_RATE_PER_THREAD, TOKEN_BUCKET_INT_MILLIS);
    }

    cleanupWorker_.reset(new CleanupWorker(config_->bulkWriteSize,activityDB_, this));
    cleanupWorker_->initRateLimit(
            config_->dbIOPSRateLimit/4, //25% of total
            TOKEN_BUCKET_MAX_RATE_PER_THREAD, TOKEN_BUCKET_INT_MILLIS);

    int rc = activityDB_.configure(pConfigActivityDB);
    if (rc != ISMRC_OK)
    {
        state_ = PXACT_STATE_TYPE_INIT;
        started_lock_free_ = false;
        TRACE(1,"Error: %s, failure to configure ActivityDB, state=%d, rc=%d\n", __FUNCTION__, state_, rc);
        return rc;
    }

    state_ = PXACT_STATE_TYPE_CONFIG;
    started_lock_free_ = false;

    TRACE(3,"Info: %s, ActivityTracker configured OK, state=%d, config=%s, rc=%d\n",
            __FUNCTION__, state_, config_->toString().c_str(), rc);
    return rc;
}

int ActivityTracker::configure_during_stop(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB)
{
    TRACE(9, "%s Entry: \n", __FUNCTION__);

    ActivityDBConfig_SPtr config_prev = config_;
    config_.reset(new ActivityDBConfig(pConfigActivityDB));

    TRACE(3, "%s Info: previous config=%s\n", __FUNCTION__, config_prev->toString().c_str());
    TRACE(3, "%s Info: new config=%s\n", __FUNCTION__, config_->toString().c_str());

    //Cannot change: numBanks, numWorkerThreads
    config_->numBanksPerWorkerThread = config_prev->numBanksPerWorkerThread;
    config_->numWorkerThreads = config_prev->numWorkerThreads;

    TRACE(3, "%s Info: merged config=%s\n", __FUNCTION__, config_->toString().c_str());

    uint64_t memLimitPerBank_Bytes = 0;
    if (config_->memoryLimitPercentOfTotal > 0)
    {
        std::size_t total = ism_common_getTotalMemory();
        memLimitPerBank_Bytes = config_->memoryLimitPercentOfTotal * static_cast<double>(total) * 1024 * 1024 / 100
                / numBanks_;
        TRACE(4, "%s Config: setting memory limit: %f%s of %ldMB = %ld per bank, x%d banks.\n", __FUNCTION__,
                config_->memoryLimitPercentOfTotal, "%", total, memLimitPerBank_Bytes, numBanks_);
    }
    else
    {
        TRACE(4, "%s Config: setting memory limit: disabled.\n", __FUNCTION__);
    }

    for (int i = 0; i < numBanks_; ++i)
    {
        ActivityBankSPtr& bank = banksVec_[i];
        bank->setMemoryLimitBytes(memLimitPerBank_Bytes);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_DEVICE, config_->monitoringPolicyDeviceClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_GATEWAY, config_->monitoringPolicyGatewayClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_APP, config_->monitoringPolicyAppClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_APP_SCALE, config_->monitoringPolicyAppScaleClientClass);
        bank->setClientTypePolicy(PXACT_CLIENT_TYPE_CONNECTOR, config_->monitoringPolicyConnectorClientClass);
        bank->setEnable(false);
    }

    workerVec_.clear();
    for (int i = 0; i < config_->numWorkerThreads; ++i)
    {
        workerVec_.push_back(
                std::shared_ptr<ActivityWorker>(
                        new ActivityWorker(i,
                                config_->numBanksPerWorkerThread, config_->bulkWriteSize,
                                banksVec_, activityDB_, this)));
        workerVec_.back()->initRateLimit(config_->dbIOPSRateLimit / config_->numWorkerThreads,
                TOKEN_BUCKET_MAX_RATE_PER_THREAD, TOKEN_BUCKET_INT_MILLIS);
    }

    cleanupWorker_.reset(new CleanupWorker(config_->bulkWriteSize,activityDB_, this));
    cleanupWorker_->initRateLimit(config_->dbIOPSRateLimit / 4, //25% of total
            TOKEN_BUCKET_MAX_RATE_PER_THREAD, TOKEN_BUCKET_INT_MILLIS);

    int rc = activityDB_.configure(pConfigActivityDB);
    if (rc != ISMRC_OK)
    {
        TRACE(1, "Error: %s, failure to re-configure ActivityDB, state=%d, config=%s, rc=%d\n",
                __FUNCTION__, state_, config_->toString().c_str(), rc);
        return rc;
    }

    TRACE(3, "Info: %s, re-configure ActivityTracker OK, state=%d, config=%s, rc=%d\n",
            __FUNCTION__, state_, config_->toString().c_str(), rc);
    return ISMRC_OK;
}


int ActivityTracker::configure_during_start(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB)
{
    TRACE(9, "%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;

    ActivityDBConfig_SPtr config_new(new ActivityDBConfig(pConfigActivityDB));

    TRACE(3, "%s Info: current config=%s\n", __FUNCTION__, config_->toString().c_str());
    TRACE(3, "%s Info: new config=%s\n", __FUNCTION__, config_new->toString().c_str());

    std::pair<bool,int> ret = config_->compare(*config_new);

    if (ret.first)
    {
        if (ret.second > 0)
        {
            TRACE(3, "%s Info: DB configuration changed; going to stop, re-configure, and re-start with new configuration. \n", __FUNCTION__);

            rc = stop_internal();
            if (rc != ISMRC_OK)
            {
                TRACE(2, "%s Warning: failure in stop_internal: rc=%d; going to re-configure, and re-start anyway. \n", __FUNCTION__, rc);
            }

            rc = configure_during_stop(pConfigActivityDB);
            if (rc != ISMRC_OK)
            {
                TRACE(1, "%s Error: failure in configure_during_stop: rc=%d; not going to re-start. \n", __FUNCTION__, rc);
                return rc;
            }

            rc = start_during_stop(true);
            if (rc != ISMRC_OK)
            {
                TRACE(1, "%s Error: failure in start_during_stop: rc=%d; not going to re-start. \n", __FUNCTION__, rc);
                return rc;
            }

            TRACE(3, "Info: %s, re-configure ActivityTracker OK, Restarted OK; state=%d, config=%s, rc=%d\n",
                    __FUNCTION__, state_, config_->toString().c_str(), rc);
        }
        else if (ret.second == 0)
        {
            config_->dbIOPSRateLimit = config_new->dbIOPSRateLimit;
            updateRateLimit(config_->dbIOPSRateLimit);
            config_->dbRateLimitGlobalEnabled = config_new->dbRateLimitGlobalEnabled;

            config_->memoryLimitPercentOfTotal = config_new->memoryLimitPercentOfTotal;
            uint64_t memLimitPerBank_Bytes = 0;
            if (config_->memoryLimitPercentOfTotal > 0 && config_->memoryLimitPercentOfTotal < 100)
            {
                std::size_t total = ism_common_getTotalMemory();
                memLimitPerBank_Bytes = config_->memoryLimitPercentOfTotal* static_cast<double>(total) * 1024 * 1024 / 100 / numBanks_;
                TRACE(4,"%s Config: setting memory limit: %f%s of %ldMB = %ld per bank, x%d banks.\n",
                        __FUNCTION__, config_->memoryLimitPercentOfTotal, "%", total, memLimitPerBank_Bytes, numBanks_);
            }
            else
            {
                TRACE(4,"%s Config: setting memory limit: disabled.\n", __FUNCTION__);
            }

            for (int i = 0; i < numBanks_; ++i)
            {
                banksVec_[i]->setMemoryLimitBytes(memLimitPerBank_Bytes);
            }

            config_->monitoringPolicyDeviceClientClass = config_new->monitoringPolicyDeviceClientClass;
            config_->monitoringPolicyGatewayClientClass = config_new->monitoringPolicyGatewayClientClass;
            config_->monitoringPolicyConnectorClientClass = config_new->monitoringPolicyConnectorClientClass;
            config_->monitoringPolicyAppClientClass = config_new->monitoringPolicyAppClientClass;
            config_->monitoringPolicyAppScaleClientClass = config_new->monitoringPolicyAppScaleClientClass;
            this->setClientTypePolicy(PXACT_CLIENT_TYPE_DEVICE, config_->monitoringPolicyDeviceClientClass);
            this->setClientTypePolicy(PXACT_CLIENT_TYPE_GATEWAY, config_->monitoringPolicyGatewayClientClass);
            this->setClientTypePolicy(PXACT_CLIENT_TYPE_CONNECTOR, config_->monitoringPolicyConnectorClientClass);
            this->setClientTypePolicy(PXACT_CLIENT_TYPE_APP, config_->monitoringPolicyAppClientClass);
            this->setClientTypePolicy(PXACT_CLIENT_TYPE_APP_SCALE, config_->monitoringPolicyAppScaleClientClass);
        }
        else
        {
            TRACE(3, "Info: %s, re-configure ActivityTracker: Only static parameters changed - no change required; state=%d, config=%s, rc=%d\n",
                    __FUNCTION__, state_, config_->toString().c_str(), rc);
        }
    }
    else
    {
        TRACE(3, "Info: %s, re-configure ActivityTracker: Identical - no change required; state=%d, config=%s, rc=%d\n",
                            __FUNCTION__, state_, config_->toString().c_str(), rc);
    }

    return rc;
}


int ActivityTracker::start()
{
    TRACE(8, "%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;

    {
        LockGuardRecur lock(mutex_);

        switch (state_)
        {

        case PXACT_STATE_TYPE_INIT:
        {
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, not configured, cannot start; state: %d, rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_CONFIG:
        {
            rc = start_first();
        }
            break;

        case PXACT_STATE_TYPE_START:
        {
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, already started, state: %d; rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_STOP:
        {
            rc = start_during_stop(true);
        }
            break;

        case PXACT_STATE_TYPE_TERM:
        {
            rc = ISMRC_Closed;
            TRACE(2, "Warning: %s, already closed, rc=%d\n", __FUNCTION__, rc);
        }
            break;

        case PXACT_STATE_TYPE_ERROR:
        {
            TRACE(4, "Config: %s, start during error state, going to stop and restart.\n", __FUNCTION__);
            if (error_close_done_)
            {
                rc = start_during_stop(false);
            }
            else
            {
                rc = ISMRC_WouldBlock;
            }

            if (rc == ISMRC_OK)
            {
                auto_restart_stop();
                TRACE(3, "Info: %s, started OK during error state, rc=%d.\n", __FUNCTION__, rc);
            }
            else
            {
                state_ = PXACT_STATE_TYPE_ERROR;
                started_lock_free_ = false;
                TRACE(1, "Error: %s, failed to start during error state, current state: %d, rc=%d\n", __FUNCTION__, state_, rc);
            }
        }
            break;

        default:
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, Invalid state: %d, rc=%d\n", __FUNCTION__, state_, rc);

        }
    }

    if (rc == ISMRC_OK)
    {
        TRACE(3, "%s Info: Started, ProxyUID=%s\n", __FUNCTION__, proxy_uid_.c_str());
    }
    else
    {
        TRACE(1, "%s Error: failure to start, rc=%d\n", __FUNCTION__, rc);
    }

    return rc;
}

/*
 * Protected by mutex_
 */
int ActivityTracker::start_first()
{
    int rc = ISMRC_OK;

    rc = activityDB_.connect();
    if (rc != ISMRC_OK)
    {
        activityDB_.close();
        TRACE(1, "Error: %s cannot connect ActivityDB, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    rc = activityDB_.register_uid();
    if (rc != ISMRC_OK)
    {
        activityDB_.close();
        TRACE(1, "%s Error: failed to register new UID, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    proxy_uid_ = activityDB_.getProxyUID();

    {
        LockGuardRecur lock(db_client_mutex_);
        db_client_ = activityDB_.getClient();
        proxy_state_.reset(new ProxyStateRecord);
        proxy_state_->proxy_uid = proxy_uid_;
    }

    bool done = false;
    while (!done)
    {
        rc = start_task_timers();
        if (rc != ISMRC_OK)
        {
            TRACE(1, "Error: %s cannot start task timers, rc=%d\n", __FUNCTION__, rc);
            done = true;
            continue;
        }

        for (int i = 0; i < numBanks_; ++i)
        {
            banksVec_[i]->setEnable(true);
        }

        for (int i = 0; i < config_->numWorkerThreads; ++i)
        {
            int rc1 = workerVec_[i]->start();
            if (rc1 != ISMRC_OK)
            {
                rc = rc1;
                break;
            }
        }

        if (rc == ISMRC_OK)
        {
            rc = cleanupWorker_->start();
        }

        done = true;
    } //while (!done)

    if (rc == ISMRC_OK)
    {
        state_ = PXACT_STATE_TYPE_START;
        started_lock_free_ = true;
    }
    else
    {
        // clean up
        int rc1 = close_internal();
        TRACE(1,"%s Error: failure during close_internal(soft=false), rc=%d; Ignoring", __FUNCTION__, rc1);
    }

    TRACE(8, "%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

/**
 * This is called from a normal stop or after a DB error.
 */
int ActivityTracker::start_during_stop(bool registerProxyUID)
{
    TRACE(5, "%s Entry: \n", __FUNCTION__);
    int rc;

    rc = activityDB_.connect();
    if (rc != ISMRC_OK)
    {
        activityDB_.close();
        TRACE(1, "Error: %s cannot connect ActivityDB, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    if (registerProxyUID) {
    	rc = activityDB_.register_uid();
    	if (rc != ISMRC_OK)
    	{
    		activityDB_.close();
    		TRACE(1, "%s Error: failed to register new UID, rc=%d\n", __FUNCTION__, rc);
    		return rc;
    	}
    }

    proxy_uid_ = activityDB_.getProxyUID();
    {
        LockGuardRecur lock(db_client_mutex_);
        db_client_ = activityDB_.getClient();
        proxy_state_.reset(new ProxyStateRecord);
        proxy_state_->proxy_uid = proxy_uid_;
    }

    bool done = false;
    while (!done)
    {
        rc = start_task_timers();

        if (rc != ISMRC_OK)
        {
            TRACE(1, "Error: %s cannot start task timers, rc=%d\n", __FUNCTION__, rc);
            done = true;
            continue;
        }

        for (int i = 0; i < numBanks_; ++i)
        {
            banksVec_[i]->setEnable(true);
        }

        for (int i = 0; i < config_->numWorkerThreads; ++i)
        {
            int rc1 = workerVec_[i]->start();
            if (rc1 != ISMRC_OK)
            {
                rc = rc1;
                break;
            }
        }

        if (rc == ISMRC_OK)
        {
            rc = cleanupWorker_->start();
        }

        done = true;
    } //while (!done)


    if (rc == ISMRC_OK)
    {
        state_ = PXACT_STATE_TYPE_START;
        started_lock_free_ = true;
    }
    else
    {
        //clean up
        int rc1 = close_internal();
        TRACE(1,"%s Error: failure during close_internal(), rc=%d; Ignoring", __FUNCTION__, rc1);
    }

    TRACE(5, "%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

int ActivityTracker::start_task_timers()
{
    TRACE(8, "%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;

    heartbeat_task_ = ism_common_setTimerRate(ISM_TIMER_LOW, ActivityTracker::heartbeat_timer_task, (void* )this,
            activityDB_.getHeartbeatIntervalMillis(), activityDB_.getHeartbeatIntervalMillis(), TS_MILLISECONDS);
    if (heartbeat_task_ == NULL)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s cannot start heartbeat task, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    heartbeat_monitor_task_ = ism_common_setTimerRate(ISM_TIMER_LOW, ActivityTracker::heartbeat_monitor_timer_task,
            (void* )this, activityDB_.getHeartbeatTimeoutMillis() / 2, activityDB_.getHeartbeatTimeoutMillis() / 2,
            TS_MILLISECONDS);
    if (heartbeat_monitor_task_ == NULL)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s cannot start heartbeat monitoring task, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    tokenbucket_task_ = ism_common_setTimerRate(ISM_TIMER_LOW, ActivityTracker::token_bucket_timer_task,
            (void* )this, TOKEN_BUCKET_INT_MILLIS, TOKEN_BUCKET_INT_MILLIS, TS_MILLISECONDS);
    if (tokenbucket_task_ == NULL)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s cannot start token bucket task, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    lazy_clock_task_ = ism_common_setTimerRate(ISM_TIMER_LOW, ActivityTracker::lazy_clock_timer_task, (void* )this,
            TOKEN_BUCKET_INT_MILLIS, TOKEN_BUCKET_INT_MILLIS, TS_MILLISECONDS);
    if (lazy_clock_task_ == NULL)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s cannot start lazy_clock task, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    TRACE(8, "%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

int ActivityTracker::stop()
{
    TRACE(8, "%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;

    {
        LockGuardRecur lock(mutex_);

        switch (state_)
        {
        case PXACT_STATE_TYPE_INIT:
        {
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, not configured, cannot stop; state: %d, rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_CONFIG:
        {
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, not started, cannot stop; state: %d, rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_START:
        {
            rc = stop_internal();
            TRACE(5, "%s stopped, current state: %d; rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_STOP:
        {
            TRACE(5, "%s already stopped, current state: %d; rc=%d\n", __FUNCTION__, state_, rc);
        }
            break;

        case PXACT_STATE_TYPE_TERM:
        {
            rc = ISMRC_Closed;
            TRACE(1, "Error: %s, already closed, rc=%d\n", __FUNCTION__, rc);
        }
            break;

        case PXACT_STATE_TYPE_ERROR:
        {
            if (error_close_done_)
            {
                state_ = PXACT_STATE_TYPE_STOP;
                started_lock_free_ = false;
                auto_restart_stop();
                TRACE(5, "%s stopped during error state, current state: %d; auto-restart turned off; rc=%d\n", __FUNCTION__, state_, rc);
            }
            else
            {
                rc = ISMRC_WouldBlock;
                TRACE(5, "%s failed to stopped during error state, not fully closed yet: %d; rc=%d\n", __FUNCTION__, state_, rc);
            }
        }
            break;

        default:
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, Invalid state: %d, rc=%d\n", __FUNCTION__, state_, rc);
        }
    }

    TRACE(8, "%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

/*
 * Protected by mutex_
 */
int ActivityTracker::stop_internal()
{
    int rc = ISMRC_OK;
    //stop is always an emergency, so wait for nothing
    rc = close_internal();

    if (rc == ISMRC_OK)
    {
        state_ = PXACT_STATE_TYPE_STOP;
        started_lock_free_ = false;
    }

    return rc;
}

int ActivityTracker::startMessaging()
{
    LockGuardRecur lock(mutex_);
    start_messaging_ = true;
    return ISMRC_OK;
}

bool ActivityTracker::isMessagingStarted() const
{
    return start_messaging_;
}


PXACT_STATE_TYPE_t ActivityTracker::getState() const
{
    LockGuardRecur lock(mutex_);
    return state_;
}

bool ActivityTracker::isStarted() const
{
    return started_lock_free_;
}

std::string ActivityTracker::getProxyUID() const
{
	LockGuardRecur lock(mutex_);
	return proxy_uid_;
}

int ActivityTracker::close(bool soft)
{
	TRACE(8,"%s Entry: soft=%s \n", __FUNCTION__, (soft ? "T" : "F"));

	int rc = ISMRC_OK;

	bool close_backend = false;

	{
		LockGuardRecur lock(mutex_);

		TRACE(5, "%s Before: current state: %d \n", __FUNCTION__, state_);

		switch(state_)
		{

        case PXACT_STATE_TYPE_INIT:
        case PXACT_STATE_TYPE_CONFIG:
        case PXACT_STATE_TYPE_STOP:
        {
            state_ = PXACT_STATE_TYPE_TERM;
            started_lock_free_ = false;
            TRACE(5, "%s After: current state: %d \n", __FUNCTION__, state_);
        }
            break;

        case PXACT_STATE_TYPE_START:
        {
            if (!soft)
            {
                rc = close_internal();
            }
            else
            {
                rc = close_internal_frontend();
                close_backend = true;
            }
            state_ = PXACT_STATE_TYPE_TERM;
            started_lock_free_ = false;
            TRACE(5, "%s After: current state: %d \n", __FUNCTION__, state_);
        }
            break;

        case PXACT_STATE_TYPE_TERM:
        {
            rc = ISMRC_Closed;
            TRACE(1, "Error: %s, already closed, rc=%d\n", __FUNCTION__, rc);
        }
            break;

        case PXACT_STATE_TYPE_ERROR:
        {
            state_ = PXACT_STATE_TYPE_TERM;
            started_lock_free_ = false;
            TRACE(5, "%s After: current state: %d \n", __FUNCTION__, state_);
        }
            break;

        default:
            rc = ISMRC_Error;
            TRACE(1, "Error: %s, Invalid state: %d, rc=%d\n", __FUNCTION__, state_, rc);
		}
	}

	//Drain and close the backend on soft close / termination
	if (close_backend)
	{
	    int term_to_ms = 0;
	    {
	        LockGuardRecur lock(mutex_);
	        term_to_ms = config_->terminationTimeoutMS;
	        term_drain_ = true;
	    }

	    std::size_t qsz = getUpdateQueueSize();
	    int rounds = 0;
	    while (term_to_ms > 0)
	    {
	        if ( qsz > 0)
	        {
	            usleep(100000);
	            term_to_ms -= 100;
	            qsz = getUpdateQueueSize();
	            if (rounds++ % 10 == 0)
	                TRACE(9,"%s Draining: update-Q remaining size=%ld \n", __FUNCTION__, qsz);
	        }
	        else
	        {
                usleep(100000); //Let pending writes complete before shutting down
	            TRACE(8,"%s Draining: update-Q Empty \n", __FUNCTION__);
	            break;
	        }
	    }
        TRACE(8,"%s Before close BE: update-Q remaining size=%ld \n", __FUNCTION__, qsz);

	    rc = close_internal_backend(soft);
	}

    auto_restart_stop();

	TRACE(8,"%s Exit: rc=%d \n", __FUNCTION__, rc);
	return rc;
}

int ActivityTracker::close_internal_failure()
{
    TRACE(5,"%s Entry: \n", __FUNCTION__);
    int rc = ISMRC_OK;

    {
        LockGuardRecur lock(mutex_);

        if (heartbeat_task_)
        {
            ism_common_cancelTimer(heartbeat_task_);
            heartbeat_task_ = NULL;
        }

        if (heartbeat_monitor_task_)
        {
            ism_common_cancelTimer(heartbeat_monitor_task_);
            heartbeat_monitor_task_ = NULL;
        }

        if (tokenbucket_task_)
        {
            ism_common_cancelTimer(tokenbucket_task_);
            tokenbucket_task_ = NULL;
        }

        if (lazy_clock_task_)
        {
            ism_common_cancelTimer(lazy_clock_task_);
            lazy_clock_task_ = NULL;
        }

    }

    //do not join with a lock
    for (unsigned i=0; i<workerVec_.size(); ++i)
    {
        workerVec_[i]->stop();
    }
    cleanupWorker_->stop();

    {
        LockGuardRecur lock(mutex_);

        {
            LockGuardRecur lock(db_client_mutex_);
            if (db_client_)
            {
            	TRACE(5,"%s Closing ActivityDB client \n", __FUNCTION__);
                activityDB_.closeClient(db_client_);
                db_client_.reset();
            }
        }
        TRACE(5,"%s Closing ActivityDB \n", __FUNCTION__);
        activityDB_.close();
        error_close_done_ = true;
    }

    TRACE(5,"%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

/*
 * Protected by mutex_
 * Can be called by external (IOP), etc) threads, worker threads and timer threads
 */
int ActivityTracker::close_internal()
{
    TRACE(8,"%s Entry: \n", __FUNCTION__);
    int rc = ISMRC_OK;

    for (int i=0; i<numBanks_; ++i)
    {
        banksVec_[i]->setEnable(false);
    }

    if (heartbeat_task_)
    {
        ism_common_cancelTimer(heartbeat_task_);
        heartbeat_task_ = NULL;
    }

    if (heartbeat_monitor_task_)
    {
        ism_common_cancelTimer(heartbeat_monitor_task_);
        heartbeat_monitor_task_ = NULL;
    }

    if (tokenbucket_task_)
    {
        ism_common_cancelTimer(tokenbucket_task_);
        tokenbucket_task_ = NULL;
    }

    if (lazy_clock_task_)
    {
        ism_common_cancelTimer(lazy_clock_task_);
        lazy_clock_task_ = NULL;
    }

    for (unsigned i=0; i<workerVec_.size(); ++i)
    {
        workerVec_[i]->stop();
    }

    cleanupWorker_->stop();

    for (int i = 0; i < numBanks_; ++i)
    {
        banksVec_[i]->clear();
    }

    {
        LockGuardRecur lock(db_client_mutex_);
        if (db_client_)
        {
            activityDB_.closeClient(db_client_);
            db_client_.reset();
        }
    }
    activityDB_.close();

    TRACE(8,"%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

/*
 * Protected by mutex_
 * Can be called by external (IOP), etc) threads, worker threads and timer threads
 */
int ActivityTracker::close_internal_frontend()
{
    TRACE(8,"%s Entry: \n", __FUNCTION__);
    int rc = ISMRC_OK;

    for (int i=0; i<numBanks_; ++i)
    {
        banksVec_[i]->setEnable(false);
    }

    if (heartbeat_monitor_task_)
    {
        ism_common_cancelTimer(heartbeat_monitor_task_);
        heartbeat_monitor_task_ = NULL;
    }

    if (lazy_clock_task_)
    {
        ism_common_cancelTimer(lazy_clock_task_);
        lazy_clock_task_ = NULL;
    }

    cleanupWorker_->updateRateLimit(1); //as small as possible, 0 is unlimited

    TRACE(8,"%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}


/*
 * Can be called by external (IOP), etc) threads, worker threads and timer threads
 */
int ActivityTracker::close_internal_backend(bool soft)
{
    TRACE(8,"%s Entry: \n", __FUNCTION__);
    int rc = ISMRC_OK;

    {
        LockGuardRecur lock(mutex_);

        if (heartbeat_task_)
        {
            ism_common_cancelTimer(heartbeat_task_);
            heartbeat_task_ = NULL;
        }

        if (tokenbucket_task_)
        {
            ism_common_cancelTimer(tokenbucket_task_);
            tokenbucket_task_ = NULL;
        }

        for (unsigned i=0; i<workerVec_.size(); ++i)
        {
            workerVec_[i]->stop();
        }

        cleanupWorker_->stop();

        for (int i = 0; i < numBanks_; ++i)
        {
            banksVec_[i]->clear();
        }

        {
            LockGuardRecur lock(db_client_mutex_);
            if (db_client_)
            {
                if (soft)
                {
                    rc = db_client_->read(proxy_state_.get());
                    TRACE(8,"%s soft, after read, record=%s \n", __FUNCTION__, proxy_state_->toString().c_str());

                    if (rc == ISMRC_OK && proxy_state_->status == ProxyStateRecord::PX_STATUS_ACTIVE)
                    {
                        rc = db_client_->updateStatus_CondVersion(proxy_uid_, ProxyStateRecord::PX_STATUS_LEAVE, proxy_state_->version);
                        proxy_state_stats_.num_db_update++;
                        TRACE(8,"%s soft, after update, rc=%d \n", __FUNCTION__, rc);
                    }
                }

                activityDB_.closeClient(db_client_);
                db_client_.reset();
            }
        }
        activityDB_.close();
    }

    TRACE(8,"%s Exit: rc=%d \n", __FUNCTION__, rc);
    return rc;
}

int ActivityTracker::clientActivity(
		const ismPXACT_Client_t* pClient)
{
	TRACE(9,"%s Entry: client=%s\n", __FUNCTION__, toString(pClient).c_str());

	if (!pClient)
	{
		TRACE(1,"Error: %s NULL client, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}

	if (!pClient->pClientID)
	{
		TRACE(1,"Error: %s NULL ClientID, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	if (!pClient->pOrgName)
	{
		TRACE(1,"Error: %s NULL OrgName, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	std::size_t bank = murmur3_(pClient->pClientID) % numBanks_;
	int rc = banksVec_[bank]->clientActivity(pClient);

	TRACE(9,"%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityTracker::clientConnectivity(
    		const ismPXACT_Client_t* pClient,
			const ismPXACT_ConnectivityInfo_t* pConnInfo)
{

	TRACE(9,"%s Entry: client=%s,  connInfo=%s\n",
			__FUNCTION__, toString(pClient).c_str(), toString(pConnInfo).c_str());

	if (!pClient)
	{
		TRACE(1,"Error: %s NULL client, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}

	if (!pClient->pClientID)
	{
		TRACE(1,"Error: %s NULL ClientID, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	if (!pClient->pOrgName)
	{
		TRACE(1,"Error: %s NULL OrgName, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	if (!pConnInfo)
	{
		TRACE(1,"Error: %s NULL ConnInfo, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}


	std::size_t bank = murmur3_(pClient->pClientID) % numBanks_;
	int	rc = banksVec_[bank]->clientConnectivity(pClient, pConnInfo);

	TRACE(9,"%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;

}

void ActivityTracker::server_shutdown(int core)
{
	if (shutdownHandler_)
	{
		shutdownHandler_(core);
	}
	else
	{
		ism_common_sleep(100000);
		ism_common_shutdown(core);
	}
}


int ActivityTracker::heartbeat_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	if (userdata)
	{
		ActivityTracker* tracker = static_cast<ActivityTracker*>(userdata);
		int rc = tracker->heartbeatTask();
		if (rc == ISMRC_OK)
		{
			return 1; 		//repeat task
		}
		else
		{
		    TRACE(2,"Warning: %s ActivityTracker.heartbeatTask() failed or closed; task will not repeat.\n", __FUNCTION__);
			return 0;		//in case of error, do not repeat the task.
		}
	}
	else
	{
	    TRACE(1,"Error: %s NULL userdata - ActivityTracker; task will not repeat.\n", __FUNCTION__);
		return 0;           //in case of error, do not repeat the task.
	}
}

int ActivityTracker::heartbeatTask()
{
    using namespace std;

	int rc = ISMRC_OK;
	bool do_hb = false;

	{
		LockGuardRecur lock(mutex_);
		if (state_ == PXACT_STATE_TYPE_START)
		{
			do_hb = true;
		}
	}

	if (do_hb)
	{
	    {
	        LockGuardRecur lock(db_client_mutex_);
	        if (db_client_) {
	        	double update_start = ism_common_readTSC();
	            rc = db_client_->updateHeartbeat(proxy_state_.get());
	            sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
	            num_update_latency_++;
	        } else {
	            rc = ISMRC_NullPointer;
	        }
	    }

		proxy_state_stats_.num_db_update++;
		if (rc != ISMRC_OK)
		{
			heartbeat_missed_++;
			TRACE(1,"Warning: %s failed to update heartbeat, missed=%d\n", __FUNCTION__, heartbeat_missed_);
		}
		else
		{
			heartbeat_missed_ = 0;
		}

		int threshold = activityDB_.getHeartbeatTimeoutMillis() / activityDB_.getHeartbeatIntervalMillis();
		if (heartbeat_missed_ > threshold)
		{
		    proxy_state_stats_.num_db_error++;
			rc = ISMRC_Error;
			ostringstream what;
			what << "Failed to update "<< threshold << "  heart-beats, heart-beat timeout";
			TRACE(1,"Error: %s, %s, shutting down activity tracker, rc=%d\n", __FUNCTION__, what.str().c_str(), rc);
			notifyFailure(rc, what.str());
		}
	}
	else
	{
		return ISMRC_Closed;
	}

	return rc;
}

int ActivityTracker::heartbeat_monitor_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	if (userdata)
	{
		ActivityTracker* tracker = static_cast<ActivityTracker*>(userdata);
		int rc = tracker->heartbeatMonitorTask();
		if (rc == ISMRC_OK)
		{
			return 1; 		//repeat task
		}
		else
		{
		    TRACE(2,"Warning: %s ActivityTracker.heartbeatMonitorTask() failed or closed; task will not repeat.\n", __FUNCTION__);
			return 0;		//in case of error, do not repeat the task.
		}
	}
	else
	{
	    TRACE(1,"Error: %s NULL userdata - ActivityTracker; task will not repeat.\n", __FUNCTION__);
	    return 0;           //in case of error, do not repeat the task.
	}
}


int ActivityTracker::heartbeatMonitorTask()
{
	using namespace std;
	int rc = ISMRC_OK;
	bool do_hb_mon = false;

	{
		LockGuardRecur lock(mutex_);
		if (state_ == PXACT_STATE_TYPE_START)
		{
			do_hb_mon = true;
		}
		else
		{
		    rc = ISMRC_Closed;
		}
	}

	if (do_hb_mon)
	{


	    StringSet active_proxies_tmp;
		std::unique_ptr<ProxyStateRecord> record( new ProxyStateRecord);
		int num_results = 0;
		double read_start = ism_common_readTSC();

		{
		    LockGuardRecur lock(db_client_mutex_);
		    if (db_client_)
		        rc = db_client_->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE, record.get(), &num_results);
		    else
		        rc = ISMRC_NullPointer;
		}

        sum_read_latency_sec_ += (ism_common_readTSC() - read_start);
        num_read_latency_++;
		proxy_state_stats_.num_db_read++;
		if (rc == ISMRC_OK)
		{
 			map<string, int64_t> timeout_proxies;
			ProxyStateRecord* current = record.get();
			while (num_results>0 && current)
			{
				if (current->proxy_uid != proxy_uid_)
				{
					if (current->last_hb + current->hbto_ms < proxy_state_->last_hb)
					{
						TRACE(3, "Info: %s Proxy time-out, proxy_state: %s; currentTime=%ld\n",
								__FUNCTION__, current->toString().c_str(), proxy_state_->last_hb);
						timeout_proxies[current->proxy_uid] = current->version;
					}
					else
					{
						active_proxies_tmp.insert(current->proxy_uid);
					}
				}
				else
				{
					active_proxies_tmp.insert(current->proxy_uid);
				}

				current = current->next;
			}

			for (map<string, int64_t>::iterator it = timeout_proxies.begin(); it != timeout_proxies.end(); ++it)
			{
			    LockGuardRecur lock(db_client_mutex_);
			    int rc1 = ISMRC_NullPointer;
			    if (db_client_)
			    {
			        rc1 = db_client_->updateStatus_CondVersion(it->first, ProxyStateRecord::PX_STATUS_SUSPECT, it->second);
			    }
			    proxy_state_stats_.num_db_update++;
			    if (rc1 != ISMRC_OK)
			    {
			        TRACE(2,"Warning: %s failed to update status=SUSPECT, proxy_uid=%s, version=%ld, rc=%d; Ignored.\n",
			                __FUNCTION__, it->first.c_str(), it->second, rc1);
			    }
			}

			ostringstream oss;
			for (string s : active_proxies_tmp)
			{
				oss << s << " ";
			}
			TRACE(5, "%s Active proxies: #=%ld, { %s} \n",
					__FUNCTION__, active_proxies_.size(), oss.str().c_str());

			{
					LockGuardRecur lock(mutex_);
					active_proxies_.swap(active_proxies_tmp);
			}
		}
		else
		{
			TRACE(1,"Error: %s failed to get all active proxies, rc=%d;\n",	__FUNCTION__, rc);

		    if (rc != ISMRC_OK && rc != ISMRC_Closed)
		    {
		        proxy_state_stats_.num_db_error++;
		        notifyFailure(rc, "heartbeatMonitorTask failed");
		    }
		}
	}

	//=== client-proxy cleanup

	bool do_cleanup = false;
    if (rc == ISMRC_OK)
    {
        LockGuardRecur lock(mutex_);
        if (state_ == PXACT_STATE_TYPE_START)
        {
            do_cleanup = true;
        }
        else
        {
            rc = ISMRC_Closed;
        }
    }

    if (do_cleanup)
    {
        StringSet suspect_leave_proxies_tmp;
        unique_ptr<ProxyStateRecord> record(new ProxyStateRecord);
        int num_results = 0;
        double read_start = ism_common_readTSC();
        vector<ProxyStateRecord::PxStatus> status_set_suspect_leave;
        status_set_suspect_leave.push_back(ProxyStateRecord::PX_STATUS_SUSPECT);
        status_set_suspect_leave.push_back(ProxyStateRecord::PX_STATUS_LEAVE);
        status_set_suspect_leave.push_back(ProxyStateRecord::PX_STATUS_REMOVED);
        {
            LockGuardRecur lock(db_client_mutex_);
            if (db_client_)
            {
                rc = db_client_->getAllWithStatusIn(status_set_suspect_leave, record.get(), &num_results);
            }
            else
                rc = ISMRC_NullPointer;
        }
        sum_read_latency_sec_ += (ism_common_readTSC() - read_start);
        num_read_latency_++;
        proxy_state_stats_.num_db_read++;

        if (rc == ISMRC_OK)
        {
            ProxyStateRecord* current = record.get();
            while (num_results > 0 && current)
            {
                if (current->proxy_uid != proxy_uid_ )
                {
                    const int64_t removed_to_ms  = config_->removedProxyCleanupIntervalSec*1000;

                    if ( current->status != ProxyStateRecord::PX_STATUS_REMOVED)
                    {
                        suspect_leave_proxies_tmp.insert(current->proxy_uid);
                    }
                    else if (current->last_hb + removed_to_ms < proxy_state_->last_hb)
                    {
                        LockGuardRecur lock(db_client_mutex_);
                        int rc1 = ISMRC_NullPointer;
                        if (db_client_)
                        {
                            rc1 = db_client_->removeProxyUID(current->proxy_uid);
                        }
                        TRACE(5, "%s Deleted proxy in status=REMOVED from MongoDB; record: %s, rc=%d\n",
                                __FUNCTION__, current->toString().c_str(), rc1);
                    }
                }
                else
                {
                    TRACE(2, "Warning: %s this proxy was marked as not ACTIVE, record: %s, rc=%d;\n",
                            __FUNCTION__, current->toString().c_str(), rc);
                }
                current = current->next;
            }

            {
                LockGuardRecur lock(mutex_);
                suspect_leave_proxies_.swap(suspect_leave_proxies_tmp);
                if (cleanupWorker_)
                {
                    for (StringSet::const_iterator it = suspect_leave_proxies_.begin(); it != suspect_leave_proxies_.end(); ++it)
                    {
                        cleanupWorker_->addProxy(*it);
                    }
                }
            }
        }
        else if (rc == ISMRC_NotFound)
        {
            TRACE(9, "%s Did not find any proxy with status suspect or leave.\n", __FUNCTION__);
            rc = ISMRC_OK;
        }
        else
        {
            TRACE(1, "Error: %s failed trying to find suspect/leave proxies, rc=%d;\n", __FUNCTION__, rc);

            if (rc != ISMRC_OK && rc != ISMRC_Closed)
            {
                proxy_state_stats_.num_db_error++;
                notifyFailure(rc, "heartbeatMonitorTask failed");
            }
        }
    }

    TRACE(8,"Exit: %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
}


int ActivityTracker::token_bucket_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	TRACE(8,"%s Entry: userdata=%p\n", __FUNCTION__, userdata);

	if (userdata)
	{
		ActivityTracker* tracker = static_cast<ActivityTracker*>(userdata);
		int rc = tracker->tokenBucketTask();

		//Consider ISMRC_Closed is not an error to stop the Timer Task.
		if (rc == ISMRC_OK || rc == ISMRC_Closed)
		{
			if(rc == ISMRC_Closed){
				//Trace when RC is CLOSED so we know there are issue with the workers.
				TRACE(5,"Warning: %s ActivityTracker.token_bucket_timer_task() received closed return code. Task will continue.\n", __FUNCTION__);
			}
			return 1; 		//repeat task
		}
		else
		{
		    TRACE(2,"Warning: %s ActivityTracker.token_bucket_timer_task() failed or closed; task will not repeat.\n", __FUNCTION__);
			return 0;		//in case of error, do not repeat the task.
		}
	}
	else
	{
	    TRACE(1,"Error: %s NULL userdata - ActivityTracker; task will not repeat.\n", __FUNCTION__);
	    return 0;           //in case of error, do not repeat the task.
	}
}

int ActivityTracker::tokenBucketTask()
{
	using namespace std;

	TRACE(8,"%s Entry: \n", __PRETTY_FUNCTION__);

	int rc = ISMRC_OK;
	bool do_refill = false;
	StringSet active_proxies_tmp;

	{
		LockGuardRecur lock(mutex_);
		if (state_ == PXACT_STATE_TYPE_START || (state_ == PXACT_STATE_TYPE_TERM && term_drain_))
		{
			do_refill = true;
		}
	}

	if (do_refill)
	{
		for (unsigned i=0; i<workerVec_.size(); ++i)
		{
		    ActivityWorker_SPtr& worker = workerVec_[i];
			int rc1 = worker->tokenBucketRefill();
			if (rc1 != ISMRC_OK){
				rc = rc1;
				break;
			}
		}
		if(rc == ISMRC_OK){
			int rc2 = cleanupWorker_->tokenBucketRefill();
			if (rc2 != ISMRC_OK)
				rc = rc2;
		}
	}

	if (rc != ISMRC_OK && rc != ISMRC_Closed)
	{
	    notifyFailure(rc, "tokenBucketTask failed");
	}

	return rc;
}

int ActivityTracker::lazy_clock_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    TRACE(8, "%s Entry: userdata=%p\n", __FUNCTION__, userdata);

    if (userdata)
    {
        ActivityTracker* tracker = static_cast<ActivityTracker*>(userdata);
        tracker->lazyClockTask();
        return 1; 		//repeat task
    }
    else
    {
        TRACE(1, "Error: %s NULL userdata - ActivityTracker; task will not repeat.\n", __FUNCTION__);
        return 0;           //in case of error, do not repeat the task.
    }
}

void ActivityTracker::lazyClockTask()
{
	using namespace std;

	TRACE(8,"%s Entry: \n", __PRETTY_FUNCTION__);

	bool do_update = false;
	StringSet active_proxies_tmp;

	{
		LockGuardRecur lock(mutex_);
		if (state_ == PXACT_STATE_TYPE_START)
		{
			do_update = true;
		}
	}

	if (do_update)
	{
		for (int i=0; i<numBanks_; ++i)
		{
			banksVec_[i]->updateLazyTimestamp();
		}
	}
}

void ActivityTracker::setClientTypePolicy(PXACT_CLIENT_TYPE_t client_type, int policy)
{
    TRACE(8,"%s Entry: client_type=%d, policy=%d \n", __FUNCTION__, client_type, policy);
    for (int i=0; i<numBanks_; ++i)
    {
        banksVec_[i]->setClientTypePolicy(client_type,policy);
    }
}

void ActivityTracker::updateRateLimit(uint32_t rate_iops)
{
    TRACE(8, "%s Entry: rate_iops=%d \n", __FUNCTION__, rate_iops);
    for (int i = 0; i < config_->numWorkerThreads; ++i)
    {
        workerVec_[i]->updateRateLimit(rate_iops/config_->numWorkerThreads);
    }
    cleanupWorker_->updateRateLimit(rate_iops/4); //25% of total
}

std::size_t ActivityTracker::getNumClients() const
{
	std::size_t n = 0;
	for (int i=0; i<numBanks_; ++i)
	{
	    n += banksVec_[i]->getNumClients();
	}

	return n;
}

std::size_t ActivityTracker::getMemoryLimitBytes() const
{
	std::size_t n = 0;
	for (int i=0; i<numBanks_; ++i)
	{
	    n += banksVec_[i]->getMemoryLimitBytes();
	}

	return n;
}

std::size_t ActivityTracker::getSizeBytes() const
{
    std::size_t n = 0;
    for (int i=0; i<numBanks_; ++i)
    {
        n += banksVec_[i]->getSizeBytes();
    }
    return n;
}

std::size_t ActivityTracker::getUpdateQueueSize() const
{
    std::size_t n = 0;
    for (int i=0; i<numBanks_; ++i)
    {
        n += banksVec_[i]->getUpdateQueueSize();
    }
    return n;
}

ActivityStats ActivityTracker::getStats()
{
    ActivityStats a;

    for (int i = 0; i < numBanks_; ++i)
    {
        ActivityStats b = banksVec_[i]->getActivityStats();
        a = a + b;
    }

    for (int i = 0; i < config_->numWorkerThreads; ++i)
    {
        a = a + workerVec_[i]->getActivityStats();
    }

    //TODO cleanupWorker

    int prx_exists = 0;
    {
        LockGuardRecur lock(mutex_);
        if (num_read_latency_ > 0)
        {
            proxy_state_stats_.avg_db_read_latency_ms = static_cast<int64_t>(ceil(1000 * sum_read_latency_sec_ / num_read_latency_));
            prx_exists = 1;
        }
        else
        {
            proxy_state_stats_.avg_db_read_latency_ms = 0;
        }

        sum_read_latency_sec_ = 0;
        num_read_latency_ = 0;

        if (num_update_latency_ > 0)
        {
            proxy_state_stats_.avg_db_update_latency_ms = static_cast<int64_t>(ceil(1000 * sum_update_latency_sec_ / num_update_latency_));
            prx_exists = 1;
        }
        else
        {
            proxy_state_stats_.avg_db_update_latency_ms = 0;
        }

        sum_update_latency_sec_ = 0;
        num_update_latency_ = 0;

        if (num_bulk_update_latency_ > 0)
        {
            proxy_state_stats_.avg_db_bulk_update_latency_ms = static_cast<int64_t>(ceil(1000 * sum_bulk_update_latency_sec_ / num_bulk_update_latency_));
            prx_exists = 1;
        }
        else
        {
            proxy_state_stats_.avg_db_bulk_update_latency_ms = 0;
        }

        sum_update_latency_sec_ = 0;
        num_update_latency_ = 0;

        a = a + proxy_state_stats_;
    }

    a.norm(numBanks_, config_->numWorkerThreads + prx_exists);

    a.duration_sec = (float)(ism_common_readTSC() - timestamp_First_);

    return a;
}

int ActivityTracker::getStats(ismPXACT_Stats_t* pStats)
{
    if (pStats)
    {
        PXACT_STATE_TYPE_t s = getState();
        if (s == PXACT_STATE_TYPE_START || s == PXACT_STATE_TYPE_STOP || s == PXACT_STATE_TYPE_ERROR)
        {
            pStats->activity_tracking_state = s;
            ActivityStats curr = getStats();
            TRACE(8,"%s state=%d, stats-curr: %s \n", __FUNCTION__, s, curr.toString().c_str());
            LockGuardRecur lock(mutex_);
            ActivityStats diff = curr - lastStats_;
            TRACE(8,"%s stats-prev: %s \n", __FUNCTION__, lastStats_.toString().c_str());
            TRACE(8,"%s stats-diff: %s \n", __FUNCTION__, diff.toString().c_str());
            diff.copyTo(pStats);
            lastStats_ = curr;
        } else {
            memset(pStats, 0, sizeof(ismPXACT_Stats_t));
            pStats->activity_tracking_state = s;
            TRACE(8,"%s state=%d \n", __FUNCTION__, s);
        }
        return ISMRC_OK;
    }
    else
    {
        return ISMRC_NullArgument;
    }
}

/*
 * Should not be called with a lock other than possibly this->mutex_
 */
void ActivityTracker::notifyFailure(int err_code, const std::string& err_msg)
{
    TRACE(5,"%s Entry: err_code=%d, err_msg=%s \n", __FUNCTION__, err_code, err_msg.c_str() );

    bool do_close = false;

    {
        LockGuardRecur lock(mutex_);
        if (state_ == PXACT_STATE_TYPE_START)
        {
            state_ = PXACT_STATE_TYPE_ERROR;
            started_lock_free_ = false;
            do_close = true;
            error_close_done_ = false;
        }
    }

    if (do_close)
    {
    	TRACE(1,"Error: %s, about to close, error_msg=%s\n", __FUNCTION__, err_msg.c_str());
        close_internal_failure();
        TRACE(1,"Error: %s, about to restart, error_msg=%s\n", __FUNCTION__, err_msg.c_str());
        auto_restart_start();
        TRACE(1,"Error: %s, stopped component, moved to error state, error_msg=%s \n", __FUNCTION__, err_msg.c_str());
    }
    else
    {
        TRACE(1,"Error: %s, already in error state, error_msg=%s \n", __FUNCTION__, err_msg.c_str());
    }

    TRACE(5,"%s Exit: \n", __FUNCTION__);
}


void ActivityTracker::auto_restart_run()
{
	sleep(1);
    TRACE(5,"%s Entry: delay 1 second.\n", __FUNCTION__);


    while (!auto_restart_finish_.load())
    {
        int rc = ISMRC_OK;
        bool sleep_short = false;
        if (mutex_.try_lock())
        {
            LockGuardRecur lock(mutex_, std::adopt_lock );
            if (state_ == PXACT_STATE_TYPE_ERROR)
            {
                TRACE(5, "%s, auto-restart during error state.\n", __FUNCTION__);
                rc = start_during_stop(false);

                if (rc == ISMRC_OK)
                {
                    TRACE(3, "Info: %s, restarted OK during error state, rc=%d.\n", __FUNCTION__, rc);
                    break;
                }
                else
                {
                    state_ = PXACT_STATE_TYPE_ERROR;
                    started_lock_free_ = false;
                    TRACE(2, "Warning: %s, failed to restart during error state, rc=%d; current state: %d; going to sleep 5s.\n",
                            __FUNCTION__, rc, state_);
                }
            }
            else
            {
                TRACE(5, "%s state=%d, not PXACT_STATE_TYPE_ERROR, thread going to stop.\n", __FUNCTION__, state_);
                break;
            }
        }
        else
        {
            sleep_short = true;
        }

        if (sleep_short)
            usleep(10000);
        else
            sleep(5);
    }

    bool finish_before = auto_restart_finish_.exchange(true);

    TRACE(5, "%s Exit: state=%d, finish_before=%d, thread going to exit.\n", __FUNCTION__, state_, finish_before);
}

int ActivityTracker::auto_restart_start()
{
    TRACE(5,"%s Entry: \n", __FUNCTION__);
    int rc = ISMRC_OK;

    bool done = false;
    while (!done)
    {
        {
            LockGuardRecur lock(mutex_);

            if (state_ == PXACT_STATE_TYPE_ERROR && auto_restart_finish_.load())
            {
                if (!auto_restart_thread_.joinable())
                {
                	TRACE(5,"%s, about to start auto_restart_run thread\n", __FUNCTION__);
                    auto_restart_finish_.store(false);
                    auto_restart_thread_ = std::thread(&ActivityTracker::auto_restart_run, this);
                    done = true;
                    std::ostringstream oss;
                    oss << auto_restart_thread_.get_id();
                    TRACE(5,"%s, Started successfully, joinable=%d, id=%s \n",
                            __FUNCTION__, auto_restart_thread_.joinable(), oss.str().c_str());
                }
            }
            else
            {
                rc = ISMRC_Error;
                TRACE(2,"Warning: %s, Not started, state=%d, finish=%d, joinable=%d, rc=%d \n",
                        __FUNCTION__, state_, auto_restart_finish_.load(), auto_restart_thread_.joinable(), rc);
                done = true;
            }
        }

        //do not join with a lock
        if (!done && auto_restart_finish_.load() && auto_restart_thread_.joinable())
        {
            TRACE(8,"%s Before join \n", __FUNCTION__);
            auto_restart_thread_.join();
            TRACE(8,"%s After join \n", __FUNCTION__);
        }
    }

    TRACE(5,"%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}

int ActivityTracker::auto_restart_stop()
{
    TRACE(8,"%s Entry: \n", __FUNCTION__);
    //make sure only one thread joins, the one that toggles finish
    bool finish_before = auto_restart_finish_.exchange(true);
    if (!finish_before && auto_restart_thread_.joinable())
    {
        TRACE(8,"%s Before join \n", __FUNCTION__);
        auto_restart_thread_.join();
        TRACE(8,"%s After join \n", __FUNCTION__);
    }

    TRACE(8,"%s Exit: joinable=%d\n", __FUNCTION__, auto_restart_thread_.joinable());
    return ISMRC_OK;
}

}
