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
        // TRNOTE: {0} is replaced with the server name of the server currently managed in the Web UI.
        //         {1} is replaced with the name of the cluster where the current server is a member.
		detail_title: "集群 {1} 中 {0} 的集群状态",
		title: "集群状态",
		tagline: "监视集群成员的状态。查看有关此服务器上消息传递通道的统计信息。消息传递通道提供" +
				"与远程集群成员的连接以及这些远程集群成员的缓冲消息。如果不活动成员的缓冲消息占用过多" +
				"资源，您可以从集群中除去这些不活动成员。",
		chartTitle: "吞吐量图表",
		gridTitle: "集群状态数据",
		none: "无",
		lastUpdated: "上次更新时间",
		notAvailable: "不适用",
		cacheInterval: "数据收集时间间隔：5 秒",
		refreshingLabel: "正在更新...",
		resumeUpdatesButton: "恢复更新",
		updatesPaused: "数据收集已暂停",
		serverNameLabel: "服务器名称",
		connStateLabel: "连接状态：",
		healthLabel: "服务器运行状况",
		haStatusLabel: "HA 状态：",
		updTimeLabel: "上次状态更新",
		incomingMsgsLabel: "入局消息数/秒：",
		outgoingMsgsLabel: "出局消息数/秒：",
		bufferedMsgsLabel: "已缓冲的消息数：",
		discardedMsgsLabel: "已废弃的消息数：",
		serverNameTooltip: "服务器名称",
		connStateTooltip: "连接状态",
		healthTooltip: "服务器运行状况",
		haStatusTooltip: "HA 状态",
		updTimeTooltip: "上次状态更新",
		incomingMsgsTooltip: "入局消息数/秒",
		outgoingMsgsTooltip: "出局消息数/秒",
		bufferedMsgsTooltip: "已缓冲的消息数",
		discardedMsgsTooltip: "已丢弃的消息数",
		statusUp: "活动",
		statusDown: "不活动",
		statusConnecting: "正在连接",
		healthUnknown: "未知",
		healthGood: "良好",
		healthWarning: "警告",
		healthBad: "错误",
		haDisabled: "已禁用",
		haSynchronized: "已同步",
		haUnsynchronized: "未同步",
		haError: "错误",
		haUnknown: "未知",
		BufferedAssocType: "已缓冲的消息详细信息",
		DiscardedAssocType: "已丢弃的消息详细信息",
		channelTitle: "通道",
		channelTooltip: "通道",
		channelUnreliable: "不可靠",
		channelReliable: "可靠",
		bufferedBytesLabel: "已缓冲的字节数",
		bufferedBytesTooltip: "已缓冲的字节数",
		bufferedHWMLabel: "已缓冲的消息 HWM",
		bufferedHWMTooltip: "已缓冲的消息 HWM",
		addServerDialogInstruction: "将集群成员添加到受管服务器列表中，并管理集群成员服务器。",
		saveServerButton: "保存并管理",
		removeServerDialogTitle: "从集群中除去服务器",
		removeConfirmationTitle: "确定要从集群中除去此服务器吗？",
		removeServerError: "从集群中删除服务器时发生错误。",

		removeNotAllowedTitle: "不允许除去",
		removeNotAllowed: "由于该服务器当前处于活动状态，因此无法将其从集群中除去。" +
		                  "使用除去操作来仅除去处于不活动状态的服务器。",

		cancelButton: "取消",
		yesButton: "是",
		closeButton: "关闭"
});
