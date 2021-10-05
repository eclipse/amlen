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

#ifndef __LTPAUTILS_DEFINED
#define __LTPAUTILS_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <security.h>
#include <ismrc.h>
#include <ismutil.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/des.h>
#include <openssl/aes.h>



/**
 * Get LTPA Secret key from key from LTAP Key file.
 *
 * @param keyfilename		Fullsy qualified name of the LTPA Key file
 * @param password  	 	The password used to encrypt the key file
 * @param rc     			Return code
 *
 * @return LTPA secret key
 *
 * NOTE: Caller should free returned buffer
 */
XAPI unsigned char * ism_security_get_ltpaSecretKey(const char *keyfilename, const char *password, int * outlen, int *rc);

/* TODO - add comments for the following  API */
XAPI unsigned char * ism_base64_decode_buffer(unsigned char *base64InputStr, int base64InputStr_len, long *retBytes);

XAPI unsigned char * ism_decrypt_buffer(unsigned char *message, unsigned char *key, int messageSize, const EVP_CIPHER *cipher, int * output_len);


#ifdef __cplusplus
}
#endif

#endif


