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
        "dojo/i18n",
        "./string",
        "./util",
        "dojo/i18n!./nls/base"],
        function(dLang,iMain,dI18n,iString,iUtil)
{
  /**
   * @name idx.resources
   * @namespace Provides functionality for overriding and obtaining resource bundles in a hierarchal
   *            manner so that resources can be defined globally, at the package level, and at the module
   *            level with each more specific level taking greater precedence.  Functions in this namespace
   *            typically refer to a "resource scope".  The scope looks like a module path for Dojo such 
   *            as "idx/layout/HeaderPane".  In order to build the resources for "idx/layout/HeaderPane"
   *            scope we get the merger of of the following dojo/i18n NLS files:
   *			   - idx/nls/base.js
   *               - idx/layout/nls/base.js
   *               - idx/layout/nls/HeaderPane.js 
   */
  var iResources = dLang.getObject("resources", true, iMain);
  
  /**
   * @private
   * @name idx.resources._legacyScopeMap
   * @field 
   * @description Internal map of legacy scope names to new scope names for IDX 1.2+.
   */
  iResources._legacyScopeMap = {
		  "": 								"idx/",
		  ".":								"idx/",
		  "app": 							"idx/app/",
		  "app._Launcher": 					"idx/app/_Launcher",
		  "app.A11yPrologue": 				"idx/app/A11yPrologue",
		  "app.WorkspaceType": 				"idx/app/WorkspaceType",
		  "app.WorkspaceTab": 				"idx/app/WorkspaceTab",
		  "dialogs":						"idx/dialogs",
		  "form": 							"idx/form/",
		  "form.buttons":					"idx/form/buttons",
		  "grid": 							"idx/grid/",
		  "grid.prop":	 					"idx/grid/",
		  "grid.prop.PropertyGrid":			"idx/grid/PropertyGrid",
		  "grid.prop.PropertyFormatter":	"idx/grid/PropertyGrid",
		  "layout": 						"idx/layout/",
		  "layout.BorderContainer": 		"idx/layout/BorderContainer",
		  "widget": 						"idx/widget/",
		  "widget.ModalDialog":				"idx/widget/ModalDialog",
		  "widget.TypeAhead":				"idx/widget/TypeAhead",
		  "widget.HoverHelp":				"idx/widget/HoverHelp",
		  "widget.EditController":			"idx/widget/EditController"
  };
  
  /**
   * @private
   * @name idx.resources._defaultResources
   * @field 
   * @description The default resources
   * @type Array
   */
  iResources._defaultResources = {
	dateFormatOptions: {formatLength: "medium", fullYear: true, selector: "date"},
	timeFormatOptions: {formatLength: "medium", selector: "time"},
	dateTimeFormatOptions: {formatLength: "medium", fullYear: true},
	decimalFormatOptions: {type: "decimal"},
	integerFormatOptions: {type: "decimal", fractional: false, round: 0},
	percentFormatOptions: {type: "percent", fractional: true, places: 2},
	currencyFormatOptions: {type: "currency"},
	labelFieldSeparator: ":"
  };

  /**
   * @private
   * @name idx.resources._localResources
   * @field 
   * @description The cache (by locale) of default resources.  Each locale name points to
   *              another array that maps bundle names to bundles that have been loaded. 
   * @type Array
   * @default []
   */
  iResources._localResources = [ ];

  /**
   * @private
   * @name idx.resources._currentResources
   * @field
   * @description The cache (by locale) of current resources.  Each locale name points to
   *              another array that maps bundle names to bundles that have been loaded
   *              and possibly modified.
   * @type Array
   * @default []
   */
  iResources._currentResources = [ ];

  /**
   * @private
   * @name idx.resources._scopedResources
   * @field
   * @description The cache (by locale and then scope) of flattened scope-resources
   * @type Array
   * @default []
   */
  iResources._scopeResources = [ ];

  /**
   * @private
   * @name idx.resources._normalizeScope
   * @function
   * @description Converts the previous "foo.bar" scopes to "idx/foo/bar" scopes.
   * 			  This allows legacy usage to continue to function while allowing
   * 			  product applications to use idx.resources as well for say "my/foo/bar".
   * 			  The new preferred format is "[pkgA]/[pkgB]/[module]".
   * @param {String} scope The optional scope to normalize.
   * @return The normalized scope.
   */
  iResources._normalizeScope = function(/*String*/scope) {
	  if ((! scope)||(scope.length == 0)) {
		  return "idx/";
	  }
	  if (iResources._legacyScopeMap[scope]) {
		  return iResources._legacyScopeMap[scope];
	  }
	  return scope;
  };

  /**
   * @private
   * @name idx.resources._getBundle
   * @function
   * @description Internal method for obtaining the bundle for a given name.
   * 
   * @param {String} packageName The name of the package.
   * @param {String} bundleName The name of the bundle within the package.
   * @param {String} locale The optional locale name.
   */  
  iResources._getBundle = function(/*String*/packageName,/*String*/bundleName,/*String?*/locale) {
	  locale = dI18n.normalizeLocale(locale);
	  var scope = packageName + "." + bundleName;
	  var curResources = iResources._currentResources[locale];
	  if (!curResources) {
		  curResources = [ ];
		  iResources._currentResources[locale] = curResources;
	  }
	  var locResources = iResources._localResources[locale];
	  if (!locResources) {
		  locResources = [ ];
		  iResources._localResources[locale] = locResources;
	  }
	  
	  var bundle = curResources[scope];
	  if (!bundle) {
		  var locBundle = locResources[scope];
		  if (!locBundle) {
			  locBundle = dI18n.getLocalization(packageName, bundleName, locale);
			  if (!locBundle) locBundle = new Object();
			  locResources[scope] = locBundle;
		  }
		  bundle = new Object();
		  dLang.mixin(bundle,locBundle);
		  curResources[scope] = bundle;
	  }
	  
	  // return the bundle
	  return bundle;
  };
  
  /**
   * @public
   * @function
   * @name idx.resources.clearLocalOverrides
   * @descritpion Clears any resource overrides and resets to the default resources for the 
   *              specified (or default) locale, restoring the default resources.
   * 
   * @param {String} locale The optional locale for which the overrides should be cleared.  If
   *                        not specified then the default locale is assumed.
   */
  iResources.clearLocalOverrides = function(/*String?*/ locale) {
	 locale = dI18n.normalizeLocale(locale);
     iResources._currentResources[locale] = null;
     iResources._scopeResources[locale] = null;
  };

  /**
   * @public
   * @function
   * @name idx.resources.clearOverrides
   * @description Clears all resource overrides for all locales.
   */
  iResources.clearOverrides = function() {
     iResources._currentResources = [ ];
     iResources._scopeResources = [ ];
  };

  /**
   * @public
   * @function
   * @name idx.resources.install
   * @description Installs new resources and/or overrides existing resources being either in
   *              the base resource scope or in a specified scope.  The specified scope 
   *              might look like "idx/layout/HeaderPane", "idx/layout" or "ibm/myproduct".
   *              Resources should only be installed during application startup and then should
   *              be left unchanged to maximize efficiency.
   * @param {Object} resources The new resources to override the old ones -- this is mixed in
   *                           as a layer on top of the default resources.
   * @param {String} scope The optional string to override resources in a specific scope.  If not
   *                       specified then the global scope is assumed.
   * @param {String} locale The optional locale to override for.  If not specified then the 
   *                        default locale is assumed.
   */
  iResources.install = function(/*Object*/  resources, 
                                /*String*/  scope,
                                /*String?*/ locale) {
	locale = dI18n.normalizeLocale(locale);
	scope = iResources._normalizeScope(scope);
	var lastIndex = scope.lastIndexOf("/");
	
	var packageName = "";
	var bundleName  = "";
	if (lastIndex == scope.length - 1) {
		bundleName = "base";
		packageName = scope.substr(0,scope.length-1);
	} else if (lastIndex >= 0) {
		bundleName = scope.substr(lastIndex+1);
		packageName = scope.substr(0, lastIndex);
	}
    var bundle = iResources._getBundle(packageName, bundleName, locale);
    dLang.mixin(bundle, resources);
    iResources._clearResourcesCache(locale, scope);
  };

  /**
   * @public 
   * @function
   * @name idx.resources.getResources
   *
   * @description Obtains the resources for the specified scope.  The specified scope 
   *              might look like "idx/layout/HeaderPane", "idx/layout" or "ibm/myproduct".
   * 
   * @param {String} scope The optional string to override resources in a specific scope.  If not
   *                       specified then the global scope is assumed.
   * @param {String} locale The optional locale for which the resources are being requested.  If not
   *                        specified then the default locale is assumed.
   * @returns {Object} Returns a flattened resources object containing all resources for the optionally
   *                   specified scope.
   */
  iResources.getResources = function(/*String?*/ scope, /*String?*/ locale) {
    locale = dI18n.normalizeLocale(locale);
    scope = iResources._normalizeScope(scope);
    var scopeResources = iResources._scopeResources[locale];
    if (! scopeResources) {
       scopeResources = [ ];
       iResources._scopeResources[locale] = scopeResources;
    }

    var resourcesByScope = scopeResources[scope];

    // if we have a cached array of the resources, return it
    if (resourcesByScope) return resourcesByScope;

    resourcesByScope = new Object();

    var scopes = scope.split("/");
    var index = 0;
    var pkg = "";
    var prefix = "";
    for (index = 0; index < scopes.length; index++) {
    	var bundleName = "base";
    	if (index < scopes.length-1) {
    		pkg = pkg + prefix + scopes[index];
    		prefix = ".";
    	} else {
    		bundleName = scopes[index];
    	}
		if (bundleName.length == 0) continue;
    	var bundle = iResources._getBundle(pkg,bundleName,locale);
    	if (!bundle) continue;
    	for (var field in bundle) {
    		resourcesByScope[field] = bundle[field];
    	}
    }
  
  	// cache for later
  	scopeResources[scope] = resourcesByScope;

  	// return the resources
    return resourcesByScope;     
  };

    /**
   * @public 
   * @function
   * @name idx.resources.getDependencies
   *
   * @description Obtains the array of "dojo/i18n!" dependencies for the specified scope.
   * 
   * @param {String} scope The optional string to override resources in a specific scope.  If not
   *                       specified then the global scope is assumed.
   * @returns {String[]} Returns the array of "dojo/i18n!" dependencies.
   */
  iResources.getDependencies = function(/*String?*/ scope) {
    scope = iResources._normalizeScope(scope);
	var dependencies = [];
	var scopes = scope.split("/");
    var index = 0;
    var pkg = "";
    var prefix = "";
	var bundleName = "";
	var dependency = "";
    for (index = 0; index < scopes.length; index++) {
    	bundleName = "base";
    	if (index < scopes.length-1) {
    		pkg = pkg + prefix + scopes[index];
    		prefix = "/";
    	} else {
    		bundleName = scopes[index];
    	}
		if (bundleName.length == 0) continue;
		dependency = "dojo/i18n!" + pkg + prefix + "nls/" +  bundleName;
		dependencies.push(dependency);
    }

	return dependencies;
  };

  /**
   * @public
   * @function
   * @description Legacy method for obtaining only the string resources.  This has been 
   *              superceeded by the more robust "getResources" method.
   * @param {String} scope The optional string to override resources in a specific scope.  If not
   *                       specified then the global scope is assumed.
   * @param {String} locale The optional locale for which the resources are being requested.  If not
   *                        specified then the default locale is assumed.
   * @returns {Object} Returns the result from idx.resouce.getResources.
   * @deprecated Use idx.resources.getResources instead
   */
  iResources.getStrings = function(/*String?*/ scope, /*String?*/ locale) {
	  return iResources.getResources(scope,locale);
  };
  
  /**
   * @private
   * @name idx.resources._clearResourcesCache
   * @function
   * @description Clears out any cached objects represnting the resources by scope for a
   *              particular locale.  This is called whenever new resources are installed 
   *              for the locale (which should not be very often).
   * @param {String} locale
   */
  iResources._clearResourcesCache = function(/*String?*/ locale,/*String?*/scope) {
	locale = dI18n.normalizeLocale(locale);
	if (iResources._scopeResources[locale]) {
		if (!scope) {
			iResources._scopeResources[locale] = null;
		} else {
			var cache = iResources._scopeResources[locale];
			for (field in cache) {
				if (iString.startsWith(field,scope)) {
					cache[field] = null;
				}
			}
		}
	}
  };

  /**
   * @public
   * @function
   * @name idx.resources.get
   * @description  Gets the named resource in the specified scope.  If the name is not found
   *               in the specified scope then the parent scope is searched and then its 
   *               parent up until the root scope.  If the resources is not found then null
   *               is returned.  If the scope is not specified then the root scope is assumed.
   *               The locale may be optionally specified as well.
   * @param {String} name The name of the resource to obtain from the hierarchical bundle.
   * @param {String} scope The optional scope, if not specified then the global scope is used.
   * @param {String} locale The optional locale, if not specified the default locale is used.
   * @returns {String} The value for the resource or null if not found.
   */
  iResources.get = function(/*String*/  name, 
                            /*String?*/ scope,
                            /*String?*/ locale) {
	locale = dI18n.normalizeLocale(locale);
	scope = iResources._normalizeScope(scope);
    var scopes = scope.split("/");
    var index = 0;
    for (index = 0; index < scopes.length; index++) {
    	var bundleName = (index == 0) ? scopes[scopes.length-1] : "base";
    	var pkgName = "";
    	var prefix = "";
    	var pkgMax = (index == 0) ? (scopes.length - index - 1) : (scopes.length - index);
    	for (var idx2 = 0; idx2 < pkgMax; idx2++) {
    		pkgName = pkgName + prefix + scopes[idx2];
    		prefix = ".";
    	}
    	var bundle = iResources._getBundle(pkgName,bundleName,locale);
    	if (!bundle) continue;
    	if (name in bundle) return bundle[name];
    }
    if(name in iResources._defaultResources){
    	return iResources._defaultResources[name];
    }
    return null;
  };

  /**
   * @public
   * @function
   * @name idx.resources.getLabelFieldSeparator
   * @description Returns the resource to use for separating labels from their fields.
   *              Typically this is a ":" or something to that effect.
   * @return Returns the resource to use for separating labels from their fields.
   *         Typically this is a ":" or something to that effect.
   */
  iResources.getLabelFieldSeparator = function(/*String?*/ scope,
                                                  /*String?*/ locale) {
      return iResources.get("labelFieldSeparator", scope, locale);  
  };
  
  /**
   * @public
   * @function
   * @name idx.resources.getDateFormatOptions
   * @description Getter for date format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The date format options for the given scope.
   */
  iResources.getDateFormatOptions = function(/*String?*/ scope, 
                                                /*String?*/ locale) {
     return iResources.get("dateFormatOptions", scope, locale);
  };
  
  /**
   * @public
   * @function
   * @name idx.resources.getTimeFormatOptions
   * @description Getter for time format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The time format options for the given scope.
   */
  iResources.getTimeFormatOptions = function(/*String?*/ scope,
                                                /*String?*/ locale) {
     return iResources.get("timeFormatOptions", scope, locale);
  };

  /**
   * @public
   * @function
   * @name idx.resources.getDateTimeFormatOptions
   * @description Getter for date/time format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The date/time format options for the given scope.
   */
  iResources.getDateTimeFormatOptions = function(/*String?*/ scope,
                                                    /*String?*/ locale) {
     return iResources.get("dateTimeFormatOptions", scope, locale);
  };

  /**
   * @public
   * @function
   * @name idx.resources.getDecimalFormatOptions
   * @description Getter for date format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The decimal format options for the given scope.
   */
  iResources.getDecimalFormatOptions = function(/*String?*/ scope,
                                                   /*String?*/ locale) {
     return iResources.get("decimalFormatOptions", scope, locale);
  };

  /**
   * @public
   * @function
   * @name idx.resources.getIntegerFormatOptions
   * @description Getter for integer format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The integer format options for the given scope.
   */
  iResources.getIntegerFormatOptions = function(/*String?*/ scope, 
                                                   /*String?*/ locale) {
     return iResources.get("integerFormatOptions", scope, locale);
  };

  /**
   * @public
   * @function
   * @name idx.resources.getPercentFormatOptions
   * @description Getter for percent format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The percent format options for the given scope.
   */
  iResources.getPercentFormatOptions = function(/*String?*/ scope,
                                                   /*String?*/ locale) {
     return iResources.get("percentFormatOptions", scope, locale);
  };

  /**
   * @public
   * @function
   * @name idx.resources.getCurrencyFormatOptions
   * @description Getter for currency format options
   * @param {String} scope The optional scope, if not specified then the global scope is assumed.
   * @param {String} locale The optional locale, if not specified then the default locale is assumed.
   * @returns {Object} The currency format options for the given scope.
   */
  iResources.getCurrencyFormatOptions = function(/*String?*/ scope,
                                                    /*String?*/ locale) {
     return iResources.get("currencyFormatOptions", scope, locale);
  };
  
  return iResources;
});