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
	"dojo/_base/kernel", // kernel.isAsync
	"dojo/_base/array", // array.forEach array.indexOf array.map
	"dojo/_base/declare", // declare
	"dojo/_base/html", // Deferred
	"dojo/_base/event", // event.stop
	"dojo/_base/lang", // lang.mixin lang.hitch
	"dojo/query", // Query
	"dojo/dom-attr", // attr.set
	"dojo/dom-class", // domClass.add domClass.contains
	"dojo/dom-style", // domStyle.set
	"dojo/i18n", // i18n.getLocalization
	"dojo/keys",
	"dojo/on",
	"dojo/ready",
	"dojo/date/locale",
	"dijit/_base/wai",
	"dijit/_base/manager",	// manager.defaultDuration
	"dijit/a11y",
	"dijit/focus",
	"dijit/layout/ContentPane",
	"dijit/Dialog", 
	"idx/widget/_DialogResizeMixin",
	"dijit/layout/TabContainer", 
	"dijit/TitlePane", 
	"dijit/form/Button",
	"dojo/text!./templates/ModalDialog.html",
	"dojo/i18n!./nls/ModalDialog"
], function(kernel, array, declare, html, event, lang, 
		query, domAttr, domClass, domStyle, i18n, keys, on, ready, locale, 
		wai, manager, a11y, focus, ContentPane, Dialog, _DialogResizeMixin,
		TabContainer, TitlePane, Button, template){
	var iMessaging = lang.getObject("idx.oneui.messaging", true); // for backward compatibility with IDX 1.2
	
	/**
	* @name idx.widget.ModalDialog
	* @class The ModalDialog provides the standard OneUI Modal Dialog.
	* Pops up a modal dialog window, blocking access to the screen
	* and also graying out the screen.
	* @augments dijit.Dialog
	* @see The <a href="http://livedocs.dojotoolkit.org/dijit/info">dijit.Dialog</a>.
	*/ 
	return iMessaging.ModalDialog = declare("idx.widget.ModalDialog", [Dialog, _DialogResizeMixin], {
	/**@lends idx.widget.ModalDialog*/ 
		templateString: template,
		widgetsInTemplate: true,
		baseClass: "idxModalDialog",
		alert: false, // Determines if the modal dialog is 'alertdialog' role. 
		_messagingTypeMap: {
			error: "Error",
			warning: "Warning",
			information: "Information",
			success: "Success",
			confirmation: "Confirmation",
			question: "Question",
			progress: "Progress"
		},
		/**
		 * Message type
		 * @type String
		 */
		type: "",
		/**
		 * Message summary 
		 * @type String
		 */
		text: "",
		/**
		 * Message main content, create compact tab container in array
		 * @type String | Array[{title, content}]
		 */
		info: "",
		/**
		 * Message identifier
		 * @type String
		 */
		messageId: "",
		/**
		 * Message additional reference
		 * @type HTML URL
		 */
		messageRef: null,
		/**
		 * Timestamp of Message
		 * @type String | Date
		 */
		messageTimeStamp: new Date(),
		/**
		 * The options being used for format the timestamp.
		 * Example:
		 * <pre>
		 * dateFormat: {
		 * 	formatLength: "medium",
		 * }
		 * </pre>
		 * @type dojo.date.locale.__FormatOptions
		 */
		dateFormat: {
			formatLength: "short"
		},
		/**
		 * Close button label
		 * @type String
		 */
		closeButtonLabel: "",
		/**
		 * Specifies whether to show an action bar with buttons.
		 * @type Boolean
		 * @default true
		 */
		showActionBar: true,
	
		/**
		 * Specifies whether to show an icon.
		 * @type Boolean
		 * @default true
		 */
		showIcon: true,
	
		/**
		 * Specifies whether to show a cancel button.
		 * @type Boolean
		 * @default false
		 */
		showCancel: true,
		
		executeOnEnter: true,
		
		/** @ignore */
		postMixInProperties: function(){
			//	Set "Information" as default messaging type.no 
			this._nlsResources = i18n.getLocalization("idx.widget", "ModalDialog", this.lang);
			this._a11yImageMapping = {
				error: "X",
				warning: "!",
				information: "i",
				success: "√",
				confirmation: "!",
				question: "?",
				progress: "◇"
			}
			this.messageIconText = this._a11yImageMapping[(this.type || "information").toLowerCase()];
			var type = this._messagingTypeMap[(this.type || "information").toLowerCase()],
				title = this._nlsResources[type] || "Information";
			lang.mixin(this, {title: title, type: type});
			lang.mixin(this.params, {type: type});
			//	Set error modal dialog as 'alertdialog' role by default.
			if(!this.alert && (this.type == "Error")){
				this.alert = true;
			}
			this.inherited(arguments);
		},
		/** @ignore */
		buildRendering: function(){
			this.inherited(arguments);
			if(!this.messageId && this.reference){
				domStyle.set(this.reference, "display", "none");
			}
			domAttr.set(this.iconText, "innerHTML", this.messageIconText);
			if(this.info && lang.isArray(this.info)){
				this.tabs = new TabContainer({
					useMenu: false,
					useSlider: false,
					style: "height:175px"
				}, this.containerNode);
				domStyle.set(this.messageWrapper, "borderTop", "0 none");
				array.forEach(this.info, function(item){
					var contentPane = new ContentPane({
						title: item.title,
						content: item.content
					});
					//wai.setWaiRole(contentPane.domNode, "document");
					this.tabs.addChild(contentPane);
				}, this);
			}
			
			this.showActionBarNode(this.showActionBar);
			this.showIconNode(this.showIcon);
		},
		/** @ignore */
		postCreate: function(){
			this.inherited(arguments);
			domStyle.set(this.confirmAction, "display", "none");
			this.closeAction = new Button({
				label: this.closeButtonLabel || this._nlsResources.closeButtonLabel || "Close",
				onClick: lang.hitch(this, function(evt){
					this.onCancel();
					event.stop(evt);
				})
			}, this.closeAction);
			this.showCancelNode(this.showCancel);
			
			if(this.tabs){
				this.connect(this, "show", function(){
					// enable focus indications for message details as static text.
					query(".dijitTabPane",this.messageWrapper).attr("tabindex", 0).style({padding:"6px",margin:"2px"});
					this.tabs.resize();
				});
			}else{
				//wai.setWaiRole(this.containerNode, "document");
				if(this.info){
					this.set("content", this.info);
				}
			}
			if(this.alert){
				wai.setWaiRole(this.domNode, "alertdialog");
			}
			query(".dijitTitlePaneContentInner", this.messageWrapper).attr("tabindex", 0);
			
			if(this.reference){
				if(this.messageRef){
					domAttr.set(this.reference, "href", this.messageRef);
				}else{
					domClass.add(this.reference, "messageIdOnly");
				}
			}
			if(!this.info && !lang.trim(this.containerNode.innerHTML)){
				domStyle.set(this.messageWrapper, "display", "none");
			} 
		},
		_onKey: function(evt){
			this.inherited(arguments);
			if(!this.executeOnEnter){return;}
			var node = evt.target;
			if(domAttr.has(node, "href")){return;}
			if(node == this.closeAction.focusNode || node == this.confirmAction.focusNode){return;}
			while(node){
				if(node == this.domNode || domClass.contains(node, "dijitPopup")){
					if(evt.keyCode == keys.ENTER){
						this.onExecute();
					}else{
						return; // just let it go
					}
				}
				node = node.parentNode;
			}
			event.stop(evt);
		},
		/** @ignore */
		startup: function(){
			if(this.tabs){
				this.tabs.startup();
			}
			this.inherited(arguments);
		},
		_setTypeAttr: function(type){
		    var oldType = this.type;
			this.type = type;
			var title = this._nlsResources[this.type] || "Information";
			this.set("title", title);
			
			var oldTypeName = this._messagingTypeMap[(oldType || "information").toLowerCase()];
			var typeName = this._messagingTypeMap[(this.type || "information").toLowerCase()];
			domClass.remove(this.domNode, "idx" + oldTypeName + "MessageDialog");
			domClass.add(this.domNode, "idx" + typeName + "MessageDialog");
			// Add type class to domNode
			
		},
		_setTextAttr: function(text){
			this.description.innerHTML = this.text = text;
		},
		/**
		 * Get the focused item in the content of dialog
		 */
		_getFocusItems: function(){
			//	summary:
			//		override _DialogMixin._getFocusItems.
			if(this._firstFocusItem){
				this._firstFocusItem = this.description;
				return;
			}
			if(!this.tabs){
				this._firstFocusItem = this.closeAction.focusNode;
				this._lastFocusItem = //this.messageId == "" ? this.description : this.reference;
					this.closeAction.focusNode;
			}else{
				var elems = a11y._getTabNavigable(this.messageWrapper);
				this._firstFocusItem = elems.lowest || elems.first || this.closeButtonNode || this.domNode;
				this._lastFocusItem = this.closeAction.focusNode;//this.description;
			}
		},
		/**
		* hide the dialog
		*/
		hide: function(){
			this._firstFocusItem = null;
			return this.inherited(arguments);
		},
		
		/**
		 * Shows an action bar.
		 * @param {Boolean} yes
		 */
		showActionBarNode: function(yes){
			domStyle.set(this.actionBar, "display", yes? "": "none");
		},
	
		/**
		 * Shows an icon.
		 * @param {Boolean} yes
		 */
		showIconNode: function(yes){
			domStyle.set(this.icon, "display", yes? "": "none");
		},
	
		/**
		 * Shows a cancel button
		 * @param {Boolean} yes
		 */
		showCancelNode: function(yes){
			domStyle.set(this.closeAction.domNode, "display", yes? "": "none");
		},
		
		/**
		 * Sets a label string for the cancel button.
		 * @param {String} s
		 * @private
		 */
		_setLabelCancelAttr: function(label){
			this.closeAction.set("label", label || this._nlsResources.cancelButtonLabel || "Cancel");
		},
		_setMessageTimeStampAttr: function(date){
			this._set(this.messageTimeStamp, date);
			if(!this.timestamp)return;
			if(this.messageTime && date){
				this.timestamp.innerHTML = locale.format(this.messageTimeStamp, this.dateFormat);
			}else{
				domStyle.set(this.timestamp, "display", "none");
			}
		},
		/**
		 * Callback of reference click, overriden by user
		 */
		onReference: function(event){
			
			return true;
		}
	
	});
});