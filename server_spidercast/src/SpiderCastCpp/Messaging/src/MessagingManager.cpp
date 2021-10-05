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
 * MessagingManager.cpp
 *
 *  Created on: Feb 1, 2012
 */

#include "MessagingManager.h"

namespace spdr
{

namespace messaging
{
const String MessagingManager::topicKey_Prefix = ".tpc.";

MessagingManager::MessagingManager()
{
}

MessagingManager::~MessagingManager()
{
}

//static
char MessagingManager::addBridgeSub_Flags(char current_flags)
{
	char flags = current_flags | topicFlags_SubscriberMask | topicFlags_BridgeMask;
	return flags;
}

//static
char MessagingManager::removeBridgeSub_Flags(char current_flags)
{
	char flags = current_flags & ~MessagingManager::topicFlags_BridgeMask;

	if  ((flags & MessagingManager::topicFlags_GlobalMask) == 0)
	{
		flags = flags & ~MessagingManager::topicFlags_SubscriberMask;
	}

	return flags;
}

//static
char MessagingManager::addSub_Flags(char current_flags, bool global)
{
	char flags = current_flags | topicFlags_SubscriberMask;
	if (global) //override whatever is in there
	{
		flags = flags | topicFlags_GlobalMask;
	}

	return flags;
}


//static
char MessagingManager::removeSub_Flags(char current_flags)
{
	char flags = current_flags & ~topicFlags_GlobalMask;

	if ((flags & topicFlags_BridgeMask) == 0)
	{
		flags = flags & ~topicFlags_SubscriberMask;
	}

	return flags;
}

//static
char MessagingManager::addPub_Flags(char current_flags)
{
	return current_flags | topicFlags_PublisherMask;;
}

//static
char MessagingManager::removePub_Flags(char current_flags)
{
	return current_flags & ~topicFlags_PublisherMask;;
}

}

}
