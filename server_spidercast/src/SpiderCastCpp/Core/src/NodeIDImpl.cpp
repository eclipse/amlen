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
 * NodeIDImpl.cpp
 *
 *  Created on: 03/03/2010
 */

#include "NodeIDImpl.h"

namespace spdr
{

NodeIDImpl::NodeIDImpl() : hashValue(0)
{

}

NodeIDImpl::~NodeIDImpl()
{
}

NodeIDImpl::NodeIDImpl(const String& name, const NetworkEndpoints& endpoints) :
	nodeName(name), networkEndpoints(endpoints)
{
	boost::hash<string> hasher;
	hashValue = hasher(nodeName);
}

NodeIDImpl::NodeIDImpl(const String& name, const vector<pair<string, string> >& addresses, uint16_t port) :
	nodeName(name), networkEndpoints(addresses,port)
{
	boost::hash<string> hasher;
	hashValue = hasher(nodeName);
}


NodeIDImpl::NodeIDImpl(const NodeIDImpl& id) :
	nodeName(id.nodeName), networkEndpoints(id.networkEndpoints), hashValue(id.hashValue)
{
}

NodeIDImpl& NodeIDImpl::operator=(const NodeIDImpl& id)
{
	if (this == &id)
	{
		return *this;
	}
	else
	{
		nodeName = id.nodeName;
		networkEndpoints = id.networkEndpoints;
		hashValue = id.hashValue;
		return *this;
	}
}

bool NodeIDImpl::operator==(const NodeID& id2) const
{
	return (nodeName == id2.getNodeName());
}

bool NodeIDImpl::operator!=(const NodeID& id2) const
{
	return (nodeName != id2.getNodeName());
}

bool NodeIDImpl::operator<(const NodeID& id2) const
{
	return (nodeName < id2.getNodeName());
}

bool NodeIDImpl::operator>(const NodeID& id2) const
{
	return (nodeName > id2.getNodeName());
}

bool NodeIDImpl::operator<=(const NodeID& id2) const
{
	return (nodeName <= id2.getNodeName());
}

bool NodeIDImpl::operator>=(const NodeID& id2) const
{
	return (nodeName >= id2.getNodeName());
}

const String& NodeIDImpl::getNodeName() const
{
	return nodeName;
}

const NetworkEndpoints& NodeIDImpl::getNetworkEndpoints() const
{
	return networkEndpoints;
}

bool NodeIDImpl::deepEquals(const NodeID& id2) const
{
	return ((nodeName == id2.getNodeName()) && (networkEndpoints == id2.getNetworkEndpoints()));
}

std::size_t NodeIDImpl::deepHashValue() const
{
	return hashValue + 313L * networkEndpoints.hash_value();
}


String NodeIDImpl::toString() const
{
	return "NodeName=" + nodeName + " " + networkEndpoints.toString();
}


}//namespace spdr
