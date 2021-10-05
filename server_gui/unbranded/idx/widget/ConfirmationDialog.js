/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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
	"dojo/_base/lang", // lang.mixin lang.hitch
	"dojo/_base/event", // event.stop
	"dojo/dom-style", // domStyle.set
	"dojo/cookie", // domStyle.set
	"dijit/form/Button",
	"idx/widget/ModalDialog",
	"idx/form/CheckBox",
	"dojo/text!./templates/ConfirmationDialog.html"
], function(declare, lang, event, domStyle, cookie, Button, ModelDialog, CheckBox, template){
	var iMessaging = lang.getObject("idx.oneui.messaging", true); // for backward compatibility with IDX 1.2
	/**
	 * @name idx.widget.ConfirmationDialog
	 * @class The ModalDialog provides the standard OneUI Confirmation Dialog.
	 * Pops up a modal dialog window, blocking access and graying out to the screen
	 * it support "OK/Cancel" option for user to make their decision
	 * @augments dijit.messaging.ConfirmationDialog
	 */
	return iMessaging.ConfirmationDialog = declare("idx.widget.ConfirmationDialog", ModelDialog, {
		/**@lends idx.widget.ConfirmationDialog*/
		
		baseClass: "idxConfirmDialog",
		templateString: template,
		/**
		 * Execute button label
		 * @type String
		 */
		buttonLabel:"",
		/**
		 * Cancel button label
		 * @type String
		 */
		cancelButtonLabel: "",
		
		postCreate: function(){
			this.inherited(arguments);
			domStyle.set(this.confirmAction, "display", "block");
			this.confirmAction = new Button({
				label: this.buttonLabel || this._nlsResources.executeButtonLabel || "OK", 
				onClick: lang.hitch(this, function(evt){
					this.onExecute();
					event.stop(evt);
				})
			}, this.confirmAction);
			this.closeAction.set("label", this.cancelButtonLabel || this._nlsResources.cancelButtonLabel || "Cancel");
			this.closeAction.focusNode && domStyle.set(this.closeAction.focusNode, "fontWeight", "normal");
			
			this.set("type", this.type || "Confirmation");
			
			if(this.checkboxNode && this.dupCheck){
                this.checkbox = new CheckBox({
                    label: this._nlsResources.checked || "Do not ask again",
                    onChange: lang.hitch(this, function(evt){
                        if(this.checkbox.get("value") == "on"){
                            this.check();
                        }else{
                            this.uncheck();
                        }
                    })
                }, this.checkbox);
			    domStyle.set(this.checkboxNode, "display", "");
			}
		},
		_confirmed: function(){
			return cookie(this.id + "_confirmed") == "true";
		},
		/**
		* Check the confirm checkbox
		*/
		check: function(){
			cookie(this.id + "_confirmed", "true");
		},
		/**
		* Un-check the confirm checkbox
		*/
		uncheck: function(){
			cookie(this.id + "_confirmed", null);
			this.checkbox.set("value", false);
		},
		/**
		* Un-check the confirm checkbox
		*/
		confirm: function(action, context){
			if(!this._confirmed()){
				this.show();
				if(this.checkbox && this.checkbox.set){
					this.checkbox.set("value", false);
				}
				this._actionListener && this.disconnect(this._actionListener);
				this._actionListener = this.connect(this, "onExecute", lang.hitch(context, action));
			}else{
				lang.hitch(context, action)();
			}
		},
		
		/**
		 * Sets a label string for OK button.
		 * @param {String} s
		 * @private
		 */
		_setLabelOkAttr: function(label){
			this.confirmAction.set("label", label || this._nlsResources.cancelButtonLabel || "OK");
		}
		
	});
})
