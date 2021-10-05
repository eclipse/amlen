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
 * NodeID.h
 *
 *  Created on: 02/03/2010
 */

#ifndef NODEID_H_
#define NODEID_H_

#include "Definitions.h"
#include "NetworkEndpoints.h"

namespace spdr
{

class NodeID;
/**
 * A shared pointer to a NodeID.
 */
typedef boost::shared_ptr<NodeID> NodeID_SPtr;

/**
 * Node ID carries the node's name (String) and NetworkEndpoints.
 *
 * Relational operators are defined in terms of the node's name only.
 */
class NodeID
{
public:
	/**
	 * The node name used in the bootstrap for nameless discovery.
	 */
	static const std::string NAME_ANY;

	/**
	 * A reserved prefix that cannot be used in a node name.
	 */
	static const std::string RSRV_NAME_DISCOVERY;

	/**
	 * Get node name.
	 *
	 * @return node name
	 */
	virtual const String& getNodeName() const = 0;

	/**
	 * Get network end-points.
	 *
	 * @return network end-points
	 */
	virtual const NetworkEndpoints& getNetworkEndpoints() const = 0;

	/**
	 * A string representation of the NodeID.
	 *
	 * @return a string representation.
	 */
	virtual String toString() const = 0;

	/**
	 * Equality operator.
	 *
	 * @param id2
	 * @return true if this and parameter are equal.
	 */
	virtual bool operator==(const NodeID& id2) const = 0;

	/**
	 * Inequality operator.
	 * @param id2
	 * @return true if this and parameter are unequal.
	 */
	virtual bool operator!=(const NodeID& id2) const = 0;

	/**
	 * Less than operator.
	 *
	 * @param id2
	 * @return true of this is less than parameter.
	 */
	virtual bool operator<(const NodeID& id2) const = 0;

	/**
	 * Less than or equal operator.
	 *
	 * @param id2
	 * @return true of this is less than or equal to parameter.
	 */
	virtual bool operator<=(const NodeID& id2) const = 0;

	/**
	 * Greater than operator.
	 *
	 * @param id2
	 * @return true of this is greater than parameter.
	 */
	virtual bool operator>(const NodeID& id2) const = 0;

	/**
	 * Greater than or equal operator.
	 *
	 * @param id2
	 * @return true of this is greater than or equal to parameter.
	 */
	virtual bool operator>=(const NodeID& id2) const = 0;

	/**
	 * Returns the hash value.
	 *
	 * @return
	 */
	virtual std::size_t hash_value() const = 0;

	virtual ~NodeID();

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
		 * Compares the two NodeID objects with ==.
		 * @param a1
		 * @param a2
		 * @return true if (*a1) == (*a2)
		 */
		bool operator()(const NodeID_SPtr & a1, const NodeID_SPtr & a2) const
		{
			return ((*a1) == (*a2));
		}
	};

	/**
	 * A helper function object for STL associative containers.
	 *
	 * Works with a container that holds NodeID_SPtr.
	 */
	class SPtr_Hash
	{
	public:
		/**
		 * Compute the hash value of a NodeID object.
		 * @param a
		 * @return the hash value of a
		 */
		std::size_t operator()(const NodeID_SPtr & a) const
		{
			return (a->hash_value());
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds NodeID_SPtr.
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
		bool operator()(const NodeID_SPtr & a1, const NodeID_SPtr & a2) const
		{
			return ((*a1) < (*a2));
		}
	};

	/**
	 * A helper function object for STL container sort.
	 *
	 * Works with a container that holds NodeID_SPtr.
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
		bool operator()(const NodeID_SPtr & a1, const NodeID_SPtr & a2) const
		{
			return ((*a1) > (*a2));
		}
	};

protected:
	/**
	 * Default constructor.
	 * @return
	 */
	NodeID();
};

}

#endif /* NODEID_H_ */
