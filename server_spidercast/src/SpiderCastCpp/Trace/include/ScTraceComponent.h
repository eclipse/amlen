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
 * File        : ScTraceComponent.h
 * Author      :
 * Version     : $Revision: 1.6 $
 * Last updated: $Date: 2015/12/24 10:23:08 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceComponent.h,v $
 * Revision 1.6  2015/12/24 10:23:08 
 * improvements to trace
 *
 * Revision 1.5  2015/11/18 08:37:01 
 * Update copyright headers
 *
 * Revision 1.4  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.3  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.3  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.2  2010/06/27 12:38:46 
 * *** empty log message ***
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Trace components declaration
 */
#ifndef SCTRACE_componentName_H
#define SCTRACE_componentName_H


#include <string>

#include "ScTrConstants.h"
#include "LogListener.h"


namespace spdr
{

class ScTraceComponent{
private:
    const std::string            _componentName;
    const std::string            _subcomponentName;
    const int	            _subcomponentID;
    const std::string            _traceID;
    const std::string            _tracePrefix;
    const std::string            _resourceBundleName;

    volatile uint8_t        _level;
    volatile int            _trConfig;

    ScTraceComponent(const char *componentName, const char *subcomponentName,
                   int subcomponentID, const char *traceID, const char *resourceBundleName,
                   int trConfig) :
            _componentName(componentName),
            _subcomponentName(subcomponentName),
			_subcomponentID(subcomponentID),
            _traceID(traceID),
            _tracePrefix(makeTrPrefix(componentName,subcomponentName,traceID)),
            _resourceBundleName(resourceBundleName),
            _level(static_cast<uint8_t>(trConfig & spdr::trace::ScTrConstants::Level_Mask)),
            _trConfig(trConfig)
    {
        updateTrConfig(trConfig);  
    }

    ~ScTraceComponent()
    {
    } 

    ScTraceComponent(const ScTraceComponent& tc) :
            _componentName(tc._componentName),
            _subcomponentName(tc._subcomponentName),
            _subcomponentID(tc._subcomponentID),
            _traceID(tc._traceID),
            _tracePrefix(tc._tracePrefix),
            _resourceBundleName(tc._resourceBundleName),
            _level(tc._level),
            _trConfig(tc._trConfig)
    {
    }

    const ScTraceComponent& operator=(const ScTraceComponent& traceComponent)
    {
        // Do not use assignment.
        //sysAssert(0);
        return *this; 
    }

    void updateTrConfig(int trConfig)
    {
    	_level = static_cast<uint8_t>(trConfig & spdr::trace::ScTrConstants::Level_Mask);
        _trConfig = trConfig;
    }

    int getTrConfig(void) const
    {
        return _trConfig;
    }

    static std::string  makeTrPrefix(
    		const char *componentName,
    		const char *subcomponentName,
            const char *traceID)
    {
        std::string rc(componentName);
        rc += ".";
        rc += subcomponentName;
        rc += ".";
        rc += traceID;
        return rc;
    }

public:

	bool isErrorEnabeled() const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_ERROR);
	}

	bool isWarningEnabeled() const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_WARNING);
	}

	bool isInfoEnabeled() const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_INFO);
	}

	bool isConfigEnabeled() const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_CONFIG);
	}

	bool isEventEnabled(void) const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_EVENT);
	}

	bool isDebugEnabled(void) const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_DEBUG);
	}

	bool isEntryEnabled(void) const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_ENTRY);
	}

	bool isDumpEnabled(void) const
	{
		return _level >= static_cast<uint8_t>(spdr::log::Level_DUMP);
	}

	bool isLevelEnabled(int level) const
	{
		if (level > 0)
			return (_level >= level);
		else
			return false;
	}

    const std::string& getComponentName(void)const{return _componentName;}
    const std::string& getSubcomponentName(void) const {return _subcomponentName;}
	int   getSubcomponentID(void) const {return _subcomponentID;}
    const std::string& getTraceId(void) const {return _traceID;}
    const std::string& getTracePrefix(void) const {return _tracePrefix;}
    const std::string& getResourceBundleName(void) const {return _resourceBundleName;}

    friend class ScTr;
};

}

#endif
