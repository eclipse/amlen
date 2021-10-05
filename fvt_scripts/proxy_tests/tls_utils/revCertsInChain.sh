set -x
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
cp ../intermedext_revoke.cfg .
touch revcertindex
echo 0500 > revcertserial
echo 0500 > revcrlnumber

openssl genrsa -out revRootCA-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out revCA-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out revICA-key.pem     $SIZE >/dev/null 2>/dev/null
openssl genrsa -out rclient-key.pem $SIZE >/dev/null 2>/dev/null


openssl req -new -x509 -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=revRoot" -extensions revRoot_ca -config intermedext_revoke.cfg -set_serial 500 -key revRootCA-key.pem -out revRootCA-crt.pem
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=revoke" -key revCA-key.pem -out revCA-crt.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=Corporate/CN=revokei" -key revICA-key.pem -out revICA-crt.csr

openssl x509 -days 3560 -in revCA-crt.csr -out revCA-crt.pem -req -CA revRootCA-crt.pem -CAkey revRootCA-key.pem -set_serial 501 -extensions revoke_rca -extfile intermedext_revoke.cfg
#openssl x509 -days 3560 -in revCA-crt.csr -out revCA-crt.pem -req -CA revRootCA-crt.pem -CAkey revRootCA-key.pem -set_serial 501

openssl x509 -days 3560 -in revICA-crt.csr -out revICA-crt.pem -req -CA revCA-crt.pem -CAkey revCA-key.pem -set_serial 502 -extensions revoke_ica -extfile intermedext_revoke.cfg

openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=rclient" -key rclient-key.pem -out rclient1-crt.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=rclient" -key rclient-key.pem -out rclient2-crt.csr
openssl req -new -days 3560 -subj "/C=US/ST=Texas/L=Austin/O=IBM/OU=IMATEST/CN=rclient" -key rclient-key.pem -out rclient3-crt.csr

openssl x509 -days 3560 -in rclient1-crt.csr -out rclient1-crt.pem -req -CA revICA-crt.pem -CAkey revICA-key.pem -extensions revoke_ca -extfile intermedext_revoke.cfg -set_serial 512 2>/dev/null
openssl x509 -days 3560 -in rclient2-crt.csr -out rclient2-crt.pem -req -CA revICA-crt.pem -CAkey revICA-key.pem -extensions revoke_ca -extfile intermedext_revoke.cfg -set_serial 513 2>/dev/null
openssl x509 -days 3560 -in rclient3-crt.csr -out rclient3-crt.pem -req -CA revICA-crt.pem -CAkey revICA-key.pem -extensions revoke_ca -extfile intermedext_revoke.cfg -set_serial 514 2>/dev/null

#TODO - more work on this section
openssl pkcs12 -export -inkey rclient-key.pem -in rclient1-crt.pem -out rclient1.p12 -password pass:password
openssl pkcs12 -export -inkey rclient-key.pem -in rclient2-crt.pem -out rclient2.p12 -password pass:password
openssl pkcs12 -export -inkey rclient-key.pem -in rclient3-crt.pem -out rclient3.p12 -password pass:password

keytool -importkeystore -srckeystore rclient1.p12 -destkeystore chainRevoke1.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
keytool -import -noprompt -trustcacerts -alias proxydefaultctx -file ../../keystore/ibmbServerCA.crt -keystore chainRevoke1.jks -storepass password
keytool -import -noprompt -trustcacerts -alias mqttsdefaultctx -file ../../keystore/mqttsep.pem -keystore chainRevoke1.jks -storepass password

keytool -importkeystore -srckeystore rclient2.p12 -destkeystore chainRevoke2.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
keytool -import -noprompt -trustcacerts -alias proxydefaultctx -file ../../keystore/ibmbServerCA.crt -keystore chainRevoke2.jks -storepass password
keytool -import -noprompt -trustcacerts -alias mqttsdefaultctx -file ../../keystore/mqttsep.pem -keystore chainRevoke2.jks -storepass password

keytool -importkeystore -srckeystore rclient3.p12 -destkeystore chainRevoke3.jks -srcstoretype pkcs12 -deststorepass password -deststoretype jks -srcstorepass password
keytool -import -noprompt -trustcacerts -alias proxydefaultctx -file ../../keystore/ibmbServerCA.crt -keystore chainRevoke3.jks -storepass password
keytool -import -noprompt -trustcacerts -alias mqttsdefaultctx -file ../../keystore/mqttsep.pem -keystore chainRevoke3.jks -storepass password
#END TODO - more work on this section

openssl ca -config intermedext_revoke.cfg -gencrl -cert revICA-crt.pem -keyfile revICA-key.pem -out chainrevcrl0.crl

openssl ca -config intermedext_revoke.cfg -revoke rclient1-crt.pem -cert revICA-crt.pem -keyfile revICA-key.pem
openssl ca -config intermedext_revoke.cfg -gencrl -cert revICA-crt.pem -keyfile revICA-key.pem -out chainrevcrl1.crl

openssl ca -config intermedext_revoke.cfg -revoke rclient2-crt.pem -cert revICA-crt.pem -keyfile revICA-key.pem
openssl ca -config intermedext_revoke.cfg -gencrl -cert revICA-crt.pem -keyfile revICA-key.pem -out chainrevcrl2.crl

openssl ca -config intermedext_revoke.cfg -gencrl -cert revCA-crt.pem -keyfile revCA-key.pem -out ca_revcrl0.crl
openssl ca -config intermedext_revoke.cfg -gencrl -cert revRootCA-crt.pem -keyfile revRootCA-key.pem -out rca_revcrl0.crl

openssl crl -outform DER -in chainrevcrl0.crl -out chainrevcrl0.der
openssl crl -outform DER -in chainrevcrl1.crl -out chainrevcrl1.der
openssl crl -outform DER -in chainrevcrl2.crl -out chainrevcrl2.der

openssl crl -outform DER -in ca_revcrl0.crl -out ca_revcrl0.der
openssl crl -outform DER -in rca_revcrl0.crl -out rca_revcrl0.der
