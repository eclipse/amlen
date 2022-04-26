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

/* @file ismrc.h
 * Define ISM error codes.
 * The return codes 1-99 are reserved for intra-component
 * return values and return codes which do not indicate errors.
 * These should not ever be shown to users.
 *
 * Each return code >=100 has a message string associated with
 * it. The translated value of this string might be used
 * when showing the cause of a log message.
 *
 * We assign error return codes in the range 100 to 799.
 * we assign these with some discontiguous ranges so that
 * related return code are grouped together but there is no
 * significance to any value within this range. All return
 * codes have TMS entries in the server_utils component.
 *
 * This file is also used to define the options and default string.
 */
#ifndef __ISM_RC_DEFINED
#define __ISM_RC_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ISM Return Codes
 */
#ifndef ISMRC_SPECIAL
#define RC(name, val, opt, desc) name = val,
typedef enum {
#endif
    /*  name                    value  options   description */

    /* Conditions, not errors */
    RC( ISMRC_OK                         ,   0,   0,  "")
    RC( ISMRC_WaiterDisabled             ,   9,   0,  "The waiter is disabled")
    RC( ISMRC_AsyncCompletion            ,  10,   0,  "The operation will be completed asynchronously")
    RC( ISMRC_NoMsgAvail                 ,  11,   0,  "No message is available")
    RC( ISMRC_PropertyNotFound           ,  12,   0,  "The requested property was not found")
    RC( ISMRC_EndOfMessage               ,  13,   0,  "The end of the message is reached")
    RC( ISMRC_DestinationFull            ,  14,   0,  "The request failed because a destination is full")
    RC( ISMRC_RolledBack                 ,  15,   0,  "The transaction is rolled back")
    RC( ISMRC_TransactionLimit           ,  16,   0,  "The limit of operations in a transaction has been reached")
    RC( ISMRC_NoAvailWaiter              ,  17,   0,  "No available waiter")
    RC( ISMRC_DestinationInUse           ,  18,   0,  "The destination is in use")
    RC( ISMRC_ExistingKey                ,  19,   0,  "The key already exists in the table")
    RC( ISMRC_WaiterRemoved              ,  20,   0,  "The waiter has been removed from the queue")
    RC( ISMRC_NotDelivered               ,  21,   0,  "The waiter was not ready to receive the message")
    RC( ISMRC_StoreNoMoreEntries         ,  22,   0,  "There are no more entries in the store")
    RC( ISMRC_WaiterEnabled              ,  23,   0,  "The waiter is enabled")
    RC( ISMRC_NotInThreadCache           ,  24,   0,  "The requested topic name does not match the one cached for this thread")
    RC( ISMRC_NoMsgAvailForConsumer      ,  25,   0,  "No message is available for the consumer")
    RC( IMSRC_RecheckQueue               ,  26,   0,  "A message was not returned but messages may be available")
    RC( IMSRC_ServerRestartRequired      ,  27,   0,  "Need to restart the server.")
    RC( ISMRC_DisableWaiterCancel        ,  28,   0,  "An asynchronous request to stop a waiter was canceled" )

    RC( ISMRC_WouldBlock                 ,  30,   0,  "Operation would cause the thread to block")
    RC( ISMRC_HeuristicallyCommitted     ,  31,   0,  "The transaction has already been heuristically committed")
    RC( ISMRC_HeuristicallyRolledBack    ,  32,   0,  "The transaction has already been heuristically rolled back")
    RC( ISMRC_DeliveryIdAvailable        ,  33,   0,  "A delivery id is available to be assigned to a message")
    RC( ISMRC_ResumedClientState         ,  34,   0,  "A client state was successfully created for an existing client id")
    RC( ISMRC_SomeDestinationsFull       ,  35,   0,  "The request succeeded but some destinations were full")
    RC( ISMRC_OldTimestamp               ,  36,   0,  "The retained message server timestamp is lower than the current for this topic")
    RC( ISMRC_NeedStoreCommit            ,  37,   0,  "Operations have been performed that need to be committed to the persistent store")
    RC( ISMRC_EndOfFile                  ,  38,   0,  "The end of the file has been reached")
    RC( ISMRC_NoMatchingDestinations     ,  39,   0,  "The request succeeded but no matching destinations were found")
    RC( ISMRC_NoMatchingLocalDestinations,  40,   0,  "The request succeeded but no matching local destinations were found")

    RC( ISMRC_ConnectionComplete         ,  90,   0,  "The connection has completed normally.")   /* String for return code of 0 */
    RC( ISMRC_ClosedByClient             ,  91,   0,  "The connection was closed by the client.")
    RC( ISMRC_ClosedByServer             ,  92,   0,  "The connection was closed by the server.")
    RC( ISMRC_ServerTerminating          ,  93,   0,  "The connection was closed because the server was shutdown.")
    RC( ISMRC_EndpointDisabled           ,  94,   0,  "The connection was closed by an administrator.")
    RC( ISMRC_ClosedOnSend               ,  95,   0,  "The connection was closed during a send operation.")
    RC( ISMRC_NonAckingClient            ,  96,   0,  "The connection was closed because it has failed to acknowledge a message")

    /* Common error conditions */
    RC( ISMRC_Error                ,  100,   4,  "System error")
    RC( ISMRC_Failure              ,  101,   4,  "Failure")               /* Avoid using this generic return code */
    RC( ISMRC_NotImplemented       ,  102,   4,  "The function is not implemented")
    RC( ISMRC_AllocateError        ,  103,   4,  "Unable to allocate memory.")
    RC( ISMRC_ServerCapacity       ,  104,   4,  "The server capacity is reached.")
    RC( ISMRC_BadClientData        ,  105,   4,  "The data from the client is not valid.")
    RC( ISMRC_Closed               ,  106,   0,  "The object is closed.")
    RC( ISMRC_Destroyed            ,  107,   0,  "The object is already destroyed.")
    RC( ISMRC_NullPointer          ,  108,   0,  "A null object is not allowed.")
    RC( ISMRC_TimeOut              ,  109,   0,  "The receive timed out.")
    RC( ISMRC_LockNotGranted       ,  110,   0,  "The lock could not be acquired.")
    RC( ISMRC_BadPropertyName      ,  111,   0,  "The property name is not valid: Property: {0}.")
    RC( ISMRC_BadPropertyValue     ,  112,   0,  "The property value is not valid: Property: {0} Value: \"{1}\".")
    RC( ISMRC_NotFound             ,  113,   0,  "The requested item is not found.")       /* need more for various objects? */
    RC( ISMRC_ObjectNotValid       ,  114,   0,  "The messaging object is not valid.")
    RC( ISMRC_ArgNotValid          ,  115,   0,  "An argument is not valid: Name: {0}.")
    RC( ISMRC_NullArgument         ,  116,   0,  "A null argument is not allowed.")
    RC( ISMRC_MessageNotValid      ,  117,   0,  "The message is not valid.")
    RC( ISMRC_PropertiesNotValid   ,  118,   0,  "The properties are not valid.")
    RC( ISMRC_ClientIDNotValid     ,  119,   0,  "The client ID is not valid: {0}.")
    RC( ISMRC_ClientIDRequired     ,  120,   0,  "The client ID is required and is not specified.")
    RC( ISMRC_ClientIDInUse        ,  121,   0,  "The client ID is already in use: {0}.")
    RC( ISMRC_UnicodeNotValid      ,  122,   0,  "The Unicode value is not valid.")
    RC( ISMRC_NameNotValid         ,  123,   0,  "The subscription name is not valid: {0}.")
    RC( ISMRC_DestNotValid         ,  124,   0,  "The destination name is not valid: {0}.")
    RC( ISMRC_DestTypeNotValid     ,  125,   0,  "The destination type is not valid.")
    RC( ISMRC_NoDestination        ,  126,   0,  "The destination is required and cannot be null.")
    RC( ISMRC_BadPropertyType      ,  127,   0,  "The property type is not valid. Object: {0}  Name: {1}  Property: {2}  Type: {3}")
    RC( ISMRC_BadConfigName        ,  128,   0,  "The name of a named configuration object is not valid: {0}.")
    RC( ISMRC_MessageHub           ,  129,   0,  "The endpoint must be part of a message hub.")
    RC( ISMRC_MonDataNotAvail      ,  130,   0,  "The monitoring data is not available because the server status is not \"Running\".")
    RC( ISMRC_ProtocolMismatch     ,  131,   0,  "The client ID {0} is already associated with a different protocol: {1}.")
    RC( ISMRC_BadOptionValue       ,  132,   0,  "The property value is not valid. Object: {0} Name: {1} Property: {2} Value: \"{3}\".")
    RC( ISMRC_NameLimitExceed      ,  133,   0,  "The name of the configuration object is too long. Object: {0} Property: {1} Value: {2}.")
    RC( ISMRC_PropertyRequired     ,  134,   0,  "The value specified for the required property is invalid or null. Property: {0} Value: {1}.")
    RC( ISMRC_RegularExpression    ,  135,   0,  "The {0} value is an invalid regular expression.")
    RC( ISMRC_ObjectNotFound       ,  136,   0,  "The item or object cannot be found. Type: {0} Name: {1}")
    RC( ISMRC_BadRESTfulRequest    ,  137,   0,  "The REST API call: {0} is not valid.")
    RC( ISMRC_BadAdminPropName     ,  138,   0,  "The property name is invalid. Object: {0} Name: {1} Property: {2}")
    RC( ISMRC_MinOneOptMissing     ,  139,   0,  "The object: {0} must have one of the properties {1} specified")
    RC( ISMRC_InvalidCfgObject     ,  140,   0,  "The object {0} is not valid.")
    RC( ISMRC_VerifyTestOK         ,  141,   0,  "The Object: {0} Name: {1} has been verified successfully.")
    RC( ISMRC_SysCallFailed        ,  142,   0,  "{0} system call failed: The error is {1}({2}).")
    RC( ISMRC_InvalidObjectConfig  ,  143,   0,  "Invalid object configuration data is provided for the {0} object.")
    RC( ISMRC_LenthLimitExceed     ,  144,   0,  "The value that is specified for the property on the configuration object is too long. Object: {0} Property: {1} Value: {2}.")
    RC( ISMRC_RestartNeeded        ,  145,   0,  "The server must be restarted to complete the configuration changes.")
    RC( ISMRC_LenthLimitSingleton  ,  146,   0,  "The value that is specified for the property is too long. Property: {0} Value: {1}.")
    RC( ISMRC_ILMTTagUpdatetError  ,  147,   0,  "A failure occurred while processing ILMT Tag file. License Type:{0} Version:{1}")
    RC( ISMRC_ClusterMemberNotFound,  148,   0,  "Could not find a matching inactive cluster member: ServerName:{0} ServerUID:{1}")
    RC( ISMRC_ServerStateProduction,  149,   0,  "The server state is not running in production mode.")

    /* Network errors */
    RC( ISMRC_UnableToConnect      ,  150,   0,  "Unable to connect.")
    RC( ISMRC_UnknownHost          ,  151,   0,  "Unknown host.")
    RC( ISMRC_NotConnected         ,  152,   0,  "The connection is not open.")
    RC( ISMRC_NetworkError         ,  153,   0,  "There was a networking error when data was being sent or received.")
    RC( ISMRC_TooManyProdCons      ,  154,   0,  "Too many producers and consumers in a single connection.")   /* TODO: Replace this */
    RC( ISMRC_PortInUse            ,  155,   0,  "The port is in use.")
    RC( ISMRC_EndpointSocket       ,  156,   0,  "Unable to create the endpoint socket.")
    RC( ISMRC_PortNotValid         ,  157,   0,  "The port is not valid.  The port must be between 1 and 65535.")
    RC( ISMRC_IPNotValid           ,  158,   0,  "The IP address is not valid.")
    RC( ISMRC_ConnectNotAuthorized ,  159,   0,  "The connection is not authorized.")
    RC( ISMRC_ConnectTimedOut      ,  160,   0,  "The connection timed out.")
    RC( ISMRC_PingFailed           ,  161,   0,  "Failed to ping client.")
    RC( ISMRC_NoPingResponse       ,  162,   0,  "The client did not respond to a ping.")
    RC( ISMRC_ClosedTLSHandshake   ,  163,   0,  "Closed during TLS handshake.")
    RC( ISMRC_NoFirstPacket        ,  164,   0,  "No data was received on a connection.")
    RC( ISMRC_FirstPacketTooBig    ,  165,   0,  "The initial packet is too large.")
    RC( ISMRC_BadClientID          ,  166,   0,  "The ClientID is not valid.")     /* Used as a reason code */
    RC( ISMRC_ServerNotAvailable   ,  167,   0,  "Server not available.")
    RC( ISMRC_MessagingNotAvailable,  168,   0,  "Messaging is not available.")
    RC( ISMRC_NoCertificate        ,  169,   0,  "Certificate missing.")
    RC( ISMRC_CertificateNotValid  ,  170,   0,  "Certificate not valid.")
    RC( ISMRC_BadHTTP              ,  171,   0,  "HTTP handshake failed.")
    RC( ISMRC_MsgidRemove          ,  172,   0,  "Unable to remove message ID at server.")
    RC( ISMRC_TooManyConnections   ,  173,   0,  "Too many connections for an organization.")
    RC( ISMRC_ClientSetNotValid    ,  174,   0,  "The client set specification is not valid.")
    RC( ISMRC_ChangedAuthentication,  175,   0,  "The HTTP Authorization header cannot be changed in a connection.")
    RC( ISMRC_TooManyAuthRequests,    176,   0,  "Authentication request is in delay or too many authentication requests.")
    RC( ISMRC_CreateDeviceFailed,     177,   0,  "Failed to create device.")
	RC( ISMRC_SendQBytesLimitReached, 178,   0,  "Exceeded quota for buffered data (slow consumer).")

    /* Authorization and authentication errors */
    RC( ISMRC_NotAuthorized        ,  180,   0,  "The operation is not authorized.")
    RC( ISMRC_NotAuthenticated     ,  181,   0,  "User authentication failed.")
    RC( ISMRC_LTPATokenExpired     ,  182,   0,  "LTPA token has expired.")
    RC( ISMRC_OAuthInvalidToken    ,  183,   0,  "OAuth token is not valid.")
    RC( ISMRC_OAuthServerError     ,  184,   0,  "The messaging server could not connect to the OAuth authorization server. The cURL return code is {0}.")
    RC( ISMRC_NoSecProfile         ,  185,   0,  "The security profile must be set if security is enabled.")
    RC( ISMRC_NoCertProfile        ,  186,   0,  "The certificate profile must be set if TLSEnabled is true.")
    RC( ISMRC_CreateSSLContext     ,  187,   0,  "Unable to create the TLS context.")
    RC( ISMRC_AuthUnavailable      ,  188,   0,  "Authorization service unavailable.")
    RC( ISMRC_InvalidSecContext    ,  189,   0,  "The security context is not valid.")
    RC( ISMRC_LTPAInvalidKeyFile   ,  190,   0,  "LTPA key file is not valid or missing, or the key file and password do not match.")
    RC( ISMRC_LTPADecodeError      ,  191,   0,  "Failed to decode LTPA key or token.")
    RC( ISMRC_LTPADecryptError     ,  192,   0,  "Failed to decrypt LTPA key or token.")
    RC( ISMRC_ConnectionExpired    ,  193,   0,  "The connection has expired.")
    RC( ISMRC_LTPAInvalidKeys      ,  194,   0,  "LTPA private or public key is not valid.")
    RC( ISMRC_LTPASigGenError      ,  195,   0,  "Failed to generate LTPA token signature.")
    RC( ISMRC_LTPASigVerifyError   ,  196,   0,  "Failed to verify LTPA token signature.")
    RC( ISMRC_LTPASSLError         ,  197,   0,  "Low-level SSL call failed during LTPA key file or token processing.")
    RC( ISMRC_MalformedVariable    ,  198,   0,  "A variable specified in the policy is not valid. The variable must end with a closing brace ( } ).")
    RC( ISMRC_UnknownVariable      ,  199,   0,  "An unknown variable is specified in the policy.")
    RC( ISMRC_HTTP_Success         ,  200,   0,  "The request succeeded.")   /* Used to keep rc value the same as HTTP status */

    /* Engine errors */
    /* RC( ISMRC_TransactionActive    ,  200,   0,  "The transaction is active.") Not used. Removed from TMS file. */
    RC( ISMRC_RequestInProgress             , 201,   0,  "A request is already in progress.")
    RC( ISMRC_HTTP_Accepted                 , 202,   0,  "The request is accepted.")    /* Not an engine rc, just used to keep the value the same as HTTP */
    RC( ISMRC_WaiterInUse                   , 203,   0,  "The waiter has been initialized already.")
    RC( ISMRC_WaiterInvalid                 , 204,   0,  "The waiter is not initialized.")
    RC( ISMRC_QueueDeleted                  , 205,   0,  "The queue has been deleted already.")
    RC( ISMRC_TransactionInUse              , 206,   0,  "The transaction is associated with another session.")
    RC( ISMRC_InvalidParameter              , 207,   0,  "An invalid parameter was supplied.")
    RC( ISMRC_InvalidOperation              , 208,   0,  "A request was rejected as it is currently invalid.")
    RC( ISMRC_DestinationNotEmpty           , 209,   0,  "The destination still holds messages.")
    RC( ISMRC_TooManyConsumers              , 210,   0,  "The destination has too many existing consumers.")
    RC( ISMRC_SendNotAllowed                , 211,   0,  "The sending of messages to this destination is not allowed.")
    RC( ISMRC_ExistingSubscription          , 212,   0,  "The subscription name being requested already exists.")
    RC( ISMRC_ClientIDConnected             , 213,   0,  "The client ID is in use by an active connection.")
    RC( ISMRC_ExistingTopicMonitor          , 214,   0,  "A topic monitor on the specified topic string already exists.")
    RC( ISMRC_InvalidTopicMonitor           , 215,   0,  "The topic name for a topic monitor must contain a maximum of 32 levels, end with a multi-level wildcard (#), and contain no other wildcards.")
    RC( ISMRC_MaxDeliveryIds                , 216,   0,  "No valid delivery id exists that can be used by this connection.")
    RC( ISMRC_NotEngineControlled           , 217,   0,  "Invalid attempt to start or stop message delivery on a connection.")
    RC( ISMRC_PolicyInUse                   , 218,   0,  "The policy is still being used.")
    RC( ISMRC_ClientTableGenMismatch        , 219,   0,  "The Client table generation number does not match the requested generation number.")
    RC( ISMRC_MoreChainsAvailable           , 220,   0,  "There are more chains in the client table that have not been traversed.")
    RC( ISMRC_FileCorrupt                   , 221,   0,  "The file has been truncated or contains invalid data")
    RC( ISMRC_NonDurableImport              , 222,   0,  "A non-durable object was not included during an import request")
    RC( ISMRC_FileAlreadyExists             , 223,   0,  "A file named {0} already exists.")
    RC( ISMRC_TooManyActiveRequests         , 224,   0,  "There are {0} requests in progress, the maximum is {1}.")
    RC( ISMRC_RequestInProgressAtStartup    , 225,   0,  "The request is still marked as 'in progress' at startup.")
    RC( ISMRC_SavepointActive               , 226,   0,  "There is already an active savepoint on the specified transaction")
    RC( ISMRC_WrongSubscriptionAPI          , 227,   0,  "The subscription can only be updated via the {0} API.")
    RC( ISMRC_PropertyValueMismatch         , 228,   0,  "The value {1} for property {0} must match the existing value {2}.")
    RC( ISMRC_AsyncCBQAlerted               , 229,   0,  "The operation was not performed because a warning is in place for the asynchronous callback queues.")
    RC( ISMRC_ExistingClusterRequestedTopic , 230,   0,  "The specified topic name is already defined as cluster requested topic.")
    RC( ISMRC_InvalidClusterRequestedTopic  , 231,   0,  "The topic name for a cluster requested topic must contain a maximum of 32 levels.")

    /* Message selection errors */
    RC( ISMRC_TooComplex           , 241,   0,  "The selection rule is too complex.")
    RC( ISMRC_IsNotValid           , 242,   0,  "The IS expression is not valid.")
    RC( ISMRC_InRequiresGroup      , 243,   0,  "The IN must be followed by a group.")
    RC( ISMRC_InGroupNotValid      , 244,   0,  "The IN group is not valid.")
    RC( ISMRC_InSeparator          , 245,   0,  "The separator in an IN group is missing or not valid.")
    RC( ISMRC_LikeSyntax           , 246,   0,  "The LIKE must be followed by a string.")
    RC( ISMRC_EscapeNotValid       , 247,   0,  "The ESCAPE within a LIKE must be a single character string.")
    RC( ISMRC_StringTooComplex     , 248,   0,  "A string in a selection rule is too complex.")
    RC( ISMRC_OperandNotValid      , 249,   0,  "The identifier or constant is not valid: {0}.")
    RC( ISMRC_BetweenNotValid      , 250,   0,  "The BETWEEN operator is not valid.")
    RC( ISMRC_TooManyLeftParen     , 251,   0,  "Too many left parentheses.")
    RC( ISMRC_OperandMissing       , 252,   0,  "A field or constant is expected at the end of a selection rule.")
    RC( ISMRC_OpNotBoolean         , 253,   0,  "The {0} operator does not allow a boolean argument.")
    RC( ISMRC_OpNotNumeric         , 254,   0,  "The {0} operator does not allow a numeric argument.")
    RC( ISMRC_OpRequiresID         , 255,   0,  "The {0} operator requires an identifier as an argument.")
    RC( ISMRC_NotBoolean           , 256,   0,  "A boolean result is required for the selector.")
    RC( ISMRC_TooManyRightParen    , 258,   0,  "Too many right parentheses.")
    RC( ISMRC_OpNoString           , 259,   0,  "The {0} operator does not allow a string or boolean argument.")

    /* Protocol errors */
    RC( ISMRC_WillNotAllowed       , 270,   0,  "A will message is not allowed.")
    RC( ISMRC_BadLength            , 271,   0,  "The length of the message is not correct.")
    RC( ISMRC_InvalidValue         , 272,   0,  "The MQTT message encoding is not valid.")
    RC( ISMRC_InvalidPayload       , 273,   0,  "A message payload is not valid.")
    RC( ISMRC_ParseError           , 274,   0,  "The JSON encoding is not correct. rc={0} line={1}.")
    RC( ISMRC_InvalidCommand       , 275,   0,  "An unknown message command is specified.")
    RC( ISMRC_BadTopic             , 276,   0,  "The topic is not valid.")
    RC( ISMRC_WillRequired         , 277,   0,  "If the will flag is specified, the will message and will topic are required.")
    RC( ISMRC_CreateConnection     , 278,   0,  "An error occurred while creating an MQTT connection.")
    RC( ISMRC_CreateSession        , 279,   0,  "An error occurred while creating an MQTT session.")
    RC( ISMRC_StartMsg             , 280,   0,  "An error occurred while starting MQTT messages.")
    RC( ISMRC_CreateConsumer       , 281,   0,  "An error occurred while creating an MQTT consumer.")
    RC( ISMRC_ConnectFirst         , 282,   0,  "CONNECT must be the first message in a connection.")
    RC( ISMRC_InvalidQoS           , 284,   0,  "The quality of service is not valid.")
    RC( ISMRC_InvalidID            , 285,   0,  "The ID is not valid; it must be a number between 1 and 65535.")
    RC( ISMRC_UsernameRequired     , 286,   0,  "When the password is set, the username is required.")
    RC( ISMRC_MsgTooBig            , 287,   0,  "The message size is too large for this endpoint.")
    RC( ISMRC_ClientIDReused       , 288,   0,  "The client ID was reused.")
    RC( ISMRC_BadSysTopic          , 289,   0,  "The topic cannot be used because it is a system topic.")
    RC( ISMRC_ShareMismatch        , 290,   0,  "The consumer must have a matching share with the existing subscription.")
    RC( ISMRC_MaxQoS               , 291,   0,  "The requested quality of service is not allowed.")
    RC( ISMRC_DurableNotAllowed    , 292,   0,  "A cleansession=0 session is not allowed.")
    RC( ISMRC_ProtocolVersion      , 293,   0,  "The version of the protocol is not supported.")
    RC( ISMRC_TooMuchSavedData     , 294,   0,  "Too much data was received before authentication was complete.")
    RC( ISMRC_NotBinary            , 295,   0,  "The packet type must be binary.")
    RC( ISMRC_NoSubscriptions      , 296,   0,  "There are no subscriptions.")
    RC( ISMRC_ProtocolError        , 297,   0,  "The protocol is not correct.")
    RC( ISMRC_TopicAliasNotValid   , 298,   0,  "The topic alias is invalid.")
    RC( ISMRC_ReceiveMaxExceeded   , 299,   0,  "Too many persistent messages are in process.")

    /* MQ Connectivity errors */
    RC( ISMRC_MappingUpdate        , 300,   0,  "Cannot modify destination mapping rule in enabled state.")
    RC( ISMRC_MappingStateUpdate   , 301,   0,  "Destination mapping rule state must not change when other updates to the rule are made.")
    RC( ISMRC_QMUpdate             , 302,   0,  "Queue manager connection associated with enabled destination mapping rule cannot be updated.")
    RC( ISMRC_QMDelete             , 303,   0,  "Queue manager connection associated with destination mapping rule cannot be deleted.")
    RC( ISMRC_ISMNotAvailable      , 304,   0,  "MQ Connectivity could not connect to the messaging server.")
    RC( ISMRC_UnexpectedConfig     , 305,   0,  "An unexpected MQ Connectivity configuration was encountered.")
    RC( ISMRC_TooManyRules         , 306,   0,  "Maximum number of rules already defined.")
    RC( ISMRC_Unresolved           , 307,   0,  "The queue manager connection has unresolved transactions. The update is not allowed without the force option.")
    RC( ISMRC_SSLError             , 308,   0,  "SSL error while connecting to queue manager.")
    RC( ISMRC_ChannelError         , 309,   0,  "Channel error when connecting to queue manager.")
    RC( ISMRC_HostError            , 310,   0,  "Host error while connecting to queue manager")
    RC( ISMRC_QMError              , 311,   0,  "Queue manager not responding.")
    RC( ISMRC_CDError              , 312,   0,  "Connection descriptor error while connecting to queue manager.")
    RC( ISMRC_QMNameError          , 313,   0,  "Queue manager name error.")
    RC( ISMRC_UnableToConnectToQM  , 314,   0,  "An attempt to connect to a queue manager failed.")
    RC( ISMRC_NotAuthorizedAtQM    , 315,   0,  "The operation is not authorized at the queue manager.")
    RC( ISMRC_MQCProcessError      , 316,   0,  "The messaging server could not connect to MQ Connectivity service.")

    /* Admin Errors and options */
    RC( ISMRC_InvalidComponent     , 330,   0,  "Component, item or object name is not valid or NULL.")
    RC( ISMRC_AdminUserPolicy      , 331,   0,  "Can not add or modify the admin user policy.")
    RC( ISMRC_InvalidPolicy        , 332,   0,  "The policy is not valid.")
    RC( ISMRC_NoConnectionPolicy   , 333,   0,  "No connection policy is configured.")
    RC( ISMRC_NoMessagingPolicy    , 334,   0,  "No messaging policy is configured.")
    RC( ISMRC_PropCreateError      , 335,   0,  "Object already exists. Use the imaserver update command to update the object.")
    RC( ISMRC_PropDeleteError      , 336,   0,  "Delete is not allowed for a non-existent object.")
    RC( ISMRC_SingltonDeleteError  , 337,   0,  "Delete is not allowed for a singleton object.")
    RC( ISMRC_PropEditError        , 338,   0,  "Update is not allowed for a non-existent object.")
    /* RC( ISMRC_NVDIMMConfigError    , 339,   0,  "Store memory configuration error.") Not used. Removed from TMS file. */
    RC( ISMRC_NVDIMMIntegrityError , 340,   0,  "Store memory integrity error.")
    RC( ISMRC_UserNotAuthorized    , 341,   0,  "The user is not authorized.")
    RC( ISMRC_GroupNotAuthorized   , 342,   0,  "The group is not authorized.")
    RC( ISMRC_ClientNotAuthorized  , 343,   0,  "The client is not authorized.")
    RC( ISMRC_ProtoNotAuthorized   , 344,   0,  "The protocol is not authorized.")
    RC( ISMRC_AddrNotAuthorized    , 345,   0,  "The client address is not authorized.")
    RC( ISMRC_CNameNotAuthorized   , 346,   0,  "The certificate common name is not authorized.")
    RC( ISMRC_TopicNotAuthorized   , 347,   0,  "Not authorized to publish or subscribe to the specified destination.")
    RC( ISMRC_TopIDNotAuthorized   , 348,   0,  "Not authorized to publish or subscribe to \"UserID\" substituted destination.")
    RC( ISMRC_TopCLNotAuthorized   , 349,   0,  "Not authorized to publish or subscribe to \"ClientID\" substituted destination.")
    RC( ISMRC_ConfigNotAllowed     , 350,   0,  "Configuration changes are not allowed on a standby node.")
    RC( ISMRC_EndpointNotFound     , 351,   0,  "The endpoint is not configured.")
    RC( ISMRC_ConnectionNotFound   , 352,   0,  "No connection data is found.")
    RC( ISMRC_HAInSync             , 353,   0,  "A HighAvailibility node synchronization process is in progress. Configuration changes are not allowed at this time.")
    RC( ISMRC_RunModeChangePending , 354,   0,  "The messaging server is currently in {0} mode.\nWhen it is restarted, it will be in {1} mode.")
    RC( ISMRC_CleanStorePending    , 355,   0,  "The messaging server is currently in {0} mode with {1} action pending.\nWhen it is restarted, it will be in {2} mode.")
    RC( ISMRC_UnknownServerState   , 356,   0,  "The messaging server did not respond to the status request.")
    /* RC( ISMRC_EndpointMsgPolError  , 357,   0,  "If you specify a messaging policy where the destination type is Subscription, you must specify at least one messaging policy where the destination type is Topic. The topic messaging policy must grant subscribe authority to the topic that is associated with the subscription name specified in the subscription messaging policy. These messaging policies must be associated with the same endpoint:  \"{0}\".") Not used. Removed from TMS file. */
    /* RC( ISMRC_TopCNNotAuthorized   , 358,   0,  "Not authorized to publish or subscribe to \"CommonName\" substituted destination.") Not used. Removed from TMS file. */
    RC( ISMRC_AdmnHAStandbyError   , 359,   0,  "Failed to synchronize configuration on the standby node in the HA-pair.")
    RC( ISMRC_AdmnHAPrimaryError   , 360,   0,  "Failed to complete administration task to synchronize configuration on the standby node in the HA-pair.")
    RC( ISMRC_AuthorityCheckError  , 361,   0,  "Authorization failed. The requested authority does not match the authority specified in the policy.")
    RC( ISMRC_GroupIDLimitExceeded , 362,   0,  "The messaging policy cannot have more than one instance of the \"GroupID\" substitution variable.")
    /* RC( ISMRC_InvalidGroupUpdate   , 363,   0,  "The \"Group\" cannot be updated.") Not used. Removed from TMS file. */
    RC( ISMRC_GroupUpdateFailed    , 364,   0,  "The Group cannot be synchronized on the standby node.")
    RC( ISMRC_UUIDConfigError      , 365,   0,  "A messaging server UUID configuration error has occurred.")
    RC( ISMRC_PluginZipNotValid    , 366,   0,  "The plug-in zip file is not valid.")
    RC( ISMRC_PluginApplyInProgress, 367,   0,  "The plug-in installation is still in progress.")
    RC( ISMRC_PluginConfigNotValid,  368,   0,  "The plug-in configuration file is not valid.")
    RC( ISMRC_PluginZipNotExisted ,  369,   0,  "The plug-in zip file does not exist.")
    RC( ISMRC_PolicyPendingDelete ,  370,   0,  "The messaging policy is pending deletion.")
    RC( ISMRC_ClientAddrMaxError  ,  371,   0,  "The number of client addresses exceeds the maximum number allowed: {0}.")
    RC( ISMRC_DeleteNotAllowed    ,  372,   0,  "Delete is not allowed for {0} object.")
    RC( ISMRC_NeedMoreConfigItems ,  373,   0,  "If you specify {0}, you must also specify {1}. If you specify {1}, you must also specify {0}.")
    RC( ISMRC_TooManyItems        ,  374,   0,  "The number of items exceeded the limit. Item: {0} Count: {1} Limit: {2}")
    RC( ISMRC_AdminUserRequired   ,  375,   0,  "User authentication is enabled in the admin endpoint. You must configure an external LDAP server, or set AdminUserID and AdminUserPassword.")
    RC( ISMRC_ObjectIsInUse       ,  376,   0,  "The Object: {0}, Name: {1} is still being used by Object: {2}, Name: {3}")
    RC( ISMRC_PluginJvmError      ,  377,   0,  "The plug-in process JVM configuration is not valid. The error is: {0}")
    RC( ISMRC_PluginUpdateError   ,  378,   0,  "The plug-in update failed. The error is: {0}")
    RC( ISMRC_SubTopicPolError    ,  379,   0,  "If you specify a subscription policy, you must also specify at least one topic policy. The topic policy must grant subscribe authority to the topic that is associated with the subscription name that is specified in the subscription policy. These subscription policies and topic policies must be associated with the same endpoint.")
    RC( ISMRC_DisNotifProtError   ,  380,   0,  "DisconnectedClientNotification cannot be enabled if MQTT protocol is not allowed in the topic policy.")
    RC( ISMRC_DisNotifSubsError   ,  381,   0,  "DisconnectedClientNotification cannot be enabled if topic policy does not grant subscribe authority.")
    RC( ISMRC_PropUpdateNotAllowed,  382,   0,  "Configuration change is not allowed on a standby node. Object: {0} Name: {1} Property: {2}")
    RC( ISMRC_FileUpdateError     ,  383,   0,  "Failed to create/update the file/directory: Name: {0} errno: {1}")
    RC( ISMRC_UpdateNotAllowed    ,  384,   0,  "You must disable the object {0} to update or set configuration item {1}.")
    RC( ISMRC_HADisOnClusterPrim  ,  385,   0,  "High-Availability is disabled on the Primary node with cluster enabled.")
    RC( ISMRC_CRLServerError      ,  386,   0,  "Error received from the CRL server. The cURL return code is {0}")
    RC( ISMRC_LicenseError        ,  387,   0,  "The server is not fully functional until you accept the license agreement using the REST API.")
    RC( ISMRC_NotAllowedOnStandby ,  388,   0,  "The action {0} for Object: {1} is not allowed on a standby node.")
    RC( ISMRC_RequireRunningServer,  389,   0,  "{0} objects can only be altered when the server status is \"Running\".")

    /* Other errors */
    /* RC( ISMRC_UEFIConfigError      , 390,   0,  "UEFI configuration error") Not used. Removed from TMS file. */
    /* RC( ISMRC_RAIDConfigError      , 391,   0,  "RAID configuration error") */
    RC( ISMRC_VMSHMLow             , 392,   0,  "The amount of available shared memory in the virtual messaging server is insufficient.")
    RC( ISMRC_VMSHMHigh            , 393,   0,  "The shared memory that is configured in the virtual messaging server is too large.")
    RC( ISMRC_VMDiskIsSmall        , 394,   0,  "The disk size that is configured in the virtual messaging server is too small.")
    RC( ISMRC_VMNeedMoreVCPUs      , 395,   0,  "More VCPUs are required in the virtual messaging server.")
    RC( ISMRC_VMInsufficientMemory , 396,   0,  "The amount of available system memory in the virtual messaging server is insufficient.")
    RC( ISMRC_VMCheckAdminLog      , 397,   0,  "More than one error is detected in the virtual messaging server. Check the imaserver-admin.log file for details.")
    RC( ISMRC_LicenseUsgUpdtErr	   , 398,   0,  "A failure occurred when the LicensedUsage configuration value was being updated.")
    RC( ISMRC_LicenseStatusUpdtErr , 399,   0,  "A failure occurred when the LicensedUsage status was being updated.")

    /* http status */
    RC( ISMRC_HTTP_BadRequest      , 400,   0,  "The HTTP request is not valid.")
    RC( ISMRC_HTTP_NotAuthorized   , 401,   0,  "The HTTP request is not authorized.")
    RC( ISMRC_HTTP_NotFound        , 404,   0,  "The HTTP request is for an object which does not exist.")
    RC( ISMRC_HTTP_Unsupported     , 415,   0,  "The HTTP request with unsupported content type.")

    /* Data validation errors */
    RC( ISMRC_NoMessageHub         , 430,   0,  "A MessageHub must be specified.")
    RC( ISMRC_InvalidMessageHub    , 431,   0,  "Invalid MessageHub {0} was specified.")
    /* RC( ISMRC_NoSecurityPolicies   , 432,   0,  "SecurityPolicies must be specified.") Not used. Removed from TMS file. */
    RC( ISMRC_NoConnectPolicies    , 433,   0,  "The endpoint must contain a valid connection policy.")
    RC( ISMRC_NoMesssgePolicies    , 434,   0,  "The endpoint must contain a valid messaging policy.")
    RC( ISMRC_NoPort               , 435,   0,  "A port number must be specified.")
    RC( ISMRC_InvalidPort          , 436,   0,  "An invalid port number was specified.")
    RC( ISMRC_NoEndpoint           , 437,   0,  "No endpoint is configured.")
    RC( ISMRC_MessageHubInUse      , 438,   0,  "MessageHub is in use. It still contains at least one endpoint.")
    RC( ISMRC_SecPolicyInUse       , 439,   0,  "The policy {0} is in use.")
    RC( ISMRC_SecProfileInUse      , 440,   0,  "SecurityProfile is in use.")
    RC( ISMRC_CertProfileInUse     , 441,   0,  "CertificateProfile is in use.")
    RC( ISMRC_ConnPolicyReUse      , 442,   0,  "Cannot reuse ConnectionPolicy.")
    /* RC( ISMRC_NoOptPolicyFilter    , 443,   0,  "At least one of the optional filters is required.") Not used. Removed from TMS file.*/
    RC( ISMRC_SetNotAllowed        , 444,   0,  "The property cannot be set or updated.")
    RC( ISMRC_InvalidNameLen       , 445,   0,  "The length of the name must be less than 256.")
    RC( ISMRC_InvalidLDAPURL       , 446,   0,  "The LDAP URL is not valid.")
    RC( ISMRC_FailedToBindLDAP     , 447,   0,  "Failed to bind with the  LDAP server: {0}. The error code is {1}.")
    RC( ISMRC_InvalidLDAP          , 448,   0,  "The LDAP configuration is not valid.")
    RC( ISMRC_InvalidLDAPCert      , 449,   0,  "The certificate for LDAP is not valid or does not exist.")
    RC( ISMRC_NoCertProfileFound   , 450,   0,  "The CertificateProfile was not found.")
    RC( ISMRC_CertNameInUse        , 451,   0,  "The specified certificate is in use.")
    RC( ISMRC_KeyNameInUse         , 452,   0,  "The specified key file is in use.")
    RC( ISMRC_ConfigError          , 453,   0,  "Configuration error.")
    RC( ISMRC_LTPAProfileInUse     , 454,   0,  "LTPAProfile is in use.")
    RC( ISMRC_NoLTPAProfileFound   , 455,   0,  "LTPAProfile is not found.")
    RC( ISMRC_NoTrustedCertificate , 456,   0,  "UseClientCertificate is enabled in the SecurityProfile. A Trusted Certificate or a Client Certificate must be associated with the SecurityProfile.")
    RC( ISMRC_FailedToObtainLDAPDN , 457,   0,  "Failed to obtain the LDAP distinguished name.")
    RC( ISMRC_OAuthProfileInUse    , 458,   0,  "OAuthProfile is in use.")
    RC( ISMRC_NoOAuthProfileFound  , 459,   0,  "OAuthProfile is not found.")
    RC( ISMRC_CertKeyMisMatch      , 460,   0,  "The certificate {0} and key file {1} do not match.")
    RC( ISMRC_DeleteFailure        , 461,   0,  "The HTTP DELETE request failed to delete configuration object: {0}, delete string: {1}.")
    RC( ISMRC_NoOAuthCertificate   , 462,   0,  "A secured ResourceURL or UserInfoURL is specified. You must also specify KeyFileName.")
    RC( ISMRC_SyslogInUse          , 463,   0,  "Syslog connectivity cannot be disabled.")
	RC( ISMRC_InvalidBaseDN        , 464,   0,  "The LDAP search failed for BaseDN: \"{0}\". The LDAP error is \"{1}\".")
    RC( ISMRC_InvalidPSKFile       , 465,   0,  "The PreSharedKey file is not valid.")

    /* Store errors */
    RC( ISMRC_StoreNotAvailable    , 500,   0,  "Store is not available.")
    RC( ISMRC_StoreUnrecoverable   , 501,   0,  "Store data is unrecoverable.")
    RC( ISMRC_StoreGenerationFull  , 502,   0,  "Store generation is full.")
    RC( ISMRC_StoreFull            , 503,   0,  "Store is full.")
    RC( ISMRC_StoreBusy            , 504,   0,  "Store is busy.")
    RC( ISMRC_StoreTransActive     , 505,   0,  "Store transaction is active.")
    RC( ISMRC_StoreDiskError       , 506,   0,  "Store disk error.")
    RC( ISMRC_StoreAllocateError   , 507,   0,  "Store memory allocation error.")
    RC( ISMRC_StoreBufferTooSmall  , 508,   0,  "Buffer length is too small.")
    RC( ISMRC_StoreHAError         , 509,   0,  "Store High-Availability error.")
    RC( ISMRC_StoreHABadConfigValue, 510,   0,  "An HA configuration parameter is not valid: parameter name={0} value={1}.")
    RC( ISMRC_StoreHADiscTimeExp   , 511,   0,  "Discovery time expired. Failed to communicate with the other node in the HA-pair.")
    RC( ISMRC_StoreHASplitBrain    , 512,   0,  "Two primary nodes have been identified (split-brain).")
    RC( ISMRC_StoreHASyncError     , 513,   0,  "Two unsynchronized nodes have been identified. Unable to resolve which store should be used.")
    RC( ISMRC_StoreHAUnresolved    , 514,   0,  "The two nodes have non-empty stores on startup.")
    RC( ISMRC_StoreHABadNicConfig  , 515,   0,  "The HA NIC {0} is not configured properly and cannot be used.")
    RC( ISMRC_StoreVersionConflict , 516,   0,  "Store version conflict.")
    RC( ISMRC_StoreOwnerLimit      , 517,   0,  "The store owner objects limit has been exceeded.")

    /* SNMP conditions */
    RC( ISMRC_SNMPWriteConfigFailed, 550,   0,  "A failure occurred when SNMP configuration data was being saved.")

    RC( ISMRC_SNMPParseError       , 552,   0,  "A failure occurred when SNMP configuration data was being parsed.")
    RC( ISMRC_SNMPFileReadError    , 553,   0,  "A failure occurred when SNMP configuration data was being read.")
    RC( ISMRC_SNMPNoData           , 554,   0,  "As a result of an internal error, no SNMP configuration information was obtained.")
    RC( ISMRC_SNMPGetStatusFailed  , 555,   0,  "A failure occurred when the value for SNMPEnabled was being retrieved.")
    RC( ISMRC_SNMPCommunityExists  , 556,   0,  "Community {0} was not created because the community already exists.")
    RC( ISMRC_SNMPCommunityInUse   , 557,   0,  "Community {0} was not deleted because the community is in use by one or more trap subscribers.")
    RC( ISMRC_SNMPCommNotFound     , 558,   0,  "Community {0} was not deleted because the community does not exist.")
    RC( ISMRC_SNMPTrapSubExists    , 559,   0,  "The trap subscriber was not created because a trap subscriber already exists for the host {0}.")
    RC( ISMRC_SNMPNoCommunityInput , 560,   0,  "A community was not created because no configuration data was provided.")
    RC( ISMRC_SNMPCommunityInvalid , 561,   0,  "The community was not created because the community name was not provided.")
    RC( ISMRC_SNMPRestartFailed    , 562,   0,  "The configuration change succeeded but the SNMP service was not restarted after the configuration was changed.")
    RC( ISMRC_SNMPNoDeleteCommInput, 563,   0,  "A community was not deleted because no configuration data was provided or the value for the community name was an empty string.")
    RC( ISMRC_SNMPRequestFailed    , 564,   0,  "The SNMP configuration change failed due to an internal error.")
    RC( ISMRC_SNMPTrapSubInvalid   , 565,   0,  "The trap subscriber was not created because required configuration data was not provided. When creating a trap subscriber, a host, a client port and a community must be specified.")
    RC( ISMRC_SNMPTrapSubNoComm    , 566,   0,  "The trap subscriber was not created because the community {0} does not exist.")
    RC( ISMRC_SNMPNoDelTrapSubInput, 567,   0,  "A trap subscriber was not deleted because no configuration data was provided or the value for host was an empty string.")
    RC( ISMRC_SNMPTrapSubNotFound  , 568,   0,  "The trap subscriber was not deleted because a trap subscriber for host {0} does not exist.")
    RC( ISMRC_SNMPNoTrapSubInput   , 569,   0,  "A trap subscriber was not created because no configuration data was provided.")
    RC( ISMRC_SNMPConfigLocked     , 570,   0,  "An SNMP configuration request failed because the configuration data was locked by another user.")
    RC( ISMRC_SNMPIPAddrNotValid   , 571,   0,  "The SNMP agent IP address {0} is not valid.")
	RC( ISMRC_SNMPNotConfigured	   , 572,	0,	"SNMP service is not enabled")

    /* Cluster errors */
    RC( ISMRC_ClusterDisabled                     , 700, 0, "Cluster membership is disabled.")
    RC( ISMRC_ClusterNotAvailable                 , 701, 0, "The cluster is not started or is not available.")
    RC( ISMRC_ClusterArrayTooSmall                , 702, 0, "The cluster array capacity is too small.")
    RC( ISMRC_ClusterInternalError                , 703, 0, "Cluster internal error: {0}.")
    RC( ISMRC_ClusterIOError                      , 704, 0, "Cluster I/O error {0}.")
    RC( ISMRC_ClusterInternalErrorState           , 705, 0, "The cluster stopped due to an internal error state.")
    RC( ISMRC_ClusterRemoveLocalServerNoAck       , 706, 0, "The removal of a local server from the cluster timed out without acknowledgement from remote servers.")
    RC( ISMRC_ClusterRemoveRemoteServerStillAlive , 707, 0, "An attempt to remove a remote server failed because the server is still active.")
    RC( ISMRC_ClusterRemoveRemoteServerFailed     , 708, 0, "The removal of a local server from the cluster failed.")
    RC( ISMRC_ClusterBindErrorLocalAddress        , 709, 0, "A cluster bind error has occurred, and local address {0} cannot be assigned.")
    RC( ISMRC_ClusterBindErrorPortBusy            , 710, 0, "A cluster bind error has occurred. Port {0} is busy.")
    RC( ISMRC_ClusterVersionConflict              , 711, 0, "A cluster version conflict has occurred.")
    RC( ISMRC_ClusterStoreVersionConflict         , 712, 0, "A cluster store version conflict has occurred.")
    RC( ISMRC_ClusterStoreOwnershipConflict       , 713, 0, "A cluster store ownership conflict has occurred.")
    RC( ISMRC_ClusterConfigError                  , 714, 0, "Cluster configuration error: {0}.")
    RC( ISMRC_ClusterLocalServerRemoved           , 715, 0, "The local server was previously removed from the cluster.")
    RC( ISMRC_ClusterDuplicateServerUID           , 716, 0, "The local server discovered another server with the same UID, the local server will shut down.")


#ifndef ISMRC_SPECIAL
} ism_rc_t;
#endif
#undef RC

#ifdef __cplusplus
}
#endif

#endif

/* End of ismmessage.h                                               */
