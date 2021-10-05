/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.plugin;

/**
 * Define a subset of the IBM MessgeSight server return codes which
 * might be visible to the plug-in.
 */
public class ImaReturnCode {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /** Success                                 */
    public static final int IMARC_OK                        =  0;
    /** The destination is full                 */
    public static final int IMARC_DestinationFull           =  14;
    /** The transaction was rolled back         */
    public static final int IMARC_RolledBack                =  15;
    /** The destination is in use               */
    public static final int IMARC_DestinationInUse          =  18;
    /** The connection was closed by the client */
    public static final int IMARC_ClosedByClient            =  91;
    /** The connection was closed by the server */
    public static final int IMARC_ClosedByServer            =  92;
    /** The connection was closed because the server was shutdown */
    public static final int IMARC_ServerTerminating         =  93;
    /** The connection was closed by an administrator */
    public static final int IMARC_EndpointDisabled          =  94;
    /** The connection was closed during a send */
    public static final int IMARC_ClosedOnSend              =  95;
    /** System error                            */
    public static final int IMARC_Error                     =  100;
    /** The function is not implemented         */
    public static final int IMARC_NotImplemented            =  102;
    /** Unable to allocate memory               */
    public static final int IMARC_AllocateError             =  103;
    /** The server capacity is reached          */
    public static final int IMARC_ServerCapacity            =  104;
    /** The data from the client is not valid   */
    public static final int IMARC_BadClientData             =  105;
    /** The object is closed                    */
    public static final int IMARC_Closed                    =  106;
    /** The object is already destroyed         */
    public static final int IMARC_Destroyed                 =  107;
    /** A null object is not allowed            */
    public static final int IMARC_NullPointer               =  108;
    /** The receive timed out                   */
    public static final int IMARC_TimeOut                   =  109;
    /** The lock could not be acquired          */
    public static final int IMARC_LockNotGranted            =  110;
    /** The property name is not valid: property={0}  */
    public static final int IMARC_BadPropertyName           =  111;
    /** A property value is not valid: property={0} value={1}  */
    public static final int IMARC_BadPropertyValue          =  112;
    /** The requested item is not found         */
    public static final int IMARC_NotFound                  =  113;
    /** The messaging object is not valid       */
    public static final int IMARC_ObjectNotValid            =  114;
    /** An argument not valid: name={0}         */
    public static final int IMARC_ArgNotValid               =  115;
    /** A null argument is not allowed          */
    public static final int IMARC_NullArgument              =  116;
    /** The message is not valid                */
    public static final int IMARC_MessageNotValid           =  117;
    /** The properties are not valid            */
    public static final int IMARC_PropertiesNotValid        =  118;
    /** The client ID is not valid: {0}         */
    public static final int IMARC_ClientIDNotValid          =  119;
    /** The client ID is required               */
    public static final int IMARC_ClientIDRequired          =  120;
    /** The client ID is already in use         */
    public static final int IMARC_ClientIDInUse             =  121;
    /** The Unicode value is not valid          */
    public static final int IMARC_UnicodeNotValid           =  122;
    /** The subscription name is not valid      */
    public static final int IMARC_NameNotValid              =  123;
    /** The destination name is not valid       */
    public static final int IMARC_DestNotValid              =  124;
    /** Connection is not authorized            */
    public static final int IMARC_ConnectNotAuthorized      =  159;
    /** The connection timed out                */
    public static final int IMARC_ConnectTimedOut           =  160;
    /** Failed to ping client                   */
    public static final int IMARC_PingFailed                =  161;
    /** The client did not respond to a ping    */
    public static final int IMARC_NoPingResponse            =  162;
    /** Closed during TLS handshake             */
    public static final int IMARC_ClosedTLSHandshake        =  163;
    /** No data was received on a connection    */
    public static final int IMARC_NoFirstPacket             =  164;
    /** The initial packet is too large         */
    public static final int IMARC_FirstPacketTooBig         =  165;
    /** The ClientID is not valid               */
    public static final int IMARC_BadClientID               =  166;
    /** Server not available                    */
    public static final int IMARC_ServerNotAvailable        =  167;
    /** Messaging is not available              */
    public static final int IMARC_MessagingNotAvailable     =  168;
    /** Certificate missing                     */
    public static final int IMARC_NoCertificate             =  169;
    /** Certificate not valid                   */
    public static final int IMARC_CertificateNotValid       =  170;
    /** HTTP handshake failed                   */
    public static final int IMARC_BadHTTP                   =  171;
    /** The HTTP authentication cannot be changed within a connection */
    public static final int IMARC_ChangedAuthentication     =  175;
    /** The operation is not authorized         */
    public static final int IMARC_NotAuthorized             =  180;
    /** User authentication failed              */
    public static final int IMARC_NotAuthenticated          =  181;
    /** The transaction is associated with another session */
    public static final int IMARC_TransactionInUse          =  206;
    /** An invalid parameter was supplied */
    public static final int IMARC_InvalidParameter          =  207;
    /** The sending of messages to this destination is not allowed */
    public static final int IMARC_SendNotAllowed            =  211;
    /** The subscription name being requested already exists */
    public static final int IMARC_ExistingSubscription      =  212;
    /** The client ID is in use by an active connection */
    public static final int IMARC_ClientIDConnected         =  213;
    /** The message size is too large for this endpoint. */
    public static final int IMARC_MsgTooBig                 =  287;
    /** The topic is a system topic             */
    public static final int IMARC_BadSysTopic               =  289;
    /** The consumer must have a matching share with the existing subscription */
    public static final int IMARC_ShareMismatch             =  290;
    /** There are no subscriptions              */
    public static final int IMARC_NoSubscriptions           =  296;
    /** The HTTP request is not valid           */
    public static final int IMARC_HTTP_BadRequest           =  400;
    /** The HTTP request is not authorized      */
    public static final int IMARC_HTTP_NotAuthorized        =  401;
    /** The HTTP request is for an object which does not exist */
    public static final int IMARC_HTTP_NotFound             =  404;
}
