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
var mqtt = require('mqtt')

var url = 'http://'+process.env.A1_HOST+':16102';
var mqttURL = 'http://'+process.env.A1_HOST+':16102';
var urlNoPort = 'http://'+process.env.A1_HOST+':';

function verifyResponse(response, statuscode, content, done) {
  response.text.should.equal(content);
  response.statusCode.should.equal(statuscode);
  console.log(response.text);
  done();
}

function verifyErrorResponse(response, statuscode, errorcode, errormessage, done) {
  response.statusCode.should.equal(statuscode);
  console.log(response.text);
  var output = JSON.parse(response.text);
  output['Code'].should.equal(errorcode);
  output['Message'].should.equal(errormessage);
  done();
}

function verifyError(error, statuscode, errorcode, errormessage, done) {
  error.response.statusCode.should.equal(statuscode);
  console.log(error.response.text);
  var output = JSON.parse(error.response.text);
  output['Code'].should.equal(errorcode);
  output['Message'].should.equal(errormessage);
  done();
}

describe('RESTMsg Plugin Messaging Tests', function() {
  describe('POST message tests', function() {
    // Publish message with restmsg plugin 
    it('should return status code 200 when trying to POST message to /restPublish1', function(done) {
      request(url)
       .post('/restmsg/message//restPublish1')
       .set('ClientID', 'POSTValid1')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
  });

  describe('GET message tests', function() {
    // Published a non-retained message so a new MQTT client should not receive it.
    it('should NOT receive MQTT message', function(done) {
      this.timeout(10000);
      var client = mqtt.connect(mqttURL);
      client.on('connect', function() {
        client.subscribe('/restPublish1');
      });
      client.on('message', function(topic, message) {
        should.fail('should not have received a message');
        client.end();
      });
      setTimeout(function() {
        client.end();
        done();
      }, 5000);
    });

    // Get message with MQTT client published by restmsg plugin
    it('should receive MQTT message', function(done) {
      this.timeout(10000);
      var client = mqtt.connect(mqttURL);
      client.on('connect', function() {
        client.subscribe('/restPublish1');
      });
      client.on('message', function(topic, message) {
        console.log('received message');
        client.end();
        done();
      });
      setTimeout(function() {
        request(url)
         .post('/restmsg/message//restPublish1')
         .set('ClientID', 'POSTValid2')
         .send('testing post')
         .end(function(err, res) {
          if (err) {
            return done(err);
          }
         });
      }, 500);
    });
    it('should receive resource statistics messages', function(done) {
        this.timeout(70000);
        var client = mqtt.connect(mqttURL);
        client.on('connect', function() {
          client.subscribe('$SYS/ResourceStatistics/Plugin');
        });
        client.on('message', function(topic, message) {
          console.log(message.toString());
          client.end();
          done();
        });
      });
  });
});
