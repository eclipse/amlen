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
		// Monitoring
		// ------------------------------------------------------------------------
		monitoring : {
			title : "Überwachung",
			monitoringNotAvailable: "Wegen des Status des ${IMA_PRODUCTNAME_SHORT}-Servers ist keine Überwachung verfügbar.",
			monitoringError:"Beim Abrufen der Überwachungsdaten ist ein Fehler aufgetreten.",
			pool1LimitReached: "Die Belegung von Pool 1 ist zu hoch. Veröffentlichungen und Subskriptionen können zurückgewiesen werden.",
			connectionStats: {
				title: "Verbindungsüberwachung",
				tagline: "Es werden aggregierte Verbindungsdaten überwacht und die Verbindungen mit der besten Leistung und die Verbindungen mit der schlechtesten Leistung für mehrere Verbindungsstatistiken abgefragt.",
		 		charts: {
					sectionTitle: "Verbindungsdiagramme",
					tagline: "Die Anzahl aktiver und neuer Verbindungen zum Server wird überwacht. Zum Anhalten der Diagrammaktualisierungen klicken Sie auf die Schaltfläche unterhalb der Diagramme.",
					activeConnectionsTitle: "Snapshot der aktiven Verbindungen, der ungefähr alle fünf Sekunden erstellt wird.",
					newConnectionsTitle: "Snapshot der neuen und geschlossenen Verbindungen, der ungefähr alle fünf Sekunden erstellt wird."
				},
		 		grids: {
					sectionTitle: "Verbindungsdaten",
					tagline: "Es werden die Verbindungen mit der besten und der schlechtesten Leistung für einen bestimmten Endpunkt und eine bestimmte Statistik angezeigt." +
							"Es können bis zu 50 Verbindungen angezeigt werden. Die Überwachungsdaten werden auf dem Server zwischengespeichert, und dieser Cache wird minütlich aktualisiert." +
							"Deshalb können die Daten maximal eine Minute lang nicht auf dem neuesten Stand sein. "
				}
			},
			endpointStats: {
				title: "Endpunktüberwachung",
				tagline: "Die Langzeitleistung einzelner Endpunkte wird in Form eines Zeitreihendiagramms oder in Form aggregierter Daten überwacht. ",
		 		charts: {
					sectionTitle: "Endpunktdiagramm",
					tagline: "Es werden Zeitreihendiagramme für einen bestimmten Endpunkt, eine bestimmte Statistik oder eine bestimmte Dauer angezeigt." +
						"Bei einer Dauer von 1 Stunde oder weniger basieren die Diagramme auf Snapshots, die alle fünf Sekunden erstellt werden. " +
						"Bei einer Dauer von mehr als einer Stunde basieren die Diagramme auf Snapshots, die minütlich erstellt werden. "
				},
		 		grids: {
					sectionTitle: "Endpunktdaten",
					tagline: "Es werden aggregierte Daten für einzelne Endpunkte angezeigt. Die Überwachungsdaten werden auf dem Server zwischengespeichert, und dieser Cache wird minütlich aktualisiert." +
							 "Deshalb können die Daten maximal eine Minute lang nicht auf dem neuesten Stand sein. "
				}
			},
			queueMonitor: {
				title: "Warteschlangenüberwachung",
				tagline: "Es werden einzelne Warteschlangen mithilfe verschiedener Warteschlangenstatistiken überwacht. Es können bis zu 100 Warteschlangen angezeigt werden."
			},
			mqttClientMonitor: {
				title: "Nicht verbundene MQTT-Clients",
				tagline: "Es werden nicht verbundene MQTT-Clients gesucht und gelöscht. Es können bis zu 100 MQTT-Clients angezeigt werden.",
				taglineNoDelete: "Es werden nicht verbundene MQTT-Clients gesucht. Es können bis zu 100 MQTT-Clients angezeigt werden."
			},
			subscriptionMonitor: {
				title: "Subskriptionsüberwachung",
				tagline: "Es werden Subskriptionen mithilfe verschiedener Subskriptionsstatistiken überwacht. Permanente Subskriptionen können gelöscht werden. Es können bis zu 100 Subskriptionen angezeigt werden.",
				taglineNoDelete: "Es werden Subskriptionen mithilfe verschiedener Subskriptionsstatistiken überwacht. Es können bis zu 100 Subskriptionen angezeigt werden."
			},
			transactionMonitor: {
				title: "Transaktionsüberwachung",
				tagline: "Es werden XA-Transaktionen überwacht, die von einem externen Transaktionsmanager koordiniert werden."
			},
			destinationMappingRuleMonitor: {
				title: "Monitor für MQ Connectivity-Zielzuordnungsregeln",
				tagline: "Es werden Statistiken für Zielzuordnungsregeln überwacht. Die Statistiken sind für ein Zielzuordnungsregel/Warteschlangenmanagerverbindung-Paar bestimmt. Es können bis zu 100 Paare angezeigt werden."
			},
			applianceMonitor: {
				title: "Servermonitor",
				tagline: "Es werden Informationen für die verschiedenen ${IMA_PRODUCTNAME_FULL}-Statistiken erfasst.",
				storeMonitorTitle: "Persistenter Speicher",
				storeMonitorTagline: "Es werden Statistiken für Ressourcen überwacht, die vom persistenten ${IMA_PRODUCTNAME_FULL}-Speicher verwendet werden.",
				memoryMonitorTitle: "Hauptspeicher",
				memoryMonitorTagline: "Es werden Statistiken für den von anderen ${IMA_PRODUCTNAME_FULL}-Prozessen belegten Hauptspeicher überwacht."
			},
			topicMonitor: {
				title: "Topicüberwachung",
				tagline: "Sie können Topicüberwachungen konfigurieren, die verschiedene aggregierte Statistiken zu Topiczeichenfolgen bereitstellen.",
				taglineNoConfigure: "Sie können Topicüberwachungen anzeigen, die verschiedene aggregierte Statistiken zu Topiczeichenfolgen bereitstellen."
			},
			views: {
				conntimeWorst: "Neueste Verbindungen",
				conntimeBest: "Älteste Verbindungen",
				tputMsgWorst: "Niedrigster Durchsatz (Nachrichten)",
				tputMsgBest: "Höchster Durchsatz (Nachrichten)",
				tputBytesWorst: "Niedrigster Durchsatz (KB)",
				tputBytesBest: "Höchster Durchsatz (KB)",
				activeCount: "Aktive Verbindungen",
				connectCount: "Aufgebaute Verbindungen",
				badCount: "Kumulative fehlgeschlagene Verbindungen",
				msgCount: "Durchsatz (Nachrichten)",
				bytesCount: "Durchsatz (Byte)",

				msgBufHighest: "Warteschlangen mit den meisten gepufferten Nachrichten",
				msgProdHighest: "Warteschlangen mit den meisten erzeugten Nachrichten",
				msgConsHighest: "Warteschlangen mit den meisten konsumierten Nachrichten",
				numProdHighest: "Warteschlangen mit den meisten Produzenten",
				numConsHighest: "Warteschlangen mit den meisten Konsumenten",
				msgBufLowest: "Warteschlangen mit den wenigsten gepufferten Nachrichten",
				msgProdLowest: "Warteschlangen mit den wenigsten erzeugten Nachrichten",
				msgConsLowest: "Warteschlangen mit den wenigsten konsumierten Nachrichten",
				numProdLowest: "Warteschlangen mit den wenigsten Produzenten",
				numConsLowest: "Warteschlangen mit den wenigsten Konsumenten",
				BufferedHWMPercentHighestQ: "Warteschlangen mit der höchsten Kapazitätsauslastung",
				BufferedHWMPercentLowestQ: "Warteschlangen mit der niedrigsten Kapazitätsauslastung",
				ExpiredMsgsHighestQ: "Warteschlangen mit den meisten abgelaufenen Nachrichten",
				ExpiredMsgsLowestQ: "Warteschlangen mit den wenigsten abgelaufenen Nachrichten",

				msgPubHighest: "Topics mit den meisten veröffentlichten Nachrichten",
				subsHighest: "Topics mit den meisten Subskribenten",
				msgRejHighest: "Topics mit den meisten zurückgewiesenen Nachrichten",
				pubRejHighest: "Topics mit den meisten fehlgeschlagenen Veröffentlichungen",
				msgPubLowest: "Topics mit den wenigsten veröffentlichten Nachrichten",
				subsLowest: "Topics mit den wenigsten Subskribenten",
				msgRejLowest: "Topics mit den wenigsten zurückgewiesenen Nachrichten",
				pubRejLowest: "Topics mit den wenigsten fehlgeschlagenen Veröffentlichungen",

				publishedMsgsHighest: "Subskriptionen mit den meisten veröffentlichten Nachrichten",
				bufferedMsgsHighest: "Subskriptionen mit den meisten gepufferten Nachrichten",
				bufferedPercentHighest: "Subskriptionen mit dem höchsten Prozentsatz gepufferter Nachrichten",
				rejectedMsgsHighest: "Subskriptionen mit den meisten zurückgewiesenen Nachrichten",
				publishedMsgsLowest: "Subskriptionen mit den wenigsten veröffentlichten Nachrichten",
				bufferedMsgsLowest: "Subskriptionen mit den wenigsten gepufferten Nachrichten",
				bufferedPercentLowest: "Subskriptionen mit dem niedrigsten Prozentsatz gepufferter Nachrichten",
				rejectedMsgsLowest: "Subskriptionen mit den wenigsten zurückgewiesenen Nachrichten",
				BufferedHWMPercentHighestS: "Subskriptionen mit der höchsten Kapazitätsauslastung",
				BufferedHWMPercentLowestS: "Subskriptionen mit der niedrigsten Kapazitätsauslastung",
				ExpiredMsgsHighestS: "Subskriptionen mit den meisten abgelaufenen Nachrichten",
				ExpiredMsgsLowestS: "Subskriptionen mit den wenigsten abgelaufenen Nachrichten",
				DiscardedMsgsHighestS: "Subskriptionen mit den meisten verworfenen Nachrichten",
				DiscardedMsgsLowestS: "Subskriptionen mit den wenigsten verworfenen Nachrichten",
				mostCommitOps: "Regeln mit den meisten Commitoperationen",
				mostRollbackOps: "Regeln mit den meisten Rollbackoperationen",
				mostPersistMsgs: "Regeln mit den meisten persistenten Nachrichten",
				mostNonPersistMsgs: "Regeln mit den meisten nicht persistenten Nachrichten",
				mostCommitMsgs: "Regeln mit den meisten festgeschriebenen Nachrichten"
			},
			rules: {
				any: "Beliebig",
				imaTopicToMQQueue: "${IMA_PRODUCTNAME_SHORT}-Topic zu MQ-Warteschlange",
				imaTopicToMQTopic: "${IMA_PRODUCTNAME_SHORT}-Topic zu MQ-Topic",
				mqQueueToIMATopic: "MQ-Warteschlange zu ${IMA_PRODUCTNAME_SHORT}-Topic",
				mqTopicToIMATopic: "MQ-Topic zu ${IMA_PRODUCTNAME_SHORT}-Topic",
				imaTopicSubtreeToMQQueue: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Warteschlange",
				imaTopicSubtreeToMQTopic: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Topic",
				imaTopicSubtreeToMQTopicSubtree: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Topicunterverzeichnisstruktur",
				mqTopicSubtreeToIMATopic: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Topic",
				mqTopicSubtreeToIMATopicSubtree: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur",
				imaQueueToMQQueue: "${IMA_PRODUCTNAME_SHORT}-Warteschlange zu MQ-Warteschlange",
				imaQueueToMQTopic: "${IMA_PRODUCTNAME_SHORT}-Warteschlange zu MQ-Topic",
				mqQueueToIMAQueue: "MQ-Warteschlange zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange",
				mqTopicToIMAQueue: "MQ-Topic zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange",
				mqTopicSubtreeToIMAQueue: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange"
			},
			chartLabels: {
				activeCount: "Aktive Verbindungen",
				connectCount: "Aufgebaute Verbindungen",
				badCount: "Kumulative fehlgeschlagene Verbindungen",
				msgCount: "Durchsatz (Nachrichten/Sekunde)",
				bytesCount: "Durchsatz (MB/Sekunde)"
			},
			connectionVolume: "Aktive Verbindungen",
			connectionVolumeAxisTitle: "Aktiv",
			connectionRate: "Neue Verbindungen",
			connectionRateAxisTitle: "Geändert",
			newConnections: "Geöffnete Verbindungen",
			closedConnections: "Geschlossene Verbindungen",
			timeAxisTitle: "Zeit",
			pauseButton: "Diagrammaktualisierungen anhalten",
			resumeButton: "Diagrammaktualisierungen fortsetzen",
			noData: "Keine Daten",
			grid: {				
				name: "Client-ID",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "Der Verbindungsname. (Name)",
				endpoint: "Endpunkt",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "Der Name des Endpunkts. (Endpoint)",
				clientIp: "Client-IP-Adresse",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "Die IP-Adresse des Clients. (ClientAddr)",
				clientId: "Client-ID:",
				userId: "Benutzer-ID",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "Die primäre Benutzer-ID. (UserId)",
				protocol: "Protokoll",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "Der Name des Protokolls. (Protocol)",
				bytesRecv: "Empfangen (KB)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "Die Anzahl der seit der Verbindungszeit gelesenen Bytes. (ReadBytes)",
				bytesSent: "Gesendet (KB)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "Die Anzahl der seit der Verbindungszeit geschriebenen Bytes. (WriteBytes)",
				msgRecv: "Empfangen (Nachrichten)",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "Die Anzahl der seit der Verbindungszeit gelesenen Nachrichten. (ReadMsg)",
				msgSent: "Gesendet (Nachrichten)",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "Die Anzahl der seit der Verbindungszeit geschriebenen Nachrichten. (WriteMsg)",
				bytesThroughput: "Durchsatz (KB/Sekunde)",
				msgThroughput: "Durchsatz (Nachrichten/Sekunde)",
				connectionTime: "Verbindungszeit (Sekunden)",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "Die Anzahl der Sekunden seit der Herstellung der Verbindung. (ConnectTime)",
				updateButton: "Aktualisieren",
				refreshButton: "Anzeige aktualisieren",
				deleteClientButton: "Client löschen",
				deleteSubscriptionButton: "Permanente Subskription löschen",
				endpointLabel: "Quelle:",
				listenerLabel: "Endpunkt:",
				queueNameLabel: "Warteschlangenname:",
				ruleNameLabel: "Regelname:",
				queueManagerConnLabel: "Warteschlangenmanagerverbindung:",
				topicNameLabel: "Überwachte Topics:",
				topicStringLabel: "Topiczeichenfolge:",
				subscriptionLabel: "Subskriptionsname:",
				subscriptionTypeLabel: "Subskriptionstyp:",
				refreshingLabel: "Aktualisierung wird durchgeführt...",
				datasetLabel: "Abfrage:",
				ruleTypeLabel: "Regeltyp:",
				durationLabel: "Dauer:",
				resultsLabel: "Ergebnisse:",
				allListeners: "Alle",
				allEndpoints: "(alle Endpunkte) ",
				allSubscriptions: "Alle",
				metricLabel: "Abfrage:",
				// TRNOTE min is short for minutes
				last10: "Letzte 10 Minuten",
				// TRNOTE hr is short for hours
				last60: "Letzte Stunde",
				// TRNOTE hr is short for hours
				last360: "Letzte 6 Stunden",
				// TRNOTE hr is short for hours
				last1440: "Letzte 24 Stunden",
				// TRNOTE min is short for minutes
				min5: "5 Minuten",
				// TRNOTE min is short for minutes
				min10: "10 Minuten",
				// TRNOTE min is short for minutes
				min30: "30 Minuten",
				// TRNOTE hr is short for hours
				hr1: "1 Stunde",
				// TRNOTE hr is short for hours
				hr3: "3 Stunden",
				// TRNOTE hr is short for hours
				hr6: "6 Stunden",
				// TRNOTE hr is short for hours
				hr12: "12 Stunden",
				// TRNOTE hr is short for hours
				hr24: "24 Stunden",
				totalMessages: "Gesamtzahl der Nachrichten",
				bufferedMessages: "Gepufferte Nachrichten",
				subscriptionProperties: "Subskriptionseigenschaften",
				commitTransactionButton: "Commit durchführen",
				rollbackTransactionButton: "Rollback durchführen",
				forgetTransactionButton: "Übergehen",
				queue: {
					queueName: "Warteschlangenname",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "Der Name der Warteschlange. (QueueName)",
					msgBuf: "Aktueller Wert",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "Die Anzahl der Nachrichten, die in der Warteschlange gepuffert sind und darauf warten, konsumiert zu werden. (BufferedMsgs)",
					msgBufHWM: "Höchstwert",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "Die höchste Anzahl in der Warteschlange gepufferter Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Warteschlange, je nachdem, welches Ereignis jünger ist. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Höchster % des Maximums",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "Die höchste Anzahl gepufferter Nachrichten als Prozentsatz der maximalen Anzahl an Nachrichten, die gepuffert werden können. (BufferedHWMPercent)",
					perMsgBuf: "Aktueller % des Maximums",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "Die Anzahl gepufferter Nachrichten als Prozentsatz der maximalen Anzahl gepufferter Nachrichten. (BufferedPercent)",
					totMsgProd: "Erzeugt",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "Die Anzahl der an die Warteschlange gesendeten Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Warteschlange, je nachdem, welches Ereignis jünger ist. (ProducedMsgs)",
					totMsgCons: "Konsumiert",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "Die Anzahl der aus der Warteschlange konsumierten Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Warteschlange, je nachdem, welches Ereignis jünger ist. (ConsumedMsgs)",
					totMsgRej: "Zurückgewiesen",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "Die Anzahl der Nachrichten, die nicht an die Warteschlange gesendet wurden, weil die maximale Anzahl gepufferter Nachrichten erreicht ist. (RejectedMsgs)",
					totMsgExp: "Abgelaufen",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "Die Anzahl der in der Warteschlange abgelaufenen Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Warteschlange, je nachdem, welches Ereignis jünger ist. (ExpiredMsgs)",			
					numProd: "Produzenten",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "Die Anzahl aktiver Produzenten in der Warteschlange. (Producers)", 
					numCons: "Konsumenten",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "Die Anzahl aktiver Konsumenten in der Warteschlange. (Consumers)",
					maxMsgs: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "Die maximale Anzahl gepufferter Nachrichten, die in der Warteschlange zulässig sind. Dieser Wert ist eine Richtlinie und kein absoluter Grenzwert. Wenn das System unter Belastung ausgeführt wird, kann die Anzahl  gepufferter Nachrichten in einer Warteschlange geringfügig höher sein als der Wert von MaxMessages. (MaxMessages)"
				},
				destmapping: {
					ruleName: "Regelname",
					qmConnection: "Warteschlangenmanagerverbindung",
					commits: "Commits",
					rollbacks: "Rollbacks",
					committedMsgs: "Festgeschriebene Nachrichten",
					persistentMsgs: "Persistente Nachrichten",
					nonPersistentMsgs: "Nicht persistente Nachrichten",
					status: "Status",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "Der Name der Zielzuordnungsregel, die überwacht wird. (RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "Der Name der Warteschlangenmanagerverbindung, der die Zielzuordnungsregel zugeordnet ist. (QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "Die Anzahl der für die Zielzuordnungsregel ausgeführten Commitoperationen. Eine einzige Commitoperation kann viele Nachrichten festschreiben. (CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "Die Anzahl der für die Zielzuordnungsregel ausgeführten Rollbackoperationen. (RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "Die Anzahl der für die Zielzuordnungsregel festgeschriebenen Nachrichten. (CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "Die Anzahl persistenter Nachrichten, die über die Zielzuordnungsregel gesendet werden. (PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "Die Anzahl nicht persistenter Nachrichten, die über die Zielzuordnungsregel gesendet werden. (NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "Der Status der Regel: Enabled, Disabled, Reconnecting oder Restarting. (Status)"
				},
				mqtt: {
					clientId: "Client-ID",
					lastConn: "Letztes Verbindungsdatum",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "Die Client-ID. (ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "Die Uhrzeit, zu der der Client zuletzt eine Verbindung zum ${IMA_PRODUCTNAME_SHORT}-Server hergestellt hat. (LastConnectedTime)",	
					errorMessage: "Ursache",
					errorMessageTooltip: "Der Grund, aus dem der Client nicht gelöscht werden konnte."
				},
				subscription: {
					subsName: "Name",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "Der Name, der der Subskription zugeordnet wird. Dieser Wert kann für eine nicht dauerhafte Subskription eine leere Zeichenfolge sein. (SubName)",
					topicString: "Topiczeichenfolge",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "Das Topic, in dem die Subskription subskribiert wird. (TopicString)",					
					clientId: "Client-ID",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "Die Client-ID des Clients, der Eigner der Subskription ist. (ClientID)",
					consumers: "Konsumenten",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "Die Anzahl der Konsumenten der Subskription. (Consumers)",
					msgPublished: "Veröffentlicht",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "Die Anzahl der für diese Subskription veröffentlichten Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Subskription, je nachdem, welches Ereignis jünger ist. (PublishedMsgs)",
					msgBuf: "Aktueller Wert",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "Die Anzahl veröffentlichter Nachrichten, die darauf warten, an den Client gesendet zu werden. (BufferedMsgs)",
					perMsgBuf: "Aktueller % des Maximums",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "Der Prozentsatz der maximal gepufferten Nachrichten, der die momentan gepufferten Nachrichten darstellt. (BufferedPercent)",
					msgBufHWM: "Höchstwert",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "Die höchste Anzahl gepufferter Nachrichten seit dem Start des Servers bzw. seit der Erstellung der Subskription, je nachdem, welches Ereignis jünger ist. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Höchster % des Maximums",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "Die höchste Anzahl gepufferter Nachrichten als Prozentsatz der maximalen Anzahl an Nachrichten, die gepuffert werden können. (BufferedHWMPercent)",
					maxMsgBuf: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "Die maximale Anzahl gepufferter Nachrichten, die für diese Subskription zulässig sind. (MaxMessages)",
					rejMsg: "Zurückgewiesen",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "Die Anzahl der Nachrichten, die zurückgewiesen werden, weil die maximale Anzahl gepufferter Nachrichten erreicht wurde, als die Nachrichten für die Subskription veröffentlicht wurden. (RejectedMsgs)",
					expDiscMsg: "Abgelaufen/Verworfen",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "Die Anzahl der Nachrichten, die nicht zugestellt wurden, weil sie entweder abgelaufen oder verworfen wurden, als der Puffer voll war. (ExpiredMsgs + DiscardedMsgs)",
					expMsg: "Abgelaufen",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "Die Anzahl der Nachrichten, die die nicht zugestellt wurden, weil sie abgelaufen sind. (ExpiredMsgs)",
					DiscMsg: "Verworfen",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "Die Anzahl der Nachrichten, die nicht zugestellt wurden, weil sie verworfen wurden, als der Puffer voll war. (DiscardedMsgs)",
					subsType: "Typ",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "Diese Spalte gibt an, ob die Subskription permanent oder nicht permanent ist und ob die Subskription gemeinsam genutzt wird. Permanente Subskriptionen bleiben auch nach Serverneustarts erhalten, sofern die Subskription nicht explizit gelöscht wird. (IsDurable, IsShared)",				
					isDurable: "Permanent",
					isShared: "Gemeinsam genutzt",
					notDurable: "Nicht permanent",
					noName: "(ohne)",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "Abgelaufen: {0}; Verworfen: {1}",
					errorMessage: "Ursache",
					errorMessageTooltip: "Der Grund, aus dem die Subskription nicht gelöscht werden konnte.",
					messagingPolicy: "Messaging-Richtlinie",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "Der Name der von der Subskription verwendeten Messaging-Richtlinie. (MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "Transaktions-ID (XID)",
					timestamp: "Zeitmarke",
					state: "Status",
					stateCode2: "Vorbereitet",
					stateCode5: "Heuristisches Commit",
					stateCode6: "Heuristisches Rollback"
				},
				topic: {
					topicSubtree: "Topiczeichenfolge",
					msgPub: "Veröffentlichte Nachrichten",
					rejMsg: "Zurückgewiesene Nachrichten",
					rejPub: "Fehlgeschlagene Veröffentlichungen",
					numSubs: "Subskribenten",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "Das Topic, das überwacht wird. Die Topiczeichenfolge enthält immer einen Platzhalter. (TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "Die Anzahl der Nachrichten, die erfolgreich in einem Topic veröffentlicht wurden, das der Topiczeichenfolge mit dem Platzhalter entspricht. (PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "Die Anzahl der Nachrichten, die von einer oder mehreren Subskriptionen zurückgewiesen wurden, bei denen die Servicequalitätsebene kein Fehlschlagen der Veröffentlichungsanforderung bewirkt hat. (RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "Die Anzahl der Veröffentlichungsanforderungen, die fehlgeschlagen sind, weil die Nachricht von einer oder mehreren Subskriptionen zurückgewiesen wurde. (FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "Die Anzahl aktiver Subskriptionen in den überwachten Topics. Die Abbildung zeigt alle aktiven Subskriptionen, die der Topiczeichenfolge mit Platzhalter entsprechen. (Subscriptions)"
				},
				store: {
					name: "Statistik",
					value: "Wert",
                    diskUsedPercent: "Belegter Plattenspeicherplatz (%)",
                    diskFreeBytes: "Freier Plattenspeicherplatz (MB)",
					persistentMemoryUsedPercent: "Belegter Hauptspeicher (%)",
					memoryTotalBytes: "Gesamthauptspeicherkapazität (MB)",
					Pool1TotalBytes: "Gesamtkapazität von Pool 1 (MB)",
					Pool1UsedBytes: "Belegte Kapazität von Pool 1 (MB)",
					Pool1UsedPercent: "Belegte Kapazität von Pool 1 (%)",
					Pool1RecordLimitBytes: "Datensatzgrenzwert für Pool 1 (MB)", 
					Pool1RecordsUsedBytes: "Belegte Datensätze in Pool 1 (MB)",
                    Pool2TotalBytes: "Gesamtkapazität von Pool 2 (MB)",
                    Pool2UsedBytes: "Belegte Kapazität von Pool 2 (MB)",
                    Pool2UsedPercent: "Belegte Kapazität von Pool 2 (%)"
				},
				memory: {
					name: "Statistik",
					value: "Wert",
					memoryTotalBytes: "Gesamthauptspeicherkapazität (MB)",
					memoryFreeBytes: "Freier Hauptspeicher (MB)",
					memoryFreePercent: "Freier Hauptspeicher (%)",
					serverVirtualMemoryBytes: "Virtueller Speicher (MB)",
					serverResidentSetBytes: "Residente Gruppe (MB)",
					messagePayloads: "Nachrichtennutzdaten (MB)",
					publishSubscribe: "Publish/Subscribe (MB)",
					destinations: "Ziele (MB)",
					currentActivity: "Aktuelle Aktivität (MB)",
					clientStates: "Clientstatus (MB)"
				}
			},
			logs: {
				title: "Protokolle",
				pageSummary: "", 
				downloadLogsSubTitle: "Protokolle herunterladen",
				downloadLogsTagline: "Sie können Protokolle für Diagnosezwecke herunterladen.",
				lastUpdated: "Letzte Aktualisierung",
				name: "Protokolldatei",
				size: "Größe (Byte)"
			},
			lastUpdated: "Letzte Aktualisierung: ",
			bufferedMsgs: "Gepufferte Nachrichten: ",
			retainedMsgs: "Aufbewahrte Nachrichten: ",
			dataCollectionInterval: "Datenerfassungsintervall: ",
			interval: {fiveSeconds: "5 Sekunden", oneMinute: "1 Minute" },
			cacheInterval: "Datenerfassungsintervall: 1 Minute",
			notAvailable: "Nicht zutreffend",
			savingProgress: "Speicheroperation wird durchgeführt...",
			savingFailed: "Die Speicheroperation ist fehlgeschlagen.",
			noRecord: "Es wurde kein Datensatz auf dem Server gefunden.",
			deletingProgress: "Löschoperation wird durchgeführt...",
			deletingFailed: "Die Löschoperation ist fehlgeschlagen.",
			deletePending: "Es steht eine Löschoperation an.",
			deleteSuccessful: "Die Löschoperation war erfolgreich.",
			testingProgress: "Tests werden durchgeführt...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "Geben Sie die zu überwachende Topiczeichenfolge an. Die Zeichenfolge darf nicht mit $SYS beginnen und muss mit einem Platzhalter für mehrere Ebenen (z. B. /animals/dogs/#) enden. " +
						                    "Für die Aggregation von Statistiken für alle Topics geben Sie einen einzelnen Platzhalterzeichen für mehrere Ebenen (#) an. " +
						                    "Wenn ein Topic keine untergeordneten Topics hat, zeigt die Überwachung nur Daten für dieses Topic an. " +
						                    "Wenn Sie beispielsweise /animals/dogs/labradors/# angeben, wird das Topic /animals/dogs/labradors nur überwacht, wenn labradors keine untergeordneten Topics hat.",
				addTopicMonitorTitle: "Topicüberwachung hinzufügen",
				removeTopicMonitorTitle: "Topicüberwachung löschen",
				removeSubscriptionTitle: "Permanente Subskription löschen",
				removeSubscriptionsTitle: "Permanente Subskriptionen löschen",
				removeSubscriptionContent: "Möchten Sie diese Subskription Datensatz wirklich löschen?",
				removeSubscriptionsContent: "Möchten Sie wirklich {0} Subskriptionen löschen?",
				removeClientTitle: "MQTT-Client löschen",
				removeClientsTitle: "MQTT-Clients löschen",
				removeClientContent: "Möchten Sie diesen Client wirklich löschen? Wenn Sie den Client löschen, werden auch die Subskriptionen für diesen Client gelöscht.",
				removeClientsContent: "Möchten Sie wirklich {0} Clients löschen? Wenn Sie die Clients löschen, werden auch die Subskriptionen für diese Clients gelöscht.",
				topicStringLabel: "Topiczeichenfolge:",
				descriptionLabel: "Beschreibung",
				topicStringTooltip: "Die zu überwachende Topiczeichenfolge. Mit Ausnahme des erforderlichen Platzhalters (#) am Ende sind die folgenden Zeichen in der Topiczeichenfolge nicht zulässig: + #",
				saveButton: "Speichern",
				cancelButton: "Abbrechen"
			},
			removeDialog: {
				title: "Eintrag löschen",
				content: "Möchten Sie diesen Datensatz wirklich löschen?",
				deleteButton: "Löschen",
				cancelButton: "Abbrechen"
			},
			actionConfirmDialog: {
				titleCommit: "Transaktionscommit durchführen",
				titleRollback: "Transaktionsrollback durchführen",
				titleForget: "Transaktion übergehen",
				contentCommit: "Möchten Sie diese Transaktion wirklich festschreiben?",
				contentRollback: "Möchten Sie diese Transaktion wirklich rückgängig machen?",
				contentForget: "Möchten Sie diese Transaktion wirklich übergehen?",
				actionButtonCommit: "Commit durchführen",
				actionButtonRollback: "Rollback durchführen",
				actionButtonForget: "Übergehen",
				cancelButton: "Abbrechen",
				failedRollback: "Beim Rollback der Transaktion ist ein Fehler aufgetreten.",
				failedCommit: "Beim Commit der Transaktion ist ein Fehler aufgetreten.",
				failedForget: "Beim Übergehen der Transaktion ist ein Fehler aufgetreten."
			},
			asyncInfoDialog: {
				title: "Anstehende löschen",
				content: "Die Subskription konnte nicht sofort gelöscht werden. Sie wird sobald wie möglich gelöscht.",
				closeButton: "Schließen"
			},
			deleteResultsDialog: {
				title: "Löschstatus der Subskription",
				clientsTitle: "Löschstatus des Clients",
				allSuccessful:  "Alle Subskriptionen wurden erfolgreich gelöscht.",
				allClientsSuccessful: "Alle Clients wurden erfolgreich gelöscht.",
				pendingTitle: "Anstehende löschen",
				pending: "Die folgenden Subskriptionen konnte nicht sofort gelöscht werden. Sie wird sobald wie möglich gelöscht:",
				clientsPending: "Die folgenden Clients konnte nicht sofort gelöscht werden. Sie wird sobald wie möglich gelöscht:",
				failedTitle: "Löschvorgang fehlgeschlagen",
				failed:  "Die folgenden Subskriptionen konnte nicht gelöscht werden: ",
				clientsFailed: "Die folgenden Clients konnte nicht gelöscht werden: ",
				closeButton: "Schließen"					
			},
			invalidName: "Es ist bereits ein Datensatz mit diesem Namen vorhanden.",
			// TRNOTE Do not translate "$SYS"
			reservedName: "Die Topiczeichenfolge darf nicht mit $SYS beginnen. ",
			invalidChars: "Die folgenden Zeichen sind nicht zulässig: + #",			
			invalidTopicMonitorFormat: "Die Topiczeichenfolge muss # lauten oder mit /# enden.",
			invalidRequired: "Es muss ein Wert eingegeben werden.",
			clientIdMissingMessage: "Dieser Wert ist erforderlich. Sie können einen oder mehrere Sterne (*) als Platzhalter verwenden.",
			ruleNameMissingMessage: "Dieser Wert ist erforderlich. Sie können einen oder mehrere Sterne (*) als Platzhalter verwenden.",
			queueNameMissingMessage: "Dieser Wert ist erforderlich. Sie können einen oder mehrere Sterne (*) als Platzhalter verwenden.",
			subscriptionNameMissingMessage: "Dieser Wert ist erforderlich. Sie können einen oder mehrere Sterne (*) als Platzhalter verwenden.",
			discardOldestMessagesInfo: "Mindestens ein aktivierter Endpunkt verwendet eine Messaging-Richtlinie mit der Einstellung <em>Alte Nachrichten verwerfen</em> für die Option Verhalten bei maximaler Nachrichtenanzahl. Verworfene Nachrichten haben keine Auswirkungen auf die Werte für <em>Zurückgewiesene Nachrichten</em> und <em>Fehlgeschlagene Veröffentlichungen</em>. ",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "SNMP-Einstellungen",
				tagline: "Aktivieren Sie das community-basierte Simple Network Management Protocol (SNMPv2C), laden Sie die MIB-Dateien (Management Information Base) herunter und konfigurieren Sie SNMPv2C-Communitys. " +
						"${IMA_PRODUCTNAME_FULL} unterstützt SNMPv2C.",
				status: {
					title: "SNMP-Status",
					tagline: {
						disabled: "SNMP ist nicht aktiviert.",
						enabled: "SNMP ist aktiviert und konfiguriert.",
						warning: "Es muss eine SNMP-Community definiert werden, um den Zugriff auf die SNMP-Daten auf Ihrem Server zuzulassen."
					},
					enableLabel: "SNMPv2C-Agenten aktivieren",
					addressLabel: "Adresse des SNMP-Agenten:",					
					editLinkLabel: "Bearbeiten",
					editAddressDialog: {
						title: "Adresse des SNMP-Agenten bearbeiten",
						instruction: "Geben Sie die statische IP-Adresse an, die ${IMA_PRODUCTNAME_FULL} für SNMP verwenden kann." +
								"Wenn Sie alle IP-Adressen zulassen möchten, die auf dem ${IMA_PRODUCTNAME_SHORT}-Server konfiguriert sind, oder wenn Ihr Server nur dynamische Adressen verwendet, wählen Sie <em>Alle</em> aus."
					},
					getSnmpEnabledError: "Beim Abrufen des SNMP-Aktivierungsstatus ist ein Fehler aufgetreten.",
					setSnmpEnabledError: "Beim Festlegen des SNMP-Aktivierungsstatus ist ein Fehler aufgetreten.",
					getSnmpAgentAddressError: "Beim Abrufen der Adresse des SNMP-Agenten ist ein Fehler aufgetreten.",
					setSnmpAgentAddressError: "Beim Festlegen der Adresse des SNMP-Agenten ist ein Fehler aufgetreten."
				},
				mibs: {
					title: "Unternehmens-MIBs",
					tagline: "Die Unternehmens-MIB-Dateien beschreiben, welche Funktionen und Daten vom integrierten SNMP-Agenten verfügbar sind, so dass Ihr Client entsprechend auf diese Daten zugreifen kann. ",
					messageSightLinkLabel: "${IMA_PRODUCTNAME_SHORT}-MIB",
					hwNotifyLinkLabel: "Hardwarebenachrichtigungs-MIB"
				},
				communities: {
					title: "SNMP-Communitys",
					tagline: "Sie können den Zugriff auf die SNMP-Daten auf Ihrem Server durch Erstellung einer oder mehrerer SNMP-v2C-Communitys definieren." +
							"Wenn die Überwachung aktiviert ist, ist eine SNMP-Community erforderlich. " +
							"Alle Communitys haben Lesezugriff. ",
					grid: {
						nameColumn: "Name",
						hostRestrictionColumn: "Hosteinschränkung"
					},
					dialog: {
						addTitle: "SNMP-Community hinzufügen",
						editTitle: "SNMP-Community bearbeiten",
						instruction: "Eine SNMP-Community lässt den Zugriff auf die SNMP-Daten auf Ihrem Server zu. ",
						deleteTitle: "SNMP-Community löschen",
						deletePrompt: "Möchten Sie diese SNMP-Community wirklich löschen?",
						deleteNotAllowedTitle: "Entfernen nicht zulässig", 
						deleteNotAllowedMessage: "Die SNMP-Community kann nicht entfernt werden, weil sie von mindestens einem Trapsubskribenten verwendet wird. " +
								"Entfernen Sie die SNMP-Community aus allen Trapsubskribenten und wiederholen Sie dann die Operation. ",
						defineTitle: "SNMP-Community definieren",
						defineInstruction: "Es muss eine SNMP-Community definiert werden, um den Zugriff auf die SNMP-Daten auf Ihrem Server zuzulassen.",
						nameLabel: "Name",
						nameInvalid: "Die Community-Zeichenfolge darf keine Anführungszeichen enthalten.",
						hostRestrictionLabel: "Hosteinschränkung",
						hostRestrictionTooltip: "Sofern angegeben, schränkt diese Option den Zugriff auf die angegebenen Hosts ein. " +
								"Wenn Sie dieses Feld leer lassen, ermöglichen Sie die Kommunikation mit allen IP-Adressen." +
								"Geben Sie eine durch Kommas getrennte Liste mit Hostnamen oder Adressen an. " +
								"Wenn Sie ein Teilnetz angeben möchten, verwenden Sie eine CIDR-IP-Adresse oder einen CIDR-Hostnamen (Classless Inter-Domain Routing). ",
						hostRestrictionInvalid: "Das Feld Hosteinschränkung muss leer sein oder eine durch Kommas getrennte Liste mit gültigen IP-Adressen, Hostnamen oder Domänen enthalten. "
					},
					getCommunitiesError: "Bei Abrufen der SNMP-Communitys ist ein Fehler aufgetreten. ",
					saveCommunityError: "Beim Speichern der SNMP-Community ist ein Fehler aufgetreten.",
					deleteCommunityError: "Beim Löschen der SNMP-Community ist ein Fehler aufgetreten."					
				},
				trapSubscribers: {
					title: "Trapsubskribenten",
					tagline: "Trapsubskribenten stellen die SNMP-Clients dar, die für die Kommunikation mit dem auf dem Server integrierten SNMP-Agenten verwendet werden. " +
							"SNMP-Trapsubskribenten müssen vorhandene SNMP-Clients sein. " +
							"Konfigurieren Sie Ihren SNMP-Client so, dass er die Traps vor dem Erstellen des Trapsubskribenten empfängt.",
					grid: {
						hostColumn: "Host",
						portColumn: "Client-Portnummer",
						communityColumn: "Community"
					},
					dialog: {
						addTitle: "Trapsubskribenten hinzufügen",
						editTitle: "Trapsubskribenten bearbeiten",
						instruction: "Ein Trapsubskribent stellt einen SNMP-Client dar, der für die Kommunikation mit dem auf dem Server integrierten SNMP-Agenten verwendet wird.",
						deleteTitle: "Trapsubskribenten löschen",
						deletePrompt: "Möchten Sie diesen Trapsubskribenten wirklich löschen?",
						hostLabel: "Host",
						hostTooltip: "Die IP-Adresse, an der der SNMP-Client für Trapinformationen empfangsbereit ist. ",
						hostInvalid: "Sie müssen eine gültige IP-Adresse angeben.",
				        duplicateHost: "Es ist bereits ein Trapsubskribent für diesen Host vorhanden.",
						portLabel: "Client-Portnummer",
						portTooltip: "Der Port, an dem der SNMP-Client für Trapinformationen empfangsbereit ist." +
								"Der gültige Portbereich ist 1 bis 65535 einschließlich.",
						portInvalid: "Sie müssen eine Portnummer zwischen 1 und 65535 einschließlich eingeben.",
						communityLabel: "Community",
	                    noCommunitiesTitle: "Keine Communitys",
	                    noCommunitiesDetails: "Ohne vorherige Definition einer Community kann kein Trapsubskribent definiert werden."						
					},
					getTrapSubscribersError: "Beim Abrufen der SNMP-Trapsubskribenten ist ein Fehler aufgetreten.",
					saveTrapSubscriberError: "Beim Speichern des SNMP-Trapsubskribenten ist ein Fehler aufgetreten.",
					deleteTrapSubscriberError: "Beim Löschen  des SNMP-Trapsubskribenten ist ein Fehler aufgetreten."										
				},
				okButton: "OK",
				saveButton: "Speichern",
				cancelButton: "Abbrechen",
				closeButton: "Schließen"
			}
		}
});
