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

/**
* @name idx.widget.HoverCard
* @class HoverCard provides pop-up information that displays when users hover the mouse pointer over an help indicator.  
* HoverCard is implemented according to standard and specification of IBM One UI(tm) 
* <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y21&vsub=*&hsub=*&openpanes=1111111111">Hover Preview Card</a></b>, 
* <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y44&vsub=*&hsub=*&openpanes=1111111111">Hover Person Card</a></b>
* @augments dijit.TooltipDialog
* @example
* 
&lt;!--Preview content of HoverCard--&gt;
&lt;div data-dojo-type="idx.widget._Preview" data-dojo-props='
	id: "preview",
	title: "Preview Card title",
	image: "../../themes/oneui/idx/images/objectImagePlaceholder90px.png",
	content: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam et purus lorem, eu semper massa. Phasellus rutrum, dui non ultrices convallis, nunc nunc dignissim neque, elementum imperdiet sapien massa ut risus&lt;br /&gt;&lt;br /&gt;"
'&gt;
&lt;/div&gt;
&lt;!--Declaration of Hover Preview Card--&gt;
&lt;div data-dojo-type="idx.widget.HoverCard" 
	data-dojo-props='
		target:"anchor", 
		moreActions:[
			{label: "Item 1", onClick: fClick},
			{label: "Item 2", onClick: fClick},
			{label: "Item 3", onClick: fClick}
		],
		actions: [
			{iconClass: "placeHolderIcon", onClick: fClick, text:""},
			{iconClass: "placeHolderIcon", onClick: fClick, text:""},
			{iconClass: "placeHolderIcon", onClick: fClick, text:""}
		],
		content: "preview"
	'&gt;
&lt;/div&gt;
* <br>
* @example 
&lt;!--Declaration of Hover Person Card--&gt;
&lt;div dojoType="idx.widget.PersonCard" id="personInfo" query="{email: 'someone@some.company.com'}"
url="https://w3-connections.ibm.com/profiles/json/profile.do" jsonp="callback"
spec="photo,fn,title,org,adr.work,tel.work,email.internet,sametime.awareness"
placeHolder="Sametime is not available on your client."&gt;&lt;/div&gt;

* @see dijit.TooltipDialog
**/
define([
	"dojo/_base/declare",
	"dojo/_base/array",
	"dojo/keys", // keys
	"dijit/focus", 
	"dijit/a11y",
	"dojo/_base/event", // event.stop
	"dojo/_base/fx", // fx.fadeIn fx.fadeOut
	"dojo/_base/lang", // lang.hitch lang.isArrayLike
	"dojo/_base/html",
	"dojo/dom-geometry", // domGeometry.getMarginBox domGeometry.position
	"dijit/place",
	"dojo/dom",
	"dojo/dom-style", // domStyle.set, domStyle.get
	"dojo/dom-class",
	"dojo/dom-attr",
	"dojo/dom-construct",
	"dojo/dnd/Moveable", // Moveable
	"dojo/dnd/TimedMoveable", // TimedMoveable
	"dojo/_base/window", // win.body()
	"dojo/_base/connect",
	"dojo/_base/sniff", // has("ie")
	"dojo/topic", // publish
	"dijit/_base/wai",
	"dijit/_base/manager",	// manager.defaultDuration
	"dijit/popup",
	"dijit/focus",
	"dijit/BackgroundIframe",
	"dijit/TooltipDialog",
	"idx/widget/Menu",
	"dijit/MenuItem",
	"dijit/form/DropDownButton",
	"dijit/form/Button",
	"dojo/text!./templates/HoverCard.html",
	"dijit/registry",
	"idx/form/_FocusManager",
	"idx/util",
	"dojo/i18n!./nls/HoverCard",
	"dojo/i18n!./nls/Dialog"
], function(declare, array, keys, dijitfocus, a11y, event, fx, lang, html, domGeometry, place, dom, domStyle, domClass,domAttr, domConstruct,
	Moveable, TimedMoveable, win, connect, has, topic, wai, manager, popup, focus, BackgroundIframe, TooltipDialog, Menu, MenuItem, DropDownButton, Button, 
	template, registry, focusMgr, iUtil, hoverCardNls, dialogNls){
	
	var oneuiRoot = lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2

	var HoverCard = declare("idx.widget.HoverCard", [TooltipDialog], {
	/** @lends idx.widget.HoverCard.prototype */
		templateString: template,
		
		/**
		* target: String|HTML DOM
		*		Id of domNode or domNode to attach the HoverCard to.
		*		When user hovers over specified dom node, the HoverCard will appear.
		*		@type string
		**/	
		target: "",
		
		/**
		* draggable: Boolean
		*		Toggles the moveable aspect of the HoverCard. If true, HoverCard
		*		can be dragged by it's grippy bar. If false it will remain positioned 
		*		relative to the attached node
		*		@type boolean
		**/		
		draggable: true,
		
		/**
		 * The delay in milliseconds before the HoverCard displays
		 * @type integer
		 */
		showDelay: 500,		
		
		/**
		 * The delay in milliseconds before the HoverCard hides
		 * @type integer
		 */
		hideDelay: 800,	
		/**
		 * The items of "more action" menu at the bottom right of the HoverCard
		 * @type array
		*/
		moreActions: null,
		/**
		 * The items of "actions" listed at bottom left of HoverCard
		 * @type array
		 */
		actions: null,
		
		content: null,
		
		forceFocus: true,
		/**
		 * List of positions to try to position HoverCard, ex: ["after-centered", "before-centered", "below", "above"];
		 * @type array
		 */
		position: null,
		
		/**
		 * duration: Integer
		 * Milliseconds to fade in/fade out
		 * @type integer
		 */
		duration: manager.defaultDuration,
		
		postMixInProperties: function(){
			this.moreActionsLabel = hoverCardNls.moreActionsLabel;
			this.buttonClose = dialogNls.closeButtonLabel;
		},
		/**
		 * destroy connection of "_targetConnections"
		 * and wrapper node associated with the widget
		 */
		destroy: function() {
			if (this._targetConnections) {
				for (var index = 0; index < this._targetConnections.length; index++) {
					connect.disconnect(this._targetConnections[index]);
				}
				delete this._targetConnections;
			}			
			this.inherited(arguments);		
			if (this._popupWrapper) {    
				domConstruct.destroy(this._popupWrapper);  
			}
		},
		
		_setTargetAttr: function(/*String*/ target){
			// summary:
			//		Connect to specified node(s)
			var target = dom.byId(target);
			if(target == this.target) {return;}
			
			// disconnect previous connections
			if (this._targetConnections) {
				for (var index = 0; index < this._targetConnections.length; index++) {
					connect.disconnect(this._targetConnections[index]);
				}
				delete this._targetConnections;
			}
			// Make connections
			this._targetConnections = [
				connect.connect(target, "onmouseenter", this, "_onHover"),
				connect.connect(target, "onmouseleave", this, "_onUnHover"),				
				connect.connect(target, "onkeypress", this, "_onTargetNodeKey")
			];

			this._set("target", target);
			this._createWrapper();
		},
		_onTargetNodeKey: function(/*Event*/evt){
            // summary:
            //		Handler for keyboard events
            // description:
            // tags:
            //		private
            var node = evt.target;
            
            if (evt.charOrCode == keys.ENTER || evt.charOrCode == keys.SPACE || evt.charOrCode == " ") {
                // Use setTimeout to avoid crash on IE, see #10396.
                 this._showTimer = setTimeout(lang.hitch(this, function(){
                    this.open(node)
                }), this.showDelay);
                event.stop(evt);     
            }
    	},
		
		_setActionsAttr: function(actions){
			array.forEach(actions, function(action){
				var button = new Button({
					iconClass:action.iconClass, 
					title: action.text,
					label: action.text,
					showLabel: false,
					onClick: action.content ? lang.hitch(this, function(){
						//add content to footer expand
					}) : action.onClick,
					baseClass: "idxOneuiHoverCardFooterButton"})
				html.place(button.domNode, this.actionIcons);
			}, this);
		},
		
		_setMoreActionsAttr: function(actions){
			var menu = new Menu({});
			array.forEach(actions, function(action){
				menu.addChild(new MenuItem({label:action.label, onClick:action.onClick}));
			});
			menu.startup();
			var button = new DropDownButton({
				label: this.moreActionsLabel, 
				dropDown: menu,
				baseClass: "idxOneuiHoverCardMenu"}, this.moreActionsNode);
			this.moreActionsMenu = menu;
		},	
	
		_setContentAttr: function(content){
			var innerWidget = registry.byId(content);
			if(!innerWidget.declaredClass){
				this.inherited(arguments);
			}else{
				html.place(innerWidget.domNode, this.containerNode);
				domClass.toggle(this.containerNode, "idxOneuiHoverCardWithoutPreviewImg", !innerWidget.image)
			}
		},
		/**
		 * Despite the name of this method, it actually handles both hover and focus
		 * events on the target node, setting a timer to show the HoverCard.
		 * @private
		 */
		_onHover: function(/*Event*/ e){		
			if(this.isShowingNow){return;}	
			if(!this._showTimer){
				var target = e.target;
				this._showTimer = setTimeout(lang.hitch(this, function(){this.open(target)}), this.showDelay);
			}
			if(this._hideTimer){
				clearTimeout(this._hideTimer);
				delete this._hideTimer;
			}
		},
		
		/**
		 * Despite the name of this method, it actually handles both mouseleave and blur
		 * events on the target node, hiding the HoverCard.
		 * @private
		 */
		_onUnHover: function(/*Event*/ /*===== e =====*/){
			// keep a HoverCard open if the associated element still has focus (even though the
			// mouse moved away)
			if(this._focus){ return; }

			if(this._showTimer){
				clearTimeout(this._showTimer);
				delete this._showTimer;
			}
			if(!this._hideTimer){
				this._hideTimer = setTimeout(lang.hitch(this, function(){this.close()}), this.hideDelay);
			}
		}, 
		onClose: function(){
			// summary:
			//		Called when hoverCard is hidden.
			//		This is called from the dijit.popup code.
			// tags:
			//		protected
			this.close();
		},
		
		onBlur: function(){
			// Close immediately on blur
			this.close();
		},
		
		postCreate: function(){
			this.bgIframe = new BackgroundIframe(this.domNode);

			// Setup fade-in and fade-out functions.
			this.fadeIn = fx.fadeIn({ node: this.domNode, duration: this.duration, onEnd: lang.hitch(this, "_onShow")});
			this.fadeOut = fx.fadeOut({ node: this.domNode, duration: this.duration, onEnd: lang.hitch(this, "_onHide")});
			// Detect other hoverCard open, determine whether close this instance or not. 
			this.subscribe("new-hoverCard-open", "onOtherCardOpen")
			
			this.connect(this.gripNode, "onmouseenter", function(){domClass.add(this.gripNode,"idxOneuiHoverCardGripHover");});
			this.connect(this.gripNode, "onmouseleave", function(){domClass.remove(this.gripNode,"idxOneuiHoverCardGripHover");});			
			this.connect(this.domNode, "onkeypress", "_onKey");
			this.connect(this.domNode, "onmouseenter", function(){
				this._hovered = true;
				if(this._hideTimer){
					clearTimeout(this._hideTimer);
					delete this._hideTimer;
				}
			});
			this.connect(this.domNode, "onmouseleave", function(){
				this._hovered = false;
				var stack = popup._stack;
				//if hoverCard is not top in popup stack, leave it be
				if(stack.length && stack[stack.length-1].widget != this){return;}
				this._onUnHover();
			});			
		},
		
		/**
		 * Display the HoverCard
		 * @public
		 */
		open: function(/*DomNode*/ target){
 			// summary:
			//		Display the HoverCard; usually not called directly.
			// tags:
			//		private
			focus._onTouchNode(this.target, "mouse");
			wai.setWaiRole(this.domNode, "alertdialog");
			
			var id = target.id;
			if(id && typeof(id) === "string"){
				wai.setWaiState(this.domNode, "labelledby", id);
			}
			
			if(this._showTimer){
				clearTimeout(this._showTimer);
				delete this._showTimer;
			}
			if(this.isShowingNow){
				if(this.forceFocus){
					this.focus();
				}
				return;
			}
			topic.publish("new-hoverCard-open", this.target);
			this.show(this.domNode.innerHTML, target, this.position, !this.isLeftToRight(), this.textDir);
			
			this._connectNode = target;
			this.onShow(target, this.position);
		},
		/**
		 * Hide the HoverCard or cancel timer for show of HoverCard
		 */
		close: function(){
			if(this._connectNode && !this._hovered && !this._moved && 
				(!this.moreActionsMenu || !this.moreActionsMenu._hoveredChild)){
				this.hide(this._connectNode);
				delete this._connectNode;
				this.onHide();
				this._removeFromPopupStack();
			}
			if(this._showTimer){
				clearTimeout(this._showTimer);
				delete this._showTimer;
			}
		},
		
		focus: function(){
			this.inherited(arguments);
			this._focus = true;
		},
		
		/**
		 * Display HoverCard around given node with specified contents.
		 * @param {string} innerHTML
		 * Contents of the hoverCard
		 * @param {DOM | Object} aroundNode
		 * Specifies that hoverCard should be next to this node / area
		 * @param {string[]} position
		 * List of positions to try to position tooltip (ex: ["right", "above"])
		 * @param {boolean} rtl
		 * Corresponds to `WidgetBase.dir` attribute, where false means "ltr" and true
		 * means "rtl"; specifies GUI direction, not text direction.
		 * @param {string} textDir
		 * Corresponds to `WidgetBase.textdir` attribute; specifies direction of text.
		 */
		show:  function(innerHTML, aroundNode, position, rtl, textDir){
			
			domStyle.set(this._popupWrapper, "display", "");
			domClass.remove(this.domNode, "dijitHidden");
			if(dojo.isIE <= 7){
				domStyle.set(this.bodyNode, "width", domStyle.get(this.containerNode, "width") + 5 + "px");
			}
			domStyle.set(this.connectorNode, "visibility", "visible");
			// reset width; it may have been set by orient() on a previous HoverCard show()
			this.domNode.width = "auto";

			if(this.fadeOut.status() == "playing"){
				// previous HoverCard is being hidden; wait until the hide completes then show new one
				this._onDeck=arguments;
				return;
			}
			
			this.set("textDir", textDir);
			this.containerNode.align = rtl? "right" : "left"; //fix the text alignment
			
			var pos = place.around(this.domNode, aroundNode,
				position && position.length ? position : HoverCard.defaultPosition, !rtl, lang.hitch(this, "orient"));
			
			this._connectorReposition(pos);
			
			
			if(this.gripNode && this.draggable){
				// Enable grip feature
				this.enableDnd();
			}
			// Prepare style before fadeIn animation
			domStyle.set(this.domNode, {
				"opacity": 0,
				"position": "absolute"
			});
			this._popupWrapper.appendChild(this.domNode);
			if(rtl){
				domStyle.set(this.domNode, {
					"left": domGeometry.position(this.domNode).x + "px",
					"right": "auto"
				})
			}
			//Set order in popup stack
			this._addToPopupStack(); 
			
			this.fadeIn.play();
			this.isShowingNow = true;
			this.aroundNode = aroundNode;
		},
		_addToPopupStack: function(){
			var stack = popup._stack;
			domStyle.set(this.domNode, "zIndex", popup._beginZIndex + stack.length);
			popup._stack.push({
				widget: this,
				handlers: []
			});
		},
		_createWrapper: function(){
			//This is for focusManager to find popup parent of hoverCard
			if(!this._popupWrapper){
				this._popupWrapper = domConstruct.create("div", {
					style: {"display": "none"}
				}, this.ownerDocumentBody);
				if(this.target && this.target.id){
					domAttr.set(this._popupWrapper, "dijitPopupParent", this.target.id);
				}else{
					this._popupWrapper.dijitPopupParent = this.target;
				}
			}
		},
		_removeFromPopupStack: function(){
			var stack = popup._stack, top;
			while((top = stack.pop())){
				var onClose = top.onClose,
					widget = top.widget;

				widget.onClose && widget.onClose();

				var h;
				while(h = top.handlers.pop()){ h.remove(); }

				// Hide the widget and it's wrapper unless it has already been destroyed in above onClose() etc.
				if(widget && widget.domNode){
					popup.hide(widget);
				}
				onClose && onClose();
				if(top.widget == this){break;}
			}
		},
		_connectorReposition: function(pos){
			// Reposition the HoverCard connector.
			if(pos.corner.charAt(0) == 'B' && pos.aroundCorner.charAt(0) == 'B'){
				var mb = domGeometry.getMarginBox(this.domNode);
				var connectorHeight = this.connectorNode.offsetHeight;
				if(mb.h > pos.spaceAvailable.h){
					// The HoverCard starts at the top of the page and will extend past the aroundNode
					var aroundNodePlacement = pos.spaceAvailable.h - ((pos.aroundNodePos.h + connectorHeight) >> 1);
					this.connectorNode.style.top = aroundNodePlacement + "px";
					this.connectorNode.style.bottom = "";
				}else{
					// Align center of connector with center of aroundNode, except don't let bottom
					// of connector extend below bottom of HoverCard content, or top of connector
					// extend past top of HoverCard content
					this.connectorNode.style.bottom = Math.min(
						Math.max(pos.aroundNodePos.h/2 - connectorHeight/2, 0),
						mb.h - connectorHeight) + "px";
					this.connectorNode.style.top = "";
				}
			}else if(pos.corner.charAt(0) == 'M' && pos.aroundCorner.charAt(0) == 'M'){
				this.connectorNode.style.top = pos.aroundNodePos.y + ((pos.aroundNodePos.h - this.connectorNode.offsetHeight) >> 1) - pos.y + "px";
				this.connectorNode.style.left = "";
			}else if(pos.corner.charAt(1) == 'M' && pos.aroundCorner.charAt(1) == 'M'){
				this.connectorNode.style.left = pos.aroundNodePos.x + ((pos.aroundNodePos.w - this.connectorNode.offsetWidth) >> 1) - pos.x + "px";
			}else{
				// reset the HoverCard back to the defaults
				this.connectorNode.style.top = "";
				this.connectorNode.style.bottom = "";
			}
		},
	
		orient: function(/*DomNode*/ node, /*String*/ aroundCorner, /*String*/ hoverCardCorner, /*Object*/ spaceAvailable, /*Object*/ aroundNodeCoords){
			// summary:
			//		Private function to set CSS for HoverCard node based on which position it's in.
			//		This is called by the dijit popup code.   It will also reduce the HoverCard's
			//		width to whatever width is available
			// tags:
			//		protected
			this.connectorNode.style.top = ""; //reset to default

			//Adjust the spaceAvailable width, without changing the spaceAvailable object
			var HoverCardSpaceAvaliableWidth = spaceAvailable.w - this.connectorNode.offsetWidth;

			node.className = "idxOneuiHoverCard " +
				{
					"MR-ML": "idxOneuiHoverCardRight",
					"ML-MR": "idxOneuiHoverCardLeft",
					"TM-BM": "idxOneuiHoverCardAbove",
					"BM-TM": "idxOneuiHoverCardBelow",
					"BL-TL": "idxOneuiHoverCardBelow idxOneuiHoverCardABLeft",
					"TL-BL": "idxOneuiHoverCardAbove idxOneuiHoverCardABLeft",
					"BR-TR": "idxOneuiHoverCardBelow idxOneuiHoverCardABRight",
					"TR-BR": "idxOneuiHoverCardAbove idxOneuiHoverCardABRight",
					"BR-BL": "idxOneuiHoverCardRight",
					"BL-BR": "idxOneuiHoverCardLeft",
					"TR-TL": "idxOneuiHoverCardRight"
					//"MR-ML": "idxOneuiHoverCardLeft"
				}[aroundCorner + "-" + hoverCardCorner];

			// reduce HoverCard's width to the amount of width available, so that it doesn't overflow screen
			this.domNode.style.width = "auto";
			var size = domGeometry.getContentBox(this.domNode);

			var width = Math.min((Math.max(HoverCardSpaceAvaliableWidth,1)), size.w);
			var widthWasReduced = width < size.w;

			this.domNode.style.width = width+"px";

			//Adjust width for HoverCards that have a really long word or a nowrap setting
			if(widthWasReduced){
				this.containerNode.style.overflow = "auto"; //temp change to overflow to detect if our HoverCard needs to be wider to support the content
				var scrollWidth = this.containerNode.scrollWidth;
				this.containerNode.style.overflow = "visible"; //change it back
				if(scrollWidth > width){
					scrollWidth = scrollWidth + domStyle.get(this.domNode,"paddingLeft") + domStyle.get(this.domNode,"paddingRight");
					this.domNode.style.width = scrollWidth + "px";
				}
			}

			return Math.max(0, size.w - HoverCardSpaceAvaliableWidth);
		},
		
		/**
		 * Hide the HoverCard on specified node / area
		 * @param {DOM | Object} aroundNode
		 */
		hide: function(aroundNode){
			
			if(this._onDeck && this._onDeck[1] == aroundNode){
				// this hide request is for a show() that hasn't even started yet;
				// just cancel the pending show()
				this._onDeck=null;
			}else if(this.aroundNode === aroundNode || this.isShowingNow){
				// this hide request is for the currently displayed HoverCard
				this.fadeIn.stop();
				this.isShowingNow = false;
				this.aroundNode = null;
				
				this.fadeOut.play();
				this._focus = false;
				this._hovered = false;
				this._moved = false;
			}else{
				// just ignore the call, it's for a HoverCard that has already been erased
			}
			
		},
		
		enableDnd: function(){
			var node = this.domNode, connector = this.connectorNode;
			this._moveable = new ((has("ie") == 6) ? TimedMoveable // prevent overload, see #5285
				: Moveable)(node, { handle: this.gripNode });
			
			this.connect(this._moveable, "onMoveStart", function(){
				domStyle.set(connector, "visibility", "hidden");
				domClass.add(this.gripNode,"idxOneuiHoverCardGripActive");
				this._moved = true;
			});
			this.connect(this._moveable, "onMoveStop", function(){
				domClass.remove(this.gripNode,"idxOneuiHoverCardGripActive");
				domClass.add(this.gripNode,"idxOneuiHoverCardGrip");
			});
		},
		
		onOtherCardOpen: function(otherCardTarget){
			// summary:
			//		Called at opening of other hoverCard, determine if close current hoverCard or not
			if(!this.isShowingNow){return;}
			var stack = popup._stack;
			if(stack.length && stack[stack.length - 1].widget == this){
				if(!dom.isDescendant(otherCardTarget, this.bodyNode)){
					this.close();
				}
			}
		},
		
		_onShow: function(){
			// summary:
			//		Called at end of fade-in operation
			// tags:
			//		protected
			if(has("ie")){
				// the arrow won't show up on a node w/an opacity filter
				this.domNode.style.filter="";
			}
			if(this.forceFocus){
				this.focus();
			}
		},
		
		_onHide: function(){
			// summary:
			//		Called at end of fade-out operation
			// tags:
			//		protected
			
			//
			// hideResult is not safe for destroy when the popup is showing
			// which caused by the time interval parameter
			//
			if (this.domNode)
				domClass.add(this.domNode, "dijitHidden");
			domStyle.set(this._popupWrapper, "display", "none");
			this.target.focus && this.target.focus();
			
			if(this._onDeck){
				// a show request has been queued up; do it now
				this.show.apply(this, this._onDeck);
				this._onDeck=null;
			}
		},
		_getFocusItems: function(){
            // summary:
            //		Finds focusable items in dialog,
            //		and sets this._firstFocusItem and this._lastFocusItem
            // tags:
            //		protected
            
            var elems = a11y._getTabNavigable(this.domNode);
            this._firstFocusItem = elems.lowest || elems.first || this.closeButtonNode || this.domNode;
            this._lastFocusItem = elems.last || elems.highest || this._firstFocusItem;
     	},
		_onKey: function(/*Event*/ evt){
			// summary:
			//		Handler for keyboard events
			// description:
			//		Keep keyboard focus in dialog; close dialog on escape key
			// tags:
			//		private

			var node = evt.target;
			if(evt.charOrCode === keys.TAB){
				this._getFocusItems(this.domNode);
			}
			var singleFocusItem = (this._firstFocusItem == this._lastFocusItem);
			if(evt.charOrCode == keys.ESCAPE){
				// Use setTimeout to avoid crash on IE, see #10396.
				setTimeout(lang.hitch(this, "hide"), 0);
				event.stop(evt);
			}else if(node == this._firstFocusItem && evt.shiftKey && evt.charOrCode === keys.TAB){
				if(!singleFocusItem){
					dijitfocus.focus(this._lastFocusItem); // send focus to last item in dialog
				}
				event.stop(evt);
			}else if(node == this._lastFocusItem && evt.charOrCode === keys.TAB && !evt.shiftKey){
				if(!singleFocusItem){
					dijitfocus.focus(this._firstFocusItem); // send focus to first item in dialog
				}
				event.stop(evt);
			}else if(evt.charOrCode === keys.TAB){
				// we want the browser's default tab handling to move focus
				// but we don't want the tab to propagate upwards
				evt.stopPropagation();
			}
		}
		
	});
	
//	// summary:
//		//		Static method to display HoverCard w/specified contents in specified position.
//		//		See description of idx.widget.HoverCard.defaultPosition for details on position parameter.
//		//		If position is not specified then idx.widget.HoverCard.defaultPosition is used.
//		// innerHTML: String
//		//		Contents of the HoverCard
//		// aroundNode: dijit.__Rectangle
//		//		Specifies that HoverCard should be next to this node / area
//		// position: String[]?
//		//		List of positions to try to position HoverCard (ex: ["right", "above"])
//		// rtl: Boolean?
//		//		Corresponds to `WidgetBase.dir` attribute, where false means "ltr" and true
//		//		means "rtl"; specifies GUI direction, not text direction.
//		// textDir: String?
//		//		Corresponds to `WidgetBase.textdir` attribute; specifies direction of text.	
//	HoverCard.show = idx.widget.showHoverCard = function(innerHTML, title, action, imgSrc, aroundNode, position, rtl, textDir){
//		
//		if(!HoverCard._masterTT){ idx.widget._masterTT = HoverCard._masterTT = new MasterHoverCard(); }
//		return HoverCard._masterTT.show(innerHTML, title, action, imgSrc, aroundNode, position, rtl, textDir);
//	};
//
//	// summary:
//		//		Static method to hide the HoverCard displayed via showHoverCard()
//	HoverCard.hide = idx.widget.hideHoverCard = function(aroundNode){
//		
//		return HoverCard._masterTT && HoverCard._masterTT.hide(aroundNode);
//	};
//	
	HoverCard.defaultPosition = ["after-centered", "before-centered", "below", "above"];
	
	oneuiRoot.HoverCard = HoverCard;
	
	return HoverCard;	
	
});
