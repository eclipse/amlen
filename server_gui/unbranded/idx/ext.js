/*
 * Copyright (c) 2011-2021 Contributors to the Eclipse Foundation
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

/**
 * @name idx.ext
 * @class This module is a placeholder to automatically require all "extension" modules
 *        from IDX.  Extensions modules typically perform prototype modification of the
 *        Dojo modules to either work around known defects (often with respect to BiDi or
 *        a11y) or to add features or additional styling capabilities to the base dojo 
 *        widgets.
 */
define(["dojo/_base/lang",
        "idx/main",
        "./widgets",
        "./containers",
        "./trees",
        "./form/buttons",
        "dijit/_base/manager"],  // temporarily resolves parser issue with dijit.byId
        function(dLang,iMain) {
	return dLang.getObject("ext", true, iMain);
});
