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
	    // Page title and tagline

	    title: "Appartenance à un cluster",
		tagline: "Détermine si le serveur appartient à un cluster. " +
				"Un cluster est une série de serveurs connectés les uns aux autres sur un réseau local à haute vitesse dont le but est d'augmenter le nombre maximal de connexions simultanées ou le débit maximal au-delà de la capacité d'une unité unique. " +
				"Les serveurs dans un cluster partagent une arborescence de rubriques commune.",

		// Status section
		statusTitle: "Statut",
		statusNotConfigured: "Le serveur n'est pas configuré pour participer à un cluster. " +
				"Pour que le serveur puisse participer à un cluster, éditez la configuration, puis ajoutez le serveur au cluster.",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "Ce serveur n'est pas membre d'un cluster. " +
				"Pour ajouter le serveur au cluster, cliquez sur <em>Redémarrer le serveur et le joindre au cluster {0}</em>.",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "Ce serveur est membre du cluster {0}. Pour retirer ce serveur du cluster, cliquez sur <em>Quitter le cluster</em>.",		
		clusterMembershipLabel: "Appartenance à un cluster :",
		notConfigured: "Non configurée",
		notSet: "Non défini",
		notAMember: "Pas un membre",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "Membre du cluster {0}",

        // Cluster states  
        clusterStateLabel: "Etat du cluster :", 
        initializing: "Initialisation en cours",       
        discover: "Reconnaissance",               
        active:  "Actif",                   
        removed: "Retiré",                 
        error: "Erreur",                     
        inactive: "Inactif",               
        unavailable: "Indisponible",         
        unknown: "Inconnu",                 
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "Redémarrer le serveur et le joindre au cluster <em>{0}</em>",
        leaveLink: "Quitter le cluster",
        // Configuration section

        configurationTitle: "Configuration",
		editLink: "Modifier",
	    resetLink: "Réinitialiser",
		configurationTagline: "La configuration du cluster est affichée. " +
				"Les modifications apportées à la configuration ne prennent effet qu'une fois le serveur ${IMA_PRODUCTNAME_SHORT} redémarré.",
		clusterNameLabel: "Nom du cluster :",
		controlAddressLabel: "Adresse de contrôle",
		useMulticastDiscoveryLabel: "Utiliser la reconnaissance multidiffusion :",
		withMembersLabel: "Liste des serveurs de reconnaissance :",
		multicastTTLLabel: "Durée de vie de la reconnaissance multidiffusion :",
		controlInterfaceSectionTitle: "Interface de contrôle",
        messagingInterfaceSectionTitle: "Interface de messagerie",
		addressLabel: "Adresse :",
		portLabel: "Port :",
		externalAddressLabel: "Adresse externe :",
		externalPortLabel: "Port externe :",
		useTLSLabel: "Utiliser TLS :",
        yes: "Oui",
        no: "Non",
        discoveryPortLabel: "Port de reconnaissance :",
        discoveryTimeLabel: "Délai de reconnaissance :",
        seconds: "{0} secondes", 

		editDialogTitle: "Edition de la configuration du cluster",
        editDialogInstruction_NotAMember: "Définissez un nom de cluster afin de déterminer le cluster que ce serveur doit rejoindre. " +
                "Une fois que vous avez sauvegardé une configuration valide, vous pouvez rejoindre le cluster.",
        editDialogInstruction_Member: "Modifiez la configuration du cluster. " +
        		"Pour changer le nom du cluster auquel ce serveur appartient, vous devez d'abord retirer le serveur du cluster {0}.",
        saveButton: "Enregistrer",
        cancelButton: "Annuler",
        advancedSettingsLabel: "Paramètres avancés",
        advancedSettingsTagline: "Vous pouvez configurer les paramètres ci-après. " +
                "Consultez la documentation d'${IMA_PRODUCTNAME_FULL} pour des instructions détaillées et des recommandations avant de modifier ces paramètres.",
		// Tooltips

        clusterNameTooltip: "Nom du cluster à rejoindre.",
        membersTooltip: "Liste d'URI de serveur séparés par une virgule pour les membres appartenant au cluster que ce serveur souhaite rejoindre. " +
        		"Si la case <em>Utiliser la reconnaissance multidiffusion</em> n'est pas cochée, spécifiez au moins un URI de serveur. " +
        		"L'URI de serveur pour un membre est affiché dans la page Appartenance à un cluster de ce membre.",
        useMulticastTooltip: "Indiquez si la multidiffusion doit être utilisée pour identifier les serveurs dont le nom de cluster est identique.",
        multicastTTLTooltip: "Si vous utilisez la reconnaissance multidiffusion, vous spécifiez ici le nombre de segments réseau que le trafic de multidiffusion est autorisé à traverser.", 
        controlAddressTooltip: "Adresse IP de l'interface de contrôle.",
        controlPortTooltip: "Numéro de port à utiliser pour l'interface de contrôle.  Le port par défaut est 9104. ",
        controlExternalAddressTooltip: "Adresse IP externe ou nom d'hôte de l'interface de contrôle si différent de l'adresse de contrôle. " +
        		"L'adresse externe est utilisée par les autres membres pour la connexion à l'interface de contrôle de ce serveur.",
		controlExternalPortTooltip: "Port externe à utiliser pour l'interface de contrôle s'il est différent du port de contrôle. "  +
                "Le port externe est utilisé par les autres membres pour la connexion à l'interface de contrôle de ce serveur.",
        messagingAddressTooltip: "Adresse IP de l'interface de messagerie.",
        messagingPortTooltip: "Numéro de port à utiliser pour l'interface de messagerie.  Le port par défaut est 9105. ",
        messagingExternalAddressTooltip: "Adresse IP externe ou nom d'hôte de l'interface de messagerie si différent de l'adresse de messagerie. " +
                "L'adresse externe est utilisée par les autres membres pour la connexion à l'interface de messagerie de ce serveur.",
        messagingExternalPortTooltip: "Port externe à utiliser pour l'interface de messagerie s'il est différent du port de messagerie. "  +
                "Le port externe est utilisé par les autres membres pour la connexion à l'interface de messagerie de ce serveur.",
        discoveryPortTooltip: "Numéro de port à utiliser pour la reconnaissance multidiffusion. Le port par défaut est 9106. " +
        		"Le même port doit être utilisé sur tous les membres du cluster.",
        discoveryTimeTooltip: "Durée, en secondes, passée au cours du démarrage du serveur à identifier d'autres serveurs dans le cluster et à obtenir des informations à jour les concernant. " +
                "La plage valide est comprise entre 1 et 2147483647. La valeur par défaut est de 10. " + 
                "Ce serveur devient un membre actif du cluster une fois les informations à jour obtenues ou le délai d'attente de reconnaissance dépassé, selon l'événement qui survient en premier.",
        controlInterfaceSectionTooltip: "L'interface de contrôle est utilisée pour la configuration, la surveillance et le contrôle du cluster.",
        messagingInterfaceSectionTooltip: "L'interface de messagerie permet le transfert aux autres membres du cluster des messages qui ont été publiés depuis un client vers un membre du cluster.",
        // confirmation dialogs
        restartTitle: "Redémarrer ${IMA_PRODUCTNAME_SHORT}",
        restartContent: "Voulez-vous vraiment redémarrer ${IMA_PRODUCTNAME_SHORT} ?",
        restartButton: "Redémarrer",

        resetTitle: "Réinitialisation de l'appartenance à un cluster",
        resetContent: "Voulez-vous vraiment réinitialiser la configuration de l'appartenance à un cluster ?",
        resetButton: "Réinitialiser",

        leaveTitle: "Quitter le cluster",
        leaveContent: "Voulez-vous vraiment quitter le cluster ?",
        leaveButton: "Quitter",      
        restartRequiredTitle: "Redémarrage requis",
        restartRequiredContent: "Vos modifications ont été enregistrées mais ne prendront effet qu'une fois le serveur ${IMA_PRODUCTNAME_SHORT} redémarré.<br /><br />" +
                 "Pour redémarrer le serveur ${IMA_PRODUCTNAME_SHORT} maintenant, cliquez sur <b>Redémarrer maintenant</b>. Sinon, cliquez sur <b>Redémarrer ultérieurement</b>.",
        restartLaterButton: "Redémarrer ultérieurement",
        restartNowButton: "Redémarrer maintenant",

        // messages
        savingProgress: "Enregistrement en cours...",
        errorGettingClusterMembership: "Une erreur est survenue lors de l'extraction des informations d'appartenance à un cluster.",
        saveClusterMembershipError: "Une erreur est survenue lors de la sauvegarde de la configuration de l'appartenance à un cluster.",
        restarting: "Redémarrage du serveur ${IMA_PRODUCTNAME_SHORT} en cours...",
        restartingFailed: "Echec du redémarrage du serveur ${IMA_PRODUCTNAME_SHORT}.",
        resetting: "Réinitialisation de la configuration...",
        resettingFailed: "Echec de la réinitialisation de la configuration de l'appartenance à un cluster.",
        addressInvalid: "Une adresse IP valide est requise.", 
        portInvalid: "Le numéro de port doit être un nombre compris entre 1 et 65535 inclus."

});
