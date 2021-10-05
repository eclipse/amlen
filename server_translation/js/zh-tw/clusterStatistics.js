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
		detail_title: "{1} 叢集中的 {0} 的叢集狀態",
		title: "叢集狀態",
		tagline: "監視叢集成員的狀態。檢視此伺服器上傳訊通道的相關統計資料。傳訊通道提供" +
				"遠端叢集成員的連線功能及緩衝區訊息。如果非作用中成員的緩衝訊息耗用太多" +
				"資源，您可以從叢集中移除非作用中成員。",
		chartTitle: "傳輸量圖表",
		gridTitle: "叢集狀態資料",
		none: "無",
		lastUpdated: "前次更新時間",
		notAvailable: "不適用",
		cacheInterval: "資料收集間隔：5 秒",
		refreshingLabel: "更新中...",
		resumeUpdatesButton: "回復更新",
		updatesPaused: "已暫停資料收集",
		serverNameLabel: "伺服器名稱",
		connStateLabel: "連線狀態：",
		healthLabel: "伺服器性能狀態",
		haStatusLabel: "HA 狀態：",
		updTimeLabel: "前次狀態更新",
		incomingMsgsLabel: "送入的訊息數/秒：",
		outgoingMsgsLabel: "送出的訊息數/秒：",
		bufferedMsgsLabel: "緩衝的訊息數：",
		discardedMsgsLabel: "捨棄的訊息數：",
		serverNameTooltip: "伺服器名稱",
		connStateTooltip: "連線狀態",
		healthTooltip: "伺服器性能狀態",
		haStatusTooltip: "HA 狀態",
		updTimeTooltip: "前次狀態更新",
		incomingMsgsTooltip: "送入的訊息數/秒",
		outgoingMsgsTooltip: "送出的訊息數/秒",
		bufferedMsgsTooltip: "緩衝的訊息數",
		discardedMsgsTooltip: "捨棄的訊息數",
		statusUp: "作用中",
		statusDown: "非作用中",
		statusConnecting: "連線中",
		healthUnknown: "不明",
		healthGood: "良好",
		healthWarning: "警告",
		healthBad: "錯誤",
		haDisabled: "已停用",
		haSynchronized: "已同步",
		haUnsynchronized: "未同步",
		haError: "錯誤",
		haUnknown: "不明",
		BufferedAssocType: "緩衝的訊息詳細資料",
		DiscardedAssocType: "捨棄的訊息詳細資料",
		channelTitle: "通道",
		channelTooltip: "通道",
		channelUnreliable: "不可靠",
		channelReliable: "可靠",
		bufferedBytesLabel: "緩衝的位元組數",
		bufferedBytesTooltip: "緩衝的位元組數",
		bufferedHWMLabel: "緩衝的訊息數 HWM",
		bufferedHWMTooltip: "緩衝的訊息數 HWM",
		addServerDialogInstruction: "將叢集成員新增到受管理伺服器的清單，並管理叢集成員伺服器。",
		saveServerButton: "儲存並管理",
		removeServerDialogTitle: "從叢集中移除伺服器",
		removeConfirmationTitle: "您確定要從叢集中移除此伺服器嗎？",
		removeServerError: "從叢集刪除伺服器時發生錯誤。",

		removeNotAllowedTitle: "不容許移除",
		removeNotAllowed: "因為伺服器目前為作用中，所以無法從叢集中移除該伺服器。" +
		                  "請使用移除動作，只移除那些非作用中的伺服器。",

		cancelButton: "取消",
		yesButton: "是",
		closeButton: "關閉"
});
