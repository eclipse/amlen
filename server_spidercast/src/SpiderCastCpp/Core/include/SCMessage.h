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
 * SCMessage.h
 *
 *  Created on: 15/03/2010
 */

#ifndef SCMESSAGE_H_
#define SCMESSAGE_H_

#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/cstdint.hpp>

#include "SpiderCastConfig.h"
#include "NodeIDImpl.h"
#include "BusName.h"
#include "ByteBuffer.h"
#include "CommEventInfo.h"
#include "NodeVersion.h"
#include "SCMembershipEvent.h"
#include "VirtualID.h"
//#include "ConcurrentSharedPtr.h"
#include "SpiderCastRuntimeError.h"
#include "RxMessageImpl.h"

namespace spdr
{
class SCMessage;
typedef boost::shared_ptr<SCMessage> 	SCMessage_SPtr;
typedef uint64_t streamId_t;

//===================================================================

/**
 * TODO make copy semantics
 *

 *
 */
class SCMessage
{
public:

	//=== H1 - Prefix, Type, Total-Length ===================================
	static const uint16_t 	Message_Prefix = config::SPIDERCAST_VERSION; //0xABE2; //TODO replace with supported version
	static const uint16_t 	Message_Prefix_Pad = config::SPIDERCAST_VERSION; //TODO replace with used version
	static const int 		Message_Prefix_Offset = 0;
	static const int 		Message_Prefix_Size = 4;

	static const int 		Message_Type_Offset = Message_Prefix_Offset + Message_Prefix_Size;
	static const int 		Message_Type_Size = 2;

	static const int 		Message_TotalLength_Offset = Message_Type_Offset + Message_Type_Size;
	static const int 		Message_TotalLength_Size = 4;

	//=== H2 - Multi-hop ====================================================
	// H2 - RoutingProtocol (1B), Flags (1B), Reserved (1B), TTL (1B)
	static const int 		Message_H2_Offset = Message_TotalLength_Offset + Message_TotalLength_Size;

	static const uint8_t	Message_H2_Flags_Default = 0;
	static const uint8_t	Message_H2_Flags_LCL_Mask = 0x01; //local-publisher, no need to route
	static const uint8_t	Message_H2_Flags_BDR_Mask = 0x02; //bi-directional routing
	static const uint8_t	Message_H2_Flags_CCW_Mask = 0x04; //counter-clock-wise routing
	static const uint8_t	Message_H2_Flags_GLB_Mask = 0x08; //global routing


	static const uint8_t	Message_H2_TTL_Default = 64;

	static const int 		Message_H2_Size = 4;

	//=== H2_One2Many
	// VID (20B), topic-hash (4B)
	static const int 		Message_H2_O2M_Offset = Message_H2_Offset + Message_H2_Size;
	static const int 		Message_H2_O2M_TID_Offset = Message_H2_O2M_Offset + 20;
	static const int 		Message_H2_O2M_Size = 24;

	//=== H2_One2One
	// VID (20B), Reserved (4B)
	static const int 		Message_H2_O2O_Offset = Message_H2_Offset + Message_H2_Size;
	static const int 		Message_H2_O2O_RSV_Offset = Message_H2_O2O_Offset + 20;
	static const int 		Message_H2_O2O_Size = 24;

	//=== H3 - Transport =================================================
	//Transport protocol (1B), Reliability Mode (1B),
	//StreamID (2x8B), Message number (8B),Timeout interval ms (4B),
	//Target (node/topic) name (String),
	//Source name (String) TODO change to NodeID
	//TODO add source bus
	static const int 		Message_H3_Offset = Message_H2_O2M_Offset + Message_H2_O2M_Size;
	static const int 		Message_H3_TrnsProto_Size = 1;
	static const int 		Message_H3_RelMode_Size = 1;
	static const int 		Message_H3_StreamID_Size = 16;
	static const int 		Message_H3_MessageNumber_Offset = Message_H3_Offset
								+ Message_H3_TrnsProto_Size + Message_H3_RelMode_Size + Message_H3_StreamID_Size;
	static const int 		Message_H3_MessageNumber_Size = 8;
	static const int 		Message_H3_TO_MS_Size = 4;
	static const int 		Message_H3_Target_Offset = Message_H3_MessageNumber_Offset
								+ Message_H3_MessageNumber_Size + Message_H3_TO_MS_Size;

	//=== Message Type ==============================================

	/**
	 * The bit shift of the message group field.
	 * This is the number of bits allocated to the MessageType.
	 * The number of bits allocated to the MessageGroup is
	 * 		(Message_Type_Size*8-Message_Group_Shift_Bits);
	 * The wire format is (16 bits)
	 * 		(MessageGroup << Message_Group_Shift_Bits) | MessageType
	 */
	static const int Message_Group_Shift_Bits = 8;
	static const int Message_Group_Mask = 0x00FF;

	/**
	 * Message group.
	 *
	 * When editing this enum remember to update the messageGroupName array.
	 */
	enum MessageGroup
	{
		Group_None 			= 0,
		Group_Membership,
		Group_Topology,
		Group_Hierarchy,
		Group_DHT,
		Group_GeneralCommEvents,
		Group_Data,
		Group_Transport,
		Group_Max
	};

	static const char* const messageGroupName[];

	/**
	 * When editing this enum remember to:
	 * 1) watch the range-guard, Type_Max <=255 (or update the message group shift, etc)
	 * 2) update the messageType2Group and messageTypeName arrays.
	 */
	enum MessageType
	{
		/** No buffer */
		Type_None = 0,

		//=== Membership ===================

		/** node membership update message */
		Type_Mem_Node_Update,

		/** node membership leave message */
		Type_Mem_Node_Leave,

		/** node membership leave acknowledge message */
		Type_Mem_Node_Leave_Ack,

		/** meta-data dissemination update message */
		Type_Mem_Metadata_Update,

		/** meta-data dissemination request message */
		Type_Mem_Metadata_Request,

		/** meta-data dissemination request message */
		Type_Mem_Metadata_RequestPush,

		/** meta-data dissemination reply message */
		Type_Mem_Metadata_Reply,

		/** for HPM */
		Type_Mem_Node_Update_UDP, //TODO SplitBrain


		//=== Topology =====================
		Type_Topo_Discovery_Request_TCP,

		Type_Topo_Discovery_Reply_TCP,

		Type_Topo_Discovery_Request_UDP, //TODO SplitBrain

		Type_Topo_Discovery_Reply_UDP, //TODO SplitBrain

		Type_Topo_Discovery_Request_Multicast, //TODO SplitBrain

		Type_Topo_Discovery_Reply_Multicast,  //TODO SplitBrain

		Type_Topo_Connect_Successor,

		Type_Topo_Connect_Successor_OK,

		Type_Topo_Connect_Random_Request,

		Type_Topo_Connect_Random_Reply,

		Type_Topo_Connect_Structured_Request,

		Type_Topo_Connect_Structured_Reply,

		Type_Topo_Disconnect_Request,

		Type_Topo_Disconnect_Reply,

		Type_Topo_Degree_Changed,

		Type_Topo_Node_Leave,

		/** Comm Event (message group is topology) */
		Type_Topo_Comm_Event,

		//=== Hierarchy =====================
		Type_Hier_Connect_Request,

		Type_Hier_Connect_Reply,

		Type_Hier_Disconnect_Request,

		Type_Hier_Disconnect_Reply,

		Type_Hier_Leave,

		Type_Hier_DelOp_Request_StartMembershipPush,

		Type_Hier_DelOp_Reply_StartMembershipPush,

		Type_Hier_DelOp_Request_StopMembershipPush,

		Type_Hier_DelOp_Reply_StopMembershipPush,

		Type_Hier_SupOp_Request_ViewUpdate,

		Type_Hier_SupOp_Reply_ViewUpdate,

		Type_Hier_SupOp_Request_ForeignZoneMembership,

		Type_Hier_SupOp_Reply_ForeignZoneMemberships,

		Type_Hier_SupOp_Request_ZoneCensus,

		Type_Hier_SupOp_Reply_ZoneCensus,

		Type_Hier_PubSubBridge_BaseZoneInterest,

		Type_Hier_PubSubBridge_BaseZoneInterest_Reply,

		//=== DHT ==========================
		Type_DHT_SOMETHING,
		Type_DHT_Comm_Event,

		//=== Group_GeneralCommEvents==============
		Type_General_Comm_Event,

		//=== Data =========================

		//TODO add pub/sub, broadcast, p2p,
		Type_Data_PubSub,
		Type_Data_Direct,
		Type_Data_SOMETHING,

		//=== Transport ====================
		Type_Trans_SOMETHING,

		//etc...

		//=== Range guard ====================
		Type_Max
	};

	static const char* const messageTypeName[];
	static const MessageGroup messageType2Group[];

	/**
	 * Message routing protocol.
	 *
	 * When editing this enum remember to update the messageRoutingProtocolName array.
	 */
	enum MessageRoutingProtocol
	{
		RoutingProto_None = 0,
		RoutingProto_P2P,
		RoutingProto_PubSub,
		RoutingProto_Broadcast,
		RoutingProto_Flooding,
		RoutingProto_Max
	};

	static const char* const messageRoutingProtocolName[];

	/**
	 * Message transport protocol.
	 *
	 * When editing this enum remember to update the messageRoutingProtocolName array.
	 */
	enum MessageTransProtocol
	{
		TransProto_None = 0,
		TransProto_PubSub,
		TransProto_P2P,
		TransProto_Broadcast,
		TransProto_Max
	};

	static const char* const messageTransProtocolName[];

	/**
	 * Message reliability mode.
	 *
	 * When editing this enum remember to update the messageReliabilityModeName array.
	 */
	enum MessageReliabilityMode
	{
		ReliabilityMode_None = 0,
		ReliabilityMode_BestEffort,
		ReliabilityMode_BulletinBoard,
		ReliabilityMode_NackBase,
		ReliabilityMode_AckBased,
		ReliabilityMode_Max
	};

	static const char* const messageReliabilityModeName[];

	//===============================================================

	SCMessage();

	virtual ~SCMessage();

	//=== getters and setters ===

	NodeIDImpl_SPtr getSender() const;

	void setSender(NodeIDImpl_SPtr sender);

	/**
	 * Get the local sender/target name attached to the connection
	 * @return
	 */
	StringSPtr getSenderLocalName() const;

	/**
	 * Set the local sender/target name attached to the connection
	 * @param senderLocalName
	 */
	void setSenderLocalName(const StringSPtr& senderLocalName);

	streamId_t getStreamId() const;

	void setStreamId(streamId_t streamId);

	BusName_SPtr getBusName() const;

	void setBusName(BusName_SPtr busName);

	boost::shared_ptr<ByteBuffer> getBuffer() const;

	void setBuffer(boost::shared_ptr<ByteBuffer> buffer);

	boost::shared_ptr<CommEventInfo> getCommEventInfo() const;

	void setCommEventInfo(boost::shared_ptr<CommEventInfo>  eventInfo);

	/**
	 * Reset internal smart pointers.
	 */
	void reset();

	/**
	 * If the undelying buffer contains a message, or a CommEvent.
	 *
	 * @return
	 */
	bool isValid();

	//=== enum utilities ===
	/**
	 * Get the message group from the message type.
	 *
	 * @param type
	 * @return
	 *
	 * @throw std::range_error if the conversion results in an illegal MessageGroup, or MessageType is invalid.
	 */
	static MessageGroup getMessageGroupFromType(MessageType type);

	/**
	 * Get group name.
	 * @param group
	 * @return
	 * @throw std::range_error if the enum is invalid.
	 */
	static String getMessageGroupName(MessageGroup group);

	/**
	 * Get type name.
	 * @param type
	 * @return
	 * @throw std::range_error if the enum is invalid.
	 */
	static String getMessageTypeName(MessageType type);

	/**
	 * Create group from int.
	 * @param groupVal
	 * @return
	 * @throw std::range_error
	 */
	static MessageGroup createMessageGroupFromInt(int groupVal) throw(std::range_error);

	/**
	 * Create type from int.
	 * @param typeVal
	 * @return
	 * @throw std::range_error
	 */
	static MessageType createMessageTypeFromInt(int typeVal) throw(std::range_error);

	/**
	 * Create routing protocol from int.
	 * @param rpVal
	 * @return
	 * @throw std::range_error
	 */
	static MessageRoutingProtocol createMessageRoutingProtocolFromInt(int rpVal) throw(std::range_error);

	/**
	 * Create transport protocol from int.
	 * @param rpVal
	 * @return
	 * @throw std::range_error
	 */
	static MessageTransProtocol createMessageTransProtocolFromInt(int tpVal) throw(std::range_error);

	/**
	 * Create reliability mode from int.
	 * @param rmVal
	 * @return
	 * @throw std::range_error
	 */
	static MessageReliabilityMode createMessageReliabilityModeFromInt(int rmVal)
					throw (std::range_error);

	//=== H1 utilities ===

	/**
	 * A tuple holding MessageGroup, MessageType and Total-Length.
	 * Total-Length includes all data including headers, but without the CRC.
	 */
	typedef boost::tuples::tuple<SCMessage::MessageGroup, SCMessage::MessageType, int32_t> H1Header; //TODO convert to a struct

	struct H1HeaderVer
	{
		uint16_t supportedVersion;
		uint16_t usedVersion;
		SCMessage::MessageGroup group;
		SCMessage::MessageType type;
		int32_t length;

		H1HeaderVer() : supportedVersion(0), usedVersion(0), group(SCMessage::Group_None), type(SCMessage::Type_None), length(0) {}
	};


	/**
	 * Get the H1-header.
	 * Advances the position index.
	 *
	 * @return a tuple holding MessageGroup, MessageType and Total-Length.
	 * 		Works correctly for CommEvent too.
	 *
	 * @throw MessageUnmarshlingException
	 * 		if the underlying buffer is null,
	 * 		or the prefix doesn't match,
	 *  	or contains data which cannot be converted into a MessageType, MessageGroup, etc.
	 */
	H1Header readH1Header() throw (MessageUnmarshlingException);

	/**
	 * Write prefix, type, and length.
	 *
	 * @param type
	 * 		message type, but not Type_Topo_Comm_Event
	 * @param totalLength
	 * 		total length, if zero, skips it
	 *
	 * @throw MessageMarshlingException
	 */
	void writeH1Header(MessageType type, int32_t totalLength = 0, uint16_t usedVersion = config::SPIDERCAST_VERSION)
			throw (MessageMarshlingException);

	/**
	 * Get the H1-header with versions.
	 * Advances the position index.
	 *
	 * @return a H1HeaderUDP struct holding Supported- and Used-version, MessageGroup, MessageType and Total-Length.
	 * 		Works correctly for CommEvent too.
	 *
	 * @throw MessageUnmarshlingException
	 * 		if the underlying buffer is null,
	 * 		or the prefix doesn't match,
	 *  	or contains data which cannot be converted into a MessageType, MessageGroup, etc.
	 */
	H1HeaderVer readH1HeaderV() throw (MessageUnmarshlingException);

	//=== H2 utilities

	/**
	 * A tuple holding RoutingProtocol, Flags, and TTL.
	 */
	typedef boost::tuples::tuple<SCMessage::MessageRoutingProtocol, uint8_t,
			uint8_t> H2Header;

	H2Header readH2Header() throw (MessageUnmarshlingException);

	void writeH2Header(SCMessage::MessageRoutingProtocol rp, uint8_t flags,
			uint8_t ttl) throw (MessageMarshlingException);

	//=== H3 utilities

	/**
	 * A tuple holding TransportProtocol, ReliabilityMode.
	 */
	typedef boost::tuples::tuple<SCMessage::MessageTransProtocol,
			SCMessage::MessageReliabilityMode> H3HeaderStart;

	H3HeaderStart readH3HeaderStart() throw (MessageUnmarshlingException);

	void writeH3HeaderStart(SCMessage::MessageTransProtocol tp,
			SCMessage::MessageReliabilityMode rm) throw (MessageMarshlingException);


	//===

	/**
	 * Read NodeIDImpl from current location in the buffer
	 * Advances the position index.
	 *
	 * @throw MessageUnmarshlingException
	 * 		if the underlying buffer is null, data mismatches, etc.
	 */
	NodeIDImpl_SPtr readNodeID()
			throw (MessageUnmarshlingException);
	/**
	 * Write NodeIDImpl to current location in the buffer
	 *
	 * @throw MessageMarshlingException
	 */
	void writeNodeID(const NodeIDImpl_SPtr& nodeID) throw (MessageMarshlingException);

	/**
	 * Read NodeVrsion from current location in the buffer
	 * Advances the position index.
	 *
	 *  @throw MessageUnmarshlingException
	 * 		if the underlying buffer is null, data mismatches, etc.
	 */
	NodeVersion readNodeVersion() throw (MessageUnmarshlingException);

	/**
	 * Write NodeVersion to current location in the buffer
	 *
	 * @throw MessageMarshlingException
	 */
	void writeNodeVersion(const NodeVersion& version) throw(MessageMarshlingException);

	/**
	 * Read SCMembershipEvent from current location in the buffer
	 * Advances the position index.
	 *
	 * @return
	 * @throw MessageUnmarshlingException
	 * 		if the underlying buffer is null, data mismatches, etc.
	 */
	SCMembershipEvent readSCMembershipEvent() throw (MessageUnmarshlingException);

	void writeVirtualID(const util::VirtualID& vid) throw(MessageMarshlingException);

	util::VirtualID_SPtr readVirtualID() throw (MessageUnmarshlingException);

	/**
	 * Write SCMembershipEvent to current location in the buffer
	 *
	 * @param event
	 * @param includeAttributes if false, all attribute maps are omitted
	 * @throw MessageMarshlingException
	 */
	void writeSCMembershipEvent(const SCMembershipEvent& event, bool includeAttributes)
		throw (MessageMarshlingException);



	/**
	 * Take the current position, write it in the total length offset,
	 * and set position to original value.
	 *
	 * This method should be called at the end of a message marshaling process.
	 *
	 * @throw MessageMarshlingException
	 */
	void updateTotalLength() throw (MessageMarshlingException);

	/**
	 * Calculate a CRC checksum on [0,position), and
	 * write checksum at current position (32 bit).
	 */
	void writeCRCchecksum() throw (MessageMarshlingException);

	/**
	 * Read the total-length from the total-length-offset,
	 * read the CRC checksum from there, and compare it to the
	 * CRC checksum computed over [0,total-length).
	 *
	 * @throw MessageUnmarshlingException if comparison fails.
	 */
	void verifyCRCchecksum() throw (MessageUnmarshlingException);

	/**
	 * Produces a string that holds message type and size.
	 * Does NOT alter the position index.
	 *
	 * @return
	 */
	String toString() const;

	/**
	 * Get a reference to the attached RxMessage.
	 *
	 * @return
	 */
	inline messaging::RxMessageImpl& getRxMessage()
	{
		return rxMessage_;
	}

private:
	NodeIDImpl_SPtr _sender;

	StringSPtr _senderLocalName;

	BusName_SPtr _busName;

	boost::shared_ptr<ByteBuffer> _buffer;

	streamId_t _streamId;

	boost::shared_ptr<CommEventInfo> _commEventInfo;

	messaging::RxMessageImpl rxMessage_;

	static void verifyMessageGroupRange(int groupVal) throw (std::range_error);

	static void verifyMessageTypeRange(int typeVal) throw (std::range_error);

	void writeMetaData(const event::MetaData_SPtr& meta, bool includeAttributes) throw (MessageMarshlingException);
	event::MetaData_SPtr readMetaData() throw (MessageUnmarshlingException);

};

}

#endif /* SCMESSAGE_H_ */
