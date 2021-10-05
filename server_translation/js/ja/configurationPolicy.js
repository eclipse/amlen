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
	    title:  "構成ポリシー",
	    tagline: "構成ポリシーでは、ユーザーがどの管理タスクの実行を許可されるかを制御します。 " +
	    		"構成ポリシーを有効にするためには、それを管理エンドポイントに追加する必要があります。",
	    // Add / Edit dialog
	    addDialogTitle: "構成ポリシーの追加",
	    editDialogTitle: "構成ポリシーの編集",
	    dialogInstruction: "構成ポリシーでは、ユーザーがどの管理タスクの実行を許可されるかを制御します。",
		nameLabel:  "名前:",
		nameColumnLabel:  "名前",
		descriptionLabel: "説明:",
		descriptionColumnLabel: "説明",
		authorityLabel: "権限:",
		authorityColumnLabel: "権限",
		authorityTooltip: "<dl><dt>構成:</dt><dd>サーバー構成を変更する権限を付与します。</dd>" +
		                      "<dt>表示:</dt><dd>サーバー構成とサーバー状況を表示する権限を付与します。</dd>" +
                              "<dt>モニター:</dt><dd>モニター・データを表示する権限を付与します。</dd>" +
                              "<dt>管理:</dt><dd>サーバー再始動などのサービス要求を発行する権限を付与します。</dd>" +
                          "</dl>",
		configure: "構成",
		view: "表示",
		monitor: "モニター",
		manage: "管理",
		listSeparator: ",",

	    dialogFilterInstruction: "このポリシーで定義された管理アクションまたはモニター・アクションを、特定のユーザーまたはユーザー・グループに制限するには、以下のフィルターを 1 つ以上指定します。 " +
            "例えば、このポリシーを特定のグループのメンバーに制限するには、グループ ID を選択します。 " +
            "このポリシーは、指定されたフィルターのすべてに当てはまる場合にのみアクセスが許可されます。",
        dialogFilterTooltip: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。 " +
            "ほとんどのフィルターでは、0 個以上の文字と一致する単一のアスタリスク (*) を最後の文字として使用することができます。 " +
            "クライアント IP アドレスは、アスタリスク (*) を含めることや、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストにすることができます (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
            "IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。",

	    clientIPLabel: "クライアント IP アドレス:",
	    clientIPColumnLabel: "クライアント IP アドレス",
	    userIdLabel: "ユーザー ID:",
	    userIdColumnLabel: "ユーザー ID",
	    groupIdLabel: "グループ ID:",
	    groupIdColumnLabel: "グループ ID",
	    commonNameLabel: "証明書共通名:",
	    commonNameColumnLabel: "証明書共通名",
	    invalidWildcard: "0 個以上の文字と一致する単一のアスタリスク (*) を最後の文字として使用することができます。",
	    invalidClientIP: "クライアント IP アドレスは、アスタリスク (*) を含めるか、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストにしなければなりません (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
            "IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。",
        invalidFilter: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。",
	    // buttons
	    saveButton: "保管",
	    cancelButton: "キャンセル",
	    closeButton: "閉じる",
        // messages
        savingProgress: "保管中...",
        updatingMessage: "更新中...",
        deletingProgress: "削除中...",
        noItemsGridMessage: "表示する項目がありません",
        getConfigPolicyError: "構成ポリシーの取得中にエラーが発生しました。",
        saveConfigPolicyError: "構成ポリシーの保管中にエラーが発生しました。",
        deleteConfigurationPolicyError: "構成ポリシーの削除中にエラーが発生しました。",
        // remove dialog
        removeDialogTitle: "構成ポリシーの削除",
        removeDialogContent: "この構成ポリシーを削除しますか?"
});
