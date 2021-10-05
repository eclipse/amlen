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
 * CommUDP.cpp
 *
 *  Created on: Jun 16, 2011
 */

#include <boost/lexical_cast.hpp>

#include "CommAdapterDefinitions.h"
#include "CommUDP.h"
#include "CommUtils.h"

namespace spdr
{

ScTraceComponent* CommUDP::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter,
		"CommUDP",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

CommUDP::CommUDP(const String& instID,
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
			IncomingMsgQ_SPtr incomingMsgQ) :
		ScTraceContext(tc_, instID, ""),
		myID_(myNodeID),
		busName_(busName->toOrgString()),
		busName_SPtr_(busName),
		incarnationNum_(incarnationNum),
		interface_v4_str_(interface_v4),
		bindAll_v4_(bindAll_v4),
		interface_v6_str_(interface_v6),
		bindAll_v6_(bindAll_v6),
		port_(port),
		packetSize_(packetSize),
		sendBufferSize_(sendBufferSize),
		rcvBufferSize_(rcvBufferSize),
		nodeID_Cache_(cache),
		incomingMsgQ_(incomingMsgQ),
		stop_(false),
		stopMutex_(),
		txMutex_(),
		io_service_(),
		resolver_(io_service_),
		tx_socket_v4_(io_service_),
		rcv_socket_v4_(io_service_),
		v4_tx_enabled_(false),
		v4_rcv_enabled_(false),
		tx_socket_v6_(io_service_),
		rcv_socket_v6_(io_service_),
		v6_tx_enabled_(false),
		v6_rcv_enabled_(false),
		thread_(instID, "CommUDP", io_service_),
		rcv_buffer_(new char[packetSize]),
		rcv_buffer_v6_(new char[packetSize]),
		remote_endpoint_(),
		remote_endpoint_v6_()
{

}

CommUDP::~CommUDP()
{
	Trace_Entry(this,"~CommUDP()");

	try
	{
		tx_socket_v4_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDP()", "error closing (IPv4 tx) socket",
				"what", String(e.what()));
	}

	try
	{
		rcv_socket_v4_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDP()", "error closing (IPv4 rcv) socket",
				"what", String(e.what()));
	}

	try
	{
		tx_socket_v6_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDP()", "error closing (IPv6 tx) socket",
				"what", String(e.what()));
	}

	try
	{
		rcv_socket_v6_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDP()", "error closing (IPv6 rcv) socket",
				"what", String(e.what()));
	}

	delete[] rcv_buffer_;
	delete[] rcv_buffer_v6_;
}

void CommUDP::init() throw (SpiderCastRuntimeError)
{
	using namespace boost::asio;
	using boost::asio::ip::udp;

	Trace_Entry(this,"init()");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);

		//=== IPv4 ===
		ip::address_v4 bind_address_v4;
		try
		{
			if (!interface_v4_str_.empty())
			{
				bind_address_v4 = ip::address_v4::from_string(interface_v4_str_);
				Trace_Event(this,"init()","parsed UDP IPv4 bind address",
						"address", bind_address_v4.to_string());
				if (bind_address_v4.is_multicast())
				{
					std::string what("Error: UDP IPv4 bind address is multicast");
					Trace_Error(this, "init()", what);
					throw SpiderCastRuntimeError(what);
				}
			}
			else
			{
				Trace_Event(this,"init()","UDP IPv4 bind address unspecified",
						"address", bind_address_v4.to_string(),
						"bindAll-v4", (bindAll_v4_?"T":"F"));
			}
		}
		catch (boost::system::system_error& se)
		{
			std::ostringstream what;
			what << "Error: failed to parse UDP IPv4 bind address='"
					<< interface_v4_str_
					<< "'; what=" << se.what()
					<< "; code=" << se.code().message();
			Trace_Error(this,"init()", what.str());
			throw SpiderCastRuntimeError(what.str());
		}


		try
		{
			//Transmitter
			tx_socket_v4_.open(udp::v4());
			if (spdr::set_CLOEXEC(tx_socket_v4_.native_handle()) != 0)
			{
				int errsv = errno;
				Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Tx V4 socket, ignoring",
						"errno", spdr::stringValueOf(errsv));
			}

			boost::asio::socket_base::send_buffer_size optW(sendBufferSize_);
			tx_socket_v4_.set_option(optW);
			boost::asio::socket_base::send_buffer_size optR;
			tx_socket_v4_.get_option(optR);

			v4_tx_enabled_ = true;
			Trace_Event<int>(this, "init()","IPv4 Tx socket opened OK",
					"send buffer size", optR.value());

			//Receiver
			if ((bind_address_v4.is_unspecified() && bindAll_v4_) || !bind_address_v4.is_unspecified())
			{
				rcv_socket_v4_.open(udp::v4());
				if (spdr::set_CLOEXEC(rcv_socket_v4_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Rcv V4 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}


				boost::asio::socket_base::receive_buffer_size optW(rcvBufferSize_);
				rcv_socket_v4_.set_option(optW);
				boost::asio::socket_base::receive_buffer_size optR;
				rcv_socket_v4_.get_option(optR);

				//boost::asio::socket_base::reuse_address option(true);
				//rcv_socket_v4_.set_option(option);
				if (bind_address_v4.is_unspecified())
				{
					rcv_socket_v4_.bind(udp::endpoint(udp::v4(),port_));
				}
				else
				{
					rcv_socket_v4_.bind(udp::endpoint(bind_address_v4,port_));
				}

				v4_rcv_enabled_ = true;
			}
		}
		catch (boost::system::system_error& se)
		{
			Trace_Warning(this,"init()", "Warning: IPv4 UDP failed to initialize",
					"what",se.what(),
					"message", se.code().message());
		}

		//=== IPv6 ===
		ip::address_v6 bind_address_v6;
		try
		{
			if (!interface_v6_str_.empty())
			{
				bind_address_v6 = ip::address_v6::from_string(interface_v6_str_);
				Trace_Event(this,"init()","parsed UDP IPv6 bind address",
						"address", bind_address_v4.to_string());
				if (bind_address_v6.is_multicast())
				{
					std::string what("Error: UDP IPv6 bind address is multicast");
					Trace_Error(this, "init()", what);
					throw SpiderCastRuntimeError(what);
				}
			}
			else
			{
				Trace_Event(this,"init()","UDP IPv6 bind address unspecified",
						"address", bind_address_v6.to_string(),
						"bindAll-v6", (bindAll_v6_?"T":"F"));
			}
		}
		catch (boost::system::system_error& se)
		{
			std::ostringstream what;
			what << "Error: failed to parse UDP IPv6 bind address='"
					<< interface_v6_str_
					<< "'; what=" << se.what()
					<< "; code=" << se.code().message();
			Trace_Error(this,"init()", what.str());
			throw SpiderCastRuntimeError(what.str());
		}

		try
		{
			//Transmitter
			tx_socket_v6_.open(udp::v6());
			if (spdr::set_CLOEXEC(tx_socket_v6_.native_handle()) != 0)
			{
				int errsv = errno;
				Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Tx V6 socket, ignoring",
						"errno", spdr::stringValueOf(errsv));
			}

			boost::asio::socket_base::send_buffer_size optW(sendBufferSize_);
			tx_socket_v6_.set_option(optW);
			boost::asio::socket_base::send_buffer_size optR;
			tx_socket_v6_.get_option(optR);

			v6_tx_enabled_ = true;
			Trace_Event<int>(this, "init()","IPv6 Tx socket opened OK",
					"send buffer size", optR.value());

			if ((bind_address_v6.is_unspecified() && bindAll_v6_) || !bind_address_v6.is_unspecified())
			{
				rcv_socket_v6_.open(udp::v6());
				if (spdr::set_CLOEXEC(rcv_socket_v6_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Rcv V6 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}

				boost::asio::socket_base::receive_buffer_size optW(rcvBufferSize_);
				rcv_socket_v6_.set_option(optW);
				boost::asio::socket_base::receive_buffer_size optR;
				rcv_socket_v6_.get_option(optR);

				//boost::asio::socket_base::reuse_address option(true);
				//rcv_socket_v6_.set_option(option);
				if (bind_address_v6.is_unspecified())
				{
					rcv_socket_v6_.bind(udp::endpoint(udp::v6(),port_));
				}
				else
				{
					rcv_socket_v6_.bind(udp::endpoint(bind_address_v6,port_));
				}
				v6_rcv_enabled_ = true;

				Trace_Event<int>(this, "init()","IPv6 Rcv socket open & bind OK",
						"rcv buffer size", optR.value());
			}
		}
		catch (boost::system::system_error& se)
		{
			v6_rcv_enabled_ = false;
			Trace_Warning(this,"init()", "Warning: IPv6 UDP failed to initialize",
					"what",se.what(),
					"message", se.code().message());
		}

		if (!v4_rcv_enabled_ && !v6_rcv_enabled_)
		{
			std::string what("Error: Both IPv4 and IPv6 UDP receivers are disabled or failed to initialize");
			Trace_Error(this, "init()", what);
			throw SpiderCastRuntimeError(what);
		}
	}

	Trace_Exit(this,"init()");
}

void CommUDP::start()
{
	using boost::asio::ip::udp;

	Trace_Entry(this,"start()");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);

		if (!stop_)
		{
			if (v4_rcv_enabled_)
			{
				start_receive_v4();
			}
			if (v6_rcv_enabled_)
			{
				start_receive_v6();
			}
			thread_.start();
		}
		else
		{
			Trace_Event(this,"start()", "stopped, skipping");
		}
	}

	Trace_Exit(this,"start()");
}

void CommUDP::stop()
{
	Trace_Entry(this,"stop()");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);

		stop_ = true;

		thread_.finish();
		if (!io_service_.stopped())
		{
			io_service_.stop();
		}

		if (v4_rcv_enabled_)
		{
			try
			{
				rcv_socket_v4_.cancel();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error (IPv4 rcv) socket cancel", "what", e.what());
			}

			try
			{
				rcv_socket_v4_.close();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error (IPv4 rcv) socket close", "what", e.what());
			}
		}

		if (v4_tx_enabled_)
		{
			try
			{
				tx_socket_v4_.close();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error closing (IPv4 tx) socket",
						"what", String(e.what()));
			}
		}

		if (v6_rcv_enabled_)
		{
			try
			{
				rcv_socket_v6_.cancel();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error (IPv6 rcv) socket cancel", "what", e.what());
			}

			try
			{
				rcv_socket_v6_.close();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error (IPv6 rcv) socket close", "what", e.what());
			}
		}

		if (v6_tx_enabled_)
		{
			try
			{
				tx_socket_v6_.close();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error closing (IPv6 tx) socket", "what", e.what());
			}
		}
	}

	if (boost::this_thread::get_id() != thread_.getID())
	{
		thread_.join();
	}

	Trace_Exit(this,"stop()");
}

bool CommUDP::sentTo(NodeIDImpl_SPtr target, SCMessage_SPtr msg)
{
	using boost::asio::ip::udp;

	Trace_Entry(this,"sentTo(single)");

	String targetAddress = spdr::comm::endpointScopeMatch(
			myID_->getNetworkEndpoints().getAddresses(),
			target->getNetworkEndpoints().getAddresses());
	if (targetAddress == "")
	{
		Trace_Exit(this,"sentTo(single)","false, failed to match scope");
		return false;
	}

	try
	{
		udp::endpoint target_endpoint = resolveAddress(
				targetAddress,
				target->getNetworkEndpoints().getPort());

		if (target_endpoint != udp::endpoint())
		{
			bool res = sendTo(target_endpoint, msg);
			Trace_Exit<bool>(this,"sentTo()",res);
			return res;
		}
		else
		{
			Trace_Exit(this,"sentTo(single)","false, failed to resolve address");
			return false;
		}
	}
	catch (std::exception& e)
	{
		Trace_Event(this,"sentTo(single)","failed to send",
				"what", e.what(), "typeid", typeid(e).name());
		return false;
	}
}

bool CommUDP::sentTo(NodeIDImpl_SPtr target,
		std::vector<SCMessage_SPtr> & msgBundle, int numMsgs)
{
	using boost::asio::ip::udp;

	Trace_Entry(this,"sentTo(bundle)", "num", boost::lexical_cast<std::string>(numMsgs));

	String targetAddress = spdr::comm::endpointScopeMatch(
			myID_->getNetworkEndpoints().getAddresses(),
			target->getNetworkEndpoints().getAddresses());

	if (targetAddress == "")
	{
		Trace_Event(this,"sentTo(bundle)","failed to match scope",
				"target", toString<NodeIDImpl>(target));
		Trace_Exit(this,"sentTo(bundle)","false, match scope");
		return false;
	}

	Trace_Debug(this,"sentTo()","match scope", "target", targetAddress);

	try
	{
		udp::endpoint target_endpoint = resolveAddress(
				targetAddress,
				target->getNetworkEndpoints().getPort());

		if (target_endpoint != udp::endpoint())
		{
			bool res = false;
			for (int i=0; i<numMsgs; ++i)
			{
				res = sendTo(target_endpoint, msgBundle[i]);
				if (!res)
				{
					Trace_Event(this,"sentTo()","failed to send bundle",
							"i",boost::lexical_cast<std::string>(i),
							"#msgs", boost::lexical_cast<std::string>(numMsgs));
					break;
				}
			}
			Trace_Exit<bool>(this,"sentTo()",res);
			return res;
		}
		else
		{
			Trace_Event(this,"sentTo(bundle)","failed to resolve address",
					"target", targetAddress);
			return false;
		}

	}
	catch (std::exception& e)
	{
		Trace_Event(this,"sentTo(bundle)","failed send bundle",
				"what", e.what(), "typeid", typeid(e).name());
		return false;
	}
}

boost::asio::ip::udp::endpoint CommUDP::resolveAddress(const String& targetAddress, uint16_t port)
{
	using namespace std;
	using boost::asio::ip::udp;

	boost::recursive_mutex::scoped_lock lock(txMutex_);

	udp::resolver::query query(targetAddress, boost::lexical_cast<std::string>(port),
			boost::asio::ip::resolver_query_base::numeric_service);
	udp::resolver::iterator res_it = resolver_.resolve(query);

	udp::endpoint first_target_endpoint;
	for (udp::resolver::iterator end; res_it != end; ++res_it)
	{
		udp::endpoint target_endpoint = *res_it;
		if (first_target_endpoint == udp::endpoint())
		{
			first_target_endpoint = target_endpoint;
		}

		if (v4_rcv_enabled_ && target_endpoint.protocol() == udp::v4())
		{
			return target_endpoint;
		}

		if (v6_rcv_enabled_ && target_endpoint.protocol() == udp::v6())
		{
			return target_endpoint;
		}
	}

	return first_target_endpoint;
}

bool CommUDP::sendTo(boost::asio::ip::udp::endpoint targetEP, SCMessage_SPtr msg)
{
	using boost::asio::ip::udp;

	Trace_Entry(this, "sentTo(EP)",	"EP", boost::lexical_cast<std::string>(targetEP));

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);
		if (stop_)
		{
			Trace_Event(this, "sentTo(EP)", "closed, skipped");
			return false;
		}
	}

	{
		boost::recursive_mutex::scoped_lock lock(txMutex_);

		try
		{
			if (targetEP.protocol() == udp::v4() && v4_tx_enabled_)
			{
				Trace_Dump(this, "sentTo(EP)", "before IPv4 socket.send_to");

				size_t sent = tx_socket_v4_.send_to(
						boost::asio::buffer(msg->getBuffer()->getBuffer(), msg->getBuffer()->getDataLength()),
						targetEP);

				if (sent == msg->getBuffer()->getDataLength())
				{
					Trace_Dump(this, "sentTo(EP)", "after IPv4 socket.send_to",
							"bytes-sent", boost::lexical_cast<std::string>(sent));
					return true;
				}
				else
				{
					Trace_Event(this, "sentTo(EP)", "failed to send packet, short write, IPv4");
					return false;
				}
			}

			if (targetEP.protocol() == udp::v6() && v6_tx_enabled_)
			{
				Trace_Dump(this, "sentTo(EP)", "before IPv6 socket.send_to");

				size_t sent = tx_socket_v6_.send_to(
						boost::asio::buffer(msg->getBuffer()->getBuffer(), msg->getBuffer()->getDataLength()),
						targetEP);

				if (sent == msg->getBuffer()->getDataLength())
				{
					Trace_Dump(this, "sentTo(EP)", "after IPv6 socket.send_to",
							"bytes-sent", boost::lexical_cast<std::string>(sent));
					return true;
				}
				else
				{
					Trace_Event(this, "sentTo(EP)", "failed to send packet, short write, IPv6");
					return false;
				}
			}

			Trace_Event(this, "sentTo(EP)", "failed to send packet, no transmitter available for target protocol",
					"target protocol", (targetEP.protocol() == udp::v4() ? "IPv4" : "IPv6"));
			return false;
		}
		catch (boost::system::system_error& se)
		{
			Trace_Event(this, "sentTo(EP)", "failed to send packet", "what", se.what());
			return false;
		}
	}
}

void CommUDP::start_receive_v4()
{
	Trace_Entry(this, "start_receive_v4()");

	rcv_socket_v4_.async_receive_from(
			boost::asio::buffer(rcv_buffer_ ,packetSize_),
			remote_endpoint_,
			boost::bind(&CommUDP::handle_receive_v4, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	Trace_Exit(this, "start_receive_v4()");
}


void CommUDP::start_receive_v6()
{
	Trace_Entry(this, "start_receive_v6()");

	rcv_socket_v6_.async_receive_from(
			boost::asio::buffer(rcv_buffer_v6_ ,packetSize_),
			remote_endpoint_v6_,
			boost::bind(&CommUDP::handle_receive_v6, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	Trace_Exit(this, "start_receive_v6()");
}

void CommUDP::handle_receive_v4(
		const boost::system::error_code& error,
		std::size_t bytes_transferred)
{
	Trace_Entry(this, "handle_receive_v4()");

	{
		boost::recursive_mutex::scoped_lock lock (stopMutex_);
		if (stop_)
		{
			Trace_Event(this, "handle_receive_v4()", "closed, ignoring packet");
			return;
		}
	}

	if (!error)
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "handle_receive_v4");
			buffer->addProperty<size_t>("bytes-transferred", bytes_transferred);
			buffer->invoke();
		}

		try
		{
			ByteBuffer_SPtr buffer = ByteBuffer::createReadOnlyByteBuffer(
					rcv_buffer_, bytes_transferred, false); //allocate and copy into buffer
			SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
			scMsg->setBuffer(buffer);
			handle_discovery_msg(scMsg);

		}
		catch (std::exception& e)
		{
			std::ostringstream what;
			what << "Warning: IPv4 UDP discovery: Exception while unmarshling incoming message: what="
					<< e.what() << "; typeid=" << typeid(e).name();
			Trace_Warning(this, "handle_receive_v4()", what.str());
			deliver_warning_event(what.str(),event::Message_Refused_Parse_Error);
		}

		start_receive_v4();
	}
	else
	{
		if (error.value() == boost::system::errc::operation_canceled)
		{
			Trace_Event(this, "handle_receive_v4()", "expected on stop",
					"message", error.message());
		}
		else if (error.value() == boost::system::errc::message_size)
		{
			Trace_Event(this, "handle_receive_v4()", "error message_size, ignored. sleep 1ms and continue receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v4();
		}
		else
		{
			Trace_Event(this, "handle_receive_v4()", "unexpected error, ignored. sleep 1ms and continue receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v4();
		}
	}

	Trace_Exit(this, "handle_receive_v4");
}


void CommUDP::handle_receive_v6(
		const boost::system::error_code& error,
		std::size_t bytes_transferred)
{
	Trace_Entry(this, "handle_receive_v6()");

	{
		boost::recursive_mutex::scoped_lock lock (stopMutex_);
		if (stop_)
		{
			Trace_Event(this, "handle_receive_v6()", "closed, ignoring packet");
			return;
		}
	}

	if (!error)
	{
		if (ScTraceBuffer::isDebugEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::debug(this, "handle_receive_v6");
			buffer->addProperty<size_t>("bytes-transferred", bytes_transferred);
			buffer->invoke();
		}

		try
		{
			ByteBuffer_SPtr buffer = ByteBuffer::createReadOnlyByteBuffer(
					rcv_buffer_v6_, bytes_transferred, false); //allocate and copy into buffer
			SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
			scMsg->setBuffer(buffer);
			handle_discovery_msg(scMsg);
		}
		catch (std::exception& e)
		{
			std::ostringstream what;
			what << "Warning: IPv6 UDP discovery: Exception while unmarshling incoming message: what="
					<< e.what() << "; typeid=" << typeid(e).name();
			Trace_Warning(this, "handle_receive_v6()", what.str());
			deliver_warning_event(what.str(),event::Message_Refused_Parse_Error);
		}

		start_receive_v6();
	}
	else
	{
		if (error.value() == boost::system::errc::operation_canceled)
		{
			Trace_Event(this, "handle_receive_v6()", "expected on stop",
					"message", error.message());
		}
		else if (error.value() == boost::system::errc::message_size)
		{
			Trace_Event(this, "handle_receive_v6()", "error message_size, ignored. sleep 1ms and continue receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v6();
		}
		else
		{
			Trace_Event(this, "handle_receive_v6()", "unexpected error, ignored. sleep 1ms and continue receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v6();
		}
	}

	Trace_Exit(this, "handle_receive_v6");
}

void CommUDP::handle_discovery_msg(SCMessage_SPtr scMsg)
{
	Trace_Entry(this, "handle_discovery_msg");

	try
	{

		ByteBuffer_SPtr buffer = scMsg->getBuffer();

		SCMessage::H1Header h1 = scMsg->readH1Header(); //may throw

		bool put_in_Q = true;

		switch (h1.get<1>())
		{
		case SCMessage::Type_Topo_Discovery_Request_UDP:
		case SCMessage::Type_Topo_Discovery_Reply_UDP:
		{
			String bus = buffer->readString();
			String sender = buffer->readString();
			int64_t inc_num = buffer->readLong();
			//SplitBrain
			if (bus == busName_)
			{
				if (sender == myID_->getNodeName())
				{
					handle_self_message(scMsg, inc_num);
					put_in_Q = false;
				}

				scMsg->setSender(nodeID_Cache_.getOrCreate(sender));
				scMsg->setBusName(busName_SPtr_);
				Trace_Debug(this, "handle_discovery_msg()", "received",
						"msg", scMsg->toString(), "sender", sender, "bus", bus);
			}
			else
			{
				String what("Incompatible Bus: ");
				what.append(bus);
				what.append(", message ignored");
				Trace_Event(this, "handle_discovery_msg()", what,
						"msg", scMsg->toString(),
						"sender", sender, "bus", bus);
				deliver_warning_event(what,event::Message_Refused_Incompatible_BusName);
				put_in_Q = false;
			}
		}
		break;

		case SCMessage::Type_Mem_Node_Update_UDP: //For HPM (High Priority Monitoring)
		{

			String bus = buffer->readString();
			String sender = buffer->readString();
			int64_t inc_num = buffer->readLong();
			//SplitBrain
			if (bus == busName_)
			{
				if (sender == myID_->getNodeName())
				{
					handle_self_message(scMsg, inc_num);
					put_in_Q = false;
				}

				scMsg->setSender(nodeID_Cache_.getOrCreate(sender));
				scMsg->setBusName(busName_SPtr_);
				Trace_Debug(this, "handle_discovery_msg()", "received",
						"msg", scMsg->toString(), "bus", bus, "sender", sender,
						"inc", boost::lexical_cast<std::string>(inc_num));

			}
			else
			{
				String what("Incompatible Bus: ");
				what.append(bus);
				what.append(", message ignored");
				Trace_Event(this, "handle_discovery_msg()", what,
						"msg", scMsg->toString(),
						"sender", sender, "bus", bus);
				deliver_warning_event(what,event::Message_Refused_Incompatible_BusName);
				put_in_Q = false;
			}
		}
		break;

		default:
		{
			String what("CommUDP: Unsupported message type: ");
			what.append(scMsg->toString());
			Trace_Event(this, "handle_discovery_msg()", what);
			put_in_Q = false;
			deliver_warning_event(what,event::Message_Refused_Parse_Error);
		}
		}

		if (put_in_Q)
		{
			incomingMsgQ_->onMessage(scMsg);
		}

	}
	catch(MessageUnmarshlingException& mue)
	{
		String what("Warning: UDP discovery: Exception while unmarshling incoming message: ");
		what.append(mue.what());
		Trace_Warning(this, "handle_discovery_msg()", what);
		if (mue.getErrorCode() != event::Component_Failure)
		{
			deliver_warning_event(what,mue.getErrorCode());
		}
		else
		{
			deliver_fatal_event(what,mue.getErrorCode());
		}
	}
	catch (std::exception& e)
	{
		std::ostringstream what;
		what << "Warning: UDP discovery: Exception while unmarshling incoming message: what="
				<< e.what() << "; typeid=" << typeid(e).name();
		Trace_Warning(this, "handle_discovery_msg()", what.str());
		deliver_warning_event(what.str(),event::Message_Refused_Parse_Error);
	}

	Trace_Exit(this, "handle_discovery_msg");
}

//SplitBrain
void CommUDP::deliver_dup_local_node_event(const String& errMsg, spdr::event::ErrorCode errCode, int64_t incomingIncNum)
{
	try
	{	// create the suspicion event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender(myID_);
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<CommEventInfo>(
				new CommEventInfo(CommEventInfo::Fatal_Error, 0));
		eventInfo->setErrCode(errCode);
		eventInfo->setErrMsg(errMsg);
		eventInfo->setIncNum(incomingIncNum);
		scMsg->setCommEventInfo(eventInfo);
		incomingMsgQ_->onMessage(scMsg);
	}
	catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "deliver_dup_local_node_event()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
}

void  CommUDP::deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode)
{
	try
	{	// create the warning event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender(nodeID_Cache_.getOrCreate(String("Not Available")));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<CommEventInfo>(
				new CommEventInfo(CommEventInfo::Warning_UDP_Message_Refused, 0));
		eventInfo->setErrCode(errCode);
		eventInfo->setErrMsg(errMsg);
		scMsg->setCommEventInfo(eventInfo);
		incomingMsgQ_->onMessage(scMsg);
	}
	catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "deliver_warning_event()", "SpiderCastRuntimeError, ignored", "what", e.what());
	}
}

void  CommUDP::deliver_fatal_event(const String& errMsg, spdr::event::ErrorCode errCode)
{
	try
	{	// create the warning event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender(nodeID_Cache_.getOrCreate(String("Not Available")));
		boost::shared_ptr<CommEventInfo> eventInfo = boost::shared_ptr<CommEventInfo>(
				new CommEventInfo(CommEventInfo::Fatal_Error, 0));
		eventInfo->setErrCode(errCode);
		eventInfo->setErrMsg(errMsg);
		scMsg->setCommEventInfo(eventInfo);
		incomingMsgQ_->onMessage(scMsg);
	}
	catch (SpiderCastRuntimeError& e)
	{
		Trace_Warning(this, "deliver_fatal_event()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
}

void CommUDP::handle_self_message(SCMessage_SPtr scMsg, int64_t inc_num)
{
	if (inc_num > incarnationNum_)
	{
		std::ostringstream errMsg;
		errMsg << "Duplicate node detected (AKA 'Split Brain')."
				<< " Another node with the same name but a higher incarnation number was detected."
				<< " this node is shutting down, the other node will probably continue.";
		Trace_Error(this, "handle_self_message()", errMsg.str(),
				"msg", scMsg->toString(),
				"local-inc", boost::lexical_cast<std::string>(incarnationNum_),
				"remote-inc", boost::lexical_cast<std::string>(inc_num));
		deliver_dup_local_node_event(errMsg.str(), event::Duplicate_Local_Node_Detected, inc_num);
	}
	else
	{
		Trace_Event(this, "handle_self_message()", "Duplicate node suspicion, message dropped.",
				"msg", scMsg->toString(),
				"local-inc", boost::lexical_cast<std::string>(incarnationNum_),
				"remote-inc", boost::lexical_cast<std::string>(inc_num));
	}
}
}
