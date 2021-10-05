/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#ifndef SUBCOVERINGFILTERWIREFORMAT_H_
#define SUBCOVERINGFILTERWIREFORMAT_H_

#include "ByteBuffer.h"
#include "SubscriptionPattern.h"
#include "RemoteSubscriptionStats.h"

namespace mcp
{
/**
 * A collection of static methods for reading and writing the SCFs into the attribute value
 */
class SubCoveringFilterWireFormat
{
public:
	static const uint16_t ATTR_VERSION;
	static const uint16_t STORE_VERSION;

	enum StoreRecordType
	{
	    Store_Local_Server_Record = 1,
	    Store_Remote_Server_Record = 2
	};

	static void writeSubscriptionPattern(const uint32_t wireFormatVer, const SubscriptionPattern& pattern, ByteBuffer_SPtr buffer);
	static void readSubscriptionPattern(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, SubscriptionPattern_SPtr& pattern);

	static int writeSubscriptionStats(const uint32_t wireFormatVer, const RemoteSubscriptionStats& stats, ByteBuffer_SPtr buffer);
	static int readSubscriptionStats(const uint32_t wireFormatVer, ByteBufferReadOnlyWrapper& buffer, RemoteSubscriptionStats* pStats);

private:
	SubCoveringFilterWireFormat();
	virtual ~SubCoveringFilterWireFormat();
};

} /* namespace spdr */

#endif /* SUBCOVERINGFILTERWIREFORMAT_H_ */
