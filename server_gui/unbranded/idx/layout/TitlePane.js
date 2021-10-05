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
        "dijit/TitlePane",
        "./ContentPane",
        "dijit/layout/_LayoutWidget",
        "dojo/_base/array",
        "dojo/on",
        "dojo/aspect",
        "dojo/_base/event",
        "dojo/dom-construct",
        "dojo/dom-class",
        "dojo/dom-style",
        "dojo/dom-attr",
        "dojo/dom-geometry",
        "dojo/query",
        "dojo/keys",
        "dijit/_WidgetBase",
        "dijit/form/Button",
        "../util",
        "../string",
        "../widget/_MaximizeMixin",
        "dojo/NodeList-dom",
        "../ext",
        "dijit/layout/BorderContainer"	// to add "region" attribute to ContentPane
        ],
	    function(dDeclare,			// (dojo/_base/declare)
	     		 dLang,				// (dojo/_base/lang)
				 dTitlePane,		// (dijit/TitlePane)
				 iContentPane,		// (./ContentPane)
				 dLayoutWidget,		// (dijit/layout/_LayoutWidget)
				 dArray,			// (dojo/_base/array)
				 dOn,				// (dojo/on)
				 dAspect,			// (dojo/aspect)
				 dEvent,			// (dojo/_base/event) for (dEvent.stop)
				 dDomConstruct,		// (dojo/dom-construct)
				 dDomClass,			// (dojo/dom-class) for (dDomClass.add/remove)
				 dDomStyle,			// (dojo/dom-style) for (dDomStyle.set)
				 dDomAttr,			// (dojo/dom-attr)
				 dDomGeo,			// (dojo/dom-geometry) for (dDomGeo.getContentBox/getMarginBox)
				 dQuery,			// (dojo/query.NodeList) + (dojo/NodeList-dom) for (dQuery.NodeList)
				 dKeys,				// (dojo/keys)
				 dWidget,			// (dijit/_WidgetBase)
				 dButton,			// (dijit/form/Button)
				 iUtil,				// (../util)
				 iString,			// (../string)
				 iMaximizeMixin)	// (../widget/MaximizeMixin)
{
	var dNodeList = dQuery.NodeList;
	
	/**
 	 * @name idx.layout.TitlePane
 	 * @class Collapsible title pane.  This extends the dijit.TitlePane to provide additional features.
 	 * @augments dijit.TitlePane
 	 * @augments dijit.layout._LayoutWidget
 	 */
	return dDeclare("idx.layout.TitlePane",[dTitlePane,iContentPane,dLayoutWidget],
			/**@lends idx.layout.TitlePane#*/
{
	/**
   	 * Overrides of the base CSS class.
   	 * Prevent the _LayoutWidget from overriding the "baseClass"
   	 * we want "dijitTitlePane".
   	 * @private
   	 * @constant
   	 * @type String
   	 * @default "dijitTitlePane"
   	 */
    baseClass: "dijitTitlePane",

	_setTitleAttr: iContentPane.prototype._setTitleAttr,
	
	/**
   	 * IDX base CSS class.
   	 * @private
   	 * @constant
   	 * @type String
   	 * @default "idxTitlePane"
   	 */
    idxBaseClass: "idxTitlePane",

	/**
   	 * Controls whether or not you can collapse the pane.
   	 * If not collapsible, then even the toggle button is
   	 * not shown.
   	 * @type boolean
   	 * @default true
   	 */
    collapsible: true,
  
	/**
   	 * Set this to true if you want the actions in the title
   	 * to disappear when the pane is not focused or hovered.
   	 * @type boolean
   	 * @default true
   	 */
    autoHideActions: false,

	/**
	 * Specifies whether the pane should have a button added
	 * to the title actions region with placement=toolbar
	 * and buttonType=close with that button's "onClick" event
	 * linked to this title pane's close() method. 
	 * 
	 * @type Boolean
	 * @default false
	 */
	closable: false,

	/**
	 * Specifies whether the pane is refreshable.  If so, then
	 * a button is added to the title actions region with
	 * placement=toolbar and buttonType=refresh with the 
	 * button's "onClick" event linked to this title pane's
	 * "refreshPane" function.
	 * @type Boolean
	 * @default false
	 */
	refreshable: false,

	/**
	 * Specifies whether the pane is resizable.  If so, then
	 * a button is added to the title actions region with
	 * placement=toolbar and buttonType=maxRestore with the
	 * button's "onClick" event linked to this title pane's 
	 * "maximizePane" and "restorePane" funtions.
	 * @type Boolean
	 * @default false
	 */
	resizable: false,

	/**
	 * Sets the interval in milliseconds for animating
	 * the resize event if resizable.  If this is set
	 * to zero (0) or a negative number then no 
	 * animation is used. 
	 */
	resizeDuration: 0,

	/**
	 * URL for a help document.  If provided then a button is
	 * added to the title actions with the placement=toolbar
	 * and buttonType=help with the button's "onClick" event
	 * configured to open a new browser window with the 
	 * specified URL. 
	 * 
	 * @type String
	 * @default ""
	 */
	helpURL: "",

	/**
	 * The default action displayMode to be set on buttons for the
	 * default actions such as "closable", "resizable", "helpURL,
	 * and "refreshable".  This can control whether you view buttons
	 * only, buttons and icons or icons only.  You can set the default
	 * display mode globally for the "toolbar" buttons using idx.buttons. 
	 */
	defaultActionDisplay: "",
	
	/**
	 * Constructor
	 * Handles the reading any attributes passed via markup.
	 * @param {Object} args
	 * @param {Object} node
	 */
    constructor: function(args, node) {
      this._titleActions = [ ];
      this._built = false;
      this._started = false;
      this._idxStarted = false;
      this._maximizeMixin = new iMaximizeMixin({inPlace: true});
    },
    
  /**
   * Overridden to handle destorying the many handles and actions associated with
   * the HeaderPane.
   */
  destroy: function() {
  	this._destroyDefaultCloseButton();
  	this._destroyDefaultRefreshButton();
  	this._destroyDefaultResizeButton();
  	this._destroyDefaultHelpButton();
  	if (this._actionsLookup) {
  		for (var region in this._actionsLookup) {
  			var actions = this._actionsLookup[region];
  			dArray.forEach(actions, function(action) {
  				if (action.handle) {
  					action.handle.remove();
  					delete action.handle;
  				}
  			});
  		}
  	}
	dArray.forEach(this.getTitleActionChildren(), function(child) {
		child.destroyRecursive();
	});
  	this.inherited(arguments);
  },
  
    /**
     * Internal method for stopping the propagation of events.
     */
    _killActionsMouseEvent: function(e) {
    	this.set("active", false);    	
    	this._setStateClass();
    	dDomClass.remove(this.domNode, this.baseClass+"Active");
    	dDomClass.remove(this.titleBarNode, this.baseClass+"TitleActive");
    	if (e) {
    		dEvent.stop(e);
    	}
    },
    
    /**
     * Internal method for stopping the propagation of events.
     */
    _killActionsKeyEvent: function(e) {
    	if (e) {
    		if (e.keyCode == dKeys.SPACE || e.keyCode == dKeys.ENTER) {
	    		this.set("active", false);
		    	this._setStateClass();
		    	dDomClass.remove(this.domNode, this.baseClass+"Active");
		    	dDomClass.remove(this.titleBarNode, this.baseClass+"TitleActive");
		    	// Comment this out for dojo 1.9.1 since it prevents events on the action buttons
   			 	// TODO: work with dojo team to understand event handling change that prevents 
    			// blocking style changes to title bar (active state on key down)
    			//dEvent.stop(e);
    		}
    	}
    },
    
    /**
     * Overridden to set state and setup nodes 
     * @see dijit.TitlePane.buildRendering
     */
    buildRendering: function() {
      // defer to the base function
      this.inherited(arguments);
      this._titleActionsNode = dDomConstruct.create(
         "div", { "class": this.idxBaseClass + "Actions" }, this.titleBarNode);

      this.own(dOn(this._titleActionsNode, "mousedown", dLang.hitch(this, this._killActionsMouseEvent)));
      this.own(dOn(this._titleActionsNode, "click", dLang.hitch(this, this._killActionsMouseEvent)));
      this.own(dOn(this._titleActionsNode, "dblclick", dLang.hitch(this, this._killActionsMouseEvent)));
      this.own(dOn(this._titleActionsNode, "keypress", dLang.hitch(this, this._killActionsKeyEvent)));      
      this.own(dOn(this._titleActionsNode, "keydown", dLang.hitch(this, this._killActionsKeyEvent)));      
      this.own(dOn(this._titleActionsNode, "keyup", dLang.hitch(this, this._killActionsKeyEvent)));      
      this.own(dAspect.after(this._maximizeMixin, "_restore", dLang.hitch(this, this._onRestore), true));
 
      // check titleNode/containrNode for proper ID and aria-labelledby attributes
      var titleID = this.id + "_titleNode";
      if (dDomAttr.has(this.titleNode, "id")) titleID = dDomAttr.get(this.titleNode, "id");
      else dDomAttr.set(this.titleNode, "id", titleID);
      if (!dDomAttr.has(this.containerNode, "aria-labelledby")) dDomAttr.set(this.containerNode, "aria-labelledby", titleID);
      dDomClass.add(this.domNode, this.idxBaseClass);
      this._built = true;
      this._updateCollapsibility();
      this._updateActionHiding();
      this._setupNodes();
    },
  
    /**
     * postMixinProperties - default behavior
     * @see dijit.TitlePane.postMixinProperties
     */
    postMixInProperties: function() {
      this.inherited(arguments);
      this._setupMaximizeAnimation();
    },
  
    /**
     * Private method to set up contained nodes
     * @private
     */
    _setupNodes: function() {
      if (! this._built) return;
  
      this._regionLookup = {
        body: this.containerNode,
        titleActions: this._titleActionsNode
      };

      this._actionsLookup = {
        titleActions: this._titleActions
      };
    },
  
    /**
     * postCreate - default behavior
     * @see dijit.TitlePane.postCreate
     */
    postCreate: function() {
      this.inherited(arguments);
      dDomStyle.set(this._titleActionsNode, "width", "auto");
    },

    /**
     * Toggles whether the pane is collapsed or open
     * @see dijit.TitlePane.toggle
     */
    toggle: function() {
      if (((this.collapsible) || (!this.open)) && (! this._refreshing)) {
    	 this.inherited(arguments);
      }
    },
  
    /**
     * Private method to set the collapsible value
     * and call internaml methods to update
     * collapsibility.
     * @private
     * @param {boolean} value
     */
    _setCollapsibleAttr: function(value) {
      this.collapsible = value;
      this._updateCollapsibility();
    },

    /**
     * Private method to update collapsible attributes
     * @private
     */
    _updateCollapsibility: function() {
      if (!this._built) return;
      if (this.collapsible) {
         dDomClass.remove(this.titleBarNode, this.idxBaseClass + "NoCollapse");
      } else {
         dDomClass.add(this.titleBarNode, this.idxBaseClass + "NoCollapse");
      }

      // ensure we are open if not collapsible
      if (!this.collapsible) this.toggle();
    },

    /**
     * Private method to handle setting the autoHideActions attribute.
     * @private
     */
    _setAutoHideActionsAttr: function(hide) {
    	this.autoHideActions = hide;
    	this._updateActionHiding();
    },
    
    /**
     * Private method to add or remove the auto hide class. 
     * @private
     */
    _updateActionHiding: function() {
      if (!this._built) return;
      if (this.autoHideActions) {
         dDomClass.add(this.domNode, this.idxBaseClass + "AutoHide");
      } else {
         dDomClass.remove(this.domNode, this.idxBaseClass + "AutoHide");
      }
    },

    /**
     * Private method to handle changes in the "displayMode" attribute
     * and update the default buttons accordingly.
     * @param (String) displayMode
     */
    _setDefaultActionDisplayAttr: function(displayMode) {
    	this.defaultActionDisplay = displayMode;
    	if (this._closeButton) {
    		this._closeButton.set("displayMode", this.defaultActionDisplay);
    	}
    	if (this._refreshButton) {
    		this._refreshButton.set("displayMode", this.defaultActionDisplay);
    	}
    	if (this._resizeButton) {
    		this._resizeButton.set("displayMode", this.defaultActionDisplay);
    	}
    	if (this._helpButton) {
    		this._helpButton.set("displayMode", this.defaultActionDisplay);
    	}
    	this.resize();
    },
    /**
     * Private method to handle setting of the "Closable" attribute.  This
     * method will add or remove the close button depending on the parameter.
     * @param (Boolean) closable -- true if closable, otherwise false 
     * @private
     */
    _setClosableAttr: function(closable) {
    	var oldValue = this.closable;
    	this.closable = closable;
    	if (! this._idxStarted) return; // see note in startup() why not to use this._started
    	if (oldValue == this.closable) return;
    	if (this.closable) {
    		this._createDefaultCloseButton();
    	} else {
    		this._destroyDefaultCloseButton();
    	}
    },
    
    /**
     * Private method to handle setting of the "refreshable" attribute.  This
     * method will add or remove the refresh button depending on the parameter.
     * @param (Boolean) refreshable -- true if refreshable, otherwise false 
     * @private
     * 
     */
    _setRefreshableAttr: function(refreshable) {
    	var oldValue = this.refreshable;
    	this.refreshable = refreshable;
    	if (! this._idxStarted) return; // see note in startup() why not to use this._started
    	if (oldValue == this.refreshable) return;
    	if (this.refreshable) {
    		this._createDefaultRefreshButton();
    	} else {
    		this._destroyDefaultRefreshButton();
    	}
    },
    
    /**
     * Private method to handle setting of the "resizable" attribute.  This
     * method will add or remove the max/restore button depending on the parameter.
     * @param (Boolean) resizable -- true if resizable, otherwise false 
     * @private
     */
    _setResizableAttr: function(resizable) {
    	var oldValue = this.resizable;
    	this.resizable = resizable;
    	if (! this._idxStarted) return; // see note in startup() why not to use this._started
    	if (oldValue == this.resizable) return;
    	if (this.resizable) {
    		this._createDefaultResizeButton();
    	} else {
    		this._destroyDefaultResizeButton();
    		if (this._maximized) {
    			this._restorePane();
    		}
    	}
    },
    
    /**
     * Private method to handle setting of the "resizeDuration" attribute.  
     * @param (Number) duration -- the number of milliseconds
     * @private
     */
    _setResizeDurationAttr: function(duration) {
    	this.resizeDuration = duration;
        this._setupMaximizeAnimation();
    },
    
    /**
     * Internal method that should be called whenever the resize duration changes.
     * @private
     */
    _setupMaximizeAnimation: function() {
    	if (this.resizeDuration > 0) {
        	this._maximizeMixin.useAnimation = true;
    		this._maximizeMixin.duration = this.resizeDuration;
    	} else {
        	this._maximizeMixin.useAnimation = false;
    		this._maximizeMixin.duration = 0;
    	}
    },
    
    /**
     * Private method to handle setting of the "helpURL" attribute.  This
     * method will add or remove the help button depending on the parameter.
     * @param (String) helpURL -- The URL to open a page to when clicked.
     * @private
     */
    _setHelpURLAttr: function(url) {
    	var oldValue = this.helpURL;
    	this.helpURL = url;
    	if (! this._idxStarted) return; // see note in startup() why not to use this._started
    	var wasNull = (iString.nullTrim(oldValue) ? false : true);
    	var isNull  = (iString.nullTrim(this.helpURL) ? false : true);
    	if (wasNull == isNull) return;
    	if (! isNull) {
    		this._createDefaultHelpButton();
    	} else {
    		this._destroyDefaultHelpButton();
    	}
    },
    
    /**
     * Internal method for creating the default close button.
     * @private
     */
    _createDefaultCloseButton: function() {
      if (this._closeButton) return;
  	  this._closeButton = new dButton(
			  { buttonType: "close", placement: "toolbar", region: "titleActions",
				displayMode: this.defaultActionDisplay });
	  if (this._closeHandle) {
		this._closeHandle.remove();
		delete this._closeHandle;
	  }
  	  this._closeHandle = dAspect.after(this._closeButton, "onClick", dLang.hitch(this, "_closePane"), true);
	  this.addChild(this._closeButton);    	
	  // see note in startup() why not to use this._started
	  if (this._idxStarted) this._closeButton.startup();
    },
    
    /**
     * Internal method for destroying the default close button.
     * @private
     */
    _destroyDefaultCloseButton: function() {
    	if (this._closeHandle) {
    		this._closeHandle.remove();
    		delete this._closeHandle;
    	}
    	if (this._closeButton) {
    		this.removeChild(this._closeButton);
    		this._closeButton.destroyRecursive();
    		delete this._closeButton;
    	}
    },

    /**
     * Internal method for creating the default refresh button.
     * @private
     */
    _createDefaultRefreshButton: function() {
      if (this._refreshButton) return;
  	  this._refreshButton = new dButton(
			  { buttonType: "refresh", placement: "toolbar", region: "titleActions",
				displayMode: this.defaultActionDisplay  });
      if (this._refreshHandle) {
    	this._refreshHandle.remove();
    	delete this._refreshHandle;
      }
  	  this._refreshHandle = dAspect.after(this._refreshButton, "onClick", dLang.hitch(this, "refreshPane"), true);
	  this.addChild(this._refreshButton);  
	  // see note in startup() why not to use this._started
	  if (this._idxStarted) this._refreshButton.startup();
    },
    
    /**
     * Internal method for destroying the default refresh button.
     * @private
     */
    _destroyDefaultRefreshButton: function() {
    	if (this._refreshHandle) {
    		this._refreshHandle.remove();
    		delete this._refreshHandle;
    	}
    	if (this._refreshButton) {
    		this.removeChild(this._refreshButton);
    		this._refreshButton.destroyRecursive();
    		delete this._refreshButton;
    	}
    },

    /**
     * Internal method for creating the default max/restore (resize) button.
     * @private
     */
    _createDefaultResizeButton: function() {
      if (this._resizeButton) return;
  	  this._resizeButton = new dButton(
			  { buttonType: "maxRestore", placement: "toolbar", region: "titleActions",
				displayMode: this.defaultActionDisplay  });
      if (this._resizeHandle) {
    	this._resizeHandle.remove();
    	delete this._resizeHandle;
      }
  	  this._resizeHandle = dAspect.after(this._resizeButton, "onButtonTypeClick", dLang.hitch(this, "_resizePane"), true);
	  this.addChild(this._resizeButton);
	  // see note in startup() why not to use this._started
	  if (this._idxStarted) this._resizeButton.startup();
    },
    
    /**
     * Internal method for destroying the default max/restore (resize) button.
     * @private
     */
    _destroyDefaultResizeButton: function() {
    	if (this._resizeHandle) {
    		this._resizeHandle.remove();
    		delete this._resizeHandle;
    	}
    	if (this._resizeButton) {
    		this.removeChild(this._resizeButton);
    		this._resizeButton.destroyRecursive();
    		delete this._resizeButton;
    	}
    },
    
    /**
     * Internal method for creating the defaulthelp button.
     * @private
     */
    _createDefaultHelpButton: function() {
      if (this._helpButton) return;
  	  this._helpButton = new dButton(
			  { buttonType: "help", placement: "toolbar", region: "titleActions",
				displayMode: this.defaultActionDisplay  });
      if (this._helpHandle) {
    	this._helpHandle.remove();
    	delete this._helpHandle;
      }
  	  this._helpHandle = dAspect.after(this._helpButton, "onClick", dLang.hitch(this, "_displayHelp"), true);
	  this.addChild(this._helpButton);
	  // see note in startup() why not to use this._started
	  if (this._idxStarted) this._helpButton.startup();
    },
    
    /**
     * Internal method for destroying the defaulthelp button.
     * @private
     */
    _destroyDefaultHelpButton: function() {
    	if (this._helpHandle) {
    		this._helpHandle.remove();
    		delete this._helpHandle;
    	}
    	if (this._helpButton) {
    		this.removeChild(this._helpButton);
    		this._helpButton.destroyRecursive();
    		delete this._helpButton;
    	}
    },

    /**
     * Called at startup to set state, init children and resize when done.
     * @see dijit.TitlePane.startup
     */
    startup: function() {
      if (! this._started) this.inherited(arguments);
      // NOTE: we cannot use a check against this._started here because dijit.layout.ContentPane
  	  // will execute "delete this._started" with every time content is loaded via refresh
      if(this._idxStarted) return;
      this._idxStarted = true;
      
      // check if we intercepted the href
      if (this._href) {
         this.set("href", this._href);
      }
  
      // preload now (from ContentPane)
      if(this._isShown() || this.preload){
        this._onShow();
      }
	  dArray.forEach(this.getChildren(), this._setupChild, this);
  
      if (this.refreshable) {
    	 	this._createDefaultRefreshButton();
      }
	  if (iString.nullTrim(this.helpURL)) {
    	 	this._createDefaultHelpButton();
      }
      if (this.resizable) {
    		this._createDefaultResizeButton();
      }
      if (this.closable) {
    	 	this._createDefaultCloseButton();
      }

      // mark started and resize
      this.resize();
    },

    /**
     * Internal method for removing the defaut buttons to make way for custom buttons.
     * 
     * @private
     */
    _removeDefaultButtons: function(child) {
    	if (this._restoringDefaultButtons) return false;
    	var result = false;
    	if (this._closeButton) {
    		if (child != this._closeButton) {
    			this.removeChild(this._closeButton);
    			result = true;
    		}
    	}
    	if (this._resizeButton) {
    		if (child != this._resizeButton) {
    			this.removeChild(this._resizeButton);
    			result = true;
    		}
    	}
    	if (this._helpButton) {
    		if (child != this._helpButton) {
    			this.removeChild(this._helpButton);
    			result = true;
    		}
    	}
    	if (this._refreshButton) {
    		if (child != this._refreshButton) {
    			this.removeChild(this._refreshButton);
    			result = true;
    		}
    	}
    	return result;
    },
    
    /**
     * Internal method for restoring the default buttons.
     * @private
     */
    _restoreDefaultButtons: function() {
    	if (this._restoringDefaultButtons) return;
    	this._restoringDefaultButtons = true;
    	if (this._refreshButton) {
   			this.addChild(this._refreshButton);
    	}
    	if (this._helpButton) {
   			this.addChild(this._helpButton);
    	}
    	if (this._resizeButton) {
   			this.addChild(this._resizeButton);
    	}
    	if (this._closeButton) {
   			this.addChild(this._closeButton);
    	}
    	this._restoringDefaultButtons = false;
    },
    
    /**
     * Extends parent method by adding in a resize after the 
     * child node is added.
     * @param {Object} child
     * @param {int} index
     * @see dijit.TitlePane.addChild
     */
    addChild: function(child, index) {
    	var restoreDefaults = this._removeDefaultButtons(child);
    	if ((!this._isDefaultButton(child)) || (! restoreDefaults)) {
    		this.inherited(arguments);
    	}
    	if (restoreDefaults) this._restoreDefaultButtons(child);
    	this.resize();
    },
    
    /**
     * Checks if the specified child is one of the default buttons.
     * @param {Object} child
     * 
     */
    _isDefaultButton: function(child) {
    	return ((child == this._closeButton)
    			|| (child == this._resizeButton)
    			|| (child == this._refreshButton)
    			|| (child == this._helpButton));
    },

    /**
     * Extends parent method 
     * @param {Object} child
     * @see dijit.TitlePane.remove3Child
     */
    removeChild: function(child) {
      var index = 0;
      var foundIndex = -1;
      var actions = null;
      
      // check if the child is a title action
      for (index = 0; index < this._titleActions.length; index++) {
        if (child === this._titleActions[index].widget) {
           foundIndex = index;
           actions = this._titleActions;
           var handle = this._titleActions[index].handle;
           if (handle) {
           		handle.remove();
           		delete this._titleActions[index].handle;
           }
           break;
        }
      }
  
      // check if the instance was found
      if (foundIndex >= 0) {
        // remove the element from the array
        actions.splice(foundIndex, 1);
      }

      // let the base method remove it
      this.inherited(arguments);
      
      // check if the child is a widget title
      if ((child) && (child == this._enhancedTitle)) {
      	// the widget has already been detached so just set the title to empty-string
    	this._internalSet = true;
      	this.set("title", "");
      	this._internalSet = false;
      }

      // resize
      this.resize();
    },
  
    /**
     * Private worker to set up specified child 
     * @param {Widget} child
     * @private
     */
    _setupChild: function(/*Widget*/ child) {
      this.inherited(arguments);
      var region  = iString.nullTrim(child.get("region"));
      if (region == "title") {
    	  this.set("title", child);
    	  return;
      }
      
      var actions = null;
  
      if ((region == null) || (region.length == 0)) region = "body";
      
      var node = this._regionLookup[region];
      var actions = this._actionsLookup[region];
      if (node == null) {
        console.log("Child region unrecognized - defaulting to 'body': "
                    + region);
        node = this._regionLookup["body"];
        actions = null;
      }
      
      // check if the widget is already where it belongs
      if (node == this.containerNode) return;
  
      // otherwise move the widget
      var nodeList = new dNodeList(child.domNode);
      nodeList.orphan();
      dDomConstruct.place(child.domNode, node, "last");

      var handle = null;
      if (child.onResize) {
         handle = dAspect.after(child, "onResize", dLang.hitch(this, "resize"), true);
      } 
      actions.push({widget: child, handle: handle});

    },
  
    /**
     * Returns the children that are in the "titleActions" region.
     * 
     */
    getTitleActionChildren: function() {
    	var result = [ ];
    	for (var index = 0; index < this._titleActions.length; index++) {
    		var action = this._titleActions[index];
    		result.push(action.widget);
    	}
    	return result;
    },
    
    /**
     * Pass through parent layout method
     * Checks to see if single child first.
     * @see dijit._Widget.layout
     */
    layout: function(){
      this._checkIfSingleChild();
      this.inherited(arguments);
    },
    
    /**
     * Defect 14165
     * As Dojo 1.10 change the remove check of doLayout in _layoutChildren
     * force to skip _checkIfSingleChild in titlepane for proper layout 
     */
    _layoutChildren: function(){
        var tmpfunc = this._checkIfSingleChild;
        this._checkIfSingleChild = function(){};
        this.inherited(arguments);
        this._checkIfSingleChild = tmpfunc;
      },

    /**
     * Override since the implementation in _ContentPaneResizeMixin now only performs the
	 * check if "doLayout" is true, and "doLayout" is false for TitlePane.
     */
    _checkIfSingleChild: function() {
		this.doLayout = true;
		this.inherited(arguments);
		this.doLayout = false;
    },

    /**
     * Resize method that extends parent by also sizing the children.
     * @param {Object} changeSize
     * @param {Object} resultSize
     * @see dijit.layout._LayoutWidget.resize
     */
    resize: function(changeSize,resultSize) {
      if (! this._idxStarted) return; // see note in startup() why not to use this._started
      this.inherited(arguments);
      // determine the proper heights for header and actions nodes
      dDomStyle.set(this.titleNode, {width: "auto", height: "auto"});
      dDomStyle.set(this._titleActionsNode, {width: "auto", height: "auto"});

      var size1 = iUtil.getStaticSize(this.titleNode);
      var size2 = iUtil.getStaticSize(this._titleActionsNode);
    
      var tHeight = ((size1.h > size2.h) ? size1.h : size2.h);
    
      dDomStyle.set(this.titleNode, {width: "auto", height: "auto"});
      dDomStyle.set(this._titleActionsNode, {width: "auto", height: "auto"});
      //dDomStyle.set(this.focusNode, {height: tHeight + "px"});
 
      // determine if we have a single child
      if (this.open && this._singleChild && this._singleChild.resize) {
          var cb = dDomGeo.getContentBox(this.containerNode);
	  // note: if widget has padding this._contentBox will have l and t set,
  	  // but don't pass them to resize() or it will doubly-offset the child

          // NOTE: We do NOT set the height since the TitlePane should grow with contents
          // if you want a fixed height then place a ContentPane insize of it with a fixed
          // height on it.
          this._singleChild.resize({w: cb.w});
          dDomClass.add(this._singleChild.domNode, this.baseClass + "-onlyChild");
      }
    },
  
    /**
     * Private onShow callback
     * @private
     * @see dijit.layout._LayoutWidget._onShow
     */
    _onShow: function() {
       if (!this._idxStarted) return; // see note in startup() why not to use this._started
       this.inherited(arguments);
    },
  
    /**
     * Private worker to set the Href attr
     * @private
     * @see dijit.layout.TitlePane._setHrefAttr
     */
    _setHrefAttr: function(/*String|Uri*/ href) {
       if (! this._idxStarted) { // see note in startup() why not to use this._started
          this._href = href;
          return;
       } else {
    	   this._href = null;
       }
       if (this.href == href) return;
       this.inherited(arguments);
    },

    /**
     * Allow replacement of the title node.
     * @param {String} name
     * @param {String} value
     */
    set: function(name, value) {
    	// Override default behavior
        if((!this._internalSet) && (name == "title")) {
        	this._setTitleObject(value);
         } else {
            this.inherited(arguments);
         }
    },
    
    /**
     * Allow replacement of the title node.
     * @param {String} name
     * @param {String} value
     */
    get: function(name) {
    	// Override default behavior
        if((!this._internalGet) && (name == "title") && (this._enhancedTitle)) {
        	return this._enhancedTitle;
         } else {
            return this.inherited(arguments);
         }
    },
    
    /**
     * Allow replacement of the title node.
     * @param {Objec} node
     */
    _setTitleObject: function(titleObj) {
    	// clear out any old title
    	if (this._enhancedTitle) {
			if (this._enhancedTitle instanceof dWidget) {
				this.removeChild(this._enhancedTitle);
				if (this._enhancedTitle.destroyRecursive) {
					this._enhancedTitle.destroyRecursive();
				} else if (this._enhancedTitle.destroy) {
					this._enhancedTitle.destroy();
				}
			} else {
				dDomConstruct.destroy(this._enhancedTitle);
			}
    	}
        this.titleNode.innerHTML = "";
        
    	// check if we have a basic string
    	if (iUtil.typeOfObject(titleObj) != "object") {
			this._internalSet = true;
			this.set("title", "" + titleObj);
			this._internalSet = false;
			this._enhancedTitle = null;
			return;
		}

		// set the enhanced title
		this._enhancedTitle = titleObj;
		// get the node for the title object
        var node = null;
        if (titleObj instanceof dWidget) {
        	node = titleObj.domNode;
        	
        } else {
        	node = titleObj;
        }
        // otherwise move the widget
        var nodeList = new dNodeList(node);
        nodeList.orphan();
		dDomConstruct.place(node, this.titleNode, "last");
        this.resize();
    },
    
	/**
	 * Closes the pane.
	 * @param {Obkect} e
	 * @private
	 */
	_closePane: function(e) {
		if (e) {
			dEvent.stop(e);
		}
		this.onClose();
		this.destroyRecursive();
	},
	
	/**
	 * Attach to this to be alerted to "refresh" button events.
	 */
	onRefresh: function(pane, href, deferred) {
		
	},
	
	/**
	 * Refreshes the pane.
	 * @param {Obkect} e
	 * @private
	 */
	refreshPane: function(e) {
		if (e) {
			dEvent.stop(e);
		}
		var href = this.get("href");
		if (! iString.nullTrim(href)) {
			this.onRefresh(this);
			return;
		}
		var contentBox = dDomGeo.getContentBox(this.containerNode);
		dDomStyle.set(this.containerNode, {height: "" + contentBox.h + "px"});
		this._refreshing = true;
		var result = this.refresh();
		this.onRefresh(this, href, result);
	},
	
	/**
	 * Called to handle success when downloading content from the href attribute.
     * @private
\	 */
	onDownloadEnd: function() {
		this.inherited(arguments);
		this._refreshing = false;
		dDomStyle.set(this.containerNode, {height: null});
		this.resize();
	},
	
	/**
     * Called to handle errors when downloading content from the href attribute.
     * @private
	 */
	onDownloadError: function() {
		this.inherited(arguments);
		this._refreshing = false;
		dDomStyle.set(this.containerNode, {height: null});
		this.resize();
	},
	
	/**
	 * Resizes the pane.
	 * @param {Object} e
	 * @param {Object} linkNode
	 * @param {Object} iconNode
	 * @private
	 */
	_resizePane: function(e, buttonTypeState) {
		if (e) {
			dEvent.stop(e);
		}
		switch (buttonTypeState) {
		case "maximize":
			this.maximizePane();
			break;
		case "restore":
			this.restorePane();
			break;
		}
		return false;
	},

	/**
	 * Maximizes this TitlePane to fill its container, typically a parent
	 * or ancestor node.
	 * 
	 * @param container -- The container to fill.
	 */
	maximizePane: function(/*Node*/ container) {
		if (! container) container = this.domNode.parentNode;
		if (! container) return;
		var currentPos = this.domNode.style.position;
		this._restorePosition = (iString.nullTrim(currentPos) ? currentPos : "");
		this._restoreCollapse = this.get("collapsible");
		this.set("collapsible", false);
		this._restoreScrollPosition = {
			scrollLeft: container.scrollLeft,
			scrollTop: container.scrollTop
		};
		switch (currentPos) {
		case "absolute":
			// do nothing
			break;
		default:
			dDomStyle.set(this.domNode, {position: "relative"});
		}
		var mbox = dDomGeo.getMarginBox(this.titleBarNode);
		this._maximizeMixin.maximize(this.domNode, container);
		this._maximized = true;
		dDomStyle.set(this.hideNode, {position: "absolute", top: "" + mbox.h + "px", left: "0px", right: "0px", bottom: "0px"});
		
		//handle max status for idx titlepane's max mode
		dDomClass.add(this.domNode, "idxTitlePaneMaxMode");
		
		dDomClass.remove(this.titleBarNode, "dijitTitlePaneTitleHover");
		if (this._resizeButton) {
			this._resizeButton.set("hovering", false);
			this._resizeButton.set("active", false);
		}
		this.onMaximize();
	},
	
	/**
	 * Restores this TitlePane to to its previous size.
	 */
	restorePane: function() {
		if (! this._maximized) return;
		this._maximizeMixin.restore();
	},

	_onRestore: function(){
		dDomStyle.set(this.hideNode, {position: "", top: "", left: "", right: "", bottom: ""});
		dDomStyle.set(this.domNode, {position: this._restorePosition});
		this.set("collapsible", this._restoreCollapse);
		this._maximized = false;
		
		//max status return to normal
		dDomClass.remove(this.domNode, "idxTitlePaneMaxMode");
		
		dDomClass.remove(this.titleBarNode, "dijitTitlePaneTitleHover");
		if (this._resizeButton) {
			this._resizeButton.set("hovering", false);
			this._resizeButton.set("active", false);
		}
		this.domNode.parentNode.scrollLeft=this._restoreScrollPosition.scrollLeft;
		this.domNode.parentNode.scrollTop=this._restoreScrollPosition.scrollTop;
		this.onRestore();
	},
	
	/**
	 * Callback to be called when maximized.
	 */
	onMaximize: function() {
		
	},

	/**
	 * Callback to be called when restored.
	 */
	onRestore: function() {

	},
	
	/**
	 * Opens help document.
	 * @param {Object} e
	 * @private
	 */
	_displayHelp: function(e) {
		// TODO(bcaceres): Update this to publish a message on the bus to see 
		// if there is a widget registered that can handle displaying online help
		// otherwise fallback to just opening a window
		if (e) {
			dEvent.stop(e);
		}
		if (iString.nullTrim(this.helpURL)) {
			if ( (! this._helpWindow) || (this._helpWindow.closed) ) {
				this._openNewHelpWindow();
			} else {
				try {
					this._helpWindow.location.assign(this.helpURL);
					this._helpWindow.focus();
				} catch (err) {
					this._openNewHelpWindow();
				}
			}
		}
	},
	
	/**
	 * Internal method for opening the help window.
	 * @private
	 */
	_openNewHelpWindow: function() {
		if (! iString.nullTrim(this.helpURL)) return;
		this._helpWindow = window.open(this.helpURL, "_blank");
		
		// check if we should discard the help window reference
		// -- do this if we cannot utilize it due to cross-site
		// scripting security measures
		if ((this.helpURL.indexOf("http://") == 0) 
			|| (this.helpURL.indexOf("https://") == 0)) {
			this._helpWindow = null;
		}
	}
  });
});