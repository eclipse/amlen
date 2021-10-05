/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
#ifndef IOTMONITOR_H_DEFINED
#define IOTMONITOR_H_DEFINED

/* Fields that appear in the monitoring messages */
#define MONITOR_MSG_VALUE_ACTION      "Action"
#define MONITOR_MSG_VALUE_TIME        "Time"
#define MONITOR_MSG_VALUE_CLIENTADDR  "ClientAddr"
#define MONITOR_MSG_VALUE_CLIENTID    "ClientID"
#define MONITOR_MSG_VALUE_PORT        "Port"
#define MONITOR_MSG_VALUE_SECURE      "Secure"
#define MONITOR_MSG_VALUE_PROTOCOL    "Protocol"
#define MONITOR_MSG_VALUE_USER        "User"
#define MONITOR_MSG_VALUE_CERTNAME    "CertName"
#define MONITOR_MSG_VALUE_CONNECTTIME "ConnectTime"
#define MONITOR_MSG_VALUE_CLOSECODE   "CloseCode"
#define MONITOR_MSG_VALUE_REASON      "Reason"
#define MONITOR_MSG_VALUE_DURABLE     "Durable"
#define MONITOR_MSG_VALUE_NEWSESSION  "NewSession"
#define MONITOR_MSG_VALUE_READBYTES   "ReadBytes"
#define MONITOR_MSG_VALUE_READMSG     "ReadMsg"
#define MONITOR_MSG_VALUE_WRITEBYTES  "WriteBytes"
#define MONITOR_MSG_VALUE_WRITEMSG    "WriteMsg"

/* Valid values for MONTIOR_MSG_VALUE_ACTION */
#define MONITOR_MSG_ACTION_CONNECT       "Connect"
#define MONITOR_MSG_ACTION_DISCONNECT    "Disconnect"
#define MONITOR_MSG_ACTION_FAILEDCONNECT "FailedConnect"
#define MONITOR_MSG_ACTION_ACTIVE        "Active"
#define MONITOR_MSG_ACTION_INFO          "Info"
#define MONITOR_MSG_ACTION_UNKNOWN       "Unknown"

/* Number of days after which monitoring messages will expire */
#define MONITOR_MSG_DEFAULT_EXPIRATION_DAYS 45

#define RECONCILE_CONFIG_NAME "MqttProxyMonitoring"

#endif /* IOTMONITOR_H_DEFINED */
