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
	"dojo/dom-style", 
	"dojo/i18n", 
	"dijit/_base/wai", 
	"idx/widget/HoverHelpTooltip",
	"dojo/i18n!dijit/form/nls/validate"
], function(declare, domStyle, i18n, wai, HoverHelpTooltip){
	/**
	 * @public
	 * @name idx.form._ValidationMixin
	 * @class Mix-in class to enable form widget perform validation, implemented according to 
	 * IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y14&vsub=*&hsub=*&openpanes=0000011100">Validation</a></b>.
	 */
	return declare("idx.form._ValidationMixin", null, 
	/**@lends idx.form._ValidationMixin#*/
	{
		/**
		 * Configurable flag of the validation timing, the widget fires validation when widget get input by setting true, 
		 * fire validation when widget get blur by setting false.
		 * @type boolean
		 * @default false
		 */
		instantValidate: false,
		
		// required: Boolean
		//		Indicate whether this widget must have a value
		/**
		 * Indicate whether this widget must have a value
		 * @type boolean
		 * @default false
		 */
		required: false,
		
		/**
		 * The message to display if value is invalid
		 * @type String
		 */
		invalidMessage: "$_unset_$",
		
		/**
		 * The message to display if value is empty and the field is required
		 * @type String
		 */
		missingMessage: null,
		
		/**
		 * The position of the hoverhelpTooltip
		 * @type String[]
		 */
		tooltipPosition: [], // need to override this class-static value in postMixInProperties
		
		postMixInProperties: function(){
			this.tooltipPosition = ["after-centered", "above"]; // set this to a instance-local value
			this.inherited(arguments);
			var messages = i18n.getLocalization("dijit.form", "validate", this.lang);
			this.missingMessage || (this.missingMessage = messages.missingMessage);
			// check the nls message === null
			this.invalidMessage = (this.invalidMessage == "$_unset_$") ? ( (messages && messages.invalidMessage)? messages.invalidMessage :this.invalidMessage ) : this.invalidMessage;
		},
		/*buildRendering: function(){
			this.inherited(arguments);
			this.messageTooltip = new HoverHelpTooltip({
				connectId: [this.iconNode],
				label: this.message,
				position: this.tooltipPosition,
				forceFocus: false
			});
		},*/
		postCreate: function(){
			this.inherited(arguments);
			if(this.instantValidate){
				this.connect(this, this._event["focus"], function(){
					//if (this._hasBeenBlurred && (!this._refocusing)) {
					/* Start validation once instantValidate widget got focus. Fix for #9977 */
					if (!this._refocusing) {
						this.validate(true);
					}
				});
				this.connect(this, this._event["input"], function(){
					this.validate(true);
				});
			}else{
				this.connect(this, this._event["focus"], function(){
					if (this.message && this._hasBeenBlurred && (!this._refocusing)) {
						this.displayMessage(this.message);
					}
				})
			}
		},
		refocus: function() {
			this._refocusing = true;
			this.focus();
			this._refocusing = false;
		},
		
		/**
		 * Tests if value is valid.
		 * Can override with the routine in a subclass.
		 * @public
		 * @param {boolean} isFocused
		 * If the widget focused
		 */
		_isValid: function(/*Boolean*/ isFocused){
			return this.isValid(isFocused) && !(this.required && this._isEmpty());
		},
		
		/**
		 * Checks if value is empty.
		 * Should be override with the routine in a subclass
		 * @public
		 */
		_isEmpty: function(){
			// summary:
			// 		Checks for whitespace. Should be overridden.
			return false;
		},
		
		/**
		 * Extension point for user customizing validation rules.
		 * @param {boolean} isFocused
		 */
		isValid: function(isFocused){
			return true;
		},
		
		/**
		 * Return proper error message according to the error type
		 * @param {boolean} isFocused
		 */
		getErrorMessage: function(/*Boolean*/ isFocused){
			return (this.required && this._isEmpty()) ? this.missingMessage : this.invalidMessage;
		},
		
		/**
		 * Perform validation for the widget, called by "input" event if "instantValidate" setting to true,
		 * called by "blur" event if "instantValidate" setting to false.
		 * @param {boolean} isFocused
		 */
		validate: function(/*Boolean*/ isFocused){
			var message, isValid = this.disabled || this._isValid(isFocused);
			this.set("state", isValid ? "" : (this._hasBeenBlurred || (isFocused && this.instantValidate)) ? "Error" : "Incomplete");
			if (this._hasBeenBlurred) {
				wai.setWaiState(this.focusNode, "invalid", !isValid);
			}
			if (this.state == "Error") {
				message = this.getErrorMessage(isFocused);
			}
			this._set("message", message);
			this.displayMessage(message);
			return isValid;
		},
		
		/**
		 * Show error message using a hoverHelpTooltip, hide the tooltip if message is empty.
		 * Overridable method to display validation errors/hints.
		 * By default uses a hoverHelpTooltip.
		 * @protected
		 * @param {string} message
		 * Error message
		 * @param {boolean} force
		 * Force displaying message if the value is true, no matter if the widget got focus or not.
		 */
		displayMessage: function(/*String*/ message, /*Boolean*/ force){
			if(message){
				if(!this.messageTooltip){
					this.messageTooltip = new HoverHelpTooltip({
						connectId: [this.iconNode],
						label: message,
						position: this.tooltipPosition,
						forceFocus: false
					});
				}else{
					this.messageTooltip.set("label", message);
				}
				if(this.focused || force){
					var node = domStyle.get(this.iconNode, "visibility") == "hidden" ? this.oneuiBaseNode : this.iconNode;
					this.messageTooltip.open(node);
				}
			}else{
				this.messageTooltip && this.messageTooltip.close();
			}
		},
		
		_onBlur: function(){
			this.inherited(arguments);
			this.displayMessage("");
		}
	});
});

