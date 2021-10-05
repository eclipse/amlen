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

#include <MCPRouting.h>
#include <boost/lexical_cast.hpp>

#include "MCPRoutingImpl.h"

namespace mcp {


spdr::ScTraceComponent* MCPRouting::tc_ = spdr::ScTr::enroll(
		mcp::trace::Component_Name,
		mcp::trace::SubComponent_Core,
		spdr::trace::ScTrConstants::Layer_ID_App,
		"MCPRouting",
		spdr::trace::ScTrConstants::ScTr_ResourceBundle_Name);

spdr::ScTraceContextImpl MCPRouting::tcntx_(tc_,"MCP","static");

MCPRouting::MCPRouting()
{
}

MCPRouting::~MCPRouting()
{
}

MCPReturnCode MCPRouting::create(
			const spdr::PropertyMap& mcpProps,
			const spdr::PropertyMap& spidercastProps,
			const std::vector<spdr::NodeID_SPtr>& spidercastBootstrapSet,
			MCPRouting** mcpRouting)
{
    Trace_Entry(&tcntx_, "create()");

    //create instance ID for tracing several instances in the same process.
    static int instanceCounter = 0;
    std::string instanceID("MCC");
    instanceID = instanceID.append(
            boost::lexical_cast<std::string>(++instanceCounter));

    MCPReturnCode rc = ISMRC_OK;

    MCPRouting* instance = NULL;
    try
    {
        instance = static_cast<MCPRouting*>(new MCPRoutingImpl(instanceID,
                mcpProps, spidercastProps, spidercastBootstrapSet));
    }
    catch (spdr::IllegalConfigException& ice)
    {
        Trace_Debug(&tcntx_, "create()", "IllegalConfigException", "what",
                ice.what(), "stacktrace", ice.getStackTrace());
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s","-",ice.what());
    }
    catch (MCPRuntimeError& re)
    {
        Trace_Debug(&tcntx_, "create()", "MCPRuntimeError", "what", re.what(),
                "stacktrace", re.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(ISMRC_ClusterInternalError, "%s", re.what());
    }
    catch (spdr::BindException& be)
    {
        Trace_Debug(&tcntx_, "create()", "spdr::BindException", "what",
                be.what(), "stacktrace", be.getStackTrace());

        rc = ISMRC_ClusterInternalError;
        if (be.getErrorCode() == spdr::event::Bind_Error_Port_Busy)
        {
            rc = ISMRC_ClusterBindErrorPortBusy;
            ism_common_setErrorData(rc,"%u", be.getPort());

        }
        else if (be.getErrorCode()
                == spdr::event::Bind_Error_Cannot_Assign_Local_Interface)
        {
            rc = ISMRC_ClusterBindErrorLocalAddress;
            ism_common_setErrorData(rc, "%s", be.getAddress().c_str());
        }
    }
    catch (spdr::SpiderCastLogicError& scle)
    {
        Trace_Debug(&tcntx_, "create()", "SpiderCastLogicError", "what",
                scle.what(), "stacktrace", scle.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(rc, "%s", scle.what());
    }
    catch (spdr::SpiderCastRuntimeError& scre)
    {
        Trace_Debug(&tcntx_, "create()", "SpiderCastRuntimeError", "what",
                scre.what(), "stacktrace", scre.getStackTrace());
        rc = ISMRC_ClusterInternalError;
        ism_common_setErrorData(rc, "%s", scre.what());
    }
    catch (std::bad_alloc& ba)
    {
        Trace_Debug(&tcntx_, "create()", "std::bad_alloc", "what", ba.what());
        rc = ISMRC_AllocateError;
    }
    catch (std::exception& e)
    {
        Trace_Debug(&tcntx_, "create()", "std::exception", "what", e.what());
        rc = ISMRC_Error;
    }
    catch (...)
    {
        Trace_Debug(&tcntx_, "create()", "untyped exception (...)");
        rc = ISMRC_Error;
    }

    if (rc == ISMRC_OK)
    {
        *mcpRouting = instance;
    }

    Trace_Exit(&tcntx_, "create()", rc);
    return rc;
}

MCPReturnCode MCPRouting::freeRetainedStats(
        ismCluster_LookupRetainedStatsInfo_t *pLookupInfo)
{
    return GlobalRetainedStatsManager::freeRetainedStats(pLookupInfo);
}

MCPReturnCode MCPRouting::freeView(ismCluster_ViewInfo_t *pView)
{
    return ViewKeeper::freeView(pView);
}

String MCPRouting::toStringView(ismCluster_ViewInfo_t *pView)
{
    return ViewKeeper::toString_ViewInfo(pView);
}

spdr::log::Level MCPRouting::ismLogLevel_to_spdrLogLevel(int ismLogLevel)
{
    using namespace spdr;

    switch (ismLogLevel)
    {
    case 0:
        return log::Level_NONE;

    case 1:
        return log::Level_ERROR;

    case 2:
        return log::Level_WARNING;

    case 3:
        return log::Level_INFO;

    case 4:
    case 5:
        return log::Level_CONFIG;

    case 6:
        return log::Level_EVENT;

    case 7:
        return log::Level_DEBUG;

    case 8:
        return log::Level_ENTRY;

    case 9:
        return log::Level_DUMP;

    default:
        return log::Level_NONE;

    }
}


} /* namespace mcp */
