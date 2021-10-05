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

#ifndef MCP_REMOTESERVERSTATUS_H_
#define MCP_REMOTESERVERSTATUS_H_

#include <string>
#include <map>

#include "cluster.h"
#include "MCPTypes.h"
#include "RemoteServerInfo.h"

namespace mcp
{

class RemoteServerStatus
{
public:
	RemoteServerStatus(
			const std::string& name,
			const std::string& uid,
			ServerIndex index,
			int64_t incarnation,
			bool controlAdded,
			bool controlConnected);

	virtual ~RemoteServerStatus();

	ismCluster_RemoteServer_t info;

	std::string name;
	const std::string uid;
	int64_t incarnation;

	/* Connectivity management */

	//Whether in the SpiderCast view, in any state, Alive or otherwise
	bool controlAdded;
	//Whether in the SpiderCast view as Alive
	bool controlConnected;

	bool engineAdded;
	bool engineConnected;
	bool forwardingAdded;
	bool forwardingConnected;

	bool routeAll;

	std::string forwardingAddress;
	uint16_t forwardingPort;
	uint8_t forwardingUseTLS;
	ism_time_t connectivityChangeTime;

	/* Monitoring status */
	ismCluster_HealthStatus_t  healthStatus;
	ismCluster_HaStatus_t haStatus;

	/* Sequence number management */

	uint64_t sqn_bf_exact_base;
	uint64_t sqn_bf_exact_last_update;

	uint64_t sqn_bf_wildcard_base;
	uint64_t sqn_bf_wildcard_last_update;

	uint64_t sqn_bf_wcsp_base;
	uint64_t sqn_bf_wcsp_last_update;

	uint64_t sqn_rcf_base;
	uint64_t sqn_rcf_last_update;

	typedef std::map<uint64_t,String_SPtr> RCF_Map;
	RCF_Map rcf_map;

	uint64_t sqn_sub_stats;

    uint64_t sqn_retained_stats_last_updated;

    uint64_t sqn_monitoring_status_last_update;

    uint64_t sqn_removed_servers_last_update;

    uint64_t sqn_restored_not_in_view_last_update;

	uint32_t protoVer_supported;
	uint32_t protoVer_used;

	std::string toString() const;

private:
	RemoteServerStatus();
	RemoteServerStatus(const RemoteServerStatus&);
	RemoteServerStatus& operator=(const RemoteServerStatus&);
};

typedef boost::shared_ptr<RemoteServerStatus> RemoteServerStatus_SPtr;

} /* namespace mcp */

#endif /* REMOTESERVERSTATUS_H_ */
