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
	    title:  "配置原則",
	    tagline: "配置原則控制允許使用者執行哪些管理作業。" +
	    		"配置原則必須新增至管理端點才能生效。",
	    // Add / Edit dialog
	    addDialogTitle: "新增配置原則",
	    editDialogTitle: "編輯配置原則",
	    dialogInstruction: "配置原則控制允許使用者執行哪些管理作業。",
		nameLabel:  "名稱：",
		nameColumnLabel:  "名稱",
		descriptionLabel: "說明：",
		descriptionColumnLabel: "說明",
		authorityLabel: "權限：",
		authorityColumnLabel: "權限",
		authorityTooltip: "<dl><dt>配置：</dt><dd>授與權限來修改伺服器配置。</dd>" +
		                      "<dt>檢視：</dt><dd>授與權限來檢視伺服器配置及狀態。</dd>" +
                              "<dt>監視：</dt><dd>授與權限來檢視監視資料。</dd>" +
                              "<dt>管理：</dt><dd>授與權限來發出服務要求，例如重新啟動伺服器。</dd>" +
                          "</dl>",
		configure: "配置",
		view: "檢視",
		monitor: "監視",
		manage: "管理",
		listSeparator: ",",

	    dialogFilterInstruction: "若要將此原則中所定義的管理或監視動作限制為只有特定的使用者或使用者群組才能使用，請指定一個以上的下列過濾器。" +
            "例如，選取「群組 ID」，將此原則限制為只有特定群組的成員才能使用。" +
            "此原則僅在所有指定過濾器皆為 True 時，才會容許存取：",
        dialogFilterTooltip: "您必須至少指定一個過濾器欄位。" +
            "您可以使用單一星號 (*) 作為大部分過濾器的最後一個字元，其符合零個以上的字元。" +
            "用戶端 IP 位址可以包含星號 (*)，或是以逗點分隔的 IPv4 或 IPv6 位址或範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
            "IPv6 位址必須用方括弧括住，例如：[2001:db8::]。",

	    clientIPLabel: "用戶端 IP 位址：",
	    clientIPColumnLabel: "用戶端 IP 位址",
	    userIdLabel: "使用者 ID︰",
	    userIdColumnLabel: "使用者 ID",
	    groupIdLabel: "群組 ID：",
	    groupIdColumnLabel: "群組 ID",
	    commonNameLabel: "憑證通用名稱：",
	    commonNameColumnLabel: "憑證通用名稱",
	    invalidWildcard: "您可以使用單一星號 (*) 作為最後一個字元，其符合零個以上的字元。",
	    invalidClientIP: "用戶端 IP 位址必須包含星號 (*)，或是以逗點分隔的 IPv4 或 IPv6 位址或範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
            "IPv6 位址必須用方括弧括住，例如：[2001:db8::]。",
        invalidFilter: "您必須至少指定一個過濾器欄位。",
	    // buttons
	    saveButton: "儲存",
	    cancelButton: "取消",
	    closeButton: "關閉",
        // messages
        savingProgress: "儲存中…",
        updatingMessage: "更新中...",
        deletingProgress: "刪除中...",
        noItemsGridMessage: "無項目可顯示",
        getConfigPolicyError: "擷取配置原則時發生錯誤。",
        saveConfigPolicyError: "儲存配置原則時發生錯誤。",
        deleteConfigurationPolicyError: "刪除配置原則時發生錯誤。",
        // remove dialog
        removeDialogTitle: "刪除配置原則",
        removeDialogContent: "您確定要刪除這項配置原則嗎？"
});
