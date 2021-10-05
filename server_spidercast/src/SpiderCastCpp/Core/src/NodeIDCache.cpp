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
 * NodeIDCache.cpp
 *
 *  Created on: 22/04/2010
 */

#include <iostream>

#include "NodeIDCache.h"

namespace spdr
{

NodeIDCache::NodeIDCache()
{
	//std::cout << "NodeIDCache()" << std::endl;
	// TODO Trace
}

NodeIDCache::~NodeIDCache()
{
	//std::cout << "~NodeIDCache()" << std::endl;
	// TODO Trace
}


bool NodeIDCache::put(NodeIDImpl_SPtr node)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);

	std::pair<Name2IDMap::iterator,bool> res = name2IDMap_.insert(
			Name2IDMap::value_type(node->getNodeName(),node));
	if (!res.second)
	{
		res.first->second = node;
	}

	return res.second;
}


bool NodeIDCache::remove(const String& name)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);

	size_t count = name2IDMap_.erase(name);
	return (count>0);
}

bool NodeIDCache::remove(NodeIDImpl_SPtr node)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);

	return remove(node->getNodeName());
}

/**
 * Returns whatever is in the cache, or null.
 *
 * Never puts values in the cache.
 *
 * @param name
 * @return
 */
NodeIDImpl_SPtr NodeIDCache::get(const String& name)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);

	Name2IDMap::iterator pos = name2IDMap_.find(name);
	if (pos != name2IDMap_.end())
	{
		return pos->second;
	}
	else
	{
		return NodeIDImpl_SPtr();
	}
}

/**
 * Returns whatever is in the cache, or creates a new NodeIdImpl (with empty
 * NetworkEndpoints).
 *
 * Never puts values in the cache.
 *
 * @param name
 * @return
 */
NodeIDImpl_SPtr NodeIDCache::getOrCreate(const String& name)
{
	boost::recursive_mutex::scoped_lock lock(mutex_);

	Name2IDMap::iterator pos = name2IDMap_.find(name);
	if (pos != name2IDMap_.end())
	{
		return pos->second;
	}
	else
	{
		return NodeIDImpl_SPtr(new NodeIDImpl(name, NetworkEndpoints()));
	}
}

}
