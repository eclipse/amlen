/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
		// ------------------------------------------------------------------------
		// Global Definitions
		// ------------------------------------------------------------------------
		global : {
			productName : "${IMA_PRODUCTNAME_FULL}", // Do_Not_Translate
			productNameTM: "${IMA_PRODUCTNAME_FULL_HTML}",	// Do_Not_Translate 		
			webUIMainTitle: "${IMA_PRODUCTNAME_FULL} Web UI",
			node: "節點",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "授權合約",
			menuContent: "功能表選項內容",
			home : "首頁",
			messaging : "傳訊",
			monitoring : "監視",
			appliance : "伺服器",
			login: "登入",
			logout: "登出",
			changePassword: "變更密碼",
			yes: "是",
			no: "否",
			all: "All",
			trueValue: "True",
			falseValue: "False",
			days: "日",
			hours: "小時",
			minutes: "分鐘",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0} 日 {1} 小時",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0} 小時 {1} 分",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0} 分 {1} 秒",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0} 秒", 
			notAvailable: "不適用",
			missingRequiredMessage: "需要值",
			pageNotAvailable: "因為 ${IMA_PRODUCTNAME_SHORT} 伺服器的狀態而無法使用此頁面",
			pageNotAvailableServerDetail: "若要操作此頁面，${IMA_PRODUCTNAME_SHORT} 伺服器必須在正式作業模式下執行。",
			pageNotAvailableHAroleDetail: "若要操作此頁面，${IMA_PRODUCTNAME_SHORT} 伺服器必須是主要伺服器且不在同步化，或必須停用 HA。"			
		},
		name: {
			label: "名稱",
			tooltip: "名稱不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
					 "第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "名稱不能有前導空格或尾端空格。",
			noSpaces: "名稱不能有任何空格。",
			invalidFirstChar: "第一個字元不能是數字、控制字元或下列任何特殊字元：! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "名稱不得包含控制字元或下列任何特殊字元：&quot; , = \\ ",
			duplicateName: "已存在具有該名稱的記錄。",
			unicodeAlphanumericOnly: {
				invalidChar: "名稱必須只能由英數字元組成。",
				invalidFirstChar: "第一個字元不得是數字。"
			}
		},
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "傳訊通訊協定",
				userAdministration: "使用者鑑別",
				messageHubs: "訊息中心",
				messageHubDetails: "訊息中心詳細資料",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "訊息佇列",
				messagingTester: "範例應用程式"
			},
			monitoring: {
				connectionStatistics: "連線",
				endpointStatistics: "端點",
				queueMonitor: "佇列",
				topicMonitor: "主題",
				mqttClientMonitor: "中斷連線的 MQTT 用戶端",
				subscriptionMonitor: "訂閱",
				transactionMonitor: "交易",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "伺服器",
				downloadLogs: "下載日誌",
				snmpSettings: "SNMP 設定"
			},
			appliance: {
				users: "Web UI 使用者",
				networkSettings: "網路設定",
			    locale: "語言環境、日期和時間",
			    securitySettings: "安全設定",
			    systemControl: "伺服器控制",
			    highAvailability: "高可用性",
			    webuiSecuritySettings: "Web UI 設定"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "說明",
			homeTasks : "在首頁還原作業",
			about : {
				linkTitle : "關於",
				dialogTitle: "關於 ${IMA_PRODUCTNAME_FULL}",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "顯示 {0} 授權合約",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} ${ISM_VERSION_ID} 版"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "變更密碼",
				currpasswd: "現行密碼：",
				newpasswd: "新密碼：",
				password2: "確認密碼：",
				password2Invalid: "密碼不符",
				savingProgress: "儲存中…",
				savingFailed: "儲存失敗。"
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "資訊",
			Warning : "警告",
			Error : "錯誤",
			Confirmation : "確認",
			Success : "成功"
		},

		action : {
			Ok : "確定",
			Close : "關閉",
			Cancel : "取消",
			Save: "儲存",
			Create: "建立",
			Add: "新增",
			Edit: "編輯",
			Delete: "刪除",
			MoveUp: "上移",
			MoveDown: "下移",
			ResetPassword: "重設密碼",
			Actions: "動作",
			OtherActions: "其他動作",
			View: "檢視",
			ResetColWidth: "重設直欄寬度",
			ChooseColumns: "選擇可見直欄",
			ResetColumns: "重設可見直欄"
		},
		// new pages and tabs need to go here
        cluster: "叢集",
        clusterMembership: "加入/離開",
        adminEndpoint: "管理端點",
        firstserver: "連接至伺服器",
        portInvalid: "埠號必須是 1 到 65535 範圍內的一個數字。",
        connectionFailure: "無法連接至 ${IMA_PRODUCTNAME_FULL} 伺服器。",
        clusterStatus: "狀態",
        webui: "Web UI",
        licenseType_Devlopers: "開發者",
        licenseType_NonProd: "非正式作業",
        licenseType_Prod: "正式作業",
        licenseType_Beta: "測試版"
});
