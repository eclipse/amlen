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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : ActionFactory.java
 * Author      : 
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import org.w3c.dom.Element;

//import com.ibm.ism.test.RmmUtils.RmmObjectType;
/**
 * This class is a factory for all actions.
 * <p>
 * 
 */
enum ActionType {
	TestAction, CompositeAction, SyncAction, SyncComponentAction, ShellAction, MessageGenerationAction,
	CreateConnection, CloseConnection, CreateMessage, CreateRandomMessage, ReceiveMessage,SendMessage,
	CompareMessage, Subscribe, Unsubscribe, CheckMessagesReceived, CheckConnectOptions, CompareMessageData, CheckConnection,
	CreateRandomString, CreateSSLProperties, CheckPendingDeliveryTokens, WaitPendingDelivery,
	WaitForReconnection, CreateStoreInteger, CreateStoreString, CreateStoreNumberString,
	SetStoreInteger, DeleteAllRetainedMessages, CreateMonitorRecord, Sleep,
	CheckMonitorRecordCounts, CheckMonitorRecordValues, GetLTPAToken, DecodeLTPAToken,
	Reconnect, GetRetainedMessage, DeleteRetainedMessage, PublishTopicTree, SubscribeTopicTree,
	UnsubscribeTopicTree, RestAPI, CompareREST, GetServerUID, CompareClusterStats, PublishLoop,
	CreateHttpConnection, HttpRequest, CloseHttpConnection, CreateConnectionKafka, SubscribeKafka, ReceiveMessageKafka, CompareMessageDataKafka, CloseConnectionKafka
}

final class ActionFactory {

	private ActionFactory() {
	}

	static Action createAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		ActionType actionType = Enum.valueOf(ActionType.class,
				config.getAttribute("type"));
		switch (actionType) {
		case TestAction:
			return new TestAction(config, dataRepository, trWriter);
		case CompositeAction:
			return new CompositeAction(config, dataRepository, trWriter);
		case SyncAction:
			return new SyncAction(config, dataRepository, trWriter);
		case SyncComponentAction:
			return new SyncComponentAction(config, dataRepository, trWriter);
		case ShellAction:
			return new ShellAction(config, dataRepository, trWriter);
		case MessageGenerationAction:
			return new MessageGenerationAction(config, dataRepository, trWriter);
		case CreateConnection:
			return new CreateConnectionAction(config, dataRepository, trWriter);
		case CloseConnection:
			return new CloseConnectionAction(config, dataRepository, trWriter);
		case CreateMessage:
			return new CreateMessageAction(config, dataRepository, trWriter);
		case CreateRandomMessage:
			return new CreateRandomMessageAction(config, dataRepository, trWriter);
		case CompareMessage:
			return new CompareMessageAction(config, dataRepository, trWriter);
		case CompareMessageData:
			return new CompareMessageDataAction(config, dataRepository, trWriter);
		case ReceiveMessage:
			return new ReceiveMessageAction(config, dataRepository, trWriter);
		case SendMessage:
			return new SendMessageAction(config, dataRepository, trWriter);
		case Subscribe:
			return new SubscribeAction(config, dataRepository, trWriter);
		case Unsubscribe:
			return new UnsubscribeAction(config, dataRepository, trWriter);
		case CheckConnection:
			return new CheckConnectionAction(config, dataRepository, trWriter);
		case CheckConnectOptions:
			return new CheckConnectOptionsAction(config, dataRepository, trWriter);
		case CheckMessagesReceived:
			return new CheckMessagesReceivedAction(config, dataRepository, trWriter);
		case CreateRandomString:
			return new CreateRandomStringAction(config, dataRepository, trWriter);
		case CreateSSLProperties:
			return new CreateSSLPropertiesAction(config, dataRepository, trWriter);
		case CheckPendingDeliveryTokens:
			return new CheckPendingDeliveryTokensAction(config, dataRepository, trWriter);
		case WaitPendingDelivery:
			return new WaitPendingDeliveryAction(config, dataRepository, trWriter);
		case WaitForReconnection:
			return new WaitForReconnectionAction(config, dataRepository, trWriter);
		case CreateStoreInteger:
			return new CreateStoreIntegerAction(config, dataRepository, trWriter);
		case CreateStoreString:
			return new CreateStoreStringAction(config, dataRepository, trWriter);
		case CreateStoreNumberString:
			return new CreateStoreNumberStringAction(config, dataRepository, trWriter);
		case SetStoreInteger:
			return new SetStoreIntegerAction(config, dataRepository, trWriter);
		case DeleteAllRetainedMessages:
			return new DeleteAllRetainedMessagesAction(config, dataRepository, trWriter);
		case CreateMonitorRecord:
			return new CreateMonitorRecordAction(config, dataRepository, trWriter);
		case Sleep:
			return new SleepAction(config, dataRepository, trWriter);
		case CheckMonitorRecordCounts:
			return new CheckMonitorRecordCountsAction(config, dataRepository, trWriter);
		case CheckMonitorRecordValues:
			return new CheckMonitorRecordValuesAction(config, dataRepository, trWriter);
		case GetLTPAToken:
			return new GetLTPATokenAction(config, dataRepository, trWriter);
		case DecodeLTPAToken:
			return new DecodeLTPATokenAction(config, dataRepository, trWriter);
		case Reconnect:
			return new ReconnectAction(config, dataRepository, trWriter);
		case GetRetainedMessage:
			return new GetRetainedMessageAction(config, dataRepository, trWriter);
		case DeleteRetainedMessage:
			return new DeleteRetainedMessageAction(config, dataRepository, trWriter);
		case PublishTopicTree:
			return new PublishTopicTreeAction(config, dataRepository, trWriter);
		case SubscribeTopicTree:
			return new SubscribeTopicTreeAction(config, dataRepository, trWriter);
		case UnsubscribeTopicTree:
			return new UnsubscribeTopicTreeAction(config, dataRepository, trWriter);
		case RestAPI:
			return new RestAPIAction(config, dataRepository, trWriter);
		case CompareREST:
			return new CompareRESTAction(config, dataRepository, trWriter);
		case GetServerUID:
			return new GetServerUIDAction(config, dataRepository, trWriter);
		case CompareClusterStats:
			return new CompareClusterStatsAction(config, dataRepository, trWriter);
		case PublishLoop:
			return new PublishLoopAction(config, dataRepository, trWriter);
		case CreateHttpConnection:
	        return new CreateHttpConnectionAction(config, dataRepository, trWriter);
		case CloseHttpConnection:
            return new CloseHttpConnectionAction(config, dataRepository, trWriter);
		case HttpRequest:
	        return new HttpRequestAction(config, dataRepository, trWriter);
		case CreateConnectionKafka:
			return new CreateConnectionKafkaAction(config, dataRepository, trWriter);
		case SubscribeKafka:
			return new SubscribeKafkaAction(config, dataRepository, trWriter);
		case ReceiveMessageKafka:
			return new ReceiveMessageKafkaAction(config, dataRepository, trWriter);
		case CompareMessageDataKafka:
			return new CompareMessageDataKafkaAction(config, dataRepository, trWriter);
		case CloseConnectionKafka:
			return new CloseConnectionKafkaAction(config, dataRepository, trWriter);
		default:
			throw new RuntimeException("Action type " + actionType
					+ " is not supported.");
		}
	}

}
