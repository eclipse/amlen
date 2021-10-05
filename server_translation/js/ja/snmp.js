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
        all: "すべて",
        unavailableAddressesTitle: "使用できないエージェント・アドレスがあります。",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "SNMP エージェント・アドレスは、有効なアクティブ・イーサネット・インターフェース・アドレスでなければなりません。 " +
        		"以下のエージェント・アドレスは、構成済みですが使用できません: {0}",
        agentAddressInvalidMessage: "<em>「すべて」</em> または少なくとも 1 つの IP アドレスを選択してください。",
        notRunningError: "SNMP は有効になっていますが、実行されていません。",
        snmpRestartFailureMessageTitle: "SNMP サービスの再始動に失敗しました。",
        statusLabel: "状況",
        running: "実行中",
        stopped: "停止",
        unknown: "不明",
        title: "SNMP サービス",
		tagLine: "SNMP サービスに影響を与える設定およびアクション。",
    	enableLabel: "SNMP の使用可能化",
    	stopDialogTitle: "SNMP サービスの停止",
    	stopDialogContent: "SNMP サービスを停止しますか?  サービスを停止すると、SNMP メッセージが失われる可能性があります。",
    	stopDialogOkButton: "停止",
    	stopDialogCancelButton: "キャンセル",
    	stopping: "停止中",
    	stopError: "SNMP サービスの停止中にエラーが発生しました。",
    	starting: "始動中",
    	started: "実行中",
    	savingProgress: "保管中...",
    	setSnmpEnabledError: "SNMP 使用可能状態の設定中にエラーが発生しました。",
    	startError: "SNMP サービスの始動中にエラーが発生しました。",
    	startLink: "サービスの始動",
    	stopLink: "サービスの停止",
		savingProgress: "保管中..."
});
