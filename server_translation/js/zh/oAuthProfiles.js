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
        groupInfoKey: "组信息键",
        groupInfoKeyTooltip: "用于从用户信息 URL 响应中检索组信息的键的名称。" +
                "如果已指定此项，那么 ${IMA_PRODUCTNAME_SHORT} 不会从任何其他源中检索组信息。",
        tokenSendMethodLabel: "令牌发送方法",
        tokenSendMethodTooltip: "确定如何将访问令牌传输至 OAuth 服务器",
        tokenSendMethodURLParamOpt: "URL 参数",
        tokenSendMethodHTTPHeaderOpt: "HTTP 头",
        checkcertLabel: "检查服务器证书",
        checkcertTooltip: "指定如何检查 OAuth2 服务器证书。",
        checkcertTStoreOpt: "使用消息传递服务器信任库",
        checkcertDisableOpt: "禁用证书验证",
        checkcertPublicTOpt: "使用公用信任库",
});
