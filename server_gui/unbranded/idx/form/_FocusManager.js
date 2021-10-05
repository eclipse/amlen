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
	"dijit/focus",
	"dojo/_base/window",
	"dojo/window",
	"dojo/dom", // dom.isDescendant
	"dojo/dom-attr",
	"dojo/dom-class",
	"dojo/_base/declare", // declare
	"dojo/_base/lang", // lang.extend	
	"dijit/registry"
], function(focus, win, winUtils, dom, domAttr, domClass, declare, lang, registry){
	
	var lastFocusin;
	var lastTouchOrFocusin;
	
	focus._onTouchNode = function(/*DomNode*/ node, /*String*/ by){
		// Keep track of time of last focusin or touch event.
		lastTouchOrFocusin = (new Date()).getTime();
		
		var srcNode = node;
		if(this._clearActiveWidgetsTimer){
			clearTimeout(this._clearActiveWidgetsTimer);
			delete this._clearActiveWidgetsTimer;
		}
		
		// if the click occurred on the scrollbar of a dropdown, treat it as a click on the dropdown,
		// even though the scrollbar is technically on the popup wrapper (see dojo ticket #10631)
		if(domClass.contains(node, "dijitPopup")){
			node = node.firstChild;
		}
		
		var newStack=[];
		try{
			while(node){
				var popupParent = domAttr.get(node, "dijitPopupParent");
				if(typeof node.dijitPopupParent == "object"){
					node = node.dijitPopupParent;
				}else if(popupParent){
					node = registry.byId(popupParent) ? 
						registry.byId(popupParent).domNode :
						dom.byId(popupParent);
				}else if(node.tagName && node.tagName.toLowerCase() == "body"){
					if(node === win.body()){
						break;
					}
					node=winUtils.get(node.ownerDocument).frameElement;
				}else{
					var id = node.getAttribute && node.getAttribute("widgetId"),
						widget = id && registry.byId(id);
					if(widget && !(by == "mouse" && widget.get("disabled"))){
						if(!widget._isValidFocusNode || widget._isValidFocusNode(srcNode)){
							newStack.unshift(id);
						}
					}
					node=node.parentNode;
				}
			}
		}catch(e){}
		this._setStack(newStack, by);
	};
	
	focus._onBlurNode = function(/*DomNode*/ node){

		var now = (new Date()).getTime();

		if(now < lastFocusin + 100){
			return;
		}

		if(this._clearFocusTimer){
			clearTimeout(this._clearFocusTimer);
		}
		this._clearFocusTimer = setTimeout(lang.hitch(this, function(){
			this.set("prevNode", this.curNode);
			this.set("curNode", null);
		}), 0);

		if(this._clearActiveWidgetsTimer){
			clearTimeout(this._clearActiveWidgetsTimer);
		}

		if(now < lastTouchOrFocusin + 100){
			// This blur event is coming late (after the call to _onTouchNode() rather than before.
			// So let _onTouchNode() handle setting the widget stack.
			// See https://bugs.dojotoolkit.org/ticket/17668
			return;
		}

		this._clearActiveWidgetsTimer = setTimeout(lang.hitch(this, function(){
			delete this._clearActiveWidgetsTimer;
			this._setStack([]);
		}), 0);
	};
	
	focus._onFocusNode = function(/*DomNode*/ node){

		if(!node){
			return;
		}

		if(node.nodeType == 9){
			return;
		}

		// Keep track of time of last focusin event.
		lastFocusin = (new Date()).getTime();

		// There was probably a blur event right before this event, but since we have a new focus,
		// forget about the blur
		if(this._clearFocusTimer){
			clearTimeout(this._clearFocusTimer);
			delete this._clearFocusTimer;
		}

		this._onTouchNode(node);

		if(node == this.curNode){ return; }
		this.set("prevNode", this.curNode);
		this.set("curNode", node);
	};
	
	
	
	return focus;
});
