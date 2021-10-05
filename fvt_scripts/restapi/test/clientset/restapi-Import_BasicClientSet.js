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

var expectOrgStay000Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgStay/000/1","TopicString":"/OrgStay/000/1","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/2","TopicString":"/OrgStay/000/2","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/3","TopicString":"/OrgStay/000/3","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/4","TopicString":"/OrgStay/000/4","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/5","TopicString":"/OrgStay/000/5","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/6","TopicString":"/OrgStay/000/6","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/7","TopicString":"/OrgStay/000/7","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/8","TopicString":"/OrgStay/000/8","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/9","TopicString":"/OrgStay/000/9","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/000/10","TopicString":"/OrgStay/000/10","ClientID":"a-OrgStay-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgStay001Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgStay/001/1","TopicString":"/OrgStay/001/1","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/2","TopicString":"/OrgStay/001/2","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/3","TopicString":"/OrgStay/001/3","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/4","TopicString":"/OrgStay/001/4","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/5","TopicString":"/OrgStay/001/5","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/6","TopicString":"/OrgStay/001/6","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/7","TopicString":"/OrgStay/001/7","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/8","TopicString":"/OrgStay/001/8","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/9","TopicString":"/OrgStay/001/9","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/001/10","TopicString":"/OrgStay/001/10","ClientID":"a-OrgStay-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgStay002Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgStay/002/1","TopicString":"/OrgStay/002/1","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/2","TopicString":"/OrgStay/002/2","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/3","TopicString":"/OrgStay/002/3","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/4","TopicString":"/OrgStay/002/4","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/5","TopicString":"/OrgStay/002/5","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/6","TopicString":"/OrgStay/002/6","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/7","TopicString":"/OrgStay/002/7","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/8","TopicString":"/OrgStay/002/8","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/9","TopicString":"/OrgStay/002/9","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/002/10","TopicString":"/OrgStay/002/10","ClientID":"a-OrgStay-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgStay003Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgStay/003/1","TopicString":"/OrgStay/003/1","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/2","TopicString":"/OrgStay/003/2","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/3","TopicString":"/OrgStay/003/3","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/4","TopicString":"/OrgStay/003/4","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/5","TopicString":"/OrgStay/003/5","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/6","TopicString":"/OrgStay/003/6","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/7","TopicString":"/OrgStay/003/7","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/8","TopicString":"/OrgStay/003/8","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/9","TopicString":"/OrgStay/003/9","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/003/10","TopicString":"/OrgStay/003/10","ClientID":"a-OrgStay-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgStay004Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgStay/004/1","TopicString":"/OrgStay/004/1","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/2","TopicString":"/OrgStay/004/2","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/3","TopicString":"/OrgStay/004/3","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/4","TopicString":"/OrgStay/004/4","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/5","TopicString":"/OrgStay/004/5","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/6","TopicString":"/OrgStay/004/6","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/7","TopicString":"/OrgStay/004/7","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/8","TopicString":"/OrgStay/004/8","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/9","TopicString":"/OrgStay/004/9","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgStay/004/10","TopicString":"/OrgStay/004/10","ClientID":"a-OrgStay-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;


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
// expected 002 and 003 to have BufferedMsgs:2 -- sometimes it's zero ??!!
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


var expectOrgExist000Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgExist/000/1","TopicString":"/OrgExist/000/1","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/2","TopicString":"/OrgExist/000/2","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/3","TopicString":"/OrgExist/000/3","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/4","TopicString":"/OrgExist/000/4","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/5","TopicString":"/OrgExist/000/5","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/6","TopicString":"/OrgExist/000/6","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/7","TopicString":"/OrgExist/000/7","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/8","TopicString":"/OrgExist/000/8","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/9","TopicString":"/OrgExist/000/9","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/000/10","TopicString":"/OrgExist/000/10","ClientID":"a-OrgExist-uid0","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgExist001Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgExist/001/1","TopicString":"/OrgExist/001/1","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/2","TopicString":"/OrgExist/001/2","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/3","TopicString":"/OrgExist/001/3","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/4","TopicString":"/OrgExist/001/4","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/5","TopicString":"/OrgExist/001/5","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/6","TopicString":"/OrgExist/001/6","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/7","TopicString":"/OrgExist/001/7","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/8","TopicString":"/OrgExist/001/8","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/9","TopicString":"/OrgExist/001/9","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/001/10","TopicString":"/OrgExist/001/10","ClientID":"a-OrgExist-uid1","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgExist002Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgExist/002/1","TopicString":"/OrgExist/002/1","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/2","TopicString":"/OrgExist/002/2","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/3","TopicString":"/OrgExist/002/3","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/4","TopicString":"/OrgExist/002/4","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/5","TopicString":"/OrgExist/002/5","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/6","TopicString":"/OrgExist/002/6","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/7","TopicString":"/OrgExist/002/7","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/8","TopicString":"/OrgExist/002/8","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/9","TopicString":"/OrgExist/002/9","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/002/10","TopicString":"/OrgExist/002/10","ClientID":"a-OrgExist-uid2","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgExist003Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgExist/003/1","TopicString":"/OrgExist/003/1","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/2","TopicString":"/OrgExist/003/2","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/3","TopicString":"/OrgExist/003/3","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/4","TopicString":"/OrgExist/003/4","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/5","TopicString":"/OrgExist/003/5","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/6","TopicString":"/OrgExist/003/6","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/7","TopicString":"/OrgExist/003/7","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/8","TopicString":"/OrgExist/003/8","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/9","TopicString":"/OrgExist/003/9","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/003/10","TopicString":"/OrgExist/003/10","ClientID":"a-OrgExist-uid3","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;
var expectOrgExist004Subscription = '{ "Version":"v1", "Subscription":  \
[{"SubName":"/OrgExist/004/1","TopicString":"/OrgExist/004/1","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/2","TopicString":"/OrgExist/004/2","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/3","TopicString":"/OrgExist/004/3","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/4","TopicString":"/OrgExist/004/4","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/5","TopicString":"/OrgExist/004/5","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/6","TopicString":"/OrgExist/004/6","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/7","TopicString":"/OrgExist/004/7","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/8","TopicString":"/OrgExist/004/8","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/9","TopicString":"/OrgExist/004/9","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
,{"SubName":"/OrgExist/004/10","TopicString":"/OrgExist/004/10","ClientID":"a-OrgExist-uid4","IsDurable":true,  "Consumers":1,  "MessagingPolicy":"DemoTopicPolicy"} \
]}' ;

var expectOrgMoveMQTTClient = ' { "Version":"v1", "MQTTClient": [\
 {"ClientID":"d:OrgMove:pub:cid0","IsConnected":false} \
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
var expectOrgMoveMQTTClient = ' { "Version":"v1", "MQTTClient": [\
 {"ClientID":"d:OrgMove:pub:cid2","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid3","IsConnected":false} \
,{"ClientID":"d:OrgMove:pub:cid4","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid1","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid2","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid3","IsConnected":false} \
,{"ClientID":"a-OrgMove-uid4","IsConnected":false} \
]}' ;

var expectOrgStayMQTTClient = ' { "Version":"v1", "MQTTClient": \
[{"ClientID":"d:OrgStay:pub:cid0","IsConnected":false} \
,{"ClientID":"d:OrgStay:pub:cid1","IsConnected":false} \
,{"ClientID":"d:OrgStay:pub:cid2","IsConnected":false} \
,{"ClientID":"d:OrgStay:pub:cid3","IsConnected":false} \
,{"ClientID":"d:OrgStay:pub:cid4","IsConnected":false} \
,{"ClientID":"a-OrgStay-uid0","IsConnected":false} \
,{"ClientID":"a-OrgStay-uid1","IsConnected":false} \
,{"ClientID":"a-OrgStay-uid2","IsConnected":false} \
,{"ClientID":"a-OrgStay-uid3","IsConnected":false} \
,{"ClientID":"a-OrgStay-uid4","IsConnected":false} \
]}' ;

var expectOrgExistMQTTClient = ' { "Version":"v1", "MQTTClient": \
[{"ClientID":"d:OrgExist:pub:cid0","IsConnected":false} \
,{"ClientID":"d:OrgExist:pub:cid1","IsConnected":false} \
,{"ClientID":"d:OrgExist:pub:cid2","IsConnected":false} \
,{"ClientID":"d:OrgExist:pub:cid3","IsConnected":false} \
,{"ClientID":"d:OrgExist:pub:cid4","IsConnected":false} \
,{"ClientID":"a-OrgExist-uid0","IsConnected":false} \
,{"ClientID":"a-OrgExist-uid1","IsConnected":false} \
,{"ClientID":"a-OrgExist-uid2","IsConnected":false} \
,{"ClientID":"a-OrgExist-uid3","IsConnected":false} \
,{"ClientID":"a-OrgExist-uid4","IsConnected":false} \
]}' ;


var verifyConfig = {};
var verifyMessage = {};



//  ====================  Test Cases Start Here  ====================  //
//  Delete Retained messages in Durable Topic /CD/001/3 for ClientID with name starting a-org, then Delete entire a-org-cid1 ClientSet.

describe('ClientSet:', function() {
this.timeout( FVT.clientSetTimeout );

    // Verify Expected Subscriptions exist in OrgStay, OrgMove and OrgExist
    describe('Verify OrgStay A1 Subscriptions:', function() {

        it('should return 200 on GET A1:/OrgStay/000 Subscriptions', function(done) {
			// Timeout to allow Subscription STATs to sync
			this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectOrgStay000Subscription );
// Mocha does not like & in parameters plain or encoded %26 \u0026
//            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'ResultCount=100\u0026SubName=/OrgStay*', FVT.getSuccessCallback, verifyConfig, done);
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/001 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgStay001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/002 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgStay002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/003 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgStay003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/004 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgStay004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	
    describe('Verify OrgMove A1 Subscriptions:', function() {

        it('should return 200 on GET A1:/OrgMove/000 Subscriptions', function(done) {
			// Timeout to allow Subscription STATs to sync after OrgMove_ClientSetVerify_A1_A2.xml
			this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectOrgMove000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/001 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgMove001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/002 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgMove002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/003 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgMove003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/004 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgMove004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	
    describe('Verify OrgMove A3 Subscriptions:', function() {

        it('should return 200 on GET OrgMove A3 Subscriptions', function(done) {
		    // cause they have not been imported yet...
            verifyConfig = JSON.parse(expectNoSubscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	
    describe('Verify OrgExist A3 Subscriptions:', function() {

        it('should return 200 on GET A3:/OrgExist/000 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgExist000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/001 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgExist001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/002 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgExist002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/003 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgExist003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/004 Subscriptions', function(done) {
            verifyConfig = JSON.parse(expectOrgExist004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    // Verify Expected MQTT Durable Clients have connected in OrgStay, OrgMove and OrgExist
    describe('Verify MQTTClient:', function() {

        it('should return 200 on GET OrgStay A1 MQTTClients', function(done) {
// None are disconnected            verifyConfig = JSON.parse( expectOrgStayMQTTClient );
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgStay*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMove A1 MQTTClients', function(done) {
            verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMove A3 MQTTClients', function(done) {
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });


        it('should return 200 on GET OrgExist A3 MQTTClients', function(done) {
// None are disconnected            verifyConfig = JSON.parse( expectOrgExistMQTTClient );
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ClientID=*OrgExist*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

	if ( FVT.TestEnv == "HA" ) {  // send cmd to A4 machine, A1 is now Standby Mode

	// Can NOT Import from Standby
		describe('NOT-ImportStandby:', function() {
		
			it('should return 200 on GET A4:Service/Status', function(done) {
				verifyConfig =  JSON.parse( FVT.expectHARunningA3A4Sync ) ;
				FVT.makeGetRequestWithURL( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriServiceDomain + "status/" , FVT.getSuccessCallback, verifyConfig, done);
			});
		
			it('should return 400 on Import to Standby', function(done) {
				payload = '{"FileName":"ImportStandby", "Password":"OrgMovePswd", "Topic":"^", "ClientID":"^"}'
				verifyConfig = {} ;
				verifyMessage = { "status":400, "Code":"CWLNA0388", "Message":"The action import for Object: ClientSet is not allowed on a standby node.", "RequestID":"RequestIDStandby" };
				// exportSuccessCallback need to retrieve FVT.RequestID
				FVT.makePostRequestWithURLVerify( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
			});
		
			it('should return 404 on GET RequestIDStandby', function(done) {
				aRequestID = FVT.getRequestID( "RequestIDStandby" );
				verifyConfig =  {} ;
//				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found." }' );
				verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID '+ aRequestID +' was not found." }' );
				FVT.makeGetRequestWithURL( 'http://'+FVT.A4_msgServer +':'+ FVT.A4_PORT , FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getFailureCallback,  verifyMessage, done);
			});

		});
		
	};


    // Import OrgMove ClientSet by Topic
    describe('Import OrgMove by Topic', function() {

        it('should return 200 on Import ClientSet of OrgMove by Topic', function(done) {
		    payload = '{"FileName":"ExportOrgMoveTopic", "Password":"OrgMovePswd" }'
            verifyConfig = {} ;
//            verifyMessage = { "status":200, "Code":"CWLNA0010", "Message":"Success", "RequestID":"RequestIDTopic" };
            verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"RequestIDTopic" };
			// Since NO JMS Clients yet, there is no Export file and this returns msg:
//            verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The ClientSet request with request id #aRequestID# failed with reason ISMRC_InvalidParameter (221).", "RequestID":"RequestIDTopic" };			
      //      verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated IBM IoT MessageSight message number (221).", "RequestID":"RequestIDTopic" };			
            verifyMessage = { "status":400, "Code":"CWLNA6174", "Message":"The import ClientSet request with request ID #aRequestID# failed with reason ISMRC_FileCorrupt associated ${IMA_PRODUCTNAME_SHORT} message number (221).", "RequestID":"RequestIDTopic" };			
            
            FVT.makePostRequestWithURLVerify( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
        });

        it('should return 200 on GET Topic Import status', function(done) {
		    FVT.sleep( FVT.DELETE_CLIENTSET );  // Allow IMPORT STATUS to become AVAILABLE
			aRequestID = FVT.getRequestID( "RequestIDTopic" );
			console.log ( "aRequestID:"+ aRequestID  );
            verifyConfig =  {"Status":0,  "RetCode":0,  "ClientsImported":10, "SubscriptionsImported":50,"RetainedMsgsImported": 0 } ;
			// Since NO JMS Clients yet, there is no Export file and this returns msg:  
//            verifyConfig =  {"Status":2,  "RetCode":207,  "ClientsImported":0, "SubscriptionsImported":0,"RetainedMsgsImported": 0 } ;
//            verifyConfig =  JSON.parse( '{"Status":2,  "RetCode":221,  "ClientsImported":0, "SubscriptionsImported":0,"RetainedMsgsImported": 0, "Code": "CWLNA6174", "Message": "The import ClientSet request with request ID '+ aRequestID +' failed with reason ISMRC_FileCorrupt associated IBM IoT MessageSight message number (221).","RequestID": '+ aRequestID +' }' ) ;
            verifyConfig =  JSON.parse( '{"Status":2,  "RetCode":221,  "ClientsImported":0, "SubscriptionsImported":0,"RetainedMsgsImported": 0, "Code": "CWLNA6174", "Message": "The import ClientSet request with request ID '+ aRequestID +' failed with reason ISMRC_FileCorrupt associated ${IMA_PRODUCTNAME_SHORT} message number (221).","RequestID": '+ aRequestID +' }' ) ;
            FVT.makeGetRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getSuccessCallback, verifyConfig, done);
        });
	
        it('should return 200 on DELETE Topic Import status', function(done) {
			aRequestID = FVT.getRequestID( "RequestIDTopic" );
            verifyConfig =  {} ;
			verifyMessage = JSON.parse( '{ "Status":200, "Code":"CWLNA6173", "Message":"The import ClientSet with request ID '+ aRequestID +' has been deleted." }' );
            FVT.makeDeleteRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "import/ClientSet/"+ aRequestID , FVT.deleteSuccessCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return 404 on GET RequestIDTopic Import status in now gone', function(done) {
			aRequestID = FVT.getRequestID( "RequestIDTopic" );
            verifyConfig =  {  } ;
//			verifyMessage = { "status":404, "Code":"CWLNA0113", "Message":"The requested item is not found."} ;
			verifyMessage = JSON.parse( '{ "status":404, "Code":"CWLNA6178", "Message":"The import request for ClientSet with request ID '+ aRequestID +' was not found." }' );
            FVT.makeGetRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getFailureCallback, verifyMessage, done);
        });
/* see comment #2 in DEFECT 149607	--  Need to add MQTT Client sharing the JMS topic to get it exported.
        it('should return 200 on GET A3:/OrgMove/000 Subscriptions after Topic Import', function(done) {
			// Timeout to allow Subscription STATs to sync
			this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectOrgMove000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/001 Subscriptions after Topic Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/002 Subscriptions after Topic Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/003 Subscriptions after Topic Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/004 Subscriptions after Topic Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMove A3:MQTTClients after Topic Import', function(done) {
            verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });
*/
    });


    // Import OrgMove ClientSet by ClientID
    describe('Import OrgMove by ClientID', function() {

        it('should return 200 on Import ClientSet of OrgMove by ClientID', function(done) {
		    payload = '{"FileName":"ExportOrgMoveClientID", "Password":"OrgMovePswd"  }'
            verifyConfig = {} ;
//            verifyMessage = { "status":200, "Code":"CWLNA0010", "Message":"Success", "RequestID":"RequestIDClientID" };
            verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"RequestIDClientID" };
			// exportSuccessCallback need to retrieve RequestID
            FVT.makePostRequestWithURLVerify( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return 200 on GET ClientID Import status', function(done) {
		    FVT.sleep( FVT.DELETE_CLIENTSET );  // REPLACE WITH LOOP ON GET IMPORT STATUS WHEN AVAILABLE
//            verifyConfig =  {"Status":0,  "RetCode":0,  "ClientsImported":10, "SubscriptionsImported":50,"RetainedMsgsImported": 0 } ;
// not jms            verifyConfig =  {"Status":1,  "RetCode":0,  "ClientsImported":8, "SubscriptionsImported":50,"RetainedMsgsImported": 0 } ;
            verifyConfig =  {"Status":1,  "RetCode":0,  "ClientsImported":8, "SubscriptionsImported":42,"RetainedMsgsImported": 0 } ;
			aRequestID = FVT.getRequestID( "RequestIDClientID" );
			console.log ( "aRequestID:"+ aRequestID  );
            FVT.makeGetRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/000 Subscriptions after ClientID Import', function(done) {
			// Timeout to allow Subscription STATs to sync
			this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectOrgMove000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/001 Subscriptions after ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/002 Subscriptions after ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/003 Subscriptions after ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/004 Subscriptions after ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMoveClientID A3_MQTTClients after ClientID Import', function(done) {
            verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });
            
    });


    // Import OrgMove ClientSet by Topic and ClientId
    describe('Import OrgMove by Topic&ClientID', function() {
		
        it('should return 202 on Import ClientSet of OrgMove by Topic and ClientID', function(done) {
		    payload = '{"FileName":"ExportOrgMoveTopicClientID", "Password":"OrgMovePswd" }'
            verifyConfig = {} ;
            verifyMessage = { "status":202, "Code":"CWLNA0010", "Message":"The operation will be completed asynchronously", "RequestID":"RequestIDTopicClientID" };
//            verifyMessage = { "status":200, "Code":"CWLNA0010", "Message":"Success", "RequestID":"RequestIDTopicClientID" };
			// exportSuccessCallback need to retrieve FVT.RequestID
            FVT.makePostRequestWithURLVerify( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "import/ClientSet", payload, FVT.exportSuccessCallback, verifyConfig, verifyMessage, done);
        });
	
        it('should return 200 on GET Topic and ClientID Import status', function(done) {
		    FVT.sleep( FVT.DELETE_CLIENTSET );  // REPLACE WITH LOOP ON GET IMPORT STATUS WHEN AVAILABLE
//            verifyConfig =  {"Status":0,  "RetCode":0,  "ClientsImported":10, "SubscriptionsImported":50, "RetainedMsgsImported": 0 } ;
//?? 8 clients ??
// not jms            verifyConfig =  {"Status":1,  "RetCode":0,  "ClientsImported":8, "SubscriptionsImported":50, "RetainedMsgsImported": 0 } ;
            verifyConfig =  {"Status":1,  "RetCode":0,  "ClientsImported":8, "SubscriptionsImported":42, "RetainedMsgsImported": 0 } ;
			console.log( "RequestID[]'s:"+  FVT.RequestID[0] +" "+  FVT.RequestID[1] +" "+  FVT.RequestID[2] +" get 'RequestIDTopicClientID'" );
			aRequestID = FVT.getRequestID( "RequestIDTopicClientID" );
			console.log ( "aRequestID:"+ aRequestID  );
            FVT.makeGetRequestWithURL( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriServiceDomain + "status/import/ClientSet/"+ aRequestID , FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/000 Subscriptions after Topic&ClientID Import', function(done) {
			// Timeout to allow Subscription STATs to sync
			this.timeout( FVT.clientSetTimeout + 3000 );
			FVT.sleep( FVT.clientSetTimeout );
            verifyConfig = JSON.parse(expectOrgMove000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/001 Subscriptions after Topic&ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/002 Subscriptions after Topic&ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/003 Subscriptions after Topic&ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgMove/004 Subscriptions after Topic&ClientID Import', function(done) {
            verifyConfig = JSON.parse(expectOrgMove004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMoveTopicClientID A3_MQTTClients after Topic&ClientID Import', function(done) {
            verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });
            

    });

	
// Statistic should not be altered by the Export (other than OrgMove clients are disconnected
    // Verify Expected Subscriptions exist in OrgStay, OrgMove and OrgExist
    describe('PostImport OrgStay A1 Subscriptions:', function() {

        it('should return 200 on GET A1:/OrgStay/000 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgStay000Subscription );
// Mocha does not like & in parameters plain or encoded %26 \u0026
//            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'ResultCount=100\u0026SubName=/OrgStay*', FVT.getSuccessCallback, verifyConfig, done);
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/001 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgStay001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/002 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgStay002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/003 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgStay003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgStay/004 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgStay004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgStay/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	
    describe('PostImport OrgMove A1 Subscriptions:', function() {

        it('should return 200 on GET A1:/OrgMove/000 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgMove000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/001 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgMove001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/002 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgMove002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/003 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgMove003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A1:/OrgMove/004 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgMove004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer+':'+FVT.A1_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgMove/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });
	
    describe('PostImport OrgExist A3 Subscriptions:', function() {

        it('should return 200 on GET A3:/OrgExist/000 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgExist000Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/000*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/001 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgExist001Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/001*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/002 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgExist002Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/002*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/003 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgExist003Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/003*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET A3:/OrgExist/004 Subscriptions after Imports', function(done) {
            verifyConfig = JSON.parse(expectOrgExist004Subscription );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer+':'+FVT.A3_PORT , FVT.uriMonitorDomain+'Subscription' , 'SubName=/OrgExist/004*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

    // Verify Expected MQTT Durable Clients have connected in OrgStay, OrgMove and OrgExist
    describe('Verify MQTTClient:', function() {

        it('should return 200 on GET OrgStay MQTTClients after Imports', function(done) {
// none are disconnected            verifyConfig = JSON.parse( expectOrgStayMQTTClient );
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgStay*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgMove A1_MQTTClients after Imports', function(done) {
            verifyConfig = JSON.parse( expectOrgMoveMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.msgServer +':'+ FVT.A1_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgMove*', FVT.getSuccessCallback, verifyConfig, done);
        });

        it('should return 200 on GET OrgExist MQTTClients after Imports', function(done) {
// none are disconnected            verifyConfig = JSON.parse( expectOrgExistMQTTClient );
            verifyConfig = JSON.parse( expectNoMQTTClient );
            FVT.makeGetRequestWithQuery( 'http://'+FVT.A3_msgServer +':'+ FVT.A3_PORT , FVT.uriMonitorDomain + 'MQTTClient' , 'ResultCount=100&ClientID=*OrgExist*', FVT.getSuccessCallback, verifyConfig, done);
        });

    });

		
// TODO: REPLACE call run_cli to erase the Export/Import files and  ClientSet RequestID status files (on A1 and A3)

});
