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
 * NodeIDCache.h
 *
 *  Created on: 22/04/2010
 */

#ifndef NODEIDCACHE_H_
#define NODEIDCACHE_H_

//#include <map>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "Definitions.h"
//#include "ConcurrentSharedPtr.h"
#include "NodeIDImpl.h"

namespace spdr
{

/**
 * A thread safe cache for NodeIDImpl_SPtr objects.
 *
 * The only writer to this cache is the MemTop-thread.
 *
 * Readers are potentially all: App., Transport, Routing, MemTopo, Comm(RUM).
 *
 */
class NodeIDCache : boost::noncopyable
{
public:
	NodeIDCache();
	virtual ~NodeIDCache();

	/**
	 * Put a node in the cache, replace if already exist.
	 *
	 * Only the topology thread (through one of the
	 * layers that are activated by this thread, i.e., membership, discovery,
	 * and topology) should put a node in the cache.
	 *
	 * @param node
	 * @return if set size changed.
	 */
	bool put(NodeIDImpl_SPtr node);

	/**
	 * Remove a node from the cache.
	 *
	 * Only the topology thread (through one of the
	 * layers that are activated by this thread, i.e., membership, discovery,
	 * and topology) should remove a node from the cache.
	 *
	 * @param node
	 * @return if set size changed.
	 */
	bool remove(NodeIDImpl_SPtr node);

	/**
	 * Remove a node from the cache.
	 *
	 * @param name
	 * @return if set size changed.
	 */
	bool remove(const String& name);

	/**
	 * Returns whatever is in the cache, or null.
	 *
	 * Never puts values in the cache.
	 *
	 * @param name
	 * @return
	 */
	NodeIDImpl_SPtr get(const String& name);

	/**
	 * Returns whatever is in the cache, or creates a new NodeIdImpl (with empty
	 * NetworkEndpoints).
	 *
	 * Never puts values in the cache.
	 *
	 * @param name
	 * @return
	 */
	NodeIDImpl_SPtr getOrCreate(const String& name);

private:
	//typedef std::map<String, NodeIDImpl_SPtr > Name2IDMap;
	typedef boost::unordered_map<String, NodeIDImpl_SPtr > Name2IDMap;
	Name2IDMap name2IDMap_;
	boost::recursive_mutex mutex_;


};

}//namespace spdr

#endif /* NODEIDCACHE_H_ */
