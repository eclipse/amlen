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
 * CleanupWorker.cpp
 *  Created on: Mar 4, 2018
 */

#include <CleanupWorker.h>
#include <sys/prctl.h>

namespace ism_proxy
{

CleanupWorker::CleanupWorker(int batchSize, ActivityDB& activityDB, FailureNotify* failureNotify) :
        batchSize_(batchSize), activityDB_(activityDB),
        started_(false), finish_(false),
        sum_read_latency_sec_(0.0), num_read_latency_(0),
        failureNotify_(failureNotify)
{
    TRACE(5, "%s Entry: batchSz=%d this=%p\n", __FUNCTION__, batchSize, this);
    if (batchSize < 1)
    {
        TRACE(1, "%s Error: invalid_argument batchSz=%d\n", __FUNCTION__, batchSize);
        throw std::invalid_argument("batchSz must be >0");
    }

    TRACE(8, "%s Exit: thread-name=%s\n", __FUNCTION__, thread_name_);
}

CleanupWorker::~CleanupWorker()
{
    using namespace std;

    TRACE(8, "%s Entry: \n", __FUNCTION__);
    try
    {
        if (thread_.joinable())
            thread_.join();
    } catch (system_error& err)
    {
        ostringstream what;
        what << "CleanupWorker id=" << thread_.get_id() << ", calling thread id=" << this_thread::get_id() << ", code="
                << err.code() << " what=" << err.what();
        TRACE(1, "Error: %s, thread.join exception: %s;\n", __FUNCTION__, what.str().c_str());
    }
}


int CleanupWorker::start()
{
    TRACE(5, "%s Entry: \n", __FUNCTION__);
    LockGuard lock(mutex_);
    if (!started_)
    {
        dbClient_ = activityDB_.getClient();
        try
        {
            thread_ = std::thread(&CleanupWorker::run, this);
            started_ = true;
            TRACE(5, "%s Exit: rc=%d\n", __FUNCTION__, ISMRC_OK);
            return ISMRC_OK;
        }
        catch (std::system_error& se)
        {
            dbClient_->close();
            TRACE(1, "Error: %s failed to create thread, caught system_error exception: what=%s, code=%d, rc=%d \n",
                    __FUNCTION__, se.what(), se.code().value(),  ISMRC_Error);
            return ISMRC_Error;
        }
        catch (std::runtime_error& re)
        {
            dbClient_->close();
            TRACE(1, "Error: %s failed to create thread, caught runtime_error exception: what=%s, rc=%d \n",
                    __FUNCTION__, re.what(), ISMRC_Error);
            return ISMRC_Error;
        }
    }
    else
    {
        TRACE(1, "Error: %s already started, rc=%d \n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }
}

void CleanupWorker::run()
{
    using namespace std;

    TRACE(8, "%s Entry: starting thread.\n", __FUNCTION__);
    prctl(PR_SET_NAME, (unsigned long) (uintptr_t) thread_name_);

    int rc = ISMRC_OK;
    string err_msg;
    constexpr int NOWORK_WAIT_MS = 1000;

    while (true)
    {
        while (!isWork())
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!finish_)
            {
                cond_.wait_for(lock, std::chrono::milliseconds(NOWORK_WAIT_MS));
            }

            if (finish_)
            {
                break;
            }
        }

        if (isFinish())
        {
            break;
        }

        if (tokenBucket_)
        {
            rc = tokenBucket_->getTokens(batchSize_);
            if (rc == ISMRC_Closed)
            {
                TRACE(8, "%s closed while waiting for tokens, rc=%d.\n",
                        __FUNCTION__, rc);
                rc = ISMRC_OK;
                goto run_exit;
            }
            else if (rc != ISMRC_OK)
            {
                err_msg = "failure to getTokens";
                TRACE(1, "Error: %s, %s, rc=%d.\n", __FUNCTION__, err_msg.c_str(), rc);
                goto run_exit;
            }
        }

        rc  = cleanClientProxy();
        if (rc == ISMRC_NotFound)
        {
            TRACE(7,"%s No more work, going to wait.\n", __FUNCTION__);
            rc = ISMRC_OK;
        }
        else if (rc != ISMRC_OK)
        {
            err_msg = "cleanClientProxy failed";
            TRACE(1, "Error: %s, %s, rc=%d\n", __FUNCTION__, err_msg.c_str(), rc);
            goto run_exit;
        }
    }

    run_exit:
    int rc1 = dbClient_->close();
    if (rc1 != ISMRC_OK)
    {
        TRACE(9, "%s thread failure closing ActivityDBClient, rc=%d.\n", __FUNCTION__, rc1);
    }

    if (rc != ISMRC_OK && rc != ISMRC_Closed)
    {
        TRACE(1, "Error: %s thread exit because of error: rc=%d.\n", __FUNCTION__, rc);
        if (failureNotify_)
        {
            //error reporting function, move state to error
            failureNotify_->notifyFailure(rc, err_msg);
        }
    }

    {  //prepare for restart
        LockGuard lock(mutex_);
        started_ = false;
        finish_ = false;
        proxies_high_priority_.clear();
        proxies_low_priority_.clear();
        if (tokenBucket_)
		{
			tokenBucket_->restart();
		}
    }

    TRACE(5, "%s Exit: about to finish.\n", __FUNCTION__);
}


int CleanupWorker::stop()
{
    using namespace std;

    TRACE(5, "%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;

    {
        LockGuard lock(mutex_);
        if (started_)
        {
            finish_ = true;
			if (tokenBucket_)
			{
				tokenBucket_->close();
			}
        }
    }

    cond_.notify_one();
    if (thread_.get_id() != this_thread::get_id())
    {
        try
        {
            if (thread_.joinable())
                thread_.join();
        }
        catch (system_error& err)
        {
            rc = ISMRC_Error;
            ostringstream what;
            what << "CleanupWorker id=" << thread_.get_id() << " calling thread id=" << this_thread::get_id()
                    << " code=" << err.code() << " what=" << err.what();
            TRACE(1, "Error: %s, thread.join exception: %s; rc=%d\n",
                    __FUNCTION__, what.str().c_str(), rc);
        }
    }
    else
    {
        try
        {
            thread_.detach();
        }
        catch (system_error& err)
        {
            rc = ISMRC_Error;
            ostringstream what;
            what << "CleanupWorker id=" << thread_.get_id() << " code=" << err.code() << " what=" << err.what();
            TRACE(1, "Error: %s, thread.detach exception: %s; rc=%d\n",
                    __FUNCTION__, what.str().c_str(), rc);
        }
    }

    TRACE(5, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}

void CleanupWorker::addProxy(const std::string& proxy_uid)
{
    {
        LockGuard lock(mutex_);
        if (started_)
        {
            if (proxies_low_priority_.count(proxy_uid) == 0)
            {
                proxies_high_priority_.insert(proxy_uid);
            }
        }
    }
    cond_.notify_one();
}

bool CleanupWorker::isWork() const
{

    LockGuard lock(mutex_);
    if (started_)
    {
        if (!proxies_low_priority_.empty() || !proxies_high_priority_.empty())
        {
            return true;
        }
    }

    return false;
}

bool CleanupWorker::isFinish() const
{
    LockGuard lock(mutex_);
    return finish_;
}

int CleanupWorker::cleanClientProxy()
{
    using namespace std;

    TRACE(8, "%s Entry:\n", __FUNCTION__);

    int rc = ISMRC_OK;
    string target_proxy;
    int select_conn = 1;

    {
        LockGuard lock(mutex_);
        if (started_ && !finish_)
        {
            target_proxy = randomSelect(proxies_high_priority_);
            if (target_proxy.empty())
            {
                target_proxy = randomSelect(proxies_low_priority_);
                select_conn = 0;
            }
        }
    }

    if (!target_proxy.empty())
    {
        int num_updated = 0;
        int rc1 = dbClient_->disassociatedFromProxy(target_proxy, select_conn, batchSize_, &num_updated);
        if (rc1 == ISMRC_OK)
        {
            TRACE(7, "%s proxy_uid=%s, #cleaned=%d, %s clients.\n",
                    __FUNCTION__, target_proxy.c_str(), num_updated, (select_conn?"connected":"disconnected"));
        }
        else if (rc1 == ISMRC_NotFound)
        {
            if (select_conn == 1)
            {
                TRACE(7, "%s proxy_uid=%s, no more connected clients to clean.\n", __FUNCTION__, target_proxy.c_str());
                LockGuard lock(mutex_);
                proxies_low_priority_.insert(target_proxy);
                proxies_high_priority_.erase(target_proxy);
            }
            else if (select_conn == 0)
            {
                TRACE(7, "%s proxy_uid=%s, no more disconnected clients to clean, going to change status to REMOVED.\n",
                        __FUNCTION__, target_proxy.c_str());
                {
                    LockGuard lock(mutex_);
                    proxies_low_priority_.erase(target_proxy);
                }
                //Change status
                ProxyStateRecord proxy_state;
                proxy_state.proxy_uid = target_proxy;
                int rc2 = dbClient_->read(&proxy_state);
                if (rc2 == ISMRC_OK && (proxy_state.status == ProxyStateRecord::PX_STATUS_SUSPECT || proxy_state.status == ProxyStateRecord::PX_STATUS_LEAVE))
                {
                    rc2 = dbClient_->updateStatus_CondVersion(target_proxy, ProxyStateRecord::PX_STATUS_REMOVED, proxy_state.version);
                    if (rc2 == ISMRC_OK)
                    {
                        TRACE(7, "%s proxy_uid=%s, changed status to REMOVED.\n", __FUNCTION__, target_proxy.c_str());
                    }
                    else
                    {
                        TRACE(7, "%s After updateStatus_CondVersion, could not change status to REMOVED, proxy_state=%s, rc2=%d; Ignored.\n",
                                __FUNCTION__, proxy_state.toString().c_str(), rc2);
                    }
                }
                else
                {
                    TRACE(7, "%s After read, could not change status to REMOVED, proxy_state=%s, rc2=%d; Ignored.\n",
                            __FUNCTION__, proxy_state.toString().c_str(), rc2);
                }
            }
        }
        else
        {
            rc = rc1;
            TRACE(2, "Warning: %s, failed to disassociate, proxy_uid=%s, rc=%d\n", __FUNCTION__, target_proxy.c_str(), rc);
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}


int CleanupWorker::initRateLimit(uint32_t rate_iops, uint32_t max_rate_iops, uint32_t refill_interval_ms)
{
    int rc = ISMRC_OK;
    LockGuard lock(mutex_);
    if (started_)
    {
        return ISMRC_Error;
    }

    if (!tokenBucket_)
    {
        double refill_freq = 1000.0 / (static_cast<double>(refill_interval_ms));
        uint32_t bucket_sz = static_cast<uint32_t>(std::ceil(max_rate_iops/refill_freq));
        if (bucket_sz < 4*static_cast<uint32_t>(batchSize_))
        {
            bucket_sz = 4*batchSize_;
        }
        tokenBucket_.reset(new TokenBucket(rate_iops,bucket_sz));
    }
    else
    {
        rc = ISMRC_Error;
    }
    return rc;
}

int CleanupWorker::updateRateLimit(uint32_t rate_iops)
{
    int rc = ISMRC_OK;
    LockGuard lock(mutex_);
    if (tokenBucket_)
    {
        rc = tokenBucket_->setLimit(rate_iops);
    }
    return rc;
}


int CleanupWorker::tokenBucketRefill()
{
    TRACE(8,"%s Entry: \n", __FUNCTION__);

    int rc = ISMRC_OK;
    LockGuard lock(mutex_);
    if (tokenBucket_)
    {
        rc = tokenBucket_->refill();
    }
    return rc;
}


} /* namespace ism_proxy */
