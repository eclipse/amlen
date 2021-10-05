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
 * HierarchyViewListener.h
 *
 *  Created on: Jan 18, 2011
 */

#ifndef HIERARCHYVIEWLISTENER_H_
#define HIERARCHYVIEWLISTENER_H_

namespace spdr
{

class HierarchyViewListener
{
public:
	HierarchyViewListener();
	virtual ~HierarchyViewListener();

	virtual void hierarchyViewChanged() = 0;

	virtual void globalViewChanged() = 0;
};

}

#endif /* HIERARCHYVIEWLISTENER_H_ */
