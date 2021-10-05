/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <MCPConfig.h>
#include "hashFunction.h"

namespace mcp
{

namespace trace
{
const char* Component_Name = "ISM_Cluster";
const char* SubComponent_Core = "Core";
const char* SubComponent_Control = "Control";
const char* SubComponent_Local = "Local";
const char* SubComponent_Global = "Global";
}

MCPConfig::MCPConfig(const spdr::PropertyMap& properties) :
		BasicConfig(properties)
{
	initProperties();
}

//UPDATE
MCPConfig::MCPConfig(const MCPConfig& config) :
		BasicConfig(config.prop),
		serverUID(config.serverUID),
		serverName(config.serverName),
		clusterName(config.clusterName),
		discoveryTimeoutMillis(config.discoveryTimeoutMillis),
		bloomFilterErrorRate(config.bloomFilterErrorRate),
		bloomFilterProjectedNumElements(config.bloomFilterProjectedNumElements),
		bloomFilterCounterSize(config.bloomFilterCounterSize),
		bloomFilterMaxAttributes(config.bloomFilterMaxAttributes),
		bloomFilterHashType(config.bloomFilterHashType),
		wcttLimit(config.wcttLimit),
		wcttHwm(config.wcttHwm),
		wcttLwm(config.wcttLwm),
		wcttPatMinSize(config.wcttPatMinSize),
		localForwardingAddress(config.localForwardingAddress),
		localForwardingPort(config.localForwardingPort),
		localForwardingUseTLS(config.localForwardingUseTLS),
		publishLocalBFTaskIntervalMillis(config.publishLocalBFTaskIntervalMillis),
		publishRetainedStatsIntervalMillis(config.publishRetainedStatsIntervalMillis),
		engineStatsIntervalSec(config.engineStatsIntervalSec),
		deletedNodeListCleanIntervalSec(config.deletedNodeListCleanIntervalSec),
		recoveryWithUIDChangeAllowed(config.recoveryWithUIDChangeAllowed)

{
}

//UPDATE
MCPConfig& MCPConfig::operator =(const MCPConfig& config)
{
	BasicConfig::prop = config.prop;
	serverUID = config.serverUID;
	serverName = config.serverName;
	clusterName = config.clusterName;
	discoveryTimeoutMillis = config.discoveryTimeoutMillis;
	bloomFilterErrorRate = config.bloomFilterErrorRate;
	bloomFilterProjectedNumElements = config.bloomFilterProjectedNumElements;
	bloomFilterCounterSize = config.bloomFilterCounterSize;
	bloomFilterMaxAttributes = config.bloomFilterMaxAttributes;
	bloomFilterHashType = config.bloomFilterHashType;
	wcttLimit = config.wcttLimit;
	wcttHwm = config.wcttHwm;
	wcttLwm = config.wcttLwm;
	wcttPatMinSize = config.wcttPatMinSize;
	localForwardingAddress = config.localForwardingAddress;
	localForwardingPort = config.localForwardingPort;
	localForwardingUseTLS = config.localForwardingUseTLS;
	publishLocalBFTaskIntervalMillis = config.publishLocalBFTaskIntervalMillis;
    publishRetainedStatsIntervalMillis = config.publishRetainedStatsIntervalMillis;
    engineStatsIntervalSec = config.engineStatsIntervalSec;
    deletedNodeListCleanIntervalSec = config.deletedNodeListCleanIntervalSec;
    recoveryWithUIDChangeAllowed = config.recoveryWithUIDChangeAllowed;


	return *this;
}

MCPConfig::~MCPConfig()
{
}

//UPDATE
void MCPConfig::initProperties()
{
    using namespace mcp::config;

    serverUID = getMandatoryProperty(LocalServerUID_PROP_KEY);
    serverName = getMandatoryProperty(LocalServerName_PROP_KEY);
    clusterName = getMandatoryProperty(ClusterName_PROP_KEY);

    discoveryTimeoutMillis = getOptionalInt32Property(DiscoveryTimeoutMillis_PROP_KEY,DiscoveryTimeoutMillis_DEF_VALUE);

    localForwardingAddress = getOptionalProperty(LocalForwadingAddress_PROP_KEY,"");
    localForwardingPort = getOptionalInt32Property(LocalForwadingPort_PROP_KEY,-1);
    localForwardingUseTLS = getOptionalInt32Property(LocalForwadingUseTLS_PROP_KEY,0);

    //NOTE: add/update resolved properties to the map

    bloomFilterErrorRate = getOptionalDoubleProperty(
            BloomFilterErrorRate_PROP_KEY, BloomFilterErrorRate_DEFVALUE);
    if (bloomFilterErrorRate >= 1 || bloomFilterErrorRate <= 0)
    {
        std::ostringstream what;
        what << BloomFilterErrorRate_PROP_KEY << " out of range (0,1): "
                << bloomFilterErrorRate;
        throw spdr::IllegalConfigException(what.str());
    }
    setDoubleProperty(BloomFilterErrorRate_PROP_KEY, bloomFilterErrorRate);

    bloomFilterProjectedNumElements = getOptionalInt32Property(
            BloomFilterProjectedNumberOfElements_PROP_KEY,
            BloomFilterProjectedNumberOfElements_DEFVALUE);
    if (bloomFilterProjectedNumElements < 1)
    {
        std::ostringstream what;
        what << BloomFilterProjectedNumberOfElements_PROP_KEY << " must be >0: "
                << bloomFilterProjectedNumElements;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(BloomFilterProjectedNumberOfElements_PROP_KEY, bloomFilterProjectedNumElements);

    bloomFilterCounterSize = getOptionalInt32Property(
            BloomFilterCounterSize_PROP_KEY, BloomFilterCounterSize_DEFVALUE);
    if (bloomFilterCounterSize != 4 && bloomFilterCounterSize != 8)
    {
        std::ostringstream what;
        what << BloomFilterCounterSize_PROP_KEY << " must be {4|8}: "
                << bloomFilterCounterSize;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(BloomFilterCounterSize_PROP_KEY,bloomFilterCounterSize);

    bloomFilterMaxAttributes = getOptionalInt32Property(
            BloomFilterMaxAttributes_PROP_KEY, BloomFilterMaxAttributes_DEFVALUE);
    if (bloomFilterMaxAttributes < 10)
    {
        std::ostringstream what;
        what << BloomFilterMaxAttributes_PROP_KEY << " must be >= 10, "
                << bloomFilterMaxAttributes;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(BloomFilterMaxAttributes_PROP_KEY,bloomFilterMaxAttributes);

    std::string hashType = getOptionalProperty(
            BloomFilterHashFunctionType_PROP_KEY,
            BloomFilterHashFunctionType_DEFVALUE);
    if (hashType == BloomFilterHashFunctionType_City64_CH_VALUE)
    {
        bloomFilterHashType = ISM_HASH_TYPE_CITY64_CH;
    }
    else if ( hashType == BloomFilterHashFunctionType_City64_LC_VALUE )
    {
        bloomFilterHashType = ISM_HASH_TYPE_CITY64_LC;
    }
    else if ( hashType == BloomFilterHashFunctionType_MURMUR3_x64_128_LC_VALUE)
    {
        bloomFilterHashType = ISM_HASH_TYPE_MURMUR_x64_128_LC;
    }
    else if ( hashType == BloomFilterHashFunctionType_MURMUR3_x64_128_CH_VALUE)
    {
        bloomFilterHashType = ISM_HASH_TYPE_MURMUR_x64_128_CH;
    }
    else
    {
        std::ostringstream what;
        what << "Illegal hash type: " << BloomFilterHashFunctionType_PROP_KEY
                << " = " << hashType;
        throw spdr::IllegalConfigException(what.str());
    }
    setProperty(BloomFilterHashFunctionType_PROP_KEY, hashType);

    publishLocalBFTaskIntervalMillis = getOptionalInt32Property(
            BloomFilterPublishTaskIntervalMillis_PROP_KEY,
            BloomFilterPublishTaskIntervalMillis_DEFVALUE);
    if (publishLocalBFTaskIntervalMillis < 0)
    {
        std::ostringstream what;
        what << BloomFilterPublishTaskIntervalMillis_PROP_KEY
                << " must be >= 0, " << publishLocalBFTaskIntervalMillis;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(BloomFilterPublishTaskIntervalMillis_PROP_KEY,publishLocalBFTaskIntervalMillis);

    wcttLimit = getOptionalInt32Property(WildCardTopicTreeLimit_PROP_KEY,WildCardTopicTreeLimit_DEFVALUE);
    if (wcttLimit < 0)
    {
        std::ostringstream what;
        what << WildCardTopicTreeLimit_PROP_KEY	<< " must be >= 0, " << wcttLimit;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(WildCardTopicTreeLimit_PROP_KEY, wcttLimit);

    wcttHwm = getOptionalInt32Property(WildCardTopicTreeHWM_PROP_KEY,(100+WildCardTopicTreeWMPCT_DEFVALUE)*wcttLimit/100);
    if (wcttHwm < wcttLimit)
    {
        std::ostringstream what;
        what << WildCardTopicTreeHWM_PROP_KEY	<< " must be >= the value of " << WildCardTopicTreeLimit_PROP_KEY << "(" << wcttLimit << ")";
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(WildCardTopicTreeHWM_PROP_KEY, wcttHwm);

    wcttLwm = getOptionalInt32Property(WildCardTopicTreeLWM_PROP_KEY,(100-WildCardTopicTreeWMPCT_DEFVALUE)*wcttLimit/100);
    if (wcttLwm > wcttLimit)
    {
        std::ostringstream what;
        what << WildCardTopicTreeLWM_PROP_KEY	<< " must be <= the value of " << WildCardTopicTreeLimit_PROP_KEY << "(" << wcttLimit << ")";
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(WildCardTopicTreeHWM_PROP_KEY, wcttHwm);

    wcttPatMinSize = getOptionalInt32Property(WildCardTopicTreePatternFreqMinSize_PROP_KEY,WildCardTopicTreePatternFreqMinSize_DEFVALUE);
    if ( wcttPatMinSize > wcttLimit/20 && wcttPatMinSize == WildCardTopicTreePatternFreqMinSize_DEFVALUE )
        wcttPatMinSize = wcttLimit/20;
    if (wcttPatMinSize < 0)
    {
        std::ostringstream what;
        what << WildCardTopicTreePatternFreqMinSize_PROP_KEY	<< " must be >= 0, " << wcttPatMinSize;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(WildCardTopicTreePatternFreqMinSize_PROP_KEY, wcttPatMinSize);

    publishRetainedStatsIntervalMillis = getOptionalInt32Property(
            RetainStats_PublishInterval_Millis_PROP_KEY,
            RetainStats_PublishInterval_Millis_DEFVALUE);
    if (publishRetainedStatsIntervalMillis < 0)
    {
        std::ostringstream what;
        what << RetainStats_PublishInterval_Millis_PROP_KEY
                << " must be >= 0, " << publishRetainedStatsIntervalMillis;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(RetainStats_PublishInterval_Millis_PROP_KEY,publishRetainedStatsIntervalMillis);

    engineStatsIntervalSec = getOptionalInt32Property(
            EngineStats_Interval_Sec_PROP_KEY,
            EngingStats_Interval_Sec_DEFVALUE);
    if (engineStatsIntervalSec < 1)
    {
        std::ostringstream what;
        what << EngineStats_Interval_Sec_PROP_KEY
                << " must be >= 1, " << engineStatsIntervalSec;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(EngineStats_Interval_Sec_PROP_KEY,engineStatsIntervalSec);

    deletedNodeListCleanIntervalSec = getOptionalInt32Property(
            DeletedNodeList_CleanInterval_Sec_PROP_KEY,
            DeletedNodeList_CleanInterval_Sec_DEFVALUE);
    if (deletedNodeListCleanIntervalSec < 5)
    {
        std::ostringstream what;
        what << DeletedNodeList_CleanInterval_Sec_PROP_KEY
                << " must be >= 5, " << deletedNodeListCleanIntervalSec;
        throw spdr::IllegalConfigException(what.str());
    }
    setInt32Property(DeletedNodeList_CleanInterval_Sec_PROP_KEY,deletedNodeListCleanIntervalSec);

    recoveryWithUIDChangeAllowed = getOptionalBooleanProperty(
            Recovery_With_UID_Change_Allowed_PROP_KEY,
            Recovery_With_UID_Change_Allowed_DEFVALUE);
    setBooleanProperty(Recovery_With_UID_Change_Allowed_PROP_KEY,recoveryWithUIDChangeAllowed);



}

std::string MCPConfig::toString() const
{
	std::ostringstream oss;
	oss << "MCPConfig: " << BasicConfig::toString();
	return oss.str();
}

void MCPConfig::setProperty(const std::string& propName,
		const std::string& value)
{
	prop.setProperty(propName,value);
}

void MCPConfig::setDoubleProperty(const std::string& propName,
		const double value)
{
	prop.setProperty(propName, boost::lexical_cast<std::string>(value));
}

void MCPConfig::setInt32Property(const std::string& propName,
		const int32_t value)
{
	prop.setProperty(propName, boost::lexical_cast<std::string>(value));
}
void MCPConfig::setInt64Property(const std::string& propName,
		const int64_t value)
{
	prop.setProperty(propName, boost::lexical_cast<std::string>(value));
}
void MCPConfig::setBooleanProperty(const std::string& propName,
		const bool value)
{
	prop.setProperty(propName, boost::lexical_cast<std::string>(value));
}

} /* namespace mcp */

