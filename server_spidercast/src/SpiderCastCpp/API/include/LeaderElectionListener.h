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
 * LeaderElectionListener.h
 *
 *  Created on: Jun 13, 2012
 */

#ifndef LEADERELECTIONLISTENER_H_
#define LEADERELECTIONLISTENER_H_

#include "NodeID.h"

namespace spdr
{

namespace leader_election
{

/**
 * A call back interface for leader election results.
 *
 * @see LeaderElectionService
 */
class LeaderElectionListener
{
public:
	virtual ~LeaderElectionListener();

	/**
	 * Delivers the identity of a new leader, or NULL in case there is no leader.
	 *
	 * @param leader
	 */
	virtual void onLeaderChange(NodeID_SPtr leader) = 0;

protected:
	LeaderElectionListener();
};

}

}

#endif /* LEADERELECTIONLISTENER_H_ */
