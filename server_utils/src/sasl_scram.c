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

/**
 * The implemenation of SASL Scram based the RFC 5802
 * https://tools.ietf.org/html/rfc5802
 * The Hash function is based on the OpenSSL SHA256 AND SHA512
 * The HMAC function is based on the OpenSSL HMAC
 *
 * According to RFC 5802, there are 4 messages between client and server.
 *
 * 1. Client First Message
 *    Client will send the username and client nonce. Here is an example:
 *    n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
 *
 *    Where
 *    -First 3 characters ('n,,') represent the headers
 *    -Client Bare message: n=user,r=fyko+d2lbbFgONRv9qkxdawL
 *          n attribute is the username. Note: , and = will be encoded as '=2C' and
 *          '=3D' respectively
 *    r attribute is the client nonce
 *
 * 2. Server First Response
 *    The Response from the server. Here is an example of the response:
 *
 *    r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,
 *    i=4096
 *
 *    Where
 *     r attribute is the server nonce (which also include the client nonce on the first part)
 *     s attribute is the salt which encoded in base64
 *     i attribute is the iteration which need for the process of salting the password.
 *
 * 3. Client Final Message
 *      The client final message will contain the client proof. Here is an example:
 *      c=biws,r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,
 *     p=v0X8v3Bz2T0CJGbJQyF0X+HI4Ts=
 *
 *     Where
 *     c attribute is the bind input where biws is the base64 encoding of 'n,,'
 *     r attribute is the server nonce
 *     p attribute is the client proof which encode in base64
 *
 * 4. Server Final Response
 *      Server Final Response containes the Server Proof
 *      Here is an example:
 *      v=rmF9pqV8S7suAoZWja4dJRkFsKQ=
 *
 * To use this implementation, First you need to create a SASL Scram context by calling
 *      ism_sasl_scram_context_new(
 *      NOTE: You need to call ism_sasl_scram_context_destroy to destroy the context.
 *
 * The Server Response will be parsed and stored in the Properties in the context.
 *
 *
 */

#ifndef TRACE_COMP
#define TRACE_COMP Kafka
#endif

#include <ismutil.h>
#include <ismjson.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sasl_scram.h>

/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/**
 * Create new SASL SCRAM Context
 * Note: The caller must call ism_sasl_scram_context_destroy
 * to destroy the context
 *
 */
ism_sasl_scram_context *  ism_sasl_scram_context_new(ism_sasl_machanism_e in_mechanism){
    ism_sasl_scram_context * context = ism_common_calloc(ISM_MEM_PROBE(ism_memory_saslScramProfile,394),
                                                         1,
                                                         sizeof(ism_sasl_scram_context));

    context->state = SASL_SCRAM_STATE_NEW;
    context->server_response_props = ism_common_newProperties(10);
    context->mechanism = in_mechanism;

    //This switch needs to be kept in sync with the equivalent in getEVPMDfromMechanism()
    switch(context->mechanism ){
        case SASL_MECHANISM_SCRAM_SHA_256:
            context->scram_evp = EVP_sha256();
            context->scram_hash_f_digest_length = SHA256_DIGEST_LENGTH;
            context->scram_hash_f = SHA256;
            break;
        case SASL_MECHANISM_SCRAM_SHA_512:
        default:
           context->scram_evp = EVP_sha512();
           context->scram_hash_f_digest_length = SHA512_DIGEST_LENGTH;
           context->scram_hash_f = SHA512;
    }

    return context;
}


/**
 * Free resources and destroy the SASL SCRAM context
 */
int ism_sasl_scram_context_destroy(ism_sasl_scram_context * context){
    if(!context){
        return 1;
    }
    if(context->client_nonce){
        ism_common_free(ism_memory_saslScram,context->client_nonce);
    }
    if(context->client_first_msg_bare){
        ism_common_free(ism_memory_saslScram,context->client_first_msg_bare);
    }
    if(context->server_signature_b64){
        ism_common_free(ism_memory_saslScram,context->server_signature_b64);
    }
    if(context->server_first_msg){
        ism_common_free(ism_memory_saslScram,context->server_first_msg);
    }
    if(context->server_response_props){
        ism_common_freeProperties(context->server_response_props);
    }

    ism_common_free(ism_memory_saslScramProfile,context);

    return 0;
}

/*
 * Base62 is used for the random part of a generated clientID.
 */
static char g_base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

/*
 * Genenrate a UID
 * Caller will have to free returned uuid
 */
static char * sasl_scram_genUUID(void) {
    unsigned char randID[24];
    unsigned char uuid[24];
    int i=0;
    char *nuid=NULL;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randID, sizeof randID);
#else
    RAND_bytes(randID, sizeof randID);
#endif

    for (i=0; i<sizeof(randID); i++) {
        uuid[i] = g_base62[(int)(randID[i]%62)];
    }

    nuid = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_saslScram,391),25);
    memcpy(nuid, uuid, 24);
    nuid[24] = '\0';
    return nuid;
}

/**
 * Generate the nonce
 */
int sasl_scram_generate_nonce (ism_sasl_scram_context * context) {
    context->client_nonce = sasl_scram_genUUID();
    context->client_nonce_size = strlen( context->client_nonce);
    return 0;
}

/**
 * Build Client First Message to the Server
 *
 * The format of the message based on RFC 5802
 * Here is the example:
 *  n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
 *
 */
int  ism_sasl_scram_build_client_first_message (ism_sasl_scram_context * context, const char * username, concat_alloc_t *outbuf) {


    //1. Generate nonce. Need to store nonce for later checking.
    //2. Generate First Message with the following format
    //    "n,,n=<username>,r=%.*s"

    int rc = 0;
    if(!username){
        return 1;
    }
    rc = sasl_scram_generate_nonce(context);
    if(rc){
        return 1;
    }

    ism_common_allocBufferCopyLen(outbuf,"n,,n=", 5 );
    // According to RFC5802, The characters ',' or '=' in usernames are sent as '=2C' and
    //'=3D' respectively.
    char * username_tmp_ptr=NULL;
    for (username_tmp_ptr = (char *)username; *username_tmp_ptr; username_tmp_ptr++) {
        char username_char = *username_tmp_ptr;
        switch (username_char){
        case ',':
            ism_common_allocBufferCopyLen(outbuf,"=2C",3);
            break;
        case '=':
            ism_common_allocBufferCopyLen(outbuf,"=3D",3);
            break;
        default:
            bputchar(outbuf,username_char);
            break;
        }
    }
    ism_common_allocBufferCopyLen(outbuf, ",r=", 3);
    ism_common_allocBufferCopyLen(outbuf,context->client_nonce,context->client_nonce_size);
    bputchar(outbuf,0);
    outbuf->used--;

    //Save the Client First Message without the header when construct the Server First Message
    context->client_first_msg_bare_size = outbuf->used-3;
    context->client_first_msg_bare = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_saslScram,392),
                                                               context->client_first_msg_bare_size+1);

    memcpy(context->client_first_msg_bare, outbuf->buf+3, context->client_first_msg_bare_size);
    context->client_first_msg_bare[context->client_first_msg_bare_size]=0;

    return 0;
}


/**
 * Parse the Server Response and put the attribute and its value into properties object
 */
int ism_sasl_scram_parse_properties (char  * inbuf, ism_prop_t * props) {
    char * more= inbuf;
    char * token = ism_common_getToken(more, " ,", " ,", &more);
    ism_field_t f = {0};
    while (token) {
        char * key = ism_common_getToken(token, " =", " =", &token);

        if(token!=NULL){
            f.type = VT_String;
            f.val.s = (char *)token;
            ism_common_setProperty(props, key, &f);
        }
        token = ism_common_getToken(more, " ,", " ,", &more);
    }

    return 0;
}

/**
 * Salting the password
 *
 * NB: Don't call this directly. Callers should use ism_sasl_scram_salt_password 
 * (prefered but caller needs ism_sasl_scram_context) or 
 * ism_sasl_scram_mechanism_salt_password
 * 
 * RFC Spec on SaltedPassword
 *    SaltedPassword  := Hi(Normalize(password), salt, i)
 *
 *    Hi(str, salt, i):

     U1   := HMAC(str, salt + INT(1))
     U2   := HMAC(str, U1)
     ...
     Ui-1 := HMAC(str, Ui-2)
     Ui   := HMAC(str, Ui-1)

     Hi := U1 XOR U2 XOR ... XOR Ui
 *
 */
static int ism_sasl_scram_EVP_salt_password (const EVP_MD *evp, 
                        const char *password, int password_size,
                        const char *salt, int salt_size,
                        int iteration, concat_alloc_t * outbuf) {
    unsigned int  digest_size = 0;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned char tempbuf[EVP_MAX_MD_SIZE];
    unsigned char * key;
    int i, x;

    /*INT(g) is a 4-octet encoding of the integer g, most
      significant octet first.
     */
    key = alloca(salt_size + 4);
    memcpy( key, salt, salt_size);
    key[salt_size]   = 0;
    key[salt_size+1] = 0;
    key[salt_size+2] = 0;
    key[salt_size+3] = 1;


    /*Perform U1   := HMAC(str, salt + INT(1))*/
    if (!HMAC(evp,
              (const unsigned char *)password, (int)password_size,
               key, salt_size+4,
               digest, &digest_size)) {

            return 1;
    }

    memcpy(tempbuf, digest, digest_size);

    //Perform:
    //Ui-1 := HMAC(str, Ui-2)
    //Ui   := HMAC(str, Ui-1)
    for (i = 1 ; i < iteration ; i++) {
        if (UNLIKELY(!HMAC(evp,
                           (const unsigned char *)password, (int)password_size,
                           digest, digest_size,
                           digest, NULL))) {
                return 1;
        }

        //Perform:  Hi := U1 XOR U2 XOR ... XOR Ui
        for (x = 0 ; x < (int)digest_size ; x++) {
                tempbuf[x] ^= digest[x];
        }

    }

    ism_common_allocBufferCopyLen(outbuf,(char *)tempbuf, digest_size);
    bputchar(outbuf,0);
    outbuf->used--;

    return 0;
}

/**
 * Choose an Envelope API message digest appropriate for the scheme we're using
 */
static const EVP_MD *getEVPMDfromMechanism(ism_sasl_machanism_e in_mechanism) {
    const EVP_MD *evpmd = NULL;

    //This switch needs to be kept in sync with the equivalent in ism_sasl_scram_context_new()
    switch(in_mechanism ){
        case SASL_MECHANISM_SCRAM_SHA_256:
            evpmd = EVP_sha256();
            break;
        case SASL_MECHANISM_SCRAM_SHA_512:
        default:
            evpmd = EVP_sha512();
    }

    return evpmd;
}

/**
 * Salting the password
 *
 * This should be the default function called when salting passwords (but see also
 * ism_sasl_scram_mechanism_salt_password)
 */
int ism_sasl_scram_salt_password (ism_sasl_scram_context * context, const char *password, int password_size,
                        const char *salt, int salt_size,
                        int iteration, concat_alloc_t * outbuf) {
    return ism_sasl_scram_EVP_salt_password (context->scram_evp, password,
                password_size, salt, salt_size, iteration, outbuf);
}

/**
 * Wrapper around ism_sasl_scram_EVP_salt_password to be used when we don't have a
 * ism_sasl_scram_context - normally use ism_sasl_scram_salt_password
 */
int ism_sasl_scram_mechanism_salt_password (ism_sasl_machanism_e in_mechanism, 
                        const char *password, int password_size,
                        const char *salt, int salt_size,
                        int iteration, concat_alloc_t * outbuf) {

    const EVP_MD *evp = getEVPMDfromMechanism(in_mechanism);

    if (evp == NULL) {
        return 1;
    }
    return ism_sasl_scram_EVP_salt_password (evp, password,
                password_size, salt, salt_size, iteration, outbuf);
}


/**
 * Perform keyed-hash message authentication
 *
 */
int ism_sasl_scram_hmac (ism_sasl_scram_context * context, const char *key, int key_size,
                          const char  *message, int message_size,
                      concat_alloc_t * outbuf ) {
    const EVP_MD *evp = context->scram_evp;
    unsigned int outsize;
    unsigned char tempbuf[EVP_MAX_MD_SIZE];

    if (!HMAC(evp,
              (const unsigned char *)key, (int)key_size,
              (const unsigned char *)message, (int)message_size,
              (unsigned char *)&tempbuf, &outsize)) {

            return 1;
    }
    ism_common_allocBufferCopyLen(outbuf,(char *)tempbuf, outsize);
    bputchar(outbuf,0);
    outbuf->used--;

    return 0;
}

/**
 * Perform Hashing
 *
 * NOTE OpenSSL SHA256 or SHA512 will be used
 *
 */
int ism_sasl_scram_hash ( ism_sasl_scram_context * context, const char *message, int message_size,
                       concat_alloc_t * outbuf ) {

    char * tempbuf = alloca(context->scram_hash_f_digest_length);
    context->scram_hash_f(
            (const unsigned char *)message, message_size,
            (unsigned char *)tempbuf);

    ism_common_allocBufferCopyLen(outbuf,tempbuf, context->scram_hash_f_digest_length);
    bputchar(outbuf,0);
    outbuf->used--;
    return 0;
}

/**
 * Building Client message without proof
 *
 */
int ism_sasl_scram_build_client_final_message_wo_proof (ism_sasl_scram_context * context,
    const char *snonce, int snonce_size,
    const char * cnonce, int cnonce_size,
    concat_alloc_t * outbuf) {

    ism_common_allocBufferCopyLen(outbuf,"c=biws,r=", 9 );
    ism_common_allocBufferCopyLen(outbuf, snonce, snonce_size);
    bputchar(outbuf,0);
    outbuf->used--;

    return 0;

}

/**
 * Build client auth message for final message
 */
int ism_sasl_scram_build_client_final_auth_message (ism_sasl_scram_context * context,
    const char * client_first_msg_bare, int client_first_msg_bare_size,
    const char * server_first_msg, int server_first_msg_size,
    const char * client_final_wo_proof_msg, int client_final_wo_proof_msg_size,
    concat_alloc_t * outbuf) {

    ism_common_allocBufferCopyLen(outbuf,client_first_msg_bare, client_first_msg_bare_size );
    ism_common_allocBufferCopyLen(outbuf,",", 1 );
    ism_common_allocBufferCopyLen(outbuf, server_first_msg, server_first_msg_size);
    ism_common_allocBufferCopyLen(outbuf,",", 1 );
    ism_common_allocBufferCopyLen(outbuf, client_final_wo_proof_msg, client_final_wo_proof_msg_size);
    bputchar(outbuf,0);
    outbuf->used--;

    return 0;
}

/**
 * Build Client Final Message
 *
 */
int ism_sasl_scram_build_client_final_message (ism_sasl_scram_context * context,
    const char * client_final_msg_wo_proof, int client_final_msg_wo_proof_size,
    const char * client_proof_base64, int client_proof_base64_size,
    concat_alloc_t * outbuf) {

    ism_common_allocBufferCopyLen(outbuf,client_final_msg_wo_proof, client_final_msg_wo_proof_size );
    ism_common_allocBufferCopyLen(outbuf,",p=", 3 );
    ism_common_allocBufferCopyLen(outbuf, client_proof_base64, client_proof_base64_size);
    bputchar(outbuf,0);
    outbuf->used--;

    return 0;
}


/**
 * Build and Store the Server Signature
 *
 * RFC 5802: ServerSignature := HMAC(ServerKey, AuthMessage)
 */
int ism_sasl_scram_build_server_signature(ism_sasl_scram_context * context,
        const char * server_key, int server_key_size,
        const char * auth_msg, int auth_msg_size,
        concat_alloc_t * outbuf)
{
    /* ServerSignature := HMAC(ServerKey, AuthMessage) */
    ism_sasl_scram_hmac(context, server_key, server_key_size,
            auth_msg, auth_msg_size,
            outbuf);

    /* Encode ServerSignature to Base64 */
    char b64SigBuf[1024];
    char * serverSignatureB64 = (char *)&b64SigBuf;
    int encodeSize = ism_common_toBase64((char *) outbuf->buf,serverSignatureB64, outbuf->used);
    char * serverSignatureB64_str = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_saslScram,393),
                                                                encodeSize+1);
    memcpy(serverSignatureB64_str,serverSignatureB64, encodeSize);
    serverSignatureB64_str[encodeSize]=0;

    //Store ServerSignature for later processing of Final Server Message
    context->server_signature_b64=serverSignatureB64_str;
    context->server_signature_b64_size = encodeSize;
    return 0;
}


/* Construct the Buffer for the Client Final Message
 *
 * RFC5802 Specs for building the Client Final Message to send to Broker
 *   SaltedPassword  := Hi(Normalize(password), salt, i)
     ClientKey       := HMAC(SaltedPassword, "Client Key")
     StoredKey       := H(ClientKey)
     AuthMessage     := client-first-message-bare + "," +
                        server-first-message + "," +
                        client-final-message-without-proof
     ClientSignature := HMAC(StoredKey, AuthMessage)
     ClientProof     := ClientKey XOR ClientSignature
     ServerKey       := HMAC(SaltedPassword, "Server Key")
     ServerSignature := HMAC(ServerKey, AuthMessage)
 */
int ism_sasl_scram_build_client_final_message_buffer(ism_sasl_scram_context * context, const char * password, ism_prop_t * props, concat_alloc_t * client_final_msg_outbuf)
{

    const char * serverNonce = ism_common_getStringProperty(props, "r");
    const char * salt = ism_common_getStringProperty(props, "s");
    int iteration = ism_common_getIntProperty(props, "i", 0);


    //Decode base64
    char privateKeyBuf[1024];
    char * salt_decoded = (char *)&privateKeyBuf;
    int size = ism_common_fromBase64((char *)salt, salt_decoded, strlen(salt) );

    /*  SaltedPassword  := Hi(Normalize(password), salt, i) */
    char salted_password_buf[512];
    concat_alloc_t saltedpassword_outbuf={salted_password_buf, sizeof(salted_password_buf)};
    ism_sasl_scram_salt_password (context, password, strlen(password),
            salt_decoded, size,
            iteration, &saltedpassword_outbuf);

    /* ClientKey       := HMAC(SaltedPassword, "Client Key") */
    char client_key_buf[512];
    concat_alloc_t clientkey_outbuf={client_key_buf, sizeof(client_key_buf)};
    ism_sasl_scram_hmac (context, saltedpassword_outbuf.buf, saltedpassword_outbuf.used,
                    SASL_CRAM_CLIENT_KEY_NAME, SASL_CRAM_CLIENT_KEY_NAME_SIZE,
                    &clientkey_outbuf ) ;

    /* StoredKey       := H(ClientKey) */
    char storekey_buf[512];
    concat_alloc_t storekey_outbuf={storekey_buf, sizeof(storekey_buf)};
    ism_sasl_scram_hash (context, clientkey_outbuf.buf, clientkey_outbuf.used,
                           &storekey_outbuf ) ;

    /* ServerKey       := HMAC(SaltedPassword, "Server Key") */
    char serverkey_buf[512];
    concat_alloc_t serverkey_outbuf={serverkey_buf, sizeof(serverkey_buf)};
    ism_sasl_scram_hmac(context, saltedpassword_outbuf.buf, saltedpassword_outbuf.used,
            SASL_CRAM_SERVER_KEY_NAME, SASL_CRAM_SERVER_KEY_NAME_SIZE,
            &serverkey_outbuf);


    /* client-final-message-without-proof */
    char clientwoproofmsg_buf[512];
    concat_alloc_t client_msg_wo_proof_outbuf={clientwoproofmsg_buf, sizeof(clientwoproofmsg_buf)};
    ism_sasl_scram_build_client_final_message_wo_proof (context,
            serverNonce, strlen(serverNonce),
           context->client_nonce, context->client_nonce_size,
           &client_msg_wo_proof_outbuf) ;


    /* AuthMessage     := client-first-message-bare + "," +
     *                    server-first-message + "," +
     *                    client-final-message-without-proof */
    char authmsg_buf[512];
    concat_alloc_t authmsg_outbuf={authmsg_buf, sizeof(authmsg_buf)};
    ism_sasl_scram_build_client_final_auth_message (context,
            context->client_first_msg_bare, context->client_first_msg_bare_size,
            context->server_first_msg, context->server_first_msg_size,
            client_msg_wo_proof_outbuf.buf, client_msg_wo_proof_outbuf.used,
            &authmsg_outbuf);

    /* ClientSignature := HMAC(StoredKey, AuthMessage) */
    char clientsignature_buf[512];
    concat_alloc_t clientsignature_outbuf={clientsignature_buf, sizeof(clientsignature_buf)};
    ism_sasl_scram_hmac(context, storekey_outbuf.buf, storekey_outbuf.used,
            authmsg_outbuf.buf, authmsg_outbuf.used,
            &clientsignature_outbuf);

    /* ClientProof     := ClientKey XOR ClientSignature */
    char * clientproof = alloca(clientkey_outbuf.used);
    int clientproof_size = clientkey_outbuf.used;
    int i=0;
    for (i = 0 ; i < (int)clientkey_outbuf.used ; i++){
        clientproof[i] = clientkey_outbuf.buf[i] ^ clientsignature_outbuf.buf[i];
    }

    /* ServerSignature := HMAC(ServerKey, AuthMessage) */
    char serversignature_buf[512];
    concat_alloc_t serversignature_outbuf={serversignature_buf, sizeof(serversignature_buf)};
    ism_sasl_scram_build_server_signature(context, serverkey_outbuf.buf, serverkey_outbuf.used,
          authmsg_outbuf.buf, authmsg_outbuf.used, &serversignature_outbuf
    );

    //Encode the ClientProof to base64
    char b64ClientProofBuf[1024];
    char * b64ClientProof = (char *)&b64ClientProofBuf;
    int proofencodeSize = ism_common_toBase64((char *) clientproof,b64ClientProof, clientproof_size);

    //Build and Copy the client final message to outbuf
    ism_sasl_scram_build_client_final_message (context,
            client_msg_wo_proof_outbuf.buf, client_msg_wo_proof_outbuf.used,
            b64ClientProof, proofencodeSize,
            client_final_msg_outbuf);



    //Clean up the concat buf
    ism_common_freeAllocBuffer(&saltedpassword_outbuf);
    ism_common_freeAllocBuffer(&clientkey_outbuf);
    ism_common_freeAllocBuffer(&storekey_outbuf);
    ism_common_freeAllocBuffer(&serverkey_outbuf);
    ism_common_freeAllocBuffer(&client_msg_wo_proof_outbuf);
    ism_common_freeAllocBuffer(&serversignature_outbuf);
    ism_common_freeAllocBuffer(&authmsg_outbuf);
    ism_common_freeAllocBuffer(&serversignature_outbuf);
    ism_common_freeAllocBuffer(&clientsignature_outbuf);



    return 0;
}

/**
 * Verify Server last Message
 */
int ism_sasl_scram_server_final_verify(ism_sasl_scram_context * context,  ism_prop_t * props, concat_alloc_t * outbuf)
{
    const char *  verifier =  ism_common_getStringProperty(props, "v");
    if(verifier!=NULL){
        //Compare the Server Signature
        if(!strcmp(verifier,context->server_signature_b64)){
           //Authentication Success
           return 0;
        }else{
           //Mismatch Server Signature. Authentication Failed.
           char * error_str = "Server Signature validation failed";
           ism_common_allocBufferCopyLen(outbuf, error_str, strlen(error_str));
           return 1;
        }
    }else{
        const char *  error =  ism_common_getStringProperty(props, "e");
        //Server Error
        if(error!=NULL){
            ism_common_allocBufferCopyLen(outbuf, error, strlen(error));
        }
        return 1;
    }
}

/**
 * Set Server First Message into the context
 */
int ism_sasl_scram_server_set_first_message(ism_sasl_scram_context * context, const char * server_first_message)
{
    context->server_first_msg = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,395),server_first_message);
    context->server_first_msg_size = (int)strlen(server_first_message);
    return 0;
}
