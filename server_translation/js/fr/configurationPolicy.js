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
	    title:  "Règles de configuration",
	    tagline: "Une règle de configuration contrôle les tâches d'administration qu'un utilisateur peut effectuer. " +
	    		"Les règles de configuration doivent être ajoutées au noeud final d'administration pour être appliquées.",
	    // Add / Edit dialog
	    addDialogTitle: "Ajout de règle de configuration",
	    editDialogTitle: "Edition de règle de configuration",
	    dialogInstruction: "Une règle de configuration contrôle les tâches d'administration qu'un utilisateur peut effectuer.",
		nameLabel:  "Nom :",
		nameColumnLabel:  "Nom",
		descriptionLabel: "Description :",
		descriptionColumnLabel: "Description",
		authorityLabel: "Droits d'accès :",
		authorityColumnLabel: "Droits d'accès",
		authorityTooltip: "<dl><dt>Configurer :</dt><dd>accorde le droit de modifier la configuration du serveur.</dd>" +
		                      "<dt>Afficher :</dt><dd>accorde le droit d'afficher le statut et la configuration du serveur.</dd>" +
                              "<dt>Surveiller :</dt><dd>accorde le droit d'afficher les données de surveillance.</dd>" +
                              "<dt>Gérer :</dt><dd>accorde le droit d'émettre des demandes de service, comme le redémarrage du serveur.</dd>" +
                          "</dl>",
		configure: "Configurer",
		view: "Afficher",
		monitor: "Surveiller",
		manage: "Gérer",
		listSeparator: ",",

	    dialogFilterInstruction: "Pour restreindre les actions d'administration et de surveillance définies dans cette règle à des utilisateurs ou des groupes d'utilisateurs spécifiques, spécifiez un ou plusieurs des filtres ci-après. " +
            "Par exemple, sélectionnez ID groupe pour restreindre cette règle aux membres d'un groupe particulier. " +
            "La règle autorise l'accès uniquement lorsque tous les filtres spécifiés sont satisfaits :",
        dialogFilterTooltip: "Vous devez spécifier au moins l'une des zones de filtre. " +
            "Vous pouvez utiliser un astérisque (*) unique comme dernier caractère dans la plupart des filtres pour mettre en correspondance 0 caractère ou plus. " +
            "L'adresse IP du client peut contenir un astérisque (*) ou une liste d'adresses IPv4 ou IPv6 ou de plages d'adresses séparées par une virgule, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::].",

	    clientIPLabel: "Adresse IP du client :",
	    clientIPColumnLabel: "Adresse IP du client",
	    userIdLabel: "ID utilisateur :",
	    userIdColumnLabel: "ID utilisateur",
	    groupIdLabel: "ID groupe :",
	    groupIdColumnLabel: "ID groupe",
	    commonNameLabel: "Nom usuel du certificat :",
	    commonNameColumnLabel: "Nom usuel du certificat",
	    invalidWildcard: "Vous pouvez utiliser un astérisque (*) unique comme dernier caractère pour mettre en correspondance 0 caractère ou plus.",
	    invalidClientIP: "L'adresse IP du client doit contenir un astérisque (*) ou une liste d'adresses IPv4 ou IPv6 ou de plages d'adresses séparées par une virgule, par exemple 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "Les adresses IPv6 doivent être placées entre crochets, par exemple [2001:db8::].",
        invalidFilter: "Vous devez spécifier au moins l'une des zones de filtre.",
	    // buttons
	    saveButton: "Enregistrer",
	    cancelButton: "Annuler",
	    closeButton: "Fermer",
        // messages
        savingProgress: "Enregistrement en cours...",
        updatingMessage: "Mise à jour en cours...",
        deletingProgress: "Suppression en cours...",
        noItemsGridMessage: "Aucun élément à afficher",
        getConfigPolicyError: "Une erreur est survenue lors de l'extraction des règles de configuration.",
        saveConfigPolicyError: "Une erreur est survenue lors de la sauvegarde de la règle de configuration.",
        deleteConfigurationPolicyError: "Une erreur est survenue lors de la suppression de la règle de configuration.",
        // remove dialog
        removeDialogTitle: "Suppression de la règle de configuration",
        removeDialogContent: "Voulez-vous vraiment supprimer cette règle de configuration ?"
});
