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
 * ActivityWorker.cpp
 *  Created on: Nov 13, 2017
 */

#include <sys/prctl.h>
#include "ActivityWorker.h"

namespace ism_proxy
{

ActivityWorker::ActivityWorker(int num, int numBanksPerWorker, int batchSize, ActivityBanksVector& banksVec, ActivityDB& activityDB, FailureNotify* failureNotify) :
				num_(num),
				activityDB_(activityDB),
				work_(false),
				started_(false),
				finish_(false),
				error_(false),
				sum_read_latency_sec_(0.0),
				num_read_latency_(0),
				sum_update_latency_sec_(0.0),
				num_update_latency_(0),
				sum_bulk_update_latency_sec_(0.0),
				num_bulk_update_latency_(0),
				failureNotify_(failureNotify)
{
	TRACE(5, "%s Entry: num=%d, numBanksPerWorker=%d, batchSz=%d this=%p\n", __FUNCTION__, num, numBanksPerWorker, batchSize, this);
	if ((numBanksPerWorker < 1) || (batchSize < 1))
	{
		TRACE(1, "%s Error: num=%d, invalid_argument numBanksPerWorker=%d, batchSz=%d\n", __FUNCTION__, num, numBanksPerWorker, batchSize);
		throw std::invalid_argument("numBanksPerWorker & batchSz must be >0");
	}

	while (batchSize-- >0)
	{
		updateVec_.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	for (int i=(num*numBanksPerWorker); i<(num*numBanksPerWorker)+numBanksPerWorker; ++i)
	{
		myBanksVec_.push_back(banksVec.at(i));
		TRACE(8, "%s Entry: num=%d, using bank=%d\n", __FUNCTION__, num, i);
	}

	for (auto bank : myBanksVec_)
	{
	    bank->registerActivityNotify(static_cast<ActivityNotify*>(this));
	}

	std::string name("pxact.w");
	name.append(std::to_string(num));
	memcpy(thread_name_, name.c_str(), name.size());
	TRACE(8, "%s Exit: num=%d, thread-name=%s\n", __FUNCTION__, num, thread_name_);
}

ActivityWorker::~ActivityWorker()
{
    using namespace std;

    TRACE(8, "%s Entry: \n", __FUNCTION__);
    updateVec_.clear();
    myBanksVec_.clear();
    try
    {
        if (thread_.joinable())
            thread_.join();
    }
    catch (system_error& err)
    {
        ostringstream what;
        what << "ActivityWorker id=" << thread_.get_id() << ", calling thread id=" << this_thread::get_id() << ", code="
                << err.code() << " what=" << err.what();
        TRACE(1, "Error: %s, ActivityWorker %d, thread.join exception: %s;\n", __FUNCTION__, num_,
                what.str().c_str());
    }
}

int ActivityWorker::start()
{
	TRACE(5, "%s Entry: thread %d this=%p\n", __FUNCTION__, num_, this);
	LockGuard lock(mutex_);
	if (!started_)
	{
	    try
	    {
	        dbClient_ = activityDB_.getClient();
	        proxy_uid_ = activityDB_.getProxyUID();
	        TRACE(5, "%s starting worker thread %d\n", __FUNCTION__, num_);
	        thread_ = std::thread(&ActivityWorker::run, this);
	        started_ = true;
	        TRACE(5, "%s worker thread %d started successfully.\n", __FUNCTION__, num_);
	        return ISMRC_OK;
	    }
	    catch (std::system_error& se)
	    {
	        dbClient_->close();
	        TRACE(1, "Error: %s failed to create thread, caught system_error exception: what=%s, code=%d, rc=%d, thread %d \n",
	                __FUNCTION__, se.what(), se.code().value(),  ISMRC_Error, num_);
	        return ISMRC_Error;
	    }
	    catch (std::runtime_error& re)
	    {
	        dbClient_->close();
	        TRACE(1, "Error: %s failed to create thread, caught runtime_error exception: what=%s, rc=%d thread %d\n",
	                __FUNCTION__, re.what(), ISMRC_Error, num_);
	        return ISMRC_Error;
	    }
	}
	else
	{
		TRACE(1, "Error: %s already started, rc=%d, thread=%d \n", __FUNCTION__, ISMRC_Error, num_);
		return ISMRC_Error;
	}
}

void ActivityWorker::run()
{
    using namespace std;

	TRACE(5, "%s Entry: starting ActivityWorker thread %d this=%p.\n", __FUNCTION__, num_, this);
	prctl(PR_SET_NAME, (unsigned long)(uintptr_t)thread_name_);

	int rc = ISMRC_OK;
	string err_msg;

	while (true)
	{
		while (!work_.load() && !isWork())
		{
			std::unique_lock<std::mutex> lock(mutex_);

			if (!finish_)
			{
				cond_.wait_for(lock, std::chrono::milliseconds(20));
			}

			if (finish_)
			{
				break;
			}
		}

		if (error_) {
			TRACE(1, "Error: %s ActivityWorker thread %d, waiting for stop, rc=%d.\n",
							__FUNCTION__, num_,	rc);
			continue;
		}

		if (isFinish())
		{
			break;
		}

		for (unsigned b = 0; b < myBanksVec_.size(); ++b)
		{
			uint32_t numUpdates = 0;
			rc = myBanksVec_[b]->getUpdateBatch(updateVec_, numUpdates);
			if (rc != ISMRC_OK)
			{
			    err_msg = "failure to getUpdateBatch";
				TRACE(1, "Error: %s ActivityWorker thread %d, %s, rc=%d.\n",
				        __FUNCTION__, num_, err_msg.c_str(), rc);
				goto run_exit;
			}

			TRACE(8, "%s W=%d, b=%d, Batch: numUpdt=%d.\n", __FUNCTION__, num_,
					b, numUpdates);

			if (tokenBucket_)
			{
				rc = tokenBucket_->getTokens(numUpdates);
				if (rc == ISMRC_Closed)
				{
					TRACE(8, "%s ActivityWorker thread %d, closed while waiting for tokens, rc=%d.\n",
							__FUNCTION__, num_, rc);
					rc = ISMRC_OK;
					goto run_exit;
				}
				else if (rc != ISMRC_OK)
				{
				    err_msg = "failure to getTokens";
					TRACE(1, "Error: %s ActivityWorker thread %d, %s, rc=%d.\n",
							__FUNCTION__, num_,	err_msg.c_str(), rc);
					goto run_exit;
				}
			}

			if (numUpdates == 1)
			{
				bool isConnect = ClientStateRecord::isConnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(updateVec_[0]->conn_ev_type));
				bool isDisconnect = ClientStateRecord::isDisconnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(updateVec_[0]->disconn_ev_type));
				ClientUpdateVector processRecords;
				ClientUpdateRecord_SPtr connectRecord = std::make_shared<ClientUpdateRecord>(*(updateVec_[0]));
				ClientUpdateRecord_SPtr disconnectRecord = std::make_shared<ClientUpdateRecord>(*(updateVec_[0]));
				if(isDisconnect && isConnect)
				{
					//When getting both connect and disconnect events in the same record, needs to process them in the order they are received by using the connect and disconnect times in the record
					TRACE(9, "Info: %s Got both connect and disconnect events: %s\n",
												__FUNCTION__, updateVec_[0]->toString().c_str());
					connectRecord->disconn_ev_type = PXACT_ACTIVITY_TYPE_NONE; //For connect record set the disconnect event type to None
					disconnectRecord->conn_ev_type = PXACT_ACTIVITY_TYPE_NONE;// For disconnect record set the connect event type to None
					if(updateVec_[0]->connect_time < updateVec_[0]->disconnect_time)
					{
						processRecords.push_back(connectRecord);
						processRecords.push_back(disconnectRecord);
					}
					else
					{
						processRecords.push_back(disconnectRecord);
						processRecords.push_back(connectRecord);
					}
				}
				else
				{
					processRecords.push_back(updateVec_[0]);

				}

				for(ClientUpdateRecord_SPtr updateRecord : processRecords)
				{
					TRACE(9, "Info: %s Processing record: %s\n",
																	__FUNCTION__, updateRecord->toString().c_str());
					if ((updateRecord->dirty_flags	& ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD) > 0)
					{
						//New record in cache
						rc = write_new_record(updateRecord);
						if (rc != ISMRC_OK)
						{
							err_msg = "failed to write_new_record";
							TRACE(1, "Error: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
									__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
							goto run_exit;
						}
					}
					else
					{
						if ((updateRecord->dirty_flags
								& ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) > 0)
						{
							//Connectivity event
							rc = write_conn_ev(updateRecord);
							if (rc == ISMRC_NotFound)
							{
								err_msg = "failed to write_conn_ev because of ISMRC_NotFound, probably connection-stealing race, ignoring";
								TRACE(2, "Warning: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
										__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
							}
							else if (rc != ISMRC_OK)
							{
								err_msg = "failed to write_conn_ev";
								TRACE(1, "Error: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
										__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
								goto run_exit;
							}
						}
						else
						{
							//Activity & Metadata only
							double update_start = ism_common_readTSC();
							rc = dbClient_->updateOne(updateRecord.get(), proxy_uid_);
							sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
							num_update_latency_++;
							stats_.num_db_update++;
							if (rc != ISMRC_OK) {
								switch (rc) {
								case ISMRC_NotFound:
									TRACE(5, "%s ActivityWorker thread %d, failed to updateOne, update=%s, rc=%d. Re-Inserting\n",
											__FUNCTION__, num_, updateRecord->toString().c_str(), rc);
									updateRecord->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
									rc = dbClient_->insert(updateRecord.get(), proxy_uid_);
									if (rc == ISMRC_ExistingKey)
									{
										err_msg = "failed to re-insert because of ISMRC_ExistingKey, probably connection-stealing race, ignoring";
										TRACE(2, "Warning: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
												__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
									}
									else if (rc == PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID) {
										TRACE(5, "%s ActivityWorker thread %d, failed to updateOne because of invalid flag, update=%s, rc=%d. Ignored.\n",
												__FUNCTION__, num_, updateRecord->toString().c_str(), rc);
										rc = ISMRC_OK;
									}
									else if (rc != ISMRC_OK)
									{
										stats_.num_db_error++;
										err_msg = "failed to re-insert";
										TRACE(1, "Error: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
												__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
										goto run_exit;
									}
									break;
								case PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID:
									TRACE(5, "%s ActivityWorker thread %d, failed to updateOne because of invalid flag, update=%s, rc=%d. Ignored.\n",
											__FUNCTION__, num_, updateRecord->toString().c_str(), rc);
									rc = ISMRC_OK;
									break;
								default:
									stats_.num_db_error++;
									err_msg = "failed to updateOne";
									TRACE(1, "Error: %s ActivityWorker thread %d, %s, update=%s, rc=%d.\n",
											__FUNCTION__, num_, err_msg.c_str(), updateRecord->toString().c_str(), rc);
									goto run_exit;
								}
							}
						}
					}
				}
				connectRecord.reset();
				disconnectRecord.reset();
				processRecords.clear();
			}
			else if (numUpdates > 1)
			{
				double bulk_update_start = ism_common_readTSC();
				rc = dbClient_->updateBulk(updateVec_, numUpdates, proxy_uid_);
		        sum_bulk_update_latency_sec_ += (ism_common_readTSC() - bulk_update_start);
		        num_bulk_update_latency_++;
				stats_.num_db_bulk++;
				stats_.num_db_bulk_update += numUpdates;
				if (rc != ISMRC_OK)
				{
				    stats_.num_db_error++;
				    err_msg = "failed to updateBulk";
					TRACE(1, "Error: %s ActivityWorker thread %d, %s, numUpdates=%d, rc=%d.\n",
							__FUNCTION__, num_, err_msg.c_str(), numUpdates, rc);
					goto run_exit;
				}
			}

			//Manage the memory of the bank
			rc = myBanksVec_[b]->manageMemory();
			if (rc != ISMRC_OK)
			{
			    err_msg = "failed to manageMemory";
				TRACE(1, "Error: %s ActivityWorker thread %d, %s, rc=%d.\n",
						__FUNCTION__, num_, err_msg.c_str(), rc);
				goto run_exit;
			}

		} //for

		work_.store(isWork());

		if (isFinish())
		{
			break;
		}
	}

	run_exit:
	int rc1 = dbClient_->close();
    if (rc1 != ISMRC_OK)
    {
        TRACE(5, "%s ActivityWorker thread %d, failure closing ActivityDBClient, rc=%d.\n", __FUNCTION__, num_, rc1);
    }

	if (rc != ISMRC_OK && rc != ISMRC_Closed)
	{
		TRACE(1, "Error: %s ActivityWorker thread %d, exit because of error: rc=%d.\n",
				__FUNCTION__, num_, rc);
		if (failureNotify_)
		{
			error_ = true;
			TRACE(5, "%s ActivityWorker thread %d, notify failure, error_=%d\n",
						__FUNCTION__, num_, error_);
	        //error reporting function, move state to error
		    failureNotify_->notifyFailure(rc, err_msg);
		    // Reset error_ flag
		    error_ = false;
		    stop();
		}
	}

	{  //prepare for restart
	    LockGuard lock(mutex_);
	    started_ = false;
	    finish_ = false;
	    if (tokenBucket_)
	    {
	        tokenBucket_->restart();
	    }
	}

	TRACE(5, "%s Exit: ActivityWorker thread %d, about to finish, error_=%d\n",
			__FUNCTION__, num_, error_);
}

int ActivityWorker::stop()
{
    using namespace std;
    int rc = ISMRC_OK;

    TRACE(5, "%s Entry: ActivityWorker thread %d error=%d this=%p.\n", __FUNCTION__, num_, error_, this);
    if (!error_) {
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

		work_.store(false);
		cond_.notify_one();

        if (thread_.get_id() != this_thread::get_id())
        {
            try
            {
            	TRACE(5, "%s Joining ActivityWorker thread %d error=%d this=%p.\n", __FUNCTION__, num_, error_, this);
                if (thread_.joinable())
                    thread_.join();
            }
            catch (system_error& err)
            {
                rc = ISMRC_Error;
                ostringstream what;
                what << "ActivityWorker id=" << thread_.get_id() << " calling thread id=" << this_thread::get_id()
                        << " code=" << err.code() << " what=" << err.what();
                TRACE(1, "Error: %s, ActivityWorker %d, thread.join exception: %s; rc=%d\n",
                        __FUNCTION__, num_, what.str().c_str(), rc);
            }
        }
        else
        {
            try
            {
            	TRACE(5, "%s Detacting ActivityWorker thread %d error=%d this=%p.\n", __FUNCTION__, num_, error_, this);
                thread_.detach();
            }
            catch (system_error& err)
            {
                rc = ISMRC_Error;
                ostringstream what;
                what << "ActivityWorker id=" << thread_.get_id() << " code=" << err.code() << " what=" << err.what();
                TRACE(1, "Error: %s, thread.detach exception: %s; rc=%d\n",
                        __FUNCTION__, what.str().c_str(), rc);
            }
        }
	}
    TRACE(5, "%s Exit: ActivityWorker thread %d, rc=%d, error=%d\n", __FUNCTION__, num_, rc, error_);
    return rc;
}


void ActivityWorker::notifyWork()
{
	TRACE(8, "%s Entry: ActivityWorker %d.\n", __FUNCTION__, num_);
	work_.store(true);
	cond_.notify_one();
}

bool ActivityWorker::isFinish()
{
	LockGuard lock(mutex_);
	return finish_;
}

bool ActivityWorker::isWork()
{
	for (unsigned b=0; b<myBanksVec_.size(); ++b)
	{
		if (myBanksVec_[b]->getUpdateQueueSize() >0)
			return true;
	}

	return false;
}

int ActivityWorker::write_new_record(ClientUpdateRecord_SPtr& update)
{
	TRACE(8, "%s Entry: W=%d, update=%s \n", __FUNCTION__, num_, update->toString().c_str());

	int rc = ISMRC_OK;

	bool assume_db_contains = true;

	if (assume_db_contains)
	{
		client_state_record_.client_id = *update->client_id;
		client_state_record_.org = *update->org;
		double read_start = ism_common_readTSC();
		rc = dbClient_->read(&client_state_record_);
		double read_stop = ism_common_readTSC();
	    sum_read_latency_sec_ += (read_stop-read_start);
	    num_read_latency_++;
		stats_.num_db_read++;


		if (rc == ISMRC_OK)
		{
			update->dirty_flags = update->dirty_flags & (~ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD);
			if ((update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) > 0)
			{
				int num_retry = 3;
				do
				{
					if (ClientStateRecord::isConnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(update->conn_ev_type)))
					{
						if (ClientStateRecord::isDisconnectType(client_state_record_.conn_ev_type)
								|| client_state_record_.conn_ev_type == PXACT_ACTIVITY_TYPE_NONE)
						{
							double update_start = ism_common_readTSC();
							rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
							sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
							num_update_latency_++;
							stats_.num_db_update++;
						}
						else if (ClientStateRecord::isConnectType(client_state_record_.conn_ev_type))
						{
							rc = report_potential_connection_stealing(update);
							if (rc == ISMRC_OK)
							{
								double update_start = ism_common_readTSC();
								rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
								sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
								num_update_latency_++;
								stats_.num_db_update++;
							}
						}
						else
						{
							rc = ISMRC_Error;
						}
					}
					else if (ClientStateRecord::isDisconnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(update->disconn_ev_type)))
					{
						if (client_state_record_.proxy_uid == proxy_uid_)
						{
							double update_start = ism_common_readTSC();
							rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
							sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
							num_update_latency_++;
							stats_.num_db_update++;
							if (rc != ISMRC_OK)
							{
								TRACE(1, "Error: %s W=%d, updateOne_CondVersion failed, update=%s, ver=%ld, rc=%d.\n", __FUNCTION__,
										num_, update->toString().c_str(), client_state_record_.version, rc);
							}
						}
						else
						{
							//Do nothing
							TRACE(7, "%s Disconnect event, update=%s, but DB client-state=%s, is from another proxy, do nothing. client_state_record proxy_uid:%s and updating proxy_uid:%s \n",
									__FUNCTION__, update->toString().c_str(), client_state_record_.toString().c_str(), client_state_record_.proxy_uid.c_str(), proxy_uid_.c_str());
						}
					}
					else
					{
						rc = ISMRC_Error;
						TRACE(1, "Error: %s W=%d, wrong event type, update=%s, rc=%d.\n", __FUNCTION__, num_,
								update->toString().c_str(), rc);
					}

					if (rc == ISMRC_NotFound) //condition failed
					{
						num_retry--;
						rc = dbClient_->read(&client_state_record_);
					}
					else //OK, Error
					{
						num_retry = 0;
					}
				}
				while ( rc == ISMRC_OK && num_retry > 0);
			}
			else
			{
				//Update (Activity & Metadata only)
				double update_start = ism_common_readTSC();
				rc = dbClient_->updateOne(update.get(), proxy_uid_);
				sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
				num_update_latency_++;
				stats_.num_db_update++;
			}
		}
		else if (rc == ISMRC_NotFound)
		{
			assume_db_contains = false;
		}
	}

	if (!assume_db_contains)
	{
		rc = dbClient_->insert(update.get(), proxy_uid_);
		stats_.num_db_insert++;
		if (rc == ISMRC_ExistingKey)
		{
			update->dirty_flags = update->dirty_flags & (~ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD);
			if ((update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) > 0)
			{
				client_state_record_.client_id = *update->client_id;
				client_state_record_.org = *update->org;
				int num_retry = 3;
				do
				{
					//RMW
					rc = rmw_conn_ev(update);
					if (rc == ISMRC_NotFound)
					{
						num_retry--;
					}
					else //OK, Error
					{
						num_retry = 0;
					}
				}
				while ( num_retry > 0);
			}
			else
			{
				//Update (Activity & Metadata only)
				double update_start = ism_common_readTSC();
				rc = dbClient_->updateOne(update.get(), proxy_uid_);
				sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
				num_update_latency_++;
				stats_.num_db_update++;
			}
		}
	}

	if (rc != ISMRC_OK && rc != ISMRC_NotFound)
	{
	    stats_.num_db_error++;
	}

	return rc;
}

/*
 * Assuming: (update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) > 0
 */
int ActivityWorker::write_conn_ev(ClientUpdateRecord_SPtr& update)
{
	TRACE(8, "%s Entry: W=%d, update=%s \n", __FUNCTION__, num_, update->toString().c_str());

	int rc = ISMRC_OK;

	client_state_record_.client_id = *update->client_id;
	client_state_record_.org = *update->org;
	const int NUM_RETRY = 5;
	int num_retry = NUM_RETRY;
	do
	{
		//RMW
		rc = rmw_conn_ev(update);
		if (rc == ISMRC_NotFound)
		{
			num_retry--;
			if (num_retry == 0)
			{
			    TRACE(2, "Warning: %s W=%d, rmw_conn_ev failed %d attempts, update=%s, num_retry=%d, rc=%d \n",
			            __FUNCTION__, num_, NUM_RETRY, update->toString().c_str(), num_retry, rc);
			}
			else
			{
			    TRACE(7, "%s W=%d, rmw_conn_ev failed, update=%s, num_retry=%d, rc=%d \n",
			                        __FUNCTION__, num_, update->toString().c_str(), num_retry, rc);
			}
		}
		else //OK, Error
		{
			num_retry = 0;
		}
	}
	while ( num_retry > 0);

	return rc;
}

/*
 * Assuming: (update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) > 0
 * Assuming: client_state_record_.client_id = *update->client_id;
 */
int ActivityWorker::rmw_conn_ev(ClientUpdateRecord_SPtr& update)
{
	TRACE(8, "%s Entry: W=%d, update=%s \n", __FUNCTION__, num_, update->toString().c_str());
	double read_start = ism_common_readTSC();
	int rc = dbClient_->read(&client_state_record_);
    double read_stop = ism_common_readTSC();
    sum_read_latency_sec_ += (read_stop-read_start);
    num_read_latency_++;
	stats_.num_db_read++;


	if (rc == ISMRC_OK)
	{
		if (ClientStateRecord::isConnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(update->conn_ev_type)))
		{
			if (ClientStateRecord::isDisconnectType(client_state_record_.conn_ev_type)
					|| client_state_record_.conn_ev_type == PXACT_ACTIVITY_TYPE_NONE)
			{
				double update_start = ism_common_readTSC();
				rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
				sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
				num_update_latency_++;
				stats_.num_db_update++;
				if (rc != ISMRC_OK && rc != ISMRC_NotFound)
				{
				    TRACE(1, "Error: %s W=%d, updateOne_CondVersion failed, update=%s, ver=%ld, rc=%d.\n", __FUNCTION__,
				            num_, update->toString().c_str(), client_state_record_.version, rc);
				}
			}
			else if (ClientStateRecord::isConnectType(client_state_record_.conn_ev_type))
			{
				rc = report_potential_connection_stealing(update);
				if (rc == ISMRC_OK)
				{
					double update_start = ism_common_readTSC();
					rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
					sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
					num_update_latency_++;
					stats_.num_db_update++;
				}
			}
			else
			{
				rc = ISMRC_Error;
			}
		}
		else if (ClientStateRecord::isDisconnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(update->disconn_ev_type)))
		{
			if (client_state_record_.proxy_uid == proxy_uid_)
			{
				double update_start = ism_common_readTSC();
				rc = dbClient_->updateOne_CondVersion(update.get(), proxy_uid_, client_state_record_.version);
				sum_update_latency_sec_ += (ism_common_readTSC() - update_start);
				num_update_latency_++;
				stats_.num_db_update++;
				if (rc != ISMRC_OK && rc != ISMRC_NotFound)
				{
					TRACE(1, "Error: %s W=%d, updateOne_CondVersion failed, update=%s, ver=%ld, rc=%d.\n", __FUNCTION__,
							num_, update->toString().c_str(), client_state_record_.version, rc);
				}
			}
			else
			{
				//Do nothing
				TRACE(5, "%s Disconnect event, update=%s, but DB client-state=%s, is from another proxy, do nothing. client_state_record proxy_uid:%s and updating proxy_uid:%s \n",
						__FUNCTION__, update->toString().c_str(), client_state_record_.toString().c_str(), client_state_record_.proxy_uid.c_str(), proxy_uid_.c_str());
			}
		}
		else
		{
			rc = ISMRC_Error;
			TRACE(1, "Error: %s W=%d, wrong event type, update=%s, rc=%d.\n", __FUNCTION__, num_,
					update->toString().c_str(), rc);
		}
	}
	else if (rc == ISMRC_NotFound)
	{
	    TRACE(5, "%s W=%d, read not found, Re-Inserting rc=%d, update=%s.\n",
	            __FUNCTION__, num_, rc, update->toString().c_str());
	    update->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
	    rc = dbClient_->insert(update.get(), proxy_uid_);
	}
	else
	{
		TRACE(1, "Error: %s W=%d, read failed, update=%s, rc=%d.\n",
		        __FUNCTION__, num_, update->toString().c_str(),	rc);
	}

	if (rc != ISMRC_OK && rc != ISMRC_NotFound)
	{
	    stats_.num_db_error++;
	}

	TRACE(8, "%s Exit: W=%d, rc=%d\n", __FUNCTION__, num_, rc);
	return rc;
}

int ActivityWorker::report_potential_connection_stealing(ClientUpdateRecord_SPtr& update)
{
	TRACE(7,"%s Potential connection stealing: client_state=%s, update=%s, this.proxy_uid=%s; IGNORED\n",
			__FUNCTION__, client_state_record_.toString().c_str(), update->toString().c_str(), proxy_uid_.c_str());
	//TODO
	return ISMRC_OK;
}

int ActivityWorker::initRateLimit(uint32_t rate_iops, uint32_t max_rate_iops, uint32_t refill_interval_ms)
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
		if (bucket_sz < 2*updateVec_.size())
		{
			bucket_sz = 2*updateVec_.size();
		}
		tokenBucket_.reset(new TokenBucket(rate_iops,bucket_sz));
	}
	else
	{
		rc = ISMRC_Error;
	}
	return rc;
}

int ActivityWorker::updateRateLimit(uint32_t rate_iops)
{
	int rc = ISMRC_OK;
	LockGuard lock(mutex_);
	if (tokenBucket_)
	{
		rc = tokenBucket_->setLimit(rate_iops);
	}
	return rc;
}


int ActivityWorker::tokenBucketRefill()
{
	TRACE(8,"%s Entry: \n", __PRETTY_FUNCTION__);

	int rc = ISMRC_OK;
	LockGuard lock(mutex_);
	if (tokenBucket_)
	{
		rc = tokenBucket_->refill();
	}
	return rc;
}

ActivityStats ActivityWorker::getActivityStats()
{
    LockGuard lock(mutex_);

    if (num_read_latency_ > 0)
        stats_.avg_db_read_latency_ms = static_cast<int64_t>(ceil(1000 * sum_read_latency_sec_ / num_read_latency_));
    else
        stats_.avg_db_read_latency_ms = 0;

    sum_read_latency_sec_ = 0;
    num_read_latency_ = 0;

    if (num_update_latency_ > 0)
        stats_.avg_db_update_latency_ms = static_cast<int64_t>(ceil(1000 * sum_update_latency_sec_ / num_update_latency_));
    else
        stats_.avg_db_update_latency_ms = 0;

    sum_update_latency_sec_ = 0;
    num_update_latency_ = 0;

    if (num_bulk_update_latency_ > 0)
        stats_.avg_db_bulk_update_latency_ms = static_cast<int64_t>(ceil(1000 * sum_bulk_update_latency_sec_ / num_bulk_update_latency_));
    else
        stats_.avg_db_bulk_update_latency_ms = 0;

    sum_bulk_update_latency_sec_ = 0;
    num_bulk_update_latency_ = 0;

    return stats_;
}

} /* namespace ism_proxy */
