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

#include <stdint.h>
#include <time.h>

/* for translating  interface to IP */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* ***********************************/

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "ismLogListener.h"
#include "mccTrace.h"

#include "cluster.h"
#include "ismutil.h"
#include "config.h"

#include "MCPRouting.h"
#include "EngineEventCallbackCAdapter.h"
#include "ForwardingControlCAdapter.h"
#include "CyclicFileLogger.h"
#include "SpiderCastFactory.h"
#include "SpiderCastConfig.h"
#include "PropertyMap.h"
#include "MCPConfig.h"

static boost::shared_ptr<mcp::CyclicFileLogger> cyclicFileLogger_SPtr;
static boost::shared_ptr<mcp::ismLogListener> ll_SPtr;

static boost::shared_ptr<spdr::PropertyMap> spdrProps_SPtr;
static boost::shared_ptr<spdr::PropertyMap> mcpProps_SPtr;
static boost::shared_ptr<std::vector<spdr::NodeID_SPtr> > spdrBootstrapSet_SPtr;

static boost::tuple<std::string,int,uint8_t> localForwadingInfo;
static int controlTLSPolicy = 1;

static ismCluster_HaStatus_t haStatus_beforeStart = ISM_CLUSTER_HA_UNKNOWN;

//static mcp::MCPRouting* mcpInstance = NULL;
static boost::shared_ptr<mcp::MCPRouting> mcpInstance_SPtr;
static boost::shared_ptr<mcp::EngineEventCallbackCAdapter> engineEventCallbackCAdapter_SPtr;
static boost::shared_ptr<mcp::ForwardingControlCAdapter> forwardingControlCAdapter_SPtr;

extern "C"
{

static int enableClusterFlag = 0; /* 0=false | 1=true */
static int isStandAloneFlag = 0; /* 0=false | 1=true */

/**
 * Resolving a NIC name to a host address.
 *
 * @param nic input NIC name (in)
 * @param host host address (out)
 * @return
 */
int ism_cluster_get_resolve_interface(const char* nic, char* host)
{
    char host_temp[NI_MAXHOST];

    TRACE(9, "Entry: %s NIC=%s\n", __FUNCTION__,SoN(nic));

    if (nic == NULL)
    {
        TRACE(1, "Error: %s, argument 'nic' is NULL, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    struct ifaddrs *ifaddr, *ifa;
    //int family;
    int s;

    if (getifaddrs(&ifaddr) == -1)
    {
        TRACE(1, "Error: %s getifaddrs failed, rc=%u\n", __FUNCTION__,ISMRC_Error);
        return ISMRC_Error;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host_temp,
                NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if ((strcmp(ifa->ifa_name, nic) == 0)
                && (ifa->ifa_addr->sa_family == AF_INET))
        {
            if (s != 0)
            {
                //printf("getnameinfo() failed: %s\n", gai_strerror(s));
                TRACE(1, "Error: %s getnameinfo failed, rc=%u\n", __FUNCTION__,ISMRC_Error);
                return ISMRC_Error;
            }
            //printf("\tControl Interface : <%s>\n",ifa->ifa_name );
            //printf("\tControl Address : <%s>\n", host_temp);
            break;
        }
    }

    freeifaddrs(ifaddr);

    strcpy(host, host_temp);

    TRACE(5, "%s resolved NIC=%s to %s\n", __FUNCTION__,nic,SoN(host));

    TRACE(9, "Exit: %s rc=%u\n", __FUNCTION__,ISMRC_OK);
    return ISMRC_OK;
}

/*********************************************************************/
/* Callbcak to get Cluster run time configuration changes from Admin */
/*                                                                   */
/* Currently there are no Cluster run time settable configuration    */
/* parameters.                                                       */
/* This callack always return ISMRC_OK. It is added to satisfy       */
/* configuration service.                                            */
/*********************************************************************/
int ism_cluster_configCallback(char *pObject, char *pName, ism_prop_t *pProps, ism_ConfigChangeType_t flag)
{
    TRACE(5, "%s Cluster Configuration callback is invoked. Name = %s, flag = %d \n",
            __FUNCTION__, pName, flag);
    return ISMRC_OK;
}

int ism_cluster_initClusterConfig(void)
{
    int rc = ISMRC_OK;
    ism_config_t *hConfig = NULL;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if ((rc = ism_config_register(ISM_CONFIG_COMP_CLUSTER, NULL,
            ism_cluster_configCallback, &hConfig)) != ISMRC_OK)
    {
        TRACE(1, "Error: %s, ism_config_register failed! rc=%u\n", __FUNCTION__, rc);
        return rc;
    }

    ism_prop_t *pProps, *pCfgProps;
    ism_field_t f;
    char propName[256];
    const char *pName, *p1, *p2;

    if ((pProps = ism_config_getPropertiesDynamic(hConfig, NULL, NULL)) != NULL)
    {
        int off = snprintf(propName, 256, "Cluster.");
        pCfgProps = ism_common_getConfigProperties();
        for (int i = 0; ism_common_getPropertyIndex(pProps, i, &pName) == 0; i++)
        {
            if (ism_common_getProperty(pProps, pName, &f))
            {
                continue;
            }
            else
            {
                for (p1 = pName; *p1 != 0 && *p1 != '.'; p1++)
                    ; // empty body
                if (*p1 != '.')
                    continue;
                p2 = ++p1;
                for (; *p1 != 0 && *p1 != '.'; p1++)
                    ; // empty body
                if (*p1 != '.')
                    continue;
                memcpy(propName + off, p2, (p1 - p2));
                propName[off + (p1 - p2)] = 0;
            }
            ism_common_setProperty(pCfgProps, propName, &f);
            TRACE(5, "%s Adding Config var %s\n", __FUNCTION__, propName);
        }
    }
    else
    {
        TRACE(1, "Warning: %s, ism_config_getPropertiesDynamic returned NULL properties",__FUNCTION__);
    }

    if (NULL != pProps)
        ism_common_freeProperties(pProps);

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

int ism_cluster_getTraceLevel()
{
//    int level = 0;
//    for (int i=0; i<10; ++i)
//    {
//        if (SHOULD_TRACE(i))
//        {
//            level = i;
//        }
//        else
//        {
//            break;
//        }
//    }
//    return level;
    return TRACE_LEVEL;
}

int ism_cluster_getTraceLevelSpiderCast()
{
    return TRACE_LEVELX(SpiderCast);
}


int ism_cluster_convert_to_nameless_bss(const char* server_endpoint_list, std::string& bss)
{
    using namespace std;

    string line_str(server_endpoint_list);
    std::vector<string> tokens;
    boost::split(tokens, line_str, boost::is_any_of(std::string(",")));

    ostringstream oss;
    for (unsigned int i=0; i < tokens.size(); ++i)
    {
        string token = tokens[i];
        boost::trim(token);
        if (token.empty())
            continue;

        if (token.find('@') != std::string::npos)
        {
            return ISMRC_Error;
        }

        oss << spdr::NodeID::NAME_ANY << "@" << token;
        if (i < tokens.size()-1)
            oss << ", ";
    }

    bss.append(oss.str());

    return ISMRC_OK;
}

/*
 * Reset all static shared pointers (for unit test, since we keep them after ism_cluster_term)
 */
void destroyStaticSharedPtr()
{
    spdrProps_SPtr.reset();
    mcpProps_SPtr.reset();
    engineEventCallbackCAdapter_SPtr.reset();
    forwardingControlCAdapter_SPtr.reset();
    spdrBootstrapSet_SPtr.reset();
    mcpInstance_SPtr.reset();
}

/*
 * This is declared only in the unit test, and is used to clear these
 * variables when doing repeated back to back tests of the C-API.
 */
extern void ism_cluster_test_destroy()
{
	destroyStaticSharedPtr();
	cyclicFileLogger_SPtr.reset();
	ll_SPtr.reset();
}
/* Startup sequence:
 *
 * init,
 * The following functions can be invoked at any order,
 * except that setLocalForwardingInfo() must be called before start() in order to receive "useTLS" on time.
 * {
 *   registerProtocolEventCallback,
 *   (setLocalForwardingInfo => start),
 *   registerEngineEventCallback,
 * }
 * restoreNodes,
 * addSubscriptions,
 * recoveryCompleted
 *
 * Check the configuration with a bogus node name.
 * Save the PropertyMap for the actual start.
 */
extern int ism_cluster_init()
{
    using namespace mcp;
    using namespace std;

    TRACE(9, "Entry: %s\n", __FUNCTION__);

    if (ism_cluster_initClusterConfig() != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    enableClusterFlag = ism_common_getBooleanConfig(ismCLUSTER_CFG_ENABLECLUSTER, 0);
    TRACE(5,"%s Value of config var %s is %u\n", __FUNCTION__, ismCLUSTER_CFG_ENABLECLUSTER, enableClusterFlag);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    //=== reset all static shared pointers (for unit test, since we keep them after ism_cluster_term) ===
    destroyStaticSharedPtr();

    //=== Get properties: cluster-name, server-name, server-UID ===
    const char* propVal;
    propVal = ism_common_getStringConfig(ismCLUSTER_CFG_CLUSTERNAME);
    TRACE(5,"%s Value of config var %s is %s\n", __FUNCTION__,ismCLUSTER_CFG_CLUSTERNAME,SoN(propVal));
    if (propVal == NULL || !(*propVal))
    {
        TRACE(1,"Error: %s failed, missing property %s rc=%d\n", __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_CLUSTERNAME,"NULL");
        return ISMRC_BadPropertyValue;
    }
    std::string clusterName(propVal);
    if (clusterName.find_first_of('/') != std::string::npos)
    {
        TRACE(1,"Error: %s failed, bad property value, %s (should not contain a '/'): %s, rc=%d\n",
                __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, clusterName.c_str(), ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_CLUSTERNAME,clusterName.c_str());
        return ISMRC_BadPropertyValue;
    }

    propVal = ism_common_getServerName();
    TRACE(5,"%s Value of ism_common_getServerName is %s\n",__FUNCTION__,SoN(propVal));
    std::string nodeName(propVal != NULL ? propVal : "");

    if ((propVal=ism_common_getServerUID()) == NULL)
    {
        propVal = ism_common_generateServerUID();
        TRACE(5,"%s Value of ism_common_generateServerUID is %s\n",__FUNCTION__,SoN(propVal));
    }
    else
    {
        TRACE(5,"%s Value of ism_common_getServerUID is %s\n",__FUNCTION__, SoN(propVal));
    }

    if ( !propVal || !(*propVal) )
    {
        TRACE(1,"Error: %s failed, missing mandatory 'ServerUID', rc=%d\n",__FUNCTION__,ISMRC_Error);
        return ISMRC_Error;
    }

    if (!ism_common_validServerUID(propVal))
    {
        TRACE(1,"Error: %s failed, invalid mandatory 'ServerUID': %s, rc=%d\n",__FUNCTION__,propVal,ISMRC_Error);
        return ISMRC_Error;
    }
    std::string serverUID(propVal);

    //=== Control TLS Policy ===
    controlTLSPolicy = ism_common_getIntConfig(ismCluster_CFG_CONTROL_TLS_POLICY, 1); //default: Always-On
    TRACE(5,"%s Value of config var %s is %d\n", __FUNCTION__,ismCluster_CFG_CONTROL_TLS_POLICY,controlTLSPolicy);
    if (controlTLSPolicy < 1 || controlTLSPolicy > 3)
    {
        TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                __FUNCTION__, ismCluster_CFG_CONTROL_TLS_POLICY, controlTLSPolicy, ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCluster_CFG_CONTROL_TLS_POLICY,controlTLSPolicy);
        return ISMRC_BadPropertyValue;
    }

    //=== Configure logging ===
    isStandAloneFlag = ism_common_getBooleanConfig(ismCLUSTER_CFG_TESTSTANDALONE, 0);
    TRACE(5,"%s Value of config var %s is %u\n",__FUNCTION__,ismCLUSTER_CFG_TESTSTANDALONE,isStandAloneFlag);
    propVal = ism_common_getStringConfig(ismCLUSTER_CFG_LOGFILE);
    TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCLUSTER_CFG_LOGFILE,SoN(propVal));

    if (isStandAloneFlag > 0)
    {
        std::string logFile;
        if (propVal != NULL && (*propVal))
        {
            logFile = propVal;
        }
        else
        {
            //Initialize logging <cluster>_<uid>_<time>_<part>.log
            logFile = clusterName + "_" + serverUID + "_";

            time_t rawtime;
            struct tm * timeinfo;
            char buffer[80];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(buffer, 80, "%y%m%d-%H%M%S", timeinfo);
            logFile.append(buffer);
        }

        cyclicFileLogger_SPtr.reset(new mcp::CyclicFileLogger(logFile.c_str(), "log", 4,
                100 * 1024));
        int ism_log_level_cluster = ism_common_getIntConfig(ismCLUSTER_CFG_LOGLEVEL, 0);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_LOGLEVEL,ism_log_level_cluster);
        spdr::log::Level spdr_log_level = mcp::MCPRouting::ismLogLevel_to_spdrLogLevel(ism_log_level_cluster);
        spdr::SpiderCastFactory::getInstance().registerLogListener(
                cyclicFileLogger_SPtr.get(), spdr_log_level);
        TRACE(5,"%s Configured SpiderCast logger (stand-alone): ISM-log-level=%d, spdr-log-level=%d\n",
                __FUNCTION__, ism_log_level_cluster, static_cast<spdr::log::Level>(spdr_log_level));

    }
    else if (propVal != NULL && (*propVal))
    {
        cyclicFileLogger_SPtr.reset(new mcp::CyclicFileLogger(propVal, "log", 4,
                100 * 1024));
        int ism_log_level_cluster = ism_cluster_getTraceLevel();
        spdr::log::Level spdr_log_level = mcp::MCPRouting::ismLogLevel_to_spdrLogLevel(ism_log_level_cluster);
        spdr::SpiderCastFactory::getInstance().registerLogListener(
                cyclicFileLogger_SPtr.get(), spdr_log_level);
        TRACE(5,"%s Registered SpiderCast logger (separate log file): ISM-log-level=%d, spdr-log-level=%d\n",
                __FUNCTION__, ism_log_level_cluster, static_cast<spdr::log::Level>(spdr_log_level));

        int ism_log_level_spdr = ism_cluster_getTraceLevelSpiderCast();
        spdr_log_level = mcp::MCPRouting::ismLogLevel_to_spdrLogLevel(ism_log_level_spdr);
        spdr::SpiderCastFactory::getInstance().changeLogLevel(
                spdr_log_level, spdr::trace::ScTrConstants::ScTr_Component_Name);
        spdr::SpiderCastFactory::getInstance().changeRUMLogLevel(ism_log_level_spdr);
        TRACE(5,"%s Configured SpiderCast logger (separate log file); Trace level for SpiderCast component: ism-trace-level=%d => spdr-trace-level=%d\n",
                __FUNCTION__, ism_log_level_spdr, static_cast<spdr::log::Level>(spdr_log_level));
    }
    else
    {
        ll_SPtr.reset(new mcp::ismLogListener());
        int ism_log_level_cluster = ism_cluster_getTraceLevel(); //Cluster
        spdr::log::Level spdr_log_level = mcp::MCPRouting::ismLogLevel_to_spdrLogLevel(ism_log_level_cluster);
        spdr::SpiderCastFactory::getInstance().registerLogListener(
                ll_SPtr.get(), spdr_log_level);
        TRACE(5,"%s Registered SpiderCast logger (adapter to ISM trace); Trace level for Cluster component: ism-trace-level=%d => spdr-trace-level=%d\n",
                __FUNCTION__, ism_log_level_cluster, static_cast<spdr::log::Level>(spdr_log_level));

        int ism_log_level_spdr = ism_cluster_getTraceLevelSpiderCast();
        spdr_log_level = mcp::MCPRouting::ismLogLevel_to_spdrLogLevel(ism_log_level_spdr);
        spdr::SpiderCastFactory::getInstance().changeLogLevel(
                spdr_log_level, spdr::trace::ScTrConstants::ScTr_Component_Name);
        spdr::SpiderCastFactory::getInstance().changeRUMLogLevel(ism_log_level_spdr);
        TRACE(5,"%s Configured SpiderCast logger (adapter to ISM trace); Trace level for SpiderCast component: ism-trace-level=%d => spdr-trace-level=%d\n",
                __FUNCTION__, ism_log_level_spdr, static_cast<spdr::log::Level>(spdr_log_level));

    }

    try
    {
        localForwadingInfo.get<0>() = "";
        localForwadingInfo.get<1>() = 0;
        localForwadingInfo.get<2>() = 0;

        //=== Tuning configuration file ===
        //An optional configuration file for tuning parameters
        //Interpret the configuration object for SpiderCast & MCP
        spdr::PropertyMap tuningProps;
        propVal = ism_common_getStringConfig(ismCLUSTER_CFG_CONFIGFILE);
        TRACE(5, "%s Value of config var %s is %s\n",
                __FUNCTION__,ismCLUSTER_CFG_CONFIGFILE,SoN(propVal));
        std::string configFile;
        if (propVal != NULL)
        {
            configFile = std::string(propVal);
        }
        //Load other properties
        if (!configFile.empty())
        {
            ifstream is;
            is.exceptions(ifstream::badbit | ifstream::failbit);

            try
            {
                is.open(configFile.c_str(), std::ios::in);
                tuningProps.load(is);
                is.close();
                TRACE(5, "%s Loaded tuning parameters (from %s): %s\n",
                        __FUNCTION__,configFile.c_str(),tuningProps.toString().c_str());
            } catch (ifstream::failure & ex)
            {
                TRACE(1,"Error: %s failed to read configuration file: %s, what=%s, rc=%d\n",
                        __FUNCTION__,configFile.c_str(),ex.what(),ISMRC_ClusterIOError);
                return ISMRC_ClusterIOError; //
            }
        }
        spdr::PropertyMap spdrProps(tuningProps);
        spdr::PropertyMap mcpProps(tuningProps);

        spdrProps.setProperty(spdr::config::NodeName_PROP_KEY, serverUID);
        spdrProps.setProperty(spdr::config::BusName_PROP_KEY,string("/") + clusterName);
        if (controlTLSPolicy == 1)
        {
            spdrProps.setProperty(spdr::config::UseSSL_PROP_KEY, "true");
            int reqCerts = ism_common_getIntConfig(ismCluster_CFG_CONTROL_REQ_CERTS_POLICY, 1);
            spdrProps.setProperty(spdr::config::RequireCerts_PROP_KEY,reqCerts ? "true" : "false");
        }
        else if (controlTLSPolicy == 2)
        {
            spdrProps.setProperty(spdr::config::UseSSL_PROP_KEY, "false");
        }

        mcpProps.setProperty(mcp::config::LocalServerUID_PROP_KEY, serverUID);
        mcpProps.setProperty(mcp::config::LocalServerName_PROP_KEY, nodeName);
        mcpProps.setProperty(mcp::config::ClusterName_PROP_KEY, clusterName);

        //=== get the control end-points ===
        //(Address, port) x {Internal, External}
        std::string controlAddr;
        propVal = ism_common_getStringConfig(ismCLUSTER_CFG_CONTROLADDR);
        TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCLUSTER_CFG_CONTROLADDR,SoN(propVal));
        if (propVal == NULL || !(*propVal)) //NULL or empty string
        {
            if (isStandAloneFlag > 0)
            {
                char host_ctrl[NI_MAXHOST];
                const char* nic = ism_common_getStringConfig(
                        ismCLUSTER_CFG_TESTNIC);
                if (nic == NULL || !(*nic))
                {
                    nic = "eth0";
                }
                int rc = ism_cluster_get_resolve_interface(nic, host_ctrl);
                if (rc == ISMRC_OK)
                {
                    controlAddr = host_ctrl;
                }
                else
                {
                    TRACE(1,"Error: %s failed, Missing property: %s. In stand-alone mode, but cannot resolve address from NIC=%s, rc=%d\n",
                            __FUNCTION__, ismCLUSTER_CFG_CONTROLADDR, nic, rc);
                    return rc;
                }
                TRACE(5,"%s Resolved %s using NIC=%s, to %s\n",
                        __FUNCTION__,ismCLUSTER_CFG_CONTROLADDR,nic,controlAddr.c_str());
            }
            else
            {
                TRACE(1,"Error: %s failed, Missing property: %s, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_CONTROLADDR, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_CONTROLADDR,"NULL");
                return ISMRC_BadPropertyValue;
            }
        }
        else
        {
            controlAddr = propVal;
        }

        std::string controlAddrExtr;
        propVal = ism_common_getStringConfig(ismCLUSTER_CFG_CONTROLADDREXT);
        TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__, ismCLUSTER_CFG_CONTROLADDREXT,SoN(propVal));

        if (propVal == NULL || !(*propVal)) //NULL or empty string
        {
            controlAddrExtr = controlAddr;
        }
        else
        {
            controlAddrExtr =  propVal;
        }

        spdrProps.setProperty(spdr::config::NetworkInterface_PROP_KEY,
                controlAddrExtr + string(",")); //Global scope
        spdrProps.setProperty(spdr::config::BindNetworkInterface_PROP_KEY,
                controlAddr);
        spdrProps.setProperty(spdr::config::BindAllInterfaces_PROP_KEY,"false");

        int control_port = ism_common_getIntConfig(ismCLUSTER_CFG_CONTROLPORT,9102);
        TRACE(5,"%s Value of config var %s is %u\n",__FUNCTION__,ismCLUSTER_CFG_CONTROLPORT,control_port);
        if (control_port <=0 || control_port >= (64*1024))
        {
            TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                    __FUNCTION__, ismCLUSTER_CFG_CONTROLPORT, control_port, ISMRC_BadPropertyValue);
            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_CONTROLPORT,control_port);
            return ISMRC_BadPropertyValue;
        }

        int control_port_ext = ism_common_getIntConfig(ismCLUSTER_CFG_CONTROLPORTEXT,0);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_CONTROLPORTEXT,control_port_ext);
        if (control_port_ext == 0)
        {
            control_port_ext = control_port;
        }
        else if (control_port_ext <=0 || control_port_ext >= (64*1024))
        {
            TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                    __FUNCTION__, ismCLUSTER_CFG_CONTROLPORTEXT, control_port_ext, ISMRC_BadPropertyValue);
            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_CONTROLPORTEXT,control_port_ext);
            return ISMRC_BadPropertyValue;
        }

        spdrProps.setProperty(spdr::config::TCPReceiverPort_PROP_KEY,
                boost::lexical_cast<string>(control_port_ext));
        spdrProps.setProperty(spdr::config::BindTCPReceiverPort_PROP_KEY,
                boost::lexical_cast<string>(control_port));

        //=== get discovery properties ===
        int discovery_to = ism_common_getIntConfig(ismCLUSTER_CFG_DISCOVERYTIME, 10);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_DISCOVERYTIME,discovery_to);
        if (discovery_to >=0)
        {
            mcpProps.setProperty(mcp::config::DiscoveryTimeoutMillis_PROP_KEY,
                    boost::lexical_cast<string>(discovery_to*1000));
        }
        else
        {
            TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                    __FUNCTION__, ismCLUSTER_CFG_DISCOVERYTIME, discovery_to, ISMRC_BadPropertyValue);
            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_DISCOVERYTIME,discovery_to);
            return ISMRC_BadPropertyValue;
        }

        //=== Multicast discovery ===
        int fMulticastDisc = ism_common_getIntConfig(ismCLUSTER_CFG_MULTICASTDISCOVERY, 1);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_MULTICASTDISCOVERY,fMulticastDisc);
        if (fMulticastDisc == 1) //Multicast discovery
        {
            spdrProps.setProperty(spdr::config::DiscoveryProtocol_PROP_KEY,
                    spdr::config::DiscoveryProtocol_Multicast_TCP_VALUE);

            int discovery_port = ism_common_getIntConfig(ismCLUSTER_CFG_DISCOVERYPORT, 9091);
            TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_DISCOVERYPORT,discovery_port);
            if (discovery_port > 0 && discovery_port < (64*1024))
            {
                spdrProps.setProperty(
                        spdr::config::DiscoveryMulticastPort_PROP_KEY,
                        boost::lexical_cast<string>(discovery_port));
            }
            else
            {
                TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_DISCOVERYPORT, discovery_port, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_DISCOVERYPORT,discovery_port);
                return ISMRC_BadPropertyValue;
            }

            //currently assumes internal==external, Spidercast does not implement external/internal
            int discvery_port_ext = ism_common_getIntConfig(ismCLUSTER_CFG_DISCOVERYPORTEXT, 0);
            TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_DISCOVERYPORTEXT,discvery_port_ext);
            if (discvery_port_ext == 0)
            {
                discvery_port_ext = discovery_port;
            }
            else if (discvery_port_ext <=0 || discvery_port_ext >= (64*1024))
            {
                TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_DISCOVERYPORTEXT, discvery_port_ext, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_DISCOVERYPORTEXT,discvery_port_ext);
                return ISMRC_BadPropertyValue;
            }

            int mcast_ttl = ism_common_getIntConfig(ismCLUSTER_CFG_MCASTTTL, 1);
            TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_MCASTTTL,mcast_ttl);
            if (mcast_ttl<0 || mcast_ttl>255)
            {
                TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_MCASTTTL, mcast_ttl, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_MCASTTTL,mcast_ttl);
                return ISMRC_BadPropertyValue;
            }
            spdrProps.setProperty(spdr::config::DiscoveryMulticastHops_PROP_KEY,
                    boost::lexical_cast<string>(mcast_ttl));

            spdrProps.setProperty(spdr::config::DiscoveryMulticastInOutInterface_PROP_KEY,controlAddr);

            const char* mc_addr_v4 = ism_common_getStringConfig(ismCluster_CFG_IPV4_MCAST_ADDR);
            TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCluster_CFG_IPV4_MCAST_ADDR,SoN(mc_addr_v4));
            if (mc_addr_v4 == NULL || !(*mc_addr_v4))
            {
                spdrProps.setProperty(spdr::config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY, "239.192.97.105");
            }
            else
            {
                spdrProps.setProperty(spdr::config::DiscoveryMulticastGroupAddressIPv4_PROP_KEY, mc_addr_v4);
            }

            const char* mc_addr_v6 = ism_common_getStringConfig(ismCluster_CFG_IPV6_MCAST_ADDR);
            TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCluster_CFG_IPV6_MCAST_ADDR,SoN(mc_addr_v6));
            if (mc_addr_v6 == NULL || !(*mc_addr_v6))
            {
                spdrProps.setProperty(spdr::config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY, "ff18::6169");
            }
            else
            {
                spdrProps.setProperty(spdr::config::DiscoveryMulticastGroupAddressIPv6_PROP_KEY, mc_addr_v6);
            }
        }
        else
        {
            spdrProps.setProperty(spdr::config::DiscoveryProtocol_PROP_KEY,
                    spdr::config::DiscoveryProtocol_TCP_VALUE);
        }

        //=== parse bootstrap ===
        std::vector<spdr::NodeID_SPtr> spdrBSS;
        const char* server_ep_list = ism_common_getStringConfig(ismCLUSTER_CFG_BOOTSTRAPSET);
        TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCLUSTER_CFG_BOOTSTRAPSET,SoN(server_ep_list));
        if (server_ep_list != NULL)
        {
            std::string spdr_bss_nameless;
            if (ISMRC_OK != ism_cluster_convert_to_nameless_bss(server_ep_list, spdr_bss_nameless))
            {
                TRACE(1,"Error: %s failed, bad property value: %s = %s, out of range, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_BOOTSTRAPSET, server_ep_list, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_BOOTSTRAPSET,server_ep_list);
                return ISMRC_BadPropertyValue;
            }
            TRACE(5,"%s Converted %s to nameless URI bootstrap: %s\n",__FUNCTION__,ismCLUSTER_CFG_BOOTSTRAPSET,spdr_bss_nameless.c_str());
            spdr::SpiderCastFactory& factory =	spdr::SpiderCastFactory::getInstance();
            spdrBSS = factory.loadBootstrapSetURIs(spdr_bss_nameless.c_str());
        }

        //=== get Control HBTO (seconds) ===
        int control_hbto  = ism_common_getIntConfig(ismCLUSTER_CFG_CONTROLHBTO, 20);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_CONTROLHBTO,control_hbto);
        if (control_hbto >=0)
        {
            if (control_hbto == 0)
            {
                spdrProps.setProperty(spdr::config::HeartbeatTimeoutMillis_PROP_KEY,
                        boost::lexical_cast<string>(20000));
                spdrProps.setProperty(spdr::config::HeartbeatIntervalMillis_PROP_KEY,
                        boost::lexical_cast<string>(2000));
            }
            else
            {
                spdrProps.setProperty(spdr::config::HeartbeatTimeoutMillis_PROP_KEY,
                        boost::lexical_cast<string>(control_hbto*1000));
                spdrProps.setProperty(spdr::config::HeartbeatIntervalMillis_PROP_KEY,
                        boost::lexical_cast<string>(control_hbto*100));
            }
        }
        else
        {
            TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range (>=0), rc=%d\n",
                    __FUNCTION__, ismCLUSTER_CFG_CONTROLHBTO, control_hbto, ISMRC_BadPropertyValue);
            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_CONTROLHBTO,control_hbto);
            return ISMRC_BadPropertyValue;
        }

        //=== get Bloom Filter config parameters
        int bf_max_attr = ism_common_getIntConfig(ismCLUSTER_CFG_BF_MAX_ATTRIBUTES,
        		mcp::config::BloomFilterMaxAttributes_DEFVALUE);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_BF_MAX_ATTRIBUTES,bf_max_attr);
        if (bf_max_attr >= 0)
        {
        	//Zero = default
        	if (bf_max_attr > 0)
        	{
        		mcpProps.setProperty(mcp::config::BloomFilterMaxAttributes_PROP_KEY,
        				boost::lexical_cast<string>(bf_max_attr));
        	}
        }
        else
        {
        	TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range (>=0), rc=%d\n",
        			__FUNCTION__, ismCLUSTER_CFG_BF_MAX_ATTRIBUTES, bf_max_attr, ISMRC_BadPropertyValue);
        	ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d", ismCLUSTER_CFG_BF_MAX_ATTRIBUTES, bf_max_attr);
        	return ISMRC_BadPropertyValue;
        }

        int bf_publish_interval_ms = ism_common_getIntConfig(ismCLUSTER_CFG_BF_PUBLISH_INTERVAL_MILLIS,
        		mcp::config::BloomFilterPublishTaskIntervalMillis_DEFVALUE);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_BF_PUBLISH_INTERVAL_MILLIS,bf_publish_interval_ms);
        if (bf_publish_interval_ms >= 0)
        {
        	//Zero = default
        	if (bf_publish_interval_ms > 0)
        	{
        		mcpProps.setProperty(mcp::config::BloomFilterPublishTaskIntervalMillis_PROP_KEY,
        				boost::lexical_cast<string>(bf_publish_interval_ms));
        	}
        }
        else
        {
        	TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range (>=0), rc=%d\n",
        			__FUNCTION__, ismCLUSTER_CFG_BF_PUBLISH_INTERVAL_MILLIS, bf_publish_interval_ms, ISMRC_BadPropertyValue);
        	ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d", ismCLUSTER_CFG_BF_PUBLISH_INTERVAL_MILLIS, bf_publish_interval_ms);
        	return ISMRC_BadPropertyValue;
        }

        //create a config object to make sure all is well with the bootstrap and the parameters
        {
            spdr::SpiderCastConfig_SPtr spdrConfig =
                    spdr::SpiderCastFactory::getInstance().createSpiderCastConfig(
                            spdrProps, spdrBSS);
            int errCode = 0;
            std::string errMsg;
            if (!spdrConfig->verifyBindNetworkInterface(errMsg, errCode))
            {
                TRACE(1,"Error: %s failed to validate control network interface: %s = %s; error message=%s, error code=%d; rc=%d\n",
                                    __FUNCTION__, spdr::config::BindNetworkInterface_PROP_KEY.c_str(), controlAddr.c_str(), errMsg.c_str(), errCode, ISMRC_ClusterConfigError);
                ism_common_setErrorData(ISMRC_ClusterConfigError,"%s",errMsg.c_str());
                return ISMRC_ClusterConfigError;
            }
            
        }

        //save the properties & bootstrap
        spdrProps_SPtr.reset(new spdr::PropertyMap(spdrProps));
        mcpProps_SPtr.reset(new spdr::PropertyMap(mcpProps));
        spdrBootstrapSet_SPtr.reset(
                new std::vector<spdr::NodeID_SPtr>(spdrBSS));

        TRACE(2,"%s ISM Cluster initialized with the following configuration parameters: mcc properties=%s, spdr properties=%s\n",
                __FUNCTION__,mcpProps_SPtr->toString().c_str(),spdrProps_SPtr->toString().c_str());

        TRACE(2,"%s ISM Cluster is going to join with members: {%s}\n",
                __FUNCTION__,SoN(server_ep_list));

        TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, ISMRC_OK);
        return ISMRC_OK;

    }
    catch (spdr::SpiderCastLogicError& le)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n stacktrace: %s\n",
                __FUNCTION__, le.what(),ISMRC_ClusterConfigError, le.getStackTrace().c_str()); //FIXME
        ism_common_setErrorData(ISMRC_ClusterConfigError,"%s",le.what());
        return ISMRC_ClusterConfigError;
    }
    catch (spdr::SpiderCastRuntimeError& re)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n stacktrace: %s\n",
                __FUNCTION__, re.what(), ISMRC_Error, re.getStackTrace().c_str()); //FIXME
        return ISMRC_Error;
    }
    catch (bad_alloc& ba)
    {
        TRACE(1,"Error: %s failed to allocate memory, what=%s, rc=%d\n",
                __FUNCTION__, ba.what(), ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
    catch (exception& e)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n",
                __FUNCTION__, e.what(), ISMRC_Error);
        return ISMRC_Error;
    }
    catch(...)
    {
        TRACE(1,"Error: %s failed, untyped exception (...), rc=%d\n",
                __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }
}

/* Must be called after init
 * Can be called either before or after start
 * Must be called before recoveryCompleted
 */
int ism_cluster_registerProtocolEventCallback(
        ismProtocol_RemoteServerCallback_t callback, void *pContext)
{
    using namespace std;

    TRACE(9, "Entry: %s\n", __FUNCTION__);

    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!(mcpProps_SPtr && spdrProps_SPtr && spdrBootstrapSet_SPtr))
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    if (callback != NULL && forwardingControlCAdapter_SPtr)
    {
        TRACE(1, "Error: %s, callback already registered and argument 'callback' not NULL, rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    if (callback == NULL && !forwardingControlCAdapter_SPtr)
    {
        TRACE(1, "Warning: %s, callback not registered and argument 'callback' is NULL\n", __FUNCTION__);
        return ISMRC_OK;
    }

    int rc = ISMRC_OK;
    try
    {
        if (callback != NULL)
        {
            if (!forwardingControlCAdapter_SPtr)
            {
                forwardingControlCAdapter_SPtr.reset(
                        new mcp::ForwardingControlCAdapter(callback, pContext));

                if (mcpInstance_SPtr) //called after start
                {
                    rc = mcpInstance_SPtr->registerProtocolEventCallback(forwardingControlCAdapter_SPtr.get());
                    if (rc != ISMRC_OK)
                    {
                        TRACE(1,"Error: %s failed to register callback into mcpInstance, rc=%d\n",
                                __FUNCTION__,  rc);
                    }
                    else
                    {
                        TRACE(5,"%s OK, after cluster start\n",__FUNCTION__);
                    }
                }
                else
                {
                    TRACE(5,"%s OK, before cluster start\n",__FUNCTION__);
                }
            }
            else
            {
                rc = ISMRC_Error;
                TRACE(1, "Error: %s failed to register callback into mcpInstance, already registered, rc=%d\n",
                        __FUNCTION__,  rc);
            }
        }
        else
        {
            if (forwardingControlCAdapter_SPtr)
            {
                forwardingControlCAdapter_SPtr->close();
                TRACE(5,"%s OK, after callback un-registered\n",__FUNCTION__);
            }
            else
            {
                TRACE(5, "%s Ignoring un-register callback into mcpInstance, already not registered rc=%d\n",
                        __FUNCTION__,  rc);
            }
        }
    }
    catch (bad_alloc& ba)
    {
        TRACE(1,"Error: %s failed to allocate memory, what=%s, rc=%d\n",
                __FUNCTION__, ba.what(), ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
    catch (exception& e)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n",
                __FUNCTION__, e.what(), ISMRC_Error);
        return ISMRC_Error;
    }
    catch(...)
    {
        TRACE(1,"Error: %s failed, untyped exception (...), rc=%d\n",
                __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}


/* Must be called after init
 * Must be called before start
 * Must be called before recoveryCompleted
 */
int ism_cluster_setLocalForwardingInfo(
        const char *pServerName, const char *pServerUID, //Do they know this before our start? probably not, hence ignored
        const char *pServerAddress, int serverPort, uint8_t fUseTLS)
{
    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!(mcpProps_SPtr && spdrProps_SPtr && spdrBootstrapSet_SPtr))
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    std::string name(pServerName == NULL ? "" : pServerName);
    std::string uid(pServerUID == NULL ? "" : pServerUID);
    TRACE(5,"%s Forwarding provided: name=%s, uid=%s; Ignored.\n",__FUNCTION__, name.c_str(), uid.c_str());

    if (pServerAddress == NULL)
    {
        TRACE(1, "Error: %s, argument 'pServerAddress' is NULL, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    localForwadingInfo.get<0>() = std::string(pServerAddress);
    localForwadingInfo.get<1>() = serverPort;
    localForwadingInfo.get<2>() = fUseTLS;

    if (localForwadingInfo.get<0>().empty())
    {
        TRACE(1, "Error: %s, argument 'pServerAddress' is empty string, rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    if ((localForwadingInfo.get<1>() <= 0) || (localForwadingInfo.get<1>() >= 64*1024))
    {
        TRACE(1, "Error: %s, argument 'serverPort' (%d) is out of range, rc=%d\n", __FUNCTION__, serverPort, ISMRC_Error);
        return ISMRC_Error;
    }

    int rc = ISMRC_OK;

    if (mcpInstance_SPtr)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s failed, must be called before start(). ServerName %s, ServerUID %s, ServerAddress %s, serverPort %d, fUseTLS %d, rc=%d\n",
                           __FUNCTION__, SoN(pServerName), SoN(pServerUID), SoN(pServerAddress), serverPort, fUseTLS, rc);
        return rc;

//        int rc = mcpInstance->setLocalForwardingInfo(pServerName, pServerUID, pServerAddress, serverPort, fUseTLS);
//        if (rc != ISMRC_OK)
//        {
//            TRACE(1, "Error: %s failed, ServerName %s, ServerUID %s, ServerAddress %s, serverPort %d, fUseTLS %d, rc=%d\n",
//                    __FUNCTION__, SoN(pServerName), SoN(pServerUID), SoN(pServerAddress), serverPort, fUseTLS, rc);
//        }
//        else
//        {
//            TRACE(5, "%s OK, ServerName %s, ServerUID %s, ServerAddress %s, serverPort %d, fUseTLS %d; after cluster start\n",
//                    __FUNCTION__, SoN(pServerName), SoN(pServerUID), SoN(pServerAddress), serverPort, fUseTLS);
//        }
    }
    else
    {

        TRACE(5, "%s OK, ServerName %s, ServerUID %s, ServerAddress %s, serverPort %d, fUseTLS %d; before cluster start\n",
            __FUNCTION__, SoN(pServerName), SoN(pServerUID), SoN(pServerAddress), serverPort, fUseTLS);
    }


    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return ISMRC_OK;
}

/* For HA:
 * refresh config variables that are replicated between primary and backup:
 * Enable cluster,
 * Cluster name
 * ServerUID,
 * Discovery properties:
 *   use-mcast,
 *   mcast-port,
 *   Bootstrap
 */
int ism_cluster_refreshReplicatedConfig()
{
    using namespace std;

    TRACE(9, "Entry: %s\n", __FUNCTION__);

    int rc = ISMRC_OK;

    rc = ism_cluster_initClusterConfig(); //this needs to loads the replicated config
    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed, cannot init config, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }

    enableClusterFlag = ism_common_getBooleanConfig(ismCLUSTER_CFG_ENABLECLUSTER, 0);
    TRACE(5,"%s Value of config var %s is %u\n", __FUNCTION__, ismCLUSTER_CFG_ENABLECLUSTER, enableClusterFlag);
    if (enableClusterFlag == 0)
    {
        TRACE(2, "Warning: %s, in init() cluster was enabled, but now it is cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    //=== Get properties: cluster-name, server-name, server-UID ===
    //=== ClusterName ===
    const char* propVal;
    propVal = ism_common_getStringConfig(ismCLUSTER_CFG_CLUSTERNAME);
    TRACE(5,"%s Value of config var %s is %s\n", __FUNCTION__,ismCLUSTER_CFG_CLUSTERNAME,SoN(propVal));
    if (propVal == NULL || !(*propVal))
    {
        TRACE(1,"Error: %s failed, missing property %s rc=%d\n",
                __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_CLUSTERNAME,"NULL");
        return ISMRC_BadPropertyValue;
    }
    std::string clusterName(propVal);
    if (clusterName.find_first_of('/') != std::string::npos)
    {
        TRACE(1,"Error: %s failed, bad property value, %s (should not contain a '/'): %s, rc=%d\n",
                __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, clusterName.c_str(), ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_CLUSTERNAME,clusterName.c_str());
        return ISMRC_BadPropertyValue;
    }

    std::pair<std::string,bool> res_prop = mcpProps_SPtr->getProperty(mcp::config::ClusterName_PROP_KEY);

    if (!res_prop.second)
    {
        TRACE(1,"Error: %s failed, cannot retrieve %s from MCP properties, rc=%d\n",
                __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, ISMRC_Error);
        return ISMRC_Error;
    }
    else if (clusterName != res_prop.first)
    {
        TRACE(5,"%s Value of %s changed between init (=%s) and start (=%s)\n",
                __FUNCTION__, ismCLUSTER_CFG_CLUSTERNAME, res_prop.first.c_str(), clusterName.c_str());
        spdrProps_SPtr->setProperty(spdr::config::BusName_PROP_KEY, std::string("/") + clusterName);
        mcpProps_SPtr->setProperty(mcp::config::ClusterName_PROP_KEY, clusterName);
    }

    //=== ServerName ===
    propVal = ism_common_getServerName();
    std::string nodeName(propVal != NULL ? propVal : "");
    TRACE(5,"%s Value of ism_common_getServerName is %s\n",__FUNCTION__,nodeName.c_str());
    res_prop = mcpProps_SPtr->getProperty(mcp::config::LocalServerName_PROP_KEY);
    if (!res_prop.second)
    {
        TRACE(1,"Error: %s failed, cannot retrieve ServerName from MCP properties, rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }
    else if (nodeName != res_prop.first)
    {
        TRACE(5,"%s Value of ServerName changed between init (=%s) and start (=%s)\n",
                __FUNCTION__, res_prop.first.c_str(), nodeName.c_str());
        mcpProps_SPtr->setProperty(mcp::config::LocalServerName_PROP_KEY, nodeName);
    }

    //=== ServerUID ===
    propVal=ism_common_getServerUID();
    if (propVal == NULL)
    {
        TRACE(1,"Error: %s Value of ism_common_getServerUID is NULL, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    std::string serverUID(propVal);
    TRACE(5,"%s Value of ism_common_getServerUID is %s\n",__FUNCTION__, serverUID.c_str());

    if (!ism_common_validServerUID(serverUID.c_str()))
    {
        TRACE(1,"Error: %s failed, invalid mandatory 'ServerUID': %s, rc=%d\n",__FUNCTION__,serverUID.c_str(),ISMRC_Error);
        return ISMRC_Error;
    }

    res_prop = mcpProps_SPtr->getProperty(mcp::config::LocalServerUID_PROP_KEY);
    if (!res_prop.second)
    {
        TRACE(1,"Error: %s failed, cannot retrieve ServerUID from MCP properties, rrc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }
    else if (serverUID != res_prop.first)
    {
        TRACE(5,"%s Value of ServerUID changed between init (=%s) and start (=%s)\n",
                __FUNCTION__, res_prop.first.c_str(), serverUID.c_str());
        mcpProps_SPtr->setProperty(mcp::config::LocalServerUID_PROP_KEY, serverUID);
        spdrProps_SPtr->setProperty(spdr::config::NodeName_PROP_KEY, serverUID);
    }

    //=== Control TLS Policy ===
    controlTLSPolicy = ism_common_getIntConfig(ismCluster_CFG_CONTROL_TLS_POLICY, 1); //default: Always-On
    TRACE(5,"%s Value of config var %s is %d\n", __FUNCTION__,ismCluster_CFG_CONTROL_TLS_POLICY,controlTLSPolicy);
    if (controlTLSPolicy < 1 || controlTLSPolicy > 3)
    {
        TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                __FUNCTION__, ismCluster_CFG_CONTROL_TLS_POLICY, controlTLSPolicy, ISMRC_BadPropertyValue);
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCluster_CFG_CONTROL_TLS_POLICY,controlTLSPolicy);
        return ISMRC_BadPropertyValue;
    }

    if (controlTLSPolicy == 1)
    {
        spdrProps_SPtr->setProperty(spdr::config::UseSSL_PROP_KEY, "true");
    }
    else if (controlTLSPolicy == 2)
    {
        spdrProps_SPtr->setProperty(spdr::config::UseSSL_PROP_KEY, "false");
    }

    //=== Multicast discovery ===
    int fMulticastDisc = ism_common_getIntConfig(ismCLUSTER_CFG_MULTICASTDISCOVERY, 1);
    TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_MULTICASTDISCOVERY,fMulticastDisc);
    if (fMulticastDisc == 1) //Multicast discovery
    {
        spdrProps_SPtr->setProperty(spdr::config::DiscoveryProtocol_PROP_KEY,
                spdr::config::DiscoveryProtocol_Multicast_TCP_VALUE);

        int discovery_port = ism_common_getIntConfig(ismCLUSTER_CFG_DISCOVERYPORT, 9091);
        TRACE(5,"%s Value of config var %s is %d\n",__FUNCTION__,ismCLUSTER_CFG_DISCOVERYPORT,discovery_port);
        if (discovery_port > 0 && discovery_port < (64*1024))
        {
            spdrProps_SPtr->setProperty(
                    spdr::config::DiscoveryMulticastPort_PROP_KEY,
                    boost::lexical_cast<std::string>(discovery_port));
        }
        else
        {
            TRACE(1,"Error: %s failed, bad property value: %s = %d, out of range, rc=%d\n",
                    __FUNCTION__, ismCLUSTER_CFG_DISCOVERYPORT, discovery_port, ISMRC_BadPropertyValue);
            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%d",ismCLUSTER_CFG_DISCOVERYPORT,discovery_port);
            return ISMRC_BadPropertyValue;
        }
    }
    else
    {
        spdrProps_SPtr->setProperty(spdr::config::DiscoveryProtocol_PROP_KEY,
                spdr::config::DiscoveryProtocol_TCP_VALUE);
    }

    try
    {
        //=== parse bootstrap ===
        std::vector<spdr::NodeID_SPtr> spdrBSS;
        const char* server_ep_list = ism_common_getStringConfig(ismCLUSTER_CFG_BOOTSTRAPSET);
        TRACE(5,"%s Value of config var %s is %s\n",__FUNCTION__,ismCLUSTER_CFG_BOOTSTRAPSET,SoN(server_ep_list));
        if (server_ep_list != NULL)
        {
            std::string spdr_bss_nameless;
            if (ISMRC_OK != ism_cluster_convert_to_nameless_bss(server_ep_list, spdr_bss_nameless))
            {
                TRACE(1,"Error: %s failed, bad property value: %s = %s, out of range, rc=%d\n",
                        __FUNCTION__, ismCLUSTER_CFG_BOOTSTRAPSET, server_ep_list, ISMRC_BadPropertyValue);
                ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_BOOTSTRAPSET,server_ep_list);
                return ISMRC_BadPropertyValue;
            }
            TRACE(5,"%s Converted %s to nameless URI bootstrap: %s\n",__FUNCTION__,ismCLUSTER_CFG_BOOTSTRAPSET,spdr_bss_nameless.c_str());
            spdr::SpiderCastFactory& factory =  spdr::SpiderCastFactory::getInstance();
            spdrBSS = factory.loadBootstrapSetURIs(spdr_bss_nameless.c_str());
        }

//        //validate that all the node names in the BSS are valid ServerUIDs
//        bool bss_error = false;
//        for (unsigned int i=0; i<spdrBSS.size(); ++i)
//        {
//            if (!ism_common_validServerUID(spdrBSS[i]->getNodeName().c_str()))
//            {
//                TRACE(1,"Error: %s failed, property %s contains invalid ServerUID: %s\n",
//                        __FUNCTION__, ismCLUSTER_CFG_BOOTSTRAPSET, spdrBSS[i]->getNodeName().c_str());
//                bss_error = true;
//            }
//        }
//
//        if (bss_error)
//        {
//            ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s",ismCLUSTER_CFG_BOOTSTRAPSET,SoN(server_ep_list));
//            return ISMRC_BadPropertyValue;
//        }

        if (spdrBSS != *spdrBootstrapSet_SPtr)
        {
            TRACE(5,"%s SpiderCast bootstrap changed between init and start\n",__FUNCTION__);
        }

        //create a config object to make sure all is well
        spdr::SpiderCastConfig_SPtr spdrConfig =
                spdr::SpiderCastFactory::getInstance().createSpiderCastConfig(
                        *spdrProps_SPtr, *spdrBootstrapSet_SPtr);

        spdrBootstrapSet_SPtr->swap(spdrBSS);
    }
    catch (spdr::SpiderCastLogicError& le)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n stacktrace: %s\n",
                __FUNCTION__, le.what(),ISMRC_BadPropertyValue, le.getStackTrace().c_str()); //FIXME c_str
        ism_common_setErrorData(ISMRC_BadPropertyValue,"%s%s","-", le.what());
        return ISMRC_BadPropertyValue;
    }
    catch (spdr::SpiderCastRuntimeError& re)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n stacktrace: %s\n",
                __FUNCTION__, re.what(), ISMRC_Error, re.getStackTrace().c_str()); //FIXME c_str
        return ISMRC_Error;
    }
    catch (bad_alloc& ba)
    {
        TRACE(1,"Error: %s failed to allocate memory, what=%s, rc=%d\n",
                __FUNCTION__, ba.what(), ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
    catch (exception& e)
    {
        TRACE(1,"Error: %s failed, what=%s, rc=%d\n",
                __FUNCTION__, e.what(), ISMRC_Error);
        return ISMRC_Error;
    }
    catch(...)
    {
        TRACE(1,"Error: %s failed, untyped exception (...), rc=%d\n",
                __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    TRACE(9, "Exit: %s\n", __FUNCTION__);
    return rc;
}

extern int ism_cluster_start()
{
    TRACE(9, "Entry: %s\n", __FUNCTION__);

    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (mcpInstance_SPtr)
    {
        TRACE(1,"Error: %s failed, mcpInstance is not NULL, rc=%d\n",__FUNCTION__,ISMRC_Error);
        return ISMRC_Error;
    }

    //Resolve replicated config, in case server start from backup
    int rc = ism_cluster_refreshReplicatedConfig();
    if (rc != ISMRC_OK)
    {
        TRACE(1,"%s failed to refresh the replicated part of the configuration, rc=%d\n",__FUNCTION__,rc);
        return rc;
    }

    if (controlTLSPolicy == 3) //Follow Messaging
    {
        spdrProps_SPtr->setProperty(spdr::config::UseSSL_PROP_KEY, ((localForwadingInfo.get<2>() > 0) ? "true" : "false"));
    }

    //===
    mcp::MCPRouting* pMCPInstance = NULL;
    rc = mcp::MCPRouting::create(*mcpProps_SPtr, *spdrProps_SPtr,
            *spdrBootstrapSet_SPtr, &pMCPInstance);
    if (rc != ISMRC_OK)
    {
        TRACE(1,"%s failed to create MCPRouting, rc=%d\n",__FUNCTION__,rc);
        return rc;
    }
    else if (pMCPInstance == NULL)
    {
        TRACE(1,"Error: %s failed, mcpInstance is NULL, rc=%d\n",__FUNCTION__,ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    mcpInstance_SPtr.reset(pMCPInstance);
    //===

    if (forwardingControlCAdapter_SPtr)
    {
        rc = mcpInstance_SPtr->registerProtocolEventCallback(forwardingControlCAdapter_SPtr.get());
        if (rc != ISMRC_OK)
        {
            TRACE(1,"Error: %s failed to register the protocol-callback-adapter into MCPRouting, rc=%d\n",__FUNCTION__,rc);
            return rc;
        }
    }
    else
    {
        TRACE(5,"%s, protocol-callback not set yet\n",__FUNCTION__);
    }

    if (engineEventCallbackCAdapter_SPtr)
    {
        rc = mcpInstance_SPtr->registerEngineEventCallback(engineEventCallbackCAdapter_SPtr.get());
        if (rc != ISMRC_OK)
        {
            TRACE(1,"Error: %s failed to register the engine-callback-adapter into MCPRouting, rc=%d\n",__FUNCTION__,rc);
            return rc;
        }
    }
    else
    {
        TRACE(5,"%s, engine-callback not set yet\n",__FUNCTION__);
    }

    std::pair<std::string,bool> res_uid = mcpProps_SPtr->getProperty(mcp::config::LocalServerUID_PROP_KEY);
    std::pair<std::string,bool> res_name = mcpProps_SPtr->getProperty(mcp::config::LocalServerName_PROP_KEY);

    if (!localForwadingInfo.get<0>().empty() && localForwadingInfo.get<1>() > 0)
    {
        rc = mcpInstance_SPtr->setLocalForwardingInfo(res_name.first.c_str(), res_uid.first.c_str(),
                        localForwadingInfo.get<0>().c_str(), localForwadingInfo.get<1>(), localForwadingInfo.get<2>());
        if (rc != ISMRC_OK)
        {
            TRACE(1, "Error: %s failed to set local-forwarding-info into mcpInstance, rc=%d\n", __FUNCTION__, rc);
            return rc;
        }
    }
    else
    {
        TRACE(5, "%s, local-forwarding-info not set yet\n", __FUNCTION__);
    }

    if (haStatus_beforeStart != ISM_CLUSTER_HA_UNKNOWN)
    {
        TRACE(5,"%s HA Status set before start, applying status=%d\n",__FUNCTION__, haStatus_beforeStart);
        rc = mcpInstance_SPtr->setHaStatus(haStatus_beforeStart);
        if (rc != ISMRC_OK)
        {
            TRACE(1, "Error: %s failed to set HA Status into mcpInstance, rc=%d\n", __FUNCTION__, rc);
            return rc;
        }
    }

    rc = mcpInstance_SPtr->start();
    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed to start MCPRouting, rc=%d\n",__FUNCTION__,rc);
    }
    else
    {
        TRACE(5,"%s Cluster started successfully\n",__FUNCTION__);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

/* Must be called after init
 * Can be called either before or after start
 * Must be called before recoveryCompleted
 */
int ism_cluster_registerEngineEventCallback(
        ismEngine_RemoteServerCallback_t callback, void *pContext)
{
    using namespace std;

    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (callback == NULL && !engineEventCallbackCAdapter_SPtr)
    {
        TRACE(2,"Warning: %s, callback not registered and argument 'callback' is NULL\n",__FUNCTION__);
        return ISMRC_OK;
    }

    if (callback != NULL && engineEventCallbackCAdapter_SPtr)
    {
        TRACE(1,"Error: %s, callback already registered and argument 'callback' is not NULL, rc=%d\n",__FUNCTION__,ISMRC_Error);
        return ISMRC_Error;
    }

    try
    {
        if (callback != NULL && !engineEventCallbackCAdapter_SPtr)
        {
            engineEventCallbackCAdapter_SPtr.reset(
                    new mcp::EngineEventCallbackCAdapter(callback, pContext));
            if (mcpInstance_SPtr)
            {
                rc = mcpInstance_SPtr->registerEngineEventCallback(engineEventCallbackCAdapter_SPtr.get());
                if (rc != ISMRC_OK)
                {
                    TRACE(1,"Error: %s failed with rc=%d\n",__FUNCTION__,rc);
                }
                else
                {
                    TRACE(5,"%s OK, after cluster start\n",__FUNCTION__);
                }
            }
            else
            {
                TRACE(5,"%s OK, before cluster start\n",__FUNCTION__);
            }
        }
        else
        {
            engineEventCallbackCAdapter_SPtr->close();
            TRACE(5,"%s OK, un-registered callback\n",__FUNCTION__);
        }
    }
    catch (bad_alloc& ba)
    {
        TRACE(1,"Error: %s failed to allocate memory, what=%s, rc=%d\n",
                __FUNCTION__, ba.what(), ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
    catch (exception& e)
    {
        TRACE(1,"Error: %s failed, unexpected exception, what=%s, rc=%d\n",
                __FUNCTION__, e.what(), ISMRC_Error);
        return ISMRC_Error;
    }
    catch(...)
    {
        TRACE(1,"Error: %s failed, untyped exception (...), rc=%d\n",
                __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Arrives during recovery, as well as during normal run-time
 */
extern int ism_cluster_addSubscriptions(
        const ismCluster_SubscriptionInfo_t* pSubInfo, int numSubs)
{
    int rc;

    TRACE(9, "Entry: %s, numSubs=%d\n", __FUNCTION__,numSubs);

    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->addSubscriptions(pSubInfo, numSubs);

    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed with rc=%d, numSubs=%d\n",__FUNCTION__,rc,numSubs);
    }
    else
    {
        TRACE(7,"%s added numSubs=%d, successfully\n",__FUNCTION__,numSubs);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

int ism_cluster_restoreRemoteServers(
        const ismCluster_RemoteServerData_t *pServersData, int numServers)
{
    int rc;

    TRACE(9, "Entry: %s, numServers=%d\n", __FUNCTION__,numServers);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    if (numServers > 0 && pServersData == NULL)
    {
        TRACE(1, "Error: %s, numServers=%d, but pServerData=NULL, rc=%d\n", __FUNCTION__, numServers, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    rc = mcpInstance_SPtr->restoreRemoteServers(pServersData, numServers);

    if (rc != ISMRC_OK)
    {
        TRACE(1," %s failed with rc=%d, numServers=%d\n",__FUNCTION__,rc,numServers);
    }
    else
    {
        TRACE(7,"%s restored numServers=%d, successfully\n",__FUNCTION__,numServers);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_recoveryCompleted(void)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->recoveryCompleted();

    if (rc != ISMRC_OK)
    {
        TRACE(1," %s failed with rc=%d\n",__FUNCTION__,rc);
    }
    else
    {
        TRACE(5," %s Cluster recovery completed OK.\n",__FUNCTION__);
    }


    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_startMessaging(void)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->startMessaging();

    if (rc != ISMRC_OK)
    {
        TRACE(1," %s failed with rc=%d\n",__FUNCTION__,rc);
    }
    else
    {
        TRACE(5," %s Cluster start messaging completed OK.\n",__FUNCTION__);
    }


    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_term(void)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);

    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (mcpInstance_SPtr)
    {
        rc = mcpInstance_SPtr->stop();
        if (rc != ISMRC_OK)
        {
            TRACE(1,"Error: %s failed while trying to stop, rc=%d\n",__FUNCTION__,rc);
        }
//        delete mcpInstance;
//        mcpInstance = NULL;
    }
    else if (!(spdrProps_SPtr && mcpProps_SPtr && spdrBootstrapSet_SPtr))
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        rc = ISMRC_ClusterNotAvailable;
    }

//    spdrProps_SPtr.reset();
//    mcpProps_SPtr.reset();
//    engineEventCallbackCAdapter_SPtr.reset();
//    forwardingControlCAdapter_SPtr.reset();
//    spdrBootstrapSet_SPtr.reset();

    if (rc == ISMRC_OK)
    {
        TRACE(5,"%s Cluster terminated successfully.\n",__FUNCTION__);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_removeSubscriptions(
        const ismCluster_SubscriptionInfo_t* pSubInfo, int numSubs)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->removeSubscriptions(pSubInfo, numSubs);

    if (rc != ISMRC_OK)
    {
        TRACE(1, "Error: %s failed with rc=%d, numSubs=%d\n",
                __FUNCTION__, rc, numSubs);
    }
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_routeLookup(ismCluster_LookupInfo_t* pLookupInfo)
{
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    if (pLookupInfo == NULL)
    {
        TRACE(1, "Error: %s, argument 'pLookupInfo' is NULL, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    int rc = mcpInstance_SPtr->lookup(pLookupInfo);
    if (rc != ISMRC_OK && rc != ISMRC_ClusterArrayTooSmall)
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, rc);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_remoteServerConnected(
        const ismCluster_RemoteServerHandle_t phServerHandle)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    if (phServerHandle == NULL)
    {
        TRACE(1, "Error: %s, cluster handle NULL, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    rc = mcpInstance_SPtr->nodeForwardingConnected(phServerHandle);

    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed with rc=%d\n",__FUNCTION__,rc);
    }
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_remoteServerDisconnected(
        const ismCluster_RemoteServerHandle_t phServerHandle)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    if (phServerHandle == NULL)
    {
        TRACE(1, "Error: %s, cluster handle NULL, rc=%d\n", __FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    rc = mcpInstance_SPtr->nodeForwardingDisconnected(phServerHandle);

    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed with rc=%d\n",__FUNCTION__,rc);
    }
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_removeRemoteServer(const ismCluster_RemoteServerHandle_t phServerHandle)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->adminDeleteNode(phServerHandle);

    if (rc != ISMRC_OK)
    {
        TRACE(1,"Error: %s failed with rc=%d\n",__FUNCTION__,rc);
    }
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_removeLocalServer()
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->adminDetachFromCluster();

    if (rc == ISMRC_OK)
    {
        TRACE(5,"%s Local server was successfully removed from the cluster.\n",__FUNCTION__);
    }
    else if (rc == ISMRC_ClusterRemoveLocalServerNoAck)
    {
        TRACE(2,"Warning: %s removed local server from the cluster, but no acknowledgment was received. Use another server with removeRemoteServer, providing this node as parameter.\n",__FUNCTION__);
    }
    else
    {
        TRACE(1,"Error: %s failed with rc=%d\n",__FUNCTION__,rc);
    }

    const char* uid = ism_common_generateServerUID();
    TRACE(5,"%s Regenerated ServerUID: %s\n",__FUNCTION__,SoN(uid));

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int ism_cluster_getStatistics(const ismCluster_Statistics_t *pStatistics)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(9, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        if (mcpProps_SPtr && spdrProps_SPtr && spdrBootstrapSet_SPtr)
        {
            ismCluster_Statistics_t *pStats = const_cast<ismCluster_Statistics_t*>(pStatistics);
            pStats->state = ISM_CLUSTER_LS_STATE_INIT;
            pStats->connectedServers = 0;
            pStats->disconnectedServers = 0;
            spdr::PropertyMap::const_iterator it = mcpProps_SPtr->find(mcp::config::ClusterName_PROP_KEY);
            if (it != mcpProps_SPtr->end())
            {
                pStats->pClusterName = it->second.c_str();
            }
            else
            {
                TRACE(1," %s failed with rc=%d\n",__FUNCTION__,ISMRC_Error);
                return ISMRC_Error;
            }

            it = mcpProps_SPtr->find(mcp::config::LocalServerName_PROP_KEY);
            if (it != mcpProps_SPtr->end())
            {
                pStats->pServerName = it->second.c_str();
            }
            else
            {
                TRACE(1," %s failed with rc=%d\n",__FUNCTION__,ISMRC_Error);
                return ISMRC_Error;
            }

            it = mcpProps_SPtr->find(mcp::config::LocalServerUID_PROP_KEY);
            if (it != mcpProps_SPtr->end())
            {
                pStats->pServerUID = it->second.c_str();
            }
            else
            {
                TRACE(1," %s failed with rc=%d\n",__FUNCTION__,ISMRC_Error);
                return ISMRC_Error;
            }

            pStats->haStatus = haStatus_beforeStart;
            if (pStats->haStatus == ISM_CLUSTER_HA_STANDBY)
            {
            	pStats->state = ISM_CLUSTER_LS_STATE_STANDBY;
            }	
            pStats->healthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;
        }
        else
        {
            TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
            return ISMRC_ClusterNotAvailable;
        }
    }
    else
    {
        rc = mcpInstance_SPtr->getStatistics(pStatistics);
    }

    if (rc != ISMRC_OK)
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, rc);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;

}

//=== Admin View ==============================================================
extern int ism_cluster_getView(ismCluster_ViewInfo_t **pView)
{
    int rc;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->getView(pView);
    if (rc != ISMRC_OK)
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, rc);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_freeView(ismCluster_ViewInfo_t *pView)
{
    TRACE(9, "Entry: %s\n", __FUNCTION__);

    int rc = mcp::MCPRouting::freeView(pView);
    if (rc != ISMRC_OK)
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, rc);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

//=== Monitoring ==============================================================
extern int32_t ism_cluster_setHealthStatus(ismCluster_HealthStatus_t  healthStatus)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s status=%d\n", __FUNCTION__, static_cast<int>(healthStatus));
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->setHealthStatus(healthStatus);

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_setHaStatus(ismCluster_HaStatus_t haStatus)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s status=%d\n", __FUNCTION__, static_cast<int>(haStatus));
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(5, "%s, called before start, status=%d\n", __FUNCTION__, haStatus);
        haStatus_beforeStart = haStatus;
    }
    else
    {
        rc = mcpInstance_SPtr->setHaStatus(haStatus);
    }

    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

//=== Retained stats ==========================================================
extern int32_t ism_cluster_updateRetainedStats(
        const char *pServerUID,
        void *pData, uint32_t dataLength)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->updateRetainedStats(pServerUID,pData,dataLength);
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_lookupRetainedStats(
        const char *pServerUID,
        ismCluster_LookupRetainedStatsInfo_t **pLookupInfo)
{
    int rc = ISMRC_OK;

    TRACE(9, "Entry: %s\n", __FUNCTION__);
    if (enableClusterFlag == 0)
    {
        TRACE(1, "Warning: %s, cluster disabled, rc=%d\n", __FUNCTION__, ISMRC_ClusterDisabled);
        return ISMRC_ClusterDisabled;
    }

    if (!mcpInstance_SPtr)
    {
        TRACE(1, "Error: %s, cluster not available, rc=%d\n", __FUNCTION__, ISMRC_ClusterNotAvailable);
        return ISMRC_ClusterNotAvailable;
    }

    rc = mcpInstance_SPtr->lookupRetainedStats(pServerUID,pLookupInfo);
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

extern int32_t ism_cluster_freeRetainedStats(
        ismCluster_LookupRetainedStatsInfo_t *pLookupInfo)
{
    TRACE(9, "Entry: %s\n", __FUNCTION__);
    int rc = mcp::MCPRouting::freeRetainedStats(pLookupInfo);
    TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
    return rc;
}

}
