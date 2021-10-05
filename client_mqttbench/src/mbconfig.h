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

#ifndef __MBCONFIG_H
#define __MBCONFIG_H

#include "mqttbench.h"

#define PROPS_BUF_DFLT_SIZE 64

/*
 * mqttbench client list numeric values for JSON entry names
 */
typedef enum mbc_fields_e {
	MB_id        					= 0,
	MB_cleanSession    				= 1,
	MB_dst       					= 2,
	MB_dstPort						= 3,
	MB_lingerTimeSecs				= 4,
	MB_password						= 5,
	MB_publishTopics				= 6,
	MB_qos							= 7,
	MB_retain						= 8,
	MB_topicStr						= 9,
	MB_reconnectDelayUSecs			= 10,
	MB_subscriptions				= 11,
	MB_useTLS						= 12,
	MB_username						= 13,
	MB_version						= 14,
	MB_clientCertPath				= 15,
	MB_clientKeyPath				= 16,
	MB_messageDirPath				= 17,
	MB_lastWillMsg					= 18,

	// MQTT v5 fields
	MB_cleanStart					= 19,
	MB_sessionExpiryIntervalSecs	= 20,
	MB_recvMaximum					= 21,
	MB_maxPktSizeBytes				= 22,
	MB_topicAliasMaxIn				= 23,
	MB_requestResponseInfo			= 24,
	MB_requestProblemInfo			= 25,
	MB_userProperties				= 26,

	// Message Object Fields
	MB_path							= 27,
	MB_payload						= 28,
	MB_payloadSizeBytes				= 29,
	MB_payloadFile					= 30,
	MB_topic						= 31,
	MB_contentType					= 32,
	MB_expiryIntervalSecs			= 33,
	MB_responseTopicStr				= 34,
	MB_correlationData				= 35,

	// User Properties Fields
	MB_name							= 36,
	MB_value						= 37,

	// Topic Fields
	MB_noLocal						= 38,
	MB_retainAsPublished			= 39,
	MB_retainHandling				= 40,

	MB_topicAliasMaxOut				= 41,
	MB_lingerMsgsCount              = 42,

	MB_connectionTimeoutSecs        = 43
} mbc_fields_e;

int processConfig(mqttbenchInfo_t *mqttbenchInfo);
int processMessageDir(const char *msgDirPath, mqttbenchInfo_t *mqttbenchInfo);

#endif /* __MBCONFIG_H_ */
