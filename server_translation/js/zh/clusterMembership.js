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

// Reviewed by Dev and ID
// Any exceptions are flagged with // Needs review until review is complete

define({
	    // Page title and tagline

	    title: "集群成员信息",
		tagline: "控制服务器是否属于集群。" +
				"集群是本地高速 LAN 上连接在一起的服务器集合，旨在将最大并行连接数或最大吞吐量扩展至超出单一设备能力。" +
				"集群中的服务器共享公共的主题树。",

		// Status section
		statusTitle: "状态",
		statusNotConfigured: "此服务器未配置为参与集群。" +
				"要使服务器参与集群，请编辑配置，然后将服务器添加到集群中。",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "此服务器不是集群成员。" +
				"要将服务器添加到集群中，请单击<em>重新启动服务器并加入集群 {0}</em>。",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "此服务器是集群 {0} 的成员。要从集群中移除此服务器，请单击<em>脱离集群</em>。",		
		clusterMembershipLabel: "集群成员信息：",
		notConfigured: "未配置",
		notSet: "未设置",
		notAMember: "不是成员",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "集群 {0} 的成员",

        // Cluster states  
        clusterStateLabel: "集群状态：", 
        initializing: "正在初始化",       
        discover: "发现",               
        active:  "活动",                   
        removed: "已移除",                 
        error: "错误",                     
        inactive: "不活动",               
        unavailable: "不可用",         
        unknown: "未知",                 
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "重新启动服务器并且加入集群 <em>{0}</em>",
        leaveLink: "脱离集群",
        // Configuration section

        configurationTitle: "配置",
		editLink: "编辑",
	    resetLink: "重置",
		configurationTagline: "这样会显示集群配置。" +
				"重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器后，对配置的更改才会生效。",
		clusterNameLabel: "集群名称：",
		controlAddressLabel: "控制地址",
		useMulticastDiscoveryLabel: "使用多点广播发现：",
		withMembersLabel: "发现服务器列表：",
		multicastTTLLabel: "多点广播发现 TTL：",
		controlInterfaceSectionTitle: "控制接口",
        messagingInterfaceSectionTitle: "消息传递接口",
		addressLabel: "地址：",
		portLabel: "端口：",
		externalAddressLabel: "外部地址：",
		externalPortLabel: "外部端口：",
		useTLSLabel: "使用 TLS：",
        yes: "是",
        no: "否",
        discoveryPortLabel: "发现端口：",
        discoveryTimeLabel: "发现时间：",
        seconds: "{0} 秒", 

		editDialogTitle: "编辑集群配置",
        editDialogInstruction_NotAMember: "设置集群名称以确定此服务器加入的集群。" +
                "保存有效的配置后，可以加入集群。",
        editDialogInstruction_Member: "更改集群配置。" +
        		"要更改此服务器所属集群的名称，必须先从 {0} 集群中移除此服务器。",
        saveButton: "保存",
        cancelButton: "取消",
        advancedSettingsLabel: "高级设置",
        advancedSettingsTagline: "您可以配置以下设置。" +
                "在更改这些设置之前，请参阅 ${IMA_PRODUCTNAME_FULL} 文档，以获取详细的指示信息和建议。",
		// Tooltips

        clusterNameTooltip: "要加入的集群名称。",
        membersTooltip: "成员的服务器 URI 的逗号分隔列表，这些成员属于此服务器要加入的集群。" +
        		"如果未选中<em>使用多点广播发现</em>，请指定至少一个服务器 URI。" +
        		"成员的服务器 URI 显示在该成员的“集群成员信息”页面上。",
        useMulticastTooltip: "指定是否应使用多点广播来发现具有相同集群名称的服务器。",
        multicastTTLTooltip: "如果您使用多点广播发现，这将指定允许多点广播流量经过的网络中继段数量。", 
        controlAddressTooltip: "控制接口的 IP 地址。",
        controlPortTooltip: "用于控制接口的端口号。缺省端口为 9104。",
        controlExternalAddressTooltip: "控制接口的外部 IP 地址或主机名（如果不同于控制地址）。" +
        		"外部地址供其他成员用于连接至此服务器的控制接口。",
		controlExternalPortTooltip: "用于控制接口的外部端口（如果不同于控制端口）。"  +
                "外部端口供其他成员用于连接至此服务器的控制接口。",
        messagingAddressTooltip: "消息传递接口的 IP 地址。",
        messagingPortTooltip: "用于消息传递接口的端口号。缺省端口为 9105。",
        messagingExternalAddressTooltip: "消息传递接口的外部 IP 地址或主机名（如果不同于消息传递地址）。" +
                "外部地址供其他成员用于连接至此服务器的消息传递接口。",
        messagingExternalPortTooltip: "用于消息传递接口的外部端口（如果不同于消息传递端口）。"  +
                "外部端口供其他成员用于连接至此服务器的消息传递接口。",
        discoveryPortTooltip: "用于多点广播发现的端口号。缺省端口为 9106。" +
        		"必须在集群的所有成员上使用相同的端口。",
        discoveryTimeTooltip: "服务器启动期间用于发现集群中的其他服务器并从中获取更新信息的时间（秒）。" +
                "有效范围为 1 - 2147483647。缺省值为 10。" + 
                "在获取更新信息或者发生发现超时（以先发生者为准）后，该服务器会成为集群的活动成员。",
        controlInterfaceSectionTooltip: "控制接口用于集群的配置、监视和控制。",
        messagingInterfaceSectionTooltip: "消息传递接口支持将从客户机发布至集群成员的消息转发至集群的其他成员。",
        // confirmation dialogs
        restartTitle: "重新启动 ${IMA_PRODUCTNAME_SHORT}",
        restartContent: "确定要重新启动 ${IMA_PRODUCTNAME_SHORT} 吗？",
        restartButton: "重新启动",

        resetTitle: "重置集群成员信息",
        resetContent: "确定要重置集群成员信息配置吗？",
        resetButton: "重置",

        leaveTitle: "脱离集群",
        leaveContent: "确定要脱离集群吗？",
        leaveButton: "脱离",      
        restartRequiredTitle: "需要重新启动",
        restartRequiredContent: "您的更改已保存，但是在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器后更改才会生效。<br /><br />" +
                 "要现在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器，请单击<b>立即重新启动</b>。否则单击<b>稍后重新启动</b>。",
        restartLaterButton: "稍后重新启动",
        restartNowButton: "立即重新启动",

        // messages
        savingProgress: "正在保存...",
        errorGettingClusterMembership: "检索集群成员信息时发生错误。",
        saveClusterMembershipError: "保存集群成员信息配置时发生错误。",
        restarting: "正在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器...",
        restartingFailed: "无法重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器。",
        resetting: "正在重置配置...",
        resettingFailed: "无法重置集群成员信息配置。",
        addressInvalid: "需要有效 IP 地址。", 
        portInvalid: "端口号必须是 1 和 65535（含）之间的一个数字。"

});
