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
		"dojo/_base/lang",
		"dojo/_base/event",
		"dojo/_base/array",
		"dojo/keys",
		"dojo/on",
		"dojo/aspect",
		"dojo/dom",
		"dojo/dom-class",
		"dojo/dom-geometry",
    	"dijit/registry",
		"dijit/a11y",
		"dijit/popup",
		"dijit/layout/TabController",
        "dijit/layout/ScrollingTabController"],
       function(declare, lang, event, array, keys, on, aspect, dom, domClass, domGeometry, registry, a11y, popupMgr, TabController, ScrollingTabController){
	/**
	 * Creates an idx.layout.MenuTabController
	 * @name idx.layout.MenuTabController
	 * @class The MenuTabController widget provides a tab controller with
	 * support for drop-down menus on tabs. It is an extension of
	 * dijit.layout.ScrollingTabController, and supports a new 'popup' property
	 * on each content pane in the associated dijit.layout.StackContainer.
	 * <p>
	 * To add a drop-down menu to a tab, create a menu widget and assign it to
	 * the 'popup' property of the content pane for the tab in the associated
	 * dijit.layout.StackContainer. A drop-down arrow affordance will be added
	 * to the tab. Clicking the drop-down arrow, or using right-click or Shift+F10
	 * on the tab, will cause the drop-down menu to be shown.
	 * </p>
	 * <p>
	 * Note that if the 'closeable' property is set to true on a content pane that
	 * also has a 'popup' set, the corresponding tab will also be closeable and will
	 * show a close affordance alongside the drop-down arrow affordance. However, the
	 * drop-down menu will replace the automatic menu containing a 'Close' item, so
	 * in order to maintain keyboard access to the close function a suitable action
	 * item should be included in the drop-down menu that is supplied.
	 * </p>
	 * @augments dijit.layout.ScrollingTabController
	 */

 	var PopupTabButton = declare("idx.layout._PopupTabButton", [TabController.TabButton], {
		//	summary:
		//
		//		_PopupTabButton
		//
		//      An extension of the dijit _TabButton which adds a popup affordance.
		//
	    /*templateString:
	    	'<div role="presentation" data-dojo-attach-point="titleNode" data-dojo-attach-event="onclick:onClick">' +
				'<div role="presentation" data-dojo-attach-point="focusNode">' +
					'<div role="presentation" class="dijitTabInnerDiv" data-dojo-attach-point="innerDiv">' +
						'<div role="presentation" class="dijitTabContent" data-dojo-attach-point="tabContent">' +
			        		'<img src="${_blankGif}" alt="" class="dijitIcon dijitTabButtonIcon" data-dojo-attach-point="iconNode" />' +
					        '<span data-dojo-attach-point="containerNode" class="tabLabel">' + 
					        '</span>' +
					        '<span class="dijitInline dijitTabArrowButton dijitTabArrowIcon" data-dojo-attach-point="dropdownNode" data-dojo-attach-event="onclick: onClickArrowButton, onmouseenter: onMouseEnterArrowButton, onmouseleave: onMouseLeaveArrowButton" role="presentation">' +
					            '<span class="dijitTabDropDownText">${tabDropDownText}</span>' +
					        '</span>' +
					        '<span class="dijitInline dijitTabCloseButton dijitTabCloseIcon" data-dojo-attach-point="closeNode" role="presentation">' +
					            '<span class="dijitTabCloseText">${tabCloseText}</span>' +
					        '</span>' +
					        '<span class="idxTabSeparator" aria-hidden="true" data-dojo-attach-point="separatorNode">${tabSeparatorText}</span>' +
						'</div>' +
					'</div>' +
				'</div>' +
			'</div>',*/
		templateString:
			'<div role="presentation" data-dojo-attach-point="titleNode,innerDiv,tabContent" class="dijitTabInner dijitTabContent">' +
				'<img src="${_blankGif}" alt="" class="dijitIcon dijitTabButtonIcon" data-dojo-attach-point="iconNode"/>' +
				'<span data-dojo-attach-point="containerNode,focusNode" class="tabLabel"></span>' +
		        '<span class="dijitInline dijitTabArrowButton dijitTabArrowIcon" data-dojo-attach-point="dropdownNode" data-dojo-attach-event="onmouseenter: onMouseEnterArrowButton, onmouseleave: onMouseLeaveArrowButton" role="presentation">' +
		            '<span data-dojo-attach-point="dropdownText" class="dijitTabDropDownText">${tabDropDownText}</span>' +
		        '</span>' +
				'<span class="dijitInline dijitTabCloseButton dijitTabCloseIcon" data-dojo-attach-point="closeNode" role="presentation">' +
					'<span data-dojo-attach-point="closeText" class="dijitTabCloseText">[x]</span>' +
				'</span>' +
		        '<span class="idxTabSeparator" aria-hidden="true" data-dojo-attach-point="separatorNode">${tabSeparatorText}</span>' +
			'</div>',		
			
		tabDropDownText: "&nbsp;[v]",
		tabCloseText: "[x]",
		tabSeparatorText: "",
			
		_setCloseButtonAttr: function(/*Boolean*/ disp){
			// we suppress the default pop-up menu containing "Close"
			// if the tab is closable but we have our own menu to display
			if(this.arrowButton){
				this._set("closeButton", disp);
				domClass.toggle(this.innerDiv, "dijitClosable", disp);
				this.closeNode.style.display = disp ? "" : "none";
			}else{
				this.inherited(arguments);
			}
		},

		_setArrowButtonAttr: function(/*Boolean*/ disp){
			this._set("arrowButton", disp);
			//domClass.toggle(this.innerDiv, "dijitPopup", disp);
			this.dropdownNode.style.display = disp ? "" : "none";
		},

		onClickArrowButton : function(/*Event*/ evt){
			evt.stopPropagation();
		},

		onMouseEnterArrowButton : function(event){
			domClass.add(event.currentTarget, "enter");
		},

		onMouseLeaveArrowButton : function(event){
			domClass.remove(event.currentTarget, "enter");
		}
	});

	var MenuTabController = declare("idx.layout.MenuTabController", [ScrollingTabController], {
		/** @lends idx.layout.MenuTabController.prototype */
		buttonWidget: PopupTabButton,
		pages: {},
		
		constructor: function(){
			this._watches = {};
			this._connects = {};
			this._handles = {};
		},

		 /**
		 * Pop up the drop down menu, can be called directly
		 * 
		 * @param pageID
		 *		Point the tab which has the menu
		 * @param e
		 * 		Mouse click event, can be undefined
		 */
		showPagePopup: function(pageID,e) {

		if(pageID in this.pages){

			var page = this.pages[pageID];
			var button = page.controlButton;

			// we need trap events to activate the menu and ensure the menu position is correct
			// otherwise we end up with some odd menu positioning (especially on IE when using
			// the context-menu key).  We do this by trapping the events and temporarily using
			// dojo/aspect:before() on the popup manager -- there three scenarios to handle....
			var showingPopup = false;
			var hoverTimeout = null, exitHoverHandle = null, enterHoverHandle = null, 
				exitTabHandle = null, enterTabHandle = null, cleanupTime = 0;
				
			var cleanupPopup = lang.hitch(this, function(e) {
				cleanupTime = (new Date()).getTime();
				hoverTimeout = null;
				if (exitHoverHandle) exitHoverHandle.remove();
				if (enterHoverHandle) enterHoverHandle.remove();
				if (exitTabHandle) exitTabHandle.remove();
				if (enterTabHandle) enterTabHandle.remove();
				exitHoverHandle = enterHoverHandle = exitTabHandle = enterTabHandle = null;
				if (button === this._currentPopupButton) this._currentPopupButton = null;
				showingPopup = false;			
			});
			
			var closePopup = lang.hitch(this, function() {
				popupMgr.close(popup);
			});

			// clear any hover timeout
			if (hoverTimeout) {
				clearTimeout(hoverTimeout);
				hoverTimeout = null;
			}
			// check if already showing the popup
			if (showingPopup && button === this._currentPopupButton) {
				// already showing, do not move and reshow it
				return null;
			}
			// check if another popup has been shown and this one is a lame duck
			if (this._currentPopupButton && button !== this._currentPopupButton) {
				// complete closing the popup to clean up, but then we will open it
				closePopup();
			}
			
			// avoid reopening just after closing on a click on the tab that closed it
			// if we closed less than 300ms ago, then don't reopen
			var currentTime = (new Date()).getTime();
			if ((currentTime - cleanupTime) < 300) return null;
			
			showingPopup = true;
			// get the new coordinates for the menu
			var coords = {}, titlePos = null, focusPos = null, posX = 0, posY = 0, offsetX = 0, offsetY = 0;
			titlePos = domGeometry.position(button.titleNode);
			focusPos = domGeometry.position(button.focusNode);
			

			if(e){
				posX = ("clientX" in e) ? e.clientX : titlePos.x;
				posY = focusPos.y;
				offsetX = ("clientX" in e) ? (titlePos.x - focusPos.x)
							: (("layerX" in e) && (e.layerX>0))
								? e.layerX - (focusPos.x-titlePos.x): (focusPos.x-titlePos.x);
				offsetY = domGeometry.getMarginBox(button.focusNode).h;
			}
			else{
				// if event 'e' undefined, align dropdown menu 
				posX = titlePos.x;
				offsetX = 0;
				posY = titlePos.y;
				offsetY = titlePos.h;
			}
							
			// get the popup and ensure we have a popup for this page, if not do nothing
			var popup = (page.popup) ? registry.byId(page.popup) : null;
			
			// if we are worrying about left-clicks then use aspect/before to replace
			// positioning parameters
			if (!popup) {
				showingPopup = false;
				return null;
			}
			
			// defect 14340
			// check dir to modify the position
			var ltr = popup.isLeftToRight();
			coords = {x: Math.round(posX + (ltr? offsetX : (-1)*offsetX)), y: Math.round(posY + offsetY)};

			var closeAndRestoreFocus = null;
			var prevFocusNode = null;
			var curFocusNode = null;
			var savedFocusNode = null;
			var elems = null;				
				
			prevFocusNode = this._focusManager.get("prevNode");
			curFocusNode  = this._focusManager.get("curNode");
			savedFocusNode = (!curFocusNode || dom.isDescendant(curFocusNode,popup.domNode)) ? prevFocusNode : curFocusNode;
			closeAndRestoreFocus = lang.hitch(this, function() {
				if (savedFocusNode) {
					savedFocusNode.focus();
				}
				popupMgr.close(popup);
			});
			var result = popupMgr.open({
				popup: popup,
				x: coords.x,
				y: coords.y,
				onExecute: closeAndRestoreFocus,
				onCancel: closeAndRestoreFocus,
				onClose: cleanupPopup,
				orient: popup.isLeftToRight() ? 'L' : 'R'
			});
			this._currentPopupButton = button;
							
			// focus the popup (use defer in case of timing issues with display)
			this.defer(lang.hitch(this, function() {
				if (popup.focus && (typeof popup.focus == "function")) {
					popup.focus();
				} else if (popup.focusNode) {
					this._focusManager.focus(popup.focusNode);
				} else {
					// find the first tab-navigable node and focus it
					elems = a11y._getTabNavigable(popup.containerNode||popup.domNode);
					if (elems.lowest||elems.first) this._focusManager.focus(elems.lowest || elems.first);
				}
			}));
					
			var handle = null;
			if (popup.onBlur && (typeof popup.onBlur == "function")) {
				handle = aspect.after(popup, "onBlur", function() {
					handle.remove();
					popupMgr.close(popup);
				});
			}
			enterHoverHandle = on(popup.domNode, "mouseover", lang.hitch(this, function() {
				if (hoverTimeout) clearTimeout(hoverTimeout);
				hoverTimeout = null;
			}));
			enterTabHandle = on(button.domNode, "mouseover", lang.hitch(this, function() {
				if (hoverTimeout) clearTimeout(hoverTimeout);
				hoverTimeout = null;
			}));
			var autoHide = lang.hitch(this, function() {
				if (hoverTimeout) return;
				if (!popup.openOnHover) return;
				hoverTimeout = setTimeout(closePopup, 800);
			});
			exitHoverHandle = on(popup.domNode, "mouseout", autoHide);
			exitTabHandle = on(button.domNode, "mouseout", autoHide);
			
			return popup;
		}

		},
		
		onAddChild: function(/*dijit._Widget*/ page, /*Integer?*/ insertIndex){
			this.inherited(arguments);

			this.pages[page.id]=page;
			
			// at this point (although it might get overridden later)
			// page.controlButton gives us the button instance we just created
			var button = page.controlButton,
				toggleArrow = function(name, oldpopup, newpopup){
					if(newpopup){
						button.set("arrowButton", true);
					}else{
						button.set("arrowButton", false);
					}
				};

			if (!this._handles[page.id]) this._handles[page.id] = [];
			
			this._handles[page.id].push(
				on(button.domNode, "mouseover", lang.hitch(this, function(e) {
					var container = registry.byId(this.containerId);
					var popup = (page.popup) ? registry.byId(page.popup) : null;
					if (!popup || !popup.openOnHover) return;
					popup = this.showPagePopup(page.id, e);
					if (popup) event.stop(e);
				}))
			);
			
			this._handles[page.id].push(
				on(button.domNode, "click", lang.hitch(this, function(e) {
					var container = registry.byId(this.containerId);
					var popup = null;
					if (button && button.closeNode && e.target === button.closeNode) {
						return;
					}
					if (page === container.selectedChildWidget) popup = this.showPagePopup(page.id, e);
					if (popup) event.stop(e);
				}))
			);

			this._handles[page.id].push(
				on(button.dropdownNode, "click", lang.hitch(this, function(e) {
					var popup = this.showPagePopup(page.id, e);
					if (popup) event.stop(e);
				}))
			);

			this._handles[page.id].push(
				on(button.domNode, "keydown", lang.hitch(this, function(e) {
					var popup = null;
					if (e.keyCode == keys.SPACE || e.keyCode == keys.ENTER) {
						popup = this.showPagePopup(page.id, e);
						if (popup) event.stop(e);
					}
				}))
			);
			
			var popup = (page.popup) ? registry.byId(page.popup) : null;
			toggleArrow(null, null, popup);
			
			// watch() for changes to the 'popup' property on the content pane
			this._watches[page.id] = page.watch("popup", this, toggleArrow);
		},
		
		onRemoveChild: function(/*dijit._Widget*/ page){
			this.inherited(arguments);

			if(this._connects[page.id]){
				this.disconnect(this._connects[page.id]);
				delete this._connects[page.id];
			}
			if (this._handles && this._handles[page.id]) {
				array.forEach(this._handles[page.id], function(handle) {
					handle.remove();
				});
				this._handles[page.id] = null;
				delete this._handles[page.id];
			}
			if(this._watches[page.id]){
				this._watches[page.id].unwatch();
				delete this._watches[page.id];
			}
		}
	});	
	MenuTabController._PopupTabButton = PopupTabButton;
	
	return MenuTabController;	
});