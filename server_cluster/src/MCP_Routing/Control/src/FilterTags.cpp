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

#include <FilterTags.h>

namespace mcp
{

const std::string FilterTags::BF_Prefix = "bf";

const std::string FilterTags::BF_ExactSub = FilterTags::BF_Prefix + "E";

const std::string FilterTags::BF_WildcardSub = FilterTags::BF_Prefix + "W";

const std::string FilterTags::Fwd_Endpoint = "FwdEP";

const std::string FilterTags::LocalServerInfo = "ServerInfo";

const std::string FilterTags::RetainedStats = "RetainedStats";

const std::string FilterTags::MonitoringStatus = "MonitoringStatus";

const std::string FilterTags::RemovedServersList = "RemovedServersList";

const std::string FilterTags::RestoredNotInView = "RestoredNotInView";


const std::string FilterTags::BF_ExactSub_Base = FilterTags::BF_ExactSub
		+ FilterTags::BF_Base_Suffix;

const std::string FilterTags::BF_ExactSub_Update = FilterTags::BF_ExactSub
		+ FilterTags::BF_Update_Suffix;

const std::string FilterTags::BF_WildcardSub_Base = FilterTags::BF_WildcardSub
		+ FilterTags::BF_Base_Suffix;

const std::string FilterTags::BF_WildcardSub_Update = FilterTags::BF_WildcardSub
		+ FilterTags::BF_Update_Suffix;

const std::string FilterTags::BF_Wildcard_SubscriptionPattern =
		FilterTags::BF_Prefix + "P";

const std::string FilterTags::BF_Wildcard_SubscriptionPattern_Base =
		FilterTags::BF_Wildcard_SubscriptionPattern
				+ FilterTags::BF_Base_Suffix;

const std::string FilterTags::BF_Wildcard_SubscriptionPattern_Update =
		FilterTags::BF_Wildcard_SubscriptionPattern
				+ FilterTags::BF_Update_Suffix;

const std::string FilterTags::RCF_Prefix = std::string("rcf");
const std::string FilterTags::RCF_Base = FilterTags::RCF_Prefix
		+ FilterTags::BF_Base_Suffix;
const std::string FilterTags::RCF_Update = FilterTags::RCF_Prefix
		+ FilterTags::BF_Update_Suffix;

const std::string FilterTags::WCSub_Stats = std::string("wcSubStats");

FilterTags::FilterTags()
{
}

FilterTags::~FilterTags()
{
}

} /* namespace mcp */
