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

		license : {
			heading : "许可协议",
			languageSelection: "许可证语言选择",
			intro : "单击下方的<q>我同意</q> 按钮，即表示您同意许可条款、非 IBM 条款以及保修信息的内容。" +
					"如果不同意，请单击<q>我不同意</q> 按钮。",
			introAccepted: "已接受许可条款、非 IBM 条款以及保修信息的内容。",
			nonibm : {
				link : "阅读软件非 IBM 条款",
				title : "软件非 IBM 条款"
			},
			notices : {
				link : "阅读软件声明",
				title : "软件声明"
			},
			laMachineCode : {
				link : "阅读 IBM 机器代码许可协议"
			},
			hardwareNonibm : {
				link : "阅读硬件非 IBM 条款"
			},
			hardwareNotices : {
				link : "阅读硬件声明"
			},
			warranty : {
				info : "有关保修信息，请参阅产品包装中的<em>保修信息</em>资料。"
			},
			accept : "我同意",
			decline : "我不同意",
			print : "打印许可",
			error : "错误：无法使用请求的语言加载许可。",
			acceptDialog : {
				title : "许可证已接受",
				content : "已接受该许可证。${IMA_PRODUCTNAME_FULL} 服务器正在启动。"
			},
			declineDialog : {
				title : "许可证已拒绝",
				content : "如果您不接受该许可协议、非 IBM 条款和保修信息，您应将产品退回给采购点并获得退款（如适用）。"
			},
			loading: "正在加载...",
			lang : {
				en : "英语 (en)",
				zh : "简体中文 (zh)",
				zh_TW : "繁体中文 (zh_TW)",
				cs : "捷克语 (cs)",
				fr : "法语 (fr)",
				de : "德语 (de)",
				el : "希腊语 (el)",
				in_ : "印度尼西亚语 (in)", // "in" is reserved
				it : "意大利语 (it)",
				ja : "日语 (ja)",
				ko : "韩语 (ko)",
				lt : "立陶宛语 (lt)",
				pl : "波兰语 (pl)",
				pt : "葡萄牙语 (pt)",
				ru : "俄语 (ru)",
				sl : "斯洛文尼亚语 (sl)",
				es : "西班牙语 (es)",
				tr : "土耳其语 (tr)"
			}
		},
		licenseNotAcceptedTitle: "尚未接受 ${IMA_PRODUCTNAME_FULL} 许可协议。",
		licenseNotAcceptedContent: "在系统管理员接受许可协议后，方可正常运行 ${IMA_PRODUCTNAME_FULL} 全部功能。"

});
