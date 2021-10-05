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
	    configurationTitle: "管理エンドポイント",
	    editLink: "編集",
	    configurationTagline_unsecured: "この管理エンドポイントは保護されていません。 " +
	            "管理エンドポイントにより、Web UI または REST API を使用してサーバーを管理することができます。 " +
	    		"このエンドポイントを保護するには、セキュリティー・プロファイルと 1 つ以上の構成ポリシーを構成してください。",
	    configurationTagline: "管理エンドポイントにより、Web UI または REST API を使用してサーバーを管理することができます。 " +
	    		"適切なセキュリティー・プロファイルと構成ポリシーが使用可能であることを確認してください。",
	    editDialogTitle: "管理エンドポイントの編集",
	    editDialogInstruction: "Web UI と REST API は、管理エンドポイントに接続して管理タスクおよびモニター・タスクを実行します。",
	    // property labels and hover helps
	    interfaceLabel:  "IP アドレス:",
	    interfaceTooltip: "管理エンドポイントが待機する IP アドレス。",
	    portLabel: "ポート:",
	    portTooltip: "管理エンドポイントが待機するポート。 ポート番号は 1 から 65535 までの範囲内でなければなりません。",
	    securityProfileLabel: "セキュリティー・プロファイル:",
	    securityProfileTooltip: "接続を保護して認証の実行方法を定義するには、セキュリティー・プロファイルを選択します。 " +
	    		"既存のセキュリティー・プロファイルがリストに含まれています。 " +
	    		"新しいセキュリティー・プロファイルを作成するには、<em>「セキュリティー設定」</em> ページに移動してください。",
	    configurationPoliciesLabel: "構成ポリシー:",
	    configurationPoliciesTooltip: "管理エンドポイントを保護するには、1 個以上 100 個以下の構成ポリシーを追加します。 " +
	    		"構成ポリシーは、特定の管理タスクおよびモニター・タスクを実行する権限を、接続済みクライアントに付与します。 " +
	    		"構成ポリシーは、示された順序で評価されます。 順序を変更するには、ツールバーの上矢印と下矢印を使用します。",
	    // configuration policy chooser dialog
	    addConfigPolicyDialogTitle: "管理エンドポイントへの構成ポリシーの追加",
	    addConfigPoliciesDialogInstruction: "構成ポリシーは、特定の管理タスクおよびモニター・タスクを実行する権限を、接続済みクライアントに付与します。",
	    addLabel: "追加",
	    configurationPolicyLabel: "構成ポリシー",
	    descriptionLabel: "説明",	
	    authorityLabel: "権限",
	    none: "なし",
	    all: "すべて",
	    // buttons
	    saveButton: "保管",
	    cancelButton: "キャンセル",
	    add: "追加",
        // messages
        savingProgress: "保管中...",
        getAdminEndpointError: "管理エンドポイント構成の取得中にエラーが発生しました。",
        saveAdminEndpointError: "管理エンドポイント構成の保管中にエラーが発生しました。",
        getSecurityProfilesError: "セキュリティー・プロファイルの取得中にエラーが発生しました。",
        tooManyPolicies: "選択されているポリシーが多すぎます。 管理エンドポイントが保有できる構成ポリシーは最大 100 個までです。",
        certificateInvalidMessage: "Web UI が ${IMA_PRODUCTNAME_SHORT} サーバーに接続できません。 " +
            "サーバーが停止しているか、管理エンドポイントを保護する証明書が無効である可能性があります。 " +
            "証明書が無効の場合は、例外を追加してください。 " +
            "<a href='{0}' target='_blank'>ここをクリック</a>して、例外が必要な場合に例外の追加を求めるプロンプトを出すページにナビゲートしてください。 ",
        certificateRefreshInstruction: "例外の追加後、このページの最新表示が必要になることがあります。 ",
        certificatePreventError: "この問題の回避に関する情報については、IBM Knowledge Center の ${IMA_PRODUCTNAME_SHORT} の資料を参照してください。",
        // AdminUserID and AdminUserPassword
        superUserTitle: "管理スーパーユーザー",
        superUserTagline: "LDAP の構成に誤りがあったり、LDAP が使用不可であったりしても ${IMA_PRODUCTNAME_SHORT} サーバーを管理できるように、管理スーパーユーザー ID とパスワードはローカルに保管されます。 " +
        		"管理スーパーユーザーには、すべてのアクションの実行権限があります。 " +
        		"管理スーパーユーザー ID とパスワードは、推測されにくい値にしてください。",

        adminUserIDLabel: "ユーザー ID:",
        changeAdminUserIDLink: "ユーザー ID 変更",
        editAdminUserIdDialogTitle: "管理スーパーユーザー ID の編集",
        editAdminUserIdDialogInstruction: "管理スーパーユーザー ID は、推測されにくい値に変更してください。",
        saveAdminUserIDError: "管理スーパーユーザー ID の保管中にエラーが発生しました。",
        getAdminUserIDError: "管理スーパーユーザー ID の取得中にエラーが発生しました。",

        changeAdminUserPasswordLink: "パスワード変更",
        editAdminUserPasswordDialogTitle: "管理スーパーユーザー・パスワードの編集",
        editAdminUserPasswordDialogInstruction: "管理スーパーユーザー・パスワードは、推測されにくい値に変更してください。",
        adminUserPasswordLabel: "パスワード",
        confirmPasswordLabel: "パスワードの確認",
        passwordInvalidMessage : "パスワードが一致しません",
        saveAdminUserPasswordError: "管理スーパーユーザー・パスワードの保管中にエラーが発生しました。"            
});
