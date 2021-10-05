/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
var assert = require('assert')
var request = require('supertest')
var should = require('should')
var fs = require('fs')
var mqtt = require('mqtt')

var FVT = require('../restapi-CommonFunctions.js')

var MQTT_Setup_ExpectedMQTTClients = ' { "Version":"v1", "MQTTClient": [ \
	 {"ClientID":"sessionexpirysub1","IsConnected":false} \
	,{"ClientID":"sessionexpirysub2","IsConnected":false} \
]}'
	
var MQTT_Setup_ExpectedExpiredClientSub = ' { "Version":"v1", "Server": { \
	 "ExpiredClientSessions":0 \
	,"ClientSessions":2 \
	,"Subscriptions":1 }}';

var MQTT_Expire_ExpectedMQTTClients = ' { "Version":"v1", "MQTTClient": [ \
	 {"ClientID":"sessionexpirysub1","IsConnected":false} \
]}'

var MQTT_Expire_ExpectedExpiredClientSub = ' { "Version":"v1", "Server": { \
	 "ExpiredClientSessions":2 \
	,"ClientSessions":1 \
	,"Subscriptions":1 }}';
	
	
function MQTTClientExpiryTimeCheck ( err, res, done ) {

	try {
        	var jsonResponse = JSON.parse( res.text );
            var responseArray = jsonResponse[ 'MQTTClient' ];
            for (var i = 0; i < Object.keys(responseArray).length; i++) {
                for (var key in responseArray[i]) {
                	var val = responseArray[i][key];
                	if (key === 'ExpiryTime') {
                		if (val == null) {
                			var msg = 'Found NULL Session Expiry Time for ClientID:' + responseArray[i].ClientID ;
                			verifyThis = '' ;
                			throw new Error(msg)
                		}
                		break;
                	}
                }
            }
            FVT.getSuccessCallback(err, res, done);
    } catch ( ex ) {
        FVT.trace( 0, "Unexpected Exception in MQTTClientExpiryTimeCheck : "+ ex );
        FVT.trace( 0, JSON.stringify(ex, null, 2));
    }

};

var expectedNumDisconnected;
function MQTTClientDisconnectedCheck( err, res, done ) {
	try {
    	var jsonResponse = JSON.parse( res.text );
        var responseArray = jsonResponse[ 'MQTTClient' ];
        var responseArrayLength = Object.keys(responseArray).length;
        if (responseArrayLength != expectedNumDisconnected) {
        	var msg = 'Number of disconnected clients does not match. Actual: (' + responseArrayLength + '), Expected:(' + expectedNumDisconnected + ')';
        	verifyThis = ''
        	expectedNumDisconnected = 0;
        	throw new Error(msg);
        } else {
        	expectedNumDisconnected = 0;
        	FVT.getSuccessCallback(err, res, done);
        }
	} catch ( ex ) {
        FVT.trace( 0, "Unexpected Exception in MQTTClientDisconnectedCheck : "+ ex );
        FVT.trace( 0, JSON.stringify(ex, null, 2));
	}
}

//  ====================  Test Cases Start Here  ====================  //
if ( FVT.SESSION_EXPIRY_TEST === 'SetupMQTTSession') {
	describe('Check Monitor Fields after MQTT Client Setup:', function() {
		this.timeout( FVT.clientSetTimeout );
		it('should return 200 for Expected # of MQTT Disconnected Clients With ExpiryTime non NULL', function(done) {
			verifyConfig = JSON.parse( MQTT_Setup_ExpectedMQTTClients );
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient', MQTTClientExpiryTimeCheck, verifyConfig, done);
		});
		
		it('should return 200 for Server Monitoring with correct # of ClientSessions, ExpiredSessions, and Subscriptions', function(done) {
			verifyConfig = JSON.parse( MQTT_Setup_ExpectedExpiredClientSub );
			FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'Server', FVT.getSuccessCallback, verifyConfig, done);

		});
	});
} else if ( FVT.SESSION_EXPIRY_TEST === 'SessionExpireMQTT'){
	describe('Check Monitor Fields after MQTT Client Session Expires:', function() {
		this.timeout( FVT.clientSetTimeout );
		it('should return 200 for Expected # of MQTT Disconnected Clients', function(done) {
			verifyConfig = JSON.parse( MQTT_Expire_ExpectedMQTTClients );
			expectedNumDisconnected = 1;
            FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient', MQTTClientDisconnectedCheck, verifyConfig, done);
		});
		
		it('should return 200 for Server Monitoring with correct # of ClientSessions, ExpiredSessions, and Subscriptions', function(done) {
			verifyConfig = JSON.parse( MQTT_Expire_ExpectedExpiredClientSub );
			FVT.makeGetRequestWithURL( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'Server', FVT.getSuccessCallback, verifyConfig, done);
		});
	});
}
