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
	    title:  "Konfigurationsrichtlinien",
	    tagline: "Eine Konfigurationsrichtlinie steuert, welche Verwaltungstasks ein Benutzer ausführen darf. " +
	    		"Konfigurationsrichtlinien müssen dem Administratorendpunkt hinzugefügt werden, damit sie gültig werden. ",
	    // Add / Edit dialog
	    addDialogTitle: "Konfigurationsrichtlinie hinzufügen",
	    editDialogTitle: "Konfigurationsrichtlinie bearbeiten",
	    dialogInstruction: "Eine Konfigurationsrichtlinie steuert, welche Verwaltungstasks ein Benutzer ausführen darf. ",
		nameLabel:  "Name:",
		nameColumnLabel:  "Name",
		descriptionLabel: "Beschreibung:",
		descriptionColumnLabel: "Beschreibung",
		authorityLabel: "Berechtigung:",
		authorityColumnLabel: "Berechtigung",
		authorityTooltip: "<dl><dt>Konfigurieren: </dt><dd>Erteilt die Berechtigung zum Ändern der Serverkonfiguration.</dd>" +
		                      "<dt>Anzeigen:</dt><dd>Erteilt die Berechtigung zum Anzeigen der Serverkonfiguration und des Serverstatus.</dd>" +
                              "<dt>Überwachen: </dt><dd>Erteilt die Berechtigung zum Anzeigen von Überwachungsdaten.</dd>" +
                              "<dt>Verwalten:</dt><dd>Erteilt die Berechtigung zum Ausstellen von Serviceanfragen, z. B. für einen Neustart des Servers.</dd>" +
                          "</dl>",
		configure: "Konfigurieren",
		view: "Anzeigen",
		monitor: "Überwachen",
		manage: "Verwalten",
		listSeparator: ",",

	    dialogFilterInstruction: "Zum Beschränken der in dieser Richtlinie definierten Verwaltungs- und Überwachungsaktionen auf bestimmte Benutzer und Benutzergruppen geben Sie einen oder mehrere der folgenden Filter an." +
            "Wählen Sie beispielsweise Gruppen-ID aus, um diese Richtlinie auf Mitglieder einer bestimmten Gruppe zu beschränken. " +
            "Die Richtlinie lässt den Zugriff nur zu, wenn alle angegebenen Filter mit wahr ausgewertet werden:",
        dialogFilterTooltip: "Sie müssen mindestens eines der Filterfelder angeben. " +
            "Ein einzelner Stern (*) kann in den meisten Filtern als Platzhalter für 0 oder mehr Zeichen als letztes Zeichen verwendet werden. " +
            "Die Client-IP-Adresse muss einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::].",

	    clientIPLabel: "Client-IP-Adresse:",
	    clientIPColumnLabel: "Client-IP-Adresse",
	    userIdLabel: "Benutzer-ID:",
	    userIdColumnLabel: "Benutzer-ID",
	    groupIdLabel: "Gruppen-ID:",
	    groupIdColumnLabel: "Gruppen-ID",
	    commonNameLabel: "Allgemeiner Name des Zertifikats:",
	    commonNameColumnLabel: "Allgemeiner Name des Zertifikats",
	    invalidWildcard: "Ein einzelner Stern (*) kann als Platzhalter für 0 oder mehr Zeichen als letztes Zeichen verwendet werden. ",
	    invalidClientIP: "Die Client-IP-Adresse kann einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::].",
        invalidFilter: "Sie müssen mindestens eines der Filterfelder angeben. ",
	    // buttons
	    saveButton: "Speichern",
	    cancelButton: "Abbrechen",
	    closeButton: "Schließen",
        // messages
        savingProgress: "Speicheroperation wird durchgeführt...",
        updatingMessage: "Aktualisierung wird durchgeführt...",
        deletingProgress: "Löschoperation wird durchgeführt...",
        noItemsGridMessage: "Es sind keine anzuzeigenden Elemente vorhanden.",
        getConfigPolicyError: "Beim Abrufen der Konfigurationsrichtlinien ist ein Fehler aufgetreten. ",
        saveConfigPolicyError: "Beim Speichern der Konfigurationsrichtlinie ist ein Fehler aufgetreten.",
        deleteConfigurationPolicyError: "Beim Löschen der Konfigurationsrichtlinie ist ein Fehler aufgetreten.",
        // remove dialog
        removeDialogTitle: "Konfigurationsrichtlinie löschen",
        removeDialogContent: "Möchten Sie diese Konfigurationsrichtlinie wirklich löschen?"
});
