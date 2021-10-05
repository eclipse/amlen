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
        resourcesSection: "指定允许 MQTT 客户机使用的资源：",
        allowDurable: "允许带有持久预订的客户机",
        allowDurableHelp: "如果选中，那么 MQTT 客户机可使用 CleanSession 0 进行连接。",
        allowPersistentMessages: "允许持久消息",
        allowPersistentMessagesHelp: "如果选中，那么 MQTT 客户机可使用 QoS 为 1 或 2 的设置发布消息。",
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "期望的消息速率",
        expectedMessageRateHoverHelp: "期望的消息速率决定每个连接可以使用的内存量" +
            "和带宽。如果您期望实现较高的消息速率，" +
            "可以通过将期望的消息速率设置为较高的值或最大值来提高性能。如果您" +
            "期望较低的消息速率并且希望保持低资源利用率，可以将期望的消息速率设置为较低的值。",
        lowRate: "低级",
        defaultRate: "缺省",
        highRate: "高级",
        maxRate: "最大值"
});
