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

// This mix-in provides support for binding a widget to one or more DOM nodes 
// and being notified when specified events occur. Support for a "hover" trigger
// is also provided.

define(["dojo/_base/array",
        "dojo/_base/declare",
        "dojo/_base/event",
        "dojo/_base/lang",
        "dojo/dom",
		"dojo/request/iframe",
        "dojo/mouse",
        "dojo/on",
        "dojo/window",
        "dijit/_MenuBase"],
        function(array,
                 declare,
                 event,
                 lang,
                 dom,
				 iframe,
                 mouse,
                 on,
                 winUtils,
                 _MenuBase){
    
	// Ensure we're not relying on the old globals, ready for 2.0.
	var dojo = {}, dijit = {};

	/**
	 * Creates a new idx.widget._EventTriggerMixin
	 * @name idx.widget._EventTriggerMixin
	 * @class This mix-in provides support for binding a widget to one or more
	 * DOM nodes and being notified when specified events occur. Support for a
	 * "hover" trigger is also provided.
	 */
	return declare("idx.widget._EventTriggerMixin", null, 
	/** @lends idx.widget._EventTriggerMixin.prototype */
	{
		/**
		 * An array of "binding" objects that store details of the event
		 * triggers that are currently bound to trigger nodes.
		 * @type Object[]
		 */
		_bindings: null,
    
		/**
		 * A timer that is started at the beginning of a hover event and
		 * triggers the calling of onTrigger() after hoverDuration, unless it
		 * is cancelled.
		 * @type timer handle
		 */
		_hoverTimer: null,
    
		/**
		 * The time in milliseconds that the mouse needs to remain within a DOM
		 * node in order to cause a "hover" event to be reported. Initialised
		 * to match the hover delay used by menus.
		 * @type number
		 */
		hoverDuration: _MenuBase.prototype.popupDelay,
    
		/**
		 * Constructor.
		 * @private
		 */
		constructor: function(){
			this._bindings = [];
		},

		/**
		 * Register a DOM node to act as an event trigger. When the specified
		 * event is detected, the "_onTrigger" method is called, passing a
		 * trigger object containing contextual and event information as the
		 * single argument. The fields in the trigger object are as follows:
		 * <ul>
		 * <li>
		 * triggerNode: {DOMNode} the DOM node that triggered the event
		 * </li>
		 * <li>
		 * eventName: {string} the name of the event (as passed as eventName)
		 * </li>
		 * <li>
		 * event: {Event} the event object (in the case of "hover", this will be
		 * the most recent mouse.enter or mousemove event received
		 * </li>
		 * <li>
		 * additionalData: {Object} the additional data object supplied as additionalData
		 * </li>
		 * </ul>
		 * @param {string | DOMNode} triggerNode The DOM node or node ID that
		 * trigger events are to be detected on.
		 * @param {string} eventName The name of the event to be detected.
		 * As well as regular dojo events the value "hover" can also be used
		 * to configure a hover trigger.
		 * @param {function(params)} filterCallback An optional filter callback
		 * function that can be used to limit the events that are detected.
		 * If the function returns true the event is considered to be a
		 * trigger otherwise it is ignored. The same params object is supplied
		 * to the filter callback that will be passed to the _onTrigger method
		 * if the filter callback returns true. 
		 * @param {Object} additionalData Optional application-specific data
		 * that will be returned when an event is detected.
		 */
		_addEventTrigger: function(triggerNode, eventName, filterCallback, additionalData) {
 			// Resolve node ID (if passed as parameter) and validate DOM node.
			triggerNode = dom.byId(triggerNode);
			if(!triggerNode){
				require.log("ERROR: oneui._EventTriggerMixin._addEventTrigger(): Invalid triggerNode parameter.");
				return;
			}

			// a utility function to check the filter and call onTrigger
			// using data in the closure, hitched this, and a passed event			
			var trigger = lang.hitch(this, function(event){
				var params = { triggerNode: triggerNode, eventName: eventName, event: event, additionalData: additionalData }
				
				// If a filter callback is not defined or it returns true then
				// trigger the event. Otherwise ignore it.
				if (!filterCallback || filterCallback(params)) {
					this._onTrigger(params);
				}
			});
			
			// A utility function to create a 'hover' pseudo-event from real
			// mouse events.
			var createHoverEvent = function(event){
				return {
					type: "hover",
					pageX: event.pageX,
					pageY: event.pageY,
					screenX: event.screenX,
					screenY: event.screenY,
					clientX: event.clientX,
					clientY: event.clientY
				};
			};
      
			// store a binding object per call, containing:
			//   triggerNode: the node we attach a listener/listeners to
			//   connectHandles: an array of currently attached listeners
			//   hoverDuration: if eventName=="hover", the hover duration in ms
			//   hoverTimer: the handle to any currently-running hover timer
			//   bindFunction: a function that attaches the required listener(s)
			//   unbindFunction: a function that removes the listener(s)
			//   iframeOnLoadHandler: if triggerNode is an iframe, a function
			//                         that reattaches the listeners on iframe load
			var binding = { triggerNode: triggerNode, connectHandles: [] }
			
			if(eventName == "hover"){
				binding.hoverDuration = this.hoverDuration;
				binding.hoverTimer = null;
			} 
			
			// Create a function that implements the necessary binding logic.
			binding.bindFunction = function(){
      
				// Determine which node to connect event listeners to, which may need to 
				// be different to the specified trigger node.
				var connectNode;
				if(triggerNode.tagName == "IFRAME"){
					// If the trigger node is an iFrame then we need to attach event 
					// listeners to the <body> element of the iFrame's contents. 
					try{
						var iframeDocNode = iframe.doc(triggerNode);
						connectNode = iframeDocNode ? iframeDocNode.body : null;
					}catch(e){
						require.log("ERROR: oneui._EventTriggerMixin._addEventTrigger(): Error accessing body of document within iframe. " + e);
					}
				}else{
					// Normal trigger node specified - just attach event listeners to it.
					connectNode = triggerNode;
				}
				
				if(!connectNode){
					require.log("ERROR: oneui._EventTriggerMixin._addEventTrigger(): Unable to determine node to attach event listener(s) to.");
					return;
				}
				
				// If the event is "hover" then attach specific event listeners and 
				// callbacks to synthesise it, otherwise just attach the specified
				// event listener.
				if(eventName == "hover"){
					var hoverEvent = null;
					
					// Connect mouseenter and mouseleave event listeners to the 
					// connection node to start/clear the hover timer. Trigger
					// if/when the timer expires. Keep track of the mouse recent
					// mouse event too.
					binding.connectHandles.push(on(connectNode, mouse.enter, lang.hitch(this, function(event){
						hoverEvent = createHoverEvent(event);
						
						// there shouldn't be a running timer, but clear it if there is
						if(binding.hoverTimer){
							clearTimeout(binding.hoverTimer);
						} 
						
						// the hoverEvent may be replaced by the time the hover timer fires
						binding.hoverTimer = setTimeout(function(){ trigger(hoverEvent); }, binding.hoverDuration);
					})));
					
					binding.connectHandles.push(on(connectNode, mouse.leave, lang.hitch(this, function(event){
						if(binding.hoverTimer){
							clearTimeout(binding.hoverTimer);
							binding.hoverTimer = null;
						}
						hoverEvent = undefined;
					})));
					
					binding.connectHandles.push(on(connectNode, "mousemove", function(event){
						hoverEvent = createHoverEvent(event);
					}));
					
				}else{
					// Connect the appropriate event listener to the connect node.
					binding.connectHandles.push(on(connectNode, eventName, function(event){ trigger(event); }));
				}
			}
			
			// Create a disconnect function that removes all the event listeners.
			binding.unbindFunction = function(){
				array.forEach(binding.connectHandles, function(conn){ conn.remove(); });
				
				if(binding.hoverTimer){
					clearTimeout(binding.hoverTimer);
					binding.hoverTimer = null;
				}
			}
			
			// If the trigger node is an iframe then add an onload listener to it, 
			// with a callback that rebinds the handlers to the iframe content if 
			// it changes. Store the connect handle in the binding, so that it can
			// be cleaned up.
			if(triggerNode.tagName === "IFRAME"){
				binding.iframeOnLoadHandler = function(event){
					// Remove all existing event listeners.
					try{
						// This may fail because the iframe content node that the event
						// listeners are bound to has been destroyed.
						binding.unbindFunction();
					}catch(e){
						// Ignore failures - 'best efforts' cleanup.
					}

					// Rebind all of the event listeners to the iframe and new content 
					// document.
					binding.bindFunction();
				}
				
				if(triggerNode.addEventListener){
					triggerNode.addEventListener("load", binding.iframeOnLoadHandler, false);
				}else{
					triggerNode.attachEvent("onload", binding.iframeOnLoadHandler);
				}
			}

			// store the binding data, and call the bind function once now
			this._bindings.push(binding);
			binding.bindFunction(); 
		},

		/**
		 * Callback that is called whenever one of the specified trigger
		 * events (as configured via _addEventTrigger()) is detected on
		 * a DOM node.
		 * @param {Object} trigger An object that describes the triggering 
		 * event. The fields in the trigger object are as follows:
		 * <ul>
		 * <li>
		 * triggerNode: {DOMNode} the DOM node that triggered the event
		 * </li>
		 * <li>
		 * eventName: {string} the name of the event
		 * </li>
		 * <li>
		 * event: {Event} the event object (in the case of "hover", this will be
		 * a pseudo-event with properties of clientX, clientY, pageX, pageY, 
		 * screenX, screenY set when the data is available from the original 
		 * event, and type set to 'hover').
		 * </li>
		 * <li>
		 * additionalData: {Object} the additional data object supplied as
		 * additionalData via _addEventTrigger()
		 * </li>
		 * </ul>
		 */
		_onTrigger: function(trigger) {
			// summary:
			//   Callback that is called whenever one of the specified trigger events
			//   (as configured via _addEventTrigger()) is detected on a DOM node.
			//
			// parameters:
			//   trigger: Object
			//     an object containing data about the event.
			//
			//         triggerNode: DOMNode
			//           the DOM node that the trigger event was detected on.
			//
			//         eventName: String
			//           the name of the event that was detected.
			//
			//         event: Event
			//           the dojo event object describing the event.
			//
			//         additionalData: Object
			//           optional application-specific data that may have been 
			//           supplied when the event trigger was configured via
			//           _addEventTrigger().
			//
			// returns: nothing.
		},
    
		/**
		 * Deregisters a DOM node as an event trigger, by removing all the 
		 * event triggers that have been configured via _addEventTrigger().
		 * @param {string | DOMNode} triggerNode The DOM node or node ID that
		 * trigger events are no longer to be detected on. If undefined or
		 * null all event triggers for all nodes are unbound.
		 */
		_removeEventTriggers: function(triggerNode) {

			// Resolve node ID (if passed as parameter) and validate DOM node.
			if(triggerNode){
				triggerNode = dom.byId(triggerNode);
			}
      
			// Iterate through the array of bindings, unbind the appropriate ones,
			// and remove the binding records.
			for(var i = this._bindings.length - 1; i >= 0; i--){
				var binding = this._bindings[i];
				
				if(!triggerNode || (triggerNode === binding.triggerNode)){
					binding.unbindFunction();
					if(binding.iframeOnLoadHandler){
						if(binding.triggerNode.removeEventListener){
							binding.triggerNode.removeEventListener("load", binding.iframeOnLoadHandler, false);
						}else{
							binding.triggerNode.detachEvent("onload", binding.iframeOnLoadHandler);
						}
					}
					this._bindings.splice(i, 1);
				}
			}
		}
		
	});
});
	