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

define(["dojo/_base/declare",
		"dojo/_base/array",
		"dojo/_base/connect",
		"dojo/_base/event",
		"dojo/_base/lang",
		"dojo/_base/sniff",
		"dojo/_base/window",
		"dojo/aspect",
		"dojo/dom",
		"dojo/dom-class",
		"dojo/dom-geometry",
		"dojo/dom-style",
		"dojo/request/iframe",
		"dojo/keys",
		"dojo/window",
		"dijit/popup",
		"dijit/TooltipDialog",
		"./_EventTriggerMixin",
		"dojo/text!./templates/MenuDialog.html"],
		function(declare, array, connect, event, lang, has, win, aspect, dom, domClass, domGeometry, domStyle, iframe, keys, _window, popup, TooltipDialog, _EventTriggerMixin, template){
	var oneuiRoot = lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2
		
	function nodeUsesArrowKeys(/*DOMNode*/ node){
		// return true if the DOM node is one that uses arrow keys (like
		// a text entry field) but doesn't stop them propagating -- we
		// will ignore arrow keypresses that were originally targetted
		// at such nodes so as not to double-act when the arrow keys are
		// pressed.
		
		return (node.nodeName === "TEXTAREA")
		    || ((node.nodeName === "INPUT") && (node.type === "text"));
	}

	/**
	 * Creates a new idx.widget.MenuDialog
	 * @name idx.widget.MenuDialog
	 * @class The MenuDialog widget is the main container for rich mega-menu
	 * content. It extends dijit.TooltipDialog and adds the necessary 
	 * functionality to enable the resulting popup to act like a menu, and integrate
	 * with the rest of the existing dijit menu framework. This results in a 
	 * widget that 
	 * facilitates the creation of menus with arbitrary dialog content. 
	 * <p>Instances can be supplied as the "popup" parameter for 
	 * dijit.PopupMenuItem and dijit.PopupMenuBarItem, and will work
	 * "as expected". Instances can also operate as popup menus on arbitrary
	 * DOM nodes, or for the whole window.
	 * </p>
	 * <p>Instances can display with a "shark fin" connector to identify
	 * the initiating element or with a thinner border, no shark
	 * fin and abutting the initiating element.
	 * </p>
	 * <p>Instances behave as content panes, and HTML and dijits may be
	 * laid out within them. If a menu with "menuForDialog" set to true is
	 * included within the layout, the menu dialog will operate as a
	 * drop-down menu in combination with the contained	menu.
	 * </p>
	 * @augments dijit.TooltipDialog
	 * @augments idx.widget._EventTriggerMixin
	 * @example
	 * &lt;div data-dojo-type="idx.widget.MenuBar"&gt;
  &lt;div data-dojo-type="dijit.PopupMenuBarItem"&gt;
    &lt;span&gt;Menu #1&lt;/span&gt;
    &lt;div data-dojo-type="idx.widget.MenuDialog"&gt;</span>
      &lt;!-- Arbitrary mega menu content --&gt;
    &lt;/div&gt;
  &lt;/div&gt;
&lt;/div&gt;
	 */
	return oneuiRoot.MenuDialog = declare("idx.widget.MenuDialog", [TooltipDialog, _EventTriggerMixin],
	/** @lends idx.widget.MenuDialog.prototype */
	{
		/**
		 * CSS class applied to DOM node. Part of standard templated widget 
		 * behavior.
		 * @type string
		 * @constant
		 * @private
		 */
		baseClass: "oneuiMenuDialog",
		
		/**
		 * Flag to indicate whether we need to close ourselves in onBlur. This
		 * flag is only true while we are popped up from a manual open or
		 * bindDomNode trigger.
		 * @type boolean
		 * @private
		 */
		_closeOnBlur: false,
		
		/**
		 * If true, hover will trigger popup whenever the menu dialog is
		 * bound to a DOM node (via bindToDomNode). Otherwise, a mouse
		 * click (or tap on a touch interface) will be required. This setting
		 * does not affect keyboard alternative triggers, which will be
		 * active in the same way in all cases. This value can be overridden
		 * for individual calls to bindToDomNode by including a "hoverToOpen"
		 * property in the (optional) parameters object.
		 * @type boolean
		 */
		hoverToOpen: true,
		
		/**
		 * True if the menu dialog is currently showing, false otherwise. Part
		 * of the standard dijit menu logic.
		 * @type boolean
		 * @private
		 */
		isShowingNow: false,
		
		/**
		 * If true, a left mouse click (or tap on a touch interface) will
		 * trigger popup whenever the menu dialog is bound to a DOM node
		 * (via bindDomNode). Otherwise, a right mouse click will be
		 * required. Furthermore, when leftClickToOpen is true the keyboard
		 * alternative triggers become space-bar and enter key and the popup
		 * is automatically positioned relative to the triggering DOM node,
		 * whilst when leftClickToOpen is false the keyboard alternative
		 * trigger is Shift+F10 and the popup is positioned relative to the
		 * mouse position (when triggered by a mouse event, and relative to
		 * the triggering DOM node otherwise). This value can be overridden
		 * for individual calls to bindDomNode by including a
		 * "leftClickToOpen" property in the (optional) parameters object.
		 * @type boolean
		 */
		leftClickToOpen: false,
		
		/**
		 * Pointer to menu that displayed the MenuDialog. Part
		 * of the standard dijit menu logic.
		 * @type dijit.Widget
		 * @private
		 */
		parentMenu: null,

		/**
		 * Ordered list of positions to try placing the popup at whenever the
		 * menu dialog is bound to a DOM node (via bindToDomNode). The first
		 * position which allows the popup to be fully visible within the
		 * viewport will be used. Possible values are:
		 * <ul>
		 * <li>
		 * before: places drop down to the left of the anchor node/widget, or
		 * to the right in RTL mode
		 * </li>
		 * <li>
		 * after: places drop down to the right of the anchor node/widget, or
		 * to the right in RTL mode
		 * </li>
		 * <li>
		 * above: drop down goes above anchor node/widget
		 * </li>
		 * <li>
		 * above-alt: same as above except right sides aligned instead of left
		 * </li>
		 * <li>
		 * below: drop down goes below anchor node/widget
		 * </li>
		 * <li>
		 * below-alt: same as below except right sides aligned instead of left
		 * </li>
		 * </ul>
		 * This value can be overridden for individual calls to bindToDomNode
		 * by including a "popupPosition" property in the (optional)
		 * parameters object. If no value is provided (or null) a default
		 * sequence of positions (below, below-alt, above, above-alt) is used.
		 * @name popupPosition
		 * @memberOf idx.widget.MenuDialog.prototype
		 * @type string[]
		 */
		
		/**
		 * If true, when this menu dialog closes re-focus the element
		 * which had focus before it was opened.
		 * @type boolean
		 * @private
		 */
		refocus: true,
	
		/**
		 * An array of DOM node ids to which the menu dialog should be
		 * bound during widget initialisation. Note that this array is
		 * processed only once, during creation, and later changes that
		 * may be made will be ignored; also, the array is not updated
		 * if/when further explicit calls to bindDomNode/unbindDomNode are made.
		 * @name targetNodeIds
		 * @memberOf idx.widget.MenuDialog.prototype
		 * @type DOMNode[]
		 */
		targetNodeIds: [], // need to override this class-static value in postMixInProperties
		
		/**
	 	 * The template HTML for the widget.
		 * @constant
		 * @type string
		 * @private
		 * @default Loaded from idx/widget/templates/MenuDialog.html.
		 */
		templateString: template,
		
		/**
		 * If true, the menu will show a "shark-fin" connector linking the
		 * opened popup dialog to the initiating DOM element or menu item.
		 * This value can be overridden for individual calls to bindDomNode
		 * by including a "useConnector" property in the (optional)
		 * parameters object.
		 * @type boolean
		 */
		useConnector: false,

		postMixInProperties: function() {
			this.targetNodeIds = this.targetNodeIds || [];
			this.inherited(arguments);
		},
		
		/**
		 * Standard Widget lifecycle method.
		 * @private
		 */		
		postCreate: function(){
			this.inherited(arguments);
			
			// intercept and process certain navigational keystrokes
			var l = this.isLeftToRight();
			this._nextMenuKey = l ? keys.RIGHT_ARROW : keys.LEFT_ARROW;
			this._prevMenuKey = l ? keys.LEFT_ARROW : keys.RIGHT_ARROW;
			this.connect(this.domNode, "onkeypress", "_onDomNodeKeypress");
			
			// do any initial bindings to DOM nodes requested
			if(this.contextMenuForWindow){
				this.bindDomNode(win.body());
			}else{
				array.forEach(this.targetNodeIds, function(nodeid){
					this.bindDomNode(nodeid);
				}, this);
			}
		},
		
		/**
		 * Handle keypress events at the outer containing DOM node to process
		 * arrow-key navigation.
		 * @param {Event} evt
		 */
		_onDomNodeKeypress: function(evt){
			var target = evt.target || evt.srcElement,
				handled = false;

			if(this.parentMenu && !evt.ctrlKey && !evt.altKey && (!target || !nodeUsesArrowKeys(target))){
				switch(evt.charOrCode){
					case this._nextMenuKey:
						this.parentMenu._getTopMenu().focusNext();
						handled = true;
						break;
						
					case this._prevMenuKey:
						if(this.parentMenu._isMenuBar){
							this.parentMenu.focusPrev();
						}else{
							this.onCancel(false);
						}
						handled = true;
						break;
				}
			}
			
			if(handled){
				event.stop(evt);
			}else{
				this.inherited(arguments);
			}
		},		
		
		/**
		 * Return the menu for the dialog, or undefined if no menu for the
		 * dialog can be found.
		 */
		_getMenuForDialog: function(){
			var children = this.getChildren(),
				result;
				
			for(var i = 0; !result && (i < children.length); i++){
				if(children[i] && children[i].menuForDialog){
					result = children[i];
				}
			}

			return result;
		},
		
		focus: function(){
			// Override focus() method to have no effect if called when the
			// menu dialog already has focus. This is slightly more efficient,
			// but more importantly it avoids a current problem (in Dojo 1.8.0)
			// in which a superfluous second call to focus() when the default
			// focus should go to a menu places focus in another widget because
			// the KeyNavContainer behaviour of the menu has left it temporarily
			// unfocusable -- in Dojo 1.8.0 the Dijit Menubar makes such a
			// superfluous second call when down-arrow is used to transfer focus
			// to an already-open menu. Both of these issues have been reported
			// as Dojo defects via IDT.
			
			if(!this.focused){
				this.inherited(arguments);
			}
		},
		
		/**
		 * Finds focusable items in dialog,	and sets this._firstFocusItem and
		 * this._lastFocusItem.
		 */		
		_getFocusItems: function(){
			this.inherited(arguments);
			
			// if _firstFocusItem or _lastFocusItem have been set to our DOM node,
			// set them to the containerNode instead, which is a better focus target
			if(this._firstFocusItem == this.domNode){
				this._firstFocusItem = this.containerNode;
			}
			if(this._lastFocusItem == this.domNode){
				this._lastFocusItem = this.containerNode;
			}
		},
		
		/**
		 * Called when an event occurs that should result in the MenuDialog
		 * being shown.
		 * <p>
		 * Set timer to display myself. Using a timer rather than displaying
		 * immediately solves two problems:
		 * <ol>
		 * <li>
		 * IE: without the delay, focus work in "open" causes the system
		 * context menu to appear in spite of stopEvent.
		 * </li>
		 * <li>
		 * Avoid double-shows on linux, where shift-F10 generates an 
		 * oncontextmenu event even after a event.stop(e). (Shift-F10 on
		 * windows doesn't generate the oncontextmenu event.)
		 * </p>
		 * @private
		 * @param {Object} params An object containing data about the event. See
		 * {@link idx.widget._EventTriggerMixin#_onTrigger}
		 */
		_onTrigger: function(params){
			var coords = null;
			if(!params.additionalData.leftClickToOpen && ("pageX" in params.event)){
				coords = { x: params.event.pageX, y: params.event.pageY }; 			
				if (params.triggerNode.tagName === "IFRAME") {
					// Specified coordinates are on <body> node of an <iframe>, convert to match main document
					var ifc = domGeometry.position(params.triggerNode, true), scroll = win.withGlobal(_window.get(iframe.doc(params.triggerNode)), "docScroll", domGeometry);
					
					var cs = domStyle.getComputedStyle(params.triggerNode), tp = domStyle.toPixelValue, left = (has("ie") && has("quirks") ? 0 : tp(params.triggerNode, cs.paddingLeft)) + (has("ie") && has("quirks") ? tp(params.triggerNode, cs.borderLeftWidth) : 0), top = (has("ie") && has("quirks") ? 0 : tp(params.triggerNode, cs.paddingTop)) + (has("ie") && has("quirks") ? tp(params.triggerNode, cs.borderTopWidth) : 0);
					
					coords.x += ifc.x + left - scroll.x;
					coords.y += ifc.y + top - scroll.y;
				}
			}
	
			if(!this._openTimer){
				this._openTimer = setTimeout(lang.hitch(this, function(){
					delete this._openTimer;

					this.open({
						around: params.triggerNode,
						coords: coords,
						position: params.additionalData.popupPosition,
						useConnector: params.additionalData.useConnector
					});
				}), 1);
			}
			
			if(params.event.type != "hover")
				event.stop(params.event);
		},
	
		/**
		 * An attach point that is called when the MenuDialog loses focus. 
		 */
		onBlur: function(){
			this.inherited(arguments);

			if(this._closeOnBlur){
				this.close();
			}
		},

		/**
		 * Opens the MenuDialog at the specified position.
		 * @param {Object} args An object containing some the following fields,
		 * which are all optional but at least one of 'around' and 'coords'
		 * should be supplied:
		 * <ul>
		 * <li>
		 * 	around: A DOM node that the popup should be placed around.
		 * </li>
		 * <li>
		 * 	coords: A position (with fields x and y) containing coordinates
		 * 	that the popup should be placed at.
		 * </li>
		 * <li>
		 * 	position: Ordered list of positions to try placing the popup at.
		 * 	The first position which allows the popup to be fully visible
		 * 	within the viewport will be used, or the best available option.
		 * 	Possible values are:
		 * 	<ul>
		 * 		<li>
		 * 			before: places drop down to the left of the around
		 * 			node/coords, or to the right in RTL mode
		 * 		</li>
		 * 		<li>
		 *			after: places drop down to the right of the around
		 *			node/coords, or to the right in RTL mode
		 * 		</li>
		 * 		<li>
		 *			above: drop down goes above around node/coords
		 * 		</li>
		 * 		<li>
		 *			above-alt: same as above except right sides aligned
		 *			instead of left
		 * 		</li>
		 * 		<li>
		 *			below: drop down goes below around node/coords
		 * 		</li>
		 * 		<li>
		 *			below-alt: same as below except right sides aligned instead
		 *			of left
		 * 		</li>
		 * 		</ul>
		 *		If this field is omitted the current value of the widget
		 *		popupPosition property is used.
		 * </li>
		 * <li>
		 *		useConnector: If true, a shark-fin connector is shown linking
		 *		the popup to the around node/coords. If this field is omitted
		 *		the current value of the widget useConnector property is used.
		 * </li>
		 * </ul>
		 */		
		open: function(args){
			// store the node which focus should return to, if necessary, and if possible
			var refocusNode = null;
			if(this.refocus){
				refocusNode = this._focusManager.get("curNode");
				if(!refocusNode || dom.isDescendant(refocusNode, this.domNode)){
					refocusNode = this._focusManager.get("prevNode");
				}
				if(dom.isDescendant(refocusNode, this.domNode)){
					refocusNode = null;
				}
			}
			
			// use coords if supplied, otherwise use the around node if supplied,
			// or use current focus node, or (0,0) as a last resort
			var around = (args && (args.coords ? { x: args.coords.x, y: args.coords.y, w: 0, h: 0 } : args.around)) || refocusNode || this._focusManager.get("curNode") || { x: 0, y: 0, w: 0, h: 0 };
			//work around for popup.open(call "around.getBoundingClientRect" inside)
			if(!around.getBoundingClientRect){
				around.getBoundingClientRect = function(){return around};
			}
			
			var closeFunction = lang.hitch(this, function(){
				if(refocusNode){
					refocusNode.focus();
				}
				
				this.close();
			});
						
			this._useConnectorForPopup = (args && ("useConnector" in args)) ? args.useConnector : this.useConnector;
				
			popup.open({
				popup: this,
				around: around,
				onExecute: closeFunction,
				onCancel: closeFunction,
				orient: (args && ("position" in args)) ? args.position : this.popupPosition
			});
				
			delete this._useConnectorForPopup;
						
			this.focus();
			this._closeOnBlur = true;
		},
		
		/**
		 * Closes the MenuDialog. 
		 */
		close: function(){
			popup.close(this);
		},
		
		/**
		 * Attach the MenuDialog to the specified trigger node.
		 * @param {string|DOMNode} node The node to be used to trigger the
		 * MenuDialog to pop-up. 
		 * @param {Object} params An object that can contain properties that 
		 * override the popupPosition, useConnector, leftClickToOpen,
		 * and hoverToOpen properties of the 
		 * MenuDialog on a per node attachment basis.
		 */
		bindDomNode: function(/*String|DomNode*/ node, /*Object*/ params){
			var settings = lang.delegate(this);
			for(var name in params){
				settings[name] = params[name];
			}
			
			this._addEventTrigger(node, "click", function(fparams){
				return settings.leftClickToOpen;
			}, settings);
			
			this._addEventTrigger(node, "contextmenu", function(fparams){
				return !settings.leftClickToOpen;
			}, settings);
			
			this._addEventTrigger(node, "keydown", function(fparams){
				// if right-click is the trigger, accept Shift+F10 as an equivalent
				// in case the browser doesn't send us the contextmenu event
				return !settings.leftClickToOpen && fparams.event.shiftKey && (fparams.event.keyCode == keys.F10);
			}, settings);
			
			this._addEventTrigger(node, "hover", function(fparams){
				return settings.hoverToOpen;
			}, settings);
		},
	
		/**
		 * Detach menu dialog from given DOM node. If no node is specified,
		 * detach menu dialog from all bound DOM nodes.
		 * @param {string|DOMNode} nodeName The DOM node to dissociate the 
		 * MenuDialog from.
		 */
		unBindDomNode: function(/*String|DomNode*/ nodeName){
			this._removeEventTriggers(nodeName);
		},
		
		/**
		 * Configure the widget to display in a given position. This may be
		 * called several times during popup in order to "try out" different
		 * configurations and then select the best. It will always be called
		 * one final time with the actual configuration to be used.
		 * @param {Object} aroundNodeSize
		 * @param {string} aroundCorner
		 * @param {string} corner
		 * @param {boolean} useConnector
		 */
		_layoutNodes: function(/*Object*/ aroundNodeSize, /*String*/ aroundCorner, /*String*/ corner, /*Boolean*/ useConnector){
			var newcss = useConnector ? "oneuiMenuDialogConnected" : "",
				newparentcss = "",
				cornerv = corner && (corner.length >= 1) && corner.charAt(0),
				cornerh = corner && (corner.length >= 2) && corner.charAt(1),
				mybox = domGeometry.getContentBox(this.domNode),
				displacementProperty, displacementValue,
				getContentBox = function(node){
					var style = node.style,
						oldDisplay = style.display,
						oldVis = style.visibility;
					if(style.display == "none"){
						style.visibility = "hidden";
						style.display = "";
					}
					var result = domGeometry.getContentBox(node);
					style.display = oldDisplay;
					style.visibility = oldVis;
					return result;
				}
			
			// we use a combination of two classes to enable the CSS markup to be applied
			// to position our connector correctly.
			//
			// dijitTooltipAbove, dijitTooltipBelow, dijitTooltipLeft, dijitTooltipRight,
			// indicating which way the popup goes from the position or around node
			//
			// When dijitTooltipAbove or dijitTooltipBelow are used, one of the classes
			// dijitTooltipABLeft, dijitTooltipABRight, dijitTooltipABMiddle is also
			// applied to indicate where on the top/bottom edge the connector should appear.    
			//
			// When dijitTooltipLeft or dijitTooltipRight are used, one of the classes
			// dijitTooltipLRTop, dijitTooltipLRBottom, dijitTooltipLRMiddle is also
			// applied to indicate where on the left/right edge the connector should appear.			
			 
			if( (cornerv === 'M')  // 'ML' or 'MR'
			 || ((cornerh !== 'M') && (cornerh !== aroundCorner.charAt(1)))){  // '?L' against '?R' means a left/right abuttal
				// the popup is going left or right
				newcss += " dijitTooltip" + (cornerh === 'L' ? "Right" : "Left");
				
				switch(cornerv){
					case 'M':
						newcss += " dijitTooltipLRMiddle";
						break;
					case 'T':
						newcss += " dijitTooltipLRTop";
						useConnector && (newparentcss = "connectorNearTopEdge");
						if(aroundNodeSize.h > 0){
							displacementProperty = "top";
							if(this.isLeftToRight()){
							displacementValue = Math.max(4, 4 + Math.min(getContentBox(this.domNode.parentNode).h - 24, aroundNodeSize.h / 2)) + "px"; 
							}else{
								displacementValue = (4 + Math.min(getContentBox(this.domNode.parentNode).h - 24, aroundNodeSize.h / 2)) + "px";
							}
						}
						break;
					case 'B':
						newcss += " dijitTooltipLRBottom";
						useConnector && (newparentcss = "connectorNearBottomEdge");
						if(aroundNodeSize.h > 0){
							displacementProperty = "bottom";
							if(this.isLeftToRight()){
							displacementValue = (4 + Math.min(getContentBox(this.domNode.parentNode).h - 24, aroundNodeSize.h / 2)) + "px"; 
							}else{
								displacementValue = Math.max(4, 4 + Math.min(getContentBox(this.domNode.parentNode).h - 24, aroundNodeSize.h / 2)) + "px";
							}
						}
						break;
				}
			}else{  // 'TM' or 'BM', or '?L' against '?L' or '?R' against '?R' which both mean an above/below abuttal
				// the popup is going above or below
				newcss += " dijitTooltip" + (cornerv === 'T' ? "Below" : "Above");
				
				switch(cornerh){
					case 'M':
						newcss += " dijitTooltipABMiddle";
						break;
					case 'L':
						newcss += " dijitTooltipABLeft";
						useConnector && (newparentcss = "connectorNearLeftEdge");
						if(aroundNodeSize.w > 0){
							displacementProperty = "left";
							if(this.isLeftToRight()){
							displacementValue = Math.max(4, 4 + Math.min(getContentBox(this.domNode.parentNode).w - 16, aroundNodeSize.w / 2)) + "px"; 
							}else{
								displacementValue = (4 + Math.min(getContentBox(this.domNode.parentNode).w - 24, aroundNodeSize.w / 2)) + "px";
							}
						}
						break;
					case 'R':
						newcss += " dijitTooltipABRight";
						useConnector && (newparentcss = "connectorNearRightEdge");
						if(aroundNodeSize.h > 0){
							displacementProperty = "right";
							if(this.isLeftToRight()){
							displacementValue = (4 + Math.min(getContentBox(this.domNode.parentNode).w - 24, aroundNodeSize.w / 2)) + "px"; 
							}else{
								displacementValue = Math.max(4, 4 + Math.min(getContentBox(this.domNode.parentNode).w - 16, aroundNodeSize.w / 2)) + "px";
							}
						}
						break;
				}
			}
			
			domClass.replace(this.domNode, newcss, this._currentOrientClass || "");
			this._currentOrientClass = newcss;
			
			domClass.replace(this.domNode.parentNode, newparentcss, this._currentConnectorClass || "");
			this._currentConnectorClass = newparentcss;
			
			// remove any previous displacement values that might have been applied
			this.connectorNode.style.top = "";
			this.connectorNode.style.bottom = "";
			this.connectorNode.style.left = "";
			this.connectorNode.style.right = "";
			
			if(displacementProperty){
				this.connectorNode.style[displacementProperty] = displacementValue;
			}
		},
		
		/**
		 * Configure widget to be displayed in given position relative to the
		 * trigger node. This is called from the dijit.popup code, and should
		 * not be called directly.
		 * @private
		 * @param {DOMNode} node
		 * @param {string} aroundCorner
		 * @param {string} corner
		 * @param {Object} spaceAvailable
		 * @param {Object} aroundNodeCoords
		 */
		orient: function(/*DomNode*/ node, /*String*/ aroundCorner, /*String*/ corner, spaceAvailable, aroundNodeCoords){
			this._layoutNodes(aroundNodeCoords, aroundCorner, corner, ("_useConnectorForPopup" in this) ? this._useConnectorForPopup : this.useConnector);
		},

		/**
		 * An attach point that is called when the MenuDialog is opened. 
		 */
		onOpen: function(pos){
			this.isShowingNow = true;
			
			// set up our initial display
			this._layoutNodes(pos.aroundNodePos, pos.aroundCorner, pos.corner, ("_useConnectorForPopup" in this) ? this._useConnectorForPopup : this.useConnector);
			this.reset();
			
			// if there is a menu we are to work with, connect to it now
			var menu = this._getMenuForDialog();
			if(menu){
				// ensure that our menu has parentMenu set to our parent
				// so that focus chains connect up correctly
				if(this._menuparented){
					// this shouldn't happen, but handle it cleanly if it does
					this._menuparented.parentMenu = null;
				}
				menu.parentMenu = this.parentMenu;
				this._menuparented = menu;
				
				// ensure that "execute" events on our menu trigger "execute"
				// events on us, so that the notification passes up the menu
				// cascade chain correctly
				if(this._handleexecute){
					// this shouldn't happen, but handle it cleanly if it does
					this._handleexecute.remove();
				}
				var fireexecute = lang.hitch(this, this.onExecute);
				this._handleexecute = menu.on("execute", fireexecute);
				
				// ensure that "execute" events returning from popup/cascade
				// menus opened in turn by our menu also trigger "execute"
				// events on us. The "execute" events will otherwise terminate
				// there, because our menu will not be in the popup stack; all
				// the child menus cascading from it will be correctly folded
				// away, but we need to pass the notification upwards so that
				// the next segment of the popup stack can also be folded away.
				if(this._handleopen){
					// this shouldn't happen, but handle it cleanly if it does
					this._handleopen.remove();
				}
				this._handleopen = aspect.after(menu, "_openPopup", function(){
					// after _openPopup is called, retrieve the popup stack item just
					// added and hook into its onExecute() method, which will be called
					// when an execute is triggered on a descendant menu item
					var stackitem = popup._stack[popup._stack.length - 1];
					if(!stackitem._menuregistered){
						stackitem._menuregistered = true;
						stackitem.handlers.push(aspect.around(stackitem, "onExecute", function(originalOnExecute){
							return function(){
								// during the default onExecute handling, all the handlers
								// will be removed, so we can't use an 'after'. Instead,
								// we use 'around', call the original, then fire our own
								// 'execute' event. 
								originalOnExecute.apply(this, arguments);
								fireexecute();							
							}
						}));
					}
				}, true);
			}
			
			this._onShow(); // lazy load trigger
		},

		/**
		 * An attach point that is called when the MenuDialog is closed. 
		 */
		onClose: function(){
			this.isShowingNow = false;
			this._closeOnBlur = false;
			
			// if we connected to a menu, disconnect it now
			if(this._handleexecute){
				this._handleexecute.remove();
				this._handleexecute = null;
			}
			if(this._handleopen){
				this._handleopen.remove();
				this._handleopen= null;
			}
			
			// if we set the parent of a menu, reset it now
			if(this._menuparented){
				this._menuparented.parentMenu = null;
				this._menuparented = null;
			}
			
			this.onHide();
		},

		/**
		 * An attach point for the parent menu to listen for the MenuDialog being
		 * cancelled. Calling it will cause this MenuDialog to be closed, 
		 * but not any menus it may have cascaded from.
		 * @name idx.widget.MenuDialog.prototype.onCancel
		 * @function
		 * @public
		 */		

		/**
		 * An attach point for the parent menu to listen for menu items being
		 * executed. Calling it will cause this MenuDialog and any menus it
		 * may have cascaded from to be closed.
		 */		
		onExecute: function(){
		}

	});
});
