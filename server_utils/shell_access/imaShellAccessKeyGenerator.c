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

/*
 * Utility to generate access key.
 * Usage: imaShellAccessKeyGenerator <serial_number> <validity_time>
 *        serial_number - Serial number of the appliance as returned by
 *                        "show version" command.
 *        validity_time - Time in hours the access key will be valid.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/md5.h>
#include <openssl/rc4.h>

#define BUFLEN      2048
#define MAXEXPTIME  120

static unsigned char * encryptKey(unsigned char * src, int * len) {
    char key[EVP_MAX_KEY_LENGTH] = "pDm99d30ccF3W8+8ak5CN4jrnCSBh+ML";
    unsigned char *iv = NULL;
    int inLen = strlen(src) + 1;
    unsigned char *out = NULL;
    int outLen = 0;

    if (len == NULL) {
    	return NULL;
    }

    const EVP_CIPHER* cipherType = EVP_des_ede3_ecb();
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);

    /* Encrypt */
    unsigned char *inpSrc = alloca(inLen * sizeof(char));
    memcpy(inpSrc, src, inLen);
    out = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,252),2 * inLen * sizeof(char));
    memset(out, 0, 2*inLen);

    if ( EVP_EncryptInit(ctx, cipherType, (const unsigned char*)key, iv) == 0 )
        printf("EecryptInit Failed\n");

    if ( EVP_EncryptUpdate(ctx, out, &outLen, inpSrc, inLen) == 0 )
        printf("EncryptUpdate error\n");

    *len = outLen;

    EVP_EncryptFinal(ctx, out+outLen, &outLen);

    *len += outLen;

    EVP_CIPHER_CTX_cleanup(ctx);
    EVP_cleanup();

    return out;
}

static unsigned char * decryptKey(unsigned char * src, int inLen) {
	char key[EVP_MAX_KEY_LENGTH] = "pDm99d30ccF3W8+8ak5CN4jrnCSBh+ML";
	unsigned char *iv = NULL;
	unsigned char *out;
	int            outLen = 0;

	const EVP_CIPHER * cipherType = EVP_des_ede3_ecb();
	EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);

	out = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,253),inLen * sizeof(char));
	memset(out, 0x00, inLen);
	if (EVP_DecryptInit(ctx, cipherType, (const unsigned char*)key, iv) == 0){
		fprintf(stderr, "_enabledhell: DecryptInit error\n");
		ism_common_free(ism_memory_utils_misc,out);
		return NULL;
	}

	if ( EVP_DecryptUpdate(ctx, out, &outLen, src, inLen) == 0 ) {
		printf("_enabledhell: DecryptUpdate error\n");
		ism_common_free(ism_memory_utils_misc,out);
		return NULL;
	}

	EVP_CIPHER_CTX_cleanup(ctx);
	EVP_cleanup();

	return out;
}

static char * getToken(char * from, const char * leading, const char * trailing, char * * more) {
    char * ret;
    if (!from)
        return NULL;
    while (*from && strchr(leading, *from))
        from++;
    if (!*from) {
        if (more)
            *more = NULL;
        return NULL;
    }
    ret = from;
    while (*from && !strchr(trailing, *from))
        from++;
    if (*from) {
        *from = 0;
        if (more)
            *more = from + 1;
    } else {
        if (more)
            *more = NULL;
    }
    return ret;
}

static int validate_shellKey(char *decryptedKey, time_t * expirationTime, char * systemSerialNumber) {
	char *more;
	char *dkptr = (char *)decryptedKey;
	char *serialNumber = (char *)getToken(dkptr, " \t\r\n", "|\r\n", &more);
	if (serialNumber && serialNumber[0]!='*' && serialNumber[0]!='#') {
		char * cp = serialNumber+strlen(serialNumber); /* trim trailing white space */
		while (cp>serialNumber && (cp[-1]==' ' || cp[-1]=='\t' ))
			cp--;
		*cp = 0;
		char *expTimeStr = (char *)getToken(more, " =\t\r\n", "\r\n", &more);
		if ( expTimeStr == NULL ) {
			return 1;
		}

		/* Match serial number */
		if ( systemSerialNumber == NULL ) return 1;
		if ( !strncmp(systemSerialNumber, serialNumber, strlen(systemSerialNumber))) {
			/* check expiration time */
			char tmpstr[11];
			memcpy(tmpstr, expTimeStr, 10);
			time_t expTime = atol(tmpstr);
			time_t curtime = time(NULL);
			fprintf(stderr, "Access key is for system serial number:%s\n", serialNumber);
			if ( expTime > curtime) {
				if (expirationTime) {
					*expirationTime = expTime;
				}
				fprintf(stdout, "Access key will expire on %s\n", ctime(&expTime));
				return 0;
			}
		}
	} else {
		return -1;
	}
	return 1;
}


static int hex2int(const char h){
    if(isdigit(h))
        return h - '0';
    else
        return toupper(h) - 'A' + 10;
}

static int verifyKey(char *newKey, char *serialNumber) {
	/* validate key and grant shell access */
	char *key = newKey;
	time_t expirationTime = 0;
	fprintf(stderr, "Verify Access Key: %s\n", key);

	char hkey[1024];
	memset(hkey, 0, 1024);
	int inLen = snprintf(hkey, sizeof(hkey), "%s", key);
	int outLen = inLen / 2;
	unsigned char *keyint = (unsigned char *)alloca(outLen);
	int i;

	for (i = 0; i < outLen; i++) {
		keyint[i] = hex2int(hkey[i * 2])*16 + hex2int(hkey[i * 2 + 1]);
	}

	char *decKey = (char *)decryptKey(keyint, outLen);
	int decLen = strlen(decKey) + 1;		// The string is NULL-terminated
	char *tmpkey = alloca(decLen + 1);
	memcpy(tmpkey, decKey, decLen + 1);
	int rc1 = validate_shellKey(tmpkey, &expirationTime, serialNumber);
	if ( rc1 != 0 ) {
		fprintf(stderr, "The access key is either expired or not valid. \n");
		return 1;
	}
	ism_common_free(ism_memory_utils_misc,decKey);
}


int main(int argc, char * argv[])
{
    int rc = 0;

    if ( argc < 3 ) {
        fprintf(stderr, "Usage: %s <serial_number> <validity_time>\n", argv[0]);
        return 1;
    }

    char *serialNumber = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),argv[1]);
    char *valTimeStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),argv[2]);
    unsigned long valTime = atol(valTimeStr);
    unsigned long curTime = time(NULL);
    unsigned long expTime = curTime + 3600 * valTime;
    int len;

    fprintf(stdout, "System Serial Number: %s\n", serialNumber);
    fprintf(stdout, "Key is valid for %ld hours starting %s\n", valTime, ctime(&curTime));
    if ( valTime < 1 || valTime > MAXEXPTIME ) {
        fprintf(stderr, "The valid range for validity time is from 1 hour to 120 hours.\n");
        return 1;
    }

    int klen = strlen(serialNumber) + 32;
    char *licStr = (char *)alloca(klen);
    memset(licStr, 0, klen);
    snprintf(licStr, klen, "%s | %ld", serialNumber, expTime);

    unsigned char *encLic = encryptKey((unsigned char *)licStr, &len);

    if ( encLic == NULL ) {
        fprintf(stderr, "Encryption error\n");
        return 1;
    }

    int i = 0;
    int slen = 0;
    char hkey[BUFLEN];
    memset(hkey, 0, BUFLEN);
    unsigned char *tl = encLic;
    for (i=0;i<len;i++) {
        snprintf(hkey+slen, BUFLEN - slen,  "%02x", tl[i]);
        slen += 2;
    }
    hkey[slen] = 0;

    fprintf(stdout, "Access Key: %s\n", hkey);

    int l = (sizeof(hkey) - 1)/2;
    unsigned char *keyint = (unsigned char *)alloca(l+1);
    const char *p;
    unsigned char *ch;

    for ( p = hkey, ch = keyint; *p; p+=2,++ch ) {
        *ch = hex2int(p[0])*16 + hex2int(p[1]);
    }
    keyint[l] = 0;

    char *decLic = (char *)decryptKey(keyint, len);

    if ( !decLic || strncmp(licStr, decLic, strlen(licStr)) != 0 ) {
        fprintf(stderr, "Validation of access key failed\n");
        // fprintf(stderr, "licStr = %s\n", licStr);
        // fprintf(stderr, "decLic = %s\n", decLic);
        rc = 1;
    } else {
    	rc = verifyKey(hkey, serialNumber);
    }

    if ( rc != 0 )
    	fprintf(stderr, "\nERROR: Failed to verify key\n\n");

    ism_common_free(ism_memory_utils_misc,encLic);
    if ( decLic) ism_common_free(ism_memory_utils_misc,decLic);
    ism_common_free(ism_memory_utils_misc, serialNumber );
    ism_common_free(ism_memory_utils_misc, valTimeStr );

    fprintf(stdout, "NB: If this key is for INTERNAL use and is being used to debug/diagnose an issue a customer might hit\n");
    fprintf(stdout, "    please consider how we could improve must-gather (or other serviceability features) to obtain the\n");
    fprintf(stdout, "    information instead. How would we proceed if we were debugging a customer problem where they could/\n");
    fprintf(stdout, "    would not type arbitrary shell commands provided by service into their production machine?\n\n");
    fprintf(stdout, "NB: If this key is for EXTERNAL use, it should only ever be generated by a L3 Service Engineer. Each time\n");
    fprintf(stdout, "    a key is needed, a review will determine whether future serviceability improvements are needed.\n");

    return rc;
}


