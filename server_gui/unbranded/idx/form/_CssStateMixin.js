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
"dojo", 
"dijit/dijit", 
"dijit/_WidgetBase", 
"dojo/_base/array", 
"dojo/has", 
"dojo/ready", 
"dojo/on", 
"dojo/_base/window", 
"dijit/registry",
"dojo/dom", 
"dojo/dom-class"], 
function(dojo, dijit, widgetBase, array, has, ready, on, win, registry, dom, domClass){

var CssStateMixin =  dojo.declare("idx.form._CssStateMixin", [], {
	
	cssStateNodes: {},
	hovering: false,
	active: false,

	// stateNode
	//		The original dijit domNode (inner field widget)
	
	// oneuiBaseClass
	//		The original dijit baseClass (inner field widget)

	_applyAttributes: function(){
		widgetBase.prototype._applyAttributes.apply(this, arguments);

		// Monitoring changes to disabled, readonly, etc. state, and update CSS class of root node
		dojo.forEach(["disabled", "readOnly", "checked", "selected", "focused", "state", "hovering", "active", "required"], function(attr){
			this.watch(attr, dojo.hitch(this, "_setStateClass"));
		}, this);

		// Events on sub nodes within the widget
		for(var ap in this.cssStateNodes){
			this._trackMouseState(this[ap], this.cssStateNodes[ap]);
		}
		this._trackMouseState(this.stateNode, this.oneuiBaseClass);
		// Set state initially; there's probably no hover/active/focus state but widget might be
		// disabled/readonly/checked/selected so we want to set CSS classes for those conditions.
		this._setStateClass();
	},

	_cssMouseEvent: function(/*Event*/ event){
		// summary:
		//	Sets hovering and active properties depending on mouse state,
		//	which triggers _setStateClass() to set appropriate CSS classes for this.domNode.
		if(!this.disabled){
			switch(event.type){
				case "mouseover":
					this._set("hovering", true);
					this._set("active", this._mouseDown);
					break;
				case "mouseout":
					this._set("hovering", false);
					this._set("active", false);
					break;
				case "mousedown":
				case "touchstart":
					this._set("active", true);
					break;
				case "mouseup":
				case "touchend":
					this._set("active", false);
					break;
			}
		}
	},
	
	_setStateClass: function(){
		// Compute new set of classes
		var newStateClasses = this._getModifiedClasses(this.oneuiBaseClass);
		this._applyStateClass(this.stateNode, newStateClasses);
		newStateClasses = this._getModifiedClasses(this.baseClass);
		this._applyStateClass(this.domNode, newStateClasses);
	},
	
	_getModifiedClasses: function(/*String*/className){
		var clazz = className.split(" ");
		function multiply(modifier){
			clazz = clazz.concat(dojo.map(clazz, function(c){ return c+modifier; }), "dijit"+modifier);
		}

		if(!this.isLeftToRight()){
			// For RTL mode we need to set an addition class like dijitTextBoxRtl.
			multiply("Rtl");
		}

		var checkedState = this.checked == "mixed" ? "Mixed" : (this.checked ? "Checked" : "");
		if(this.checked){
			multiply(checkedState);
		}
		if(this.state){
			multiply(this.state);
		}
		if(this.selected){
			multiply("Selected");
		}
		if(this.required){
			multiply("Required");
		}
		if(this.disabled){
			multiply("Disabled");
		}else if(this.readOnly){
			multiply("ReadOnly");
		}else{
			if(this.active){
				multiply("Active");
			}else if(this.hovering){
				multiply("Hover");
			}
		}

		if(this.focused){
			multiply("Focused");
		}
		return clazz;
	},
	
	_applyStateClass: function(/*DomNode*/ node, /*Array*/classes){
		// Compute new set of classes
		// Remove old state classes and add new ones.
		// For performance concerns we only write into stateNode.className and domNode.className once.
		var classHash = {};	// set of all classes (state and otherwise) for node
		
		dojo.forEach(node.className.split(" "), function(c){ classHash[c] = true; });

		if("_stateClasses" in node){
			dojo.forEach(node._stateClasses, function(c){ delete classHash[c]; });
		}

		dojo.forEach(classes, function(c){ classHash[c] = true; });

		var newClasses = [];
		for(var c in classHash){
			newClasses.push(c);
		}
		node.className = newClasses.join(" ");
		node._stateClasses = classes;
	},
	
	_subnodeCssMouseEvent: function(node, clazz, evt){
		// summary:
		//		Handler for hover/active mouse event on widget's subnode
		if(this.disabled || this.readOnly){
			return;
		}
		var classSet = clazz.split(/\s+/);
		function hover(isHovering){
			array.forEach(classSet, function(clazz){
				domClass.toggle(node, clazz+"Hover", isHovering);
			})
		}
		function active(isActive){
			array.forEach(classSet, function(clazz){
				domClass.toggle(node, clazz+"Active", isActive);
			})
		}
		function focused(isFocused){
			array.forEach(classSet, function(clazz){
				domClass.toggle(node, clazz+"Focused", isFocused);
			})
		}
		switch(evt.type){
			case "mouseover":
				hover(true);
				break;
			case "mouseout":
				hover(false);
				active(false);
				break;
			case "mousedown":
			case "touchstart":
				active(true);
				break;
			case "mouseup":
			case "touchend":
				active(false);
				break;
			case "focus":
			case "focusin":
				focused(true);
				break;
			case "blur":
			case "focusout":
				focused(false);
				break;
		}
	},

	_trackMouseState: function(/*DomNode*/ node, /*String*/ clazz){
		// summary:
		//		Track mouse/focus events on specified node and set CSS class on that node to indicate
		//		current state.   Usually not called directly, but via cssStateNodes attribute.
		// description:
		//		Given class=foo, will set the following CSS class on the node
		//
		//		- fooActive: if the user is currently pressing down the mouse button while over the node
		//		- fooHover: if the user is hovering the mouse over the node, but not pressing down a button
		//		- fooFocus: if the node is focused
		//
		//		Note that it won't set any classes if the widget is disabled.
		// node: DomNode
		//		Should be a sub-node of the widget, not the top node (this.domNode), since the top node
		//		is handled specially and automatically just by mixing in this class.
		// clazz: String
		//		CSS class name (ex: dijitSliderUpArrow)

		// Flag for listener code below to call this._cssMouseEvent() or this._subnodeCssMouseEvent()
		// when node is hovered/active
		node._cssState = clazz;
	}
});
ready(function(){
	// Document level listener to catch hover etc. events on widget root nodes and subnodes.
	// Note that when the mouse is moved quickly, a single onmouseenter event could signal that multiple widgets
	// have been hovered or unhovered (try test_Accordion.html)
	function handler(evt){
		// Poor man's event propagation.  Don't propagate event to ancestors of evt.relatedTarget,
		// to avoid processing mouseout events moving from a widget's domNode to a descendant node;
		// such events shouldn't be interpreted as a mouseleave on the widget.
		if(!dom.isDescendant(evt.relatedTarget, evt.target)){
			for(var node = evt.target; node && node != evt.relatedTarget; node = node.parentNode){
				// Process any nodes with _cssState property.   They are generally widget root nodes,
				// but could also be sub-nodes within a widget
				if(node._cssState){
					var widget = registry.getEnclosingWidget(node);
					if(widget){
						if(node == widget.oneuiBaseNode){
							widget._cssMouseEvent(evt);
						}else{
							if(node == widget.domNode){
								// event on the widget's root node
								widget._cssMouseEvent(evt);
							}else{
								// event on widget's sub-node
								widget._subnodeCssMouseEvent(node, node._cssState, evt);
							}
						}
					}
				}
			}
		}
	}
	function ieHandler(evt){
		evt.target = evt.srcElement;
		handler(evt);
	}

	// Use addEventListener() (and attachEvent() on IE) to catch the relevant events even if other handlers
	// (on individual nodes) call evt.stopPropagation() or event.stopEvent().
	// Currently typematic.js is doing that, not sure why.
	// Don't monitor mouseover/mouseout on mobile because iOS generates "phantom" mouseover/mouseout events when
	// drag-scrolling, at the point in the viewport where the drag originated.   Test the Tree in api viewer.
	var body = win.body(),
		types = (has("touch") ? [] : ["mouseover", "mouseout"]).concat(["mousedown", "touchstart", "mouseup", "touchend"]);
	array.forEach(types, function(type){
		if(body.addEventListener){
			body.addEventListener(type, handler, true);	// W3C
		}else{
			body.attachEvent("on"+type, ieHandler);	// IE
		}
	});

	// Track focus events on widget sub-nodes that have been registered via _trackMouseState().
	// However, don't track focus events on the widget root nodes, because focus is tracked via the
	// focus manager (and it's not really tracking focus, but rather tracking that focus is on one of the widget's
	// nodes or a subwidget's node or a popup node, etc.)
	// Remove for 2.0 (if focus CSS needed, just use :focus pseudo-selector).
	on(body, "focusin, focusout", function(evt){
		var node = evt.target;
		if(node._cssState && !node.getAttribute("widgetId")){
			var widget = registry.getEnclosingWidget(node);
			widget._subnodeCssMouseEvent(node, node._cssState, evt);
		}
	});
});
return CssStateMixin;
});