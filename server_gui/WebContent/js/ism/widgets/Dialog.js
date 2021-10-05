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

define([ "dojo/_base/declare", "dojo/dom-construct", "dojo/dom-attr", 	"dojo/dom-style", "dojo/_base/event", 
         "dojo/dom-class", "dojo/keys", "dojo/query", "idx/widget/Dialog", "idx/widget/SingleMessage", "dojo/domReady!",
         "dojo/window","dojo/dom-geometry", "dojo/aspect", 'ism/IsmUtils'],
		function(declare, domConstruct, domAttr, domStyle, event, domClass, keys, query, Dialog, SingleMessage, domReady, 
				winUtils, domGeom, aspect, Utils) {
			/**
			 * @name ism.widgets.Dialog
			 * @class ISM extension of One UI version Dialog which adds a
			 *        message area for showing errors
			 * @augments idx.widget.Dialog
			 */
			return declare("ism.widgets.Dialog", [ Dialog ], {
				// summary:
				// An extension of the One UI Dialog

				_messageAreaInitialised : null,

				_singleMessage : undefined,
				_messageDiv : undefined,
				
				overflowContentStyle: "auto",

				constructor : function(args) {
					dojo.safeMixin(this, args);
				},

				postCreate : function() {
					console.debug(this.declaredClass + ".postCreate()");
					this.inherited(arguments);

					if (!this._messageAreaInitialised) {
						var node = query(".dijitDialogPaneContentWrapper",
								this.domNode)[0];
						this._messageDiv = domConstruct.create("div", null, node,
								"first");
						this._messageAreaInitialised = true;
						// setting overflow to auto helps prevent the content from
						// overrunning the content area
						if (this.overflowContentStyle){
							domStyle.set(node, "overflow", this.overflowContentStyle);
						}
					}
					this.titleNode = query(".dijitDialogTitle", this.domNode)[0];
					this.instructionNode = query(".dijitDialogInstruction", this.domNode)[0];
					this.actionPaneNode = query(".dijitDialogPaneActionBar", this.domNode)[0];
					aspect.after(this, "show", function(){
						dojo.publish("lastAccess", {requestName: "dialog"});
					});
				},
				
				startup: function() {
					this.inherited(arguments);
					dojo.attr(this.closeButton.domNode, "id", this.id + "_closeButton"); 
				},

				hide : function() {
					this.inherited(arguments);
					
					// close the message area in case the dialog is reopened
					this.clearErrorMessage();
				},
				
				_size: function() {
					
					var node = query(".dijitDialogPaneContentWrapper",
							this.domNode)[0];
					
					// clear things so a resize works correctly when the dialog needs to grow
					domStyle.set(node, "maxHeight", "");
					domStyle.set(this.domNode, "overflow", "auto");

					this.inherited(arguments);

					
					// set the max-height of the content area and scroll if needed
					var dialogCS = domStyle.get(this.domNode);
					var dialogBox = domGeom.getContentBox(this.domNode, dialogCS);
					var titleCS = domStyle.get(this.titleNode);
					var titleBox = domGeom.getMarginBox(this.titleNode, titleCS);
					var instructionCS = domStyle.get(this.instructionNode);
					var instructionBox = domGeom.getMarginBox(this.instructionNode, instructionCS);
					var actionBox = domGeom.getMarginBox(this.actionPaneNode, domStyle.get(this.actionPaneNode));
					var maxHeight = dialogBox.h - titleBox.h - instructionBox.h - actionBox.h - 22;
					if (maxHeight < 100) {
						// really too small, just let the whole dialog scroll
						domStyle.set(node, "maxHeight", "");
						domStyle.set(this.domNode, "overflow", "auto");
					} else {
						domStyle.set(node, "maxHeight", maxHeight + "px");
						// prevent an occasional double scroll bar in IE 10
						domStyle.set(this.domNode, "overflow", "hidden");
						console.debug("dialog content maxHeight set to " + maxHeight + "px");
					}
				},
				
				showErrorMessage : function(title, message, code, isCertError) {
					return this._showMessage({
						type: 'error', 
						title: title,
						description: message ? message : "", 
						messageId: code ? code : "",
						collapsed: (isCertError !== undefined) ? !isCertError : false});
				},

				showWarningMessage : function(title, message, code) {
					return this._showMessage({
						type: 'warning', 
						title: title,
						description: message ? message : "",
						messageId: code ? code : ""});
				},
				
				showInfoMessage : function(title, message, code) {
					return this._showMessage({
						type: 'information', 
						title: title,
						description: message ? message : "",
						messageId: code ? code : ""});
				},
				
				_showMessage: function(options) {			
					if (!options) {
						options = {};
					}
					
					this.clearErrorMessage();
					
					var messageDiv = domConstruct.create("div", null, this._messageDiv, "first");
					options.dateFormat = options.dateFormat ? options.dateFormat : {selector: 'time', timePattern: ism.user.localeInfo.timeFormatString};
					options.showRefresh = false;
					options.showAction = false;
					options.showDetailsLink	= false;
					options.style = 'width: 98%';
					this._singleMessage = new SingleMessage(options, messageDiv);
					var _this = this;
					aspect.after(this._singleMessage, "onClick", function() {
						_this.resize();
					});
                    if (this._singleMessage.collapsed === false) {
                        this.resize();
                    }

					// scroll to top
					this._messageDiv.scrollTop = 0;
					this._singleMessage.focusNode.focus();					
					
					return this._singleMessage;
				},

				
				clearErrorMessage : function() {
					console.debug("clearing message");
					if (this._singleMessage) {
						this._singleMessage.destroy();
						this._singleMessage = undefined;
					}
				},

				showXhrError : function(title, error, ioargs) {
					var response = undefined;
					var message = undefined;
					if (ioargs && ioargs.xhr) {
						if (ioargs.xhr.response) {
							response = JSON.parse(ioargs.xhr.response);
						} else if (ioargs.xhr.responseText) {
							response = JSON.parse(ioargs.xhr.responseText);
						}
					} 
					if (response == undefined) {
						console.error("showXhrError can't find a response: ", ioargs.xhr);
						// is there an error message?
						if (error && error.message) {
							message = error.message;
						} else if (ioargs && ioargs.error && ioargs.error.message) {
							message = ioargs.error.message;
						}
						if (title && title !== "") {
							this.showErrorMessage(title, message, null, false);							
						}
						return null;
					}
					message = response.message || null;
					var code = response.code || null;
					this.showErrorMessage(title, message, code, false);
					return code; // return the extracted code
				},
				
				showErrorFromPromise: function(title, error, adminEndpoint, addServerError) {
		            // is it a certificate issue?
		            var message = Utils.checkForCertificateError(error, adminEndpoint, addServerError);
		            if (message !== null) {
                        this.showErrorMessage(title, message, "", true);
		            } else {
	                    this.showErrorMessage(title, Utils.getErrorMessageFromPromise(error), Utils.getErrorCodeFromPromise(error), false);		                
		            }
				},
				
				showErrorList: function(title, errors) {
					var message = this._processErrorList(errors);
					this.showErrorMessage(title, message, "", false);
				},
				
				getQuery : function(ioargs) {
					var query = JSON.parse(ioargs.query);
					return query;
				},
				
				_processErrorList: function(errors) {
					var results = "<table role='presentation' style='margin: 10px 0;'>";
					if (errors && errors.length > 0) {
						for (var i = 0; i < errors.length; i++) {
							var result = "<tr><td style='padding: 0 5px 0 0;'>" + (errors[i].name ? errors[i].name : " ") +"</td>";
							result += "<tr><td style='padding: 0 5px 0 0;'>" + errors[i].errorMessage +"</td>";
							results += result;
						}
					}
					results += "</table>";
					return results;
				},
				
				_onKey: function(evt){
					if(evt.keyCode != keys.ENTER) {
						this.inherited(arguments);
						return;
					}
					var node = evt.target;
					if(domAttr.has(node, "href")){return;}
					if(node == this.closeButton.domNode){return;}
					while(node){
						if(node == this.domNode || domClass.contains(node, "dijitPopup")){
							if (this.hasOwnProperty("onEnter")) {
								this.onEnter();
							} else {
								this.onExecute();
							}
						}
						node = node.parentNode;
					}
					event.stop(evt);
				}
			});
		});
