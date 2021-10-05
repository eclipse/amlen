/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.mqtt;

/*
 * Define MQTT Reason Codes (CSD20)
 */
public class MqttRC {

    /*
     * MQTT return codes
     */
    public static  final int OK                   = 0x00;
    public static  final int QoS0                 = 0x00;  /*   0 QoS=0 return on SUBSCRIBE */
    public static  final int QoS1                 = 0x01;  /*   1 QoS=1 return on SUBSCRIBE */
    public static  final int QoS2                 = 0x02;  /*   2 QoS=2 return on SUBSCRIBE */
    public static  final int KeepWill             = 0x04;  /*   4 Normal close of connection but keep the will */
    public static  final int NoSubscription       = 0x10;  /*  16 There was no matching subscription on a PUBLISH */
    public static  final int NoSubExisted         = 0x11;  /*  17 No subscription existed on UNSUBSCRIBE */
    public static  final int ContinueAuth         = 0x18;  /*  24 Continue authentication */
    public static  final int Reauthenticate       = 0x19;  /*  25 Start a re-authentication */
    public static  final int UnspecifiedError     = 0x80;  /* 128 Unspecified error */
    public static  final int MalformedPacket      = 0x81;  /* 129 The packet is malformed */
    public static  final int ProtocolError        = 0x82;  /* 130 Protocol error */
    public static  final int ImplError            = 0x83;  /* 131 Implementation specific error */
    public static  final int UnknownVersion       = 0x84;  /* 132 Unknown MQTT version */
    public static  final int IdentifierNotValid   = 0x85;  /* 133 ClientID not valid */
    public static  final int BadUserPassword      = 0x86;  /* 134 Username or Password not valid */
    public static  final int NotAuthorized        = 0x87;  /* 135 Not authorized */
    public static  final int ServerUnavailable    = 0x88;  /* 136 The server in not available */
    public static  final int ServerBusy           = 0x89;  /* 137 The server is busy */
    public static  final int ServerBanned         = 0x8A;  /* 138 The server is banned from connecting */
    public static  final int ServerShutdown       = 0x8B;  /* 139 The server is being shut down */
    public static  final int BadAuthMethod        = 0x8C;  /* 140 The authentication method is not valid */
    public static  final int KeepAliveTimeout     = 0x8D;  /* 141 The Keep Alive time has been exceeded */
    public static  final int SessionTakenOver     = 0x8E;  /* 142 The session has been taken over */
    public static  final int TopicFilterInvalid   = 0x8F;  /* 143 The topic filter is not valid for this server */
    public static  final int TopicInvalid         = 0x90;  /* 144 The topic is not valid for this server */
    public static  final int PacketIDInUse        = 0x91;  /* 145 The PacketID is in use */
    public static  final int PacketIDNotFound     = 0x92;  /* 146 The PacketID is not found */
    public static  final int ReceiveMaxExceeded   = 0x93;  /* 147 The Receive Maximum value is exceeded */
    public static  final int TopicAliasInvalid    = 0x94;  /* 148 The topic alias is not valid */
    public static  final int PacketTooLarge       = 0x95;  /* 149 The packet is too large */
    public static  final int MsgRateTooHigh       = 0x96;  /* 150 The message rate is too high */
    public static  final int QuotaExceeded        = 0x97;  /* 151 A client quota is exceeded */
    public static  final int AdminAction          = 0x98;  /* 152 The connection is closed due to an administrative action */
    public static  final int PayloadInvalid       = 0x99;  /* 153 The payload format is invalid */
    public static  final int RetainNotSupported   = 0x9A;  /* 154 Retain not supported */
    public static  final int QoSNotSupported      = 0x9B;  /* 155 The QoS is not supported */
    public static  final int UseAnotherServer     = 0x9C;  /* 156 Temporary use another server */
    public static  final int ServerMoved          = 0x9D;  /* 157 Permanent use another server */
    public static  final int SharedNotSupported   = 0x9E;  /* 158 Shared subscriptions are not allowed */
    public static  final int ConnectRate          = 0x9F;  /* 159 Connection rate exceeded */
    public static  final int MaxConnectTime       = 0xA0;  /* 160 The maximum connect time is exceeded */
    public static  final int SubIDNotSupported    = 0xA1;  /* 161 Subscription IDs are not supportd */
    public static  final int WildcardNotSupported = 0xA2;  /* 162 Wildcard subscriptions are not supported */

    /*
     * Return the name of the RC
     */
    public static  String toString(int rc) {
        /*
         * MQTT return codes
         */
        String ret = "Unknown";
        switch (rc) {
        case OK                 : /* 0x00   0 */  ret = "OK";  break;
        case KeepWill           : /* 0x04   4 */  ret = "OK but keep the Will Message";   break;
        case NoSubscription     : /* 0x10  16 */  ret = "There was no matching subscription on a PUBLISH"; break;
        case NoSubExisted       : /* 0x11  17 */  ret = "No subscription existed on UNSUBSCRIBE"; break;
        case ContinueAuth       : /* 0x18  24 */  ret = "Continue authentication"; break;
        case Reauthenticate     : /* 0x19  25 */  ret = "Start a re-authentication"; break;
        case UnspecifiedError   : /* 0x80 128 */  ret = "Unspecified error"; break;
        case MalformedPacket    : /* 0x81 129 */  ret = "The packet is malformed"; break;
        case ProtocolError      : /* 0x82 130 */  ret = "Protocol error"; break;
        case ImplError          : /* 0x83 131 */  ret = "Implementation specific error"; break;
        case UnknownVersion     : /* 0x84 132 */  ret = "Unknown MQTT version"; break;
        case IdentifierNotValid : /* 0x85 133 */  ret = "ClientID not valid"; break;
        case BadUserPassword    : /* 0x86 134 */  ret = "Username or Password not valid"; break;
        case NotAuthorized      : /* 0x87 135 */  ret = "Not authorized"; break;
        case ServerUnavailable  : /* 0x88 136 */  ret = "The server in not available"; break;
        case ServerBusy         : /* 0x89 137 */  ret = "The server is busy"; break;
        case ServerBanned       : /* 0x8A 138 */  ret = "The server is banned from connecting"; break;
        case ServerShutdown     : /* 0x8B 139 */  ret = "The server is being shut down"; break;
        case BadAuthMethod      : /* 0x8C 140 */  ret = "The authentication method is not valid"; break;
        case KeepAliveTimeout   : /* 0x8D 141 */  ret = "The Keep Alive time has been exceeded"; break;
        case SessionTakenOver   : /* 0x8E 142 */  ret = "The session has been taken over"; break;
        case TopicFilterInvalid : /* 0x8F 143 */  ret = "The topic filter is not valid for this server"; break;
        case TopicInvalid       : /* 0x90 144 */  ret = "The topic is not valid for this server"; break;
        case PacketIDInUse      : /* 0x91 145 */  ret = "The PacketID is in use"; break;
        case PacketIDNotFound   : /* 0x92 146 */  ret = "The PacketID is not found"; break;
        case ReceiveMaxExceeded : /* 0x93 147 */  ret = "The Receive Maximum value is exceeded"; break;
        case TopicAliasInvalid  : /* 0x94 148 */  ret = "The topic alias is not valid"; break;
        case PacketTooLarge     : /* 0x95 149 */  ret = "The packet is too large"; break;
        case MsgRateTooHigh     : /* 0x96 150 */  ret = "The message rate is too high"; break;
        case QuotaExceeded      : /* 0x97 151 */  ret = "A cliemt quota is exceeded"; break;
        case AdminAction        : /* 0x98 152 */  ret = "The connection is closed due to an administrative action"; break;
        case PayloadInvalid     : /* 0x99 153 */  ret = "The payload format is invalid"; break;
        case RetainNotSupported : /* 0x9A 154 */  ret = "Retain not supported"; break;
        case QoSNotSupported    : /* 0x9B 155 */  ret = "The QoS is not supported"; break;
        case UseAnotherServer   : /* 0x9C 156 */  ret = "Temporary use another server"; break;
        case ServerMoved        : /* 0x9D 157 */  ret = "Permanent use another server"; break;
        case SharedNotSupported : /* 0x9E 158 */  ret = "Shared subscriptions are not allowed"; break;
        case ConnectRate        : /* 0x9F 159 */  ret = "Connection rate exceeded"; break;
        case MaxConnectTime     : /* 0xA0 160 */  ret = "The maximum connect time is exceeded"; break;
        case SubIDNotSupported  : /* 0xA1 161 */  ret = "Subscription IDs are not supportd"; break;
        case WildcardNotSupported:/* 0xA2 162 */  ret = "Wildcard subscriptions are not supported"; break;
        }
        return ret;
    }
    
    /*
     * Return the name of the CONNACK RC for versions before 5 
     */
    public static int connackToMqttRC(int rc) {
        switch (rc) {
        case 0:   return OK;  
        case 1:   return UnknownVersion;
        case 2:   return IdentifierNotValid;
        case 3:   return ServerUnavailable;
        case 4:   return BadUserPassword;
        case 5:   return NotAuthorized;
        default:  return UnspecifiedError;
        }  
    }    
}
