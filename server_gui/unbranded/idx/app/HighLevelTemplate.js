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
	"dojo/_base/array",
	"dojo/_base/lang",
	"dojo/dom-class",
	"dojo/dom-attr",
	"dojo/dom-construct",
	"dojo/dom-style",
	"dijit/registry",
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/_Widget",
	"idx/layout/ToggleSplitter"
], function(declare, array, lang, domClass, domAttr, domConstruct, domStyle, registry, BorderContainer, ContentPane, _Widget){
	var iLayout = lang.getObject("idx.oneui.layout", true); // for IDX 1.2 naming compatibility
	
	/**
	* @name idx.app.HighLevelTemplate
	* @class The HighLevelTemplate provides the standard OneUI page architecture, implemented according to 
	* IBM One UI(tm) standard and specification <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y4&vsub=*&hsub=*">High Level Templates</a></b>
	* @augments dijit.layout.BorderContainer
	* @example
&lt;!-- HighLevelTemplate template 1: with left column and right column -->
&lt;div id="hlt1" data-dojo-type="idx.app.HighLevelTemplate" data-dojo-props="
	style: 'width: 100%; height: 100%;', header: 'header1'"&gt;	
	
	&lt;!-- Left Nav start --&gt;
	&lt;div data-dojo-type="dijit.layout.AccordionContainer" data-dojo-props="
		splitter: 'toggle',
		minSize: 20,
		style: 'width: 20%;',
		region: 'leading'
	"&gt;
		&lt;div data-dojo-type="dijit.layout.ContentPane" data-dojo-props="
			title: 'Navigation',
			style: 'background-color: #dbdbdb;'"&gt;
			&lt;div class="content"&gt;left column&lt;br/&gt;(optional)&lt;/div&gt;
		&lt;/div&gt;
	&lt;/div&gt;
	&lt;!-- Left Nav end --&gt;
	
	&lt;!-- Main Content start --&gt;
	&lt;div data-dojo-type="dijit.layout.ContentPane" data-dojo-props="
		style: 'padding: 10px;background-color:transparent;',
		region: 'center'
	"&gt;
		&lt;div class="centeredContent"&gt;content area&lt;br/&gt;(required)&lt;/div&gt;
	&lt;/div&gt;
	&lt;!-- Main Content end --&gt;
	
	&lt;!-- Right Column start --&gt;
	&lt;div data-dojo-type="dijit.layout.ContentPane" data-dojo-props="
		splitter: 'drag',
		style: 'background-color: #dbdbdb; width: 15%;',
		region: 'trailing'
	"&gt;
		&lt;div class="centeredContent"&gt;right column&lt;br/&gt;(optional)&lt;/div&gt;
	&lt;/div&gt;
	&lt;!-- Right Column end --&gt;
	
	&lt;!-- Footer start --&gt;
	&lt;div data-dojo-type="dijit.layout.ContentPane" data-dojo-props="
		splitter: 'toggle',
		style: 'background-color: #dbdbdb; height: 10%;',
		region: 'bottom'
	"&gt;
		&lt;div class="content" &gt;footer&lt;br/&gt;(optional)&lt;/div&gt;
	&lt;/div&gt;
	&lt;!-- Footer end --&gt;
&lt;/div&gt;
	*/ 
	iLayout.HighLevelTemplate = declare("idx.app.HighLevelTemplate", [BorderContainer], {
	/**@lends idx.app.HighLevelTemplate.prototype*/ 
		gutters: false,
		/**
		 * The id or the widget itself of idx.app.Header, The header would be the header of HighLevelTemplate container
		 * @type String | idx.app.Header
		 */
		header: null,
		
		_regionRoleMap: {
			"top": "banner",
			"center": "main",
			"leading": "navigation",
			"trailing": "complementary",
			"bottom": "contentinfo"
		},
		
		buildRendering: function(){
			this.inherited(arguments);
			if(this.header){
				var header = registry.byId(this.header);
				if(header && (header.declaredClass == "idx.app.Header")){
					domConstruct.place(header.domNode, this.domNode, "first");
					if(!header.region){
						// if the header doesn't set a region, default to "top"
						header.set("region", "top");
					}
				}
			}		
		},
		
		startup: function(){
			if(this._started){ return; }
			
			
			// make sure there are top&center for the layout widget.
			var topRegion, centerRegion, domNode  = this.domNode;
			
			array.forEach(this.getChildren(), function(child){
				if(child && child.region == "top"){
					// CHANGE BY ROCK: Only one top content is accepted in HLT by now.
					if(child.declaredClass == "idx.app.Header"){
						topRegion = child;
						domStyle.set(domNode, "minWidth", "980px");
					}else{
						child.destroyRecursive && child.destroyRecursive();
					}
				}else if(child && child.region == "center"){
					centerRegion = child;
				}
			});
			
			if(!topRegion){
				// Top region is required so we need to add one
				// and make some defaults
				topRegion = new ContentPane({
					region: "top",
					style: "height: 75px"
				});
				this.addChild(topRegion);
			}
			if(!centerRegion){
				// Center region is required so we need to add one
				centerRegion = new ContentPane({
					region: "center"
				});
				this.addChild(centerRegion);
			}
			
			this.inherited(arguments);
		},
		
		_setupChild: function(/*dijit._Widget*/ child){
			// Override _LayoutWidget._setupChild().
			var region = child.region;
			if(region){
				dijit.layout._LayoutWidget.prototype._setupChild.apply(this, arguments);
				domClass.add(child.domNode, this.baseClass+"Pane");
				domAttr.set(child.domNode, "role", this._regionRoleMap[region]);
				var ltr = this.isLeftToRight();
				if(region == "leading"){ region = ltr ? "left" : "right"; }
				if(region == "trailing"){ region = ltr ? "right" : "left"; }
	
				// Create draggable splitter or toggle splitter for resizing pane,
				// or alternately if splitter=false but gutters=true then
				// insert dummy div just for spacing
				if((child.splitter || child.gutter || this.gutters) && !child._splitterWidget){
					var _Splitter = lang.getObject(
						child.splitter ? 
						(child.splitter == "toggle" ? "idx.layout.ToggleSplitter" : "dijit.layout._Splitter") :
						 "dijit.layout._Gutter");
					var splitter = new _Splitter({
						id: child.id + "_splitter",
						container: this,
						child: child,
						region: region,
						live: this.liveSplitters
					});
					splitter.isSplitter = true;
					child._splitterWidget = splitter;
	
					domConstruct.place(splitter.domNode, child.domNode, "after");
	
					// Splitters aren't added as Contained children, so we need to call startup explicitly
					splitter.startup();
				}
				child.region = region;
			}
		}
	});
	
	return iLayout.HighLevelTemplate;
});
