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
	    // Page title and tagline

	    title: "叢集成員資格",
		tagline: "控制伺服器是否屬於叢集。" +
				"叢集是一組在本端高速 LAN 上彼此連接的伺服器，目標是要調整超越單一裝置功能的並行連線數上限或傳輸量上限。" +
				"叢集中的伺服器之間共用一般主題樹狀結構。",

		// Status section
		statusTitle: "狀態",
		statusNotConfigured: "伺服器未配置為參與叢集。" +
				"若要讓伺服器參與叢集，請編輯配置，然後將伺服器新增至叢集。",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "此伺服器不是叢集的成員。" +
				"若要將伺服器新增至叢集，請按一下「<em>重新啟動伺服器並加入叢集 {0}</em>」。",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "此伺服器為叢集 {0} 的成員。若要從叢集中移除此伺服器，請按一下「<em>離開叢集</em>」。",		
		clusterMembershipLabel: "叢集成員資格：",
		notConfigured: "未配置",
		notSet: "未設定",
		notAMember: "不是成員",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "叢集 {0} 的成員",

        // Cluster states  
        clusterStateLabel: "叢集狀態：", 
        initializing: "正在起始設定",       
        discover: "探索",               
        active:  "作用中",                   
        removed: "已移除",                 
        error: "錯誤",                     
        inactive: "非作用中",               
        unavailable: "無法使用",         
        unknown: "不明",                 
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "重新啟動伺服器並加入叢集 <em>{0}</em>",
        leaveLink: "離開叢集",
        // Configuration section

        configurationTitle: "配置",
		editLink: "編輯",
	    resetLink: "重設",
		configurationTagline: "顯示叢集配置。" +
				"在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前，對配置所做的變更不會生效。",
		clusterNameLabel: "叢集名稱：",
		controlAddressLabel: "控制位址",
		useMulticastDiscoveryLabel: "使用多重播送探索：",
		withMembersLabel: "探索伺服器清單：",
		multicastTTLLabel: "多重播送探索 TTL：",
		controlInterfaceSectionTitle: "控制介面",
        messagingInterfaceSectionTitle: "傳訊介面",
		addressLabel: "位址：",
		portLabel: "埠：",
		externalAddressLabel: "外部位址：",
		externalPortLabel: "外部埠：",
		useTLSLabel: "使用 TLS：",
        yes: "是",
        no: "否",
        discoveryPortLabel: "探索埠：",
        discoveryTimeLabel: "探索時間：",
        seconds: "{0} 秒", 

		editDialogTitle: "編輯叢集配置",
        editDialogInstruction_NotAMember: "設定叢集名稱，以決定此伺服器要加入的叢集。" +
                "儲存有效的配置之後，您可以加入叢集中。",
        editDialogInstruction_Member: "變更叢集配置。" +
        		"若要變更此伺服器所屬的叢集名稱，您必須先從 {0} 叢集中移除該伺服器。",
        saveButton: "儲存",
        cancelButton: "取消",
        advancedSettingsLabel: "進階設定",
        advancedSettingsTagline: "您可以配置下列設定。" +
                "在變更這些設定之前，請參閱 ${IMA_PRODUCTNAME_FULL} 文件以取得詳細指示和建議。",
		// Tooltips

        clusterNameTooltip: "要加入的叢集名稱。",
        membersTooltip: "屬於此伺服器想要加入之叢集的成員的伺服器 URI 清單（以逗點區隔）。" +
        		"如果未勾選「<em>使用多重播送探索</em>」，請至少指定一個伺服器 URI。" +
        		"成員的伺服器 URI 會顯示在該成員的「叢集成員資格」頁面上。",
        useMulticastTooltip: "指定是否應使用多重播送來探索具有相同叢集名稱的伺服器。",
        multicastTTLTooltip: "如果您使用多重播送探索，這會指定容許多重播送資料流量通過的網路中繼站數目。", 
        controlAddressTooltip: "控制介面的 IP 位址。",
        controlPortTooltip: "用於控制介面的埠號。預設埠為 9104。",
        controlExternalAddressTooltip: "此控制介面的外部 IP 位址或主機名稱（若不同於控制位址）。" +
        		"其他成員使用外部位址來連接到這個伺服器的控制介面。",
		controlExternalPortTooltip: "用於控制介面的外部埠（若不同於控制埠）。"  +
                "其他成員使用外部埠來連接到這個伺服器的控制介面。",
        messagingAddressTooltip: "傳訊介面的 IP 位址。",
        messagingPortTooltip: "用於傳訊介面的埠號。預設埠為 9105。",
        messagingExternalAddressTooltip: "傳訊介面的外部 IP 位址或主機名稱（若不同於傳訊位址）。" +
                "其他成員使用外部位址來連接到這個伺服器的傳訊介面。",
        messagingExternalPortTooltip: "用於傳訊介面的外部埠（若不同於傳訊埠）。"  +
                "其他成員使用外部埠來連接到這個伺服器的傳訊介面。",
        discoveryPortTooltip: "用於多重播送探索的埠號。預設埠為 9106。" +
        		"叢集的所有成員必須使用相同的埠。",
        discoveryTimeTooltip: "在伺服器啟動期間所花費的時間（以秒為單位），用來探索叢集中的其他伺服器，以及從中取得更新的資訊。" +
                "有效範圍為 1 - 2147483647。預設值是 10。" + 
                "在取得更新的資訊或發生探索逾時之後（以較早發生者為準），此伺服器會變成叢集的作用中成員。",
        controlInterfaceSectionTooltip: "此控制介面用於叢集的配置、監視及控制。",
        messagingInterfaceSectionTooltip: "此傳訊介面可讓已從用戶端發佈至叢集成員的訊息轉遞至該叢集的其他成員。",
        // confirmation dialogs
        restartTitle: "重新啟動 ${IMA_PRODUCTNAME_SHORT}",
        restartContent: "您確定要重新啟動 ${IMA_PRODUCTNAME_SHORT} 嗎？",
        restartButton: "重新啟動",

        resetTitle: "重設叢集成員資格",
        resetContent: "您確定要重設叢集成員資格配置嗎？",
        resetButton: "重設",

        leaveTitle: "離開叢集",
        leaveContent: "您確定要離開叢集嗎？",
        leaveButton: "離開",      
        restartRequiredTitle: "需要重新啟動",
        restartRequiredContent: "您的變更已儲存，但在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前不會生效。<br /><br />" +
                 "若要立即重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，請按一下<b>立即重新啟動</b>。否則，請按一下<b>稍後重新啟動</b>。",
        restartLaterButton: "稍後重新啟動",
        restartNowButton: "立即重新啟動",

        // messages
        savingProgress: "儲存中…",
        errorGettingClusterMembership: "擷取叢集成員資格資訊時發生錯誤。",
        saveClusterMembershipError: "儲存叢集成員資格配置時發生錯誤。",
        restarting: "正在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器...",
        restartingFailed: "無法重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器。",
        resetting: "正在重設配置...",
        resettingFailed: "無法重設叢集成員資格配置。",
        addressInvalid: "需要有效的 IP 位址。", 
        portInvalid: "埠號必須是介於 1 和 65535（含）之間的數字。"

});
