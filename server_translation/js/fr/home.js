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
			title : "Accueil",
			taskContainer : {
				title : "Tâches de configuration et de personnalisation communes",
				tagline: "Liens rapides vers les tâches administratives générales",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "({0} tâches restantes)",
				links : {
					restore : "Restaurer des tâches",
					restoreTitle: "Restaure des tâches fermées dans la section des tâches de configuration et de personnalisation communes.",
					hide : "Masquer la section",
					hideTitle: "Masquer la section des tâches de configuration et de personnalisation communes. " +
							"Pour restaurer la section, sélectionnez Restaurer les tâches sur la page d'accueil dans le menu Aide."						
				}
			},
			tasks : {
				messagingTester : {
					heading: "Vérifier votre configuration à l'aide de l’exemple d'application ${IMA_PRODUCTNAME_SHORT} Messaging Tester",
					description: "Messaging Tester est un modèle d'application HTML5 simple qui utilise MQTT sur WebSockets pour vérifier que le serveur est configuré correctement.",
					links: {
						messagingTester: "Exemple d'application Messaging Tester"
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
					heading : "Créer des utilisateurs",
					description : "Attribuez aux utilisateurs l'accès à l'interface utilisateur Web.",
					links : {
						users : "Utilisateurs de l'interface utilisateur Web",
						userGroups : "Configuration LDAP"						
					}
				},
				connections: {
					heading: "Configurer ${IMA_PRODUCTNAME_FULL} pour accepter des connexions",
					description: "Définissez un concentrateur de messages pour gérer les connexions du serveur.  Configurez MQ Connectivity pour connecter ${IMA_PRODUCTNAME_FULL} à des gestionnaires de files d'attente MQ. Configurez LDAP pour authentifier les utilisateurs de messagerie.",
					links: {
						listeners: "Concentrateurs de messages",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "Sécuriser votre environnement",
					description : "Contrôlez l'interface et le port sur lequel l'interface utilisateur Web est à l'écoute. Importez des clés et des certificats pour sécuriser les connexions au serveur.",
					links : {
						keysAndCerts : "Sécurité du serveur",
						webuiSettings : "Paramètres de l'interface utilisateur Web"
					}
				}
			},
			dashboards: {
				tagline: "Présentation des connexions serveur et des performances.",
				monitoringNotAvailable: "La surveillance n'est pas disponible en raison du statut du serveur ${IMA_PRODUCTNAME_SHORT}.",
				flex: {
					title: "Tableau de bord du serveur",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "Tableau de bord du serveur pour {0}",
					quickStats: {
						title: "Statistiques rapides"
					},
					liveCharts: {
						title: "Connexions actives et débit"
					},
					applianceResources: {
						title: "Utilisation de la mémoire du serveur"
					},
					resourceDetails: {
						title: "Utilisation de la mémoire de stockage"
					},
					diskResourceDetails: {
						title: "Utilisation des disques"
					},
					activeConnectionsQS: {
						title: "Nombre de connexions actives",
						description: "Nombre de connexions actives.",
						legend: "Connexions actives"
					},
					throughputQS: {
						title: "Débit actuel",
						description: "Débit actuel de messages par seconde",
						legend: "Messages/seconde"
					},
					uptimeQS: {
						title: "Temps d'activité d'${IMA_PRODUCTNAME_FULL}",
						description: "Statut d'${IMA_PRODUCTNAME_FULL} et durée d'exécution.",
						legend: "Temps d'activité de ${IMA_PRODUCTNAME_SHORT}"
					},
					applianceQS: {
						title: "Ressources du serveur",
						description: "Pourcentages de disque de stockage de persistance et de mémoire du serveur utilisés.",
						bars: {
							pMem: {
								label: "Mémoire permanente :"								
							},
							disk: { 
								label: "Disque :",
								warningThresholdText: "L'utilisation des ressources est égale ou supérieure au seuil d'avertissement.",
								alertThresholdText: "L'utilisation des ressources est égale ou supérieure au seuil d'alerte."
							},
							mem: {
								label: "Mémoire :",
								warningThresholdText: "L'utilisation de la mémoire devient élevée. Lorsque l'utilisation de la mémoire atteint 85%, des publications sont rejetées et des connexions peuvent être supprimées.",
								alertThresholdText: "L'utilisation de la mémoire est trop élevée. Des publications vont être rejetées et des connexions peuvent être supprimées."	
							}
						},
						warningThresholdAltText: "Icône d'avertissement",
						alertThresholdText: "L'utilisation des ressources est égale ou supérieure au seuil d'alerte.",
						alertThresholdAltText: "Icône d'alerte"
					},
					connections: {
						title: "Connexions",
						description: "Image instantanée des connexions prise toutes les cinq secondes environ.",
						legend: {
							x: "Heure",
							y: "Connexions"
						}
					},
					throughput: {
						title: "Débit",
						description: "Image instantanée des messages par seconde prise toutes les cinq secondes environ.",
						legend: {
							x: "Heure",						
							y: "Messages/seconde",
							title: "Messages/seconde",
							incoming: "Entrants",
							outgoing: "Sortants",
							hover: {
								incoming: "Image instantanée des messages lus par seconde depuis des clients, prise toutes les cinq secondes environ.",
								outgoing: "Image instantanée des messages écrits par seconde sur des clients, prise toutes les cinq secondes environ."
							}
						}
					},
					memory: {
						title: "Mémoire",
						description: "Image instantanée de l'utilisation de la mémoire prise toutes les minutes environ.",
						legend: {
							x: "Heure",
							y: "Mémoire utilisée (%)"
						}
					},
					disk: {
						title: "Disque",
						description: "Image instantanée de l'utilisation du disque de stockage de persistance prise toutes les minutes environ.",
						legend: {
							x: "Heure",
							y: "Disque utilisé (%)"
						}						
					},
					memoryDetail: {
						title: "Détails de l'utilisation de la mémoire",
						description: "Image instantanée de l'utilisation de la mémoire prise toutes les minutes environ pour les ressources de messagerie clé.",
						legend: {
							x: "Heure",
							y: "Mémoire utilisée (%)",
							title: "Utilisation de la mémoire de messagerie",
							system: "Système hôte du serveur",
							messagePayloads: "Charges de message",
							publishSubscribe: "Publication/abonnement",
							destinations: "Destinations",
							currentActivity: "Activité en cours",
							clientStates: "Etats du client",
							hover: {
								system: "Mémoire allouée pour les utilisations d'autres systèmes, par exemple le système d'exploitation, des connexions, la sécurité, la consignation et l'administration.",
								messagePayloads: "Mémoire allouée pour des messages. " +
										"Lorsqu'un message est publié pour plusieurs abonnés, une seule copie du message est allouée dans la mémoire. " +
										"La mémoire des messages conservés est également allouée dans cette catégorie. " +
										"Les charges de travail qui placent en mémoire tampon de gros volumes de messages sur le serveur pour des consommateurs déconnectés ou lents et les charges de travail qui utilisent un grand nombre de messages conservés peuvent consommer beaucoup de mémoire de charge de message.",
								publishSubscribe: "Mémoire allouée pour l'exécution de la messagerie de publication/abonnement. " +
										"Le serveur alloue de la mémoire dans cette catégorie pour conserver une trace des messages conservés et des abonnements. " +
										"Le serveur gère les caches des informations de publication/abonnement à des fins de performance. " +
										"Les charges de travail qui utilisent un grand nombre d'abonnements et les charges de travail qui utilisent un grand nombre de messages conservés peuvent consommer beaucoup de mémoire de publication/abonnement.",
								destinations: "Mémoire allouée pour les destinations de messagerie. " +
										"Cette catégorie de mémoire est utilisée pour organiser des messages dans les files d'attente et les abonnements qui sont utilisés par des clients. " +
										"Les charges de travail qui placent de nombreux messages en mémoire tampon sur le serveur et les charges de travail qui utilisent un grand nombre d'abonnements peuvent consommer beaucoup de mémoire de destination.",
								currentActivity: "Mémoire allouée pour l'activité en cours. " +
										"Inclut des sessions, des transactions et des accusés de réception de message. " +
										"Les informations permettant de répondre aux demandes de surveillance sont également allouées dans cette catégorie. " +
										"Les charges de travail complexes avec un grand nombre de clients connectés et les charges de travail qui utilisent de manière intensive des fonctions telles que des transactions ou des accusés de réception de message peuvent consommer beaucoup de mémoire en cours d'activité.",
								clientStates: "Mémoire allouée pour les clients connectés et déconnectés. " +
										"Le serveur alloue de la mémoire dans cette catégorie pour chaque client connecté. " +
										"Dans MQTT, les clients connectés avec le paramètre CleanSession défini sur 0 doivent être mémorisés lorsqu'ils sont déconnectés. " +
										"De plus, la mémoire est allouée depuis cette catégorie pour garder une trace des accusés de réception des messages entrants et sortants dans MQTT. " +
										"Les charges de travail qui utilisent un grand nombre de clients connectés et déconnectés peuvent consommer beaucoup de mémoire d'état client, tout particulièrement si la haute qualité des messages de service est utilisée."								
							}
						}
					},
					storeMemory: {
						title: "Détails de l'utilisation de la mémoire de stockage de persistance",
						description: "Image instantanée de l'utilisation de la mémoire de stockage de persistance prise toutes les minutes environ.",
						legend: {
							x: "Heure",
							y: "Mémoire utilisée (%)",
							pool1Title: "Utilisation de la mémoire du pool 1",
							pool2Title: "Utilisation de la mémoire du pool 2",
							system: "Système hôte du serveur",
							IncomingMessageAcks: "Accusés de réception des messages entrants",
							MQConnectivity: "MQ Connectivity",
							Transactions: "Transactions",
							Topics: "Rubriques avec messages conservés",
							Subscriptions: "Abonnements",
							Queues: "Files d'attente",										
							ClientStates: "Etats du client",
							recordLimit: "Nombre limite d'enregistrements",
							hover: {
								system: "Mémoire de stockage réservée ou en cours d'utilisation par le système.",
								IncomingMessageAcks: "Mémoire de stockage allouée pour accuser réception des messages entrants. " +
										"Le serveur alloue de la mémoire dans cette catégorie pour les clients MQTT qui se sont connectés à l'aide d'un paramètre CleanSession=0 et qui publient des messages dont la qualité de service est 2. " +
										"Cette mémoire est utilisée pour que la distribution ne soit effectuée qu'une fois et une seule.",
								MQConnectivity: "Mémoire de stockage allouée pour la connectivité avec les gestionnaires de file d'attente IBM WebSphere MQ.",
								Transactions: "Mémoire de stockage allouée aux transactions. " +
										"Le serveur alloue de la mémoire dans cette catégorie pour chaque transaction afin qu'il puisse terminer la restauration lors de son redémarrage.",
								Topics: "Mémoire de stockage allouée aux rubriques. " +
										"Le serveur alloue de la mémoire dans cette catégorie pour chaque rubrique contenant des messages conservés.",
								Subscriptions: "Mémoire de stockage allouée aux abonnements durables. " +
										"Dans MQTT, il s'agit des abonnements des clients qui se sont connectés à l'aide d'un paramètre CleanSession=0.",
								Queues: "Mémoire de stockage allouée aux files d'attente. " +
										"De la mémoire est allouée dans cette catégorie à chaque file d'attente créée pour la messagerie point-à-point.",										
								ClientStates: "Mémoire de stockage allouée aux clients qui doivent être mémorisés lorsqu'ils sont déconnectés. " +
										"Dans MQTT, il s'agit des clients qui se sont connectés à l'aide d'un paramètre CleanSession=0 ou des clients qui se sont connectés et ont défini un message-will dont la qualité de service est 1 ou 2.",
								recordLimit: "Lorsque le nombre maximal d'enregistrements est atteint, aucun enregistrement ne peut être créé pour les messages conservés, les abonnements durables, les files d'attente, les clients et la connectivité avec les gestionnaires de files d'attente WebSphere MQ."
							}
						}						
					},					
					peakConnectionsQS: {
						title: "Pic de connexions",
						description: "Pic des connexions actives sur la période spécifiée.",
						legend: "Pic de connexions"
					},
					avgConnectionsQS: {
						title: "Connexions moyennes",
						description: "Nombre moyen de connexions actives sur la période spécifiée.",
						legend: "Connexions moyennes"
					},
					peakThroughputQS: {
						title: "Pic du débit",
						description: "Pic de messages par seconde sur la période spécifiée.",
						legend: "Pic de messages/seconde"
					},
					avgThroughputQS: {
						title: "Débit moyen",
						description: "Nombre moyen de messages par seconde sur la période spécifiée.",
						legend: "Moyenne de messages/seconde"
					}
				}
			},
			status: {
				rc0: "Initialisation en cours",
				rc1: "Exécution (production)",
				rc2: "Arrêt",
				rc3: "Initialisé",
				rc4: "Transport démarré",
				rc5: "Protocole démarré",
				rc6: "Stockage démarré",
				rc7: "Moteur démarré",
				rc8: "Messagerie démarré",
				rc9: "Exécution (maintenance)",
				rc10: "De secours",
				rc11: "Stockage en cours de démarrage",
				rc12: "Maintenance (nettoyage du magasin en cours)",
				rc99: "Arrêté",				
				unknown: "Inconnu",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "Lorsque le serveur est redémarré, il est en mode <em>{0}</em>.",
				cleanStoreSelected: "L'action <em>Nettoyer le magasin</em> a été demandée. Elle prendra effet lorsque le serveur est redémarré.",
				mode_0: "production",
				mode_1: "maintenance",
				mode_2: "nettoyer le magasin",
				adminError: "${IMA_PRODUCTNAME_FULL} détecte une erreur.",
				adminErrorDetails: "Code d'erreur : {0}. Chaîne d'erreur : {1}"
			},
			storeStatus: {
				mode_0: "persistant",
				mode_1: "mémoire uniquement"
			},
			memoryStatus: {
				ok: "OK",
				unknown: "Inconnu",
				error: "La vérification de la mémoire signale une erreur.",
				errorMessage: "La vérification de la mémoire signale une erreur. Contactez le support."
			},
			role: {
				PRIMARY: "Noeud principal",
				PRIMARY_SYNC: "Noeud principal (synchronisation, {0}% terminé)",
				STANDBY: "Noeud de secours",
				UNSYNC: "Noeud hors synchronisation",
				TERMINATE: "Noeud de secours terminé par le noeud principal",
				UNSYNC_ERROR: "Le noeud ne peut plus être synchronisé.",
				HADISABLED: "Désactivé",
				UNKNOWN: "Inconnu",
				unknown: "Inconnu"
			},
			haReason: {
				"1": "Les configurations du magasin des deux noeuds ne leur permet pas de travailler en tant que paire à haute disponibilité. " +
				     "L'élément de configuration non-concordant est : {0}",
				"2": "Temps de reconnaissance expiré. Echec de la communication avec l'autre noeud de la paire à haute disponibilité.",
				"3": "Deux noeuds principaux ont été identifiés.",
				"4": "Deux noeuds non synchronisés ont été identifiés.",
				"5": "Impossible de déterminer quel noeud doit être le noeud principal car les deux noeuds comportent des magasins non vides.",
				"7": "Le noeud n'est pas parvenu à composer une paire avec le noeud distant car les deux noeuds ont des identificateurs de groupe différents.",
				"9": "Une erreur interne ou de serveur à haute disponibilité est survenue",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "Ce noeud était le noeud principal le {0}."
			},
			statusControl: {
				label: "Statut",
				ismServer: "Serveur ${IMA_PRODUCTNAME_FULL} :",
				serverNotAvailable: "Prenez contact avec l'administrateur système du serveur pour démarrer le serveur",
				serverNotAvailableNonAdmin: "Prenez contact avec l'administrateur système du serveur pour démarrer le serveur",
				haRole: "Haute disponibilité :",
				pending: "En attente..."
			}
		},
		recordsLabel: "Enregistrements",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "Cluster :",
		clusterStatus_Active: "Actif",
		clusterStatus_Inactive: "Inactif",
		clusterStatus_Removed: "Retiré",
		clusterStatus_Connecting: "Connexion en cours",
		clusterStatus_None: "Non configurée",
		clusterStatus_Unknown: "Inconnu",
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "Distant",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "Local",
		clusterLegendIncoming: "Entrants",
		clusterLegendOutgoing: "Sortants",
		clusterHoverIncoming: "Image instantanée des messages lus par seconde depuis des membres de cluster distants, prise toutes les cinq secondes environ.",
		clusterHoverOutgoing: "Image instantanée des messages écrits par seconde sur des membres de cluster distants, prise toutes les cinq secondes environ."

});
