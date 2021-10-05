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
	    configurationTitle: "Noeud final d'administration",
	    editLink: "Modifier",
	    configurationTagline_unsecured: "Le noeud final d'administration n'est pas sécurisé. " +
	            "Le noeud final d'administration permet d'administrer le serveur avec l'interface utilisateur Web ou des API REST. " +
	    		"Pour sécuriser le noeud final, configurez un profil de sécurité et une ou plusieurs règles de configuration.",
	    configurationTagline: "Le noeud final d'administration permet d'administrer le serveur avec l'interface utilisateur Web ou des API REST. " +
	    		"Vérifiez que le profil de sécurité approprié et les règles de configuration sont disponibles.",
	    editDialogTitle: "Edition du noeud final d'administration",
	    editDialogInstruction: "L'interface utilisateur Web et l'API REST se connectent au noeud final d'administration pour effectuer des tâches d'administration et de surveillance.",
	    // property labels and hover helps
	    interfaceLabel:  "Adresse IP :",
	    interfaceTooltip: "Adresse IP sur laquelle le noeud final d'administration est à l'écoute.",
	    portLabel: "Port :",
	    portTooltip: "Port sur lequel le noeud final d'administration est à l'écoute. Le numéro de port doit être compris entre 1 et 65535 inclus.",
	    securityProfileLabel: "Profil de sécurité :",
	    securityProfileTooltip: "Pour sécuriser des connexions et définir le mode d'authentification, sélectionnez un profil de sécurité. " +
	    		"Les profils de sécurité existants sont inclus dans la liste. " +
	    		"Pour créer un profil de sécurité, accédez à la page <em>Paramètres de sécurité</em>.",
	    configurationPoliciesLabel: "Règles de configuration :",
	    configurationPoliciesTooltip: "Pour sécuriser le noeud final d'administration, ajoutez au moins une règle de configuration, mais pas plus de 100. " +
	    		"Une règle de configuration autorise les clients connectés à effectuer des tâches d'administration et de surveillance spécifiques. " +
	    		"Les règles de configuration sont évaluées dans l'ordre affiché. Pour modifier l'ordre, utilisez les flèches vers le haut et vers le bas de la barre d'outils.",
	    // configuration policy chooser dialog
	    addConfigPolicyDialogTitle: "Ajout de règles de configuration au noeud final d'administration",
	    addConfigPoliciesDialogInstruction: "Une règle de configuration autorise les clients connectés à effectuer des tâches d'administration et de surveillance spécifiques.",
	    addLabel: "Ajouter",
	    configurationPolicyLabel: "Règle de configuration",
	    descriptionLabel: "Description",	
	    authorityLabel: "Droits d'accès",
	    none: "Aucun",
	    all: "Tous",
	    // buttons
	    saveButton: "Enregistrer",
	    cancelButton: "Annuler",
	    add: "Ajouter",
        // messages
        savingProgress: "Enregistrement en cours...",
        getAdminEndpointError: "Une erreur est survenue lors de l'extraction de la configuration du noeud final d'administration.",
        saveAdminEndpointError: "Une erreur est survenue lors de la sauvegarde de la configuration du noeud final d'administration.",
        getSecurityProfilesError: "Une erreur est survenue lors de l'extraction des profils de sécurité.",
        tooManyPolicies: "Un nombre trop élevé de règles est sélectionné. Le noeud final d'administration ne peut pas comporter plus de 100 règles de configuration.",
        certificateInvalidMessage: "L'interface utilisateur Web ne peut pas se connecter au serveur ${IMA_PRODUCTNAME_SHORT}. " +
            "Il se peut que le serveur soit arrêté ou que le certificat qui sécurise le noeud final d'administration ne soit pas valide. " +
            "Si le certificat n'est pas valide, vous devez ajouter une exception. " +
            "<a href='{0}' target='_blank'>Cliquez ici</a> pour accéder à une page dans laquelle vous pouvez ajouter une exception si nécessaire. ",
        certificateRefreshInstruction: "Après l'ajout d'une exception, il peut être nécessaire d'actualiser cette page. ",
        certificatePreventError: "Consultez la documentation de ${IMA_PRODUCTNAME_SHORT} dans l'IBM Knowledge Center pour savoir comment éviter ce problème.",
        // AdminUserID and AdminUserPassword
        superUserTitle: "Superutilisateur administrateur",
        superUserTagline: "L'ID et le mot de passe du superutilisateur administrateur sont stockés localement pour garantir que le serveur ${IMA_PRODUCTNAME_SHORT} peut être administré même si LDAP n'est pas configuré correctement ou n'est pas disponible. " +
        		"Le superutilisateur administrateur dispose des droits permettant d'effectuer toutes les actions. " +
        		"Veillez à ce que l'ID et le mot de passe du superutilisateur administrateur soient difficiles à deviner.",

        adminUserIDLabel: "ID utilisateur :",
        changeAdminUserIDLink: "Modifier l'ID utilisateur",
        editAdminUserIdDialogTitle: "Modification de l'ID du superutilisateur administrateur",
        editAdminUserIdDialogInstruction: "Remplacez l'ID du superutilisateur administrateur par une valeur difficile à deviner.",
        saveAdminUserIDError: "Une erreur est survenue lors de la sauvegarde de l'ID du superutilisateur administrateur.",
        getAdminUserIDError: "Une erreur est survenue lors de l'extraction de l'ID du superutilisateur administrateur.",

        changeAdminUserPasswordLink: "Modifier le mot de passe",
        editAdminUserPasswordDialogTitle: "Modification du mot de passe du superutilisateur administrateur",
        editAdminUserPasswordDialogInstruction: "Remplacez le mot de passe du superutilisateur administrateur par une valeur difficile à deviner.",
        adminUserPasswordLabel: "Mot de passe",
        confirmPasswordLabel: "Confirmation du mot de passe",
        passwordInvalidMessage : "Les mots de passe ne correspondent pas.",
        saveAdminUserPasswordError: "Une erreur est survenue lors de la sauvegarde du mot de passe du superutilisateur administrateur."            
});
