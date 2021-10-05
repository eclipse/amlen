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
	"dojo/_base/event",
	"dojo/_base/lang",
	"dojo/_base/sniff",
	"dojo/date/locale",
	"dojo/dom-class",
	"dojo/dom-attr",
	"dojo/dom-style",
	"dojo/dom-geometry",
	"dojo/i18n",
	"dojo/keys",
	"dijit/focus",
	"dijit/_Widget",
	"dijit/_TemplatedMixin",
	"dijit/_WidgetsInTemplateMixin",
	"dojo/has!dojo-bidi?../bidi/widget/SingleMessage",
	"dojo/text!./templates/SingleMessage.html",
	"dojo/i18n!./nls/SingleMessage",
	"dojo/i18n!dijit/nls/common",
	"dojox/html/ellipsis"
], function(declare, array, event, lang, has, locale, domClass, domAttr, domStyle, domGeometry, i18n, keys,focus, _Widget, _TemplatedMixin,
			_WidgetsInTemplateMixin, bidiExtension, template, singleMessageNls, commonNls){
	var iMessaging = lang.getObject("idx.oneui.messaging", true); // for backward compatibility with IDX 1.2
	
	/**
	 * @name idx.widget.SingleMessage
	 * @class SingleMessage is implemented according to IBM One UI(tm) 
	 * <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y11&vsub=*&hsub=*&options=M&openpanes=0000011000">Messaging Standard</a></b>.
	 * It provides a convenient way to create One UI compliant messages in a designated area. SingleMessage is highly configurable with following options:
	 * <ul>
	 *    <li><b>Message type</b>: SingleMessage supports 7 different message types. It can be configured via setting the 'type' property.</li>
	 *    <li><b>Message ID</b>: The ID of a SingleMessage can be configured via  setting the 'messageId' property. Setting the 'showId' property can trun on/off the Message ID.</li>
	 *    <li><b>Timestamp</b>: The timestamp of a SingleMessage can be configured via  setting the 'date' property. In addtion, the date format can be set with the
	 *    'dateFormat' property</li>
	 *    <li><b>Message text</b>: The message text can be configured via setting the 'title' property. The message text gets truncated with an ellipsis automatically when a SingleMessage resizes.</li>
	 *    <li><b>Explanation</b>: The message explanation can be confgiured via setting the 'description' property.</li>
	 *    <li><b>Refresh Button</b>: A SingleMessage contains a refresh link after the message explanation by default. It can be turned off by setting 'showRefresh' property to false.</li>
	 *    <li><b>View All</b> link: A SingleMessage contains a View All(n) link by default. The message number 'n' can be configured with 'messageNumber' property. It can be turned off by setting
	 *     'showAction' property to false.
	 *    </li>
	 * </ul>
	 * @augments dijit._Widget
	 * @augments dijit._TemplatedMixin
	 * @augments dijit._WidgetsInTemplateMixin
	 * @example
	 * Programmatic Example:
	 * <pre>
	 * new idx.widget.SingleMessage({
	 *  type: 'error',
	 *  title: 'Error message with long timestamp',
	 *  dateFormat: {datePattern: 'dd MMMM y &nbsp', timePattern: 'hh:mm a'},
	 *  messageId: 'CAT123456',
	 *  messageNumber: 7,
	 *  style: 'width: 970px;',
	 *  description: 'Here is the detail message description, it can be configured via setting the \'description\' parameter of a SingleMessage widget. By default the message description is the same as the message title.'
	 * }, domNode);
	 * </pre>
	 * Declarative Example:
	 * <pre>
	 *    &lt;div data-dojo-type="idx.widget.SingleMessage" data-dojo-props="type: 'error', title: 'Error message with long timestamp',
	 *    dateFormat:{datePattern: 'dd MMMM y &nbsp', timePattern: 'hh:mm a'}, messageId: 'CAT123456', messageNumber: 7, style: 'width: 970px;', description: 'Here is the detail message description, it can be configured via setting the \'description\' parameter of a SingleMessage widget. By default the message description is the same as the message title.'"></div>
	 * </pre>
	 */
	var baseClassName = has("dojo-bidi")? "idx.widget.SingleMessage_" : "idx.widget.SingleMessage";
	iMessaging.SingleMessage = declare(baseClassName, [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin],
	/**@lends idx.widget.SingleMessage.prototype*/
	{
		templateString: template,
		
		baseClass: "idxSingleMessage",
		
		tabIndex: "0",
		
		/**
		 * The ID of a SingleMessage. Can be turned off by setting 'showId' to false.
		 * For message type 'error', 'warning', 'critical', message id is required.
		 * @type String
		 */
		messageId: "",
		
		/**
		 * The type of a Single Message. Can be one of 'error', 'warning', 'success',
		 * 'information', 'critical', 'attention', 'compliance'.
		 * @type String
		 * @default "error"
		 */
		type: "error",

		
		/**
		 * The timestamp of a Single Message. The timestamp format can be configured with 'dateFormat'.
		 * Example:
		 * date: new Date()
		 * @type Date
		 */
		date: new Date(),
		
		/**
		 * The options being used for format the timestamp.
		 * Example:
		 * <pre>
		 * dateFormat: {
		 * 	formatLength: "medium",
		 * 	locale: this.lang
		 * }
		 * </pre>
		 * @type dojo.date.locale.__FormatOptions
		 */
		dateFormat: {
			formatLength: "medium",
			locale: this.lang
		},
		
		/**
		 * The message text of a SingleMessage. It will be trancated with an ellipsis if needed when
		 * SingleMessage resizes.
		 * @type String
		 */
		title: "",
		
		/**
		 * The text of the action link. By default, the action link is used as the View All link.
		 * The value is loaded from the nls bundle. In most cases, you should not change it. But in special cases,
		 * you still can change it with a string.
		 * @type String
		 */
		actionText: "",
		
		/**
		 * 'showId' decides whether to show the message ID. By default, for success message and information message, it is
		 * turned off.
		 * Note: For error, critical, warning messages, message ID is required and cannot be turned off.
		 * @type Boolean
		 * @default true
		 */
		showId: true,
		
		/**
		 * 'showAction' decides whether to show the action link (ususally, it's View All link).
		 * @type Boolean
		 * @default true
		 */
		showAction: true,
		
		/**
		 * 'showRefresh' decides whether to show the refresh link in the message description part.
		 * @type Boolean
		 * @default true
		 */
		showRefresh: true,
		
		/**
		 * 'showDetailsLink' decides whether to show the More Details link in the message description part.
		 * @type Boolean
		 * @default true
		 */
		showDetailsLink: true,
		
		/**
		 * 'showTimestamp' decides whether to show the timestamp.
		 * @type Boolean
		 * @default true
		 */
		showTimestamp: true,
		
		/**
		 * The message number to be put in the View All link 'View All(n)'.
		 * @type Integer | String
		 * @default "n"
		 */
		messageNumber: "n",
		
		/**
		 * The explanation of a SingleMessage.
		 * @type String
		 */
		description: "",
		
		/**
		 * 'collapsed' decides whether the explanation part is hidden.
		 * @type Boolean
		 */
		collapsed: true,
		
		/**
		 * 'closable' decides whether the SingleMessage can be closed.
		 * @type Boolean
		 */
		closable: true,
		
		/**
		 * Alt text for message icons
		 * @private
		 * @type Object
		 */
		_iconTextMap: {
			"error": "X",
			"warning": "!",
			"information": "i",
			"success": "&#8730;",
			"critical": "X",
			"attention": "&#9670",
			"compliance": "&#9671"
		},
		
		
		/**
		 * Constructor
		 * Initialize the Object variable in new Instance 
		 */
		constructor: function() {
			this.date = new Date();
		},
		
		/**
		 * Possible message types
		 * @private
		 * @type Array
		 * @default ["error", "warning", "information", "success", "critical", "attention", "compliance"]
		 */
		_allowedTypes: ["error", "warning", "information", "success", "critical", "attention", "compliance"],
		
		postMixInProperties: function(){
			this.inherited(arguments);
			this._nlsResources = singleMessageNls;
			this.type = array.indexOf(this.type.toLowerCase()) ? this.type.toLowerCase() : "error";
			if(!this.description){
				this.description = this.title;
			}
			
			
		},
		
		postCreate: function(){
			// summary:
			//		Set tab index and time stamp for the message
			this.inherited(arguments);
			this._created = true;
			if(!this.actionText){
				this.set("actionText", this._nlsResources.viewAll);
			}
			domAttr.set(this.viewDetailsNode, "innerHTML", this._nlsResources.viewDetails);
			domAttr.set(this.refreshNode, "innerHTML", this._nlsResources.refresh);
			domAttr.set(this.closeNode, "title", commonNls.itemClose);
			domAttr.set(this.closeNode.childNodes[0], "title", commonNls.itemClose);
			this.set("title", this.title);
						
			if ( has("safari") ){
				//domAttr.remove(this.viewDetailsNode, "href");
				domAttr.remove(this.actionNode, "href");
			}
			var ariaLiveMap = {
				"error": "assertive", 
				"warning": "assertive", 
				"information": "polite", 
				"success": "polite", 
				"critical": "assertive", 
				"attention": "assertive", 
				"compliance": "polite"
			}
			var ariaLiveValue = ariaLiveMap[this.type.toLowerCase()];
			if ( ariaLiveValue ){
				domAttr.set(this.domNode, "aria-live", ariaLiveValue);
			}
			
			if (this.clipNode){
				this.clipNode.innerHTML = this.type + "," + this.title;
			}
			
			// defect 13436, the follwing code should be removed when the dojo fix the a11yclick defect
			// bug fix for a11yclick trigger in ie10 and ie11 when right click
			this.closeNode.dojoClick = false;
		},
		
		startup: function(){
			this.inherited(arguments);
			this.resize();
			//this.titleNode.focus();
		},
		_setActionTextAttr: function(value){
			this._set("actionText", value);
			domAttr.set(this.actionNode, "innerHTML", lang.replace(value, {num: this.messageNumber}));
		},
		
		_setTitleAttr: function(value){
			this._set("title", value);
			this._resizeTitle();
			domAttr.set(this.fakeTitleNode, "innerHTML", value + "&nbsp;&nbsp;");
		},
		
		
		_setDescriptionAttr: function(value){
			this._set("description", value);
			domAttr.set(this.descriptionNode, "innerHTML", value);
		},
		
		_setMaxLengthAttr: function(value){
			this._set("maxLength", value);
			this._resizeTitle();
		},
		
		_setMessageIdAttr: function(/*String*/ value){
			domAttr.set(this.idNode, "innerHTML", value);
			this._set("messageId", value);
			this._resizeTitle();
			domAttr.set(this.fakeIdNode, "innerHTML", value);
		},
		
		_setTypeAttr: function(/*String*/ value){
			domAttr.set(this.typeNode, "innerHTML", this._iconTextMap[value]);
			domClass.toggle(this.domNode, this.type + "Message", false);
			domClass.toggle(this.domNode, value + "Message", true);
			this._set("type", value);
			this._toggleId();
		},
		
		_setDateAttr: function(/*Date*/ value){
			this._set("date", value);
			domAttr.set(this.timestampNode, "innerHTML", locale.format(this.date, this.dateFormat));
			this._resizeTitle();
		},
		
		_setDateFormatAttr: function(/*dojo.date.locale.__FormatOptions?*/ value){
			this._set("dateFormat", value);
			domAttr.set(this.timestampNode, "innerHTML", locale.format(this.date, this.dateFormat));
			this._resizeTitle();
		},
		
		_setMessageNumberAttr: function(/*Integer|String*/ value){
			this._set("messageNumber", value);
			this.set("actionText", this.actionText);
			this._resizeTitle();
		},
		
		_setShowIdAttr: function(/*Boolean*/ value){
			this._set("showId", value);
			this._toggleId();
			this._resizeTitle();
		},
		
		_setShowActionAttr: function(/*Boolean*/ value){
			this._set("showAction", value);
			domClass.toggle(this.actionNode, "dijitHidden", !this.showAction);
			domClass.toggle(this.separatorNode, "dijitHidden", !(this.showAction && this.showTimestamp));
			if(has("ie") == 6 || has("ie") == 7){
				domClass.toggle(this.timestampNode, "idxMessageTimeStampMargin", !value);
			}
			this._resizeTitle();
		},
		
		_setShowRefreshAttr: function(/*Boolean*/ value){
			this._set("showRefresh", value);
			domClass.toggle(this.refreshNode, "dijitHidden", !this.showRefresh);
		},
		
		_setShowDetailsLinkAttr: function(/*Boolean*/ value){
			this._set("showDetailsLink", value);
			domClass.toggle(this.viewDetailsNode, "dijitHidden", !this.showDetailsLink);
		},
		
		_setClosableAttr: function(/*Boolean*/ value){
			this._set("closable", value);
			domStyle.set(this.closeNode, {
				"visibility": this.closable ? "visible" : "hidden"
			});
		},
		
		_setShowTimestampAttr: function(/*Boolean*/ value){
			this._set("showTimestamp", value);
			domClass.toggle(this.separatorNode, "dijitHidden", !(this.showAction && this.showTimestamp));
			domClass.toggle(this.timestampNode, "dijitHidden", !this.showTimestamp);
		},
		
		_toggleId: function(){
			if(this.type == "information" || this.type == "success" || this.type == "attention" || this.type == "compliance"){
				domClass.toggle(this.idNode, "dijitHidden", !this.showId);
				domClass.toggle(this.fakeIdNode, "dijitHidden", !this.showId);
			}
		},
		
		_resizeTitle: function(){
			if(!this._created){
				return;
			}
			domStyle.set(this.titleNode, {"width": "auto"});
			domAttr.set(this.titleNode, {"innerHTML": ''});
			if(this.collapsed){
				var idWidth = domGeometry.getMarginBox(this.idNode).w;
				var focusPadding = parseInt(domStyle.get(this.focusNode, "paddingLeft")) 
					+ parseInt( domStyle.get(this.focusNode, "paddingRight") );
				var width = domGeometry.getContentBox(this.domNode).w 
					- domGeometry.getMarginBox(this.iconNode).w
					- domGeometry.getMarginBox(this.infoNode).w 
					- idWidth
					- focusPadding;
				domAttr.set(this.titleNode, {"innerHTML": '<div class="messageTitles">' + this.title + '&nbsp&nbsp</div>'});
				var currentWidth = domStyle.get(this.titleNode, "width");
				if(width > 20){
					width = width - 10;
				}

				if(currentWidth > width){
					if(width < 0){
						width = 0;
					}
					domStyle.set(this.titleNode, {"width": width + "px"});
					domAttr.set(this.titleNode, {"innerHTML": '<div class="messageTitles dojoxEllipsis">' + this.title + '&nbsp&nbsp</div>'});
					domStyle.set(this.fakeFocusNode, {"width": width + idWidth + "px"});
				}else{
					domAttr.set(this.titleNode, {"innerHTML": '<div class="messageTitles">' + this.title + '&nbsp&nbsp</div>'});
				}
			}else{
				var idWidth = domGeometry.getMarginBox(this.fakeIdNode).w;
				var width = domGeometry.getContentBox(this.domNode).w 
					- domGeometry.getMarginBox(this.iconNode).w
					- domGeometry.getMarginBox(this.infoNode).w 
					- idWidth;
				var currentWidth = domStyle.get(this.fakeTitleNode, "width");
				if(width > 20){
					width = width - 10;
				}
				domStyle.set(this.fakeFocusNode, {"width": width + idWidth + "px"});
			}
			this._enforceTitleDirection();
		},
		
		_enforceTitleDirection: function(){
		},
		
		_setCollapsedAttr: function(value){
			if(value){
				domAttr.set(this.focusNode, "aria-label", this._nlsResources.showDetails);
			}else{
				domAttr.set(this.fakeFocusNode, "aria-label", this._nlsResources.hideDetails);
			}
			
			domClass.toggle(this.domNode, "idxMessageCollapsed", value);
			if(has("ie") == 6 || has("ie") == 7){
				if(value){
					domStyle.set(this.domNode, {"height": "33px"});
				}else{
					domStyle.set(this.domNode, {"height": "auto"});
				}
			}
			domClass.toggle(this.focusNode, "dijitHidden", !value);
			domClass.toggle(this.fakeFocusNode, "dijitHidden", value);
			this._set("collapsed", value);
			this._resizeTitle();
			
			if(has("ie") == 6){
				this.resize();
			}
		},
		
		_onClick: function(e){
			this.set("collapsed", !this.collapsed);
			focus.focus(this.collapsed ? this.focusNode : this.fakeFocusNode);
			this.onClick(e);
		},
		
		_onClose: function(e){
			this.onClose(e);
			this.destroy();
		},
		
		_onKeyClose: function(e){
			if(e.keyCode === keys.ENTER || e.keyCode === keys.SPACE){
				this._onClose(e);
			}
		},
		
		/**
		 * Ajust the content of the SingleMessage.
		 */
		resize: function(){
			this._resizeTitle();
		},
		
		/**
		 * Event triggered  when the close button is clicked. Before 'destroy' method is called. 
		 */
		onClose: function(e){
		},
		
		/**
		 * Event triggered when mouse enter the icon image.
		 */
		onIconEnter: function(e){
		},
		
		/**
		 * Event triggered when mouse leave the icon image.
		 */
		onIconLeave: function(e){
		},
		
		/**
		 * Event triggered when message title is clicked.
		 */
		onClick: function(e){
		},
		
		/**
		 * Event triggered when action link 'View All' is clicked.
		 */
		onAction: function(e){
		},
		
		/**
		 * Event triggered when action link 'Refresh' is clicked.
		 */
		onRefresh: function(e){
		},
		
		/**
		 * Event triggered when action link 'More Details' is clicked.
		 */
		onMoreDetails: function(e){
		}
	});
	return has("dojo-bidi")? declare("idx.widget.SingleMessage",[iMessaging.SingleMessage,bidiExtension]) : iMessaging.SingleMessage;
});



