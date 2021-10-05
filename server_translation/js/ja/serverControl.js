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
    	serverMenuLabel: "サーバー:",
    	serverMenuHint: "管理するサーバーを選択します",
    	recentServersLabel: "最近のサーバー:",
    	addRemoveLinkLabel: "サーバーの追加または削除",
    	addRemoveDialogTitle: "サーバーの追加または削除",
    	addRemoveDialogInstruction: "この Web UI を使用して管理できる ${IMA_PRODUCTNAME_SHORT} サーバーを追加または削除します。",
    	serverName: "サーバー名",
    	serverNameTooltip: "このサーバーの表示名。",
        adminAddress: "管理アドレス:",
        adminAddressTooltip: "サーバーの管理エンドポイントが待機する IP アドレスまたはホスト名。",
        adminPort: "管理ポート:",
        adminPortTooltip: "サーバーの管理エンドポイントが待機するポート。 ポート番号は 1 から 65535 までの範囲内でなければなりません。",
        sendWebUICredentials: "Web UI 資格情報の送信:",
        sendWebUICredentialsTooltip: "Web UI が、ログインに使用されたユーザー ID とパスワードを管理エンドポイントに送信するかどうか。 " +
        		"管理エンドポイントが有効なユーザー ID とパスワードを必要とする場合は、Web UI が、ログインに使用されたユーザー ID とパスワードを管理エンドポイントに送信します。 " +
        		"そうでない場合は、管理エンドポイントへのアクセスに必要なユーザー ID とパスワードの入力を求めるプロンプトが出されます。",
        //cluster: "Cluster",
        description: "説明:",
        portInvalidMessage: "ポート番号は 1 から 65535 までの範囲内でなければなりません。",
        duplicateServerTitle: "重複したサーバー",
        duplicateServerMessage: "指定されたアドレスとポートのサーバーがリストに既に存在するため、サーバーを追加できません。",
        closeButton: "閉じる",
        saveButton: "保管",
        cancelButton: "キャンセル",
        testButton: "接続のテスト",
        YesButton: "はい",
        deletingProgress: "削除中...",
        removeConfirmationTitle: "管理対象サーバーのリストからこのサーバーを削除しますか?",
        addServerDialogTitle: "サーバーの追加",
        editDialogTitle: "サーバーの編集",
        removeServerDialogTitle: "サーバーの削除",
        addServerDialogInstruction: "この Web UI を使用して管理できる ${IMA_PRODUCTNAME_SHORT} サーバーを追加します。  " /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "テスト中...",
        testingFailed: "テスト接続に失敗しました。",
        testingSuccess: "テスト接続に成功しました。"        
});
