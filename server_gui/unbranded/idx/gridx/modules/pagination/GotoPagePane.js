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
define([
	"dojo/_base/declare",
	"gridx/support/GotoPagePane",
	"dojo/text!../../templates/GotoPagePane.html",
	"dojo/sniff"
], function(declare, GotoPagePane, goToTemplate, sniff){

	return declare(GotoPagePane, {
		templateString: goToTemplate,
		
		postCreate: function(){
			var t = this;
			if(sniff('ie') < 9){
				t.connect(t.domNode, 'onkeydown', '_onKeyDown');
			}

			setTimeout(function(){
				t.okBtn = t.dialog.buttons[0];
				t._updateStatus();
			}, 10);
		}
	});
});
