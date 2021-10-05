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
		// Home
		// ------------------------------------------------------------------------
		home : {
			title : "Startseite",
			taskContainer : {
				title : "Allgemeine Konfigurations- und Anpassungsaufgaben",
				tagline: "Quick Links zu allgemeinen Verwaltungsaufgaben.",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "({0} Aufgaben verbleibend)",
				links : {
					restore : "Aufgaben wiederherstellen",
					restoreTitle: "Stellt geschlossene Aufgaben im Abschnitt mit den allgemeinen Konfigurations- und Anpassungsaufgaben wieder her.",
					hide : "Abschnitt ausblenden",
					hideTitle: "Blendet den Abschnitt mit den allgemeinen Konfigurations- und Anpassungsaufgaben aus. " +
							"Zum Wiederherstellen des Abschnitts wählen Sie im Hilfemenü auf der Startseite die Option Aufgaben wiederherstellen aus."						
				}
			},
			tasks : {
				messagingTester : {
					heading: "Konfiguration mit der ${IMA_PRODUCTNAME_SHORT}-Beispielanwendung Messaging Tester überprüfen",
					description: "Messaging Tester ist eine einfache HTML5-Beispielanwendung, die MQTT über WebSockets verwendet, um zu überprüfen, ob der Server ordnungsgemäß konfiguriert ist.",
					links: {
						messagingTester: "Beispielanwendung Messaging Tester"
					}
				}, 
//				applianceSettings : {
//					heading : "Customize server settings",
//					description : "Configure Ethernet interfaces and domain name servers. " +
//							      "You can also customize the server locale, date and time settings, and set the node name.",
//					links : {
//						networkSettings : "Network Settings",
//						timeAndDate : "Locale, Date and Time Settings",
//						hostname: "System Control"							
//					}
//				},
				users : {
					heading : "Benutzer erstellen",
					description : "Erteilen Sie Benutzern Zugriff auf die Webbenutzerschnittstelle.",
					links : {
						users : "Benutzer der Webbenutzerschnittstelle",
						userGroups : "LDAP-Konfiguration"						
					}
				},
				connections: {
					heading: "${IMA_PRODUCTNAME_FULL} für Verbindungsannahme konfigurieren",
					description: "Definieren Sie einen Nachrichtenhub für die Verwaltung von Serververbindungen. Konfigurieren Sie MQ Connectivity, um ${IMA_PRODUCTNAME_FULL} mit MQ-Warteschlangenmanagern zu verbinden. Konfigurieren Sie LDAP für die Authentifizierung von Messaging-Benutzern.",
					links: {
						listeners: "Nachrichtenhubs",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "Umgebung sichern",
					description : "Steuern Sie die Schnittstelle und den Port, an denen die Webbenutzerschnittstelle empfangsbereit ist. Importieren Sie Schlüssel und Zertifikate, um die Verbindungen zum Server zu sichern.",
					links : {
						keysAndCerts : "Serversicherheit",
						webuiSettings : "Einstellungen für die Webbenutzerschnittstelle"
					}
				}
			},
			dashboards: {
				tagline: "Eine Übersicht über die Serververbindungen und die Systemleistung.",
				monitoringNotAvailable: "Wegen des Status des ${IMA_PRODUCTNAME_SHORT}-Servers ist keine Überwachung verfügbar.",
				flex: {
					title: "Server-Dashboard",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "Server-Dashboard für {0}",
					quickStats: {
						title: "Schnellstatistik"
					},
					liveCharts: {
						title: "Aktive Verbindungen und Durchsatz"
					},
					applianceResources: {
						title: "Serverhauptspeicherbelegung"
					},
					resourceDetails: {
						title: "Speicherbelegung der Speicherpartition"
					},
					diskResourceDetails: {
						title: "Plattenbelegung"
					},
					activeConnectionsQS: {
						title: "Anzahl aktiver Verbindungen",
						description: "Die Anzahl aktiver Verbindungen.",
						legend: "Aktive Verbindungen"
					},
					throughputQS: {
						title: "Aktueller Durchsatz",
						description: "Der aktuelle Durchsatz in Nachrichten pro Sekunde.",
						legend: "Nachrichten/Sekunde"
					},
					uptimeQS: {
						title: "Betriebszeit von ${IMA_PRODUCTNAME_FULL}",
						description: "Der Status und die Betriebszeit von ${IMA_PRODUCTNAME_FULL}.",
						legend: "Betriebszeit von ${IMA_PRODUCTNAME_SHORT}"
					},
					applianceQS: {
						title: "Serverressourcen",
						description: "Die Prozentsätze für den belegten persistenten Plattenspeicherplatz und Serverhauptspeicher.",
						bars: {
							pMem: {
								label: "Persistenter Hauptspeicher:"								
							},
							disk: { 
								label: "Platte:",
								warningThresholdText: "Die Ressourcenbelegung ist größer-gleich dem Warnungsschwellenwert.",
								alertThresholdText: "Die Ressourcenbelegung ist größer-gleich dem Alertschwellenwert."
							},
							mem: {
								label: "Hauptspeicher:",
								warningThresholdText: "Die Hauptspeicherbelegung ist hoch. Wenn die Hauptspeicherbelegung 85 % erreicht, werden Veröffentlichungen zurückgewiesen und Verbindungen unter Umständen gelöscht.",
								alertThresholdText: "Die Hauptspeicherbelegung ist zu hoch. Veröffentlichungen werden zurückgewiesen und Verbindungen unter Umständen gelöscht."	
							}
						},
						warningThresholdAltText: "Warnsymbol",
						alertThresholdText: "Die Ressourcenbelegung ist größer-gleich dem Alertschwellenwert.",
						alertThresholdAltText: "Alertsymbol"
					},
					connections: {
						title: "Verbindungen",
						description: "Snapshot der Verbindungen, der ungefähr alle fünf Sekunden erstellt wird.",
						legend: {
							x: "Zeit",
							y: "Verbindungen"
						}
					},
					throughput: {
						title: "Durchsatz",
						description: "Snapshot der Nachrichten pro Sekunde, der ungefähr alle fünf Sekunden erstellt wird.",
						legend: {
							x: "Zeit",						
							y: "Nachrichten/Sekunde",
							title: "Nachrichten/Sekunde",
							incoming: "Eingehend",
							outgoing: "Abgehend",
							hover: {
								incoming: "Snapshot der pro Sekunde von Clients gelesenen Nachrichten, der ungefähr alle fünf Sekunden erstellt wird.",
								outgoing: "Snapshot der pro Sekunde auf Clients geschriebenen Nachrichten, der ungefähr alle fünf Sekunden erstellt wird."
							}
						}
					},
					memory: {
						title: "Hauptspeicher",
						description: "Snapshot der Hauptspeicherbelegung, der ungefähr alle fünf Sekunden erstellt wird.",
						legend: {
							x: "Zeit",
							y: "Belegter Hauptspeicher (%)"
						}
					},
					disk: {
						title: "Platte",
						description: "Snapshot der Belegung des persistenten Plattenspeichers, der ungefähr alle fünf Sekunden erstellt wird.",
						legend: {
							x: "Zeit",
							y: "Belegter Plattenspeicherplatz (%)"
						}						
					},
					memoryDetail: {
						title: "Details der Hauptspeicherbelegung",
						description: "Snapshot der Hauptspeicherbelegung, der für die wichtigsten Messaging-Ressourcen ungefähr minütlich erstellt wird.",
						legend: {
							x: "Zeit",
							y: "Belegter Hauptspeicher (%)",
							title: "Belegung des Messaging-Hauptspeichers",
							system: "Server-Host-System",
							messagePayloads: "Nachrichtennutzdaten",
							publishSubscribe: "Publish/Subscribe",
							destinations: "Ziele",
							currentActivity: "Aktuelle Aktivität",
							clientStates: "Clientstatus",
							hover: {
								system: "Für andere Systemzwecke wie Betriebssystem, Verbindungen, Sicherheit, Protokollierung und Verwaltung reservierter Hauptspeicher.",
								messagePayloads: "Für Nachrichten reservierter Hauptspeicher. " +
										"Wenn eine Nachricht für mehrere Subskribenten veröffentlicht wird, wird nur eine einzige Kopie der Nachricht im Hauptspeicher reserviert. " +
										"Hauptspeicher für aufbewahrte Nachrichten werden ebenfalls in dieser Kategorie reserviert. " +
										"Workloads, die ein großes Nachrichtenvolumen im Server für nicht verbundene oder langsame Konsumenten puffern, und Workloads, die sehr viele aufbewahrte Nachrichten verwenden, können sehr viel Hauptspeicher für Nachrichtennutzdaten konsumieren.",
								publishSubscribe: "Hauptspeicher für die Durchführung des Publish/Subscribe-Messagings. " +
										"Der Server reserviert Hauptspeicher in dieser Kategorie, um aufbewahrte Nachrichten und Subskriptionen zu verfolgen. " +
										"Der Server verwaltet aus Leistungsaspekten Caches mit Publish/Subscribe-Informationen. " +
										"Workloads, die sehr viele Subskriptionen verwenden, und Workloads, die sehr viele aufbewahrte Nachrichten verwenden, können sehr viel Publish/Subscribe-Hauptspeicher konsumieren.",
								destinations: "Für Messaging-Ziele reservierter Hauptspeicher. " +
										"Diese Kategorie von Hauptspeicher wird verwendet, um Nachrichten in den Warteschlangen und Subskriptionen, die von Clients verwendet werden, zu organisieren. " +
										"Workloads, die sehr viele Nachrichten im Server puffern, und Workloads, die sehr viele Subskriptionen verwenden, können sehr viel Hauptspeicher für Ziele konsumieren.",
								currentActivity: "Für aktuelle Aktivitäten reservierter Hauptspeicher. " +
										"Zu den aktuellen Aktivitäten gehören Sitzungen, Transaktionen und Nachrichtenbestätigungen. " +
										"Informationen für Überwachungsanforderungen gehören ebenfalls zu dieser Kategorie. " +
										"Komplexe Workloads mit sehr vielen verbundenen Clients und Workloads, die Features wie Transaktionen oder nicht bestätigte Nachrichten ausgiebig nutzen, können sehr viel Hauptspeicher für aktuelle Aktivitäten konsumieren. ",
								clientStates: "Für verbundene und nicht verbundene Clients reservierter Hauptspeicher. " +
										"Der Server reserviert für jeden verbundenen Client Hauptspeicher in dieser Kategorie. " +
										"In MQTT müssen Clients, die die Verbindung mit der Einstellung CleanSession=0 hergestellt haben, auch nach der Verbindungstrennung weiterhin beachtet werden. " +
										"Außerdem wird Hauptspeicher aus dieser Kategorie reserviert, um eingehende und abgehende Nachrichtenbestätigungen in MQTT zu verfolgen. " +
										"Workloads, die sehr viele verbundene und nicht verbundene Clients verwenden, können sehr viel Clienthauptspeicher konsumieren. Dies gilt insbesondere dann, wenn Nachrichten mit hoher Servicequalität verwendet werden. "								
							}
						}
					},
					storeMemory: {
						title: "Details zur Belegung des persistenten Hauptspeichers",
						description: "Snapshot der Belegung des persistenten Hauptspeichers, der ungefähr minütlich erstellt wird.",
						legend: {
							x: "Zeit",
							y: "Belegter Hauptspeicher (%)",
							pool1Title: "Hauptspeicherbelegung von Pool 1",
							pool2Title: "Hauptspeicherbelegung von Pool 2",
							system: "Server-Host-System",
							IncomingMessageAcks: "Eingehende Nachrichtenbestätigungen",
							MQConnectivity: "MQ Connectivity",
							Transactions: "Transaktionen",
							Topics: "Topics mit aufbewahrten Nachrichten",
							Subscriptions: "Subskriptionen",
							Queues: "Warteschlangen",										
							ClientStates: "Clientstatus",
							recordLimit: "Grenzwert für Datensätze",
							hover: {
								system: "Vom System reservierter oder belegter Hauptspeicher der Speicherpartition",
								IncomingMessageAcks: "Für die Bestätigung eingehender Nachrichten reservierter Hauptspeicher der Speicherpartition. " +
										"Der Server reserviert Hauptspeicher in dieser Kategorie für MQTT-Clients, die mit der Einstellung CleanSession=0 verbunden sind und Nachrichten mit der Servicequalität 2 veröffentlichen. " +
										"Dieser Hauptspeicher wird verwendet, um eine einmalige Zustellung zu gewährleisten.",
								MQConnectivity: "Der für die Konnektivität mit IBM WebSphere MQ-Warteschlangenmanagern reservierte Hauptspeicher der Speicherpartition.",
								Transactions: "Für Transaktionen reservierter Hauptspeicher der Speicherpartition. " +
										"Der Server reserviert Hauptspeicher in der Kategorie für jede Transaktion, damit er beim Neustart des Servers eine Wiederherstellung durchführen kann.",
								Topics: "Für Topics reservierter Hauptspeicher der Speicherpartition. " +
										"Der Server reserviert Hauptspeicher in der Kategorie für jedes Topic mit aufbewahrten Nachrichten. ",
								Subscriptions: "Für permanente Subskriptionen reservierter Hauptspeicher der Speicherpartition. " +
										"In MQTT sind diese Subskriptionen für Clients bestimmt, die mit der Einstellung CleanSession=0 verbunden sind.",
								Queues: "Für Warteschlangen reservierter Hauptspeicher der Speicherpartition. " +
										"Hauptspeicher in dieser Kategorie wird für jede Warteschlange reserviert, die für Punkt-zu-Punkt-Messaging erstellt wird.",										
								ClientStates: "Der für Clients reservierte Hauptspeicher der Speicherpartition muss auch nach der Verbindungstrennung weiterhin beachtet werden. " +
										"In MQTT handelt es sich dabei um Clients, die mit der Einstellung CleanSession=0 verbunden sind, oder um Clients, die verbunden sind und eine Will-Nachricht mit der Servicequalität 1 oder 2 definieren.",
								recordLimit: "Nach dem Erreichen des Datensatzgrenzwerts können keine Datensätze mehr für aufbewahrte Nachrichten, permanente Subskriptionen, Warteschlangen, Clients und Konnektivität mit WebSphere MQ-Warteschlangenmanagern erstellt werden."
							}
						}						
					},					
					peakConnectionsQS: {
						title: "Maximale Verbindungsanzahl",
						description: "Die maximale Anzahl aktiver Verbindungen im angegebenen Zeitraum.",
						legend: "Maximale Verbindungsanzahl"
					},
					avgConnectionsQS: {
						title: "Durchschnittliche Anzahl an Verbindungen",
						description: "Die durchschnittliche Anzahl aktiver Verbindungen im angegebenen Zeitraum.",
						legend: "Durchschnittliche Verbindungsanzahl"
					},
					peakThroughputQS: {
						title: "Spitzendurchsatz",
						description: "Die maximale Nachrichtenanzahl pro Sekunde im angegebenen Zeitraum.",
						legend: "Maximale Nachrichtenanzahl/Sekunde"
					},
					avgThroughputQS: {
						title: "Durchschnittlicher Durchsatz",
						description: "Die durchschnittliche Anzahl an Nachrichten pro Sekunde im angegebenen Zeitraum.",
						legend: "Durchschnittliche Nachrichtenanzahl/Sekunde"
					}
				}
			},
			status: {
				rc0: "Initialisierung wird durchgeführt",
				rc1: "Aktiv (Produktion)",
				rc2: "Stoppoperation wird ausgeführt",
				rc3: "Initialisiert",
				rc4: "Transport gestartet",
				rc5: "Protokoll gestartet",
				rc6: "Speicher gestartet",
				rc7: "Engine gestartet",
				rc8: "Messaging gestartet",
				rc9: "Aktiv (Wartung)",
				rc10: "Standby",
				rc11: "Speicher wird gestartet",
				rc12: "Wartung (Speicherbereinigung in Bearbeitung)",
				rc99: "Gestoppt",				
				unknown: "Unbekannt",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "Wenn der Server erneut gestartet wird, befindet er sich im Modus <em>{0}</em>. ",
				cleanStoreSelected: "Die Aktion <em>Speicherbereinigung</em> wurde angefordert. Sie wird beim Neustart des Servers ausgeführt.",
				mode_0: "Produktion",
				mode_1: "Wartung",
				mode_2: "Speicherbereinigung",
				adminError: "${IMA_PRODUCTNAME_FULL} hat einen Fehler erkannt.",
				adminErrorDetails: "Der Fehlercode ist {0}. Die Fehlerzeichenfolge ist {1}."
			},
			storeStatus: {
				mode_0: "Persistent",
				mode_1: "Nur Hauptspeicher"
			},
			memoryStatus: {
				ok: "OK",
				unknown: "Unbekannt",
				error: "Bei der Hauptspeicherprüfung wurde ein Fehler gemeldet.",
				errorMessage: "Bei der Hauptspeicherprüfung wurde ein Fehler gemeldet. Wenden Sie sich an den Support."
			},
			role: {
				PRIMARY: "Primärer Knoten",
				PRIMARY_SYNC: "Primärer Knoten (Synchronisierung {0} % abgeschlossen)",
				STANDBY: "Standby-Knoten",
				UNSYNC: "Synchronisation des Knotens nicht möglich",
				TERMINATE: "Standby-Knoten vom primären Knoten beendet",
				UNSYNC_ERROR: "Knoten kann nicht mehr synchronisiert werden",
				HADISABLED: "Inaktiviert",
				UNKNOWN: "Unbekannt",
				unknown: "Unbekannt"
			},
			haReason: {
				"1": "Die Speicherkonfigurationen der beiden Knoten lassen eine Zusammenarbeit als Hochverfügbarkeitspaar nicht zu. " +
				     "Das nicht übereinstimmende Konfigurationselement ist {0}.",
				"2": "Die Erkennungszeit ist abgelaufen. Die Kommunikation mit dem anderen Knoten im Hochverfügbarkeitspaar ist fehlgeschlagen.",
				"3": "Es wurden zwei primäre Knoten identifiziert.",
				"4": "Es wurden zwei nicht synchronisierte Knoten identifiziert.",
				"5": "Es kann nicht festgestellt werden, welcher Knoten der primäre Knoten sein soll, weil zwei Knoten nicht leere Speicher haben.",
				"7": "Der Knoten konnte nicht mit dem fernen Knoten gepaart werden, weil die beiden Knoten unterschiedliche Gruppen-IDs haben.",
				"9": "Es ist ein Fehler im Hochverfügbarkeitsserver oder ein interner Fehler aufgetreten.",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "Dieser Knoten war am {0} der primäre Knoten."
			},
			statusControl: {
				label: "Status",
				ismServer: "${IMA_PRODUCTNAME_FULL}-Server:",
				serverNotAvailable: "Bitten Sie Ihren Serversystemadministrator, den Server zu starten.",
				serverNotAvailableNonAdmin: "Bitten Sie Ihren Serversystemadministrator, den Server zu starten.",
				haRole: "Hochverfügbarkeit:",
				pending: "Anstehend..."
			}
		},
		recordsLabel: "Datensätze",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "Cluster:",
		clusterStatus_Active: "Aktiv",
		clusterStatus_Inactive: "Inaktiv",
		clusterStatus_Removed: "Entfernt",
		clusterStatus_Connecting: "Verbindung wird hergestellt",
		clusterStatus_None: "Nicht konfiguriert",
		clusterStatus_Unknown: "Unbekannt",
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "Fern",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "Lokal",
		clusterLegendIncoming: "Eingehend",
		clusterLegendOutgoing: "Abgehend",
		clusterHoverIncoming: "Snapshot der pro Sekunde von fernen Cluster-Membern gelesenen Nachrichten, der ungefähr alle fünf Sekunden erstellt wird.",
		clusterHoverOutgoing: "Snapshot der pro Sekunde auf ferne Cluster-Member geschriebenen Nachrichten, der ungefähr alle fünf Sekunden erstellt wird."

});
