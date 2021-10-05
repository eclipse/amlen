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
	    // Page title and tagline

	    title: "Clusterzugehörigkeit",
		tagline: "Steuert, ob der Server zu einem Cluster gehört." +
				"Ein Cluster ist ein Verbund von Servern, die in einem lokalen Hochgeschwindigkeitsnetz (LAN) miteinander verbunden werden, um die maximale Anzahl gleichzeitiger Verbindungen oder den maximalen Durchsatz über die Kapazität einer einzigen Einheit hinaus skalieren zu können." +
				"Server in einem Cluster haben eine gemeinsame Topicbaumstruktur.",

		// Status section
		statusTitle: "Status",
		statusNotConfigured: "Der Server ist nicht für den Einsatz in einem Cluster konfiguriert. " +
				"Damit der Server in einen Cluster eingebunden werden kann, müssen Sie die Konfiguration bearbeiten und den Server dann den Cluster hinzufügen.",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "Dieser Server gehört nicht zu einem Cluster. " +
				"Wenn Sie den Server dem Cluster hinzufügen möchten, klicken Sie auf <em>Server erneut starten und in Cluster {0} einbinden</em>.",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "Dieser Server gehört zum Cluster {0}. Wenn Sie den Server aus dem Cluster entfernen möchten, klicken Sie auf <em>Cluster verlassen</em>.",		
		clusterMembershipLabel: "Clusterzugehörigkeit:",
		notConfigured: "Nicht konfiguriert",
		notSet: "Nicht definiert",
		notAMember: "Kein Member",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "Member von Cluster {0}",

        // Cluster states  
        clusterStateLabel: "Clusterstatus:", 
        initializing: "Initialisierung wird durchgeführt",       
        discover: "Erkennen",               
        active:  "Aktiv",                   
        removed: "Entfernt",                 
        error: "Fehler",                     
        inactive: "Inaktiv",               
        unavailable: "Nicht verfügbar",         
        unknown: "Unbekannt",                 
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "Server erneut starten und in Cluster <em>{0}</em> einbinden",
        leaveLink: "Cluster verlassen",
        // Configuration section

        configurationTitle: "Konfiguration",
		editLink: "Bearbeiten",
	    resetLink: "Zurücksetzen",
		configurationTagline: "Die Clusterkonfiguration wird angezeigt. " +
				"Änderungen an der Konfiguration werden erst nach einem Neustart des ${IMA_PRODUCTNAME_SHORT}-Server wirksam.",
		clusterNameLabel: "Clustername:",
		controlAddressLabel: "Steueradresse",
		useMulticastDiscoveryLabel: "Multicasterkennung verwenden:",
		withMembersLabel: "Erkennungsserverliste:",
		multicastTTLLabel: "Lebensdauer der Multicasterkennung:",
		controlInterfaceSectionTitle: "Steuerschnittstelle",
        messagingInterfaceSectionTitle: "Messaging-Schnittstelle",
		addressLabel: "Adresse:",
		portLabel: "Port:",
		externalAddressLabel: "Externe Adresse:",
		externalPortLabel: "Externer Port:",
		useTLSLabel: "TLS verwenden:",
        yes: "Ja",
        no: "Nein",
        discoveryPortLabel: "Erkennungsport:",
        discoveryTimeLabel: "Erkennungszeit:",
        seconds: "{0} Sekunden", 

		editDialogTitle: "Clusterkonfiguration bearbeiten",
        editDialogInstruction_NotAMember: "Legen Sie den Namen des Clusters fest, in den dieser Server eingebunden werden soll. " +
                "Nachdem Sie eine gültige Konfiguration gespeichert haben, kann die Einbindung in den Cluster erfolgen.",
        editDialogInstruction_Member: "Clusterkonfiguration ändern" +
        		"Um den Namen des Clusters zu ändern, zu dem dieser Server gehört, müssen Sie den Server zuerst aus dem Cluster {0} entfernen.",
        saveButton: "Speichern",
        cancelButton: "Abbrechen",
        advancedSettingsLabel: "Erweiterte Einstellungen",
        advancedSettingsTagline: "Sie können die folgenden Einstellungen konfigurieren. " +
                "Ausführliche Anweisungen und Empfehlungen zum Ändern dieser Einstellungen finden Sie in der Dokumentation zu ${IMA_PRODUCTNAME_FULL}.",
		// Tooltips

        clusterNameTooltip: "Der Name des Clusters, in den der Server eingebunden werden soll. ",
        membersTooltip: "Eine durch Kommas begrenzte Liste mit den Server-URIs der Member, die zu dem Cluster gehören, in den dieser Server eingebunden werden soll. " +
        		"Wenn <em>Multicasterkennung verwenden</em> nicht ausgewählt ist, geben Sie mindestens einen Server-URI an. " +
        		"Der Server-URI für ein Member wird auf der Seite Clusterzugehörigkeit dieses Members angezeigt. ",
        useMulticastTooltip: "Geben sie an, ob Multicasting für die Erkennung von Servern mit demselben Clusternamen verwendet werden soll.",
        multicastTTLTooltip: "Gibt bei der Verwendung der Multicasterkennung die Anzahl der Netzhops an, über die Multicastverkehr übertragen werden kann.", 
        controlAddressTooltip: "Die IP-Adresse der Steuerschnittstelle. ",
        controlPortTooltip: "Die Portnummer für die Steuerschnittstelle. Der Standardport ist 9104. ",
        controlExternalAddressTooltip: "Die externe IP-Adresse oder der Hostname der Steuerschnittstelle, sofern diese bzw. dieser von der Steueradresse abweicht." +
        		"Die externe Adresse wird von anderen Membern verwendet, um eine Verbindung zur Steuerschnittstelle dieses Servers herzustellen. ",
		controlExternalPortTooltip: "Der externe Port für die Steuerschnittstelle, falls dieser vom Steuerport abweicht. "  +
                "Der externe Port wird von anderen Membern verwendet, um eine Verbindung zur Steuerschnittstelle dieses Servers herzustellen. ",
        messagingAddressTooltip: "Die IP-Adresse der Messaging-Schnittstelle. ",
        messagingPortTooltip: "Die Portnummer für die Messaging-Schnittstelle. Der Standardport ist 9105. ",
        messagingExternalAddressTooltip: "Die externe IP-Adresse oder der Hostname der Messaging-Schnittstelle, falls diese bzw. dieser von der Messaging-Adresse abweicht. " +
                "Die externe Adresse wird von anderen Membern verwendet, um eine Verbindung zur Messaging-Schnittstelle dieses Servers herzustellen. ",
        messagingExternalPortTooltip: "Der externe Port für die Messaging-Schnittstelle, falls dieser vom Messaging-Port abweicht. "  +
                "Der externe Port wird von anderen Membern verwendet, um eine Verbindung zur Messaging-Schnittstelle dieses Servers herzustellen. ",
        discoveryPortTooltip: "Die Portnummer für die Multicasterkennung. Der Standardport ist 9106. " +
        		"Auf allen Membern des Clusters muss derselbe Port verwendet werden.",
        discoveryTimeTooltip: "Die Zeit (in Sekunden), die während des Serverstarts für die Erkennung anderer Server im Cluster und für den Abruf aktualisierter Informationen von diesen Servern aufgebracht wird." +
                "Der gültige Bereich ist 1-2147483647. Der Standardwert ist 10. " + 
                "Dieser Server wird nach Erhalt der aktualisierten Informationen bzw. Ablauf des Erkennungszeitlimits (je nachdem, welches Ereignis zuerst eintritt) zu einem aktiven Member des Clusters.",
        controlInterfaceSectionTooltip: "Die Steuerschnittstelle wird für die Konfiguration, Überwachung und Steuerung des Clusters verwendet. ",
        messagingInterfaceSectionTooltip: "Die Messaging-Schnittstelle ermöglicht die Weiterleitung von Nachrichten, die von einem Client auf einem Member des Clusters veröffentlicht wurden, an andere Member des Clusters.",
        // confirmation dialogs
        restartTitle: "${IMA_PRODUCTNAME_SHORT} erneut starten",
        restartContent: "Möchten Sie ${IMA_PRODUCTNAME_SHORT} wirklich erneut starten? ",
        restartButton: "Erneut starten",

        resetTitle: "Clusterzugehörigkeit zurücksetzen",
        resetContent: "Möchten Sie die Clusterzugehörigkeitskonfiguration wirklich zurücksetzen?",
        resetButton: "Zurücksetzen",

        leaveTitle: "Cluster verlassen",
        leaveContent: "Möchten Sie den Cluster wirklich verlassen?",
        leaveButton: "Verlassen",      
        restartRequiredTitle: "Neustart erforderlich",
        restartRequiredContent: "Ihre Änderungen wurden zwar gespeichert, werden aber erst nach einem Neustart des ${IMA_PRODUCTNAME_SHORT}-Servers wirksam.<br /><br />" +
                 "Wenn Sie den ${IMA_PRODUCTNAME_SHORT}-Server jetzt erneut starten möchten, klicken Sie auf <b>Sofort erneut starten</b>. Andernfalls klicken Sie auf <b>Später erneut starten</b>.",
        restartLaterButton: "Später erneut starten",
        restartNowButton: "Sofort erneut starten",

        // messages
        savingProgress: "Speicheroperation wird durchgeführt...",
        errorGettingClusterMembership: "Beim Abrufen der Clusterzugehörigkeitsinformationen ist ein Fehler aufgetreten.",
        saveClusterMembershipError: "Beim Speichern der Clusterzugehörigkeitskonfiguration ist ein Fehler aufgetreten.",
        restarting: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird erneut gestartet...",
        restartingFailed: "Der ${IMA_PRODUCTNAME_SHORT}-Server konnte nicht erneut gestartet werden.",
        resetting: "Die Konfiguration wird zurückgesetzt...",
        resettingFailed: "Die Clusterzugehörigkeitskonfiguration konnte nicht zurückgesetzt werden.",
        addressInvalid: "Sie müssen eine gültige IP-Adresse angeben.", 
        portInvalid: "Sie müssen eine Portnummer zwischen 1 und 65535 einschließlich eingeben."

});
