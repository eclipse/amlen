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

/*
 * Macro to create log categories.
 * This same description is used to create the name and index lookup tables
 */
#ifndef LOGCAT_SPECIAL
#define LOGCAT_(name, val) LOGCAT_##name = val,
enum ismLogCategories {
#endif

LOGCAT_(Server      		,  1)      /* Server startup and shutdown */
LOGCAT_(Exception   		,  2)      /* Exception handling          */
LOGCAT_(Transport   		,  3)      /* Transport other than connections */
LOGCAT_(Connection  		,  4)      /* Entries normally written to the connection log */
LOGCAT_(Security    		,  5)      /* Entries normally written to the security log  */
LOGCAT_(Messaging   		,  6)      /* Messaging */
LOGCAT_(Protocol    		,  7)      /* Protocol component */
LOGCAT_(Engine    			,  8)      /* Engine component */
LOGCAT_(Admin       		,  9)      /* Administration */
LOGCAT_(Config      		, 10)      /* Changes to config (might be written to security log) */
LOGCAT_(Store       		, 11)      /* Message store and disk */
LOGCAT_(Monitoring  		, 12)      /* System monitoring */
LOGCAT_(MQConnectivity    	, 13)      /* MQ Connectivity component */
LOGCAT_(Kafka    		, 14)      /* Kafka component */

#ifndef LOGCAT_SPECIAL
};
#endif
