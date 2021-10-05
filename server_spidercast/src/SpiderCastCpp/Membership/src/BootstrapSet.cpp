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
 * BootstrapSet.cpp
 *
 *  Created on: 14/04/2010
 */

#include <iostream>
#include <algorithm>

#include "SpiderCastConfig.h"

#include "BootstrapSet.h"

namespace spdr
{

using namespace trace;

Bootstrap::Bootstrap()
{
}

Bootstrap::~Bootstrap()
{
}

ScTraceComponent* BootstrapSet::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"BootstrapSet",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

BootstrapSet::BootstrapSet(const String& instID,
		const vector<NodeIDImpl_SPtr > & configBootstrap,
		NodeIDImpl_SPtr my_id,
		VirtualIDCache& vidCache,
		bool fullBSS) :
		Bootstrap(),
		ScTraceContext(tc_,instID,my_id->getNodeName()),
		instanceID(instID),
		numNotInView(0),
		//numNotInViewHier(0),
		myID(my_id),
		virtualIDCache(vidCache),
		myIDInBootstrap(false),
		full(fullBSS),
		//policy(config::FullViewBootstrapPolicy_Random_VALUE),
		//hierarchyGroupSize(std::max(hierGroupSize,2)),
		lastNode(),
		successor(),
		numRequests(0),
		successorInView(false),
		bootstrapMap(),
		//hierarchyMap(),
		randomOrder(),
		lastRandomIndex(0),
		myVID()
{
	Trace_Entry(this,"BootstrapSet()");

	myVID = virtualIDCache.get(myID->getNodeName());
	srand(static_cast<unsigned int>( myVID->getLower64() ));

	NodeIDImpl_SPtr lowestVID_Node;
	util::VirtualID_SPtr lowestVID;
	util::VirtualID_SPtr successorVID;

	std::set<NodeIDImpl_SPtr, NodeIDImpl::SPtr_Less> orderedSet;

	vector<NodeIDImpl_SPtr >::const_iterator pos;
	for (pos = configBootstrap.begin(); pos != configBootstrap.end(); ++pos)
	{
		orderedSet.insert(*pos); //used for the hierarchy map


		if ( (*(*pos)) != (*myID) )
		{
			bootstrapMap.insert(std::make_pair( (*pos), false ));
			randomOrder.push_back(*pos); //used for random

			util::VirtualID_SPtr curr_vid = virtualIDCache.get((*pos)->getNodeName());

			if (!lowestVID_Node)
			{
				lowestVID_Node = *pos;
				lowestVID = curr_vid;
			}
			else if (*curr_vid < *lowestVID)
			{
				lowestVID_Node = *pos;
				lowestVID = curr_vid;
			}

			if (*curr_vid > *myVID)
			{
				if (!successor)
				{
					successor = *pos;
					successorVID = curr_vid;
				}
				else if ( *curr_vid < *successorVID)
				{
					successor = *pos;
					successorVID = curr_vid;
				}
			}
		}
		else
		{
			myIDInBootstrap = true;
		}
	}

	if (!successor && lowestVID_Node) //wrap around, myVID is largest
	{
		successor = lowestVID_Node;
	}

	numNotInView = bootstrapMap.size(); //excluding my ID

	//=== create a node specific random iteration order
	std::random_shuffle(randomOrder.begin(), randomOrder.end());

	Trace_Exit(this,"BootstrapSet()");
}

BootstrapSet::~BootstrapSet()
{
	Trace_Entry(this,"~BootstrapSet()");
}

bool BootstrapSet::setInView(NodeIDImpl_SPtr id, bool inView)
{
	if (full)
	{
		if (successor && successor->getNodeName() == id->getNodeName())
		{
			successorInView = inView;
			numRequests = 0;
		}
	}

	if (*id == *myID)
	{
		 if (myIDInBootstrap)
			 return true;
		 else
			 return false;
	}
	else
	{
		return updateMap(id, inView, bootstrapMap, numNotInView);
	}
}

bool BootstrapSet::updateMap(NodeIDImpl_SPtr id, const bool inView, BootstrapMap& map, int& notInView_counter)
{
	BootstrapMap::iterator pos = map.find(id);
	if (pos != map.end())
	{
		if (pos->second != inView)
		{
			pos->second = inView;
			notInView_counter = notInView_counter + ((inView) ? (-1) : (+1));
		}
		return true;
	}
	else
	{
		return false;
	}
}

NodeIDImpl_SPtr BootstrapSet::getNextNode_NotInView()
{
		return getNextNode_NotInView_Random();
}

NodeIDImpl_SPtr BootstrapSet::getNextNode_NotInView_Random()
{
	if (numNotInView > 0)
	{
		if (full && successor && !successorInView)
		{
			if (numRequests++ % numNotInView == 0)
			{
				lastNode = successor;
				return successor;
			}
		}

		for (;;)
		{
			lastNode = randomOrder.at(lastRandomIndex % randomOrder.size());
			++lastRandomIndex;
			BootstrapMap::iterator pos = bootstrapMap.find(lastNode);
			if (!pos->second)
			{
				break;
			}
		}
	}
	else
	{
		numRequests = 0;
		lastNode.reset();
		lastRandomIndex = 0;
	}

	return lastNode;
}




int BootstrapSet::size() const
{
	return bootstrapMap.size() + (myIDInBootstrap ? 1 : 0);
}

int BootstrapSet::getNumNotInView() const
{
	return numNotInView;
}

String BootstrapSet::toString() const
{
	std::ostringstream oss;
	oss << "BootstrapSet (I/T=" << (size() - getNumNotInView()) << "/" << (size()) << ") "
			<< "Full:" << std::boolalpha << full
			//<< ", Policy:" << policy
			<< ", Succ:" << (successor?successor->getNodeName():"null") << "; B-Set: ";

	if (isMyIDInBootstrap())
	{
		oss << myID->getNodeName() << " I";
		if (size()>1)
			oss << ", ";
	}

	for (BootstrapMap::const_iterator pos = bootstrapMap.begin(); pos != bootstrapMap.end();)
	{
		oss << pos->first->getNodeName();
		if (pos->second)
			oss << " I";
		else
			oss << " O";

		if (++pos != bootstrapMap.end())
			oss << ", ";
	}

	return oss.str();
}

//===========================================================================
// BootstrapMultimap
//===========================================================================

ScTraceComponent* BootstrapMultimap::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"BootstrapMultimap",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

BootstrapMultimap::BootstrapMultimap(const String& instID,
		const vector<NodeIDImpl_SPtr > & configBootstrap,
		NodeIDImpl_SPtr my_id,
		VirtualIDCache& vidCache,
		bool fullBSS) :
		Bootstrap(),
		ScTraceContext(tc_,instID,my_id->getNodeName()),
		instanceID(instID),
		numNotInView(0),
		myID(my_id),
		virtualIDCache(vidCache),
		myIDInBootstrap(false),
		full(fullBSS),
		lastNode(),
		successor(),
		numRequests(0),
		successorInView(false),
		bootstrapMap(),
		randomOrder(),
		lastRandomIndex(0),
		bootstrapMap_nameless(),
		lastNameless(),
		nextIsNameless(false),
		myVID()
{
	using namespace std;

	Trace_Entry(this,"BootstrapMultimap()");

	myVID = virtualIDCache.get(myID->getNodeName());
	srand(static_cast<unsigned int>( myVID->getLower64() ));

	string lowestVID_NodeName;
	util::VirtualID_SPtr lowestVID;
	util::VirtualID_SPtr successorVID;

	for (vector<NodeIDImpl_SPtr >::const_iterator pos = configBootstrap.begin(); pos != configBootstrap.end(); ++pos)
	{
		if ((*pos)->getNodeName() != NodeID::NAME_ANY)
		{
			//Named nodes
			if ( (*(*pos)) != (*myID) )
			{
				BootstrapMap::iterator bss_it = bootstrapMap.find((*pos)->getNodeName());
				if (bss_it != bootstrapMap.end())
				{
					bool uniqueID = true;
					for (NodeID_Vec::iterator vec_it = bss_it->second.id_vec.begin(); vec_it != bss_it->second.id_vec.end(); ++vec_it)
					{
						if ((*vec_it)->deepEquals(*(*pos))) //compares EP as well
						{
							uniqueID = false;
							break;
						}
					}

					if (uniqueID)
						bss_it->second.id_vec.push_back(*pos);
				}
				else
				{
					randomOrder.push_back((*pos)->getNodeName());
					BootstrapValue val;
					val.id_vec.push_back(*pos);
					bootstrapMap.insert(std::make_pair( (*pos)->getNodeName(), val ));

					//find successor
					util::VirtualID_SPtr curr_vid = virtualIDCache.get((*pos)->getNodeName());

					if (lowestVID_NodeName.empty())
					{
						lowestVID_NodeName = (*pos)->getNodeName();
						lowestVID = curr_vid;
					}
					else if (*curr_vid < *lowestVID)
					{
						lowestVID_NodeName = (*pos)->getNodeName();
						lowestVID = curr_vid;
					}

					if (*curr_vid > *myVID)
					{
						if (successor.empty())
						{
							successor = (*pos)->getNodeName();
							successorVID = curr_vid;
						}
						else if ( *curr_vid < *successorVID)
						{
							successor = (*pos)->getNodeName();
							successorVID = curr_vid;
						}
					}
				}
			}
			else
			{
				myIDInBootstrap |= true;
			}
		}
		else
		{
			//Nameless nodes
			if ((*pos)->getNetworkEndpoints() != myID->getNetworkEndpoints())
			{
				//std::pair<BootstrapMapNameless::iterator,bool> res = bootstrapMap_nameless.insert(std::make_pair((*pos)->getNetworkEndpoints(),false));
				bootstrapMap_nameless.insert(std::make_pair((*pos)->getNetworkEndpoints(),false));
			}
			else
			{
				myIDInBootstrap |= true;
			}
		}
	}

	//make BootstrapMap and BootstrapMapNameless mutually exclusive
	for (BootstrapMap::const_iterator it = bootstrapMap.begin(); it != bootstrapMap.end(); ++it)
	{
		for (NodeID_Vec::const_iterator id_it = it->second.id_vec.begin(); id_it != it->second.id_vec.end(); ++id_it)
		{
			BootstrapMapNameless::iterator pos = bootstrapMap_nameless.find((*id_it)->getNetworkEndpoints());
			if (pos != bootstrapMap_nameless.end())
			{
				bootstrapMap_nameless.erase(pos);
			}
		}
	}

	if (successor.empty() && !lowestVID_NodeName.empty()) //wrap around, myVID is largest
	{
		successor = lowestVID_NodeName;
	}

	numNotInView = bootstrapMap.size(); //excluding my ID
	numNotInView_nameless =  bootstrapMap_nameless.size();
	if (numNotInView_nameless > 0)
	{
		full = false;
	}

	//=== create a node specific random iteration order

	std::random_shuffle(randomOrder.begin(), randomOrder.end());
	lastNameless = bootstrapMap_nameless.begin();
	if (bootstrapMap_nameless.size() > 1)
	{
		for (int i = rand() % bootstrapMap_nameless.size(); i>0; --i)
		{
			++lastNameless;
		}
	}

	Trace_Exit(this,"BootstrapMultimap()");
}

BootstrapMultimap::~BootstrapMultimap()
{
	Trace_Entry(this,"~BootstrapMultimap()");
}

bool BootstrapMultimap::setInView(NodeIDImpl_SPtr id, bool inView)
{
	Trace_Entry(this,"setInView()", "ID", spdr::toString(id),"inView",(inView?"T":"F"));

	if (id->isNameANY())
	{
		throw SpiderCastRuntimeError("Error: BootstrapMultimap::setInView with node name: NAME_ANY ($ANY)");
	}

	if (full)
	{
		if (successor == id->getNodeName())
		{
			successorInView = inView;
			numRequests = 0;
		}
	}

	bool rc = false;

	if (*id == *myID)
	{
		if (myIDInBootstrap)
			rc = true;
	}
	else
	{
		BootstrapMap::iterator pos = bootstrapMap.find(id->getNodeName());
		if (pos != bootstrapMap.end())
		{
			if (pos->second.in_view != inView)
			{
				pos->second.in_view = inView;
				numNotInView = numNotInView + ((inView) ? (-1) : (+1));
			}
			rc = true;
		}

		BootstrapMapNameless::iterator pos2 = bootstrapMap_nameless.find(id->getNetworkEndpoints());
		if (pos2 != bootstrapMap_nameless.end())
		{
			if (pos2->second != inView)
			{
				pos2->second = inView;
				numNotInView_nameless = numNotInView_nameless + ((inView) ? (-1) : (+1));
			}
			rc = true;
		}
	}

	Trace_Exit(this,"setInView()",rc);
	return rc;
}

NodeIDImpl_SPtr BootstrapMultimap::getNextNode_NotInView()
{
	Trace_Entry(this,"getNextNode_NotInView()");

	NodeIDImpl_SPtr node;
	if (nextIsNameless)
	{
		nextIsNameless = false;
		node = getNextNode_NotInView_Nameless();
		if (!node)
			node = getNextNode_NotInView_Named();
	}
	else
	{
		nextIsNameless = true;
		node = getNextNode_NotInView_Named();
		if (!node)
			node = getNextNode_NotInView_Nameless();
	}

	Trace_Exit(this,"getNextNode_NotInView()", spdr::toString(node));
	return node;
}

NodeIDImpl_SPtr BootstrapMultimap::getNextNode_NotInView_Named()
{
	std::string name = getNextNode_NotInView_Random();
	if (name.empty())
	{
		return NodeIDImpl_SPtr();
	}
	else
	{
		BootstrapMap::iterator pos = bootstrapMap.find(name);
		if (pos != bootstrapMap.end())
		{
			//must be random to prevent the second node being always skipped (see TopologyManager)
			int i = rand() % pos->second.id_vec.size();
			NodeIDImpl_SPtr target = pos->second.id_vec[i];
			return target;
		}
		else
		{
			throw SpiderCastRuntimeError("BootstrapMultimap::getNextNode_NotInView inconsistent data structure");
		}
	}
}

std::string BootstrapMultimap::getNextNode_NotInView_Random()
{
	if (numNotInView > 0)
	{
		if (full && !successor.empty() && !successorInView)
		{
			if (numRequests++ % numNotInView == 0)
			{
				lastNode = successor;
				return successor;
			}
		}

		for (;;)
		{
			lastNode = randomOrder.at(lastRandomIndex % randomOrder.size());
			++lastRandomIndex;
			BootstrapMap::iterator pos = bootstrapMap.find(lastNode);
			if (!pos->second.in_view)
			{
				break;
			}
		}
	}
	else
	{
		numRequests = 0;
		lastNode.clear();
		lastRandomIndex = 0;
	}

	return lastNode;
}


NodeIDImpl_SPtr BootstrapMultimap::getNextNode_NotInView_Nameless()
{
	NodeIDImpl_SPtr node;

	if (numNotInView_nameless > 0)
	{
		for (unsigned int i = 0; i < bootstrapMap_nameless.size(); ++i)
		{
			if (lastNameless == bootstrapMap_nameless.end())
				lastNameless = bootstrapMap_nameless.begin();

			if (!lastNameless->second)
			{
				NodeIDImpl_SPtr node(new NodeIDImpl(NodeID::NAME_ANY, lastNameless->first));
				++lastNameless;
				return node;
			}
			else
			{
				++lastNameless;
			}
		}
	}
	else
	{
		lastNameless = bootstrapMap_nameless.begin();
	}

	return node;
}



int BootstrapMultimap::size() const
{
	return bootstrapMap.size() + bootstrapMap_nameless.size() + (myIDInBootstrap ? 1 : 0);
}

int BootstrapMultimap::getNumNotInView() const
{
	return numNotInView + numNotInView_nameless;
}

String BootstrapMultimap::toString() const
{
	std::ostringstream oss;
	oss << "BootstrapMultimap (I/T=" << (size() - getNumNotInView()) << "/" << (size()) << ") "
			<< "Full:" << std::boolalpha << full
			<< ", Succ=" << successor << "; B-Set: " << std::endl;

	if (isMyIDInBootstrap())
	{
		oss << myID->getNodeName() << " I, " << myID->getNetworkEndpoints().toString() << ";" << std::endl;
	}

	for (BootstrapMap::const_iterator pos = bootstrapMap.begin(); pos != bootstrapMap.end(); ++pos)
	{
		oss << pos->first;
		if (pos->second.in_view)
			oss << " I,";
		else
			oss << " O,";

		for (NodeID_Vec::const_iterator it = pos->second.id_vec.begin(); it != pos->second.id_vec.end(); ++it)
		{
			oss << " " << (*it)->getNetworkEndpoints().toString();
		}

		oss << ";" << std::endl;
	}

	for (BootstrapMapNameless::const_iterator pos = bootstrapMap_nameless.begin(); pos != bootstrapMap_nameless.end(); ++pos)
	{
		oss << NodeID::NAME_ANY;
		if (pos->second)
			oss << " I, ";
		else
			oss << " O, ";

		oss << pos->first.toString() << ";" << std::endl;
	}

	return oss.str();
}

} //namespace spdr
