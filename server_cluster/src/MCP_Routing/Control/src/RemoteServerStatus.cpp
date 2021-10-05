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
#include "RemoteServerStatus.h"

namespace mcp
{

RemoteServerStatus::RemoteServerStatus(const std::string& name, const std::string& uid, ServerIndex index, int64_t incarnation, bool controlAdded, bool controlConnected) :
		info(),
		name(name),
		uid(uid),
		incarnation(incarnation),
		controlAdded(controlAdded),
        controlConnected(controlConnected),
		engineAdded(false),
		engineConnected(false),

		forwardingAdded(false),
		forwardingConnected(false),
		routeAll(false),
		forwardingAddress(""),
		forwardingPort(0),
		forwardingUseTLS(0),
		connectivityChangeTime(ism_common_currentTimeNanos()),
		healthStatus(ISM_CLUSTER_HEALTH_UNKNOWN),
		haStatus(ISM_CLUSTER_HA_UNKNOWN),
		sqn_bf_exact_base(0),
		sqn_bf_exact_last_update(0),
		sqn_bf_wildcard_base(0),
		sqn_bf_wildcard_last_update(0),
		sqn_bf_wcsp_base(0),
		sqn_bf_wcsp_last_update(0),
		sqn_rcf_base(0),
		sqn_rcf_last_update(0),
		sqn_sub_stats(0),
	    sqn_retained_stats_last_updated(0),
	    sqn_monitoring_status_last_update(0),
	    sqn_removed_servers_last_update(0),
	    sqn_restored_not_in_view_last_update(0),
		protoVer_supported(0),
		protoVer_used(0)
{
	info.index = index;
	info.engineHandle = NULL;
	info.protocolHandle = NULL;
	info.deletedFlag = 0;
}

RemoteServerStatus::~RemoteServerStatus()
{
}

std::string RemoteServerStatus::toString() const
{
    std::ostringstream s("RemoteServerStatus: ");
    s << "uid=" << uid << " name=" << name << " Inc=" << incarnation;
    s << "cA=" << controlAdded << " cC=" << controlConnected << " eA=" << engineAdded << " eC="
            << engineConnected << " fA=" << forwardingAdded << " fC="
            << forwardingConnected << " CCT=" << connectivityChangeTime << " Ra=" << routeAll;
    s << " Info={i=" << info.index << " cH=" << (&info) << " eH="
            << info.engineHandle << " pH=" << info.protocolHandle << " fDel="
            << static_cast<bool>(info.deletedFlag) << "}";
    s << " sqn={" << sqn_bf_exact_base << "," << sqn_bf_exact_last_update << ","
            << sqn_bf_wildcard_base << "," << sqn_bf_wildcard_last_update << ","
            << sqn_bf_wcsp_base << "," << sqn_bf_wcsp_last_update << ","
            << sqn_rcf_base << "," << sqn_rcf_last_update << ","
            << sqn_sub_stats << "," << sqn_retained_stats_last_updated << ","
            << sqn_monitoring_status_last_update << "," << sqn_removed_servers_last_update
            << sqn_restored_not_in_view_last_update << "}";
    s << " ver={" << protoVer_supported << "," << protoVer_used << "}";

    return s.str();
}

} /* namespace mcp */
