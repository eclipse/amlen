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
 * CommUDP.h
 *
 *  Created on: Jun 16, 2011
 */

#ifndef COMMUDP_H_
#define COMMUDP_H_

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
 * A UDP sender and receiver.
 *
 * The receiver binds to the "any" UDP address with the given port, and moves every
 * "legal" packet to the IncomingMessageQ.
 *
 * The sender is an unconnected UDP socket that searches the targets best interface (by scope),
 * and send the packet to it.
 *
 * The implementation is based on boost.asio.
 *
 * TODO currently supports only IPv4
 */
class CommUDP : public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:

	CommUDP(const String& instID,
			NodeIDImpl_SPtr myNodeID,
			BusName_SPtr busName,
			int64_t incarnationNum,
			const std::string& interface_v4,
			bool bindAll_v4,
			const std::string& interface_v6,
			bool bindAll_v6,
			uint16_t port,
			uint16_t packetSize,
			uint32_t sendBufferSize,
			uint32_t rcvBufferSize,
			NodeIDCache& cache,
			IncomingMsgQ_SPtr incomingMsgQ);

	virtual ~CommUDP();

	/**
	 * bind to the port
	 *
	 * @throw SpiderCastRuntimeError if fails to bind or otherwise acquire the port
	 */
	void init() throw (SpiderCastRuntimeError);

	/**
	 * Start the CommUDPThread
	 */
	void start();

	/**
	 * Stop the thread, free all resources
	 */
	void stop();

	/**
	 * Send the message to the target, message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msg
	 * @return
	 */
	bool sentTo(NodeIDImpl_SPtr target, SCMessage_SPtr msg);

	/**
	 * Send the message bundle to the target, each message must be no longer then a single packet.
	 *
	 * @param target
	 * @param msgBundle
	 * @param numMsgs
	 * @return
	 */
	bool sentTo(NodeIDImpl_SPtr target, std::vector<SCMessage_SPtr> & msgBundle, int numMsgs);

private:
	//const SpiderCastConfigImpl& config_;
	NodeIDImpl_SPtr myID_;
	std::string busName_;
	BusName_SPtr busName_SPtr_;
	int64_t incarnationNum_;

	//empty means unspecified
	const std::string interface_v4_str_; //
	bool bindAll_v4_;
	//empty means unspecified
	const std::string interface_v6_str_; //
	bool bindAll_v6_;

	uint16_t port_;
	uint16_t packetSize_;
	uint32_t sendBufferSize_;
	uint32_t rcvBufferSize_;

	NodeIDCache& nodeID_Cache_;
	IncomingMsgQ_SPtr incomingMsgQ_;



	/*
	 * Thread: Any
	 */
	bool stop_;

	boost::recursive_mutex stopMutex_;

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
	bool v4_tx_enabled_;
	bool v4_rcv_enabled_;

	boost::asio::ip::udp::socket tx_socket_v6_;
	boost::asio::ip::udp::socket rcv_socket_v6_;
	bool v6_tx_enabled_;
	bool v6_rcv_enabled_;

	CommUDPThread thread_;

	char* rcv_buffer_;
	char* rcv_buffer_v6_;

	boost::asio::ip::udp::endpoint remote_endpoint_;
	boost::asio::ip::udp::endpoint remote_endpoint_v6_;

	/**
	 * Thread safe
	 *
	 * @param targetEP
	 * @param msg
	 * @return
	 */
	bool sendTo(boost::asio::ip::udp::endpoint targetEP, SCMessage_SPtr msg);

	/**
	 * Thread safe
	 *
	 * @param address
	 * @param port
	 * @return
	 * @throw boost::system::system_error Thrown on failure.
	 */
	boost::asio::ip::udp::endpoint resolveAddress(const String& address, uint16_t port);

	void start_receive_v4();

	void start_receive_v6();

	/**
	 * Single threaded - CommUDPThread
	 *
	 * @param error
	 * @param bytes_transferred
	 */
	void handle_receive_v4(const boost::system::error_code& error,	std::size_t bytes_transferred);

	void handle_receive_v6(const boost::system::error_code& error,	std::size_t bytes_transferred);

	void handle_discovery_msg(SCMessage_SPtr msg);

	void deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode);

	void deliver_fatal_event(const String& errMsg, spdr::event::ErrorCode errCode);

	void deliver_dup_local_node_event(const String& errMsg, spdr::event::ErrorCode errCode, int64_t incomingIncNum);

	void handle_self_message(SCMessage_SPtr scMsg, int64_t inc_num);

};

}

#endif /* COMMUDP_H_ */
