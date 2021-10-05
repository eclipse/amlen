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
        groupInfoKey: "Schlüssel für Gruppeninformationen",
        groupInfoKeyTooltip: "Der Name des Schlüssels, der zum Abrufen der Gruppeninformationen aus der Antwort zur URL für Benutzerinformationen verwendet wird. " +
                "Wenn der Schlüssel angegeben ist, ruft ${IMA_PRODUCTNAME_SHORT} keine Gruppeninformationen aus anderen Quellen ab.",
        tokenSendMethodLabel: "Tokensendemethode",
        tokenSendMethodTooltip: "Legen Sie fest, wie das Zugriffstoken an den OAuth-Server übertragen wird.",
        tokenSendMethodURLParamOpt: "URL-Parameter",
        tokenSendMethodHTTPHeaderOpt: "HTTP-Header",
        checkcertLabel: "Serverzertifikat überprüfen",
        checkcertTooltip: "Geben Sie an, wie das OAuth2-Serverzertifikat überprüft werden soll.",
        checkcertTStoreOpt: "Truststore des Messaging-Servers verwenden",
        checkcertDisableOpt: "Zertifikatsprüfung inaktivieren",
        checkcertPublicTOpt: "Öffentlichen Truststore verwenden",
});
