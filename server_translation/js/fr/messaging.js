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
			title : "Messagerie",
			users: {
				title: "Authentification d'utilisateur",
				tagline: "Configurez LDAP pour authentifier les utilisateurs du serveur de messagerie."
			},
			endpoints: {
				title: "Configuration des noeuds finaux",
				listenersSubTitle: "Noeuds finaux",
				endpointsSubTitle: "Définir un noeud final",
		 		form: {
					enabled: "Activé",
					name: "Nom",
					description: "Description",
					port: "Port",
					ipAddr: "Adresse IP",
					all: "Tous",
					protocol: "Protocole",
					security: "Sécurité",
					none: "Aucun",
					securityProfile: "Profil de sécurité",
					connectionPolicies: "Règles de connexion",
					connectionPolicy: "Règles de connexion",
					messagingPolicies: "Règles de messagerie",
					messagingPolicy: "Règle de messagerie",
					destinationType: "Type de destination",
					destination: "Destination",
					maxMessageSize: "Taille maximale de message",
					selectProtocol: "Sélectionner un protocole",
					add: "Ajouter",
					tooltip: {
						description: "",
						port: "Entrez un port disponible. Les ports valides sont compris entre 1 et 65535, inclus.",
						connectionPolicies: "Ajoutez au moins une règle de connexion. Une règle de connexion autorise des clients à se connecter au noeud final. " +
								"Les règles de connexion sont évaluées dans l'ordre indiqué. Pour modifier l'ordre, utilisez les flèches vers le haut et vers le bas de la barre d'outils.",
						messagingPolicies: "Ajoutez au moins une règle de messagerie. " +
								"Une règle de messagerie autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les rubriques ou les files d'attente auxquelles le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
								"Les règles de messagerie sont évaluées dans l'ordre indiqué. Pour modifier l'ordre, utilisez les flèches vers le haut et vers le bas de la barre d'outils.",									
						maxMessageSize: "Taille maximale des messages en Ko. Les valeurs valides sont comprises entre 1 et 262,144, inclus.",
						protocol: "Indiquez des protocoles valides pour ce noeud final."
					},
					invalid: {						
						port: "Le numéro de port doit être un nombre compris entre 1 et 65535, inclus.",
						maxMessageSize: "La taille maximale de message doit être un nombre compris entre 1 et 262,144, inclus.",
						ipAddr: "Une adresse IP valide est requise.",
						security: "Une valeur est requise."
					},
					duplicateName: "Un enregistrement du même nom existe déjà."
				},
				addDialog: {
					title: "Ajouter un noeud final",
					instruction: "Un noeud final est un port auquel les applications client peuvent se connecter.  Un noeud final doit avoir au moins une règle de connexion et une règle de messagerie."
				},
				removeDialog: {
					title: "Supprimer un noeud final",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
				editDialog: {
					title: "Modifier un noeud final",
					instruction: "Un noeud final est un port auquel les applications client peuvent se connecter.  Un noeud final doit avoir au moins une règle de connexion et une règle de messagerie."
				},
				addProtocolsDialog: {
					title: "Ajouter des protocoles au noeud final",
					instruction: "Ajoutez des protocoles autorisés pour vous connecter à ce noeud final. Au moins un protocole doit être sélectionné.",
					allProtocolsCheckbox: "Tous les protocoles sont valides pour ce noeud final.",
					protocolSelectErrorTitle: "Aucun protocole sélectionné.",
					protocolSelectErrorMsg: "Au moins un protocole doit être sélectionné. Indiquez que tous les protocoles doivent être ajoutés ou sélectionnez des protocoles spécifiques dans la liste des protocoles."
				},
				addConnectionPoliciesDialog: {
					title: "Ajouter des règles de connexion au noeud final",
					instruction: "Une règle de connexion autorise des clients à se connecter aux noeuds finaux ${IMA_PRODUCTNAME_FULL}. " +
						"Chaque noeud final doit avoir au moins une règle de connexion."
				},
				addMessagingPoliciesDialog: {
					title: "Ajouter des règles de messagerie au noeud final",
					instruction: "Une règle de messagerie permet de contrôler les rubriques, files d'attente ou abonnements partagés globalement auxquels un client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
							"Chaque noeud final doit avoir au moins une règle de messagerie. " +
							"Si un noeud final a une règle de messagerie pour un abonnement partagé globalement, il doit également avoir une règle de messagerie pour les rubriques souscrites."
				},
                retrieveEndpointError: "Une erreur s'est produite lors de l'extraction des noeuds finaux.",
                saveEndpointError: "Une erreur s'est produite lors de l'enregistrement du noeud final.",
                deleteEndpointError: "Une erreur s'est produite lors de la suppression du noeud final.",
                retrieveSecurityProfilesError: "Une erreur s'est produite lors de l'extraction des règles de sécurité."
			},
			connectionPolicies: {
				title: "Règles de connexion",
				grid: {
					applied: "Appliqué(e)",
					name: "Nom"
				},
		 		dialog: {
					name: "Nom",
					protocol: "Protocole",
					description: "Description",
					clientIP: "Adresse IP du client",  
					clientID: "ID client",
					ID: "ID utilisateur",
					Group: "ID groupe",
					selectProtocol: "Sélectionner un protocole",
					commonName: "Nom usuel du certificat",
					protocol: "Protocole",
					tooltip: {
						name: "",
						filter: "Vous devez spécifier au moins l'une des zones de filtre. " +
								"Un seul astérisque (*) peut être utilisé comme dernier caractère dans la plupart des filtres pour 0 caractère ou plus. " +
								"L'adresse IP du client peut contenir un astérisque (*) ou une liste séparée par des virgules d'adresses IPv4 ou IPv6 ou de plages d'adresses, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::]."
					},
					invalid: {						
						filter: "Vous devez spécifier au moins l'une des zones de filtre.",
						wildcard: "Un seul astérisque (*) peut être utilisé à la fin de la valeur pour 0 caractère ou plus.",
						vars: "Ne doit pas contenir la variable de substitution ${UserID}, ${GroupID}, ${ClientID} ou ${CommonName}.",
						clientIDvars: "Ne doit pas contenir la variable de substitution ${GroupID} ou ${ClientID}.",
						clientIP: "L'adresse IP du client doit contenir un astérisque (*) ou une liste séparée par des virgules d'adresses IPv4 ou IPv6 ou de plages d'adresses, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::]."
					}										
				},
				addDialog: {
					title: "Ajouter une règle de connexion",
					instruction: "Une règle de connexion autorise des clients à se connecter aux noeuds finaux ${IMA_PRODUCTNAME_FULL}. Chaque noeud final doit avoir au moins une règle de connexion."
				},
				removeDialog: {
					title: "Supprimer une règle de connexion",
					instruction: "Une règle de connexion autorise des clients à se connecter aux noeuds finaux ${IMA_PRODUCTNAME_FULL}. Chaque noeud final doit avoir au moins une règle de connexion.",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
                removeNotAllowedDialog: {
                	title: "Suppression non autorisée",
                	content: "La règle de connexion ne peut pas être supprimée car elle est utilisée par un ou plusieurs noeuds finaux. " +
                			 "Supprimez la règle de connexion de tous les noeuds finaux puis renouvelez l'opération.",
                	closeButton: "Fermer"
                },								
				editDialog: {
					title: "Modifier une règle de connexion",
					instruction: "Une règle de connexion autorise des clients à se connecter aux noeuds finaux ${IMA_PRODUCTNAME_FULL}. Chaque noeud final doit avoir au moins une règle de connexion."
				},
                retrieveConnectionPolicyError: "Une erreur s'est produite lors de l'extraction des règles de connexion.",
                saveConnectionPolicyError: "Une erreur s'est produite lors de l'enregistrement de la règle de connexion.",
                deleteConnectionPolicyError: "Une erreur s'est produite lors de la suppression de la règle de connexion."
 			},
			messagingPolicies: {
				title: "Règles de messagerie",
				listSeparator : ",",
		 		dialog: {
					name: "Nom",
					description: "Description",
					destinationType: "Type de destination",
					destinationTypeOptions: {
						topic: "Rubrique",
						subscription: "Abonnement partagé globalement",
						queue: "File d'attente"
					},
					topic: "Rubrique",
					queue: "File d'attente",
					selectProtocol: "Sélectionner un protocole",
					destination: "Destination",
					maxMessages: "Nombre maximal de messages",
					maxMessagesBehavior: "Comportement du nombre maximal de messages",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "Rejeter les nouveaux messages",
						DiscardOldMessages: "Supprimer les anciens messages"
					},
					action: "Droits d'accès",
					actionOptions: {
						publish: "Publier",
						subscribe: "S'abonner",
						send: "Envoyer",
						browse: "Parcourir",
						receive: "Recevoir",
						control: "Contrôler"
					},
					clientIP: "Adresse IP du client",  
					clientID: "ID client",
					ID: "ID utilisateur",
					Group: "ID groupe",
					commonName: "Nom usuel du certificat",
					protocol: "Protocole",
					disconnectedClientNotification: "Notification de client déconnecté",
					subscriberSettings: "Paramètres des abonnés",
					publisherSettings: "Paramètres de l'éditeur",
					senderSettings: "Paramètres de l'émetteur",
					maxMessageTimeToLive: "Durée de vie maximale des messages",
					unlimited: "illimitée",
					unit: "secondes",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} secondes",
					tooltip: {
						name: "",
						destination: "Rubrique de message, file d'attente ou abonnement partagé globalement auquel s'applique cette règle. Utilisez l'astérisque (*) avec précaution. " +
							"Un astérisque correspond à 0 caractère ou plus, y compris la barre oblique (/). Par conséquent, il peut correspondre à plusieurs niveaux dans une arborescence de rubriques.",
						maxMessages: "Nombre maximal de messages à conserver pour un abonnement. Ce nombre doit être compris entre 1 et 20 000000.",
						maxMessagesBehavior: "Comportement appliqué lorsque la mémoire tampon d'un abonnement est saturée. " +
								"C'est-à-dire lorsque le nombre de messages dans la mémoire tampon d'un abonnement atteint la valeur Max Messages.<br />" +
								"<strong>Rejeter les nouveaux messages :&nbsp;</strong> lorsque la mémoire tampon est saturée, les nouveaux messages sont rejetés.<br />" +
								"<strong>Supprimer les anciens messages :&nbsp;</strong> Lorsque la mémoire tampon est saturée et qu'un nouveau message arrive, les messages non distribués les plus anciens sont supprimés.",
						discardOldestMessages: "Ce paramètre peut entraîner la non distribution de certains messages aux abonnés, même si le diffuseur reçoit un accusé de livraison. " +
								"Les messages sont supprimés même si le diffuseur et l'abonné ont demandé une messagerie fiable.",
						action: "Actions autorisées par cette règle.",
						filter: "Vous devez spécifier au moins l'une des zones de filtre. " +
								"Un seul astérisque (*) peut être utilisé comme dernier caractère dans la plupart des filtres pour 0 caractère ou plus. " +
								"L'adresse IP du client peut contenir un astérisque (*) ou une liste séparée par des virgules d'adresses IPv4 ou IPv6 ou de plages d'adresses, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::].",
				        disconnectedClientNotification: "Indique si des messages de notification sont publiés pour les clients MQTT déconnectés à l'arrivée d'un message. " +
				        		"Les notifications sont activées uniquement pour les clients MQTT CleanSession=0.",
				        protocol: "Activation du filtrage des protocoles pour la règle de messagerie. Si le filtrage est activé, un ou plusieurs protocoles doivent être spécifiés. S'il n'est pas activé, tous les protocoles seront autorisés pour la règle de messagerie.",
				        destinationType: "Type de destination auquel autoriser l'accès. " +
				        		"Pour autoriser l'accès à un abonnement partagé globalement, deux règles de messagerie sont requises :"+
				        		"<ul><li>Une règle de messagerie avec le type de destination <em>rubrique</em>, qui permet d'accéder à une ou plusieurs rubriques.</li>" +
				        		"<li>Une règle de messagerie avec le type de destination <em>abonnement partagé</em>, qui permet d'accéder à l'abonnement durable partagé globalement sur ces rubriques.</li></ul>",
						action: {
							topic: "<dl><dt>Publier :</dt><dd>Permet aux clients de publier des messages dans la rubrique qui est spécifiée dans la règle de messagerie.</dd>" +
							       "<dt>S'abonner :</dt><dd>Permet aux clients de s'abonner à la rubrique spécifiée dans la règle de messagerie.</dd></dl>",
							queue: "<dl><dt>Envoyer :</dt><dd>Permet aux clients d'envoyer des messages à la file d'attente spécifiée dans la règle de messagerie.</dd>" +
								    "<dt>Parcourir :</dt><dd>Permet aux clients de parcourir la file d'attente qui est spécifiée dans la règle de messagerie.</dd>" +
								    "<dt>Recevoir :</dt><dd>Permet aux clients de recevoir des messages de la file d'attente spécifiée dans la règle de messagerie.</dd></dl>",
							subscription:  "<dl><dt>Recevoir :</dt><dd>Permet aux clients de recevoir des messages de l'abonnement partagé globalement spécifié dans la règle de messagerie.</dd>" +
									"<dt>Contrôler :</dt><dd>Permet aux clients de créer et de supprimer l'abonnement partagé globalement spécifié dans la règle de messagerie.</dd></dl>"
							},
						maxMessageTimeToLive: "Nombre maximal de secondes pendant lequel un message publié sous la règle peut exister. " +
								"Si le diffuseur indique une valeur d'expiration plus petite, c'est la valeur du diffuseur qui est utilisée. " +
								"Les valeurs valides sont <em>illimitées</em> ou comprises entre 1 et 2,147,483,647 secondes. La valeur <em>illimitée</em> n'a pas de maximum défini.",
						maxMessageTimeToLiveSender: "Nombre maximal de secondes pendant lequel un message envoyé sous la règle peut exister. " +
								"Si l'émetteur indique une valeur d'expiration plus petite, c'est la valeur de l'émetteur qui est utilisée. " +
								"Les valeurs valides sont <em>illimitées</em> ou comprises entre 1 et 2,147,483,647 secondes. La valeur <em>illimitée</em> n'a pas de maximum défini."
					},
					invalid: {						
						maxMessages: "La valeur est un nombre compris entre 1 et 20,000,000.",                       
						filter: "Vous devez spécifier au moins l'une des zones de filtre.",
						wildcard: "Un seul astérisque (*) peut être utilisé à la fin de la valeur pour 0 caractère ou plus.",
						vars: "Ne doit pas contenir la variable de substitution ${UserID}, ${GroupID}, ${ClientID} ou ${CommonName}.",
						clientIP: "L'adresse IP du client doit contenir un astérisque (*) ou une liste séparée par des virgules d'adresses IPv4 ou IPv6 ou de plages d'adresses, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								  "Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::].",
					    subDestination: "La substitution de variable d'ID client n'est pas autorisée lorsque le type de destination est <em>abonnement partagé</em>.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "Le ou les protocoles {0} ne sont pas valides pour le type de destination {1}.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "Le ou les protocoles {0} spécifiés pour le filtre de protocole ne sont pas valides pour le type de destination {1}.",
					    maxMessageTimeToLive: "La valeur doit être <em>illimitée</em> ou comprise entre 1 et 2,147,483,647."
					}					
				},
				addDialog: {
					title: "Ajouter une règle de messagerie",
					instruction: "Une règle de messagerie autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les rubriques, les files d'attente ou les abonnements partagés globalement auxquels le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
							     "Dans un abonnement partagé globalement, le travail de réception des messages à partir d'un abonnement de rubrique durable est partagé entre plusieurs abonnés. Chaque noeud final doit avoir au moins une règle de messagerie."
				},
				removeDialog: {
					title: "Supprimer une règle de messagerie",
					instruction: "Une règle de messagerie autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les rubriques, les files d'attente ou les abonnements partagés globalement auxquels le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
							"Dans un abonnement partagé globalement, le travail de réception des messages à partir d'un abonnement de rubrique durable est partagé entre plusieurs abonnés. Chaque noeud final doit avoir au moins une règle de messagerie.",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
                removeNotAllowedDialog: {
                	title: "Suppression non autorisée",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "La règle de type {0} ne peut pas être supprimée car elle est utilisée par un ou plusieurs noeuds finaux. " +
                			 "Supprimez la règle de type {0} de tous les noeuds finaux, puis renouvelez l'opération.",
                	closeButton: "Fermer"
                },				
				editDialog: {
					title: "Modifier une règle de messagerie",
					instruction: "Une règle de messagerie autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les rubriques, les files d'attente ou les abonnements partagés globalement auxquels le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
							     "Dans un abonnement partagé globalement, le travail de réception des messages à partir d'un abonnement de rubrique durable est partagé entre plusieurs abonnés. Chaque noeud final doit avoir au moins une règle de messagerie. "
				},
				viewDialog: {
					title: "Afficher une règle de messagerie"
				},		
				confirmSaveDialog: {
					title: "Sauvegarder une règle de messagerie",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "Cette règle est utilisée par environ {0} abonnés ou expéditeurs. " +
							"Les clients autorisés par cette règle utiliseront les nouveaux paramètres suivants : {1}" +
							"Voulez-vous vraiment modifier cette règle ?",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "Cette règle est peut-être utilisée par des abonnés ou des expéditeurs. " +
							"Les clients autorisés par cette règle utiliseront les nouveaux paramètres suivants : {1}" +
							"Voulez-vous vraiment modifier cette règle ?",
					saveButton: "Enregistrer",
					closeButton: "Annuler"
				},
                retrieveMessagingPolicyError: "Une erreur s'est produite lors de l'extraction des règles de messagerie.",
                retrieveOneMessagingPolicyError: "Une erreur s'est produite lors de l'extraction de la règle de messagerie.",
                saveMessagingPolicyError: "Une erreur s'est produite lors de l'enregistrement de la règle de messagerie.",
                deleteMessagingPolicyError: "Une erreur s'est produite lors de la suppression de la règle de messagerie.",
                pendingDeletionMessage:  "Cette règle est en attente de suppression.  Elle sera supprimée lorsqu'elle ne sera plus utilisée.",
                tooltip: {
                	discardOldestMessages: "Comportement du nombre maximal de messages est défini sur <em>Supprimer les anciens messages</em>. " +
                			"Ce paramètre peut entraîner la non distribution de certains messages aux abonnés, même si le diffuseur reçoit un accusé de livraison. " +
                			"Les messages sont supprimés même si le diffuseur et l'abonné ont demandé une messagerie fiable."
                }
			},
			messageProtocols: {
				title: "Protocoles de messagerie",
				subtitle: "Les protocoles de messagerie sont utilisés pour envoyer des messages entre des clients et ${IMA_PRODUCTNAME_FULL}.",
				tagline: "Protocoles de messagerie disponibles et capacités associées",
				messageProtocolsList: {
					name: "Nom du protocole",
					topic: "Rubrique",
					shared: "Abonnement partagé globalement",
					queue: "File d'attente",
					browse: "Parcourir"
				}
			},
			messageHubs: {
				title: "Concentrateurs de messages",
				subtitle: "Les administrateurs système et les administrateurs de messagerie peuvent définir, modifier ou supprimer des concentrateurs de message. " +
						  "Un concentrateur de message est un objet de configuration organisationnel qui regroupe des noeuds finaux, des règles de connexion et des règles de messagerie connexes. <br /><br />" +
						  "Un noeud final est un port auquel des applications client peuvent se connecter. Un noeud final doit avoir au moins une règle de connexion et une règle de messagerie. " +
						  "La règle de connexion autorise des clients à se connecter au noeud final, alors que la règle de messagerie autorise des clients à exécuter des actions de messagerie spécifiques une fois connectés au noeud final. ",
				tagline: "Définir, modifier ou supprimer un concentrateur de messages",
				defineAMessageHub: "Définir un concentrateur de messages",
				editAMessageHub: "Modifier un concentrateur de messages",
				defineAnEndpoint: "Définir un noeud final",
				endpointTabTagline: "Un noeud final est un port auquel les applications client peuvent se connecter.  " +
						"Un noeud final doit avoir au moins une règle de connexion et une règle de messagerie.",
				messagingPolicyTabTagline: "Une règle de messagerie permet de contrôler les rubriques, files d'attente ou abonnements partagés globalement auxquels un client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
						"Chaque noeud final doit avoir au moins une règle de messagerie.",
				connectionPolicyTabTagline: "Une règle de connexion autorise des clients à se connecter aux noeuds finaux ${IMA_PRODUCTNAME_FULL}. " +
						"Chaque noeud final doit avoir au moins une règle de connexion.",						
                retrieveMessageHubsError: "Une erreur s'est produite lors de l'extraction des concentrateurs de messages.",
                saveMessageHubError: "Une erreur s'est produite lors de l'enregistrement du concentrateur de messages.",
                deleteMessageHubError: "Une erreur s'est produite lors de la suppression du concentrateur de messages.",
                messageHubNotFoundError: "Le concentrateur de messages est introuvable. Il a peut-être été supprimé par un autre utilisateur.",
                removeNotAllowedDialog: {
                	title: "Suppression non autorisée",
                	content: "Le concentrateur de messages ne peut pas être supprimé car il contient un ou plusieurs noeuds finaux. " +
                			 "Modifiez le concentrateur de messages pour supprimer les noeuds finaux puis renouvelez l'opération.",
                	closeButton: "Fermer"
                },
                addDialog: {
                	title: "Ajouter un concentrateur de messages",
                	instruction: "Définissez un concentrateur de messages pour gérer les connexions du serveur.",
                	saveButton: "Enregistrer",
                	cancelButton: "Annuler",
                	name: "Nom :",
                	description: "Description :"
                },
                editDialog: {
                	title: "Modifier les propriétés du concentrateur de messages",
                	instruction: "Modifiez le nom et la description du concentrateur de messages."
                },                
				messageHubList: {
					name: "Concentrateur de messages",
					description: "Description",
					metricLabel: "Noeuds finaux",
					removeDialog: {
						title: "Supprimer le concentrateur de messages",
						content: "Voulez-vous vraiment supprimer ce concentrateur de messages ?"
					}
				},
				endpointList: {
					name: "Noeud final",
					description: "Description",
					connectionPolicies: "Règles de connexion",
					messagingPolicies: "Règles de messagerie",
					port: "Port",
					enabled: "Activé",
					status: "Statut",
					up: "Vers le haut",
					down: "Vers le bas",
					removeDialog: {
						title: "Supprimer un noeud final",
						content: "Voulez-vous vraiment supprimer ce noeud final ?"
					}
				},
				messagingPolicyList: {
					name: "Règle de messagerie",
					description: "Description",					
					metricLabel: "Noeuds finaux",
					destinationLabel: "Destination",
					maxMessagesLabel: "Nombre maximal de messages",
					disconnectedClientNotificationLabel: "Notification de client déconnecté",
					actionsLabel: "Droits d'accès",
					useCountLabel: "Nombre d'utilisations",
					unknown: "Inconnu",
					removeDialog: {
						title: "Supprimer une règle de messagerie",
						content: "Voulez-vous vraiment supprimer cette règle de messagerie ?"
					},
					deletePendingDialog: {
						title: "Suppression en attente",
						content: "La demande de suppression a été reçue, mais la règle ne peut pas être supprimée à l'heure actuelle. " +
							"La règle est utilisée par environ {0} abonnés ou expéditeurs. " +
							"La règle sera supprimée lorsqu'elle ne sera plus utilisée.",
						contentUnknown: "La demande de suppression a été reçue, mais la règle ne peut pas être supprimée à l'heure actuelle. " +
						"La règle est peut-être utilisée par des abonnés ou des expéditeurs. " +
						"La règle sera supprimée lorsqu'elle ne sera plus utilisée.",
						closeButton: "Fermer"
					},
					deletePendingTooltip: "Cette règle sera supprimée lorsqu'elle ne sera plus utilisée."
				},	
				connectionPolicyList: {
					name: "Règles de connexion",
					description: "Description",					
					endpoints: "Noeuds finaux",
					removeDialog: {
						title: "Supprimer une règle de connexion",
						content: "Voulez-vous vraiment supprimer cette règle de connexion ?"
					}
				},				
				messageHubDetails: {
					backLinkText: "Revenir aux concentrateurs de messages",
					editLink: "Modifier",
					endpointsTitle: "Noeuds finaux",
					endpointsTip: "Configurez des noeuds finaux et des règles de connexion pour ce concentrateur de messages.",
					messagingPoliciesTitle: "Règles de messagerie",
					messagingPoliciesTip: "Configurez des règles de messagerie pour ce concentrateur de messages.",
					connectionPoliciesTitle: "Règles de connexion",
					connectionPoliciesTip: "Configurez des règles de connexion pour ce concentrateur de messages."
				},
				hovercard: {
					name: "Nom",
					description: "Description",
					endpoints: "Noeuds finaux",
					connectionPolicies: "Règles de connexion",
					messagingPolicies: "Règles de messagerie",
					warning: "Avertissement"
				}
			},
			referencedObjectGrid: {
				applied: "Appliqué(e)",
				name: "Nom"
			},
			savingProgress: "Enregistrement en cours...",
			savingFailed: "L'enregistrement a échoué.",
			deletingProgress: "Suppression en cours...",
			deletingFailed: "La suppression a échoué.",
			refreshingGridMessage: "Mise à jour en cours...",
			noItemsGridMessage: "Aucun élément à afficher",
			testing: "Test en cours...",
			testTimedOut: "Le test de connexion est trop long.",
			testConnectionFailed: "Le test de connexion a échoué.",
			testConnectionSuccess: "Le test de connexion a réussi.",
			dialog: {
				saveButton: "Enregistrer",
				deleteButton: "Supprimer",
				deleteContent: "Voulez-vous vraiment supprimer cet enregistrement ?",
				cancelButton: "Annuler",
				closeButton: "Fermer",
				testButton: "Tester la connexion",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "Test de connexion à {0}...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "Le test de connexion à {0} a échoué.",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "Le test de connexion à {0} a abouti."
			},
			updating: "Mise à jour en cours...",
			invalidName: "Un enregistrement du même nom existe déjà.",
			filterHeadingConnection: "Pour restreindre des connexions à des clients spécifiques à l'aide de cette règle, indiquez un ou plusieurs des filtres suivants. " +
					"Par exemple, sélectionnez <em>ID groupe</em> pour restreindre cette règle aux membres d'un groupe particulier. " +
					"La règle autorise l'accès uniquement lorsque tous les filtres spécifiés sont satisfaits :",
			filterHeadingMessaging: "Pour restreindre les actions de messagerie définies dans cette règle à des clients spécifiques, indiquez un ou plusieurs des filtres suivants. " +
					"Par exemple, sélectionnez <em>ID groupe</em> pour restreindre cette règle aux membres d'un groupe particulier. " +
					"La règle autorise l'accès uniquement lorsque tous les filtres spécifiés sont satisfaits :",
			settingsHeadingMessaging: "Indiquez les ressources et les actions de messagerie auxquelles le client est autorisé à accéder :",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "Configurer des connexions sur un ou plusieurs gestionnaires de files d'attente WebSphere MQ."				
			},
			connectionProperties: {
				title: "Propriétés de connexion du gestionnaire de files d'attente",
				tagline: "Définissez, éditez ou supprimez des informations sur la façon dont le serveur se connecte aux gestionnaires de files d'attente.",
				retrieveError: "Une erreur s'est produite lors de l'extraction des propriétés de connexion du gestionnaire de files d'attente.",
				grid: {
					name: "Nom",
					qmgr: "Gestionnaire de files d'attente",
					connName: "Nom de la connexion",
					channel: "Nom de canal",
					description: "Description",
					SSLCipherSpec: "SSL CipherSpec",
					status: "Statut"
				},
				dialog: {
					instruction: "La connectivité avec MQ requiert les détails de connexion du gestionnaire de files d'attente suivants.",
					nameInvalid: "Le nom ne peut pas commencer ou se terminer par un espace.",
					connTooltip: "Entrez une liste séparée par des virgules de noms de connexion sous la forme d'adresses IP ou de noms d'hôte et de numéros de port, par exemple   224.0.138.177(1414).",
					connInvalid: "Le nom de la connexion contient des caractères non valides.",
					qmInvalid: "Le nom du gestionnaire de files d'attente doit uniquement être composé de lettres ou de chiffres ainsi que de quatre caractères spéciaux (. _ % et /.",
					qmTooltip: "Entrez un nom de gestionnaire de files d'attente composé uniquement de lettres, de chiffres et des quatre caractères spéciaux . _ % et /.",
				    channelTooltip: "Entrez un nom de canal composé uniquement de lettres, de chiffres et des quatre caractères spéciaux . _ % et /.",
					channelInvalid: "Le nom du canal doit être composé uniquement de lettres, de chiffres et des quatre caractères spéciaux . _ % et /.",
					activeRuleTitle: "La modification n'est pas autorisée.",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "La connexion du gestionnaire de files d'attente ne peut pas être modifiée car elle est utilisée par la règle de mappage de destination {0} activée.",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "La connexion du gestionnaire de files d'attente ne peut pas être modifiée car elle est utilisée par les règles de mappage de destination activées suivantes : {0}.",
					SSLCipherSpecTooltip: "Entrez une spécification de chiffrement pour une connexion SSL avec une longueur maximale de 32 caractères."
				},
				addDialog: {
					title: "Ajouter une connexion de gestionnaire de files d'attente"
				},
				removeDialog: {
					title: "Supprimer une connexion de gestionnaire de files d'attente",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
				editDialog: {
					title: "Modifier une connexion de gestionnaire de files d'attente"
				},
                removeNotAllowedDialog: {
                	title: "Suppression non autorisée",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "La connexion du gestionnaire de files d'attente ne peut pas être supprimée car elle est utilisée par la règle de mappage de destination {0}.",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "La connexion du gestionnaire de files d'attente ne peut pas être supprimée car elle est utilisée par les règles de mappage de destination suivantes : {0}.",
                	closeButton: "Fermer"
                }
			},
			destinationMapping: {
				title: "Règles de mappage de destination",
				tagline: "Les administrateurs système et les administrateurs de messagerie peuvent définir, modifier ou supprimer les règles qui régissent les messages qui sont transférés de ou vers des gestionnaires de files d'attente.",
				note: "Les règles doivent être désactivées pour pouvoir être supprimées.",
				retrieveError: "Une erreur s'est produite lors de l'extraction des règles de mappage de destination.",
				disableRulesConfirmationDialog: {
					text: "Voulez-vous vraiment désactiver cette règle ?",
					info: "La désactivation de la règle arrête la règle, ce qui provoque la perte des messages placés en mémoire tampon ainsi que de tous les messages en cours d'envoi.",
					buttonLabel: "Désactiver la règle"
				},
				leadingBlankConfirmationDialog: {
					textSource: "La <em>source</em> commence par un blanc. Voulez-vous vraiment enregistrer cette règle avec un blanc ?",
					textDestination: "La <em>destination</em> commence par un blanc. Voulez-vous vraiment enregistrer cette règle avec un blanc ?",
					textBoth: "La <em>source</em> et la <em>destination</em> commencent par des blancs. Voulez-vous vraiment enregistrer cette règle avec des blancs ?",
					info: "Les rubriques sont autorisées à contenir des blancs mais n'en comportent généralement pas.  Vérifiez la chaîne de rubrique pour vous assurer qu'elle est correcte.",
					buttonLabel: "Enregistrer la règle"
				},
				grid: {
					name: "Nom",
					type: "Type de règle",
					source: "Source",
					destination: "Destination",
					none: "Aucun",
					all: "Tous",
					enabled: "Activé",
					associations: "Associations",
					maxMessages: "Nombre maximal de messages",
					retainedMessages: "Messages conservés"
				},
				state: {
					enabled: "Activé",
					disabled: "Désactivé",
					pending: "En attente"
				},
				ruleTypes: {
					type1: "Rubrique ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
					type2: "Rubrique ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
					type3: "File d'attente MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
					type4: "Rubrique MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
					type5: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
					type6: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
					type7: "Sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT} vers la sous-arborescence de rubrique MQ",
					type8: "Sous-arborescence de rubrique MQ vers la rubrique ${IMA_PRODUCTNAME_SHORT}",
					type9: "Sous-arborescence de rubrique MQ vers la sous-arborescence de rubrique ${IMA_PRODUCTNAME_SHORT}",
					type10: "File d'attente ${IMA_PRODUCTNAME_SHORT} vers la file d'attente MQ",
					type11: "File d'attente ${IMA_PRODUCTNAME_SHORT} vers la rubrique MQ",
					type12: "File d'attente MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}",
					type13: "Rubrique MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}",
					type14: "Sous-arborescence de rubrique MQ vers la file d'attente ${IMA_PRODUCTNAME_SHORT}"
				},
				sourceTooltips: {
					type1: "Entrez la rubrique ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage.",
					type2: "Entrez la rubrique ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage.",
					type3: "Entrez la file d'attente MQ à partir de laquelle procéder au mappage. La valeur peut comporter les caractères a à z, A à Z et 0 à 9, ainsi que les caractères suivants : % . /  _",
					type4: "Entrez la rubrique MQ à partir de laquelle procéder au mappage.",
					type5: "Entrez la sous-arborescence de rubriques ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage, par exemple MessageGatewayRoot/Level2",
					type6: "Entrez la sous-arborescence de rubriques ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage, par exemple MessageGatewayRoot/Level2",
					type7: "Entrez la sous-arborescence de rubriques ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage, par exemple MessageGatewayRoot/Level2",
					type8: "Entrez la sous-arborescence de rubriques MQ à partir de laquelle procéder au mappage, par exemple MQRoot/Layer1.",
					type9: "Entrez la sous-arborescence de rubriques MQ à partir de laquelle procéder au mappage, par exemple MQRoot/Layer1.",
					type10: "Entrez la file d'attente ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage. " +
							"La valeur ne doit pas contenir d'espace de début ou de fin, de caractère de contrôle, de virgule, de guillemets, de barre oblique inversée ou de signe égal. " +
							"Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "Entrez la file d'attente ${IMA_PRODUCTNAME_SHORT} à partir de laquelle procéder au mappage. " +
							"La valeur ne doit pas contenir d'espace de début ou de fin, de caractère de contrôle, de virgule, de guillemets, de barre oblique inversée ou de signe égal. " +
							"Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "Entrez la file d'attente MQ à partir de laquelle procéder au mappage. La valeur peut comporter les caractères a à z, A à Z et 0 à 9, ainsi que les caractères suivants : % . /  _",
					type13: "Entrez la rubrique MQ à partir de laquelle procéder au mappage.",
					type14: "Entrez la sous-arborescence de rubriques MQ à partir de laquelle procéder au mappage, par exemple MQRoot/Layer1."
				},
				targetTooltips: {
					type1: "Entrez la file d'attente MQ vers laquelle procéder au mappage. La valeur peut comporter les caractères a à z, A à Z et 0 à 9, ainsi que les caractères suivants : % . /  _",
					type2: "Entrez la rubrique MQ vers laquelle procéder au mappage.",
					type3: "Entrez la rubrique ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage.",
					type4: "Entrez la rubrique ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage.",
					type5: "Entrez la file d'attente MQ vers laquelle procéder au mappage. La valeur peut comporter les caractères a à z, A à Z et 0 à 9, ainsi que les caractères suivants : % . /  _",
					type6: "Entrez la rubrique MQ vers laquelle procéder au mappage.",
					type7: "Entrez la sous-arborescence de rubriques MQ vers laquelle procéder au mappage, par exemple MQRoot/Layer1.",
					type8: "Entrez la rubrique ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage.",
					type9: "Entrez la sous-arborescence de rubriques ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage, par exemple MessageGatewayRoot/Level2.",
					type10: "Entrez la file d'attente MQ vers laquelle procéder au mappage. La valeur peut comporter les caractères a à z, A à Z et 0 à 9, ainsi que les caractères suivants : % . /  _",
					type11: "Entrez la rubrique MQ vers laquelle procéder au mappage.",
					type12: "Entrez la file d'attente ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage. " +
							"La valeur ne doit pas contenir d'espace de début ou de fin, de caractère de contrôle, de virgule, de guillemets, de barre oblique inversée ou de signe égal. " +
							"Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "Entrez la file d'attente ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage. " +
							"La valeur ne doit pas contenir d'espace de début ou de fin, de caractère de contrôle, de virgule, de guillemets, de barre oblique inversée ou de signe égal. " +
							"Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "Entrez la file d'attente ${IMA_PRODUCTNAME_SHORT} vers laquelle procéder au mappage. " +
							"La valeur ne doit pas contenir d'espace de début ou de fin, de caractère de contrôle, de virgule, de guillemets, de barre oblique inversée ou de signe égal. " +
							"Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "Association",
				associatedQMs: "Connexions de gestionnaire de files d'attente associées :",
				associatedMessages: {
					errorMissing: "Une connexion de gestionnaire de files d'attente doit être sélectionnée.",
					errorRetained: "Le paramètre des messages conservés autorise une seule association de connexion de gestionnaire de files d'attente. " +
							"Changez les messages conservés pour <em>Aucun</em> ou supprimez certaines associations."
				},
				ruleTypeMessage: "Le paramètre des messages conservés autorise les types de règle avec rubrique ou les destinations de sous-arborescence de rubriques. " +
							"Changez les messages conservés pour <em>Aucun</em> ou sélectionnez un autre type de règle.",
				status: {
					active: "Actif",
					starting: "Démarrage en cours",
					inactive: "Inactif"
				},
				dialog: {
					instruction: "Les règles de mappage de destination définissent la direction dans laquelle les messages sont déplacés ainsi que la nature des objets source et cible.",
					nameInvalid: "Le nom ne peut pas commencer ou se terminer par un espace.",
					noQmgrsTitle: "Aucune connexion de gestionnaire de files d'attente",
					noQmrsDetail: "Impossible de définir une règle de mappage de destination sans définir au préalable une connexion de gestionnaire de files d'attente.",
					maxMessagesTooltip: "Indiquez le nombre maximal de messages pouvant être placés en mémoire tampon sur la destination.",
					maxMessagesInvalid: "Le nombre maximal de messages doit être un nombre compris entre 1 et 20 000000, inclus.",
					retainedMessages: {
						label: "Messages conservés",
						none: "Aucun",
						all: "Tous",
						tooltip: {
							basic: "Indiquez quels messages sont réacheminés vers une rubrique en tant que messages conservés.",
							disabled4Type: "Les messages conservés doivent être définis sur <em>Aucun</em> lorsque la destination est une file d'attente.",
							disabled4Associations: "Les messages conservés doivent être définis sur <em>Aucun</em> lorsque plusieurs connexions de gestionnaire de files d'attente sont sélectionnées.",
							disabled4Both: "Les messages conservés doivent être définis sur <em>Aucun</em> lorsque la destination est une file d'attente ou lorsque plusieurs connexions de gestionnaire de files d'attente sont sélectionnées."
						}
					}
				},
				addDialog: {
					title: "Ajouter une règle de mappage de destination"
				},
				removeDialog: {
					title: "Supprimer une règle de mappage de destination",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
                removeNotAllowedDialog: {
                	title: "Suppression non autorisée",
                	content: "La règle de mappage de destination ne peut pas être supprimée car elle est activée.  " +
                			 "Désactivez la règle à l'aide du menu 'Autres actions' puis renouvelez l'opération.",
                	closeButton: "Fermer"
                },
				editDialog: {
					title: "Modifier une règle de mappage de destination",
					restrictedEditingTitle: "La modification est limitée lorsque la règle est activée.",
					restrictedEditingContent: "Vous pouvez modifier des propriétés limitées lorsque la règle est activée. " +
							"Pour modifier des propriétés supplémentaires, désactivez la règle, modifiez les propriétés et enregistrez vos modifications. " +
							"La désactivation de la règle arrête la règle, ce qui provoque la perte des messages placés en mémoire tampon ainsi que de tous les messages en cours d'envoi. " +
							"La règle reste désactivée jusqu'à ce que vous la réactiviez."
				},
				action: {
					Enable: "Activer la règle",
					Disable: "Désactiver la règle",
					Refresh: "Actualiser le statut"
				}
			},
			sslkeyrepository: {
				title: "Référentiel de clés SSL",
				tagline: "Les administrateurs système et les administrateurs de messagerie peuvent télécharger un référentiel de clés SSL et un fichier de mots de passe. " +
						 "Le référentiel de clés SSL et le fichier de mots de passe sont utilisés pour sécuriser la connexion entre le serveur et le gestionnaire de files d'attente.",
                uploadButton: "Télécharger",
                downloadLink: "Télécharger",
                deleteButton: "Supprimer",
                lastUpdatedLabel: "Dernière mise à jour des fichiers de référentiel de clés SSL :",
                noFileUpdated: "Jamais",
                uploadFailed: "Le téléchargement a échoué.",
                deletingFailed: "La suppression a échoué.",                
                dialog: {
                	uploadTitle: "Télécharger des fichiers de référentiel de clés SSL",
                	deleteTitle: "Supprimer des fichiers de référentiel de clés SSL",
                	deleteContent: "Si le service MQConnectivity est démarré, il sera redémarré. Voulez-vous vraiment supprimer les fichiers de référentiel de clés SSL ?",
                	keyFileLabel: "Fichier de la base de données de clés :",
                	passwordFileLabel: "Fichier de mot de passe secret :",
                	browseButton: "Parcourir...",
					browseHint: "Sélection d'un fichier...",
					savingProgress: "Enregistrement en cours...",
					deletingProgress: "Suppression en cours...",
                	saveButton: "Enregistrer",
                	deleteButton: "OK",
                	cancelButton: "Annuler",
                	keyRepositoryError:  "Le fichier de référentiel de clés SSL doit être un fichier .kdb.",
                	passwordStashError:  "Le fichier de mot de passe secret doit être un fichier .sth.",
                	keyRepositoryMissingError: "Un fichier de référentiel de clés SSL est requis.",
                	passwordStashMissingError: "Un fichier de mot de passe secret est requis."
                }
			},
			externldap: {
				subTitle: "Configuration LDAP",
				tagline: "Si LDAP est activé, il est utilisé pour les utilisateurs et les groupes du serveur.",
				itemName: "Connexion LDAP",
				grid: {
					ldapname: "Nom LDAP",
					url: "URL",
					userid: "ID utilisateur",
					password: "Mot de passe"
				},
				enableButton: {
					enable: "Activer LDAP",
					disable: "Désactiver LDAP"
				},
				dialog: {
					ldapname: "Nom d'objet LDAP",
					url: "URL",
					certificate: "Certificat",
					checkcert: "Vérfier le certificat du serveur",
					checkcertTStoreOpt: "Utiliser le magasin de clés de confiance du serveur de messagerie",
					checkcertDisableOpt: "Désactiver la vérification du certificat",
					checkcertPublicTOpt: "Utiliser le magasin de clés de confiance public",
					timeout: "Délai d'attente",
					enableCache: "Activer la mémoire cache",
					cachetimeout: "Délai d'attente du cache",
					groupcachetimeout: "Délai d'attente du cache des groupes",
					ignorecase: "Ignorer la casse",
					basedn: "BaseDN",
					binddn: "BindDN",
					bindpw: "Liaison du mot de passe",
					userSuffix: "Suffixe utilisateur",
					groupSuffix: "Suffixe groupe",
					useridmap: "Mappe ID utilisateur",
					groupidmap: "Mappe ID groupe",
					groupmemberidmap: "Mappe ID membre de groupe",
					nestedGroupSearch: "Inclure des groupes imbriqués",
					tooltip: {
						url: "URL qui pointe vers le serveur LDAP. Le format est le suivant :<br/> &lt;protocole&gt;://&lt;serveur&nbsp;ip&gt;:&lt;port&gt;.",
						checkcert: "Indiquez comment vérifier le certificat du serveur LDAP.",
						basedn: "Nom distinctif de base",
						binddn: "Nom distinctif à utiliser lors de la liaison au protocole LDAP. Pour une liaison anonyme, laisser vide.",
						bindpw: "Mot de passe à utiliser lors de la liaison au protocole LDAP. Pour une liaison anonyme, laisser vide.",
						userSuffix: "Suffixe du nom distinctif utilisateur à rechercher. " +
									"Si aucune valeur n'est spécifiée, recherchez le nom distinctif utilisateur à l'aide de la mappe d'ID utilisateur, puis effectuez la liaison avec ce nom distinctif.",
						groupSuffix: "Suffixe du nom distinctif de groupe.",
						useridmap: "Propriété vers laquelle mapper un ID utilisateur.",
						groupidmap: "Propriété vers laquelle mapper un ID groupe.",
						groupmemberidmap: "Propriété vers laquelle mapper un ID de membre de groupe.",
						timeout: "Délai d'attente des appels LDAP, en secondes.",
						enableCache: "Indiquez si les données d'identification doivent être placées en mémoire cache.",
						cachetimeout: "Délai de mise en cache, en secondes.",
						groupcachetimeout: "délai de mise en mémoire cache du groupe, en secondes.",
						nestedGroupSearch: "Si cette case est cochée, incluez tous les groupes imbriqués à la recherche d'appartenance à un groupe d'un utilisateur.",
						testButtonHelp: "Teste la connexion au serveur LDAP. Vous devez spécifier l'URL du serveur LDAP et pouvez éventuellement indiquer des valeurs de certificat, BindDN et BindPassword."
					},
					secondUnit: "secondes",
					browseHint: "Sélection d'un fichier...",
					browse: "Parcourir...",
					clear: "Effacer",
					connSubHeading: "Paramètres de connexion LDAP",
					invalidTimeout: "Le délai d'attente doit être un nombre compris entre 1 et 60, inclus.",
					invalidCacheTimeout: "Le délai d'attente doit être un nombre compris entre 1 et 60, inclus.",
					invalidGroupCacheTimeout: "Le délai d'attente doit être un nombre compris entre 1 et 86 400, inclus.",
					certificateRequired: "Il faut un certificat lorsqu'une URL ldaps est indiquée et que le magasin de clés de confiance est utilisé",
					urlThemeError: "Entrez une URL valide avec un protocole ldap ou ldaps."
				},
				addDialog: {
					title: "Ajouter une connexion LDAP",
					instruction: "Configurez une connexion LDAP."
				},
				editDialog: {
					title: "Modifier la connexion LDAP"
				},
				removeDialog: {
					title: "Supprimer la connexion LDAP"
				},
				warnOnlyOneLDAPDialog: {
					title: "La connexion LDAP est déjà définie.",
					content: "Une seule connexion LDAP peut être spécifiée.",
					closeButton: "Fermer"
				},
				restartInfoDialog: {
					title: "Paramètres LDAP modifiés",
					content: "Les nouveaux paramètres LDAP seront utilisés la prochaine fois qu'un client ou une connexion est authentifié ou autorisé.",
					closeButton: "Fermer"
				},
				enableError: "Une erreur s'est produite lors de l'activation ou de la désactivation de la configuration LDAP externe.",				
				retrieveError: "Une erreur s'est produite lors de l'extraction de la configuration LDAP.",
				saveSuccess: "L'enregistrement a abouti. Le serveur ${IMA_PRODUCTNAME_SHORT} doit être redémarré pour que les modifications prennent effet."
			},
			messagequeues: {
				title: "Files d'attente de messages",
				subtitle: "Les files d'attente de messages sont utilisées dans la messagerie point-à-point."				
			},
			queues: {
				title: "Files d'attente",
				tagline: "Définissez, modifiez ou supprimez une file d'attente de messages.",
				retrieveError: "Une erreur s'est produite lors de l'extraction de la liste des files d'attente de messages.",
				grid: {
					name: "Nom",
					allowSend: "Autoriser les envois",
					maxMessages: "Nombre maximal de messages",
					concurrentConsumers: "Consommateurs simultanés",
					description: "Description"
				},
				dialog: {	
					instruction: "Définissez une file d'attente pour une utilisation avec votre application de messagerie.",
					nameInvalid: "Le nom ne peut pas commencer ou se terminer par un espace.",
					maxMessagesTooltip: "Indiquez le nombre maximal de messages pouvant être stockés sur la file d'attente.",
					maxMessagesInvalid: "Le nombre maximal de messages doit être un nombre compris entre 1 et 20 000000, inclus.",
					allowSendTooltip: "Spécifiez si les applications peuvent envoyer des messages à cette file d'attente. Cette propriété n'affecte pas la capacité des applications à recevoir des messages de cette file d'attente.",
					concurrentConsumersTooltip: "Indique si la file d'attente autorise plusieurs clients en même temps. Si la case est décochée, seul un utilisateur est autorisé sur la file d'attente.",
					performanceLabel: "Propriétés de performance"
				},
				addDialog: {
					title: "Ajouter une file d'attente"
				},
				removeDialog: {
					title: "Supprimer une file d'attente",
					content: "Voulez-vous vraiment supprimer cet enregistrement ?"
				},
				editDialog: {
					title: "Modifier une file d'attente"
				}	
			},
			messagingTester: {
				title: "Exemple d'application Messaging Tester",
				tagline: "Messaging Tester est un modèle d'application HTML5 simple qui utilise MQTT sur WebSockets pour simuler l'interaction de plusieurs clients avec le serveur ${IMA_PRODUCTNAME_SHORT}. " +
						 "Une fois les clients connectés au serveur, ils peuvent publier des données dans trois rubriques/s'abonner à trois rubriques.",
				enableSection: {
					title: "1. Activer le noeud final DemoMqttEndpoint",
					tagline: "L'exemple d'application Messaging Tester doit se connecter à un noeud final ${IMA_PRODUCTNAME_SHORT} non sécurisé.  Vous pouvez utiliser DemoMqttEndpoint."					
				},
				downloadSection: {
					title: "2. Télécharger et exécuter l'exemple d'application Messaging Tester",
					tagline: "Cliquez sur le lien pour télécharger MessagingTester.zip.  Extrayez les fichiers, puis ouvrez index.html dans un navigateur prenant en charge WebSockets. " +
							 "Suivez les instructions de la page Web pour vérifier que ${IMA_PRODUCTNAME_SHORT} est prêt pour la messagerie MQTT.",
					linkLabel: "Télécharger MessagingTester.zip"
				},
				disableSection: {
					title: "3. Désactiver le noeud final DemoMqttEndpoint",
					tagline: "Une fois la vérification de la messagerie MQTT ${IMA_PRODUCTNAME_SHORT} terminée, désactivez le noeud final DemoMqttEndpoint pour empêcher l'accès non autorisé à ${IMA_PRODUCTNAME_SHORT}."					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "Statut de {0}",
					state: {
						enabled: "Activé",					
						disabled: "Désactivé",
						missing: "Manquant",
						down: "Vers le bas"
					},
					linkLabel: {
						enableLinkLabel: "Activer",
						disableLinkLabel: "Désactiver"						
					},
					missingMessage: "Si un autre noeud final non sécurisé est indisponible, créez-en un.",
					retrieveEndpointError: "Une erreur s'est produite lors de l'extraction de la configuration du noeud final.",					
					retrieveEndpointStatusError: "Une erreur s'est produite lors de l'extraction du statut du noeud final.",
					saveEndpointError: "Une erreur s'est produite lors de la définition de l'état du noeud final."
				}
			}
		},
		protocolsLabel: "Protocoles",

		// Messaging policy types
		topicPoliciesTitle: "Règles de rubrique",
		subscriptionPoliciesTitle: "Règles d'abonnement",
		queuePoliciesTitle: "Règles de file d'attente",
		// Messaging policy dialog strings
		addTopicMPTitle: "Ajout de règle de rubrique",
	    editTopicMPTitle: "Edition de règle de rubrique",
	    viewTopicMPTitle: "Affichage de règle de rubrique",
		removeTopicMPTitle: "Suppression de règle de rubrique",
		removeTopicMPContent: "Voulez-vous vraiment supprimer cette règle de rubrique ?",
		topicMPInstruction: "Une règle de rubrique autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les rubriques auxquelles le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
					     "Chaque noeud final doit avoir au moins une règle de rubrique, une règle d'abonnement ou une règle de file d'attente.",
		addSubscriptionMPTitle: "Ajout de règle d'abonnement",
		editSubscriptionMPTitle: "Edition de règle d'abonnement",
		viewSubscriptionMPTitle: "Affichage de règle d'abonnement",
		removeSubscriptionMPTitle: "Suppression de règle d'abonnement",
		removeSubscriptionMPContent: "Voulez-vous vraiment supprimer cette règle d'abonnement ?",
		subscriptionMPInstruction: "Une règle d'abonnement autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les abonnements partagés globalement auxquels le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
					     "Dans un abonnement partagé globalement, le travail de réception des messages à partir d'un abonnement de rubrique durable est partagé entre plusieurs abonnés. Chaque noeud final doit avoir au moins une règle de rubrique, une règle d'abonnement ou une règle de file d'attente.",
		addQueueMPTitle: "Ajout de règle de file d'attente",
		editQueueMPTitle: "Edition de règle de file d'attente",
		viewQueueMPTitle: "Affichage de règle de file d'attente",
		removeQueueMPTitle: "Suppression de règle de file d'attente",
		removeQueueMPContent: "Voulez-vous vraiment supprimer cette règle de file d'attente ?",
		queueMPInstruction: "Une règle de file d'attente autorise des clients connectés à exécuter des actions de messagerie spécifiques telles que définir les files d'attente auxquelles le client peut accéder sur ${IMA_PRODUCTNAME_FULL}. " +
					     "Chaque noeud final doit avoir au moins une règle de rubrique, une règle d'abonnement ou une règle de file d'attente.",
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "Type de règle",
		policyTypeName_topic: "Rubrique",
		policyTypeName_subscription: "Abonnement partagé globalement",
		policyTypeName_queue: "File d'attente",
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "rubrique",
		policyTypeShortName_subscription: "abonnement",
		policyTypeShortName_queue: "file d'attente",
		policyTypeTooltip_topic: "Rubrique à laquelle cette règle s'applique. Utilisez l'astérisque (*) avec précaution. " +
		    "Un astérisque correspond à 0 caractère ou plus, y compris la barre oblique (/). Par conséquent, il peut correspondre à plusieurs niveaux dans une arborescence de rubriques.",
		policyTypeTooltip_subscription: "Abonnement durable partagé globalement auquel cette règle s'applique. Utilisez l'astérisque (*) avec précaution. " +
			"Un astérisque correspond à 0 caractère ou plus.",
	    policyTypeTooltip_queue: "File d'attente à laquelle cette règle s'applique. Utilisez l'astérisque (*) avec précaution. " +
			"Un astérisque correspond à 0 caractère ou plus.",
	    topicPoliciesTagline: "Une règle de rubrique permet de contrôler quelles sont les rubriques auxquelles un client peut accéder sur ${IMA_PRODUCTNAME_FULL}.",
		subscriptionPoliciesTagline: "Une règle d'abonnement permet de contrôler quels sont les abonnements durables partagés globalement auxquels un client peut accéder sur ${IMA_PRODUCTNAME_FULL}.",
	    queuePoliciesTagline: "Une règle de file d'attente permet de contrôler quelles sont les files d'attente auxquelles un client peut accéder sur ${IMA_PRODUCTNAME_FULL}.",
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "Utilisateur de canal",
	    channelPassword: "Mot de passe de l'utilisateur de canal",
	    channelUserTooltip: "Si votre gestionnaire de files d'attente est configuré pour l'authentification des utilisateurs de canal, vous devez définir cette valeur.",
	    channelPasswordTooltip: "Si vous spécifiez un utilisateur de canal, vous devez aussi définir cette valeur.",
	    // Additional LDAP dialog properties
	    emptyLdapURL: "L'adresse URL LDAP n'est pas définie",
		externalLDAPOnlyTagline: "Configurez les propriétés de connexion pour le référentiel LDAP qui est utilisé pour les utilisateurs et les groupes du serveur ${IMA_PRODUCTNAME_SHORT}.", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "Effacer",
		resetLdapButton: "Réinitialiser",
		resetLdapTitle: "Réinitialisation de la configuration LDAP",
		resetLdapContent: "Les valeurs par défaut de tous les paramètres de configuration LDAP seront restaurées.",
		resettingProgress: "Réinitialisation...",
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "Si le service MQConnectivity est démarré, il sera redémarré une fois les fichiers de référentiel téléchargés.",
		savingRestartingProgress: "Sauvegarde et redémarrage...",
		deletingRestartingProgress: "Suppression et redémarrage..."
});
