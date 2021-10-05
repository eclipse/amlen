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
		detail_title: "クラスター {1} の {0} のクラスターの状況",
		title: "クラスターの状況",
		tagline: "クラスター・メンバーの状況をモニターします。 このサーバー上のメッセージング・チャネルに関する統計を表示します。 メッセージング・チャネルは、 " +
				"リモート・クラスター・メンバーへの接続を提供し、それらのメンバーに対するメッセージをバッファーに入れます。  非アクティブ・メンバーのバッファーに入れられているメッセージが多量のリソースを消費している場合には、 " +
				"それらの非アクティブ・メンバーをクラスターから削除できます。",
		chartTitle: "スループットのグラフ",
		gridTitle: "クラスター状況のデータ",
		none: "なし",
		lastUpdated: "最終更新日時",
		notAvailable: "n/a",
		cacheInterval: "データ収集間隔: 5 秒",
		refreshingLabel: "更新中...",
		resumeUpdatesButton: "更新を再開",
		updatesPaused: "データ収集が一時停止されました",
		serverNameLabel: "サーバー名",
		connStateLabel: "接続状態:",
		healthLabel: "サーバーの正常性",
		haStatusLabel: "HA の状況:",
		updTimeLabel: "最終状態更新",
		incomingMsgsLabel: "着信メッセージ/秒:",
		outgoingMsgsLabel: "発信メッセージ/秒:",
		bufferedMsgsLabel: "バッファーに入れられているメッセージ:",
		discardedMsgsLabel: "廃棄メッセージ:",
		serverNameTooltip: "サーバー名",
		connStateTooltip: "接続状態",
		healthTooltip: "サーバーの正常性",
		haStatusTooltip: "HA の状況",
		updTimeTooltip: "最終状態更新",
		incomingMsgsTooltip: "着信メッセージ/秒",
		outgoingMsgsTooltip: "発信メッセージ/秒",
		bufferedMsgsTooltip: "バッファーに入れられているメッセージ",
		discardedMsgsTooltip: "廃棄メッセージ",
		statusUp: "アクティブ",
		statusDown: "非アクティブ",
		statusConnecting: "接続中",
		healthUnknown: "不明",
		healthGood: "良好",
		healthWarning: "警告",
		healthBad: "エラー",
		haDisabled: "使用不可",
		haSynchronized: "同期",
		haUnsynchronized: "非同期",
		haError: "エラー",
		haUnknown: "不明",
		BufferedAssocType: "バッファーに入れられているメッセージの詳細",
		DiscardedAssocType: "廃棄メッセージの詳細",
		channelTitle: "チャネル",
		channelTooltip: "チャネル",
		channelUnreliable: "不安定",
		channelReliable: "安定",
		bufferedBytesLabel: "バッファーに入れられたバイト数",
		bufferedBytesTooltip: "バッファーに入れられたバイト数",
		bufferedHWMLabel: "バッファーに入るメッセージの上限基準点",
		bufferedHWMTooltip: "バッファーに入るメッセージの上限基準点",
		addServerDialogInstruction: "管理対象サーバーのリストにクラスター・メンバーを追加し、クラスター・メンバー・サーバーを管理します。",
		saveServerButton: "保管して管理",
		removeServerDialogTitle: "クラスターからのサーバーの削除",
		removeConfirmationTitle: "クラスターからこのサーバーを削除しますか?",
		removeServerError: "クラスターからサーバーを削除中にエラーが発生しました。",

		removeNotAllowedTitle: "削除は許可されていません",
		removeNotAllowed: "サーバーは、現在アクティブであるためクラスターから削除できません。 " +
		                  "削除アクションでは、アクティブでないサーバーのみを削除してください。",

		cancelButton: "キャンセル",
		yesButton: "はい",
		closeButton: "閉じる"
});
