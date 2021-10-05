/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

/*
 * IMA server return codes used in the JMS client
 */
public class ImaReturnCode {
    public static final int IMARC_DestinationFull           =  14;      /* The destination is full                 */
    public static final int IMARC_RolledBack                =  15;      /* The transaction was rolled back         */
    public static final int IMARC_DestinationInUse          =  18;      /* The destination is in use               */
    public static final int IMARC_HeuristicallyCommitted    =  31;      /* The transaction has already been heuristically committed */
    public static final int IMARC_HeuristicallyRolledBack   =  32;      /* The transaction has already been heuristically rolled back */
    public static final int IMARC_NotImplemented            =  102;     /* The function is not implemented         */
    public static final int IMARC_AllocateError             =  103;     /* Unable to allocate memory               */
    public static final int IMARC_ServerCapacity            =  104;     /* The server capacity is reached          */
    public static final int IMARC_BadClientData             =  105;     /* The data from the client is not valid   */
    public static final int IMARC_Closed                    =  106;     /* The object is closed                    */
    public static final int IMARC_Destroyed                 =  107;     /* The object is already destroyed         */
    public static final int IMARC_NullPointer               =  108;     /* A null object is not allowed            */
    public static final int IMARC_TimeOut                   =  109;     /* The receive timed out                   */
    public static final int IMARC_LockNotGranted            =  110;     /* The lock could not be acquired          */
    public static final int IMARC_BadPropertyName           =  111;     /* The property name is not valid: property={0}  */
    public static final int IMARC_BadPropertyValue          =  112;     /* A property value is not valid: property={0} value={1}  */
    public static final int IMARC_NotFound                  =  113;     /* The requested item is not found         */ 
    public static final int IMARC_ObjectNotValid            =  114;     /* The messaging object is not valid       */
    public static final int IMARC_ArgNotValid               =  115;     /* An argument not valid: name={0}         */
    public static final int IMARC_NullArgument              =  116;     /* A null argument is not allowed          */
    public static final int IMARC_MessageNotValid           =  117;     /* The message is not valid                */
    public static final int IMARC_PropertiesNotValid        =  118;     /* The properties are not valid            */
    public static final int IMARC_ClientIDNotValid          =  119;     /* The client ID is not valid: {0}         */
    public static final int IMARC_ClientIDInUse             =  121;     /* The client ID is already in use         */
    public static final int IMARC_NameNotValid              =  123;     /* The subscription name is not valid      */
    public static final int IMARC_DestNotValid              =  124;     /* The destination name is not valid       */
    public static final int IMARC_TooManyProdCons           =  154;     /* There are too many consumers in the connection */
    public static final int IMARC_NotAuthorized             =  180;     /* The operation is not authorized         */
    public static final int IMARC_NotAuthenticated          =  181;     /* User authentication failed              */
    public static final int IMARC_TransactionInUse          =  206;     /* The transaction is associated with another session */
    public static final int IMARC_InvalidParameter          =  207;     /* An invalid parameter was supplied */
    public static final int IMARC_SendNotAllowed            =  211;     /* The sending of messages to this destination is not allowed */
    public static final int IMARC_ExistingSubscription      =  212;     /* The subscription name being requested already exists */
    public static final int IMARC_ClientIDConnected         =  213;     /* The client ID is in use by an active connection */
    public static final int IMARC_MsgTooBig                 =  287;     /* The message size is too large for this endpoint. */
    public static final int IMARC_BadSysTopic               =  289;     /* The topic is a system topic             */
    public static final int IMARC_ShareMismatch             =  290;     /* The consumer must have a matching share with the existing subscription */

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
