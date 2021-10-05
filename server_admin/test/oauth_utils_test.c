/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
 * File: oauth_utils_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on: 19/01/2015
 * --------------------------------------------------------------
 *
 *
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <admin.h>
#include <ismjson.h>
#include <authentication.h>
#include <security.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <curl/curl.h>

/************************************************************************/

/* Curl handle stored in thread-specific */
extern ism_threadkey_t curlHandleTLSKey;


/* Faked up ism auth token with just enough details
 * to test the validate oauth token function */
static ismAuthToken_t token;
static ism_transport_t *myTport;

ism_domain_t  c_ism_defaultDomain = {"DOM", 0, "Default" };
ism_trclevel_t * c_ism_defaultTrace;


/* Flag to control the listener thread.  It will stop
 * when this is != 0 */
static volatile int done = 0;
/* Flag to indicate that the server should return a 'bad' response */
static volatile int doBadRet = 0;
static volatile int twoStage = 0;
static volatile int stage = 0;
/* thread handle for the listener */
static pthread_t oauth_thread;
/* used to signal to the init function that the listener
 * is spun up and ready to go. */
static pthread_cond_t readycond;
/* mutex to go with the cond var */
static pthread_mutex_t readymutex;
/* Flag to indicate that the listener thread is in a ready state */
static int listenerReady = 0;

/* The listener will try to find an unused port.  Once it
 * has signaled that it is ready, it will set this variable
 * to the value of the port it is listening on. */
static volatile int port = 0;
/* Storage for the URL that the listener thread will accept
 * connections on.  This represents our faked up OAuth server
 * URL. */
static char url[256];

/* Prepares shared state for use by the various tests.
 * Starts the listener thread which is used to fake up
 * a positive response from an OAuth authorisation server */
void oauth_utils_test_setup(void);

/* Cleans up the shared state used by the various tests.
 * Most importantly, it ensures that the listener thread is
 * shut down.  */
void oauth_utils_test_teardown(void);

/* Thread function which listens on a local port for connections,
 * simply returning an HTTP header which includes a "200 OK" code.
 * This is needed because the validate function uses curl to send
 * the token to an oauth server.  These tests are intended to check
 * the validation of the auth token itself.  */
void thread_oauth_listener(void *arg);

/*************************************************************************************/
/* Need to mask all of these functions so that the validate function
 * doesn't explode.
 */
int ism_admin_getLDAPDN ( ismLDAPConfig_t *ldapobj,
        const char* username, int username_len,
        char ** pDN, int * DNLen, bool isGroup, int *dnInHeap);
static ismLDAPConfig_t ldapconfig;
ismLDAPConfig_t *ism_security_getLDAPConfig(void);

static char tmpbuffer[1024];
char * ism_transport_allocBytes(ism_transport_t * transport, int len, int align) {
        return tmpbuffer;
}

ism_time_t ism_transport_setConnectionExpire(ism_transport_t * transport, ism_time_t expire) {
    return 0;
}

XAPI ism_transport_t * ism_security_getTransFromSecContext(ismSecurity_t *sContext);

/*************************************************************************************/

/* Tests the golden path case where we are presented with a
 * nice, complete token comprising an expiry and access token. */
void test_oauth_validate_completeToken(void);
/* Tests that the validate function returns a 'not authorised' rc
 * when presented with an auth token that does not contain an
 * access token. */
void test_oauth_validate_missingToken(void);
/* Tests that the validate function returns a 'not authorised' rc
 * when presented with an auth token that contains a zero-length
 * access token. */
void test_oauth_validate_zeroLenToken(void);
/* Tests that the validate function returns a 'not authorised' rc
 * when presented with an auth token that does not contain an
 * expiry duration. */
void test_oauth_validate_missingExpiry(void);
/* Tests that we don't segfault when we get a long token.
 * This test assumes that the server doesn't return a '200 OK'
 * response, so we miss out a bunch of code.  This test highlights
 * a memory overwrite that could occur before IT06928. */
void test_oauth_long_token(void);
/*
 * tests that we can extract the user info when it is given back to us
 * in the body of the first curl request (when userInfoURL == oauthURL)
 */
void test_oauth_userInfoURL_equal(void);
/*
 * tests that we can request, and subsequently extract the user info
 * when the userInfoURL != oauthURL.
 */
void test_oauth_userInfoURL_notEqual(void);
/*
 * tests that we can request, and subsequently extract the user info
 * when the userInfoURL != oauthURL, when data has previously been
 * written into the body buffer, from previous curl request.
 */
void test_oauth_userInfoURL_pollutedBody(void);

/************************************************************************/

void oauth_utils_test_setup(void) {
    int rc=0;
    char msg[255];
    srand(time(0));
    CURLcode curlRc = 0;

    ism_common_initUtil();

    ism_field_t fptr;
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_allocBytes;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_allocBytes_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_setConnectionExpire;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_setConnectionExpire_fnptr", &fptr);


    ism_common_createThreadKey(&curlHandleTLSKey);

    curlRc = curl_global_init(CURL_GLOBAL_ALL);

    if (curlRc != 0) {
        sprintf(msg,"Failed to perform curl global init: %d\n", curlRc);
        CU_FAIL_FATAL(msg);
    }

    rc = pthread_cond_init(&readycond, NULL);

    if (rc != 0) {
        sprintf(msg, "Failed to initialise condition variable: %d\n", rc);
        curl_global_cleanup();
        CU_FAIL_FATAL(msg);
    }

    rc = pthread_mutex_init(&readymutex, NULL);

    if (rc != 0) {
        pthread_cond_destroy(&readycond);
        curl_global_cleanup();
        sprintf(msg, "Failed to initialise mutex variable: %d\n", rc);
        CU_FAIL_FATAL(msg);
    }

    /* Grab the mutex before we start the listener thread */
    rc = pthread_mutex_lock(&readymutex);

    if (rc != 0) {
        pthread_mutex_destroy(&readymutex);
        pthread_cond_destroy(&readycond);
        curl_global_cleanup();
        sprintf(msg, "Failed to lock the mutex during init: %d\n.", rc);
        CU_FAIL_FATAL(msg);
    }

    rc = pthread_create(&oauth_thread, NULL, (void*(*)(void*))thread_oauth_listener, NULL);

    if (rc != 0) {
        pthread_mutex_destroy(&readymutex);
        pthread_cond_destroy(&readycond);
        curl_global_cleanup();
        sprintf(msg, "Couldn't start fake oauth listener thread: %d\n", rc);
        CU_FAIL_FATAL(msg);
    }

    /* the listener thread will signal when it is ready for business */
    rc = pthread_cond_wait(&readycond,&readymutex);

    if (rc != 0) {
        sprintf(msg, "Failed to wait on the condition variable: %d\n", rc);
        curl_global_cleanup();
        CU_FAIL_FATAL(msg);
    }

    rc = pthread_mutex_unlock(&readymutex);

    if (rc != 0) {
        sprintf(msg, "Failed to unlock the mutex during init: %d\n", rc);
        curl_global_cleanup();
        CU_FAIL_FATAL(msg);
    }

    /* if the listener wasn't able to start properly, then there's
     * no point in going any further. */
    CU_ASSERT(listenerReady != 0);

    /* The listener thread will have assigned a valid port now. */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_url = url;
    token.oauth_userName = "";
    token.oauth_userPassword = "";
    token.oauth_userInfoUrl = NULL;
    token.oauth_usernameProp = "user";
    token.oauth_keyFileName = "";
    token.oauth_tokenNameProp = "access_token";
    token.oauth_fullKeyFilePath = "";
    token.oauth_isURLsEqual = 0;
    token.oauth_securedConnection = 0;
    sprintf(token.userDN,"userdn");

    ldapconfig.SearchUserDN = true;
    sprintf(ldapconfig.UserIdPrefix, "prefix");
    ldapconfig.UserSuffix = (char*)malloc(32);
    sprintf(ldapconfig.UserSuffix, "suffix");
    ldapconfig.UserDNMaxLen = 256;

    /* Our fake common transport handle */
    c_ism_defaultTrace = &c_ism_defaultDomain.trace;
    myTport = (ism_transport_t *) calloc(1,sizeof(ism_transport_t));
    myTport->clientID = "ClientA";
    myTport->userid = "UserA";
    myTport->cert_name = "CertA";
    myTport->client_addr = "127.0.0.1";
    myTport->protocol = "JMS,MQTT";
    myTport->listener = NULL;
    myTport->state = ISM_TRANST_Open;
    myTport->trclevel = c_ism_defaultTrace;
}

void oauth_utils_test_teardown(void) {
    /* update the done var, so the listener thread knows to stop */
    done = 1;
    /* wait for it to finish up, then clean up the mutex/cond var */
    pthread_join(oauth_thread, NULL);
    pthread_mutex_destroy(&readymutex);
    pthread_cond_destroy(&readycond);
    curl_global_cleanup();
}

static ismSecurity_t * create_security_token(void) {
    int rc;

    rc = ism_security_init();
    printf("\ncreate_security_token: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0 || rc == 113);

    if ( !statCount )
        statCount = (security_stat_t *)calloc(1, sizeof(security_stat_t));

    ismSecurity_t *secContext = NULL;
    //char *trans[1024] = {0};

    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, myTport, &secContext);
    printf("create_security_token: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);

    return secContext;
}


/**************** The Actual Tests ***********************/
void test_oauth_validate_completeToken(void) {
    int rc;
    ismSecurity_t *secContext = create_security_token();

    token.password = "{\"expires_in\":86400, \"access_token\":\"18926970-A-nMnSHDqg8Fsunm6Qx1cF1APp\"}";
    token.password_len = 74;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;
    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_completeToken - Default URL case: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* URL with query param */
    sprintf(url, "http://localhost:%d/?testQuery=1", port);
    token.oauth_urlProcessMethod = 1;
    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_completeToken - URL with query case: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* URL with query param and & */
    sprintf(url, "http://localhost:%d/?testQuery=1&", port);
    token.oauth_urlProcessMethod = 2;
    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_completeToken - URL with query + and case: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);


    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_validate_completeToken: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_validate_missingToken(void) {
    int rc;
    ismSecurity_t *secContext = create_security_token();

    token.password = "{\"expires_in\":86400}";
    token.password_len = 19;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_missingToken: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_NotAuthenticated);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_validate_missingToken: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_validate_zeroLenToken(void) {
    ismSecurity_t *secContext = create_security_token();
    int rc=ISMRC_OK;

    token.password = "{\"expires_in\":86400, \"access_token\":}";
    token.password_len = 36;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_zeroLenToken: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_NotAuthenticated);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_validate_zeroLenToken: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_validate_missingExpiry(void) {
    ismSecurity_t *secContext = create_security_token();
    int rc=ISMRC_OK;

    token.password = "{\"access_token\":\"18926970-A-nMnSHDqg8Fsunm6Qx1cF1APp\"}";
    token.password_len = 54;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_validate_missingExpiry: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_validate_missingExpiry: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_long_token(void) {
    ismSecurity_t *secContext = create_security_token();
    int rc=ISMRC_OK;

    int biglen = 4096;
    char access_token[4096] = {113};

    access_token[biglen-1] = '\0';
    char *token_prefix = "{\"expires_in\":86400, \"access_token\":\0";
    size_t prefixlen = 36;
    size_t tokenlen = biglen;
    size_t totallen = prefixlen + tokenlen + 64;

    char *whole_token = (char*)malloc(totallen);

    sprintf(whole_token, "%s\"%s\"}", token_prefix, access_token);

    token.password = whole_token;
    token.password_len = strlen(whole_token);
    token.oauth_isURLsEqual = 0;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    // set flag to ensure we get a bad response
    doBadRet = 1;

    rc = ism_validate_oauth_token(secContext, &token);

    doBadRet = 0;

    free(whole_token);
    printf("test_oauth_long_token: Return RC: %d\n", rc);
    CU_ASSERT(rc == ISMRC_NotAuthenticated);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_long_token: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_userInfoURL_equal(void) {
    token.oauth_userInfoUrl = token.oauth_url;
    token.oauth_usernameProp = "user";
    token.oauth_isURLsEqual = 1;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    ismSecurity_t *secContext = create_security_token();
    int rc=ISMRC_OK;

    token.password = "{\"expires_in\":86400, \"access_token\":\"18926970-A-nMnSHDqg8Fsunm6Qx1cF1APp\"}";
    token.password_len = 74;

    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_userInfoURL_equal: Return RC: %d\n", rc);
    CU_ASSERT(rc== ISMRC_OK);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_userInfoURL_equal: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_userInfoURL_notEqual(void) {
    token.oauth_userInfoUrl = token.oauth_url;
    token.oauth_usernameProp = "user";
    token.oauth_isURLsEqual = 0;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    ismSecurity_t *secContext = create_security_token();
    int rc=ISMRC_OK;

    token.password = "{\"expires_in\":86400, \"access_token\":\"18926970-A-nMnSHDqg8Fsunm6Qx1cF1APp\"}";
    token.password_len = 74;

    rc = ism_validate_oauth_token(secContext, &token);
    printf("test_oauth_userInfoURL_notEqual: Return RC: %d\n", rc);
    CU_ASSERT(rc== ISMRC_OK);

    rc = ism_security_destroy_context(secContext);
    printf("test_oauth_userInfoURL_notEqual: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

void test_oauth_userInfoURL_pollutedBody(void) {
    token.oauth_userInfoUrl = token.oauth_url;
    token.oauth_usernameProp = "user";
    token.oauth_isURLsEqual = 0;
    twoStage = 1;
    stage = 0;

    ismSecurity_t *sContext = create_security_token();
    int rc=ISMRC_OK;

    token.password = "{\"expires_in\":86400, \"access_token\":\"18926970-A-nMnSHDqg8Fsunm6Qx1cF1APp\"}";
    token.password_len = 74;

    /* Default processing of URL */
    sprintf(url, "http://localhost:%d/", port);
    token.oauth_urlProcessMethod = 0;

    rc = ism_validate_oauth_token(sContext, &token);
    printf("test_oauth_userInfoURL_pollutedBody: Return RC: %d\n", rc);

    twoStage = 1;
    stage = 0;
    CU_ASSERT(rc== ISMRC_OK);

    rc = ism_security_destroy_context(sContext);
    printf("test_oauth_userInfoURL_pollutedBody: Return RC: %d\n", rc);
}

/**************** The Actual Tests END  ***********************/

void thread_oauth_listener(void *arg) {

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[2048];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    pthread_mutex_lock(&readymutex);

    /* basic reply for any received OAuth token - just an HTTP header.  The key
     * bit here is that the status code is '200 OK'.
     * The body contains user user info, which the validate function attempts to parse.
     */
    char *reply = "HTTP/1.1 200 OK\nX-Powered-By:  Servlet/3.0\nContent-Language:  en-GB\nContent-Length:14\nContent-Type: text/plain\n\n{\"user\":\"bob\"}\0";
    int rlen = strlen(reply);

    char *reply2 = "HTTP/1.1 200 OK\nX-Powered-By:  Servlet/3.0\nContent-Language:  en-GB\nContent-Length:5\nContent-Type: text/plain\n\nHello\0";
    int rlen2 = strlen(reply2);

    char *badreply = "HTTP/1.1 405 Method Not Allowed\r\nX-Powered-By: Servlet/3.0\r\n\0";
    int brlen = strlen(badreply);

    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sockfd < 0) {
        perror("Error opening socket.");
        /* don't leave the init function hanging */
        pthread_cond_broadcast(&readycond);
        pthread_mutex_unlock(&readymutex);
        return;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int retryCount = 0;
    /* chosen arbitrarily */
    int maxRetries = 30;
    int bindrc = 0;

    /* Attempt to find a usable port we can listen on */
    do {
        portno = rand() % (65535-1025) + 1025;
        serv_addr.sin_port = htons(portno);
        retryCount++;
        bindrc = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    } while((bindrc != 0) && (retryCount < maxRetries));

    /* were we able to find a good port? */
    if (bindrc != 0) {
        perror("Failed to find a port within max retries.");
        pthread_cond_broadcast(&readycond);
        pthread_mutex_unlock(&readymutex);
        return;
    }

    /* update global var so the init func can construct the URL to send
     * OAuth tokens to. */
    port = portno;

    int listenrc = listen(sockfd, 5);

    if (listenrc != 0) {
        perror("Failed to listen on socket.");
        pthread_cond_broadcast(&readycond);
        pthread_mutex_unlock(&readymutex);
        return;
    }

    clilen = sizeof(cli_addr);

    /* ready to begin... signal to the init func that we are ready to
     * start accepting connections on this thread. */
    listenerReady = 1;
    pthread_cond_broadcast(&readycond);
    pthread_mutex_unlock(&readymutex);

    /* main thread loop.  wait for conns until we're done */
    while (done == 0) {
        /* we've opened our socket in non-blocking mode so that
         * we can keep checking for changes to the done var.  As
         * such, this call will 'fail' if there isn't a connection
         * in the queue. */
        newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            /* If the error is one of these, then it just means that
             * there wasn't a connection in the queue, so we're good
             * to continue. */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
            }
            else {
                /* This is a real error... */
                perror("Error on accept.");
            }

            /* In either case, we will wait a short while, then just
             * continue waiting for connections until were told to
             * stop. */
            usleep(50000);
            continue;
        }

        bzero(buffer,2048);
        /* expecting an HTTP get with the token, but we don't care what it is */
        n = read(newsockfd,buffer,2048);

        /* if we can't read from the socket, probs not worth trying to write to it */
        if (n < 0) {
            perror("Error reading from socket.");
        }
        else {
            /* Have we been asked for a good or bad response? */
            if (doBadRet == 1) {
                n = write(newsockfd, badreply, brlen);
            }
            else if (twoStage == 1) {
                if (stage == 0) {
                    n = write(newsockfd, reply2, rlen2);
                    stage = 1;
                }
                else if (stage ==1) {
                    n = write(newsockfd, reply, rlen);
                }
            }
            else {
                /* just send back our standard '200 OK' response */
                n = write(newsockfd, reply, rlen);
            }

            if (n < 0) {
                perror("Error writing to socket.");
            }
        }
        close(newsockfd);
    }

    /* finally.. */
    close(sockfd);
}


int ism_admin_getLDAPDN ( ismLDAPConfig_t *ldapobj,
        const char* username, int username_len,
        char ** pDN, int * DNLen, bool isGroup, int *dnInHeap)
{
    return ISMRC_OK;
}

ismLDAPConfig_t *ism_security_getLDAPConfig(void) {
    return NULL;
}

