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

#include "SubCoveringFilterPublisherImpl.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include "SubCoveringFilterWireFormat.h"

namespace mcp
{

spdr::ScTraceComponent* SubCoveringFilterPublisherImpl::tc_ = spdr::ScTr::enroll(
        mcp::trace::Component_Name,
        mcp::trace::SubComponent_Control,
		spdr::trace::ScTrConstants::Layer_ID_App,
		"SubCoveringFilterPublisherImpl",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);


SubCoveringFilterPublisherImpl::SubCoveringFilterPublisherImpl(const std::string& inst_ID,
		spdr::MembershipService& memService) :
		spdr::ScTraceContext(tc_, inst_ID, ""),
		membershipService(memService),
		sqn_(0),
		sqn_retained_stats_(0),
		sqn_monitoring_status_(0),
		sqn_removed_servers_(0),
		sqn_restored_notin_view_(0)
{
	byteBuffer = ByteBuffer::createByteBuffer((uint32_t) 1024);
	permitted_BF_Tags.insert(FilterTags::BF_ExactSub);
	permitted_BF_Tags.insert(FilterTags::BF_WildcardSub);
}

SubCoveringFilterPublisherImpl::~SubCoveringFilterPublisherImpl()
{
}

uint64_t SubCoveringFilterPublisherImpl::publishBloomFilterBase(
		const std::string& tag,
		const mcc_hash_HashType_t bfType,
		const int16_t numHash,
		const int32_t numBins,
		const char* buffer)
{
	using namespace std;
	using namespace spdr;
	Trace_Entry(this,"publishBloomFilterBase()","tag",tag);

	if (permitted_BF_Tags.count(tag) == 0)
	{
		std::string what = "Illegal BF tag: " + tag;
		throw MCPIllegalArgumentError(what, ISMRC_ArgNotValid);
	}

	if (numBins % 8 != 0)
	{
		std::ostringstream what;
		what << "numBins must be multiple of 8: " << numBins;
		throw MCPIllegalArgumentError(what.str() , ISMRC_ArgNotValid);
	}

	boost::mutex::scoped_lock lock(mutex);

	//manage sequence numbers
	uint64_t base_sqn = ++sqn_;
	uint32_t num_updates = 0;

	BFTagInfo_Map::iterator it = bfTagInfoMap.find(tag);
	if (it == bfTagInfoMap.end())
	{
		// the first sequence number is 1'.
		pair<BFTagInfo_Map::iterator, bool> res = bfTagInfoMap.insert(
				pair<string, BFTagInfo_Map::mapped_type>(tag, SqnInfo()));
		if (!res.second)
		{
			throw MCPRuntimeError("Cannot insert tag to BFTagInfo_Map",
					ISMRC_Error);
		}
		res.first->second.base = base_sqn;
		res.first->second.last_update = base_sqn;

		it = res.first;
	}
	else
	{
		it->second.base = base_sqn;
		it->second.last_update = base_sqn;
		num_updates = it->second.num_updates;
		it->second.num_updates = 0;
		it->second.updates_size_bytes = 0;
	}



	ostringstream keyB;
	keyB << tag << FilterTags::BF_Base_Suffix;

	//the value wire format (network byte order / big-endian) :
	// (uint64_t) sqnNum,
	// (int16_t) bfType,
	// (int16_t) numHash,
	// (int32_t) numBins, (must be a multiple of 8)
	// buffer

	byteBuffer->reset();
	byteBuffer->writeLong((int64_t) (base_sqn));
	byteBuffer->writeShort(static_cast<int16_t>(bfType));
	byteBuffer->writeShort(static_cast<int16_t>(numHash));
	byteBuffer->writeInt(static_cast<int32_t>(numBins));
	if (numBins>0)
	{
		byteBuffer->writeByteArray(buffer, static_cast<std::size_t>(numBins / 8));
	}

	it->second.base_size_bytes = byteBuffer->getDataLength();

	membershipService.setAttribute(keyB.str(),
			spdr::Const_Buffer(
					static_cast<int32_t>(byteBuffer->getDataLength()),
					byteBuffer->getBuffer()));

	//delete all updates
	//updates are numbered 1:(last_update_sqn-base_sqn)
	for (size_t k = 1; k <= num_updates; ++k)
	{
		ostringstream keyU;
		keyU << tag << FilterTags::BF_Update_Suffix << dec << k;
		membershipService.removeAttribute(keyU.str());
	}

	Trace_Exit<uint64_t>(this,"publishBloomFilterBase()",base_sqn);

	return base_sqn;
}

uint64_t SubCoveringFilterPublisherImpl::publishBloomFilterUpdate(
		const std::string& tag, const std::vector<int32_t>& binUpdates)
{
	using namespace std;

	if (permitted_BF_Tags.count(tag) == 0)
	{
		std::string what = "Illegal BF tag: " + tag;
		throw MCPIllegalArgumentError(what, ISMRC_ArgNotValid);
	}

	boost::mutex::scoped_lock lock(mutex);

	//manage sequence numbers
	BFTagInfo_Map::iterator it = bfTagInfoMap.find(tag);
	if (it == bfTagInfoMap.end())
	{
		throw MCPIllegalStateError("Cannot update BF without a base",
				ISMRC_Error);
	}

	it->second.last_update = ++sqn_;
	it->second.num_updates += 1;


	//updates are numbered 1:num_updates
	ostringstream keyU;
	keyU << tag << FilterTags::BF_Update_Suffix << dec << it->second.num_updates;

	//the value wire format (network byte order / big-endian) :
	// (uint64_t) sqnNum
	// (int32_t) numUpdates,
	// ((int32_t) update)  x numUpdates

	byteBuffer->reset();
	byteBuffer->writeLong(static_cast<int64_t>(it->second.last_update));
	byteBuffer->writeInt(static_cast<int32_t>(binUpdates.size()));
	for (size_t i = 0; i < binUpdates.size(); ++i)
	{
		byteBuffer->writeInt(binUpdates[i]);
	}

	it->second.updates_size_bytes += byteBuffer->getDataLength();

	membershipService.setAttribute(keyU.str(),
			spdr::Const_Buffer(
					static_cast<int32_t>(byteBuffer->getDataLength()),
					byteBuffer->getBuffer()));

	return it->second.last_update;
}

uint64_t SubCoveringFilterPublisherImpl::removeBloomFilter(
		const std::string& tag)
{
	using namespace std;

	return publishBloomFilterBase(tag,ISM_HASH_TYPE_NONE,0,0,NULL);
}

uint32_t SubCoveringFilterPublisherImpl::getNumBloomFilterUpdates(const std::string& tag) const
{
	boost::mutex::scoped_lock lock(mutex);
	BFTagInfo_Map::const_iterator it = bfTagInfoMap.find(tag);
	if (it != bfTagInfoMap.end())
	{
		return it->second.num_updates;
	}
	else
	{
		return 0;
	}
}

uint32_t SubCoveringFilterPublisherImpl::getSizeBytesBloomFilterUpdates(const std::string& tag) const
{
	boost::mutex::scoped_lock lock(mutex);
	BFTagInfo_Map::const_iterator it = bfTagInfoMap.find(tag);
	if (it != bfTagInfoMap.end())
	{
		return it->second.updates_size_bytes;
	}
	else
	{
		return 0;
	}
}

uint32_t SubCoveringFilterPublisherImpl::getSizeBytesBloomFilterBase(const std::string& tag) const
{
	boost::mutex::scoped_lock lock(mutex);
	BFTagInfo_Map::const_iterator it = bfTagInfoMap.find(tag);
	if (it != bfTagInfoMap.end())
	{
		return it->second.base_size_bytes;
	}
	else
	{
		return 0;
	}
}

/*
 * @see SubCoveringFilterPublisher
 */
 int SubCoveringFilterPublisherImpl::publishWCSubscriptionPatternBase(
		const std::vector<SubscriptionPatternUpdate>& subPatternBase,
		uint64_t* sqn)
 {
		using namespace std;

		Trace_Entry(this, "publishWCSubscriptionPatternBase()");

		boost::mutex::scoped_lock lock(mutex);
		int rc = ISMRC_OK;

		uint32_t num_updates = wcspSqnInfo_.num_updates;

		wcspSqnInfo_.base = ++sqn_;
		wcspSqnInfo_.last_update = wcspSqnInfo_.base;
		wcspSqnInfo_.num_updates = 0;
		wcspSqnInfo_.updates_size_bytes = 0;

		try
		{
			//	the value wire format (network byte order / big-endian) :
			//	(uint64_t) sqnNum,
			//	(uint32_t) numPatterns
			//  {
			//    (uint64_t) SubscriptionPatternID
			//	  (SubscriptionPattern) removal is marked by an empty pattern
			//  } x numPattern
			byteBuffer->reset();
			byteBuffer->writeLong(static_cast<int64_t>(wcspSqnInfo_.base));
			byteBuffer->writeInt(static_cast<int32_t>(subPatternBase.size()));
			for (unsigned int i = 0; i < subPatternBase.size(); ++i)
			{
				byteBuffer->writeLong(static_cast<int64_t>(subPatternBase[i].first));
				if (subPatternBase[i].second)
				{
					SubCoveringFilterWireFormat::writeSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, *(subPatternBase[i].second),byteBuffer);
				}
				else
				{
					rc = ISMRC_NullArgument;
					Trace_Exit(this, "publishWCSubscriptionPatternBase()", rc);
					return rc;
				}
			}

			wcspSqnInfo_.base_size_bytes = byteBuffer->getDataLength();

			membershipService.setAttribute(FilterTags::BF_Wildcard_SubscriptionPattern_Base,
					spdr::Const_Buffer(
							static_cast<int32_t>(byteBuffer->getDataLength()),
							byteBuffer->getBuffer()));

			//delete all updates
			//updates are numbered 1:num_updates
			for (size_t k = 1; k <= num_updates; ++k)
			{
				ostringstream keyU;
				keyU << FilterTags::BF_Wildcard_SubscriptionPattern_Update
						<< dec << k;
				membershipService.removeAttribute(keyU.str());
			}

			*sqn = wcspSqnInfo_.base;

		} catch (MCPRuntimeError& mre)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: MCPRuntimeError while publishing WCSP base",
		            "what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()), "stacktrace", mre.getStackTrace());
		    rc = mre.getReturnCode();
		} catch (spdr::IllegalStateException& ise)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()",
		            "Error: IllegalStateException while publishing WCSP base",
		            "what", ise.what(),"stacktrace", ise.getStackTrace());
		    rc = ISMRC_Closed;
		} catch (spdr::SpiderCastLogicError& le)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: SpiderCastLogicError while publishing WCSP base",
		            "what", le.what(), "stacktrace", le.getStackTrace());
			rc = ISMRC_ClusterInternalError;
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
		} catch (spdr::SpiderCastRuntimeError& re)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: SpiderCastRuntimeError while publishing WCSP base",
		            "what", re.what(),"stacktrace", re.getStackTrace());
			rc = ISMRC_ClusterInternalError;
			ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
		}
		catch (std::bad_alloc& ba)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: bad_alloc while publishing WCSP base",
		            "what", ba.what());
		    rc = ISMRC_AllocateError;
		}
		catch (std::exception& e)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: exception while publishing WCSP base",
		            "what", e.what());
			rc = ISMRC_Error;
		} catch (...)
		{
		    Trace_Error(this,"publishWCSubscriptionPatternBase()","Error: untyped exception while publishing WCSP base");
		    rc = ISMRC_Error;
		}

		Trace_Exit(this, "publishWCSubscriptionPatternBase()", rc);
		return rc;

 }

/*
 * @see SubCoveringFilterPublisher
 */
 int SubCoveringFilterPublisherImpl::publishWCSubscriptionPatternUpdate(
		const std::vector<SubscriptionPatternUpdate>& subPatternUpdate,
		uint64_t* sqn)
 {
	 using namespace std;

	 Trace_Entry(this, "publishWCSubscriptionPatternUpdate()");

	 boost::mutex::scoped_lock lock(mutex);
	 int rc = ISMRC_OK;

	 wcspSqnInfo_.num_updates +=1;
	 wcspSqnInfo_.last_update = ++sqn_;

	 ostringstream keyU;
	 keyU << FilterTags::BF_Wildcard_SubscriptionPattern_Update << dec << wcspSqnInfo_.num_updates;

	 try
	 {
		 //	the value wire format (network byte order / big-endian) :
		 //	(uint64_t) sqnNum,
		 //	(uint32_t) numPatterns
		 //  {
		 //    (uint64_t) SubscriptionPatternID
		 //	  (SubscriptionPattern) removal is marked by an empty pattern
		 //  } x numPattern
		 byteBuffer->reset();
		 byteBuffer->writeLong(static_cast<int64_t>(wcspSqnInfo_.last_update));
		 byteBuffer->writeInt(static_cast<int32_t>(subPatternUpdate.size()));
		 const SubscriptionPattern empty;
		 for (unsigned int i = 0; i < subPatternUpdate.size(); ++i)
		 {
			 byteBuffer->writeLong(static_cast<int64_t>(subPatternUpdate[i].first));
			 if (subPatternUpdate[i].second)
			 {
				 SubCoveringFilterWireFormat::writeSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, *(subPatternUpdate[i].second),byteBuffer);
			 }
			 else
			 {
				 SubCoveringFilterWireFormat::writeSubscriptionPattern(SubCoveringFilterWireFormat::ATTR_VERSION, empty,byteBuffer);
			 }
		 }

		 wcspSqnInfo_.updates_size_bytes += byteBuffer->getDataLength();

		 membershipService.setAttribute(keyU.str(),
				 spdr::Const_Buffer(
						 static_cast<int32_t>(byteBuffer->getDataLength()),
						 byteBuffer->getBuffer()));

		 *sqn = wcspSqnInfo_.last_update;

	 } catch (MCPRuntimeError& mre)
	 {
	     Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: MCPRuntimeError while publishing WCSP update",
				 "what", mre.what(),"RC", boost::lexical_cast<std::string>(mre.getReturnCode()),"stacktrace", mre.getStackTrace());
		 rc = mre.getReturnCode();
	 } catch (spdr::IllegalStateException& ise)
     {
         Trace_Error(this,"publishWCSubscriptionPatternUpdate()",
                 "Error: IllegalStateException while publishing WCSP update",
                 "what", ise.what(),"stacktrace", ise.getStackTrace());
         rc = ISMRC_Closed;
     } catch (spdr::SpiderCastLogicError& le)
	 {
	     Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: SpiderCastLogicError while publishing WCSP update",
	             "what", le.what(),"stacktrace", le.getStackTrace());
		 rc = ISMRC_ClusterInternalError;
		 ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
	 } catch (spdr::SpiderCastRuntimeError& re)
	 {
	     Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: SpiderCastRuntimeError while publishing WCSP update",
	             "what", re.what(),"stacktrace", re.getStackTrace());
		 rc = ISMRC_ClusterInternalError;
		 ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
	 }
	 catch (std::bad_alloc& ba)
	 {
	     Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: bad_alloc while publishing WCSP update",
	             "what", ba.what());
	     rc = ISMRC_AllocateError;
	 }
	 catch (std::exception& e)
	 {
		 Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: exception while publishing WCSP update",
		         "what", e.what());
		 rc = ISMRC_Error;
	 } catch (...)
	 {
		 Trace_Error(this,"publishWCSubscriptionPatternUpdate()","Error: untyped exception while publishing WCSP update");
		 rc = ISMRC_Error;
	 }

	 Trace_Exit(this, "publishWCSubscriptionPatternUpdate()", rc);
	 return rc;

 }

/*
 * @see SubCoveringFilterPublisher
 */
 uint32_t SubCoveringFilterPublisherImpl::getNumWCSubscriptionPatternUpdates() const
 {
	 boost::mutex::scoped_lock lock(mutex);
	 return wcspSqnInfo_.num_updates;
 }

/*
 * @see SubCoveringFilterPublisher
 */
 uint32_t SubCoveringFilterPublisherImpl::getSizeBytesWCSubscriptionPatternUpdates() const
 {
	 boost::mutex::scoped_lock lock(mutex);
	 return wcspSqnInfo_.updates_size_bytes;
 }

 uint32_t SubCoveringFilterPublisherImpl::getSizeBytesWCSubscriptionPatternBase() const
 {
	 boost::mutex::scoped_lock lock(mutex);
	 return wcspSqnInfo_.base_size_bytes;
 }

int SubCoveringFilterPublisherImpl::publishSubscriptionStats(
		const RemoteSubscriptionStats& stats, uint64_t* sqn)
{
	using namespace std;

	Trace_Entry(this, "publishSubscriptionStats()");

	boost::mutex::scoped_lock lock(mutex);
	int rc = ISMRC_OK;

	try
	{
		//	the value wire format (network byte order / big-endian) :
		//	(uint64_t) sqnNum,
		//
		// uint64_t wildcardSubscriptions_NumOnBloomFilter
		// uint64_t wildcardSubscriptions_NumOnTopicTree
		// uint32_t num_topicTree_Top
		// {
		//    SubscriptionPattern
		//    uint32_t pattern_frequency
		// } x num_topicTree_Top
		// uint32_t num_bloomFilter_Bottom
		// {
		//    SubscriptionPattern
		//    uint32_t pattern_frequency
		// } x num_bloomFilter_Bottom

		byteBuffer->reset();
		byteBuffer->writeLong(static_cast<int64_t>(++sqn_));

		rc = SubCoveringFilterWireFormat::writeSubscriptionStats(SubCoveringFilterWireFormat::ATTR_VERSION, stats,byteBuffer);
		if (rc == ISMRC_OK)
		{
			membershipService.setAttribute(FilterTags::WCSub_Stats,
					spdr::Const_Buffer(
							static_cast<int32_t>(byteBuffer->getDataLength()),
							byteBuffer->getBuffer()));
			*sqn = sqn_;;
		}

    } catch (MCPRuntimeError& mre)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: MCPRuntimeError while publishing WCSub_Stats base",
                "what", mre.what(),
                "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
                "stacktrace", mre.getStackTrace());
        rc = mre.getReturnCode();
    } catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishSubscriptionStats()",
                "Error: IllegalStateException while publishing WCSub_Stats base",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: SpiderCastLogicError while publishing WCSub_Stats base",
                "what", le.what(), "stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    } catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: SpiderCastRuntimeError while publishing WCSub_Stats base",
                "what", re.what(), "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    } catch (std::bad_alloc& ba)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: exception while publishing WCSub_Stats base",
                "what", ba.what());
        rc = ISMRC_AllocateError;
    } catch (std::exception& e)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: exception while publishing WCSub_Stats base",
                "what", e.what());
        rc = ISMRC_Error;
    } catch (...)
    {
        Trace_Error(this, "publishSubscriptionStats()",
                "Error: untyped exception while publishing WCSub_Stats base");
        rc = ISMRC_Error;
    }

	Trace_Exit(this, "publishSubscriptionStats()",rc);
	return rc;
}

/*
 * @see SubCoveringFilterPublisher
 */
int SubCoveringFilterPublisherImpl::publishRegularCoveringFilterBase(
		const std::vector<RegularCoveringFilterUpdate>& rcfBase,
		uint64_t* sqn)
{
	using namespace std;

	Trace_Entry(this, "publishRegularCoveringFilterBase()");

	boost::mutex::scoped_lock lock(mutex);
	int rc = ISMRC_OK;

	uint32_t num_updates = rcfSqnInfo_.num_updates;

	rcfSqnInfo_.base = ++sqn_;
	rcfSqnInfo_.last_update = rcfSqnInfo_.base;
	rcfSqnInfo_.num_updates = 0;
	rcfSqnInfo_.updates_size_bytes = 0;

	try
	{
		//	the value wire format (network byte order / big-endian) :
		//	(uint64_t) sqnNum,
		//	(uint32_t) numFilters
		//  {
		//    (uint64_t) RegularCoveringFilterID
		//	  (string) (length, data)
		//  } x numFilters
		size_t len = 12 ;
		for (unsigned int i = 0; i < rcfBase.size(); ++i)
		{
			if (rcfBase[i].second)
			{
				len += 12 + rcfBase[i].second->size() ;
			}
			else
			{
				rc = ISMRC_NullArgument;
				Trace_Exit(this, "publishBloomFilterBase()", rc);
				return rc;
			}
		}

		byteBuffer->reset();
		byteBuffer->setPosition(len);
		byteBuffer->reset();
		byteBuffer->writeLong(static_cast<int64_t>(rcfSqnInfo_.base));
		byteBuffer->writeInt(static_cast<int32_t>(rcfBase.size()));
		for (unsigned int i = 0; i < rcfBase.size(); ++i)
		{
			byteBuffer->writeLong(static_cast<int64_t>(rcfBase[i].first));
			if (rcfBase[i].second)
			{
				byteBuffer->writeString(*(rcfBase[i].second));
			}
			else
			{
				rc = ISMRC_NullArgument;
				Trace_Exit(this, "publishBloomFilterBase()", rc);
				return rc;
			}
		}

		rcfSqnInfo_.base_size_bytes = byteBuffer->getDataLength();

		membershipService.setAttribute(FilterTags::RCF_Base,
				spdr::Const_Buffer(
						static_cast<int32_t>(byteBuffer->getDataLength()),
						byteBuffer->getBuffer()));

		//delete all updates
		//updates are numbered 1:num_updates
		for (size_t k = 1; k <= num_updates; ++k)
		{
			ostringstream keyU;
			keyU << FilterTags::RCF_Update
					<< dec << k;
			membershipService.removeAttribute(keyU.str());
		}

		*sqn = rcfSqnInfo_.base;

	} catch (MCPRuntimeError& mre)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
				"Error: MCPRuntimeError while publishing RCF base",
				"what", mre.what(),
				"RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
				"stacktrace", mre.getStackTrace());
		rc = mre.getReturnCode();
	} catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishRegularCoveringFilterBase()",
                "Error: IllegalStateException while publishing RCF base",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
				"Error: SpiderCastLogicError while publishing RCF base",
				"what", le.what(), "stacktrace", le.getStackTrace());
		rc = ISMRC_ClusterInternalError;
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
	} catch (spdr::SpiderCastRuntimeError& re)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
				"Error: SpiderCastRuntimeError while publishing RCF base",
				"what", re.what(), "stacktrace", re.getStackTrace());
		rc = ISMRC_ClusterInternalError;
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
	}
	catch (std::bad_alloc& ba)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
	            "Error: exception while publishing RCF base",
	            "what", ba.what());
	    rc = ISMRC_AllocateError;
	}
	catch (std::exception& e)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
				"Error: exception while publishing RCF base",
				"what", e.what());
		rc = ISMRC_Error;
	} catch (...)
	{
	    Trace_Error(this,"publishRegularCoveringFilterBase()",
				"Error: untyped exception while publishing RCF base");
		rc = ISMRC_Error;
	}

	Trace_Exit(this, "publishBloomFilterBase()", rc);
	return rc;
}

/*
 * @see SubCoveringFilterPublisher
 */
int SubCoveringFilterPublisherImpl::publishRegularCoveringFilterUpdate(
		const std::vector<RegularCoveringFilterUpdate>& rcfUpdate,
		uint64_t* sqn)
{
	using namespace std;

	Trace_Entry(this, "publishRegularCoveringFilterUpdate()");

	boost::mutex::scoped_lock lock(mutex);
	int rc = ISMRC_OK;

	rcfSqnInfo_.num_updates +=1;
	rcfSqnInfo_.last_update = ++sqn_;

	ostringstream keyU;
	keyU << FilterTags::RCF_Update << dec << rcfSqnInfo_.num_updates;

	try
	{
		//	the value wire format (network byte order / big-endian) :
		//	(uint64_t) sqnNum,
		//	(uint32_t) numFilters
		//  {
		//    (uint64_t) RegularCoveringFilterID
		//	  (string) (length, data) empty string for delete
		//  } x numFilters
		byteBuffer->reset();
		byteBuffer->writeLong(static_cast<int64_t>(rcfSqnInfo_.last_update));
		byteBuffer->writeInt(static_cast<int32_t>(rcfUpdate.size()));
		const std::string empty;
		for (unsigned int i = 0; i < rcfUpdate.size(); ++i)
		{
			byteBuffer->writeLong(static_cast<int64_t>(rcfUpdate[i].first));
			if (rcfUpdate[i].second)
			{
				byteBuffer->writeString(*(rcfUpdate[i].second));
			}
			else
			{
				byteBuffer->writeString(empty);
			}
		}

		rcfSqnInfo_.updates_size_bytes += byteBuffer->getDataLength();

		membershipService.setAttribute(keyU.str(),
				spdr::Const_Buffer(
						static_cast<int32_t>(byteBuffer->getDataLength()),
						byteBuffer->getBuffer()));

		*sqn = rcfSqnInfo_.last_update;

	} catch (MCPRuntimeError& mre)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
				"Error: MCPRuntimeError while publishing RCF update",
				"what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
				"stacktrace", mre.getStackTrace());
		rc = mre.getReturnCode();
	} catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishRegularCoveringFilterUpdate()",
                "Error: IllegalStateException while publishing RCF update",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
				"Error: SpiderCastLogicError while publishing RCF update",
				"what", le.what(), "stacktrace", le.getStackTrace());
		rc = ISMRC_ClusterInternalError;
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
	} catch (spdr::SpiderCastRuntimeError& re)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
				"Error: SpiderCastRuntimeError while publishing RCF update",
				"what", re.what(), "stacktrace", re.getStackTrace());
		rc = ISMRC_ClusterInternalError;
		ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
	}
	catch (std::bad_alloc& ba)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
	            "Error: exception while publishing RCF update",
	            "what", ba.what());
	    rc = ISMRC_AllocateError;
	}
	catch (std::exception& e)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
				"Error: exception while publishing RCF update",
				"what", e.what());
		rc = ISMRC_Error;
	} catch (...)
	{
	    Trace_Error(this, "publishRegularCoveringFilterUpdate()",
				"Error: untyped exception while publishing RCF update");
		rc = ISMRC_Error;
	}

	Trace_Exit(this, "publishRegularCoveringFilterUpdate()", rc);
	return rc;
}

uint32_t SubCoveringFilterPublisherImpl::getNumRegularCoveringFilterUpdates() const
{
	boost::mutex::scoped_lock lock(mutex);
	return rcfSqnInfo_.num_updates;
}

uint32_t SubCoveringFilterPublisherImpl::getSizeBytesRegularCoveringFilterUpdates() const
{
	boost::mutex::scoped_lock lock(mutex);
	return rcfSqnInfo_.updates_size_bytes;
}

uint32_t SubCoveringFilterPublisherImpl::getSizeBytesRegularCoveringFilterBase() const
{
	boost::mutex::scoped_lock lock(mutex);
	return rcfSqnInfo_.base_size_bytes;
}

void SubCoveringFilterPublisherImpl::publishForwardingAddress(const std::string& fwdAddr, int fwdPort, uint8_t fUseTLS)
{
	using namespace std;

	boost::mutex::scoped_lock lock(mutex);

	/* Format:
	 * String 		FwdAddr
	 * uint16_t     FwdPort
	 * boolean      fUseTLS
	 */
	byteBuffer->reset();
	byteBuffer->writeString(fwdAddr);
	byteBuffer->writeShort( static_cast<int16_t>( fwdPort ));
	byteBuffer->writeBoolean( fUseTLS>0 );

	membershipService.setAttribute(FilterTags::Fwd_Endpoint,
			spdr::Const_Buffer(
					static_cast<int32_t>(byteBuffer->getDataLength()),
					byteBuffer->getBuffer()));
}

void SubCoveringFilterPublisherImpl::publishLocalServerInfo(const std::string& serverName)
{
	using namespace std;

	boost::mutex::scoped_lock lock(mutex);

	/* Format:
	 * uint16_r     wire format supported-version
	 * uint16_r     wire format used-version
	 * String 		ServerName
	 */
	byteBuffer->reset();
	byteBuffer->writeShort(static_cast<int16_t>(SubCoveringFilterWireFormat::ATTR_VERSION));
	byteBuffer->writeShort(static_cast<int16_t>(SubCoveringFilterWireFormat::ATTR_VERSION));
	byteBuffer->writeString(serverName);

	membershipService.setAttribute(FilterTags::LocalServerInfo,
			spdr::Const_Buffer(
					static_cast<int32_t>(byteBuffer->getDataLength()),
					byteBuffer->getBuffer()));
}

int SubCoveringFilterPublisherImpl::publishRetainedStats(
        const RetainedStatsMap& retainedStats, uint64_t* sqn)
{
    using namespace std;
    Trace_Entry(this,"publishRetainedStats");

    int rc = ISMRC_OK;

    boost::mutex::scoped_lock lock(mutex);

    try
    {
        /* Format:
         * uint64                  sequence-number
         * uint32                  num-servers
         * {
         *    String               server UID (32 bit length + bytes)
         *    uint32               data-length
         *    char x data-length   Data
         * } x  num-servers
         *
         */
        byteBuffer->reset();
        byteBuffer->writeLong(static_cast<uint64_t>(++sqn_retained_stats_));
        byteBuffer->writeInt(static_cast<uint32_t>(retainedStats.size()));
        for (RetainedStatsMap::const_iterator it = retainedStats.begin();
                it != retainedStats.end(); ++it)
        {
            byteBuffer->writeString(it->first);
            byteBuffer->writeInt(it->second.dataLength);
            byteBuffer->writeByteArray((char*) it->second.pData, it->second.dataLength);
        }

        membershipService.setAttribute(FilterTags::RetainedStats,
                spdr::Const_Buffer(
                        static_cast<int32_t>(byteBuffer->getDataLength()),
                        byteBuffer->getBuffer()));

        *sqn = sqn_retained_stats_;

    } catch (MCPRuntimeError& mre)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: MCPRuntimeError while publishing Retained Stats",
                "what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
                "stacktrace", mre.getStackTrace());
        rc = mre.getReturnCode();
    } catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: IllegalStateException while publishing Retained Stats",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: SpiderCastLogicError while publishing Retained Stats",
                "what", le.what(),"stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    } catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: SpiderCastRuntimeError while publishing Retained Stats",
                "what", re.what(), "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: exception while publishing Retained Stats",
                "what", ba.what());
        rc = ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: exception while publishing Retained Stats", "what", e.what());
        rc = ISMRC_Error;
    } catch (...)
    {
        Trace_Error(this,"publishRetainedStats()",
                "Error: untyped exception while publishing Retained Stats");
        rc = ISMRC_Error;
    }

    Trace_Exit(this, "publishRetainedStats()", rc);
    return rc;
}

int SubCoveringFilterPublisherImpl::publishMonitoringStatus(
        ismCluster_HealthStatus_t  healthStatus,
        ismCluster_HaStatus_t haStatus,
        uint64_t* sqn)
{
    using namespace std;
    Trace_Entry(this,"publishMonitoringStatus");

    int rc = ISMRC_OK;

    boost::mutex::scoped_lock lock(mutex);

    try
    {
        /* Format:
         * uint64         sequence-number
         * char           healthStatus
         * char           haStatus
         *
         */
        byteBuffer->reset();
        byteBuffer->writeLong(static_cast<uint64_t>(++sqn_monitoring_status_));

        byteBuffer->writeChar( static_cast<char>( healthStatus ));
        byteBuffer->writeChar( static_cast<char>( haStatus ));

        membershipService.setAttribute(FilterTags::MonitoringStatus,
                spdr::Const_Buffer(
                        static_cast<int32_t>(byteBuffer->getDataLength()),
                        byteBuffer->getBuffer()));

        *sqn = sqn_monitoring_status_;

    } catch (MCPRuntimeError& mre)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: MCPRuntimeError while publishing Monitoring Status",
                "what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
                "stacktrace", mre.getStackTrace());
        rc = mre.getReturnCode();
    }
    catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: IllegalStateException while publishing Monitoring Status",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    }
    catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: SpiderCastLogicError while publishing Monitoring Status",
                "what", le.what(),"stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    } catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: SpiderCastRuntimeError while publishing Monitoring Status",
                "what", re.what(), "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: exception while publishing Monitoring Status",
                "what", ba.what());
        rc = ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: exception while publishing Monitoring Status", "what", e.what());
        rc = ISMRC_Error;
    } catch (...)
    {
        Trace_Error(this,"publishMonitoringStatus()",
                "Error: untyped exception while publishing Monitoring Status");
        rc = ISMRC_Error;
    }

    Trace_Exit(this, "publishMonitoringStatus()", rc);
    return rc;
}

int SubCoveringFilterPublisherImpl::publishRemovedServers(
            const RemoteServerVector& servers, uint64_t* sqn)
{
    using namespace std;
    Trace_Entry(this,"publishRemovedServers");

    int rc = ISMRC_OK;

    boost::mutex::scoped_lock lock(mutex);

    try
    {
        /* Format:
         * uint64         sequence-number
         * int32_t        #records
         * {
         *   String       serverUID
         *   int64_t      incNum
         * }              x #records
         *
         */
        byteBuffer->reset();
        byteBuffer->writeLong(static_cast<uint64_t>(++sqn_removed_servers_));

        byteBuffer->writeInt(static_cast<int32_t>( servers.size() ));
        for (unsigned int i=0; i<servers.size(); ++i)
        {
            byteBuffer->writeString(servers[i]->serverUID);
            byteBuffer->writeLong(static_cast<uint64_t>(servers[i]->incarnationNumber));
        }

        membershipService.setAttribute(FilterTags::RemovedServersList,
                spdr::Const_Buffer(
                        static_cast<int32_t>(byteBuffer->getDataLength()),
                        byteBuffer->getBuffer()));

        *sqn = sqn_removed_servers_;

    } catch (MCPRuntimeError& mre)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: MCPRuntimeError while publishing Removed Servers",
                "what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
                "stacktrace", mre.getStackTrace());
        rc = mre.getReturnCode();
    } catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: IllegalStateException while publishing Removed Servers",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: SpiderCastLogicError while publishing Removed Servers",
                "what", le.what(),"stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    } catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: SpiderCastRuntimeError while publishing Removed Servers",
                "what", re.what(), "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: exception while publishing Removed Servers",
                "what", ba.what());
        rc = ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: exception while publishing Removed Servers", "what", e.what());
        rc = ISMRC_Error;
    } catch (...)
    {
        Trace_Error(this,"publishRemovedServers()",
                "Error: untyped exception while publishing Removed Servers");
        rc = ISMRC_Error;
    }

    Trace_Exit(this, "publishRemovedServers()", rc);
    return rc;
}

int SubCoveringFilterPublisherImpl::publishRestoredNotInView(
            const RemoteServerVector& servers, uint64_t* sqn)
{
    using namespace std;
    Trace_Entry(this,"publishRestoredNotInView");

    int rc = ISMRC_OK;

    boost::mutex::scoped_lock lock(mutex);

    try
    {
        /* Format:
         * uint64         sequence-number
         * int32_t        #records
         * {
         *   String       serverUID
         *   String       serverName
         *   int64_t      incNum
         * }              x #records
         *
         */
        byteBuffer->reset();
        byteBuffer->writeLong(static_cast<uint64_t>(++sqn_restored_notin_view_));

        byteBuffer->writeInt(static_cast<int32_t>( servers.size() ));
        for (unsigned int i=0; i<servers.size(); ++i)
        {
            byteBuffer->writeString(servers[i]->serverUID);
            byteBuffer->writeString(servers[i]->serverName);
            byteBuffer->writeLong(static_cast<uint64_t>(servers[i]->incarnationNumber));
        }

        membershipService.setAttribute(FilterTags::RestoredNotInView,
                spdr::Const_Buffer(
                        static_cast<int32_t>(byteBuffer->getDataLength()),
                        byteBuffer->getBuffer()));

        *sqn = sqn_restored_notin_view_;

    } catch (MCPRuntimeError& mre)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: MCPRuntimeError while publishing Restored-not-in-view Servers",
                "what", mre.what(), "RC", boost::lexical_cast<std::string>(mre.getReturnCode()),
                "stacktrace", mre.getStackTrace());
        rc = mre.getReturnCode();
    } catch (spdr::IllegalStateException& ise)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: IllegalStateException while publishing Restored-not-in-view Servers",
                "what", ise.what(),"stacktrace", ise.getStackTrace());
        rc = ISMRC_Closed;
    } catch (spdr::SpiderCastLogicError& le)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: SpiderCastLogicError while publishing Restored-not-in-view Servers",
                "what", le.what(),"stacktrace", le.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", le.what());
    } catch (spdr::SpiderCastRuntimeError& re)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: SpiderCastRuntimeError while publishing Restored-not-in-view Servers",
                "what", re.what(), "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: exception while publishing Restored-not-in-view Servers",
                "what", ba.what());
        rc = ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: exception while publishing Restored-not-in-view Servers", "what", e.what());
        rc = ISMRC_Error;
    } catch (...)
    {
        Trace_Error(this,"publishRestoredNotInView()",
                "Error: untyped exception while publishing Restored-not-in-view Servers");
        rc = ISMRC_Error;
    }

    Trace_Exit(this, "publishRestoredNotInView()", rc);
    return rc;
}


} /* namespace mcp */
