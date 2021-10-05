/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

define([ "dojo/_base/declare", "idx/widget/ConfirmationDialog" ],
		function(declare, ConfirmationDialog) {
			/**
			 * @name ism.widgets.Dialog
			 * @class ISM extension of One UI version ConfirmationDialog which replaces
			 *        cookie store with preferences store
			 * @augments idx.widget.ConfirmationDialog
			 * 
			 * Example (from OneUI site):  
			 * 
			 * var confirmationDialog = manager.byId("confirmationDialog");
			 * if(!confirmationDialog){
			 * 		confirmationDialog = new ConfirmationDialog({
			 * 			id: "confirmationDialog",  
			 * 			showConfirmationId: "The id is used to store the user preference about whether to show this again or not!"
			 * 			text: "Are you sure that you want to change the background color?",
			 * 			info: "Any additional info can optionally go here...",
			 *			buttonLabel: "Change",
			 *			type: "confirmation",
			 *			dupCheck: false  // if true, shows the Do not ask again checkbox.  We don't have a way for the user to undo yet, so leave false
			 *		}, "confirmation");
			 * }
			 * confirmationDialog.confirm(function(){
			 * 		dojo.toggleClass(dojo.body(), "blackBackground");
			 * }); 
			 */
			return declare("ism.widgets.ConfirmationDialog", [ ConfirmationDialog ], {
				// summary:
				// An extension of the One UI ConfirmationDialog to replace the use of cookies
				// for storing user choice to using preferences.
				
				_getConfirmationId: function() {
					return this.showConfirmationId ? this.showConfirmationId : this.id + "_confirmation";
				},
				
				_confirmed: function(){
					if (!this.dupCheck) {
						return false;
					}
					return !ism.user.getShowConfirmation(this._getConfirmationId());
				},
				
				/**
				* Check the confirm checkbox
				*/
				check: function(){
					ism.user.setShowConfirmation(this._getConfirmationId(), false);
				},
				/**
				* Un-check the confirm checkbox
				*/
				uncheck: function(){
					ism.user.setShowConfirmation(this._getConfirmationId(), true);
					this.checkbox.set("value", false);
				}			
			});
		});
