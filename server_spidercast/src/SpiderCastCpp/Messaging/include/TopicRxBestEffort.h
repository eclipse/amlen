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
 * TopicRxBestEffort.h
 *
 *  Created on: Feb 8, 2012
 */

#ifndef TOPICRXBESTEFFORT_H_
#define TOPICRXBESTEFFORT_H_

#include <boost/noncopyable.hpp>

#include "SpiderCastConfigImpl.h"
#include "TopicImpl.h"
#include "MessageListener.h"
#include "RxMessageImpl.h"
#include "SCMessage.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

namespace messaging
{

class TopicRxBestEffort : boost::noncopyable, public ScTraceContext
{
	static ScTraceComponent* tc_;

public:
	TopicRxBestEffort(
			const String& instID,
			const SpiderCastConfigImpl& config,
			MessageListener& messageListener,
			Topic_SPtr topic);

	virtual ~TopicRxBestEffort();

	/**
	 * buffer position is right after the H3 header.
	 *
	 * @param message
	 */
	void processIncomingDataMessage(SCMessage_SPtr message);

	String toString() const;

private:
	const String& instID_;
	const SpiderCastConfigImpl& config_;
	MessageListener& messageListener_;
	Topic_SPtr topic_;


};

}

}

#endif /* TOPICRXBESTEFFORT_H_ */
