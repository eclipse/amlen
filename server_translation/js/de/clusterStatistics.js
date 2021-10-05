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
        // TRNOTE: {0} is replaced with the server name of the server currently managed in the Web UI.
        //         {1} is replaced with the name of the cluster where the current server is a member.
		detail_title: "Clusterstatus für {0} im Cluster {1}",
		title: "Clusterstatus",
		tagline: "Überwacht den Status von Cluster-Membern. Zeigt Statistiken zu den Messaging-Kanälen auf diesem Server an. Messaging-Kanäle " +
				"ermöglichen die Konnectivität mit fernen Cluster-Membern und Puffern Nachrichten für diese. Wenn gepufferte Nachrichten für inaktive Member " +
				"zu viele Ressourcen belegen, können Sie die inaktiven Member aus dem Cluster entfernen. ",
		chartTitle: "Durchsatzdiagramm",
		gridTitle: "Clusterstatusdaten",
		none: "Ohne",
		lastUpdated: "Letzte Aktualisierung",
		notAvailable: "Nicht zutreffend",
		cacheInterval: "Datenerfassungsintervall: 5 Sekunden",
		refreshingLabel: "Aktualisierung wird durchgeführt...",
		resumeUpdatesButton: "Aktualisierungen fortsetzen",
		updatesPaused: "Datenerfassung ist angehalten",
		serverNameLabel: "Servername",
		connStateLabel: "Verbindungsstatus:",
		healthLabel: "Serverstatus",
		haStatusLabel: "Hochverfügbarkeitsstatus:",
		updTimeLabel: "Letzte Statusaktualisierung",
		incomingMsgsLabel: "Eingehende Nachrichten/Sekunde:",
		outgoingMsgsLabel: "Abgehende Nachrichten/Sekunde:",
		bufferedMsgsLabel: "Gepufferte Nachrichten: ",
		discardedMsgsLabel: "Verworfene Nachrichten:",
		serverNameTooltip: "Servername",
		connStateTooltip: "Verbindungsstatus",
		healthTooltip: "Serverstatus",
		haStatusTooltip: "Hochverfügbarkeitsstatus",
		updTimeTooltip: "Letzte Statusaktualisierung",
		incomingMsgsTooltip: "Eingehende Nachrichten/Sekunde",
		outgoingMsgsTooltip: "Abgehende Nachrichten/Sekunde",
		bufferedMsgsTooltip: "Gepufferte Nachrichten",
		discardedMsgsTooltip: "Verworfene Nachrichten",
		statusUp: "Aktiv",
		statusDown: "Inaktiv",
		statusConnecting: "Verbindung wird hergestellt",
		healthUnknown: "Unbekannt",
		healthGood: "Gut",
		healthWarning: "Warnung",
		healthBad: "Fehler",
		haDisabled: "Inaktiviert",
		haSynchronized: "Synchronisiert",
		haUnsynchronized: "Nicht synchronisiert",
		haError: "Fehler",
		haUnknown: "Unbekannt",
		BufferedAssocType: "Details zu gepufferten Nachrichten",
		DiscardedAssocType: "Details zu verworfenen Nachrichten",
		channelTitle: "Kanal",
		channelTooltip: "Kanal",
		channelUnreliable: "Nicht zuverlässig",
		channelReliable: "Zuverlässig",
		bufferedBytesLabel: "Gepufferte Byte",
		bufferedBytesTooltip: "Gepufferte Byte",
		bufferedHWMLabel: "Obergrenze für gepufferte Nachrichten",
		bufferedHWMTooltip: "Obergrenze für gepufferte Nachrichten",
		addServerDialogInstruction: "Fügen Sie das Cluster-Member der Liste verwalteter Server hinzu und verwalten Sie den Cluster-Member-Server.",
		saveServerButton: "Speichern und verwalten",
		removeServerDialogTitle: "Server aus Cluster entfernen",
		removeConfirmationTitle: "Möchten Sie diesen Server wirklich aus dem Cluster entfernen?",
		removeServerError: "Beim Löschen des Servers aus dem Cluster ist ein Fehler aufgetreten.",

		removeNotAllowedTitle: "Entfernen nicht zulässig",
		removeNotAllowed: "Der Server kann nicht aus dem Cluster entfernt werden, weil der Server momentan aktiv ist. " +
		                  "Verwenden Sie die Aktion Entfernen, um nur die Server zu entfernen, die nicht aktiv sind. ",

		cancelButton: "Abbrechen",
		yesButton: "Ja",
		closeButton: "Schließen"
});
