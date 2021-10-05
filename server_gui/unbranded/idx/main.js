/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 
define(["require","dojo/_base/lang","dojo/_base/kernel","dojo/_base/window","dojo/ready","dojo/has","dojo/sniff","dojo/dom-class"], 
	   function(dRequire,dLang,dKernel,dWindow,dReady,dHas,dSniff,dDomClass) {
  /**
   * @name idx.main
   * @namespace This serves as the main module for the "idx" package when using "packages" in the "dojo config".
   *            This module's only action is to apply a CSS class to the body tag that identifies the Dojo version
   *            being utilized on the page. 
   */
	var iMain = dLang.getObject("idx", true);
	
	var majorVersion = dKernel.version.major;
	var minorVersion = dKernel.version.minor;
	
	var applyExtraClasses = function(){
		// apply dojo version class, "idx_dojo_1.x"
		var bodyNode = dWindow.body();
		var versionClass = "idx_dojo_" + dKernel.version.major + "_" + dKernel.version.minor;
		dDomClass.add(bodyNode, versionClass);	
		
		// apply bidi class for hebrew
		var locale = dKernel.locale.toLowerCase();
		if ((locale.indexOf("he") == 0) || (locale.indexOf("_il") >= 0)) {
			dDomClass.add(bodyNode, "idx_i18n_il");
		}
		
		// apply browser-specific classes
		if (dHas("ie")) {
			dDomClass.add(bodyNode, "dj_ie");
			dDomClass.add(bodyNode, "dj_ie" + dHas("ie"));
		} else if (dHas("trident")) {
			dDomClass.add(bodyNode, "dj_trident");
			dDomClass.add(bodyNode, "dj_trident" + Math.floor(dHas("trident")));
		} else if (dHas("ff")) {
			dDomClass.add(bodyNode, "dj_ff");
			dDomClass.add(bodyNode, "dj_ff" + dHas("ff"));
		} else if (dHas("safari")) {
			dDomClass.add(bodyNode, "dj_safari");
			dDomClass.add(bodyNode, "dj_safari" + dHas("safari"));		
		} else if (dHas("chrome")) {
			dDomClass.add(bodyNode, "dj_chrome");
			dDomClass.add(bodyNode, "dj_chrome" + dHas("chrome"));		
		} else if (dHas("webkit")) {
			dDomClass.add(bodyNode, "dj_webkit");		
		}
	}
	
	
	if ((majorVersion < 2) && (minorVersion < 7)) {
		// for dojo 1.6 we need to use "addOnLoad" to ensure the body exists first
		dojo.addOnLoad(applyExtraClasses);
	} else {
		dReady(applyExtraClasses);
	}
	
	return iMain;
});