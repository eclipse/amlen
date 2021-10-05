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
 * PubSubViewListener.h
 *
 *  Created on: Jul 9, 2012
 */

#ifndef PUBSUBVIEWLISTENER_H_
#define PUBSUBVIEWLISTENER_H_

#include "Definitions.h"

namespace spdr
{

namespace route
{

class PubSubViewListener
{
public:
	PubSubViewListener();
	virtual ~PubSubViewListener();

	virtual void globalPub_add(const String&  topic_name) = 0;
	virtual void globalPub_remove(const String&  topic_name) = 0;

	virtual void globalSub_add(const String&  topic_name) = 0;
	virtual void globalSub_remove(const String&  topic_name) = 0;
};

}

}

#endif /* PUBSUBVIEWLISTENER_H_ */
