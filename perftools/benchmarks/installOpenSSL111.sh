#!/bin/bash

OPENSSL_111=openssl-1.1.1
OPENSSL_111_TGZ=$OPENSSL_111.tar.gz
OPENSSL_111_URL=https://www.openssl.org/source/$OPENSSL_111_TGZ

wget $OPENSSL_111_URL
tar zxvf $OPENSSL_111_TGZ
cd $OPENSSL_111
./config --prefix=/usr/local/$OPENSSL_111
make
sudo make install


