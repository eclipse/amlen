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
 * ActivityTracker.h
 *  Created on: Oct 18, 2017
 */

#ifndef ACTIVITYTRACKER_H_
#define ACTIVITYTRACKER_H_

#include <pthread.h>
#include <string>
#include <set>
#include <thread>
#include <vector>
#include <atomic>

#include "pxactivity.h"
#include "ActivityUtils.h"
#include "ActivityDB.h"
#include "ActivityBank.h"
#include "ActivityWorker.h"
#include "CleanupWorker.h"

namespace ism_proxy
{

/*
 *
 * TODO client state
 * 1. optimistic connect
 * 2. connect sequences
 * 5. track activity <from device>, <to device>,
 * 7. connection stealing - connection reference count
 * 8. connection stealing - kafka
 */


/**
 *
 * Startup sequence:
 * 1. Constructor
 * 2a. (Testing) Register shutdown handler
 * 2b. Configure
 * 3. Start
 */
class ActivityTracker : public FailureNotify
{
public:
    /**
     * After the constructor is called all the banks are initialized, and
     * the state is INIT.
     */
	ActivityTracker();
	virtual ~ActivityTracker();

    /**
     * Register the shutdown handler;
     *
     * The handler is called when the ActivityTracker encounters an unrecoverable error
     * and wants to shutdown the server.
     *
     * @param handler
     * @return
     */
    int registerShutdownHandler(ismPXActivity_ShutdownHandler_t handler);

    /**
     *
     * @param pConfigActivityDB
     * @return
     */
    int configure(
    		const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB);

    int start();

    int stop();

    int startMessaging();
    bool isMessagingStarted() const;

    /**
     * Get the state of the component.
     * Viable state transitions:
     *
     * Viable state transitions:
     * NONE                     initial state
     * NONE    => INIT          constructor
     * INIT    => CONFIG        call _configure() that returns OK
     * INIT    => INIT          call _configure() that returns not-OK
     * CONFIG  => START         call _start()
     * CONFIG  => INIT          call _configure() that returns not-OK
     * START   => STOP          call _stop()
     * START   => ERROR         internal error
     * START   => CLOSE         call _term()
     * STOP    => START         call _start()
     * STOP    => CLOSE         call _term()
     * ERROR   => STOP          call _stop() or _configure()
     * ERROR   => START         call _start()
     * ERROR   => CLOSE         call _term()
     * CLOSE   => CLOSE         end state
     *
     * @return
     */
    PXACT_STATE_TYPE_t getState() const;

    /**
     * Is started? lock free.
     * Equivalent to (PXACT_STATE_TYPE_START == getState()), but does not take a lock.
     * The visibility and ordering of the value returned by this method may be less accurate then getState().
     * @return
     */
    bool isStarted() const;

    /**
     * Get the proxy UID.
     *
     * The proxy UID can be retrieved after the first successful start, before that an empty string is returned.
     * @return
     */
    std::string getProxyUID() const;

    int close(bool soft);

    void setClientTypePolicy(PXACT_CLIENT_TYPE_t client_type, int policy);

    void updateRateLimit(uint32_t rate_iops);

    int clientActivity(
    		const ismPXACT_Client_t* pClient);

    int clientConnectivity(
    		const ismPXACT_Client_t* pClient,
			const ismPXACT_ConnectivityInfo_t* pConnInfo);

    std::size_t getNumClients() const;

    std::size_t getSizeBytes() const;

    std::size_t getMemoryLimitBytes() const;

    std::size_t getUpdateQueueSize() const;


    ActivityStats getStats();

    /**
     * The differential stats between two calls.
     */
    int getStats(ismPXACT_Stats_t* pStats);


    //=== Tasks ===============================================================
    static int heartbeat_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata);
    int heartbeatTask();

    static int heartbeat_monitor_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata);
    int heartbeatMonitorTask();

    static int token_bucket_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata);
    int tokenBucketTask();

    static int lazy_clock_timer_task(ism_timer_t key, ism_time_t timestamp, void * userdata);
    void lazyClockTask();

    //==========================================================================
    /**
     * @see NotifyFailure
     *
     * @param err_code
     * @param err_msg
     */
    virtual void notifyFailure(int err_code, const std::string& err_msg);

private:
    static constexpr int MAX_BANKS = 256;
    static constexpr int MAX_WORKERS = 64;
    static constexpr int TOKEN_BUCKET_INT_MILLIS = 10;
    static constexpr int TOKEN_BUCKET_MAX_RATE_PER_THREAD = 10000;

    mutable std::recursive_mutex mutex_;

    ismPXActivity_ShutdownHandler_t shutdownHandler_;

     PXACT_STATE_TYPE_t state_;
     bool started_lock_free_;
     bool start_messaging_;
     bool term_drain_;
     const double timestamp_First_;
     bool error_close_done_;

     using ActivityDBConfig_SPtr = std::shared_ptr<ActivityDBConfig>;
     ActivityDBConfig_SPtr config_;

     std::string proxy_uid_;

     ism_timer_t heartbeat_task_; //Repeating
     int heartbeat_missed_;

     ism_timer_t heartbeat_monitor_task_; //Repeating
     using StringSet = std::set<std::string>;
     StringSet active_proxies_;
     StringSet suspect_leave_proxies_;
     StringSet to_delete_proxies_;

     ism_timer_t tokenbucket_task_; //Repeating

     ism_timer_t lazy_clock_task_; //Repeating

     ActivityDB activityDB_;

     CStringMurmur3Hash murmur3_;

     ActivityBanksVector banksVec_;
     int numBanks_;

     using ActivityWorker_SPtr = std::shared_ptr<ActivityWorker>;
     using WorkerVector = std::vector<ActivityWorker_SPtr>;
     WorkerVector workerVec_;

     using CleanupWorker_SPtr = std::shared_ptr<CleanupWorker>;
     CleanupWorker_SPtr cleanupWorker_;

     //===>>> DB Client ===
     //FIXME protect db_client_ with mutex and control life cycle !!!!
     mutable std::recursive_mutex db_client_mutex_;
     ActivityDBClient_SPtr db_client_;

     ActivityStats proxy_state_stats_;
     double sum_read_latency_sec_;
     std::size_t num_read_latency_;
     double sum_update_latency_sec_;
     std::size_t num_update_latency_;
     double sum_bulk_update_latency_sec_;
     std::size_t num_bulk_update_latency_;

     std::unique_ptr<ProxyStateRecord> proxy_state_;
     //===<<< DB Client ===

     ActivityStats lastStats_;

     //=== auto-restart ===
     std::thread auto_restart_thread_;
     std::atomic<bool> auto_restart_finish_;

     int configure_first(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB);
     int configure_during_stop(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB);
     int configure_during_start(const ismPXActivity_ConfigActivityDB_t *pConfigActivityDB);

     int start_first();
     int start_during_stop(bool registerProxyUID);
     int start_task_timers();

     int stop_internal();

     int close_internal();

     int close_internal_failure();

     //int close_internal(bool soft);

     int close_internal_frontend();
     int close_internal_drain();
     int close_internal_backend(bool soft);

     void server_shutdown(int core);

     int tockeBucket_replenish();

     void auto_restart_run();
     int auto_restart_start();
     int auto_restart_stop();

};



} //namespace: ism_proxy

#endif /* ACTIVITYTRACKER_H_ */
