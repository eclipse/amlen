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

#ifndef MCP_MCPCONFIG_H_
#define MCP_MCPCONFIG_H_

#include <boost/shared_ptr.hpp>

#include "BasicConfig.h"
#include "SpiderCastLogicError.h"

namespace mcp {

namespace trace
{
//=== component name ===
extern const char* Component_Name;          // = "ISM_Cluster";

//=== sub-component names ===
extern const char* SubComponent_Core;       // = "Core";
extern const char* SubComponent_Control;    // = "Control";
extern const char* SubComponent_Local;      // = "Local";
extern const char* SubComponent_Global;     // = "Global";
}

/**
 * Documented configuration property keys.
 *
 * The global variables under this namespace ending with the suffix _PROP_KEY, are the
 * documented public property keys that are accepted by MCPConfig.
 * The corresponding variables that end with the suffix _DEFVALUE are the default
 * values of those properties.
 *
 * @see MCPConfig
 */
namespace config {
//=== General ===============================================

/**
 * The local server unique ID.
 *
 * Mandatory property.
 *
 * This is also the SpiderCast node name.
 *
 * From SpiderCastConfig.h:
 *
 * Value - a string with no white spaces, only graphical characters.
 * In addition the name cannot contain control characters, commas,
 * double quotation marks, backslashes or equal signs.
 * The string '$ANY' cannot be used as it is reserved for marking
 * nameless discovery nodes in the bootstrap set.
 */
const std::string LocalServerUID_PROP_KEY = "mcp.LocalServerUID";


/**
 * The local human readable (display) name.
 *
 * Mandatory property.
 *
 * The name is not necessarily unique.
 *
 * May be empty.
 */
const std::string LocalServerName_PROP_KEY = "mcp.LocalServerName";


/**
 * The cluster name.
 *
 * Mandatory property.
 *
 * This is also the SpiderCast bus name (with a '/' prefix).
 *
 * The ClusterName must not have leading or trailing spaces and cannot contain
 * control characters, commas, double quotation marks, backslashes, slashes,
 * or equal signs. The first character must not be a number or any of the
 * following special characters: ! # $ % & ' ( ) * + - . : ; < > ? @
 */
const std::string ClusterName_PROP_KEY = "mcp.ClusterName";

/**
 * Discovery timeout.
 *
 * Optional property.
 *
 * The discovery period is defined as the time between the end of recovery completed,
 * until the the component moves to the active state (when routing actually begins).
 * This is the maximal time until the server moves from "Recovered" state, after recovery completed,
 * to the "Active" state.
 *
 * Default: 10s
 */
const std::string DiscoveryTimeoutMillis_PROP_KEY = "mcp.DiscoveryTimeoutMillis";

const int32_t DiscoveryTimeoutMillis_DEF_VALUE = 10000;

//=== Forwarding ============================================
/**
 * The local data forwarding address.
 *
 * Optional property.
 *
 * This is the local data forwarding address.
 * MCP routing will publish this to the rest of the cluster.
 *
 * Value - a string encoding an explicit IP address in dotted decimal form (IPv4), or hexadecimal notation (IPv6).
 * The address can be a host name.
 *
 * e.g. "10.10.10.123", or "myserver.ibm.com"
 *
 * Default: "" (empty)
 */
const std::string LocalForwadingAddress_PROP_KEY = "mcp.LocalForwardingAddress";

/**
 * The local data forwarding port.
 *
 * Optional property.
 *
 * This is the local data forwarding port.
 * MCP routing will publish this to the rest of the cluster.
 *
 * Value - an integer in the range [1-65535].
 *
 * Default: 0
 */
const std::string LocalForwadingPort_PROP_KEY = "mcp.LocalForwardingPort";

/**
 * The local data use TLS flag.
 *
 * Optional property.
 *
 * This is the local data forwarding use-TLS flag.
 * MCP routing will publish this to the rest of the cluster.
 *
 * Value - an integer 0/1
 *
 * Default: 0
 */
const std::string LocalForwadingUseTLS_PROP_KEY = "mcp.LocalForwardingUseTLS";


//=== Bloom filters =========================================

/**
 * The Bloom filter error rate.
 *
 * Optional property.
 *
 * Value - a double, in (0,1).
 *
 * Default: 0.01
 *
 * Note:
 * p=0.01 => 9.6 bits/item, k=7
 * p=0.005 => 11 bits/item, k=8
 * p=0.002 => 13 bits/item, k=9
 * p=0.001 => 14 bits/item, k=10
 */
const std::string BloomFilterErrorRate_PROP_KEY = "mcp.BloomFilter.ErrorRate";

const double BloomFilterErrorRate_DEFVALUE = 0.01;

/**
 * The Bloom filter projected number of items (topics).
 *
 * Optional property.
 *
 * Value - an integer
 *
 * Default: 20000
 */
const std::string BloomFilterProjectedNumberOfElements_PROP_KEY = "mcp.BloomFilter.ProjectedNumberOfElements";

const int32_t BloomFilterProjectedNumberOfElements_DEFVALUE = 20000;

/**
 * The counting Bloom filter counter size (bits).
 *
 * Optional property.
 *
 * Value - an integer, either 4 or 8
 *
 * Default: 8
 */
const std::string BloomFilterCounterSize_PROP_KEY = "mcp.BloomFilter.CounterSize";

const int32_t BloomFilterCounterSize_DEFVALUE = 8; //change to 4 when the overflow set in the CBF is ready

/**
 * The maximal number of attributes dedicated to Bloom filter updates before a new base is published.
 *
 * Optional property.
 *
 * Value - an integer, >=10
 *
 * Default: 1000
 */
const std::string BloomFilterMaxAttributes_PROP_KEY = "mcp.BloomFilter.MaxAttributes";

const int32_t BloomFilterMaxAttributes_DEFVALUE = 1000;

/**
 * The interval in milliseconds of the task that publishes local filters, bloom filters and otherwise.
 *
 * Optional property.
 *
 * Value - an integer, >=0;
 *
 * Default: 100
 */
const std::string BloomFilterPublishTaskIntervalMillis_PROP_KEY = "mcp.BloomFilter.PublishTaskIntervalMillis";

const int32_t BloomFilterPublishTaskIntervalMillis_DEFVALUE = 100;

/**
 * The hash function type used by the bloom filter.
 *
 * This value must be consistent across the cluster.
 *
 * Optional property.
 *
 * Enumerated string, see below:
 * MURMUR3_x64_128_LC, MURMUR3_x64_128_CH, City64_LC, City64_CH
 *
 * Default: City64_LC
 */
const std::string BloomFilterHashFunctionType_PROP_KEY = "mcp.BloomFilter.HashFunctionType";

/**
 * Murmur3 x64 128bit (Public domain) with linear combinations
 */
const std::string BloomFilterHashFunctionType_MURMUR3_x64_128_LC_VALUE = "mcp.BloomFilter.HashFunctionType.MURMUR3_x64_128_LC";
/**
 * Murmur3 x64 128bit (Public domain) with seed chaining
 */
const std::string BloomFilterHashFunctionType_MURMUR3_x64_128_CH_VALUE = "mcp.BloomFilter.HashFunctionType.MURMUR3_x64_128_CH";
/**
 * CityHash (from Google) with linear combinations
 */
const std::string BloomFilterHashFunctionType_City64_LC_VALUE = "mcp.BloomFilter.HashFunctionType.City64_LC";
/**
 * CityHash (from Google) with seed chaining
 */
const std::string BloomFilterHashFunctionType_City64_CH_VALUE = "mcp.BloomFilter.HashFunctionType.City64_CH";

const std::string BloomFilterHashFunctionType_DEFVALUE = BloomFilterHashFunctionType_City64_LC_VALUE;

const std::string WildCardTopicTreeLimit_PROP_KEY = "mcp.WildCardTopicTree.Limit";
const uint32_t WildCardTopicTreeLimit_DEFVALUE = 50000;

const std::string WildCardTopicTreeLWM_PROP_KEY = "mcp.WildCardTopicTree.LWM";
const std::string WildCardTopicTreeHWM_PROP_KEY = "mcp.WildCardTopicTree.HWM";
const uint32_t WildCardTopicTreeWMPCT_DEFVALUE = 20;

const std::string WildCardTopicTreePatternFreqMinSize_PROP_KEY = "mcp.BloomFilter.WildCardTopicTreePatternFreqMinSize";
const uint32_t WildCardTopicTreePatternFreqMinSize_DEFVALUE = 500;

//=== Control Manager =========================================================
const std::string DeletedNodeList_CleanInterval_Sec_PROP_KEY = "mcp.DeletedNodeList_CleanInterval_Sec_PROP_KEY";
const uint32_t DeletedNodeList_CleanInterval_Sec_DEFVALUE = 600;

//=== Retain Stats ===============================================
const std::string RetainStats_PublishInterval_Millis_PROP_KEY = "mcp.RetainStats_PublishInterval_Millis";
const uint32_t RetainStats_PublishInterval_Millis_DEFVALUE = 1000;

//=== Engine Stats ===============================================
const std::string EngineStats_Interval_Sec_PROP_KEY = "mcp.EngineStats_Interval_Sec";
const uint32_t EngingStats_Interval_Sec_DEFVALUE = 10;

//=== Recovery ===================================================
const std::string Recovery_With_UID_Change_Allowed_PROP_KEY = "mcp.Recovery_With_UID_Change_Allowed";
const bool Recovery_With_UID_Change_Allowed_DEFVALUE = true;


} /* namespace config */

class MCPConfig: public spdr::BasicConfig {
public:

	virtual ~MCPConfig();

	/**
	 * Construct from properties.
	 * @param properties
	 * @return
	 */
	explicit MCPConfig(const spdr::PropertyMap& properties);

	/**
	 * Copy constructor.
	 * @param config
	 * @return
	 */
	MCPConfig(const MCPConfig& config);

	/**
	 * Assignment operator.
	 * @param config
	 * @return
	 */
	MCPConfig& operator=(const MCPConfig& config);

	/**
	 * A string representation of the configuration.
	 *
	 * @return
	 */
	virtual std::string toString() const;

	//=== Getters ===================================================

	double getBloomFilterErrorRate() const
	{
		return bloomFilterErrorRate;
	}

	int32_t getBloomFilterCounterSize() const
	{
		return bloomFilterCounterSize;
	}

	int32_t getBloomFilterProjectedNumElements() const
	{
		return bloomFilterProjectedNumElements;
	}

	int32_t getBloomFilterMaxAttributes() const
	{
		return bloomFilterMaxAttributes;
	}

	const int getBloomFilterHashType() const
	{
		return bloomFilterHashType;
	}

	const std::string& getLocalForwardingAddress() const
	{
		return localForwardingAddress;
	}

	int32_t getLocalForwardingPort() const
	{
		return localForwardingPort;
	}

	uint8_t getLocalForwardingUseTLS() const
	{
		return localForwardingUseTLS;
	}

	int32_t getPublishLocalBFTaskIntervalMillis() const
	{
		return publishLocalBFTaskIntervalMillis;
	}

	uint32_t getWcttLimit() const
	{
		return wcttLimit;
	}

	uint32_t getWcttLwm() const
	{
		return wcttLwm;
	}

	uint32_t getWcttHwm() const
	{
		return wcttHwm;
	}

	uint32_t getWcttPatMinSize() const
	{
		return wcttPatMinSize;
	}

	const std::string& getClusterName() const
	{
		return clusterName;
	}

	const std::string& getServerName() const
	{
		return serverName;
	}

	const std::string& getServerUID() const
	{
		return serverUID;
	}

	int32_t getDiscoveryTimeoutMillis() const
	{
		return discoveryTimeoutMillis;
	}

    int32_t getPublishRetainedStatsIntervalMillis() const
    {
        return publishRetainedStatsIntervalMillis;
    }

    int32_t getEngineStatsIntervalSec() const
    {
        return engineStatsIntervalSec;
    }

    int32_t getDeletedNodeListCleanIntervalSec() const
    {
        return deletedNodeListCleanIntervalSec;
    }

    bool isRecoveryWithUIDChangeAllowed() const
    {
        return recoveryWithUIDChangeAllowed;
    }

protected:
	void setProperty(const std::string& propName, const std::string& value);
	void setDoubleProperty(const std::string& propName, const double value);
	void setInt32Property(const std::string& propName, const int32_t value);
	void setInt64Property(const std::string& propName, const int64_t value);
	void setBooleanProperty(const std::string& propName, const bool value);

private:
	//NOTE: remember to update constructor & copy-constructor & operator= when adding fields.

	std::string serverUID;
	std::string serverName;
	std::string clusterName;

	int32_t discoveryTimeoutMillis;

	double bloomFilterErrorRate;
	int32_t bloomFilterProjectedNumElements;
	int32_t bloomFilterCounterSize;
	int32_t bloomFilterMaxAttributes;
	int32_t bloomFilterHashType;

	uint32_t wcttLimit;
	uint32_t wcttHwm;
	uint32_t wcttLwm;
	uint32_t wcttPatMinSize;

	std::string localForwardingAddress;
	int32_t localForwardingPort;
	uint8_t localForwardingUseTLS;

	int32_t publishLocalBFTaskIntervalMillis;

	int32_t publishRetainedStatsIntervalMillis;

	int32_t engineStatsIntervalSec;

	int32_t deletedNodeListCleanIntervalSec;

	bool recoveryWithUIDChangeAllowed;


	void initProperties();

};

typedef boost::shared_ptr<MCPConfig> MCPConfig_SPtr;

} /* namespace mcp */

#endif /* MCP_MCPCONFIG_H_ */
