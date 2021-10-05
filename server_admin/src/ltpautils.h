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
#ifndef __ISM_LTPAUTILSH_DEFINED
#define __ISM_LTPAUTILSH_DEFINED

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>

/* LTPA Key structure with parsed contents from LTPA key file */
typedef struct ltpa_keydata {
    void          *des_key;
    size_t        des_key_len;
    RSA           *rsa;
    unsigned char *rsaMod;
    size_t        rsaModLen;
    char          *realm;
    char          *version;
} ltpa_keydata_t;


#endif
