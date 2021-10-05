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
	    configurationTitle: "管理端點",
	    editLink: "編輯",
	    configurationTagline_unsecured: "管理端點未受到保護。" +
	            "管理端點可讓您使用 Web UI 或 REST API 來管理伺服器。" +
	    		"若要保護端點安全，請配置安全設定檔，以及一個以上的配置原則。",
	    configurationTagline: "管理端點可讓您使用 Web UI 或 REST API 來管理伺服器。" +
	    		"請驗證有適當的安全設定檔和配置原則可供使用。",
	    editDialogTitle: "編輯管理端點",
	    editDialogInstruction: "Web UI 和 REST API 會連接到管理端點，以執行管理和監視作業。",
	    // property labels and hover helps
	    interfaceLabel:  "IP 位址：",
	    interfaceTooltip: "管理端點接聽的 IP 位址。",
	    portLabel: "埠：",
	    portTooltip: "管理端點接聽的埠。埠號必須在 1 到 65535（含）的範圍內。",
	    securityProfileLabel: "安全設定檔：",
	    securityProfileTooltip: "若要保護連線安全及定義如何執行鑑別，請選取一個安全設定檔。" +
	    		"現有的安全設定檔會併入清單中。" +
	    		"若要建立新的安全設定檔，請跳至「<em>安全設定</em>」頁面。",
	    configurationPoliciesLabel: "配置原則：",
	    configurationPoliciesTooltip: "若要保護管理端點安全，至少要新增一個配置原則，但不能超過 100 個。" +
	    		"配置原則會授權連線的用戶端執行特定的管理及監視作業。" +
	    		"配置原則依顯示的順序進行評估。若要變更順序，請使用工具列上的上移鍵與下移鍵。",
	    // configuration policy chooser dialog
	    addConfigPolicyDialogTitle: "將配置原則新增至管理端點",
	    addConfigPoliciesDialogInstruction: "配置原則會授權連線的用戶端執行特定的管理及監視作業。",
	    addLabel: "新增",
	    configurationPolicyLabel: "配置原則",
	    descriptionLabel: "說明",	
	    authorityLabel: "權限",
	    none: "無",
	    all: "All",
	    // buttons
	    saveButton: "儲存",
	    cancelButton: "取消",
	    add: "新增",
        // messages
        savingProgress: "儲存中…",
        getAdminEndpointError: "擷取管理端點配置時發生錯誤。",
        saveAdminEndpointError: "儲存管理端點配置時發生錯誤。",
        getSecurityProfilesError: "擷取安全設定檔時發生錯誤。",
        tooManyPolicies: "選取了太多原則。管理端點最多只能有 100 個配置原則。",
        certificateInvalidMessage: "Web UI 無法連接至 ${IMA_PRODUCTNAME_SHORT} 伺服器。" +
            "伺服器可能已停止，或是保護管理端點的憑證可能無效。" +
            "如果憑證無效，您必須新增異常狀況。" +
            "<a href='{0}' target='_blank'>請按一下這裡</a>，以導覽至另一個頁面，該頁面將提示您新增異常狀況（如果需要的話）。",
        certificateRefreshInstruction: "新增異常狀況之後，您可能需要重新整理此頁面。",
        certificatePreventError: "如需防止發生此問題的相關資訊，請參閱 IBM Knowledge Center 中的 ${IMA_PRODUCTNAME_SHORT} 文件。",
        // AdminUserID and AdminUserPassword
        superUserTitle: "管理超級使用者",
        superUserTagline: "管理超級使用者 ID 和密碼儲存在本端，以確保即使 LDAP 配置錯誤或無法使用，仍可管理 ${IMA_PRODUCTNAME_SHORT} 伺服器。" +
        		"管理超級使用者有權執行所有動作。" +
        		"請確定管理超級使用者 ID 和密碼不易猜到。",

        adminUserIDLabel: "使用者 ID︰",
        changeAdminUserIDLink: "變更使用者 ID",
        editAdminUserIdDialogTitle: "編輯管理超級使用者 ID",
        editAdminUserIdDialogInstruction: "將管理超級使用者 ID 變更為不易猜到的值。",
        saveAdminUserIDError: "儲存管理超級使用者 ID 時發生錯誤。",
        getAdminUserIDError: "擷取管理超級使用者 ID 時發生錯誤。",

        changeAdminUserPasswordLink: "變更密碼",
        editAdminUserPasswordDialogTitle: "編輯管理超級使用者密碼",
        editAdminUserPasswordDialogInstruction: "將管理超級使用者密碼變更為不易猜到的值。",
        adminUserPasswordLabel: "密碼",
        confirmPasswordLabel: "確認密碼",
        passwordInvalidMessage : "密碼不相符。",
        saveAdminUserPasswordError: "儲存管理超級使用者密碼發生錯誤。"            
});
