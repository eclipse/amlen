/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
    	tlsSettings: "传输层安全性 (TLS) 设置",
    	clientAuthSettings: "客户机认证设置",
        tlsEnabled: "使用 TLS",
        tlsEnabledTooltip: "选择是否使用传输层安全性确保连接安全。",
        certprofileTooltipDisabled: "由于未选择<em>使用 TLS</em>，因此无法选择证书概要文件。",
        mprotocolTooltipDisabled: "由于未选择<em>使用 TLS</em>，因此无法选择最低协议级别。",
        ciphersTooltipDisabled: "由于未选择<em>使用 TLS</em>，因此无法选择密码安全设置。",
        certauthTooltipDisabled: "由于未选择<em>使用 TLS</em>，因此无法指定客户机证书认证。",
        clientciphersTooltipDisabled: "由于未选择<em>使用 TLS</em>，因此无法指定允许客户机确定密码套件。",
        noCertProfileSelected: "选择<em>使用 TLS</em> 时，必须指定证书概要文件。"
});
