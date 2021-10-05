#!/bin/bash

echo keytool -import -alias chain -keystore trust.jks -file ../server-trust-chain-jmqtt08.pem -storepass password -noprompt
keytool -import -alias chain -keystore trust.jks -file ../server-trust-chain-jmqtt08.pem -storepass password -noprompt

for i in rev*crt.pem; do 
f="${i%-crt.*}"


# openssl pkcs12 -export -inkey revoked-10000.pem -in revoked-10000-crt.pem -out revoked-10000.p12 -password pass:qwertyuiop
  openssl pkcs12 -export -inkey $f.pem -in $f-crt.pem -out $f.p12 -password pass:password

# keytool -importkeystore -destkeystore revoked-10000.jks -srckeystore revoked-10000.p12 -srcstorepass qwertyuiop -deststorepass qwertyuop -noprompt -srcstoretype PKCS12
  keytool -importkeystore -destkeystore $f.jks -srckeystore $f.p12 -srcstorepass password -deststorepass password -noprompt -srcstoretype PKCS12

# echo keytool -import -alias intCA1 -keystore $f.jks -file ../intCA1-client-crt-jmqtt08.pem -storepass password -noprompt
# keytool -import -alias intCA1 -keystore $f.jks -file ../intCA1-client-crt-jmqtt08.pem -storepass password -noprompt
# echo keytool -import -alias rootCA -keystore $f.jks -file ../rootCA-crt-jmqtt08.pem -storepass password -noprompt
# keytool -import -alias rootCA -keystore $f.jks -file ../rootCA-crt-jmqtt08.pem -storepass password -noprompt
done
