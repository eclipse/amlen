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
 * RumNeighbor.h
 *
 *  Created on: Apr 10, 2012
 *      Author:
 */

#ifndef RUMNEIGHBOR_H_
#define RUMNEIGHBOR_H_

#include "Neighbor.h"

#include "rumCapi.h"

#include "Definitions.h"
#include "SpiderCastRuntimeError.h"
#include "SCMessage.h"
#include "BusName.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#include <boost/thread/recursive_mutex.hpp>



namespace spdr
{

typedef boost::shared_ptr<rumQueueT> rumQueueT_SPtr;

class RumNeighbor: public Neighbor
{
private:
	rumQueueT_SPtr _tx;

	//streamId_t _reciever;

	rumTxMessage msg; //TODO init in virgin

	rumConnection _connection; //TODO init in virgin
	//rumStreamID_t _sid;



public:

	virtual ~RumNeighbor();

	RumNeighbor(const rumConnection& connection, rumQueueT_SPtr tx,
			rumStreamID_t sid, const String& targetName, const String& myName, const String& instID);

	RumNeighbor(const rumConnection& connection, const String& myName, const String& instID, const String& targetName );

	/**
	 *
	 * @param msg
	 * @return 0 for success
	 */
	int sendMessage(SCMessage_SPtr msg);

	rumConnection getConnection() const
	{
		return _connection;
	}

	uint64_t getConnectionId() const;

	uint64_t getSid() const
	{
		return static_cast<uint64_t>(_sid);
	}

	rumQueueT_SPtr getTx() const
	{
		return _tx;
	}

	//streamId_t getReceiverId() const;

	//void setReceiverId(streamId_t streamId);

	String toString() const;

	/*
	 * @see Neighbor
	 */
	bool operator==(Neighbor& rhs) const;

	/*
	 * @see Neighbor
	 */
	bool operator!=(Neighbor& rhs) const;

	friend
	std::size_t hash_value(const RumNeighbor& n);

};

typedef boost::shared_ptr<RumNeighbor> 	RumNeighbor_SPtr;

}

#endif /* RUMNEIGHBOR_H_ */


