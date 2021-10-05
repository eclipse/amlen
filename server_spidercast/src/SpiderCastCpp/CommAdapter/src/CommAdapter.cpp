// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * CommAdapter.cpp
 *
 *  Created on: Feb 8, 2010
 *      Author:
 *
 * Version     : $Revision: 1.11 $
 *-----------------------------------------------------------------
 * $Log: CommAdapter.cpp,v $
 * Revision 1.11  2016/04/07 17:28:00 
 * improve CommUtils
 *
 * Revision 1.10  2015/11/18 08:37:02 
 * Update copyright headers
 *
 * Revision 1.9  2015/09/06 12:09:16 
 * CommUDP v6 support
 *
 * Revision 1.8  2015/08/06 11:30:18 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.7  2015/08/06 06:59:17 
 * remove ConcurrentSharedPtr
 *
 * Revision 1.6  2015/07/30 12:14:35 
 * split brain
 *
 * Revision 1.5  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.4  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.3  2015/05/20 11:11:01 
 * support for IPv6 in multicast discovery
 *
 * Revision 1.2  2015/05/07 09:51:28 
 * Support for internal/external endpoints
 *
 * Revision 1.1  2015/03/22 12:29:17 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.24  2015/01/26 12:38:14 
 * MembershipManager on boost::shared_ptr
 *
 * Revision 1.23  2014/11/04 09:51:24 
 * Added multicast discovery
 *
 * Revision 1.22  2014/10/30 15:46:25 
 * Implemented the CommUDPMulticast, towards multicast discovery
 *
 * Revision 1.21  2012/10/25 10:11:07 
 * Added copyright and proprietary notice
 *
 * Revision 1.20  2012/02/14 12:43:02 
 * remove comment. no functional changes
 *
 * Revision 1.19  2011/06/20 11:38:43 
 * Add commUdp component
 *
 * Revision 1.18  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.17  2010/11/25 16:15:06 
 * After merge of comm changes back to head
 *
 * Revision 1.16.4.1  2011/01/04 14:50:10 
 * Merge HEAD into Hierarchy branch
 *
 * Revision 1.16.2.2  2010/11/15 12:46:32 
 * Hopefully,last bug fixed
 *
 * Revision 1.16.2.1  2010/10/21 16:10:42 
 * Massive comm changes
 * 
 * Revision 1.16  2010/07/08 09:53:30 
 * Switch from cout to trace
 *
 * Revision 1.15  2010/06/30 06:33:39 
 * Add thisMemeberName to printouts - no functional change
 *
 * Revision 1.14  2010/06/27 15:11:30 
 * Remove unused code
 *
 * Revision 1.13  2010/06/23 14:17:19 
 * No functional chnage
 *
 * Revision 1.12  2010/06/08 15:46:25 
 * changed instanceID to String&
 *
 * Revision 1.11  2010/05/20 09:13:58 
 * Add incomingMsgQ, and the use of nodeId on the connect rather than a string
 *
 * Revision 1.10  2010/05/13 09:06:11 
 * added NodeIDCache to constructor/factory
 *
 * Revision 1.9  2010/05/10 08:29:54 
 * Handle termination better
 *
 * Revision 1.8  2010/05/04 11:35:23 
 * Added IncomingMsgQ
 *
 * Revision 1.7  2010/04/15 10:08:51 
 * *** empty log message ***
 *
 * Revision 1.6  2010/04/14 09:16:21 
 * Add some print statemnets - no funcional changes
 *
 * Revision 1.5  2010/04/12 12:00:58 
 * Membership periodic/termination tasks, refactor enums
 *
 * Revision 1.4  2010/04/12 09:20:19 
 * Use concurrentSharedPtr with getInstance() + misc. non-funcational changes
 *
 * Revision 1.3  2010/04/07 13:55:16 
 * Add CVS log
 *
 * Revision 1.1  2010/03/07 10:44:45 
 * Intial Version of the Comm Adapter
 */

#include "CommAdapter.h"
#include "CommRumAdapter.h"
#include "CommUtils.h"

using namespace std;

namespace spdr
{


ScTraceComponent* CommAdapter::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter,
		"CommAdapter",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

CommAdapter::CommAdapter(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& cache, int64_t incarnationNum) :
	ctx_(tc_,instID,config.getNodeName()),
	_started(false),
	_thisMemberName(NULL),
	_instID(instID),
	_isUdpDiscovery(config.isUDPDiscovery()),
	_isMulticastDiscovery(config.isMulticastDiscovery()),
	_terminated(false),
	_nodeIdCache(cache)
{
	Trace_Entry(&ctx_,"CommAdapter()");

	_myNodeId = config.getMyNodeID();
	_incomingMsgQueue = IncomingMsgQ_SPtr (new IncomingMsgQ(
			_instID, _myNodeId, cache));

	//NOTE: UDP and Multicast discovery are not mutually exclusive
	if (_isUdpDiscovery)
	{
		boost::asio::ip::address bind_address;
		if (!config.getBindNetworkAddress().empty())
		{
			try
			{
				bind_address = boost::asio::ip::address::from_string(config.getBindNetworkAddress());
			}
			catch (boost::system::system_error& se)
			{
				std::ostringstream what;
				what << "Error: failed to parse UDP bind address='"
						<< config.getBindNetworkAddress()
						<< "'; what=" << se.what()
						<< "; code=" << se.code().message();
				Trace_Error(&ctx_, "CommAdapter()", what.str());
				throw SpiderCastRuntimeError(what.str());
			}
		}

		_commUDP = boost::shared_ptr<CommUDP>(new CommUDP(
				_instID,
				config.getMyNodeID(),
				config.getBusName_SPtr(),
				incarnationNum,
				(bind_address.is_v4() ? config.getBindNetworkAddress(): ""),
				config.getBindAllInterfaces(),
				(bind_address.is_v6() ? config.getBindNetworkAddress(): ""),
				config.getBindAllInterfaces(),
				config.getBindTcpRceiverPort(),
				config.getUDPPacketSizeBytes(),
				config.getUdpSendBufferSizeBytes(),
				config.getUdpReceiveBufferSizeBytes(),
				_nodeIdCache,
				_incomingMsgQueue));
		_commUDP->init();
		Trace_Event(&ctx_,"CommAdapter()","UDP discovery initialized successfully");
	}

	if (_isMulticastDiscovery)
	{
		CommUtils::NICInfo nic_info;
		int errCode = 0;
		std::string errMsg;
		CommUtils::get_nic_info(config.getMulticastOutboundInterface().c_str(), &nic_info, &errCode, &errMsg);
		Trace_Event(&ctx_,"CommAdapter()",std::string("Multicast discovery NIC: ") + nic_info.toString() );
		_commUDPMulticast = boost::shared_ptr<CommUDPMulticast>(new CommUDPMulticast(
				_instID,
				config.getMyNodeID(),
				config.getBusName_SPtr(),
				incarnationNum,
				nic_info.address_v4,
				config.getMulticastGroupAddressIPv4(),
				nic_info.index,
				config.getMulticastGroupAddressIPv6(),
				config.getMulticastPort(),
				config.getMulticastHops(),
				config.getUDPPacketSizeBytes(),
				config.getUdpSendBufferSizeBytes(),
				config.getUdpReceiveBufferSizeBytes(),
				_nodeIdCache,
				_incomingMsgQueue));
		_commUDPMulticast->init();

		Trace_Event(&ctx_,"CommAdapter()","Multicast discovery initialized successfully",
				"v4-enabled", (_commUDPMulticast->isV4Enabled() ? "True" : "False"),
				"v6-enabled", (_commUDPMulticast->isV6Enabled() ? "True" : "False"));
	}

	Trace_Exit(&ctx_,"CommAdapter()");
}

CommAdapter::~CommAdapter()
{
	Trace_Entry(&ctx_,"~CommAdapter()");
	//	terminate(true);
}

void CommAdapter::start()
{
	Trace_Entry(&ctx_,"start()");
	_started = true;
	if (_isUdpDiscovery)
	{
		_commUDP->start();
	}

	if (_isMulticastDiscovery)
	{
		_commUDPMulticast->start();
	}
	Trace_Exit(&ctx_,"start()");
}

CommAdapter_SPtr CommAdapter::getInstance(const String& instID,
		const SpiderCastConfigImpl& config, NodeIDCache& cache, int64_t incarnationNumber)
{
	return CommAdapter_SPtr (new CommRumAdapter(instID, config,
			cache, incarnationNumber));
}

IncomingMsgQ_SPtr CommAdapter::getIncomingMessageQ()
{
	return _incomingMsgQueue;
}

/**
 * Send the message to the target, message must be no longer then a single packet.
 *
 * @param target
 * @param msg
 * @return
 */
bool CommAdapter::sendTo(NodeIDImpl_SPtr target, SCMessage_SPtr msg)
{
	if (_isUdpDiscovery)
	{
		return _commUDP->sentTo(target, msg);
	}
	else
	{
		String errMsg("Error: UDP discovery disabled");
		Trace_Error(&ctx_, "sendTo()", errMsg);
		throw NullPointerException(errMsg);
	}
}

/**
 * Send the message bundle to the target, each message must be no longer then a single packet.
 *
 * @param target
 * @param msgBundle
 * @param numMsgs
 * @return
 */
bool CommAdapter::sendTo(NodeIDImpl_SPtr target,
		std::vector<SCMessage_SPtr> & msgBundle, int numMsgs)
{
	if (_isUdpDiscovery)
	{
		return _commUDP->sentTo(target, msgBundle, numMsgs);
	}
	else
	{
		String errMsg("Error: UDP discovery disabled");
		Trace_Error(&ctx_, "sendTo(bundle)", errMsg);
		throw NullPointerException(errMsg);
	}

}

bool CommAdapter::sendToMCgroup(SCMessage_SPtr msg)
{
	if (_isMulticastDiscovery)
	{
		return _commUDPMulticast->sendToMCGroup(msg);
	}
	else
	{
		String errMsg("Error: Multicast discovery disabled");
		Trace_Error(&ctx_, "sendToMCgroup()", errMsg);
		throw NullPointerException(errMsg);
	}
}

bool CommAdapter::sendToMCgroup(	std::vector<SCMessage_SPtr> & msgBundle, int numMsgs)
{
	if (_isMulticastDiscovery)
	{
		return _commUDPMulticast->sendToMCGroup(msgBundle, numMsgs);
	}
	else
	{
		String errMsg("Error: Multicast discovery disabled");
		Trace_Error(&ctx_, "sendToMCgroup(bundle)", errMsg);
		throw NullPointerException(errMsg);
	}
}

void CommAdapter::terminate(bool grace)
{
	Trace_Entry(&ctx_, "terminate()", grace ? "grace=true" : "grace=false");
	if (_isUdpDiscovery)
	{
		_commUDP->stop();
	}

	if (_isMulticastDiscovery)
	{
		_commUDPMulticast->stop();
	}

	Trace_Exit(&ctx_, "terminate()");
}
}
