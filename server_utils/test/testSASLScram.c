/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

#include <ismutil.h>
#include <imacontent.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "sasl_scram.c"

void sanitize_output(char * input, int len) {
    for ( int i = 0 ; i < len ; i++ ){
      if(input[i] < 32){
        input[i] = 63;
      }
    }
}

void sasl_scram_generate_nonce_test(void){
    int rc=0;
    ism_sasl_scram_context * context = ism_sasl_scram_context_new(SASL_MECHANISM_SCRAM_SHA_512);
    rc = sasl_scram_generate_nonce(context);
    CU_ASSERT(rc == 0);
    char * nonce = alloca(context->client_nonce_size+1);
    memcpy(nonce, context->client_nonce, context->client_nonce_size);
    nonce[context->client_nonce_size]=0;
    printf("nonce=%s\n", nonce);
    printf("nonce_length=%d\n",(int) strlen(nonce));

    ism_sasl_scram_context_destroy(context);



}

void sasl_scram_build_client_first_message_test(void){
    char xbuf[64];
    concat_alloc_t buf = {xbuf, sizeof(xbuf)};
    int rc=0;

    ism_sasl_scram_context *  context = ism_sasl_scram_context_new(SASL_MECHANISM_SCRAM_SHA_512);

    rc = ism_sasl_scram_build_client_first_message(context, "iot=user", &buf);
    CU_ASSERT(rc == 0);
    char * first_message = alloca(buf.used+1);
    memcpy(first_message, buf.buf, buf.used);
    first_message[ buf.used]=0;
    printf("first_message=%s\n", first_message);
    CU_ASSERT( context->client_first_msg_bare_size > 10);
    CU_ASSERT( strstr(first_message, "n=iot=3Duser") !=NULL);


    buf.used=0;
    rc = ism_sasl_scram_build_client_first_message(context, "iot,user", &buf);
    first_message = alloca(buf.used+1);
    memcpy(first_message, buf.buf, buf.used);
    first_message[ buf.used]=0;
    printf("first_message=%s\n", first_message);
    CU_ASSERT( context->client_first_msg_bare_size > 10);
    CU_ASSERT( strstr(first_message, "n=iot=2Cuser") !=NULL);

    buf.used=0;
    rc = ism_sasl_scram_build_client_first_message(context, "iot-user", &buf);
    first_message = alloca(buf.used+1);
    memcpy(first_message, buf.buf, buf.used);
    first_message[ buf.used]=0;
    printf("first_message=%s\n", first_message);
    CU_ASSERT( context->client_first_msg_bare_size > 10);
    CU_ASSERT( strstr(first_message, "n=iot-user") !=NULL);

    ism_sasl_scram_context_destroy(context);
}

void sasl_scram_get_property_test (void)
{
    char * serverResponse = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"r=WyICfw3qL7u9wk8h3WCUJ1hr39pfqeejypqciml473wncyf,s=bGRjZ2QyM20wN21vaXRybjJoNXJ1YTcxag==,i=4096");

    ism_prop_t * props = ism_common_newProperties(10);

    ism_sasl_scram_parse_properties(serverResponse, props);

    CU_ASSERT(ism_common_getPropertyCount(props) == 3);
    const char * serverNonce = ism_common_getStringProperty(props, "r");
    printf("serverNonce=%s\n", serverNonce);
    CU_ASSERT(serverNonce!=NULL);
    CU_ASSERT(!strcmp("WyICfw3qL7u9wk8h3WCUJ1hr39pfqeejypqciml473wncyf", serverNonce));

    const char * salt = ism_common_getStringProperty(props, "s");
    printf("salt=%s\n", salt);
    CU_ASSERT(salt!=NULL);
    CU_ASSERT(!strcmp("bGRjZ2QyM20wN21vaXRybjJoNXJ1YTcxag==", salt));

    int iteration = ism_common_getIntProperty(props, "i", 0);
    printf("iteration=%d\n", iteration);
    CU_ASSERT(iteration==4096);

    if(serverResponse) ism_common_free(ism_memory_saslScram,serverResponse);
    ism_common_freeProperties(props);

}

void sasl_scram_Hi_test(void)
{

    char xbuf[256];
    concat_alloc_t outbuf={xbuf, sizeof(xbuf)};

    ism_sasl_scram_context *  context = ism_sasl_scram_context_new(SASL_MECHANISM_SCRAM_SHA_512);

    ism_sasl_scram_build_client_first_message(context, "iot-user", &outbuf);

    char * serverResponse = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"r=WyICfw3qL7u9wk8h3WCUJ1hr39pfqeejypqciml473wncyf,s=bGRjZ2QyM20wN21vaXRybjJoNXJ1YTcxag==,i=4096");

    ism_prop_t * props = ism_common_newProperties(10);

    ism_sasl_scram_parse_properties(serverResponse, props);

    CU_ASSERT(ism_common_getPropertyCount(props) == 3);
    const char * serverNonce = ism_common_getStringProperty(props, "r");
    printf("serverNonce=%s\n", serverNonce);
    CU_ASSERT(serverNonce!=NULL);
    CU_ASSERT(!strcmp("WyICfw3qL7u9wk8h3WCUJ1hr39pfqeejypqciml473wncyf", serverNonce));

    const char * salt = ism_common_getStringProperty(props, "s");
    printf("salt=%s\n", salt);
    CU_ASSERT(salt!=NULL);
    CU_ASSERT(!strcmp("bGRjZ2QyM20wN21vaXRybjJoNXJ1YTcxag==", salt));

    int iteration = ism_common_getIntProperty(props, "i", 0);
    printf("iteration=%d\n", iteration);
    CU_ASSERT(iteration==4096);

    //Decode base64
    char privateKeyBuf[1024];
    char * salted_decoded = (char *)&privateKeyBuf;
    int size = ism_common_fromBase64((char *)salt, salted_decoded, strlen(salt) );
    char * salted_decode_str = alloca(size+1);
    memcpy(salted_decode_str, salted_decoded, size);
    salted_decode_str[size]=0;
    printf("salt_decoded=%s\n", salted_decode_str);

    CU_ASSERT(size>10);

    char * password = "testpassword";
    /* SaltedPassword  := Hi(Normalize(password), salt, i) */
    char salt_password_buf[512];
    concat_alloc_t salt_password_outbuf={salt_password_buf, sizeof(salt_password_buf)};
    ism_sasl_scram_salt_password (context, password, strlen(password),
            salted_decode_str, size,
            iteration, &salt_password_outbuf);
    char * saltedpassword = alloca(salt_password_outbuf.used);
    memcpy(saltedpassword, salt_password_outbuf.buf, salt_password_outbuf.used);
    saltedpassword[salt_password_outbuf.used]=0;
    sanitize_output(saltedpassword,salt_password_outbuf.used);
    printf("Salted Password=%s\n", saltedpassword);
    CU_ASSERT(salt_password_outbuf.used>10);


    if(serverResponse) ism_common_free(ism_memory_saslScram,serverResponse);
    ism_common_freeProperties(props);

    ism_sasl_scram_context_destroy(context);
}


void sasl_scram_HMAC_test(void)
{
    char xbuf[256];
    concat_alloc_t outbuf={xbuf, sizeof(xbuf)};

    ism_sasl_scram_context *  context = ism_sasl_scram_context_new(SASL_MECHANISM_SCRAM_SHA_512);

    //Client First Message Bare:
    //n=iot-user,r=SyUNnFEsQXd3gLw07OIZi1Hh
    //ServerFirstMsg:
    //r=SyUNnFEsQXd3gLw07OIZi18vzo69dbb0esvbg6le0vuoj9u,s=Mzk0aDI1dHlrcWc1cjZ5YzFycWh0aHFjeA==,i=4096

    ism_sasl_scram_build_client_first_message(context, "iot-user", &outbuf);

    ism_common_free(ism_memory_saslScram, context->client_first_msg_bare);
    ism_common_free(ism_memory_saslScram, context->client_nonce);
    context->client_first_msg_bare = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"n=iot-user,r=SyUNnFEsQXd3gLw07OIZi1Hh");
    context->client_first_msg_bare_size = strlen(context->client_first_msg_bare);
    context->client_nonce = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"SyUNnFEsQXd3gLw07OIZi1Hh");
    context->client_nonce_size= strlen(context->client_nonce );
    char * serverResponse = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"r=SyUNnFEsQXd3gLw07OIZi18vzo69dbb0esvbg6le0vuoj9u,s=Mzk0aDI1dHlrcWc1cjZ5YzFycWh0aHFjeA==,i=4096");
    context->server_first_msg=ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),serverResponse);
    context->server_first_msg_size=strlen(serverResponse);

    ism_prop_t * props = ism_common_newProperties(10);

    ism_sasl_scram_parse_properties(serverResponse, props);

    CU_ASSERT(ism_common_getPropertyCount(props) == 3);
    const char * serverNonce = ism_common_getStringProperty(props, "r");
    printf("serverNonce=%s\n", serverNonce);
    CU_ASSERT(serverNonce!=NULL);
    CU_ASSERT(!strcmp("SyUNnFEsQXd3gLw07OIZi18vzo69dbb0esvbg6le0vuoj9u", serverNonce));

    const char * salt = ism_common_getStringProperty(props, "s");
    printf("salt=%s\n", salt);
    CU_ASSERT(salt!=NULL);
    CU_ASSERT(!strcmp("Mzk0aDI1dHlrcWc1cjZ5YzFycWh0aHFjeA==", salt));

    int iteration = ism_common_getIntProperty(props, "i", 0);
    printf("iteration=%d\n", iteration);
    CU_ASSERT(iteration==4096);

    //Decode base64
    char privateKeyBuf[1024];
    char * salted_decoded = (char *)&privateKeyBuf;
    int size = ism_common_fromBase64((char *)salt, salted_decoded, strlen(salt) );
    char * salted_decode_str = alloca(size+1);
    memcpy(salted_decode_str, salted_decoded, size);
    salted_decode_str[size]=0;
    printf("salt_decoded=%s\n", salted_decode_str);

    CU_ASSERT(size>10);

    char * password = "testpassword";
    outbuf.used=0;

    /* SaltedPassword  := Hi(Normalize(password), salt, i) */
    char salt_password_buf[512];
    concat_alloc_t saltpassword_outbuf={salt_password_buf, sizeof(salt_password_buf)};
    ism_sasl_scram_salt_password (context, password, strlen(password),
            salted_decoded, size,
            iteration, &saltpassword_outbuf);
    char * saltedpassword = alloca(saltpassword_outbuf.used);
    memcpy(saltedpassword, saltpassword_outbuf.buf, saltpassword_outbuf.used);
    saltedpassword[saltpassword_outbuf.used]=0;
    printf("Satled Password encryption: len=%d\n", saltpassword_outbuf.used);
    sanitize_output(saltedpassword,saltpassword_outbuf.used);
    printf("Salted Password=%s\n", saltedpassword);
    CU_ASSERT(saltpassword_outbuf.used>10);

    /* ClientKey       := HMAC(SaltedPassword, "Client Key") */
    char client_key_buf[512];
    concat_alloc_t clientkey_outbuf={client_key_buf, sizeof(client_key_buf)};
    ism_sasl_scram_hmac (context, saltpassword_outbuf.buf, saltpassword_outbuf.used,
                    SASL_CRAM_CLIENT_KEY_NAME, SASL_CRAM_CLIENT_KEY_NAME_SIZE,
                    &clientkey_outbuf ) ;
    char * clientkey = alloca(clientkey_outbuf.used);
    memcpy(clientkey, clientkey_outbuf.buf, clientkey_outbuf.used);
    clientkey[clientkey_outbuf.used]=0;
    printf("Client Key encryption: len=%d\n", clientkey_outbuf.used);
    sanitize_output(clientkey,clientkey_outbuf.used);
    printf("CKName Encryption : %s\n", clientkey);
    CU_ASSERT(clientkey_outbuf.used>10);

    /* StoredKey       := H(ClientKey) */
    char storekey_buf[512];
    concat_alloc_t storekey_outbuf={storekey_buf, sizeof(storekey_buf)};
    ism_sasl_scram_hash (context, clientkey_outbuf.buf, clientkey_outbuf.used,
                           &storekey_outbuf ) ;
    char * storedkey = alloca(storekey_outbuf.used);
    memcpy(storedkey, outbuf.buf, storekey_outbuf.used);
    storedkey[storekey_outbuf.used]=0;
    printf("storedkey encryption: len=%d\n", storekey_outbuf.used);
    printf("storedkey Encryption : %s\n", storedkey);
    CU_ASSERT(storekey_outbuf.used>10);


    /* ServerKey       := HMAC(SaltedPassword, "Server Key") */
    char serverkey_buf[512];
    concat_alloc_t serverkey_outbuf={serverkey_buf, sizeof(serverkey_buf)};
    ism_sasl_scram_hmac(context, saltpassword_outbuf.buf, saltpassword_outbuf.used,
            SASL_CRAM_SERVER_KEY_NAME, SASL_CRAM_SERVER_KEY_NAME_SIZE,
            &serverkey_outbuf);
    char * serverkey = alloca(serverkey_outbuf.used);
    memcpy(serverkey, serverkey_outbuf.buf, serverkey_outbuf.used);
    serverkey[serverkey_outbuf.used]=0;
    sanitize_output(serverkey,serverkey_outbuf.used);
    printf("Server Key Encryption : %s\n", serverkey);
    CU_ASSERT(serverkey_outbuf.used>10);


    /* client-final-message-without-proof */
    char clientwoproofmsg_buf[512];
    concat_alloc_t client_msg_wo_proof_outbuf={clientwoproofmsg_buf, sizeof(clientwoproofmsg_buf)};
    ism_sasl_scram_build_client_final_message_wo_proof (context,
            serverNonce, strlen(serverNonce),
           context->client_nonce, context->client_nonce_size,
           &client_msg_wo_proof_outbuf) ;

    char * client_msg_wo_proof = alloca(client_msg_wo_proof_outbuf.used);
    memcpy(client_msg_wo_proof, client_msg_wo_proof_outbuf.buf, client_msg_wo_proof_outbuf.used);
    client_msg_wo_proof[client_msg_wo_proof_outbuf.used]=0;
    printf("Client WoProof_msg : %s\n", client_msg_wo_proof);
    CU_ASSERT(client_msg_wo_proof_outbuf.used>10);

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

    char * auth_msg = alloca(authmsg_outbuf.used);
    memcpy(auth_msg, authmsg_outbuf.buf, authmsg_outbuf.used);
    auth_msg[authmsg_outbuf.used]=0;
    printf("Client first msg bare: %s\n", context->client_first_msg_bare);
    printf("auth_msg : %s\n", auth_msg);
    CU_ASSERT(authmsg_outbuf.used>10);


    /* ServerSignature := HMAC(ServerKey, AuthMessage) */
    char serversignature_buf[512];
    concat_alloc_t serversignature_outbuf={serversignature_buf, sizeof(serversignature_buf)};
    ism_sasl_scram_hmac(context, serverkey, strlen(serverkey),
            auth_msg, strlen(auth_msg),
            &serversignature_outbuf);
    char * serverSignature = alloca(serversignature_outbuf.used);
    memcpy(serverSignature, serversignature_outbuf.buf, serversignature_outbuf.used);
    serverSignature[serversignature_outbuf.used]=0;
    sanitize_output(serverSignature,serversignature_outbuf.used);
    printf("serverSignature : %s\n", serverSignature);
    CU_ASSERT(serversignature_outbuf.used>10);


    /* Store the Base64 encoded ServerSignature for quick comparison */

    char b64SigBuf[1024];
    char * b64Sig = (char *)&b64SigBuf;
    int encodeSize = ism_common_toBase64((char *) serversignature_outbuf.buf,b64Sig, serversignature_outbuf.used);
    char * b64Sig_str = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_saslScram,1),
            encodeSize+1);
    memcpy(b64Sig_str,b64Sig, encodeSize);
    b64Sig_str[encodeSize]=0;
    context->server_signature_b64=b64Sig_str;
    context->server_signature_b64_size = encodeSize;
    printf("serverSignatureB64 : %s\n", context->server_signature_b64);



    /* ClientSignature := HMAC(StoredKey, AuthMessage) */
    char clientsignature_buf[512];
    concat_alloc_t clientsignature_outbuf={clientsignature_buf, sizeof(clientsignature_buf)};
    ism_sasl_scram_hmac(context, storekey_outbuf.buf, storekey_outbuf.used,
            auth_msg, strlen(auth_msg),
            &clientsignature_outbuf);
    char * clientSignature = alloca(clientsignature_outbuf.used);
    memcpy(clientSignature, clientsignature_outbuf.buf, clientsignature_outbuf.used);
    clientSignature[clientsignature_outbuf.used]=0;
    sanitize_output(clientSignature,clientsignature_outbuf.used);
    printf("clientSignature : %s len=%d\n", clientSignature, clientsignature_outbuf.used);
    CU_ASSERT(clientsignature_outbuf.used>10);


    printf("ClientKeySize=%d ClientSignatureSize=%d\n", clientkey_outbuf.used, clientsignature_outbuf.used);
    CU_ASSERT(clientkey_outbuf.used == clientsignature_outbuf.used);

    /* ClientProof     := ClientKey XOR ClientSignature */
    char * clientproof = alloca(clientkey_outbuf.used);
    int clientproof_size = clientkey_outbuf.used;
    int i=0;
    for (i = 0 ; i < (int)clientkey_outbuf.used ; i++){
        clientproof[i] = clientkey_outbuf.buf[i] ^ clientsignature_outbuf.buf[i];
    }

    char b64ClientProofBuf[1024];
    char * b64ClientProof = (char *)&b64ClientProofBuf;
    int proofencodeSize = ism_common_toBase64((char *) clientproof,b64ClientProof, clientproof_size);
    char * b64ClientProof_str = malloc(proofencodeSize+1);
    memcpy(b64ClientProof_str,b64ClientProof, proofencodeSize);
    b64ClientProof_str[proofencodeSize]=0;
    printf("clientProofB64 : %s\n", b64ClientProof_str);

    char client_final_msg_buf[512];
    concat_alloc_t client_final_msg_outbuf={client_final_msg_buf, sizeof(client_final_msg_buf)};
    ism_sasl_scram_build_client_final_message (context,
            client_msg_wo_proof_outbuf.buf, client_msg_wo_proof_outbuf.used,
            b64ClientProof, proofencodeSize,
            &client_final_msg_outbuf);

    char * clientFinalMessage = alloca(client_final_msg_outbuf.used);
    memcpy(clientFinalMessage, client_final_msg_outbuf.buf, client_final_msg_outbuf.used);
    clientFinalMessage[client_final_msg_outbuf.used]=0;
    printf("clientFinalMessage : %s len=%d\n", clientFinalMessage, client_final_msg_outbuf.used);
    CU_ASSERT(client_final_msg_outbuf.used>10);

    if(serverResponse) ism_common_free(ism_memory_saslScram,serverResponse);
    ism_common_freeProperties(props);

    ism_sasl_scram_context_destroy(context);

}

void sasl_scram_build_client_final_message_buffer_test(void)
{

    char xbuf[256];
    concat_alloc_t outbuf={xbuf, sizeof(xbuf)};
    ism_sasl_scram_context *  context = ism_sasl_scram_context_new(SASL_MECHANISM_SCRAM_SHA_512);

    //Client First Message Bare:
    //n=iot-user,r=SyUNnFEsQXd3gLw07OIZi1Hh
    //ServerFirstMsg:
    //r=SyUNnFEsQXd3gLw07OIZi18vzo69dbb0esvbg6le0vuoj9u,s=Mzk0aDI1dHlrcWc1cjZ5YzFycWh0aHFjeA==,i=4096

    ism_sasl_scram_build_client_first_message(context, "iot-user", &outbuf);

    ism_common_free(ism_memory_saslScram,context->client_first_msg_bare);
    ism_common_free(ism_memory_saslScram,context->client_nonce);
    context->client_first_msg_bare = strdup("n=user,r=fyko+d2lbbFgONRv9qkxdawL");
    context->client_first_msg_bare_size = strlen(context->client_first_msg_bare);
    context->client_nonce = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"fyko+d2lbbFgONRv9qkxdawL");
    context->client_nonce_size= strlen(context->client_nonce );
    char * serverResponse = ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),"r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=4096");
    context->server_first_msg=ism_common_strdup(ISM_MEM_PROBE(ism_memory_saslScram,1000),serverResponse);
    context->server_first_msg_size=strlen(serverResponse);

    ism_prop_t * props = ism_common_newProperties(10);
    ism_sasl_scram_parse_properties(serverResponse, props);

    char * password="pencil";
    outbuf.used =0;
    ism_sasl_scram_build_client_final_message_buffer(context,  password,
                                           props, &outbuf);

    char * clientFinalMessage = alloca(outbuf.used);
    memcpy(clientFinalMessage, outbuf.buf, outbuf.used);
    clientFinalMessage[outbuf.used]=0;
    printf("clientFinalMessage frominal_message_buffer_test : %s len=%d\n", clientFinalMessage, outbuf.used);


}

/* The timer tests initialization function.
 * Creates a timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int initSASLScramTests(void) {

    return 0;
}

/* The timer test suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int initSASLScramSuite(void) {
    //ism_log_init();
    return 0;
}

/* The timer test suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int cleanSASLScramSuite(void) {

    return 0;
}

void sasl_scram_test(void) {

    ism_common_setTraceLevel(2);
    //pxrouting_test_parseRouteOnly();
    sasl_scram_generate_nonce_test();
    sasl_scram_build_client_first_message_test();
    sasl_scram_get_property_test();
    sasl_scram_Hi_test();
    sasl_scram_HMAC_test();
    sasl_scram_build_client_final_message_buffer_test();


}

CU_TestInfo ISM_Util_CUnit_SASLScram[] = {
        { "sasl_scram_generate_nonce_test", sasl_scram_generate_nonce_test },
        { "sasl_scram_build_client_first_message_test", sasl_scram_build_client_first_message_test },
        { "sasl_scram_get_property_test", sasl_scram_get_property_test },
        { "sasl_scram_Hi_test", sasl_scram_Hi_test },
        { "sasl_scram_HMAC_test", sasl_scram_HMAC_test },
        { "sasl_scram_build_client_final_message_buffer_test", sasl_scram_build_client_final_message_buffer_test },
        CU_TEST_INFO_NULL };

