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
 * SpiderCastFactory.cpp
 *
 *  Created on: 28/01/2010
 *
 */

#include <boost/thread/once.hpp>

#include "SpiderCastFactory.h"
#include "SpiderCastFactoryImpl.h"
//#include <iostream>

namespace spdr
{

using namespace std;

boost::once_flag SpiderCastFactory::flag = BOOST_ONCE_INIT;
boost::scoped_ptr<SpiderCastFactory> SpiderCastFactory::instanceScopedPtr(0);

//ScTraceComponent* SpiderCastFactory::tc_ = ScTr::enroll(
//		trace::ScTrConstants::ScTr_Component_Name,
//		trace::ScTrConstants::ScTr_SubComponent_Core,
//		trace::ScTrConstants::Layer_ID_Core,
//		"SpiderCastFactory",
//		trace::ScTrConstants::ScTr_ResourceBundle_Name);

void SpiderCastFactory::init()
{
//	if (ScTraceBuffer::isEntryEnabled(tc_))
//	{
//		ScTr::entry(tc_,"SpiderCastFactory.init()");
//	}
	instanceScopedPtr.reset(new (std::nothrow) SpiderCastFactoryImpl());
}

SpiderCastFactory& SpiderCastFactory::getInstance()
{
//	if (ScTraceBuffer::isEntryEnabled(tc_))
//	{
//		ScTr::entry(tc_,"SpiderCastFactory.getInstance().Entry");
//	}

	boost::call_once(init, flag);
	return *instanceScopedPtr;
}

SpiderCastFactory::SpiderCastFactory() //: ScTraceContext(tc_,"","")
{
	//std::cout << "SpiderCastFactory()" << std::endl;
//	Trace_Entry((ScTraceContext*)this, "SpiderCastFactory()");
}

SpiderCastFactory::~SpiderCastFactory()
{
	//std::cout << "~SpiderCastFactory()" << std::endl;
//	Trace_Entry((ScTraceContext*)this, "~SpiderCastFactory()");
}

}
