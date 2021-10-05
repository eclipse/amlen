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
		// Messaging
		// ------------------------------------------------------------------------
		messaging : {
			title : "Messaging",
			users: {
				title: "Benutzerauthentifizierung",
				tagline: "Konfigurieren Sie LDAP für die Authentifizierung von Messaging-Server-Benutzern."
			},
			endpoints: {
				title: "Endpunktkonfiguration",
				listenersSubTitle: "Endpunkte",
				endpointsSubTitle: "Endpunkt definieren",
		 		form: {
					enabled: "Aktiviert",
					name: "Name",
					description: "Beschreibung",
					port: "Port",
					ipAddr: "IP-Adresse",
					all: "Alle",
					protocol: "Protokoll",
					security: "Sicherheit",
					none: "Ohne",
					securityProfile: "Sicherheitsprofil",
					connectionPolicies: "Verbindungsrichtlinien",
					connectionPolicy: "Verbindungsrichtlinie",
					messagingPolicies: "Messaging-Richtlinien",
					messagingPolicy: "Messaging-Richtlinie",
					destinationType: "Zieltyp",
					destination: "Ziel",
					maxMessageSize: "Maximale Nachrichtengröße",
					selectProtocol: "Protokoll auswählen",
					add: "Hinzufügen",
					tooltip: {
						description: "",
						port: "Geben Sie einen verfügbaren Port ein. Der gültige Portbereich ist 1 bis 65535 einschließlich.",
						connectionPolicies: "Fügen Sie mindestens eine Verbindungsrichtlinie hinzu. Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zum Endpunkt. " +
								"Verbindungsrichtlinien werden in der angezeigten Reihenfolge ausgewertet. Zum Ändern der Reihenfolge verwenden Sie den Aufwärtspfeil und den Abwärtspfeil in der Symbolleiste. ",
						messagingPolicies: "Fügen Sie mindestens eine Messaging-Richtlinie hinzu. " +
								"Eine Messaging-Richtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Topics und Warteschlangen in ${IMA_PRODUCTNAME_FULL}. " +
								"Messaging-Richtlinien werden in der angezeigten Reihenfolge ausgewertet. Zum Ändern der Reihenfolge verwenden Sie den Aufwärtspfeil und den Abwärtspfeil in der Symbolleiste. ",									
						maxMessageSize: "Die maximale Nachrichtengröße in KB. Die gültigen Werte sind 1 bis 262.144 einschließlich.",
						protocol: "Geben Sie gültige Protokolle für diesen Endpunkt an."
					},
					invalid: {						
						port: "Sie müssen eine Portnummer zwischen 1 und 65535 einschließlich eingeben.",
						maxMessageSize: "Die maximale Nachrichtengröße muss eine Zahl zwischen 1 und 262.144 einschließlich sein.",
						ipAddr: "Sie müssen eine gültige IP-Adresse angeben.",
						security: "Es muss ein Wert eingegeben werden."
					},
					duplicateName: "Es ist bereits ein Datensatz mit diesem Namen vorhanden."
				},
				addDialog: {
					title: "Endpunkt hinzufügen",
					instruction: "Ein Endpunkt ist ein Port, zu dem Clientanwendungen eine Verbindung herstellen können. Ein Endpunkt muss mindestens eine Verbindungsrichtlinie und eine Messaging-Richtlinie haben."
				},
				removeDialog: {
					title: "Endpunkt löschen",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
				editDialog: {
					title: "Endpunkt bearbeiten",
					instruction: "Ein Endpunkt ist ein Port, zu dem Clientanwendungen eine Verbindung herstellen können. Ein Endpunkt muss mindestens eine Verbindungsrichtlinie und eine Messaging-Richtlinie haben."
				},
				addProtocolsDialog: {
					title: "Protokolle zum Endpunkt hinzufügen",
					instruction: "Fügen Sie die Protokolle hinzu, über die eine Verbindung zu diesem Endpunkt hergestellt werden darf. Sie müssen mindestens ein Protokoll auswählen.",
					allProtocolsCheckbox: "Es sind alle Protokolle für diesen Endpunkt gültig.",
					protocolSelectErrorTitle: "Es wurde kein Protokoll ausgewählt.",
					protocolSelectErrorMsg: "Sie müssen mindestens ein Protokoll auswählen. Geben Sie an, dass alle Protokolle hinzugefügt werden sollen, oder wählen Sie bestimmte Protokolle in der Protokollliste aus."
				},
				addConnectionPoliciesDialog: {
					title: "Verbindungsrichtlinien zum Endpunkt hinzufügen",
					instruction: "Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zu ${IMA_PRODUCTNAME_FULL}-Endpunkten. " +
						"Jeder Endpunkt muss mindestens eine Verbindungsrichtlinie haben."
				},
				addMessagingPoliciesDialog: {
					title: "Messaging-Richtlinien zum Endpunkt hinzufügen",
					instruction: "Eine Messaging-Richtlinie ermöglicht Ihnen zu steuern, auf welche Topics, Warteschlangen oder globalen gemeinsam genutzten Subskriptionen ein Client in ${IMA_PRODUCTNAME_FULL} zugreifen kann. " +
							"Jeder Endpunkt muss mindestens eine Messaging-Richtlinie haben. " +
							"Wenn ein Endpunkt eine Messaging-Richtlinie für eine globale gemeinsam genutzte Subskription hat, muss er auch eine Messaging-Richtlinie für die subskribierten Topics haben. "
				},
                retrieveEndpointError: "Beim Abrufen der Endpunkte ist ein Fehler aufgetreten.",
                saveEndpointError: "Beim Speichern des Endpunkts ist ein Fehler aufgetreten.",
                deleteEndpointError: "Beim Löschen des Endpunkts ist ein Fehler aufgetreten.",
                retrieveSecurityProfilesError: "Beim Abrufen der Sicherheitsrichtlinien ist ein Fehler aufgetreten."
			},
			connectionPolicies: {
				title: "Verbindungsrichtlinien",
				grid: {
					applied: "Angewendet",
					name: "Name"
				},
		 		dialog: {
					name: "Name",
					protocol: "Protokoll",
					description: "Beschreibung",
					clientIP: "Client-IP-Adresse",  
					clientID: "Client-ID",
					ID: "Benutzer-ID",
					Group: "Gruppen-ID",
					selectProtocol: "Protokoll auswählen",
					commonName: "Allgemeiner Name des Zertifikats",
					protocol: "Protokoll",
					tooltip: {
						name: "",
						filter: "Sie müssen mindestens eines der Filterfelder angeben. " +
								"Ein einzelner Stern (*) kann in den meisten Filtern als Platzhalter für 0 oder mehr Zeichen als letztes Zeichen verwendet werden. " +
								"Die Client-IP-Adresse kann einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::]."
					},
					invalid: {						
						filter: "Sie müssen mindestens eines der Filterfelder angeben. ",
						wildcard: "Ein einzelner Stern (*) kann am Ende des Werts als Platzhalter für 0 oder mehr Zeichen verwendet werden.",
						vars: "Der Wert darf die Substitutionsvariable ${UserID}, ${GroupID}, ${ClientID} oder ${CommonName} nicht enthalten.",
						clientIDvars: "Der Wert darf die Substitutionsvariable ${GroupID} oder ${ClientID} nicht enthalten.",
						clientIP: "Die Client-IP-Adresse muss einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::]."
					}										
				},
				addDialog: {
					title: "Verbindungsrichtlinie hinzufügen",
					instruction: "Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zu ${IMA_PRODUCTNAME_FULL}-Endpunkten. Jeder Endpunkt muss mindestens eine Verbindungsrichtlinie haben."
				},
				removeDialog: {
					title: "Verbindungsrichtlinie löschen",
					instruction: "Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zu ${IMA_PRODUCTNAME_FULL}-Endpunkten. Jeder Endpunkt muss mindestens eine Verbindungsrichtlinie haben.",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
                removeNotAllowedDialog: {
                	title: "Entfernen nicht zulässig",
                	content: "Die Verbindungsrichtlinie kann nicht entfernt werden, weil sie von mindestens einem Endpunkt verwendet wird." +
                			 "Entfernen Sie die Verbindungsrichtlinie aus allen Endpunkten und wiederholen Sie dann die Operation.",
                	closeButton: "Schließen"
                },								
				editDialog: {
					title: "Verbindungsrichtlinie bearbeiten",
					instruction: "Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zu ${IMA_PRODUCTNAME_FULL}-Endpunkten. Jeder Endpunkt muss mindestens eine Verbindungsrichtlinie haben."
				},
                retrieveConnectionPolicyError: "Beim Abrufen der Verbindungsrichtlinien ist ein Fehler aufgetreten.",
                saveConnectionPolicyError: "Beim Speichern der Verbindungsrichtlinie ist ein Fehler aufgetreten.",
                deleteConnectionPolicyError: "Beim Löschen der Verbindungsrichtlinie ist ein Fehler aufgetreten."
 			},
			messagingPolicies: {
				title: "Messaging-Richtlinien",
				listSeparator : ",",
		 		dialog: {
					name: "Name",
					description: "Beschreibung",
					destinationType: "Zieltyp",
					destinationTypeOptions: {
						topic: "Topic",
						subscription: "Globale gemeinsam genutzte Subskription",
						queue: "Warteschlange"
					},
					topic: "Topic",
					queue: "Warteschlange",
					selectProtocol: "Protokoll auswählen",
					destination: "Ziel",
					maxMessages: "Maximale Nachrichtenanzahl",
					maxMessagesBehavior: "Verhalten bei maximaler Nachrichtenanzahl",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "Neue Nachrichten zurückweisen",
						DiscardOldMessages: "Alte Nachrichten verwerfen"
					},
					action: "Berechtigung",
					actionOptions: {
						publish: "Veröffentlichen",
						subscribe: "Subskribieren",
						send: "Senden",
						browse: "Durchsuchen",
						receive: "Empfangen",
						control: "Steuern"
					},
					clientIP: "Client-IP-Adresse",  
					clientID: "Client-ID",
					ID: "Benutzer-ID",
					Group: "Gruppen-ID",
					commonName: "Allgemeiner Name des Zertifikats",
					protocol: "Protokoll",
					disconnectedClientNotification: "Benachrichtigung über nicht verbundenen Client",
					subscriberSettings: "Subskribenteneinstellungen",
					publisherSettings: "Publishereinstellungen",
					senderSettings: "Sendereinstellungen",
					maxMessageTimeToLive: "Maximale Nachrichtenlebensdauer",
					unlimited: "Unbegrenzt",
					unit: "Sekunden",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} Sekunden",
					tooltip: {
						name: "",
						destination: "Nachrichtentopic, Nachrichtenwarteschlange oder globale gemeinsam genutzte Nachrichtensubskription, für das bzw. die die Richtlinie gilt. Verwenden Sie den Stern (*) mit Vorsicht. " +
							"Ein Stern dient als Platzhalter für 0 oder mehr Zeichen, einschließlich des Schrägstrichs (/). Deshalb kann der Stern für mehrere Ebenen in einer Topicverzeichnisstruktur stehen.",
						maxMessages: "Die maximale Anzahl an Nachrichten, die für eine Subskription aufbewahrt werden sollen. Sie müssen eine Zahl zwischen 1 und 20.000.000 angeben.",
						maxMessagesBehavior: "Das Verhalten, das angewendet wird, wenn der Puffer für eine Subskription voll ist, " +
								"d. h., wenn die die Anzahl der Nachrichten im Puffer für eine Subskription den Wert des Felds Maximale Nachrichtenanzahl erreicht. <br />" +
								"<strong>Neue Nachrichten zurückweisen:&nbsp;</strong>  Während der Puffer voll ist, werden neue Nachrichten zurückgewiesen.<br />" +
								"<strong>Alte Nachrichten verwerfen:&nbsp;</strong> Wenn der Puffer voll ist und eine neue Nachricht ankommt, werden die ältesten nicht zugestellten Nachrichten verworfen. ",
						discardOldestMessages: "Diese Einstellung kann dazu führen, dass einige Nachrichten den Subskribenten nicht zugestellt werden, selbst wenn der Publisher eine Zustellbestätigung empfängt. " +
								"Die Nachrichten werden auch dann verworfen, wenn der Publisher und der Subskribent Reliable Messaging angefordert haben. ",
						action: "Von dieser Richtlinie zugelassene Aktionen",
						filter: "Sie müssen mindestens eines der Filterfelder angeben. " +
								"Ein einzelner Stern (*) kann in den meisten Filtern als Platzhalter für 0 oder mehr Zeichen als letztes Zeichen verwendet werden. " +
								"Die Client-IP-Adresse kann einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::].",
				        disconnectedClientNotification: "Gibt an, ob Benachrichtigungen für nicht verbundene MQTT-Clients beim Eingang einer Nachricht veröffentlicht werden. " +
				        		"Benachrichtigungen werden nur für MQTT-Clients mit der Einstellung CleanSession=0 aktiviert.",
				        protocol: "Aktiviert die Filterung von Protokollen für die Messaging-Richtlinie. Wenn Sie diese Einstellung aktivieren, müssen Sie mindestens ein Protokoll angeben. Wenn Sie diese Einstellung nicht aktivieren, werden alle Protokolle für die Messaging-Richtlinie zugelassen. ",
				        destinationType: "Der Typ des Ziels, auf den der Zugriff zugelassen werden soll. " +
				        		"Wenn Sie den Zugriff auf eine globale gemeinsam genutzte Subskription zulassen möchten, sind zwei Messaging-Richtlinien erforderlich: "+
				        		"<ul><li>Eine Messaging-Richtlinie mit dem Zieltyp <em>Topic</em>, die den Zugriff auf mindestens ein Topic zulässt.</li>" +
				        		"<li>Eine Messaging-Richtlinie mit dem Zieltyp <em>Globale gemeinsam genutzte Subskription</em>, die den Zugriff auf die globale gemeinsam genutzte und permanente Subskription in diesen Topics zulässt.</li></ul>",
						action: {
							topic: "<dl><dt>Veröffentlichen:</dt><dd>Ermöglicht Clients, Nachrichten in dem in der Messaging-Richtlinie angegebenen Topic zu veröffentlichen.</dd>" +
							       "<dt>Subskribieren:</dt><dd>Ermöglicht Clients, das in der Messaging-Richtlinie angegebenen Topic zu subskribieren.</dd></dl>",
							queue: "<dl><dt>Senden:</dt><dd>Ermöglicht Clients, Nachrichten an die in der Messaging-Richtlinie angegebene Warteschlange zu senden.</dd>" +
								    "<dt>Anzeigen:</dt><dd>Ermöglicht Clients, die in der Messaging-Richtlinie angegebene Warteschlange anzuzeigen.</dd>" +
								    "<dt>Empfangen:</dt><dd>Ermöglicht Clients, Nachrichten aus der in der Messaging-Richtlinie angegebenen Warteschlange zu empfangen. </dd></dl>",
							subscription:  "<dl><dt>Empfangen:</dt><dd>Ermöglicht Clients, Nachrichten von der in der Messaging-Richtlinie angegebenen globalen gemeinsam genutzten Subskription zu empfangen. </dd>" +
									"<dt>steuern:</dt><dd>Ermöglicht Clients, die in der Messaging-Richtlinie angegebene globale gemeinsam genutzte Subskription zu erstellen und zu löschen. </dd></dl>"
							},
						maxMessageTimeToLive: "Die maximale Lebensdauer einer unter der Richtlinie veröffentlichten Nachricht in Sekunden." +
								"Wenn der Publisher einen kleineren Verfallswert angibt, wird der Publisherwert verwendet. " +
								"Die gültigen Werte sind <em>Unbegrenzt</em> und 1-2.147.483.647 Sekunden. Der Wert <em>Unbegrenzt</em> bedeutet, dass die Lebensdauer nicht beschränkt ist.",
						maxMessageTimeToLiveSender: "Die maximale Lebensdauer einer unter der Richtlinie gesendeten Nachricht in Sekunden." +
								"Wenn der Sender einen kleineren Verfallswert angibt, wird der Senderwert verwendet. " +
								"Die gültigen Werte sind <em>Unbegrenzt</em> und 1-2.147.483.647 Sekunden. Der Wert <em>Unbegrenzt</em> bedeutet, dass die Lebensdauer nicht beschränkt ist."
					},
					invalid: {						
						maxMessages: "Sie müssen eine Zahl zwischen 1 und 20.000.000 angeben.",                       
						filter: "Sie müssen mindestens eines der Filterfelder angeben. ",
						wildcard: "Ein einzelner Stern (*) kann am Ende des Werts als Platzhalter für 0 oder mehr Zeichen verwendet werden.",
						vars: "Der Wert darf die Substitutionsvariable ${UserID}, ${GroupID}, ${ClientID} oder ${CommonName} nicht enthalten.",
						clientIP: "Die Client-IP-Adresse muss einen Stern (*) oder eine durch Kommas begrenzte Liste mit IPv4- oder IPv6-Adressen oder -Bereichen enthalten, z. B. 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								  "IPv6-Adressen müssen in eckige Klammern eingeschlossen werden, z. B. [2001:db8::].",
					    subDestination: "Eine Variablensubstitution für die Client-ID ist nicht zulässig, wenn der Zieltyp <em>Globale gemeinsam genutzte Subskription</em> definiert ist.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "Das Protokoll/die Protokolle {0} sind für den Zieltyp {1} nicht gültig.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "Das Protokoll/die Protokolle {0}, das/die für den Protokollfilter angegeben wurde(n), sind für den Zieltyp {1} nicht gültig.",
					    maxMessageTimeToLive: "Sie müssen <em>Unbegrenzt</em> oder eine Zahl zwischen 1 und 2.147.483.647 angeben."
					}					
				},
				addDialog: {
					title: "Messaging-Richtlinie hinzufügen",
					instruction: "Eine Messaging-Richtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Topics, Warteschlangen und globale gemeinsam genutzte Subskriptionen in ${IMA_PRODUCTNAME_FULL}. " +
							     "In einer globalen gemeinsam genutzten Subskription verteilt sich die Arbeit für den Empfang von Nachrichten von einer permanenten Topicsubskription auf mehrere Subskribenten. Jeder Endpunkt muss mindestens eine Messaging-Richtlinie haben. "
				},
				removeDialog: {
					title: "Messaging-Richtlinie löschen",
					instruction: "Eine Messaging-Richtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Topics, Warteschlangen und globale gemeinsam genutzte Subskriptionen in ${IMA_PRODUCTNAME_FULL}. " +
							"In einer globalen gemeinsam genutzten Subskription verteilt sich die Arbeit für den Empfang von Nachrichten von einer permanenten Topicsubskription auf mehrere Subskribenten. Jeder Endpunkt muss mindestens eine Messaging-Richtlinie haben. ",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
                removeNotAllowedDialog: {
                	title: "Entfernen nicht zulässig",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "Die Richtlinie {0} kann nicht entfernt werden, weil sie von mindestens einem Endpunkt verwendet wird." +
                			 "Entfernen Sie die Richtlinie {0} aus allen Endpunkten und wiederholen Sie dann die Operation.",
                	closeButton: "Schließen"
                },				
				editDialog: {
					title: "Messaging-Richtlinie bearbeiten",
					instruction: "Eine Messaging-Richtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Topics, Warteschlangen und globale gemeinsam genutzte Subskriptionen in ${IMA_PRODUCTNAME_FULL}. " +
							     "In einer globalen gemeinsam genutzten Subskription verteilt sich die Arbeit für den Empfang von Nachrichten von einer permanenten Topicsubskription auf mehrere Subskribenten. Jeder Endpunkt muss mindestens eine Messaging-Richtlinie haben. "
				},
				viewDialog: {
					title: "Messaging-Richtlinie anzeigen"
				},		
				confirmSaveDialog: {
					title: "Messaging-Richtlinie speichern",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "Diese Richtlinie wird von ungefähr {0} Subskribenten oder Produzenten verwendet. " +
							"Von dieser Richtlinie berechtigte Clients verwenden die folgenden neuen Einstellungen: {1}" +
							"Möchten Sie diese Richtlinie wirklich ändern?",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "Diese Richtlinie wird möglicherweise von Subskribenten oder Produzenten verwendet. " +
							"Von dieser Richtlinie berechtigte Clients verwenden die folgenden neuen Einstellungen: {1}" +
							"Möchten Sie diese Richtlinie wirklich ändern?",
					saveButton: "Speichern",
					closeButton: "Abbrechen"
				},
                retrieveMessagingPolicyError: "Beim Abrufen der Messaging-Richtlinien ist ein Fehler aufgetreten.",
                retrieveOneMessagingPolicyError: "Beim Abrufen der Messaging-Richtlinie ist ein Fehler aufgetreten.",
                saveMessagingPolicyError: "Beim Speichern der Messaging-Richtlinie ist ein Fehler aufgetreten.",
                deleteMessagingPolicyError: "Beim Löschen der Messaging-Richtlinie ist ein Fehler aufgetreten.",
                pendingDeletionMessage:  "Diese Richtlinie wird bald gelöscht. Sie wird gelöscht, sobald sie nicht mehr im Gebrauch ist. ",
                tooltip: {
                	discardOldestMessages: "Das Verhalten bei maximaler Nachrichtenanzahl ist auf <em>Alte Nachrichten verwerfen</em> gesetzt. " +
                			"Diese Einstellung kann dazu führen, dass einige Nachrichten den Subskribenten nicht zugestellt werden, selbst wenn der Publisher eine Zustellbestätigung empfängt. " +
                			"Die Nachrichten werden auch dann verworfen, wenn der Publisher und der Subskribent Reliable Messaging angefordert haben. "
                }
			},
			messageProtocols: {
				title: "Messaging-Protokolle",
				subtitle: "Messaging-Protokolle werden verwendet, um Nachrichten zwischen Clients und ${IMA_PRODUCTNAME_FULL} zu senden.",
				tagline: "Verfügbare Messaging-Protokolle und deren Funktionalität.",
				messageProtocolsList: {
					name: "Protokollname",
					topic: "Topic",
					shared: "Globale gemeinsam genutzte Subskription",
					queue: "Warteschlange",
					browse: "Durchsuchen"
				}
			},
			messageHubs: {
				title: "Nachrichtenhubs",
				subtitle: "Systemadministratoren und Messaging-Administratoren können Nachrichtenhubs definieren, bearbeiten und löschen." +
						  "Ein Nachrichtenhub ist ein organisatorisches Konfigurationsobjekt, das zusammengehörige Endpunkte, Verbindungsrichtlinien und Messaging-Richtlinien gruppiert. <br /><br />" +
						  "Ein Endpunkt ist ein Port, zu dem Clientanwendungen eine Verbindung herstellen können. Ein Endpunkt muss mindestens eine Verbindungsrichtlinie und eine Messaging-Richtlinie haben." +
						  "Eine Verbindungsrichtlinie berechtigt Clients zur Herstellung einer Verbindung zum Endpunkt, während die Messaging-Richtlinie Clients zur Ausführung bestimmter Messaging-Aktionen berechtigt, sobald diese mit dem Endpunkt verbunden sind. ",
				tagline: "Definieren, bearbeiten oder löschen Sie einen Nachrichtenhub.",
				defineAMessageHub: "Nachrichtenhub definieren",
				editAMessageHub: "Nachrichtenhub bearbeiten",
				defineAnEndpoint: "Endpunkt definieren",
				endpointTabTagline: "Ein Endpunkt ist ein Port, zu dem Clientanwendungen eine Verbindung herstellen können. " +
						"Ein Endpunkt muss mindestens eine Verbindungsrichtlinie und eine Messaging-Richtlinie haben.",
				messagingPolicyTabTagline: "Eine Messaging-Richtlinie ermöglicht Ihnen zu steuern, auf welche Topics, Warteschlangen oder globalen gemeinsam genutzten Subskriptionen ein Client in ${IMA_PRODUCTNAME_FULL} zugreifen kann. " +
						"Jeder Endpunkt muss mindestens eine Messaging-Richtlinie haben. ",
				connectionPolicyTabTagline: "Eine Verbindungsrichtlinie berechtigt Clients für die Herstellung einer Verbindung zu ${IMA_PRODUCTNAME_FULL}-Endpunkten. " +
						"Jeder Endpunkt muss mindestens eine Verbindungsrichtlinie haben.",						
                retrieveMessageHubsError: "Beim Abrufen des Nachrichtenhubs ist ein Fehler aufgetreten.",
                saveMessageHubError: "Beim Speichern des Nachrichtenhubs ist ein Fehler aufgetreten.",
                deleteMessageHubError: "Beim Löschen des Nachrichtenhubs ist ein Fehler aufgetreten.",
                messageHubNotFoundError: "Der Nachrichtenhub wurde nicht gefunden. Er wurde möglicherweise von einem anderen Benutzer gelöscht.",
                removeNotAllowedDialog: {
                	title: "Entfernen nicht zulässig",
                	content: "Der Nachrichtenhub kann nicht entfernt werden, weil er mindestens einen Endpunkt enthält." +
                			 "Bearbeiten Sie den Nachrichtenhub, um die Endpunkte zu löschen, und wiederholen Sie dann die Operation.",
                	closeButton: "Schließen"
                },
                addDialog: {
                	title: "Nachrichtenhub hinzufügen",
                	instruction: "Definieren Sie einen Nachrichtenhub für die Verwaltung von Serververbindungen. ",
                	saveButton: "Speichern",
                	cancelButton: "Abbrechen",
                	name: "Name:",
                	description: "Beschreibung:"
                },
                editDialog: {
                	title: "Eigenschaften des Nachrichtenhubs bearbeiten",
                	instruction: "Bearbeiten Sie den Namen und die Beschreibung des Nachrichtenhubs."
                },                
				messageHubList: {
					name: "Nachrichtenhub",
					description: "Beschreibung",
					metricLabel: "Endpunkte",
					removeDialog: {
						title: "Nachrichtenhub löschen",
						content: "Möchten Sie diesen Nachrichtenhub wirklich löschen?"
					}
				},
				endpointList: {
					name: "Endpunkt",
					description: "Beschreibung",
					connectionPolicies: "Verbindungsrichtlinien",
					messagingPolicies: "Messaging-Richtlinien",
					port: "Port",
					enabled: "Aktiviert",
					status: "Status",
					up: "Aktiv",
					down: "Inaktiv",
					removeDialog: {
						title: "Endpunkt löschen",
						content: "Möchten Sie diesen Endpunkt wirklich löschen?"
					}
				},
				messagingPolicyList: {
					name: "Messaging-Richtlinie",
					description: "Beschreibung",					
					metricLabel: "Endpunkte",
					destinationLabel: "Ziel",
					maxMessagesLabel: "Maximale Nachrichtenanzahl",
					disconnectedClientNotificationLabel: "Benachrichtigung über nicht verbundenen Client",
					actionsLabel: "Berechtigung",
					useCountLabel: "Nutzungszähler",
					unknown: "Unbekannt",
					removeDialog: {
						title: "Messaging-Richtlinie löschen",
						content: "Möchten Sie diese Messaging-Richtlinie wirklich löschen?"
					},
					deletePendingDialog: {
						title: "Anstehende löschen",
						content: "Die Löschanforderung wurde empfangen, aber die Richtlinie kann momentan nicht gelöscht werden. " +
							"Die Richtlinie wird von ungefähr {0} Subskribenten oder Produzenten verwendet. " +
							"Die Richtlinie wird gelöscht, sobald sie nicht mehr im Gebrauch ist. ",
						contentUnknown: "Die Löschanforderung wurde empfangen, aber die Richtlinie kann momentan nicht gelöscht werden. " +
						"Die Richtlinie wird möglicherweise von Subskribenten oder Produzenten verwendet. " +
						"Die Richtlinie wird gelöscht, sobald sie nicht mehr im Gebrauch ist. ",
						closeButton: "Schließen"
					},
					deletePendingTooltip: "Diese Richtlinie wird gelöscht, sobald sie nicht mehr im Gebrauch ist. "
				},	
				connectionPolicyList: {
					name: "Verbindungsrichtlinie",
					description: "Beschreibung",					
					endpoints: "Endpunkte",
					removeDialog: {
						title: "Verbindungsrichtlinie löschen",
						content: "Möchten Sie diese Verbindungsrichtlinie wirklich löschen?"
					}
				},				
				messageHubDetails: {
					backLinkText: "Zurück zu Nachrichtenhubs",
					editLink: "Bearbeiten",
					endpointsTitle: "Endpunkte",
					endpointsTip: "Endpunkte und Verbindungsrichtlinien für diesen Nachrichtenhub konfigurieren",
					messagingPoliciesTitle: "Messaging-Richtlinien",
					messagingPoliciesTip: "Messaging-Richtlinien für diesen Nachrichtenhub konfigurieren",
					connectionPoliciesTitle: "Verbindungsrichtlinien",
					connectionPoliciesTip: "Verbindungsrichtlinien für diesen Nachrichtenhub konfigurieren"
				},
				hovercard: {
					name: "Name",
					description: "Beschreibung",
					endpoints: "Endpunkte",
					connectionPolicies: "Verbindungsrichtlinien",
					messagingPolicies: "Messaging-Richtlinien",
					warning: "Warnung"
				}
			},
			referencedObjectGrid: {
				applied: "Angewendet",
				name: "Name"
			},
			savingProgress: "Speicheroperation wird durchgeführt...",
			savingFailed: "Die Speicheroperation ist fehlgeschlagen.",
			deletingProgress: "Löschoperation wird durchgeführt...",
			deletingFailed: "Die Löschoperation ist fehlgeschlagen.",
			refreshingGridMessage: "Aktualisierung wird durchgeführt...",
			noItemsGridMessage: "Es sind keine anzuzeigenden Elemente vorhanden.",
			testing: "Tests werden durchgeführt...",
			testTimedOut: "Die Ausführung des Verbindungstests dauert zu lange.",
			testConnectionFailed: "Verbindungstest fehlgeschlagen",
			testConnectionSuccess: "Verbindungstest erfolgreich",
			dialog: {
				saveButton: "Speichern",
				deleteButton: "Löschen",
				deleteContent: "Möchten Sie diesen Datensatz wirklich löschen?",
				cancelButton: "Abbrechen",
				closeButton: "Schließen",
				testButton: "Verbindung testen",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "Verbindung zu {0} wird getestet...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "Das Testen der Verbindung zu {0} ist fehlgeschlagen.",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "Das Testen der Verbindung zu {0} war erfolgreich."
			},
			updating: "Aktualisierung wird durchgeführt...",
			invalidName: "Es ist bereits ein Datensatz mit diesem Namen vorhanden.",
			filterHeadingConnection: "Wenn Sie die Verbindungen, die diese Richtlinie verwenden, auf bestimmte Clients beschränken möchten, geben Sie einen oder mehrere der folgenden Filter an. " +
					"Wählen Sie beispielsweise <em>Gruppen-ID</em> aus, um diese Richtlinie auf Mitglieder einer bestimmten Gruppe zu beschränken. " +
					"Die Richtlinie lässt den Zugriff nur zu, wenn alle angegebenen Filter mit wahr ausgewertet werden:",
			filterHeadingMessaging: "Wenn Sie die in dieser Richtlinie definierten Messaging-Aktionen auf bestimmte Clients beschränken möchten, geben Sie einen oder mehrere der folgenden Filter an. " +
					"Wählen Sie beispielsweise <em>Gruppen-ID</em> aus, um diese Richtlinie auf Mitglieder einer bestimmten Gruppe zu beschränken. " +
					"Die Richtlinie lässt den Zugriff nur zu, wenn alle angegebenen Filter mit wahr ausgewertet werden:",
			settingsHeadingMessaging: "Geben Sie die Ressourcen und Messaging-Aktionen an, auf die der Client zugreifen darf:",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "Konfigurieren Sie Verbindungen zu einem oder mehreren WebSphere MQ-Warteschlangenmanagern."				
			},
			connectionProperties: {
				title: "Eigenschaften der Warteschlangenmanagerverbindung",
				tagline: "Informationen zur Herstellung von Verbindungen zwischen dem Server und Warteschlangenmanagern definieren, bearbeiten und löschen.",
				retrieveError: "Beim Abrufen der Eigenschaften der Warteschlangenmanagerverbindung ist ein Fehler aufgetreten.",
				grid: {
					name: "Name",
					qmgr: "Warteschlangenmanager",
					connName: "Verbindungsname",
					channel: "Kanalname",
					description: "Beschreibung",
					SSLCipherSpec: "SSL-Verschlüsselungsspezifikation",
					status: "Status"
				},
				dialog: {
					instruction: "Für die Konnektivität mit MQ sind die folgenden Verbindungsdetails des Warteschlangenmanagers erforderlich.",
					nameInvalid: "Der Name darf keine führenden und nachfolgenden Leerzeichen enthalten.",
					connTooltip: "Geben Sie eine durch Kommas getrennte Liste mit Verbindungsnamen in Form von IP-Adressen oder Hostnamen und Portnummern ein, z. B. 224.0.138.177(1414)",
					connInvalid: "Der Verbindungsname enthält ungültige Zeichen.",
					qmInvalid: "Der Warteschlangenmanagername darf nur Buchstaben und Zahlen sowie die vier Sonderzeichen . _ % und / enthalten. ",
					qmTooltip: "Geben Sie einen Warteschlangenmanagernamen ein, der nur Buchstaben, Zahlen und die vier Sonderzeichen . _ % und / enthält.",
				    channelTooltip: "Geben Sie einen Kanalnamen ein, der nur Buchstaben, Zahlen und die vier Sonderzeichen . _ % und / enthält.",
					channelInvalid: "Der Kanalname darf nur Buchstaben und Zahlen sowie die vier Sonderzeichen . _ % und / enthalten. ",
					activeRuleTitle: "Bearbeiten nicht zulässig",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "Die Warteschlangenmanagerverbindung kann nicht bearbeitet werden, weil sie von der aktivierten Zielzuordnungsregel {0} verwendet wird.",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "Die Warteschlangenmanagerverbindung kann nicht bearbeitet werden, weil sie von den folgenden aktivierten Zielzuordnungsregeln verwendet wird: {0} ",
					SSLCipherSpecTooltip: "Geben Sie eine Verschlüsselungsspezifikation für eine SSL-Verbindung mit einer maximalen Länge von 32 Zeichen ein."
				},
				addDialog: {
					title: "Warteschlangenmanagerverbindung hinzufügen"
				},
				removeDialog: {
					title: "Warteschlangenmanagerverbindung löschen",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
				editDialog: {
					title: "Warteschlangenmanagerverbindung bearbeiten"
				},
                removeNotAllowedDialog: {
                	title: "Entfernen nicht zulässig",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "Die Warteschlangenmanagerverbindung kann nicht entfernt werden, weil sie von der Zielzuordnungsregel {0} verwendet wird.",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "Die Warteschlangenmanagerverbindung kann nicht entfernt werden, weil sie von den folgenden Zielzuordnungsregeln verwendet wird: {0} ",
                	closeButton: "Schließen"
                }
			},
			destinationMapping: {
				title: "Zielzuordnungsregeln",
				tagline: "Systemadministratoren und Messaging-Administratoren können Regeln definieren, bearbeiten und löschen, die steuern, welche Nachrichten an Warteschlangenmanager und von diesen weitergeleitet werden.",
				note: "Bevor Regeln gelöscht werden können, müssen sie inaktiviert werden.",
				retrieveError: "Beim Abrufen der Zielzuordnungsregeln ist ein Fehler aufgetreten.",
				disableRulesConfirmationDialog: {
					text: "Möchten Sie diese Regel wirklich inaktivieren?",
					info: "Wenn Sie die Regel inaktivieren, wird sie gestoppt, was zum Verlust von gepufferten Nachrichten und von Nachrichten, die momentan gesendet werden, führt.",
					buttonLabel: "Regel inaktivieren"
				},
				leadingBlankConfirmationDialog: {
					textSource: "Die <em>Quelle</em> enthält ein führendes Leerzeichen. Möchten Sie diese Regel mit dem führenden Leerzeichen wirklich speichern?",
					textDestination: "Das <em>Ziel</em> enthält ein führendes Leerzeichen. Möchten Sie diese Regel mit dem führenden Leerzeichen wirklich speichern?",
					textBoth: "Die <em>Quelle</em> und das <em>Ziel</em> enthalten führende Leerzeichen. Möchten Sie diese Regel mit den führenden Leerzeichen wirklich speichern?",
					info: "Topics dürfen führende Leerzeichen enthalten, tun dies aber in der Regel nicht. Überprüfen Sie die Topic-Zeichenfolge, um sicherzustellen, dass sie korrekt ist.",
					buttonLabel: "Regel speichern"
				},
				grid: {
					name: "Name",
					type: "Regeltyp",
					source: "Quelle",
					destination: "Ziel",
					none: "Ohne",
					all: "Alle",
					enabled: "Aktiviert",
					associations: "Zuordnungen",
					maxMessages: "Maximale Nachrichtenanzahl",
					retainedMessages: "Aufbewahrte Nachrichten"
				},
				state: {
					enabled: "Aktiviert",
					disabled: "Inaktiviert",
					pending: "Anstehend"
				},
				ruleTypes: {
					type1: "${IMA_PRODUCTNAME_SHORT}-Topic zu MQ-Warteschlange",
					type2: "${IMA_PRODUCTNAME_SHORT}-Topic zu MQ-Topic",
					type3: "MQ-Warteschlange zu ${IMA_PRODUCTNAME_SHORT}-Topic",
					type4: "MQ-Topic zu ${IMA_PRODUCTNAME_SHORT}-Topic",
					type5: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Warteschlange",
					type6: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Topic",
					type7: "${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur zu MQ-Topicunterverzeichnisstruktur",
					type8: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Topic",
					type9: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur",
					type10: "${IMA_PRODUCTNAME_SHORT}-Warteschlange zu MQ-Warteschlange",
					type11: "${IMA_PRODUCTNAME_SHORT}-Warteschlange zu MQ-Topic",
					type12: "MQ-Warteschlange zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange",
					type13: "MQ-Topic zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange",
					type14: "MQ-Topicunterverzeichnisstruktur zu ${IMA_PRODUCTNAME_SHORT}-Warteschlange"
				},
				sourceTooltips: {
					type1: "Geben Sie das ${IMA_PRODUCTNAME_SHORT}-Topic als Quelle für die Zuordnung ein.",
					type2: "Geben Sie das ${IMA_PRODUCTNAME_SHORT}-Topic als Quelle für die Zuordnung ein.",
					type3: "Geben Sie die MQ-Warteschlange als Quelle für die Zuordnung an. Der Wert darf nur die Zeichen a-z, A-Z und 0-9 sowie die folgenden Zeichen enthalten: % . /  _",
					type4: "Geben Sie das MQ-Topic als Quelle für die Zuordnung ein.",
					type5: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MessageGatewayRoot/Level2.",
					type6: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MessageGatewayRoot/Level2.",
					type7: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MessageGatewayRoot/Level2.",
					type8: "Geben Sie die MQ-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MQRoot/Layer1.",
					type9: "Geben Sie die MQ-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MQRoot/Layer1.",
					type10: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Warteschlange als Quelle für die Zuordnung ein." +
							"Der wert darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
							"Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Warteschlange als Quelle für die Zuordnung ein." +
							"Der wert darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
							"Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "Geben Sie die MQ-Warteschlange als Quelle für die Zuordnung an. Der Wert darf nur die Zeichen a-z, A-Z und 0-9 sowie die folgenden Zeichen enthalten: % . /  _",
					type13: "Geben Sie das MQ-Topic als Quelle für die Zuordnung ein.",
					type14: "Geben Sie die MQ-Topicunterverzeichnisstruktur als Quelle für die Zuordnung ein, z. B. MQRoot/Layer1."
				},
				targetTooltips: {
					type1: "Geben Sie die MQ-Warteschlange als Ziel für die Zuordnung an. Der Wert darf nur die Zeichen a-z, A-Z und 0-9 sowie die folgenden Zeichen enthalten: % . /  _",
					type2: "Geben Sie das MQ-Topic als Ziel für die Zuordnung ein.",
					type3: "Geben Sie das ${IMA_PRODUCTNAME_SHORT}-Topic als Ziel für die Zuordnung ein.",
					type4: "Geben Sie das ${IMA_PRODUCTNAME_SHORT}-Topic als Ziel für die Zuordnung ein.",
					type5: "Geben Sie die MQ-Warteschlange als Ziel für die Zuordnung an. Der Wert darf nur die Zeichen a-z, A-Z und 0-9 sowie die folgenden Zeichen enthalten: % . /  _",
					type6: "Geben Sie das MQ-Topic als Ziel für die Zuordnung ein.",
					type7: "Geben Sie die MQ-Topicunterverzeichnisstruktur als Ziel für die Zuordnung ein, z. B. MQRoot/Layer1.",
					type8: "Geben Sie das ${IMA_PRODUCTNAME_SHORT}-Topic als Ziel für die Zuordnung ein.",
					type9: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Topicunterverzeichnisstruktur als Ziel für die Zuordnung ein, z. B. MessageGatewayRoot/Level2.",
					type10: "Geben Sie die MQ-Warteschlange als Ziel für die Zuordnung an. Der Wert darf nur die Zeichen a-z, A-Z und 0-9 sowie die folgenden Zeichen enthalten: % . /  _",
					type11: "Geben Sie das MQ-Topic als Ziel für die Zuordnung ein.",
					type12: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Warteschlange als Ziel für die Zuordnung ein." +
							"Der wert darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
							"Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Warteschlange als Ziel für die Zuordnung ein." +
							"Der wert darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
							"Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "Geben Sie die ${IMA_PRODUCTNAME_SHORT}-Warteschlange als Ziel für die Zuordnung ein." +
							"Der wert darf keine führenden oder nachfolgenden Leerzeichen und keine Steuerzeichen, Kommas, doppelten Anführungszeichen, Backslashes oder Gleichheitszeichen enthalten. " +
							"Das erste Zeichen darf keine Zahl, kein Anführungszeichen und keines der folgenden Sonderzeichen sein: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "Zugeordnet",
				associatedQMs: "Zugeordnete Warteschlangenmanagerverbindungen:",
				associatedMessages: {
					errorMissing: "Es muss eine Warteschlangenmanagerverbindung ausgewählt werden.",
					errorRetained: "Die Einstellung Aufbewahrte Nachrichten lässt die Zuordnung einer einzigen Warteschlangenmanagerverbindung zu. " +
							"Ändern Sie die Einstellung Aufbewahrte Nachrichten in <em>Ohne</em> oder entfernen Sie einige Zuordnungen."
				},
				ruleTypeMessage: "Die Einstellung Aufbewahrte Nachrichten lässt Regeltypen mit Topics oder Topicunterverzeichnisstrukturen als Ziel zu." +
							"Ändern Sie die Einstellung Aufbewahrte Nachrichten in <em>Ohne</em> oder wählen Sie einen anderen Regeltyp aus.",
				status: {
					active: "Aktiv",
					starting: "Startoperation wird ausgeführt",
					inactive: "Inaktiv"
				},
				dialog: {
					instruction: "Zielzuordungsregeln definieren die Richtung, in die Nachrichten verschoben werden, und die Art der Quellen- und Zielobjekte.",
					nameInvalid: "Der Name darf keine führenden und nachfolgenden Leerzeichen enthalten.",
					noQmgrsTitle: "Keine Warteschlangenmanagerverbindungen",
					noQmrsDetail: "Es kann keine Zielzuordnungsregel definiert werden, wenn zuvor keine Warteschlangenmanagerverbindung definiert wurde.",
					maxMessagesTooltip: "Geben Sie die maximale Anzahl an Nachrichten an, die im Ziel gepuffert werden kann.",
					maxMessagesInvalid: "Sie müssen für die maximale Nachrichtenanzahl eine Zahl zwischen 1 und 20.000.000 einschließlich angeben.",
					retainedMessages: {
						label: "Aufbewahrte Nachrichten",
						none: "Ohne",
						all: "Alle",
						tooltip: {
							basic: "Geben Sie an, welche Nachrichten als aufbewahrte Nachrichten an ein Topic weitergeleitet werden.",
							disabled4Type: "Die Einstellung Aufbewahrte Nachrichten muss auf <em>Ohne</em> gesetzt werden, wenn das Ziel eine Warteschlange ist.",
							disabled4Associations: "Die Einstellung Aufbewahrte Nachrichten muss auf <em>Ohne</em> gesetzt werden, wenn mehrere Warteschlangenmanagerverbindungen ausgewählt sind.",
							disabled4Both: "Die Einstellung Aufbewahrte Nachrichten muss auf <em>Ohne</em> gesetzt werden, wenn das Ziel eine Warteschlange ist oder wenn mehrere Warteschlangenmanagerverbindungen ausgewählt sind."
						}
					}
				},
				addDialog: {
					title: "Zielzuordnungsregel hinzufügen"
				},
				removeDialog: {
					title: "Zielzuordnungsregel löschen",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
                removeNotAllowedDialog: {
                	title: "Entfernen nicht zulässig",
                	content: "Die Zielzuordnungsregel kann nicht entfernt werden, weil sie aktiviert ist. " +
                			 "Inaktivieren Sie die Regel über das Menü 'Weitere Aktionen' und wiederholen Sie dann die Operation.",
                	closeButton: "Schließen"
                },
				editDialog: {
					title: "Zielzuordnungsregel bearbeiten",
					restrictedEditingTitle: "Bearbeitung ist eingeschränkt, wenn Regel aktiviert ist",
					restrictedEditingContent: "Sie können nur bestimmte Eigenschaften bearbeiten, wenn die Regel aktiviert ist. " +
							"Wenn Sie weitere Eigenschaften bearbeiten möchten, müssen Sie die Regel inaktivieren. Bearbeiten Sie dann die Eigenschaften und speichern Sie Ihre Änderungen. " +
							"Wenn Sie die Regel inaktivieren, wird sie gestoppt, was zum Verlust von gepufferten Nachrichten und von Nachrichten, die momentan gesendet werden, führt." +
							"Die Regel bleibt inaktiviert, bis Sie sie erneut aktivieren. "
				},
				action: {
					Enable: "Regel aktivieren",
					Disable: "Regel inaktivieren",
					Refresh: "Status aktualisieren"
				}
			},
			sslkeyrepository: {
				title: "SSL-Schlüsselrepository",
				tagline: "Systemadministratoren und Messaging-Administratoren können ein SSL-Schlüsselrepository und eine Kennwortdatei hochladen und herunterladen. " +
						 "Das SSL-Schlüsselrepository und die Kennwortdatei werden verwendet, um die Verbindung zwischen dem Server und dem Warteschlangenmanager zu sichern.",
                uploadButton: "Hochladen",
                downloadLink: "Herunterladen",
                deleteButton: "Löschen",
                lastUpdatedLabel: "Letzte Aktualisierung der SSL-Schlüsselrepository-Dateien:",
                noFileUpdated: "Nie",
                uploadFailed: "Upload fehlgeschlagen.",
                deletingFailed: "Die Löschoperation ist fehlgeschlagen.",                
                dialog: {
                	uploadTitle: "SSL-Schlüsselrepository-Dateien hochladen",
                	deleteTitle: "SSL-Schlüsselrepository-Dateien löschen",
                	deleteContent: "Wenn der MQConnectivity-Service aktiv ist, wird er erneut gestartet. Möchten Sie die SSL-Schlüsselrepository-Dateien wirklich löschen?",
                	keyFileLabel: "Schlüsseldatenbankdatei:",
                	passwordFileLabel: "Kennwortstashdatei:",
                	browseButton: "Durchsuchen...",
					browseHint: "Datei auswählen...",
					savingProgress: "Speicheroperation wird durchgeführt...",
					deletingProgress: "Löschoperation wird durchgeführt...",
                	saveButton: "Speichern",
                	deleteButton: "OK",
                	cancelButton: "Abbrechen",
                	keyRepositoryError:  "Die SSL-Schlüsselrepository-Datei muss eine .kdb-Datei sein. ",
                	passwordStashError:  "Die Kennwortstashdatei muss eine .sth-Datei sein.",
                	keyRepositoryMissingError: "Sie müssen eine SSL-Schlüsselrepository-Datei angeben.",
                	passwordStashMissingError: "Sie müssen eine Kennwortstashdatei angeben."
                }
			},
			externldap: {
				subTitle: "LDAP-Konfiguration",
				tagline: "Wenn LDAP aktiviert ist, wird LDAP für Serverbenutzer und -gruppen verwendet.",
				itemName: "LDAP-Verbindung",
				grid: {
					ldapname: "LDAP-Name",
					url: "URL",
					userid: "Benutzer-ID",
					password: "Kennwort"
				},
				enableButton: {
					enable: "LDAP aktivieren",
					disable: "LDAP inaktivieren"
				},
				dialog: {
					ldapname: "Name des LDAP-Objekts",
					url: "URL",
					certificate: "Zertifikat",
					checkcert: "Serverzertifikat überprüfen",
					checkcertTStoreOpt: "Truststore des Messaging-Servers verwenden",
					checkcertDisableOpt: "Zertifikatsprüfung inaktivieren",
					checkcertPublicTOpt: "Öffentlichen Truststore verwenden",
					timeout: "Zeitlimit",
					enableCache: "Cache aktivieren",
					cachetimeout: "Cachezeitlimit",
					groupcachetimeout: "Gruppencachezeitlimit",
					ignorecase: "Groß-/Kleinschreibung ignorieren",
					basedn: "Basis-DN",
					binddn: "Bindungs-DN",
					bindpw: "Bindungskennwort",
					userSuffix: "Benutzersuffix",
					groupSuffix: "Gruppensuffix",
					useridmap: "Benutzer-ID-Map",
					groupidmap: "Gruppen-ID-Map",
					groupmemberidmap: "Gruppenmitglieder-ID-Map",
					nestedGroupSearch: "Verschachtelte Gruppen einschließen",
					tooltip: {
						url: "Die URL, die auf den LDAP-Server verweist. Das Format ist folgendermaßen: <br/> &lt;Protokoll&gt;://&lt;Server&nbsp;IP-Adresse&gt;:&lt;Port&gt;.",
						checkcert: "Geben Sie an, wie das LDAP-Serverzertifikat überprüft werden soll.",
						basedn: "Der Basis-DN. ",
						binddn: "Der bei der Bindung an LDAP zu verwendende definierte Name. Lassen Sie das Feld für eine anonyme Bindung leer.",
						bindpw: "Das für die Bindung an LDAP zu verwendende Kennwort. Lassen Sie das Feld für eine anonyme Bindung leer.",
						userSuffix: "Das Suffix für den Benutzer-DN, nach dem gesucht werden soll. " +
									"Wenn Sie das Suffix nicht angeben, suchen Sie über die Benutzer-ID-Map nach dem Benutzer-DN und nehmen Sie dann die Bindung unter Verwendung dieses DN vor.",
						groupSuffix: "Das Suffix für den Gruppen-DN.",
						useridmap: "Eigenschaft, der eine Benutzer-ID zugeordnet werden soll.",
						groupidmap: "Eigenschaft, der eine Gruppen-ID zugeordnet werden soll.",
						groupmemberidmap: "Eigenschaft, der eine Gruppenmitglieds-ID zugeordnet werden soll.",
						timeout: "Zeitlimit für LDAP-Aufrufe in Sekunden.",
						enableCache: "Geben Sie an, ob Berechtigungsnachweise zwischengespeichert werden sollen.",
						cachetimeout: "Das Cachezeitlimit in Sekunden.",
						groupcachetimeout: "Das Gruppencachezeitlimit in Sekunden.",
						nestedGroupSearch: "Wenn Sie diese Option auswählen, werden alle verschachtelten Gruppen in die Suche nach der Gruppenzugehörigkeit eines Benutzers eingeschlossen. ",
						testButtonHelp: "Testet die Verbindung zum LDAP-Server. Sie müssen die URL des LDAP-Servers angeben. Optional können Sie Werte für das Zertifikat, den Bindungs-DN und das Bindungskennwort angeben."
					},
					secondUnit: "Sekunden",
					browseHint: "Datei auswählen...",
					browse: "Durchsuchen...",
					clear: "Abwählen",
					connSubHeading: "LDAP-Verbindungseinstellungen",
					invalidTimeout: "Das Zeitlimit muss ein Wert zwischen 1 und 60 einschließlich sein.",
					invalidCacheTimeout: "Das Zeitlimit muss ein Wert zwischen 1 und 60 einschließlich sein.",
					invalidGroupCacheTimeout: "Das Zeitlimit muss ein Wert zwischen 1 und 86.400 einschließlich sein.",
					certificateRequired: "Ein Zertifikat ist erforderlich, wenn eine ldaps-URL angegeben wird und der Truststore verwendet wird. ",
					urlThemeError: "Geben Sie eine gültige URL mit dem Protokoll ldap oder ldaps ein."
				},
				addDialog: {
					title: "LDAP-Verbindung hinzufügen",
					instruction: "Konfigurieren Sie eine LDAP-Verbindung."
				},
				editDialog: {
					title: "LDAP-Verbindungen bearbeiten"
				},
				removeDialog: {
					title: "LDAP-Verbindung löschen"
				},
				warnOnlyOneLDAPDialog: {
					title: "LDAP-Verbindung bereits definiert",
					content: "Es kann nur eine einzige LDAP-Verbindung angegeben werden.",
					closeButton: "Schließen"
				},
				restartInfoDialog: {
					title: "Änderung der LDAP-Einstellungen",
					content: "Die neuen LDAP-Einstellungen werden bei der nächsten Authentifizierung oder Berechtigung eines Clients oder einer Verbindung verwendet. ",
					closeButton: "Schließen"
				},
				enableError: "Beim Aktivieren/Inaktivieren der externen LDAP-Konfiguration ist ein Fehler aufgetreten.",				
				retrieveError: "Beim Abrufen der LDAP-Konfiguration ist ein Fehler aufgetreten.",
				saveSuccess: "Die Speicheroperation war erfolgreich. Der ${IMA_PRODUCTNAME_SHORT}-Server muss erneut gestartet werden, damit die Änderungen wirksam werden."
			},
			messagequeues: {
				title: "Nachrichtenwarteschlangen",
				subtitle: "Nachrichtenwarteschlangen werden für Punkt-zu-Punkt-Messaging verwendet."				
			},
			queues: {
				title: "Warteschlangen",
				tagline: "Definieren, bearbeiten oder löschen Sie eine Nachrichtenwarteschlange.",
				retrieveError: "Beim Abrufen der Liste der Nachrichtenwarteschlangen ist ein Fehler aufgetreten.",
				grid: {
					name: "Name",
					allowSend: "Senden zulassen",
					maxMessages: "Maximale Nachrichten",
					concurrentConsumers: "Gleichzeitige Konsumenten",
					description: "Beschreibung"
				},
				dialog: {	
					instruction: "Definieren Sie eine Warteschlange für Ihre Messaging-Anwendung.",
					nameInvalid: "Der Name darf keine führenden und nachfolgenden Leerzeichen enthalten.",
					maxMessagesTooltip: "Geben Sie die maximale Anzahl an Nachrichten an, die in der Warteschlange gespeichert werden können.",
					maxMessagesInvalid: "Sie müssen für die maximale Nachrichtenanzahl eine Zahl zwischen 1 und 20.000.000 einschließlich angeben.",
					allowSendTooltip: "Geben Sie an, ob Anwendungen Nachrichten an diese Warteschlange senden können. Diese Eigenschaft hat keine Auswirkung auf die Möglichkeit von Anwendungen, Nachrichten aus dieser Warteschlange zu empfangen.",
					concurrentConsumersTooltip: "Geben Sie an, ob die Warteschlange mehrere gleichzeitig angemeldete Konsumenten zulässt. Wenn das Kontrollkästchen ausgewählt ist, ist nur ein einziger Konsument für die Warteschlange zulässig.",
					performanceLabel: "Leistungseigenschaften"
				},
				addDialog: {
					title: "Warteschlange hinzufügen"
				},
				removeDialog: {
					title: "Warteschlange löschen",
					content: "Möchten Sie diesen Datensatz wirklich löschen?"
				},
				editDialog: {
					title: "Warteschlange bearbeiten"
				}	
			},
			messagingTester: {
				title: "Beispielanwendung Messaging Tester",
				tagline: "Messaging Tester ist eine einfache HTML5-Beispielanwendung, die MQTT über WebSockets verwendet, um mehrere Clients zu simulieren, die mit dem ${IMA_PRODUCTNAME_SHORT}-Server interagieren. " +
						 "Sobald diese Clients mit dem Server verbunden sind, können sie Nachrichten in drei Topics veröffentlichen/subskribieren. ",
				enableSection: {
					title: "1. Endpunkt DemoMqttEndpoint aktivieren",
					tagline: "Die Beispielanwendung Messaging Tester muss eine Verbindung zu einem nicht gesicherten ${IMA_PRODUCTNAME_SHORT}-Endpunkt herstellen. Sie können DemoMqttEndpoint verwenden."					
				},
				downloadSection: {
					title: "2. Beispielanwendung Messaging Tester herunterladen und ausführen",
					tagline: "Klicken Sie auf den Link, um die Datei MessagingTester.zip herunterzuladen. Entpacken Sie die Dateien und öffnen Sie dann die Datei index.html in einem Browser, der WebSockets unterstützt. " +
							 "Folgen Sie den Anweisungen auf der Webseite, um sicherzustellen, dass ${IMA_PRODUCTNAME_SHORT} für MQTT-Messaging bereit ist.",
					linkLabel: "MessagingTester.zip herunterladen"
				},
				disableSection: {
					title: "3. Endpunkt DemoMqttEndpoint inaktivieren",
					tagline: "Nachdem Sie die Überprüfung des ${IMA_PRODUCTNAME_SHORT}-MQTT-Messagings abgeschlossen haben, inaktivieren Sie den Endpunkt DemoMqttEndpoint, um unbefugte Zugriffe auf ${IMA_PRODUCTNAME_SHORT} zu verhindern."					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "Status von {0}",
					state: {
						enabled: "Aktiviert",					
						disabled: "Inaktiviert",
						missing: "Nicht vorhanden",
						down: "Inaktiv"
					},
					linkLabel: {
						enableLinkLabel: "Aktivieren",
						disableLinkLabel: "Inaktivieren"						
					},
					missingMessage: "Wenn kein anderer nicht gesicherter Endpunkt verfügbar ist, erstellen Sie einen.",
					retrieveEndpointError: "Beim Abrufen der Endpunktkonfiguration ist ein Fehler aufgetreten.",					
					retrieveEndpointStatusError: "Beim Abrufen des Endpunktstatus ist ein Fehler aufgetreten.",
					saveEndpointError: "Beim Festlegen des Endpunktstatus ist ein Fehler aufgetreten."
				}
			}
		},
		protocolsLabel: "Protokolle",

		// Messaging policy types
		topicPoliciesTitle: "Topicrichtlinien",
		subscriptionPoliciesTitle: "Subskriptionsrichtlinien",
		queuePoliciesTitle: "Warteschlangenrichtlinien",
		// Messaging policy dialog strings
		addTopicMPTitle: "Topicrichtlinie hinzufügen",
	    editTopicMPTitle: "Topicrichtlinie bearbeiten",
	    viewTopicMPTitle: "Topicrichtlinie anzeigen",
		removeTopicMPTitle: "Topicrichtlinie löschen",
		removeTopicMPContent: "Möchten Sie diese Topicrichtlinie wirklich löschen?",
		topicMPInstruction: "Eine Topic-Richtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Topics in ${IMA_PRODUCTNAME_FULL}. " +
					     "Jeder Endpunkt muss mindestens eine Topicrichtlinie, Subskriptionsrichtlinie oder Warteschlangenrichtlinie haben.",
		addSubscriptionMPTitle: "Subskriptionsrichtlinie hinzufügen",
		editSubscriptionMPTitle: "Subskriptionsrichtlinie bearbeiten",
		viewSubscriptionMPTitle: "Subskriptionsrichtlinie anzeigen",
		removeSubscriptionMPTitle: "Subskriptionsrichtlinie löschen",
		removeSubscriptionMPContent: "Möchten Sie diese Subskriptionsrichtlinie wirklich löschen?",
		subscriptionMPInstruction: "Eine Subskriptionsrichtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte globale gemeinsam genutzte Subkriptionen in ${IMA_PRODUCTNAME_FULL}." +
					     "In einer globalen gemeinsam genutzten Subskription verteilt sich die Arbeit für den Empfang von Nachrichten von einer permanenten Topicsubskription auf mehrere Subskribenten. Jeder Endpunkt muss mindestens eine Topicrichtlinie, Subskriptionsrichtlinie oder Warteschlangenrichtlinie haben.",
		addQueueMPTitle: "Warteschlangenrichtlinie hinzufügen",
		editQueueMPTitle: "Warteschlangenrichtlinie bearbeiten",
		viewQueueMPTitle: "Warteschlangenrichtlinie anzeigen",
		removeQueueMPTitle: "Warteschlangenrichtlinie löschen",
		removeQueueMPContent: "Möchten Sie diese Warteschlangenrichtlinie wirklich löschen?",
		queueMPInstruction: "Eine Warteschlangenrichtlinie berechtigt verbundene Clients zur Ausführung bestimmter Messaging-Aktionen, z. B. den Zugriff auf bestimmte Warteschlangen in ${IMA_PRODUCTNAME_FULL}." +
					     "Jeder Endpunkt muss mindestens eine Topicrichtlinie, Subskriptionsrichtlinie oder Warteschlangenrichtlinie haben.",
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "Richtlinientyp",
		policyTypeName_topic: "Topic",
		policyTypeName_subscription: "Globale gemeinsam genutzte Subskription",
		policyTypeName_queue: "Warteschlange",
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "Topic",
		policyTypeShortName_subscription: "Subskription",
		policyTypeShortName_queue: "Warteschlange",
		policyTypeTooltip_topic: "Das Topic, für das diese Richtlinie gilt. Verwenden Sie den Stern (*) mit Vorsicht. " +
		    "Ein Stern dient als Platzhalter für 0 oder mehr Zeichen, einschließlich des Schrägstrichs (/). Deshalb kann der Stern für mehrere Ebenen in einer Topicverzeichnisstruktur stehen.",
		policyTypeTooltip_subscription: "Die globale, gemeinsam genutzte permanente Subskription, für die diese Richtlinie gilt. Verwenden Sie den Stern (*) mit Vorsicht. " +
			"Ein Stern dient als Platzhalter für 0 oder mehr Zeichen.",
	    policyTypeTooltip_queue: "Die Warteschlange, für die diese Richtlinie gilt. Verwenden Sie den Stern (*) mit Vorsicht. " +
			"Ein Stern dient als Platzhalter für 0 oder mehr Zeichen.",
	    topicPoliciesTagline: "Mit einer Topicrichtlinie können Sie steuern, auf welche Topics ein Client in ${IMA_PRODUCTNAME_FULL} zugreifen kann.",
		subscriptionPoliciesTagline: "Mit einer Subskriptionsrichtlinie können Sie steuern, auf welche permanenten Subskriptionen ein Client in ${IMA_PRODUCTNAME_FULL} zugreifen kann.",
	    queuePoliciesTagline: "Mit einer Warteschlangenrichtlinie können Sie steuern, auf welche Warteschlange ein Client in ${IMA_PRODUCTNAME_FULL} zugreifen kann.",
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "Kanalbenutzer",
	    channelPassword: "Kennwort des Kanalbenutzers",
	    channelUserTooltip: "Wenn Ihr Warteschlangenmanager für die Authentifizierung von Kanalbenutzern konfiguriert ist, müssen Sie diesen Wert definieren. ",
	    channelPasswordTooltip: "Wenn Sie einen Kanalbenutzer angeben, müssen Sie diesen auch diesen Wert definieren. ",
	    // Additional LDAP dialog properties
	    emptyLdapURL: "Die LDAP-URL ist nicht definiert.",
		externalLDAPOnlyTagline: "Konfigurieren Sie die Verbindungseigenschaften für das LDAP-Repository, das für Benutzer und Gruppen des ${IMA_PRODUCTNAME_SHORT}-Servers verwendet wird.", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "Abwählen",
		resetLdapButton: "Zurücksetzen",
		resetLdapTitle: "LDAP-Konfiguration zurücksetzen",
		resetLdapContent: "Alle LDAP-Konfigurationseinstellungen werden auf Standardwerte zurückgesetzt.",
		resettingProgress: "Einstellungen werden zurückgesetzt...",
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "Wenn der MQConnectivity-Service aktiv ist, wird er erneut gestartet, nachdem die Repository-Dateien hochgeladen wurden. ",
		savingRestartingProgress: "Speichern und erneut starten...",
		deletingRestartingProgress: "Löschen und erneut starten..."
});
