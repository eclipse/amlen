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
        all: "All",
        unavailableAddressesTitle: "部分代理程式位址無法使用。",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "SNMP 代理程式位址必須是有效的作用中乙太網路介面位址。" +
        		"下列代理程式位址已配置但無法使用：{0}",
        agentAddressInvalidMessage: "請選取「<em>全部</em>」或至少一個 IP 位址。",
        notRunningError: "SNMP 已啟用但不在執行中。",
        snmpRestartFailureMessageTitle: "SNMP 服務無法重新啟動。",
        statusLabel: "狀態",
        running: "執行中",
        stopped: "已停止",
        unknown: "不明",
        title: "SNMP 服務",
		tagLine: "影響 SNMP 服務的設定和動作。",
    	enableLabel: "啟用 SNMP",
    	stopDialogTitle: "停止 SNMP 服務",
    	stopDialogContent: "您確定要停止 SNMP 服務嗎？停止服務可能會導致 SNMP 訊息遺失。",
    	stopDialogOkButton: "停止",
    	stopDialogCancelButton: "取消",
    	stopping: "停止中",
    	stopError: "停止 SNMP 服務時發生錯誤。",
    	starting: "啟動中",
    	started: "執行中",
    	savingProgress: "儲存中…",
    	setSnmpEnabledError: "設定 SNMPEnabled 狀態時發生錯誤。",
    	startError: "啟動 SNMP 服務時發生錯誤。",
    	startLink: "啟動服務",
    	stopLink: "停止服務",
		savingProgress: "儲存中…"
});
