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
        "dojo/when",
        "dojo/ready",
        "dojo/Deferred",
        "dojo/_base/window",
        "dojo/dom",
        "dojo/dom-attr",
        "dojo/dom-class", 
        "dojo/dom-construct", 
        "dojox/layout/ContentPane",
        "dojox/html/_base",
        "../util",
        "../string"],
	function(dDeclare, 
			 dLang, 
			 dWhen, 
			 dReady, 
			 dDeferred, 
			 dWindow,
			 dDom,
			 dDomAttr,
			 dDomClass, 
			 dDomConstruct, 
			 dContentPane, 
			 dxHTML, 
			 iUtil,
			 iString)
	/**@lends idx.layout.ContentPane#*/
	{
		var dRequireFunc = function(contentClass) {
			var self = this;
			var dependency = contentClass.replace(/\./g,"/");
			require([dependency], function(cClass) { self._loadContent(); });
		};
		 
		/**
		 * Internal ContentSetter implementation that is used to implement the 
		 * idx.layout.ContentPane functionality.
		 */
		var ContentSetter = dDeclare("idx.layout._ContentSetter", [dxHTML._ContentSetter], {
			contentPane: null,
			fakeContent: false,
			content: null,
			/**
			 * This funtion is essentially copied from dojo.html._ContentSetter's onEnd() method
			 * to handle parsing the content.  We have to delay parsing until all JavaScript in 
			 * the page has been executed (potentially asycnhronously) and then after we have called
			 * the "preParse" callbacks (if any have been defined).
			 */
			handleParse: function() {
				if(this.parseContent){
					// populates this.parseResults if you need those..
					this._parse();
				}
				return this.node; /* DomNode */
			},
			
			/**
			 * This method should be called AFTER the JavaScript on the page has been executed.
			 * Calling this method is triggered by appending JavaScript to the JS code to 
			 * trigger a callback to the "scriptExecuted" function of the idx.layout.ContentPane.
			 * The idx.layout.ContentPane in turn calls this function.  Because script handling
			 * is handled by dojox.layout.ContentPane via "script" tag injection, it was found
			 * that actual script execution could happen asynchronously on some browsers.  This
			 * function essentially enables a callback for when the script has finished execution.
			 */
			scriptsExecuted: function() {
				var cp = this.contentPane;
				var self = this;
				// call back to the "handleRequires" function of the content pane
				cp._handleRequires(function(modules) {
					dReady(function() {
						// once dependencies are loaded handle any pre-parse callbacks 
						// that were registered with the content pane
						cp._preParse();
					
						// now that we have done the pre-parse we need to parse the HTML content
						self.handleParse();
					
						dWhen(cp.parseDeferred, function() {												
							// after all content is parsed and children are started, trigger the post-parse
							// callbacks that were registered with the content pane
							cp._postParse();
					
							self.scriptsDeferred.resolve();							
						});
					});
				});
			},

			/**
			 * For the most part, the onEnd() implementation was copied from 
			 * dojox.html._ContentSetter.  Differences include:
			 *  -- When a script is present, appending the "_scriptsExecuted" callback to the end 
			 *     of the script to be notified when asynchronous execution completes
			 *     
			 *  -- when no scripts are present ensuring processing is done synchronously and that
			 *     the "preParse()" and "postParse()" functions are called. 
			 */
			onEnd: function() {
				// summary
				//		Called after set(), when the new content has been pushed into the node
				//		It provides an opportunity for post-processing before handing back the node to the caller
				//		This implementation extends that of dojo.html._ContentSetter

				var code = this._code,
					styles = this._styles;

				// clear old stylenodes from the DOM
				// these were added by the last set call
				// (in other words, if you dont keep and reuse the ContentSetter for a particular node
				// .. you'll have no practical way to do this)
				if(this._styleNodes && this._styleNodes.length){
					while(this._styleNodes.length){
						dDomConstruct.destroy(this._styleNodes.pop());
					}
				}
				// render new style nodes
				if(this.renderStyles && styles && styles.length){
					this._renderStyles(styles);
				}
				
				if(this.executeScripts && code){
					this.scriptsDeferred = new dDeferred();
					code = code + "\n" + this.scriptHookReplacement + "._scriptsExecuted();";
					if(this.cleanContent){
						// clean JS from html comments and other crap that browser
						// parser takes care of in a normal page load
						code = code.replace(/(<!--|(?:\/\/)?-->|<!\[CDATA\[|\]\]>)/g, '');
					}
					if(this.scriptHasHooks){
						// replace _container_ with this.scriptHookReplace()
						// the scriptHookReplacement can be a string
						// or a function, which when invoked returns the string you want to substitute in
						code = code.replace(/_container_(?!\s*=[^=])/g, this.scriptHookReplacement);
					}
					try{
						dxHTML.evalInGlobal(code, this.node);
					}catch(e){
						this._onError('Exec', 'Error eval script in '+this.id+', '+e.message, e);
					}
					
					return this.scriptsDeferred.promise;
					
				} else {
					var d = new dDeferred();
					var self = this;
					dReady(function() {
						self.contentPane._deferResolve = false;
						self.contentPane._preParse();
						self.handleParse();
						dWhen(self.parseDeferred, function() {
							self.contentPane._postParse();
							d.resolve();
						});
					});
					return d.promise;
				}
			},
			
			/**
			 * Override to tear down the scriptsDeferred
			 * @public
			 * @function
			 */
			tearDown: function(){
				delete this.scriptsDeferred;
				this.inherited(arguments);
			}
			
		});
		
	/**
	 * @name idx.layout.ContentPane
	 * @class ContentPane enabling lazy instantiation of child widget as well as asynchronous
	 *        module dependency for href'd content with embedded JavaScript and "require" calls.
	 * @augments dijit.layout.ContentPane
	 */
	  var ContentPane = dDeclare("idx.layout.ContentPane", [dojox.layout.ContentPane], {	
		/**
		 * Single child widget class.
		 * @type String
		 * @default ""
		 */
		contentClass: "",

		/**
		 * Parameters for the single child widget.
		 * @type String
		 * @default ""
		 */
		contentArgs: "",
		
		/**
		 * Flag indicating if the label should be inferred from the title in some cases.
		 * Set this to false if you want the label to have to be explicitly set via the
		 * "label" attribute.
		 */
		inferLabel: true,
		
		/**
		 * The label to use for accessibility purposes.  This will be added to an off-screen node
		 * and the aria-labelledby attribute will be set on the dom node.
		 */
		label: "",
		
		/**
		 * Default "scriptHasHooks" to true.  This differs from dojox.layout.ContentPane.
		 * If using idx.layout.ContentPane, then most likely the reason is to get access to
		 * _container_.require() and _container_.requireLocalization() as well as
		 * _container_.registerPreParse() and _container_.registerPostParse() 
		 */
		scriptHasHooks: true,

		/**
		 * Callback function to be notified when the page-embedded scripts have completed
		 * execution.  A JS call is appended to the embedded script to call this function
		 * on the container. 
		 */
		_scriptsExecuted: function() {
			if (this._contentSetter) this._contentSetter.scriptsExecuted();
		},
		
		/**
		 * Records the dependency to be required later by AMD loader.
		 */
		_recordDependency: function(dependency) {
			if (! this._requires) this._requires = [ "dojo/parser" ];
			if (! this._modules) this._modules = { "dojo/parser": null };
			if (! (dependency in this._modules)) {
				this._requires.push(dependency);
				this._modules[dependency] = null;
			}			
		},
		
		/**
		 * Hook to allow href'd pages to require Dojo modules before the markup is parsed.
		 * The specified dependency can take one of two forms:
		 *  - "foo.bar.phoo" -- Legacy "dot notation" for modules that are dependencies for
		 *                      Dojo 1.6 and above.  For Dojo 1.7 and above all dots/periods (".")
		 *                      are replaced with forward slashes ("/").
		 *  
		 *  - "foo/bar/phoo" -- AMD "slash notation" for modules that are dependencies for
		 *                      Dojo 1.7 and above.  These modules are not loaded if running
		 *                      under Dojo 1.6.
		 *                      
		 * Example: 
		 * _container_.require("dojo/dom"); 			// for Dojo 1.7 or later
		 * _container_.require("dijit.form.Button");	// for Dojo 1.6 or later
		 */
		require: function(dependency) {
			var dependencyKey = dependency.replace(/\./g,"/");
			this._recordDependency(dependencyKey);
		},

		/**
		 * Hook to allow href'd pages to require Dojo i18n modules before the markup is parsed.
		 * The dependency is specified in the legacy fashion like dojo.requireLocalization()
		 * function.  If running under Dojo 1.6, the dependency is loadded using 
		 * dojo.requireLocalization().  If running under Dojo 1.7 the dependency is translated
		 * into "dojo/i18n!" + packageName + "/nls/" + bundleName.  Where "packageName" has 
		 * every dot/period (".") replaced with a forward slash ("/").
		 * 
		 * Example: 
		 * _container_.requireLocalization("ibm.myproduct", "messages");
		 */
		requireLocalization: function(packageName, bundleName) {
			var dependencyKey = "dojo/i18n!" + packageName.replace(/\./g,"/") + "/nls/" + bundleName;
			this._recordDependency(dependencyKey);
		},
		
		/**
		 * Registers a pre-parse callback to be triggered AFTER module dependencies have been 
		 * loaded but BEFORE the HTML markup has been parsed.  Each specified callback is 
		 * passed a single parameter that is an associative array of AMD-style module names
		 * to the actual module that was loaded via "require" and "requireLocalization" functions.
		 * For the "requireLocalization" function running under Dojo 1.6, an actual call to
		 * "dojo.getLocalization" is done to obtain the bundle after requiring it.  Modules that
		 * were specified to "require()" function using AMD "slash notation" are excluded from
		 * the associative array if running under Dojo 1.6.  Localization modules are provided
		 * under their AMD-style "dojo/i18n!" path.  Example:
		 * 
		 * var onChangeCallback = null;
		 * _container_.registerPreParse(function(modules) {
		 *    // get the "dom" module for AMD or use "dojo" for legacy loader
		 *    var dom = modules["dojo/dom"] ? modules["dojo/dom"] : dojo;  
		 *    
		 *    onChangeCallback = new function(value) {
		 *       var node = dom.byId("messageNode");
		 *       messageNode.innerHTML = value;
		 *    }
		 *  }
		 */
		registerPreParse: function(preParseFunc) {
			if (!preParseFunc) return;
			if (!this._preParses) this._preParses = [ ];
			this._preParses.push(preParseFunc);
		},
		
		/**
		 * This function is called after module dependencies have been loaded and prior to parsing
		 * the content and handles triggering all "preParse" callbacks that have been registered.
		 */
		_preParse: function(modules) {
			if (this._preParses) {
				for (var index = 0; index < this._preParses.length; index++) {
					var func = this._preParses[index];
					func.call(null, this._modules);
				}
				delete this._preParses;
				this._preParses = null;
			}
		},
		
		/**
		 * Registers a post-parse callback to be triggered AFTER module dependencies have been 
		 * loaded and AFTER the HTML markup has been parsed.  Each specified callback is 
		 * passed a single parameter that is an associative array of AMD-style module names
		 * to the actual module that was loaded via "require" and "requireLocalization" functions.
		 * For the "requireLocalization" function running under Dojo 1.6, an actual call to
		 * "dojo.getLocalization" is done to obtain the bundle after requiring it.  Modules that
		 * were specified to "require()" function using AMD "slash notation" are excluded from
		 * the associative array if running under Dojo 1.6.  Localization modules are provided
		 * under their AMD-style "dojo/i18n!" path.  Example:
		 * 
		 * _container_.require("dojo/dom"); 			// for Dojo 1.7 or later
		 * _container_.require("dijit.form.Button");	// for Dojo 1.6 or later
		 * _container_.requireLocalization("ibm.myproduct", "messages");
		 * 
		 * _container_.registerPostParse(function(modules) {
		 *    // get the "dom" module for AMD or use "dojo" for legacy loader
		 *    var dom = modules["dojo/dom"] ? modules["dojo/dom"] : dojo;
		 *    
		 *    var Button = modules["dijit/form/Button"]; // same for both AMD and legacy
		 * 
		 *    var bundle = modules["dojo/i18n!ibm/myproduct/nls/messages"];
		 *    
		 *    var node = dom.byId("buttonNode");
		 *    
		 *    // progammatically create the widget
		 *    var button = new Button({label: bundle.buttonLabel}, node);
		 *  }
		 * 
		 */
		registerPostParse: function(postParseFunc) {
			if (!postParseFunc) return;
			if (!this._postParses) this._postParses = [ ];
			this._postParses.push(postParseFunc);
		},
		
		/**
		 * This function is called after parsing the content and handles triggering all
		 * "postParse" callbacks that have been registered.
		 */
		_postParse: function(modules) {
			if (this._postParses) {
				for (var index = 0; index < this._postParses.length; index++) {
					var func = this._postParses[index];
					func.call(null, this._modules);
				}
				delete this._postParses;
				this._postParses = null;
			}
			
			// we no longer want to defer resolution of the "Deferred" used for loading
			this._deferResolve = false;
			
			// clear out the requires and modules of the idx.layout.ContentPane since they
			// have all been used and handled at this point
			if (this._requires) delete this._requires;
			if (this._modules) delete this._modules;
			this._requires = null;
			this._modules = null;			
		},
		
		/**
		 * Internal method for loading all dependencies that have been registered and then 
		 * calling the specified callback with an associative array of module names mapped
		 * to the value for the loaded modules.
		 */
		_handleRequires: function(callback) {
			if (!this._modules) this._modules = { "dojo/parser" : null };
			if (!this._requires) this._requires = [ "dojo/parser" ];
			
			var self = this;
			require(this._requires, function() {
				for (var index = 0; index < self._requires.length; index++) {
					self._modules[self._requires[index]] = arguments[index];
				}
				if (callback) {
					callback.call(null, self._modules);
				}
			});
		},
		
		/**
		 * Overridden to avoid competing attempts to call "_onLoadHandler" from the base class
		 * which may assume loading is complete because the call to the ContentSetter completed.
		 * This may not be the case if the ContentSetter triggered asynchronous loading of 
		 * modules (e.g.: "require" returns before it has loaded all modules and simply triggers
		 * the callback when it is complete).
		 * 
		 * This version of the method respects the "_deferResolve" flag to indicate that resolving
		 * of the onLoadDeferred object should be deferred until the asynchronous tasks have 
		 * completed as well as the "_resolved" flag which is reset to "false" every time the 
		 * content is set and set to "true" whenever this method is called with "_deferResolve"
		 * set to false.
		 */
		_onLoadHandler: function(data) {
			// handle destroy widget with un-cancelled xhr callback 
			if(this._destroyed) return;
			// check if we are deferring to the asynchronous thread that may be requiring modules
			if (this._deferResolve) return;
			
			// if no longer deferring then check if another thread already resolved it for us
			if (this._resolved) return;
			
			// mark it as resolved and delegate to the base implementation
			this._resolved = true;
			this.inherited(arguments);
		},
		
		/**
		 * Sets up CSS class.
		 */
		buildRendering: function(){
			this.inherited(arguments);

			dDomClass.add(this.domNode, "idxContentPane");
			
		},
		
		/**
		 *
		 */
		_setInferLabelAttr: function(value) {
			var oldValue = this.inferLabel;
			this.inferLabel = value;
			if (value == oldValue) return;
			
			if (this.value) {
				var title = iString.nullTrim(this.title);
				if (title && !this._explicitLabel) {
					this._inferLabel();
				}
			} else {
				var label = iString.nullTrim(this.label);
				if ((label == title) && !this._explicitLabel) {
					this.set("label", "");
				}
			}
		},
		
		/**
		 * Handles making the title into the label if no explicit label is provided.
		 */
		_setTitleAttr: function(value) {
			this.title = value;
			if (this.titleNode) this.titleNode.innerHTML = this.title;
			var title = iString.nullTrim(value);
			
			// check if we are inferring the label
			if (! this.inferLabel) return;
			
			// if we have an explicit label then we do not override it
			if (this._explicitLabel) return;
			
			// check if the title is being set to empty-string or null - this is typical of
			// stack controllers which want to avoid the mouse-hover display of the title on
			// the content pane -- in such cases we want to keep the title as a label
			if (!title) return;
			
			// otherwise set the label using the title
			this._inferLabel();
		},

		/**
		 * Internal method to infer the label using the current title.
		 */
		_inferLabel: function() {
			this._inferringLabel = true;
			this.set("label", this.title);
			this._inferringLabel = false;
		},
				 
		/**
		 * Internal method to update the label.
		 */
		_updateLabel: function(oldValue, label) {
			if ((oldValue) && (!label)) {
				var labelAttr = dDomAttr.get(this.domNode, "aria-label");
				// check if label was set externally (if so, then do not remove it)
				if (labelAttr == oldValue) {
					dDomAttr.remove(this.domNode, "aria-label");
				}
				
			} else if ((!oldValue) && (label)) {
				var labelledBy = iString.nullTrim(dDomAttr.get(this.domNode, "aria-labelledby"));
				var role = iString.nullTrim(dDomAttr.get(this.domNode, "role"));
				
				var labelAttr = iString.nullTrim(dDomAttr.get(this.domNode, "aria-label"));
				// check if label was set externally (if so, then do not override it)
				if (role && !labelledBy && !labelAttr) {
					dDomAttr.set(this.domNode, "aria-label", label);
				}				
			} else {
				var labelledBy = iString.nullTrim(dDomAttr.get(this.domNode, "aria-labelledby"));
				var role = iString.nullTrim(dDomAttr.get(this.domNode, "role"));
				
				// old value existed and is being replaced
				var labelAttr = iString.nullTrim(dDomAttr.get(this.domNode, "aria-label"));
				// check if label was set externally (if so, then do not override it)
				if (role && !labelledBy && (this._explicitLabel || (labelAttr == oldValue))) {
					dDomAttr.set(this.domNode, "aria-label", label);
				}				
			}		
		},
		
		/**
		 * Override to handle setting an off-screen label when the label is provided.
		 */
		_setLabelAttr: function(value) {
			var label = iString.nullTrim(value);
			
			// check if the label is being implicitly or explicitly set
			if (this._inferringLabel && (value === this.title)) this._explicitLabel = false;
			else this._explicitLabel = (label != null);
			var oldValue = iString.nullTrim(this.label);
			this.label = value;
			
			// if no change then return
			if (oldValue == label) return;
			this._updateLabel(oldValue, label);			
		},
		
		/**
		 * Override to add hooks for asynchronous require when getting content by href.
		 */
		_setContent: function(cont, isFakeContent){
			if (!this.href) {
				this.inherited("_setContent", arguments);
				return;
			}
			
			// override dijit.layout.ContentPane._setContent, to enable path adjustments
			// COPIED FROM DOJO 1.7.2 dojox.layout.ContentPane and modified
			//
			var self = this;
			var setter = this._contentSetter;
			if(! (setter && setter instanceof ContentSetter)) {
				setter = this._contentSetter = new ContentSetter({
					contentPane: self,
					node: this.containerNode,
					_onError: dLang.hitch(this, this._onError),
					onContentError: dLang.hitch(this, function(e){
						// fires if a domfault occurs when we are appending this.errorMessage
						// like for instance if domNode is a UL and we try append a DIV
						var errMess = this.onContentError(e);
						try{
							this.containerNode.innerHTML = errMess;
						}catch(e){
							console.error('Fatal '+this.id+' could not change content due to '+e.message, e);
						}
					})/*,
					_onError */
				});
			};
			setter.set("fakeContent", isFakeContent);
			setter.set("content", cont);
						
			this._deferResolve = true;
			this._resolved     = false;
			this.inherited("_setContent", arguments);
		},
		
		/**
		 * Creates the single child widget.
		 * @private
		 */
		_onShow: function(){
			this.inherited(arguments);

			if(this._contentWidget){
				if(this._contentWidget._onShow){
					this._contentWidget._onShow();
				}
			}else{
				// delayed content widget creation
				if(this.contentClass){
					var t = this;
					setTimeout(function(){
						// requiring already loaded modules won't suffer much performance hit
						dRequireFunc.apply(t, [t.contentClass]);
					}, 0);
				}
			}
		},
		
		/**
		 * Performs the actual control creation and placement, after the
		 * module has been required and appropriate ready callbacks have fired
		 * (this callbacks are version dependent)
		 * @private
		 */
		_loadContent: function() {
			var contentClass = dLang.getObject(this.contentClass);
			var contentArgs = this.contentArgs;
			if(dLang.isString(contentArgs)){
				var jsonText = "{" + contentArgs + "}";
				contentArgs = iUtil.fromJson(jsonText);
			}
			// add reference to myself so content widget can update tab title etc. 
			contentArgs.container = this;
			
			this._contentWidget = new contentClass(contentArgs);
			
			// call postCreate callback if set
			if (this.contentCreate && typeof(this.contentCreate) == "function") {
				this.contentCreate(this._contentWidget);
			}
			
			dDomConstruct.place(this._contentWidget.domNode, this.domNode);
			this._contentWidget.startup();
			this._layoutChildren();
		}
	  });
	  
	  return ContentPane;
	  
	} // end _idxLayoutContentPane
);