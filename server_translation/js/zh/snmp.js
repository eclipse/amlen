/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
        all: "全部",
        unavailableAddressesTitle: "某些代理地址不可用。",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "SNMP 代理地址必须是有效的活动以太网接口地址。" +
        		"已配置以下代理地址，但是这些地址不可用：{0}",
        agentAddressInvalidMessage: "选择 <em>All</em> 或者至少选择一个 IP 地址。",
        notRunningError: "SNMP 已启用但未在运行。",
        snmpRestartFailureMessageTitle: "SNMP 服务无法重新启动。",
        statusLabel: "状态",
        running: "正在运行",
        stopped: "已停止",
        unknown: "未知",
        title: "SNMP 服务",
		tagLine: "影响 SNMP 服务的设置和操作。",
    	enableLabel: "启用 SNMP",
    	stopDialogTitle: "停止 SNMP 服务",
    	stopDialogContent: "确定要停止 SNMP 服务吗？停止服务可能导致 SNMP 消息丢失。",
    	stopDialogOkButton: "停止",
    	stopDialogCancelButton: "取消",
    	stopping: "正在停止",
    	stopError: "停止 SNMP 服务时发生错误。",
    	starting: "正在启动",
    	started: "正在运行",
    	savingProgress: "正在保存...",
    	setSnmpEnabledError: "设置 SNMP 已启用状态时发生错误。",
    	startError: "启动 SNMP 服务时发生错误。",
    	startLink: "启动服务",
    	stopLink: "停止服务",
		savingProgress: "正在保存..."
});
