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
"dojo/_base/array",
"dojo/_base/lang",
"dojo/sniff",
"dojo/dom-geometry", 
"dojo/dom-style", 
"dojo/window",
"dojo/topic",
"idx/widget/ResizeHandle"
],function(declare, array, lang, has, domGeometry, domStyle, winUtils, topic, ResizeHandle){
	return declare("idx.widget._DialogResizeMixin", null, {
		resizable: false,
		/**
		 * Override to better handle zooming for accessiblity and to properly resize DOM node.
		 * NOTE: most of this code is copied from the Dojo 1.9.3 implementation and then modified.
		 */
		_size: function(){
			var bb = domGeometry.position(this.domNode);

			// Get viewport size but then reduce it by a bit; Dialog should always have some space around it
			// to indicate that it's a popup.  This will also compensate for possible scrollbars on viewport.
			var viewport = winUtils.getBox(this.ownerDocument);
			//var viewport ={w: window.innerWidth, h: window.innerHeight};
			viewport.w *= this.maxRatio;
			viewport.h *= this.maxRatio;
			if(bb.w >= viewport.w || bb.h >= viewport.h){
				// record the preferred width and height
				if (! ("_preferredWidth" in this)) {
					this._preferredWidth = bb.w;
				}
				if (! ("_preferredHeight" in this)) {
					this._preferredHeight = bb.h;
				}
				// Reduce size of dialog contents so that dialog fits in viewport
				var padBorderSize = domGeometry.getPadBorderExtents(this.domNode),
					w = Math.min(bb.w, viewport.w) - padBorderSize.w,
					h = Math.min(bb.h, viewport.h) - padBorderSize.h;
				var hasOverflow = (w < this._preferredWidth) || (h < this._preferredHeight);
				
				domStyle.set(this.domNode, {
					width: w + "px",
					height: h + "px",
					overflow: hasOverflow ? "auto" : "hidden"
				});
			}else{
				if ((this._preferredWidth && this._preferredWidth > bb.w) 
					||(this._preferredHeight && this._preferredHeight > bb.h)) {
					// try to enlarge the dialog back to its original dimensions
					var padBorderSize = domGeometry.getPadBorderExtents(this.domNode),
						w = Math.min(viewport.w,this._preferredWidth) - padBorderSize.w,
						h = Math.min(viewport.h,this._preferredHeight) - padBorderSize.h;
					
					var hasOverflow = (w < this._preferredWidth) || (h < this._preferredHeight);
				
					domStyle.set(this.domNode, {
						width: w + "px",
						height: h + "px",
						overflow: hasOverflow ? "auto" : "hidden"
					});
				} else {
					//resize the Dialog to wrap the content
					var children = this.containerNode.children,
						innerWidth = 0;
					array.forEach(children, function(child){
						innerWidth = Math.max(domStyle.get(child, "width"), innerWidth);
					});
					if(innerWidth > domStyle.get(this.containerNode, "width")){
						domStyle.set(this.domNode, {
							width:"auto"
						});
						domStyle.set(this.containerNode, {
							width:"auto",
							height:"auto"
						})
					}
				}
			}
		},
		resize: function(dim){
			if(dim){
				if(!domGeometry.isBodyLtr()){
					var dialogLeft = this._resizeHandle.startPosition.l + this._resizeHandle.startSize.w - dim.w + "px";
					domStyle.set(this.domNode, "left", dialogLeft);
				}
				domStyle.set(this.domNode, {
					"height": dim.h + "px",
					"width": dim.w + "px"
				})
				var containerHeight = domGeometry.getContentBox(this.contentWrapper).h;
				domStyle.set(this.contentWrapper, "height", 
					containerHeight + (dim.h - this._resizeHandle.startSize.h) + "px");
				
			}else if(this.domNode.style.display != "none"){
				this._size();
				if(!has("touch")){
					this._position();
				}
			}
		},
		_setResizableAttr: function(resizable){
			if(!this._resizeHandle && this.resizable){
				
				this._resizeHandle = new ResizeHandle({
					targetId: this.id,
					resizeAxis: "xy",
					startTopic: this.id + "_ResizingStart"
				})
				this._resizeHandle.placeAt(this.domNode);
				var resizeSubscription = topic.subscribe(this.id + "_ResizingStart",lang.hitch(this, function(resizeHandle){
					var box = domGeometry.getMarginBox(this.domNode);
					this._minSize = {
						w: box.w,
						h: box.h
					}
					resizeHandle.minSize = this._minSize;
					resizeSubscription.remove();
				}));
			}if(this._resizeHandle){
				domStyle.set(this._resizeHandle.domNode, "display", resizable ? "block" : "none");
			}
			this._set("resizable", resizable);
		}
		
	})
	
});

