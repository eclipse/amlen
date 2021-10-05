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
			heading : "授權合約",
			languageSelection: "授權語言選擇",
			intro : "按下面的<q>我同意</q> 按鈕，即表示您同意授權條款、非 IBM 條款及保固資訊。" +
					"如果您不同意，請按一下<q>我不同意</q> 按鈕。",
			introAccepted: "已接受授權條款、非 IBM 條款及保固資訊。",
			nonibm : {
				link : "閱讀軟體非 IBM 條款",
				title : "軟體非 IBM 條款"
			},
			notices : {
				link : "閱讀軟體注意事項",
				title : "軟體注意事項"
			},
			laMachineCode : {
				link : "閱讀 IBM 機器碼授權合約"
			},
			hardwareNonibm : {
				link : "閱讀硬體非 IBM 條款"
			},
			hardwareNotices : {
				link : "閱讀硬體注意事項"
			},
			warranty : {
				info : "如需保固資訊，請參閱在您的產品包裝中提供的「<em>保固資訊</em>」出版品。"
			},
			accept : "我同意",
			decline : "我不同意",
			print : "列印授權",
			error : "錯誤：授權無法以所要求的語言載入。",
			acceptDialog : {
				title : "已接受授權",
				content : "已接受授權。${IMA_PRODUCTNAME_FULL} 伺服器正在啟動。"
			},
			declineDialog : {
				title : "已拒絕授權",
				content : "如果您不接受授權合約、非 IBM 條款及保固資訊，就應該將產品退回到購買點，並要回退款（如果適用的話）。"
			},
			loading: "正在載入...",
			lang : {
				en : "英文 (en)",
				zh : "簡體中文 (zh)",
				zh_TW : "繁體中文 (zh_TW)",
				cs : "捷克文 (cs)",
				fr : "法文 (fr)",
				de : "德文 (de)",
				el : "希臘文 (el)",
				in_ : "印尼文 (in)", // "in" is reserved
				it : "義大利文 (it)",
				ja : "日文 (ja)",
				ko : "韓文 (ko)",
				lt : "立陶宛文 (lt)",
				pl : "波蘭文 (pl)",
				pt : "葡萄牙文 (pt)",
				ru : "俄文 (ru)",
				sl : "斯洛維尼亞文 (sl)",
				es : "西班牙文 (es)",
				tr : "土耳其文 (tr)"
			}
		},
		licenseNotAcceptedTitle: "尚未接受 ${IMA_PRODUCTNAME_FULL} 授權合約。",
		licenseNotAcceptedContent: "在系統管理者接受授權合約之前，${IMA_PRODUCTNAME_FULL} 不會完全運作。"

});
