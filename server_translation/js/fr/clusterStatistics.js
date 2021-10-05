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
        // TRNOTE: {0} is replaced with the server name of the server currently managed in the Web UI.
        //         {1} is replaced with the name of the cluster where the current server is a member.
		detail_title: "Statut du cluster pour {0} dans le cluster {1}",
		title: "Statut du cluster",
		tagline: "Surveillez le statut des membres du cluster. Affichez des statistiques sur les canaux de transmission des messages sur ce serveur. Les canaux de transmission des messages fournissent " +
				"une connectivité pour les messages et placent ces derniers en mémoire tampon pour les membres du cluster distant.  Si les messages placés en mémoire tampon pour des membres inactifs consomment trop " +
				"de ressources, vous pouvez retirer les membres inactifs du cluster.",
		chartTitle: "Graphique du débit",
		gridTitle: "Données de statut du cluster",
		none: "Aucun",
		lastUpdated: "Dernière mise à jour",
		notAvailable: "n/a",
		cacheInterval: "Intervalle de collecte de données : 5 secondes",
		refreshingLabel: "Mise à jour en cours...",
		resumeUpdatesButton: "Reprendre les mises à jour",
		updatesPaused: "La collection de données est interrompue",
		serverNameLabel: "Nom du serveur",
		connStateLabel: "Etat de la connexion :",
		healthLabel: "Santé du serveur",
		haStatusLabel: "Statut de la haute disponibilité :",
		updTimeLabel: "Dernière mise à jour de l'état",
		incomingMsgsLabel: "Messages entrants/seconde :",
		outgoingMsgsLabel: "Messages sortants/seconde :",
		bufferedMsgsLabel: "Messages en mémoire tampon :",
		discardedMsgsLabel: "Messages supprimés :",
		serverNameTooltip: "Nom du serveur",
		connStateTooltip: "Etat de la connexion",
		healthTooltip: "Santé du serveur",
		haStatusTooltip: "Statut de la haute disponibilité",
		updTimeTooltip: "Dernière mise à jour de l'état",
		incomingMsgsTooltip: "Messages entrants/seconde",
		outgoingMsgsTooltip: "Messages sortants/seconde",
		bufferedMsgsTooltip: "Messages placés en mémoire tampon",
		discardedMsgsTooltip: "Messages supprimés",
		statusUp: "Actif",
		statusDown: "Inactif",
		statusConnecting: "Connexion en cours",
		healthUnknown: "Inconnu",
		healthGood: "Bon",
		healthWarning: "Avertissement",
		healthBad: "Erreur",
		haDisabled: "Désactivé",
		haSynchronized: "Synchronisé",
		haUnsynchronized: "Non synchronisé",
		haError: "Erreur",
		haUnknown: "Inconnu",
		BufferedAssocType: "Détails des messages placés en mémoire tampon",
		DiscardedAssocType: "Détails des messages supprimés",
		channelTitle: "Canal",
		channelTooltip: "Canal",
		channelUnreliable: "Non fiable",
		channelReliable: "Fiable",
		bufferedBytesLabel: "Octets en mémoire tampon",
		bufferedBytesTooltip: "Octets en mémoire tampon",
		bufferedHWMLabel: "Pic des messages placés en mémoire tampon",
		bufferedHWMTooltip: "Pic des messages placés en mémoire tampon",
		addServerDialogInstruction: "Ajoutez le membre de cluster à la liste des serveurs gérés et gérez le serveur du membre de cluster.",
		saveServerButton: "Sauvegarder et gérer",
		removeServerDialogTitle: "Retrait du serveur du cluster",
		removeConfirmationTitle: "Voulez-vous vraiment retirer ce serveur du cluster ?",
		removeServerError: "Une erreur est survenue lors de la suppression du serveur depuis le cluster.",

		removeNotAllowedTitle: "Suppression non autorisée",
		removeNotAllowed: "Le serveur ne peut pas être retiré du cluster car il est actif. " +
		                  "Utilisez l'action de retrait pour retirer uniquement les serveurs qui ne sont pas actifs.",

		cancelButton: "Annuler",
		yesButton: "Oui",
		closeButton: "Fermer"
});
