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
        groupInfoKey: "グループ情報鍵",
        groupInfoKeyTooltip: "ユーザー情報 URL 応答からグループ情報を取得するために使用する鍵の名前。 " +
                "これを指定すると、${IMA_PRODUCTNAME_SHORT} は他のソースからはグループ情報を取得しません。",
        tokenSendMethodLabel: "トークン送信メソッド",
        tokenSendMethodTooltip: "アクセス・トークンを OAuth サーバーに送信する方法を決定します",
        tokenSendMethodURLParamOpt: "URL パラメーター",
        tokenSendMethodHTTPHeaderOpt: "HTTP ヘッダー",
        checkcertLabel: "サーバー証明書の確認",
        checkcertTooltip: "OAuth2 サーバー証明書を確認する方法を指定します。",
        checkcertTStoreOpt: "メッセージ・サーバーのトラストストアを使用",
        checkcertDisableOpt: "証明書の検証を無効化",
        checkcertPublicTOpt: "パブリック・トラストストアを使用",
});
