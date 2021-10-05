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
        resourcesSection: "Ressourcen angeben, die ein MQTT-Client verarbeiten darf:",
        allowDurable: "Clients mit permanenten Subskriptionen zulassen",
        allowDurableHelp: "Wenn diese Option ausgewählt ist, können MQTT-Clients eine Verbindung mit der Einstellung CleanSession=0 herstellen.",
        allowPersistentMessages: "Persistente Nachrichten zulassen",
        allowPersistentMessagesHelp: "Wenn diese Option ausgewählt ist, können MQTT-Clients Nachrichten mit der Servicequalität 1 oder 2 veröffentlichen.",
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "Erwartete Nachrichtenrate",
        expectedMessageRateHoverHelp: "Die erwartete Nachrichtenrate bestimmt die Speicherkapazität " +
            "und die Bandbreite, die jede Verbindung nutzen kann. Wenn Sie mit hohen Nachrichtenraten rechnen, " +
            "können Sie die Leistung verbessern, indem Sie die erwartete Nachrichtenrate auf Hoch oder Maximum setzen. Wenn " +
            "Sie mit geringen Nachrichtenraten rechnen und die Ressourcenauslastung niedrig halten möchten, können Sie die erwartete Nachrichtenrate auf Niedrig setzen.",
        lowRate: "Niedrig",
        defaultRate: "Standard",
        highRate: "Hoch",
        maxRate: "Maximum"
});
