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
 * BusName.h
 *
 *  Created on: Sep 12, 2010
 *      Author:
 */

#ifndef BUSNAME_H_
#define BUSNAME_H_

#include "Definitions.h"
#include "SpiderCastLogicError.h"
//#include "ConcurrentSharedPtr.h"

namespace spdr
{

class BusName;
typedef boost::shared_ptr<BusName>  BusName_SPtr;

class BusName
{
private:
	String _busName;
	String _busL1Name;
	String _busL2Name;
public:
	/**
	 * Construct a bus-name, checking argument validity.
	 *
	 * Format:
	 * "/L1"
	 * "/L1/L2"
	 *
	 * @param busName as a C-string
	 * @return
	 * @throw IllegalArgumentException if busName is ill-formed.
	 */
	BusName(const char * busName) throw(IllegalArgumentException);
	virtual ~BusName();

	bool isEqual(const BusName& other) const;
	bool isManager(const BusName& other) const;
	bool isManaged(const BusName& other) const;

	int comparePrefix(const BusName& other) const;

	int getLevel() const;

	String toString() const;
	String toShortString() const;
	String toOrgString() const;

private:
	static void validateBusName(const std::string& level);

};

}

#endif /* BUSNAME_H_ */
