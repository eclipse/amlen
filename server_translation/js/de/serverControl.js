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
    	serverMenuLabel: "Server:",
    	serverMenuHint: "Zu verwaltenden Server auswählen",
    	recentServersLabel: "Kürzlich verwendete Server:",
    	addRemoveLinkLabel: "Server hinzufügen oder entfernen",
    	addRemoveDialogTitle: "Server hinzufügen oder entfernen",
    	addRemoveDialogInstruction: "In diesem Dialog können Sie ${IMA_PRODUCTNAME_SHORT}-Server hinzufügen oder entfernen, die Sie mit dieser Webbenutzerschnittstelle verwalten können.",
    	serverName: "Servername",
    	serverNameTooltip: "Der Anzeigename für diesen Server.",
        adminAddress: "Verwaltungsadresse:",
        adminAddressTooltip: "Die IP-Adresse oder der Host, an der bzw. dem der Verwaltungsendpunkt des Servers empfangsbereit ist.",
        adminPort: "Verwaltungsport:",
        adminPortTooltip: "Der Port, an dem der Verwaltungsendpunkt des Servers empfangsbereit ist. Der gültige Portnummernbereich ist 1-65535 einschließlich.",
        sendWebUICredentials: "Berechtigungsnachweise für Webbenutzerschnittstelle senden:",
        sendWebUICredentialsTooltip: "Gibt an, ob die Webbenutzerschnittstelle die Benutzer-ID/Kennwort-Kombination, mit der Sie sich am Administratorendpunkt angemeldet haben, senden soll. " +
        		"Wenn der Administratorendpunkt eine gültige Benutzer-ID und ein gültiges Kennwort erfordert, sendet die Webbenutzerschnittstelle die Benutzer-ID/Kennwort-Kombination, mit der Sie sich am Administratorendpunkt angemeldet haben. " +
        		"Andernfalls werden Sie aufgefordert, die für den Zugriff auf den Administratorendpunkt benötigte Benutzer-ID/Kennwort-Kombination einzugeben. ",
        //cluster: "Cluster",
        description: "Beschreibung:",
        portInvalidMessage: "Der gültige Portnummernbereich ist 1-65535 einschließlich.",
        duplicateServerTitle: "Doppelter Server",
        duplicateServerMessage: "Der Server kann nicht hinzugefügt werden, weil bereits ein Server mit der angegebenen Adresse und dem angegebenen Port in der Liste vorhanden ist.",
        closeButton: "Schließen",
        saveButton: "Speichern",
        cancelButton: "Abbrechen",
        testButton: "Verbindung testen",
        YesButton: "Ja",
        deletingProgress: "Löschoperation wird durchgeführt...",
        removeConfirmationTitle: "Möchten Sie diesen Server wirklich aus der Liste der verwalteten Server entfernen?",
        addServerDialogTitle: "Server hinzufügen",
        editDialogTitle: "Server bearbeiten",
        removeServerDialogTitle: "Server entfernen",
        addServerDialogInstruction: "Fügen Sie einen ${IMA_PRODUCTNAME_SHORT}-Server hinzu, den Sie mit dieser Webbenutzerschnittstelle verwalten können." /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "Tests werden durchgeführt...",
        testingFailed: "Der Verbindungstest ist fehlgeschlagen.",
        testingSuccess: "Der Verbindungstest war erfolgreich."        
});
