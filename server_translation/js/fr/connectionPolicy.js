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
        resourcesSection: "Spécifiez les ressources qu'un client MQTT est autorisé à consommer :",
        allowDurable: "Autoriser les clients avec des abonnements durables",
        allowDurableHelp: "Si cette option est cochée, les clients MQTT peuvent se connecter avec CleanSession 0.",
        allowPersistentMessages: "Autoriser les messages persistants",
        allowPersistentMessagesHelp: "Si cette option est cochée, les clients MQTT peuvent publier des messages avec une qualité de service de 1 ou 2.",
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "Débit des messages attendu",
        expectedMessageRateHoverHelp: "Le débit des messages attendu détermine la quantité de mémoire " +
            "et la bande passante pouvant être utilisées par chaque connexion.  Si vous vous attendez à ce que le débit des messages soit élevé, vous " +
            "pouvez améliorer les performances en définissant le débit des messages attendu sur élevé ou maximum.  Si vous " +
            "vous attendez à ce que le débit des messages soit faible et souhaitez que le taux d'utilisation des ressources reste faible, vous pouvez définir le débit des messages attendu sur faible.",
        lowRate: "Faible",
        defaultRate: "Valeur par défaut",
        highRate: "Elevé",
        maxRate: "Maximum"
});
