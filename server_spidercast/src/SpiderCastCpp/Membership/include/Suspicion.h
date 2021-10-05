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
 * Suspicion.h
 *
 *  Created on: 21/04/2010
 */

#ifndef SUSPICION_H_
#define SUSPICION_H_

#include <boost/operators.hpp>

#include "Definitions.h"
#include "NodeVersion.h"

namespace spdr
{

class Suspicion: public boost::less_than_comparable<Suspicion>,
		public boost::equality_comparable<Suspicion>
{
public:
	Suspicion(StringSPtr reportingNode, StringSPtr suspectedNode,
			NodeVersion suspectedVertion);

	Suspicion(const Suspicion& other);

	Suspicion& operator=(const Suspicion& other);

	virtual ~Suspicion();

	//===
	StringSPtr getReporting() const
	{
		return reporting;
	}

	StringSPtr getSuspected() const
	{
		return suspected;
	}

	NodeVersion getSuspectedNodeVersion() const
	{
		return suspectedNodeVersion;
	}

	void setSuspectedNodeVersion(NodeVersion ver)
	{
		suspectedNodeVersion = ver;
	}

	bool equalsWithVer(const Suspicion& other) const;

	//===
	/**
	 * For sort and STL containers, orders by suspected-node and then reporting-node.
	 *
	 * @param lhs
	 * @param rhs
	 * @return
	 */
	friend
	bool operator<(const Suspicion& lhs, const Suspicion& rhs);

	/**
	 * For sort and STL containers, equals if suspected-node == reporting-node.
	 *
	 * @param lhs
	 * @param rhs
	 * @return
	 */
	friend
	bool operator==(const Suspicion& lhs, const Suspicion& rhs);

	//TODO add hash_value

private:
	StringSPtr reporting;
	StringSPtr suspected;
	NodeVersion suspectedNodeVersion;

};

}

#endif /* SUSPICION_H_ */
