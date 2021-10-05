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
/*
 * TopicRxBestEffort.cpp
 *
 *  Created on: Feb 8, 2012
 */

#include "TopicRxBestEffort.h"

namespace spdr
{

namespace messaging
{



ScTraceComponent* TopicRxBestEffort::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Msgn,
		trace::ScTrConstants::Layer_ID_Msgn,
		"TopicRxBestEffort",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

TopicRxBestEffort::TopicRxBestEffort(
		const String& instID,
		const SpiderCastConfigImpl& config,
		MessageListener& messageListener,
		Topic_SPtr topic) :
		ScTraceContext(tc_,instID,config.getMyNodeID()->getNodeName()),
		instID_(instID),
		config_(config),
		messageListener_(messageListener),
		topic_(topic)
{
	Trace_Entry(this, "TopicRxBestEffort()");
}

TopicRxBestEffort::~TopicRxBestEffort()
{
	Trace_Entry(this, "~TopicRxBestEffort()");
}

String TopicRxBestEffort::toString() const
{
	String s("TopicRx RM=BestEffort Topic=");
	s.append(topic_->toString());
	return s;
}

void TopicRxBestEffort::processIncomingDataMessage(SCMessage_SPtr message)
{
	Trace_Entry(this, "processIncomingDataMessage()");
	//TODO try/catch and throw MessageUnmarshlingException

	ByteBuffer_SPtr bb = message->getBuffer();
	int32_t length = bb->readInt();
	RxMessageImpl rx_msg = message->getRxMessage();
	if (length>0)
	{
		rx_msg.setBuffer(length,(bb->getBuffer()+bb->getPosition()));
	}
	else
	{
		rx_msg.setBuffer(0,NULL);
	}

	//TODO try/catch and isolate user exceptions
	Trace_Debug(this, "processIncomingDataMessage()","before delivery");
	messageListener_.onMessage(rx_msg);

	Trace_Exit(this, "processIncomingDataMessage()");
}

}

}
