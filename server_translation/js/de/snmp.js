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
        all: "Alle",
        unavailableAddressesTitle: "Einige Agentenadressen sind nicht verfügbar.",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "SNMP-Agentenadressen müssen gültige, aktive Ethernet-Schnittstellenadressen sein. " +
        		"Die folgenden Agentenadressen sind zwar konfiguriert, aber nicht verfügbar: {0}",
        agentAddressInvalidMessage: "Wählen Sie <em>Alle</em> oder mindestens eine IP-Adresse aus.",
        notRunningError: "SNMP ist aktiviert, aber nicht aktiv.",
        snmpRestartFailureMessageTitle: "Der SNMP-Service konnte nicht erneut gestartet werden.",
        statusLabel: "Status",
        running: "Aktiv",
        stopped: "Gestoppt",
        unknown: "Unbekannt",
        title: "SNMP-Service",
		tagLine: "Einstellungen und Aktionen, die sich auf den SNMP-Service auswirken.",
    	enableLabel: "SNMP aktivieren",
    	stopDialogTitle: "SNMP-Service stoppen",
    	stopDialogContent: "Möchten Sie den SNMP-Service wirklich stoppen? Das Stoppen des Service kann zu einem Verlust von SNMP-Nachrichten führen.",
    	stopDialogOkButton: "Stoppen",
    	stopDialogCancelButton: "Abbrechen",
    	stopping: "Stoppoperation wird ausgeführt",
    	stopError: "Beim Stoppen des SNMP-Service ist ein Fehler aufgetreten.",
    	starting: "Startoperation wird ausgeführt",
    	started: "Aktiv",
    	savingProgress: "Speicheroperation wird durchgeführt...",
    	setSnmpEnabledError: "Beim Festlegen des SNMP-Aktivierungsstatus ist ein Fehler aufgetreten.",
    	startError: "Beim Starten des SNMP-Service ist ein Fehler aufgetreten.",
    	startLink: "Service starten",
    	stopLink: "Service stoppen",
		savingProgress: "Speicheroperation wird durchgeführt..."
});
