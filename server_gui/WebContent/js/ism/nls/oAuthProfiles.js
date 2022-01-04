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
    root : {
        groupInfoKey: "Group Info Key",
        groupInfoKeyTooltip: "The name of the key that is used to retrieve the group information from the user info URL response. " +
                "If specified, ${IMA_PRODUCTNAME_SHORT} does not retrieve group information from any other source.",
        tokenSendMethodLabel: "Token Send Method",
        tokenSendMethodTooltip: "Determine how the access token is transmitted to the OAuth server",
        tokenSendMethodURLParamOpt: "URL Parameter",
        tokenSendMethodHTTPHeaderOpt: "HTTP Header",
        tokenSendMethodHTTPPostOpt: "HTTP Post",
        checkcertLabel: "Check Server Certificate",
        checkcertTooltip: "Specify how to check the OAuth2 server certificate.",
        checkcertTStoreOpt: "Use messaging server trust store",
        checkcertDisableOpt: "Disable certificate verification",
        checkcertPublicTOpt: "Use public trust store",
    },
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
