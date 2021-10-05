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
			node: "ノード",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "ご使用条件",
			menuContent: "メニュー選択内容",
			home : "ホーム",
			messaging : "メッセージング",
			monitoring : "モニター",
			appliance : "サーバー",
			login: "ログイン",
			logout: "ログアウト",
			changePassword: "パスワード変更",
			yes: "はい",
			no: "いいえ",
			all: "すべて",
			trueValue: "True",
			falseValue: "False",
			days: "日",
			hours: "時間",
			minutes: "分",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0} 日 {1} 時間",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0} 時間 {1} 分",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0} 分 {1} 秒",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0} 秒", 
			notAvailable: "NA",
			missingRequiredMessage: "値が必要です",
			pageNotAvailable: "${IMA_PRODUCTNAME_SHORT} サーバーの状況が適切でないため、このページを使用できません",
			pageNotAvailableServerDetail: "このページを操作するには、${IMA_PRODUCTNAME_SHORT} サーバーが実動モードで稼働している必要があります。",
			pageNotAvailableHAroleDetail: "このページを操作するには、${IMA_PRODUCTNAME_SHORT} サーバーは 1 次サーバーであって同期中でないか、HA が使用不可になっている必要があります。 "			
		},
		name: {
			label: "名前",
			tooltip: "名前の先頭と末尾にはスペースがあってはなりません。また、名前には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
					 "先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "名前の先頭および末尾をスペースにすることはできません。",
			noSpaces: "名前にはスペースを使用できません。",
			invalidFirstChar: "先頭文字は、数字、制御文字、または次のいずれかの特殊文字であってはなりません。! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "名前には、制御文字、または次のいずれかの特殊文字を含めることはできません。&quot; , = &#xa5; ",
			duplicateName: "その名前のレコードはすでに存在します。",
			unicodeAlphanumericOnly: {
				invalidChar: "名前は英数字のみで構成されている必要があります。",
				invalidFirstChar: "先頭文字は数字であってはなりません。"
			}
		},
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "メッセージング・プロトコル",
				userAdministration: "ユーザー認証",
				messageHubs: "メッセージング・ハブ",
				messageHubDetails: "メッセージング・ハブの詳細",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "メッセージ・キュー",
				messagingTester: "サンプル・アプリケーション"
			},
			monitoring: {
				connectionStatistics: "接続",
				endpointStatistics: "エンドポイント",
				queueMonitor: "キュー",
				topicMonitor: "トピック",
				mqttClientMonitor: "切断された MQTT クライアント",
				subscriptionMonitor: "サブスクリプション",
				transactionMonitor: "トランザクション",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "サーバー",
				downloadLogs: "ログのダウンロード",
				snmpSettings: "SNMP 設定"
			},
			appliance: {
				users: "Web UI ユーザー",
				networkSettings: "ネットワーク設定",
			    locale: "ロケール、日付と時刻",
			    securitySettings: "セキュリティー設定",
			    systemControl: "サーバー制御",
			    highAvailability: "高可用性",
			    webuiSecuritySettings: "Web UI 設定"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "ヘルプ",
			homeTasks : "ホーム・ページにタスクを復元",
			about : {
				linkTitle : "バージョン情報",
				dialogTitle: "${IMA_PRODUCTNAME_FULL} について",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "{0} ご使用条件の表示",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} バージョン ${ISM_VERSION_ID}"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "パスワード変更",
				currpasswd: "現在のパスワード:",
				newpasswd: "新規パスワード:",
				password2: "パスワードの確認:",
				password2Invalid: "パスワードが一致しません",
				savingProgress: "保管中...",
				savingFailed: "保管に失敗しました。"
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "情報",
			Warning : "警告",
			Error : "エラー",
			Confirmation : "確認",
			Success : "正常終了"
		},

		action : {
			Ok : "OK",
			Close : "閉じる",
			Cancel : "キャンセル",
			Save: "保管",
			Create: "作成",
			Add: "追加",
			Edit: "編集",
			Delete: "削除",
			MoveUp: "上へ移動",
			MoveDown: "下へ移動",
			ResetPassword: "パスワードの再設定",
			Actions: "アクション",
			OtherActions: "その他のアクション",
			View: "表示",
			ResetColWidth: "列幅のリセット",
			ChooseColumns: "表示する列の選択",
			ResetColumns: "表示する列のリセット"
		},
		// new pages and tabs need to go here
        cluster: "クラスター",
        clusterMembership: "参加/離脱",
        adminEndpoint: "管理エンドポイント",
        firstserver: "サーバーへ接続",
        portInvalid: "ポート番号は 1 から 65535 までの範囲内の番号でなければなりません。",
        connectionFailure: "${IMA_PRODUCTNAME_FULL} サーバーに接続できません。",
        clusterStatus: "状況",
        webui: "Web UI",
        licenseType_Devlopers: "開発者",
        licenseType_NonProd: "非実動",
        licenseType_Prod: "実動",
        licenseType_Beta: "Beta"
});
