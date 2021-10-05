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

var expectNoMQTTClient = '{ "MQTTClient": [], "Version": "'+ FVT.version +'" }' ;
var expectNoSubscription = '{ "Subscription": [], "Version": "'+ FVT.version +'" }' ;

var expectOrgMove000Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/000/1","TopicString":"/OrgMove/000/1","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/2","TopicString":"/OrgMove/000/2","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/3","TopicString":"/OrgMove/000/3","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/4","TopicString":"/OrgMove/000/4","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/5","TopicString":"/OrgMove/000/5","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/6","TopicString":"/OrgMove/000/6","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/7","TopicString":"/OrgMove/000/7","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/8","TopicString":"/OrgMove/000/8","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/9","TopicString":"/OrgMove/000/9","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/10","TopicString":"/OrgMove/000/10","ClientID":"a-OrgMove-uid0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
// When Client0 is JMS
var expectOrgMove000Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/000/durableSub0","TopicString":"/OrgMove/000/jms_0","ClientID":"a-OrgMove-jms0","IsDurable":true,"BufferedMsgs":30 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/000/durable_sub0_lucky","TopicString":"/OrgMove/000/jms_0/lucky","ClientID":"a-OrgMove-jms0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;

var expectOrgMove001Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/001/1","TopicString":"/OrgMove/001/1","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/2","TopicString":"/OrgMove/001/2","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/3","TopicString":"/OrgMove/001/3","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/4","TopicString":"/OrgMove/001/4","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/5","TopicString":"/OrgMove/001/5","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/6","TopicString":"/OrgMove/001/6","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/7","TopicString":"/OrgMove/001/7","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/8","TopicString":"/OrgMove/001/8","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/9","TopicString":"/OrgMove/001/9","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/001/10","TopicString":"/OrgMove/001/10","ClientID":"a-OrgMove-uid1","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
//EXPECTED 002 and 003 to have BufferMsgs:2 ??!!
var expectOrgMove002Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/002/1","TopicString":"/OrgMove/002/1","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/2","TopicString":"/OrgMove/002/2","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/3","TopicString":"/OrgMove/002/3","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/4","TopicString":"/OrgMove/002/4","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/5","TopicString":"/OrgMove/002/5","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/6","TopicString":"/OrgMove/002/6","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/7","TopicString":"/OrgMove/002/7","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/8","TopicString":"/OrgMove/002/8","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/9","TopicString":"/OrgMove/002/9","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/002/10","TopicString":"/OrgMove/002/10","ClientID":"a-OrgMove-uid2","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgMove003Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/003/1","TopicString":"/OrgMove/003/1","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/2","TopicString":"/OrgMove/003/2","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/3","TopicString":"/OrgMove/003/3","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/4","TopicString":"/OrgMove/003/4","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/5","TopicString":"/OrgMove/003/5","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/6","TopicString":"/OrgMove/003/6","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/7","TopicString":"/OrgMove/003/7","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/8","TopicString":"/OrgMove/003/8","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/9","TopicString":"/OrgMove/003/9","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/003/10","TopicString":"/OrgMove/003/10","ClientID":"a-OrgMove-uid3","IsDurable":true,"BufferedMsgs":2 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgMove004Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgMove/004/1","TopicString":"/OrgMove/004/1","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/2","TopicString":"/OrgMove/004/2","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/3","TopicString":"/OrgMove/004/3","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/4","TopicString":"/OrgMove/004/4","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/5","TopicString":"/OrgMove/004/5","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/6","TopicString":"/OrgMove/004/6","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/7","TopicString":"/OrgMove/004/7","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/8","TopicString":"/OrgMove/004/8","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/9","TopicString":"/OrgMove/004/9","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgMove/004/10","TopicString":"/OrgMove/004/10","ClientID":"a-OrgMove-uid4","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
]}' ;


var expectOrgMoveMQTTClient = ' { "Version":"v1", "MQTTClient": \
[{"ClientID":"d:OrgMove:pub:cid0","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid1","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid2","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid3","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid4","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid0","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid1","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid2","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid3","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid4","IsConnected":false} \
]}' ;
// When Client0 is JMS
var expectOrgMoveMQTTClient = ' { "Version":"v1", "MQTTClient": \
[{"ClientID":"d:OrgMove:pub:cid1","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid2","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid3","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid4","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid1","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid2","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid3","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid4","IsConnected":false} \
]}' ;

var verifyConfig = {};
var verifyMessage = {};



//  ====================  Test Cases Start Here  ====================  //
describe('ClientSet:', function() {
this.timeout( FVT.clientSetTimeout );

    // A1:Restart after Import OrgMove ClientSet 
    describe('Restart A1:', function() {

    	it('should return 200 POST A1:Restart Server', function(done) {
		    payload = '{"Service":"Server" }'
            verifyConfig = {} ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." };
            FVT.makePostRequestWithVerify( FVT.uriServiceDomain + "restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return 200 GET Status after A1:Restart', function(done) {
			if ( FVT.TestEnv == "HA" ) {
				this.timeout( FVT.START_HA + 3000 );
				FVT.sleep( FVT.START_HA );
                verifyConfig = JSON.parse( FVT.expectHARunningA2A1Sync ) ;
			} else if ( FVT.TestEnv == "CLUSTER" ) {
				this.timeout( FVT.START_CLUSTER + 3000 );
				FVT.sleep( FVT.START_CLUSTER );
                verifyConfig = JSON.parse( FVT.expectClusterConfigedStatus ) ;
			} else {
				this.timeout( FVT.REBOOT + 3000 );
				FVT.sleep( FVT.REBOOT );
                verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
			}
            FVT.makeGetRequest( FVT.uriServiceDomain + "status", FVT.getSuccessCallback, verifyConfig, done);
        });

		if ( FVT.TestEnv == "HA" ) {  // send cmd to A2 machine, A1 is now Standby Mode
		
			it('should return 200 on GET NO OrgMove MQTTClients after A1:Restart', function(done) {
				// Subscription Stats are slow to update...
				this.timeout( FVT.clientSetTimeout + 3000 );
				FVT.sleep( FVT.clientSetTimeout );
				verifyConfig = JSON.parse( expectNoMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A2_msgServer +':'+ FVT.A2_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET NO OrgMove Subscriptions after A1:Restart', function(done) {
				verifyConfig = JSON.parse(expectNoSubscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A2_msgServer+':'+FVT.A2_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
        } else { // send cmd to A1
			it('should return 200 on GET NO OrgMove MQTTClients after A1:Restart', function(done) {
				// Subscription Stats are slow to update...
				this.timeout( FVT.clientSetTimeout + 3000 );
				FVT.sleep( FVT.clientSetTimeout );
				verifyConfig = JSON.parse( expectNoMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET NO OrgMove Subscriptions after A1:Restart', function(done) {
				verifyConfig = JSON.parse(expectNoSubscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
			
		}
    });


    // A2:Restart after Import OrgMove ClientSet 
    describe('Restart A2:', function() {

    	it('should return 200 POST A2:Restart Server', function(done) {
		    payload = '{"Service":"Server" }'
            verifyConfig = {} ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." };
            FVT.makePostRequestWithURLVerify(  'http://'+ FVT.A2_msgServer +':'+ FVT.A2_PORT ,  FVT.uriServiceDomain + "restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return 200 GET Status after A2:Restart', function(done) {
			if ( FVT.TestEnv == "HA" ) {
				this.timeout( FVT.START_HA + 3000 );
				FVT.sleep( FVT.START_HA );
                verifyConfig = JSON.parse( FVT.expectHARunningA1A2Sync ) ;
			} else if ( FVT.TestEnv == "CLUSTER" ) {
				this.timeout( FVT.START_CLUSTER + 3000 );
				FVT.sleep( FVT.START_CLUSTER );
                verifyConfig = JSON.parse( FVT.expectClusterConfigedStatus ) ;
			} else {
				this.timeout( FVT.REBOOT + 3000 );
				FVT.sleep( FVT.REBOOT );
                verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
			}
            FVT.makeGetRequestWithURL(  'http://'+ FVT.A2_msgServer +':'+ FVT.A2_PORT ,  FVT.uriServiceDomain + "status", FVT.getSuccessCallback, verifyConfig, done);
        });

		if ( FVT.TestEnv == "HA" ) {  // send cmd to A1 machine, A2 is now Standby Mode
		
			it('should return 200 on GET NO OrgMove MQTTClients after A2:Restart', function(done) {
				// Subscription Stats are slow to update...
				this.timeout( FVT.clientSetTimeout + 3000 );
				FVT.sleep( FVT.clientSetTimeout );
				verifyConfig = JSON.parse( expectNoMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET NO OrgMove Subscriptions after A2:Restart', function(done) {
				verifyConfig = JSON.parse(expectNoSubscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
        } else { // send cmd to A2
			it('should return 200 on GET NO OrgMove MQTTClients after A2:Restart', function(done) {
				// Subscription Stats are slow to update...
				this.timeout( FVT.clientSetTimeout + 3000 );
				FVT.sleep( FVT.clientSetTimeout );
				verifyConfig = JSON.parse( expectNoMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A2_msgServer +':'+ FVT.A2_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET NO OrgMove Subscriptions after A2:Restart', function(done) {
				verifyConfig = JSON.parse(expectNoSubscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A2_msgServer+':'+FVT.A2_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
			
		}
    });
	
	
    // A3:Restart after Import OrgMove ClientSet 
    describe('Restart A3:', function() {

    	it('should return 200 POST A3:Restart Server', function(done) {
		    payload = '{"Service":"Server" }'
            verifyConfig = {} ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." };
            FVT.makePostRequestWithURLVerify( 'http://'+ FVT.A3_msgServer +':'+ FVT.A3_PORT ,  FVT.uriServiceDomain + "restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return 200 GET Status after A3:Restart', function(done) {
			if ( FVT.TestEnv == "HA" ) {
				this.timeout( FVT.START_HA + 3000 );
				FVT.sleep( FVT.START_HA );
                verifyConfig = JSON.parse( FVT.expectHARunningA4A3Sync ) ;
			} else if ( FVT.TestEnv == "CLUSTER" ) {
				this.timeout( FVT.START_CLUSTER + 3000 );
				FVT.sleep( FVT.START_CLUSTER );
                verifyConfig = JSON.parse( FVT.expectClusterConfigedStatus ) ;
			} else {
				this.timeout( FVT.REBOOT + 3000 );
				FVT.sleep( FVT.REBOOT );
                verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
			}
            FVT.makeGetRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "status", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

// Statistic should not be altered by the Export (other than OrgMove clients are disconnected
    // Verify Expected Subscriptions exist in OrgMove
    describe('PostRestart A3:OrgMove Stats:', function() {

		if ( FVT.TestEnv == "HA" ) {  // send cmd to A4 machine, A3 is now Standby Mode
		
			it('should return 200 on GET A4:OrgMove/000 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove000Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A4:OrgMove/001 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove001Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A4:OrgMove/002 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove002Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A4:OrgMove/003 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove003Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A4:OrgMove/004 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove004Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
			});

			// Verify Expected MQTT Durable Clients have connected in OrgMove
			it('should return 200 on GET A4:OrgMove MQTTClients', function(done) {
            	// Client are dropped with reboot            verifyConfig = JSON.parse( expectNoMQTTClient );
				verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
        } else {  // send cmd to A3 
		
			it('should return 200 on GET A3:OrgMove/000 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove000Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/001 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove001Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/002 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove002Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/003 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove003Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/004 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove004Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
			});

			// Verify Expected MQTT Durable Clients have connected in OrgMove
			it('should return 200 on GET A3:OrgMove MQTTClients', function(done) {
            	// Client conn's are dropped with reboot            verifyConfig = JSON.parse( expectNoMQTTClient );
				verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

		}
    });


    // A4:Restart after Import OrgMove ClientSet 
    describe('Restart A4:', function() {

    	it('should return 200 POST A4:Restart Server', function(done) {
		    payload = '{"Service":"Server" }'
            verifyConfig = {} ;
            verifyMessage = { "status":200, "Code":"CWLNA6011", "Message":"The requested configuration change has completed successfully." };
            FVT.makePostRequestWithURLVerify( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriServiceDomain + "restart", payload, FVT.postSuccessCallback, verifyConfig, verifyMessage, done);
        });
            
        it('should return 200 GET Status after A4:Restart', function(done) {
			if ( FVT.TestEnv == "HA" ) {
				this.timeout( FVT.START_HA + 3000 );
				FVT.sleep( FVT.START_HA );
                verifyConfig = JSON.parse( FVT.expectHARunningA3A4Sync ) ;
			} else if ( FVT.TestEnv == "CLUSTER" ) {
				this.timeout( FVT.START_CLUSTER + 3000 );
				FVT.sleep( FVT.START_CLUSTER );
                verifyConfig = JSON.parse( FVT.expectClusterConfigedStatus ) ;
			} else {
				this.timeout( FVT.REBOOT + 3000 );
				FVT.sleep( FVT.REBOOT );
                verifyConfig = JSON.parse( FVT.expectDefaultStatus ) ;
			}
            FVT.makeGetRequestWithURL(  'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriServiceDomain + "status", FVT.getSuccessCallback, verifyConfig, done);
        });

    });

// Statistic should not be altered by the Export (other than OrgMove clients are disconnected
    // Verify Expected Subscriptions exist in OrgMove
    describe('PostRestart A4:OrgMove Stats:', function() {

		if ( FVT.TestEnv == "HA" ) {  // send cmd to A4 machine, A3 is now Standby Mode
		
			it('should return 200 on GET A3:OrgMove/000 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove000Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/001 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove001Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/002 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove002Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/003 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove003Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
			});

			it('should return 200 on GET A3:OrgMove/004 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectOrgMove004Subscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
			});

			// Verify Expected MQTT Durable Clients have connected in OrgMove
			it('should return 200 on GET A3:OrgMove MQTTClients', function(done) {
            	// Client are dropped with reboot            verifyConfig = JSON.parse( expectNoMQTTClient );
				verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});
        } else {  // send cmd to A4 
		
			it('should return 200 on GET A4:OrgMove/000 Subscriptions', function(done) {
				verifyConfig = JSON.parse(expectNoSubscription );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer+':'+FVT.A4_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/*', FVT.getSuccessCallback, verifyConfig, done);
			});

			// Verify Expected MQTT Durable Clients have connected in OrgMove
			it('should return 200 on GET A4:OrgMove MQTTClients', function(done) {
            	// Client conn's are dropped with reboot
				verifyConfig = JSON.parse( expectNoMQTTClient );
				FVT.makeGetRequestWithQuery( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
			});

		}
    });
	
});
