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
 * CommUDPMulticast.cpp
 *
 *  Created on: Oct 29, 2014
 */

#include <CommUDPMulticast.h>
#include <CommUtils.h>

namespace spdr
{

ScTraceComponent* CommUDPMulticast::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Comm,
		trace::ScTrConstants::Layer_ID_CommAdapter,
		"CommUDPMulticast",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

CommUDPMulticast::CommUDPMulticast(
		const String& instID,
		NodeIDImpl_SPtr myNodeID,
		BusName_SPtr busName,
		int64_t incarnationNum,
		const std::string& multicastOutboundInterface_v4,
		const std::string& multicastAddress_v4,
		int64_t multicastOutboundInterface_v6,
		const std::string& multicastAddress_v6,
		uint16_t multicastPort,
		uint8_t multicastHops,
		uint16_t packetSize,
		uint32_t sendBufferSize,
		uint32_t rcvBufferSize,
		NodeIDCache& cache,
		IncomingMsgQ_SPtr incomingMsgQ) :
				ScTraceContext(tc_, instID, ""),
				nodeID_Cache_(cache),
				incomingMsgQ_(incomingMsgQ),
				stop_(false),
				stopMutex_(),
				txMutex_(),
				io_service_(),
				resolver_(io_service_),
				tx_socket_v4_(io_service_),
				rcv_socket_v4_(io_service_),
				tx_socket_v6_(io_service_),
				rcv_socket_v6_(io_service_),
				thread_(instID, "CommMulticast", io_service_),
				myID_(myNodeID),
				busName_(busName->toOrgString()),
				busName_SPtr_(busName),
				incarnationNum_(incarnationNum),
				multicastOutboundInterface_v4_str_(multicastOutboundInterface_v4),
				multicastAddress_v4_str_(multicastAddress_v4),
				multicastOutboundInterface_v6_index_(multicastOutboundInterface_v6),
				multicastAddress_v6_str_(multicastAddress_v6),
				multicastPort_(multicastPort),
				multicastHops_(multicastHops),
				udpPacketSize_(packetSize),
				sendBufferSize_(sendBufferSize),
				rcvBufferSize_(rcvBufferSize),
				rcv_buffer_(new char[udpPacketSize_]),
				rcv_buffer_v6_(new char[udpPacketSize_]),
				remote_endpoint_v4_(),
				multicast_endpoint_v4_(),
				remote_endpoint_v6_(),
				multicast_endpoint_v6_(),
				v4_enabled_(false),
				v6_enabled_(false)
{
	using namespace boost::asio::ip;

	Trace_Entry(this,"CommUDPMulticast()");
}

CommUDPMulticast::~CommUDPMulticast()
{
	Trace_Entry(this,"~CommUDPMulticast()");

	try
	{
		tx_socket_v4_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDPMulticast()", "error closing (IPv4 tx) socket",
				"what", String(e.what()));
	}

	try
	{
		rcv_socket_v4_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDPMulticast()", "error closing (IPv4 rcv) socket",
				"what", String(e.what()));
	}

	try
	{
		tx_socket_v6_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDPMulticast()", "error closing (IPv6 tx) socket",
				"what", String(e.what()));
	}

	try
	{
		rcv_socket_v6_.close();
	}
	catch (boost::system::system_error & e)
	{
		Trace_Event(this,"~CommUDPMulticast()", "error closing (IPv6 rcv) socket",
				"what", String(e.what()));
	}

	delete[] rcv_buffer_;
	delete[] rcv_buffer_v6_;

	Trace_Exit(this,"~CommUDPMulticast()");
}

void CommUDPMulticast::init() throw (SpiderCastRuntimeError)
{
	using namespace boost::asio;
	using boost::asio::ip::udp;

	Trace_Entry(this, "init()");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);

		//IPv4
		if (!multicastAddress_v4_str_.empty())
		{
			try
			{
				ip::address_v4 multicast_address_v4 = ip::address_v4::from_string(multicastAddress_v4_str_);
				if (!multicast_address_v4.is_multicast())
				{
					stop_ = true;
					std::ostringstream what;
					what << "Not an IPv4 multicast address: " << multicastAddress_v4_str_ << " / " << multicast_address_v4.to_string();
					Trace_Event(this, "init()", "failed", "what", what.str());
					throw SpiderCastRuntimeError(what.str());
				}

				multicast_endpoint_v4_ = udp::endpoint(multicast_address_v4,multicastPort_);

				Trace_Event(this, "init()", "multicast IPv4 endpoint",
						"address", multicastAddress_v4_str_,
						"port", boost::lexical_cast<std::string>(multicastPort_));

				ip::address_v4 multicast_outbound_interface;
				if (!multicastOutboundInterface_v4_str_.empty())
				{
					multicast_outbound_interface = ip::address_v4::from_string(multicastOutboundInterface_v4_str_);
				}

				tx_socket_v4_.open(udp::v4());
				if (spdr::set_CLOEXEC(tx_socket_v4_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Tx V4 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}

				tx_socket_v4_.set_option(
						boost::asio::socket_base::send_buffer_size(sendBufferSize_));
				boost::asio::socket_base::send_buffer_size optionR;
				tx_socket_v4_.get_option(optionR);
				Trace_Event<int>(this, "init()", "send buffer v4", "size",
						optionR.value());

				if (!multicast_outbound_interface.is_unspecified())
				{
					tx_socket_v4_.set_option( ip::multicast::outbound_interface( multicast_outbound_interface ) );
					Trace_Event(this, "init()", "set multicast v4 out-bound interface",
							"address", multicastOutboundInterface_v4_str_);
				}
				else
				{
					Trace_Event(this, "init()", "multicast v4 out/in-bound IFC - Unspecified");
				}

				rcv_socket_v4_.open(udp::v4());
				if (spdr::set_CLOEXEC(rcv_socket_v4_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Rcv V4 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}

				rcv_socket_v4_.set_option(
						boost::asio::socket_base::receive_buffer_size(rcvBufferSize_));
				boost::asio::socket_base::receive_buffer_size optR;
				rcv_socket_v4_.get_option(optR);
				Trace_Event<int>(this, "init()", "rcv buffer v4", "size", optionR.value());

				rcv_socket_v4_.set_option( boost::asio::socket_base::reuse_address(true) );
				rcv_socket_v4_.set_option( boost::asio::ip::multicast::enable_loopback(true) );

				rcv_socket_v4_.set_option(boost::asio::ip::multicast::hops(multicastHops_) );
				boost::asio::ip::multicast::hops hopsR;
				rcv_socket_v4_.get_option(hopsR);
				Trace_Event<int>(this, "init()", "multicast-hops v4", "num", hopsR.value());

				rcv_socket_v4_.bind(udp::endpoint(multicast_address_v4, multicastPort_));

				rcv_socket_v4_.set_option( ip::multicast::join_group( multicast_address_v4, multicast_outbound_interface ) );

				v4_enabled_ = true;

				Trace_Event(this, "init()", "IPv4 Multicast initialized successfully.");

			}
			catch (boost::system::system_error& se)
			{
				Trace_Event(this, "init()", "v4 failed",
						"what", se.what(), "message", se.code().message());
				v4_enabled_ = false;
			}
		}
		else
		{
			Trace_Event(this, "init()", "IPv4 group address empty, protocol disabled");
			v4_enabled_ = false;
		}

		//IPv6
		if (!multicastAddress_v6_str_.empty())
		{
			try
			{
				ip::address_v6 multicast_address_v6 = ip::address_v6::from_string(multicastAddress_v6_str_);
				if (!multicast_address_v6.is_multicast())
				{
					stop_ = true;
					std::ostringstream what;
					what << "Not an IPv6 multicast address: " << multicastAddress_v6_str_ << " / " << multicast_address_v6.to_string();
					Trace_Event(this, "init()", "v6 failed", "what", what.str());
					throw SpiderCastRuntimeError(what.str());
				}

				multicast_endpoint_v6_ = udp::endpoint(multicast_address_v6,multicastPort_);

				Trace_Event(this, "init()", "multicast IPv6 endpoint",
						"address", multicastAddress_v6_str_,
						"port", boost::lexical_cast<std::string>(multicastPort_));

				tx_socket_v6_.open(udp::v6());
				if (spdr::set_CLOEXEC(tx_socket_v6_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Tx V6 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}

				tx_socket_v6_.set_option(
						boost::asio::socket_base::send_buffer_size(sendBufferSize_));
				boost::asio::socket_base::send_buffer_size optionR;
				tx_socket_v6_.get_option(optionR);
				Trace_Event<int>(this, "init()", "send buffer v6", "size",
						optionR.value());

				if (multicastOutboundInterface_v6_index_ > 0)
				{
					tx_socket_v6_.set_option( ip::multicast::outbound_interface( multicastOutboundInterface_v6_index_ ) );
					Trace_Event(this, "init()", "set multicast v6 out-bound interface",
							"address", multicastOutboundInterface_v4_str_);
				}
				else
				{
					Trace_Event(this, "init()", "multicast v6 out/in-bound IFC - Unspecified");
				}

				rcv_socket_v6_.open(udp::v6());
				if (spdr::set_CLOEXEC(rcv_socket_v6_.native_handle()) != 0)
				{
					int errsv = errno;
					Trace_Warning(this,"init()","Warning: failed to set FD_CLOEXEC on UDP Rcv V6 socket, ignoring",
							"errno", spdr::stringValueOf(errsv));
				}

				rcv_socket_v6_.set_option(
						boost::asio::socket_base::receive_buffer_size(rcvBufferSize_));
				boost::asio::socket_base::receive_buffer_size optR;
				rcv_socket_v6_.get_option(optR);
				Trace_Event<int>(this, "init()", "rcv buffer v6", "size", optionR.value());

				rcv_socket_v6_.set_option( boost::asio::socket_base::reuse_address(true) );
				rcv_socket_v6_.set_option( boost::asio::ip::multicast::enable_loopback(true) );

				rcv_socket_v6_.set_option(boost::asio::ip::multicast::hops(multicastHops_) );
				boost::asio::ip::multicast::hops hopsR;
				rcv_socket_v6_.get_option(hopsR);
				Trace_Event<int>(this, "init()", "multicast-hops v6", "num", hopsR.value());

				rcv_socket_v6_.bind(udp::endpoint(multicast_address_v6, multicastPort_));
//				ip::v6_only v6only(true);
//				rcv_socket_v6_.set_option( v6only );
//				Trace_Event(this, "init()", "v6 only");
				if (multicastOutboundInterface_v6_index_>0)
				{
					rcv_socket_v6_.set_option( ip::multicast::join_group(multicast_address_v6, multicastOutboundInterface_v6_index_) );
				}
				else
				{
					rcv_socket_v6_.set_option( ip::multicast::join_group(multicast_address_v6) );
				}
				Trace_Event(this, "init()", "join");

				v6_enabled_ = true;
				Trace_Event(this, "init()", "IPv6 Multicast initialized successfully.");
			}
			catch (boost::system::system_error& se)
			{
				Trace_Event(this, "init()", "v6 failed",
						"what", se.what(),
						"message", se.code().message());
				v6_enabled_ = false;
			}
		}
		else
		{
			Trace_Event(this, "init()", "IPv4 group address empty, protocol disabled");
			v6_enabled_ = false;
		}

		if (!v4_enabled_ && !v6_enabled_)
		{
			std::string what("Error: both IPv4 and IPv6 multicast are disabled or failed to initialize");
			Trace_Error(this, "init()", what);
			throw SpiderCastRuntimeError(what);
		}
	}

	Trace_Exit(this, "init()");

}

void CommUDPMulticast::start()
{
	using boost::asio::ip::udp;

	Trace_Entry(this,"start()");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);

		if (!stop_)
		{
			if (v4_enabled_)
			{
				start_receive_v4();
			}
			if (v6_enabled_)
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

void CommUDPMulticast::start_receive_v4()
{
	Trace_Entry(this, "start_receive_v4()");

	//TODO check return value
	rcv_socket_v4_.async_receive_from(
			boost::asio::buffer(rcv_buffer_ ,udpPacketSize_),
			remote_endpoint_v4_,
			boost::bind(&CommUDPMulticast::handle_receive_v4, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	Trace_Exit(this, "start_receive_v4()");
}

void CommUDPMulticast::start_receive_v6()
{
	Trace_Entry(this, "start_receive_v6()");

	rcv_socket_v6_.async_receive_from(
			boost::asio::buffer(rcv_buffer_v6_ ,udpPacketSize_),
			remote_endpoint_v6_,
			boost::bind(&CommUDPMulticast::handle_receive_v6, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	Trace_Exit(this, "start_receive_v6()");
}

void CommUDPMulticast::handle_receive_v4(
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
			what << "Warning: multicast discovery: Exception while unmarshling incoming message: "
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
			Trace_Event(this, "handle_receive_v4()", "error operation_canceled expected on stop",
					"message", error.message());
		}
		else if (error.value() == boost::system::errc::message_size)
		{
			Trace_Event(this, "handle_receive_v4()", "error message_size, ignored. sleep 1ms, continue to receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v4();
		}
		else
		{
			Trace_Event(this, "handle_receive_v4()", "unexpected error, ignored. sleep 1ms, continue to receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v4();

			// We will not stop the server because of errors...
			// std::ostringstream errMsg;
			// errMsg << "CommUDPMulticast failure: " << error.message() << "; value=" << error.value();
			// deliver_fatal_event(errMsg.str(), event::Component_Failure);
		}
	}

	Trace_Exit(this, "handle_receive_v4");
}


void CommUDPMulticast::handle_receive_v6(
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
			what << "Warning: multicast discovery: Exception while unmarshling incoming message: what=" << e.what() << "; typeid=" << typeid(e).name();
			Trace_Warning(this, "handle_receive_v6()", what.str());
			deliver_warning_event(what.str(),event::Message_Refused_Parse_Error);
		}

		start_receive_v6();
	}
	else
	{
		if (error.value() == boost::system::errc::operation_canceled)
		{
			Trace_Event(this, "handle_receive_v6()", "operation_canceled expected on stop",
					"message", error.message());
		}
		else if (error.value() == boost::system::errc::message_size)
		{
			Trace_Event(this, "handle_receive_v6()", "error message_size, ignored. sleep 1ms, continue to receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v6();
		}
		else
		{
			Trace_Event(this, "handle_receive_v6()", "unexpected error, ignored. sleep 1ms, continue to receive.",
					"message", error.message());
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
			start_receive_v6();

			// We will not stop the server because of errors...
			// std::ostringstream errMsg;
			// errMsg << "CommUDPMulticast failure: " << error.message() << "; value=" << error.value();
			// deliver_fatal_event(errMsg.str(), event::Component_Failure);
		}
	}

	Trace_Exit(this, "handle_receive_v6");
}

void CommUDPMulticast::handle_discovery_msg(SCMessage_SPtr scMsg)
{
	Trace_Entry(this, "handle_discovery_msg()", spdr::toString(scMsg));

	try
	{
		SCMessage::H1HeaderVer h1 = scMsg->readH1HeaderV();

		ByteBuffer_SPtr bb = scMsg->getBuffer();

		bool put_in_Q = true;

		switch (h1.type)
		{
		case SCMessage::Type_Topo_Discovery_Request_Multicast:
		case SCMessage::Type_Topo_Discovery_Reply_Multicast:
		{
			String bus = bb->readString();
			if (bus == busName_)
			{
				NodeIDImpl_SPtr senderID = bb->readNodeID();
				if ( senderID->getNodeName() != myID_->getNodeName())
				{
					scMsg->setSender(senderID);
					scMsg->setBusName(busName_SPtr_);
					Trace_Debug(this, "handle_discovery_msg()", "received",
							"msg", scMsg->toString(), "bus", bus, "sender", senderID->toString());
				}
				else
				{
					NodeVersion ver = bb->readNodeVersion();
					if (ver.getIncarnationNumber() > incarnationNum_)
					{
						std::ostringstream errMsg;
						errMsg << "Duplicate node detected (AKA 'Split Brain')."
								<< " Another node with the same name but a higher incarnation number was detected."
								<< " this node is shutting down, the other node will probably continue.";
						Trace_Error(this, "handle_discovery_msg()", errMsg.str(),
								"msg", scMsg->toString(),
								"local-inc", boost::lexical_cast<String>(incarnationNum_),
								"remote-inc", boost::lexical_cast<String>(ver.getIncarnationNumber()));
						deliver_dup_local_node_event(errMsg.str(), event::Duplicate_Local_Node_Detected, ver.getIncarnationNumber());
					}
					else
					{
						Trace_Debug(this, "handle_discovery_msg()", "Message from myself, ignored",
								"msg", scMsg->toString(),
								"local-inc", boost::lexical_cast<String>(incarnationNum_),
								"remote-inc", boost::lexical_cast<String>(ver.getIncarnationNumber()));
					}
					put_in_Q = false;
				}
			}
			else
			{
				String what("Warning: multicast discovery: Incompatible Bus: ");
				what.append(bus);
				what.append(", message ignored");
				Trace_Warning(this, "handle_discovery_msg()", what, "msg", scMsg->toString());
				put_in_Q = false;
				deliver_warning_event(what,event::Message_Refused_Incompatible_BusName);
			}
		}
		break;

		default:
		{
			String what("Warning: multicast discovery: CommUDPMulticast: Unsupported message type: ");
			what.append(scMsg->toString());
			Trace_Warning(this, "handle_discovery_msg()", what);
			put_in_Q = false;
			deliver_warning_event(what,event::Message_Refused_Parse_Error);
		}
		}

		if (put_in_Q)
		{
			//The position is before the version
			incomingMsgQ_->onMessage(scMsg);
		}
	}
	catch(MessageUnmarshlingException& mue)
	{
		String what("Warning: multicast discovery: Exception while unmarshling incoming message: ");
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
		what << "Warning: multicast discovery: Exception while unmarshling incoming message: "
				<< e.what() << "; typeid=" << typeid(e).name();
		Trace_Warning(this, "handle_discovery_msg()", what.str());
		deliver_warning_event(what.str(),event::Message_Refused_Parse_Error);
	}

	Trace_Exit(this, "handle_discovery_msg()");
}

bool CommUDPMulticast::isStopped() const
{
	boost::recursive_mutex::scoped_lock lock(stopMutex_);
	return stop_;
}

bool CommUDPMulticast::isV4Enabled() const
{
	boost::recursive_mutex::scoped_lock lock(stopMutex_);
	return v4_enabled_;
}

bool CommUDPMulticast::isV6Enabled() const
{
	boost::recursive_mutex::scoped_lock lock(stopMutex_);
	return v6_enabled_;
}

void CommUDPMulticast::stop()
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

		if (v4_enabled_)
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

		if (v6_enabled_)
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

			try
			{
				tx_socket_v6_.close();
			}
			catch (boost::system::system_error & e)
			{
				Trace_Event(this,"stop()", "error closing (IPv6 tx) socket",
						"what", String(e.what()));
			}
		}
	}

	if (boost::this_thread::get_id() != thread_.getID())
	{
		thread_.join();
	}

	Trace_Exit(this,"stop()");
}

bool CommUDPMulticast::sendToMCGroup(SCMessage_SPtr msg)
{
	Trace_Entry(this, "sendToMCGroup()","single message");

	{
		boost::recursive_mutex::scoped_lock lock(stopMutex_);
		if (stop_)
		{
			Trace_Event(this, "sendToMCGroup()", "closed, skipped");
			return false;
		}
	}

	{
		boost::recursive_mutex::scoped_lock lock(txMutex_);

		bool status = false;

		if (v4_enabled_)
		{
			try
			{
				Trace_Debug(this, "sendToMCGroup()", "before v4 socket.send_to");

				size_t sent = tx_socket_v4_.send_to(
						boost::asio::buffer(msg->getBuffer()->getBuffer(), msg->getBuffer()->getDataLength()),
						multicast_endpoint_v4_);

				if (sent == msg->getBuffer()->getDataLength())
				{
					Trace_Debug(this, "sendToMCGroup()", "after v4 socket.send_to",
							"bytes-sent", boost::lexical_cast<std::string>(sent));
					status = true;
				}
				else
				{
					Trace_Event(this, "sendToMCGroup()", "failed to send packet, short write, v4");
					status = false;
				}
			}
			catch (boost::system::system_error& se)
			{
				Trace_Event(this, "sendToMCGroup()", "failed to send packet, v4", "what", se.what());
				status = false;
			}
		}

		if (v6_enabled_)
		{
			try
			{
				Trace_Debug(this, "sendToMCGroup()", "before v6 socket.send_to",
						"addr", multicast_endpoint_v6_.address().to_string(),
						"port", boost::lexical_cast<std::string>(multicast_endpoint_v6_.port()));

				size_t sent = tx_socket_v6_.send_to(
						boost::asio::buffer(msg->getBuffer()->getBuffer(), msg->getBuffer()->getDataLength()),
						multicast_endpoint_v6_);

				if (sent == msg->getBuffer()->getDataLength())
				{
					Trace_Debug(this, "sendToMCGroup()", "after v6 socket.send_to",
							"bytes-sent", boost::lexical_cast<std::string>(sent));
					status = true;
				}
				else
				{
					Trace_Event(this, "sendToMCGroup()", "failed to send packet, short write, v6");
					status = status || false;
				}
			}
			catch (boost::system::system_error& se)
			{
				Trace_Event(this, "sendToMCGroup()", "failed to send packet, v6", "what", se.what());
				status = status || false;
			}
		}

		Trace_Exit(this, "sendToMCGroup()",status);
		return status;
	}
}

bool CommUDPMulticast::sendToMCGroup(std::vector<SCMessage_SPtr>& msgBundle,
		int numMsgs)
{
	Trace_Event<int>(this,"sendToMCGroup(Bundle)","Bundle","num", numMsgs);

	bool res = false;
	for (int i=0; i<numMsgs; ++i)
	{
		res = sendToMCGroup(msgBundle[i]);
		if (!res)
		{
			Trace_Event(this,"sendToMCGroup(Bundle)","failed to send bundle");
			break;
		}
	}
	Trace_Exit<bool>(this,"sendToMCGroup(Bundle)",res);
	return res;

}

void  CommUDPMulticast::deliver_warning_event(const String& errMsg, spdr::event::ErrorCode errCode)
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

void  CommUDPMulticast::deliver_fatal_event(const String& errMsg, spdr::event::ErrorCode errCode)
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
		Trace_Warning(this, "deliver_fatal_event()", "SpiderCastRuntimeError, ignored", "what", e.what());
	}
}

void CommUDPMulticast::deliver_dup_local_node_event(const String& errMsg, spdr::event::ErrorCode errCode, int64_t incomingIncNum)
{
try
	{	// create the suspicion event for topology
		SCMessage_SPtr scMsg = SCMessage_SPtr(new SCMessage());
		scMsg->setSender( myID_);
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
		Trace_Warning(this, "deliver_dup_node_suspicion_event()", "SpiderCastRuntimeError, ignored", "what",e.what());
	}
}
} /* namespace mcp */
