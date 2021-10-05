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
[{"SubName":"/OrgMove/000/durableSub0","TopicString":"/OrgMove/000/jms_0","ClientID":"a-OrgMove-jms0","IsDurable":true,"BufferedMsgs":0 ,"MaxMessages":5000 ,"RejectedMsgs":0 ,"IsShared":false,"Consumers":0,"DiscardedMsgs":0,"ExpiredMsgs":0,"MessagingPolicy":"DemoTopicPolicy"} \
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

var verifyConfig = {};
var verifyMessage = {};



//  ====================  Test Cases Start Here  ====================  //
//  Delete Retained messages in Durable Topic /CD/001/3 for ClientID with name starting a-org, then Delete entire a-org-cid1 ClientSet.

describe('ClientSet:', function() {
this.timeout( FVT.clientSetTimeout );

    // Verify Expected Subscriptions exist in OrgStay, OrgMove and OrgExist
    describe('Delete A1:OrgMove:', function() {


        it('should return 200 on DELETE ClientSet A1:OrgMove', function(done) {
            verifyConfig =  {} ;
			verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6197", "Message":"The request is complete. Clients found: 9, Clients deleted: 9, Deletion errors: 0" }' );
            FVT.makeDeleteRequest( FVT.uriServiceDomain + "ClientSet?Retain=^/OrgMove&ClientID=^..OrgMove" , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on GET NO A1:OrgMove MQTTClients after DELETE', function(done) {
            // Subscription Stats are slow to update...
    		this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });
	
        it('should return 200 on GET NO A1:OrgMove Subscriptions after DELETE', function(done) {
            // Subscription Stats are slow to update...
    		this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectNoSubscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

});
