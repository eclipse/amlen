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
    root : {
    	tlsSettings: "Transport Layer Security (TLS) Settings",
    	clientAuthSettings: "Client Authentication Settings",
        tlsEnabled: "Use TLS",
        tlsEnabledTooltip: "Choose whether to secure connections with Transport Layer Security.",
        certprofileTooltipDisabled: "A certificate profile cannot be selected because <em>Use TLS</em> is not selected.",
        mprotocolTooltipDisabled: "A minimum protocol level cannot be selected because <em>Use TLS</em> is not selected.",
        ciphersTooltipDisabled: "A cipher security setting cannot be selected because <em>Use TLS</em> is not selected.",
        certauthTooltipDisabled: "Client certificate authentication cannot be specified because <em>Use TLS</em> is not selected.",
        clientciphersTooltipDisabled: "Allowing the client to determine the cipher suite cannot be specified because <em>Use TLS</em> is not selected.",
        noCertProfileSelected: "You must specify a certificate profile when <em>Use TLS</em> is selected."
    },
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
