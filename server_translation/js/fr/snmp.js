/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
        all: "Tous",
        unavailableAddressesTitle: "Certaines adresses d'agent ne sont pas disponibles.",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "Les adresses d'agent SNMP doivent être des adresses d'interface Ethernet actives valides. " +
        		"Les adresses d'agent suivantes sont configurées, mais ne sont pas disponibles : {0}",
        agentAddressInvalidMessage: "Sélectionnez <em>Toutes</em> ou au moins une adresse IP.",
        notRunningError: "SNMP est activé, mais inactif.",
        snmpRestartFailureMessageTitle: "Le service SNMP n'a pas réussi à redémarrer.",
        statusLabel: "Statut",
        running: "Exécution",
        stopped: "Arrêté",
        unknown: "Inconnu",
        title: "Service SNMP",
		tagLine: "Paramètres et actions affectant le service SNMP.",
    	enableLabel: "Activer SNMP",
    	stopDialogTitle: "Arrêt du service SNMP",
    	stopDialogContent: "Voulez-vous vraiment arrêter le service SNMP ?  L'arrêt du service peut entraîner la perte de messages SNMP.",
    	stopDialogOkButton: "Arrêter",
    	stopDialogCancelButton: "Annuler",
    	stopping: "Arrêt",
    	stopError: "Une erreur est survenue lors de l'arrêt du service SNMP.",
    	starting: "Démarrage en cours",
    	started: "Exécution",
    	savingProgress: "Enregistrement en cours...",
    	setSnmpEnabledError: "Une erreur s'est produite lors de la définition de l'état activé de SNMP.",
    	startError: "Une erreur est survenue lors du démarrage du service SNMP.",
    	startLink: "Démarrer le service",
    	stopLink: "Arrêter le service",
		savingProgress: "Enregistrement en cours..."
});
