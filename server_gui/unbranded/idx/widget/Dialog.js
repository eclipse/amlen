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
	"require",
	"dojo/_base/array", // array.forEach array.indexOf array.map
	"dojo/_base/connect", // connect._keypress
	"dojo/_base/declare", // declare
	"dojo/_base/Deferred", // Deferred
	"dojo/dom", // dom.isDescendant
	"dojo/dom-class", // domClass.add domClass.contains
	"dojo/dom-construct",
	"dojo/dom-style", // domStyle.set
	"dojo/dom-attr", // attr.has
	"dojo/_base/event", // event.stop
	"dojo/_base/fx", // fx.fadeIn fx.fadeOut
	"dojo/i18n", // i18n.getLocalization
	"dojo/_base/kernel", // kernel.isAsync
	"dojo/keys",
	"dojo/_base/lang", // lang.mixin lang.hitch
	"dojo/on",
	"dojo/ready",
	"dojo/_base/sniff", // has("ie") has("opera")
	"dojo/_base/window", // win.body
	"dijit/focus",
	"dijit/a11y",
	"dijit/_base/manager",	// manager.defaultDuration
	"dijit/Dialog",
	"idx/widget/_DialogResizeMixin",
	"dijit/form/Button",
	"dojo/text!./templates/Dialog.html",
	"dijit",			// for back-compat, exporting dijit._underlay (remove in 2.0)
	"dojo/i18n!./nls/Dialog"
], function(require, array, connect, declare, Deferred,
			dom, domClass, domConstruct, domStyle, domAttr, event, fx, 
			i18n, kernel, keys, lang, on, ready, has, win,
			focus, a11y, manager, Dialog, _DialogResizeMixin, Button, template, dijit, dialogNls){
	var oneuiRoot = lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2
	
	/**
	 * Creates a new idx.widget.Dialog
	 * @name idx.widget.Dialog
	 * @class idx.widget.Dialog enhanced dijit.Dialog with specified structure following IBM One UI standard
	 * <b><a href="http://mds.torolab.ibm.com/IBM_OneUI/UI_DesignSignature_Workstream/UI_design_signature/9%2E%20%20Forms_Dialog%20Box/VS_DialogBox_BP_Mar-21-12%2Epng">Dialog Box</a></b> 
	 * @augments dijit.Dialog
	 * @example
	 * var dialog = new idx.widget.Dialog({
			id: "dialog",
			title: "Dialog Title",
			instruction: "Instructional information goes here.",
			content: "&lt;div style='height:80px'&gt;Lorem ipsum dolor sit amet, consectetuer adipiscing elit.&lt;/div&gt;",
			reference: {
				name: "Link goes here",
				link: "http://dojotoolkit.org/"
			},
			closeButtonLabel: "Cancel"
		}, "div");
	 */
	var Dialog = declare("idx.widget.Dialog", [Dialog, _DialogResizeMixin], {
	/**@lends idx.widget.Dialog.prototype*/
		templateString: template,
		baseClass: "idxDialog",
		
		draggable: true,
		/**
		 * Dialog title
		 * @type String
		 */
		title: "",
		/**
		 * Dialog instruction, just below the title
		 * @type String
		 */
		instruction: "",
		/**
		 * Dialog content
		 * @type String | dijit.layout.TabContainer
		 */
		content: "",
		/**
		 * Referance link of Dialog, reference.name for link name, and reference.link for link url
		 * @type Object
		 */
		reference: {
			name: "",
			link: ""
		},
		/**
		 * Action buttons for Dialog in the action bar
		 * @type Array [dijit.form.Button]
		 */
		buttons: null,
		/**
		 * Label on Dialog close button
		 * @type String
		 */
		closeButtonLabel: "",
		
		referenceName: "",
		referenceLink: "",
		
		executeOnEnter: true,
		
		postCreate: function(){
			this.inherited(arguments);
			this.closeButton = new Button({
				label: this.closeButtonLabel || dialogNls.closeButtonLabel,
				onClick: lang.hitch(this, function(evt){
					this.onCancel();
					event.stop(evt);
				})
			}, this.closeButtonNode);
			array.forEach(this.buttons, function(button){
				if(button.declaredClass == "dijit.form.Button"){
					if(!domClass.contains(this.closeButton.domNode, "idxSecondaryButton")){
						domClass.add(this.closeButton.domNode, "idxSecondaryButton");
					}
					domConstruct.place(button.domNode, this.closeButton.domNode, "before");
				}
			}, this);
		},
		startup: function(){
			this.inherited(arguments);
			//display content and make it as tab stop
			if(this.containerNode.innerHTML || this.href){
				domAttr.set(this.containerNode, "tabindex", 0);
				domStyle.set(this.contentWrapper, "display", "block");
			}
			//re-style content if it's a tabcontainer
			if(this.containerNode.children[0] && 
			domClass.contains(this.containerNode.children[0], "dijitTabContainer")){
				domStyle.set(this.contentWrapper, {
					borderTop: "0 none",
					paddingTop: "0"
				});
			}
		},
		
		_getFocusItems: function(){
			//	summary:
			//		override _DialogMixin._getFocusItems.
			if(this._firstFocusItem){
				this._firstFocusItem = this._getFirstItem();
				if(!this._firstFocusItem){
					var elems = a11y._getTabNavigable(this.containerNode);
					this._firstFocusItem = elems.lowest || elems.first || this.closeButton.focusNode || this.domNode;
				}
				return;
			}
			var elems = a11y._getTabNavigable(this.containerNode);
			this._firstFocusItem = elems.lowest || elems.first || this.closeButton.focusNode;
			this._lastFocusItem = this.closeButton.focusNode;
		},
		hide: function(){
			var promise = this.inherited(arguments);
			this._firstFocusItem = null;
			return promise;
		},
		_getFirstItem: function(){
			if(this.title){return this.titleNode;}
			if(this.instruction){return this.instructionNode;}
			if(this.content){return this.containerNode;}
			return null;
		},
		_setTitleAttr: function(title){
			this.titleNode.innerHTML = title;
			domAttr.set(this.titleNode, "tabindex", title ? 0 : -1);
		},
		_setInstructionAttr: function(instruction){
			this.instructionNode.innerHTML = instruction;
			domAttr.set(this.instructionNode, "tabindex", instruction ? 0 : -1);
		},
		_setContentAttr: function(content){
			this.inherited(arguments);
			var isEmpty = this.containerNode.innerHTML;
			domAttr.set(this.containerNode, "tabindex", isEmpty ? 0 : -1);
			domStyle.set(this.contentWrapper, "display", isEmpty ? "block" : "none");
		},
		_setReferenceAttr: function(reference){
			this.reference = reference;
			this.referenceName = (this.reference && this.reference.name) ? this.reference.name : "";
			this.referenceLink = (this.reference && this.reference.link) ? this.reference.link : "";
			this._updateReferenceLink();
		},
		_setReferenceNameAttr: function(name){
			this.referenceName = name;
			this._updateReferenceLink();
		},
		_setReferenceLinkAttr: function(link){
			this.referenceLink = link;
			this._updateReferenceLink();
		},
		_updateReferenceLink: function(){
			var referenceLinkNodeHidden = !this.referenceLink || !this.referenceName;
			domClass.toggle(this.referenceNode, "referenceLinkHidden", referenceLinkNodeHidden);
			domAttr.set(this.referenceNode, {
				"href": referenceLinkNodeHidden ? "about:blank": this.referenceLink, 
				innerHTML: referenceLinkNodeHidden ? "NO LINK" : this.referenceName
			});
		},
		/**
		 * Callback of reference click, overriden by user
		 */
		onReference: function(event){
			
			return true;
		}
	});

	// Back compat w/1.6, remove for 2.0
	if(!kernel.isAsync){
		ready(0, function(){
			var requires = ["dijit/TooltipDialog"];
			require(requires);	// use indirection so modules not rolled into a build
		});
	}

	// for IDX 1.2 compatibility
	oneuiRoot.Dialog = Dialog;
	
	return Dialog;
});
