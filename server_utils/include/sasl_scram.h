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
 * ISM SASL SCRAM Header file
 */

#ifndef IOTSASLSCRAM_H
#define IOTSASLSCRAM_H

#include <ismutil.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SASL_CRAM_CLIENT_KEY_NAME           "Client Key"
#define SASL_CRAM_CLIENT_KEY_NAME_SIZE      10
#define SASL_CRAM_SERVER_KEY_NAME           "Server Key"
#define SASL_CRAM_SERVER_KEY_NAME_SIZE      10

/**
 * SASL Mechanism
 * Note: for SASL SCRAM, we only support SCRAM-SHA-256 AND SCRAM-SHA512
 */
typedef enum {
        SASL_MECHANISM_PLAIN                          = 0,
        SASL_MECHANISM_SCRAM_SHA_256                  = 1,
        SASL_MECHANISM_SCRAM_SHA_512                  = 2,
} ism_sasl_machanism_e;


/**
 * the state of the SASL SCRAM process
 */
typedef enum {
        SASL_SCRAM_STATE_NEW                    = 0,
        SASL_SCRAM_STATE_CLIENT_FIRST_MESSAGE   = 1,
        SASL_SCRAM_STATE_CLIENT_FINAL_MESSAGE   = 2,
} ism_sasl_scram_state_e;

/**
 * Function pointer to the hashing function
 */
typedef unsigned char *(*scram_hash_f_t) (const unsigned char *d, size_t n,
                                                  unsigned char *md);

/**
 * SASL SCRAM context
 *
 */
typedef struct ism_sasl_scram_context {
        ism_sasl_machanism_e    mechanism;
        const EVP_MD *          scram_evp;
        int                     scram_hash_f_digest_length;
        scram_hash_f_t          scram_hash_f;
        ism_sasl_scram_state_e  state;
        char *                  client_nonce;               /* client c-nonce */
        int                     client_nonce_size;      /* client c-nonce size */
        char *                  client_first_msg_bare;       /* client-first-message-bare */
        int                     client_first_msg_bare_size;
        char *                  server_first_msg;
        int                     server_first_msg_size;
        char *                  server_signature_b64;     /* ServerSignature in Base64 */
        int                     server_signature_b64_size;     /* ServerSignature in Base64 */
        ism_prop_t *            server_response_props;
} ism_sasl_scram_context;

/**
 * Create new SASL SCRAM Context
 * Note: The caller must call ism_sasl_scram_context_destroy
 * to destroy the context
 * @param mechanism SASL mechanism. SCRAM_SHA_256, AND SCRAM_SHA_512
 * @return the SASL Scram context
 *
 */
ism_sasl_scram_context *  ism_sasl_scram_context_new(ism_sasl_machanism_e in_mechanism);

/**
 * Free resources and destroy the SASL SCRAM context
 * @param context the SASL SCRAM context
 */
int ism_sasl_scram_context_destroy(ism_sasl_scram_context * context);

/**
 * Build Client First Message to the Server
 *
 * The format of the message based on RFC 5802
 * Here is the example:
 *  n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
 *
 *  @param context  the SASL SCRAM context
 *  @param username the username
 *  @param outbuf   the output buffer which will contain the message
 *  @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_build_client_first_message (ism_sasl_scram_context * context, const char * username, concat_alloc_t *outbuf);

/**
 * Parse the Server Response and put the attribute and its value into properties object
 * @param inbuf the server response buffer
 * @param props the properties object which the attributes/values will be stored.
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_parse_properties (char  * inbuf, ism_prop_t * props) ;

/**
 * Salting the password
 *
 * RFC Spec on SaltedPassword
 *    SaltedPassword  := Hi(Normalize(password), salt, i) */
int ism_sasl_scram_salt_password (ism_sasl_scram_context * context, const char *in, int in_size,
                        const char *salt, int salt_size,
                        int iteration, concat_alloc_t * outbuf) ;

/**
 * Perform keyed-hash message authentication
 *
 * NOTE: OpenSSL HMAC will be used.
 *
 * @param context       the SASL SCRAM context
 * @param key           the key which will be use for hashing
 * @param key_size      the key size
 * @param message       the message which be hashed using the key
 * @param message_size  the message size
 * @param outbuf        the output buffer
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_hmac (ism_sasl_scram_context * context, const char *key, int key_size,
                          const char  *message, int message_size,
                      concat_alloc_t * outbuf ) ;

/**
 * Perform Hashing
 *
 * NOTE OpenSSL SHA256 or SHA-512 will be used
 *
 * @param context           the SSL SCRAM context
 * @param message           the message will be hashed
 * @param message_size      the message size
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_hash ( ism_sasl_scram_context * context, const char *message, int message_size,
                       concat_alloc_t * outbuf );

/**
 * Building Client message without proof
 *
 * @param context       the SSL SCRAM context
 * @param snonce        Server nonce
 * @param snonce_size   Server nonce length
 * @param cnonce        Client nonce
 * @param cnonce        Client nonce length
 * @return outbuf       output buffer
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_build_client_final_message_wo_proof (ism_sasl_scram_context * context,
        const char *snonce, int snonce_size,
        const char * cnonce, int cnonce_size,
        concat_alloc_t * outbuf) ;

/**
 * Build client auth message for final message
 * @param context                           The SSL SCRAM context
 * @param client_first_msg_bare             The client bare message
 * @param client_first_msg_bare_size        the client bare message size
 * @param client_final_wo_proof_msg         the client message without proof
 * @param client_final_wo_proof_msg_size    the client message without proof size
 * @param outbuf                            the output buffer
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_build_client_final_auth_message (ism_sasl_scram_context * context,
        const char * client_first_msg_bare, int client_first_msg_bare_size,
        const char * server_first_msg, int server_first_msg_size,
        const char * client_final_wo_proof_msg, int client_final_wo_proof_msg_size,
        concat_alloc_t * outbuf) ;

/**
 * Build Client Final Message
 *
 * Here is an example:
 * c=biws,r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,
 *     p=v0X8v3Bz2T0CJGbJQyF0X+HI4Ts=
 *
 * @param context       the SASL SCRAM context
 * @param client_final_msg_wo_proof
 * @param client_final_msg_wo_proof_size
 * @param client_proof_base64
 * @param client_proof_base64_size
 * @param output output buffer
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_build_client_final_message (ism_sasl_scram_context * context,
        const char * client_final_msg_wo_proof, int client_final_msg_wo_proof_size,
        const char * client_proof_base64, int client_proof_base64_size,
        concat_alloc_t * outbuf) ;

/**
 * Build and Store the Server Signature
 *
 * @param context           the SASL SCRAM context
 * @param server_key        the server key
 * @param server_key_size   the server key size
 * @paramm auth_msg         the auth message
 * @param auth_msg_size     the auth message size
 * @param outbuf            the output buffer
 * @return 0 for success. Otherwise failed.
 */
int ism_sasl_scram_build_server_signature(ism_sasl_scram_context * context,
        const char * server_key, int server_key_size,
        const char * auth_msg, int auth_msg_size,
        concat_alloc_t * outbuf);

/**
 * Building the final client message to the server. This is the last message from the client
 *
 * @param context                   the SASL SCRAM context
 * @param password                  the password which will be salted and send to the server
 * @param props                     Properties object contained server attributes/values
 * @param client_final_msg_buffer   the output buffer
 * @return 0 for success, otherwise is an error.
 */
int ism_sasl_scram_build_client_final_message_buffer(ism_sasl_scram_context * context, const char * password,
                                        ism_prop_t * props, concat_alloc_t * client_final_msg_buffer);

/**
 * Verify the authentication based on the server final response
 * @param   context
 * @param   props which contains the final verification message
 * @param   outbuf output buff for any error from server final message
 * @return  0 means authorized. False otherwise.
 */
int ism_sasl_scram_server_final_verify(ism_sasl_scram_context * context,  ism_prop_t * props, concat_alloc_t * outbuf);

/**
 * Set Server First Message into the context
 * @param context the SASL SCRAM context
 * @param server_first_message the Server First Message
 * @return  0 means authorized. False otherwise.
 */
int ism_sasl_scram_server_set_first_message(ism_sasl_scram_context * context, const char * server_first_message);

#ifdef __cplusplus
}
#endif

#endif
