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
        "idx/main",
        "dojo/dom",	
        "dojo/dom-class",			
        "dijit/_WidgetBase",
        "dijit/_base/manager",
        "./string", 
        "./util"],
        function(dLang,			// (dojo/_base/lang)
				 iMain,			// (idx)
		 		 dDom, 			// (dojo/dom) for (dDom.byId)
				 dDomClass, 	// (dojo/dom-class) for (dDomClass.add/remove)
				 dWidget,		// (dijit/_WidgetBase)
				 dijitMgr,		// (dijit/_base/manager)
				 iString,		// (idx/string)
				 iUtil) 		// (idx/util)
{
	/**
 	 * @name idx.widgets
 	 * @class Provides extensions for all descendants of dijit._WidgetBase.
 	 */
	var iWidgets = dLang.getObject("widgets", true, iMain);
	
	dLang.extend(dWidget, 
	/**@lends idx.widgets#*/{	
	/**
	 * Used for when the the default dijit base class needs to be left 
	 * unmodified, but a child widget or extension needs its a separate
	 * base class with an "idx" prefix. 
	 *
	 * @field
	 * @type String
	 * @default ""
	 */
	idxBaseClass: "",
	
	/**
	 * A comma-separated list of additional CSS class names to add to the DOM node
	 * of this widget.
	 * @field
	 * @type String
	 * @default ""
	 */
	idxExtraClasses: "",
	
	/**
	 * The explicit CSS class to be used for finding the CSS defaults for
	 * the widget.
	 *
	 * @field
	 * @type String
	 * @default ""
	 */
	idxDefaultsClass: "",
	
	/**
	 * The CSS defaults to use of high-contrast mode is detected (i.e.: no background image found).
	 * This can be an Object containing the properties or a String containing a query string.
	 * @field
	 * @type Object|String
	 * @default null
	 */
	idxHCDefaults: null,
	
	/**
	 * The child class to be applied to children.  This
	 * should not contain spaces or commas and should be
	 * a single CSS class. 
	 *
	 * @field
	 * @type String
	 * @default ""
	 */
	idxChildClass: "",

	/**
	 * The arguments to directly mixin.  This allows you to specify references
	 * to other objects for string properties if you like.  These will get
	 * automatically mixed through the postMixInProperties() implementation, 
	 * so if you override that function make sure to call the inherited 
	 * version before doing other work.
	 *
	 * @field
	 * @type Object
	 * @default null
	 */
	mixinArgs: null,
	
	/**
	 * Called by all widgets prior to the "postMixinProperties" function.
	 * This allows for "postMixinProperties" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @function
	 * @public
	 */
	idxBeforePostMixInProperties: function() {
		// store the original mixin args
		var origMixInArgs = this.mixinArgs;
		
	    // we use a while loop in case the mixin args contain another "mixinArgs"
	    // in such a case there may be a nested mixinArgs 
	    while (this.mixinArgs != null) {
	    	// copy the reference
	    	var args = this.mixinArgs;

	    	// clear the reference
	    	this.mixinArgs = null;

	    	// perform the mixin
	    	dLang.mixin(this, args);
	    	if (! this.params) this.params = {};
	    	dLang.mixin(this.params, args); 
	    }
	     
	    // reset the mixin args back to original settings
	    this.mixinArgs = origMixInArgs;
	    if (this.params) {
	    	this.params.mixinArgs = origMixInArgs;
	    } else {
	    	this.params = { mixinArgs: origMixInArgs };
	    }		
	},
	
	/**
	 * Called by all widgets aftr the "postMixinProperties" function.
	 * This allows for "postMixinProperties" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @function
	 * @public
	 */
	idxAfterPostMixInProperties: function() {
		// do nothing
	},
	
	/**
	 * Called by all widgets prior to the "buildRendering" function.
	 * This allows for "buildRendering" to be overridden while still
	 * maintaining the call to this function.
	 * 
	 * @function
	 * @public
	 */
	idxBeforeBuildRendering: function() {
		// do nothing
	},
	
	/**
	 * Called by all widgets after the "buildRendering" function.
	 * This allows for "buildRendering" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @public
	 * @function
	 */
	idxAfterBuildRendering: function() {
		// apply the IDX base class
		if ((this.domNode) && (iString.nullTrim(this.idxBaseClass))) {
			dDomClass.add(this.domNode, this.idxBaseClass);
		}
		if ((this.domNode) && (iString.nullTrim(this.idxExtraClasses))) {
			var classes = this.idxExtraClasses.split(",");
			for (var index = 0; index < classes.length; index++) {
				var cssClass = classes[index];
				dDomClass.add(this.domNode, cssClass);
			}
		}
	},
	
	/**
	 * Called by all widgets prior to the "postCreate" function.
	 * This allows for "postCreate" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @public
	 * @function
	 */
	idxBeforePostCreate: function() {
		// do nothing
	},
	
	/**
	 * Called by all widgets after the "postCreate" function.
	 * This allows for "postCreate" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @public
	 * @function
	 */
	idxAfterPostCreate: function() {
		// do nothing
	},
	
	/**
	 * Called by all widgets prior to the "startup" function.
	 * This allows for "startup" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @public
	 * @function
	 */
	idxBeforeStartup: function() {
		// do nothing
	},
	
	/**
	 * Called by all widgets after the "startup" function.
	 * This allows for "startup" to be overridden while still
	 * maintaining the call to this function.
	 *
	 * @public
	 * @function
	 */
	idxAfterStartup: function() {
    	var children = this.getChildren();
		
    	// get child nodes that are part of this widget and style them
    	if ((this.domNode) && (iString.nullTrim(this.idxChildClass))) {
    		this._idxStyleChildNodes(this.idxChildClass, this.domNode);
    	}
		
    	// now style the children
    	this._idxStyleChildren();
    },
	
	/**
	 * Private function that is used to replace "postMixInProperties"
	 * in the constructor so that we can ensure the "before" and "after"
	 * functions get called before and after the defined "postMixInProperties"
	 * function.
	 *
	 * @private
	 */
	_idxWidgetPostMixInProperties: function() {
		// ensure we don't clobber the postMixInProperties() function
		if (! ("_idxWidgetOrigPostMixInProperties" in this)) return;
		if (! this._idxWidgetOrigPostMixInProperties) return;
		
		// restore the postMixInProperties() function to its original state
		this.postMixInProperties = this._idxWidgetOrigPostMixInProperties;
		
	    // call the "before" function
	    this.idxBeforePostMixInProperties();
	    
		// proceed with normal postMixInProperties()
		this.postMixInProperties();
		
	    // call the "after" function
	    this.idxAfterPostMixInProperties();
	},
	
	/**
	 * Private function that is used to replace "buildRendering"
	 * in the constructor so that we can ensure the "before" and "after"
	 * functions get called before and after the defined "buildRendering"
	 * function.
	 *
	 * @private
	 */
	_idxWidgetBuildRendering: function() {
		// ensure we don't clobber the buildRendering() function
		if (! ("_idxWidgetOrigBuildRendering" in this)) return;
		if (! this._idxWidgetOrigBuildRendering) return;
		
		// restore the buildRendering() function to its original state
		this.buildRendering = this._idxWidgetOrigBuildRendering;
		
		// call the "before" function
		this.idxBeforeBuildRendering();
		
		// proceed with normal buildRendering()
		this.buildRendering();
		
		// call the "after" function
		this.idxAfterBuildRendering();
	},
	
	/**
	 * Private function that is used to replace "postCreate"
	 * in the constructor so that we can ensure the "before" and "after"
	 * functions get called before and after the defined "postCreate"
	 * function.
	 *
	 * @private
	 */
	_idxWidgetPostCreate: function() {
		// ensure we don't clobber the postCreate() function
		if (! ("_idxWidgetOrigPostCreate" in this)) return;
		if (! this._idxWidgetOrigPostCreate) return;
		
		// restore the postCreate() function to its original state
		this.postCreate = this._idxWidgetOrigPostCreate;
		
		// call the before function
		this.idxBeforePostCreate();
		
		// proceed with normal postCreate()
		this.postCreate();
		
		// call the after function
		this.idxAfterPostCreate();
	},

	/**
	 * Private function that is used to replace "startup"
	 * in the constructor so that we can ensure the "before" and "after"
	 * functions get called before and after the defined "startup"
	 * function.
	 *
	 * @private
	 */	
	_idxWidgetStartup: function() {
		// ensure we don't clobber the startup() function
		if (! ("_idxWidgetOrigStartup" in this)) return;
		if (! this._idxWidgetOrigStartup) return;
		
		// restore the startup() function to its original state
		this.startup = this._idxWidgetOrigStartup;
		
		// call the before function
		this.idxBeforeStartup();
		
		// proceed with normal startup
		this.startup();
		
		// call the after function
		this.idxAfterStartup();
	},
	
	/**
	 * The function for applying classes to the child widgets if the
	 * idxChildClass attribute is non-empty.
	 *
	 * @private
	 */
	_idxStyleChildren: function() {
		if (! iString.nullTrim(this.idxChildClass)) return;
		if (! iString.nullTrim(this.baseClass)) return;

		// find all children that were previously styled
		var prevChildren = this._idxPrevStyledChildren;
		if ((prevChildren) && (prevChildren.length > 0)) {
			var childBase = this._idxPrevChildBase;
			var childClass = childBase + "-idxChild";
			var firstChildClass = childBase + "-idxFirstChild";
			var midChildClass = childBase + "-idxMiddleChild";
			var lastChildClass = childBase + "-idxLastChild";
			var onlyChildClass = childBase + "-idxOnlyChild";
			
			// loop through the previously styled children
			for (var index = 0; index < prevChildren.length; index++) {
				var child = prevChildren[index];
				
				if (! child.domNode) continue;
				if (child._idxUnstyleChildNodes) {
					child._idxUnstyleChildNodes(child.domNode, childBase);
				} else {
					dRemoveClass(child.domNode, childClass);
					dRemoveClass(child.domNode, firstChildClass);
					dRemoveClass(child.domNode, midChildClass);
					dRemoveClass(child.domNode, lastChildClass);
					dRemoveClass(child.domNode, onlyChildClass);
				}
			}
		}
		
		// clear the previous settings
		this._idxPrevStyledChildren = null;
		this._idxPrevChildBase = null;
		
		
		// get the children currently in the container node in order
		var index = 0;
		var children = this.getChildren();
    	if ((children) && (children.length > 0)) {
    		// create the prefix for the child class
    		var childBase = this.baseClass + "-" + this.idxChildClass;
    		var childClass = childBase + "-idxChild";
    		var firstChildClass = childBase + "-idxFirstChild";
    		var midChildClass = childBase + "-idxMiddleChild";
    		var lastChildClass = childBase + "-idxLastChild";
    		var onlyChildClass = childBase + "-idxOnlyChild";
    		
    		// set up the variables for next time to unstyle
    		this._idxPrevStyledChildren = [ ];
    		this._idxPrevChildBase = childBase;

    		// setup the classes
    		for (index = 0; index < children.length; index++) {
    			var child = children[index];
			
    			// check for a dom node
    			if (! child.domNode) continue;
    			this._idxPrevStyledChildren.push(child);
    			
    			// add the appropriate classes to the children
    			if (child._idxStyleChildNodes) child._idxStyleChildNodes(childClass, child.domNode);
    			else dDomClass.add(child.domNode, childClass);

    			if (index == 0) {
    				if (child._idxStyleChildNodes) child._idxStyleChildNodes(firstChildClass, child.domNode);
    				else dDomClass.add(child.domNode, firstChildClass); 
    			}

    			if ((index > 0) && (index < (children.length - 1))) {
    				if (child._idxStyleChildNodes) child._idxStyleChildNodes(midChildClass, child.domNode);
    				else dDomClass.add(child.domNode, midChildClass);
    			}

    			if (index == (children.length - 1)) {
    				if (child._idxStyleChildNodes) child._idxStyleChildNodes(lastChildClass, child.domNode);
    				else dDomClass.add(child.domNode, lastChildClass);
    			}

    			if (children.length == 1) {
    				if (child._idxStyleChildNodes) child._idxStyleChildNodes(onlyChildClass, child.domNode);
    				else dDomClass.add(child.domNode, onlyChildClass);
    			}
    			
    		}
    	}
    	
	},
	
	/**
	 * The function for removing previously applied classes to the child 
	 * widgets if the idxChildClass attribute is non-empty.
	 * 
	 * @private
	 */
	_idxUnstyleChildNodes: function(rootNode, childBase) {
		if (!rootNode) rootNode = this.domNode;
		if (!rootNode) return;
		
		var childClass = childBase + "-idxChild";
		var firstChildClass = childBase + "-idxFirstChild";
		var midChildClass = childBase + "-idxMiddleChild";
		var lastChildClass = childBase + "-idxLastChild";
		var onlyChildClass = childBase + "-idxOnlyChild";
		
		dRemoveClass(rootNode, childClass);
		dRemoveClass(rootNode, firstChildClass);
		dRemoveClass(rootNode, midChildClass);
		dRemoveClass(rootNode, lastChildClass);
		dRemoveClass(rootNode, onlyChildClass);
		
		var childNodes = rootNode.childNodes;
		if (!childNodes) return;
		for (var index = 0; index < childNodes.length; index++) {
			var childNode = childNodes[index];
			if (dijitMgr.getEnclosingWidget(childNode) == this) {
				this._idxUnstyleChildNodes(childNode, childBase);
			}
		}
	},
	
	/**
	 * The function that styles the nodes under the specified root
	 * using the specified "child class".  This is called for this
	 * widget's nodes but with different classes for the nodes of
	 * the children of this widget.
	 *
	 * @private
	 */
	_idxStyleChildNodes: function(childClass, rootNode) {
		if (!rootNode) rootNode = this.domNode;
		if (!rootNode) return;
		dDomClass.add(rootNode, childClass);
		var childNodes = rootNode.childNodes;
		if (!childNodes) return;
		for (var index = 0; index < childNodes.length; index++) {
			var childNode = childNodes[index];
			if (childNode.nodeType != 1) continue;
			if (dijitMgr.getEnclosingWidget(childNode) == this) {
				this._idxStyleChildNodes(childClass, childNode);
			}
		}
	}
	
	});

	// get the base prototype
    var widgetProto  = dWidget.prototype;
    
	// 
	// Get the base functions so we can call them from our overrides
	//
	var baseCreate = widgetProto.create;

	/**
	 * Replaces dijit._WidgetBase.create() function with one that calls the
	 * function defined by dijit._WidgetBase, but not before replacing all
	 * lifecycle methods with ones that gurantee calling of the "before" and
	 * "after" methods.  This version of "create()" will also perform argument
	 * mixin with defaults determined by the CSS theme via the "idxDefaultsClass"
	 * or the defaults class derived from the "idxBaseClass".
	 *
	 * @name idx.widgets#create
	 * @public
	 *
	 * @param params The parameters passed for widget creation.
	 * @param srcNodeRef The optional node reference for creating the widget.
	 */
	widgetProto.create = function(params,srcNodeRef) {
		var defaultsBase = "";
		var needsSuffix  = false;
		if (params) {
			if (params.idxDefaultsClass) {
				defaultsBase = params.idxDefaultsClass;
				defaultsBase = iString.nullTrim(defaultsBase);
				needsSuffix = false;
			}
			if ((! defaultsBase) && params.idxBaseClass) {
				defaultsBase = params.idxBaseClass;
				defaultsBase = iString.nullTrim(defaultsBase);
				needsSuffix = true;
			}
			if ((! defaultsBase) && params.baseClass) {
				defaultsBase = params.baseClass;
				defaultsBase = iString.nullTrim(defaultsBase);
				needsSuffix = true;
			}
		}
		if (! defaultsBase) {
			defaultsBase = iString.nullTrim(this.idxDefaultsClass);
			needsSuffix = false;
		}
		if (! defaultsBase) {
			defaultsBase = iString.nullTrim(this.idxBaseClass);
			needsSuffix = true;
		}
		if (! defaultsBase) {
			defaultsBase = iString.nullTrim(this.baseClass);
			needsSuffix = true;
		}
		if ((defaultsBase) && (srcNodeRef)) {
			// check if we need a suffix
			if (needsSuffix) defaultsBase = defaultsBase + "_idxDefaults";
			// get the CSS defaults - make sure to pass in a DOM node and not just an id
			var cssDefaults = iUtil.getCSSOptions(defaultsBase, dDom.byId(srcNodeRef), this, this.idxHCDefaults);
			if (cssDefaults != null) {
				for (var field in cssDefaults) {
					if (! (field in params)) {
						params[field] = cssDefaults[field];
					}
				}
			}
		}
		
		// override the buildRendering function 
		if (! this._idxWidgetOrigBuildRendering) {
			this._idxWidgetOrigBuildRendering = this.buildRendering;
		}
		this.buildRendering = this._idxWidgetBuildRendering;			
		
		// override the postMixInProperties function
		if (! this._idxWidgetOrigPostMixInProperties) {
			this._idxWidgetOrigPostMixInProperties = this.postMixInProperties;
		}
		// replace the postMixInProperties() function temporarily
		this.postMixInProperties = this._idxWidgetPostMixInProperties;
		
		// override the post-create function
		if (! this._idxWidgetOrigPostCreate) {
			this._idxWidgetOrigPostCreate = this.postCreate;
		}
		// replace the postCreate() function temporarily
		this.postCreate = this._idxWidgetPostCreate;
		
		// override the startup function
		if (! this._idxWidgetOrigStartup) {
			this._idxWidgetOrigStartup = this.startup;
		}
		// replace the startup() function temporarily
		this.startup = this._idxWidgetStartup;
		
		// perform the base create function
		return baseCreate.call(this, params, srcNodeRef);		
	};
	
	return iWidgets;
});
