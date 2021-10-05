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

var urlA = 'https://'+process.env.A1_HOST+':31004';
var urlAinsecure = 'http://'+process.env.A1_HOST+':31004';
var urlB = 'https://pluginuser:pluginuser@'+process.env.A1_HOST+':31005';
var urlBinsecure = 'http://pluginuser:pluginuser@'+process.env.A1_HOST+':31005';
var urlBnouser = 'https://'+process.env.A1_HOST+':31005';
var urlBbaduser = 'https://pluginuser:plugin@'+process.env.A1_HOST+':31005';

function verifyResponse(response, statuscode, content, done) {
  response.text.should.equal(content);
  response.statusCode.should.equal(statuscode);
  console.log(response.text);
  done();
}

function verifyErrorResponse(response, statuscode, errorcode, errormessage, done) {
  response.statusCode.should.equal(statuscode);
  console.log(response.text);
  response.text.should.equal(errormessage);
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

describe('RESTMsg Plugin HTTPS Tests', function() {
  before(function() {
    process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";
  });
  // 31004 allow all over https
  // 31005 allow all over https with basic auth
  describe('Valid HTTPS tests with no basic auth', function() {
    it('should successfully GET 204 No Content using HTTPS', function(done) {
      request(urlA)
       .get('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsGET204')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 204, '', done);
       });
    });
    it('should successfully POST a message using HTTPS', function(done) {
      request(urlA)
       .post('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsPOST200')
       .send('testpost')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully PUT a retained message using HTTPS', function(done) {
      request(urlA)
       .put('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsPUT200')
       .send('testput')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully GET 200 testput using HTTPS', function(done) {
      request(urlA)
       .get('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsGET200')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'testput', done);
       });
    });
    it('should successfully DELETE a retained message using HTTPS', function(done) {
      request(urlA)
       .delete('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsDELETE200')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully GET 204 No Content using HTTPS after clear retained', function(done) {
      request(urlA)
       .get('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsGET204Cleared')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 204, '', done);
       });
    });
  });
  describe('Valid HTTPS tests with basic auth', function() {
    it('should successfully GET 204 No Content using HTTPS with basic auth', function(done) {
      request(urlB)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGET204Auth')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 204, '', done);
       });
    });
    it('should successfully POST a message using HTTPS with basic auth', function(done) {
      request(urlB)
       .post('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsPOST200Auth')
       .send('testpost')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully PUT a retained message using HTTPS with basic auth', function(done) {
      request(urlB)
       .put('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsPUT200Auth')
       .send('testput')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully GET 200 testput using HTTPS with basic auth', function(done) {
      request(urlB)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGET200Auth')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'testput', done);
       });
    });
    it('should successfully DELETE a retained message using HTTPS with basic auth', function(done) {
      request(urlB)
       .delete('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsDELETE200Auth')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    it('should successfully GET 204 No Content using HTTPS with basic auth after clear retained', function(done) {
      request(urlB)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGET204ClearedAuth')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 204, '', done);
       });
    });
  });
  describe('Invalid HTTPS tests', function() {
    it('should fail to connect to a secure endpoint without HTTPS', function(done) {
      request(urlAinsecure)
       .get('/restmsg/message//restSecureA')
       .set('ClientID', 'httpsGETErrInsecure')
       .end(function(err, res) {
        err.code.should.equal('ECONNRESET');
        done();
       });
    });
    it('should fail to connect to a secure endpoint without HTTPS with basic auth', function(done) {
      request(urlBinsecure)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGETErrInsecureAuth')
       .end(function(err, res) {
        err.code.should.equal('ECONNRESET');
        done();
       });
    });
    it('should fail to connect due to missing basic auth', function(done) {
      request(urlBnouser)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGETErrNoAuth')
       .end(function(err, res) {
          res.statusCode.should.equal(401);
          res.text.should.equal('Not authorized');
          done();
       });
    });
    it('should fail to connect due to invalid basic auth', function(done) {
      request(urlBbaduser)
       .get('/restmsg/message//restSecureB')
       .set('ClientID', 'httpsGETErrBadAuth')
       .end(function(err, res) {
          res.statusCode.should.equal(403);
          res.text.should.equal('The operation is not authorized.');
          done();
       });
    });
  });
});
