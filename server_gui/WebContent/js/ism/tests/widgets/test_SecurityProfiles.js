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
define(['ism/tests/Utils','dojo/_base/lang','ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {

	doh.register("ism.tests.widgets.test_SecurityProfiles", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name:"test_prep", 
			runTest: function () {
		          var cert = '-----------------------------239602725422497\nContent-Disposition: form-data; name="certificate"; filename="ima.spTest.cp01.ca.cert"\nContent-Type: application/octet-stream\n\n-----BEGIN CERTIFICATE-----\nMIIDzDCCArQCCQC9AWB6DZzoXDANBgkqhkiG9w0BAQUFADCBpzELMAkGA1UEBhMC\nVVMxCzAJBgNVBAgMAlRYMQ8wDQYDVQQHDAZBdXN0aW4xGDAWBgNVBAoMD0lCTSBD\nb3Jwb3JhdGlvbjEbMBkGA1UECwwSSUJNIFNvZnR3YXJlIEdyb3VwMSMwIQYDVQQD\nDBpJQk0gTWVzc2FnaW5nIEFwcGxpYW5jZSBDQTEeMBwGCSqGSIb3DQEJARYPYWlz\nbUB1cy5pYm0uY29tMB4XDTEzMDExNzIwNDc0MVoXDTE0MDExNzIwNDc0MVowgacx\nCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJUWDEPMA0GA1UEBwwGQXVzdGluMRgwFgYD\nVQQKDA9JQk0gQ29ycG9yYXRpb24xGzAZBgNVBAsMEklCTSBTb2Z0d2FyZSBHcm91\ncDEjMCEGA1UEAwwaSUJNIE1lc3NhZ2luZyBBcHBsaWFuY2UgQ0ExHjAcBgkqhkiG\n9w0BCQEWD2Fpc21AdXMuaWJtLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\nAQoCggEBAKI6Y13fBwYBtuG7ZgDL6FEUKz/UmVqY0DEVoje2jq5NAqgAbXToCc5i\n06VZSN3m8LMaEvNikmHAA8NW2ulQ77v7OlYd94bu/9QCts6rdgyT8k5pt3tXNm6T\nylS2h134PeKhoYm4mqxokg3nMJdKNwtSpbqoguZ/sViPMxLmncCg6e4aOBYewGap\n9flffIiXJO3OXms9IghGvivBl8i0ECyGOAO5lw3FagD9qmUkAjdPY0zD6dF1Jnkp\nJHvLKpwPyMGrPuaa4sLUrL6v+w3n/+Q/P+kTAAR4QbFbm2d1yIF4HjjfwrI7L52d\n2SNYeohR0Q9Gwaqpds22cQruwVjKpycCAwEAATANBgkqhkiG9w0BAQUFAAOCAQEA\nRSJKn3mRuxxUO6Dhejy+m4W2lJpgEPEtrGuhSMRfI6O6i0l068ze6uLobEaF2PL4\nYuIs8LJN89BXUzOhPVuhT+TJQ/21XoOhM+C6F+QX14zrpfU9xpde8ZOnaZuWsP3V\nrQLYgsC2nBl9+AQi3jSFNi1SMn2ToxIbFqHsckXJUOgIsRCnXiRHmvYV/+63XT6K\nP5VHN/3Xv4GEIE0Q4nb3jqvcMrfdvKz2vunr2kAKsD89m3/OgVOB3FOiVaQVAp07\nyQtbLX49ON/mNU08/fxij4q15S8ECUNS/alBqQ7G4kj36NHdaSqp5eU6IJ0XfKJ9\njuSzm0QwQwvRA6WwYtWfuQ==\n-----END CERTIFICATE-----\n\n-----------------------------239602725422497--\n';
		          var key = '-----------------------------30206167019393\nContent-Disposition: form-data; name="certificate"; filename="ima.spTest.cp01.ca.key"\nContent-Type: application/octet-stream\n\n-----BEGIN RSA PRIVATE KEY-----\nMIIEpQIBAAKCAQEAojpjXd8HBgG24btmAMvoURQrP9SZWpjQMRWiN7aOrk0CqABt\ndOgJzmLTpVlI3ebwsxoS82KSYcADw1ba6VDvu/s6Vh33hu7/1AK2zqt2DJPyTmm3\ne1c2bpPKVLaHXfg94qGhibiarGiSDecwl0o3C1KluqiC5n+xWI8zEuadwKDp7ho4\nFh7AZqn1+V98iJck7c5eaz0iCEa+K8GXyLQQLIY4A7mXDcVqAP2qZSQCN09jTMPp\n0XUmeSkke8sqnA/Iwas+5priwtSsvq/7Def/5D8/6RMABHhBsVubZ3XIgXgeON/C\nsjsvnZ3ZI1h6iFHRD0bBqql2zbZxCu7BWMqnJwIDAQABAoIBAQCabkC3VJ9H/YvN\nmOpCKdnujOea7NRLZRsTDsgMhzGOBWtY6IdJ+bWUDYnyZmsyKizKIjEWFajJetNa\nOa1M26pLZZ2j6wT+IzfP6AGD/b7zvEa2lHaA6IW9f9zlBZkZQD4RJtIy21QKecVH\njOQ5sQFzOurfJJjvuXDmn/L7tCNKAaABXQbX9gIV5tNNGwl4cqBVloEFoC8YHmPR\n5MrfVfPjEoA/lu50EJdno+5yiNfJ6wYmaLczaRcalL/JpqNAbnTLLu4/88/E73ij\n6EAQ+v2G9K4p1BZOuFo41AAv5a2hsZjQLXVATYV8FE9v7ODHycsURV6AaySE0856\nisOwPJA5AoGBANcCDBtwQIR0KzskWO8DehZaCMBlQ/g/WDTzPcRwGIypi/X5I+uP\nx8sqpvjSWdZ4WhNyHNkq+6mvgYfsVPrSyrvNl8mQiCJxaApU82l+ry6nn78hVaff\nw1lUzftviBvxKhw57cSabvWImSq2eSjOwK7myD/JNITrH/f7dca7g7E7AoGBAMEo\nTBsCvcvkNh51lj51GPNHyRYVsVFOyqGgcZ6iEU44bd3LYQKyseCr65PgAsKCuIgk\nngXQCXISKJ0DpOVyyYp2Snq0N72YLK0Z07p4N8sin3lNPT5Dj8nSugclL4HVUKke\n8ENkeaV9oKmZRBR0OIyjNYyrYRrnK4MNA30qJYMFAoGBALupy8t/NLDnfHxIg19L\nF5q+xvi26paZI3JEBNuaQ7MyoTj2VkXa3zYTal2vrD4oGebzKP7cJ4C58UMkIiAz\nMESvdBa4kjoN5hNuhm7D5j/AiwwWGl9GTYmBHbCibpiE7I5qeX+qk8K3kYjYb/QQ\nUdnXEV2rTq3dU6/syaXGMXHVAoGATwXpfPN2KsBG09dPjGXju0QXJI3jaVxO0ikN\n0tSDN/kmGaNnIO9yjnRHgMwY1PMeA7TXYZFnC+AZ9YLUJ3r6sUcL2X95fnuPa5Ix\naQxd5yFXFQ1gjOSfIvavXNT9xqQ6x7X8ndWxXt8yp7AohiW2LPNoqRBEPfltd8QP\nNVnU1vECgYEAkubQ865x64Li1bzEIPCpCbcUdni/Ye5M4yzDilYFirw0Fm90u20P\n/8eAXoE7STS69IDvXN35wkLlZZBnvxWjNJd5Q2uAtJp+nSFJsaxxduTNBIQ/5BKV\nWZcHZdfnvpapilryIR+s6edVbFQ7201aBWE/6u9MfvihomyP+va1H6Q=\n-----END RSA PRIVATE KEY-----\n\n-----------------------------30206167019393--\n';
		          dojo.xhrPost({
		        	 	headers: { 
		        	 		"Content-Type": "multipart/form-data; boundary=---------------------------239602725422497",
		        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
		        	 	},	  	  
					    url:Utils.getBaseUri() + "rest/config/certificateProfiles/0",
					    handleAs: "json",
					    putData: cert,
					    sync: true,
		                load: function(data) {
		  		          dojo.xhrPost({
				        	 	headers: { 
				        	 		"Content-Type": "multipart/form-data; boundary=---------------------------30206167019393",
				        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
				        	 	},	  	  
							    url:Utils.getBaseUri() + "rest/config/certificateProfiles/0",
							    handleAs: "json",
							    sync: true,
							    putData: key,
				                load: function(data) {
				  		          dojo.xhrPost({
						        	 	headers: { 
						        	 		"Content-Type": "application/json",
						        	 		"X-Session-Verify": dojo.cookie("xsrfToken")						    
						        	 	},	  	  
									    url:Utils.getBaseUri() + "rest/config/certificateProfiles/0",
									    handleAs: "json",
									    sync: true,
									    putData: JSON.stringify({"Name":"spTest.cp01", "Certificate": "ima.spTest.cp01.ca.cert", "Key": "ima.spTest.cp01.ca.key"}),
						                load: function(data) {
						                },
										error: function(error) {
											console.error(error);
											assertTrue(false);											
										}
								 }); 
				                },
								error: function(error) {
									console.error(error);
									assertTrue(false);									
								}
						 });
		                },
						error: function(error) {
							console.error(error);
							assertTrue(false);							
						}
				 }); 
			}
		}),
        new LoginFixture ({name: "test_getSecurityProfilesA", minExpected: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
	        		  console.log("securityProfiles: " );
	        		  console.dir(data);
	        	      doh.assertTrue(data.length >= this.minExpected);
					  console.log("finished test_getSecurityProfilesA")	        	      
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0",
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                },
		                error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
		                }
				 }); 
		         return d;
		    }
		}),

        new LoginFixture ({name: "test_createSecurityProfile",
			securityProfileName: "SP1",
			MinimumProtocolMethod: "SSLv3",
			Ciphers: "Best",
			UseClientCertificate: "False",
			CertificateProfile: "spTest.cp01",
			UseClientCipher: "True",		    		    
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {
						  console.log("profile is " );
						  console.dir(data);
						  doh.assertTrue(data.MinimumProtocolMethod == this.MinimumProtocolMethod);
						  doh.assertTrue(data.Ciphers == this.Ciphers);
						  doh.assertTrue(data.UseClientCertificate == this.UseClientCertificate);
				    	  doh.assertTrue(data.CertificateProfile == this.CertificateProfile);
				    	  doh.assertTrue(data.UseClientCipher == this.UseClientCipher);				        	  
						  console.log("finished test_createSecurityProfile")	        	      
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/securityProfiles/0/" + this.securityProfileName,
						    handleAs: "json",
                            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
			                load: function(data) {
			                      d2.callback(data);
			                },
							error: function(error) {
								console.error(error);
			                	doh.assertTrue(false);  // this is a failure
							}
					 }); 
			         return d2;	        		  
		          }));
		          dojo.xhrPost({
		        	 	headers: { 
		        	 		"Content-Type": "application/json",
                             "X-Session-Verify": dojo.cookie("xsrfToken")
		        	 	},	  	  
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0",
					    handleAs: "json",
					    putData: JSON.stringify({"Name":this.securityProfileName, 
									    	 	 "MinimumProtocolMethod": this.MinimumProtocolMethod, 
									    	 	 "UseClientCertificate": this.UseClientCertificate,
									    		 "Ciphers": this.Ciphers,
									    		 "CertificateProfile":this.CertificateProfile,
									    		 "UseClientCipher": this.UseClientCipher}),
		                load: function(data) {
		                      d.callback(data);
		                },
						error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
						}
				 }); 
		         return d;
		    }
		}),		
        new LoginFixture ({name: "test_addSecurityProfile2",
			securityProfileName: "SP2",
			MinimumProtocolMethod: "TLSv1.1",
			Ciphers: "Medium",
			UseClientCertificate: "True",
			CertificateProfile: "spTest.cp01",
			UseClientCipher: "False",		    		    
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
			          var d2 = new doh.Deferred();
			          d2.addCallback(lang.hitch(this,function(data) {
						  console.log("profile is " );
						  console.dir(data);
						  doh.assertTrue(data.MinimumProtocolMethod == this.MinimumProtocolMethod);
						  doh.assertTrue(data.Ciphers == this.Ciphers);
						  doh.assertTrue(data.UseClientCertificate == this.UseClientCertificate);
				    	  doh.assertTrue(data.CertificateProfile == this.CertificateProfile);
				    	  doh.assertTrue(data.UseClientCipher == this.UseClientCipher);				        	  
						  console.log("finished test_createSecurityProfile")	        	      
			          }));
			          dojo.xhrGet({
						    url:Utils.getBaseUri() + "rest/config/securityProfiles/0/" + this.securityProfileName,
						    handleAs: "json",
                            headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
			                load: function(data) {
			                      d2.callback(data);
			                },
							error: function(error) {
								console.error(error);
			                	doh.assertTrue(false);  // this is a failure
							}
					 }); 
			         return d2;	        		  
		          }));
		          dojo.xhrPost({
		        	 	headers: { 
		        	 		"Content-Type": "application/json",
                            "X-Session-Verify": dojo.cookie("xsrfToken")
                        },	  	  
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0",
					    handleAs: "json",
					    putData: JSON.stringify({"Name":this.securityProfileName, 
									    	 	 "MinimumProtocolMethod": this.MinimumProtocolMethod, 
									    	 	 "UseClientCertificate": this.UseClientCertificate,
									    		 "Ciphers": this.Ciphers,
									    		 "CertificateProfile":this.CertificateProfile,
									    		 "UseClientCipher": this.UseClientCipher}),
		                load: function(data) {
		                      d.callback(data);
		                },
						error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
						}
				 }); 
		         return d;
		    }
		}),		
		
        new LoginFixture ({name: "test_getSecurityProfiles",
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
	        		  console.log("securityProfiles: " );
	        		  console.dir(data);
	        	      doh.assertTrue(data.length >= 2);
					  console.log("finished test_getSecurityProfiles")	        	      
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0",
					    handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                },
		                error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
						}
				 }); 
		         return d;
		    }
		}),
        new LoginFixture ({name: "test_deleteSecurityProfile", securityProfileName: "SP1",		    		    		    
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
	        	      doh.assertTrue(true); // if we get here it passed
		          }));
		          dojo.xhrDelete({
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0/" + this.securityProfileName,
					    handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                },
		                error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
						}
				 }); 
		         return d;
		    }
		}),
        new LoginFixture ({name: "test_deleteSecurityProfile2", securityProfileName: "SP2",		    		    		    
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
	        	      doh.assertTrue(true); // if we get here it passed
		          }));
		          dojo.xhrDelete({
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0/" + this.securityProfileName,
					    handleAs: "json",
                        headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                },
		                error: function(error) {
							console.error(error);
		                	doh.assertTrue(false);  // this is a failure
						}
				 }); 
		         return d;
		    }
		}),		
        new LoginFixture ({name: "test_getDeletedSecurityProfile", securityProfileName: "SP2",		    
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
	        	      doh.assertTrue(data); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/securityProfiles/0/"+this.securityProfileName,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},					    
		                load: function(data) {
		                      d.callback(false);
		                },
		                error: function(error) {
		                	d.callback(true);
						}
				 }); 
		         return d;
		    }
		}),
		new LoginFixture ({name:"test_reset", certificateName: "spTest.cp01",
			runTest: function () {
		          dojo.xhrDelete({
					    url:Utils.getBaseUri() + "rest/config/certificateProfiles/0/"+this.certificateName,
					    handleAs: "json",
					    sync: true,
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},						    
		                load: function(data) {
		                },
		                error: function(error) {
							console.error(error);
		                	doh.assertTrue(false); // this is a failure
						}
				 }); 				
			}
		})		
	]);
});
