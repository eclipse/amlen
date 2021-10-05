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
			node: "节点",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "许可协议",
			menuContent: "菜单选择内容",
			home : "主页",
			messaging : "消息传递",
			monitoring : "监视",
			appliance : "服务器",
			login: "登录",
			logout: "注销",
			changePassword: "更改密码",
			yes: "是",
			no: "否",
			all: "全部",
			trueValue: "True",
			falseValue: "False",
			days: "天",
			hours: "小时",
			minutes: "分钟",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0} 天 {1} 小时",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0} 小时 {1} 分",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0} 分 {1} 秒",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0} 秒", 
			notAvailable: "不适用",
			missingRequiredMessage: "需要一个值",
			pageNotAvailable: "由于 ${IMA_PRODUCTNAME_SHORT} 服务器的状态，所以该页面不可用",
			pageNotAvailableServerDetail: "要操作此页面，${IMA_PRODUCTNAME_SHORT} 服务器必须以生产方式运行。",
			pageNotAvailableHAroleDetail: "要操作此页面，${IMA_PRODUCTNAME_SHORT} 服务器必须是主服务器并且未同步，否则必须禁用 HA。"			
		},
		name: {
			label: "姓名",
			tooltip: "名称不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
					 "第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "名称不能包含前导或尾部空格。",
			noSpaces: "名称不能包含任何空格。",
			invalidFirstChar: "第一个字符不能为数字、控制字符或以下任何特殊字符：! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "名称不能包含控制字符或以下任何特殊字符：&quot; , =\\ ",
			duplicateName: "已存在同名记录。",
			unicodeAlphanumericOnly: {
				invalidChar: "名称只能包含字母数字字符。",
				invalidFirstChar: "第一个字符不能是数字。"
			}
		},
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "消息传递协议",
				userAdministration: "用户认证",
				messageHubs: "消息中心",
				messageHubDetails: "消息中心详细信息",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "消息队列",
				messagingTester: "样本应用程序"
			},
			monitoring: {
				connectionStatistics: "连接",
				endpointStatistics: "端点",
				queueMonitor: "队列",
				topicMonitor: "主题",
				mqttClientMonitor: "断开连接的 MQTT 客户机",
				subscriptionMonitor: "订购",
				transactionMonitor: "事务",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "服务器",
				downloadLogs: "下载日志",
				snmpSettings: "SNMP 设置"
			},
			appliance: {
				users: "Web UI 用户",
				networkSettings: "网络设置",
			    locale: "语言环境，日期和时间",
			    securitySettings: "安全设置",
			    systemControl: "服务器控制",
			    highAvailability: "高可用性",
			    webuiSecuritySettings: "Web UI 设置"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "帮助",
			homeTasks : "恢复主页上的任务",
			about : {
				linkTitle : "关于",
				dialogTitle: "关于 ${IMA_PRODUCTNAME_FULL}",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "显示“{0}”许可协议",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} V${ISM_VERSION_ID}"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "更改密码",
				currpasswd: "当前密码：",
				newpasswd: "新密码：",
				password2: "确认密码：",
				password2Invalid: "密码不匹配",
				savingProgress: "正在保存...",
				savingFailed: "保存失败。"
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "信息",
			Warning : "警告",
			Error : "错误",
			Confirmation : "确认",
			Success : "成功"
		},

		action : {
			Ok : "确定",
			Close : "关闭",
			Cancel : "取消",
			Save: "保存",
			Create: "创建",
			Add: "添加",
			Edit: "编辑",
			Delete: "删除",
			MoveUp: "上移",
			MoveDown: "下移",
			ResetPassword: "重置密码",
			Actions: "操作",
			OtherActions: "其他操作",
			View: "查看",
			ResetColWidth: "重置列宽",
			ChooseColumns: "选择可视列",
			ResetColumns: "重置可视列"
		},
		// new pages and tabs need to go here
        cluster: "集群",
        clusterMembership: "加入/脱离",
        adminEndpoint: "管理端点",
        firstserver: "连接到服务器",
        portInvalid: "端口号必须是 1 到 65535 范围内的一个数字。",
        connectionFailure: "无法连接到 ${IMA_PRODUCTNAME_FULL} 服务器。",
        clusterStatus: "状态",
        webui: "Web UI",
        licenseType_Devlopers: "开发人员",
        licenseType_NonProd: "非生产",
        licenseType_Prod: "生产",
        licenseType_Beta: "Beta 测试版"
});
