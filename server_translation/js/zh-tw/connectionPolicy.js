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
        resourcesSection: "指定允許 MQTT 用戶端耗用的資源：",
        allowDurable: "容許用戶端具有可延續訂閱",
        allowDurableHelp: "如果已勾選，MQTT 用戶端即可使用 CleanSession 0 進行連接。",
        allowPersistentMessages: "容許持續訊息",
        allowPersistentMessagesHelp: "如果已勾選，MQTT 用戶端即可發佈 QoS 為 1 或 2 的訊息。",
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "預期的訊息比率",
        expectedMessageRateHoverHelp: "預期的訊息比率決定每個連線可以使用的記憶體數量" +
            "及頻寬。如果預期較高的訊息比率，則可以改善效能，方式是將預期的" +
            "訊息比率設為較大的值或上限值。如果您預期較低的訊息比率，" +
            "而且想要保持較低的資源使用率，則可以將預期的訊息比率設為較低值。",
        lowRate: "低",
        defaultRate: "預設值",
        highRate: "高",
        maxRate: "上限"
});
