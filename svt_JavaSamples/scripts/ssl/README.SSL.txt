------------------------------
Steps to get MQTT Java client Security working 
------------------------------

1. Setup ISM server side - following the instructions at this wiki , to setup secure port 16111. In this case setup on server 9.3.177.15

https://w3-connections.ibm.com/wikis/home?lang=en#/wiki/W11ac9778a1d4_4f61_913c_a76a78c7bff7/page/Example%20ISM%20Server%20SSL%20setup%20commands

Console> imaserver import certificate scp://root@10.10.1.231:/root/ismserver-crt.pem
Password:
ismserver-crt.pem                                                                     100% 1273     1.2KB/s   00:00
Console> imaserver import certificate scp://root@10.10.1.231:/root/ismserver-key.pem
Password:
ismserver-key.pem                                                                     100% 1679     1.6KB/s   00:00
Console> imaserver apply certificate CertFilename=ismserver-crt.pem KeyFilename=ismserver-key.pem
Certificate or key file ismserver-key.pem is stored in key store.
Console> imaserver set CertificateProfile Name=SVTCertProf Certificate=ismserver-crt.pem Key=ismserver-key.pem
Configuration item is set. (rc=0)
Console> imaserver set SecurityProfile Name=SVTSecProf MinimumProtocolMethod=SSLv3 UseClientCertificate=False Ciphers=Fast CertificateProfile=SVTCertProf
Configuration item is set. (rc=0)
Console> imaserver set MessageHub Name=SVTSSLHub Description=SVTSSLHub
Configuration item is set. (rc=0)
Console> imaserver set MessagingPolicy Name=SVTSSLMsgPol Topic=/car "ActionList=Publish,Subscribe"
Configuration item is set. (rc=0)
Console> imaserver set Endpoint Name=SVTSSLEndpoint Enabled=True SecurityProfile=SVTSecProf Port=16111 SecurityPolicies=DemoConnectionPolicy,SVTSSLMsgPol MessageHub=SVTSSLHub
Configuration item is set. (rc=0)

Create connection policy instructions
Console> file get "scp://root@9.3.179.167:/niagara/test/svt_jmqtt/ssl/" "ismserver-crt.pem"
Console> file get "scp://root@9.3.179.167:/niagara/test/svt_jmqtt/ssl/" "ismserver-key.pem"
Console> imaserver apply  Certificate "CertFileName=ismserver-crt.pem" "KeyFileName=ismserver-key.pem" "CertFilePassword=" "KeyFilePassword=" "Overwrite=True"
Console> imaserver create CertificateProfile "Name=SVTCertProf" "Certificate=ismserver-crt.pem" "Key=ismserver-key.pem"
Console> imaserver create SecurityProfile "Name=SVTSecProf" "MinimumProtocolMethod=SSLv3" "UseClientCertificate=False" "Ciphers=Fast" "UsePasswordAuthentication=False" "CertificateProfile=SVTCertProf"

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

For Java MQTT subscriber:
java -Djavax.net.ssl.keyStoreType=pkcs12 -Djavax.net.ssl.keyStore=cacerts.pkcs12 -Djavax.net.ssl.keyStorePassword=###### 
-Djavax.net.ssl.trustStore=cacerts.jks -Djavax.net.ssl.trustStorePassword=###### com.ibm.ism.samples.mqttv3.MqttSample
-a subscribe -t "#" -s tcps://9.3.177.15:16111 -n 100 -v

For Java MQTT publisher:
java -Djavax.net.ssl.keyStoreType=pkcs12 -Djavax.net.ssl.keyStore=cacerts.pkcs12 -Djavax.net.ssl.keyStorePassword=###### 
-Djavax.net.ssl.trustStore=cacerts.jks -Djavax.net.ssl.trustStorePassword=###### com.ibm.ism.samples.mqttv3.MqttSample 
-a publish -t /car -s tcps://9.3.177.15:16111 -n 100 -v


