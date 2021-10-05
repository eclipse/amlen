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
 * NodeIDImpl.h
 *
 *  Created on: 03/03/2010
 */

#ifndef NODEIDIMPL_H_
#define NODEIDIMPL_H_

#include <boost/functional/hash.hpp>

#include "Definitions.h"
#include "NodeID.h"
#include "NetworkEndpoints.h"

namespace spdr
{

using std::string;
using std::vector;
using std::pair;

class NodeIDImpl;
typedef boost::shared_ptr<NodeIDImpl> NodeIDImpl_SPtr;


class NodeIDImpl: public NodeID
{
public:
	NodeIDImpl();
	NodeIDImpl(const String& name, const NetworkEndpoints& endpoints);
	NodeIDImpl(const String& name, const vector<pair<string, string> >& addresses, uint16_t port);

	NodeIDImpl(const NodeIDImpl& id);
	NodeIDImpl& operator=(const NodeIDImpl& id);

	virtual ~NodeIDImpl();

	const String& getNodeName() const;
	const NetworkEndpoints& getNetworkEndpoints() const;

	String toString() const;

	bool isNameANY() const
	{
		return (NAME_ANY == nodeName);
	}

	bool isNameStartsWithANY() const
	{
		return nodeName.substr(0,NAME_ANY.size()) == NAME_ANY;
	}

	static String stringValueOf(const NodeIDImpl_SPtr & id)
	{
		if (id)
			return id->toString();
		else
			return "null";
	}

	/*==========================================================
	 * The 6 overloaded relational operators compare node names.
	 * The hash_value() is based on the node name only.
	 */

	bool operator==(const NodeID& id2) const;
	bool operator!=(const NodeID& id2) const;

	bool operator<(const NodeID& id2) const;
	bool operator>(const NodeID& id2) const;

	bool operator<=(const NodeID& id2) const;
	bool operator>=(const NodeID& id2) const;

	std::size_t hash_value() const
	{
		return hashValue;
	}

	friend
	std::size_t hash_value( NodeIDImpl const& id)
	{
		return id.hashValue;
	}

	/**
	 * Compares two NodeID's using both the name and network end-points.
	 *
	 * @param id2
	 * @return true if both the name and network end-points are equal.
	 */
	bool deepEquals(const NodeID& id2) const;

	/**
	 * A hash value based on both the name and network end-points.
	 *
	 * Two nodes with different deepHashValue cannot be equal according to deepEquals().
	 *
	 * @return
	 */
	std::size_t deepHashValue() const;


protected:
	String nodeName;
	NetworkEndpoints networkEndpoints;
	std::size_t hashValue;

public:
	//=== Helper Classes ==

	/**
	 * A helper function object for STL associative containers.
	 *
	 * Works with a container that holds NodeID_SPtr.
	 */
	class SPtr_Equals
	{
	public:
		/**
		 * Compares the two NodeIDImpl_SPtr objects with ==.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) < (*a2)
		 */
		bool operator()(const NodeIDImpl_SPtr & a1, const NodeIDImpl_SPtr & a2) const
		{
			return ((*a1) == (*a2));
		}
	};

	/**
	 * A helper function object for STL associative containers.
	 *
	 * Works with a container that holds NodeIDImpl_SPtr.
	 */
	class SPtr_Hash
	{
	public:
		/**
		 * Compares the two NodeID objects with <.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) < (*a2)
		 */
		std::size_t operator()(const NodeIDImpl_SPtr & a) const
		{
			return (a->hash_value());
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds NodeIDImpl_SPtr.
	 */
	class SPtr_Less
	{
	public:
		/**
		 * Compares the two NodeID objects with <.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) < (*a2)
		 */
		bool operator()(const NodeIDImpl_SPtr & a1, const NodeIDImpl_SPtr & a2) const
		{
			return ((*a1) < (*a2));
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds NodeIDImpl_SPtr.
	 */
	class SPtr_Greater
	{
	public:
		/**
		 * Compares the two NodeID objects with >.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) > (*a2)
		 */
		bool operator()(const NodeIDImpl_SPtr & a1, const NodeIDImpl_SPtr & a2) const
		{
			return ((*a1) > (*a2));
		}
	};

};


}//namespace spdr

#endif /* NODEIDIMPL_H_ */
