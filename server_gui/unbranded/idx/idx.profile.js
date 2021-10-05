/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
var profile = (function(){
	
	var isTest = function(filename, mid){
			return /\/tests\//.test(filename) ||
				/\/demos\//.test(filename) ||
				/lessTests/.test(mid);
		},
		
		isCopyOnly = function(filename, mid){
			var list = {
				"idx/idx.profile":1,
				"idx/package.json":1,
				"idx/themes/dlBlue/dijit/compile": 1
			};
			
			var dlPathRegex = /idx\/themes\/dlBlue\//i,
				isInDlTheme = dlPathRegex.test( mid );
			
			var isDlBlueForIE = /dlBlue_ie\.css/i.test( filename ); 
			return (mid in list) || isDlBlueForIE ||
				isTest(filename, mid) ||
				/(png|jpg|jpeg|gif|tiff)$/.test(filename)||
				/idx\/themes\/oneuiLess/.test(mid);
		},

		ignore = function(filename, mid){
			return false;
		},
		isAMD = function(filename, mid){
			return !isTest(filename, mid) &&
				!isCopyOnly(filename, mid) &&
				/\.js$/.test(filename) &&
				!/oneuiLess/.test(mid);
		};

	return {
		resourceTags:{
			test: isTest,
			copyOnly: isCopyOnly,
			amd: isAMD,
			ignore: ignore
		}
	};
})();
