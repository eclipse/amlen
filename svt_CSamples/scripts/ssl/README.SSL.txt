------------------------------
Steps to get MQTT C client Security working 
------------------------------

1. Setup ISM server side - following the instructions at this wiki , to setup secure port 16111. In this case setup on server 10.10.10.10

Console> ismserver import certificate scp://root@10.10.1.231:/root/ismserver-crt.pem
Password:
ismserver-crt.pem                                                                     100% 1273     1.2KB/s   00:00
Console> ismserver apply certificate ismserver-crt.pem
Certificate or key file ismserver-crt.pem is stored in key store.
Console> ismserver import certificate scp://root@10.10.1.231:/root/ismserver-key.pem
Password:
ismserver-key.pem                                                                     100% 1679     1.6KB/s   00:00
Console> ismserver apply certificate ismserver-key.pem
Certificate or key file ismserver-key.pem is stored in key store.
Console> ismserver set CertificateProfile Name=SVTCertProf Certificate=ismserver-crt.pem Key=ismserver-key.pem
Configuration item is set. (rc=0)
Console> ismserver set SecurityProfile Name=SVTSecProf MinimumProtocolMethod=SSLv3 UseClientCertificate=False Ciphers=Fast CertificateProfile=SVTCertProf
Configuration item is set. (rc=0)
Console> ismserver set MessageHub Name=SVTSSLHub Description=SVTSSLHub
Configuration item is set. (rc=0)
Console> ismserver set MessagingPolicy Name=SVTSSLMsgPol PolicyType=Messaging Topic=/car "ActionList=Publish,Subscribe"
Configuration item is set. (rc=0)
Console> ismserver set Listener Name=SVTSSLEndpoint Enabled=True Security=Ture SecurityProfile=SVTSecProf Port=16111 SecurityPolicies=DemoConnectionPolicy,SVTSSLMsgPol MessageHub=SVTSSLHub
Configuration item is set. (rc=0)
Console> ismserver stop
ISM server is stopped.
 
Console> ismserver start
Start ISM Server.
ISM server is started.
Console>

2. Setup client side - create CAfile.pem from input files

 rm -rf CAfile.pem; 
 for x in ismserver-crt.pem rootCA-crt.pem  ismCA-crt.pem ; do openssl x509 -in $x -text >> CAfile.pem; done
 cp CAfile.pem trustStore.pem

3. Start running secure commands:

 ./mqttsample_array -S trustStore=CAfile.pem -s ssl://10.10.10.10:16111 -a publish -v -n 10

4. As a side note this command can be used for insecure communication but still using the 16111 port:
* - Anonymous connection: Both client and server do not get authenticated and no credentials are needed
*   to establish an SSL connection. Note that this scenario is not fully secure since it is subject to
*   man-in-the-middle attacks

 - from Allan Stockdill-Mander: Also on point 4 in your doc; to be clear this isn't really anonymous 
as such in that it will not be using an unauthenticated cipher suite (a *_anon_* one), disabling 
ServerCertAuth means that the client will accept any certificate from the server, treat it as valid 
and use it as the basis to encrypt a session.

 ./mqttsample_array -S enableServerCertAuth=0 -s ssl://10.10.10.10:16111 -a publish -v



---------------------------
Update: 3.22.13 - Client Certificates. 
---------------------------

Documentation: How To get Client Certificates working with the IMA and the MQTT C client 

1. IMA setup - Server Side setup for Client Certificates
1a. Create Certificate profile w/ imaserver-crt.pem and imasever-key.pem
1b. Create security profile and check Client Certificate Authentication 
1c. Upload Trusted Certificates to Security Profile : imaCA-crt.pem , rootCA-crt.pem
1d. Configure Message Hub for your Secure Endpoint with Client Certificates Make sure that your Secure Endpoint with Client Certificates uses the Security Profile configured in steps 1b, 1c

2. MQTT C client setup
2a. Develop Client application to use the SSL . Note: there is no documentation in the Dec 14th client Pack about how to do this.  -> Opened SVT: IMA: No ssl documentation in client pack. (29362)
2b. Important -Make sure to link to the secure version of the library libmqttv3cs.so . Note there is no documentation in the client pack about why/ when you would use the various libraries delivered. Opened defect: SVT: IMA : no documentation about why/when various libraries delivered in client pack should be used. (29363)
2c Create trustStore.pem file Using files configured on IMA in step 1c
Keys to success:
Create client side trustStore.pem file using the same files configured on IMA in step 1c

rm -rf trustStore.pem ;
for x in  /niagara/test/common/rootCA-crt.pem  /niagara/test/common/imaCA-crt.pem  ; do
     openssl x509 -in $x -text >> trustStore.pem ; 
done

2d Run your client application to do Client Certificate authenticated messaging
Keys to success:
Specify your trustStore created in step 2c, your keyStore and privateKey parameters
Specify ssl:// as the URI parameter supplied to MQTTClient_create API calls 


 ./mqttsample_array -S trustStore=trustStore.pem  -s ssl://10.10.10.10:16113 -a publish  -S keyStore=imaclient-crt.pem  -S privateKey=imaclient-key.pem  -v




---------------------------
Update: 1.23.14 - Client Certificates and samples provided wth messaging pack
---------------------------

To do client certificate authentication with the samples provided with the messaging pack , they only
provide the option to pass in keyStore and trustStore. Thus you need to make a conncatenated file 
for the keystore and use that file to do client certificate auth.

cat imaclient-crt.pem  imaclient-key.pem  >imaclient-keystore.pem

e.g. connecting to w/ client certifcate only using trustStore and Keystore

MQTTV3SSAMPLE -a publish -t /MQTTV3ASAMPLE -b 10.10.10.10 -p 17774 -s 1 -v 1 -r ssl/trustStore.pem -m "Houston we have a problem" -k ssl/imaclient-keystore.pem 

e.g. connecting to w/ client certifcate only using trustStore and Keystore using mqttsample_array app. Previously only did this with privateKey specified.

mqttsample_array  -S trustStore=ssl/trustStore.pem -s ssl://10.10.1.125:17774 -a subscribe  -S keyStore=ssl/imaclient-keystore.pem 

