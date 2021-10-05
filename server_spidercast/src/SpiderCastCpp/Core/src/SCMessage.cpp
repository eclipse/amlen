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
 * SCMessage.cpp
 *
 *  Created on: 15/03/2010
 */

#include "SCMessage.h"

//FIXME convert all exceptions to either Marshaling/Unmarshaling exceptions
namespace spdr
{

const char* const SCMessage::messageTypeName[] =
{
		"Type_None",

		"Type_Mem_Node_Update",
		"Type_Mem_Node_Leave",
		"Type_Mem_Node_Leave_Ack",
		"Type_Mem_Metadata_Update",
		"Type_Mem_Metadata_Request",
		"Type_Mem_Metadata_RequestPush",
		"Type_Mem_Metadata_Reply",

		"Type_Mem_Node_Update_UDP",

		"Type_Topo_Discovery_Request_TCP",
		"Type_Topo_Discovery_Reply_TCP",
		"Type_Topo_Discovery_Request_UDP",
		"Type_Topo_Discovery_Reply_UDP",
		"Type_Topo_Discovery_Request_Multicast",
		"Type_Topo_Discovery_Reply_Multicast",
		"Type_Topo_Connect_Successor",
		"Type_Topo_Connect_Successor_OK",
		"Type_Topo_Connect_Random_Request",
		"Type_Topo_Connect_Random_Reply",
		"Type_Topo_Connect_Structured_Request",
		"Type_Topo_Connect_Structured_Reply",
		"Type_Topo_Disconnect_Request",
		"Type_Topo_Disconnect_Reply",
		"Type_Topo_Degree_Changed",
		"Type_Topo_Node_Leave",

		"Type_Topo_Comm_Event",

		"Type_Hier_Connect_Request",
		"Type_Hier_Connect_Reply",
		"Type_Hier_Disconnect_Request",
		"Type_Hier_Disconnect_Reply",
		"Type_Hier_Leave",

		"Type_Hier_DelOp_Request_StartMembershipPush",
		"Type_Hier_DelOp_Reply_StartMembershipPush",
		"Type_Hier_DelOp_Request_StopMembershipPush",
		"Type_Hier_DelOp_Reply_StopMembershipPush",

		"Type_Hier_SupOp_Request_ViewUpdate",
		"Type_Hier_SupOp_Reply_ViewUpdate",

		"Type_Hier_SupOp_Request_ForeignZoneMembership",
		"Type_Hier_SupOp_Reply_ForeignZoneMemberships",
		"Type_Hier_SupOp_Request_ZoneCensus",
		"Type_Hier_SupOp_Reply_ZoneCensus",

		"Type_Hier_PubSubBridge_BaseZoneInterest",
		"Type_Hier_PubSubBridge_BaseZoneInterest_Reply",

		"Type_DHT_SOMETHING",
		"Type_DHT_Comm_Event",

		"Type_General_Comm_Event",

		"Type_Data_PubSub",
		"Type_Data_Direct",
		"Type_Data_SOMETHING",

		"Type_Trans_SOMETHING"
//etc...
		};

const SCMessage::MessageGroup SCMessage::messageType2Group[] =
{
		Group_None, //"Type_None"

		Group_Membership, //"Type_Mem_Node_Update"
		Group_Membership, //"Type_Mem_Node_Leave"
		Group_Membership, //"Type_Mem_Node_Leave_Ack"
		Group_Membership, //"Type_Mem_Metadata_Update"
		Group_Membership, //"Type_Mem_Metadata_Request"
		Group_Membership, //"Type_Mem_Metadata_RequestPush"
		Group_Membership, //"Type_Mem_Metadata_Reply"

		Group_Membership, //"Type_Mem_Node_Update_UDP"

		Group_Topology, //"Type_Topo_Discovery_Request_TCP"
		Group_Topology, //"Type_Topo_Discovery_Reply_TCP"
		Group_Topology, //"Type_Topo_Discovery_Request_UDP"
		Group_Topology, //"Type_Topo_Discovery_Reply_UDP"
		Group_Topology, //"Type_Topo_Discovery_Request_Multicast"
		Group_Topology, //"Type_Topo_Discovery_Reply_Multicast"

		Group_Topology, //"Type_Topo_Connect_Successor"
		Group_Topology, //"Type_Topo_Connect_Successor_OK"
		Group_Topology, //"Type_Topo_Connect_Random_Request"
		Group_Topology, //"Type_Topo_Connect_Random_Reply"
		Group_Topology, //"Type_Topo_Connect_Structured_Request",
		Group_Topology, //"Type_Topo_Connect_Structured_Reply",
		Group_Topology, //"Type_Topo_Disconnect_Request"
		Group_Topology, //"Type_Topo_Disconnect_Reply"
		Group_Topology, //"Type_Topo_Degree_Changed"
		Group_Topology, //"Type_Topo_Node_Leave"

		Group_Topology, //"Type_Topo_Comm_Event"

		Group_Hierarchy, //"Type_Hier_Connect_Request"
		Group_Hierarchy, //"Type_Hier_Connect_Reply"
		Group_Hierarchy, //"Type_Hier_Disconnect_Request"
		Group_Hierarchy, //"Type_Hier_Disconnect_Reply"
		Group_Hierarchy, //"Type_Hier_Leave",
		Group_Hierarchy, //"Type_Hier_DelOp_Request_Start..."
		Group_Hierarchy, //"Type_Hier_DelOp_Reply_Start..."
		Group_Hierarchy, //"Type_Hier_DelOp_Request_Stop..."
		Group_Hierarchy, //"Type_Hier_DelOp_Reply_Stop..."
		Group_Hierarchy, //"Type_Hier_SupOp_Request_ViewUpdate"
		Group_Hierarchy, //"Type_Hier_SupOp_Reply_ViewUpdate"
		Group_Hierarchy, //"Type_Hier_SupOp_Request_ForeignZoneMembership",
		Group_Hierarchy, //"Type_Hier_SupOp_Reply_ForeignZoneMemberships",
		Group_Hierarchy, //"Type_Hier_SupOp_Request_ZoneCensus",
		Group_Hierarchy, //"Type_Hier_SupOp_Reply_ZoneCensus",

		Group_Hierarchy, //"Type_Hier_PubSubBridge_BaseZoneInterest",
		Group_Hierarchy, //"Type_Hier_PubSubBridge_BaseZoneInterest_Reply",

		Group_DHT, //"Type_DHT_SOMETHING",
		Group_DHT, //"Type_DHT_Comm_Event",

		Group_GeneralCommEvents, // "Type_General_Comm_Event",

		Group_Data, //Type_Data_PubSub
		Group_Data, // "Type_Data_Direct",
		Group_Data, //"Type_Data_SOMETHING"

		Group_Transport, //"Type_Trans_SOMETHING"
		//etc...
		};

const char* const SCMessage::messageGroupName[] =
{ 
		"Group_None",
		"Group_Membership",
		"Group_Topology",
		"Group_Hierarchy",
		"Group_DHT",
		"Group_GeneralCommEvents",
		"Group_Data",
		"Group_Transport" };

const char* const SCMessage::messageRoutingProtocolName[] =
{
		"RoutingProto_None",
		"RoutingProto_P2P",
		"RoutingProto_PubSub",
		"RoutingProto_Broadcast",
		"RoutingProto_Flooding",
		"RoutingProto_Max" };

const char* const SCMessage::messageTransProtocolName[] =
{
		"TransProto_None",
		"TransProto_PubSub",
		"TransProto_P2P",
		"TransProto_Broadcast",
		"TransProto_Max" };

const char* const SCMessage::messageReliabilityModeName[] =
{
		"ReliabilityMode_None",
		"ReliabilityMode_BestEffort",
		"ReliabilityMode_BulletinBoard",
		"ReliabilityMode_NackBase",
		"ReliabilityMode_AckBased",
		"ReliabilityMode_Max"
};

SCMessage::SCMessage() :
		_sender(),
		_senderLocalName(),
		_busName(),
		_buffer(),
		_streamId(0),
		_commEventInfo(),
		rxMessage_()
{
}

SCMessage::~SCMessage()
{
}

//=== setters and getters ===

NodeIDImpl_SPtr SCMessage::getSender() const
{
	return _sender;
}

void SCMessage::setSender(NodeIDImpl_SPtr sender)
{
	_sender = sender;
}


StringSPtr SCMessage::getSenderLocalName() const
{
	return _senderLocalName;
}

void SCMessage::setSenderLocalName(const StringSPtr& senderLocalName)
{
	_senderLocalName = senderLocalName;
}

streamId_t SCMessage::getStreamId() const
{
	return _streamId;
}

void SCMessage::setStreamId(streamId_t streamId)
{
	_streamId = streamId;
}

BusName_SPtr SCMessage::getBusName() const
{
	return _busName;
}

void SCMessage::setBusName(BusName_SPtr busName)
{
	_busName = busName;
}

boost::shared_ptr<ByteBuffer> SCMessage::getBuffer() const
{
	return _buffer;
}

void SCMessage::setBuffer(boost::shared_ptr<ByteBuffer> buffer)
{
	_buffer = buffer;
}

boost::shared_ptr<CommEventInfo> SCMessage::getCommEventInfo() const
{
	return _commEventInfo;
}

void SCMessage::setCommEventInfo(boost::shared_ptr<CommEventInfo> eventInfo)
{
	_commEventInfo = eventInfo;
}

void SCMessage::reset()
{
	_buffer.reset();
	_sender.reset();
	_senderLocalName.reset();
	_commEventInfo.reset();
}

bool SCMessage::isValid()
{
	if ((_buffer && _buffer->isValid()) || _commEventInfo)
		return true;
	else
		return false;
}

//=== enum utilities ===


void SCMessage::verifyMessageGroupRange(int groupVal) throw (std::range_error)
{
	if (groupVal <= SCMessage::Group_None || groupVal >= SCMessage::Group_Max)
	{
		std::ostringstream oss;
		oss << "MessageGroup integer value " << groupVal << " out of range";
		throw std::range_error(oss.str());
	}
}

void SCMessage::verifyMessageTypeRange(int typeVal) throw (std::range_error)
{
	if (typeVal >= SCMessage::Type_Max || typeVal <= SCMessage::Type_None)
	{
		std::ostringstream oss;
		oss << "MessageType integer value " << typeVal << " out of range";
		throw std::range_error(oss.str());
	}
}

SCMessage::MessageGroup SCMessage::getMessageGroupFromType(MessageType type)
{
	int typeVal = (int) type;
	SCMessage::verifyMessageTypeRange(typeVal);
	MessageGroup group = messageType2Group[typeVal];
	SCMessage::verifyMessageGroupRange((int) group);
	return group;
}

String SCMessage::getMessageGroupName(MessageGroup group)
{
	int groupVal = (int) group;
	SCMessage::verifyMessageGroupRange(groupVal);
	return String(messageGroupName[groupVal]);
}

String SCMessage::getMessageTypeName(MessageType type)
{
	int typeVal = (int) type;
	SCMessage::verifyMessageTypeRange(typeVal);
	//return String((messageTypeAuxilary[typeVal]).name);
	return String(messageTypeName[typeVal]);
}

SCMessage::MessageGroup SCMessage::createMessageGroupFromInt(int groupVal)
		throw (std::range_error)
{
	verifyMessageGroupRange(groupVal);
	return static_cast<SCMessage::MessageGroup> (groupVal);
}

SCMessage::MessageType SCMessage::createMessageTypeFromInt(int typeVal)
		throw (std::range_error)
{
	verifyMessageTypeRange(typeVal);
	return static_cast<SCMessage::MessageType> (typeVal);
}

SCMessage::MessageRoutingProtocol SCMessage::createMessageRoutingProtocolFromInt(
		int rpVal) throw (std::range_error)
{
	if (rpVal >= SCMessage::RoutingProto_Max || rpVal
			<= SCMessage::RoutingProto_None)
	{
		std::ostringstream oss;
		oss << "MessageRoutingProtocol integer value " << rpVal
				<< " out of range";
		throw std::range_error(oss.str());
	}
	return static_cast<SCMessage::MessageRoutingProtocol> (rpVal);
}


SCMessage::MessageTransProtocol SCMessage::createMessageTransProtocolFromInt(
		int tpVal) throw (std::range_error)
{
	if (tpVal >= SCMessage::TransProto_Max || tpVal
			<= SCMessage::TransProto_None)
	{
		std::ostringstream oss;
		oss << "MessageTransProtocol integer value " << tpVal
				<< " out of range";
		throw std::range_error(oss.str());
	}
	return static_cast<SCMessage::MessageTransProtocol> (tpVal);
}

SCMessage::MessageReliabilityMode SCMessage::createMessageReliabilityModeFromInt(
		int rmVal) throw (std::range_error)
{
	if (rmVal >= SCMessage::ReliabilityMode_Max || rmVal
			<= SCMessage::ReliabilityMode_None)
	{
		std::ostringstream oss;
		oss << "MessageReliabilityMode integer value " << rmVal
				<< " out of range";
		throw std::range_error(oss.str());
	}
	return static_cast<SCMessage::MessageReliabilityMode> (rmVal);
}

//=== H1 header utilities


SCMessage::H1Header SCMessage::readH1Header()
		throw (MessageUnmarshlingException)
{
	H1Header h1(Group_None, Type_None, 0);

	if (_buffer && (*_buffer).isValid())
	{
		try
		{

			(*_buffer).setPosition(Message_Prefix_Offset);

			uint16_t suppVer = _buffer->readShort();
			uint16_t usedVer = _buffer->readShort();
			if (usedVer > suppVer)
			{
				std::ostringstream oss;
				oss << "Corrupt message, supp-version < used-version, supp=" << suppVer
						<< ", used=" << usedVer;
				throw MessageUnmarshlingException(oss.str(), event::Message_Refused_Parse_Error);
			}

			//Only for the base version
			if (usedVer != config::SPIDERCAST_VERSION)
			{
				std::ostringstream oss;
				oss << "message used-version cannot be different than local version, used=" << usedVer
						<< ", expected(local)=" << config::SPIDERCAST_VERSION;
				throw MessageUnmarshlingException(oss.str(), event::Message_Refused_Incompatible_Version);
			}

			uint16_t type = (*_buffer).readShort();
			uint16_t group = type >> Message_Group_Shift_Bits;
			type = type & Message_Group_Mask;

			try
			{
				h1.get<0> () = SCMessage::createMessageGroupFromInt(group);
				h1.get<1> () = SCMessage::createMessageTypeFromInt(type);
			} catch (std::range_error& e)
			{
				throw MessageUnmarshlingException(e.what(), event::Message_Refused_Parse_Error);
			}

			h1.get<2> () = (*_buffer).readInt();
			int size_limit =
					((*_buffer).isReadOnly() ? (*_buffer).getDataLength()
							: (*_buffer).getCapacity());
			if ((h1.get<2> () < Message_H2_Offset) || (h1.get<2> ()
					> size_limit))
			{
				std::ostringstream oss;
				oss << "SCMessage total-length error, read=" << h1.get<2> ()
						<< ", expected [" << Message_H2_Offset << ","
						<< size_limit << "]";
				throw MessageUnmarshlingException(oss.str(),  event::Message_Refused_Parse_Error);
			}
		}
		catch (MessageUnmarshlingException& mue)
		{
			throw;
		}
		catch (SpiderCastLogicError& le)
		{
			String what("Failed to read H1-header, ");
			what.append(le.what());
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what("Failed to read H1-header, ");
			what.append(re.what());
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else if (_commEventInfo)
	{
		if (_commEventInfo->getContext() == DHTProxyCtx || _commEventInfo->getContext() == DHTServerCtx)
		{
			h1.get<0> () = getMessageGroupFromType(Type_DHT_Comm_Event);
			h1.get<1> () = Type_DHT_Comm_Event;
		}
		else if (_commEventInfo->getContext() != -1)
		{
			h1.get<0> () = getMessageGroupFromType(Type_Topo_Comm_Event);
			h1.get<1> () = Type_Topo_Comm_Event;
		}
		else
		{
			h1.get<0> () = getMessageGroupFromType(Type_General_Comm_Event);
			h1.get<1> () = Type_General_Comm_Event;
		}
	}
	else
	{
		throw MessageUnmarshlingException("null buffer", event::Component_Failure);
	}

	return h1;
}

SCMessage::H1HeaderVer SCMessage::readH1HeaderV()
		throw (MessageUnmarshlingException)
{
	SCMessage::H1HeaderVer h1;

	if (_buffer && _buffer->isValid())
	{
		try
		{
			(*_buffer).setPosition(Message_Prefix_Offset);

			h1.supportedVersion = static_cast<uint16_t>(_buffer->readShort());
			h1.usedVersion = static_cast<uint16_t>(_buffer->readShort());

			if (h1.usedVersion > h1.supportedVersion)
			{
				std::ostringstream oss;
				oss << "Corrupt message, supp-version < used-version, supp=" << h1.supportedVersion
						<< ", used=" << h1.usedVersion;
				throw MessageUnmarshlingException(oss.str(), event::Message_Refused_Parse_Error);
			}

			//Only for the base version
			if (h1.usedVersion != config::SPIDERCAST_VERSION)
			{
				std::ostringstream oss;
				oss << "message used-version cannot be different than local version, used=" << h1.usedVersion
						<< ", expected(local)=" << config::SPIDERCAST_VERSION;
				throw MessageUnmarshlingException(oss.str(), event::Message_Refused_Incompatible_Version);
			}

			uint16_t type = static_cast<uint16_t>(_buffer->readShort());
			uint16_t group = type >> Message_Group_Shift_Bits;
			type = type & Message_Group_Mask;

			try
			{
				h1.group = SCMessage::createMessageGroupFromInt(group);
				h1.type = SCMessage::createMessageTypeFromInt(type);
			}
			catch (std::range_error& e)
			{
				throw MessageUnmarshlingException(e.what(), event::Message_Refused_Parse_Error);
			}

			h1.length = (*_buffer).readInt();
			int size_limit =
					(_buffer->isReadOnly() ? _buffer->getDataLength()
							: _buffer->getCapacity());
			if ((h1.length < Message_H2_Offset) || (h1.length > size_limit))
			{
				std::ostringstream oss;
				oss << "SCMessage total-length error, read=" << h1.length
						<< ", expected range ["	<< Message_H2_Offset << ","
						<< size_limit << "]";
				throw MessageUnmarshlingException(oss.str(), event::Message_Refused_Parse_Error);
			}
		}
		catch (MessageUnmarshlingException& mue)
		{
			throw;
		}
		catch (SpiderCastLogicError& le)
		{
			String what("Failed to read H1-header, ");
			what.append(le.what());
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what("Failed to read H1-header, ");
			what.append(re.what());
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else if (_commEventInfo)
	{
		if (_commEventInfo->getContext() == DHTProxyCtx || _commEventInfo->getContext() == DHTServerCtx)
		{
			h1.group = getMessageGroupFromType(Type_DHT_Comm_Event);
			h1.type = Type_DHT_Comm_Event;
		}
		else if (_commEventInfo->getContext() != -1)
		{
			h1.group = getMessageGroupFromType(Type_Topo_Comm_Event);
			h1.type = Type_Topo_Comm_Event;
		}
		else
		{
			h1.group = getMessageGroupFromType(Type_General_Comm_Event);
			h1.type = Type_General_Comm_Event;
		}
	}
	else
	{
		throw MessageUnmarshlingException("null buffer", event::Component_Failure);
	}

	return h1;
}

void SCMessage::writeH1Header(MessageType type, int32_t totalLength, uint16_t usedVersion)
		throw (MessageMarshlingException)
{
	if (usedVersion > config::SPIDERCAST_VERSION)
	{
		std::ostringstream oss;
		oss << "usedVersion=" <<usedVersion << " cannot be bigger than supportedVersion=" << config::SPIDERCAST_VERSION;
		throw MessageMarshlingException(oss.str());
	}

	if (_buffer && (*_buffer).isValid())
	{
		try
		{
			if (type == Type_Topo_Comm_Event)
			{
				std::ostringstream oss;
				oss << "Illegal type=" << messageTypeName[(int) type];
				throw MessageMarshlingException(oss.str());
			}

			(*_buffer).setPosition(Message_Prefix_Offset);
			(*_buffer).writeShort(config::SPIDERCAST_VERSION); //Supported
			(*_buffer).writeShort(usedVersion); //Used
			int typeVal = (int) type;
			int groupVal = (int) getMessageGroupFromType(type);
			typeVal = typeVal | (groupVal << Message_Group_Shift_Bits);
			(*_buffer).writeShort((uint16_t) typeVal);
			if (totalLength > 0)
			{
				(*_buffer).writeInt(totalLength);
			}
			else
			{
				(*_buffer).setPosition(Message_H2_Offset);
			}
		}
		catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to write H1-header, ");
			throw MessageMarshlingException(what);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to write H1-header, ");
			throw MessageMarshlingException(what);
		}
	}
	else
	{
		throw MessageMarshlingException("null buffer");
	}
}

SCMessage::H2Header SCMessage::readH2Header() throw (MessageUnmarshlingException)
{
	H2Header h2(RoutingProto_None, 0, 0);

	if (_buffer && (*_buffer).isValid())
	{
		try
		{
			(*_buffer).setPosition(Message_H2_Offset);

			int32_t h2bits = (int32_t)_buffer->readInt();
			try
			{
				int rp = (h2bits >> 24) & 0x000000FF;
				h2.get<0>() = SCMessage::createMessageRoutingProtocolFromInt(rp);
			}
			catch (std::range_error& e)
			{
				throw MessageUnmarshlingException(e.what(), event::Message_Refused_Parse_Error);
			}

			h2.get<1>() = (uint8_t)((h2bits >> 16) & 0x000000FF); //flags
			h2.get<2>() = (uint8_t)(h2bits & 0x000000FF); //ttl

		}
		catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to read H2-header, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to read H2-header, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else
	{
		throw MessageUnmarshlingException("Failed to read H2-header, null buffer", event::Component_Failure);
	}

	return h2;
}

void SCMessage::writeH2Header(SCMessage::MessageRoutingProtocol rp,
		uint8_t flags, uint8_t ttl) throw (MessageMarshlingException)
{
	if (_buffer && (*_buffer).isValid())
	{
		int32_t h2 = ((int32_t)rp) << 24;
		h2 = h2 | (0x00FF0000 & ((int32_t)flags << 16)) | (0x000000FF & ((int32_t)ttl));

		try
		{
			(*_buffer).setPosition(Message_H2_Offset);
			(*_buffer).writeInt(h2);
		}
		catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to write H2-header, ");
			throw MessageMarshlingException(what);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to write H2-header, ");
			throw MessageMarshlingException(what);
		}
	}
	else
	{
		throw MessageMarshlingException("Failed to write H2-header, null buffer");
	}
}

SCMessage::H3HeaderStart SCMessage::readH3HeaderStart()
		throw (MessageUnmarshlingException)
{

	H3HeaderStart h3(TransProto_None, ReliabilityMode_None);

	if (_buffer && (*_buffer).isValid())
	{
		try
		{
			(*_buffer).setPosition(Message_H3_Offset);

			uint16_t h3bits = (uint16_t)_buffer->readShort();

			//std::cout << "H3-read: " << std::hex << h3bits << std::endl;
			try
			{
				h3.get<0>() = SCMessage::createMessageTransProtocolFromInt((int)( (h3bits>>8) & 0x00FF));
				h3.get<1>() = SCMessage::createMessageReliabilityModeFromInt((int)(h3bits & 0x00FF));
			}
			catch (std::range_error& e)
			{
				throw MessageUnmarshlingException(e.what(), event::Message_Refused_Parse_Error);
			}
		}
		catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to read H3-header, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to read H3-header, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else
	{
		throw MessageUnmarshlingException("Failed to read H3-header, null buffer", event::Component_Failure);
	}

	return h3;
}

void SCMessage::writeH3HeaderStart(SCMessage::MessageTransProtocol tp,
		SCMessage::MessageReliabilityMode rm) throw (MessageMarshlingException)
{

	if (_buffer && (*_buffer).isValid())
	{
		int16_t h3 = (int16_t)((tp<<8) & 0x0000FF00) | (rm & 0x000000FF);

		try
		{
			(*_buffer).setPosition(Message_H3_Offset);
			(*_buffer).writeShort(h3);
		}
		catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to write H3-header, ");
			throw MessageMarshlingException(what);
		}
		catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to write H3-header, ");
			throw MessageMarshlingException(what);
		}
	}
	else
	{
		throw MessageMarshlingException("Failed to write H3-header, null buffer");
	}
}

//===

NodeIDImpl_SPtr SCMessage::readNodeID() throw (MessageUnmarshlingException)
{

	if (_buffer && (*_buffer).isValid())
	{
		//TODO try catch IndexOutOfBounds
		try
		{
			//			StringSPtr name = (*_buffer).readStringSPtr();
			//			uint8_t num_address = (uint8_t) (*_buffer).readChar();
			//			std::vector<std::pair<String, String> > addresses;
			//			for (uint8_t i = 0; i < num_address; ++i)
			//			{
			//				StringSPtr address = (*_buffer).readStringSPtr();
			//				StringSPtr scope = (*_buffer).readStringSPtr();
			//				addresses.push_back(std::make_pair(*address, *scope));
			//			}
			//			uint16_t port = (*_buffer).readShort();
			//			NetworkEndpoints ne(addresses, port);
			//
			//			return NodeIDImpl_SPtr (new NodeIDImpl(*name, ne));
			return _buffer->readNodeID();
		} catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to read NodeID, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		} catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to read NodeID, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else
	{
		throw MessageUnmarshlingException("null buffer", event::Component_Failure);
	}
}

NodeVersion SCMessage::readNodeVersion() throw (MessageUnmarshlingException)
{
	if (_buffer && (*_buffer).isValid())
	{
		//TODO try catch IndexOutOfBounds
		try
		{
			//			int64_t incarnationNum = (*_buffer).readLong();
			//			int64_t minorVer = (*_buffer).readLong();
			//			return NodeVersion(incarnationNum, minorVer);
			return _buffer->readNodeVersion();
		} catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to read NodeVersion, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		} catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to read NodeVersion, ");
			throw MessageUnmarshlingException(what, event::Message_Refused_Parse_Error);
		}
	}
	else
	{
		throw MessageUnmarshlingException("null buffer", event::Component_Failure);
	}
}

void SCMessage::writeNodeID(const NodeIDImpl_SPtr & nodeID)
		throw (MessageMarshlingException)
{
	if (_buffer && (*_buffer).isValid())
	{
		try
		{
			//			(*_buffer).writeString(nodeID->getNodeName());
			//			size_t num_address = nodeID->getNetworkEndpoints().getNumAddresses();
			//			if (num_address > 255)
			//			{
			//				throw MessageMarshlingException(
			//						"Number of addresses exceeds 255, violates wire-format");
			//			}
			//			(*_buffer).writeChar((uint8_t) num_address);
			//
			//			vector<pair<string, string> >::const_iterator it =
			//					nodeID->getNetworkEndpoints().getAddresses().begin();
			//			vector<pair<string, string> >::const_iterator it_end =
			//					nodeID->getNetworkEndpoints().getAddresses().end();
			//			while (it != it_end)
			//			{
			//				(*_buffer).writeString(it->first);
			//				(*_buffer).writeString(it->second);
			//				++it;
			//			}
			//			(*_buffer).writeShort(nodeID->getNetworkEndpoints().getPort());
			_buffer->writeNodeID(nodeID);
		} catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to write NodeID, ");
			throw MessageMarshlingException(what);
		} catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to write NodeID, ");
			throw MessageMarshlingException(what);
		}
	}
	else
	{
		throw MessageMarshlingException("null buffer");
	}
}

void SCMessage::writeNodeVersion(const NodeVersion& version)
		throw (MessageMarshlingException)
{
	if (_buffer && (*_buffer).isValid())
	{
		try
		{
			//			(*_buffer).writeLong(version.getIncarnationNumber());
			//			(*_buffer).writeLong(version.getMinorVersion());
			_buffer->writeNodeVersion(version);
		} catch (SpiderCastLogicError& le)
		{
			String what(le.what());
			what.insert(0, "Failed to write NodeID, ");
			throw MessageMarshlingException(what);
		} catch (SpiderCastRuntimeError& re)
		{
			String what(re.what());
			what.insert(0, "Failed to write NodeID, ");
			throw MessageMarshlingException(what);
		}
	}
	else
	{
		throw MessageMarshlingException("null buffer");
	}
}

SCMembershipEvent SCMessage::readSCMembershipEvent()
		throw (MessageUnmarshlingException)
{
	int type = (int) _buffer->readChar();

	switch (type)
	{
	case SCMembershipEvent::View_Change:
	case SCMembershipEvent::Change_of_Metadata:
	{
		SCViewMap_SPtr view;
		int32_t view_size = _buffer->readInt();
		if (view_size > 0)
		{
			view = SCViewMap_SPtr(new SCViewMap);

			for (int32_t i = 0; i < view_size; ++i)
			{
				NodeIDImpl_SPtr id_impl = _buffer->readNodeID();
				event::MetaData_SPtr meta = readMetaData();
				std::pair<SCViewMap::iterator, bool> res = view->insert(
						std::make_pair(boost::static_pointer_cast<NodeID>(
								id_impl), meta));
				if (!res.second)
				{
					std::ostringstream what;
					what << "Read of SCViewMap failed, duplicate node, "
							<< NodeIDImpl::stringValueOf(id_impl);
					throw MessageUnmarshlingException(what.str(), event::Message_Refused_Parse_Error);
				}
			}
		}

		if (type == SCMembershipEvent::View_Change)
			return SCMembershipEvent(SCMembershipEvent::View_Change, view);
		else
			return SCMembershipEvent(SCMembershipEvent::Change_of_Metadata,
					view);
	}

	case SCMembershipEvent::Node_Join:
	{
		NodeID_SPtr id = boost::static_pointer_cast<NodeID>(
				_buffer->readNodeID());
		event::MetaData_SPtr meta = readMetaData();
		return SCMembershipEvent(SCMembershipEvent::Node_Join, id, meta);
	}

	case SCMembershipEvent::Node_Leave:
	{
		NodeID_SPtr id = boost::static_pointer_cast<NodeID>(
				_buffer->readNodeID());
		return SCMembershipEvent(SCMembershipEvent::Node_Leave, id);
	}

	default:
	{
		std::ostringstream what;
		what << "Unexpected event type: " << type;
		throw MessageMarshlingException(what.str());
	}
	}//switch
}

void SCMessage::writeSCMembershipEvent(const SCMembershipEvent& event,
		bool includeAttributes) throw (MessageMarshlingException)
{
	_buffer->writeChar((char) event.getType());

	switch (event.getType())
	{
	case SCMembershipEvent::View_Change:
	case SCMembershipEvent::Change_of_Metadata:
	{
		SCViewMap_SPtr view_sptr = event.getView();
		if (view_sptr)
		{
			_buffer->writeInt((int32_t) view_sptr->size());
			SCViewMap::const_iterator it;
			for (it = view_sptr->begin(); it != view_sptr->end(); ++it)
			{
				_buffer->writeNodeID(boost::static_pointer_cast<NodeIDImpl>(
						it->first));
				writeMetaData(it->second, includeAttributes);
			}
		}
		else
		{
			_buffer->writeInt(0);
		}

		break;
	}

	case SCMembershipEvent::Node_Join:
	{
		_buffer->writeNodeID(boost::static_pointer_cast<NodeIDImpl>(
				event.getNodeID()));
		writeMetaData(event.getMetaData(), includeAttributes);
		break;
	}

	case SCMembershipEvent::Node_Leave:
	{
		_buffer->writeNodeID(boost::static_pointer_cast<NodeIDImpl>(
				event.getNodeID()));
		break;
	}

	default:
	{
		std::ostringstream what;
		what << "Unexpected event type: " << event.getType();
		throw MessageMarshlingException(what.str());
	}
	}//switch
}

void SCMessage::writeMetaData(const event::MetaData_SPtr& meta,
		bool includeAttributes) throw (MessageMarshlingException)
{
	if (meta)
	{
		_buffer->writeLong(meta->getIncarnationNumber());
		event::AttributeMap_SPtr map = meta->getAttributeMap();
		if (map && includeAttributes)
		{
			_buffer->writeInt((int32_t) map->size());
			event::AttributeMap::const_iterator map_it;
			for (map_it = map->begin(); map_it != map->end(); ++map_it)
			{
				_buffer->writeString(map_it->first);
				_buffer->writeInt(map_it->second.getLength());
				if (map_it->second.getLength() > 0)
				{
					_buffer->writeByteArray(map_it->second.getBuffer().get(),
							map_it->second.getLength());
				}
			}
		}
		else
		{
			_buffer->writeInt(0);
		}
	}
	else
	{
		throw MessageMarshlingException("Null MetaData");
	}
}

event::MetaData_SPtr SCMessage::readMetaData()
		throw (MessageUnmarshlingException)
{
	int64_t inc = _buffer->readLong();
	event::AttributeMap_SPtr map;
	int32_t map_size = _buffer->readInt();
	if (map_size > 0)
	{
		map.reset(new event::AttributeMap);
		for (int i = 0; i < map_size; ++i)
		{
			String key = _buffer->readString();
			int32_t length = _buffer->readInt();
			char* copy = NULL;
			if (length > 0)
			{
				copy = new char[length];
				_buffer->readByteArray(copy, length);
			}
			event::AttributeValue value(length, copy);

			std::pair<event::AttributeMap::iterator, bool> res = map->insert(
					std::make_pair(key, value));
			if (!res.second)
			{
				std::ostringstream what;
				what << "Read of AttributeMap failed, duplicate key, " << key;
				throw MessageUnmarshlingException(what.str(), event::Message_Refused_Parse_Error);
			}
		}
	}

	return event::MetaData_SPtr(new event::MetaData(map, inc, spdr::event::STATUS_ALIVE)); //TODO read node status
}

//==

void SCMessage::writeVirtualID(const util::VirtualID& vid)
		throw (MessageMarshlingException)
{
	_buffer->writeVirtualID(vid);
}

util::VirtualID_SPtr SCMessage::readVirtualID()
		throw (MessageUnmarshlingException)
{
	return _buffer->readVirtualID_SPtr();
}

//==

void SCMessage::updateTotalLength() throw (MessageMarshlingException)
{
	if (_buffer && (*_buffer).isValid())
	{
		//TODO try catch IndexOutOfBounds

		int pos = (*_buffer).getPosition();
		(*_buffer).setPosition(Message_TotalLength_Offset);
		(*_buffer).writeInt(pos);
		(*_buffer).setPosition(pos);
	}
	else
	{
		throw MessageMarshlingException("null buffer");
	}

}

/*
 * Calculate a CRC checksum on [0,position), and
 * write checksum at current position.
 */
void SCMessage::writeCRCchecksum() throw (MessageMarshlingException)
{
	if (_buffer && _buffer->isValid())
	{
		uint32_t crc = _buffer->getCRCchecksum();
		_buffer->writeInt(crc);
	}
	else
	{
		throw MessageMarshlingException("null buffer");
	}
}

/*
 * Read the total-length from the total-length-offset,
 * read the CRC checksum from there, and compare it to the
 * CRC checksum computed over [0,total-length).
 *
 * @throw MessageUnmarshlingException if comparison fails.
 */
void SCMessage::verifyCRCchecksum() throw (MessageUnmarshlingException)
{
	size_t pos = _buffer->getPosition();
	H1Header h1 = readH1Header();
	_buffer->setPosition(h1.get<2> ()); //total-length
	uint32_t crc1 = _buffer->getCRCchecksum(4);
	uint32_t crc2 = _buffer->readInt();

	if (crc1 != crc2)
	{
		std::ostringstream what;
		what << "MessageUnmarshlingException: CRC verification failed, in-msg:"
				<< std::dec << crc2 << ", calculated:" << crc1 << std::endl;
		what << _buffer->toString() << std::endl;
		throw MessageUnmarshlingException(what.str(), event::Component_Failure);
	}

	_buffer->setPosition(pos);
}

String SCMessage::toString() const
{
	if (_buffer && _buffer->isValid())
	{
		try
		{
			size_t last_pos = _buffer->getPosition();

			_buffer->setPosition(Message_Prefix_Offset);
			uint16_t suppVer = _buffer->readShort();
			uint16_t usedVer = _buffer->readShort();
			if (suppVer < usedVer)
			{
				std::ostringstream oss;
				oss << "Corrupt: message supported-version < used-version, supp="
						<< suppVer << ", used=" << usedVer;
				_buffer->setPosition(last_pos);
				return oss.str();
			}

			//Only for the base version
			if (usedVer != config::SPIDERCAST_VERSION)
			{
				std::ostringstream oss;
				oss << "message used-version different than local version, used="
						<< usedVer << ", expected=" << config::SPIDERCAST_VERSION;
				_buffer->setPosition(last_pos);
				return oss.str();
			}

			uint16_t type = _buffer->readShort();
			type = type & Message_Group_Mask;
			_buffer->setPosition(last_pos);
			return String(messageTypeName[(int) type]);
		}
		catch (std::exception& e)
		{
			return String("Corrupt: exception while converting to string: ") + e.what();
		}
	}
	else if (_commEventInfo)
	{
		if (_commEventInfo->getContext() == -1)
		{
			return String(messageTypeName[Type_General_Comm_Event]);
		}
		else if (_commEventInfo->getContext() == DHTProxyCtx || _commEventInfo->getContext() == DHTServerCtx)
		{
			return String(messageTypeName[Type_DHT_Comm_Event]);
		}
		else
		{
			return String(messageTypeName[Type_Topo_Comm_Event]);
		}
	}
	else
	{
		return String("null buffer");
	}


}

}//namespace spdr
