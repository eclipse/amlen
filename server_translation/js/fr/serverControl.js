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
    	serverMenuLabel: "Serveur :",
    	serverMenuHint: "Sélectionnez un serveur à gérer",
    	recentServersLabel: "Serveurs récents :",
    	addRemoveLinkLabel: "Ajouter ou retirer des serveurs",
    	addRemoveDialogTitle: "Ajout ou retrait de serveurs",
    	addRemoveDialogInstruction: "Ajoutez ou retirez des serveurs ${IMA_PRODUCTNAME_SHORT} que vous pouvez gérer avec cette interface utilisateur Web.",
    	serverName: "Nom du serveur",
    	serverNameTooltip: "Nom d'affichage de ce serveur.",
        adminAddress: "Adresse d'administration :",
        adminAddressTooltip: "Adresse IP ou nom d'hôte sur laquelle/lequel le noeud final d'administration du serveur est à l'écoute.",
        adminPort: "Port d'administration :",
        adminPortTooltip: "Port sur lequel le noeud final d'administration de ce serveur est à l'écoute. Le numéro de port doit être compris entre 1 et 65535 inclus.",
        sendWebUICredentials: "Envoyer les données d'identification de l'interface utilisateur Web :",
        sendWebUICredentialsTooltip: "Indique si l'interface utilisateur Web doit envoyer l'ID utilisateur et le mot de passe avec lesquels vous vous êtes connecté au noeud final d'administration. " +
        		"Si le noeud final d'administration requiert un ID utilisateur et un mot de passe valides, l'interface utilisateur Web envoie l'ID utilisateur et le mot de passe avec lesquels vous vous êtes connecté au noeud final d'administration. " +
        		"Sinon, vous devrez entrer l'ID utilisateur et le mot de passe requis pour accéder au noeud final d'administration.",
        //cluster: "Cluster",
        description: "Description :",
        portInvalidMessage: "Le numéro de port doit être compris entre 1 et 65535 inclus.",
        duplicateServerTitle: "Serveur en double",
        duplicateServerMessage: "Le serveur ne peut pas être ajouté car un serveur associé à l'adresse et au port spécifiés existe déjà dans la liste.",
        closeButton: "Fermer",
        saveButton: "Enregistrer",
        cancelButton: "Annuler",
        testButton: "Tester la connexion",
        YesButton: "Oui",
        deletingProgress: "Suppression en cours...",
        removeConfirmationTitle: "Voulez-vous vraiment retirer ce serveur de la liste des serveurs gérés ?",
        addServerDialogTitle: "Ajout de serveur",
        editDialogTitle: "Edition de serveur",
        removeServerDialogTitle: "Retrait de serveur",
        addServerDialogInstruction: "Ajoutez un serveur ${IMA_PRODUCTNAME_SHORT} que vous pouvez gérer avec cette interface utilisateur Web.  " /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "Test en cours...",
        testingFailed: "Le test de connexion a échoué.",
        testingSuccess: "Le test de connexion a réussi."        
});
