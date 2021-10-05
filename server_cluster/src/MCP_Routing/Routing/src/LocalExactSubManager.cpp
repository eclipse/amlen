/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#include <boost/lexical_cast.hpp>
#include <LocalExactSubManager.h>

namespace mcp
{

spdr::ScTraceComponent* LocalExactSubManager::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Local,
		spdr::trace::ScTrConstants::Layer_ID_App, "LocalExactSubManager",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

LocalExactSubManager::LocalExactSubManager(
		const std::string& inst_ID,
		const MCPConfig& mcpConfig,
		LocalSubManager& localSubManager) :
				spdr::ScTraceContext(tc_, inst_ID, ""),
				config(mcpConfig),
				localSubManager(localSubManager),
				filterPublisher(),
				m_started(false),
				m_closed(false),
				m_recovered(false),
				m_subscribedTopics(),
				m_bf_base_sqn(0),
				m_bf_last_sqn(0),
				m_numUpdates(0),
				m_republish_base(false)
{
    Trace_Entry(this, "LocalExactSubManager()");
	// Initialize CountingBloomFilter pointer from config
	double desiredFPP = config.getBloomFilterErrorRate();
	int32_t counterSize = config.getBloomFilterCounterSize();

	// Compute the optimal parameters for the counting bloom filter
	std::pair<size_t, uint8_t> parameters = CountingBloomFilter::computeOptimalParameters(
					config.getBloomFilterProjectedNumElements(), desiredFPP);


	m_cbf.reset(
			new CountingBloomFilter(parameters.first, parameters.second,
					static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()), counterSize));
	m_bf.reset(
			new BloomFilter(parameters.first, parameters.second, static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType())));

}

LocalExactSubManager::~LocalExactSubManager()
{
    Trace_Entry(this, "~LocalExactSubManager()");
	m_subscribedTopics.clear();
}


MCPReturnCode LocalExactSubManager::setSubCoveringFilterPublisher(
		SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher)
{
    Trace_Entry(this, "setSubCoveringFilterPublisher()");

	if (subCoveringFilterPublisher)
	{
		filterPublisher = subCoveringFilterPublisher;
		return ISMRC_OK;
	}
	else
	{
	    Trace_Error(this, __FUNCTION__, "Error: subCoveringFilterPublisher is NULL", "RC", ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

MCPReturnCode LocalExactSubManager::start()
{
    Trace_Entry(this, "start()");

	if (m_closed)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already closed.", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (m_started)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already started.", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	if (m_recovered)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already recovered.", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	m_started = true;

	return ISMRC_OK;
}


MCPReturnCode LocalExactSubManager::recoveryCompleted()
{
    Trace_Entry(this, "recoveryCompleted()");

	MCPReturnCode rc = ISMRC_OK;

	if (m_closed)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already closed.", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (!m_started)
	{
	    Trace_Error(this, __FUNCTION__, "Error: not yet started.", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (m_recovered)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already recovered.", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	if (!filterPublisher)
	{
	    Trace_Error(this, __FUNCTION__, "Error: filterPublisher is NULL.", "RC", ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	m_republish_base = true;
	m_recovered = true;

	return rc;
}

MCPReturnCode LocalExactSubManager::close()
{
    Trace_Entry(this, "close()");
	MCPReturnCode rc = ISMRC_OK;
	m_closed = true;
	return rc;
}

MCPReturnCode LocalExactSubManager::subscribe(const char* topic)
{
    using namespace spdr;
    using namespace std;

	Trace_Entry(this, "subscribe()", topic);

	string strTopic(topic);

	if (m_subscribedTopics.count(strTopic) > 0)
	{
		Trace_Error(this, "subscribe()", "Error: subscription already exists", "topic", topic, "RC", ISMRC_ExistingSubscription);
		return ISMRC_ExistingSubscription;
	}

	m_subscribedTopics.insert(strTopic);

	vector<int32_t> updates;
	try
	{
	    updates = m_cbf->add(topic, strlen(topic));
	}
	catch (std::exception& e)
	{
	    Trace_Error(this, __FUNCTION__, "Error: failed to add to CBF",
	            "topic", topic, "what", e.what(),
	            "RC", boost::lexical_cast<string>(ISMRC_Error));
	    return ISMRC_Error;
	}

	if (m_cbf->estimateFPP() > config.getBloomFilterErrorRate())
	{
		size_t numCounters = 2 * m_cbf->getNumCounters();
		uint8_t numHashes = m_cbf->getNumHashes();
		uint8_t counterSize = m_cbf->getCounterSize();

		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "subscribe()","exact CBF passed FP threshold, resizing");
			buffer->addProperty<double>("desired", config.getBloomFilterErrorRate());
			buffer->addProperty("estimated", m_cbf->estimateFPP());
			buffer->addProperty("new-numBins", numCounters);
			buffer->invoke();
		}

		m_cbf.reset(
				new CountingBloomFilter(numCounters, numHashes, static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()),
						counterSize));

		for (SubscribedTopicsSet::iterator it = m_subscribedTopics.begin();
				it != m_subscribedTopics.end(); ++it)
		{
		    try
		    {
		        m_cbf->add(*it);
		    }
		    catch (std::exception& e)
		    {
		        Trace_Error(this, __FUNCTION__, "Error: failed to add to CBF while resizing",
		                "topic", topic, "what", e.what(),
		                "RC", boost::lexical_cast<string>(ISMRC_Error));
		        return ISMRC_Error;
		    }
		}

		m_republish_base = true;
		m_bf_updates_vec.clear();
	}

	if (m_recovered)
	{
		if (!m_republish_base)
		{
			m_numUpdates += updates.size();
			m_bf_updates_vec.insert(m_bf_updates_vec.end(),updates.begin(), updates.end());

			if ( static_cast<size_t>(m_numUpdates) > (m_cbf->getNumBits()/32))
			{
				if (ScTraceBuffer::isEventEnabled(tc_))
				{
					ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
							"subscribe()", "trimming updates, republish base");
					buffer->addProperty("#topics", m_subscribedTopics.size());
					buffer->addProperty("#attributes", filterPublisher->getNumBloomFilterUpdates(FilterTags::BF_ExactSub));
					buffer->addProperty("#updates", m_numUpdates);
					buffer->invoke();
				}

				m_republish_base = true;
			}
		}

		localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
	}

	Trace_Exit(this, "subscribe()");
	return ISMRC_OK;
}


MCPReturnCode LocalExactSubManager::unsubscribe(const char* topic)
{
    using namespace std;

    Trace_Entry(this, "unsubscribe()", topic);

    string strTopic(topic);

    if (m_subscribedTopics.find(strTopic) == m_subscribedTopics.end())
    {
        Trace_Error(this, __FUNCTION__, "Error: subscription not found",
                "topic", topic, "RC", ISMRC_Error);
        return ISMRC_NotFound;
    }

    m_subscribedTopics.erase(strTopic);
    try
    {
        vector<int32_t> updates = m_cbf->remove(topic, strlen(topic));

        if (!m_republish_base)
        {
            m_numUpdates += updates.size();
            m_bf_updates_vec.insert(m_bf_updates_vec.end(),updates.begin(), updates.end());
        }
    }
    catch (std::exception& e)
    {
        Trace_Error(this, __FUNCTION__, "Error: failed to remove from CBF",
                "topic", topic, "what", e.what(),
                "RC", boost::lexical_cast<string>(ISMRC_Error));
        return ISMRC_Error;
    }

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

    Trace_Exit(this, "unsubscribe()");
    return ISMRC_OK;
}


MCPReturnCode LocalExactSubManager::publishLocalExactBF()
{
    using namespace spdr;
    using namespace std;

    MCPReturnCode rc = ISMRC_OK;

    if (m_republish_base)
    {
        if (ScTraceBuffer::isEventEnabled(tc_))
        {
            ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
                    "publishLocalExactBF()", "republish base, re-sending BF-Base");
            buffer->addProperty("#topics", m_subscribedTopics.size());
            buffer->invoke();
        }
        rc = pushBloomFilterBase(); //ignore ISMRC_Closed
    }
    else if ( static_cast<int32_t>(filterPublisher->getNumBloomFilterUpdates(FilterTags::BF_ExactSub)) > config.getBloomFilterMaxAttributes() )
    {
        if (ScTraceBuffer::isEventEnabled(tc_))
        {
            ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
                    "publishLocalExactBF()", "trimming attributes, re-sending BF-Base");
            buffer->addProperty("#topics", m_subscribedTopics.size());
            buffer->addProperty("#attributes", filterPublisher->getNumBloomFilterUpdates(FilterTags::BF_ExactSub));
            buffer->addProperty("#updates", m_numUpdates);
            buffer->invoke();
        }
        rc = pushBloomFilterBase(); //ignore ISMRC_Closed
    }
    else if (!m_bf_updates_vec.empty())
    {
        try
        {
            m_bf_last_sqn = filterPublisher->publishBloomFilterUpdate(FilterTags::BF_ExactSub, m_bf_updates_vec);
            if (ScTraceBuffer::isDebugEnabled(tc_))
            {
                ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,
                        "publishLocalExactBF()", "sending updates, single attribute");
                buffer->addProperty("SQN", m_bf_last_sqn);
                buffer->addProperty("#topics", m_subscribedTopics.size());
                buffer->addProperty("#updates", m_bf_updates_vec.size());
                buffer->invoke();
            }
            m_bf_updates_vec.clear();
        }
        catch (MCPRuntimeError& re)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: MCPRuntimeError while publishing BF base",
                    "what", re.what(), "RC", re.getReturnCode());
            rc = re.getReturnCode();
        }
        catch (MCPIllegalArgumentError& iae)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: publishing update",
                    "what", iae.what(), "RC", iae.getReturnCode());
            rc = iae.getReturnCode();
        } catch (MCPIllegalStateError& ise)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: publishing update",
                    "what", ise.what(),"RC", ise.getReturnCode());
            rc = ise.getReturnCode();
        }
        catch (spdr::IllegalStateException& ise)
        {
            Trace_Error(this,"publishLocalExactBF()",
                    "Error: IllegalStateException while publishing update",
                    "what", ise.what(),"stacktrace", ise.getStackTrace());
            rc = ISMRC_Closed;
        }
        catch (spdr::SpiderCastLogicError& le)
        {
            Trace_Error(this,"publishLocalExactBF()","Error: SpiderCastLogicError while publishing update",
                    "what", le.what(), "stacktrace", le.getStackTrace());
            rc = ISMRC_ClusterInternalError;
            ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
        }
        catch (spdr::SpiderCastRuntimeError& re)
        {
            Trace_Error(this,"publishLocalExactBF()","Error: SpiderCastRuntimeError while publishing update",
                    "what", re.what(),"stacktrace", re.getStackTrace());
            rc = ISMRC_ClusterInternalError;
            ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
        }
        catch (std::bad_alloc& ba)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: publishing update","what", ba.what());
            rc = ISMRC_AllocateError;
        }
        catch (std::exception& e)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: publishing update","what", e.what());
            rc = ISMRC_Error;
        }
        catch (...)
        {
            Trace_Error(this,"publishLocalExactBF()", "Error: untyped exception (...) publishing update");
            rc = ISMRC_Error;
        }
    }

    return rc;
}

MCPReturnCode LocalExactSubManager::pushBloomFilterBase()
{
    using namespace spdr;

    MCPReturnCode rc = m_cbf->updateBloomFilter(m_bf);
    if (rc != ISMRC_OK)
    {
        Trace_Error(this, __FUNCTION__, "Error: failed to update BF from CBF", "RC", rc);
        return rc;
    }

    try
    {
        m_bf_base_sqn = filterPublisher->publishBloomFilterBase(FilterTags::BF_ExactSub,
                static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()), m_bf->getNumHashes(),
                m_bf->getNumBits(), m_bf->buffer());
        m_bf_last_sqn = m_bf_base_sqn;

        m_numUpdates = 0;
        m_bf_updates_vec.clear();
        m_republish_base = false;

        if (ScTraceBuffer::isEventEnabled(tc_))
        {
            ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "pushBloomFilterBase()");
            buffer->addProperty("#bits", m_bf->getNumBits());
            buffer->addProperty("SQN", m_bf_base_sqn);
            buffer->invoke();
        }
    }
    catch (MCPRuntimeError& re)
    {
        Trace_Error(this,"pushBloomFilterBase()", "Error: MCPRuntimeError while publishing BF base",
                "what", re.what(), "RC", re.getReturnCode());
        return re.getReturnCode();
    }
    catch (MCPIllegalArgumentError& ae)
    {
        Trace_Error(this,"pushBloomFilterBase()", "Error: MCPIllegalArgumentError while publishing BF base",
                "what", ae.what(), "RC", ae.getReturnCode());
        return ae.getReturnCode();
    }
    catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"pushBloomFilterBase()",
                "Error: IllegalStateException while publishing BF base",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    }
    catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this,"pushBloomFilterBase()","Error: SpiderCastLogicError while publishing BF base",
                "what", le.what(), "stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    }
    catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this,"pushBloomFilterBase()","Error: SpiderCastRuntimeError while publishing BF base",
                "what", re.what(),"stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Error(this,"pushBloomFilterBase()", "Error: publishing BF base","what", ba.what());
        return ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Error(this,"pushBloomFilterBase()", "Error: publishing BF base","what", e.what());
        return ISMRC_Error;
    }
    catch (...)
    {
        Trace_Error(this,"pushBloomFilterBase()", "Error: publishing BF base, untyped (...) exception");
        return ISMRC_Error;
    }

    return rc;
}

} /* namespace mcp */
