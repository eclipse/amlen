/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

define(['dojo/_base/declare',  
        'dojo/_base/connect',
        'dojo/_base/lang',
		'dojo/json',
		'ism/IsmUtils',
		'ism/config/Resources'
		], function(declare, connect, lang, JSON, Utils, Resources) {

	var Preferences = declare("ism.widgets.Preferences", null, {
		// summary:
		// 		A representation of an Preference JSON object
			
		_prefs: null,  // the preferences once retrieved from the server
		_url: null,
		
		/*
		 * To get preferences for the user, make sure
		 * to pass in the userId, e.g. new Preferences({ userId: userid })
		 */
		constructor: function(args) {
			// summary:
			// 		Constructor.  Set fields from args.
			dojo.safeMixin(this, args);
			this._url = Utils.getBaseUri() + "rest/config/webui/preferences";
			if (this.userId) {
				this._url += "/" + encodeURIComponent(this.userId);
			}
			if (!this.blocking) {
				this.blocking = false;
			}
			
			if (this.preferences) {
				this._prefs = this.preferences;
			} else {			
				this._loadPreferences();
			}
		},

		/**
		 * Loads preferences for the userId passed in on the constructor
		 * Once loaded, publishes a message containing the preferences on 
		 * Resources.preferences.topic
		 */
		_loadPreferences: function() {
			dojo.xhrGet({
				url: this._url,
				handleAs: "json",
				preventCache: true,
				sync: this.blocking, 
				load: lang.hitch(this, function(data, ioArgs) {
					this._prefs = data;
					connect.publish(Resources.preferences.uploadedTopic, data);
				}),
				error: function(error, ioargs) {
					Utils.checkXhrError(ioargs);
				}
			});
		},
		
		/**
		 * Returns current prefs object. It could be null! If it is,
		 * try subscribing to Resources.preferences.uploadedTopic
		 * to get notified when they finish uploading.
		 * @returns
		 */
		getPreferences: function() {
			return this._prefs;
		},
		
		/**
		 * Persists preferences to the server. If preferences are passed in,
		 * then persists those to the server and updates the client preferences
		 * with those returned from the server. If no preferences are passed in,
		 * persists the current preferences.  Once loaded, publishes a message 
		 * containing the preferences on Resources.preferences.topic
		 * @param preferences
		 */
		persistPreferences: function(/*JSONObject*/ preferences, /*boolean*/ blocking ) {
			var prefs = this._prefs;
			if (preferences) {
				prefs = preferences;								
			}
			var sync = blocking ? true: false;
			dojo.xhrPut({
				url: this._url,
				postData: JSON.stringify(prefs),
				handleAs: "json",
				sync: sync,
				headers: {
					"Content-Type": "application/json"
				},
				load: lang.hitch(this, function(data, ioArgs) {
					this._prefs = data;
					connect.publish(Resources.preferences.updatedTopic, data);
				}),
				error: function(error, ioargs) {
					Utils.checkXhrError(ioargs);
				}
			});		
		}
		
	});

	return Preferences;
});

