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

describe('RESTMsg Plugin Authorization Tests', function() {
  // 31000 allow all
  // 31001 allow publish with restcid* clientid
  // 31002 allow subscribe with restcid* clientid
  // 31003 allow all with allowpersistent = false
  describe('Successful authorization tests', function() {
    it('should successfully POST a message based on protocol, clientid and action publish', function(done) {
      request(urlNoPort+'31001')
       .post('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidValidPublish')
       .send('testing authorized publish')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully GET statuscode 204 No Content because of protocol, clientid and actionlist subscribe', 
        function(done) {
      request(urlNoPort+'31002')
       .get('/restmsg/message//restPublish1')
       .set('ClientID', 'restcidValidGET')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 204, '', done);
       });
    });
    it('should successfully POST a message because of non-persistence but fail silently because of actionlist subscribe only', function(done) {
    	// Does not fail for non-persistent
    	// TODO: update to add mqtt client that fails receive
      request(urlNoPort+'31002')
       .post('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidSilentFail')
       .send('testing unauthorized publish')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully PUT a message based on protocol, clientid and actionlist publish', function(done) {
      request(urlNoPort+'31001')
       .put('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidValidPUT')
       .send('testing authorized PUT')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully DELETE because of protocol, clientid and actionlist publish', function(done) {
      request(urlNoPort+'31001')
       .delete('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidValidDelete')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        console.log(res.statusCode);
        verifyResponse(res, 200, 'Success', done);
       });
    });
  });
  describe('Unsuccessful authorization tests', function() {
    it('should fail to POST a message based on unauthorized clientid', function(done) {
      request(urlNoPort+'31001')
       .post('/restmsg/message//restPublishA')
       .set('ClientID', 'restInvalidPublish')
       .send('testing authorized publish')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 403, 'The operation is not authorized.', done);
       });
    });
    it('should fail to GET a message because of actionlist publish only', function(done) {
      request(urlNoPort+'31001')
       .get('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidInvalidGET')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 403, 'The get is not authorized', done);
       });
    });
    it('should fail to GET because of unauthorized clientid', function(done) {
      request(urlNoPort+'31002')
       .get('/restmsg/message//restPublish1')
       .set('ClientID', 'restInvalidGET')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 403, 'The operation is not authorized.', done);
       });
    });
    it('should fail to POST because of actionlist subscribe only and persist=true', function(done) {
    	// Does not fail for non-persistent
      request(urlNoPort+'31002')
       .post('/restmsg/message//restPublishA')
       .query({ persist: 'true' })
       .set('ClientID', 'restcidInvalidPublish')
       .send('testing unauthorized publish')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 403, 'The operation is not authorized.', done);
       });
    });
    it('should fail to PUT a message based on actionlist subscribe', function(done) {
      request(urlNoPort+'31002')
       .put('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidInvalidPUT')
       .send('testing unauthorized PUT')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 403, 'The operation is not authorized.', done);
       });
    });
    it('should fail to DELETE because of actionlist subscribe only', function(done) {
      request(urlNoPort+'31002')
       .delete('/restmsg/message//restPublishA')
       .set('ClientID', 'restcidInvalidDelete')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        console.log(res.statusCode);
        verifyResponse(res, 403, 'The operation is not authorized.', done);
     });
    });
  });
});
