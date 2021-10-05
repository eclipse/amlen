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

	    title: "クラスター・メンバーシップ",
		tagline: "サーバーがクラスターに属するかどうかを制御します。 " +
				"クラスターとは、最大同時接続数または最大スループットのいずれかを単一のデバイスの能力を超えてスケーリングする目的で相互に接続された、ローカル高速 LAN 上のサーバーの集合のことです。 " +
				"クラスター内のサーバーは、共通トピック・ツリーを共有します。",

		// Status section
		statusTitle: "状況",
		statusNotConfigured: "サーバーは、クラスターに参加するように構成されていません。 " +
				"サーバーのクラスターへの参加を可能にするには、構成を編集して、サーバーをクラスターに追加します。",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "このサーバーは、クラスターのメンバーではありません。 " +
				"サーバーをクラスターに追加するには、<em>「サーバーを再始動してクラスター {0} に参加」</em> をクリックします。",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "このサーバーは、クラスター {0} のメンバーです。 クラスターからこのサーバーを削除するには、<em>「クラスターから離脱」</em> をクリックします。",		
		clusterMembershipLabel: "クラスター・メンバーシップ:",
		notConfigured: "未構成",
		notSet: "未設定",
		notAMember: "非メンバー",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "クラスター {0} のメンバー",

        // Cluster states  
        clusterStateLabel: "クラスターの状態:", 
        initializing: "初期化中",       
        discover: "ディスカバー",               
        active:  "アクティブ",                   
        removed: "削除済み",                 
        error: "エラー",                     
        inactive: "非アクティブ",               
        unavailable: "使用不可",         
        unknown: "不明",                 
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "サーバーを再始動してクラスター <em>{0}</em> に参加",
        leaveLink: "クラスターから離脱",
        // Configuration section

        configurationTitle: "構成",
		editLink: "編集",
	    resetLink: "リセット",
		configurationTagline: "クラスター構成を示します。 " +
				"構成の変更は、${IMA_PRODUCTNAME_SHORT} サーバーが再始動するまで有効になりません。",
		clusterNameLabel: "クラスター名:",
		controlAddressLabel: "制御アドレス",
		useMulticastDiscoveryLabel: "マルチキャスト・ディスカバリーの使用:",
		withMembersLabel: "ディスカバリー・サーバー・リスト:",
		multicastTTLLabel: "マルチキャスト・ディスカバリー TTL:",
		controlInterfaceSectionTitle: "制御インターフェース",
        messagingInterfaceSectionTitle: "メッセージング・インターフェース",
		addressLabel: "アドレス:",
		portLabel: "ポート:",
		externalAddressLabel: "外部アドレス:",
		externalPortLabel: "外部ポート:",
		useTLSLabel: "TLS の使用:",
        yes: "はい",
        no: "いいえ",
        discoveryPortLabel: "ディスカバリー・ポート:",
        discoveryTimeLabel: "ディスカバリー時間:",
        seconds: "{0} 秒", 

		editDialogTitle: "クラスター構成の編集",
        editDialogInstruction_NotAMember: "クラスター名を設定して、このサーバーが参加するクラスターを決定します。 " +
                "有効な構成を保管したら、クラスターに参加できます。",
        editDialogInstruction_Member: "クラスター構成を変更します。 " +
        		"このサーバーが属するクラスターの名前を変更するには、まず、{0} クラスターからこのサーバーを削除する必要があります。",
        saveButton: "保管",
        cancelButton: "キャンセル",
        advancedSettingsLabel: "高度な設定",
        advancedSettingsTagline: "以下の設定を構成することができます。 " +
                "これらの設定を変更する前に、${IMA_PRODUCTNAME_FULL} の資料で詳細な手順および推奨事項を確認してください。",
		// Tooltips

        clusterNameTooltip: "参加するクラスターの名前。",
        membersTooltip: "このサーバーが参加するクラスターに属するメンバーのサーバー URI のコンマ区切りリスト。 " +
        		"<em>「マルチキャスト・ディスカバリーの使用」</em> がチェックされていない場合は、少なくとも 1 つのサーバー URI を指定してください。 " +
        		"メンバーのサーバー URI は、そのメンバーの「クラスター・メンバーシップ」ページに表示されます。",
        useMulticastTooltip: "マルチキャストを使用して同じクラスター名のサーバーをディスカバーするかどうかを指定します。",
        multicastTTLTooltip: "マルチキャスト・ディスカバリーを使用する場合に、これは、マルチキャスト・トラフィックが通過可能なネットワーク・ホップの数を指定します。", 
        controlAddressTooltip: "制御インターフェースの IP アドレス。",
        controlPortTooltip: "制御インターフェースに使用するポート番号。  デフォルト・ポートは 9104 です。 ",
        controlExternalAddressTooltip: "制御アドレスと異なる場合の、制御インターフェースの外部 IP アドレスまたはホスト名。 " +
        		"外部アドレスは、他のメンバーがこのサーバーの制御インターフェースに接続するために使用されます。",
		controlExternalPortTooltip: "制御ポートと異なる場合の、制御インターフェースに使用する外部ポート。 "  +
                "外部ポートは、他のメンバーがこのサーバーの制御インターフェースに接続するために使用されます。",
        messagingAddressTooltip: "メッセージング・インターフェースの IP アドレス。",
        messagingPortTooltip: "メッセージング・インターフェースに使用するポート番号。  デフォルト・ポートは 9105 です。 ",
        messagingExternalAddressTooltip: "メッセージング・アドレスと異なる場合の、メッセージング・インターフェースの外部 IP アドレスまたはホスト名。 " +
                "外部アドレスは、他のメンバーがこのサーバーのメッセージング・インターフェースに接続するために使用されます。",
        messagingExternalPortTooltip: "メッセージング・ポートと異なる場合の、メッセージング・インターフェースに使用する外部ポート。 "  +
                "外部ポートは、他のメンバーがこのサーバーのメッセージング・インターフェースに接続するために使用されます。",
        discoveryPortTooltip: "マルチキャスト・ディスカバリーに使用するポート番号。 デフォルト・ポートは 9106 です。 " +
        		"クラスターのすべてのメンバーで、同じポートが使用されなければなりません。",
        discoveryTimeTooltip: "クラスター内の他のサーバーをディスカバーし、それらのサーバーから更新済み情報を入手するためにサーバー始動時に費やす時間 (秒)。 " +
                "有効範囲は 1 から 2147483647 です。 デフォルトは 10 です。 " + 
                "更新済み情報を入手するか、ディスカバリーがタイムアウトになるか、どちらか先に発生したほうの後、このサーバーはクラスターのアクティブ・メンバーになります。",
        controlInterfaceSectionTooltip: "制御インターフェースは、クラスターの構成、モニター、および制御に使用されます。",
        messagingInterfaceSectionTooltip: "メッセージング・インターフェースにより、クライアントからクラスターのメンバーにパブリッシュされたメッセージを、クラスターの他のメンバーに転送することができます。",
        // confirmation dialogs
        restartTitle: "${IMA_PRODUCTNAME_SHORT} の再始動",
        restartContent: "${IMA_PRODUCTNAME_SHORT} を再始動しますか?",
        restartButton: "再始動",

        resetTitle: "クラスター・メンバーシップのリセット",
        resetContent: "クラスター・メンバーシップ構成をリセットしますか?",
        resetButton: "リセット",

        leaveTitle: "クラスターから離脱",
        leaveContent: "クラスターから離脱しますか?",
        leaveButton: "離脱",      
        restartRequiredTitle: "再起動が必要",
        restartRequiredContent: "変更は保管されましたが、${IMA_PRODUCTNAME_SHORT} サーバーが再始動するまで有効になりません。<br /><br />" +
                 "${IMA_PRODUCTNAME_SHORT} サーバーをすぐに再始動する場合は、<b>「今すぐ再始動」</b>をクリックします。 そうでない場合は、<b>「後で再始動」</b>をクリックします。",
        restartLaterButton: "後で再始動",
        restartNowButton: "今すぐ再始動",

        // messages
        savingProgress: "保管中...",
        errorGettingClusterMembership: "クラスター・メンバーシップ情報の取得中にエラーが発生しました。",
        saveClusterMembershipError: "クラスター・メンバーシップ構成の保管中にエラーが発生しました。",
        restarting: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動中です...",
        restartingFailed: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動に失敗しました。",
        resetting: "構成をリセット中...",
        resettingFailed: "クラスター・メンバーシップ構成のリセットに失敗しました。",
        addressInvalid: "有効な IP アドレスが必要です。", 
        portInvalid: "ポート番号は、1 から 65535 までの数値でなければなりません。"

});
