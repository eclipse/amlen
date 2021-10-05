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

describe('RESTMsg Plugin Retained Message Tests', function() {
  describe('PUT new retained message tests', function() {
    // Publish retained message with restmsg plugin 
    it('should return status code 200 when trying to PUT message to /restPublishA', function(done) {
      request(url)
       .put('/restmsg/message//restPublishA')
       .set('ClientID', 'PUTValidA')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });

    // Publish retained message with restmsg plugin 
    it('should return status code 200 when trying to PUT message to /restPublishB', function(done) {
      request(url)
       .put('/restmsg/message//restPublishB')
       .set('ClientID', 'PUTValidB')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });

    // Publish retained message with MQTT client
    it('should successfully publish retained MQTT message to /mqttPublishA', function(done) {
      var client = mqtt.connect(mqttURL);
      client.on('connect', function() {
        client.publish('/mqttPublishA', 'testing', {'retain': true}, function() {
          console.log('MQTT client publish complete');
          client.end();
          done();
        });
      });
    });

    // Publish retained message with MQTT client
    it('should successfully publish retained MQTT message to /mqttPublishB', function(done) {
      var client = mqtt.connect(mqttURL);
      client.on('connect', function() {
        client.publish('/mqttPublishB', 'testing', {'retain': true}, function() {
          console.log('MQTT client publish complete');
          client.end();
          done();
        });
      });
    });
  });

  describe('GET Existing retained message tests', function() {
    // Receive retained message with MQTT client published by restmsg plugin
    it('should receive retained MQTT message', function(done) {
      var client = mqtt.connect(mqttURL);
      client.on('connect', function() {
        client.subscribe('/restPublishA');
      });
      client.on('message', function(topic, message) {
        message.toString().should.equal('testing');
        client.end();
        done();
      })
    });

    // Get retained message with restmsg plugin published by restmsg plugin
    it('should return status code 200 when trying to GET message from /restPublishA', function(done) {
      request(url)
       .get('/restmsg/message//restPublishA')
       .set('ClientID', 'GETValidA')
       .end(function(err, res) {
        if (err) {
          console.log(err);
          return done(err);
        }
        verifyResponse(res, 200, 'testing', done);
       });
    });

    // Get retained message with restmsg plugin published by MQTT client
    it('should return status code 200 when trying to GET message from /mqttPublishA', function(done) {
      this.timeout(10000);
      setTimeout(function() {
	      request(url)
	       .get('/restmsg/message//mqttPublishA')
	       .set('ClientID', 'GETValidB')
	       .end(function(err, res) {
		if (err) {
		  console.log(err);
		  return done(err);
		}
		verifyResponse(res, 200, 'testing', done);
	       });
       }, 5000);
    });
  });

  describe('DELETE Existing retained message tests', function() {
    // Delete retained message with restmsg plugin published by restmsg plugin
    it('should return status code 200 when trying to DELETE message from /restPublishA', function(done) {
      request(url)
        .delete('/restmsg/message//restPublishA')
        .set('ClientID', 'DELETEValidA')
        .end(function(err, res) {
          if (err) {
            console.log(err);
            return done(err);
          }
          verifyResponse(res, 200, 'Success', done);
        });
    });

    // Delete retained message with restmsg plugin published by MQTT client
    it('should return status code 200 when trying to DELETE message from /mqttPublishA', function(done) {
      request(url)
        .delete('/restmsg/message//mqttPublishA')
        .set('ClientID', 'DELETEValidB')
        .end(function(err, res) {
          if (err) {
            console.log(err);
            return done(err);
          }
          verifyResponse(res, 200, 'Success', done);
        });
    });

    // Delete retained message with MQTT client published by restmsg plugin
    it('should successfully clear retained message with MQTT client', function(done) {
      this.timeout(5000);
      var client = mqtt.connect(mqttURL);

      client.on('connect', function() {
        client.publish('/restPublishB', '', {'retain': true}, function() {
          console.log('MQTT client publish complete');
          // give the retain a moment to be cleared...
          setTimeout(function() {
              client.end();
              done();
          }, 2000);
        })
      })
    });
    
    // Delete retained message with MQTT client published by MQTT client
    it('should successfully clear retained message with MQTT client', function(done) {
      var client = mqtt.connect(mqttURL);

      client.on('connect', function() {
        client.publish('/mqttPublishB', '', {'retain': true}, function() {
          console.log('MQTT client publish complete');
          client.end();
          done();
        })
      })
    });
  });
  
  describe('DELETE Non-Existing retained message tests', function() {
	// Verify DELETE fails when there is no message to delete
	it('should return status code ??? when trying to DELETE message from /restPublishA', function(done) {
		request(url)
		  .get('/restmsg/message//restPublishA')
		  .set('ClientID', 'DELETEInvalidA')
		  .end(function(err, res) {
			if (err) {
				return done(err);
			}
			console.log(res.text);
			res.statusCode.should.equal(204);
			done();
		  });
	});
  });

  describe('GET Non-Existing retained message tests', function() {
    // Verify GET returns nothing after deleting restmsg plugin published msg with restmsg plugin
    it('should return status code 204 when trying to GET message from /restPublishA', function(done) {
      request(url)
        .get('/restmsg/message//restPublishA')
        .set('ClientID', 'GETInvalidA')
        .end(function(err, res) {
          if (err) {
            return done(err);
          }
          console.log(res.text);
          res.statusCode.should.equal(204);
          done();
        });
    });

    // Verify GET returns nothing after deleting MQTT client published msg with restmsg plugin
    it('should return status code 204 when trying to GET message from /mqttPublishA', function(done) {
      request(url)
        .get('/restmsg/message//mqttPublishA')
        .set('ClientID', 'GETInvalidB')
        .end(function(err, res) {
          if (err) {
            return done(err);
          }
          console.log(res.text);
          res.statusCode.should.equal(204);
          done();
        });
    });

    // Verify GET returns nothing after deleting restmsg plugin published msg with MQTT client
    it('should return status code 204 when trying to GET message from /restPublishB', function(done) {
      request(url)
        .get('/restmsg/message//restPublishB')
        .set('ClientID', 'GETInvalidC')
        .end(function(err, res) {
          if (err) {
            return done(err);
          }
          console.log(res.text);
          res.statusCode.should.equal(204);
          done();
        });
    });
    
    // Verify GET returns nothing after deleting MQTT client published msg with MQTT client
    it('should return status code 204 when trying to GET message from /mqttPublishB', function(done) {
      this.timeout(10000);
      setTimeout(function() {
	      request(url)
		.get('/restmsg/message//mqttPublishB')
		.set('ClientID', 'GETInvalidD')
		.end(function(err, res) {
		  if (err) {
		    return done(err);
		  }
		  console.log(res.text);
		  res.statusCode.should.equal(204);
		  done();
		});
      }, 5000);
    });

    // Verify MQTT client does not get retained message cleared by restmsg plugin
    it('should not get a retained MQTT message', function(done) {
      this.timeout(10000);
      var receivedMessage = false;
      var client = mqtt.connect(mqttURL);

      client.on('connect', function() {
        client.subscribe('/restPublishA');
        client.subscribe('/restPublishB');
        client.subscribe('/mqttPublishA');

        setTimeout(function() {
          client.end();
          if (receivedMessage == false) {
            done();
          }
        }, 5000);
      });

      client.on('message', function(topic, message) {
        receivedMessage = true;
        should.fail('No messages should have been received');
      });
    });
  });

  describe('PUT to invalid topic strings', function() {
    // PUT to null topic fails
    it('should return status code 400 when trying to PUT message to null', function(done) {
      request(url)
       .put('/restmsg/message/')
       .set('ClientID', 'PUTInvalidA')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          verifyError(err, 400, 'CWLNA0400', 'The topic is not valid', done);
        } else {
          verifyErrorResponse(res, 400, 'CWLNA0400', 'The topic is not valid', done);
        }
       });
    });

    // PUT to + topic
    it('should return status code 400 when trying to PUT message to +', function(done) {
      request(url)
       .put('/restmsg/message/+')
       .set('ClientID', 'PUTInvalidB')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          verifyError(err, 400, 'CWLNA0400', 'The topic is not valid', done);
        } else {
          verifyErrorResponse(res, 400, 'CWLNA0400', 'The topic is not valid', done);
        }
       });
    });

    // PUT to mytopic/+ topic
    it('should return status code 400 when trying to PUT message to mytopic/+', function(done) {
      request(url)
       .put('/restmsg/message/mytopic/+')
       .set('ClientID', 'PUTInvalidC')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          verifyError(err, 400, 'CWLNA0400', 'The topic is not valid', done);
        } else {
          verifyErrorResponse(res, 400, 'CWLNA0400', 'The topic is not valid', done);
        }
       });
    });

    // PUT to # topic
    it('should return status code 400 when trying to PUT message to #', function(done) {
      request(url)
       .put('/restmsg/message/%23')
       .set('ClientID', 'PUTInvalidD')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          verifyError(err, 400, 'CWLNA0400', 'The topic is not valid', done);
        } else {
          verifyErrorResponse(res, 400, 'CWLNA0400', 'The topic is not valid', done);
        }
       });
    });

    // PUT to mytopic/# topic
    it('should return status code 400 when trying to PUT message to mytopic/#', function(done) {
      request(url)
       .put('/restmsg/message/mytopic/%23')
       .set('ClientID', 'PUTInvalidE')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          verifyError(err, 400, 'CWLNA0400', 'The topic is not valid', done);
        } else {
          verifyErrorResponse(res, 400, 'CWLNA0400', 'The topic is not valid', done);
        }
       });
    });
  });

  describe('PUT to update retained messages', function() {
    // set initial retained message on /restPublishA using restmsg plugin
    it('should return status code 200 when trying to PUT message to /restPublishA initial', function(done) {
      request(url)
       .put('/restmsg/message//restPublishA')
       .set('ClientID', 'PUTInitialA')
       .send('testing')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });
    
    // update retained message set by restmsg plugin using restmsg plugin
    it('should return status code 200 when trying to PUT message to /restPublishA update', function(done) {
      request(url)
       .put('/restmsg/message//restPublishA')
       .set('ClientID', 'PUTUpdateA')
       .send('testing update')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });

    // set initial retained message on /mqttPublishA using MQTT client
    it('should successfully publish a retained message with MQTT client', function(done) {
      var client = mqtt.connect(mqttURL);

      client.on('connect', function() {
        client.publish('/mqttPublishA', 'test mqtt', {'retain': true}, function() {
          console.log('MQTT client publish complete');
          client.end();
          done();
        })
      })
    });

    // update retained message set by MQTT client using restmsg plugin
    it('should return status code 200 when trying to PUT message to /mqttPublishA update', function(done) {
      request(url)
       .put('/restmsg/message//mqttPublishA')
       .set('ClientID', 'PUTUpdateB')
       .send('test mqtt update')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'Success', done);
       });
    });

    // get updated retained message from /restPublishA
    it('should return status code 200 when trying to GET message from /restPublishA', function(done) {
      request(url)
       .get('/restmsg/message//restPublishA')
       .set('ClientID', 'GETRetainedA')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'testing update', done);
       });
    });

    // get updated retained message from /mqttPublishA
    it('should return status code 200 when trying to GET message from /mqttPublishA', function(done) {
      request(url)
       .get('/restmsg/message//mqttPublishA')
       .set('ClientID', 'GETRetainedB')
       .end(function(err, res) {
        if (err) {
          return done(err);
        }
        verifyResponse(res, 200, 'test mqtt update', done);
       });
    });
  });

  describe('clientid tests', function() {
    // test 2 back to back connections with no clientid set
    it('should be successful', function(done) {
      this.timeout(5000);
      var errA = null;
      var errB = null;

      request(url)
       .get('/restmsg/message//mqttPublishA')
       .end(function(err, res) {
        if (err) {
          errA = err;
          return;
        }
        res.text.should.equal('test mqtt update');
        res.statusCode.should.equal(200);
        console.log(res.text);
       });
      
      request(url)
       .get('/restmsg/message//mqttPublishA')
       .end(function(err, res) {
        if (err) {
          errB = err;
          return;
        }
        res.text.should.equal('test mqtt update');
        res.statusCode.should.equal(200);
        console.log(res.text);
       });

      setTimeout(function() {
        if (errA != null) {
          return done(errA);
        }
        if (errB != null) {
          return done(errB);
        }
        done();
      }, 2000);
    });
  });
});
