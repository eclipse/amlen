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
/*-----------------------------------------------------------------
 * File        : ScTraceable.cpp
 * Author      :
 * Version     : $Revision: 1.3 $
 * Last updated: $Date: 2015/11/18 08:37:03 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceable.cpp,v $
 * Revision 1.3  2015/11/18 08:37:03 
 * Update copyright headers
 *
 * Revision 1.2  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:19 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Traceable components implementation
 */

#include "ScTraceable.h"

namespace spdr
{

using namespace std;

    /**
     * Key names for common  objects that are not {@link ScDCSTraceable}
     * To be used with {@link ScDCSTraceBuffer#addProperty(string, Object)}.
     */
    /** key for name of a member */
    const string ScTraceable::MEMBER_NAME = "memberName";
    

    /** key for an array of members (string[]) */
    const string ScTraceable::VIEW_MEMBERS = "viewMembers";
    const string ScTraceable::FAILED_MEMBERS = "failedMembers";

    const string ScTraceable::VIEW_SIZE = "viewSize";

    
    /** key for DCSTraceables */
    const string ScTraceable::VIEW_ID = "ViewID";
    
    /** key for request context */

    /** Keys for ExteranlEvents **/
    const string ScTraceable::OBJECTS = "Objects";
    const string ScTraceable::OBJECT = "Object";
    const string ScTraceable::CLASS_NAME = "ClassName";
    const string ScTraceable::NLSKEY = "NLSKey";
    const string ScTraceable::NLS_PARAMS = "params";
    const string ScTraceable::SEVERITY = "Severity";
    const string ScTraceable::RECOMMENDED_ACTION = "RecommendedAction";
    const string ScTraceable::STACK_NAME = "StackName";
    const string ScTraceable::REASON = "Reason";
    const string ScTraceable::METHOD= "Method";
    const string ScTraceable::REASON_EXCEPTION = "ReasonException";
    const string ScTraceable::INTERNAL_DETAILS = "InternalDetails";
    const string ScTraceable::ACTION_TYPE = "ActionType";
    const string ScTraceable::ADDED_MEMBERS = "AddedMembers";
    const string ScTraceable::SUSPECTED_MEMBERS = "SuspectedMembers";
    const string ScTraceable::REMOVED_MEMBERS = "RemovedMembers";
    const string ScTraceable::MESSAGE_TYPE = "MessageType";
    const string ScTraceable::TYPE_CODE = "TypeCode";
    const string ScTraceable::EXTERNAL_EVENT = "ExternalEvent";
    const string ScTraceable::INTERNAL_EVENT = "InternalEvent";
    const string ScTraceable::TARGET = "Target";
    const string ScTraceable::THREAD_NAME = "ThreadName";
    const string ScTraceable::TIMEOUT = "Timeout";
    const string ScTraceable::MESSAGE = "Message";
    const string ScTraceable::VERSION = "Version";
    const string ScTraceable::QOS = "QoS";
    const string ScTraceable::FLAG = "Flag";
    const string ScTraceable::EXCEPTION = "EXCEPTION";

    const string ScTraceable::HEADER_VERSION = "HeaderVersion";
    const string ScTraceable::METHOD_RESULT = "MethodResult";
    const string ScTraceable::GRACEFUL = "Graceful";
    const string ScTraceable::MESSAGE_QUEUE_SIZE = "MessageQueueSize";
    const string ScTraceable::MEMORY = "Memory";
    const string ScTraceable::SUSPECT_EVENT = "SuspectEvent";
    const string ScTraceable::FATAL = "Fatal";
    const string ScTraceable::HEADER = "Header";
    const string ScTraceable::STATUS = "Status";
    const string ScTraceable::ADDRESS= "Address";
    const string ScTraceable::MUTEX= "Mutex";
    const string ScTraceable::RETURN_CODE= "ReturnCode";
    const string ScTraceable::REQUEST_ID= "RequestId";
     const string ScTraceable::TOKEN="Token";
    const string ScTraceable::TOKEN_LENGTH="TokenLength";
    const string ScTraceable::INET_ADDRESS = "Inet" + ADDRESS;
     const string ScTraceable::PORT = "Port";
     const string ScTraceable::RMR = "RMReceiver";
    const string ScTraceable::RMT = "RMTransmitter";
     const string ScTraceable::TOPIC_NAME = "TopicName";
    const string ScTraceable::STREAM_ID = "StreamId";
    const string ScTraceable::REJECT_STREAM_REASON = "RejectReason";
    const string ScTraceable::REPORT = "Report";
    const string ScTraceable::RMM_EVENT = "RmmEvent";
    const string ScTraceable::RMM_NODES = "RmmNodes";
    const string ScTraceable::ACK = "Ack";
    const string ScTraceable::DELAYER = "Delayer";
    const string ScTraceable::ACK_GAP = "AckGap";
    const string ScTraceable::ACK_REQUIRED_PACKETS = "AckRequiredPackets";
    const string ScTraceable::FRONT_SEQ_NUM = "FrontSequenceNumber";
    const string ScTraceable::ACKERS_INFO = "AckersInfo";
    const string ScTraceable::LAZY_ACKER = "LazyAcker";

    const string ScTraceable::PARAMS_NUM = "ParamsNumber";
    const string ScTraceable::CONFIGURATION = "Configuration";
    const string ScTraceable::PROPERTIES = "Properties";
    const string ScTraceable::MAP = "Map";
    const string ScTraceable::RMM = "Rmm";
    const string ScTraceable::KEY = "Key";
    const string ScTraceable::SENDERS = "Senders";
    const string ScTraceable::CASHED_STREAMS = "CashedStreams";

    const string ScTraceable::CONFIG_MAP = "ConfigMap";
    const string ScTraceable::PARAMETER_NAME = "ParameterName";
    const string ScTraceable::PARAMETER_VALUE= "ParameterValue";

    const string ScTraceable::EXPECTED= "Expected";
    const string ScTraceable::AVAILABLE= "Available";

    /** Keys for prefixes **/

    const string ScTraceable::INCOMING = "Incoming";
    const string ScTraceable::OUTGOING = "Outgoing";
    const string ScTraceable::DELIVERED = "Delivered";
    const string ScTraceable::SOURCE = "Source";
    const string ScTraceable::UNDEFINED = "Undefined";
    const string ScTraceable::FAILED= "Failed";
    const string ScTraceable::STORED = "Stored";
    const string ScTraceable::READ = "Read";
    
    

    const string ScTraceable::TRUE_STR = "TRUE";
    const string ScTraceable::FALSE_STR = "FALSE";
}
