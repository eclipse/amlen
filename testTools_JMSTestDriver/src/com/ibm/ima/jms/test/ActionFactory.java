/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
 * System Name : MQ Low Latency Messaging
 * File        : ActionFactory.java
 * Author      : 
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.JmsMessageUtils.JmsActionType;
import com.ibm.ima.jms.test.jca.GetTestPropsMsgAction;
/**
 * This class is a factory for all actions.
 * <p>
 * 
 */
enum ActionType {
    CheckIntVariable,
    CheckIsmProps,
    ClearMsg, 
    CloseConnection,
    CloseConsumer,
    CloseProducer,
    CloseQueueBrowser,
    CloseSession, 
    CompareMessage,
    CompositeAction,
    CreateConnection,
    CreateConnectionFactory,
    CreateConsumer, 
    CreateDestination,
    CreateDurableConsumer,
    CreateDurableSubscriber,
    CreateExceptionListener, 
    CreateMessage, 
    CreateTestPropsMessage,
    CreateMessageListener,
    CreateProducer, 
    CreateQueueBrowser, 
    CreateRandomMessage, 
    CreateSession, 
    CreateSharedConsumer,
    CreateSharedDurableConsumer,
    CreateTemporaryQueue, 
    CreateTemporaryTopic, 
    CreateTokenBucket,
    FillIsmProps, 
    FuzzTest,
    GetBrowserEnumeration, 
    GetMessageBodyLength,
    GetMessageMapItem,
    GetMessageObject,
    GetMessageProperty, 
    GetMessageText, 
    GetReplyToDest,
    GetTestPropsMsg,
    GetOAuthAccessToken,
    HttpAction,
    MessageGenerationAction,
    MsgAcknowledge,
    MsgReset,
    OpenStream,
    PollBrowser, 
    PseudoTransTest,
    ReadBytesMessage,
    ReadStreamMessage,
    ReceiveMessage,
    ReceiveMessageCRC,
    ReceiveMessageLoop,
    RecoverSession,
    SendMessage,
    SendMessageCRC,
    SendMessageLoop,
    SessionCommit,
    SessionRollback,
    SetMessageListener,
    SetMessageMapItem,
    SetMessageObject,
    SetMessageProperty,
    SetMessageText,
    ShellAction,
    StartConnection,
    StopConnection,
    StoreInJndi,
    StoreMessages, 
    SyncAction,
    SyncComponentAction,
    Unsubscribe,
    WriteBytesMessage,
    WriteStreamMessage,
    TestAction,
    RestAPI,
    CompareREST,
    GetServerUID
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
        case FuzzTest:
        	return new FuzzTestAction(config, dataRepository, trWriter);
        case SyncAction:
            return new SyncAction(config, dataRepository, trWriter);
        case SyncComponentAction:
            return new SyncComponentAction(config, dataRepository, trWriter);
        case ShellAction:
            return new ShellAction(config, dataRepository, trWriter);
        case GetOAuthAccessToken:
            return new GetOAuthAccessToken(config, dataRepository, trWriter);
        case HttpAction:
            return new HttpAction(config, dataRepository, trWriter);
        case CreateDurableConsumer:
        	return new CreateDurableConsumerAction(config, dataRepository, trWriter);
        case CreateSharedConsumer:
        	return new CreateSharedConsumerAction(config, dataRepository, trWriter);
        case CreateSharedDurableConsumer:
        	return new CreateSharedDurableConsumerAction(config, dataRepository, trWriter);
        case MessageGenerationAction:
            return new MessageGenerationAction(config, dataRepository, trWriter);
        case FillIsmProps:
            return new FillIsmPropsAction(config, dataRepository, trWriter);
        case CreateConnectionFactory:
            return new CreateConnectionFactoryAction(config, dataRepository,trWriter);
        case CreateTestPropsMessage:
            return new CreateTestPropsMessageAction(config, dataRepository, trWriter);
        case CreateConnection:
            return new CreateConnectionAction(config, dataRepository, trWriter);
        case CloseConnection:
            return new CloseConnectionAction(config, dataRepository, trWriter);
        case StartConnection:
            return new ConnectionStartAction(config, dataRepository, trWriter);
        case StopConnection:
            return new ConnectionStopAction(config, dataRepository, trWriter);
        case CreateDestination:
            return new CreateDestinationAction(config, dataRepository, trWriter);
        case CreateSession:
            return new CreateSessionAction(config, dataRepository,trWriter);
        case CloseSession:
            return new CloseSessionAction(config, dataRepository,trWriter);
        case CloseQueueBrowser:
            return new CloseQueueBrowserAction(config, dataRepository, trWriter);
        case RecoverSession:
            return new RecoverSessionAction(config, dataRepository, trWriter);
        case SessionCommit:
            return new SessionCommitAction(config, dataRepository, trWriter);
        case SessionRollback:
            return new SessionRollbackAction(config, dataRepository, trWriter);
        case CreateProducer:
            return new CreateProducerAction(config, dataRepository, trWriter);
        case CreateConsumer:
            return new CreateConsumerAction(config, dataRepository, trWriter);
        case CreateQueueBrowser:
            return new CreateQueueBrowserAction(config, dataRepository, trWriter);
        case CreateExceptionListener:
            return new CreateExceptionListenerAction(config, dataRepository,trWriter);
        case CreateMessageListener:
            return new CreateMessageListenerAction(config, dataRepository,trWriter);
        case CreateTemporaryTopic:
            return new CreateTemporaryTopicAction(config, dataRepository, trWriter);
        case CreateTemporaryQueue:
            return new CreateTemporaryQueueAction(config, dataRepository, trWriter);
        case GetBrowserEnumeration:
            return new GetBrowserEnumerationAction(config, dataRepository, trWriter);
        case SetMessageProperty:
            return new GetSetMessagePropertyAction(config, JmsActionType.SET, dataRepository, trWriter);
        case GetMessageProperty:
            return new GetSetMessagePropertyAction(config, JmsActionType.GET, dataRepository, trWriter);
        case SetMessageText:
            return new GetSetMessageTextAction(config, JmsActionType.SET, dataRepository, trWriter);
        case GetMessageText:
            return new GetSetMessageTextAction(config, JmsActionType.GET, dataRepository, trWriter);
        case GetTestPropsMsg:
            return new GetTestPropsMsgAction(config, dataRepository, trWriter);
        case CompareMessage:
            return new CompareMessageAction(config, dataRepository, trWriter);
        case SetMessageObject:
            return new GetSetMessageObjectAction(config, JmsActionType.SET, dataRepository, trWriter);
        case GetMessageObject:
            return new GetSetMessageObjectAction(config, JmsActionType.GET, dataRepository, trWriter);
        case SetMessageMapItem:
            return new GetSetMessageMapAction(config, JmsActionType.SET, dataRepository, trWriter);
        case GetMessageMapItem:
            return new GetSetMessageMapAction(config, JmsActionType.GET, dataRepository, trWriter);
        case WriteBytesMessage:
            return new ReadWriteBytesMessageAction(config, JmsActionType.SET, dataRepository, trWriter);
        case ReadBytesMessage:
            return new ReadWriteBytesMessageAction(config, JmsActionType.GET, dataRepository, trWriter);
        case WriteStreamMessage:
            return new ReadWriteStreamMessageAction(config, JmsActionType.SET, dataRepository, trWriter);
        case ReadStreamMessage:
            return new ReadWriteStreamMessageAction(config, JmsActionType.GET, dataRepository, trWriter);
        case GetMessageBodyLength:
            return new GetMessageBodyLengthAction(config, JmsActionType.GET, dataRepository, trWriter);
        case GetReplyToDest:
            return new GetReplyToDestAction(config, dataRepository, trWriter);
        case CreateMessage:
            return new CreateMessageAction(config, dataRepository, trWriter);
        case CreateRandomMessage:
            return new CreateRandomMessageAction(config, dataRepository, trWriter);
        case ClearMsg:
            return new ClearMsgAction(config, dataRepository, trWriter);
        case MsgAcknowledge:
            return new MsgAcknowledgeAction(config, dataRepository, trWriter);
        case MsgReset:
            return new MsgResetAction(config, dataRepository, trWriter);
        case PollBrowser:
            return new PollBrowserAction(config, dataRepository, trWriter);
        case PseudoTransTest:
            return new PseudoTranslationTester(config, dataRepository, trWriter);
        case ReceiveMessage:
            return new ReceiveMessageAction(config, dataRepository, trWriter);
        case ReceiveMessageCRC:
            return new ReceiveMessageCRCAction(config, dataRepository, trWriter);
        case ReceiveMessageLoop:
            return new ReceiveMessageLoopAction(config, dataRepository, trWriter);
        case SendMessage:
            return new SendMessageAction(config, dataRepository, trWriter);
        case SendMessageLoop:
            return new SendMessageLoopAction(config, dataRepository, trWriter);
        case SendMessageCRC:
            return new SendMessageCRCAction(config, dataRepository, trWriter);
        case SetMessageListener:
            return new SetMessageListenerAction(config, dataRepository, trWriter);
        case CreateTokenBucket:
            return new CreateTokenBucketAction(config, dataRepository, trWriter);
        case OpenStream:
            return new OpenStreamAction(config, dataRepository, trWriter);
        case Unsubscribe:
            return new UnsubscribeAction(config, dataRepository, trWriter);
        case CloseProducer:
            return new CloseProducerAction(config, dataRepository,trWriter);
        case CloseConsumer:
            return new CloseConsumerAction(config, dataRepository,trWriter);
        case CheckIsmProps:
            return new CheckIsmPropsAction(config, dataRepository,trWriter);
        case StoreInJndi:
            return new StoreInJndiAction(config, dataRepository,trWriter);
        case CreateDurableSubscriber:
            return new CreateDurableSubscriberAction(config, dataRepository, trWriter);
        case CheckIntVariable:
            return new CheckIntVariableAction(config, dataRepository, trWriter);
        case RestAPI:
        	return new RestAPIAction(config, dataRepository, trWriter);
        case CompareREST:
        	return new CompareRESTAction(config, dataRepository, trWriter);
        case GetServerUID:
        	return new GetServerUIDAction(config, dataRepository, trWriter);
        default:
            throw new RuntimeException("Action type " + actionType
                    + " is not supported.");
        }
    }

}
