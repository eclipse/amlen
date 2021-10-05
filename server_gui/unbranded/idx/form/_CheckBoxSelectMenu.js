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
	"dojo/_base/declare",
	"dojo/dom-class",
	"dojo/dom-construct",
	"dojo/dom-geometry",
	"dojo/dom-attr",
	"idx/widget/Menu"
],function(declare, domClass, domConstruct, domGemometry, domAttr, Menu){
	
	//	module:
	//		idx/form/_CheckBoxSelectMenu
	//	summary:
	//		An internally-used menu for CheckBoxSelect
	
	return declare("idx.form._CheckBoxSelectMenu", Menu, {
		// summary:
		//		An internally-used menu for CheckBoxSelect
		autoFocus: true,
		
		buildRendering: function(){
			// summary:
			//		Stub in our own changes, so that our domNode is not a table
			//		otherwise, we won't respond correctly to heights/overflows
			this.inherited(arguments);
			var o = (this.menuTableNode = this.domNode),
			n = (this.domNode = domConstruct.create("div", {style: {overflowX: "hidden", overflowY: "auto"}}));
			if(o.parentNode){
				o.parentNode.replaceChild(n, o);
			}
			domClass.remove(o, "dijitMenuTable");
			n.className = o.className + " idxCheckBoxSelectMenu";
			o.className = "dijitReset dijitMenuTable";
			domAttr.set(n, "role", "presentation");
			n.appendChild(o);
		},
		
		resize: function(/*Object*/ mb){
			// summary:
			//		Overridden so that we are able to handle resizing our
			//		internal widget.  Note that this is not a "full" resize
			//		implementation - it only works correctly if you pass it a
			//		marginBox.
			//
			// mb: Object
			//		The margin box to set this dropdown to.
			
			if(mb){
				domGemometry.setMarginBox(this.domNode, mb);
				if("w" in mb){
					// We've explicitly set the wrapper <div>'s width, so set <table> width to match.
					// 100% is safer than a pixel value because there may be a scroll bar with
					// browser/OS specific width.
					this.menuTableNode.style.width = "100%";
				}
			}
		},
		
		onItemClick: function(/*dijit._Widget*/ item, /*Event*/ evt){
			// summary:
			//		Handle clicks on an item.
			// tags:
			//		private
			// this can't be done in _onFocus since the _onFocus events occurs asynchronously
			if(typeof this.isShowingNow == 'undefined'){ // non-popup menu
				this._markActive();
			}
			
			this.focusChild(item);
			
			if(item.disabled || item.readOnly){ return false; }
			
			// user defined handler for click
			if(item.popup){
				this._openPopup(evt.type == "keypress");
			}else{
				// user defined handler for click
				item._onClick ? item._onClick(evt) : item.onClick(evt);
			}
		}
	});
});

