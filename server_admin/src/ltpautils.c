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

/* Some of the functions are taken from DataPower and IBM Security code,
 * and modified for ISM
 */
#ifndef TRACE_COMP
#define TRACE_COMP Security
#endif

#include <security.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/param_build.h>

/* LTPA Key structure with parsed contents from LTPA key file */
struct ismLTPA_t {
    void          *des_key;
    size_t        des_key_len;
    EVP_PKEY      *pkey;
    //RSA           *rsa;
    unsigned char *rsaMod;
    size_t        rsaModLen;
    char          *realm;
    char          *version;
};

#define TOKEN_NAME        "LtpaToken"
#define LTPAV2_TOKEN_NAME "LtpaToken2"

// Keyfile max size and token max size
xUNUSED static const int LTPA_MAXTOKENLEN   = 2048;

// Names of fields in keyfile
static const char *LTPA_KEYFILE_PUBKEY  = "com.ibm.websphere.ltpa.PublicKey";
static const char *LTPA_KEYFILE_PRIVKEY = "com.ibm.websphere.ltpa.PrivateKey";
static const char *LTPA_KEYFILE_3DES    = "com.ibm.websphere.ltpa.3DESKey";
static const char *LTPA_KEYFILE_REALM   = "com.ibm.websphere.ltpa.Realm";
static const char *LTPA_KEYFILE_VERSION = "com.ibm.websphere.ltpa.version";
static const char *LTPA_SUPPORTED_VERSION = "1.0";

static int sslModuleLoaded = 0;

// Keyfile characters
xUNUSED static const char KEYFILE_DELIM   = '=';
xUNUSED static const char KEYFILE_COMMENT = '#';
xUNUSED static const char KEYFILE_ESCAPE  = '\\';
xUNUSED static const char KEYFILE_EOL     = '\n';
xUNUSED static const char KEYFILE_CR      = '\r';

#define TOKEN_EXPIRY_PAD "000"
#define TOKEN_EXPIRY_PAD_LEN (sizeof(TOKEN_EXPIRY_PAD)-1)

#define USER_PREFIX     "user:"
#define USER_PREFIX_LEN (sizeof(USER_PREFIX)-1)
#define EXPIRE_KEY      "expire"
#define USER_KEY        "u"

xUNUSED static const char  TOKEN_REALM_DELIM_CH = '/';
xUNUSED static const char  TOKEN_DELIM_CH       = '%';

// DES constants
xUNUSED static const int NUMBER_OF_KEYS = 3;
xUNUSED static const int DESKEY_LEN = 24;
static const int DESBLOCK_SIZE = 8;

// RSA key lengths and offsets
static const int MODLENINBITS = 1024;

static const int LTPA_DLEN    = 128;
static const int LTPA_NLEN    = 128;
static const int LTPA_PLEN    = 64;
static const int LTPA_QLEN    = 64;
static const int LTPA_LENLEN  = 4;

static const int LTPA_SHA1_DIGEST_SIZE = 20;
static const int LTPA_AES_IV_LEN = 16;

// Max keyfile value/entry buffer size
static const int LTPA_MAXPROPBUFFER = 512;

// The crypto cipher used to encode the RSA private key
#define LTPA_CIPHER   "DES-EDE3"
#define LTPA2_CIPHER  "AES-128-CBC"

// The message digest algorithm used by LTPA
#define LTPA_DIGEST   "SHA1"

static const EVP_CIPHER * cipherV1 = NULL;
static const EVP_CIPHER * cipherV2 = NULL;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_free EVP_MD_CTX_cleanup
#define EVP_CIPHER_CTX_free EVP_CIPHER_CTX_cleanup
#endif


/* Get value of a key from a file containing key=value pairs */
static char *ism_security_ltpaGetKey(FILE *f, const char *keyname, size_t * keylen_out) {
    size_t len = 0;
    int    rc;
    char * line = NULL;
    char * retval = NULL;
    
    rc = getline(&line, &len, f);
    int keylen = strlen(keyname);
    while (rc >= 0) {
        if ( *line != '#' ) {
            if ( !strncmp(line, keyname, keylen) ) {
                int ln = strlen(line) - keylen - 1;
                char *start = line + keylen + 1;
                int outlen=0, i=0, j=0;
                for (i=0; i<ln; i++) {
                    if ( *start != '\\' && *start != '\n' )
                        outlen += 1;
                    start++;
                }
                if ( outlen > 0 ) {
                    retval = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,135),1, outlen+1);
                    if (keylen_out!=NULL) *keylen_out = outlen;
                    start = line + keylen + 1;
                    for (i=0; i<ln; i++) {
                        if ( *start != '\\' && *start != '\n' )
                            retval[j++] = *start;
                        start++;
                    }
                    break;
                }
            }
        }
        rc = getline(&line, &len, f);
    }

    fseek(f, 0L, SEEK_SET);
    if (line) ism_common_free_raw(ism_memory_admin_misc,line);

    return retval;
}

/* Replaces LTPA meta-characters with their quoted equivalents.
 * Returns dynamically allocated memory. Caller should free memory.
 */
//
static void ism_security_ltpaQuoteString(
    const char *in,
    char       **out)
{
    size_t len;
    char *p;

    len = (2*strlen(in))+1;
    *out = (char*)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,137),len);
    p = *out;

    while (*in) {
        switch(*in) {
        // WAS does not decode the backslashes so by escaping the slash it breaks LDAP
        // case '\\':
        case '$':
        case ':':
        case '%':
            *p++ = '\\';
            *p++ = *in++;
            break;
        default:
           *p++ = *in++;
            break;
        }
    }
    *p = '\0';
    return;
}

/* Create RSA keys form public and private keys in LTPA key file */
static int ism_security_ltpaConvertRSAKeys(
    char          *pubKey,
    size_t        pubKeyLen,
    char          *privKey,
    size_t        privKeyLen,
    //RSA           **rsa,
    EVP_PKEY      *pkey,
    unsigned char **rsaMod,
    size_t        *rsaModLen)
{
    int rc = ISMRC_Error;
    unsigned char dLenBuf[LTPA_LENLEN];
    size_t dLen = 0;

    TRACE(7, "Create RSA keys from LTPA Key file data\n");

    // copy out the private exponent length
    memcpy(dLenBuf, privKey, LTPA_LENLEN);

    // get the length they claim
    int i;
    for (i = 0; i < LTPA_LENLEN; i++) {
        dLen = (dLen<<8) + dLenBuf[i];
    }

    // if it's not 129 or 128, then we most likely have a bad decrypt
    if ((dLen == LTPA_DLEN) || (dLen == LTPA_DLEN + 1))
    {
        *rsaMod = (unsigned char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,138),sizeof(unsigned char)*LTPA_NLEN);
        memcpy(*rsaMod, pubKey + 1, LTPA_NLEN); // Copy n
        *rsaModLen = LTPA_NLEN;                 // Load n's length

        // get the number of pad bytes in the private key (0 or 1)
        size_t padByte = dLen - LTPA_DLEN;

        // Allocate a new RSA structure to hold our key
       /*  *rsa = RSA_new();
        if (*rsa == NULL) {
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        } */

        // Load the modulus
        BIGNUM * n = BN_bin2bn((const unsigned char *) (pubKey + 1), LTPA_NLEN, NULL);
        if (n == NULL) {
            TRACE(7, "BN_bin2bn failed for rsa->n\n");
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

        // Load the private exponent
        BIGNUM * d = BN_bin2bn((const unsigned char *) (privKey + LTPA_LENLEN + padByte), LTPA_DLEN, NULL);
        if (d == NULL) {
            TRACE(7, "BN_bin2bn failed for rsa->d\n");
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

        // The length of the public exponent (e) is however many bytes are
        // remaining in the public key buffer.
        size_t pubExpLen = pubKeyLen - (1 + LTPA_NLEN);

        // Load the public exponent
        BIGNUM * e = BN_bin2bn((unsigned char *) (pubKey + 1 + LTPA_NLEN), (int)pubExpLen, NULL);
        if (e == NULL) {
            TRACE(7, "BN_bin2bn failed for rsa->e\n");
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

        // Load the prime p
        BIGNUM * p = BN_bin2bn((const unsigned char *) (privKey + LTPA_LENLEN + padByte +
            LTPA_DLEN + pubExpLen + 1), LTPA_PLEN, NULL);
        if (p == NULL) {
            TRACE(7, "BN_bin2bn failed for rsa->p\n");
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

        if ((LTPA_LENLEN + padByte + LTPA_DLEN + pubExpLen + 1 + LTPA_PLEN + 1 + LTPA_QLEN) != privKeyLen) {
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

        // Load the prime q
        BIGNUM * q = BN_bin2bn((const unsigned char *)(privKey + LTPA_LENLEN + padByte +
             LTPA_DLEN + pubExpLen + 1 + LTPA_PLEN + 1), LTPA_QLEN, NULL);
        if (q == NULL) {
            TRACE(7, "BN_bin2bn failed for rsa->q\n");
            //(void) RSA_free(*rsa);
            //*rsa = NULL;
            ism_common_free(ism_memory_admin_misc,*rsaMod);
            *rsaMod = NULL;
            *rsaModLen = 0;
            return rc;
        }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
        (*rsa)->n = n;
        (*rsa)->d = d;
        (*rsa)->e = e;
        (*rsa)->p = p;
        (*rsa)->q = q;
#elif OPENSSL_VERSION_NUMBER > 0x30000000L
        OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
        OSSL_PARAM *params = NULL;
        if (bld == NULL
            || !OSSL_PARAM_BLD_push_BN(bld, "n", n)
            || !OSSL_PARAM_BLD_push_BN(bld, "e", e)
            || !OSSL_PARAM_BLD_push_BN(bld, "d", d)
            || !OSSL_PARAM_BLD_push_BN(bld, "rsa-factor1", p)
            || !OSSL_PARAM_BLD_push_BN(bld, "rsa-factor2", q)
            || (params = OSSL_PARAM_BLD_to_param(bld)) == NULL)
            {
                TRACE(7, "OSSL_PARAM_BLD or OSSL_PARAM_BLD_push_BN failed\n");
                OSSL_PARAM_BLD_free(bld);
            }

        EVP_PKEY_CTX *ctx = NULL;
        ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

        if (ctx == NULL
            || EVP_PKEY_fromdata_init(ctx) <= 0
            || EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_KEY_PARAMETERS, params) <= 0)
            {
                EVP_PKEY_CTX_free(ctx);
                OSSL_PARAM_BLD_free(bld);
                OSSL_PARAM_free(params);
                return rc;
            }
#else
        RSA_set0_key((*rsa), n, e, d);
        RSA_set0_factors((*rsa), p, q);
#endif

        rc = ISMRC_OK;
    }
    else
    {
        rc = ISMRC_Error;
    }

    return rc;
}

/* Version of strchr for LTPA quoted meta characters */
static char *ism_security_ltpaQuotedStrchr(
    char    *buf,
    char    c)
{
    char *p;

    for (p = buf; *p; p++) {
        if ('\\' == *p) {
            // Skip Quoted char
            if (*(p+1)) {
                p++;
            }
        } else if (c == *p) {
            return p;
        }
    }
    return NULL;
}

static int ism_security_ltpaParseQuotedString(
    char **s,
    char *out)
{
    char *p = *s;
    int end = 0;
    int complete = 0;

    while (!end) {
        switch(*p) {
        case '\\':
            // Quoted character, skip to next
            p++;
            if (*p) {
                *out = *p;
                out++;
            }
            break;
        case ':':
        case '$':
            // End of this string
            end = 1;
            break;
        case '\0':
        case '%':
            // End of user data
            end = 1;
            complete = 1;
            break;
        default:
            // Normal character.
            *out = *p;
            out++;
            break;
        }
        if (*p) {
            p++;
        }
    }


    *s = p;
    return complete;
}


/* PAD ISO9796
 * Ported from WS Java code
 */
static int ism_security_padISO9796(
    unsigned char *data,
    int   off,
    int   len,
    int   sigbits,
    unsigned char *pOutput,
    int   dwMaxOutputLen,
    size_t *dwOutputSize)
{
    int    error     = ISMRC_OK;
    int    n         = 0;
    int    padLength = 0;
    int    i         = 0;
    static unsigned char perm [16] = {0xE, 0x3, 0x5, 0x8, 0x9, 0x4, 0x2, 0xF,
        0x0, 0xD, 0xB, 0x6, 0x7, 0xA, 0xC, 0x1};
    sigbits--;

    if (len*16 > sigbits+3)
        return ISMRC_Error;

    padLength = (sigbits+7)/8;
    if (dwMaxOutputLen < padLength)
        return ISMRC_Error;

    for ( i=0; i<padLength/2; i++ )
        pOutput[padLength-1-2*i] = data[off+len-1-i%len];

    if ( (padLength&1) !=0 )
        pOutput[0] = data[off+len-1-(padLength/2)%len];

    for ( i=0; i<padLength/2; i++ )
    {
        int  j         = padLength-1-2*i;
        pOutput[j-1] = (unsigned char) ((perm[(pOutput[j]>>4)&0xF]<<4)
                           | perm[pOutput[j]&0xF]
                           );
    }

    pOutput[padLength-2*len] ^= 1;

    n = sigbits % 8;

    pOutput[0] &= (unsigned char)((1<<n)-1);
    pOutput[0] |= 1<<(n-1+8)%8;
    pOutput[padLength-1] = (unsigned char)(pOutput[padLength-1]<<4 | 0x06);

    return error;
}

/* This function is needed for WAS. Expects (2) 128 byte quantities. */
static void ism_security_complementSmodN(
    unsigned char *s, 
    unsigned char *n) 
{
    int i = 0;
    int borrow = 0;
    int diff = 0;

    for (i = 127; i >=0 ; i--)
    {
        diff = n[i] - s[i] - borrow;
        if (diff < 0)
        {
            diff += 256;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }
        s[i] = diff;
    }
}


/* Generate User Info signature for LTPA V1 token
 * V1 uses ISO 9796 padding for the token
 */
static int ism_security_ltpaV1GenUserInfoSignature(
    ismLTPA_t   *keys,
    char        *userInfoBuf,
    char        **sigBuf,
    size_t      *sigBufLen)
{
    int retVal = ISMRC_Error;
    
    const EVP_MD     *md;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX       mdCtx;
    EVP_MD_CTX *     pCtx = &mdCtx;
#else
    EVP_MD_CTX *     pCtx = NULL;
#endif
    int              rc;
    unsigned int     shaLen;
    size_t           signedLen;

    // Walk to the first delimiter to get signed data length
    // We just want the user data -- not the expiration
    char *delim = ism_security_ltpaQuotedStrchr(userInfoBuf, '%');
    if (delim) 
    {
        signedLen = delim - userInfoBuf;
    } 
    else
    {
        signedLen = strlen(userInfoBuf);
    }

    unsigned char digest[LTPA_SHA1_DIGEST_SIZE];

    if ( sslModuleLoaded == 0 ) {
        OpenSSL_add_all_digests();
        OpenSSL_add_all_algorithms();
        sslModuleLoaded = 1;
    }

    // Obtain a handle to the digest
    md = EVP_get_digestbyname(LTPA_DIGEST);
    if (md == (EVP_MD *) NULL) {
        TRACE(7, "EVP_get_digestbyname error\n");
        return retVal;
    }

    // Create a new digest context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_init(pCtx);
#else
    pCtx = EVP_MD_CTX_new();
#endif

    // Load the SHA1 digest into our context
    rc = EVP_DigestInit_ex(pCtx, md, NULL);
    if (rc != 1) {
        TRACE(7, "EVP_DigestInit error: %d\n", rc);
        EVP_MD_CTX_free(pCtx);
        return retVal;
    }

    // Digest the data
    rc = EVP_DigestUpdate(pCtx, (const void *) userInfoBuf,
            (unsigned int)signedLen);
    if (rc != 1) {
        TRACE(7, "EVP_DigestUpdate error: %d\n", rc);
        EVP_MD_CTX_free(pCtx);
        return retVal;
    }

    // Load the digest into our output buffer
    rc = EVP_DigestFinal(pCtx, digest, &shaLen);
    if (rc != 1) {
        TRACE(7, "EVP_DigestFinal error: %d\n", rc);
        EVP_MD_CTX_free(pCtx);
        return retVal;
    }

    EVP_MD_CTX_free(pCtx);

    //
    // Do the nasty ISO 9978 padding, padding the
    // hash of the input data into the scratch buffer
    //
    size_t modSize = MODLENINBITS;
    size_t scratchBufLen = (modSize + 7)/8;
    unsigned char *scratchBuf =
        (unsigned char *) alloca(sizeof(unsigned char) * scratchBufLen);

    retVal = ism_security_padISO9796(digest, 0, LTPA_SHA1_DIGEST_SIZE,
                 (int)modSize,
                 scratchBuf,
                 (int)scratchBufLen,
                 NULL);

    // Encrypt the buffer
    if (retVal == ISMRC_OK)
    {
        unsigned char *encBuf;
        size_t encBuflen;

        EVP_PKEY_CTX *ctx;
        ctx = EVP_PKEY_CTX_new(keys->pkey, NULL);
        if (!ctx
            || EVP_PKEY_sign_init(ctx) <= 0
            || EVP_PKEY_encrypt(ctx, NULL, &encBuflen, scratchBuf, scratchBufLen) <= 0)
        {
            EVP_PKEY_CTX_free(ctx);
        }

        encBuf = OPENSSL_malloc(encBuflen);
        if (!encBuf
            || EVP_PKEY_encrypt(ctx, encBuf, &encBuflen, scratchBuf, scratchBufLen) <= 0)
        {
            EVP_PKEY_CTX_free(ctx);
        }        

        //ism_common_free(ism_memory_admin_misc,scratchBuf);
        scratchBuf = encBuf;

        int remainder = 0;
        int tmp = 0;
        int i = 0;
        unsigned char *mod = keys->rsaMod;

        for (i = 0; i <= (LTPA_NLEN - 1); i++)
        {
            tmp = (remainder * 256) + mod[i];
            remainder = tmp%2;
            tmp = tmp/2;
            if (scratchBuf[i] != tmp)
                break;
        }
        if (i < scratchBufLen && scratchBuf[i] > tmp)
            ism_security_complementSmodN(scratchBuf, mod);

        // Base64 encode the result
        char b64SigBuf[1024];
        char * b64Sig = (char *)&b64SigBuf;
        int encodeSize = ism_common_toBase64((char *) scratchBuf,b64Sig, scratchBufLen);

        if (encodeSize>0)
        {
            *sigBuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),b64Sig);
            *sigBufLen = encodeSize;
        }
        else
        {
            retVal = ISMRC_Error;
        }
    }

    // Free the intermediate buffer
    //ism_common_free(ism_memory_admin_misc,scratchBuf);

    return retVal;

} // end of ltpaGenUserInfoSignature()



/* Verify V1 token signature
 * Verifies the RSA/SHA1 signature of the user data
 */
static int ism_security_ltpaV1VerifyTokenSignature(
    ismLTPA_t *keys,
    char      *userInfoBuf,
    char      *sigBuf,
    size_t    sigBufLen)
{
    int rc = ISMRC_OK;

    if (!sigBuf)
        return ISMRC_LTPADecodeError;

    /* Create a new signature for the buffer. */
    char*  newSigBuf = NULL;
    size_t newSigBufLen;
    rc = ism_security_ltpaV1GenUserInfoSignature(keys, userInfoBuf, &newSigBuf, &newSigBufLen);

    if ( rc != ISMRC_OK  || !newSigBuf ) {
        rc = ISMRC_LTPADecodeError;
        goto CLEANUP;
    }

    /* Compare signature with key file signature */
    sigBufLen -= sigBuf[sigBufLen - 1];
    if ((sigBufLen != newSigBufLen) || (memcmp((void*)sigBuf, (void*)newSigBuf, sigBufLen) != 0)) {
        TRACE(9, "TOKEN_SIG: %s\n", sigBuf);
        TRACE(9, "GENER_SIG: %s\n", newSigBuf);
        rc = ISMRC_LTPASigVerifyError;
    }

CLEANUP:
    if ( newSigBuf ) ism_common_free(ism_memory_admin_misc,newSigBuf);

    return rc;
}


/* Generate User Info signature for LTPA V2 token
 * V2 uses PKCS5 padding for the token
 */
static int ism_security_ltpaV2GenUserInfoSignature(
    ismLTPA_t *keys,
    char      *userInfoBuf,
    char      **sigBuf)
{
    int              rc = ISMRC_LTPASigGenError;
    const EVP_MD     *md = NULL;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX       mdCtx;
    EVP_MD_CTX *     pCtx = &mdCtx;
    EVP_MD_CTX       signCtx;
    EVP_MD_CTX *     pSignCtx = &signCtx;
#else
    EVP_MD_CTX *     pCtx;
    EVP_MD_CTX *     pSignCtx;
#endif
    int              sslrc = 0;
    unsigned int     shaLen = 0;
    size_t           signedLen = 0;
    unsigned char    digest[LTPA_SHA1_DIGEST_SIZE];
    unsigned char    *binsig = NULL;
    unsigned int     binsigLen = 0;
    EVP_PKEY         *pkey = NULL;

    *sigBuf = NULL;

    // Walk to the first delimiter to get signed data length
    // We just want the user data -- not the expiration
    char *delim = ism_security_ltpaQuotedStrchr(userInfoBuf, '%');
    if (delim) {
        signedLen = delim - userInfoBuf;
    } else {
        signedLen = strlen(userInfoBuf);
    }

    TRACE(9, "LTPA create signature for: %d.%s\n", (int)signedLen, userInfoBuf);

    // Obtain a handle to the digest (sha-1)
    md = EVP_get_digestbyname(LTPA_DIGEST);
    if (md == (EVP_MD *) NULL) {
        TRACE(7, "EVP_get_digestbyname\n");
        return rc;
    }

    if ( EVP_MD_size(md) != LTPA_SHA1_DIGEST_SIZE ) {
        TRACE(7, "EVP_MD_size error\n");
        return rc;
    }

    // Initialize the new digest context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_init(pCtx);
#else
    pCtx = EVP_MD_CTX_new();
#endif

    // Load the SHA1 digest into our context
    sslrc = EVP_DigestInit(pCtx, md);
    if (sslrc != 1) {
        TRACE(7, "EVP_DigestInit error: %d\n", sslrc);
        EVP_MD_CTX_free(pCtx);
        goto CLEANUP;
    }

    // Digest the data
    sslrc = EVP_DigestUpdate(pCtx, (const void *) userInfoBuf, (unsigned int)signedLen);
    if (sslrc != 1) {
        TRACE(7, "EVP_DigestUpdate error: %d\n", sslrc);
        EVP_MD_CTX_free(pCtx);
        goto CLEANUP;
    }

    // Load the digest into our output buffer
    sslrc = EVP_DigestFinal(pCtx, digest, &shaLen);
    if (sslrc != 1) {
        TRACE(7, "EVP_DigestFinal error: %d\n", sslrc);
        EVP_MD_CTX_free(pCtx);
        goto CLEANUP;
    }

    // We now have the sha-1 hash in the digest buffer.
    // We sign the hash using our RSA private key.
    pkey = EVP_PKEY_new();
    if (NULL == pkey) {
        TRACE(7, "EVP_PKEY_new\n");
        goto CLEANUP;
    }
    // Load our private key into the pkey structure
    keys->pkey = pkey;
    /* sslrc = EVP_PKEY_set1_RSA(pkey, keys->rsa);
    if (1 != sslrc) {
        TRACE(7, "EVP_PKEY_set1_RSA error: %d\n", sslrc);
        EVP_MD_CTX_free(pCtx);
        goto CLEANUP;
    } */

    EVP_MD_CTX_free(pCtx);

    // Create the signature context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_init(pSignCtx);
#else
    pSignCtx = EVP_MD_CTX_new();
#endif
    sslrc = EVP_SignInit(pSignCtx, md);
    if (1 != sslrc) {
        TRACE(7, "EVP_SignInit error: %d\n", sslrc);
        EVP_MD_CTX_free(pSignCtx);
        goto CLEANUP;
    }
    // Sign the data
    sslrc = EVP_SignUpdate(pSignCtx, digest, shaLen);
    if (1 != sslrc) {
        TRACE(7, "EVP_SignUpdate error: %d\n", sslrc);
        EVP_MD_CTX_free(pSignCtx);
        goto CLEANUP;
    }

    // Get signature size
    binsigLen = EVP_PKEY_size(pkey);
    binsig = (unsigned char*)alloca(binsigLen);

    // TRACE(9, "binsigLen = %d\n", binsigLen);

    // Load the signature into our buffer
    sslrc = EVP_SignFinal(pSignCtx, binsig, &binsigLen, pkey);
    if (1 != sslrc) {
        TRACE(7, "EVP_SignFinal error: %d\n", sslrc);
        EVP_MD_CTX_free(pSignCtx);
        goto CLEANUP;
    }

    EVP_MD_CTX_free(pSignCtx);

    // Base64 encode the signature
    char signEncodeBuf[1024];
    char * signEncode = (char *)&signEncodeBuf;
    int signEncodeSize= ism_common_toBase64((char *)binsig, signEncode, binsigLen);
    
    if(signEncodeSize>0){
         rc = ISMRC_OK;
         *sigBuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),signEncode);
    }

CLEANUP:
    //if (NULL != binsig) {
    //    ism_common_free(ism_memory_admin_misc,binsig);
    //}
    if (NULL != pkey) {
        EVP_PKEY_free(pkey);
    }
    return rc;
}

/* Verify V2 token signature */
static int ism_security_ltpaV2VerifyTokenSignature(
    ismLTPA_t *keys,
    char      *userInfoBuf,
    char      *sigBuf )
{
    int rc = ISMRC_OK;

    if (!sigBuf)
        return ISMRC_LTPADecodeError;

    /* Create a new signature for the buffer. */
    char*  newSigBuf = NULL;
    rc = ism_security_ltpaV2GenUserInfoSignature(keys, userInfoBuf, &newSigBuf);
    if ( rc != ISMRC_OK  || !newSigBuf ) {
        return ISMRC_LTPADecodeError;
        goto CLEANUP;
    }

    if (memcmp(sigBuf, newSigBuf, strlen(newSigBuf))) {
        TRACE(9, "TOKEN_SIG: %s\n", sigBuf);
        TRACE(9, "GENER_SIG: %s\n", newSigBuf);
        // rc = ISMRC_LTPASigVerifyError;
    }

CLEANUP:
    if (NULL != newSigBuf) {
        ism_common_free(ism_memory_admin_misc,newSigBuf);
    }
    return rc;
}

static int ism_security_ltpaParseUserInfoAndExpiration(ismLTPA_t *keys,
    char        *userAndExpBuf,
    char        **username,
    char        **realm,
    long        *expirySecs)
{
    int retVal = ISMRC_OK;
    const char *start = NULL;
    char *p;
    char *copy = NULL;

    if (username) {
        *username = NULL;
    }
    if (realm) {
        *realm = NULL;
    }

    // Parse the user data into key/value pairs.
    char key[1024];
    char val[2056];
    if (expirySecs) {
        *expirySecs = 0;
    }
    char *s = userAndExpBuf;
    int complete;

    while(1) {
        memset(key, 0, 1024);
        memset(val, 0, 2056);
        complete = ism_security_ltpaParseQuotedString(&s, key);
        if (complete) {
            // Unexpected end of user data, we'll ignore the
            // problem though.
            break;
        }
        complete = ism_security_ltpaParseQuotedString(&s, val);
        // TRACE(9, "LTPA Token Key=Value pair: %s=%s\n", key, val);

        if ( !strcmp(key, EXPIRE_KEY )) {
            *expirySecs = atol(val)/1000;        /* BEAM suppression: dereferencing NULL */
        }

        if ( !strcmp(key, USER_KEY)) {
            start = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),val);
        }

        if (complete) {
            break;
        }
    }

    if ( !start ) {
        TRACE(7, "LTPA Token User key is NULL\n");
        retVal = ISMRC_LTPADecodeError;
    }

    // Separate the user portion to realm and user DN.
    if (ISMRC_OK == retVal) {
        // start looks like this:
        //   user:custeng1.wma.ibm.com:389/cn=dodgy, o=tivoli, c=au
        // Skip over the "user:"
        if (0 == strncmp(start, USER_PREFIX, USER_PREFIX_LEN)) {
            copy = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),start + USER_PREFIX_LEN);
            p = copy;
        } else {
            retVal = ISMRC_LTPADecodeError;
        }
    }


    if (ISMRC_OK == retVal) {
        TRACE(9, "LTPA UserID: %s\n", p);
        // p and realm looks like this:
        //   custeng1.wma.ibm.com:389/cn=dodgy, o=tivoli, c=au
        // Split it into realm and DN portions
        p = strchr(p, '/');
        if (NULL != p) {
            *p = '\0';
            p++;
            if (realm) {
                *realm = copy;
                copy = NULL;
            }
            if (username) {
                *username = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),p);
            }
        } else {
            retVal = ISMRC_LTPADecodeError;
        }
    }

    if (ISMRC_OK != retVal) {
        if (username && *username) {
            ism_common_free(ism_memory_admin_misc,*username);
            *username = NULL;
        }
        if (realm && *realm) {
            ism_common_free(ism_memory_admin_misc,*realm);
            *realm = NULL;
        }
    }

    if ( start) ism_common_free(ism_memory_admin_misc,(void *)start);
    if ( copy ) ism_common_free(ism_memory_admin_misc,copy);

    return retVal;
}


/* LTPA V2 Token decrypt function:
 * Base64 decodes and 3DES Decrypts the specified buffer
 * Returns the buffer 0-terminated
 */
static int ism_security_ltpaV1DecodeAndDecrypt(
    ismLTPA_t *keys,
    char      *plainText,
    size_t    plainTextLen,
    char      **decText,
    size_t    *decTextLen)
{
    const EVP_CIPHER *cipher;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX   cipherCtx;
    EVP_CIPHER_CTX * pCipherCtx = &cipherCtx;
#else
    EVP_CIPHER_CTX * pCipherCtx = NULL;
#endif
    int              sslrc;
    int              cryptLen;
    unsigned char    *decBufp;

    // Tokens can't be empty
    if (plainTextLen == 0) {
        return ISMRC_LTPADecodeError;
    }

    // Base64 decode it
    char*  decodeText;
    long decodeTextLen = 0;
    char decodeTextBuf[1024];
    decodeText=(char* )&decodeTextBuf;

    decodeTextLen = ism_common_fromBase64(plainText,  decodeText , plainTextLen);

    // Can't have a token that doesn't BASE64 decode
    if (decodeTextLen <= 0) {
        return ISMRC_LTPADecodeError;
    }
    
    decodeText[decodeTextLen]='\0';

    // Obtain a DES-EDE cipher handle
    if(cipherV1==(EVP_CIPHER *) NULL){
        cipherV1 = EVP_get_cipherbyname(LTPA_CIPHER);
        if (cipherV1 == (EVP_CIPHER *) NULL) {
            TRACE(7, "EVP_get_cipherbyname\n");
            return ISMRC_LTPADecodeError;
        }
    }
    cipher = cipherV1;
    
    // Initialize the context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_init(pCipherCtx);
#else
    pCipherCtx = EVP_CIPHER_CTX_new();
#endif

    // Load the cipher context with the cipher handle and the key
    sslrc = EVP_DecryptInit(pCipherCtx, cipher,
                (unsigned char *) keys->des_key, (unsigned char *) NULL);
    if (sslrc != 1) {
        TRACE(7, "EVP_DecryptInit error: %d\n", sslrc);
        EVP_CIPHER_CTX_free(pCipherCtx);
        return ISMRC_LTPADecodeError;
    }

    // Turn off padding as we do all of our padding ourselves
    sslrc = EVP_CIPHER_CTX_set_padding(pCipherCtx, 0);
    if (sslrc != 1) {
        TRACE(7, "EVP_CIPHER_CTX_set_padding error:%d\n", sslrc);
        EVP_CIPHER_CTX_free(pCipherCtx);
        return ISMRC_LTPADecodeError;
    }

    unsigned char* decryptText =
        (unsigned char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,157),sizeof(unsigned char)*(decodeTextLen + 1));

    // Decrypt the cipher text
    decBufp = decryptText;
    sslrc = EVP_DecryptUpdate(pCipherCtx, decBufp, &cryptLen,
                    (unsigned char *) decodeText, (int)decodeTextLen);
    if (sslrc != 1) {
        TRACE(7, "EVP_DecryptUpdate error: %d\n", sslrc);
        EVP_CIPHER_CTX_free(pCipherCtx);
        ism_common_free(ism_memory_admin_misc,decryptText);
        return ISMRC_LTPADecodeError;
    }

    // Finish the decryption by decrypting any partial remaining block
    decBufp += cryptLen;
    sslrc = EVP_DecryptFinal(pCipherCtx, decBufp, &cryptLen);
    if (sslrc != 1) {
        TRACE(7, "EVP_DecryptFinal error: %d\n", sslrc);
        EVP_CIPHER_CTX_free(pCipherCtx);
        ism_common_free(ism_memory_admin_misc,decryptText);
        return ISMRC_LTPADecodeError;
    }
    EVP_CIPHER_CTX_free(pCipherCtx);

    if ((decBufp - decryptText) + cryptLen != decodeTextLen) {
        TRACE(7, "Crypt length check failed\n");
        ism_common_free(ism_memory_admin_misc,decryptText);
        return ISMRC_LTPADecodeError;
    }


    /* Ensure decrypted buffer is NUL terminated */
    decryptText[decodeTextLen] = 0;

    /* Data is bad if it contains an embedded NUL */
    if (strlen((char*)decryptText) != decodeTextLen) {
        ism_common_free(ism_memory_admin_misc,decryptText);
        return ISMRC_LTPADecodeError;
    }

    *decTextLen = decodeTextLen;
    *decText    = (char*)decryptText;


    return ISMRC_OK;
}

/* LTPA V2 Token decrypt function:
 * Base64 decodes and AES-128-CBC Decrypts the specified buffer
 * Returns the buffer 0-terminated
 */
static int ism_security_ltpaV2DecodeAndDecrypt(
    ismLTPA_t *keys,
    char      *token,
    size_t    tokenLen,
    char      **decText)
{

    const EVP_CIPHER *cipher = NULL;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX   cipherCtx;
    EVP_CIPHER_CTX * pCipherCtx = &cipherCtx;
#else
    EVP_CIPHER_CTX * pCipherCtx = NULL;
#endif
    int              rc = 0;
    int              cryptLen = 0;
    char    *cryptBin = NULL;
    long             cryptBinLen = 0;
    int              retVal  = ISMRC_LTPADecodeError;
    unsigned char    iv[LTPA_AES_IV_LEN];
    unsigned char    *decryptBuf = NULL;
    size_t           decryptBufLen = 0;
    unsigned char    *decBufp = NULL;

    *decText = NULL;

    if (!token || !tokenLen) {
        return retVal;;
    }

    // Base64 decode it
    char cryptBinBuf[1024];
    cryptBin = (char*) &cryptBinBuf;
    cryptBinLen = ism_common_fromBase64(token, cryptBin, tokenLen);
    
    
    if ( cryptBinLen<=0 ) {
        return retVal;
    }
    
    /*Null terminated the crypted buffer.*/
    cryptBin[cryptBinLen]='\0';
    
    // Obtain an AES-128-CBC cipher handle
    if(cipherV2==(EVP_CIPHER *) NULL){
        cipherV2 = EVP_get_cipherbyname( LTPA2_CIPHER);
        if (NULL == cipherV2) {
            TRACE(7, "EVP_get_cipherbyname error\n");
            return retVal;
        }
    }
    cipher = cipherV2;
    
    // Initialize the context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_init(pCipherCtx);
#else
    pCipherCtx = EVP_CIPHER_CTX_new();
#endif

    // Load the cipher context with the cipher handle and the key
    memcpy(iv, keys->des_key, LTPA_AES_IV_LEN);
    rc = EVP_DecryptInit(pCipherCtx, cipher,
                (unsigned char *) keys->des_key, iv);
    if (1 != rc) {
        TRACE(7, "EVP_DecryptInit error: %d\n", rc);
        goto CLEANUP;
    }

    decryptBufLen = cryptBinLen + EVP_CIPHER_block_size(cipher) + 1;
    decryptBuf = (unsigned char*)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,162),decryptBufLen);

    // Decrypt the cipher text
    decBufp = decryptBuf;
    rc = EVP_DecryptUpdate(pCipherCtx, decBufp, &cryptLen,
                    (unsigned char *) cryptBin, (int)cryptBinLen);
    if (1 != rc) {
        TRACE(7, "EVP_DecryptUpdate error: %d\n", rc);
        goto CLEANUP;
    }
    decBufp += cryptLen;

    rc = EVP_DecryptFinal(pCipherCtx, decBufp, &cryptLen);
    if (1 != rc) {
        TRACE(7, "EVP_DecryptFinal error: %d\n", rc);
        goto CLEANUP;
    }

    // Whack on a zero-terminator
    decBufp += cryptLen;
    *decBufp = '\0';

    // Check for embedded NUL bytes
    if (strlen((char*)decryptBuf) != (decBufp-decryptBuf)) {
        goto CLEANUP;
    }

    // Return the string
    *decText = (char*)decryptBuf;
    decryptBuf = NULL;
    retVal = ISMRC_OK;

CLEANUP:
    if (NULL != decryptBuf) {
        ism_common_free(ism_memory_admin_misc,decryptBuf);
    }
    EVP_CIPHER_CTX_free(pCipherCtx);

    return retVal;
}

/* Decrypt LTPA key 
 * Decrypts a key using the a SHA1'ed password as the 3DES key
 */
static int ism_security_decryptKey(
    unsigned char *buffer,
    size_t        inBufferLen,
    char          *password,
    size_t        *retBufferLen)
{
    int              retVal = ISMRC_LTPADecryptError; 
    unsigned char    decBuffer[LTPA_MAXPROPBUFFER + DESBLOCK_SIZE];
    unsigned char    pwKeyBytes[DESKEY_LEN];
    unsigned char    *decBufp;
    
    const EVP_MD     *md;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX       mdCtx;
    EVP_MD_CTX *     pCtx = &mdCtx;
    EVP_CIPHER_CTX   cipherCtx;
    EVP_CIPHER_CTX * pCipherCtx = &cipherCtx;
#else
    EVP_MD_CTX *     pCtx;
    EVP_CIPHER_CTX * pCipherCtx = NULL;
#endif
    const EVP_CIPHER *cipher;
    int              sslrc;
    unsigned int     shaLen;
    int              cryptLen;
    int              decryptedLen = 0;

    // zero the return buffer and key buffer
    memset(pwKeyBytes, 0, DESKEY_LEN);

    // Make sure it base64 decoded okay
    if (inBufferLen%8 == 0)
    {
        // Obtain a handle to the digest
        md = EVP_get_digestbyname(LTPA_DIGEST);
        if (md == NULL) {
            TRACE(7, "EVP_get_digestbyname error.\n");
            return retVal;
        }

        // Create a new digest context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        EVP_MD_CTX_init(pCtx);
#else
        pCtx = EVP_MD_CTX_new();
#endif

        // Load the SHA1 digest into our context
        sslrc = EVP_DigestInit_ex(pCtx, md, NULL);
        if (sslrc != 1) {
            TRACE(7, "EVP_DigestInit error: %d\n", sslrc);
            EVP_MD_CTX_free(pCtx);
            return retVal;
        }

        // Digest the password text
        sslrc = EVP_DigestUpdate(pCtx, (const void *) password, (unsigned int)strlen(password));
        if (sslrc != 1) {
            TRACE(7, "EVP_DigestUpdate error: %d\n", sslrc);
            EVP_MD_CTX_free(pCtx);
            return retVal;
        }

        // Load the digest into our DES key
        sslrc = EVP_DigestFinal_ex(pCtx, pwKeyBytes, &shaLen);
        if (sslrc != 1) {
            TRACE(7, "EVP_DigestFinal error: %d\n", sslrc);
            EVP_MD_CTX_free(pCtx);
            return retVal;
        }

        EVP_MD_CTX_free(pCtx);

        // 3DES decrypt the data in unchained mode
        // Obtain a DES-EDE cipher handle
        cipher = EVP_get_cipherbyname(LTPA_CIPHER);
        if (cipher == (EVP_CIPHER *) NULL) {
            TRACE(7, "EVP_get_cipherbyname\n");
            return retVal;
        }

        // Initialize cipher context
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        EVP_CIPHER_CTX_init(pCipherCtx);
#else
        pCipherCtx = EVP_CIPHER_CTX_new();
#endif

        sslrc = EVP_DecryptInit(pCipherCtx, cipher, pwKeyBytes, NULL);
        if (sslrc != 1) {
            TRACE(7, "EVP_DecryptInit error: %d\n", sslrc);
            EVP_CIPHER_CTX_free(pCipherCtx);
            return retVal;
        }

        // Decrypt the cipher text
        decBufp = decBuffer;
        sslrc = EVP_DecryptUpdate(pCipherCtx, decBufp, &cryptLen, buffer, (int)inBufferLen);
        if (sslrc != 1) {
            TRACE(7, "EVP_DecryptUpdate error: %d\n", sslrc);
            EVP_CIPHER_CTX_free(pCipherCtx);
            return retVal;
        }

        // Finish the decryption by decrypting any partial remaining block
        decryptedLen += cryptLen;
        decBufp += cryptLen;
        sslrc = EVP_DecryptFinal(pCipherCtx, decBufp, &cryptLen);
        if (sslrc != 1) {
            TRACE(7, "EVP_DecryptFinal error: %d\n", sslrc);
            EVP_CIPHER_CTX_free(pCipherCtx);
            return retVal;
        }
        decryptedLen += cryptLen;
        EVP_CIPHER_CTX_free(pCipherCtx);

        /*// trim off the PKCS5 pad - doesn't /appear to be needed
        size_t dataLen = inBufferLen;
        int    padLen = decBuffer[dataLen - 1];

        if (padLen > 0 && padLen < 9)
        {
            dataLen -= decBuffer[dataLen - 1];

            // clear the buffer and copy the decrypted data into it
            memset(buffer, 0, dataLen);
            memcpy(buffer, decBuffer, dataLen);
            *retBufferLen = dataLen;

            retVal = ISMRC_OK;
        }*/
        memset(buffer, 0, LTPA_MAXPROPBUFFER);
        memcpy(buffer, decBuffer, decryptedLen);
        *retBufferLen = decryptedLen;

        retVal = ISMRC_OK;
    }

    return retVal;

}



/* Read and validate LTPA key file
 *
 * LTPA Keyfile format is as follows:
 *
 * #IBM WebSphere Application Server key file
 * #Mon Jul 15 12:32:45 CDT 2013
 * com.ibm.websphere.CreationDate=Mon Jul 15 12\:32\:45 CDT 2013
 * com.ibm.websphere.ltpa.version=1.0
 * com.ibm.websphere.ltpa.3DESKey=1uYH7AR6IZZu216RjzVX0CacyOb3N3ErFhm0iy3DzoY\=
 * com.ibm.websphere.CreationHost=TestSystemName
 * com.ibm.websphere.ltpa.PrivateKey=gGvAQHV7L63Ysx/ZwiwWdv62Wt2bgkfwW9TXpdQ.....
 * com.ibm.websphere.ltpa.Realm=TestSystemName
 * com.ibm.websphere.ltpa.PublicKey=ALrGjWDQa9XB37omkjigxqRB6zueAiEyaHj8pUow.....
 */
XAPI int ism_security_ltpaReadKeyfile(
    char *keyfile_path,
    char *keyfile_password,
    ismLTPA_t **ltpaKey )
{
    int           rc            = ISMRC_OK;
    
    FILE          *f            = NULL;
    
    char          *publicKey    = NULL;
    long          publicKeyLen  = 0;
    char          *privateKey   = NULL;
    size_t        privateKeyLen = 0;
    char          *desKey       = NULL;
    size_t        desKeyLen     = 0;
    
    char          *version      = NULL;
    char          *escRealm     = NULL;
    //RSA           *tmpRSA       = NULL;
    EVP_PKEY      *pkey          = NULL;
    unsigned char *tmpRSAMod    = NULL;
    size_t        tmpRSAModLen  = 0;
    char          *keybuf       = NULL;
    size_t        keylen        = 0;

    if ( !keyfile_path || !keyfile_password || *keyfile_path=='\0' ) {
        if(ltpaKey!=NULL) *ltpaKey=NULL;
        ism_common_setError(ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    TRACE(9, "Create LTPA key data structure from LTPA Key file: %s\n", keyfile_path);

    if ( sslModuleLoaded == 0 ) {
        OpenSSL_add_all_digests();
        OpenSSL_add_all_algorithms();
        sslModuleLoaded = 1;
    }

    /*Initialize Ciphers for LTPAv1 and LTPAv2 for later use.*/
    if(cipherV1==(EVP_CIPHER *) NULL){
        cipherV1 = EVP_get_cipherbyname(LTPA_CIPHER);
        if (NULL == cipherV1) {
            TRACE(1, "EVP_get_cipherbyname error\n");
            ism_common_setError(ISMRC_LTPASSLError);
            return ISMRC_LTPASSLError;
        }
    }

    if(cipherV2==(EVP_CIPHER *) NULL){
        cipherV2 = EVP_get_cipherbyname( LTPA2_CIPHER);
        if (NULL == cipherV2) {
            TRACE(1, "EVP_get_cipherbyname error\n");
            ism_common_setError(ISMRC_LTPASSLError);
            return ISMRC_LTPASSLError;
        }
    }

    f = fopen(keyfile_path, "r");
    if (!f) {
        ism_common_setError(ISMRC_NotFound);
        return ISMRC_NotFound;
    }

    /* Set public key */
    keybuf = ism_security_ltpaGetKey(f, LTPA_KEYFILE_PUBKEY, &keylen);
    if ( !keybuf ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    TRACE(9, "PublicKey: %d.%s\n", (int)keylen, keybuf);
    char publicKeyBuf[1024];
    publicKey = (char *)&publicKeyBuf;
    publicKeyLen = ism_common_fromBase64(keybuf, publicKey, keylen);
    
    if ( publicKeyLen<=0 ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_free(ism_memory_admin_misc,keybuf);
        ism_common_setError(rc);
        goto CLEANUP;
    }
    ism_common_free(ism_memory_admin_misc,keybuf);
    
    publicKey[publicKeyLen]='\0';
    

    /* Set private key */
    keylen = 0;
    keybuf = ism_security_ltpaGetKey(f, LTPA_KEYFILE_PRIVKEY, &keylen);
    if ( !keybuf ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    TRACE(9, "PrivateKey: %d.%s\n", (int)keylen, keybuf);
    long dcdKeyLen;
    char privateKeyBuf[1024];
    privateKey = (char *)&privateKeyBuf;
    dcdKeyLen = ism_common_fromBase64(keybuf, privateKey, keylen);
    
    if ( dcdKeyLen<=0 ) {
        ism_common_free(ism_memory_admin_misc,keybuf);
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    ism_common_free(ism_memory_admin_misc,keybuf);
    
    privateKey[dcdKeyLen]='\0';
    
    rc = ism_security_decryptKey((unsigned char *)privateKey, dcdKeyLen, keyfile_password, &privateKeyLen);
    if ( rc != ISMRC_OK ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }

    /* Set DES key */
    keylen = 0;
    keybuf = ism_security_ltpaGetKey(f, LTPA_KEYFILE_3DES, &keylen);
    if ( !keybuf ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    TRACE(9, "Des3Key: %d.%s\n", (int)keylen, keybuf);
    char desKeyBuf[1024];
    desKey = (char *)desKeyBuf;
    dcdKeyLen = ism_common_fromBase64(keybuf, desKey, keylen);
    
    if ( dcdKeyLen<=0 ) {
        ism_common_free(ism_memory_admin_misc,keybuf);
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    ism_common_free(ism_memory_admin_misc,keybuf);
    
    desKey[dcdKeyLen]='\0';
    
    rc = ism_security_decryptKey((unsigned char *)desKey, dcdKeyLen, keyfile_password, &desKeyLen);
    if ( rc != ISMRC_OK ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }

    /* Set RSA */
    rc = ism_security_ltpaConvertRSAKeys(publicKey, publicKeyLen, privateKey, privateKeyLen, pkey, &tmpRSAMod, &tmpRSAModLen);
    if ( rc != ISMRC_OK ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }

    /* Get version */
    keylen = 0;
    keybuf = ism_security_ltpaGetKey(f, LTPA_KEYFILE_VERSION, &keylen);
    if ( !keybuf ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    TRACE(9, "Version: %d.%s\n", (int)keylen, keybuf);
    if (strncmp(keybuf, LTPA_SUPPORTED_VERSION, strlen(LTPA_SUPPORTED_VERSION)) != 0) {
        ism_common_free(ism_memory_admin_misc,keybuf);
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    version = keybuf;

    /* get Realm */
    keylen = 0;
    keybuf = ism_security_ltpaGetKey(f, LTPA_KEYFILE_REALM, &keylen);
    if ( !keybuf ) {
        rc = ISMRC_LTPAInvalidKeyFile;
        ism_common_setError(rc);
        goto CLEANUP;
    }
    TRACE(9, "Realm: %d.%s\n", (int)keylen, keybuf);
    ism_security_ltpaQuoteString(keybuf, &escRealm);
    ism_common_free(ism_memory_admin_misc,keybuf);

    int ltpaKeySize = (int)sizeof(ismLTPA_t) + desKeyLen+1;
    *ltpaKey = (ismLTPA_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,172),1,ltpaKeySize);

    char *des_keyPtr = (char *)((*ltpaKey) + 1);
    memcpy(des_keyPtr, desKey, desKeyLen);
    des_keyPtr[desKeyLen]='\0';
    (*ltpaKey)->des_key =(void *) des_keyPtr;
    
    (*ltpaKey)->des_key_len = desKeyLen;
    //(*ltpaKey)->rsa = tmpRSA;
    (*ltpaKey)->pkey = pkey;
    (*ltpaKey)->rsaMod = tmpRSAMod;
    (*ltpaKey)->rsaModLen = tmpRSAModLen;
    (*ltpaKey)->version = version;
    (*ltpaKey)->realm = escRealm;
    
    // cleanup and return
CLEANUP:
    fclose(f);
  
    return rc;
}

/* Delete LTPA key */
XAPI int ism_security_ltpaDeleteKey(
    ismLTPA_t *key )
{
    int rc = ISMRC_OK;
    if ( !key )
        return rc;

    // Free key contents
    //if (key->des_key_len > 0 && key->des_key)
    //    ism_common_free(ism_memory_admin_misc,key->des_key);

    if (key->realm)
        ism_common_free(ism_memory_admin_misc,key->realm);

    /* if (key->rsa)
        RSA_free(key->rsa); */

    if (key->rsaModLen > 0 && key->rsaMod)
        ism_common_free(ism_memory_admin_misc,key->rsaMod);

    if (key->version)
        ism_common_free(ism_memory_admin_misc,key->version);

    ism_common_free(ism_memory_admin_misc,key);
    key=NULL;

    return rc;
}

/* Decode LTPA V1 token */
XAPI int ism_security_ltpaV1DecodeToken(
    ismLTPA_t *keys,
    char      *token_data,
    size_t    token_len,
    char      **user_name,
    char      **realm,
    time_t    *expiration )
{
    int retVal        = ISMRC_LTPADecodeError;
    char*  decText    = NULL;
    size_t decTextLen = 0;
    char* userInfo    = NULL;
    char* signature   = NULL;
    char* exptimestr  = NULL;

    TRACE(9, "ENTER ltpaV1DecodeToken\n");

    if ( !keys || !token_data )
        return ISMRC_NullArgument;

    // 3DES decrypt and base64 decode the lot.
    retVal = ism_security_ltpaV1DecodeAndDecrypt(keys, token_data, token_len, &decText, &decTextLen);
    if (ISMRC_OK != retVal) {
        TRACE(7, "Unable to decode or decrypt the token.\n");
        goto CLEANUP;
    }

    TRACE(9, "DecTEXT: %s\n", decText);

    // Break the buffer into the user info / signature component.  We know that
    // the signature occurs after the 2nd '%' character.
    userInfo  = decText;
    exptimestr = ism_security_ltpaQuotedStrchr(userInfo, '%');
    if (!exptimestr)
    {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to retrieve expiration time from the token.\n");
        goto CLEANUP;
    }
    signature = ism_security_ltpaQuotedStrchr(exptimestr + 1, '%');
    if (!signature) {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to retrieve signature from the token.\n");
        goto CLEANUP;
    }

    int expstrlen = signature - exptimestr - 1;
    char *exptm = alloca(expstrlen);
    memcpy(exptm, exptimestr+1, expstrlen);
    long exptime = atol(exptm);

    *signature = '\0';
    signature++;

    // TRACE(9, "UserInfo: %s\n", userInfo);
    // TRACE(9, "Expiration Time: %ld\n", exptime);
    // TRACE(9, "Signature: %s\n", signature);

    // size_t slen = decTextLen - (signature - decText);
    size_t slen = strlen(signature);
    // TRACE(9, "DecTextLen:%d   SIGLength: %d\n", decTextLen, slen);

    // Verify the signature.
    retVal = ism_security_ltpaV1VerifyTokenSignature(keys, userInfo,
                 signature, slen);
    if (ISMRC_OK != retVal) {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to verify the signature.\n");
        goto CLEANUP;
    }

    // Suck out the user data.
    retVal = ism_security_ltpaParseUserInfoAndExpiration(keys, userInfo, user_name, realm, (long*)expiration);
    if (ISMRC_OK != retVal) {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to retrieve user info and expiration from token.\n");
    }

    if ( *expiration == 0 ) {
        /* set expiration time from token */
        *expiration = exptime/1000;
        TRACE(9, "Token expiration time:%ld   Current Server Time:%ld\n", *expiration, time(NULL));
    }

CLEANUP:
    if (decText)
        ism_common_free(ism_memory_admin_misc,decText);
    
    TRACE(9, "EXIT ltpaV1DecodeToken rc=%d\n", retVal);

    return retVal;
}

/* Decode LTPA V2 token */
XAPI int ism_security_ltpaV2DecodeToken(
    ismLTPA_t *keys,
    char      *token_data,
    size_t    token_len,
    char      **user_name,
    char      **realm,
    time_t    *expiration)
{
    int retVal       = ISMRC_LTPADecodeError;
    char* decText    = NULL;
    char* userInfo   = NULL;
    char* signature  = NULL;
    char* exptimestr  = NULL;

    TRACE(9, "ENTER ltpaV2DecodeToken\n");

    if ( !keys || !token_data )
        return ISMRC_NullArgument;

    // base64 decode and AES-128-CBC decrypt the lot.
    retVal = ism_security_ltpaV2DecodeAndDecrypt(keys, token_data, token_len, &decText);
    if (ISMRC_OK != retVal) {
        TRACE(7, "Unable to decode or decrypt the token: %d.%s\n", (int)token_len, token_data);
        goto CLEANUP;
    }

    // Break the buffer into the user info / signature component.  We know that
    // the signature occurs after the 2nd '%' character.
    userInfo  = decText;
    exptimestr = ism_security_ltpaQuotedStrchr(userInfo, '%');
    if (!exptimestr)
    {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to retrieve expiration time from the token: %s\n", decText);
        goto CLEANUP;
    }

    signature = ism_security_ltpaQuotedStrchr(exptimestr + 1, '%');
    if (!signature) {
        retVal = ISMRC_Error;
        TRACE(7, "Unable to retrieve signature from the token: %s\n", decText);
        goto CLEANUP;
    }

    if ( *expiration == 0 ) {
        /* get expiration time from token */
        int expstrlen = signature - exptimestr - 1;
        char *tmpstr = alloca(expstrlen+1);
        memcpy(tmpstr, exptimestr+1, expstrlen);
        tmpstr[expstrlen] = 0;
        *expiration = (time_t)atol(tmpstr);
        TRACE(9, "Token expiration time:%ld   Current Server Time:%ld\n", *expiration, time(NULL));
    }

    *signature = '\0';
    signature++;

    // Verify the signature.
    retVal = ism_security_ltpaV2VerifyTokenSignature(keys, userInfo, signature);
    if ( retVal != ISMRC_OK ) {
        retVal = ISMRC_LTPASigVerifyError;
        TRACE(7, "Unable to verify the signature\n");
        goto CLEANUP;
    }

    // Suck out the user data.
    retVal = ism_security_ltpaParseUserInfoAndExpiration(keys, userInfo, user_name, realm, (long*)expiration);
    if ( retVal != ISMRC_OK ) {
        TRACE(7, "Unable to retrieve userid and expiration from token: %s\n", decText);
        goto CLEANUP;
    }

CLEANUP:
    if (decText)
        ism_common_free(ism_memory_admin_misc,decText);

    TRACE(9, "EXIT ltpaV2DecodeToken rc=%d\n", retVal);

    return retVal;
}

