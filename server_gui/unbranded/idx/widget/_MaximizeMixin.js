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
	"dojo/_base/array",
	"dojo/_base/declare",
    "dojo/_base/lang",
    "dojo/dom-construct",
    "dojo/dom-style",
    "dojo/dom-attr",
    "dojo/dom-geometry",
    "dojo/_base/fx",
    "dijit/focus",
    "dijit/_base/focus",
    "../util"
], function (dArray,			// dojo/_base/array
	         dDeclare,			// (dojo/_base/declare)
	         dLang,				// (dojo/_base/lang)
	         dDomConstruct,		// (dojo/dom-construct)
	         dDomStyle,			// (dojo/dom-style) for (dDomStyle.get/set)
	         dDomAttr,			// (dojo/dom-attr) for (dDomAttr.get/set)
	         dDomGeo,			// (dojo/dom-geometry)
	         dFX,				// (dojo/_base/fx)
	         dFocus,			// (dijit/focus)
	         dBaseFocus,		// (dijit/_base/focus)
	         iUtil)				// (../util)
{
	/**
	 * @public
	 * @name idx.widget._MaximizeMixin
	 * @class Mix-in class to provide methods to maximize and restore widget.
	 *	Maximizing and restoring behavior can be animated.
	 */
return dDeclare("idx.widget._MaximizeMixin",null,
		/**@lends idx.widget._MaximizeMixin#*/
{	
		/**
		 * Specifies whether to use animation for transition.
		 * @type Boolean
		 * @default false
		 */
		useAnimation: false,

		/**
		 * Duration of transition.
		 * @type Number
		 * @default 500
		 */
		duration: 500,

		/**
		 * Specifies whether to maximize in place for already absolute positioned nodes.
		 * @type Boolean
		 * @default false
		 */
		inPlace: false,

		_placeHolder: null,
		_maximizedItem: null,
		_anchor: null,

		/**
		 * Maximizes a node to fit the target node
		 * @param {Element} node
		 * @param {Element} target
		 */
		maximize: function(node, target) {
			if(node == this._maximizedItem) {
				return;
			} else if(this._maximizedItem) {
				this.restore();
			}

			var tp = dDomGeo.position(target);
			var np = dDomGeo.position(node);

			var focus = dBaseFocus.getFocus(); // keep focus
			if(!this.inPlace){
				if(!this._placeHolder) {
					this._placeHolder = dDomConstruct.create("DIV");
				}
				if(!this._anchor) {
					this._anchor = dDomConstruct.create("DIV");
					dDomStyle.set(this._anchor, "position", "relative");
					dDomStyle.set(this._anchor, "zIndex", 99999);
					dDomStyle.set(this._anchor, "left", dDomStyle.get(target, "paddingLeft")*(-1)+"px");
					dDomStyle.set(this._anchor, "top", dDomStyle.get(target, "paddingTop")*(-1)+"px");
				}
				dDomConstruct.place(this._placeHolder, node, "before");
				dDomConstruct.place(this._anchor, target.firstChild, "before");
				dDomConstruct.place(node, this._anchor, "last");
			}
			
			//this._cachedOriginalStyle = node.getAttribute("style");
			this._useCSSText = false;
			var cachedStyle = dDomAttr.get(node, "style");
			if ((cachedStyle) && (typeof(cachedStyle) == "object")) {
				// NOTE: we must cache this as text and not as an object
				// since the object is a reference to the node's style and
				// will change.  we need to cache the actual text
				this._useCSSText = true;
				cachedStyle = cachedStyle.cssText;
			}
			this._cachedOriginalStyle = cachedStyle;
			this._cachedOriginalTargetHeight = target.style.height;
			// cache the value of overflow property for the target node
			this._cachedOriginalTargetOverflowProp = dDomStyle.get(target, "overflow");
			//set the value of overflow property to "hidden" for the target to avoid problems(width value) caused by scrollbars
			dDomStyle.set(target,"overflow","hidden");
			dDomStyle.set(target,"height",dDomStyle.get(target, "height")+"px");
			var ele = this._anchor;
			while (ele.nextElementSibling) {
				ele = ele.nextElementSibling;
				ele._cachedDisplayProp = dDomStyle.get(ele, "display") //cache the value of display property
				dDomStyle.set(ele, "display", "none");
			}

			dDomStyle.set(node, "position", "absolute");
			if(this.inPlace){
				dDomStyle.set(node, "zIndex", 99999);
			}

			// the maximum box should include paddings
			var _w = dDomStyle.get(target, "width") + dDomStyle.get(target, "paddingLeft") + dDomStyle.get(target, "paddingRight");
			var _h = dDomStyle.get(target, "height") + dDomStyle.get(target, "paddingTop") + dDomStyle.get(target, "paddingBottom");

			if(this.useAnimation) {
				var t = np.y - tp.y;
				var l = np.x - tp.x;
				var w = np.w;
				var h = np.h;
				this._restoreBox = {t: t, l: l, w: w, h: h};
				dFX.animateProperty({
					node: node,
					duration: this.duration,
					properties: {
						top: {
							start: t, end: 0, unit: "px"
						},left: {
							start: l, end: 0, unit: "px"
						}, width: {
							start: w, end: _w, unit: "px"
						}, height: {
							start: h, end: _h, unit: "px"
						}
					}
				}).play();
			} else {
				dDomStyle.set(node, "top", 0);
				dDomStyle.set(node, "left", 0);
				dDomStyle.set(node, "width", _w+"px");
				dDomStyle.set(node, "height", _h+"px");
			}
			
			this._maximizedItem = node;
			dFocus.focus(focus); // restore focus
		},

		/**
		 * Restores the maximized node.
		 */
		restore: function() {
			if(this._maximizedItem){
				if(this.useAnimation && this._restoreBox){
					var node = this._maximizedItem;
					var t = dDomStyle.get(node, "top");
					var l = dDomStyle.get(node, "left");
					var w = dDomStyle.get(node, "width");
					var h = dDomStyle.get(node, "height");
					var restoreBox = this._restoreBox;
					dFX.animateProperty({
						node: node,
						duration: this.duration,
						properties: {
							top: {
								start: t, end: restoreBox.t, unit: "px"
							},left: {
								start: l, end: restoreBox.l, unit: "px"
							}, width: {
								start: w, end: restoreBox.w, unit: "px"
							}, height: {
								start: h, end: restoreBox.h, unit: "px"
							}
						},
						onEnd: dLang.hitch(this, this._restore)
					}).play();
				}else{
					this._restore();
				}
			}
		},

		_restore: function(){
			if(this._maximizedItem){
				var focus = dBaseFocus.getFocus(); // keep focus
				if(!this.inPlace){
					dDomConstruct.place(this._maximizedItem, this._placeHolder, "before");
				}
				if(iUtil.isWebKit){ // removing style attribute seems not working on WebKit
					var node = this._maximizedItem;
					dDomStyle.set(node, "position", "");
					dDomStyle.set(node, "top", "");
					dDomStyle.set(node, "left", "");
					dDomStyle.set(node, "width", "");
					dDomStyle.set(node, "height", "");
				}else{
					this._maximizedItem.removeAttribute("style");
				}
				if(this._cachedOriginalStyle) {
					if (this._useCSSText) {
						this._maximizedItem.style.cssText = this._cachedOriginalStyle;
					} else {
						dDomAttr.set(this._maximizedItem, "style", this._cachedOriginalStyle);
					}
				}
				if(!this.inPlace){
					this._maximizedItem.parentNode.removeChild(this._placeHolder);
					this._anchor.parentNode.removeChild(this._anchor);
				}
				dDomStyle.set(this._maximizedItem.parentNode,"overflow",this._cachedOriginalTargetOverflowProp); //restore overflow property
				this._maximizedItem.parentNode.style.height = this._cachedOriginalTargetHeight; //restore height
				dArray.forEach(this._maximizedItem.parentNode.childNodes, function(item) {
					if (item._cachedDisplayProp !== undefined) {
						dDomStyle.set(item, "display", item._cachedDisplayProp); //restore the value of display property
						delete item._cachedDisplayProp;
					}
				});

				this._maximizedItem = null;
				dFocus.focus(focus); // restore focus
			}
		}
});

});
