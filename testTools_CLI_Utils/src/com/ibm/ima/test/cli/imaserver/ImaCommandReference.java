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
package com.ibm.ima.test.cli.imaserver;

/**
 * This class lists the common imaserver commands to be used.
 *
 */
public class ImaCommandReference {
	
	
	//Commands for users - note {..} will be substituted at runtime
	public static final String USER_ADD="imaserver user add \"UserID={ID}\" \"Type={TYPE}\" \"Password={PASSWORD}\" \"GroupMembership={MEMBER}\" \"Description={DESC}\"";
	public static final String USER_EXISTS="imaserver user list \"UserID={ID}\" \"Type={TYPE}\"";
	public static final String EDIT_USER_PASSWORD="imaserver user edit \"UserID={ID}\" \"Type={TYPE}\" \"Password={PASSWORD}\"";
	public static final String EDIT_USER_GROUP="imaserver user edit \"UserID={ID}\" \"Type={TYPE}\" \"{ACTION}={GROUP}\"";
	public static final String USER_DELETE="imaserver user delete \"UserID={ID}\" \"Type={TYPE}\"";
	public static final String GROUP_ADD="imaserver group add \"GroupID={ID}\" \"Description={DESC}\"";
	public static final String GROUP_ADD_MEMBER="imaserver group add \"GroupID={ID}\" \"GroupMembership={MEMBER}\" \"Description={DESC}\"";
	public static final String GROUP_EDIT_MEMBER="imaserver group edit \"GroupID={ID}\" \"{ACTION}={GROUP}\"";
	public static final String GROUP_EXISTS="imaserver group list \"GroupID={ID}\" \"Type={TYPE}\"";
	public static final String GROUP_DELETE="imaserver group delete \"GroupID={ID}\"";


	//Misc commands
	public static final String SUB_DELETE="imaserver delete Subscription \"SubscriptionName={NAME}\" \"ClientID={ID}\"";
	public static final String MQTTCLIENT_DELETE="imaserver delete MQTTClient \"Name={ID}\"";
	
	//Commands for server status
	public static final String SHOW_SERVER_COMMAND="show imaserver";
	public static final String STATUS_COMMAND="status imaserver";
	public static final String STOP_COMMAND="imaserver stop";
	public static final String STOP_FORCE_COMMAND="imaserver stop force";
	public static final String START_COMMAND="imaserver start";
	public static final String STOP_MQCONN="imaserver stop mqconnectivity";
	public static final String STOP_FORCE_MQCONN="imaserver stop mqconnectivity force";
	public static final String MQCONN_STATUS_COMMAND="status mqconnectivity";
	public static final String START_MQCONN_COMMAND="imaserver start mqconnectivity";
	public static final String HAROLE="imaserver harole";
	
	//Commands for debug - note {..} will be substituted at runtime
	public static final String MUST_GATHER="platform must-gather {MUST_GATHER}";
	public static final String STACK_TRACE="advanced-pd-options dumpstack {COMPONENT}";
	public static final String DUMP_CORE="advanced-pd-options dumpcore {COMPONENT}";
	public static final String GET_FILE_LIST="advanced-pd-options list";

	//Commands for config manipulation - note {..} will be substituted at runtime
	public static final String SHOW_COMMAND="imaserver show {TYPE} \"Name={NAME}\"";
	public static final String DELETE_COMMAND="imaserver delete {TYPE} \"Name={NAME}\"";
	public static final String UPDATE_COMMAND="imaserver update {TYPE} \"Name={NAME}\" \"{PARAM}={VALUE}\"";

	//Commands for message policy manipulation - note {..} will be substituted at runtime
	public static final String MSGPOLICY_CREATE="imaserver create MessagingPolicy \"Name={NAME}\" \"Description={DESC}\" \"Destination={DEST}\" \"DestinationType={DESTTYPE}\" \"ActionList={ACTION}\" \"Protocol={PROTYPE}\"";
	public static final String MSGPOLICY_QUEUEACTION="Send,Receive,Browse";
	public static final String MSGPOLICY_TOPICACTION="Publish,Subscribe";
	public static final String MSGPOLICY_SUBSCRIPTIONACTION="Receive,Control";

	//Commands for connection policy manipulation - note {..} will be substituted at runtime
	public static final String CONNPOLICY_CREATE="imaserver create ConnectionPolicy \"Name={NAME}\" \"Description={DESC}\" \"Protocol={PROTYPE}\"";

	//Commands for Endpoint manipulation - note {..} will be substituted at runtime
	public static final String ENDPOINT_CREATE="imaserver create Endpoint \"Name={NAME}\" \"Description={DESC}\" \"Port={PORT}\" \"Protocol={PROTYPE}\"  \"ConnectionPolicies={CONNPOL}\" \"MessagingPolicies={MSGPOL}\" \"MessageHub={HUB}\"";

	//Commands for MessageHub manipulation - note{..} will be substituted at runtime
	public static final String HUB_CREATE="imaserver create MessageHub \"Name={NAME}\" \"Description={DESC}\"";

	//Commands for queue manipulation - note{..} will be substituted at runtime
	public static final String QUEUE_CREATE="imaserver create Queue \"Name={NAME}\" \"Description={DESC}\" \"MaxMessages={MSGS}\"";
	public static final String QUEUE_DELETE="imaserver delete Queue \"Name={NAME}\" \"DiscardMessages={DISCARD}\"";
	
	// Commands for monitoring
	public static final String QUEUE_STAT="imaserver stat Queue";
	public static final String QUEUE_STAT_Q="imaserver stat Queue \"QueueName={NAME}\"";
	public static final String CONN_STAT="imaserver stat Connection";
	public static final String CONN_STAT_EP="imaserver stat Connection \"Endpoint={NAME}\"";
	public static final String TOPIC_STAT="imaserver stat Topic";
	public static final String TOPIC_STAT_T="imaserver stat Topic  \"TopicString={NAME}\"";
	public static final String MQTT_STAT="imaserver stat MQTTClient";
	public static final String MQTT_STAT_C="imaserver stat MQTTClient \"ClientID={NAME}\"";
	public static final String EP_STAT="imaserver stat Endpoint";
	public static final String EP_STAT_E="imaserver stat Endpoint \"Name={NAME}\"";
	public static final String MAPPING_STAT="imaserver stat DestinationMappingRule";
	public static final String MAPPING_STAT_RL="imaserver stat DestinationMappingRule \"Name={NAME}\"";
	public static final String SUB_STAT="imaserver stat Subscription ResultCount=100";
	public static final String SUB_STAT_T="imaserver stat Subscription \"TopicString={NAME}\" ResultCount=100";
	public static final String SUB_STAT_C="imaserver stat Subscription \"ClientID={NAME}\" ResultCount=100";
	public static final String STORE_STAT="imaserver stat Store";
	public static final String MEMORY_STAT="imaserver stat Memory";
	public static final String SERVER_STAT="imaserver stat Server";
	public static final String CREATE_TOPICMON = "imaserver create TopicMonitor \"TopicString={TOPIC}\"";
	public static final String DELETE_TOPICMON = "imaserver delete TopicMonitor \"TopicString={TOPIC}\"";

	//Commands used for additional selection parameters
	public static final String CLIENTID="ClientID";
	public static final String USERID="UserID";
	public static final String GROUPID="GroupID";
	public static final String COMMON="CommonNames";
	public static final String ADDRESS="ClientAddress";
	public static final String MAXMSG="MaxMessages";
	public static final String ENABLED="Enabled";
	public static final String INTERFACE="Interface";
	public static final String SECURITY="SecurityProfile";
	public static final String MAXMSGSIZE="MaxMessageSize";
	public static final String ALLOWSEND="AllowSend";
	public static final String CONCURRENT="ConcurrentConsumers";

	//Commands for mqconnectivity
	public static final String QMGRCONN_CREATE="imaserver create QueueManagerConnection \"Name={NAME}\" \"QueueManagerName={QMGRNAME}\" \"ConnectionName={CONNAME}\" \"ChannelName={CHLNAME}\"";
	public static final String MAPPING_CREATE="imaserver create DestinationMappingRule \"Name={NAME}\" \"QueueManagerConnection={QMGRCONN}\" \"RuleType={RULETYPE}\" \"Source={SOURCE}\" \"Destination={DEST}\"";

	//Commands used for additional mq parameters
	public static final String SSL_MQ="SSLCipherSpec";
	public static final String MAXMSG_MQ="MaxMessages";
	public static final String ENABLED_MQ="Enabled";
	
	//LDAP commands - note these are not complete command reference
	public static final String CREATE_LDAP="imaserver create LDAP";
	public static final String UPDATE_LDAP="imaserver update LDAP";
	public static final String DELETE_LDAP="imaserver delete LDAP";
	public static final String SHOW_LDAP="imaserver show LDAP";
	public static final String TEST_LDAP="imaserver test LDAP";

	//Return text
	public static final String NOT_FOUND="is not found";
	public static final String NO_STAT_FOUND="No {NAME} data is found";
	public static final String NO_USER_FOUND="The user \"{USERID}\" does not exist";
	public static final String NO_GROUP_FOUND="The group \"{GROUPID}\" does not exist";
	
	

}
