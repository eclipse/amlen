/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
		// ------------------------------------------------------------------------
		// Global Definitions
		// ------------------------------------------------------------------------
		global : {
			productName : "${IMA_PRODUCTNAME_FULL}", // Do_Not_Translate
			productNameTM: "${IMA_PRODUCTNAME_FULL_HTML}",	// Do_Not_Translate 		
			webUIMainTitle: "Webbenutzerschnittstelle von ${IMA_PRODUCTNAME_FULL} ",
			node: "Knoten",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "Lizenzvereinbarung",
			menuContent: "Inhalt der Menüauswahl",
			home : "Startseite",
			messaging : "Messaging",
			monitoring : "Überwachung",
			appliance : "Server",
			login: "Anmeldung",
			logout: "Abmeldung",
			changePassword: "Kennwort ändern",
			yes: "Ja",
			no: "Nein",
			all: "Alle",
			trueValue: "Wahr",
			falseValue: "Falsch",
			days: "Tage",
			hours: "Stunden",
			minutes: "Minuten",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0} d {1} h",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0} h {1} m",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0} m {1} s",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0} s", 
			notAvailable: "Nicht zutreffend",
			missingRequiredMessage: "Sie müssen einen Wert angeben.",
			pageNotAvailable: "Diese Seite ist wegen des Status des ${IMA_PRODUCTNAME_SHORT}-Servers nicht verfügbar.",
			pageNotAvailableServerDetail: "Zum Bearbeiten dieser Seite muss der ${IMA_PRODUCTNAME_SHORT}-Server im Produktionsmodus ausgeführt werden.",
			pageNotAvailableHAroleDetail: "Zum Bearbeiten dieser Seite muss der ${IMA_PRODUCTNAME_SHORT}-Server der primäre Server sein und darf nicht synchronisiert werden. Alternativ kann die Hochverfügbarkeit inaktiviert werden."			
		},
		name: {
			label: "Name",
			tooltip: "Der Name darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
					 "Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "Der Name darf keine führenden und nachfolgenden Leerzeichen enthalten.",
			noSpaces: "Der Name darf keine Leerzeichen enthalten.",
			invalidFirstChar: "Das erste Zeichen darf keine Zahl, kein Steuerzeichen und keines der folgenden Sonderzeichen sein: ! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "Der Name darf keine Steuerzeichen und keines der folgenden Sonderzeichen enthalten: &quot; , = \\ ",
			duplicateName: "Es ist bereits ein Datensatz mit diesem Namen vorhanden.",
			unicodeAlphanumericOnly: {
				invalidChar: "Der Name darf nur alphanumerische Zeichen enthalten. ",
				invalidFirstChar: "Das erste Zeichen darf keine Zahl sein."
			}
		},
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "Messaging-Protokolle",
				userAdministration: "Benutzerauthentifizierung",
				messageHubs: "Nachrichtenhubs",
				messageHubDetails: "Details des Nachrichtenhubs",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "Nachrichtenwarteschlangen",
				messagingTester: "Beispielanwendung"
			},
			monitoring: {
				connectionStatistics: "Verbindungen",
				endpointStatistics: "Endpunkte",
				queueMonitor: "Warteschlangen",
				topicMonitor: "Topics",
				mqttClientMonitor: "Nicht verbundene MQTT-Clients",
				subscriptionMonitor: "Subskriptionen",
				transactionMonitor: "Transaktionen",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "Server",
				downloadLogs: "Protokolle herunterladen",
				snmpSettings: "SNMP-Einstellungen"
			},
			appliance: {
				users: "Benutzer der Webbenutzerschnittstelle",
				networkSettings: "Netzeinstellungen",
			    locale: "Ländereinstellung, Datum und Uhrzeit",
			    securitySettings: "Sicherheitseinstellungen",
			    systemControl: "Serversteuerung",
			    highAvailability: "Hochverfügbarkeit",
			    webuiSecuritySettings: "Einstellungen für die Webbenutzerschnittstelle"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "Hilfe",
			homeTasks : "Aufgaben auf der Startseite wiederherstellen",
			about : {
				linkTitle : "Produktinfo",
				dialogTitle: "Produktinfo zu ${IMA_PRODUCTNAME_FULL}",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "Lizenzvereinbarung für {0} anzeigen",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} Version ${ISM_VERSION_ID}"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "Kennwort ändern",
				currpasswd: "Aktuelles Kennwort:",
				newpasswd: "Neues Kennwort:",
				password2: "Kennwort bestätigen:",
				password2Invalid: "Die Kennwörter stimmen nicht überein.",
				savingProgress: "Speicheroperation wird durchgeführt...",
				savingFailed: "Die Speicheroperation ist fehlgeschlagen."
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "Information",
			Warning : "Warnung",
			Error : "Fehler",
			Confirmation : "Bestätigung",
			Success : "Erfolg"
		},

		action : {
			Ok : "OK",
			Close : "Schließen",
			Cancel : "Abbrechen",
			Save: "Speichern",
			Create: "Erstellen",
			Add: "Hinzufügen",
			Edit: "Bearbeiten",
			Delete: "Löschen",
			MoveUp: "Nach oben",
			MoveDown: "Nach unten",
			ResetPassword: "Kennwort zurücksetzen",
			Actions: "Aktionen",
			OtherActions: "Weitere Aktionen",
			View: "Anzeigen",
			ResetColWidth: "Spaltenbreiten zurücksetzen",
			ChooseColumns: "Sichtbare Spalten auswählen",
			ResetColumns: "Sichtbare Spalten zurücksetzen"
		},
		// new pages and tabs need to go here
        cluster: "Cluster",
        clusterMembership: "Beitreten/Verlassen",
        adminEndpoint: "Administratorendpunkt",
        firstserver: "Verbindung zu einem Server herstellen",
        portInvalid: "Der gültige Portnummernbereich ist 1-65535.",
        connectionFailure: "Es kann keine Verbindung zum ${IMA_PRODUCTNAME_FULL}-Server hergestellt werden.",
        clusterStatus: "Status",
        webui: "Webbenutzerschnittstelle",
        licenseType_Devlopers: "Entwickler",
        licenseType_NonProd: "Nicht-Produktion",
        licenseType_Prod: "Produktion",
        licenseType_Beta: "Beta"
});
