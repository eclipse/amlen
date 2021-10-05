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

define(["dojo/_base/array",
        "dojo/_base/declare",
		"dojo/_base/lang",
		"dojo/_base/window",
		"dojo/aspect",
		"dojo/query",
		"dojo/dom-attr",
		"dojo/dom-class",
		"dojo/dom-construct",
		"dojo/dom-style",
		"dojo/i18n",
		"dojo/keys",
		"dojo/string",
		"dijit/_base/popup",
		"dijit/place",
		"dijit/registry",
		"dijit/_Widget",
		"dijit/_TemplatedMixin",
		"idx/util",
		"idx/resources",
		"dojo/NodeList-dom",
		"dojo/i18n!../nls/base",
		"dojo/i18n!./nls/base",
		"dojo/i18n!./nls/Header" ],
        function(_array,
		         declare,
				 _lang,
				 _window,
				 aspect,
				 query,
				 domattr,
				 domclass,
				 domconstruct,
				 domstyle,
				 i18n,
				 keys,
				 string,
				 popup,
				 place,
				 registry,
				 _Widget,
				 _TemplatedMixin,
				 iUtil,
				 iResources){
		
	var oneuiRoot = _lang.getObject("idx.oneui", true); // for backward compatibility with IDX 1.2
	
	// ensure we're not relying on the old globals, ready for 2.0
	var dojo = {}, dijit = {};
	
	// these widgets will be loaded later if needed
	var Button = function(){ log.error("dijit/form/Button has been used without being loaded"); }
	var TextBox = function(){ log.error("dijit/form/TextBox has been used without being loaded"); }
	var MenuTabController = function(){ log.error("idx/layout/MenuTabController has been used without being loaded"); }

	/**
	 * Creates a new idx.app.Header
	 * @name idx.app.Header
	 * @class The Header widget generates the HTML and CSS to provide an
	 * IBM One UI header according to the design specification and templates.
	 * <p>
	 * To construct a header, initialise the widget with the required
	 * properties. The appropriate HTML and CSS is created immediately, and
	 * subsidiary dijit components may be created and marshalled. No dynamic
	 * layout is performed: once the HTML has been injected into the DOM,
	 * all layout is delegated to the renderer and associated CSS rules.
	 * </p>
	 * @augments dijit._Widget
	 * @augments dijit._TemplatedMixin
	 * @example
	 * var hdr = new idx.app.Header({ primaryTitle: "Hello" }, "myHeader");
	 */
	return oneuiRoot.Header = declare("idx.app.Header", [_Widget, _TemplatedMixin], 
	/** @lends idx.app.Header.prototype */
	{
		/**
		 * The base CSS class for the widget.
		 */
		baseClass: "idxHeader",
		
		/**
		 * The IBM Brand/product name.
		 * @type string
		 */
		primaryTitle: "",
		
		/**
		 * The desired style of primary (black) banner: "thick" or "thin".
		 * @type string
		 * @default "thin"
		 */
		primaryBannerType: "thin",
		
		/**
		 * A menu bar, which can contain items and popup menu items, which
		 * will be displayed as navigation actions/menus in the header. The
		 * menu bar may be supplied as an instance or by id or as a DOM node.
		 * @type string | dijit.MenuBar | DOMNode
		 */
		navigation: undefined,
		
		/**
		 * True (the default) if navigation menu items that have a popup
		 * menu associated with them are to show a drop-down arrow affordance.
		 * If false, drop-down arrows are not shown on navigation items.
		 * @type boolean
		 */
		showNavigationDropDownArrows: true,

		/**
		 * Specifies that a primary search box should be included in the
		 * header, and supplies the parameters for it. All the properties are
		 * optional:
		 * <ul>
		 * <li>
		 * entryPrompt: {string | function} A string containing the prompt
		 * text for entering the search terms, or a function (which will be
		 * called with no arguments) which returns the prompt text.
		 * </li>
		 * <li>
		 * submitPrompt: {string | function} A string containing the prompt
		 * text for submitting the search, or a function (which will be called
		 * with no arguments) which returns the prompt text.
		 * </li>
		 * <li>onChange: {function} A function which will be called whenever
		 * the text in the search box changes. The function will receive one
		 * argument, which is the text currently in the search box.
		 * </li>
		 * <li>
		 * onSubmit: {function} A function which will be called whenever
		 * the user submits a search (eg, by pressing enter, or activating a
		 * search affordance). The function will receive one argument, which
		 * is the text currently in the search box.
		 * </li>
		 * </ul>
		 * @type Object
		 */
		primarySearch: undefined,
				
		/**
		 * The identity of the user to be included in the header. All
		 * properties are optional.
		 * <ul>
		 * <li>
		 * displayName: {string | function} A string containing the displayable
		 * name of the current user, or a function (which will be called with
		 * no arguments) which returns the displayable name of the current
		 * user. The displayable name may include mark-up (for example,
		 * entities for accented characters, etc). A displayName should always
		 * be supplied whenever feedback of the user's identity is required.
		 * The displayName can be modified after construction by setting the
		 * "userDisplayName" property of the header.
		 * Examples: "Clark, D. J. (Dave)"
		 * </li>
		 * <li>
		 * displayImage: {string | Object | function} A string containing the
		 * URI of an image to be displayed alongside the user name or welcome
		 * message, or an HTML image object (or other suitable mark-up object)
		 * to be used as the image alongside the user name or welcome message,
		 * or a function (which will be called with no arguments) which returns
		 * either a string or an object to specify the image to use. If omitted,
		 * null or undefined, no image is shown. The displayImage can be
		 * modified after construction by setting the "userDisplayName" property
		 * of the header.
		 * </li>
		 * <li>
		 * messageName: {string | function} A string containing the displayable
		 * name of the current user as it should appear in the message shown
		 * in the header to confirm the user's identity, if that is different
		 * from the displayName (for example, a shortened or simplified form
		 * of the user's name might be used as the messageName). If messageName
		 * is not supplied, the displayName is used. Note that displayName
		 * should still be supplied as well as messageName, because although it
		 * is the messageName that is substituted into the message for display,
		 * the displayName is also added as alternative text/title to add clarity
		 * for the user. The messageName can be set or modified after construction
		 * by setting the "userMessageName" property of the header.
		 * Examples: "Dave", "No&euml;l"
		 * </li>
		 * <li>
		 * message: {string | function} A string containing the message to be
		 * shown in the header to confirm the current user's identity, or a
		 * function (which will be called with no arguments) which returns
		 * the message to be shown. The string pattern will have the following
		 * substitutions applied:
		 * 	<ul>
		 * 	<li>
		 * 	${messageName} - the message name of the current user, if supplied,
		 * 	othewise the display name is used, if supplied
		 * 	</li>
		 * 	<li>
		 * 	$(displayName} - the display name of the current user
		 * 	</li>
		 * 	</ul>
		 * The message may include mark-up (for example, entities for accented
		 * characters, etc). If message is not supplied, the message that is
		 * used is "${messageName}". The message can be set or modified after
		 * construction by setting the "userMessage" property of the header.
		 * Examples: "Welcome back, ${messageName}",
		 * "Welcome, new user"
		 * </li>
		 * <li>
		 * actions: {string | dijit.Menu | dijit.MenuItem} A dijit.Menu to be
		 * used as the popup of available actions for the current user. If a
		 * single dijit.MenuItem is supplied, the current user name will be
		 * presented as a simple action and onClick will be triggered on the
		 * menu item when that action is selected. The menu or item may be
		 * supplied as an instance or by id.
		 * </li>
		 * </ul>
		 * @type Object
		 */
		user: undefined,
				
		/**
		 * True (the default) if a drop-down arrow affordance is to be shown
		 * on the user identification when a popup menu of user actions is supplied.
		 * If false, a drop-down arrow is not shown on the user identification.
		 * @type boolean
		 */
		showUserDropDownArrow: true,

		/**
		 * A dijit.Menu to be used as the popup of available site settings
		 * actions. If a single dijit.MenuItem is supplied, a simple site
		 * settings action will be presented and onClick will be triggered on
		 * the menu item when that action is selected.
		 * @type dijit.Menu | dijit.MenuItem
		 */
		settings: undefined,
		
		/**
		 * True (the default) if a drop-down arrow affordance is to be shown on the
		 * site settings icon when a popup menu of site settings items is supplied.
		 * If false, a drop-down arrow is not shown on the site settings icon.
		 * @type boolean
		 */
		showSettingsDropDownArrow: true,

		/**
		 * A dijit.Menu to be used as the popup of available site help actions. 
		 * If a single dijit.MenuItem is supplied, a simple site help action
		 * will be presented and onClick will be triggered on the menu item when
		 * that action is selected.
		 * @type dijit.Menu | dijit.MenuItem
		 */
		help: undefined,
	
		/**
		 * True (the default) if a drop-down arrow affordance is to be shown on the
		 * site help icon when a popup menu of site help items is supplied.
		 * If false, a drop-down arrow is not shown on the site help icon.
		 * @type boolean
		 */
		showHelpDropDownArrow: true,

		/**
		 * The context title which shows users where they are, for example
		 * if they have arrived here by selecting a menu item.
		 * @type string
		 */
		secondaryTitle: "",

		/**
		 * The desired style of secondary (context) banner: "blue" or "lightgrey".
		 * @default "blue"
		 * @type string
		 */
		secondaryBannerType: "blue",
		
		/**
		 * A subtitle which gives additional context information.
		 * @type string
		 */
		secondarySubtitle: "",		
		
		/**
		 * Text containing additional context information, such as when page
		 * content was last updated and by whom.
		 * @type string
		 */
		additionalContext: "",
		
		/**
		 * An array of objects defining actions which will be displayed as
		 * action buttons in the context part of the header. Each object
		 * must contain the following properties:
		 * <ul>
		 * <li>
		 * label: text label for the action
		 * </li>
		 * <li>
		 * onClick: click handler for the action button
		 * </li>
		 * </ul>
		 * @type Object[]
		 */
		actions: undefined,
		
		/**
		 * The id of a content container which is to be controlled by tabs
		 * included in the header, or the widget itself. Each ContentPane in
		 * the StackContainer may additionally include the following properties
		 * (all optional):
		 * <ul>
		 * <li>
		 * closable: {boolean} If true, a close affordance will be displayed on
		 * the corresponding tab and will close the content pane when activated.
		 * If false, or if this property is not set, no close affordance is shown.
		 * </li>
		 * <li>
		 * actions: {dijit.Menu} A menu of items to be presented when the
		 * drop-down affordance on the tab is activated. The drop-down
		 * affordance will be displayed on the tab if this property is set and
		 * either the tab is selected or alwaysShowMenu is true.
		 * </li>
		 * <li>
		 * alwaysShowMenu: {boolean} If true, a drop-down affordance will be
		 * displayed on the tab if the actions property has been set,
		 * regardless of whether the tab is currently selected. If false, a
		 * drop-down affordance will only be displayed on the tab if the
		 * actions property has been set AND the tab is currently selected.
		 * </li>
		 * </ul>
		 * @type string | dijit.StackContainer
		 */
		contentContainer: "",
		
		/**
		 * If true, content tabs will be placed on the same line as a context
		 * title and/or other secondary banner content. If false, the tabs will
		 * occupy their own row within the secondary banner. The default value is false.
		 * @type boolean
		 */
		contentTabsInline: false,		
		
		/**
		 * Specifies that a secondary search box should be included in the
		 * header, and supplies the parameters for it. All the properties are
		 * optional:
		 * <ul>
		 * <li>
		 * entryPrompt: {string | function} A string containing the prompt text
		 * for entering the search terms, or a function (which will be called
		 * with no arguments) which returns the prompt text.
		 * </li>
		 * <li>
		 * submitPrompt: {string | function} A string containing the prompt
		 * text for submitting the search, or a function (which will be called
		 * with no arguments) which returns the prompt text.
		 * </li>
		 * <li>
		 * onChange: {function} A function which will be called whenever the
		 * text in the search box changes. The function will receive one
		 * argument, which is the text currently in the search box.
		 * </li>
		 * <li>
		 * onSubmit: {function} A function which will be called whenever user
		 * submits a search (eg, by pressing enter, or activating a 
		 * search affordance). The function will receive one argument, which
		 * is the text currently in the search box.
		 * </li>
		 * </ul>
		 * @type Object
		 */
		secondarySearch: undefined,
				
		/**
		 * Specifies the desired layout mode, which can be "fixed" for a
		 * fixed-width layout independent of the browser width (extra space
		 * will be left at the side margins, and a scroll bar will appear if
		 * the browser window is too narrow) or "variable" for a variable-width
		 * layout that exploits the full browser window width (extra space will
		 * be left within the layout, which will change as the browser window
		 * is resized). 
		 * @default "variable".
		 * @type string
		 */
		layoutType: "variable",
		
		// The following properties (read-only) can be used to obtain the DOM
		// nodes of key elements of the constructed UI. These properties will
		// not be defined unless the corresponding UI element is used/required.
		//
		// Container nodes:
		//  domNode: outer containing DOM node
		//  primaryBannerNode: container (div) for all primary banner content
		//  navigationNode: contains a menu bar of navigation action items
		//  userNode: contains the user identity and actions display
		//  leadingSecondaryBannerNode: container (div) for all leading secondary banner content
		//  trailingSecondaryBannerNode: container (div) for all trailing secondary banner content
		//
		// Widget and content nodes:
		//  primaryTitleTextNode: contains primary title text/markup
		//  userTextNode: contains user identity text/markup
		//  primarySearchTextNode: the text field for primary search
		//  primarySearchButtonNode: the submit button for primary search
		//  secondaryTitleTextNode: contains secondary title text/markup
		//  secondarySubtitleTextNode: contains secondary subtitle text/markup
		//  contextActionNodes: array of action button nodes
		//  contentControllerNode: the content controller (tab bar)
		//  secondarySearchTextNode: the text field for secondary search
		//  secondarySearchButtonNode: the submit button for secondary search
		//  
		// Other nodes used internally:
		//  containerNode: hidden node used for declarative construction
		//  _mainContainerNode: container for the primary and secondary banners
		//  _leadingGlobalActionsNode: container for leading global actions
		//  _trailingGlobalActionsNode: container for trailing global actions
		//  _secondaryTitleSeparatorNode: text separating secondary title and subtitle
		//  _contextActionsNode: container for all context actions
		//
		/**
	 	 * The template HTML for the widget.
		 * @constant
		 * @type string
		 * @private
		 */
		templateString: '<div role="banner">' +
						'<div style="visibility: hidden; display: none;" data-dojo-attach-point="containerNode">' +
						'</div>' +
		                '<div data-dojo-attach-point="_mainContainerNode">' +
						'</div>' +
						'</div>',

		/**
	 	 * The primary textbox widget.
		 * @type object
		 */
		 _primarySearchTextBox: null,
		 
		 /**
	 	 * The secondary textbox widget.
		 * @type object
		 */
		 _secondarySearchTextBox: null,

		/**
		 * Return the textbox widget.
		 * @param {boolean} isSecondary indicating which widget should be returned
		 * the primary textbox or the secondary textbox
		 */		
		getTextBoxWidget: function(isSecondary) {
			return isSecondary ? this._secondarySearchTextBox : this._primarySearchTextBox;
		},

		/**
		 * Return the user identity display name, calling supplied functions
		 * where applicable.
		 * @private
		 */		
		_getComputedUserName: function(){
			return (this.user && (typeof this.user.displayName == "function")) ? this.user.displayName() : (this.user.displayName || "");
		},
		
		/**
		 * Return the user identity display image, calling supplied functions
		 * where applicable.
		 * @private
		 */		
		_getComputedUserImage: function(){
			return (this.user && (typeof this.user.displayImage == "function")) ? this.user.displayImage() : this.user.displayImage;
		},
		
		/**
		 * Return the user identity message, taking into account any custom
		 * message template and calling supplied functions where applicable.
		 * @private
		 */		
		_getComputedUserMessage: function(){
			// name to use in message: if no message name, use display name
			var displayname = this._getComputedUserName(),
				messagename = ((typeof this.user.messageName == "function") ? this.user.messageName() : this.user.messageName) || displayname,
				result = messagename;
			
			if(this.user && this.user.message){
				var message = (typeof this.user.message == "function") ? this.user.message() : this.user.message;
				
				result = string.substitute(message, this.user, function(value, key){
					switch(key){
						case "messageName": return messagename;
						case "displayName": return displayname;
						default: return value || "";
					}
				});
			}
			
			return result;
		},
		
		_setUserDisplayNameAttr: function(value){
			this.user = this.user || {};
			this.user.displayName = value;
			this._refreshUser();
		},
		
		_setUserDisplayImageAttr: function(value){
			this.user = this.user || {};
			this.user.displayImage = value;
			this._refreshUser();
		},
		
		_setUserMessageNameAttr: function(value){
			this.user = this.user || {};
			this.user.messageName = value;
			this._refreshUser();
		},
		
		_setUserMessageAttr: function(value){
			this.user = this.user || {};
			this.user.message = value;
			this._refreshUser();
		},
		
		/**
		 * Prepare a menu or menu bar to be presented from the One UI header widget.
		 *
		 * If cssclasses (array of strings) is specified, the open logic of the
		 * menu is overridden to ensure that if it is used as a popup then there
		 * is an outer wrapper DIV element always in place carrying the specified
		 * CSS class and containing the popup DOM elements. The menu is recursively
		 * processed to ensure this is true of all cascaded/popup menu items
		 * within the menu. Successive elements of the array are applied to each
		 * successive level of cascaded menu, and the last element is applied to 
		 * all subsequent cascade levels.
		 *
		 * if trigger (DOM Node) is specified, the menu is assumed to be a popup
		 * or cascaded menu, and is bound to the trigger.
		 *
		 * if around (DOM Node) is specified, the open logic of the popup is
		 * overridden to position the popup to that node.
		 * 
		 * @private
		 */
		_prepareMenu: function(menu, cssclasses, trigger, around, trailing, handles){
			if (!handles) handles = [];
			//Defect #12296: Place menu back to it's popupWraper after Header refresh.
			if(menu._popupWrapper){
				menu.placeAt(menu._popupWrapper);
			}
			if(cssclasses){
				if(cssclasses[0]){
					var handle2 = null;
					var handle1 = aspect.after(menu, "onOpen", function(){
						if(menu._popupWrapper){
							if(!menu._oneuiWrapper){
								// Create another wrapper <div> for our outermost oneui marker class.
								menu._oneuiWrapper = domconstruct.create("div", { "class": "idxHeaderContainer " + cssclasses[0] }, _window.body());

								var handle3 = null;
								handle3 = aspect.after(menu, "destroy", function(){
									domconstruct.destroy(menu._oneuiWrapper);
									delete menu._oneuiWrapper;
									if (handle3) handle3.remove();
								});
								handles.push(handle3);
							}
							
							menu._oneuiWrapper.appendChild(menu._popupWrapper);
						}
					});
					handle2 = aspect.after(menu, "destroy", function() {
						if (handle1) handle1.remove();
						if (handle2) handle2.remove();
					});
					
					handles.push(handle2);					
					handles.push(handle1);
				}
				
				var nextcssclasses = (cssclasses.length > 1) ? cssclasses.slice(1) : cssclasses,
					me = this;
				_array.forEach(menu.getChildren(), function(child){
					if(child.popup){				
						me._prepareMenu(child.popup, nextcssclasses, undefined, undefined, undefined, handles);
					}
					if(child.currentPage){
						domclass.add(child.domNode, "idxHeaderNavCurrentPage");
					}
				});
			}
			
			if(around){
				var _around = around;
				menu._scheduleOpen = function(/*DomNode?*/ target, /*DomNode?*/ iframe, /*Object?*/ coords){
					if(!this._openTimer){
						var ltr = menu.isLeftToRight(),
							where = place.around(//placeOnScreenAroundElement(
								popup._createWrapper(menu),
								_around,
								/*(menu.isLeftToRight() == (trailing ? false : true)) ? 
									{'BL':'TL', 'BR':'TR', 'TL':'BL', 'TR':'BR'} :
									{'BR':'TR', 'BL':'TL', 'TR':'BR', 'TL':'BL'},*/
								trailing ?
									[ "below-alt", "below", "above-alt", "above" ] : 
									[ "below", "below-alt", "above", "above-alt" ],
								ltr,
								menu.orient ? _lang.hitch(menu, "orient") : null);
							
						if(!ltr){
							where.x = where.x + where.w;
						}
						
						this._openTimer = setTimeout(_lang.hitch(this, function(){
							delete this._openTimer;
							this._openMyself({
								target: target,
								iframe: iframe,
								coords: where
							});
						}), 1);
					}
				}

				menu.leftClickToOpen = true;
			
				if(trigger){
					menu.bindDomNode(trigger);
				}
			}
			return handles;
		},
		
		_refreshUser: function(){
			if(this.userNode){
				// if userNode is unset, initialisation has not yet occurred (in which
				// case this refreshUser() method will be called again once initialisation
				// is complete, or the user area is not active in this header
				
				var name = this._getComputedUserName(),
					imgsrc = this._getComputedUserImage(),
					msg = this._getComputedUserMessage();
					
				domattr.set(this.userNode, "title", name);
				
				domattr.set(this.userImageNode, "src", imgsrc || "");
				domstyle.set(this.userImageNode, "display", imgsrc ? "block" : "none");
				
				this.userTextNode.innerHTML = msg;
				domclass.replace(this.userNode, msg ? "idxHeaderUserName" : "idxHeaderUserNameNoText", "idxHeaderUserName idxHeaderUserNameNoText");
			}
		},
		
		/**
		 * Construct UI from a template, injecting the resulting DOM items
		 * as children on of the supplied container node.
		 * @param {Object} containerNode
		 * @param {Object} templateString
		 */
		_injectTemplate: function(containerNode, templateString){
			
			// this code is generalised from _Templated.buildRendering

			// Look up cached version of template, or download to cache.
			var cached = _TemplatedMixin.getCachedTemplate(templateString, true);

			var node;
			if(_lang.isString(cached)){
				// if the cache returned a string, it contains replacement parameters,
				// so replace them and create DOM
				node = domconstruct.toDom(this._stringRepl(cached));
			}else{
				// if the cache returned a node, all we have to do is clone it
				node = cached.cloneNode(true);
			}

			// recurse through the node, looking for, and attaching to, our
			// attachment points and events, which should be defined on the template node.
			this._attachTemplateNodes(node, function(n,p){ return n.getAttribute(p); });
			
			// append resolved template as child of container
			containerNode.appendChild(node);
		},

		/**
		 * Standard widget lifecycle postMixInProperties() method.
		 * @private
		 */
		postMixInProperties: function(){
			this._nls = iResources.getResources("idx/app/Header", this.lang);
		},
		
		/**
		 * Handles setup of the header.
		 * @private
		 */
		_setup: function(){
			// summary:
			//     Generate the HTML and CSS for the header.
			
			// The following logic allocates all header items into either a
			// primary or a secondary banner, both of which are optional.
			//
			// The primary banner will accommodate:
			//  - primary title (if any)
			//  - navigation links/menus (if any)
			//  - search (if any, and if there's room)
			//  - user identity (if any)
			// The search will only be accommodated if at least one of the
			// other items is omitted; if a primary title, navigation
			// links/menus and user identity are all provided then the search
			// (if required) will be placed into the secondary banner.
			//
			// The primary banner will place the primary title at the top left
			// and the user identity at the top right. The navigation
			// links/menus and search will be placed between them, flowing onto
			// a second line if necessary.
			//
			// The secondary banner will accommodate:
			//  - secondary title (if any)
			//  - action links/menus (if any)
			//  - content controller (if any)
			//  - search (if any, and if not accommodated in primary banner)
			//
			// The secondary banner will place the secondary title at the top
			// left and the search at the top right. The action links/menus and
			// content controller will be placed between them, flowing onto a
			// second line if necessary.
			// 
			// Other layout schemes (eg for mobile) will require separate logic
			// not yet provided here.
			
			// First issue warnings for situations that may not be intended.
			if(this.contentContainer && this.secondaryBannerType && this.secondaryBannerType.toLowerCase() == "white"){
				// content tabs in "white" style are not supported
				require.log('*** Warning: Header will not display content tabs when secondaryBannerType is "white". Specify a different type to see content tabs.');
			}
			
			var show_primary_title = this.primaryTitle,
				show_primary_logo = true,
				show_primary_help = this.help,
				show_primary_sharing = this.sharing,
				show_primary_settings = this.settings,
				show_primary_user = this.user,
				show_primary_navigation = this.navigation,
				show_primary_search = this.primarySearch,
			    show_secondary_title = this.secondaryTitle || this.secondarySubtitle,
			    show_secondary_actions = this.contextActions,
			    show_secondary_search = this.secondarySearch,
			    show_content = this.contentContainer && (!this.secondaryBannerType || (this.secondaryBannerType.toLowerCase() != "white")),  // never show content tabs in "white" style (not supported)
				show_secondary_content = show_content && (this.contentTabsInline || !show_secondary_title),
				show_secondary_border = this.secondaryBannerType && (this.secondaryBannerType.toLowerCase() == "white"),
				show_tertiary_content = show_content && !show_secondary_content,
			    show_primary_items = show_primary_title || show_primary_logo || show_primary_help || show_primary_settings || show_primary_sharing || show_primary_user || show_primary_navigation || show_primary_search,  
			    show_secondary_items = show_secondary_title || show_secondary_actions || show_secondary_search || show_secondary_content,
				show_tertiary_items = show_tertiary_content,
				show_lip;
			
			if(show_primary_items || show_secondary_items || show_tertiary_items){
				domclass.add(this.domNode, "idxHeaderContainer");
				
				if(this.primaryBannerType && (this.primaryBannerType.toLowerCase() == "thick")){
					domclass.add(this._mainContainerNode, "idxHeaderPrimaryThick");
				}else{
					domclass.add(this._mainContainerNode, "idxHeaderPrimaryThin");
				}
				
				if(this.secondaryBannerType && ((this.secondaryBannerType.toLowerCase() == "lightgrey") || (this.secondaryBannerType.toLowerCase() == "lightgray"))){
					domclass.add(this._mainContainerNode, "idxHeaderSecondaryGray");
					domclass.add(this._mainContainerNode, show_tertiary_items ? "idxHeaderSecondaryGrayDoubleRow" : "idxHeaderSecondaryGraySingleRow");
					show_lip = show_primary_items;
				}else if(this.secondaryBannerType && (this.secondaryBannerType.toLowerCase() == "white")){
					domclass.add(this._mainContainerNode, "idxHeaderSecondaryWhite");
					domclass.add(this._mainContainerNode, show_tertiary_items ? "idxHeaderSecondaryWhiteDoubleRow" : "idxHeaderSecondaryWhiteSingleRow");
					show_lip = show_primary_items;
				}else{
					domclass.add(this._mainContainerNode, "idxHeaderSecondaryBlue");
					domclass.add(this._mainContainerNode, (show_tertiary_items) ? "idxHeaderSecondaryBlueDoubleRow" : "idxHeaderSecondaryBlueSingleRow");
					show_lip = show_primary_items && !show_secondary_items && !show_tertiary_items;
				}
				domclass.add(this._mainContainerNode, show_tertiary_items ? "idxHeaderSecondaryDoubleRow" : "idxHeaderSecondarySingleRow");
				
				if(this.layoutType && (this.layoutType.toLowerCase() == "fixed")){
					domclass.add(this._mainContainerNode, "idxHeaderWidthFixed");
				}else{
					domclass.add(this._mainContainerNode, "idxHeaderWidthLiquid");
				}				
			}
			
			// now load any additional modules we know we need
			var modules = [],
			    assigns = [],
				me = this;
			
			if(show_primary_search || show_secondary_search || show_secondary_actions){
				modules.push("dijit/form/Button");
				assigns.push(function(obj){ Button = obj; });
			}
			
			if(show_primary_search || show_secondary_search){
				modules.push("dijit/form/TextBox");
				assigns.push(function(obj){ TextBox = obj; });
			}
			
			if(show_content){
				modules.push("idx/layout/MenuTabController");
				assigns.push(function(obj){ MenuTabController = obj; });
			}
			
			require(modules, function(){
			
				for(var i=0; i<assigns.length; i++){
					assigns[i](arguments[i]);
				}
			
				// create the primary bar
				
				if(show_primary_items){
					me._injectTemplate(me._mainContainerNode,
										 '<div class="idxHeaderPrimary">' +
										 '<div class="idxHeaderPrimaryInner" data-dojo-attach-point="primaryBannerNode">' +
										 '<ul class="idxHeaderPrimaryActionsLeading" data-dojo-attach-point="_leadingGlobalActionsNode">' +
										 '</ul>' +
										 '<ul class="idxHeaderPrimaryActionsTrailing" data-dojo-attach-point="_trailingGlobalActionsNode">' +
										 '</ul>' +
										 '</div>' +
										 '</div>');
				}
				
				if(show_primary_logo && me._logoPosition == "leading"){
					me._renderLogo(me._leadingGlobalActionsNode);
				}
				
				if(show_primary_title){
					me._renderPrimaryTitle(me._leadingGlobalActionsNode);
				}
				
				if(show_primary_search){
					me._renderPrimarySearch(me._trailingGlobalActionsNode);
				}			
				
				if(show_primary_user){
					me._renderUser(me._trailingGlobalActionsNode);
				}
				
				if(show_primary_sharing){
					me._renderSharing(me._trailingGlobalActionsNode, show_primary_user);
				}
				
				if(show_primary_settings){
					me._renderSettings(me._trailingGlobalActionsNode, show_primary_user);
				}

				if(show_primary_help){
					me._renderHelp(me._trailingGlobalActionsNode, show_primary_settings || show_primary_sharing || show_primary_user);
				}
				/*if((me.user&&me.user.actions) || me.sharing || me.settings || me.help){
					me._trailingGlobalActionsNode.setAttribute("role", "menubar");
				}*/
				
				if(show_primary_logo && me._logoPosition != "leading"){
					me._renderLogo(me._trailingGlobalActionsNode);
				}
				
				if(show_primary_navigation){
					var navNode = domconstruct.create("li", {"class": "idxHeaderPrimaryNav"}, me._leadingGlobalActionsNode, "last");
					me._renderNavigation(navNode);
				}
				
				// create the blue lip
				
				if(show_lip){
					me._injectTemplate(me._mainContainerNode,
										 '<div class="idxHeaderBlueLip">' + 
										 '</div>');
				}
				
				// create the secondary bar
				
				if(show_secondary_items){
					me._injectTemplate(me._mainContainerNode,
										 '<div class="idxHeaderSecondary"> ' +
										 '<div class="idxHeaderSecondaryInner" data-dojo-attach-point="secondaryBannerNode"> ' +
										 '<div class="idxHeaderSecondaryLeading" data-dojo-attach-point="leadingSecondaryBannerNode">' + 
										 '</div>' + 
										 '<div class="idxHeaderSecondaryTrailing" data-dojo-attach-point="trailingSecondaryBannerNode">' +
										 '</div>' + 
										 '</div>' + 
										 '</div>');
				}
				
				if(show_secondary_title){
					me._renderSecondaryTitle(me.leadingSecondaryBannerNode);
				}
				
				if(show_secondary_content){
					me._renderContent(me.leadingSecondaryBannerNode, false);
				}

				if(show_secondary_actions){
					me._renderContextActions(me.trailingSecondaryBannerNode);
				}
				
				if(show_secondary_search){
					me._renderSecondarySearch(me.trailingSecondaryBannerNode);
				}			

				if(show_secondary_border){
					me._renderSecondaryInnerBorder(me.secondaryBannerNode);
				}
				
				// create the tertiary bar
				
				if(show_tertiary_content){
					me._renderContent(me._mainContainerNode, true);
				}
				
				// clear the "refresh required" flag
				if (me._refreshing > 0) me._refreshing--;
				if (me._refreshing == 0) {
					me._refreshRequired = false;
				} else {
					me._doRefresh();
				}
			});
		},
		
		/**
		 * Restores the widget to its initial state.
		 * @private
		 */
		_reset: function(){
			domclass.remove(this.domNode, "idxHeaderContainer");
			domclass.remove(this._mainContainerNode, "idxHeaderPrimaryThick");
			domclass.remove(this._mainContainerNode, "idxHeaderPrimaryThin");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryGray");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryGrayDoubleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryGraySingleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryWhite");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryWhiteDoubleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryWhiteSingleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryBlue");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryBlueDoubleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryBlueSingleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondaryDoubleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderSecondarySingleRow");
			domclass.remove(this._mainContainerNode, "idxHeaderWidthFixed");
			domclass.remove(this._mainContainerNode, "idxHeaderWidthLiquid");
			
			// move widgets temporarily to container node	
			if (this.help) {
				this.help = registry.byId(this.help);
				if (this.help) this.help.placeAt(this.containerNode);
			}
			this._clearHandles("_helpHandles");
			
			if (this.settings) {
				this.settings = registry.byId(this.settings);
				if (this.settings) this.settings.placeAt(this.containerNode);
			}
			this._clearHandles("_settingsHandles");
			
			if (this.sharing) {
				this.sharing = registry.byId(this.sharing);
				if (this.sharing) this.sharing.placeAt(this.containerNode);
			}
			
			this._clearHandles("_sharingHandles");
			
			if (this.user && this.user.actions) {
				this.user.actions = registry.byId(this.user.actions);
				if (this.user.actions) this.user.actions.placeAt(this.containerNode);
			}
			this._clearHandles("_actionsHandles");
			
			if (this.navigation) {
				this.navigation = registry.byId(this.navigation);
				if (this.navigation) this.navigation.placeAt(this.containerNode);
			}
			this._clearHandles("_navHandles");
			this._clearHandles("_controllerHandlers");
			
			// find and destroy any widgets in mainContainerNode
			_array.forEach(registry.findWidgets(this._mainContainerNode), function(child) {
				child.destroy();
			});
			
			// now remove the content under the _mainContainerNode
			var childNodes = [];
			_array.forEach(this._mainContainerNode.childNodes, function(node) {
				childNodes.push(node);
			});
			_array.forEach(childNodes, function(node) {
				domconstruct.destroy(node);
			});
			
			// clear out some nodes
			this._leadingGlobalActionsNode = null;
			this._trailingGlobalActionsNode = null;
			this.primaryTitleTextNode = null;
			this.primaryBannerNode = null;
			this.leadingSecondaryBannerNode = null;
			this.trailingSecondaryBannerNode = null;
			this._helpNode = null;
			this._settingsNode = null;
			this._sharingNode = null;
			this.userNode = null;
			this.userImageNode = null;
			this.userTextNode = null;
			this.primarySearchTextNode = null;
			this.primarySearchButtonNode = null;
			this.secondaryTitleTextNode = null;
			this._secondaryTitleSeparatorNode = null;
			this.secondarySubtitleTextNode = null;
			this.additionalContextTextNode = null;
			this._contextActionsNode = null;
			if (this.contextActionNodes) delete this.contextActionNodes;
			this.secondarySearchTextNode = null;
			this.secondarySearchButtonNode = null;
			this.contentControllerNode = null;
		},

		/** 
		 * Constructor
		 * @private
		 */
		constructor: function(args, node) {
			this._refreshing = 0;
		},
		
		/**
		 *
		 */
		destroy: function() {
			if (this.navigation) {
				if (this.navigation.destroyRecursive) this.navigation.destroyRecursive();
				else if (this.navigation.destroy) this.navigation.destroy();
				this.navigation = null;
			}
			this._clearHandles("_navHandles");
			
			if (this._controller) {
				if (this._controller.destroyRecursive) this._controller.destroyRecursive();
				else if (this._controller.destroy) this._controller.destroy();
				delete this._controller;
			}
			this._clearHandles("_controllerHandles");
			
			if (this.settings) {
				if (this.settings.destroyRecursive) this.settings.destroyRecursive();
				else if (this.settings.destroy) this.settings.destroy();
				this.settings = null;
			}
			this._clearHandles("_settingsHandles");
			
			if (this.sharing) {
				if (this.sharing.destroyRecursive) this.sharing.destroyRecursive();
				else if (this.sharing.destroy) this.sharing.destroy();
				this.sharing = null;
			}
			this._clearHandles("_sharingHandles");
			
			if (this.help) {
				if (this.help.destroyRecursive) this.help.destroyRecursive();
				else if (this.help.destroy) this.help.destroy();
				this.help = null;
			}
			this._clearHandles("_helpHandles");
			
			if (this.user && this.user.actions) {
				if (this.user.actions.destroyRecursive) this.user.actions.destroyRecursive();
				else if (this.user.actions.destroy) this.user.actions.destroy();
				this.user.actions = null;
				this.user = null;
			}
			this._clearHandles("_actionsHandles");
			
			this.inherited(arguments);
		},
		
		/**
		 * Clears out the handle array associated with the specified attribute name.
		 */
		_clearHandles: function(name) {
			if (! (name in this)) {
				return;
			}
			var handleArray = this[name];
			_array.forEach(handleArray, function(handle) {
				handle.remove(); 
			} );
			delete this[name];
		},
		
		/**
		 * Standard widget lifecycle buildRendering() method.
		 * @private
		 */
		buildRendering: function(){
			this.inherited(arguments);
			var cssOptions 
				= iUtil.getCSSOptions(this.baseClass + "Options", this.domNode, null, {logo:"trailing"});
			this._logoPosition = cssOptions.logo;
			console.log("LOGO POSITION: " + this._logoPosition);
			
			this._setup();
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setContentTabsInlineAttr: function(value) {
			var previous = this.contentTabsInline;
			this.contentTabsInline = value;
			if (previous != value) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setContentContainerAttr: function(value) {
			var previous = this.contentContainer;
			this.contentContainer = value;
			if (!iUtil.widgetEquals(previous, value)) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setActionsAttr: function(value) {
			var previous = this.actions;
			this.actions = value;
			if (previous !== value) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setAdditionalContextAttr: function(value) {
			var previous = this.additionalContext;
			var node = this.additionalContextTextNode;
			this.additionalContext = value;
			if (node) {
				node.innerHTML = (value) ? value : "";
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setSecondaryBannerTypeAttr: function(value) {
			var previous = this.secondaryBannerType;
			this.secondaryBannerType = value;
			if (previous != value) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setPrimaryTitleAttr: function(value) {
			var previous = this.primaryTitle;
			var node = this.primaryTitleTextNode;
			this.primaryTitle = value;
			if ((value && !previous) || (previous && !value)) {
				// check if we have gone from having a value to not having a value or vice versa
				this._refresh();
			} else if (node && value) {
				// if we are simply changing the text then just update it without a refresh
				node.innerHTML = value;
			}
		},

		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setSecondaryTitleAttr: function(value) {
			var previous = this.secondaryTitle;
			var node = this.secondaryTitleTextNode;
			this.secondaryTitle = value;
			if ((value && !previous) || (previous && !value)) {
				// check if we have gone from having a value to not having a value or vice versa
				this._refresh();
			} else if (node && value) {
				// if we are simply changing the text then just update it without a refresh
				node.innerHTML = value;
			}
		},

		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setLayoutTypeAttr: function(value) {
			var previous = this.layoutType;
			this.layoutType = value;

			if (this._mainContainerNode) {			
				if(this.layoutType && (this.layoutType.toLowerCase() == "fixed")){
					domclass.add(this._mainContainerNode, "idxHeaderWidthFixed");
				}else{
					domclass.add(this._mainContainerNode, "idxHeaderWidthLiquid");
				}				
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setSecondarySubtitleAttr: function(value) {
			var previous = this.secondarySubtitle;
			var node = this.secondarySubtitleTextNode;
			this.secondarySubtitle = value;
			if ((value && !previous) || (previous && !value)) {
				// check if we have gone from having a value to not having a value or vice versa
				this._refresh();
			} else if (node && value) {
				// if we are simply changing the text then just update it without a refresh
				node.innerHTML = value;
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setShowHelpDropDownArrowAttr: function(value) {
			var previous = this.showHelpDropDownArrow;
			this.showHelpDropDownArrow = value;
			
			if (this._helpNode) {
				domclass.toggle(this._helpNode, "idxHeaderDropDown", this.showHelpDropDownArrow);
			}
		},
		 
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setShowSettingsDropDownArrowAttr: function(value) {
			var previous = this.showSettingsDropDownArrow;
			this.showSettingsDropDownArrow = value;
			if (this._settingsNode) {
				domclass.toggle(this._settingsNode, "idxHeaderDropDown", this.showSettingsDropDownArrow);
			}			
		},
		 
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setShowSharingDropDownArrowAttr: function(value) {
			var previous = this.showSharingDropDownArrow;
			this.showSharingDropDownArrow = value;
			if (this._sharingNode) {
				domclass.toggle(this._sharingNode, "idxHeaderDropDown", this.showSharingDropDownArrow);
			}			
		},
		 
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setShowUserDropDownArrowAttr: function(value) {
			var previous = this.showUserDropDownArrow;
			this.showUserDropDownArrow = value;
			if (this.userNode) {
				domclass.toggle(this.userNode, "idxHeaderDropDown", this.showUserDropDownArrow);
			}
		},
		 
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setUserAttr: function(value) {
			var previous = this.user;
			this.user = value;
			if (previous === value) {
				// same user or from null to null, so do nothing
				return;
				
			} else if ((previous && !value) || (value && !previous)) {
				// was null, now not, or now null and was previously not null
				this._refresh();
				
			} else if (previous && previous.actions !== value.actions) {
				// previous and new value differ but have different actions
				this._refresh();
				
			} else {
				// we have a new user, but the same state for actions 
				this._refreshUser();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setShowNavigationDropDownArrowsAttr: function(value) {
			var previous = this.showNavigationDropDownArrows;
			this.showNavigationDropDownArrows = value;
			if (previous != value) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setPrimarySearchAttr: function(value) {
			var previous = this.primarySearch;
			this.primarySearch = value;
			if (previous !== value) {
				this._refresh();
			}
		},
		
		/**
		 * Setter to handle re-rendering the widget if needed.
		 * @private
		 */
		_setSecondarySearchAttr: function(value) {
			var previous = this.secondarySearch;
			this.secondarySearch = value;
			if (previous !== value) {
				this._refresh();
			}
		},
		
		/**
		 * Handler for setting the navigation.  This ensures
		 * that the header gets refreshed if needed or that at
		 * least the header gets marked as requiring a refresh.
		 *
		 * @param {Widget} widget The widget to use for the navigation
		 * @private
		 */
		_setNavigationAttr: function(widget) {
			var previous = this.navigation;
			this.navigation = widget;
			if (!iUtil.widgetEquals(previous, widget)) {
				this._refresh();
			}
		},
		
		/**
		 * Handler for setting the settings.  This ensures
		 * that the header gets refreshed if needed or that at
		 * least the header gets marked as requiring a refresh.
		 *
		 * @param {Widget} widget The widget to use for the settings
		 * @private
		 */
		_setSettingsAttr: function(widget) {
			var previous = this.settings;
			this.settings = widget;
			if (!iUtil.widgetEquals(previous, widget)) {
				this._refresh();
			}		
		},
		
		/**
		 * Handler for setting the sharing.  This ensures
		 * that the header gets refreshed if needed or that at
		 * least the header gets marked as requiring a refresh.
		 *
		 * @param {Widget} widget The widget to use for the sharing
		 * @private
		 */
		_setSharingAttr: function(widget) {
			var previous = this.sharing;
			this.sharing = widget;
			if (!iUtil.widgetEquals(previous, widget)) {
				this._refresh();
			}		
		},
		
		/**
		 * Handler for setting the help.  This ensures
		 * that the header gets refreshed if needed or that at
		 * least the header gets marked as requiring a refresh.
		 *
		 * @param {Widget} widget The widget to use for the help
		 * @private
		 */
		_setHelpAttr: function(widget) {
			var previous = this.help;
			this.help = widget;
			if (!iUtil.widgetEquals(previous, widget)) {
				this._refresh();
			}
		},
		
		/**
		 * Sets the flag to defer refresh.  This means that changes to
		 * the header will not immediately change the header but rather
		 * a call to "refresh()" will be required to trigger a change.
		 */
		deferRefresh: function() {
			this._refreshDeferred = true;
		},
		
		/**
		 * Clears the refresh deferral flag and refresshes the header.
		 */
		refresh: function() {
			if (this._refreshDeferred) this._refreshDeferred = false;
			if (this._refreshRequired) this._refresh();
		},
		
		/**
		 * Refreshes the header providing there has not been a deferral
		 * of the refresh, thus requiring a manual call to the public
		 * refresh() function to clear the deferred flag.
		 */
		_refresh: function() {
			this._refreshRequired = true;			
			if (this._started && (! this._refreshDeferred)) {
				this._refreshing++;
				if (this._refreshing == 1) {
					this._doRefresh();
				}
			} else {
				console.log("Deferring header refresh.  started=[ " + this._started + " ], deferred=[ "+ this._refreshDeferred + " ]");
			}
		},

		/**
		 * Handles performing the refresh.
		 */
		_doRefresh: function() {
			this._reset();
			this._setup();
		},
			
		/**
		 * Sets up the child using the region to assign defined
		 * elements like navigation, settings, and help.
		 *
		 * @param {Widget} child The child to setup
		 * @private
		 */
		_setupChild: function(child) {
			if (! ("region" in child)) return;
			
			var region = child.region;
			switch (region) {
			case "navigation":
				this.set("navigation", child);
				break;
			case "settings":
				this.set("settings", child);
				break;
			case "sharing":
				this.set("sharing", child);
				break;
			case "help":
				this.set("help", child);
				break;
			default:
				console.log("WARNING: Found child with unrecognized region: " + region);
				break;
			}			
		},
		
		/**
		 * Overridden so that the child is run through setup after being added
		 * and a refresh is triggered or at least a flag is set indicating it is
		 * needed.
		 */
		addChild: function(child) {
			this.inherited(arguments);
			this._setupChild(child);
		},
		
		/**
		 * Overridden to handle the special children so as to clear out
		 * help, navigation, or settings as needed.
		 */
		removeChild: function(child) {
			// handle the special children
			if (child === this.help) {
				this.set("help", null);
			}
			
			if (child === this.navigation) {
				this.set("navigation", null);
			}
			
			if (child === this.settings) {
				this.set("settings", null);
			}
			
			if (child === this.sharing) {
				this.set("sharing", null);
			}
			
			// defer the base implementation
			// the base implementation will only remove the help, navigation or settings
			// if this header was not setup after they were added as children (i.e.: if
			// they are still children of the hidden containerNode).  Otherwise, the 
			// removed widgets will simply be abandoned by the Header and be destroyed
			// upon the next refresh unless the caller places the child in a different
			// part of the DOM tree (outside the _mainContainerNode)
			this.inherited(arguments);
		},
		
		/**
		 * Standard widget lifecycle startup() method.
		 * @private
		 */
		startup: function(){
			// summary:
			//     Generate the HTML and CSS for the header.

			// call down to apply the template and base widget handling
			this.inherited(arguments);
			
			// defer refreshes
			this.deferRefresh();
			
			// get the children
			var children = this.getChildren();
			for (var index = 0; index < children.length; index++) {
				var child = children[index];
				this._setupChild(child);
			}

			// refresh the widget
			this.refresh();
		},
		
		_renderPrimaryTitle: function(domNode){
			this._injectTemplate(domNode,
			         			 '<li role="presentation">' +
			         			 '<span>' +
			         			 '<div class="idxHeaderPrimaryTitle" id="${id}_PrimaryTitle" data-dojo-attach-point="primaryTitleTextNode">' +
			         			 '${primaryTitle}' +
			         			 '</div>' +
			         			 '</span>' +
			         			 '</li>'); 
		},
		
		_renderLogo: function(domNode){
			console.log("RENDERING LOGO....");
			this._injectTemplate(domNode,
			         			 '<li role="presentation" class="idxHeaderPrimaryAction idxHeaderLogoItem">' +
			         			 '<span>' +
			         			 '<div class="idxHeaderLogoBox">' +
			         			 '<div class="idxHeaderLogo" alt="${_nls.ibmlogo}">' +
								 	'<span class="idxTextAlternative">${_nls.ibmlogo}</span>' +
			         			 '</div>' +
			         			 '</div>' +
			         			 '</span>' +
			         			 '</li>'); 
		},
		_renderSeparator: function(domNode){
			this._injectTemplate(domNode,
				'<li role="presentation" class="idxHeaderPrimaryAction idxHeaderSeparator"><span></span></li>');
		},
		_renderHelp: function(domNode, addSeparator){
			if(addSeparator){
				this._renderSeparator(domNode);
			}

			this._injectTemplate(domNode,
			         			 '<li class="idxHeaderPrimaryAction idxHeaderHelp">' +
			         			 '<a tabindex="0" href="javascript://" data-dojo-attach-point="_helpNode" title="${_nls.actionHelp}" role="presentation">' +
									 '<span class="idxHeaderHelpIcon">' +
									 	'<span class="idxTextAlternative">${_nls.actionHelp}</span>' +
									 '</span>' +
							         '<span class="idxHeaderDropDownArrow">' +
									 	'<span class="idxTextAlternative">(v)</span>' +
							         '</span>' +
			         			 '</a>' +
			         			 '</li>');
			
			if(this.help){
				this.help = registry.byId(this.help);
				this._clearHandles("_helpHandles");
				this._helpHandles = this._prepareMenu(this.help, [ "oneuiHeaderGlobalActionsMenu", "oneuiHeaderGlobalActionsSubmenu" ], this._helpNode, this._helpNode, true);
				domclass.toggle(this._helpNode, "idxHeaderDropDown", this.showHelpDropDownArrow);
				this._helpNode.setAttribute("role", "button");
				this._helpNode.setAttribute("aria-haspopup", true);
			}
		},
		
		_renderSettings: function(domNode, addSeparator){
			if(addSeparator){
				this._renderSeparator(domNode);
			}

			this._injectTemplate(domNode,
			         			 '<li class="idxHeaderPrimaryAction idxHeaderTools">' +
			         			 '<a tabindex="0" href="javascript://" data-dojo-attach-point="_settingsNode" title="${_nls.actionSettings}" role="presentation">' +
									 '<span class="idxHeaderSettingsIcon">' +
									 	'<span class="idxTextAlternative">${_nls.actionSettings}</span>' +
									 '</span>' +
							         '<span class="idxHeaderDropDownArrow">' +
									 	'<span class="idxTextAlternative">(v)</span>' +
							         '</span>' +
			         			 '</a>' +
			         			 '</li>'); 
			
			if(this.settings){
				this.settings = registry.byId(this.settings);
				this._clearHandles("_settingsHandles");
				this._settingsHandles = this._prepareMenu(this.settings, [ "oneuiHeaderGlobalActionsMenu", "oneuiHeaderGlobalActionsSubmenu" ], this._settingsNode, this._settingsNode, true);
				domclass.toggle(this._settingsNode, "idxHeaderDropDown", this.showSettingsDropDownArrow);
				this._settingsNode.setAttribute("role", "button");
				this._settingsNode.setAttribute("aria-haspopup", true);
			}
		},

		_renderSharing: function(domNode, addSeparator){
			if(addSeparator){
				this._renderSeparator(domNode);
			}

			this._injectTemplate(domNode,
			         			 '<li class="idxHeaderPrimaryAction idxHeaderTools">' +
			         			 '<a tabindex="0" href="javascript://" data-dojo-attach-point="_sharingNode" title="${_nls.actionShare}" role="presentation">' +
									 '<span class="idxHeaderShareIcon">' +
									 	'<span class="idxTextAlternative">${_nls.actionShare}</span>' +
									 '</span>' +
							         '<span class="idxHeaderDropDownArrow">' +
									 	'<span class="idxTextAlternative">(v)</span>' +
							         '</span>' +
			         			 '</a>' +
			         			 '</li>'); 
			
			if(this.sharing){
				this.sharing = registry.byId(this.sharing);
				this._clearHandles("_sharingHandles");
				this._sharingHandles = this._prepareMenu(this.sharing, [ "oneuiHeaderGlobalActionsMenu", "oneuiHeaderGlobalActionsSubmenu" ], this._sharingNode, this._sharingNode, true);
				domclass.toggle(this._sharingNode, "idxHeaderDropDown", this.showSharingDropDownArrow);
				this._sharingNode.setAttribute("role", "button");
				this._sharingNode.setAttribute("aria-haspopup", true);
			}
		},
		
		_renderUser: function(domNode){
			this._injectTemplate(domNode,
			                     '<li class="idxHeaderPrimaryAction">' + 
									'<a tabindex="0" href="javascript://" data-dojo-attach-point="userNode" class="idxHeaderUserNameNoText" role="presentation">' +
										'<span class="idxHeaderUserIcon">' +
											'<img data-dojo-attach-point="userImageNode" class="idxHeaderUserIcon" alt="" />' +
										'</span>' +
										'<span class="idxHeaderUserText" data-dojo-attach-point="userTextNode">' +
										'</span>' +
								        '<span class="idxHeaderDropDownArrow">' +
								        	'<span class="idxTextAlternative">(v)</span>' +
								        '</span>' +
									'</a>' +
			                     '</li>');			                     
			
			this._refreshUser();

			if(this.user && this.user.actions){
				this.user.actions = registry.byId(this.user.actions);
				this._clearHandles("_actionsHandles");
				this._actionsHandles = this._prepareMenu(this.user.actions, [ "oneuiHeaderGlobalActionsMenu", "oneuiHeaderGlobalActionsSubmenu" ], this.userNode, this.userNode, true);
				domclass.toggle(this.userNode, "idxHeaderDropDown", this.showUserDropDownArrow);
				this.userNode.setAttribute("role", "button");
				this.userNode.setAttribute("aria-haspopup", true);
			}
		},
		
		_renderNavigation: function(domNode){
			this.navigation = ((typeof this.navigation == "object") && ('nodeType' in this.navigation)) ? registry.byNode(this.navigation) : registry.byId(this.navigation);
			
			if(!this.navigation){
				require.log("WARNING: navigation widget not found");
			}else{
				this.navigation.placeAt(domNode);
				this.navigation.startup();
				
				var children = this.navigation.getChildren();
				if((children.length == 1) && (children[0].label == "")){
					// if there is just a single menu bar item with no text, make it a "home" icon
					var homeItem = children[0];
					domclass.toggle(homeItem.containerNode, "idxHeaderNavigationHome", true);
					domclass.toggle(homeItem.containerNode, "idxHeaderNavigationHomeButtonOnly", !homeItem.popup);
					domattr.set(homeItem.domNode, "title", this._nls.homeButton);
					domconstruct.place("<span class='idxTextAlternative'>" + this._nls.homeButton + "</span>", homeItem.containerNode, "first");
				}else if(this.showNavigationDropDownArrows){
					for(var i = 0; i < children.length; i++){
						if(children[i].popup){
							var nodes = query(".idxHeaderDropDownArrow", children[i].focusNode);
							domclass.toggle(children[i].domNode, "idxHeaderDropDown", true);
							if (nodes.length > 0) continue; // skip if arrow already injected
							this._injectTemplate(children[i].focusNode, '<span class="idxHeaderDropDownArrow"><span class="idxTextAlternative">(v)</span></span>'); 
						}
					}
				}
				
				// remove whitespace-only text nodes in the menu-bar, as these
				// can disrupt precise layout of the menu items
				var node = this.navigation.domNode.firstChild, del;
				while(node){
					del = node;
					node = node.nextSibling;
					if((del.nodeType == 3) && (!del.nodeValue.match(/\S/))){
						this.navigation.domNode.removeChild(del);
					}
				}
				
				this._clearHandles("_navHandles");
				this._navHandles = this._prepareMenu(this.navigation, [null, "oneuiHeaderNavigationMenu", "oneuiHeaderNavigationSubmenu"]);
			}
		},
		
		_renderPrimarySearch: function(domNode){
			this._injectTemplate(domNode,
			                     '<li class="idxHeaderSearchContainer">' +
			                     '<input type="text" data-dojo-attach-point="primarySearchTextNode" />' + 
			                     '<input type="image" data-dojo-attach-point="primarySearchButtonNode" />' +
			                     '</li>');
								 
			this.primarySearch.onChange = _lang.isFunction(this.primarySearch.onChange) ? this.primarySearch.onChange : new Function("value", this.primarySearch.onChange);
			this.primarySearch.onSubmit = _lang.isFunction(this.primarySearch.onSubmit) ? this.primarySearch.onSubmit : new Function("value", this.primarySearch.onSubmit);
			
			var me = this,
				entryprompt = ("entryPrompt" in this.primarySearch) ? this.primarySearch.entryPrompt : this._nls.searchEntry,  // ensure "" overrides default
				submitprompt = ("submitPrompt" in this.primarySearch) ? this.primarySearch.submitPrompt : this._nls.searchSubmit;  // ensure "" overrides default
			
			var text = new TextBox({
				trim: true,
				placeHolder: entryprompt,
				intermediateChanges: true,
				title: entryprompt
			},
			this.primarySearchTextNode);
			text.domNode.setAttribute("role", "search");
			text.domNode.setAttribute("aria-label", this.id + " " + this._nls.primarySearchLabelSuffix);
			text.own(aspect.after(text, "onChange", function(){ me._onPrimarySearchChange(text.attr("value")); }));
			text.own(text.on("keyup", function(event){ if(event.keyCode == keys.ENTER){ me._onPrimarySearchSubmit(text.attr("value")); } }));
			this._primarySearchTextBox = text;
			var button = new Button({
				label: submitprompt,
				showLabel: false,
				iconClass: "idxHeaderSearchButton"
			},
			this.primarySearchButtonNode);
			button.own(aspect.after(button, "onClick", function(){ me._onPrimarySearchSubmit(text.attr("value")); })); 
		},
		
		_renderSecondaryTitle: function(domNode){
			this._injectTemplate(domNode,
			                     '<span class="idxHeaderSecondaryTitleContainer">' +
								 '<span class="idxHeaderSecondaryTitle" id="${id}_SecondaryTitle"  data-dojo-attach-point="secondaryTitleTextNode">' +
			                     '${secondaryTitle}' +
			                     '</span>' +
			                     '<span class="idxHeaderSecondarySubtitle" data-dojo-attach-point="_secondaryTitleSeparatorNode">' +
			                     '&nbsp;&ndash;&nbsp;' +
			                     '</span>' +
			                     '<span class="idxHeaderSecondarySubtitle" data-dojo-attach-point="secondarySubtitleTextNode">' +
			                     '${secondarySubtitle}' +
			                     '</span>' +
								 '&nbsp;&nbsp;' +
			                     '<span class="idxHeaderSecondaryAdditionalContext" data-dojo-attach-point="additionalContextTextNode">' +
			                     '${additionalContext}' +
			                     '</span>' +
								 '</span>');
								 
			domstyle.set(this._secondaryTitleSeparatorNode, "display", (this.secondaryTitle && this.secondarySubtitle) ? "" : "none");
		},
		
		_renderContextActions: function(domNode){
			this._injectTemplate(domNode,
			                     '<div class="idxHeaderSecondaryActions" data-dojo-attach-point="_contextActionsNode"></div>');
			this.contextActionNodes = [];
			
			for(var i=0; i<this.contextActions.length; i++){
				this._injectTemplate(this._contextActionsNode,
									 '<button type="button" data-dojo-attach-point="_nextActionNode"></button>');
				new Button(this.contextActions[i], this._nextActionNode);
				this.contextActionNodes.push(this._nextActionNode);
				delete this._nextActionNode;
			}
		},
		
		_renderSecondarySearch: function(domNode){
			this._injectTemplate(domNode,
			                     '<div role="search" aria-label="' +
								 this.id +
								 ' ${_nls.secondarySearchLabelSuffix}" class="idxHeaderSearchContainer">' +
			                     '<input type="text" data-dojo-attach-point="secondarySearchTextNode" />' + 
			                     '<input type="image" data-dojo-attach-point="secondarySearchButtonNode" />' +
			                     '</div>');
			
			this.secondarySearch.onChange = _lang.isFunction(this.secondarySearch.onChange) ? this.secondarySearch.onChange : new Function("value", this.secondarySearch.onChange);
			this.secondarySearch.onSubmit = _lang.isFunction(this.secondarySearch.onSubmit) ? this.secondarySearch.onSubmit : new Function("value", this.secondarySearch.onSubmit);
			
			var me = this,
				entryprompt = ("entryPrompt" in this.secondarySearch) ? this.secondarySearch.entryPrompt : this._nls.searchEntry,  // ensure "" overrides default
				submitprompt = ("submitPrompt" in this.secondarySearch) ? this.secondarySearch.submitPrompt : this._nls.searchSubmit;  // ensure "" overrides default
			
			var text = new TextBox({
				trim: true,
				placeHolder: entryprompt,
				intermediateChanges: true,
				title: entryprompt
			},
			this.secondarySearchTextNode);
			text.own(aspect.after(text,"onChange",function(){ me._onSecondarySearchChange(text.attr("value")); }));
			text.own(text.on("keyup", function(event){ if(event.keyCode == keys.ENTER){ me._onSecondarySearchSubmit(text.attr("value")); } }));
			this._secondarySearchTextBox = text;
			var button = new Button({
				label: submitprompt,
				showLabel: false,
				iconClass: "idxHeaderSearchButton"
			},
			this.secondarySearchButtonNode);
			
			button.own(aspect.after(button, "onClick", function(){ me._onSecondarySearchSubmit(text.attr("value")); })); 
		},
		
		_renderSecondaryInnerBorder: function(domNode){
			this._injectTemplate(domNode,
			                     '<div class="idxHeaderSecondaryInnerBorder">' +
			                     '</div>');
		},
		
		_renderContent: function(domNode, includeInnerDiv){
			this._injectTemplate(domNode,
			                     '<div class="oneuiContentContainer">' +
								 (includeInnerDiv ? '<div class="oneuiContentContainerInner">' : '') +
			                     '<div data-dojo-attach-point="contentControllerNode"></div>' +
								 (includeInnerDiv ? '</div>' : '') +
			                     '</div>');	
			
			var controller = new MenuTabController({
				containerId: (typeof this.contentContainer === "string") ? this.contentContainer : this.contentContainer.id,
				"class": "dijitTabContainerTop-tabs",
				useMenu: this._tabMenu,
				useSlider: this._tabSlider,
				buttonWidget: _lang.extend(idx.layout._PopupTabButton, { tabDropDownText: "", tabSeparatorText: "|" })
			},
			this.contentControllerNode),
			me = this;
			
			
			this._clearHandles("_controllerHandles");
			this._controllerHandles = this._prepareMenu(controller._menuBtn, [ "oneuiHeader2ndLevMenu", "oneuiHeader2ndLevSubmenu" ]);
			this._controllerHandles.push(aspect.after(controller, "_bindPopup", function(page, tabNode, popupNode, popup){
				me._prepareMenu(popup, [ "oneuiHeader2ndLevMenu", "oneuiHeader2ndLevSubmenu" ], popupNode, tabNode, this._controllerHandles);
			}, true));
			
			controller.startup();
			// override the incredibly large width on the no-wrap tabstrip from Dojo
			domstyle.set(controller.containerNode, "width", "auto");
			this._controller = controller;
			
			// if the content container is already started, ensure the controller initialises correctly
			var container = registry.byId(this.contentContainer);
			if(container && container._started){
				controller.onStartup({ children: container.getChildren(), selected: container.selectedChildWidget });
			}
		},
		
		_onPrimarySearchChange: function(value){
			this.primarySearch.onChange(value);
		},
		
		_onPrimarySearchSubmit: function(value){
			this.primarySearch.onSubmit(value);
		},
		
		_onSecondarySearchChange: function(value){
			this.secondarySearch.onChange(value);
		},
		
		_onSecondarySearchSubmit: function(value){
			this.secondarySearch.onSubmit(value);
		}
		
	});
			
});
