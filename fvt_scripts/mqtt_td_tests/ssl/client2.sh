#!/bin/bash

# This script will generate a CA and set of certificates for use in the client certificates CommonName testing
# Just answer "password" to all password prompts and "yes" to trust question.
#
# The following files need to be retained for use in the testing:
#   cacert.pem
#   client1store.jks
#   client2store.jks
#   client3store.jks
#   client4store.jks
#   servercert.pem
#   serverkey.pem
#
set -x

openssl genrsa -out cakey.pem 2048
openssl req -new -x509 -days 3650 -key cakey.pem -out cacert.pem -sha256 -subj "/C=US/L=Austin/O=IBM/CN=IMALocalCA"
openssl x509 -noout -text -in cacert.pem
openssl genrsa -out serverkey.pem 2048
openssl genrsa -out client1key.pem 2048
openssl genrsa -out client2key.pem 2048
openssl genrsa -out client3key.pem 2048
openssl genrsa -out client4key.pem 2048
openssl req -new -key serverkey.pem -out serverkey.csr -subj "/C=US/O=IBM/CN=ibm.com"
openssl req -new -key client1key.pem -out client1key.csr -subj "/C=US/O=IBM/OU=Client1/CN=MQTTClient1"
openssl req -new -key client2key.pem -out client2key.csr -subj "/C=US/O=IBM/OU=Client1/CN=MQTTClient2"
openssl req -new -key client3key.pem -out client3key.csr -subj "/C=US/O=IBM/OU=Client1/CN=MQTT*Client3"
openssl req -new -key client4key.pem -out client4key.csr -subj "/C=US/O=IBM/OU=Client1/CN=MQTTClient4*"
openssl x509 -req -days 3650 -in client1key.csr -CA cacert.pem -CAkey cakey.pem -set_serial 1 -out client1cert.pem
openssl x509 -req -days 3650 -in client2key.csr -CA cacert.pem -CAkey cakey.pem -set_serial 2 -out client2cert.pem
openssl x509 -req -days 3650 -in client3key.csr -CA cacert.pem -CAkey cakey.pem -set_serial 3 -out client3cert.pem
openssl x509 -req -days 3650 -in client4key.csr -CA cacert.pem -CAkey cakey.pem -set_serial 4 -out client4cert.pem
openssl x509 -req -days 3650 -in serverkey.csr -CA cacert.pem -CAkey cakey.pem -set_serial 94345 -out servercert.pem
openssl rsa -noout -text -in client1key.pem
openssl rsa -noout -text -in client2key.pem
openssl rsa -noout -text -in client3key.pem
openssl rsa -noout -text -in serverkey.pem
openssl x509 -noout -text -in client1cert.pem
openssl x509 -noout -text -in client2cert.pem
openssl x509 -noout -text -in servercert.pem
openssl pkcs12 -export -inkey client1key.pem -in client1cert.pem -out client1cert.p12 -passout pass:password
openssl pkcs12 -export -inkey client2key.pem -in client2cert.pem -out client2cert.p12 -passout pass:password
openssl pkcs12 -export -inkey client3key.pem -in client3cert.pem -out client3cert.p12 -passout pass:password
openssl pkcs12 -export -inkey client4key.pem -in client4cert.pem -out client4cert.p12 -passout pass:password
keytool -import -trustcacerts -alias root -file cacert.pem -keystore client1store.jks -storepass password 
keytool -importkeystore -srckeystore client1cert.p12 -srcstoretype PKCS12 -destkeystore client1store.jks -deststoretype JKS -storepass password
keytool -import -trustcacerts -alias root -file cacert.pem -keystore client2store.jks -storepass password
keytool -importkeystore -srckeystore client2cert.p12 -srcstoretype PKCS12 -destkeystore client2store.jks -deststoretype JKS -storepass password
keytool -import -trustcacerts -alias root -file cacert.pem -keystore client3store.jks -storepass password
keytool -importkeystore -srckeystore client3cert.p12 -srcstoretype PKCS12 -destkeystore client3store.jks -deststoretype JKS -storepass password
keytool -import -trustcacerts -alias root -file cacert.pem -keystore client4store.jks -storepass password
keytool -importkeystore -srckeystore client4cert.p12 -srcstoretype PKCS12 -destkeystore client4store.jks -deststoretype JKS -storepass password
