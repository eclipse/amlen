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

/*-----------------------------------------------------------------
 * File        : ScTraceContext.h
 * Author      :
 * Version     : $Revision: 1.4 $
 * Last updated: $Date: 2015/11/18 08:37:01 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceContext.h,v $
 * Revision 1.4  2015/11/18 08:37:01 
 * Update copyright headers
 *
 * Revision 1.3  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.5  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.4  2011/02/27 12:20:20 
 * Refactoring, header file organization
 *
 * Revision 1.3  2010/06/27 12:38:47 
 * *** empty log message ***
 *
 * Revision 1.2  2010/06/15 06:32:52 
 * *** empty log message ***
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Trace context declaration
 */
#ifndef SC_TRACE_CONTEXT_H
#define SC_TRACE_CONTEXT_H

#include <string>

#include "ScTr.h"

namespace spdr
{

typedef std::auto_ptr<class ScTraceContext> ScTraceContextAPtr;

class  ScTraceContext {
protected:
    const ScTraceComponent*   _tc;
    std::string               _instanceID;
    std::string               _memberName;

    ScTraceContext(const ScTraceComponent* tc, const std::string& instance, const std::string& member):
    	_tc(tc), _instanceID(instance), _memberName(member)
    {
    	//std::cout << "ScTraceContext" << std::endl;
    }

public:
    virtual ~ScTraceContext()
    {
    	//std::cout << "~ScTraceContext() " << std::endl;
    }

    virtual const ScTraceComponent* getTraceComponent() const{
        return _tc;
    }

    virtual const std::string&   	getInstanceID() const {
        return _instanceID;
    }

    virtual const std::string&      getMemberName() const {
        return _memberName;
    }
};

class ScTraceContextImpl : public ScTraceContext {
private:

    const ScTraceContextImpl& operator=(const ScTraceContextImpl& traceContext){
        //sysAssert(0);
        return *this; 
    }
public: 
    ScTraceContextImpl(const ScTraceComponent *tc, const std::string &instance,
						const std::string &memberName): ScTraceContext (tc, instance, memberName){
    }
    
    ScTraceContextImpl(const ScTraceComponent *tc, const ScTraceContext &ctxt):
						ScTraceContext (tc, ctxt.getInstanceID(), ctxt.getMemberName()){

    }

    ScTraceContextImpl(const ScTraceContext &ctxt):
						ScTraceContext (ctxt.getTraceComponent(), ctxt.getInstanceID(), ctxt.getMemberName()) {
    }

    virtual ~ScTraceContextImpl() {
    }

};
}

#endif
