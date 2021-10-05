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

/*
 * Note: this header file defines the interface of MqttClient for MessageSight SNMP service.
 */

#ifndef _IMASNMPMQTTCLIENT_H_
#define _IMASNMPMQTTCLIENT_H_

#include <monitoringSnmpTrap.h>
/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

#ifndef _SNMP_THREADED_
int imaSnmpMqtt_topic_handler_register(int topicType, imaSnmpEvent_msgHandler_t msgHandler);
int imaSnmpMqtt_subscribe(int topicType);
int imaSnmpMqtt_unsubscribe(int topicType);
int imaSnmpMqtt_init();
void imaSnmpMqtt_deinit();
int imaSnmpMqtt_isConnected();
#endif

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPMQTTCLIENT_H_ */

