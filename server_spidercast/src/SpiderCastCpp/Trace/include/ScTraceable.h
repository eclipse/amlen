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
 * File        : ScTraceable.h
 * Author      :
 * Version     : $Revision: 1.5 $
 * Last updated: $Date: 2015/11/18 08:37:00 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceable.h,v $
 * Revision 1.5  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.4  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.3  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.2  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.1  2010/05/16 07:13:34 
 * Tracer - first version
 *
 *
 * Function: Traceable components declaration
 */
#ifndef SCTRACEABLE_H
#define SCTRACEABLE_H


//#include "Defs.h"
#include <string>


namespace spdr
{

class  ScTraceable {
public: 
    ScTraceable(){}
    ScTraceable(const ScTraceable& scTraceable) {}
    virtual ~ScTraceable(){}

    /**
     * Key names for common SC objects that are not {@link ScTraceable}
     * To be used with {@link ScTraceBuffer#addProperty(std::string, Object)}.
     */

    /** key for name of a member */
    static const std::string MEMBER_NAME;

    
    /** key for an array of members (std::string[]) */
    static const std::string VIEW_MEMBERS;
    static const std::string FAILED_MEMBERS;

    static const std::string VIEW_SIZE;

    
    /** key for ScTraceables */
    static const std::string VIEW_ID;
    
    /** Keys for ExteranlEvents **/
    static const std::string OBJECTS;
    static const std::string OBJECT;
    static const std::string CLASS_NAME;
    static const std::string NLSKEY;
    static const std::string NLS_PARAMS;
    static const std::string SEVERITY;
    static const std::string RECOMMENDED_ACTION;
    static const std::string STACK_NAME;
    static const std::string REASON;
    static const std::string METHOD;
    static const std::string REASON_EXCEPTION;
    static const std::string INTERNAL_DETAILS;
    static const std::string ACTION_TYPE;
    static const std::string ADDED_MEMBERS;
    static const std::string SUSPECTED_MEMBERS;
    static const std::string REMOVED_MEMBERS;
    static const std::string MESSAGE_TYPE;
    static const std::string TYPE_CODE;
    static const std::string EXTERNAL_EVENT;
    static const std::string INTERNAL_EVENT;
    static const std::string TARGET;
    static const std::string THREAD_NAME;
    static const std::string TIMEOUT;
    static const std::string MESSAGE;
    static const std::string VERSION;
    static const std::string QOS;
    static const std::string FLAG;
    static const std::string EXCEPTION;

    static const std::string HEADER_VERSION;
    static const std::string METHOD_RESULT;
    static const std::string GRACEFUL;
    static const std::string MESSAGE_QUEUE_SIZE;
    static const std::string MEMORY;
    static const std::string SUSPECT_EVENT;
    static const std::string FATAL;
    static const std::string HEADER;
    static const std::string STATUS;
    static const std::string ADDRESS;
    static const std::string MUTEX;
    static const std::string RETURN_CODE;
    static const std::string REQUEST_ID;
    static const std::string TOKEN;
    static const std::string TOKEN_LENGTH;
    static const std::string INET_ADDRESS;
    static const std::string PORT;
    static const std::string RMR;
    static const std::string RMT;
    static const std::string TOPIC_NAME;
    static const std::string STREAM_ID;
    static const std::string REJECT_STREAM_REASON;
    static const std::string REPORT;
    static const std::string RMM_EVENT;
    static const std::string RMM_NODES;
    static const std::string ACK;
    static const std::string DELAYER;
    static const std::string ACK_GAP;
    static const std::string ACK_REQUIRED_PACKETS;
    static const std::string FRONT_SEQ_NUM;
    static const std::string ACKERS_INFO;
    static const std::string LAZY_ACKER;
    static const std::string FW;
    static const std::string QTH_ASNYC_STATUS;
    static const std::string PARAMS_NUM;
    static const std::string CONFIGURATION;
    static const std::string PROPERTIES;
    static const std::string MAP;
    static const std::string RMM;
    static const std::string KEY;
    static const std::string SENDERS;
    static const std::string CASHED_STREAMS;
    static const std::string CONFIG_MAP;
    static const std::string PARAMETER_NAME;
    static const std::string PARAMETER_VALUE;
    static const std::string EXPECTED;
    static const std::string AVAILABLE;

    /** Keys for prefixes **/
    static const std::string INCOMING;
    static const std::string OUTGOING;
    static const std::string DELIVERED;
    static const std::string SOURCE;
    static const std::string UNDEFINED;
    static const std::string FAILED;
    static const std::string STORED;
    static const std::string READ;
    
    
    /**
    * std::strings to print boolean values
    */
    static const std::string TRUE_STR;
    static const std::string FALSE_STR;


    /**
     * Class name to be logged when writing as name,value pair.
     * 
     * @return name of class to be used in trace 
     */
    virtual const std::string& getTraceName(void) const = 0;

    virtual std::string  toString(void) const = 0;
};

}

#endif
