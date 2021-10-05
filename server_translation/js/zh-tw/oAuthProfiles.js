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
        groupInfoKey: "群組資訊金鑰",
        groupInfoKeyTooltip: "用於從使用者資訊 URL 回應擷取群組資訊的金鑰名稱。" +
                "如果已指定，${IMA_PRODUCTNAME_SHORT} 就不會從任何其他來源擷取群組資訊。",
        tokenSendMethodLabel: "記號傳送方法",
        tokenSendMethodTooltip: "判斷如何將存取記號傳輸至 OAuth 伺服器",
        tokenSendMethodURLParamOpt: "URL 參數",
        tokenSendMethodHTTPHeaderOpt: "HTTP 標頭",
        checkcertLabel: "檢查伺服器憑證",
        checkcertTooltip: "指定如何檢查 OAuth2 伺服器憑證。",
        checkcertTStoreOpt: "使用傳訊伺服器信任儲存庫",
        checkcertDisableOpt: "停用憑證驗證",
        checkcertPublicTOpt: "使用公用信任儲存庫",
});
