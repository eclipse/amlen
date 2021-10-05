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
        "dojo/string",
        "dojo/dom-construct",
        "dojo/dom-class",
        "dojo/dom-attr",
        "dojo/aspect",
        "dojo/_base/event",
        "dijit/form/Button",
        "dojo/data/ItemFileReadStore",
        "../string",
        "../resources",
        "../html",
        "dojo/i18n!../nls/base",
        "dojo/i18n!./nls/base",
        "dojo/i18n!./nls/buttons",
        "../ext"], // needed for "idxBaseClass" to be properly used
		function(dLang, 				// (dojo/_base/lang)					
				 iMain,					// (idx)
				 dString,				// (dojo/string)
				 dDomConstruct,			// (dojo/dom-construct)
				 dDomClass,				// (dojo/dom-class) for (dDomClass.add/remove/contains)
				 dDomAttr,				// (dojo/dom-attr) for (dDomAttr.get/set)
				 dAspect,				// (dojo/aspect)
				 dEvent,				// (dojo/_base/event) for (dEvent.stop)
				 dButton,				// (dijit/form/Button)
				 dItemFileReadStore,	// (dojo/date/ItemFileReadStore)
				 iString,				// (../string)
				 iResources, 			// (../resources)
				 iHTML)					// (../html)
{
	/**
	 * @name idx.form.buttons
	 * @class Extension to dijit.form.Button to add functionality and features to all instances of
	 *        dijit.form.Button and its descendants.  These extensions provide things like well-defined
	 *        button "type" attribute for common button types as well as alternate "placement", 
	 *        "displayMode", and "profile" as well as styling the button with additional 
	 *        CSS class to make it easier to style buttons of any type with a CSS selector.
	 */
	var iButtons = dLang.getObject("form.buttons", true, iMain);
	
    /**
     * This defines the known display modes:
     *   - "labelOnly"
     *   - "iconOnly"
     *   - "iconAndLabel"
     *   
     * @private
     */
    iButtons._displayModes= {
      labelOnly: { showIcon: false, showLabel: true, cssClass: "idxButtonLabelOnly" },
      iconOnly: { showIcon: true, showLabel: false, cssClass: "idxButtonIconOnly" },
      iconAndLabel: { showIcon: true, showLabel: true, cssClass: "idxButtonIconAndLabel" }
    };
    
    var displayModes = iButtons._displayModes;
    
	/**
	 * This defines the possible button profiles with the current definition
	 * for the button profile.  The possible values include:
	 * - "standard"
	 * - "compact"
	 */
	iButtons._buttonProfiles = {
			standard: { cssClass: null },
			compact: { cssClass: "idxButtonCompact" }
	};
	var buttonProfiles = iButtons._buttonProfiles;
	
    /**
     * This defines the possible button placements with the current definition 
     * for the button placement.  The possible values include:
     * - "primary"
     * - "secondary"
     * - "toolbar"
     * - "special"
     * 
     * @private
     */
	iButtons._buttonPlacements = {
			primary: { cssClass: null, defaultDisplayMode: "iconAndLabel", defaultProfile: "standard"},
			secondary: { cssClass: "idxSecondaryButton", defaultDisplayMode: "labelOnly", defaultProfile: "standard"},
			toolbar: { cssClass: "idxButtonToolbar", defaultDisplayMode: "iconOnly", defaultProfile: "standard"},
			special: { cssClass: "idxButtonSpecial", defaultDisplayMode: "iconAndLabel", defaultProfile: "standard"}
	};
	var buttonPlacements = iButtons._buttonPlacements;	
	
    /**
     * @public
     * @function
     * @name idx.form.buttons.setDefaultDisplayMode
     * @description Sets the default display mode for either all possible button placement positions
     *              or for a specific one.
     * 
     * @param {String} displayModeName A string that can be one of "labelOnly", "iconOnly", 
     *                                 or "iconAndLabel"
     *                        
     * @param {String} placementName If null then the call affects all possible button placements, otherwise
     *                               this should be one of "primary", "secondary", "special" or "toolbar".  
     * 
     */
    iButtons.setDefaultDisplayMode = function(/*String*/ displayModeName,/*String*/ placementName) {
    	var mode = displayModes[displayModeName];
    	
    	// error out if we got a bad mode
    	if (!mode) {
    		throw new Error("Invalid mode display mode name: " + displayModeName);
    	}
    	
    	if (! placementName) {
    		for (placementName in buttonPlacements) {
    			buttonPlacements[placementName].defaultDisplayMode = displayModeName;
    		}
    	} else {
    		var placement = buttonPlacements[placementName];
    		if (!placement) throw new Error("Invalid button placement name: " + placementName);
    		placement.defaultDisplayMode = displayModeName;
    	}
    };

    /**
     * @public
     * @function
     * @name idx.form.buttons.setDefaultProfile
     * @description Sets the default "profile" for either all possible button placement positions
     *              or for a specific one.
     * 
     * @param {String} profileName A string that can be one of "standard" or "compact".
     *                        
     * @param {String} placementName If null then the call affects all possible button placements, otherwise
     *                                this should be one of "primary", "secondary", "special" or "toolbar".  
     * 
     */
    iButtons.setDefaultProfile = function(/*String*/ profileName,/*String?*/ placementName) {
    	var profile = buttonProfiles[profileName];
    	
    	// error out if we got a bad mode
    	if (!profile) {
    		throw new Error("Invalid mode profile name: " + profileName);
    	}
    	
    	if (! placementName) {
    		for (placementName in buttonPlacements) {
    			buttonPlacements[placementName].defaultProfile = profileName;
    		}
    	} else {
    		var placement = buttonPlacements[placementName];
    		if (!placement) throw new Error("Invalid button placement: " + placement);
    		placement.defaultProfile = profileName;
    	}
    };


    /**
     * This defines the standard button types.
     * 
	 * Possible values are:
	 *   
	 *	"close",
	 *  "configure"
	 *	"edit"
	 *	"filter"
	 *	"clearFilter"
	 *	"toggleFilter"
	 *	"help"
	 *	"info"
	 *	"minimize"
	 *	"maximize"
	 *	"print"
	 *	"refresh"
	 *	"restore"
	 *	"maxRestore"
	 *	"nextPage"
	 *	"previousPage"
	 *	"lastPage"
	 *	"firstPage"
     * 
     * @private
     */
	iButtons._stdButtonTypes = {
			close: { iconClass: "idxCloseIcon", iconSymbol: ['X','X'], labelKey: "closeLabel", titleKey: "closeTip" },
			configure: { iconClass: "idxConfigureIcon", iconSymbol: ['\u2699','\u2699'], labelKey: "configureLabel", titleKey: "configureTip" },
			edit: { iconClass: "idxEditIcon", iconSymbol: ['\u270E','\u270E'], labelKey: "editLabel", titleKey: "editTip" },
			filter: { iconClass: "idxFilterIcon", iconSymbol: ['\u2A52','\u2A52'], labelKey: "filterLabel", titleKey: "filterTip" },
			clearFilter: { iconClass: "idxClearFilterIcon", iconSymbol: ['\u2A5D','\u2A5D'], labelKey: "clearFilterLabel", titleKey: "clearFilterTip"},
			toggleFilter: { toggleButtonTypes: [ "filter", "clearFilter" ] },
			help: { iconClass: "idxHelpIcon", iconSymbol: ['?','\u061F'], labelKey: "helpLabel", titleKey: "helpTip" },
			info: { iconClass: "idxInfoIcon", iconSymbol: ['\u24D8','\u24D8'], labelKey: "infoLabel", titleKey: "infoTip" },
			minimize: { iconClass: "idxMinimizeIcon", iconSymbol: ['\u2193','\u2193'], labelKey: "minimizeLabel", titleKey: "minimizeTip" },
			maximize: { iconClass: "idxMaximizeIcon", iconSymbol: ['\u2197','\u2196'], labelKey: "maximizeLabel", titleKey: "maximizeTip" },
			print: { iconClass: "idxPrintIcon", iconSymbol: ['\uD83D\uDCF0','\uD83D\uDCF0'], labelKey: "printLabel", titleKey: "printTip" },
			refresh: { iconClass: "idxRefreshIcon", iconSymbol: ['\u21BA','\u21BB'], labelKey: "refreshLabel", titleKey: "refreshTip" },
			restore: { iconClass: "idxRestoreIcon", iconSymbol: ['\u2199','\u2198'], labelKey: "restoreLabel", titleKey: "restoreTip" },
			maxRestore: { toggleButtonTypes: [ "maximize", "restore" ] },
			nextPage: { iconClass: "idxNextPageIcon", iconSymbol: ['\u21D2','\u21D0'], labelKey: "nextPageLabel", titleKey: "nextPageTip" },
			previousPage: { iconClass: "idxPreviousPageIcon", iconSymbol: ['\u21D0','\u21D2'], labelKey: "previousPageLabel", titleKey: "previousPageTip" },
			lastPage: { iconClass: "idxLastPageIcon", iconSymbol: ['\u21DB','\u21DA'], labelKey: "lastPageLabel", titleKey: "lastPageTip" },
			firstPage: { iconClass: "idxFirstPageIcon", iconSymbol: ['\u21DA','\u21DB'], labelKey: "firstPageLabel", titleKey: "firstPageTip" }
	};

	/**
	 * @public
	 * @function 
	 * @name idx.form.buttons.getButtonTypes
	 * @description Returns an array of strings containing the possible button type names.
	 * @return {String[]} The array of strings contianing the button type names that are recognized. 
	 * @see idx.form.buttons.getButtonTypeStore
	 */
	iButtons.getButtonTypes = function() {
		var result = [ ];
		for (field in iButtons._stdButtonTypes) {
			result.push(field);
		}
		return result;
	};
	
	/**
	 * @public
	 * @function
	 * @name idx.form.buttons.getButtonTypeStore
	 * @description Returns a dojo.data.ItemFileReadStore containing the possible button type names.
	 * @return {dojo.data.ItemFileReadStore} A read store containing all possuble button types.
	 * @see idx.form.buttons.getButtonTypes 
	 */
	iButtons.getButtonTypeStore = function(withEmpty) {
		
		var result = { identifier: 'value', label: 'name', items: [ ] };
		
		if (withEmpty) {
			result.items.push({name: "[none]", value: " "});
		}
		for (field in iButtons._stdButtonTypes) {
			result.items.push({name: field, value: field});
		}
		return new dItemFileReadStore({data: result});
	};
	
	// 
	// Get the base functions so we can call them from our overrides
	//
    var buttonProto = dButton.prototype;	
	var setLabelFunc = buttonProto._setLabelAttr;
	var setIconClassFunc = buttonProto._setIconClassAttr;
	var setShowLabelFunc = buttonProto._setShowLabelAttr;
	
	/**
	 * Overrides _setLabelAttr from dijit.form.Button
	 */
	buttonProto._setLabelAttr = function(label) {
		// setup the explicit label if starting up
		if ((!this._started) && (! ("_explicitLabel" in this)) && (this.params) && ("label" in this.params)) {
			this._explicitLabel = this.params.label;
		}
		
		// determine the explicit value so we can revert
		var abt = this._applyingButtonType;
		this._applyingButtonType = false; // clear the flag

		if (! abt) {
			this._explicitLabel = label;
		} else {
			var el = this._explicitLabel;
			if (iString.nullTrim(el)) {
				// we have an explicit label -- ignore button type label
				this.label = el;
				label = el;
			}
		}
		
		if (setLabelFunc) {
			var current = this._setLabelAttr;
			this._setLabelAttr = setLabelFunc;
			this.set("label", label);
			this._setLabelAttr = current;
		} else {
			this.label = label;
			if (this._attrToDom && ("label" in this.attributeMap)) {
				this._attrToDom("label", label);
			}
		}
		
		if ((!abt) && (! iString.nullTrim(this.label))
			&& (this._buttonTypes)) {
			var bt = this._buttonTypes[this._toggleIndex];
			this._applyButtonTypeLabel(bt);
		}
	};
	
	/**
	 * Overrides _setIconClassAttr from dijit.form.Button
	 */
	buttonProto._setIconClassAttr = function(iconClass) {
		// setup the explicit iconClass if starting up
		if ((!this._started) && (! ("_explicitIconClass" in this)) && (this.params) 
				&& ("iconClass" in this.params) && (this.params.iconClass != "dijitNoIcon")) {
			this._explicitIconClass = this.params.iconClass;
		}
		
		// determine the explicit value so we can revert
		var abt = this._applyingButtonType;
		this._applyingButtonType = false; // clear the flag
		if (! abt) { 
			dDomClass.remove(this.domNode, "idxButtonIcon");
			if (iconClass != "dijitNoIcon") this._explicitIconClass = iconClass;
		} else {
			var eic = this._explicitIconClass;
			if (iString.nullTrim(eic)) {
				// we have an explicit icon class -- ignore button type icon class
				this.iconClass = eic;
				iconClass = eic;
				dDomClass.remove(this.domNode, "idxButtonIcon");
			} else {
				dDomClass.add(this.domNode, "idxButtonIcon");
			}
		}
		if (setIconClassFunc) {
			var current = this._setIconClassAttr;
			this._setIconClassAttr = setIconClassFunc;
			this.set("iconClass", iconClass);
			this._setIconClassAttr = current;
		} else {
			this.iconClass = iconClass;
			if (this._attrToDom && ("iconClass" in this.attributeMap)) {
				this._attrToDom("iconClass", iconClass);
			}
		}
		if ((!abt) && (! iString.nullTrim(this.iconClass))
				&& (this._buttonTypes)) {
				var bt = this._buttonTypes[this._toggleIndex];
				this._applyButtonTypeIconClass(bt);
		}
	};
	
	/**
	 * Overrides _setIconClassAttr from dijit.form.Button
	 */
	buttonProto._setShowLabelAttr = function(show) {
		// check if we need to set the explicit value
		if ((!this._started) && (this.params) && ("showLabel" in this.params) && (! ("_explicitShowLabel" in this))) {
			this._explicitShowLabel = this.params.showLabel;
		}
	
		// check if we are applying the display mode
		var adm = this._applyingDisplayMode;
		this._applyingDisplayMode = false; // clear the flag
		
		if (! adm) {
			// if not applying display mode, then store the value
			this._explicitShowLabel = show;
		} 
		if (setShowLabelFunc) {
			var current = this._setShowLabelAttr;
			this._setShowLabelAttr = setShowLabelFunc;
			this.set("showLabel", show);
			this._setShowLabelAttr = current;
		} else {
			this.showLabel = show;
		}
	};
	
	var afterBuildRendering = buttonProto.idxAfterBuildRendering;
	// override to idxAfterBuildRendering to apply displayMode, placement,
	// and profile after we have a dom node.  Also, modifies the button
	// DOM to get screen readers to ignore the value node if the role & waiRole
	// are not set and the value node is being rendered off screen and the value
	// node is not the focus node.  TODO(bcaceres): file defect against dojo
	buttonProto.idxAfterBuildRendering = function() {
		if (afterBuildRendering) afterBuildRendering.call(this);
		
		// reapply these once we are sure we have a dom node
		if (this._displayMode) {
			this._applyDisplayMode(this._displayMode);
		}
		if (this._placement) {
			this._applyPlacement(this._placement);
		}
		if (this._profile) {
			this._applyProfile(this._profile);
		}
		if ((this.valueNode) && (this.valueNode != this.focusNode)
				&& (dDomClass.contains(this.valueNode, "dijitOffScreen"))
				&& (!iString.nullTrim(dDomAttr.get(this.valueNode, "role")))
				&& (!iString.nullTrim(dDomAttr.get(this.valueNode, "wairole")))) {
			// get the screen readers to ignore the value node
			
			//Defect14041 incorrect usage of the presentation role
			//dDomAttr.set(this.valueNode, {role: "presentation", wairole: "presentation"});
		}
	};
	
	
	dLang.extend(dButton, /**@lends idx.form.buttons# */{	
	/**
	 * @public
	 * @field
	 * @descritpion Sets the idxBaseClass so we can be aware of all button-derived widgets.
	 * @default "idxButtonDerived"
	 */
	idxBaseClass: "idxButtonDerived",
	
	/**
	 * <p>The buttonType can be used to provide a default "iconClass" and "label" if an 
	 * explicit one is not defined.  Certain button types also provide automatic 
	 * toggling of the icon and label through a series of possibilities (usually two)
	 * with each click of the button.  This is useful for buttons that change state
	 * when clicked (e.g.: "maximize / restore" or "filter / clear filter")
	 * </p>
	 *
	 * <p>Possible values are:
	 *	<ul><li>"close"</li>
	 *      <li>"configure"</li>
	 *      <li>"edit"</li>
	 *      <li>"filter"</li>
	 *      <li>"clearFilter"</li>
	 *      <li>"toggleFilter"</li>
	 *      <li>"help"</li>
	 *      <li>"info"</li>
	 *      <li>"minimize"</li>
	 *      <li>"maximize"</li>
	 *      <li>"print"</li>
	 *      <li>"refresh"</li>
	 *      <li>"restore"</li>
	 *      <li>"maxRestore"</li>
	 *      <li>"nextPage"</li>
	 *      <li>"previousPage"</li>
	 *      <li>"lastPage"</li>
	 *      <li>"firstPage"</li>
	 *  </ul>
	 * </p>
	 * @public
	 * @field
	 */
	buttonType: "",
	
	/**
	 * <p>
	 * The placement can be used to indicate the location of a button to help
	 * provide a hint for styling it in some themes.  The placement can also
	 * govern the default "displayMode" and "profile" if not explicitly set 
	 * otherwise.
	 * </p>
	 * <p>The possible values for placement are:
     *	<ul><li>"primary"</li>
     *	    <li>"secondary"</li>
     *	    <li>"special"</li>
     *	    <li>"toolbar"</li>
     *  </ul></p>
     *   
	 * @public
	 * @field
     */
	placement: "",
	
	/**
	 * <p>
	 * The display mode controls whether to show the icon, label, or both.
	 * If not set then the default for the specified "placement" is used.
	 * </p>
	 *
	 * <p>The possible values for displayMode are:
     *	<ul><li>"labelOnly"</li>
     *		<li>"iconOnly"</li>
     *	    <li>"iconAndLabel"</li>
     *  </ul></p>
     *   
	 * @public
	 * @field
	 */
	displayMode: "",
	
	/**
	 * <p>
	 * The profile controls the minimum size for the button in some themes.
	 * If not set then the default for the specified "placement" is used.
	 * </p>
	 *
	 * <p>The possible values for profile are:
     * 	<ul><li>"standard"</li>
     * 	    <li>"compact"</li>
     *  </ul></p>
     *
	 * @public
	 * @field
	 */
	profile: "",
	
	/**
	 * The character symbol to show for an icon in high-contrast mode.  This
	 * symbol will be hidden (in favor of the actual icon) if not in high-contrast
	 * mode.
	 * @public
	 * @field
	 * @default ""
	 */
	iconSymbol: "",
	
	/**
	 * Setter function for the iconSymbol attribute.
	 * 
	 * @private
	 */
	_setIconSymbolAttr: function(symbol) {
		// setup the explicit iconClass if starting up
		if ((!this._started) && (! ("_explicitIconSymbol" in this)) && (this.params) 
				&& ("iconSymbol" in this.params)) {
			this._explicitIconSymbol = this.params.iconSymbol;
		}
				
		// determine the explicit value so we can revert if button type removed
		var abt = this._applyingButtonType;
		this._applyingButtonType = false; // clear the flag
		if (! abt) {
			this._explicitIconSymbol = symbol;			  
		} else {
			var eis = this._explicitIconSymbol;
			if (iString.nullTrim(eis)) {
				// we have an explicit symbol -- ignore button type symbol
				this.iconSymbol = eis;
				symbol = eis;
			}
		}

		// set the symbol		
		this.iconSymbol = symbol;
		this._applyIconSymbol(symbol);
		
		if ((!abt) && (! iString.nullTrim(this.iconSymbol))
			&& (this._buttonTypes)) {
			var bt = this._buttonTypes[this._toggleIndex];
			this._applyButtonTypeIconSymbol(bt);
		}

	},
	
	/**
	 * Internal method to apply the icon symbol.
	 *
	 * @private
	 */
	_applyIconSymbol: function(symbol) {
		if (iString.nullTrim(symbol)) {
			if (!this._iconSymbolNode) {
				this._iconSymbolNode = dDomConstruct.create("div", {"class":"idxButtonIconSymbol"}, this.iconNode);
				dDomClass.add(this.domNode, "idxButtonHasIconSymbol");
			}
			this._iconSymbolNode.innerHTML = iHTML.escapeHTML(symbol);
			
		} else {
			if (this._iconSymbolNode) {
				dDomConstruct.destroy(this._iconSymbolNode);
				dDomClass.remove(this.DomNode, "idxButtonHasIconSymbol");
				delete this._iconSymbolNode;
			} 
		}
	},
	
	/**
	 * Applies the icon symbol for the button type.
	 * @private
	 */
	_applyButtonTypeIconSymbol: function(buttonType) {
		var symbols = buttonType.iconSymbol;
		var symbol  = "";
		if (symbols) {
			if (dLang.isString(symbols)) {
				symbol = symbols;
			} else {
				var ltr = this.isLeftToRight();
				symbol = symbols[(ltr ? 0 : 1)];	
			}
		}		
		if (!symbol) symbol = "";
		this._applyingButtonType = true;
		this.set("iconSymbol", symbol);
		this._applyingButtonType = false;		
			
	},
	
	/**
	 * @private
	 */
	_getIDXResources: function() {
	  if (!this._idxResources) {
		  this._idxResources = iResources.getResources("idx/form/buttons", this.lang);
	  }
	  return this._idxResources;
	},
	
	/**
	 * @private
	 */
	_removeButtonType: function() {
		var iconClass = ("_explicitIconClass" in this) ? this._explicitIconClass : "";
		var label = ("_explicitLabel" in this) ? this._explicitLabel : "";
		
		this.set("iconClass", iconClass);
		this.set("label", label);
	},
	
	/**
	 * @private
	 */
	_applyButtonTypeLabel: function(buttonType) {
		var res = this._getIDXResources();
		var btLabel = res[buttonType.labelKey];
		if (!btLabel) btLabel = "";
		
		this._applyingButtonType = true;
		this.set("label", btLabel);
		this._applyingButtonType = false;		
	},
	
	/**
	 * @private
	 */
	_applyButtonTypeIconClass: function(buttonType) {
		var btIconClass = buttonType.iconClass;
		if (!btIconClass) btIconClass = "";
		this._applyingButtonType = true;
		this.set("iconClass", btIconClass);
		this._applyingButtonType = false;		
	},
	
	/**
	 * @private
	 */
	_applyButtonTypeTitle: function(buttonType) {
		var res = this._getIDXResources();
		var btTitle = res[buttonType.titleKey];
		if (!btTitle) btTitle = "";
		
		this._applyingButtonType = true;
		this.set("title", btTitle);
		this._applyingButtonType = false;		
	},
	
	/**
	 * @private
	 * Sets the button type which will override the label and icon class.
	 */
	_applyButtonType: function(buttonType) {
		this._applyButtonTypeLabel(buttonType);
		this._applyButtonTypeIconClass(buttonType);
		this._applyButtonTypeIconSymbol(buttonType);		
		this._applyButtonTypeTitle(buttonType);
	},
	
	/**
	 * @private
	 */
	_applyDisplayModeShowLabel: function(show) {
		this._applyingDisplayMode = true;
		this.set("showLabel", show);
		this._applyingDisplayMode = false;
	},

	/**
	 * Sets the button type which will override the label and icon class.
	 * @private
	 */
	_applyDisplayMode: function(displayMode) {
		this._displayMode = displayMode;
		var showLabel     = displayMode.showLabel;
		var showIcon      = displayMode.showIcon;
		var cssClass      = displayMode.cssClass;
		
		if (cssClass) dDomClass.add(this.domNode, cssClass);
		if (!showIcon) dDomClass.add(this.domNode, "idxButtonHideIcon");
		
		this._applyDisplayModeShowLabel(showLabel);
	},
	
	/**
	 * @private
	 */
	_removeDisplayMode: function() {
		if (! this._displayMode) return;
		
		var showLabel = this._displayMode.showLabel;
		var showIcon  = this._displayMode.showIcon;
		var cssClass  = this._displayMode.cssClass;
		
		if (cssClass) dDomClass.remove(this.domNode, cssClass);
		if (!showIcon) dDomClass.remove(this.domNode, "idxButtonHideIcon");
		
		showLabel = ("_explicitShowLabel" in this) ? this._explicitShowLabel : true;
		this.set("showLabel", showLabel);
	},

	/**
	 * @private
	 */
	_applyPlacementDisplayMode: function(placement) {
		var displayMode   = placement.defaultDisplayMode;
		if (! displayMode) displayMode = "";
		
		this._applyingPlacement = true;
		this.set("displayMode", displayMode);
		this._applyingPlacement = false;
	},

	/**
	 * Sets the profile.
	 * @private
	 */
	_applyProfile: function(profile) {
		this._profile = profile;
		var cssClass = profile.cssClass;
		
		if (cssClass) dDomClass.add(this.domNode, cssClass);		
	},
	
	/**
	 * @private
	 */
	_removeProfile: function() {
		if (! this._profile) return;
		
		var cssClass  = this._profile.cssClass;
		
		if (cssClass) dDomClass.remove(this.domNode, cssClass);
	},

	/**
	 * @private
	 */
	_applyPlacementProfile: function(placement) {
		var profile   = placement.defaultProfile;
		if (! profile) profile = "";
		
		this._applyingPlacement = true;
		this.set("profile", profile);
		this._applyingPlacement = false;
	},

	/**
	 * Sets the button type which will override the label and icon class.
	 * @private
	 */
	_applyPlacement: function(placement) {
		this._placement   = placement;
		var cssClass      = placement.cssClass;
		
		if (cssClass) dDomClass.add(this.domNode, cssClass);
		this._applyPlacementDisplayMode(placement);
		this._applyPlacementProfile(placement);
	},
	
	/**
	 * @private
	 */
	_removePlacement: function() {
		if (! this._placement) return;
		
		var cssClass  	= this._placement.cssClass;
		var displayMode	= this._placement.defaultDisplayMode;
		
		if (cssClass) dDomClass.remove(this.domNode, cssClass);
		
		var displayMode = ("_explicitDisplayMode" in this) ? this._explicitDisplayMode : "";
		
		this.set("displayMode", displayMode);
	},
	
	/**
	 * @private
	 */
	_buttonTypeClick: function(e) {
		var buttonTypeState = this.buttonType;
		if (this._toggleButtonTypes && (this._toggleButtonTypes.length > 1)) {
			buttonTypeState = this._toggleButtonTypes[this._toggleIndex];
		}
		this.onButtonTypeClick(e, buttonTypeState);
		this._toggleButtonType();
	},
	
	/**
	 * @public
	 * @function
	 * @description An on-click event added to dijit.form.Button that also communicates the 
	 *              "buttonTypeState" associated with the button type.  For "maxRestore" button 
	 *              type the "buttonTypeState" will either be "maximize" or "restore".  For 
	 *              "toggleFilter" button type the "buttonTypeState" will either be "filter" or 
	 *              "clearFilter".  For non-toggle button types then the button type state is 
	 *              always equal to the button type that was set.
	 * @param {Event} e The event.
	 * @param {String} buttonTypeState The toggle state for the button or the button type itself.
	 */
	onButtonTypeClick: function(/*Event*/ e, /*String*/ buttonTypeState) {
		
	},
	
	/**
	 * @private
	 */
	_buttonTypeDblClick: function(e) {
		var buttonTypeState = this.buttonType;
		if (this._toggleButtonTypes && (this._toggleButtonTypes.length > 1)) {
			buttonTypeState = this._toggleButtonTypes[this._toggleIndex];
		}
		this.onButtonTypeDblClick(e, buttonTypeState);
		this._toggleButtonType();
	},
	
	/**
	 * @public
	 * @function
	 * @description An on-double-click event added to dijit.form.Button that also communicates the 
	 *              "buttonTypeState" associated with the button type.  For "maxRestore" button type
	 *              the "buttonTypeState" will either be "maximize" or "restore".  For "toggleFilter" 
	 *              button type the "buttonTypeState" will either be "filter" or "clearFilter".  For
	 *              non-toggle button types then the button type state is always equal to the button 
	 *              type that was set.
	 * @param {Event} e The event.
	 * @param {String} buttonTypeState The toggle state for the button or the button type itself.
	 * 
	 */
	onButtonTypeDblClick: function(/*Event*/ e, /*String*/ buttonTypeState) {
		
	},
	
	/**
	 * @private
	 */
	_toggleButtonType: function() {
		if ((! this._buttonTypes) || (this._buttonTypes.length < 2)) return;

		// get the old button type and remove it
		var oldBT = this._buttonTypes[this._toggleIndex];
		this._removeButtonType(oldBT);
		
		// increment the index
		this._toggleIndex++;
		if (this._toggleIndex >= this._buttonTypes.length) this._toggleIndex = 0;
		
		// get the new button type and apply it
		var newBT = this._buttonTypes[this._toggleIndex];
		this._applyButtonType(newBT);
	},
	
	/**
	 * @private
	 */
	_setButtonTypeAttr: function(/*String*/ buttonType) {
		// attach to "onClick"
		if (!this._hasButtonTypeClickHandle) {
			this._hasButtonTypeClickHandle = true;
			var callback = dLang.hitch(this, this._buttonTypeClick);
			this.own(dAspect.after(this, "onClick", callback, true));
		}	
		if (!this._hasButtonTypeDblClickHandle) {
			this._hasButtonTypeDblClickHandle = true;
			var callback = dLang.hitch(this, this._buttonTypeDblClick);
			this.own(dAspect.after(this, "onDblClick", callback, true));
		}
		// set the button type
		this.buttonType = dString.trim(buttonType);
		var sbt = iButtons._stdButtonTypes;
	    var bt = sbt[buttonType];
		if (this._buttonTypes) {
			var currentBT = this._buttonTypes[this._toggleIndex];
		
			this._buttonTypes = null;
			this._removeButtonType(currentBT);
		}
		if (bt) {
			if (bt.toggleButtonTypes) {
				this._buttonTypes = [ ];
				var tbt = bt.toggleButtonTypes;
				for (var index = 0; index < tbt.length; index++) {
					var btName = tbt[index];
					this._buttonTypes.push(sbt[btName]);
				}
				this._toggleButtonTypes = bt.toggleButtonTypes;
			} else {
				this._buttonTypes = [ bt ];
				this._toggleButtonTypes = [ this.buttonType ]; 
			}
			
			this._toggleIndex = 0;
		 
		  var currentBT = this._buttonTypes[this._toggleIndex];
		  this._applyButtonType(currentBT);
		  
	    } else{
	    	this._buttonTypes = null;
	    }
	    
	},
	
	/**
	 * @private
	 */
	_setPlacementAttr: function(/*String*/ placement) {
		// remove the current placement if any
		this._removePlacement();
		
		// set the placement key
		this.placement = placement;
		
		// lookup the actual placement if we have a key
		if (this.placement) {
			var newPlacement = iButtons._buttonPlacements[this.placement];
			
			// if an actual display mode exists for key then apply it
			if (newPlacement) {
				this._applyPlacement(newPlacement);
			}
		}		
	},
	
	/**
	 * @private
	 */
	_setDisplayModeAttr: function(/*String*/ displayMode) {
		// setup the explicit display mode if starting up
		if ((!this._started) && (! ("_explicitDisplayMode" in this)) && (this.params) && ("displayMode" in this.params)) {
			this._explicitDisplayMode = this.params.displayMode;
		}
		
		// check if we are applying the placement
		var ap = this._applyingPlacement;
		this._applyingPlacement = false; // clear the flag
		
		
		if (! ap) {
			// if not applying placement, then store the value
			this._explicitDisplayMode = displayMode;
		} else {
			var edm = this._explicitDisplayMode;
			if (iString.nullTrim(edm)) {
				// we have an explicit display mode -- ignore placement display mode
				displayMode = edm;
				this.displayMode = edm;
			}
		}

		// remove the old display mode (does nothing if none)
		this._removeDisplayMode();
		
		// set the display mode text key
		this.displayMode = displayMode;
		
		// lookup the actual display mode if we have a key
		if (this.displayMode) {
			var newDM = iButtons._displayModes[this.displayMode];
			this._displayMode = newDM;
			
			// if an actual display mode exists for key then apply it
			if (newDM) {
				this._applyDisplayMode(newDM);
			}
		}
		if ((!ap) && (! iString.nullTrim(this.displayMode))
			&& (this._placement)) {
			this._applyPlacementDisplayMode(this._placement);
		}
		
	},
	
	/**
	 * @private
	 */
	_setProfileAttr: function(/*String*/ profile) {
		// setup the explicit profile if starting up
		if ((!this._started) && (! ("_explicitProfile" in this)) && (this.params) && ("profile" in this.params)) {
			this._explicitProfile = this.params.profile;
		}
		
		// check if we are applying the placement
		var ap = this._applyingPlacement;
		this._applyingPlacement = false; // clear the flag
		
		
		if (! ap) {
			// if not applying placement, then store the value
			this._explicitProfile = profile;
		} else {
			var ep = this._explicitProfile;
			if (iString.nullTrim(ep)) {
				// we have an explicit profile -- ignore placement profile
				profile = ep;
				this.profile = ep;
			}
		}

		// remove the old profile (does nothing if none)
		this._removeProfile();
		
		// set the profile text key
		this.profile = profile;
		
		// lookup the actual profile if we have a key
		if (this.profile) {
			var newProf = iButtons._buttonProfiles[this.profile];
			this._profile = newProf;
			
			// if an actual profile exists for key then apply it
			if (newProf) {
				this._applyProfile(newProf);
			}
		}
		if ((!ap) && (! iString.nullTrim(this.profile))
			&& (this._placement)) {
			this._applyPlacementProfile(this._placement);
		}
		
	},

	/**
	 * @private
	 */
	_killEvent: function(e) {
		if (e) dEvent.stop(e);
	}
}); // end dojo.extend
	
	return iButtons;
	
});