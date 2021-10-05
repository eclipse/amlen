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
 * CommUDPMulticast.h
 *
 *  Created on: Oct 29, 2014
 */

#ifndef SPDR_COMMUDPMULTICAST_H_
#define SPDR_COMMUDPMULTICAST_H_


#include <boost/asio.hpp>

#include "NodeIDImpl.h"
#include "SCMessage.h"
#include "SpiderCastConfigImpl.h"
#include "NodeIDCache.h"
//#include "ConcurrentSharedPtr.h"
#include "IncomingMsgQ.h"
#include "CommUDPThread.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{


/**
 * A UDP multicast sender and receiver.
 *
 * The receiver binds to the predefined multicast address with the given port.
 * The receiver joins a single predefined multicast address.
 * The receiver moves every "legal" packet to the IncomingMessageQ.
 * The receiver filters out messages with the wrong bus-name, and messages from itself (due to the multicast loop being enabled).
 * The outbound multicast interface can be optionally defined (the IP_MULTICAST_IF socket option).
 *
 * The sender is an unconnected UDP socket that sends to the same predefined multicast address.
 *
 * The implementation is based on boost.asio.
 *
 * Supports IPv4 and IPv6.
 */
class CommUDPMulticast : public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:

	/**
	 *
	 * @param instID
	 * @param myNodeID
	 * @param busName
	 * @param multicastInOutboundInterface_v4 Sets the in/out bound interface. When empty, in is from all, out is to the default chosen by the kernel.
	 * @param multicastAddress_v4 IPv4 Multicast group to join
	 * @param multicastInOutboundInterface_v6 Sets the in/out bound interface (index). When zero, in is from all, out is to the default chosen by the kernel.
	 * @param multicastAddress_v6 IPv6 Multicast group to join
	 * @param multicastPort
	 * @param multicastHops
	 * @param packetSize
	 * @param cache
	 * @param incomingMsgQ
	 */
	CommUDPMulticast(
			const String& instID,
			NodeIDImpl_SPtr myNodeID,
			BusName_SPtr busName,
			int64_t incarnationNum,
			const std::string& multicastInOutboundInterface_v4,
			const std::string& multicastAddress_v4,
			int64_t multicastInOutboundInterface_v6,
			const std::string& multicastAddress_v6,
			uint16_t multicastPort,
			uint8_t multicastHops,
			uint16_t packetSize,
			uint32_t sendBufferSize,
			uint32_t rcvBufferSize,
			NodeIDCache& cache,
			IncomingMsgQ_SPtr incomingMsgQ);

	virtual ~CommUDPMulticast();

	/**
	 * bind to the port
	 *
	 * @throw SpiderCastRuntimeError if fails to bind or otherwise acquire the port
	 */
	void init() throw (SpiderCastRuntimeError);

	/**
	 * Start the CommUDPThread, join the multicast group
	 */
	void start();

	/**
	 * Stop the thread, leave the group, free all resources
	 */
	void stop();

	/**
	 * Is the associated thread running or stopped
	 * @return
	 */
	bool isStopped() const;

	bool isV4Enabled() const;

	bool isV6Enabled() const;

	/**
	 * Send the message to the group, message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msg
	 * @return
	 */
	bool sendToMCGroup(SCMessage_SPtr msg);

	/**
	 * Send the message bundle to the target, each message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msgBundle
	 * @param numMsgs
	 * @return
	 */
	bool sendToMCGroup(std::vector<SCMessage_SPtr> & msgBundle, int numMsgs);

private:
		NodeIDCache& nodeID_Cache_;
		IncomingMsgQ_SPtr incomingMsgQ_;

		/*
		 * Thread: Any
		 */
		bool stop_;

		mutable boost::recursive_mutex stopMutex_;

		boost::recursive_mutex txMutex_;

		/*
		 * Thread:
		 */
		boost::asio::io_service io_service_;

		/*
		 * Thread: any thread calling sendTo
		 */
		boost::asio::ip::udp::resolver resolver_;

		/*
		 * Thread: any thread calling sendTo
		 */
		boost::asio::ip::udp::socket tx_socket_v4_;
		boost::asio::ip::udp::socket rcv_socket_v4_;
		boost::asio::ip::udp::socket tx_socket_v6_;
		boost::asio::ip::udp::socket rcv_socket_v6_;


		CommUDPThread thread_;

		NodeIDImpl_SPtr myID_;
		std::string busName_;
		BusName_SPtr busName_SPtr_;
		int64_t incarnationNum_;

		//empty means unspecified
		const std::string multicastOutboundInterface_v4_str_; //
		const std::string multicastAddress_v4_str_;

		// 0 means unspecified
		const uint32_t multicastOutboundInterface_v6_index_;
		const std::string multicastAddress_v6_str_;

		uint16_t multicastPort_;
		uint8_t multicastHops_;
		uint16_t udpPacketSize_;

		uint32_t sendBufferSize_;
		uint32_t rcvBufferSize_;

		char* rcv_buffer_;
		char* rcv_buffer_v6_;

		boost::asio::ip::udp::endpoint remote_endpoint_v4_;
		boost::asio::ip::udp::endpoint multicast_endpoint_v4_;
		boost::asio::ip::udp::endpoint remote_endpoint_v6_;
		boost::asio::ip::udp::endpoint multicast_endpoint_v6_;

		bool v4_enabled_;
		bool v6_enabled_;

		void start_receive_v4();

		void start_receive_v6();

		/**
		 * Single threaded - CommUDPThread
		 *
		 * @param error
		 * @param bytes_transferred
		 */
		void handle_receive_v4(const boost::system::error_code& error,	std::size_t bytes_transferred);

		/**
		 * Single threaded - CommUDPThread
		 *
		 * @param error
		 * @param bytes_transferred
		 */
		void handle_receive_v6(const boost::system::error_code& error,	std::size_t bytes_transferred);

		void handle_discovery_msg(SCMessage_SPtr msg);

		void  deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode);

		void  deliver_fatal_event(const String& errMsg, spdr::event::ErrorCode errCode);

		void deliver_dup_local_node_event(const String& errMsg, spdr::event::ErrorCode errCode, int64_t incomingIncNum);

};

} /* namespace mcp */

#endif /* SPDR_COMMUDPMULTICAST_H_ */
