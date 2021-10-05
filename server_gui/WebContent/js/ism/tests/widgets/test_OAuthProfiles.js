/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
define(['ism/tests/Utils','dojo/_base/lang','ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {
	
	var compareObjects = function(expected, actual) {
		if (!expected || !actual) {
			return "expected or actual are not defined";
		} 
		for (var prop in expected) {
			if (expected[prop] !== actual[prop]) {
				return "Mismatch for prop="+prop+"; expected: " + expected[prop] + ", actual: " + actual[prop];
			}
		}
		return "OK";
	};

	doh.register("ism.tests.widgets.test_OAuthProfile", [
		
	    // delete any existing profiles
		new LoginFixture ({name: "test_prep", 
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				var _this = this;
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/securityProfiles/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting security profile " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/securityProfiles/0/" + encodeURIComponent(data[i].Name),
								sync: true,
								handleAs: "json",
                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                load: function(data) {},
                                error: function(error) {
            						d.errback(new Error("Unexpected error deleting security profile " + data[i].Name +": " + JSON.stringify(JSON.parse(error.responseText))));
                                }
							});
						}
						dojo.xhrGet({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: lang.hitch(this, function(data, ioArgs) {
								for (var i = 0; i < data.length; i++) {
									console.log("Deleting OAuth profile " + data[i].Name);
									dojo.xhrDelete({
										url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(data[i].Name),
										sync: true,
										handleAs: "json",
		                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                                load: function(data) {},
		                                error: function(error) {
		            						d.errback(new Error("Unexpected error deleting OAuth profile " + data[i].Name +": " + JSON.stringify(JSON.parse(error.responseText))));
		                                }
									});
								}
								d.callback(true);
							}),
							error: function(error) {
								d.errback(new Error("Unexpected error listing profiles: " + JSON.stringify(JSON.parse(error.responseText))));
							}
						});						
					}),
					error: function(error) {
						d.errback(new Error("Unexpected error listing profiles: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});
				

				return d;
			}
		}),		

		new LoginFixture ({name: "test_getNone", expected: 0,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					doh.assertTrue(obj.length == this.expected);
				}));

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error querying empty list of profiles: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add1",
			resource: {
				Name: "Unit Test 1",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"					
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(true);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getByName",
			resource: {
				Name: "Unit Test 1",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: undefined,
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					doh.assertEqual(1, obj.length);
					var result = compareObjects(this.resource, obj[0]);
					doh.assertEqual("OK", result);
				}));

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(this.resource.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error getting profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_getWrong", expectedCode: "CWLNA5017",
			runTest: function() {
				var d = new doh.Deferred();
				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + "InvalidName",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						// it's an error if we get here
						d.errback(new Error("Able to get an invalid profile."));
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add2_withKeyFileName",
			resource: {
				Name: "Unit Test 2",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "ima.doh1.ca.cert",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"									
			},
			
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var result = compareObjects(this.resource, obj);
					doh.assertEqual("OK", result);
				}));
   	            
				var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.doh1.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
				var _this = this;
	            dojo.xhrPost({
	        	 	headers: { 
	        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
	        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
	        	 	},	  	  
				    url:Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
				    handleAs: "json",
				    putData: cert,
	                load: function(data) {						
						dojo.xhrPost({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
							postData: JSON.stringify(_this.resource),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data, ioArgs) {
								d.callback(data);
							},
							error: function(error) {
								d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
							}
						});
	                },
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}	                
	            });

				return d;
			}
		}),
		new LoginFixture ({name: "test_add2a_duplicateKeyFileName",
			resource: {
				Name: "Unit Test 2a",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "ima.doh1.ca.cert",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			expectedCode: "CWLNA5122",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));
   	            
				var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.doh1.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
				var _this = this;
	            dojo.xhrPost({
	        	 	headers: { 
	        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
	        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
	        	 	},	  	  
				    url:Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
				    handleAs: "json",
				    putData: cert,
	                load: function(data) {						
						dojo.xhrPost({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
							postData: JSON.stringify(_this.resource),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data, ioArgs) {
								d.callback(true);
							},
							error: function(error) {
								var response = JSON.parse(error.responseText);
								d.callback(response);
							}
						});
	                },
					error: function(error) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					}	                
	            });

				return d;
			}
		}),

		new LoginFixture ({name: "test_add3",
			resource: {
				Name: "Unit Test 3",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				UserInfoKey: "MyUserName"				
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var result = compareObjects(this.resource, obj);
					doh.assertEqual("OK", result);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),

		new LoginFixture ({name: "test_update3_duplicateKeyFileName",
			resource: {
				Name: "Unit Test 3",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "ima.doh1.ca.cert",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			expectedCode: "CWLNA5122",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));
   	            
				var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.doh1.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
				var _this = this;
	            dojo.xhrPost({
	        	 	headers: { 
	        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
	        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
	        	 	},	  	  
				    url:Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
				    handleAs: "json",
				    putData: cert,
	                load: function(data) {						
						dojo.xhrPut({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(_this.resource.Name),
							postData: JSON.stringify(_this.resource),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data, ioArgs) {
								d.callback(true);
							},
							error: function(error) {
								var response = JSON.parse(error.responseText);
								d.callback(response);
							}
						});
	                },
					error: function(error) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					}	                
	            });

				return d;
			}
		}),

		new LoginFixture ({name: "test_update2_removeFileName",
			resource: {
				Name: "Unit Test 2",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var result = compareObjects(this.resource, obj);
					doh.assertEqual("OK", result);
				}));
   	            
				var _this = this;
				dojo.xhrPut({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(_this.resource.Name),
					postData: JSON.stringify(_this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						d.callback(data);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});               

				return d;
			}
		}),
		// make sure clearing the key file from 2 allows the file to be used in 3
		new LoginFixture ({name: "test_update3_withFileName",
			resource: {
				Name: "Unit Test 3",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "ima.doh1.ca.cert",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			expectedCode: "CWLNA5122",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var result = compareObjects(this.resource, obj);
					doh.assertEqual("OK", result);
				}));
   	            
				var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.doh1.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
				var _this = this;
	            dojo.xhrPost({
	        	 	headers: { 
	        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
	        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
	        	 	},	  	  
				    url:Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
				    handleAs: "json",
				    putData: cert,
	                load: function(data) {						
						dojo.xhrPut({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(_this.resource.Name),
							postData: JSON.stringify(_this.resource),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data) {
								d.callback(data);
							},
							error: function(error) {
								d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
							}
						});
	                },
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}	                
	            });

				return d;
			}
		}),
				
		new LoginFixture ({name: "test_add_missing_resourceUrl",
			resource: {
				Name: "Unit Test 4",
				AuthKey: "MyToken",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			expectedCode: "CWLNA5012",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						// it's an error if we get here
						d.errback(new Error("Able to create a profile with a missing ResourceURL."));
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_missing_authKey",
			resource: {
				Name: "Unit Test 4",
				ResourceURL: "http://someFakeURL.com:1234",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName",
				AuthKey: ""
			},
			expectedCode: "CWLNA5012",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						// it's an error if we get here
						d.errback(new Error("Able to create a profile with a missing AuthKey."));
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_add_invalid_resourceUrl",
			resource: {
				Name: "Unit Test 4",
				ResourceURL: "ftp://bogus"
			},
			expectedCode: "CWLNA5139",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(response) {
					doh.assertEqual(this.expectedCode, response.code);
				}));

				dojo.xhrPost({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					postData: JSON.stringify(this.resource),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						// it's an error if we get here
						d.errback(new Error("Able to create a profile with an invalid ResourceURL."));
					},
					error: lang.hitch(this, function(error, ioArgs) {
						var response = JSON.parse(error.responseText);
						d.callback(response);
					})
				});

				return d;
			}
		}),
		
		new LoginFixture ({name: "test_getAll", expected: 3,
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					console.log(obj);
					doh.assertTrue(obj.length == this.expected);
				}));

				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data, ioArgs) {
						d.callback(data);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),
		new LoginFixture ({name: "test_delete3", Name: "Unit Test 3",
			runTest: function() {
				var d = new doh.Deferred();
				
				d.addCallback(function(pass) {
					doh.assertTrue(pass);
				});

				dojo.xhrDelete({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(this.Name),
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: function(data) {
						console.log("delete returned");
						d.callback(true);
					},
					error: function(error) {
						d.errback(new Error("Unexpected error deleting profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		}),
		// test deleting 3 allows file to be used in 2
		new LoginFixture ({name: "test_update2_withFileName",
			resource: {
				Name: "Unit Test 2",
				ResourceURL: "http://someFakeURL.com:1234",
				AuthKey: "MyToken",
				KeyFileName: "ima.doh1.ca.cert",
				UserInfoURL: "http://someFakeURL.com:1234",
				UserInfoKey: "MyUserName"				
			},
			expectedCode: "CWLNA5122",
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(obj) {
					var result = compareObjects(this.resource, obj);
					doh.assertEqual("OK", result);
				}));
   	            
				var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.doh1.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
				var _this = this;
	            dojo.xhrPost({
	        	 	headers: { 
	        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
	        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
	        	 	},	  	  
				    url:Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
				    handleAs: "json",
				    putData: cert,
	                load: function(data) {						
						dojo.xhrPut({
							url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(_this.resource.Name),
							postData: JSON.stringify(_this.resource),
							handleAs: "json",
							headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
							load: function(data) {
								d.callback(data);
							},
							error: function(error) {
								d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
							}
						});
	                },
					error: function(error) {
						d.errback(new Error("Unexpected error creating profile: " + JSON.stringify(JSON.parse(error.responseText))));
					}	                
	            });

				return d;
			}
		}),		
		new LoginFixture ({name: "test_reset", 
			runTest: function() {
				var d = new doh.Deferred();

				d.addCallback(lang.hitch(this, function(pass) {
					doh.assertTrue(pass);
				}));

				var _this = this;
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0",
					handleAs: "json",
					headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
					load: lang.hitch(this, function(data, ioArgs) {
						for (var i = 0; i < data.length; i++) {
							console.log("Deleting " + data[i].Name);
							dojo.xhrDelete({
								url: Utils.getBaseUri() + "rest/config/oAuthProfiles/0/" + encodeURIComponent(data[i].Name),
								sync: true,
								handleAs: "json",
                                headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
                                load: function(data) {},
                                error: function(error) {
            						d.errback(new Error("Unexpected error deleting profile " + data[i].Name +": " + JSON.stringify(JSON.parse(error.responseText))));
                                }
							});
						}
						d.callback(true);
					}),
					error: function(error) {
						d.errback(new Error("Unexpected error listing profiles: " + JSON.stringify(JSON.parse(error.responseText))));
					}
				});

				return d;
			}
		})
	]);
});
