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

define(["dojo/_base/declare", "dojo/sniff","dojo/_base/lang","dojo/_base/window","dojo/dom","dojo/dom-attr","dojo/on","dojo/keys","dijit/Menu"],
        function(declare, has, lang, win, dom, domAttr, on, keys, Menu){
    
	// Ensure we're not relying on the old globals, ready for 2.0.
	var dojo = {}, dijit = {};

  
	/**
	 * Creates a new idx.widget._MenuOpenOnHoverMixin
	 * @name idx.widget._MenuOpenOnHoverMixin
	 * @class This mix-in can be mixed into menus and menu bars to make them
	 * permanently active so that their drop down and cascade menus are
	 * activated by mouse hover without the need for the menu or menu bar to
	 * be clicked on first.
	 */
	return declare("idx.widget._MenuOpenOnHoverMixin", null,
	/** @lends idx.widget._MenuOpenOnHoverMixin.prototype */
	{
		/**
		 * If true, this menu / menu bar will open popup menu items when they
		 * are hovered over at any time. This is in addition to the usual
		 * mouse-click and key activations, which continue to work as
		 * usual. If false, the menu / menu bar will still open popup menu
		 * items on hover when it is "active" (ie, when the user is in the
		 * process of interacting with it), but will NOT open popup menu
		 * items on hover when the menu is inactive: only mouse-click and
		 * key activations work in this case. The default value is true,
		 * enabling menus to be activated by hover.
		 * @type boolean
		 */
		openOnHover: false,

		/**
		 * Used internally to track our true activation state, because when
		 * openOnHover is on we never allow ourselves to become inactive even
		 * when we normally would.
		 * @type boolean
		 */
		_isActuallyActive: false,

		/**
		 * Standard Dojo setter for handling the 'opnOnHover' property via calls to 
		 * set().
		 * @param {Object} newvalue
		 */
		_setOpenOnHoverAttr: function(newvalue){
			this.openOnHover = newvalue;
			if(newvalue){
				this._forceActive();
			}else{
				this._disableActive();
			}
		},
			
		/**
		 * Mark the menubar as active regardless of whether it 'actually' is,
		 * but preserve our memory of our 'actual' active state.
		 */
		_forceActive: function(){
			this.set("activated", this._isActuallyActive = true);
		},
		
		/**
		 * Return to our 'actual' active state after a force.
		 */
		_disableActive: function(){
			this.set("activated", this._isActuallyActive = false);
		},
		
		onItemHover: function(/*MenuItem*/ item){
			// dojo/sniff cannot detect ie 11, temporarily fixing 
			// if(has("ie") && (has("ie") >= 11)){//for IE11+
			if(has("trident") && (has("trident") >=7)){ //for IE11+
			    //TODO
			}else{
			    this.focusChild(item);
			}
			this.inherited(arguments);
		},
		
		_cleanUp: function(){
			this.inherited(arguments);
			this.set("activated", this._isActuallyActive);
		},
		
		bindDomNode: function( node){
			var superBindDomNode = this.getInherited(arguments);
			if (! superBindDomNode) {
				console.log("Attempt to call bindDomNode() on a widget that does not "
							+ "appear to be a dijit/Menu");
				return;
			}
			
			// summary:
			//		Attach menu to given node
			node = dom.byId(node, this.ownerDocument);

			var cn;	// Connect node

			// Support context menus on iframes.  Rather than binding to the iframe itself we need
			// to bind to the <body> node inside the iframe.
			if(node.tagName.toLowerCase() == "iframe"){
				var iframe = node,
					window = this._iframeContentWindow(iframe);
				cn = win.body(window.document);
			}else{
				// To capture these events at the top level, attach to <html>, not <body>.
				// Otherwise right-click context menu just doesn't work.
				cn = (node == win.body(this.ownerDocument) ? this.ownerDocument.documentElement : node);
			}


			// "binding" is the object to track our connection to the node (ie, the parameter to bindDomNode())
			var binding = {
				node: node,
				iframe: iframe
			};

			// Save info about binding in _bindings[], and make node itself record index(+1) into
			// _bindings[] array.  Prefix w/_dijitMenu to avoid setting an attribute that may
			// start with a number, which fails on FF/safari.
			domAttr.set(node, "_dijitMenu" + this.id, this._bindings.push(binding));

			// Setup the connections to monitor click etc., unless we are connecting to an iframe which hasn't finished
			// loading yet, in which case we need to wait for the onload event first, and then connect
			// On linux Shift-F10 produces the oncontextmenu event, but on Windows it doesn't, so
			// we need to monitor keyboard events in addition to the oncontextmenu event.
			var doConnects = lang.hitch(this, function(cn){
				var selector = this.selector,
					delegatedEvent = selector ?
						function(eventType){
							return on.selector(selector, eventType);
						} :
						function(eventType){
							return eventType;
						},
					self = this;
				result = [
					// TODO: when leftClickToOpen is true then shouldn't space/enter key trigger the menu,
					// rather than shift-F10?
					on(cn, delegatedEvent(this.leftClickToOpen ? "click" : "contextmenu"), function(evt){
						// Schedule context menu to be opened unless it's already been scheduled from onkeydown handler
						evt.stopPropagation();
						evt.preventDefault();
						self._scheduleOpen(this, iframe, {x: evt.pageX, y: evt.pageY});
					}),
					on(cn, delegatedEvent("keydown"), function(evt){
						if(evt.shiftKey && evt.keyCode == keys.F10){
							evt.stopPropagation();
							evt.preventDefault();
							self._scheduleOpen(this, iframe);	// no coords - open near target node
						}
					})
				];
				result.push(
					on(cn, delegatedEvent("mouseover"), function(evt){
						if (self.openOnHover) {
							// Schedule context menu to be opened unless it's already been scheduled from onkeydown handler
							evt.stopPropagation();
							evt.preventDefault();
							self._scheduleOpen(this, iframe, {x: evt.pageX, y: evt.pageY});
						}
					})					
				);			
				return result;
			});
			binding.connects = cn ? doConnects(cn) : [];

			if(iframe){
				// Setup handler to [re]bind to the iframe when the contents are initially loaded,
				// and every time the contents change.
				// Need to do this b/c we are actually binding to the iframe's <body> node.
				// Note: can't use connect.connect(), see #9609.

				binding.onloadHandler = lang.hitch(this, function(){
					// want to remove old connections, but IE throws exceptions when trying to
					// access the <body> node because it's already gone, or at least in a state of limbo

					var window = this._iframeContentWindow(iframe),
						cn = win.body(window.document);
					binding.connects = doConnects(cn);
				});
				if(iframe.addEventListener){
					iframe.addEventListener("load", binding.onloadHandler, false);
				}else{
					iframe.attachEvent("onload", binding.onloadHandler);
				}
			}
		}	

	});
});