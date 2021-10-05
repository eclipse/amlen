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
 * BootstrapSet.h
 *
 *  Created on: 14/04/2010
 */

#ifndef BOOTSTRAPSET_H_
#define BOOTSTRAPSET_H_

#include <map>

#include <boost/unordered_map.hpp>

#include "NodeIDImpl.h"
#include "VirtualIDCache.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

namespace spdr
{

class Bootstrap
{
protected:
	Bootstrap();

public:
	virtual ~Bootstrap();

	virtual bool setInView(NodeIDImpl_SPtr id, bool inView) = 0;

	virtual NodeIDImpl_SPtr getNextNode_NotInView() = 0;

	virtual int size() const = 0;

	virtual int getNumNotInView() const = 0;

	virtual bool isMyIDInBootstrap() const = 0;

	virtual String toString() const = 0;
};

class BootstrapSet : public Bootstrap, boost::noncopyable, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	BootstrapSet(
			const String& instID,
			const vector<NodeIDImpl_SPtr > & configBootstrap,
			NodeIDImpl_SPtr my_id,
			VirtualIDCache& vidCache,
			bool fullBSS);

	virtual ~BootstrapSet();

	/**
	 *
	 * @param id
	 * @param inView
	 * @return true if found in the map
	 */
	bool setInView(NodeIDImpl_SPtr id, bool inView);

	NodeIDImpl_SPtr getNextNode_NotInView();

	int size() const;

	int getNumNotInView() const;

	bool isMyIDInBootstrap() const
	{
		return myIDInBootstrap;
	}

	//void setPolicy(const String& policy);


	String toString() const;

private:
	const String instanceID;
	int numNotInView;
//	int numNotInViewHier;
	const NodeIDImpl_SPtr myID;
	VirtualIDCache& virtualIDCache;
	bool myIDInBootstrap;
	const bool full;
//	String policy;
//	int hierarchyGroupSize;

	NodeIDImpl_SPtr lastNode;

	NodeIDImpl_SPtr successor;
	int numRequests;
	bool successorInView;

	//typedef std::set<NodeIDImpl_SPtr, NodeIDImpl::SPtr_Less> NodeIDOrderedSet;

	//	Key=Node, Value=inView
	typedef boost::unordered_map<NodeIDImpl_SPtr, bool, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > BootstrapMap;
	BootstrapMap bootstrapMap;
//	BootstrapMap hierarchyMap;

	std::vector<NodeIDImpl_SPtr > randomOrder;
	std::size_t lastRandomIndex;

	util::VirtualID_SPtr myVID;

	bool updateMap(NodeIDImpl_SPtr id, const bool inView, BootstrapMap& map, int& counter);

	NodeIDImpl_SPtr getNextNode_NotInView_Random();
//	NodeIDImpl_SPtr getNextNode_NotInView_Random1();
//	NodeIDImpl_SPtr getNextNode_NotInView_Hierarcy();

};

/*
 * TODO add/remove nodes
 */
class BootstrapMultimap : public Bootstrap, boost::noncopyable, public ScTraceContext
{
private:
	static ScTraceComponent* tc_;

public:
	BootstrapMultimap(
			const String& instID,
			const vector<NodeIDImpl_SPtr > & configBootstrap,
			NodeIDImpl_SPtr my_id,
			VirtualIDCache& vidCache,
			bool fullBSS);

	virtual ~BootstrapMultimap();

	/**
	 *
	 * @param id
	 * @param inView
	 * @return true if found in the map
	 */
	bool setInView(NodeIDImpl_SPtr id, bool inView);

	NodeIDImpl_SPtr getNextNode_NotInView();

	int size() const;

	int getNumNotInView() const;

	bool isMyIDInBootstrap() const
	{
		return myIDInBootstrap;
	}

	String toString() const;

private:
	const String instanceID;
	int numNotInView;
	const NodeIDImpl_SPtr myID;
	VirtualIDCache& virtualIDCache;

	bool myIDInBootstrap;
	bool full;

	std::string lastNode;

	std::string successor;
	int numRequests;
	bool successorInView;

	typedef std::vector<NodeIDImpl_SPtr > NodeID_Vec;

	//	Key=Node, Value=inView
	//typedef boost::unordered_map<NodeIDImpl_SPtr, bool, NodeIDImpl::SPtr_Hash, NodeIDImpl::SPtr_Equals > BootstrapMap;

	struct BootstrapValue
	{
		BootstrapValue() : in_view(false), index(0)	{};

		NodeID_Vec id_vec;
		bool in_view;
		uint16_t index;
	};

	//Named discovery nodes
	// Key=NodeName, Value=pair< a vector of nodes with same-name different-EP, inView}
	typedef boost::unordered_map<std::string, BootstrapValue > BootstrapMap;
	BootstrapMap bootstrapMap;

	std::vector<std::string> randomOrder;
	std::size_t lastRandomIndex;

	// Nameless discovery nodes
	// Key=NetworkEndpoints, Value=inView
	typedef std::map<NetworkEndpoints,bool> BootstrapMapNameless;
	BootstrapMapNameless bootstrapMap_nameless;
	int numNotInView_nameless;
	BootstrapMapNameless::const_iterator lastNameless;
	bool nextIsNameless;

	util::VirtualID_SPtr myVID;

	//bool updateMap(NodeIDImpl_SPtr id, const bool inView, BootstrapMap& map, int& counter);

	NodeIDImpl_SPtr getNextNode_NotInView_Named();
	std::string getNextNode_NotInView_Random();
	NodeIDImpl_SPtr getNextNode_NotInView_Nameless();
};

} //namespace spdr

#endif /* BOOTSTRAPSET_H_ */
