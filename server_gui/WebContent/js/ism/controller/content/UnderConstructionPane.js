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
require([
    "dojo/_base/declare",
    'ism/widgets/_TemplatedContent',
	'dijit/layout/ContentPane',    
    "dojo/text!ism/controller/content/templates/UnderConstructionPane.html"
], function(declare,  _TemplatedContent, ContentPane, template) {
 
    declare("ism.controller.content.UnderConstructionPane", _TemplatedContent, {
        
    	templateString: template,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		setItem: function(item) {
			console.debug("setItem called with " + item);
		}
    

    });
 
});
