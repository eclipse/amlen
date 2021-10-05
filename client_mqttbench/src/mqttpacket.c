/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

#include <stdio.h>
#include <string.h>
#include <ismutil.h>
#include <protoex.h>

#include "mqttbench.h"
#include "tcpclient.h"
#include "mqttpacket.h"

/* ******************************** GLOBAL VARIABLES ********************************** */
extern mqtt_prop_ctx_t *g_mqttCtx5;

const char *g_ReasonStrings[] = {
		[MQTTRC_OK] 					= "the operation completed successfully OR Granted QoS 0 on SUBACK",
		[MQTTRC_QoS1]                   = "granted QoS 1 on subscription on SUBACK",
		[MQTTRC_QoS2]                   = "granted QoS 2 on subscription on SUBACK",
		[MQTTRC_KeepWill]				= "the connection closed normally, and SEND the Will Message",
		[MQTTRC_NoSubscription]			= "the published message was accepted, but there are no subscribers",
		[MQTTRC_UnspecifiedError] 		= "Unspecified error",
		[MQTTRC_MalformedPacket]		= "the received packet does not conform to the protocol specification",
		[MQTTRC_ProtocolError]			= "an unexpected or out of order packet was received",
		[MQTTRC_ImplError]				= "the packet received is valid, but an implementation specific error occurred",
		[MQTTRC_UnknownVersion]			= "the protocol version specified is unknown or not supported",
		[MQTTRC_IdentifierNotValid]		= "the client ID is a valid string, but is not allowed by the server",
		[MQTTRC_BadUserPassword]		= "the server does not accept the user name or password specified by this client",
		[MQTTRC_NotAuthorized]			= "this client is not authorized",
		[MQTTRC_ServerUnavailable]		= "the server is not available",
		[MQTTRC_ServerBusy]				= "the server is busy and cannot continue processing requests from this client, try again later",
		[MQTTRC_ServerBanned]			= "this client has been banned by administrative action, contact the server administrator",
		[MQTTRC_ServerShutdown]			= "the connection was closed because the server is shutting down",
		[MQTTRC_BadAuthMethod]			= "the authentication method is not supported or does not match the auth method currently in use",
		[MQTTRC_KeepAliveTimeout]		= "the connection was closed because no packet has been received for 1.5 times the keepalive time",
		[MQTTRC_SessionTakenOver]		= "another connection using the same client ID has connected, causing this connection to be closed",
		[MQTTRC_TopicFilterInvalid]		= "the topic filter is correctly formed, but is not accepted by this server",
		[MQTTRC_TopicInvalid]			= "the topic name is correctly formed, but is not accepted",
		[MQTTRC_PacketIDInUse]			= "the packet identifier/message ID is already in use",
		[MQTTRC_PacketIDNotFound]		= "the packet identifier/message ID is not valid",
		[MQTTRC_ReceiveMaxExceeded]		= "the client QoS > 0 received messages which exceed the max inflight receive window",
		[MQTTRC_TopicAliasInvalid]		= "the client received and PUBLISH message containing an invalid topic alias",
		[MQTTRC_PacketTooLarge]			= "the client received packet size is greater than max packet size",
		[MQTTRC_MsgRateTooHigh]			= "the client received message rate is too high",
		[MQTTRC_QuotaExceeded]			= "the implementation or administrative imposed limit has been exceeded",
		[MQTTRC_AdminAction]			= "the client connection closed due to an admin action",
		[MQTTRC_PayloadInvalid] 		= "the payload format does not match the payload format indicator",
		[MQTTRC_RetainNotSupported]		= "the server does not support retained messages",
		[MQTTRC_QoSNotSupported]		= "the server does not support the requested QoS",
		[MQTTRC_UseAnotherServer]		= "the client should temporarily reconnect to an alternate server",
		[MQTTRC_ServerMoved]			= "the server is moved and the client should permanently change its server location",
		[MQTTRC_SharedNotSupported]		= "the server does not support shared subscriptions",
		[MQTTRC_ConnectRate]			= "the server closed this connection because the connect rate is too high",
		[MQTTRC_MaxConnectTime]			= "the max connection time authorized for this connection has been exceeded",
		[MQTTRC_SubIDNotSupported]		= "the server does not support subscription IDs",
		[MQTTRC_WildcardNotSupported]   = "the server does not support wildcard subscriptions"
};

/* *************************************************************************************
 * encodeMQTTMessageLength
 *
 * Description:  Encodes the message length according to the MQTT algorithm
 *
 *   @param[in]  buf                 = the buffer into which the encoded data is written
 *   @param[in]  length              = the length to be encoded
 *
 *   @return x   = The number of bytes written to the buffer
 * *************************************************************************************/
static inline int encodeMQTTMessageLength (char *buf, int length)
{
	int rc = 0;

	do {
		char d = length % 128;
		/* Divide by 128 which is equivalent to shifting by 7 to the right. */
		length = length >> 7;

		/* if there are more digits to encode, set the top bit of this digit */
		if (length > 0)
			d |= 0x80;

		buf[rc++] = d;
	} while (length > 0);

	return rc;
} /* encodeMQTTMessageLength */

/* *******************************************************************************************************
 * Compute the length of the Variable Integer field based on the input length
 *
 *   @param[in]  len                 = length of the buffer to calculate the variable integer length from
 *
 *   @return x  = the length of the variable integer field
 * *******************************************************************************************************/
static inline int getVarIntLen(int len) {
	int vilen = 0;
	if (UNLIKELY(len > 268435455 || len < 0)) {
		return vilen;
	}

	if(len <= 127) {
		vilen = 1;
	} else if(len <= 16383) {
		vilen = 2;
	} else if(len <= 2097151) {
		vilen = 3;
	} else {
		vilen = 4;
	}

	return vilen;
}

/* *************************************************************************************
 * Information needed pertaining to WebSockets framing
 *
 *  0                   1                   2                   3                   4
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
 * +-+-+-+-+-------+-+-------------+-------------------------------------------------+
 * |F|R|R|R| Opcode|M|   Payload   |                                                |
 * |I|S|S|S|       |A|   Length    |                                                |
 * |N|V|V|V|       |S|             |                                                |
 * | |1|2|3|       |K|             |                                                |
 * +-+-+-+-+-------+-+-------------+------------------------------------------------+
 *
 * FIN (1 bit) = Indicates whether this is the final fragment in a message.   The 1st
 *               fragment MAY also be the final fragment.
 * RSV1, RSV2, RSV3 (1 bit) - Reserved and must be 0
 * Opcode (4 bits) - Defines the interpretation of the "Payload Data"
 *                   x0 - Continuation Frame
 *                   x1 - Text Frame
 *                   x2 - Binary Frame (This is for current but if using -M with
 *                                      JSON then we will use x1).
 *                   x8 - Connection Close
 *                   x9 - Ping
 *                   xA - Pong
 * MASK (1 bit) - Whether the "Payload Data" is masked.
 * Payload Length (7 bits) -  If Payload Data length:
 *                             < 126 :: the 7 bits contain the length of payload
 *                            == 126 :: the next 2 bytes contain the length (unsigned integer)
 *                                      7 bits = 0x7E
 *                             > 126 :: the next 8 bytes contain the length (unsigned integer)
 *                                      7 bits = 0x7F
 * *************************************************************************************/
static inline void addWebSocketFrame (ism_byte_buffer_t *bfr, int message_len)
{
	WSFrame wsframe;
	char * pWSFrame;
	unsigned char wsFrameMask[4] = {0};

	/* --------------------------------------------------------------------------------
	 * The following wsframe.all = WS_FRAME_ALL_INITIALIZE is equivalent to:
	 *    wsframe.all=0
	 *    wsframe.bits.final=1
	 *    wsframe.bits.opCode=2
	 *    wsframe.bits.mask=1.
	 * -------------------------------------------------------------------------------- */
	wsframe.all = WS_FRAME_ALL_INITIALIZE;

	/* If the length is less than 126 then the 7 bits will contain the length. */
	if (message_len < 126) {  /* less than 126 -> only 2 bytes needed */
		wsframe.bits.len = message_len;
		bfr->getPtr += 8;
		pWSFrame = bfr->getPtr;
		WRITEINT16(pWSFrame, wsframe.all);
		bfr->used += MIN_WS_FRAME_LEN;
	} else if (message_len < WS_PAYLOAD_64K) {
		/* Less than 64K (max number to fit in 2 bytes; only 4 bytes needed for frame. */
		wsframe.bits.len = WS_PAYLOAD_MASK_LESS_64K;
		bfr->getPtr += 6;
		pWSFrame = bfr->getPtr;
		WRITEINT16(pWSFrame, wsframe.all);
		WRITEINT16(pWSFrame, message_len);
		bfr->used += WS_FRAME_LEN_126;
	} else { /* >64K Bytes so the next 8 bytes contain the length. */
		wsframe.bits.len = WS_PAYLOAD_MASK_MORE_64K;
		pWSFrame = bfr->getPtr;
		WRITEINT16(pWSFrame, wsframe.all);
		NputLongX(pWSFrame, (uint64_t)message_len);
		bfr->used += MAX_WS_FRAME_LEN;
	}

	WRITEUTF(pWSFrame, wsFrameMask, sizeof(wsFrameMask));
} /* addWebSocketFrame */

/* *************************************************************************************
 * createMQTTMessageAck
 *
 * Description:  Create an MQTT ACK message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  type                = Type of message (e.g. PUBREL)
 *   @param[in]  msgid               = Message ID
 *   @param[in]  qos                 = Quality of Service
 *   @param[in]  useWebSockets       = Use Web Sockets
 * *************************************************************************************/
void createMQTTMessageAck (ism_byte_buffer_t *txBuf,
		                   uint8_t type,
		                   uint8_t qos,
		                   uint16_t msgId,
                           uint8_t useWebSockets)
{
	int numBytes;
	char *ptr = txBuf->putPtr + (useWebSockets * MIN_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	Header header;

/* TODO:  BWB - need to determine why the assertion for getPtr != putPtr .....
 *              is it do to the fact that we are trying to respond to all the
 *              ack's at once? */

//	assert(txBuf->getPtr == txBuf->putPtr);

	/* Setup the header information for the MQTT PUBACK message. */
	header.byte = 0;
	header.bits.type = type;
	header.bits.qos = qos;

	/* Write the Header into the tx buffer */
	WRITECHAR(ptr, header.byte);

	/* Encode the remaining length and put into the tx Buffer */
	numBytes = encodeMQTTMessageLength(ptr, MQTT_MSGID_SIZE);

	/* --------------------------------------------------------------------------------
	 * Set the pointer to the byte after the encoded length and then write the msgid
	 * into the tx buffer.
	 * -------------------------------------------------------------------------------- */
	ptr += numBytes;
	WRITEINT16(ptr, msgId);

	/* Update the length of the data used in the buffer. */
	numBytes = (int)(ptr - framePtr);
	txBuf->used += numBytes;

	/* Set the put pointer in the buffer so that multiple ACK messages can be sent. */
	txBuf->putPtr = ptr;

	/* If using WebSockets need to put the WebSocket Frame Header prior to the mqtt message. */
	if (useWebSockets) {
		WSFrame wsframe;
		unsigned char wsFrameMask[4] = {0};

		/* ----------------------------------------------------------------------------
		 * Clear the frame, set Final=1, OpCode=1, and Mask=1.  Set the length field
		 * equal to the length of the MQTT Message created with numBytes.
		 * ---------------------------------------------------------------------------- */
		wsframe.all = WS_FRAME_ALL_INITIALIZE;
		wsframe.bits.len = numBytes;

		/* Set the framePtr to the beginning of the buffer to write the WS Frame */
		framePtr -= MIN_WS_FRAME_LEN;
		WRITEINT16(framePtr, wsframe.all);     /* Write WS Frame to buffer */
		WRITEUTF(framePtr, wsFrameMask, sizeof(wsFrameMask));
		txBuf->used += MIN_WS_FRAME_LEN;       /* Update the buffer bytes used. */
	}
} /* createMQTTMessageAck */

/* *************************************************************************************
 * createMQTTMessageConnect
 *
 * Description:  Create an MQTT CONNECT message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  client              = A structure from which to get all the required values
 *   @param[in]  mqtt_Version        = MQTT Version
 * *************************************************************************************/
void createMQTTMessageConnect (ism_byte_buffer_t *txBuf, mqttclient_t *client, uint8_t mqtt_Version)
{
	int len = 0;
	int remainingLength = 0;
	int sizeOfConnectMsg = 0;

	char *ptr = txBuf->putPtr + (client->useWebSockets * MAX_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	/**************************************************************
	 * CONNECT - Fixed header
	 *
	 * Remaining Length = Variable header length + CONNECT Payload
	 **************************************************************/
	Header header;
	Connect packet;
	header.byte = 0;
	header.bits.type = CONNECT;
	WRITECHAR(ptr, header.byte);

	/* Calculate remaining length */
	if (mqtt_Version >= MQTT_V5) {
		/* Variable Header length */
		len += MQTT_CONNECT_V5_VAR_HDR_LEN;

		/* For MQTT v5 clients, add the length of MQTT properties, if configured */
		if (client->propsBuf) {
			len += getVarIntLen(client->propsBuf->used) + client->propsBuf->used;
		} else {
			len++; // 1 byte for 0 MQTT property length
		}
	} else if (mqtt_Version == MQTT_V311) {
		len += MQTT_CONNECT_V311_VAR_HDR_LEN;
	} else if (mqtt_Version == MQTT_V3) {
		len += MQTT_CONNECT_V31_VAR_HDR_LEN;
	}

	/* Calculate length of CONNECT Payload */
	len += MQTT_STRING_LEN + (int)client->clientIDLen;

	/* Calculate length of Will Message, if configured */
	int willTopicLen = 0, willMsgLen = 0, willPropsLen = 0;
	if (client->willMsg) {
		willTopicLen = client->willMsg->topicLen;
		willMsgLen = client->willMsg->payloadLen;
		if (mqtt_Version >= MQTT_V5) {
			if (client->willMsg->propsBuf) {
				willPropsLen = getVarIntLen(client->willMsg->propsBuf->used) + client->willMsg->propsBuf->used;
			} else {
				willPropsLen = 1; // 1 byte for 0 MQTT property length in will message
			}
		}

		len += willPropsLen + MQTT_STRING_LEN + willTopicLen + MQTT_STRING_LEN + willMsgLen;
	}

	if ((client->username) && (client->usernameLen > 0))
		len += MQTT_STRING_LEN + client->usernameLen;

	if ((client->password) && (client->passwordLen > 0))
		len += MQTT_STRING_LEN + client->passwordLen;

	/* --------------------------------------------------------------------------------
	 * Encode the remaining length and put into the tx Buffer and then increment the
	 * ptr to the byte after the remaining length variable integer field.
	 * -------------------------------------------------------------------------------- */
	remainingLength = encodeMQTTMessageLength(ptr, len);
	ptr += remainingLength;

	/**************************************************************
	 * CONNECT - Variable header
	 **************************************************************/
	if (mqtt_Version >= MQTT_V5) {
		WRITEINT16(ptr, MQTT_PROT_NAME_MQTT5_LEN);
		WRITEUTF(ptr, MQTT_PROT_NAME_MQTT5, MQTT_PROT_NAME_MQTT5_LEN);
		WRITECHAR(ptr, MQTT_V5);
	} else if (mqtt_Version == MQTT_V311) {
		WRITEINT16(ptr, MQTT_PROT_NAME_MQTT311_LEN);
		WRITEUTF(ptr, MQTT_PROT_NAME_MQTT311, MQTT_PROT_NAME_MQTT311_LEN);
		WRITECHAR(ptr, MQTT_V311);
	} else if (mqtt_Version == MQTT_V3) {
		WRITEINT16(ptr, MQTT_PROT_NAME_MQTT31_LEN);
		WRITEUTF(ptr, MQTT_PROT_NAME_MQTT31, MQTT_PROT_NAME_MQTT31_LEN);
		WRITECHAR(ptr, MQTT_V3);
	}

	packet.flags.all = 0;
	packet.flags.bits.cleanstart = (mqtt_Version >= MQTT_V5) ? client->cleanStart : client->cleansession;
	packet.flags.bits.will = (client->willMsg) ? 1 : 0;
	if (packet.flags.bits.will) {
		packet.flags.bits.willQoS = client->willMsg->qos;
		packet.flags.bits.willRetain = client->willMsg->retained;
	}
	if (client->username && client->usernameLen > 0)
		packet.flags.bits.username = 1;
	if (client->password && client->passwordLen > 0)
		packet.flags.bits.password = 1;

	WRITECHAR(ptr, packet.flags.all);
	WRITEINT16(ptr, client->keepAliveInterval);

	/**************************************************************
	 * CONNECT - MQTT Properties (v5 only)
	 **************************************************************/
	if (mqtt_Version >= MQTT_V5) {
		if (client->propsBuf) {
			int vilen = encodeMQTTMessageLength(ptr, client->propsBuf->used);
			ptr += vilen; 												// move past the variable integer length field
			memcpy(ptr, client->propsBuf->buf, client->propsBuf->used); // copy the MQTT properties in to the CONNECT message
			ptr += client->propsBuf->used;
		} else {
			WRITECHAR(ptr, 0); 											// MQTT property length = 0. i.e. no MQTT properties
		}
	}

	/**************************************************************
	 * CONNECT - Payload
	 **************************************************************/
	WRITEINT16(ptr, client->clientIDLen);
	WRITEUTF(ptr, client->clientID, client->clientIDLen);

	if (client->willMsg) {
		if(mqtt_Version >= MQTT_V5){
			if (client->willMsg->propsBuf) {
				int vilen = encodeMQTTMessageLength(ptr, client->willMsg->propsBuf->used);
				ptr += vilen; 																// move past the variable integer length field
				memcpy(ptr, client->willMsg->propsBuf->buf, client->willMsg->propsBuf->used); 	// copy the Will Message MQTT properties in to the CONNECT message
				ptr += client->willMsg->propsBuf->used;
			} else {
				WRITECHAR(ptr, 0); 															// MQTT property length = 0. i.e. no MQTT properties
			}
		}
		WRITEINT16(ptr, willTopicLen);
		WRITEUTF(ptr, client->willMsg->topic, willTopicLen);
		WRITEINT16(ptr, willMsgLen);
		WRITEUTF(ptr, client->willMsg->payload, willMsgLen);
	}

	if (client->username && client->usernameLen > 0) {
		WRITEINT16(ptr, client->usernameLen);
		WRITEUTF(ptr, client->username, client->usernameLen);
	}

	if (client->password && client->passwordLen > 0) {
		WRITEINT16(ptr, client->passwordLen);
		WRITEUTF(ptr, client->password, client->passwordLen);
	}

	/* Update the length of the data used in the buffer. */
	sizeOfConnectMsg = (int)(ptr - framePtr);
	txBuf->used = sizeOfConnectMsg;
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (client->useWebSockets)
		addWebSocketFrame (txBuf, sizeOfConnectMsg);

	if(txBuf->putPtr > (txBuf->buf + txBuf->allocated)) {
		MBTRACE(MBERROR, 5, "Internal Error: CONNECT packet for client %s (line=%d) wrote %d bytes past the end of the TX buffer\n",
							client->clientID, client->line, (int)((txBuf->putPtr - txBuf->buf) - txBuf->allocated));
	}
} /* createMQTTMessageConnect */

/* *************************************************************************************
 * createMQTTMessageDisconnect
 *
 * Description:  Create an MQTT DISCONNECT message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  useWebSockets       = Use Web Sockets
 *
 *   @return 0   = Successful completion
 * *************************************************************************************/
void createMQTTMessageDisconnect (ism_byte_buffer_t *txBuf, mqttclient_t *client, int useWebSockets)
{
	Header header;
	char *ptr = txBuf->putPtr + (useWebSockets * MAX_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	char xbuf[2048];        /* Buffer used for holding properties. */
	concat_alloc_t propsBfr = {xbuf, sizeof xbuf};

	/* Setup the header information for the MQTT DISCONNECT message*/
	header.byte = 0;
	header.bits.type = DISCONNECT;

	WRITECHAR(ptr, header.byte);

	/* Calculate remaining length for the variable header (for v5 or later clients)*/
	int remainingLength = 0;

	uint8_t disconnectRC = client->disconnectRC;
	client->disconnectRC = 0;  // reset the disconnectRC

	if(client->mqttVersion >= MQTT_V5) {
		int vilen = 0;
		int propsLen = 0;
		remainingLength += 1;  // add 1 byte (1 - reason code)

		if(!MQTT_RC_CHECK(disconnectRC)) { // Perform range check on RC before getting reason string
			char reasonStr[REASON_STR_LEN] = {0};
			const char *reason = g_ReasonStrings[disconnectRC];
			if(reason) {
				strncpy(reasonStr, reason, REASON_STR_LEN - 1);  // -1 ensure null terminated string
			}

			int reasonStrLen = strlen(reasonStr);
			if(reasonStrLen) {  // If disconnectRC and reason string are valid, update the remainingLength to include length of reason string property
				ism_common_putMqttPropString(&propsBfr, MPI_Reason, g_mqttCtx5, reasonStr, reasonStrLen);

				propsLen = propsBfr.used;
				int propsVarIntLen = getVarIntLen(propsBfr.used);
				remainingLength += propsBfr.used + propsVarIntLen;
			}
		}

		vilen = encodeMQTTMessageLength(ptr, remainingLength);  /* write the remaining length varint into the variable header */
		ptr += vilen;

		WRITECHAR(ptr, disconnectRC);

		vilen = encodeMQTTMessageLength(ptr, propsLen);         /* write the properties len varint */
		ptr += vilen;

		if (propsLen) {
			memcpy(ptr, propsBfr.buf, propsLen);  				/* Write properties to the ptr */
			ptr += propsLen;
		}
	} else { /* < v5 clients*/
		WRITECHAR(ptr, remainingLength);
	}

	/* Update the length of the data used in the buffer. */
	int sizeOfDisconnectMsg = (int)(ptr - framePtr);
	txBuf->used = sizeOfDisconnectMsg;

	/* Set the put pointer in the buffer so that multiple ACK messages can be sent. */
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (client->useWebSockets)
		addWebSocketFrame(txBuf, sizeOfDisconnectMsg);

	if(txBuf->putPtr > (txBuf->buf + txBuf->allocated)) {
		MBTRACE(MBERROR, 5, "Internal Error: DISCONNECT packet for client %s (line=%d) wrote %d bytes past the end of the TX buffer\n",
							client->clientID, client->line, (int)((txBuf->putPtr - txBuf->buf) - txBuf->allocated));
	}
} /* createMQTTMessageDisconnect */

/* *************************************************************************************
 * createMQTTMessagePublish
 *
 * Description:  Create an MQTT PUBLISH message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  dest                = The destination tuple
 *   @param[in]  msgObj              = Message Object
 *   @param[in]  msg                 = Structure used to contain info on the message being sent.
 *   @param[in]  parm_clkSrc         = The clock source to use
 *   @param[in]  useWebSockets       = Use Web Sockets
 *   @param[in]  mqtt_Version        = The client's MQTT Version
 * *************************************************************************************/
void createMQTTMessagePublish (ism_byte_buffer_t *txBuf,
		                       destTuple_t *pDest,
							   message_object_t *msgObj,
		                       mqttmessage_t *msg,
		                       concat_alloc_t *propsBfr,
							   uint8_t parm_ClkSrc,
		                       uint8_t useWebSockets,
							   uint8_t mqtt_Version)
{
	int numBytes;
	int topicLen;
	int remainingLength = 0;
	int propsVarIntLen = 0;

	char *msgBody = NULL;
	char *ptr = txBuf->putPtr + (useWebSockets * MAX_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	Header header;
	uli sendTimestamp;

	uint64_t msgCount = 0;
	switch (msg->qos) {
	case 0:
		msgCount = ++pDest->currQoS0MsgCount;
		break;
	case 1:
		msgCount = ++pDest->currQoS1MsgCount;
		break;
	case 2:
		msgCount = ++pDest->currQoS2MsgCount;
		break;
	}

	/*---------------------------------------------------------------------------------
	 * The following assert (if used) checks to see if the putPtr != getPtr.  If they
	 * are NOT EQUAL (!=) then there are multiple messages in the buffer which could be
	 * an issue for WebSockets.
	 * -------------------------------------------------------------------------------- */
#ifdef _CHECK_BFR_PTRS
	assert(txBuf->getPtr == txBuf->putPtr);
#endif /* _CHECK_BFR_PTRS */

	/* Setup the MQTT PUBLISH header. */
	header.byte = 0;
	header.bits.type = PUBLISH;
	header.bits.qos = msg->qos;
	header.bits.retain = msg->retain;
	header.bits.dup = msg->dupflag;

	/* Put the header in the tx buffer */
	WRITECHAR(ptr, header.byte);

	/* --------------------------------------------------------------------------------
	 * The PUBLISH variable Header contains the following:
	 *
	 *    a) Topic Name (UTF Encoded) plus the 2 byte prefix with the length
	 *    b) Message ID (only present on QoS 1 or 2
	 *    c) Properties (only for MQTT V5 and higher)
	 * -------------------------------------------------------------------------------- */

	/* Calculate the remaining length for the variable header. */

	/* Get the length of the topic */
	topicLen = msg->sendTopicName ? pDest->topicNameLength : 0;

	/* Need to determine the length of the variable header and payload for a publish. */
	remainingLength = MQTT_TOPIC_NAME_HDR_LEN + topicLen + msg->payloadlen;

	/* If the QoS is 1 or 2 then include a message id. */
	if (msg->qos)
		remainingLength += MQTT_MSGID_SIZE;

	/* --------------------------------------------------------------------------------
	 * Unique for MQTT V5:
	 *    1) If latency testing is being performed then need to add a user property for
	 *       the timestamp.
	 *    2) If using sequence number then need to add a user property for it.
	 *    3) If using properties, add the length of MQTT properties.
	 * -------------------------------------------------------------------------------- */
	if (mqtt_Version >= MQTT_V5) {
		/* Check for Latency Sampling and add User Property lengths if specified */
		if (msg->marked == 1) {
			int tStampLen = 0;
			char tStampAscii[UINT64_ASCIISTRING_LEN ] = {0};

			/* ------------------------------------------------------------------------
			 * Get the current timestamp to be used for the send time when determining
			 * latency.  The 2 clock functions return the value as a double.  In order
			 * to send in V5 it must be converted to a UTF String which is accomplished
			 * by calling ism_common_dtoa( ).
			 * ------------------------------------------------------------------------ */
			sendTimestamp.d = (double)(parm_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

			ism_common_dtoa(sendTimestamp.d, tStampAscii);
			tStampLen = strlen(tStampAscii);

			ism_common_putMqttPropNamePair(propsBfr,
											MPI_UserProperty,
											g_mqttCtx5,
											UPROP_TIMESTAMP,
											UPROP_TIMESTAMP_LEN,
											tStampAscii,
											tStampLen);
		}

		/* ----------------------------------------------------------------------------
		 * Check if using Sequence Number on topics and if so then need to add User
		 * Properties for both Sequence Number and Stream ID.
		 * ---------------------------------------------------------------------------- */
		if (msg->useseqnum) {
			char seqNumHex[UINT64_HEXSTRING_LEN] = {0};
			uint64_t seqNum = msgCount;

			/* Convert sequence number to Hex to pass UTF-8 checking on Server */
			ism_common_ultox(seqNum, 1, seqNumHex);
			int seqNumLen = strlen(seqNumHex);     /* Obtain the seqNum length after conversion */

			ism_common_putMqttPropNamePair(propsBfr, MPI_UserProperty, g_mqttCtx5, UPROP_SEQNUM, UPROP_SEQNUM_LEN, seqNumHex, seqNumLen);

			/* Add StreamID to propsBfr */
			ism_common_putMqttPropNamePair(propsBfr, MPI_UserProperty, g_mqttCtx5, UPROP_STREAMID, UPROP_STREAMID_LEN, pDest->streamIDStr, pDest->streamIDStrLen);
		}

		/* Set the MQTT v5 topic Alias property if a topic alias has been set for this topic */
		if(msg->sendTopicAlias) {
			ism_common_putMqttPropField(propsBfr, MPI_TopicAlias, g_mqttCtx5, pDest->topicAlias);
		}

		/* Add any other message properties and the destTuple properties. */
		if (msgObj->propsBuf)
		    ism_common_allocBufferCopyLen(propsBfr, msgObj->propsBuf->buf, msgObj->propsBuf->used);

		if (pDest->propsBuf)
		    ism_common_allocBufferCopyLen(propsBfr, pDest->propsBuf->buf, pDest->propsBuf->used);

		/* ----------------------------------------------------------------------------
		 * Check to see if there were any user properties.   If so, then add propsLen
		 * to the total length, else increment total length by 1 (no user properties)
		 * ---------------------------------------------------------------------------- */
		if (propsBfr->used > 0) {
			propsVarIntLen = getVarIntLen(propsBfr->used);
			remainingLength +=  propsVarIntLen + propsBfr->used;
		} else
			remainingLength++;
	}

	/* --------------------------------------------------------------------------------
	 * Encode the remaining length and put into the tx Buffer and then increment the
	 * ptr to the byte after the remaining length.
	 * -------------------------------------------------------------------------------- */
	numBytes = encodeMQTTMessageLength(ptr, remainingLength);
	ptr += numBytes;

	WRITEINT16(ptr, topicLen);
	if(topicLen) {
		WRITEUTF(ptr, pDest->topicName, topicLen);
	}
	if (msg->qos)
		WRITEINT16(ptr, msg->msgid);

	/* --------------------------------------------------------------------------------
	 * MQTT V5
	 * --------------------------
	 * If both or one of the User Properties (Timestamp or Sequence Numbering) is
	 * specified then place the User Properties specified in the message.
	 *
	 * MQTT V3.1 and V3.1.1
	 * --------------------------
 	 * Check to see if the message was marked for latency measurement.  If so, place
	 * the timestamp in the message payload.
	 * -------------------------------------------------------------------------------- */
	if (mqtt_Version >= MQTT_V5) {
		if (propsVarIntLen > 0) {
			numBytes = encodeMQTTMessageLength(ptr, propsBfr->used);
			ptr += numBytes;

			/* copy the propsBfr into the txBuf and update ptr. */
			memcpy(ptr, propsBfr->buf, propsBfr->used);
			ptr += propsBfr->used;
		} else
			WRITECHAR(ptr, 0);
	} else { /* MQTT Vers 3.1 and 3.1.1 */
		if (msg->marked == 1) {
			msg->marked = 0;
			msgBody = msg->payload + 1;
			sendTimestamp.d = (parm_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
			NputLong(msgBody, sendTimestamp);
		}

		/* if sequence number checking enabled and a generated message then insert stream ID and seqnum in the payload */
		if (msg->useseqnum && msg->generatedMsg) {
			uli streamID, seqNum;
			/* Put streamID and Sequence number in payload from bytes 9 to 25. */
			msgBody = msg->payload + 9;
			streamID.l = pDest->streamID;
			NputLong(msgBody, streamID);
			seqNum.l = msgCount;
			NputLong(msgBody, seqNum);
		}
	}

	/* Copy the payload to the txBuf */
	WRITEUTF(ptr, msg->payload, msg->payloadlen);

	/* Update the length of the data used in the buffer. */
	numBytes = (int)(ptr - framePtr);

	txBuf->used = numBytes;

	/* Set the put pointer in the buffer */
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (useWebSockets)
		addWebSocketFrame(txBuf, numBytes);
} /* createMQTTMessagePublish */

/* *************************************************************************************
 * createMQTTMessageSubscribe
 *
 * Description:  Create an MQTT SUBSCRIBE message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  topic               = TopicName
 *   @param[in]  msg                 = Structure used to contain info on the message being sent.
 *   @param[in]  useWebSockets       = Use Web Sockets
 *   @param[in]  mqtt_Version        = The client's mqtt version being used.
 * *************************************************************************************/
void createMQTTMessageSubscribe (ism_byte_buffer_t *txBuf,
		                         destTuple_t *pDest,
		                         mqttmessage_t *msg,
		                         int useWebSockets,
								 uint8_t mqtt_Version)
{
	int numBytes;
	int propsLen = 0;
	int remainingLength;
	char *ptr = txBuf->putPtr + (useWebSockets * MAX_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

    char xbuf[2048];                                /* Buffer used for holding properties. */
    concat_alloc_t propsBfr = {xbuf, sizeof xbuf};

	Header header;
	SubOption subOption;

	/* Setup the header information for the MQTT SUBSCRIBE message. */
	header.byte = 0;
	header.bits.type = SUBSCRIBE;
	header.bits.qos = 1;    /* Required for SUBSCRIBE msgs to get a SUBACK */

	/* Put the header in the tx buffer */
	WRITECHAR(ptr, header.byte);

	/* --------------------------------------------------------------------------------
	 * The SUBSCRIBE variable Header contains the following:
	 *   Message ID - 2 bytes
	 *   Payload    - List of topics:
	 *                  2 bytes - length of topic
	 *                  Topic Name (UTF Encoded)
	 *                  1 byte which contains the QoS
	 * -------------------------------------------------------------------------------- */
	/* Need to determine the length of the variable header and payload for a subscribe. */
	remainingLength = MQTT_MSGID_SIZE + MQTT_TOPIC_NAME_HDR_LEN + pDest->topicNameLength + MQTT_SUBOPT_SIZE;

	/* If MQTT V5 then need to determine the properties Length. */
	if (mqtt_Version >= MQTT_V5) {
		ism_common_putMqttPropField(&propsBfr, MPI_SubID, g_mqttCtx5, pDest->subId);

		propsLen = propsBfr.used;
		int propsVarIntLen = getVarIntLen(propsBfr.used);
		remainingLength += propsBfr.used + propsVarIntLen;
	}

	/* --------------------------------------------------------------------------------
	 * Encode the remaining length and put into the tx Buffer and then increment the
	 * ptr to the byte after the remaining length.
	 * -------------------------------------------------------------------------------- */
	numBytes = encodeMQTTMessageLength(ptr, remainingLength);
	ptr += numBytes;

	WRITEINT16(ptr, msg->msgid);

	/* If MQTT V5 then need to set the properties Length. */
	if (mqtt_Version >= MQTT_V5) {
		numBytes = encodeMQTTMessageLength(ptr, propsLen);
		ptr += numBytes;

		/* copy the propsBfr into the txBuf and update ptr. */
		memcpy(ptr, propsBfr.buf, propsBfr.used);
		ptr += propsBfr.used;
	}

	WRITEINT16(ptr, pDest->topicNameLength);
	WRITEUTF(ptr, pDest->topicName, pDest->topicNameLength);

	/* --------------------------------------------------------------------------------
	 * If MQTT V5 then need to set the SubScribe Options byte.   The details are as
	 * follows:
	 *
	 *   bits
	 *   ------
	 *   0 & 1 : QoS of Topic Subscription
	 *   2     : No Local Option (must be set to '0' if Shared Subscription)
	 *   3     : Retain as Published Option
	 *   4 & 5 : Retain Handling Option
	 *   5 & 6 : Reserved
	 * -------------------------------------------------------------------------------* */
	if (mqtt_Version >= MQTT_V5) {
		subOption.byte = 0;                     /* Clear the byte */
		subOption.bits.qos = msg->qos;          /* Set desired qos for topic subsribe */
		subOption.bits.noLocal = pDest->noLocal;
		subOption.bits.retainPub = pDest->retainAsPublished;
		subOption.bits.retainHand = pDest->retainHandling;
		WRITECHAR(ptr, subOption.byte);
	} else /* Ver 3.1.1 */
		WRITECHAR(ptr, msg->qos);

	/* Update the length of the data used in the buffer. */
	numBytes = (int)(ptr - framePtr);
	txBuf->used = numBytes;

	/* Set the put pointer in the buffer so that multiple ACK messages can be sent. */
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (useWebSockets)
		addWebSocketFrame(txBuf, numBytes);
} /* createMQTTMessageSubscribe */

/* *************************************************************************************
 * createMQTTMessageUnSubscribe
 *
 * Description:  Create an MQTT UNSUBSCRIBE message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  pDest               = x
 *   @param[in]  msg                 = Structure used to contain info on the message being sent.
 *   @param[in]  useWebSockets       = Use Web Sockets
 * *************************************************************************************/
void createMQTTMessageUnSubscribe (ism_byte_buffer_t *txBuf,
		                           destTuple_t *pDest,
		                           mqttmessage_t *msg,
		                           int useWebSockets,
                                   uint8_t mqtt_Version)
{
	int numBytes = 0;
	int propsLen = 0;
	int remainingLength = 0;
	char *ptr = txBuf->putPtr + (useWebSockets * MAX_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	char xbuf[2048];                                /* Buffer used for holding properties. */
	concat_alloc_t propsBfr = {xbuf, sizeof xbuf};

	Header header;

	/* Setup the header information for the MQTT UNSUBSCRIBE message. */
	header.byte = 0;
	header.bits.type = UNSUBSCRIBE;
	header.bits.qos = 1;    /* Required for UNSUBSCRIBE msgs to get a UNSUBACK */

	/* Put the header in the tx buffer */
	WRITECHAR(ptr, header.byte);

	/* --------------------------------------------------------------------------------
	 * The UNSUBSCRIBE variable Header contains the following:
	 *   Message ID - 2 bytes
	 *   Payload    - List of topics:
	 *                  2 bytes - length of topic
	 *                  Topic Name (UTF Encoded)
	 * -------------------------------------------------------------------------------- */
	/* Need to determine the length of the variable header and payload for a unsubscribe. */
	remainingLength = MQTT_MSGID_SIZE + MQTT_TOPIC_NAME_HDR_LEN + pDest->topicNameLength;

	/* If MQTT V5 then need to determine the properties Length. */
	if (mqtt_Version >= MQTT_V5) {
		propsLen = propsBfr.used;
		int propsVarIntLen = getVarIntLen(propsBfr.used);
		remainingLength += propsBfr.used + propsVarIntLen;
	}

	/* --------------------------------------------------------------------------------
	 * Encode the remaining length and put into the tx Buffer and then increment the
	 * ptr to the byte after the remaining length.
	 * -------------------------------------------------------------------------------- */
	numBytes = encodeMQTTMessageLength(ptr, remainingLength);
	ptr += numBytes;

	WRITEINT16(ptr, msg->msgid);

	/* If MQTT V5 then need to set the properties Length. */
	if (mqtt_Version >= MQTT_V5) {
		numBytes = encodeMQTTMessageLength(ptr, propsLen);
		ptr += numBytes;

		/* copy the propsBfr into the txBuf and update ptr. */
		if (propsBfr.used) {
			memcpy(ptr, propsBfr.buf, propsBfr.used);
			ptr += propsBfr.used;
		}
	}

	WRITEINT16(ptr, pDest->topicNameLength);
	WRITEUTF(ptr, pDest->topicName, pDest->topicNameLength);

	/* Update the length of the data used in the buffer. */
	numBytes = (int)(ptr - framePtr);
	txBuf->used = numBytes;

	/* Set the put pointer in the buffer so that multiple ACK messages can be sent. */
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (useWebSockets)
		addWebSocketFrame(txBuf, numBytes);
} /* createMQTTMessageUnSubscribe */

/* *************************************************************************************
 * createMQTTMessagePingReq
 *
 * Description:  Create an MQTT PINGREQ message in the ByteBuffer provided.
 *
 *   @param[in]  txBuf               = TX Buffer to contain message sent via the I/O thread
 *   @param[in]  useWebSockets       = Use Web Sockets
 * *************************************************************************************/
void createMQTTMessagePingReq (ism_byte_buffer_t *txBuf, int useWebSockets)
{
	int numBytes = 0;
	char *ptr = txBuf->putPtr + (useWebSockets * MIN_WS_FRAME_LEN);
	char *framePtr = ptr;   /* Put the frame pointer at the beginning of MQTT Data */

	Header header;

	/* --------------------------------------------------------------------------------
	 * The following assert (if used) checks to see if the putPtr != getPtr.  If they
	 * are NOT EQUAL (!=) then there are multiple messages in the buffer which could be
	 * an issue for WebSockets.
	 * -------------------------------------------------------------------------------- */
#ifdef _CHECK_BFR_PTRS
	assert(txBuf->getPtr == txBuf->putPtr);
#endif /* _CHECK_BFR_PTRS */

	/* Setup the header information for the MQTT PINGREQ message. */
	header.byte = 0;
	header.bits.type = PINGREQ;

	WRITECHAR(ptr, header.byte);
	WRITECHAR(ptr, 0);

	/* Update the length of the data used in the buffer. */
	numBytes = (int)(ptr - framePtr);
	txBuf->used = numBytes;

	/* Set the put pointer in the buffer so that multiple ACK messages can be sent. */
	txBuf->putPtr = ptr;

	/* If using WebSockets Protocol need to add the WebSocket frame prior to the mqtt Message. */
	if (useWebSockets)
		addWebSocketFrame(txBuf, numBytes);
} /* createMQTTMessagePing */

/*
 * Check the validity of an MQTT Properties area for an incoming message
 *
 * @param[in] bufp    	= the location of the area
 * @param[in] len     	= the length of the area
 * @param[in] ctx     	= the MQTT property context
 * @param[in] flags   	= the MqttOptIDs_e flags for this control packet
 * @param[in] check   	= a callback function to further process the MQTT property
 * @param[in] userdata	= userdata to pass to the callback function
 *
 * @return A return code: 0=good, otherwise an ISMRC return code
 */
int checkMqttPropFields(const char * bufp, int len, mqtt_prop_ctx_t * ctx, int flags, ism_check_ext_f check, void * userdata) {
    char * foundtab;
    int    rc;

    foundtab = alloca(ctx->max_id + 1);
    memset(foundtab, 0, ctx->max_id + 1);

    while (len) {
        mqtt_prop_field_t * fld;
        int datalen;
        int value;
        uint32_t int32;
        int id = (uint8_t)*bufp++;
        len--;
        if (id > ctx->max_id) {
        	MBTRACE(MBERROR, 5, "Invalid MQTT property ID=%u, error=%s\n", id, "BadPropID");
            return MQTTRC_MalformedPacket;
        }

        fld = id<=ctx->array_id ? ctx->fields[id] : ism_common_findMqttPropID(ctx, id);
        if (fld == NULL) {
            MBTRACE(MBERROR, 5, "Invalid MQTT property ID=%u, error=%s\n", id, "UnknownProp");
            return MQTTRC_MalformedPacket;
        }
        if (!(fld->valid&flags)) {
        	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "BadContext", flags);
            return MQTTRC_MalformedPacket;
        }

        if (foundtab[id] && !(fld->valid & CPOI_MULTI)) {
            if (!(flags & CPOI_IS_ALT) || (fld->valid & CPOI_ALT_MULTI) != CPOI_ALT_MULTI) {  /* BEAM suppression: constant condition */
            	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "DupProp", flags);
                return MQTTRC_MalformedPacket;
            }
        }
        foundtab[id] = 1;

        value = 0;
        switch (fld->type) {
        case MPT_Boolean:
            datalen = 0;
            value = 1;
            break;
        case MPT_Int1:
            datalen = 1;
            if (len >= 1)
                value = (uint8_t)*bufp;
            break;
        case MPT_Int2:
            datalen = 2;
            if (len >= 2)
                value = (uint16_t)READINT16(bufp);
            break;
        case MPT_Int4:
            datalen = 4;
            if (len >= 4) {
                memcpy(&int32, bufp, 4);
                value = endian_int32(int32);
            }
            break;
        case MPT_Bytes:
        case MPT_String:
            value = 0;
            if (len<2) {
            	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "BadStringLength", flags);
                return MQTTRC_MalformedPacket;
            }
            datalen = (uint16_t)READINT16(bufp);
            bufp += 2;
            len -= 2;
            break;
        case MPT_NamePair:
            {
                int namelen;
                int vallen;
                /* Check the two inner lengths */
                if (len >= 4)
                    namelen = READINT16(bufp);
                else
                    namelen = 4;
                if (namelen > (len-4)) {
                	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "BadNameLength", flags);
                    return MQTTRC_MalformedPacket;
                }
                vallen = READINT16(bufp+namelen+2);
                if (vallen > (len-namelen-4)) {
                	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "BadValueLength", flags);
                    return MQTTRC_MalformedPacket;
                }
                value = namelen;
                datalen = namelen + vallen + 4;
            }
            break;
        case MPT_VarInt:
            {
                concat_alloc_t vibuf = {(char *)bufp, len, len};
                value = ism_common_getMqttVarInt(&vibuf);
                if (value < 0) {
                	MBTRACE(MBERROR, 5, "Invalid MQTT property name=%s, error=%s, context=%d\n", fld->name, "BadVarIntLength", flags);
                    return MQTTRC_MalformedPacket;
                }
                datalen = vibuf.pos;
            }
            break;
        default:
        	MBTRACE(MBERROR, 5, "Invalid MQTT property ID=%u, error=%s, type=%d\n", id, "BadType", fld->type);
            return MQTTRC_MalformedPacket;
        }
        if (len < datalen) {
        	MBTRACE(MBERROR, 5, "Invalid MQTT property ID=%u, error=%s\n", id, "BadLength");
            return MQTTRC_MalformedPacket;
        }
        if (check) {
            rc = check(ctx, userdata, fld, bufp, datalen, value);
            if (rc)
                return rc;
        }
        len -= datalen;
        bufp += datalen;
    }
    return 0;
}
