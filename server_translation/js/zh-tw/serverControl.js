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
    	serverMenuLabel: "伺服器：",
    	serverMenuHint: "選取要管理的伺服器",
    	recentServersLabel: "最近的伺服器：",
    	addRemoveLinkLabel: "新增或移除伺服器",
    	addRemoveDialogTitle: "新增或移除伺服器",
    	addRemoveDialogInstruction: "新增或移除您可以使用這個 Web UI 管理的 ${IMA_PRODUCTNAME_SHORT} 伺服器。",
    	serverName: "伺服器名稱",
    	serverNameTooltip: "此伺服器的顯示名稱。",
        adminAddress: "管理位址：",
        adminAddressTooltip: "伺服器的管理端點所接聽的 IP 位址或主機名稱。",
        adminPort: "管理埠：",
        adminPortTooltip: "伺服器的管理端點所接聽的埠。埠號必須在 1 到 65535（含）的範圍內。",
        sendWebUICredentials: "傳送 Web UI 認證：",
        sendWebUICredentialsTooltip: "Web UI 是否應該將您用來登入的使用者 ID 和密碼傳送至管理端點。" +
        		"如果管理端點需要有效的使用者 ID 和密碼，則 Web UI 會將您用來登入的使用者 ID 和密碼傳送至管理端點。" +
        		"否則，系統會提示您輸入存取此管理端點所需的使用者 ID 和密碼。",
        //cluster: "Cluster",
        description: "說明：",
        portInvalidMessage: "埠號必須在 1 到 65535（含）的範圍內。",
        duplicateServerTitle: "伺服器重複",
        duplicateServerMessage: "無法新增伺服器，因為具有指定位址及埠的伺服器已在此清單中。",
        closeButton: "關閉",
        saveButton: "儲存",
        cancelButton: "取消",
        testButton: "測試連線",
        YesButton: "是",
        deletingProgress: "刪除中...",
        removeConfirmationTitle: "您確定要從受管理伺服器的清單中移除此伺服器嗎？",
        addServerDialogTitle: "新增伺服器",
        editDialogTitle: "編輯伺服器",
        removeServerDialogTitle: "移除伺服器",
        addServerDialogInstruction: "新增您可以使用這個 Web UI 管理的 ${IMA_PRODUCTNAME_SHORT} 伺服器。" /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "測試中...",
        testingFailed: "測試連線失敗。",
        testingSuccess: "測試連線成功。"        
});
