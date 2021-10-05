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

#include <algorithm>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <LocalWildcardSubManager.h>
#include "ismrc.h"

namespace mcp
{

spdr::ScTraceComponent* LocalWildcardSubManager::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Local,
        spdr::trace::ScTrConstants::Layer_ID_App, "LocalWildcardSubManager",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

LocalWildcardSubManager::LocalWildcardSubManager(const std::string& inst_ID,
		const MCPConfig& mcpConfig, const std::string& server_name, LocalSubManager& localSubManager, ControlManager& controlManager) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		myName(server_name),
		firstSpi(NULL),
		lastSpi(NULL),
		wcttRemote(0),
		wcttLocal(0),
		wcbfLocal(0),
		wcttLocal_last_published(0),
		wcbfLocal_last_published(0),
		wctt_baseSqn(0),
		wctt_updtSqn(0),
		wctt_baseSize(0),
		wctt_updtSize(0),
		rcfID(0),
		patID(0),
		statSqn(0),
		nInBF(0),
		pat_baseSqn(0),
		pat_updtSqn(0),
		config(mcpConfig),
		localSubManager(localSubManager),
		controlManager(controlManager),
		filterPublisher(),
		m_started(false),
		m_closed(false),
		m_recovered(false),
		m_1st_publish(0),
		reschedule(0),
		m_bf_WC_base_sqn(0),
		m_bf_WC_last_sqn(0),
		m_numUpdates_WC(0),
		m_republish_base_WC(false),
		m_subscriptionPattern_Map(),
		m_subscriptionPattern_publish_queue(),
		isConn(NULL),
		isConnSize(0)
{
    Trace_Entry(this, "LocalWildcardSubManager()");

	// Initialize CountingBloomFilter pointer from config
	double desiredFPP = config.getBloomFilterErrorRate();
	int32_t counterSize = config.getBloomFilterCounterSize();

	// Compute the optimal parameters for the counting bloom filter
	std::pair<size_t, uint8_t> parameters =
			CountingBloomFilter::computeOptimalParameters(
					config.getBloomFilterProjectedNumElements(), desiredFPP);
	m_cbf_WC.reset(
			new CountingBloomFilter(parameters.first, parameters.second,
					static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()), counterSize));
	m_bf_WC.reset(
			new BloomFilter(parameters.first, parameters.second, static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType())));

	myNameHash = (uint32_t)CityHash64(myName.c_str(), myName.size());
}

LocalWildcardSubManager::~LocalWildcardSubManager()
{
    Trace_Entry(this, "~LocalWildcardSubManager()");
    if ( isConn && isConnSize ) ism_common_free(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,32),isConn) ; 
}

MCPReturnCode LocalWildcardSubManager::subscribe(const char* topic)
{
    using namespace std;
    using namespace spdr;

	Trace_Entry(this, "subscribe()", topic);

	SubscriptionPattern pattern;
	size_t tlen = strlen(topic);
	MCPReturnCode rc = pattern.parseSubscription(topic, tlen);
	if (rc != ISMRC_OK)
	{
	    Trace_Error( this, "subscribe()", "Error: Cannot parse wildcard subscription pattern",
	            "subscription", topic, "RC", rc);
		return rc;
	}

	if (!pattern.isWildcard())
	{
	    Trace_Error(this, "subscribe()", "Error: Not a wildcard pattern, invalid argument",
	            "subscription", topic,
	            "pattern", pattern.toString(),
	            "RC", boost::lexical_cast<string>(ISMRC_ArgNotValid));
		return ISMRC_ArgNotValid;
	}

	string strTopic = string(topic,tlen);

	//add pattern to pattern map
	SubscriptionPatternInfo *pSpi ;
	SubscriptionPatternMap::iterator spm_it = m_subscriptionPattern_Map.find(pattern);
	if (spm_it == m_subscriptionPattern_Map.end())
	{
		//New pattern
		pSpi = new SubscriptionPatternInfo;
		pSpi->id = ++patID;
		pSpi->count = 1 ;
		pSpi->pattern = SubscriptionPattern_SPtr(new SubscriptionPattern(pattern));
		pSpi->m_subscribedTopics_WC.insert(std::pair<string,uint64_t>(strTopic,++rcfID));
		m_subscriptionPattern_Map[pattern] = pSpi ; //boost::make_tuple(0, free_index,	1); //sqn==0 means not yet published
		if ( lastSpi )
		{
			lastSpi->next = pSpi;
			pSpi->prev = lastSpi;
		}
		else
		{
			firstSpi = pSpi;
		}
		lastSpi = pSpi;
		Trace_Debug(this, "subscribe()", "New pattern",
		        "pattern", pattern.toString(),
		        "subscription", strTopic,
		        "rcfID", boost::lexical_cast<std::string>(rcfID));
	}
	else
	{
		pSpi = spm_it->second ;
		std::pair<SubscribedTopicsMap::iterator,bool> r = pSpi->m_subscribedTopics_WC.insert(std::make_pair(strTopic,++rcfID));
		if (!r.second)
		{
			// Found in the subscribed topics
		    Trace_Error(this, "subscribe", "Error: wildcard subscription already exists",
		            "subscription", strTopic, "RC", ISMRC_ExistingSubscription);
			return ISMRC_ExistingSubscription;
		}

		pSpi->count++; //increase count

		SubscriptionPatternInfo *p;
		for (p=pSpi->prev ; p && pSpi->count > p->count ; p=p->prev) ; //empty body
		if (p != pSpi->prev)
		{
			pSpi->prev->next = pSpi->next;
			if (pSpi->next)
				pSpi->next->prev = pSpi->prev;
			else
				lastSpi = pSpi->prev;
			if (p)
			{
				pSpi->prev = p;
				pSpi->next = p->next;
				p->next->prev = pSpi;
				p->next = pSpi;
			}
			else
			{
				pSpi->next = firstSpi ;
				pSpi->prev = NULL;
				firstSpi->prev = pSpi;
				firstSpi = pSpi;
			}
		}

        Trace_Debug(this, "subscribe", "Existing pattern",
                "pattern", pattern.toString(),
                "subscription", strTopic,
                "count", boost::lexical_cast<string>(pSpi->count),
                "inBF", (pSpi->inBF ? "T" : "F"));
	}

	if (pSpi->inBF)
	{
	    try
	    {
	        vector<int32_t> updates = m_cbf_WC->add(topic, tlen);
	        m_numUpdates_WC += updates.size();
	        m_bf_WC_updates_vec.insert(m_bf_WC_updates_vec.end(), updates.begin(),updates.end());
	    }
	    catch (std::exception& e)
	    {
	        Trace_Error(this, __FUNCTION__, "Error: adding to CBF failed", "what", e.what(), "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }
	    wcbfLocal++;
	    Trace_Debug(this, "subscribe", "Added to CBF",
	            "subscription", strTopic,
	            "wcbfLocal", boost::lexical_cast<string>(wcbfLocal));
	}
	else
	{
		rcf_publish_queue.push_back(std::make_pair(rcfID,String_SPtr(new String(strTopic))));
		wcttLocal++ ;
		wctt_updtSize += 12 + strTopic.size();
        Trace_Debug(this, "subscribe", "Added to RCF publish Q",
                "subscription", strTopic,
                "wcttLocal", boost::lexical_cast<string>(wcttLocal));
	}

	if (m_recovered)
	{
	    localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
	}
	else
	{
	    Trace_Debug(this, "subscribe()", "In recovery, skipping schedulePublishLocalBFTask");
	}

	Trace_Exit(this, "subscribe()", ISMRC_OK);
	return ISMRC_OK;
}

MCPReturnCode LocalWildcardSubManager::unsubscribe(const char* topic)
{
    using namespace spdr;
    using namespace std;

    Trace_Entry(this, "unsubscribe()", topic);

	SubscriptionPattern pattern;
	size_t tlen = strlen(topic);
	MCPReturnCode rc = pattern.parseSubscription(topic, tlen);
	if (rc != ISMRC_OK)
	{
	    Trace_Error(this, "unsubscribe()", "Error: Cannot parse wildcard pattern",
				"subscription", topic, "RC", rc);
		return rc;
	}

	if (!pattern.isWildcard())
	{
	    Trace_Error(this, "unsubscribe()", "Error: Not a wildcard pattern",
	            "subscription", topic, "pattern", pattern.toString(),
	            "RC", boost::lexical_cast<string>(ISMRC_ArgNotValid));
		return ISMRC_ArgNotValid;
	}

	std::string strTopic(topic);

	//Manage patterns
	SubscriptionPatternInfo *pSpi ;
	SubscriptionPatternMap::iterator spm_it = m_subscriptionPattern_Map.find(pattern);
	if (spm_it == m_subscriptionPattern_Map.end())
	{
	    Trace_Error(this, "unsubscribe()", "Error: Cannot find wildcard pattern",
	            "subscription", topic, "pattern", pattern.toString(),
                "RC", boost::lexical_cast<string>(ISMRC_NotFound));
		return ISMRC_NotFound;
	}

	pSpi = spm_it->second ;
	SubscribedTopicsMap::iterator stm_it = pSpi->m_subscribedTopics_WC.find(strTopic);
	if (stm_it == pSpi->m_subscribedTopics_WC.end())
	{
	    // Not Found in the subscribed topics
        Trace_Error(this, "unsubscribe()", "Error: Cannot find wildcard subscription",
                "subscription", topic, "pattern", pattern.toString(),
                "RC", boost::lexical_cast<string>(ISMRC_NotFound));
		return ISMRC_NotFound;
	}

	if (pSpi->inBF)
	{
	    try
	    {
	        vector<int32_t> updates = m_cbf_WC->remove(topic, tlen);
	        m_numUpdates_WC += updates.size();
	        m_bf_WC_updates_vec.insert(m_bf_WC_updates_vec.end(), updates.begin(), updates.end());
	    }
	    catch (std::exception& e)
	    {
	        Trace_Error(this, __FUNCTION__, "Error: remove from CBF failed", "what", e.what(), "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }

	    wcbfLocal--;
	    Trace_Debug(this, "unsubscribe()", "In BF, removed from CBF",
	            "wcbfLocal", boost::lexical_cast<string>(wcbfLocal));
	}
	else
	{
		rcf_publish_queue.push_back(std::make_pair(stm_it->second,String_SPtr()));
		wcttLocal-- ;
		wctt_updtSize += 12 ;
        Trace_Debug(this, "unsubscribe()", "Not in BF, removing from RCF, in publish Q",
                "rcfID", boost::lexical_cast<string>(stm_it->second),
                "wcttLocal", boost::lexical_cast<string>(wcttLocal));
	}

	pSpi->m_subscribedTopics_WC.erase(stm_it);
	pSpi->count--; //decrease usage count
	if (pSpi->count == 0) //last pattern usage
	{
	    Trace_Debug(this, "unsubscribe()", "Last pattern usage", "pattern",pattern.toString());

		if (pSpi->prev)
			pSpi->prev->next = pSpi->next;
		else
			firstSpi = pSpi->next;
		if (pSpi->next)
			pSpi->next->prev = pSpi->prev;
		else
			lastSpi = pSpi->prev;

		m_subscriptionPattern_Map.erase(spm_it);
		if (pSpi->inBF)
		{
			m_subscriptionPattern_publish_queue.push_back(std::make_pair(pSpi->id,SubscriptionPattern_SPtr()));
			nInBF--;
			Trace_Debug(this, "unsubscribe()", "Pattern removed from BF Subscription Patterns",
			        "pattern",pattern.toString(), "nInBF", boost::lexical_cast<string>(nInBF));
		}
		delete pSpi ;
	}
	else
	{
		SubscriptionPatternInfo *p;
		for (p=pSpi->next ; p && pSpi->count < p->count ; p=p->next) ; //empty body
		if (p != pSpi->next)
		{
			if (pSpi->prev)
				pSpi->prev->next = pSpi->next;
			else
				firstSpi = pSpi->next;
			pSpi->next->prev = pSpi->prev;
			if (p)
			{
				pSpi->prev = p->prev;
				pSpi->next = p;
				p->prev->next = pSpi;
				p->prev = pSpi;
			}
			else
			{
				pSpi->next = NULL ;
				pSpi->prev = lastSpi;
				lastSpi->next = pSpi;
				lastSpi = pSpi;
			}
		}

		Trace_Debug(this, "unsubscribe()", "Updated Subscription Pattern Info in ordered list",
		        "pattern", pattern.toString(), "count", boost::lexical_cast<string>(pSpi->count));
	}

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

	Trace_Exit(this, "unsubscribe()");
	return ISMRC_OK;
}


MCPReturnCode LocalWildcardSubManager::start()
{
    Trace_Entry(this, "start()");

	if (m_closed)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (m_started)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already started", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	if (m_recovered)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already recovered", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	MCPReturnCode rc = isConnMakeRoom(0);
	if ( rc != ISMRC_OK)
	{
	    Trace_Error(this, __FUNCTION__, "Error: cannot allocate with isConnMakeRoom", "RC", rc);
		return rc;
	}
	isConn[0] |= 1;

	m_started = true;

	Trace_Exit(this, "start()");
	return ISMRC_OK;
}


MCPReturnCode LocalWildcardSubManager::close()
{
    Trace_Entry(this, "close()");
	MCPReturnCode rc = ISMRC_OK;
	m_closed = true;
	return rc;
}

MCPReturnCode LocalWildcardSubManager::restoreSubscriptionPatterns(const std::vector<SubscriptionPattern_SPtr>& patterns)
{
    using namespace std;

    Trace_Entry(this, "restoreSubscriptionPatterns()", "#patterns", boost::lexical_cast<string>(patterns.size()));

	//this will be invoked between start() and recoveryCompleted to provide the patterns assigned to the WCBF before the crash/shutdown
	for (size_t i=0 ; i<patterns.size() ; i++)
	{
		SubscriptionPatternInfo *pSpi ;
		if (!patterns[i])
		{
		    Trace_Error(this, __FUNCTION__, "Error: NULL pattern", "RC", ISMRC_Error);
		    return ISMRC_Error;
		}

		SubscriptionPatternMap::iterator spm_it = m_subscriptionPattern_Map.find(*patterns[i]);
		if (spm_it == m_subscriptionPattern_Map.end())
		{
			//New pattern
			pSpi = new SubscriptionPatternInfo ;
			memset(pSpi,0,sizeof(*pSpi));
			pSpi->id = ++patID;
			pSpi->count = 0 ;
			pSpi->pattern = patterns[i] ;
			pSpi->m_subscribedTopics_WC = SubscribedTopicsMap();
			m_subscriptionPattern_Map[*patterns[i]] = pSpi ;
			if ( lastSpi )
			{
				lastSpi->next = pSpi;
				pSpi->prev = lastSpi;
			}
			else
				firstSpi = pSpi;
			lastSpi = pSpi;
		}
		else
			pSpi = spm_it->second ;
		pSpi->inBF = true ;
		nInBF++;

		Trace_Debug(this, "restoreSubscriptionPatterns()", "recovered to BF",
		        "pattern",spdr::toString(patterns[i]));
	}

	Trace_Exit(this, "restoreSubscriptionPatterns()");
	return ISMRC_OK;
}

MCPReturnCode LocalWildcardSubManager::recoveryCompleted()
{
    using namespace std;

    Trace_Entry(this, "recoveryCompleted()");

	MCPReturnCode rc = ISMRC_OK;

	if (m_closed)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already closed", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (!m_started)
	{
	    Trace_Error(this, __FUNCTION__, "Error: not started", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (m_recovered)
	{
	    Trace_Error(this, __FUNCTION__, "Error: already recovered", "RC", ISMRC_Error);
		return ISMRC_Error;
	}

	if (!filterPublisher)
	{
	    Trace_Error(this, __FUNCTION__, "Error: filterPublisher is NULL", "RC", ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}

	SubscriptionPatternInfo *p;
	int n = nInBF;
	for (p=firstSpi ; p && n ; p=p->next)
    {
        if (!p->inBF)
        {
            continue;
        }

        Trace_Debug(this, "recoveryCompleted()", "pattern on BF",
                "pattern", p->pattern->toString(),
                "id", boost::lexical_cast<string>(p->id));

        for (SubscribedTopicsMap::iterator jt =
                p->m_subscribedTopics_WC.begin();
                jt != p->m_subscribedTopics_WC.end(); jt++)
        {
            try
            {
                vector<int32_t> updates = m_cbf_WC->add(jt->first);
                m_numUpdates_WC += updates.size();
                m_bf_WC_updates_vec.insert(m_bf_WC_updates_vec.end(),
                        updates.begin(), updates.end());
            }
            catch (std::exception& e)
            {
                Trace_Error(this, __FUNCTION__, "Error: adding to CBF failed", "what", e.what(), "RC", ISMRC_Error);
                return ISMRC_Error;
            }
            wcbfLocal++;

            rcf_publish_queue.push_back(
                    std::make_pair(jt->second, String_SPtr()));
            wcttLocal--;
            wctt_updtSize += 12;
        }
        m_subscriptionPattern_publish_queue.push_back(
                std::make_pair(p->id, p->pattern));
        n--;
    }

	Trace_Debug(this, "recoveryCompleted()", "patterns on BF",
	                "#patterns", boost::lexical_cast<string>(nInBF),
	                "wcbfLocal", boost::lexical_cast<string>(wcbfLocal));

	m_republish_base_WC = true;
	m_recovered = true;

	Trace_Exit(this, "recoveryCompleted()");
	return rc;
}

MCPReturnCode LocalWildcardSubManager::setSubCoveringFilterPublisher(
		SubCoveringFilterPublisher_SPtr subCoveringFilterPublisher)
{
	if (subCoveringFilterPublisher)
	{
		filterPublisher = subCoveringFilterPublisher;
		return ISMRC_OK;
	}
	else
	{
	    Trace_Error(this, "setSubCoveringFilterPublisher()", "Error: NULL argument", "RC", ISMRC_NullArgument);
		return ISMRC_NullArgument;
	}
}

MCPReturnCode LocalWildcardSubManager::publishLocalUpdates()
{
    using namespace spdr;
    using namespace std;

    Trace_Entry(this, "publishLocalUpdates()");

	MCPReturnCode rc = ISMRC_OK;
	uint32_t wcttLocal0 = wcttLocal;
	reschedule = false;

	if (m_closed)
	{
	    Trace_Error(this, "publishLocalUpdates()", "Error: already closed", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (!m_started)
	{
	    Trace_Error(this, "publishLocalUpdates()", "Error: not started", "RC", ISMRC_ClusterNotAvailable);
		return ISMRC_ClusterNotAvailable;
	}

	if (!m_recovered )
	{
	    Trace_Event(this, "publishLocalUpdates()", "not recovered yet, skipping");
		return ISMRC_OK ;
	}

    if ( (rc = publishAll()) != ISMRC_OK)
    {
        Trace_Error(this,__FUNCTION__,"Error: publishAll failed, before optimization", "RC", rc);
        return rc;
    }

	m_1st_publish = true;

	//Optimizations!!!
	if ( wcttLocal + wcttRemote > config.getWcttHwm() )
	{
		std::vector<ByCount> allByCount;
		for (RemoteStatsMap::iterator it=remoteStats.begin() ; it != remoteStats.end() ; it++)
		{
			RemoteStatsInfo *st = &it->second;
			for (int i=st->stats.topicTree_Top.size()-1 ; i>=0 ; i--)
			{
				ByCount bc;
				bc.count = st->stats.topicTree_Top[i].second ;
				bc.hash   = st->hash ;
				bc.name = &st->name ;
				bc.index = it->first;
				bc.pat = st->stats.topicTree_Top[i].first;
				allByCount.push_back(bc);
			}
		}
		std::sort(allByCount.begin(), allByCount.end());
		int32_t n = wcttLocal + wcttRemote - config.getWcttLimit();

		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
		    spdr::ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this,
		            __FUNCTION__, "Doing Optimization!!! Above High WM");
		    buffer->addProperty("wcttLocal", wcttLocal);
		    buffer->addProperty("wcttRemote", wcttRemote);
		    buffer->addProperty("to BF", n);
		    buffer->invoke();
		}

		for(int i=allByCount.size()-1 ; n>0 && i>=0 ; i--)
		{
			ByCount &bc = allByCount[i];
			if (!isConnected(bc.index))
				continue ;

			if (bc.count < config.getWcttPatMinSize() && wcttLocal0 != wcttLocal)
				break ;

			if (bc.index == 0)  // That's me
			{
				SubscriptionPatternInfo *pSpi ;
				SubscriptionPatternMap::iterator spm_it = m_subscriptionPattern_Map.find(*(bc.pat));
				if (spm_it == m_subscriptionPattern_Map.end())
				{
				    if (ScTraceBuffer::isEventEnabled(tc_))
				    {
				        ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				                "publishLocalUpdates", "Weired!!!: A top 5 pattern not fount in local map");
				        buffer->addProperty("Pattern", bc.pat->toString());
				        buffer->invoke();
				    }
				    continue ;
				}
				pSpi = spm_it->second ;
				for (SubscribedTopicsMap::iterator jt = pSpi->m_subscribedTopics_WC.begin();
						jt != pSpi->m_subscribedTopics_WC.end(); jt++)
				{
				    try
				    {
				        vector<int32_t> updates = m_cbf_WC->add(jt->first);
				        m_numUpdates_WC += updates.size();
				        m_bf_WC_updates_vec.insert(m_bf_WC_updates_vec.end(), updates.begin(),updates.end());
				    }
				    catch (std::exception& e)
				    {
				        Trace_Error(this, __FUNCTION__, "Error: adding to CBF failed", "what", e.what(), "RC", ISMRC_Error);
				        return ISMRC_Error;
				    }
				    wcbfLocal++;

				    rcf_publish_queue.push_back(std::make_pair(jt->second,String_SPtr()));
				    wcttLocal-- ;
				    wctt_updtSize += 12 ;
				}
				m_subscriptionPattern_publish_queue.push_back(std::make_pair(pSpi->id,bc.pat));
				pSpi->inBF = true;
				nInBF++;
				if (ScTraceBuffer::isEventEnabled(tc_))
				{
				    ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				            __FUNCTION__, "Doing Optimization!!! moved pattern from TT to BF");
				    buffer->addProperty("pattern", bc.pat->toString());
				    buffer->addProperty("count", bc.count);
				    buffer->addProperty("index", bc.index);
				    buffer->addProperty("name", *bc.name);
				    buffer->invoke();
				}

			}
			n -= bc.count;

		} //for

		if (wcttLocal0 != wcttLocal)
		{
            if ( (rc = publishAll()) != ISMRC_OK)
            {
                Trace_Error(this,__FUNCTION__,"Error: publishAll failed, above HWM change", "RC", rc);
                return rc;
            }
		}
		reschedule = true;
	}
	else if ( wcttLocal + wcttRemote < config.getWcttLwm() )
	{
		std::vector<ByCount> allByCount;
		for (RemoteStatsMap::iterator it=remoteStats.begin() ; it != remoteStats.end() ; it++)
		{
			RemoteStatsInfo *st = &it->second;
			for (int i=st->stats.bloomFilter_Bottom.size()-1 ; i>=0 ; i--)
			{
				ByCount bc;
				bc.count = st->stats.bloomFilter_Bottom[i].second ;
				bc.hash   = st->hash ;
				bc.name = &st->name ;
				bc.index = it->first;
				bc.pat = st->stats.bloomFilter_Bottom[i].first;
				allByCount.push_back(bc);
			}
		}
		std::sort(allByCount.begin(), allByCount.end());
		uint32_t m = config.getWcttHwm() - (wcttLocal + wcttRemote);
		int32_t  n = config.getWcttLwm() - (wcttLocal + wcttRemote);

		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
		    spdr::ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this,
		            __FUNCTION__, "Doing Optimization!!! below LWM");
		    buffer->addProperty("wcttLocal", wcttLocal);
		    buffer->addProperty("wcttRemote", wcttRemote);
		    buffer->addProperty("from BF", n);
		    buffer->invoke();
		}

		for(uint i=0 ; i<allByCount.size() && n>0 ; i++)
		{
			ByCount &bc = allByCount[i];
			if ( m < bc.count )
				continue ;
			if (!isConnected(bc.index))
				continue ;
			if (bc.index == 0)  // That's me
			{
				SubscriptionPatternInfo *pSpi ;
				SubscriptionPatternMap::iterator spm_it = m_subscriptionPattern_Map.find(*(bc.pat));
				if (spm_it == m_subscriptionPattern_Map.end())
				{
				    if (ScTraceBuffer::isEventEnabled(tc_))
				    {
				        ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				                "publishLocalUpdates", "Might happen: A bottom 5 pattern not fount in local map");
				        buffer->addProperty("Pattern", bc.pat->toString());
				        buffer->invoke();
				    }
				    continue ;
				}
				pSpi = spm_it->second ;
				for (SubscribedTopicsMap::iterator jt = pSpi->m_subscribedTopics_WC.begin();
						jt != pSpi->m_subscribedTopics_WC.end(); jt++)
				{
				    try
				    {
				        vector<int32_t> updates = m_cbf_WC->remove(jt->first);
				        m_numUpdates_WC += updates.size();
				        m_bf_WC_updates_vec.insert(m_bf_WC_updates_vec.end(), updates.begin(),updates.end());
				    }
				    catch (std::exception& e)
				    {
				        Trace_Error(this, __FUNCTION__, "Error: remove from CBF failed", "what", e.what(), "RC", ISMRC_Error);
				        return ISMRC_Error;
				    }
				    wcbfLocal--;

				    rcf_publish_queue.push_back(std::make_pair(jt->second,String_SPtr(new String(jt->first))));
				    wcttLocal++ ;
				    wctt_updtSize += 12 + jt->first.size();
				}
				m_subscriptionPattern_publish_queue.push_back(std::make_pair(pSpi->id,SubscriptionPattern_SPtr()));
				pSpi->inBF = false;
				nInBF--;
				if (ScTraceBuffer::isEventEnabled(tc_))
				{
				    ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,
				            __FUNCTION__, "Doing Optimization!!! moved pattern from BF to TT");
				    buffer->addProperty("pattern", bc.pat->toString());
				    buffer->addProperty("count", bc.count);
				    buffer->addProperty("index", bc.index);
				    buffer->addProperty("name", *bc.name);
				    buffer->invoke();
				}

			}
			m -= bc.count;
			n -= bc.count;
		}

		if (wcttLocal0 != wcttLocal)
		{
			if ( (rc = publishAll()) != ISMRC_OK)
			{
			    Trace_Error(this,__FUNCTION__,"Error: publishAll failed, below LWM change", "RC", rc);
				return rc;
			}
		}
		reschedule = true;
	}

	if ( reschedule )
	{
	    if (m_started && m_recovered && !m_closed)
	    {
	    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
	    }
	}

	return rc;
}

MCPReturnCode LocalWildcardSubManager::publishAll()
{
    MCPReturnCode rc = ISMRC_OK;

    if ( (rc = publishLocalWildcardBF()) != ISMRC_OK)
    {
        Trace_Error(this, "publishAll()", "Error: calling publishLocalWildcardBF", "RC", rc);
        return rc;
    }

    if ( (rc = publishLocalWildcardPatterns()) != ISMRC_OK)
    {
        Trace_Error(this, "publishAll()", "Error: calling publishLocalWildcardPatterns", "RC", rc);
        return rc;
    }

    if ( (rc = publishRegularCoveringFilters()) != ISMRC_OK)
    {
        Trace_Error(this, "publishAll()", "Error: calling publishRegularCoveringFilters", "RC", rc);
        return rc;
    }

    if ( (rc = publishStats()) != ISMRC_OK)
    {
        Trace_Error(this, "publishAll()", "Error: calling publishStats", "RC", rc);
        return rc;
    }

    return rc;
}

MCPReturnCode LocalWildcardSubManager::publishStats()
{
    using namespace spdr;

    Trace_Entry(this, "publishStats()");

    int rc = ISMRC_OK;
    uint32_t dif = 0;

    if (wcttLocal > wcttLocal_last_published)
        dif += wcttLocal - wcttLocal_last_published;
    else
        dif += wcttLocal_last_published - wcttLocal;

    if (wcbfLocal > wcbfLocal_last_published)
        dif += wcbfLocal - wcbfLocal_last_published;
    else
        dif += wcbfLocal_last_published - wcbfLocal;

    //if (dif > config.getWcttLimit() / 50 || !m_1st_publish) //why the 2%?
    if (dif > 0 || !m_1st_publish)
    {
        uint64_t sqn = 0;
        RemoteStatsInfo st;
        SubscriptionPatternInfo *p;
        int n;
        st.hash = myNameHash;
        st.name = myName;
        st.stats.wildcardSubscriptions_NumOnTopicTree = wcttLocal;
        st.stats.wildcardSubscriptions_NumOnBloomFilter = wcbfLocal;
        n = m_subscriptionPattern_Map.size();
        if (n > 5)
        {
            n = 5;
        }

        for (p = lastSpi; p && n; p = p->prev)
        {
            if (p->inBF)
                continue;
            st.stats.topicTree_Top.push_back(
                    std::make_pair(p->pattern, p->count));
            n--;
        }

        n = nInBF < 5 ? nInBF : 5;

        for (p = firstSpi; p && n; p = p->next)
        {
            if (!p->inBF)
                continue;
            st.stats.bloomFilter_Bottom.push_back(
                    std::make_pair(p->pattern, p->count));
            n--;
        }
        rc = filterPublisher->publishSubscriptionStats(st.stats, &sqn);
        if (rc == ISMRC_OK)
        {
            Trace_Debug(this, "publishStats()", "published OK",
                    "stats", st.stats.toString(),
                    "SQN", boost::lexical_cast<string>(sqn));
            wcttLocal_last_published = wcttLocal;
            wcbfLocal_last_published = wcbfLocal;
            statSqn = sqn;
        }
        else
        {
            Trace_Error(this, "publishStats()", "Error: publishing Subscription stats",
                    "RC", rc);
        }

        //st.hash = rand(); //verify that this is needed

        remoteStats[0] = st;
        reschedule = 1;
    }

    Trace_Exit(this, "publishStats()", rc);
    return (MCPReturnCode) rc;
}

MCPReturnCode LocalWildcardSubManager::publishRegularCoveringFilters()
{
    using namespace std;
    using namespace spdr;

    Trace_Entry(this, __FUNCTION__);

	int rc = ISMRC_OK;

	if (filterPublisher->getNumRegularCoveringFilterUpdates() > (uint32_t)config.getBloomFilterMaxAttributes() ||
		filterPublisher->getSizeBytesRegularCoveringFilterUpdates() > filterPublisher->getSizeBytesRegularCoveringFilterBase() ||
		wctt_updtSize > wctt_baseSize || !m_1st_publish)
	{
	    uint32_t baseSize=0;
	    uint64_t baseSqn=0;
	    rcf_publish_queue.clear();
	    for(SubscriptionPatternMap::iterator jt= m_subscriptionPattern_Map.begin();
	            jt != m_subscriptionPattern_Map.end(); ++jt)
	    {
	        if (!jt->second->inBF)
	        {
	            for (SubscribedTopicsMap::iterator it = jt->second->m_subscribedTopics_WC.begin();
	                    it != jt->second->m_subscribedTopics_WC.end(); ++it)
	            {
	                rcf_publish_queue.push_back(std::make_pair(it->second,String_SPtr(new String(it->first))));
	                baseSize += 12 + it->first.size();
	            }
	        }
	    }

	    rc = filterPublisher->publishRegularCoveringFilterBase(rcf_publish_queue,&baseSqn);
	    if (rc == ISMRC_OK)
	    {
	        Trace_Event(this, __FUNCTION__, "publish base OK",
	                "nTopics", boost::lexical_cast<string>(rcf_publish_queue.size()),
	                "SQN", boost::lexical_cast<string>(baseSqn));

	        rcf_publish_queue.clear();
	        wctt_baseSqn = baseSqn;
	        wctt_baseSize = baseSize;
	        wctt_updtSize = 0;
	    }
	    else
	    {
	        Trace_Error(this, __FUNCTION__, "Error: publishing base","RC", rc);
	    }
	    reschedule = 1;
	}
	else if ( rcf_publish_queue.size() )
	{
		uint64_t updtSqn=0;
		rc = filterPublisher->publishRegularCoveringFilterUpdate(rcf_publish_queue,&updtSqn);
		if (rc == ISMRC_OK)
		{
		    Trace_Event(this, __FUNCTION__, "publish update OK",
		            "nTopics", boost::lexical_cast<string>(rcf_publish_queue.size()),
		            "SQN", boost::lexical_cast<string>(updtSqn));

			rcf_publish_queue.clear();
			wctt_updtSqn = updtSqn;
		}
		else
		{
		    Trace_Error(this, "publishRegularCoveringFilters()", "Error: publishing update","RC", rc);
		}
		reschedule = 1;
	}

	Trace_Exit(this, __FUNCTION__, rc);
	return (MCPReturnCode)rc;
}

MCPReturnCode LocalWildcardSubManager::publishLocalWildcardBF()
{
    using namespace spdr;

    Trace_Entry(this, "publishLocalWildcardBF()");

	MCPReturnCode rc = ISMRC_OK;

	if (!m_1st_publish )
	{
		m_republish_base_WC = true;
	}
	else if (m_cbf_WC->estimateFPP() > config.getBloomFilterErrorRate())
	{
		size_t numCounters = 2 * m_cbf_WC->getNumCounters();
		uint8_t numHashes = m_cbf_WC->getNumHashes();
		uint8_t counterSize = m_cbf_WC->getCounterSize();

		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this, "publishLocalWildcardBF()", "wildcard CBF passed false positive threshold, resizing");
			buffer->addProperty<double>("desired", config.getBloomFilterErrorRate());
			buffer->addProperty("estimated", m_cbf_WC->estimateFPP());
			buffer->addProperty("new-numBins", numCounters);
			buffer->invoke();
		}

		m_cbf_WC.reset(new CountingBloomFilter(numCounters, numHashes, static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()),counterSize));

		for(SubscriptionPatternMap::iterator jt= m_subscriptionPattern_Map.begin();
				jt != m_subscriptionPattern_Map.end(); ++jt)
		{
			if (jt->second->inBF)
			{
				for (SubscribedTopicsMap::iterator it = jt->second->m_subscribedTopics_WC.begin();
						it != jt->second->m_subscribedTopics_WC.end(); ++it)
				{
				    try
				    {
				        m_cbf_WC->add(it->first);
				    }
				    catch (std::exception& e)
				    {
				        Trace_Error(this, __FUNCTION__, "Error: adding to CBF failed", "what", e.what(), "RC", ISMRC_Error);
				        return ISMRC_Error;
				    }
				}
			}
		}

		m_republish_base_WC = true;
	}
	else if ( static_cast<size_t>(m_numUpdates_WC) > (m_cbf_WC->getNumBits()/32))
	{
		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,"publishLocalWildcardBF()", "trimming updates, re-sending BF-Base");
			buffer->addProperty("#attributes", (m_bf_WC_last_sqn - m_bf_WC_base_sqn));
			buffer->addProperty("#updates", m_numUpdates_WC);
			buffer->invoke();
		}
		m_republish_base_WC = true;
	}
	else if ( static_cast<int32_t>(filterPublisher->getNumBloomFilterUpdates(FilterTags::BF_WildcardSub)) > config.getBloomFilterMaxAttributes() )
	{
		if (ScTraceBuffer::isEventEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::event(this,__FUNCTION__, "trimming attributes, re-sending BF-Base");
			buffer->addProperty("#attributes", filterPublisher->getNumBloomFilterUpdates(FilterTags::BF_WildcardSub));
			buffer->addProperty("#updates", m_numUpdates_WC);
			buffer->invoke();
		}
		m_republish_base_WC = true;
	}

	if (m_republish_base_WC)
	{
	    //reschedule = 1;
	    Trace_Event(this,__FUNCTION__, "republish base, re-sending BF-Base");

	    rc = m_cbf_WC->updateBloomFilter(m_bf_WC);
	    if (rc != ISMRC_OK)
	    {
	        Trace_Error(this, __FUNCTION__, "Error: failed to update Bloom filter", "RC", rc);
	        return rc;
	    }

	    try
	    {
	        m_bf_WC_base_sqn = filterPublisher->publishBloomFilterBase(FilterTags::BF_WildcardSub,
	                static_cast<mcc_hash_HashType_t>(config.getBloomFilterHashType()), m_bf_WC->getNumHashes(),
	                m_bf_WC->getNumBits(), m_bf_WC->buffer());
	        m_bf_WC_last_sqn = m_bf_WC_base_sqn;

	        m_numUpdates_WC = 0;
	        m_bf_WC_updates_vec.clear();
	        m_republish_base_WC = false;

	        Trace_Event(this, __FUNCTION__, "published BF base","SQN", m_bf_WC_base_sqn);
	    }
	    catch (MCPRuntimeError& re)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing BF base",
	                "what", re.what(),
	                "RC", boost::lexical_cast<string>(re.getReturnCode()),
	                "stacktrace", re.getStackTrace());

	        return re.getReturnCode();
	    }
	    catch (MCPIllegalArgumentError& ae)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing BF base",
	                "what", ae.what(),
	                "RC", boost::lexical_cast<string>(ae.getReturnCode()),
	                "stacktrace", ae.getStackTrace());

	        return ae.getReturnCode();
	    }
	    catch (spdr::IllegalStateException& ise)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()",
	                "Error: IllegalStateException while publishing BF base",
	                "what", ise.what(),"stacktrace", ise.getStackTrace());
	        rc = ISMRC_Closed;
	    }
	    catch (spdr::SpiderCastLogicError& le)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()","Error: SpiderCastLogicError while publishing BF base",
	                "what", le.what(), "stacktrace", le.getStackTrace());
	        rc = ISMRC_ClusterInternalError;
	        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
	    }
	    catch (spdr::SpiderCastRuntimeError& re)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()","Error: SpiderCastRuntimeError while publishing BF base",
	                "what", re.what(),"stacktrace", re.getStackTrace());
	        rc = ISMRC_ClusterInternalError;
	        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
	    }
	    catch (std::bad_alloc& ba)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing BF base",
	                "what", ba.what(), "RC", ISMRC_AllocateError);
	        return ISMRC_AllocateError;
	    }
	    catch (std::exception& e)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing BF base",
	                "what", e.what(), "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }
	    catch (...)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing BF base, untyped (...) exception",
	                "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }
	}
	else if (!m_bf_WC_updates_vec.empty())
	{
	    //reschedule = 1;
	    try
	    {
	        m_bf_WC_last_sqn = filterPublisher->publishBloomFilterUpdate(FilterTags::BF_WildcardSub, m_bf_WC_updates_vec);
	        if (ScTraceBuffer::isDebugEnabled(tc_))
	        {
	            ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this,__FUNCTION__, "sending updates, single attribute");
	            buffer->addProperty("SQN", m_bf_WC_last_sqn);
	            buffer->addProperty("#updates", m_bf_WC_updates_vec.size());
	            buffer->invoke();
	        }
	        m_bf_WC_updates_vec.clear();
	    }
	    catch (MCPRuntimeError& re)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()", "Error: MCPRuntimeError while publishing BF base",
	                "what", re.what(), "RC", re.getReturnCode());
	        rc = re.getReturnCode();
	    }
	    catch (MCPIllegalArgumentError& iae)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing update",
	                "what", iae.what(),
	                "RC", boost::lexical_cast<string>(iae.getReturnCode()),
	                "stacktrace", iae.getStackTrace());
	        return iae.getReturnCode();
	    }
	    catch (MCPIllegalStateError& ise)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing update",
	                "what", ise.what(),
	                "RC", boost::lexical_cast<string>(ise.getReturnCode()),
	                "stacktrace", ise.getStackTrace());

	        return ise.getReturnCode();
	    }
	    catch (spdr::IllegalStateException& ise)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()",
	                "Error: IllegalStateException while publishing update",
	                "what", ise.what(),"stacktrace", ise.getStackTrace());
	        rc = ISMRC_Closed;
	    }
	    catch (spdr::SpiderCastLogicError& le)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()","Error: SpiderCastLogicError while publishing update",
	                "what", le.what(), "stacktrace", le.getStackTrace());
	        rc = ISMRC_ClusterInternalError;
	        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
	    }
	    catch (spdr::SpiderCastRuntimeError& re)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()","Error: SpiderCastRuntimeError while publishing update",
	                "what", re.what(),"stacktrace", re.getStackTrace());
	        rc = ISMRC_ClusterInternalError;
	        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
	    }
	    catch (std::bad_alloc& ba)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()","Error: publishing update",
	                "what", ba.what(), "RC", ISMRC_AllocateError);
	        return ISMRC_AllocateError;
	    }
	    catch (std::exception& e)
	    {
	        Trace_Error(this, "publishLocalWildcardBF()", "Error: publishing update",
	                "what", e.what(), "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }
	    catch (...)
	    {
	        Trace_Error(this,"publishLocalWildcardBF()",
	                "Error: publishing update, untyped (...) exception",
	                "RC", ISMRC_Error);
	        return ISMRC_Error;
	    }

	}

	Trace_Exit(this, "publishLocalWildcardBF()",rc);
	return rc;
}

MCPReturnCode LocalWildcardSubManager::storeSubscriptionPatterns()
{
    using namespace std;
    using namespace spdr;

	SubscriptionPatternInfo *p;
	std::vector<SubscriptionPattern_SPtr> pats;
	int n=nInBF ;
	for (p=lastSpi ; p && n ; p=p->prev)
	{
		if (!p->inBF) continue;
		pats.push_back(p->pattern);
		n--;
	}

	MCPReturnCode rc = controlManager.storeSubscriptionPatterns(pats);
	if ( rc != ISMRC_OK )
	{
	    Trace_Error(this, __FUNCTION__, "Error: storing patterns","RC", rc);
	}
	else
	{
	    Trace_Debug(this, __FUNCTION__, "Stored", "#patterns", boost::lexical_cast<string>(pats.size()));
	    if (ScTraceBuffer::isDumpEnabled(tc_))
	    {
	        ostringstream oss;
	        oss << "patterns={";
	        for (size_t i=0; i<pats.size(); ++i)
	        {
	            oss << spdr::toString(pats[i]) << "; ";
	        }
	        oss << "}";
	        ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, __FUNCTION__, oss.str());
	        buffer->invoke();
	    }
	}

	return rc;
}



MCPReturnCode LocalWildcardSubManager::publishLocalWildcardPatterns()
{
    using namespace spdr;

	int rc = ISMRC_OK;
	try
	{
		uint64_t sqn;
		if (filterPublisher->getNumWCSubscriptionPatternUpdates() > (uint32_t)config.getBloomFilterMaxAttributes() ||
			filterPublisher->getSizeBytesWCSubscriptionPatternUpdates() > filterPublisher->getSizeBytesWCSubscriptionPatternBase() ||
			!m_1st_publish)
		{
			SubscriptionPatternInfo *p;
			int n=nInBF ;
			m_subscriptionPattern_publish_queue.clear();
			for (p=lastSpi ; p && n ; p=p->prev)
			{
				if (!p->inBF) continue;
				m_subscriptionPattern_publish_queue.push_back(std::make_pair(p->id,p->pattern));
				n--;
			}
			rc = storeSubscriptionPatterns();
			rc = filterPublisher->publishWCSubscriptionPatternBase(m_subscriptionPattern_publish_queue, &sqn);
			if (rc == ISMRC_OK)
			{
			    Trace_Debug(this, __FUNCTION__, "Published base OK",
			            "#patterns", boost::lexical_cast<string>(m_subscriptionPattern_publish_queue.size()),
			            "SQN", boost::lexical_cast<string>(sqn));
				m_subscriptionPattern_publish_queue.clear();
				pat_baseSqn = sqn;
			}
			else
			{
			    Trace_Error(this,"publishWCSubscriptionPatternUpdate()", "Error: publishing update","RC", rc);
			}
			//reschedule = 1;
		}
		else if(m_subscriptionPattern_publish_queue.size())
		{
			rc = storeSubscriptionPatterns();
			rc = filterPublisher->publishWCSubscriptionPatternUpdate(m_subscriptionPattern_publish_queue, &sqn);
			if (rc == ISMRC_OK)
			{
                Trace_Debug(this, __FUNCTION__, "Published update OK",
                        "#patterns", boost::lexical_cast<string>(m_subscriptionPattern_publish_queue.size()),
                        "SQN", boost::lexical_cast<string>(sqn));
				m_subscriptionPattern_publish_queue.clear();
				pat_updtSqn = sqn;
			}
			else
			{
			    Trace_Error(this,"publishWCSubscriptionPatternUpdate()", "Error: publishing update", "RC", rc);
			}
			//reschedule = 1;
		}

	}
	catch (MCPIllegalArgumentError& iae)
	{
	    Trace_Error(this, "publishLocalWildcardPatterns()",	"Error: publishing WC SubscriptionPattern",
	            "what", iae.what(),
	            "RC", boost::lexical_cast<string>(iae.getReturnCode()),
	            "stacktrace", iae.getStackTrace());

		return iae.getReturnCode();
	}
	catch (MCPIllegalStateError& ise)
	{
	    Trace_Error(this, "publishLocalWildcardPatterns()", "Error: publishing WC SubscriptionPattern",
	            "what", ise.what(),
	            "RC", boost::lexical_cast<string>(ise.getReturnCode()),
	            "stacktrace", ise.getStackTrace());

		return ise.getReturnCode();
	}
	catch (std::exception& e)
	{
	    Trace_Error(this, "publishLocalWildcardPatterns()", "Error: publishing WC SubscriptionPattern",
	            "what", e.what(), "RC", ISMRC_Error);

		return ISMRC_Error;
	}

	return ISMRC_OK;
}

MCPReturnCode LocalWildcardSubManager::isConnMakeRoom(uint16_t index)
{
	if (index >= isConnSize*8)
	{
		size_t s = (index+64)/8 ;
		void *p;
		if (!(p = ism_common_realloc(ISM_MEM_PROBE_CPP(ism_memory_cluster_misc,33),isConn,s)))
			return ISMRC_AllocateError ;
		isConn = (uint8_t *)p ;
		memset(isConn+isConnSize,0,(s-isConnSize));
		isConnSize = s;
	}
	return ISMRC_OK;
}

int LocalWildcardSubManager::isConnected(uint16_t index)
{
	int i = index/8 ;
	int j = index&7 ;
	return isConn[i] & (1<<j);
}

int LocalWildcardSubManager::update(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID,
		const RemoteSubscriptionStats& stats)
{
    using namespace std;

    Trace_Entry(this, "update()", "uid", pServerUID, "stats", stats.toString());

	int rc;
	if ((rc = isConnMakeRoom(hClusterHandle->index)) != ISMRC_OK)
	{
	    Trace_Error(this, __FUNCTION__, "Error: failed isConnMakeRoom()", "RC", rc);
		return rc;
	}

	RemoteStatsMap::iterator it = remoteStats.find(hClusterHandle->index);
	if ( it != remoteStats.end() )
	{
		wcttRemote -= it->second.stats.wildcardSubscriptions_NumOnTopicTree ;
	}
	wcttRemote += stats.wildcardSubscriptions_NumOnTopicTree;
	RemoteStatsInfo st;
	st.name = string(pServerUID);
	st.hash = (uint32_t)CityHash64(pServerUID, st.name.size());
	st.stats = stats;
	remoteStats[hClusterHandle->index] = st;

	Trace_Dump(this, "update()", "inserted to RemoteStatsMap",
	        "index", boost::lexical_cast<string>(hClusterHandle->index),
	        "hash",  boost::lexical_cast<string>(st.hash),
	        "wcttRemote", boost::lexical_cast<string>(wcttRemote));

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

	Trace_Exit(this, __FUNCTION__);
	return ISMRC_OK;
}

int LocalWildcardSubManager::disconnected(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
    Trace_Entry(this, __FUNCTION__, "uid", pServerUID);

    int rc;
    if ((rc = isConnMakeRoom(hClusterHandle->index)) != ISMRC_OK)
    {
        Trace_Error(this, __FUNCTION__, "Error: failed isConnMakeRoom()", "RC", rc);
        return rc;
    }
    int i = hClusterHandle->index/8 ;
    int j = hClusterHandle->index&7 ;
    isConn[i] &= ~(1<<j);

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

    Trace_Exit(this, __FUNCTION__);
    return ISMRC_OK;
}

int LocalWildcardSubManager::connected(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
    Trace_Entry(this, __FUNCTION__, "uid", pServerUID);

    int rc;
    if ((rc = isConnMakeRoom(hClusterHandle->index)) != ISMRC_OK)
    {
        Trace_Error(this, __FUNCTION__, "Error: failed isConnMakeRoom()", "RC", rc);
        return rc;
    }
    int i = hClusterHandle->index/8 ;
    int j = hClusterHandle->index&7 ;
    isConn[i] |= (1<<j);

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

    Trace_Exit(this, __FUNCTION__);
    return ISMRC_OK;
}

int LocalWildcardSubManager::remove(
		ismCluster_RemoteServerHandle_t hClusterHandle, const char* pServerUID)
{
    Trace_Entry(this, __FUNCTION__, "uid", pServerUID);

    int rc;
    if ((rc = isConnMakeRoom(hClusterHandle->index)) != ISMRC_OK)
    {
        Trace_Error(this, __FUNCTION__, "Error: failed isConnMakeRoom()", "RC", rc);
        return rc;
    }
    int i = hClusterHandle->index/8 ;
    int j = hClusterHandle->index&7 ;
    isConn[i] &= ~(1<<j);
    RemoteStatsMap::iterator it = remoteStats.find(hClusterHandle->index);
    if ( it != remoteStats.end() )
    {
        wcttRemote -= it->second.stats.wildcardSubscriptions_NumOnTopicTree ;
        remoteStats.erase(it);
    }

    if (m_started && m_recovered && !m_closed)
    {
    	localSubManager.schedulePublishLocalBFTask(config.getPublishLocalBFTaskIntervalMillis());
    }

    Trace_Exit(this, __FUNCTION__);
    return ISMRC_OK;
}

} /* namespace mcp */
