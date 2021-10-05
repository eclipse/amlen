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
        resourcesSection: "Specify the resources that an MQTT client is permitted to consume:",
        allowDurable: "Allow Clients with Durable Subscriptions",
        allowDurableHelp: "If checked, MQTT clients can connect with CleanSession 0.",
        allowPersistentMessages: "Allow Persistent Messages",
        allowPersistentMessagesHelp: "If checked, MQTT clients can publish messages with a QoS of 1 or 2.",
        
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "Expected Message Rate",
        expectedMessageRateHoverHelp: "The expected message rate determines the amount of memory " +
            "and bandwidth that each connection can use.  If you expect high message rates, you " +
            "can improve performance by setting expected message rate to high or maximum.  If you " +
            "expect low message rates and want to keep resource utilization low, you can set expected message rate to low.",
        lowRate: "Low",
        defaultRate: "Default",
        highRate: "High",
        maxRate: "Maximum"
    },
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
