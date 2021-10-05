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
	"dojo/_base/declare",
	"dojo/_base/html",
	"dojo/_base/event",
	"dojo/keys",
	"dojox/layout/ResizeHandle",
	"idx/resources",
	"idx/widgets"
], function(dojo_declare, dojo_html, dojo_event, dojo_keys, dojox_layout_ResizeHandle, idx_resources){

/**
 * @name idx.widget.ResizeHandle
 * @class Resize handle with dynamic axis change. "axis" attribute can be changed after creation.
 * 	Note that this class does not work correctly on RtoL environment due to an issue in its superclass.
 * @augments dojox.layout.ResizeHandle
 */
return dojo_declare("idx.widget.ResizeHandle", dojox_layout_ResizeHandle,
/**@lends idx.widget.ResizeHandle#*/
{
	/**
	 * Add the IDX base class.
	 */
	idxBaseClass: "idxResizeHandle",
	
	/**
	 * Installs key event hander for resizing.
	 * @private as part of widget life cycle
	 */
	postCreate: function(){
		this.inherited(arguments);

		var resources = idx_resources.getResources("idx/widget/ResizeHandle", this.lang);
		var title = resources.idxResizeHandle_resize;
		var a11yText = this.isLeftToRight() ? "&#x2921;" : "&#x2922;";
		dojo_html.attr((this.resizeHandle.firstChild || this.resizeHandle), "innerHTML", "<span class='idxResizeHandleText' title='" + title + "'>" + a11yText + "</span>"); // a11y text
		dojo_html.attr(this.resizeHandle, {
			"tabindex": 0,
			title: title
		});
		this.connect(this.resizeHandle, "keypress", this._onKeyPress);
	},

	/**
	 * Handles key event (Shift+arrow keys) for resizing.
	 * @param {Object} event
	 * @private
	 */
	_onKeyPress: function(event){
		if(event && event.shiftKey){
			var key = event.keyCode;
			var keys = dojo_keys;
			if((this._resizeX && (key == keys.LEFT_ARROW || key == keys.RIGHT_ARROW)) ||
					(this._resizeY && (key == keys.UP_ARROW || key == keys.DOWN_ARROW))){
				this.targetWidget = dijit.byId(this.targetId);
				this.targetDomNode = (this.targetContainer || (this.targetWidget ? this.targetWidget.domNode : dojo_html.byId(this.targetId)));
				if(this.targetDomNode){
					var box = (this.targetWidget ? dojo_html.marginBox(this.targetDomNode) : dojo_html.contentBox(this.targetDomNode));  
					this.startSize = {w: box.w, h: box.h};
					this._changeSizing(event);
					if(!this.intermediateChanges){
						this.onResize(event);
					}
					dojo_event.stop(event);
				}
			}
		}
	},

	/**
	 * Calculates new size for resizing by key.
	 * @private
	 */
	_getNewCoords: function(event){
		if(event && event.keyCode){
			var key = event.keyCode;
			var keys = dojo_keys;
			var w = this.startSize.w;
			var h = this.startSize.h;
			var d = 10;
			if(key == keys.UP_ARROW){
				h -= d;
			}else if(key == keys.DOWN_ARROW){
				h += d;
			}else if(key == keys.LEFT_ARROW){
				if(this.isLeftToRight()){
					w -= d;
				}else{
					w += d;
				}
			}else if(key == keys.RIGHT_ARROW){
				if(this.isLeftToRight()){
					w += d;
				}else{
					w -= d;
				}
			}
			return this._checkConstraints(w, h);
		}

		return this.inherited(arguments);
	},

	/**
	 * Changes axis attribute and update class for the new axis.
	 * @param {String} axis
	 * @private
	 */
	_setResizeAxisAttr: function(axis) {
		dojo_html.removeClass(this.resizeHandle, this._getResizeClass());
		this._resizeAxis = axis;
		this._resizeX = (axis.indexOf("x") > -1);
		this._resizeY = (axis.indexOf("y") > -1);
		dojo.addClass(this.resizeHandle, this._getResizeClass());
	},

	/**
	 * Determines CSS class for the current axis.
	 * This is called when axis attribute is changed.
	 * @private
	 */
	_getResizeClass: function() {
		var axis = this._resizeAxis;
		var cls;
		if (axis == "xy") {
			cls = "dojoxResizeNW";
		} else if (axis == "x") {
			cls = "dojoxResizeW";
		} else if (axis == "y") {
			cls = "dojoxResizeN"
		} else {
			cls = "";
		}
		return cls;
	}
});

});
