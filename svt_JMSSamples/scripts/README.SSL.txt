------------------------------
Steps to get JMS client Security working 
------------------------------

1. Setup ISM server side - following the instructions at this wiki , to setup secure port 16111. In this case setup on server 9.3.177.15

https://w3-connections.ibm.com/wikis/home?lang=en#/wiki/W11ac9778a1d4_4f61_913c_a76a78c7bff7/page/Example%20ISM%20Server%20SSL%20setup%20commands

Console> imaserver import certificate scp://root@10.10.1.231:/root/ismserver-crt.pem
Password:
ismserver-crt.pem                                                                     100% 1273     1.2KB/s   00:00
Console> imaserver apply certificate ismserver-crt.pem
Certificate or key file ismserver-crt.pem is stored in key store.
Console> imaserver import certificate scp://root@10.10.1.231:/root/ismserver-key.pem
Password:
ismserver-key.pem                                                                     100% 1679     1.6KB/s   00:00
Console> imaserver apply certificate ismserver-key.pem
Certificate or key file ismserver-key.pem is stored in key store.
Console> imaserver set CertificateProfile Name=SVTCertProf Certificate=ismserver-crt.pem Key=ismserver-key.pem
Configuration item is set. (rc=0)
Console> imaserver set SecurityProfile Name=SVTSecProf MinimumProtocolMethod=SSLv3 UseClientCertificate=False Ciphers=Fast CertificateProfile=SVTCertProf
Configuration item is set. (rc=0)
Console> imaserver set MessageHub Name=SVTSSLHub Description=SVTSSLHub
Configuration item is set. (rc=0)
Console> imaserver set MessagingPolicy Name=SVTSSLMsgPol Topic=/car "ActionList=Publish,Subscribe"
Configuration item is set. (rc=0)
Console> imaserver set Listener Name=SVTSSLEndpoint Enabled=True SecurityProfile=SVTSecProf Port=16111 SecurityPolicies=DemoConnectionPolicy,SVTSSLMsgPol MessageHub=SVTSSLHub
Configuration item is set. (rc=0)
Console> imaserver stop
ISM server is stopped.
 
Console> imaserver start
Start ISM Server.
ISM server is started.
Console>

2. Setup client side - create CAfile.pem from input files

Create pkcs12 keystore:
  cat ismserver-key.pem ismserver-crt.pem > cacerts.pem
  openssl pkcs12 -export -in cacerts.pem -out cacerts.pkcs12 -name ismAlias -noiter -nomaciter

Create jks truststore:
  keytool -import -file ismserver-crt.pem -alias ismAlias -keystore cacerts.jks
  keytool -import -file rootCA-crt.pem -alias caAlias -keystore cacerts.jks

For additional information see:  http://docs.oracle.com/cd/E19509-01/820-3503/ggfgo/index.html

3. Start running secure commands:

For JMS subscriber:
java -Djavax.net.ssl.keyStoreType=pkcs12 -Djavax.net.ssl.keyStore=cacerts.pkcs12 -Djavax.net.ssl.keyStorePassword=k1ngf1sh 
-Djavax.net.ssl.trustStore=cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh com.ibm.ima.samples.jms.JMSSample 
-a subscribe -t "#" -s tcps://9.3.177.15:16111 -n 100 -v

For JMS publisher:
java -Djavax.net.ssl.keyStoreType=pkcs12 -Djavax.net.ssl.keyStore=cacerts.pkcs12 -Djavax.net.ssl.keyStorePassword=k1ngf1sh 
-Djavax.net.ssl.trustStore=cacerts.jks -Djavax.net.ssl.trustStorePassword=k1ngf1sh com.ibm.ima.samples.jms.JMSSample 
-a publish -t /car -s tcps://9.3.177.15:16111 -n 100 -v


4. As a side note this command can be used for insecure communication but still using the 16111 port:

* - Anonymous connection: Both client and server do not get authenticated and no credentials are needed
*   to establish an SSL connection. Note that this scenario is not fully secure since it is subject to
*   man-in-the-middle attacks

 ./mqttsample_array -S enableServerCertAuth=0 -s ssl://9.3.177.15:16111 -a publish -v

