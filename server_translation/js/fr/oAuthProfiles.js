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
        groupInfoKey: "Clé des informations de groupe",
        groupInfoKeyTooltip: "Nom de la clé utilisée pour extraire les informations de groupe depuis la réponse URL des informations utilisateur. " +
                "Si la clé est spécifiée, ${IMA_PRODUCTNAME_SHORT} n'extrait pas les informations de groupe depuis une autre source.",
        tokenSendMethodLabel: "Méthode d'envoi du jeton",
        tokenSendMethodTooltip: "Détermine comment le jeton d'accès est envoyé au serveur OAuth",
        tokenSendMethodURLParamOpt: "Paramètre d'URL",
        tokenSendMethodHTTPHeaderOpt: "En-tête HTTP",
        checkcertLabel: "Vérfier le certificat du serveur",
        checkcertTooltip: "Indiquez comment vérifier le certificat du serveur OAuth2.",
        checkcertTStoreOpt: "Utiliser le magasin de clés de confiance du serveur de messagerie",
        checkcertDisableOpt: "Désactiver la vérification du certificat",
        checkcertPublicTOpt: "Utiliser le magasin de clés de confiance public",
});
