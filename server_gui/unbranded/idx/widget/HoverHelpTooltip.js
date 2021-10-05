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
define(["dojo/_base/declare", "dojo/_base/fx", // fx.fadeIn fx.fadeOut
 "dojo/keys", // keys
 "dojo/_base/array", // array.forEach array.indexOf array.map
 "dojo/dom", // dom.byId
 "dojo/on",
 "dojo/aspect",
 "dojo/when", 
 "dojo/Deferred",
 "dojo/dom-attr", // domAttr.set
 "dojo/_base/lang", // lang.hitch lang.isArrayLike
 "dojo/_base/sniff", // has("ie")
 "dijit/popup",
 "dijit/focus", "dojo/_base/event", // event.stop
 "dojo/dom-geometry", // domGeometry.getMarginBox domGeometry.position
 "dojo/dom-construct",
 "dojo/dom-class",
 "dijit/registry",
 "dijit/place", "dijit/a11y", // _getTabNavigable
 "dojo/dom-style", // domStyle.set, domStyle.get
 "dojo/_base/window", // win.body
 "dijit/_base/manager", // manager.defaultDuration
 "dijit/_Widget", "dijit/_TemplatedMixin", "dijit/Tooltip", 
 "dojo/has!dojo-bidi?../bidi/widget/HoverHelpTooltip",
 "dojo/text!./templates/HoverHelpTooltip.html", "dijit/dijit", 
 "dojo/i18n!./nls/Dialog", "dojo/i18n!./nls/HoverHelpTooltip"
], function(declare, fx, keys, array, dom, on, aspect, when, Deferred, domAttr, lang, has, popup, dijitfocus, event, domGeometry, domConstruct, domClass, registry, place, 
 a11y, domStyle, win, manager, _Widget, _TemplatedMixin, Tooltip, bidiExtension, template, dijit, dialogNls, hoverHelpTooltipNls){
	var oneuiRoot = lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2
	
    /**
     * @name idx.widget.HoverHelpTooltip
     * @class HoverHelpTooltip provides pop-up information that displays when users hover the mouse pointer over an help indicator.
     * HoverHelpTooltip is implemented following the standard and specified IBM One UI(tm)
     * <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y20&vsub=*&hsub=*&openpanes=1111111111">Hover Help</a></b>
     * @augments dijit.Tooltip
     * @example
     &lt;span data-dojo-type="idx.widget.HoverHelpTooltip" data-dojo-props='
     connectId:["anchor"],
     forceFocus: true,
     showLearnMore:true,
     learnMoreLinkValue:"http://www.ibm.com"'
     style="width: 300px"&gt;
     Passwords must be between 5 and 20 characters. There must be a combination of alphanumeric characters, starting with a letter and at least one number.&lt;br /&gt;&lt;br /&gt;
     &lt;/span&gt;
     * @see dijit.Tooltip
     **/
    var HoverHelpTooltip = declare("idx.widget.HoverHelpTooltip", Tooltip, {
        /** @lends idx.widget.HoverHelpTooltip.prototype */
        showDelay: 500,
        hideDelay: 800,
        /**
         * Whether to show Learn more link
         * @type Boolean
         */
        showLearnMore: false,
        /**
         * Learn more link value
         * @type String
         */
        learnMoreLinkValue: "#updateme",
        
        showCloseIcon: true,
        /**
         * Focus HoverHelpTooltip once it shown.
         * @type Boolean
         */
        forceFocus: false,
		
		textDir: "auto",

        _onHover: function(/*Event*/e){
            // summary:
            //		Despite the name of this method, it actually handles both hover and focus
            //		events on the target node, setting a timer to show the HoverHelpTooltip.
            // tags:
            //		private
            if (!HoverHelpTooltip._showTimer) {
                var target = e.target;
                HoverHelpTooltip._showTimer = setTimeout(lang.hitch(this, function(){
                    this.open(target)
                }), this.showDelay);
            }
            if (HoverHelpTooltip._hideTimer) {
                clearTimeout(HoverHelpTooltip._hideTimer);
                delete HoverHelpTooltip._hideTimer;
            }
        },
        
        _onUnHover: function(/*Event*/ /*===== e =====*/){
            // summary:
            //		Despite the name of this method, it actually handles both mouseleave and blur
            //		events on the target node, hiding the HoverHelpTooltip.
            // tags:
            //		private
            
            // keep a HoverHelpTooltip open if the associated element still has focus (even though the
            // mouse moved away)
            if (HoverHelpTooltip._showTimer) {
                clearTimeout(HoverHelpTooltip._showTimer);
                delete HoverHelpTooltip._showTimer;
            }
            if (!HoverHelpTooltip._hideTimer) {
                HoverHelpTooltip._hideTimer = setTimeout(lang.hitch(this, function(){
                    this.close()
                }), this.hideDelay);
            }
        },
        
        /**
         * Display the HoverHelpTooltip
         * @private
         */
        open: function(/*DomNode*/target){
            // summary:
            //		
            // tags:
            //		private
            if (HoverHelpTooltip._showTimer) {
                clearTimeout(HoverHelpTooltip._showTimer);
                delete HoverHelpTooltip._showTimer;
            }
			// Fix for Defect 12350: 
			if(!dom.byId(target)){return;}
			
			var ariaLabel = domAttr.get(this.domNode, "aria-label");
            HoverHelpTooltip.show(
				this.content || this.label || this.domNode.innerHTML, 
				target, this.position, !this.isLeftToRight(), 
				this.textDir, this.showLearnMore, this.learnMoreLinkValue, 
				this.showCloseIcon, this.forceFocus, ariaLabel);
            this._connectNode = target;
            this.onShow(target, this.position);
        },
		
        close: function(force){
            // summary:
            //		Hide the tooltip or cancel timer for show of tooltip
            // tags:
            //		private
            if (this._connectNode) {
                // if tooltip is currently shown
            	
            	// CHANGE FROM BARRY -- CREATED A DEFERRED OBJECT THAT GETS RESOLVED WHEN THE
            	// ANIMATION FOR HIDING COMPLETES AND RETURN IT FORM THIS METHOD SO THE CALLER
            	// CAN OPT TO WAIT FOR ANIMATION TO COMPLETE BEFORE TAKING A NEXT STEP
            	// -- I NEEDED THIS AT ONE POINT, BUT AFTER FIXING AN ISSUE WITH INTERFERENCE OF
            	// MULTIPLE TOOLTIPS IN RACE CONDITIONS IT WAS NO LONGER NEEDED.  I LEFT IT HERE
            	// SINCE IT CANNOT HURT TO GIVE THE CALLER THIS OPTION OF USING dojo/promise API
                var anim = HoverHelpTooltip.hide(this._connectNode, force);
                delete this._connectNode;
                this.onHide();
                if (anim) {
                   // create the deferred object
              	   var d = new Deferred();
              	   var handles = [];
              	   
              	   // define the function to resolve the deferred
              	   var resolve = function() {
              		   // mark the deferred as resolved (fulfill the promise)
              		   if (d) d.resolve();
              		   
              		   // for each aspect handle, remove it
              		   array.forEach(handles, function(handle) {
              			   if (handle) handle.remove();
              		   });
              	   };
              	   
              	   // use aspect.after to attach to the "onEnd" and "onStop" methods and
              	   // fire the "resolve" callback when one of these fires
              	   handles.push(aspect.after(anim, "onEnd", resolve));
              	   handles.push(aspect.after(anim, "onStop", resolve));
              	   
              	   // double-check if the animation already stopped and our deferred missed the event
              	   if ((anim.status() == "stopped") && (!d.isResolved())) {
              		   resolve();
              	   }
              	   
              	   // return the dojo/Deferred "promise"
              	   return d;
                 } else {
                   // if no animation then return null for dojo/when API
              	   return null;
                 }             	
            }
            if (HoverHelpTooltip._showTimer) {
                // if tooltip is scheduled to be shown (after a brief delay)
                clearTimeout(HoverHelpTooltip._showTimer);
                delete HoverHelpTooltip._showTimer;
            }
        },
        _setConnectIdAttr: function(/*String|String[]*/newId){
            // summary:
            //		Connect to specified node(s)
            
            // Remove connections to old nodes (if there are any)
            array.forEach(this._connections || [], function(nested){
                array.forEach(nested, lang.hitch(this, "disconnect"));
            }, this);
            
            // Make array of id's to connect to, excluding entries for nodes that don't exist yet, see startup()
            this._connectIds = array.filter(lang.isArrayLike(newId) ? newId : (newId ? [newId] : []), function(id){
                return dom.byId(id);
            });
            
            // Make connections
            this._connections = array.map(this._connectIds, function(id){
                var node = dom.byId(id);
                return [
                    // TODO - NOTE FROM BARRY: consider "onclick" triggering focus but "onmouseenter" NOT triggering focus
					this.connect(node, "onmouseenter", "_onHover"),
					this.connect(node, "onmouseleave", "_onUnHover"), 
					this.connect(node, "onclick", "_onHover"),
					this.connect(node, "onkeypress", "_onConnectIdKey")
				];
            }, this);
            
            this._set("connectId", newId);
        },
        _onConnectIdKey: function(/*Event*/evt){
            // summary:
            //		Handler for keyboard events
            // description:
            // tags:
            //		private
            var node = evt.target;
            
            if (evt.charOrCode == keys.ENTER || evt.charOrCode == keys.SPACE || evt.charOrCode == " " || evt.charOrCode == keys.F1) {
                // Use setTimeout to avoid crash on IE, see #10396.
                HoverHelpTooltip._showTimer = setTimeout(lang.hitch(this, function(){
                    this.open(node)
                }), this.showDelay);
                
                event.stop(evt);
            }
        }
        
    });
    
    var baseClassName = has("dojo-bidi")? "idx.widget._MasterHoverHelpTooltip_" : "idx.widget._MasterHoverHelpTooltip";
    var MasterHoverHelpTooltipBase = declare(baseClassName, [_Widget, _TemplatedMixin], {
		/**
		 * Milliseconds to fade in/fade out
		 * @type Integer
		 */
        duration: manager.defaultDuration,
        
        templateString: template,
		
        learnMoreLabel: "",
        
        /**
         * draggable: Boolean
         *		Toggles the moveable aspect of the HoverHelpTooltip. If true, HoverHelpTooltip
         *		can be dragged by it's grippy bar. If false it will remain positioned
         *		relative to the attached node
         *		@type boolean
         **/
        draggable: true,
        
        _firstFocusItem: null,
		
        _lastFocusItem: null,
        
        postMixInProperties: function(){
            this.learnMoreLabel = hoverHelpTooltipNls.learnMoreLabel;
			this.buttonClose = dialogNls.closeButtonLabel;
        },
        postCreate: function(){
            win.body().appendChild(this.domNode);
            
            //this.bgIframe = new BackgroundIframe(this.domNode);
            
            // Setup fade-in and fade-out functions.
            this.fadeIn = fx.fadeIn({
                node: this.domNode,
                duration: this.duration,
                onEnd: lang.hitch(this, "_onShow")
            });
            this.fadeOut = fx.fadeOut({
                node: this.domNode,
                duration: this.duration,
                onEnd: lang.hitch(this, "_onHide")
            });
            this.connect(this.domNode, "onkeypress", "_onKey");
            this.connect(this.domNode, "onmouseenter", lang.hitch(this, function(e){
				if(HoverHelpTooltip._hideTimer) {
	                clearTimeout(HoverHelpTooltip._hideTimer);
	                delete HoverHelpTooltip._hideTimer;
	            }

				// check if another tooltip is already deck
				if (!this._onDeck) {
					// CHANGE FROM BARRY - ONLY RESTORE A DYING TOOLTIP IF ANOTHER IS NOT ON DECK
					// no tooltip on deck so refocus this one and keep showing it
					
					if (this.forceFocus && this.showLearnMore) this.focus(); // NOTE FROM BARRY: consider removing this unless clicked
					this._keepShowing = true;
					
					// restore the previous state that was cleared out in forceHide
					if (this._prevState) {
						lang.mixin(this, this._prevState);
						this._prevState = null;
					}
					this.fadeOut.stop();
					this.fadeIn.play();
				} else {
					// CHANGE FROM BARRY -- IGNORE THE MOUSE LEAVE EVENT AS THE TOOLTIP CLOSES SINCE ANOTHER IS ON DECK
					// another tooltip is already deck so signal that the mouse leave event on the
					// current tooltip should not trigger a close
					this._ignoreMouseLeave = true;
				}
            }));
			this.connect(this.domNode, "onmouseleave", lang.hitch(this, function(e){
				this._keepShowing = false;
				if (this._ignoreMouseLeave) {
					// mouseenter occurred on previous tooltip and mouseleave is now firing
					// ignore this mouseleave event since the mouseenter was not for this tooltip
					// and we don't want to close the current tooltip
					delete this._ignoreMouseLeave;
					return;
				}
				HoverHelpTooltip._hideTimer = setTimeout(lang.hitch(this, function(){this.hide(this.aroundNode)}), this.hideDelay);
			}));
        },
        show: function(innerHTML, aroundNode, position, rtl, textDir, showLearnMore, 
			learnMoreLinkValue, showCloseIcon, forceFocus, ariaLabel){
			this._lastFocusNode = aroundNode;
            if (showLearnMore) {
                this.learnMoreNode.style.display = "inline";
                this.learnMoreNode.href = learnMoreLinkValue;
            }
            else {
                this.learnMoreNode.style.display = "none";
            }
            if (showCloseIcon || showCloseIcon == null) 
                this.closeButtonNode.style.display = "inline";
            else {
                this.closeButtonNode.style.display = "none";
            }
            //in case connectorNode was hidden on a previous call to hide
            this.connectorNode.removeAttribute('hidden');
            
            if (this.aroundNode && this.aroundNode === aroundNode && this.containerNode.innerHTML == innerHTML) {
                return;
            }
            
            // reset width; it may have been set by orient() on a previous HoverHelpTooltip show()
            this.domNode.width = "auto";
            
            if (this.fadeOut.status() == "playing") {
                // previous HoverHelpTooltip is being hidden; wait until the hide completes then show new one
                this._onDeck = arguments;
                return;
            }
			
			
			//this._attachPopParent();
			//domStyle.set(this._popupWrapper, "display", "");
            
            this.containerNode.innerHTML = innerHTML;
			if(ariaLabel)domAttr.set(this.domNode, "aria-label", ariaLabel);
            
            this.set("textDir", textDir);
            this.containerNode.align = rtl ? "right" : "left"; //fix the text alignment
            var pos = place.around(this.domNode, aroundNode, position && position.length ? position : HoverHelpTooltip.defaultPosition, !rtl, lang.hitch(this, "orient"));
            
            // Position the HoverHelpTooltip connector for middle alignment.
            // This could not have been done in orient() since the HoverHelpTooltip wasn't positioned at that time.
            var aroundNodeCoords = pos.aroundNodePos;
            if (pos.corner.charAt(0) == 'M' && pos.aroundCorner.charAt(0) == 'M') {
                this.connectorNode.style.top = aroundNodeCoords.y + ((aroundNodeCoords.h - this.connectorNode.offsetHeight) >> 1) - pos.y + "px";
                this.connectorNode.style.left = "";
            }
            else if (pos.corner.charAt(1) == 'M' && pos.aroundCorner.charAt(1) == 'M') {
                this.connectorNode.style.left = aroundNodeCoords.x + ((aroundNodeCoords.w - this.connectorNode.offsetWidth) >> 1) - pos.x + "px";
            }
			
			////Set order in popup stack
			//this._addToPopupStack(); 
			
            // show it
            domStyle.set(this.domNode, "opacity", 0);
			domClass.add(this.domNode, "dijitPopup");
			//this._popupWrapper.appendChild(this.domNode);
            this.fadeIn.play();
            this.isShowingNow = true;
			this.aroundNode = aroundNode;
			// Add WAI-ARIA attribute to the hover tooltip container
			var sourceId = domAttr.get(aroundNode, "id")
			if(typeof sourceId == "string"){
				dijit.setWaiState(this.containerNode, "labelledby", sourceId);
			}
			if (forceFocus) {
                this.focus();
            }
			return;
        },
		
		/*_addToPopupStack: function(){
			var stack = popup._stack;
			domStyle.set(this.domNode, "zIndex", popup._beginZIndex + stack.length);
			popup._stack.push({
				widget: this,
				handlers: []
			});
		},
        _attachPopParent: function(){
			//This is for focusManager to find popup parent of HHT
			if(!this._popupWrapper){
				this._popupWrapper = domConstruct.create("div", {
					style: {"display": "none"}
				}, this.ownerDocumentBody);
			}
			if(this.aroundNode && this.aroundNode.id){
				domAttr.set(this._popupWrapper, "dijitPopupParent", this.aroundNode.id);
			}else{
				this._popupWrapper.dijitPopupParent = this.aroundNode;
			}
		},
        _removeFromPopupStack: function(){
			popup._stack.pop();
		},*/
        orient: function(/*DomNode*/node, /*String*/ aroundCorner, /*String*/ HoverHelpTooltipCorner, /*Object*/ spaceAvailable, /*Object*/ aroundNodeCoords){
            // summary:
            //		Private function to set CSS for HoverHelpTooltip node based on which position it's in.
            //		This is called by the dijit popup code.   It will also reduce the HoverHelpTooltip's
            //		width to whatever width is available
            // tags:
            //		protected
            this.connectorNode.style.top = ""; //reset to default
            //Adjust the spaceAvailable width, without changing the spaceAvailable object
            var HoverHelpTooltipSpaceAvaliableWidth = spaceAvailable.w - this.connectorNode.offsetWidth;
            
            node.className = "idxOneuiHoverHelpTooltip " +
            {
                "MR-ML": "idxOneuiHoverHelpTooltipRight",
                "ML-MR": "idxOneuiHoverHelpTooltipLeft",
                "TM-BM": "idxOneuiHoverHelpTooltipAbove",
                "BM-TM": "idxOneuiHoverHelpTooltipBelow",
                "BL-TL": "idxOneuiHoverHelpTooltipBelow idxOneuiHoverHelpTooltipABLeft",
                "TL-BL": "idxOneuiHoverHelpTooltipAbove idxOneuiHoverHelpTooltipABLeft",
                "BR-TR": "idxOneuiHoverHelpTooltipBelow idxOneuiHoverHelpTooltipABRight",
                "TR-BR": "idxOneuiHoverHelpTooltipAbove idxOneuiHoverHelpTooltipABRight",
                "BR-BL": "idxOneuiHoverHelpTooltipRight",
                "BL-BR": "idxOneuiHoverHelpTooltipLeft",
                "TR-TL": "idxOneuiHoverHelpTooltipRight"
            }[aroundCorner + "-" + HoverHelpTooltipCorner];
            
            // reduce HoverHelpTooltip's width to the amount of width available, so that it doesn't overflow screen
            this.domNode.style.width = "auto";
            var size = domGeometry.position(this.domNode);
            
            var width = Math.min((Math.max(HoverHelpTooltipSpaceAvaliableWidth, 1)), size.w);
            var widthWasReduced = width < size.w;
            
            this.domNode.style.width = width + "px";
            
            //Adjust width for HoverHelpTooltips that have a really long word or a nowrap setting
            if (widthWasReduced) {
                this.containerNode.style.overflow = "auto"; //temp change to overflow to detect if our HoverHelpTooltip needs to be wider to support the content
                var scrollWidth = this.containerNode.scrollWidth;
                this.containerNode.style.overflow = "visible"; //change it back
                if (scrollWidth > width) {
                    scrollWidth = scrollWidth + domStyle.get(this.domNode, "paddingLeft") + domStyle.get(this.domNode, "paddingRight");
                    this.domNode.style.width = scrollWidth + "px";
                }
            }
            
            // Reposition the HoverHelpTooltip connector.
            if (HoverHelpTooltipCorner.charAt(0) == 'B' && aroundCorner.charAt(0) == 'B') {
                var mb = domGeometry.getMarginBox(node);
                var HoverHelpTooltipConnectorHeight = this.connectorNode.offsetHeight;
                if (mb.h > spaceAvailable.h) {
                    // The HoverHelpTooltip starts at the top of the page and will extend past the aroundNode
                    var aroundNodePlacement = spaceAvailable.h - ((aroundNodeCoords.h + HoverHelpTooltipConnectorHeight) >> 1);
                    this.connectorNode.style.top = aroundNodePlacement + "px";
                    this.connectorNode.style.bottom = "";
                }
                else {
                    // Align center of connector with center of aroundNode, except don't let bottom
                    // of connector extend below bottom of HoverHelpTooltip content, or top of connector
                    // extend past top of HoverHelpTooltip content
                    this.connectorNode.style.bottom = Math.min(Math.max(aroundNodeCoords.h / 2 - HoverHelpTooltipConnectorHeight / 2, 0), mb.h - HoverHelpTooltipConnectorHeight) +
                    "px";
                    this.connectorNode.style.top = "";
                }
            }
            else {
                // reset the HoverHelpTooltip back to the defaults
                this.connectorNode.style.top = "";
                this.connectorNode.style.bottom = "";
            }
            
            return Math.max(0, size.w - HoverHelpTooltipSpaceAvaliableWidth);
        },
        
        focus: function(){
			if(this._focus){return;}
            this._getFocusItems(this.outerContainerNode);
            this._focus = true;
            dijitfocus.focus(this._firstFocusItem);
        },
        
        _onShow: function(){
            // summary:
            //		Called at end of fade-in operation
            // tags:
            //		protected
            if (has("ie")) {
                // the arrow won't show up on a node w/an opacity filter
                this.domNode.style.filter = "";
            }
            domAttr.set(this.containerNode, "tabindex", "0");
            domAttr.set(this.learnMoreNode, "tabindex", "0");
            domAttr.set(this.closeButtonNode, "tabindex", "0");
        },
        
        hide: function(aroundNode, force){
            // summary:
            //		Hide the HoverHelpTooltip
            if(this._keepShowing){
            	this._keepShowing = false; 
            	if (!force) return;
            }
            if (this._onDeck && this._onDeck[1] == aroundNode) {
                // this hide request is for a show() that hasn't even started yet;
                // just cancel the pending show()
                this._onDeck = null;
            }
            else if(this.aroundNode === aroundNode){
                    // this hide request is for the currently displayed HoverHelpTooltip
                return this._forceHide();
            }
        },
        hideOnClickClose: function(){
            // summary:
            //		Hide the HoverHelpTooltip
            // this hide request is for the currently displayed HoverHelpTooltip    
            this._forceHide(true);
        },
        
        // CHANGE FROM BARRY: added "refocus" parameter so that refocus only happens on hiding the tooltip
        // if the user initiated the hide via "hideOnClickClose" or "ESC" key.  this prevents toltips that
        // are disappearing because their associated field blurred focus from refocusing the field that was
        // just blurred.  programmatic hiding/showing of the tooltip makes this problem evident
        _forceHide: function(refocus){
        	// CHANGE FROM BARRY
        	// only refocus if flag is set and the node is defined
            if (refocus && this._lastFocusNode) {
            	// check if the defined node has an enclosing widget with a "refocus" method
            	var currentWrapWidget = registry.getEnclosingWidget(this._lastFocusNode);
            	
            	// if the widget is found and has a refocus method then call it, else focus the node
            	if (currentWrapWidget && lang.isFunction(currentWrapWidget.refocus)) currentWrapWidget.refocus();
            	else dijitfocus.focus(this._lastFocusNode);
            }
            
            // CHANGE FROM BARRY
            // We save the state of this instance in case the "_forceHide" is cancelled by a "mouseenter"
            // event.  We need to restore the state after cancellingt he fadeOut and fading the tooltip 
            // back in.
            if ((this.aroundNode)||(this._lastFocusNode)) {
            	this._prevState = {
            			_lastFocusNode: this._lastFocusNode,
            			_firstFocusItem: this._firstFocusItem,
            			_lastFocusItem: this._lastFocusItem,
            			_focus: this._focus,
            			isShowingNow: this.isShowingNow,
            			aroundNode: this.aroundNode
            	};
            }
			this._lastFocusNode = null;
			this._firstFocusItem = null;
			this._lastFocusItem = null;
            this._focus = false;
            this.fadeIn.stop();
            this.isShowingNow = false;
			this.aroundNode = null; // moved this to here, similar to dijit.Tooltip
			//this._removeFromPopupStack();
            return this.fadeOut.play();
        },
        _getFocusItems: function(){
            // summary:
            //		Finds focusable items in tooltip,
            //		and sets this._firstFocusItem and this._lastFocusItem
            // tags:
            //		protected
			
			if(this._firstFocusItem){
				this._firstFocusItem = this.closeButtonNode;
				return;
			}
			var elems = a11y._getTabNavigable(this.containerNode),
				endFocusableNode = domStyle.get(this.learnMoreNode, "display") == "none" ? this.closeButtonNode : this.learnMoreNode;
			this._firstFocusItem = elems.lowest || elems.first || endFocusableNode;
			this._lastFocusItem = elems.last || elems.highest || endFocusableNode;
        },
        _onKey: function(/*Event*/evt){
            // summary:
            //		Handler for keyboard events
            // description:
            // tags:
            //		private
            
            var node = evt.target;
            if (evt.charOrCode === keys.TAB) {
                this._getFocusItems(this.outerContainerNode);
            }
            var singleFocusItem = (this._firstFocusItem == this._lastFocusItem);
            if (evt.charOrCode == keys.ESCAPE) {
                // Use setTimeout to avoid crash on IE, see #10396.
                setTimeout(lang.hitch(this, "hideOnClickClose"), 0);
                event.stop(evt);
            }
            else 
                if (node == this._firstFocusItem && evt.shiftKey && evt.charOrCode === keys.TAB) {
                    if (!singleFocusItem) {
                        dijitfocus.focus(this._lastFocusItem); // send focus to last item in dialog
                    }
                    event.stop(evt);
                }
                else 
                    if (node == this._lastFocusItem && evt.charOrCode === keys.TAB && !evt.shiftKey) {
                        if (!singleFocusItem) {
                            dijitfocus.focus(this._firstFocusItem); // send focus to first item in dialog
                        }
                        event.stop(evt);
                    }
                    else 
                        if (evt.charOrCode === keys.TAB) {
                            // we want the browser's default tab handling to move focus
                            // but we don't want the tab to propagate upwards
                            evt.stopPropagation();
                        }
        },
        _onHide: function(){
            // summary:
            //		Called at end of fade-out operation
            // tags:
            //		protected
            this._prevState = null;
            this.domNode.style.cssText = ""; // to position offscreen again
            this.containerNode.innerHTML = "";
			//domStyle.set(this._popupWrapper, "display", "none");
            domAttr.remove(this.containerNode, "tabindex");
            domAttr.remove(this.learnMoreNode, "tabindex");
            domAttr.remove(this.closeButtonNode, "tabindex");      
			domAttr.remove(this.domNode, "aria-label");      
            if (this._onDeck) {
            	var args = this._onDeck;
            	this._onDeck = null;
                // a show request has been queued up; do it now
                this.show.apply(this, args);
            }
        },
        onBlur: function(){
            this._forceHide();
        }
    }); //end declare
    //    var MasterHoverHelpTooltip = Tooltip._MasterTooltip;
    HoverHelpTooltip._MasterHoverHelpTooltip = MasterHoverHelpTooltip = has("dojo-bidi")? declare("idx.widget._MasterHoverHelpTooltip",[MasterHoverHelpTooltipBase,bidiExtension]) : MasterHoverHelpTooltipBase; // for monkey patching
    // summary:
    //		Static method to display HoverHelpTooltip w/specified contents in specified position.
    //		See description of idx.widget.HoverHelpTooltip.defaultPosition for details on position parameter.
    //		If position is not specified then idx.widget.HoverHelpTooltip.defaultPosition is used.
    // innerHTML: String
    //		Contents of the HoverHelpTooltip
    // aroundNode: dijit.__Rectangle
    //		Specifies that HoverHelpTooltip should be next to this node / area
    // position: String[]?
    //		List of positions to try to position HoverHelpTooltip (ex: ["right", "above"])
    // rtl: Boolean?
    //		Corresponds to `WidgetBase.dir` attribute, where false means "ltr" and true
    //		means "rtl"; specifies GUI direction, not text direction.
    // textDir: String?
    //		Corresponds to `WidgetBase.textdir` attribute; specifies direction of text.	
    HoverHelpTooltip.show = idx.widget.showHoverHelpTooltip = function(innerHTML, aroundNode, position, rtl, textDir, showLearnMore, learnMoreLinkValue, showCloseIcon, forceFocus, ariaLabel){
    
        if (!HoverHelpTooltip._masterTT) {
            idx.widget._masterTT = HoverHelpTooltip._masterTT = new MasterHoverHelpTooltip();
        }
        return HoverHelpTooltip._masterTT.show(innerHTML, aroundNode, position, rtl, textDir, showLearnMore, learnMoreLinkValue, showCloseIcon, forceFocus, ariaLabel);
    };
    
    // summary:
    //		Static method to hide the HoverHelpTooltip displayed via showHoverHelpTooltip()
    HoverHelpTooltip.hide = idx.widget.hideHoverHelpTooltip = function(aroundNode){
    
        return HoverHelpTooltip._masterTT && HoverHelpTooltip._masterTT.hide(aroundNode);
    };
    
    HoverHelpTooltip.defaultPosition = ["after-centered", "before-centered", "below", "above"];
    
    // for IDX 1.2 compatibility
	oneuiRoot.HoverHelpTooltip = HoverHelpTooltip;

    return HoverHelpTooltip;
    
});
