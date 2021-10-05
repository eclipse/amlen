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
			title : "Contrôle",
			monitoringNotAvailable: "La surveillance n'est pas disponible en raison du statut du serveur ${IMA_PRODUCTNAME_SHORT}.",
			monitoringError:"Une erreur s'est produite lors de l'extraction des données de surveillance.",
			pool1LimitReached: "L'utilisation du pool 1 est trop élevée. Les publications et les abonnements risquent d'être rejetés.",
			connectionStats: {
				title: "Surveillance des connexions",
				tagline: "Surveiller des données de connexion agrégées et analyser les connexions les plus et les moins performantes parmi plusieurs statistiques de connexion.",
		 		charts: {
					sectionTitle: "Graphiques des connexions",
					tagline: "Surveiller le nombre de connexions nouvelles et actives au serveur. Pour mettre en pause des mises à jour, cliquez sur le bouton situé sous les graphiques.",
					activeConnectionsTitle: "Images instantanées des connexions actives, prise toutes les cinq secondes environ.",
					newConnectionsTitle: "Images instantanées des connexions nouvelles et fermées, prises toutes les cinq secondes environ."
				},
		 		grids: {
					sectionTitle: "Données de connexion",
					tagline: "Afficher les connexions les plus et les moins performantes pour un noeud final et des statistiques en particulier. " +
							"Jusqu'à 50 connexions peuvent être affichées. Les données de surveillance sont mises en cache sur le serveur et ce cache est actualisé toutes les minutes. " +
							"Par conséquent, les données peuvent être obsolètes au bout d'une minute."
				}
			},
			endpointStats: {
				title: "Surveillance des noeuds finaux",
				tagline: "Surveiller l'historique des performances de noeuds finaux individuels sous forme de graphiques de séries temporelles ou de données agrégées.",
		 		charts: {
					sectionTitle: "Graphique de noeud final",
					tagline: "Afficher des graphiques de séries temporelles pour un noeud final, des statistiques ou une durée en particulier. " +
						"Pour les durées d'une heure ou moins, les graphiques sont basés sur des instantanés pris toutes les 5 secondes. " +
						"Pour des durées plus longues, les graphiques sont basés sur des instantanés pris toutes les minutes."
				},
		 		grids: {
					sectionTitle: "Données de noeud final",
					tagline: "Afficher des données agrégées pour des noeuds finaux en particulier. Les données de surveillance sont mises en cache sur le serveur et ce cache est actualisé toutes les minutes. " +
							 "Par conséquent, les données peuvent être obsolètes au bout d'une minute."
				}
			},
			queueMonitor: {
				title: "Surveillance des files d'attente",
				tagline: "Surveiller des files d'attente individuelles à l'aide de diverses statistiques de file d'attente. Jusqu'à 100 files d'attente peuvent être affichées."
			},
			mqttClientMonitor: {
				title: "Clients MQTT déconnectés",
				tagline: "Rechercher et supprimer les clients MQTT déconnectés. Jusqu'à 100 clients MQTT peuvent être affichés.",
				taglineNoDelete: "Rechercher les clients MQTT déconnectés. Jusqu'à 100 clients MQTT peuvent être affichés."
			},
			subscriptionMonitor: {
				title: "Surveillance des abonnements",
				tagline: "Surveiller des abonnements à l'aide de diverses statistiques d'abonnement. Supprimer des abonnements durables. Jusqu'à 100 abonnements peuvent être affichés.",
				taglineNoDelete: "Surveiller des abonnements à l'aide de diverses statistiques d'abonnement. Jusqu'à 100 abonnements peuvent être affichés."
			},
			transactionMonitor: {
				title: "Surveillance des transactions",
				tagline: "Surveiller des transactions XA coordonnées par un gestionnaire de transactions externe."
			},
			destinationMappingRuleMonitor: {
				title: "Surveillance de la règle de mappage de destination MQ Connectivity",
				tagline: "Surveiller des statistiques de règle de mappage de destination. Les statistiques concernent une paire Règle de mappage de destination/Connexion du gestionnaire de files d'attente. Jusqu'à 100 paires peuvent être affichées."
			},
			applianceMonitor: {
				title: "Moniteur de serveur",
				tagline: "Rassemblez des informations sur les diverses statistiques relatives à ${IMA_PRODUCTNAME_FULL}.",
				storeMonitorTitle: "Stockage de persistance",
				storeMonitorTagline: "Surveillez les statistiques sur les ressources utilisées par le stockage de persistance d'${IMA_PRODUCTNAME_FULL}.",
				memoryMonitorTitle: "Mémoire",
				memoryMonitorTagline: "Surveillez les statistiques sur la mémoire utilisée par d'autres processus ${IMA_PRODUCTNAME_FULL}."
			},
			topicMonitor: {
				title: "Surveillance des rubriques",
				tagline: "Configurer des surveillances de rubriques qui fournissent diverses statistiques agrégées sur des chaînes de rubrique.",
				taglineNoConfigure: "Afficher des surveillances de rubriques qui fournissent diverses statistiques agrégées sur des chaînes de rubrique."
			},
			views: {
				conntimeWorst: "Connexions les plus récentes",
				conntimeBest: "Connexions les plus anciennes",
				tputMsgWorst: "Débit le plus faible (messages)",
				tputMsgBest: "Débit le plus élevé (messages)",
				tputBytesWorst: "Débit le plus faible (Ko)",
				tputBytesBest: "Débit le plus élevé (Ko)",
				activeCount: "Connexions actives",
				connectCount: "Connexions établies",
				badCount: "Cumul des connexions ayant échoué",
				msgCount: "Débit (msg)",
				bytesCount: "Débit (octets)",

				msgBufHighest: "Files d'attente avec le plus de messages en mémoire tampon",
				msgProdHighest: "Files d'attente avec le plus de messages produits",
				msgConsHighest: "Files d'attente avec le plus de messages consommés",
				numProdHighest: "Files d'attente avec le plus d'expéditeurs",
				numConsHighest: "Files d'attente avec le plus de destinataires",
				msgBufLowest: "Files d'attente avec le moins de messages en mémoire tampon",
				msgProdLowest: "Files d'attente avec le moins de messages produits",
				msgConsLowest: "Files d'attente avec le moins de messages consommés",
				numProdLowest: "Files d'attente avec le moins d'expéditeurs",
				numConsLowest: "Files d'attente avec le moins de destinataires",
				BufferedHWMPercentHighestQ: "Files d'attente s'approchant le plus de la capacité",
				BufferedHWMPercentLowestQ: "Files d'attente au plus loin de la capacité",
				ExpiredMsgsHighestQ: "Files d'attente avec le plus de messages arrivés à expiration",
				ExpiredMsgsLowestQ: "Files d'attente avec le moins de messages arrivés à expiration",

				msgPubHighest: "Rubriques avec le plus de messages publiés",
				subsHighest: "Rubriques avec le plus d'abonnés",
				msgRejHighest: "Rubriques avec le plus de messages rejetés",
				pubRejHighest: "Rubriques avec le plus de publications ayant échoué",
				msgPubLowest: "Rubriques avec le moins de messages publiés",
				subsLowest: "Rubriques avec le moins d'abonnés",
				msgRejLowest: "Rubriques avec le moins de messages rejetés",
				pubRejLowest: "Rubriques avec le moins de publications ayant échoué",

				publishedMsgsHighest: "Abonnements avec le plus de messages publiés",
				bufferedMsgsHighest: "Abonnements avec le plus de messages en mémoire tampon",
				bufferedPercentHighest: "Abonnements avec le pourcentage le plus élevé de messages en mémoire tampon",
				rejectedMsgsHighest: "Abonnements avec le plus de messages rejetés",
				publishedMsgsLowest: "Abonnements avec le moins de messages publiés",
				bufferedMsgsLowest: "Abonnements avec le moins de messages en mémoire tampon",
				bufferedPercentLowest: "Abonnements avec le pourcentage le moins élevé de messages en mémoire tampon",
				rejectedMsgsLowest: "Abonnements avec le moins de messages rejetés",
				BufferedHWMPercentHighestS: "Abonnements au plus proches de la capacité",
				BufferedHWMPercentLowestS: "Abonnements au plus loin de la capacité",
				ExpiredMsgsHighestS: "Abonnements avec le plus de messages arrivés à expiration",
				ExpiredMsgsLowestS: "Abonnements avec le moins de messages arrivés à expiration",
				DiscardedMsgsHighestS: "Abonnements avec le plus de messages annulés",
				DiscardedMsgsLowestS: "Abonnements avec le moins de messages annulés",
				mostCommitOps: "Règles avec le plus d'opérations validées",
				mostRollbackOps: "Règles avec le plus d'opérations d'annulation",
				mostPersistMsgs: "Règles avec le plus de messages persistants",
				mostNonPersistMsgs: "Règles avec le plus de messages non persistants",
				mostCommitMsgs: "Règles avec le plus de messages validés"
			},
			rules: {
				any: "Indifférent",
				imaTopicToMQQueue: "Rubrique ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
				imaTopicToMQTopic: "Rubrique ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
				mqQueueToIMATopic: "File d'attente MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
				mqTopicToIMATopic: "Rubrique MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
				imaTopicSubtreeToMQQueue: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
				imaTopicSubtreeToMQTopic: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
				imaTopicSubtreeToMQTopicSubtree: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la sous-arborescence de rubrique MQ",
				mqTopicSubtreeToIMATopic: "Sous-arborescence de rubrique MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
				mqTopicSubtreeToIMATopicSubtree: "Sous-arborescence de rubrique MQ vers la sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT}",
				imaQueueToMQQueue: "File d'attente ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
				imaQueueToMQTopic: "File d'attente ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
				mqQueueToIMAQueue: "File d'attente MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}",
				mqTopicToIMAQueue: "Rubrique MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}",
				mqTopicSubtreeToIMAQueue: "Sous-arborescence de rubrique MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}"
			},
			chartLabels: {
				activeCount: "Connexions actives",
				connectCount: "Connexions établies",
				badCount: "Cumul des connexions ayant échoué",
				msgCount: "Débit (msg/sec)",
				bytesCount: "Débit (Mo/sec)"
			},
			connectionVolume: "Connexions actives",
			connectionVolumeAxisTitle: "Actif",
			connectionRate: "Nouvelles connexions",
			connectionRateAxisTitle: "Modifié",
			newConnections: "Connexions ouvertes",
			closedConnections: "Connexions fermées",
			timeAxisTitle: "Heure",
			pauseButton: "Mettre en pause les mises à jour des graphiques",
			resumeButton: "Reprendre les mises à jour des graphiques",
			noData: "Aucune donnée",
			grid: {				
				name: "ID client",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "Nom de la connexion. (Name)",
				endpoint: "Noeud final",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "Nom du noeud final. (Endpoint)",
				clientIp: "IP client",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "Adresse IP du client. (ClientAddr)",
				clientId: "ID client :",
				userId: "ID utilisateur",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "ID utilisateur principal. (UserId)",
				protocol: "Protocole",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "Nom du protocole. (Protocol)",
				bytesRecv: "Reçu (Ko)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "Nombre d'octets lus depuis le début de la connexion. (ReadBytes)",
				bytesSent: "Envoyé (Ko)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "Nombre d'octets écrits depuis le début de la connexion. (WriteBytes)",
				msgRecv: "Reçus (messages)",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "Nombre de messages lus depuis l'heure de connexion. (ReadMsg)",
				msgSent: "Envoyés (messages)",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "Nombre de messages écrits depuis le début de la connexion. (WriteMsg)",
				bytesThroughput: "Débit (ko/sec)",
				msgThroughput: "Débit (msg/sec)",
				connectionTime: "Temps de connexion (en secondes)",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "Nombre de secondes écoulées depuis la création de la connexion. (ConnectTime)",
				updateButton: "Mise à jour",
				refreshButton: "Actualiser",
				deleteClientButton: "Supprimer un client",
				deleteSubscriptionButton: "Supprimer un abonnement durable",
				endpointLabel: "Source :",
				listenerLabel: "Noeud final :",
				queueNameLabel: "Nom de la file d'attente :",
				ruleNameLabel: "Nom de la règle :",
				queueManagerConnLabel: "Connexion du gestionnaire de files d'attente :",
				topicNameLabel: "Rubriques surveillées :",
				topicStringLabel: "Chaîne de rubrique :",
				subscriptionLabel: "Nom de l'abonnement :",
				subscriptionTypeLabel: "Type d'abonnement :",
				refreshingLabel: "Mise à jour en cours...",
				datasetLabel: "Requête :",
				ruleTypeLabel: "Type de règle :",
				durationLabel: "Durée :",
				resultsLabel: "Résultats :",
				allListeners: "Tous",
				allEndpoints: "(tous les noeuds finaux) ",
				allSubscriptions: "Tous",
				metricLabel: "Requête :",
				// TRNOTE min is short for minutes
				last10: "10 dernières minutes",
				// TRNOTE hr is short for hours
				last60: "Dernière heure",
				// TRNOTE hr is short for hours
				last360: "6 dernières heures",
				// TRNOTE hr is short for hours
				last1440: "Dernières 24 heures",
				// TRNOTE min is short for minutes
				min5: "5 min",
				// TRNOTE min is short for minutes
				min10: "10 min",
				// TRNOTE min is short for minutes
				min30: "30 min",
				// TRNOTE hr is short for hours
				hr1: "1 heure",
				// TRNOTE hr is short for hours
				hr3: "3 heures",
				// TRNOTE hr is short for hours
				hr6: "6 heures",
				// TRNOTE hr is short for hours
				hr12: "12 heures",
				// TRNOTE hr is short for hours
				hr24: "24 heures",
				totalMessages: "Nombre total de messages",
				bufferedMessages: "Messages placés en mémoire tampon",
				subscriptionProperties: "Propriétés de l'abonnement",
				commitTransactionButton: "Valider",
				rollbackTransactionButton: "Ramener à l'état antérieur",
				forgetTransactionButton: "Oublier",
				queue: {
					queueName: "Nom de la file d'attente",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "Nom de la file d'attente. (QueueName)",
					msgBuf: "Actuel",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "Nombre de messages placés en mémoire tampon dans la file d'attente et en attente d'utilisation. (BufferedMsgs)",
					msgBufHWM: "Heures pleines",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "Nombre le plus élevé de messages placés en mémoire tampon sur la file d'attente depuis le démarrage du serveur ou la création de la file d'attente, selon l'opération la plus récente. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Pic de % du maximum",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "Pics de messages en mémoire tampon, sous la forme d'un pourcentage du nombre maximal de messages pouvant être placés en mémoire tampon. (BufferedHWMPercent)",
					perMsgBuf: "% actuel du maximum",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "Nombre de messages en mémoire tampon, sous la forme d'un pourcentage du nombre maximal de messages en mémoire tampon. (BufferedPercent)",
					totMsgProd: "Produit",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "Nombre de messages envoyés à la file d'attente depuis le démarrage du serveur ou la création de la file d'attente, selon l'opération la plus récente. (ProducedMsgs)",
					totMsgCons: "Consommé",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "Nombre de messages consommés à partir de la file d'attente depuis le démarrage du serveur ou la création de la file d'attente, selon l'opération la plus récente. (ConsumedMsgs)",
					totMsgRej: "Rejeté",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "Nombre de messages qui ne sont pas envoyés à la file d'attente car le nombre maximal de messages en mémoire tampon est atteint. (RejectedMsgs)",
					totMsgExp: "Arrivé à expiration",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "Nombre de messages ayant expiré dans la file d'attente depuis que le serveur a été démarré ou que la file d'attente a été créée, selon l'opération la plus récente. (ExpiredMsgs)",			
					numProd: "Expéditeurs",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "Nombre d'expéditeurs actifs dans la file d'attente. (Producers)", 
					numCons: "Destinataires",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "Nombre de destinataires actifs dans la file d'attente. (Consumers)",
					maxMsgs: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "Nombre maximal de messages en mémoire tampon autorisé dans la file d'attente. Cette valeur est une indication plutôt qu'une limite absolue. Si le système s'exécute sous contrainte, le nombre de messages en mémoire tampon dans une file d'attente peut être légèrement supérieur à la valeur MaxMessages. (MaxMessages)"
				},
				destmapping: {
					ruleName: "Nom de règle",
					qmConnection: "Connexion du gestionnaire de files d'attente",
					commits: "Validations",
					rollbacks: "Annulations",
					committedMsgs: "Messages validés",
					persistentMsgs: "Messages persistants",
					nonPersistentMsgs: "Messages non persistants",
					status: "Statut",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "Nom de la règle de mappage de destination surveillée. (RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "Nom de la connexion du gestionnaire de files d'attente à laquelle la règle de mappage de destination est associée. (QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "Nombre d'opérations de validation exécutées pour la règle de mappage de destination. Une opération de validation peut valider de nombreux messages. (CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "Nombre d'opérations d'annulation exécutées pour la règle de mappage de destination. (RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "Nombre de messages validés pour la règle de mappage de destination. (CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "Nombre de messages persistants envoyés à l'aide de la règle de mappage de destination. (PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "Nombre de messages non persistants envoyés à l'aide de la règle de mappage de destination. (NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "Statut de la règle : Enabled, Disabled, Reconnecting ou Restarting. (Status)"
				},
				mqtt: {
					clientId: "ID client",
					lastConn: "Date de la dernière connexion",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "ID client. (ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "Heure à laquelle le client s'est connecté pour la dernière fois au serveur ${IMA_PRODUCTNAME_SHORT}. (LastConnectedTime)",	
					errorMessage: "Raison",
					errorMessageTooltip: "Raison pour laquelle le client n'a pas pu être supprimé."
				},
				subscription: {
					subsName: "Nom",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "Nom associé à l'abonnement. Cette valeur peut être une chaîne vide pour un abonnement non durable. (SubName)",
					topicString: "Chaîne de rubrique",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "Rubrique sur laquelle l'abonnement est souscrit. (TopicString)",					
					clientId: "ID client",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "ID du client propriétaire de l'abonnement. (ClientID)",
					consumers: "Destinataires",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "Nombre de destinataires de l'abonnement. (Consumers)",
					msgPublished: "Publié",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "Nombre de messages publiés sur cet abonnement depuis le démarrage du serveur ou la création de l'abonnement, selon l'opération la plus récente. (PublishedMsgs)",
					msgBuf: "Actuel",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "Nombre de messages publiés en attente d'être envoyés au client. (BufferedMsgs)",
					perMsgBuf: "% actuel du maximum",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "Pourcentage du nombre maximal de messages en mémoire tampon représenté par les messages actuellement en mémoire tampon. (BufferedPercent)",
					msgBufHWM: "Heures pleines",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "Nombre le plus élevé de messages en mémoire tampon depuis le démarrage du serveur ou la création de l'abonnement, selon l'opération la plus récente. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Pic de % du maximum",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "Pics de messages en mémoire tampon, sous la forme d'un pourcentage du nombre maximal de messages pouvant être placés en mémoire tampon. (BufferedHWMPercent)",
					maxMsgBuf: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "Nombre maximal de messages en mémoire tampon qui sont admis pour cet abonnement. (MaxMessages)",
					rejMsg: "Rejeté",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "Nombre de messages rejetés car le nombre maximal de messages en mémoire tampon a été atteint lors de la publication des messages sur l'abonnement. (RejectedMsgs)",
					expDiscMsg: "Expiré/supprimé",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "Nombre de messages qui ne sont pas distribués car ils sont arrivés à expiration ou ont été supprimés car la mémoire tampon était saturée. (ExpiredMsgs + DiscardedMsgs)",
					expMsg: "Arrivé à expiration",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "Nombre de messages qui ne sont pas distribués car ils sont arrivés à expiration. (ExpiredMsgs)",
					DiscMsg: "Supprimé",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "Nombre de messages qui ne sont pas distribués car ils ont été supprimés car la mémoire tampon était saturée. (DiscardedMsgs)",
					subsType: "Type",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "Cette colonne indique si l'abonnement est durable ou non durable et s'il est partagé. Les abonnements durables persistent après le redémarrage du serveur, sauf s'ils sont supprimés explicitement. (IsDurable, IsShared)",				
					isDurable: "Durable",
					isShared: "Partagé",
					notDurable: "Non durable",
					noName: "(aucun)",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "Expiré : {0} ; supprimé : {1}",
					errorMessage: "Raison",
					errorMessageTooltip: "Raison pour laquelle l'abonnement n'a pas pu être supprimé.",
					messagingPolicy: "Règle de messagerie",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "Nom de la règle de messagerie utilisée par l'abonnement. (MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "ID de transaction (XID)",
					timestamp: "Horodatage",
					state: "Statut",
					stateCode2: "Préparé",
					stateCode5: "Validation heuristique",
					stateCode6: "Annulation heuristique"
				},
				topic: {
					topicSubtree: "Chaîne de rubrique",
					msgPub: "Messages publiés",
					rejMsg: "Messages rejetés",
					rejPub: "Publications ayant échoué",
					numSubs: "Abonnés",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "Rubrique en cours de surveillance. La chaîne de rubrique contient toujours un caractère générique. (TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "Nombre de messages publiés avec succès dans une rubrique qui correspond à la chaîne de rubrique avec caractère générique. (PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "Nombre de messages rejetés par un ou plusieurs abonnements dans lesquels la qualité du niveau de service n'a pas provoqué l'échec de la demande de publication. (RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "Nombre de demandes de publication ayant échoué car le message est rejeté par un ou plusieurs abonnements. (FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "Nombre d'abonnements actifs sur les rubriques qui sont surveillées. La figure montre tous les abonnements actifs qui correspondent à la chaîne de rubrique avec caractère générique. (Subscriptions)"
				},
				store: {
					name: "Statistiques",
					value: "Valeur",
                    diskUsedPercent: "Disque utilisé (%)",
                    diskFreeBytes: "Disque disponible (Mo)",
					persistentMemoryUsedPercent: "Mémoire utilisée (%)",
					memoryTotalBytes: "Mémoire totale (Mo)",
					Pool1TotalBytes: "Mémoire totale du pool 1 (Mo)",
					Pool1UsedBytes: "Mémoire utilisée du pool 1 (Mo)",
					Pool1UsedPercent: "Mémoire utilisée du pool 1 (%)",
					Pool1RecordLimitBytes: "Nombre maximal d'enregistrements du pool 1 (Mo)", 
					Pool1RecordsUsedBytes: "Nombre d'enregistrements utilisés dans le pool 1 (Mo)",
                    Pool2TotalBytes: "Mémoire totale du pool 2 (Mo)",
                    Pool2UsedBytes: "Mémoire utilisée du pool 2 (Mo)",
                    Pool2UsedPercent: "Mémoire utilisée du pool 2 (%)"
				},
				memory: {
					name: "Statistiques",
					value: "Valeur",
					memoryTotalBytes: "Mémoire totale (Mo)",
					memoryFreeBytes: "Mémoire disponible (Mo)",
					memoryFreePercent: "Mémoire disponible (%)",
					serverVirtualMemoryBytes: "Mémoire virtuelle (Mo)",
					serverResidentSetBytes: "Résident (Mo)",
					messagePayloads: "Charges de message (Mo)",
					publishSubscribe: "Publication/abonnement (Mo)",
					destinations: "Destinations (Mo)",
					currentActivity: "Activité en cours (Mo)",
					clientStates: "Etats client (Mo)"
				}
			},
			logs: {
				title: "Journaux",
				pageSummary: "", 
				downloadLogsSubTitle: "Journaux de téléchargement",
				downloadLogsTagline: "Journaux de téléchargement à des fins de diagnostic.",
				lastUpdated: "Dernière mise à jour",
				name: "Fichier journal",
				size: "Taille (octets)"
			},
			lastUpdated: "Dernière mise à jour : ",
			bufferedMsgs: "Messages en mémoire tampon :",
			retainedMsgs: "Messages conservés :",
			dataCollectionInterval: "Intervalle de collecte de données : ",
			interval: {fiveSeconds: "5 secondes", oneMinute: "1 minute" },
			cacheInterval: "Intervalle de collecte de données : 1 minute",
			notAvailable: "n/a",
			savingProgress: "Enregistrement en cours...",
			savingFailed: "L'enregistrement a échoué.",
			noRecord: "Impossible de trouver un enregistrement sur le serveur.",
			deletingProgress: "Suppression en cours...",
			deletingFailed: "La suppression a échoué.",
			deletePending: "Suppression en attente.",
			deleteSuccessful: "La suppression a réussi.",
			testingProgress: "Test en cours...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "Indiquez la chaîne de rubrique à surveiller. La chaîne ne peut pas commencer par $SYS et doit se terminer par un caractère générique multiniveaux (par exemple, /animals/dogs/#). " +
						                    "Pour agréger des statistiques pour toutes les rubriques, indiquez un seul caractère générique multiniveaux (#). " +
						                    "Si une rubrique ne possède pas de rubrique enfant, le moniteur affiche des données uniquement pour cette rubrique.  " +
						                    "Par exemple, /animals/dogs/labradors/# surveille uniquement la rubrique /animals/dogs/labradors si labradors ne possède pas de rubrique enfant.",
				addTopicMonitorTitle: "Ajouter un moniteur de rubrique",
				removeTopicMonitorTitle: "Supprimer un moniteur de rubrique",
				removeSubscriptionTitle: "Supprimer un abonnement durable",
				removeSubscriptionsTitle: "Supprimer des abonnements durables",
				removeSubscriptionContent: "Voulez-vous vraiment supprimer cet abonnement ?",
				removeSubscriptionsContent: "Voulez-vous vraiment supprimer {0} abonnements ?",
				removeClientTitle: "Supprimer le client MQTT",
				removeClientsTitle: "Supprimer des clients MQTT",
				removeClientContent: "Voulez-vous vraiment supprimer ce client ? La suppression du client supprimera également les abonnements pour ce client.",
				removeClientsContent: "Voulez-vous vraiment supprimer {0} clients ? La suppression des clients supprimera également les abonnements pour ces clients.",
				topicStringLabel: "Chaîne de rubrique :",
				descriptionLabel: "Description",
				topicStringTooltip: "Chaîne de rubrique à surveiller.  A l'exception du caractère générique de fin requis (#), les caractères suivants ne sont pas autorisés dans la chaîne de rubrique : + #",
				saveButton: "Enregistrer",
				cancelButton: "Annuler"
			},
			removeDialog: {
				title: "Supprimer une entrée",
				content: "Voulez-vous vraiment supprimer cet enregistrement ?",
				deleteButton: "Supprimer",
				cancelButton: "Annuler"
			},
			actionConfirmDialog: {
				titleCommit: "Valider la transaction",
				titleRollback: "Annuler la transaction",
				titleForget: "Oublier la transaction",
				contentCommit: "Voulez-vous vraiment valider cette transaction ?",
				contentRollback: "Voulez-vous vraiment annuler cette transaction ?",
				contentForget: "Voulez-vous vraiment oublier cette transaction ?",
				actionButtonCommit: "Valider",
				actionButtonRollback: "Ramener à l'état antérieur",
				actionButtonForget: "Oublier",
				cancelButton: "Annuler",
				failedRollback: "Une erreur s'est produite lors de la tentative d'annulation de la transaction.",
				failedCommit: "Une erreur s'est produite lors de la tentative de validation de la transaction.",
				failedForget: "Une erreur s'est produite lors de la tentative d'oubli de la transaction."
			},
			asyncInfoDialog: {
				title: "Suppression en attente",
				content: "L'abonnement n'a pas pu être supprimé immédiatement. Il sera supprimé dès que possible.",
				closeButton: "Fermer"
			},
			deleteResultsDialog: {
				title: "Statut de suppression de l'abonnement",
				clientsTitle: "Statut de suppression du client",
				allSuccessful:  "Tous les abonnements ont été supprimés avec succès.",
				allClientsSuccessful: "Tous les clients ont été supprimés avec succès.",
				pendingTitle: "Suppression en attente",
				pending: "Les abonnements suivants n'ont pas pu être supprimés immédiatement. Ils seront supprimés dès que possible :",
				clientsPending: "Les clients suivants n'ont pas pu être supprimés immédiatement. Ils seront supprimés dès que possible :",
				failedTitle: "Echec de la suppression",
				failed:  "Les abonnements suivants n'ont pas pu être supprimés :",
				clientsFailed: "Les clients suivants n'ont pas pu être supprimés :",
				closeButton: "Fermer"					
			},
			invalidName: "Un enregistrement du même nom existe déjà.",
			// TRNOTE Do not translate "$SYS"
			reservedName: "La chaîne de rubrique ne peut pas commencer par $SYS.",
			invalidChars: "Les caractères suivants ne sont pas autorisés : + #",			
			invalidTopicMonitorFormat: "La chaîne de rubrique doit être #, ou se terminer par /#",
			invalidRequired: "Une valeur est requise.",
			clientIdMissingMessage: "Cette valeur est obligatoire. Vous pouvez utiliser un ou plusieurs astérisques (*) comme caractères génériques.",
			ruleNameMissingMessage: "Cette valeur est obligatoire. Vous pouvez utiliser un ou plusieurs astérisques (*) comme caractères génériques.",
			queueNameMissingMessage: "Cette valeur est obligatoire. Vous pouvez utiliser un ou plusieurs astérisques (*) comme caractères génériques.",
			subscriptionNameMissingMessage: "Cette valeur est obligatoire. Vous pouvez utiliser un ou plusieurs astérisques (*) comme caractères génériques.",
			discardOldestMessagesInfo: "Au moins un noeud final activé utilise la règle de messagerie avec le Comportement du nombre maximal de messages <em>Supprimer les anciens messages</em>.  Les messages qui sont supprimés n'affectent pas le nombre de <em>Messages rejetés</em> ou de <em>Publications ayant échoué</em>.",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "Paramètres SNMP",
				tagline: "Activez SNMPv2C (community-based Simple Network Management Protocol), téléchargez les fichiers MIB (Management Information Base) et configurez les communautés SNMPv2C. " +
						"${IMA_PRODUCTNAME_FULL} prend en charge SNMPv2C.",
				status: {
					title: "Statut SNMP",
					tagline: {
						disabled: "SNMP n'est pas activé.",
						enabled: "SNMP est activé et configuré.",
						warning: "Une communauté SNMP doit être définie pour permettre l'accès aux données SNMP sur votre serveur."
					},
					enableLabel: "Activer l'agent SNMPv2C",
					addressLabel: "Adresse de l'agent SNMP",					
					editLinkLabel: "Modifier",
					editAddressDialog: {
						title: "Edition de l'adresse de l'agent SNMP",
						instruction: "Spécifiez l'adresse IP statique qu'${IMA_PRODUCTNAME_FULL} peut utiliser pour SNMP. " +
								"Pour autoriser une adresse IP configurée sur le serveur ${IMA_PRODUCTNAME_SHORT}, ou si votre serveur n'utilise que des adresses IP dynamiques, sélectionnez <em>All</em>."
					},
					getSnmpEnabledError: "Une erreur s'est produite lors de l'extraction de l'état activé de SNMP.",
					setSnmpEnabledError: "Une erreur s'est produite lors de la définition de l'état activé de SNMP.",
					getSnmpAgentAddressError: "Une erreur s'est produite lors de l'extraction de l'adresse de l'agent SNMP.",
					setSnmpAgentAddressError: "Une erreur s'est produite lors de la définition de l'adresse de l'agent SNMP."
				},
				mibs: {
					title: "MIB d'entreprise",
					tagline: "Les fichiers Enterprise MIB décrivent les fonctions et les données disponibles auprès de l'agent SNMP imbriqué pour que votre client puisse y accéder de façon appropriée.",
					messageSightLinkLabel: "MIB ${IMA_PRODUCTNAME_SHORT}",
					hwNotifyLinkLabel: "MIB de notification matérielle"
				},
				communities: {
					title: "Communautés SNMP",
					tagline: "Vous pouvez définir l'accès aux données SNMP sur votre serveur en créant une ou plusieurs communautés SNMPv2C v2C. " +
							"Une communauté SNMP est nécessaire lorsque la surveillance est activée. " +
							"Toutes les communautés possèdent un accès en lecture seule.",
					grid: {
						nameColumn: "Nom",
						hostRestrictionColumn: "Restriction de l'hôte"
					},
					dialog: {
						addTitle: "Ajout d'une communauté SNMP",
						editTitle: "Edition d'une communauté SNMP",
						instruction: "Une communauté SNMP permet l'accès aux données SNMP sur votre serveur.",
						deleteTitle: "Suppression d'une communauté SNMP",
						deletePrompt: "Voulez-vous vraiment supprimer cette communauté SNMP ?",
						deleteNotAllowedTitle: "Suppression non autorisée", 
						deleteNotAllowedMessage: "La communauté SNMP n'a pas pu être supprimée car elle est utilisée par un ou plusieurs abonnés aux alertes. " +
								"Supprimez la communauté SNMP de tous les abonnés aux alertes, puis recommencez.",
						defineTitle: "Définition d'une communauté SNMP",
						defineInstruction: "Une communauté SNMP doit être définie pour permettre l'accès aux données SNMP sur votre serveur.",
						nameLabel: "Nom",
						nameInvalid: "La chaîne de communauté ne doit pas contenir de guillemets.",
						hostRestrictionLabel: "Restriction de l'hôte",
						hostRestrictionTooltip: "Lorsqu'elle est spécifiée, elle restreint l'accès aux hôtes spécifiés. " +
								"Si vous laissez cette zone vide, vous autorisez la communication avec toutes les adresses IP. " +
								"Spécifiez une liste de noms d'hôte ou d'adresses séparés par des virgules. " +
								"Pour spécifier un sous-réseau, utilisez un nom d'hôte ou une adresse IP CIDR (Classless Inter-Domain Routing).",
						hostRestrictionInvalid: "La restriction hôte doit être vide ou spécifier une liste séparée par des virgules contenant des adresses IP, des noms d'hôte ou des domaines valides."
					},
					getCommunitiesError: "Une erreur s'est produite lors de l'extraction des communautés SNMP.",
					saveCommunityError: "Une erreur s'est produite lors de la sauvegarde de la communauté SNMP.",
					deleteCommunityError: "Une erreur s'est produite lors de la suppression de la communauté SNMP."					
				},
				trapSubscribers: {
					title: "Abonnés aux alertes",
					tagline: "Les abonnées aux alertes représentent les clients SNMP utilisés pour communiquer avec l'agent SNMP imbriqué sur le serveur. " +
							"Les abonnés aux alertes SNMP doivent être des clients SNMP existants. " +
							"Configurez votre client SNMP pour qu'il reçoive des alertes avant de créer l'abonné aux alertes. ",
					grid: {
						hostColumn: "Hôte",
						portColumn: "Numéro de port du client",
						communityColumn: "Communauté"
					},
					dialog: {
						addTitle: "Ajout d'un abonné aux alertes",
						editTitle: "Edition d'un abonné aux alertes",
						instruction: "Un abonné aux alertes représente un client SNMP qui est utilisé pour communiquer avec l'agent SNMP imbriqué sur le serveur.",
						deleteTitle: "Suppression d'un abonné aux alertes",
						deletePrompt: "Voulez-vous vraiment supprimer cet abonné aux alertes ?",
						hostLabel: "Hôte",
						hostTooltip: "Adresse IP où le client SNMP écoute les informations d'alerte.",
						hostInvalid: "Une adresse IP valide est requise.",
				        duplicateHost: "Il existe déjà un abonné aux alertes pour cet hôte.",
						portLabel: "Numéro de port du client",
						portTooltip: "Port sur lequel le client SNMP écoute les informations d'alerte. " +
								"Les ports valides sont compris entre 1 et 65535, inclus.",
						portInvalid: "Le port doit être un nombre compris entre 1 et 65535, inclus.",
						communityLabel: "Communauté",
	                    noCommunitiesTitle: "Aucune communauté",
	                    noCommunitiesDetails: "Il n'est pas possible de définir un abonné aux alertes sans définir de communauté au préalable."						
					},
					getTrapSubscribersError: "Une erreur s'est produite lors de l'extraction des abonnés aux alertes SNMP.",
					saveTrapSubscriberError: "Une erreur s'est produite lors de la sauvegarde de l'abonné aux alertes SNMP.",
					deleteTrapSubscriberError: "Une erreur s'est produite lors de la suppression de l'abonné aux alertes SNMP."										
				},
				okButton: "OK",
				saveButton: "Enregistrer",
				cancelButton: "Annuler",
				closeButton: "Fermer"
			}
		}
});
