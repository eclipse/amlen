#!/bin/sh
#------------------------------------------------------------------------------
# gencrts.sh
#
# Create various ssl certificates for testing purposes.
#
# rootCA A - self signed root certificate - serial 1
#   imaservera - server certificate signed by rootCA A - serial 11
#   imaclienta - client certificate signed by rootCA A - serial 12
# rootCA b - self signed root certificate - serial 2
#   imaserverb - server certificate signed by rootCA B - serial 21
# rootCA C - self signed root certificate (imposter of A) - serial 1
#   imaserverc - server certificate signed by rootCA C - serial 11
#   imaclientc - client certificate signed by rootCA C - serial 12
#
#------------------------------------------------------------------------------
if [ "$1" == "" -o "$2" == "" ] ; then
  echo "usage: $0 <cert size> <dir to create>"
  echo "  e.g. $0 2048 certs2Kb"
  exit 1
fi

SIZE=$1
DIR=$2
CURRDIR=$(pwd)

rm -rf $DIR
mkdir -p $DIR ; cd $DIR

# Create certificate extensions file for CA's
cat > ext.cfg <<EOF
[ req ]
attributes             = req_attributes
req_extensions         = v3_ca
prompt                 = no
[ req_attributes ]
[ v3_ca ]
basicConstraints       = CA:true
EOF

# Create certificate extensions file for server
cat > srvext.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = DNS:*.softlayer.com
EOF

cat > clientext2.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = @alt_section
[ alt_section ]
DNS=*.softlayer.com
email.1=admin@server.com
EOF

cat > clientext5.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = @alt_section
[ alt_section ]
DNS=*.softlayer.com
email=g:gtype:
EOF

cat > clientext6.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = @alt_section
[ alt_section ]
DNS=*.softlayer.com
email=d:dtype:d1
EOF

cat > clientext7.cfg <<EOF
[ req ]
req_extensions          = v3_req
[ v3_req ]
subjectAltName          = @alt_section
[ alt_section ]
DNS=*.softlayer.com
email.1=admin@server.com
email.2=g:gtype:
email.3=d:dtype:d1
EOF

# Generate RSA keys of specified size
openssl genrsa -out rootCAa-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out rootCAb-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out rootCAc-key.pem     $SIZE >/dev/null 2>/dev/null

openssl genrsa -out imaclienta-key.pem $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaclientc-key.pem $SIZE >/dev/null 2>/dev/null

openssl genrsa -out imaservera-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserverb-key.pem  $SIZE >/dev/null 2>/dev/null
openssl genrsa -out imaserverc-key.pem  $SIZE >/dev/null 2>/dev/null

# Create root CA A certificate (self-signed)
openssl req -new -x509 -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate A" -extensions v3_ca -set_serial 1 -key rootCAa-key.pem -out rootCAa-crt.pem

# Create root CA B certificate (self-signed)
openssl req -new -x509 -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate B" -extensions v3_ca -set_serial 2 -key rootCAb-key.pem -out rootCAb-crt.pem

# Create root CA C certificate (self-signed) - "Imposter" for CA A
openssl req -new -x509 -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=IBM Corporate A" -extensions v3_ca -set_serial 1 -key rootCAc-key.pem -out rootCAc-crt.pem

# Create the server A certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverA)" -key imaservera-key.pem -out imaservera-crt.csr
# Create the IMA server A certificate using the root CA A as the issuer
openssl x509 -days 3560 -in imaservera-crt.csr -out imaservera-crt.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 11 2>/dev/null

# Create the server B certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverB)" -key imaserverb-key.pem -out imaserverb-crt.csr
# Create the IMA server B certificate using the root CA B as the issuer
openssl x509 -days 3560 -in imaserverb-crt.csr -out imaserverb-crt.pem -req -CA rootCAb-crt.pem -CAkey rootCAb-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 21 2>/dev/null

# Create the server C certificate request - "Imposter" for server A cert
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(serverA)" -key imaserverc-key.pem -out imaserverc-crt.csr
# Create the IMA server C certificate using the root CA C as the issuer
openssl x509 -days 3560 -in imaserverc-crt.csr -out imaserverc-crt.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -extfile srvext.cfg -set_serial 11 2>/dev/null

# Create the client a certificate requests
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientA)" -key imaclienta-key.pem -out imaclienta-crt.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=d:dtype:" -key imaclienta-key.pem -out imaclienta-crt3.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=g:gtype:g1" -key imaclienta-key.pem -out imaclienta-crt4.csr

# Create the client c certificate request
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=IMA Test Team(imaclientC)" -key imaclientc-key.pem -out imaclientc-crt.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=d:dtype:" -key imaclientc-key.pem -out imaclientc-crt3.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=g:gtype:g1" -key imaclientc-key.pem -out imaclientc-crt4.csr

# Create the client a certificates using the root CA A as the issuer
openssl x509 -days 3560 -in imaclienta-crt.csr -out imaclienta-crt.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -set_serial 12 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt.csr -out imaclienta-crt2.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile clientext2.cfg -set_serial 1002 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt3.csr -out imaclienta-crt3.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -set_serial 1003 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt4.csr -out imaclienta-crt4.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -set_serial 1004 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt.csr -out imaclienta-crt5.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile clientext5.cfg -set_serial 1005 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt.csr -out imaclienta-crt6.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile clientext6.cfg -set_serial 1006 2>/dev/null
openssl x509 -days 3560 -in imaclienta-crt.csr -out imaclienta-crt7.pem -req -CA rootCAa-crt.pem -CAkey rootCAa-key.pem -extensions v3_req -extfile clientext7.cfg -set_serial 1007 2>/dev/null

# Create the client c certificate using the root CA C as the issuer
openssl x509 -days 3560 -in imaclientc-crt.csr -out imaclientc-crt.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -set_serial 12 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt.csr -out imaclientc-crt2.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -extfile clientext2.cfg -set_serial 2002 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt3.csr -out imaclientc-crt3.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -set_serial 2003 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt4.csr -out imaclientc-crt4.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -set_serial 2004 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt.csr -out imaclientc-crt5.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -extfile clientext5.cfg -set_serial 2005 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt.csr -out imaclientc-crt6.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -extfile clientext6.cfg -set_serial 2006 2>/dev/null
openssl x509 -days 3560 -in imaclientc-crt.csr -out imaclientc-crt7.pem -req -CA rootCAc-crt.pem -CAkey rootCAc-key.pem -extensions v3_req -extfile clientext7.cfg -set_serial 2007 2>/dev/null


echo -n "Do you want to view the certificates created? [y/N]: "
read response
if [ "$response" == "y" -o "$response" == "Y" ] ; then
  echo "IMA client certificate A..."
  openssl x509 -in imaclienta-crt.pem -text -noout -purpose
  echo "IMA client certificate C..."
  openssl x509 -in imaclientc-crt.pem -text -noout -purpose
  echo "IMA server certificate A..."
  openssl x509 -in imaservera-crt.pem -text -noout -purpose
  echo "IMA server certificate C..."
  openssl x509 -in imaserverc-crt.pem -text -noout -purpose
fi

# Create PKCS 12 bundle for client certificate A
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt.pem -out imaclienta.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt2.pem -out imaclienta2.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt3.pem -out imaclienta3.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt4.pem -out imaclienta4.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt5.pem -out imaclienta5.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt6.pem -out imaclienta6.p12 -password pass:password
openssl pkcs12 -export -inkey imaclienta-key.pem -in imaclienta-crt7.pem -out imaclienta7.p12 -password pass:password

  # Create jks for client A
  keytool -importkeystore -srckeystore imaclienta.p12 -destkeystore ibma.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A2
  keytool -importkeystore -srckeystore imaclienta2.p12 -destkeystore ibma2.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma2.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma2.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma2.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A3
  keytool -importkeystore -srckeystore imaclienta3.p12 -destkeystore ibma3.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma3.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma3.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma3.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A4
  keytool -importkeystore -srckeystore imaclienta4.p12 -destkeystore ibma4.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma4.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma4.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma4.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A5
  keytool -importkeystore -srckeystore imaclienta5.p12 -destkeystore ibma5.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma5.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma5.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma5.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A6
  keytool -importkeystore -srckeystore imaclienta6.p12 -destkeystore ibma6.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma6.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma6.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma6.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client A7
  keytool -importkeystore -srckeystore imaclienta7.p12 -destkeystore ibma7.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibma7.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibma7.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibma7.jks -storepass password -noprompt -alias mqttsDefaultCtx

# Create PKCS 12 bundle for client certificate C
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt.pem -out imaclientc.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt2.pem -out imaclientc2.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt3.pem -out imaclientc3.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt4.pem -out imaclientc4.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt5.pem -out imaclientc5.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt6.pem -out imaclientc6.p12 -password pass:password
openssl pkcs12 -export -inkey imaclientc-key.pem -in imaclientc-crt7.pem -out imaclientc7.p12 -password pass:password

  # Create jks for client C
  keytool -importkeystore -srckeystore imaclientc.p12 -destkeystore ibmc.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C2
  keytool -importkeystore -srckeystore imaclientc2.p12 -destkeystore ibmc2.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc2.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc2.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc2.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C3
  keytool -importkeystore -srckeystore imaclientc3.p12 -destkeystore ibmc3.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc3.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc3.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc3.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C4
  keytool -importkeystore -srckeystore imaclientc4.p12 -destkeystore ibmc4.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc4.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc4.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc4.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C5
  keytool -importkeystore -srckeystore imaclientc5.p12 -destkeystore ibmc5.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc5.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc5.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc5.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C6
  keytool -importkeystore -srckeystore imaclientc6.p12 -destkeystore ibmc6.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc6.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc6.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc6.jks -storepass password -noprompt -alias mqttsDefaultCtx

  # Create jks for client C7
  keytool -importkeystore -srckeystore imaclientc7.p12 -destkeystore ibmc7.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
  keytool -import -trustcacerts -file imaserverb-crt.pem -keystore ibmc7.jks -storepass password -noprompt -alias proxyDefaultCtx
  #keytool -import -trustcacerts -file ../../../common/servercert.pem -keystore ibmc7.jks -storepass password -noprompt -alias mqttsDefaultCtx
  keytool -import -trustcacerts -file ../../keystore/servercert.pem -keystore ibmc7.jks -storepass password -noprompt -alias mqttsDefaultCtx

cd $CURRDIR
