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

define(["dojo/_base/lang",
        "dojo/_base/array",
		"dojo/_base/declare",
		"dojo/_base/event",
		"dojo/dom-geometry",
		"dijit/_TemplatedMixin",
		"dijit/_WidgetBase",
		"dijit/Menu",
		"dijit/MenuItem",
		"dijit/registry",
		"./_MenuOpenOnHoverMixin",
		"dojo/text!./templates/Menu.html",
		"dojo/text!./templates/_MenuColumn.html",
		"idx/widgets"],
		function(lang,
				array,
				 declare,
				 event,
				 domgeometry,
				 _TemplatedMixin,
				 _WidgetBase,
				 Menu,
				 MenuItem,
				 registry,
				 _MenuOpenOnHoverMixin,
				 template,
				 columntemplate){
				 	
	var icons = {
		"error":        "oneuiErrorMenuItemIcon",
		"warning":      "oneuiWarningMenuItemIcon",
		"confirmation": "oneuiConfirmationMenuItemIcon",
		"information":  "oneuiInformationMenuItemIcon",
		"success":      "oneuiSuccessMenuItemIcon",
		"critical":     "oneuiCriticalMenuItemIcon",
		"attention":    "oneuiAttentionMenuItemIcon",
		"compliance":   "oneuiComplianceMenuItemIcon"
	}

	/**
	 * Creates a new idx.widget.Menu
	 * @name idx.widget.Menu
	 * @class The Menu widget is a substantial extension of dijit.Menu that adds
	 * 3 sets of functionality:
	 * <ol>
	 *   <li>
	 *     It facilitates event routing and functional linkage when used inside
	 *     an idx.widget.MenuDialog (This is a key part of the infrastructure
	 *     for implementing "mega menus")
	 *   </li>
	 *   <li>
	 *     It provides cascade-on-hover behaviour when the menu is used "flat"
	 *     (one important use of which is when one is placed inside an 
	 *     idx.widget.MenuDialog as part of a "mega menu")
	 *   </li>
	 *   <li>
	 *     It provides multi-column menu support including keyboard navigation
	 *   </li>
	 * </ol>
	 * Although the original motivation for adding these functional extensions
	 * to dijit.Menu was to support the creation of "mega menus", items (2) and (3) 
	 * could have broader applications.
	 * <p>
	 * Instances can be supplied as the "popup" parameter for dijit.PopupMenuItem
	 * and dijit.PopupMenuBarItem, and will operate as dijit.Menu. Instances can
	 * operate as popup menus on arbitrary DOM nodes, or for the whole window,
	 * and will operate as dijit.Menu. Instances can be placed "flat" within a
	 * layout in the UI, and will operate as dijit.Menu except that
	 * cascade-on-hover behaviour is available (controlled by the openOnHover
	 * property). Instances can be placed "flat" within an idx.widget.MenuDialog,
	 * and can operate as a drop-down menu in combination with the containing
	 * dialog (controlled by the menuForDialog property).
	 * </p>
	 * <p>
	 * When menu items are added to this widget, the "column" property on each
	 * item can be used to specify the column in which the item should be placed.
	 * Columns will be created as needed to accommodate each new item, and can be
	 * styled by CSS.
	 * </p>
	 * @augments dijit.Menu
	 * @augments idx.widget._MenuOpenOnHoverMixin
	 * @borrows idx.widget._MenuOpenOnHoverMixin#openOnHover as this.openOnHover
	 * @example
	 * &lt;div data-dojo-type="idx.widget.MenuDialog"&gt;
  &lt;div data-dojo-type="idx.widget.Menu"&gt;
    &lt;div data-dojo-type="dijit.MenuItem" onclick="..."&gt;
      ...
    &lt;/div&gt;
      ...
  &lt;/div&gt;</span>
    ...
&lt;/div&gt;
	 */
	var Menu = declare("idx.widget.Menu", [Menu, _MenuOpenOnHoverMixin], 
	/** @lends idx.widget.Menu.prototype */
	{
		/**
		 * Provide an IDX base class to distinguish from Dijit menus in CSS selectors. 
		 */
		idxBaseClass: "idxMenu",
		
		/**
		 * An array of the DOM nodes (tbody's), one per column, that
		 * contain the items that are in each column.
		 * @type DOMNode[]
		 * @private
		 */		
		_containerNodes: null,
		
		/**
		 * An array of the DOM nodes that can be used for styling individual
		 * columns in a multi-column menu. The nodes are ('td's), one per column,
		 * which are in a table row that implements the multiple columns, 
		 * and each of which contains
		 * a single column table containing the menu items. The column nodes
		 * are higher up in the DOM structure than the container nodes, but
		 * have a 1 to 1 relationship. They are externalised as they are more 
		 * suitable for styling the columns than the container nodes.
		 * @type DOMNode[]
		 */ 
		columnNodes: null,
		
		/**
		 * Indicates if this menu should be used by an idx.widget.MenuDialog it
		 * is in to provide the primary menu functionality.
		 * @type boolean 
		 */
		menuForDialog: true,
		
		/**
	 	 * The template HTML for the widget.
		 * @constant
		 * @type string
		 * @private
		 * @default Loaded from idx/widget/templates/Menu.html.
		 */
		templateString: template,
		
		/**
		 * Constructor.
		 * @private
		 */
		constructor: function(){
			this._containerNodes = [];
			this.columnNodes = [];
		},
		
		// define a child selector that finds the menu items in any of our columns
		childSelector: function(/*DOMNode*/ node){
			var widget = registry.byNode(node);
			return (array.indexOf(this._containerNodes, node.parentNode) >= 0) && widget && widget.focus;
		},

		/**
		 * Returns the next or previous focusable child, compared to "child".
		 * Overridden because original does not work if the child widgets are
		 * not immediate peers in the DOM.
		 * <p>THIS IMPLEMENTATION ASSUMES THAT getChildren() RETURNS THE
		 * CHILDREN IN THE CORRECT ORDER FOR NAVIGATION. THIS IS TRUE
		 * AS OF DOJO V1.7</p>
		 * @param {dijit._Widget} child The current widget.
		 * @param {number} dir Integer: 1 = after, -1 = before.
		 */
		_getNextFocusableChild: function(child, dir){
			var nextFocusableChild = null;
			var children = this.getChildren();
			var startIndex;
			if(child != null){
				startIndex = array.indexOf(children, child);
				if (startIndex != -1) {
					startIndex += dir;
					if(startIndex < 0) 
						startIndex = children.length - 1;
					if(startIndex >= children.length) 
						startIndex = 0;
				}
			}
			else if(children.length == 0) 
				startIndex = -1;
			else 
				startIndex = (dir == 1) ? 0 : children.length - 1;
			
			if(startIndex != -1){
				// If there are children and the specifed start child if any
				// is one of them. Then iterate through the array of children
				// starting at the start position plus/minus. If either end of 
				// the array is hit then loop to the other end and continue
				// until a focusable child is found or the start position is
				// reached.
				var i = startIndex;
				do{
					if (children[i].isFocusable()) {
						nextFocusableChild = children[i];
						break;
					}
					i += dir;
					if(i < 0)
						i = children.length - 1;
					if(i >= children.length)
						i = 0;
				}while(i != startIndex);
			}
			
			return nextFocusableChild;
		},
		
		/**
		 * Finds the closest column in the specified direction with one or
		 * more focusable menu items in it and moves focus to the one level
		 * with the top of the currently focused menu item, or next higher or
		 * lower if that is not possible.
		 * @param {number} dir Direction - indicates whether to move focus to
		 * the next (+1) or previous (-1) column.
		 * @returns true if focus sucessfully moved, else false.
		 */
		_moveToColumn: function(dir){
			
			// Identify the column containing the currently focused menu
			// item and determine its y-position.
			if(this.focusedChild){
				for(var i = 0; i < this._containerNodes.length; i++){
					if(this.focusedChild.domNode.parentNode == this._containerNodes[i]){
						var focusedCol = i, yPos = domgeometry.getMarginBox(this.focusedChild.domNode).t;
						break;
					}
				}
			}
			if(focusedCol != undefined){
				// Try to locate the closest column in the appropriate direction
				// with at least one focusable item in it.
				for (i = focusedCol + dir; i >= 0 && i < this._containerNodes.length; i += dir) {
					var children = registry.findWidgets(this._containerNodes[i]);
					var focusableChildren = dojo.filter(children, function(child){ return child.isFocusable() })
					if(focusableChildren.length > 0){
						var targetColumn = i;
						break;
					}
				}
				if(targetColumn != undefined){
					// Iterate through the focusable children and transfer 
					// focus to the item level with the currently focused
					// one, or the next higher, or the next lower, in that
					// order.
					for (i = 0; i < focusableChildren.length; i++){
						var child = focusableChildren[i];
						var childBox = domgeometry.getMarginBox(child.domNode);
						if(yPos >= childBox.t && yPos <= childBox.t + childBox.h - 1){
							this.focusChild(child);
							return true;
						} 
						else if(yPos < childBox.t){
							if(i > 0){
								this.focusChild(focusableChildren[i - 1]);
								return true;
							}else{
								this.focusChild(child);
								return true;
							}
						}else if(i == focusableChildren.length - 1){
							this.focusChild(child);
							return true;
						} 
					}
				}
			}
			
			// If the method didn't get to the point of focusing another menu
			// item then it failed.
			return false;
		},

		/**
		 * We replace _onKeyPress because our base class does NOT stop a
		 * close-sub-menu-key event even when it has handled the event
		 * (by cancelling itself or passing the navigation request to a
		 * parent menu bar). Curiously, it DOES stop the keypress event
		 * when it DOESN'T handle it, and this seems exactly the wrong way
		 * around. If the base class key handling changes so that handled
		 * keypress are also stopped, this override version can be deleted.
		 * @param {Event} evt
		 */		
		_onKeyPress: function(evt){

			if(evt.ctrlKey || evt.altKey){ return; }

			switch(evt.charOrCode){
				case this._openSubMenuKey:
					if (!this._moveToColumn(+1)) {
						this._moveToPopup(evt);
					}
					event.stop(evt);
					break;
				case this._closeSubMenuKey:
					if(!this._moveToColumn(-1)){
						if(this.parentMenu){
							if(this.parentMenu._isMenuBar){
								this.parentMenu.focusPrev();
							}else{
								this.onCancel(false);
							}
						}
					}				
					event.stop(evt);
					break;
			}
		},
		
		/**
		 * Refresh the layout of the menu items in the columns.
		 */
		refresh: function(){

			// Make sure that all child widgets are in the correct columns.
			var children = this.getChildren();
			for(var i = 0; i < children.length; i++){
				this.addChild(children[i]);
			}
		},		

		/**
		 * Standard widget lifecycle buildRendering() method.
		 * @private
		 */		
		buildRendering: function(){

			this.inherited(arguments);
			
			// When the widget is created the container node is associated
			// with column 0 (by the template), so that menu items are 
			// inserted into column 0 by the default template logic. The
			// container node now needs to be moved up the DOM tree so that 
			// it covers all the columns of menu items. (The menu items that
			// were initially loaded into column 0 are moved to other columns
			// in startup(). )
			this.containerNode = this._columnContainerNode;
		},
		
		/**
		 * Standard widget lifecycle startup() method.
		 * @private
		 */		
		startup: function(){

			if(this._started){ return; }
			this._started = true;
			this.inherited(arguments);
			
			// Move the menu items that were initially loaded into column 0 
			// by the default template logic to other columns, if necessary.
			this.refresh();
		},
		
		/**
		 * Override of standard container addChild() method, to allocate
		 * menuitems and separators to specified columns. 
		 * @param {dijit._Widget} widget Menu item, separator or heading to be
		 * added to the menu.
		 * @param {number} insertIndex Optional position to insert the item
		 * into the column in.
		 */
		addChild: function(widget, insertIndex){

			// Create new columns for the menu as necessary, if the item is
			// to be placed in a column that does not exist yet.
			while(this._containerNodes.length <= (widget.column || 0)){
				var node = _TemplatedMixin.getCachedTemplate(columntemplate).cloneNode(true);
				this._attachTemplateNodes(node, function(n,p){ return n.getAttribute(p); });
				this._columnContainerNode.appendChild(node);			
			}
			
			// Add the item to the column by temporarily making the column 
			// node the container node for the widget and then invoking the
			// standard, inherited dijit._Container functionality.
			this.containerNode = this._containerNodes[widget.column || 0];		
			this.inherited(arguments);
			this.containerNode = this._columnContainerNode;
		}
	});
	
	/**
	 * Create a menu item that represents a single message and which
	 * can be inserted into One UI message menus (menus which have the
	 * additional CSS class "oneuiMessageMenu").
	 * @name createMessageMenuItem
	 * @function
	 * @memberOf idx.widget.Menu
	 * @param {Object} args An object containing some the following fields,
	 * which are all optional:
	 * <ul>
	 * 	<li>
	 * 	  type: {string}
	 * 		The type of message. This can be "error", "warning",
	 *          "information", or "success".
	 *  </li>
	 * 	<li>
	 * 	  content: {string}
	 * 		The message content.
	 *  </li>
	 * 	<li>
	 * 	  messageId: {string}
	 * 	    An identifier for the message, displayed alongside the
	 *          content.
	 *  </li>
	 * 	<li>
	 * 	  timestamp: {string}
	 * 	    The date/time that the message was originated, displayed
	 *          alongside the content.
	 *  </li>
	 * </ul>
	 * @example var menuItem = Menu.createMessageMenuItem({
     *	type: "error",    // or "warning", "information" or "success"
     *	content: "Hello, world!",
     *	timestamp: locale.format(new Date(), { formatLength: "medium", 
     *	                                       locale: this.lang }),
     *	messageId: "CAT123456"
     *});
	 */
	Menu.createMessageMenuItem = function(args){

		var label = "";
		
		if(args){
			if(args.timestamp){
				label += '\u200f<span class="messageMenuTimestamp messagesContrast">\u200e' + args.timestamp + '\u200f</span>\u200e';
			}
			
			if(args.content){
				label += '\u200f <span class="messageTitles">\u200e' + args.content + '\u200f</span>\u200e';
			}
			
			if(args.messageId){
				label += '\u200f <span class="messagesContrast">(\u200e' + args.messageId + '\u200f)</span>\u200e';
			}
		}

		return new MenuItem({ label: label, iconClass: args && args.type && icons[args.type] });
	}
	
	var oneuiRoot = lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2
	oneuiRoot.Menu = Menu;
	
	return Menu;
});