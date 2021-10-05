/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.test.cli.mq;


/**
 * This class lists the common mq commands to be used.
 *
 */
public class MQCommandReference {
	
	//Commands for queries - note {..} will be substituted at runtime
	public static final String LIST_QMGRS = "{LOC}/dspmq -m {QMGR}";
	public static final String LISTENER_STATUS = "echo \"Display lsstatus({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String LISTENER_EXISTS = "echo \"Display listener({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String CHANNEL_STATUS = "echo \"Display chstatus({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String LIST_CHANNEL = "echo \"Display channel({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String LIST_QUEUE = "echo \"Display qlocal({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String QUEUE_DEPTH = "echo \"display qlocal({NAME}) where(curdepth ge 0)\" | {LOC}/runmqsc {QMGR}";
	public static final String CLEAR_QUEUE = "echo \"clear qlocal({NAME})\" | {LOC}/runmqsc {QMGR}";
	
	//Commands for stopping and starting a qmgr or listener - note {..} will be substituted at runtime	
	public static final String STOP_QMGR = "{LOC}/endmqm {QMGR}";
	public static final String START_QMGR = "{LOC}/strmqm {QMGR}";
	public static final String DIS_CHLAUTH = "echo \"Alter QMGR CHLAUTH(DISABLED)\" | {LOC}/runmqsc {QMGR}";
	public static final String START_LISTENER = "echo \"Start listener({NAME})\" | {LOC}/runmqsc {QMGR}";
	public static final String STOP_CHANNEL = "echo \"Stop channel({NAME}) MODE(FORCE)\" | {LOC}/runmqsc {QMGR}";
	public static final String START_CHANNEL = "echo \"Start channel({NAME})\" | {LOC}/runmqsc {QMGR}";
	
	
	//Commands for object creation - note {..} will be substituted at runtime	
	public static final String CREATE_QMGR = "{LOC}/crtmqm {QMGR}";
	public static final String CREATE_TOPIC = "echo \"Define topic({NAME}) TopicStr({TOPICSTR})\" | {LOC}/runmqsc {QMGR}";
	public static final String CREATE_CHANNEL="echo \"define channel({CHNL}) chltype(SVRCONN) TRPTYPE(TCP) MCAUSER('{MCAUSER}')\" | {LOC}/runmqsc {QMGR}";
	public static final String CREATE_LISTENER="echo \"define listener({NAME}) TRPTYPE(TCP) PORT({PORT}) CONTROL(QMGR)\" | {LOC}/runmqsc {QMGR}";
	public static final String CREATE_QUEUE="echo \"Define qlocal({NAME}) MAXDEPTH(10000)\" | {LOC}/runmqsc {QMGR}";
	
	//Commands for object deletion - note {..} will be substituted at runtime
	public static final String DELETE_QMGR = "{LOC}/dltmqm {QMGR}";
	public static final String DELETE_QUEUE = "echo \"delete qlocal({NAME}) PURGE\" | {LOC}/runmqsc {QMGR}";
	
	//Commands for result strings - note {..} will be substituted at runtime
	public static final String RESULT_LIST = "QMNAME({QMGR})";
	
	//Commands for mqtt service - note {..} will be substituted at runtime
	public static final String MQTT_QUEUE="echo \"DEFINE QLOCAL('SYSTEM.MQTT.TRANSMIT.QUEUE') USAGE(XMITQ) MAXDEPTH(10000000)\" | {LOC}/runmqsc {QMGR}";
	public static final String MQTT_ALTER_QMGR = "echo \"ALTER QMGR DEFXMITQ('SYSTEM.MQTT.TRANSMIT.QUEUE')\" | {LOC}/runmqsc {QMGR}";
	public static final String MQTT_CMD1 = "{LOC}/setmqaut -m {QMGR} -t q -n SYSTEM.MQTT.TRANSMIT.QUEUE -p nobody -all +put";
	public static final String MQTT_CMD2 = "{LOC}/setmqaut -m {QMGR} -t topic -n SYSTEM.BASE.TOPIC -p nobody -all +pub +sub";
	public static final String MQTT_SERVICE_CREATE = "cat {LOC}/../mqxr/samples/installMQXRService_unix.mqsc | {LOC}/runmqsc {QMGR}";
	public static final String MQTT_SERVICE_START = "echo \"START SERVICE(SYSTEM.MQXR.SERVICE)\" |  {LOC}/runmqsc {QMGR}";
	public static final String MQTT_SERVICE_CHANNEL = "echo \"define CHANNEL('PlainText') CHLTYPE(MQTT) PORT({PORT}) MCAUSER('nobody')\" |  {LOC}/runmqsc {QMGR}";
	

	//Commands for setting up mq connectivity channel authentication - note {..} will be substituted at runtime
	public static final String AUTH_CMD_1 = "echo \"SET AUTHREC OBJTYPE(QMGR) PRINCIPAL('{USER}') AUTHADD(CONNECT, INQ, DSP)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_2 = "echo \"SET CHLAUTH({CHANNEL}) TYPE(ADDRESSMAP) ADDRESS(9.*) MCAUSER('{USER}')\"  | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_3 = "echo \"SET CHLAUTH({CHANNEL}) TYPE(ADDRESSMAP) ADDRESS(10.*) MCAUSER('{USER}')\"  | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_4 = "echo \"SET AUTHREC PROFILE(SYSTEM.DEFAULT.MODEL.QUEUE) OBJTYPE(QUEUE) PRINCIPAL('{USER}') AUTHADD(DSP, GET)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_5 = "echo \"SET AUTHREC PROFILE(SYSTEM.ADMIN.COMMAND.QUEUE) OBJTYPE(QUEUE) PRINCIPAL('{USER}') AUTHADD(DSP, PUT)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_6 = "echo \"SET AUTHREC PROFILE(SYSTEM.IMA.*) OBJTYPE(QUEUE) PRINCIPAL('{USER}') AUTHADD(CRT, PUT, GET, BROWSE)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_7 = "echo \"SET AUTHREC PROFILE(SYSTEM.DEFAULT.LOCAL.QUEUE) OBJTYPE(QUEUE) PRINCIPAL('{USER}') AUTHADD(DSP)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_CMD_8 = "echo \"SET AUTHREC PROFILE(SYSTEM.BASE.TOPIC) OBJTYPE(TOPIC) PRINCIPAL('{USER}') AUTHADD(DSP)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_TOPIC = "echo \"SET AUTHREC PROFILE({TOPIC}) OBJTYPE(TOPIC) PRINCIPAL('{USER}') AUTHADD(ALL)\" | {LOC}/runmqsc {QMGR}";
	public static final String AUTH_QUEUE = "echo \"SET AUTHREC PROFILE({QUEUE}) OBJTYPE(QUEUE) PRINCIPAL('{USER}') AUTHADD(ALL)\" | {LOC}/runmqsc {QMGR}";
	
	// misc commands
	public static final String IS_RETAINED = "echo \"display tpstatus({TOPICSTRING})\" | {LOC}/runmqsc {QMGR}";
	public static final String CLEAR_RETAINED = "echo \"clear topicstr({TOPICSTRING}) clrtype(Retained)\"| {LOC}/runmqsc {QMGR}";

}
