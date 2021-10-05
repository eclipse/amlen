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

#define TRACE_COMP Security

#include <security.h>
#include <dirent.h>
#include <ismrc.h>
#include <ismutil.h>
#include <errno.h>
#include <ctype.h>
#include <config.h>
#include <authentication.h>
#include <securityInternal.h>
#include <ldaputil.h>
#include <string.h>
#include <transport.h>
#include <curl/curl.h>

extern ismLDAPConfig_t *ism_security_getLDAPConfig(void);
extern int ism_admin_getLDAPDN ( ismLDAPConfig_t *ldapobj, const char* username, int username_len, char ** pDN, int * DNLen, bool isGroup, int *dnInHeap);
extern ism_transport_t * ism_security_getTransFromSecContext(ismSecurity_t *sContext);
extern int ism_security_context_setOAuthExpirationTime(ismSecurity_t *sContext, unsigned long tmval);
extern int ism_security_context_setOAuthGroup(ismSecurity_t *sContext, char *group);
extern int ismCUNITEnv;

typedef char * (*transportAllocBytes_f)(ism_transport_t * transport, int len, int align);
static transportAllocBytes_f transportAllocBytes = NULL;

ism_threadkey_t curlHandleTLSKey;

static size_t oauth_write_boday_callback (void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;

  concat_alloc_t * resBuf = (concat_alloc_t * )data;

  ism_common_allocBufferCopyLen(resBuf, ptr, realsize);

  return realsize;
}


static size_t oauth_write_header_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
  FILE *writehere = (FILE *)data;
  return fwrite(ptr, size, nmemb, writehere);
}

/* Send a CURL request to the URL
 * NOTE: Do not call ism_common_setError() to set error string in this function. ISMRC errors are
 * set in the caller function.
 * ism_common_setErrorData() is called in this function to set cRUL errors.
 */
static int sendCurlRequest(char *url, char *header, char *trustStoreName, char *fullTrustStorePath, int checkServerCert, char **buf, int *rc, CURL *curl, int securedConnection, int sendPost, char *acctoken, char *userName, char *userPassword) {
    CURLcode cRC = CURLE_OK;
    FILE *headerfile = NULL;
    char *bufferp = NULL;
    size_t bufLen = 0;
    int resBufLen = 1024;
    char resmpbuf[resBufLen];
    concat_alloc_t resBuf = { resmpbuf, resBufLen, 0, 0, 0 };
    char *ubuffer = NULL;
    struct curl_slist *headers = NULL;

    memset(resmpbuf, 0, resBufLen);

    if (curl == NULL) {
        TRACE(3, "Failed to initialise curl.\n");
        /* curl_easy_init() can fail only if system is out of memory */
        *rc = ISMRC_AllocateError;
        ism_common_setError(*rc);
        goto mod_exit;
    }

    cRC = curl_easy_setopt(curl, CURLOPT_URL, url);

    cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    if ( securedConnection == 1 &&  checkServerCert != ismSEC_SERVER_CERT_DISABLE_VERIFY ) {
		cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }

    /* Set POST method */
    if (sendPost == 1) {
            cRC |= curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (acctoken != NULL) {
                cRC |= curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(acctoken));
                cRC |= curl_easy_setopt(curl, CURLOPT_POSTFIELDS, acctoken);
            }
    }

    /* Set username/password */
    if (userName != NULL) {
        cRC |= curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
        cRC |= curl_easy_setopt(curl, CURLOPT_USERNAME, userName);
        cRC |= curl_easy_setopt(curl, CURLOPT_PASSWORD, userPassword);
    }

    /* disable progress meter */
    cRC |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    /*SSL Certificate is existed. set option for SSL*/
    if(trustStoreName!=NULL && checkServerCert == ismSEC_SERVER_CERT_TRUST_STORE ){
         cRC |= curl_easy_setopt(curl,CURLOPT_CAINFO, fullTrustStorePath);
    }
    
    /* Add Header if present */
    if ( header ) {
        headers = curl_slist_append(headers, header);
        cRC |= curl_easy_setopt(curl,CURLOPT_HTTPHEADER, headers);
    }

    if (cRC != CURLE_OK) {
        TRACE(3, "Failed to configure curl options.\n");
        *rc = ISMRC_OAuthServerError;
        ism_common_setErrorData(ISMRC_OAuthServerError, "%d", cRC);
        goto mod_exit;
    }

    /* open the files */
    headerfile = open_memstream(&bufferp, &bufLen);
    if (headerfile == NULL) {
        TRACE(3, "open_memstream() failed. errno=%d\n", errno);
        *rc = ISMRC_Error;
        ism_common_setError(*rc);
        goto mod_exit;
    }

    /*Set header function callback to write header data*/
    cRC = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, oauth_write_header_callback);
    /* we want the headers be written to this file handle */
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEHEADER, headerfile);
    /* Set the body response function callback to write response(body) data*/
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oauth_write_boday_callback);
    /* Write the response into internal buffer*/
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resBuf);

    if (cRC != CURLE_OK) {
        TRACE(3, "Failed to configure curl options.\n");
        *rc = ISMRC_OAuthServerError;
        ism_common_setErrorData(ISMRC_OAuthServerError, "%d", cRC);
        goto mod_exit;
    }

    /* get it! */
    cRC = curl_easy_perform(curl);
    if(cRC != CURLE_OK) {
        TRACE(3, "curl_easy_perform() failed: %s\n", curl_easy_strerror(cRC));
        *rc = ISMRC_OAuthServerError;
        ism_common_setErrorData(ISMRC_OAuthServerError, "%d", cRC);
        goto mod_exit;
    }

    /* close the header file */
    fflush(headerfile);

    if (bufferp != NULL && bufLen > 0 && strstr(bufferp, "200") ) {
        TRACE(7, "OAuth Token is validated successfully\n");

        ubuffer = (char*)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,122),resBuf.used + 1);
        if ( ubuffer == NULL ) {
               TRACE(3, "Failed to allocate memory for ubuffer.\n");
            *rc = ISMRC_AllocateError;
            ism_common_setError(*rc);
            goto mod_exit;
        }

        memcpy(ubuffer, resBuf.buf, resBuf.used);
        ubuffer[resBuf.used] = 0;

        *buf = ubuffer;

    } else {
        TRACE(3, "OAuth Token validation failed: %s\n", bufferp);
        *rc = ISMRC_OAuthInvalidToken;
        ism_common_setError(*rc);
    }

mod_exit:

    ism_common_freeAllocBuffer(&resBuf);

    if (headerfile != NULL) {
        fclose(headerfile);
        headerfile = NULL;
    }

    if (bufferp != NULL) {
        // bufferp allocated through open_memstream
        ism_common_free_raw(ism_memory_admin_misc,bufferp);
        bufferp = NULL;
    }

    if (curl != NULL) {
    	curl_easy_reset(curl);
    }

    if ( headers != NULL ) {
        curl_slist_free_all(headers);
    }

    return cRC;
}

/* OAuth validation function */
XAPI int ism_validate_oauth_token(ismSecurity_t *sContext, ismAuthToken_t * token) {
    int rc = ISMRC_OK;
    char *url = NULL;
    char *token_buffer = NULL;
    char *userinfo_buffer = NULL;
    char *clientID = NULL;
    char *encoded_token = NULL;
    char *acctoken = NULL;
    CURLcode cRC = CURLE_OK;
    char *accessToken = NULL;
    char *header = NULL;
    CURL *curl;

    /* Get transport object to get client id and set user name into the Transport object */
    ism_transport_t *tport = ism_security_getTransFromSecContext(sContext);
    char *oAuthURL = token->oauth_url;
    clientID = (char *)tport->clientID;

    if ( !oAuthURL || *oAuthURL == '\0' ) {
        TRACE(3, "Invalid OAuth configuration in the Security Context\n");
        rc = ISMRC_PropertiesNotValid;
        ism_common_setError(rc);
        goto mod_exit;
    }

    if ( !token  || token->password == NULL || token->password_len <= 0 ) {
        TRACE(3, "Invalid OAuth access token\n");
        rc = ISMRC_OAuthInvalidToken;
        ism_common_setError(rc);
        goto mod_exit;
    }

    char *userName = token->oauth_userName;
    char *userPassword = token->oauth_userPassword;
    char *userInfoUrl = token->oauth_userInfoUrl;
    char *usernameProp = token->oauth_usernameProp;
    char *keyFileName = token->oauth_keyFileName;
    char *tokenNameProp = token->oauth_tokenNameProp;
    char *fullKeyFilePath = token->oauth_fullKeyFilePath;
    int isURLsEqual = token->oauth_isURLsEqual;
    char *groupnameProp = token->oauth_groupnameProp;
    int urlProcessMethod = token->oauth_urlProcessMethod;
    int accessTokenLen = token->password_len;
    int checkServerCert = token->oauth_checkServerCert;
    int securedConnection = token->oauth_securedConnection;

    accessToken = token->password;

    /*Check if transport is in closing or closed state. If yes. set error code and return.*/
    if(UNLIKELY(tport->state != ISM_TRANST_Open)){
         rc = ISMRC_Closed;
         goto mod_exit;
    }

    /*Parse the OAuth JSON*/
    token_buffer = (char*)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,124),accessTokenLen+1);

    if (token_buffer == NULL) {
        TRACE(3,"Failed to allocate memory for the token buffer.\n");
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(token_buffer, accessToken, accessTokenLen);
    token_buffer[accessTokenLen] = 0;

    char * access_token = NULL;
    int expires_in_secs = 0;

    /* Check for JSON string */
    int isJson = 0;
    int i = 0;
    char *tokptr = token_buffer;
    for ( i=0; i<accessTokenLen; i++ ) {
        if ( *tokptr == '{' || *tokptr == '[' ) {
                isJson = 1;
                break;
        }
        tokptr++;
    }

    if ( isJson == 1 ) {
        /* Parse input string and create JSON object */
        ism_json_parse_t parseobj = { 0 };
        ism_json_entry_t ents[40];

        parseobj.ent = ents;
        parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
        parseobj.source = (char *) token_buffer;
        parseobj.src_len = accessTokenLen + 1;
        ism_json_parse(&parseobj);

        if (parseobj.rc) {
            TRACE(3, "Invalid OAuth access token\n");
            rc = ISMRC_OAuthInvalidToken;
            ism_common_setError(rc);
            goto mod_exit;
        }
        /* Get action and user */
        access_token = (char *)ism_json_getString(&parseobj, "access_token");

        if (access_token == NULL) {
            TRACE(3, "OAuth 'access_token' missing.\n");
            rc = ISMRC_OAuthInvalidToken;
            ism_common_setError(rc);
            goto mod_exit;
        }

        expires_in_secs = ism_json_getInt(&parseobj, "expires_in", 0);

        if ( expires_in_secs == 0 ) {
            TRACE(3, "OAuth 'expires_in' is not found.\n");
        }
    } else {
        access_token = token_buffer;
    }

    size_t urlLen = 0;
    
    /* Get CURL easy handle from thread local storage */
	curl = ism_common_getThreadKey(curlHandleTLSKey, NULL);
	if(!curl) {
		curl = curl_easy_init();
		ism_common_setThreadKey(curlHandleTLSKey, curl);
	}

    if (curl == NULL) {
        TRACE(3, "Failed to initialise curl.\n");
        /* curl_easy_init() can fail only if system is out of memory */
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    if ( urlProcessMethod != ISM_OAUTH_URL_PROCESS_HEADER ) {
        /* Encode access_token */
        encoded_token = curl_easy_escape(curl, access_token, strlen(access_token));

        if (encoded_token == NULL) {
            TRACE(3, "Failed to encode token.\n");
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            curl_easy_reset(curl);
            goto mod_exit;
        }

        int osrc = 0;
        if ( urlProcessMethod == ISM_OAUTH_URL_PROCESS_POST ) {
            acctoken = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,125),strlen("token") + strlen(encoded_token) + 2);
            osrc = sprintf(acctoken, "token=%s", encoded_token);
        } else {
            acctoken = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,125),strlen(tokenNameProp) + strlen(encoded_token) + 2);
            osrc = sprintf(acctoken, "%s=%s", tokenNameProp, encoded_token);
        }
        if (osrc < 0) {
            TRACE(3,"Failed to construct OAuth token\n");
            rc = ISMRC_Error;
            ism_common_setError(rc);
            curl_free(encoded_token);
            curl_easy_reset(curl);
            goto mod_exit;
        }

        ism_common_free(ism_memory_admin_misc,token_buffer);
        curl_free(encoded_token);
        curl_easy_reset(curl);

        urlLen = strlen(oAuthURL) + strlen(acctoken) + 2; // 2 for ? and \0
    } else {
        urlLen = strlen(oAuthURL) + 1;
        header = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,127),strlen(tokenNameProp) + strlen(access_token) + 2); //2 for space and \0
        sprintf(header, "%s %s", tokenNameProp, access_token); 
    }

    
    url = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,128),urlLen);

    if (url == NULL) {
        TRACE(3,"Failed to allocate memory for the OAuth URL.\n");
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    int osrc;
    int sendPost = 0;
    
    if ( urlProcessMethod == ISM_OAUTH_URL_PROCESS_HEADER ) {
        osrc = sprintf(url, "%s", oAuthURL);
    } else if ( urlProcessMethod == ISM_OAUTH_URL_PROCESS_POST ) {
        osrc = sprintf(url, "%s", oAuthURL);
        sendPost = 1;
    } else if ( urlProcessMethod == ISM_OAUTH_URL_PROCESS_NORMAL ) {
        osrc = sprintf(url, "%s?%s", oAuthURL, acctoken);
    } else if ( urlProcessMethod == ISM_OAUTH_URL_PROCESS_QUERY ) {
        osrc = sprintf(url, "%s&%s", oAuthURL, acctoken);
    } else {
        osrc = sprintf(url, "%s%s", oAuthURL, acctoken);
    }

    if (osrc < 0) {
        TRACE(3,"Failed to construct OAuth URL.\n");
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    TRACE(8, "OAuth expires_in_secs:%d URL:[%s] CheckServerCert:%d method:%d\n", expires_in_secs, url, checkServerCert, urlProcessMethod);

    cRC = sendCurlRequest(url, header, keyFileName, fullKeyFilePath, checkServerCert, &userinfo_buffer, &rc, curl, securedConnection, sendPost, acctoken, userName, userPassword);
    if ( cRC != CURLE_OK || rc != ISMRC_OK ) {
        TRACE(3, "sendCurlRequest() failed: cRC=%d rc=%d url=%s\n", cRC, rc, url);
        goto mod_exit;
    }

    TRACE(9, "Curl request: cRC=%d rc=%d. Response: %s\n", cRC, rc, userinfo_buffer?userinfo_buffer:"NULL");

    /*If UserInfo URL and Resource are NOT equal, make request for UserInfo.*/
    if ( userInfoUrl != NULL && isURLsEqual != 1 ) {
        TRACE(8, "Connect to UserInfoURL\n");

        if ( userinfo_buffer )
            ism_common_free(ism_memory_admin_misc,userinfo_buffer);
        userinfo_buffer = NULL;

        size_t userInfoURLLen = strlen(userInfoUrl) + strlen(acctoken) + 3; // +3 for ?,=, and \0

        /* do we need to resize the url buffer in order to fit the new URL? */
        if (urlLen < userInfoURLLen) {
            char *tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,130),url, userInfoURLLen);

            if (tmp == NULL) {
                TRACE(3, "Failed to allocate memory for user info URL.\n");
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            url = tmp;
        }

        if (url == NULL) {
            TRACE(3,"Failed to allocate memory for the OAuth URL.\n");
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        osrc = sprintf(url, "%s?%s", userInfoUrl, acctoken);
        if (osrc < 0) {
            TRACE(3,"Failed to construct OAuth URL.\n");
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }

        TRACE(8, "OAuth URL: %s\n", url);

        /* invoke curl calls */
        cRC = sendCurlRequest(url, NULL, keyFileName, fullKeyFilePath, checkServerCert, &userinfo_buffer, &rc, curl, securedConnection, sendPost, acctoken, userName, userPassword);
        if ( cRC != CURLE_OK || rc != ISMRC_OK ) {
            TRACE(3, "sendCurlRequest() failed for userinfo data: cRC=%d rc=%d URL=%s\n", cRC, rc, url?url:"");
            goto mod_exit;
        }
    }

    /*If user info URL is specified. Get the Groups Info and set user id.*/
    if ( userInfoUrl != NULL ) {
        if (userinfo_buffer == NULL) {
            TRACE(3, "userinfo buffer is NULL\n");
            rc = ISMRC_NullPointer;
            ism_common_setError(rc);
            goto mod_exit;
        }

        TRACE(9, "Oauth UserInfoBuf: %s\n", userinfo_buffer);

        ism_json_parse_t parseobj = { 0 };
        ism_json_entry_t ents[40];

        parseobj.ent = ents;
        parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
        parseobj.source = (char *)userinfo_buffer;
        parseobj.src_len = strlen(userinfo_buffer);

        ism_json_parse(&parseobj);

        if (parseobj.rc) {
            TRACE(3, "Invalid Response For UserInfo.\n");
            rc = ISMRC_OAuthServerError;
            ism_common_setErrorData(ISMRC_OAuthServerError, "%d", parseobj.rc);
            goto mod_exit;
        }

        char * userid= (char *)ism_json_getString(&parseobj, usernameProp);
        char * groupid = (char *)ism_json_getString(&parseobj, groupnameProp);

        if (!userid){
            TRACE(3, "Invalid Username For UserInfo. Username Property: %s.\n",usernameProp);
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            goto mod_exit;
        }

        size_t userid_len = strlen(userid);

        /* If groupid is returned by OAuth server, use groupid from OAuth server,
         * else get group information from LDAP is if LDAP is configured.
         */
        if ( groupid != NULL && *groupid != '\0' ) {

            ism_security_context_setOAuthGroup(sContext, groupid);

        } else {

            /*Construct or Get the User DN if LDAP is configured*/
            ismLDAPConfig_t * ldapConfig = ism_security_getLDAPConfig();
            if(ldapConfig!=NULL){

                if(ldapConfig->SearchUserDN==false){
                    int dn_len = ldapConfig->UserDNMaxLen + userid_len+1;
                    if(ldapConfig->UserDNMaxLen + userid_len<DN_DEFAULT_LEN){
                        token->userDNPtr = token->userDN;
                        token->userDN_inheap = 0;
                    }else{
                        token->userDNPtr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,131),dn_len);
                        if(token->userDNPtr==NULL){
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            goto mod_exit;
                        }
                        token->userDN_inheap = 1;
                    }
                    snprintf(token->userDNPtr,dn_len, "%s=%s,%s", ldapConfig->UserIdPrefix, userid, ldapConfig->UserSuffix);
                }else{
                    /*Search for DN*/
                    int tmpDNLen = DN_DEFAULT_LEN;
                    int dnInHeap=0;
                    char * tmpDN=token->userDN;

                    rc = ism_admin_getLDAPDN ( ldapConfig, userid,userid_len, &tmpDN, &tmpDNLen,false, &dnInHeap);
                    if(rc!=ISMRC_OK){
                        TRACE(6,"Failed to obtain the user DN from the Directory Server. CN: %s. RC: %d\n", userid, rc);
                        ism_common_setError(rc);
                        token->userDNPtr=NULL;
                        token->userDN_inheap=0;
                        goto mod_exit;
                    }else{
                        token->userDNPtr=tmpDN;
                        token->userDN_inheap=dnInHeap;
                    }
                }
            }
        }

        /* Use the pre-allocated memory in transport if the len of the userid is less than the current one */
        transportAllocBytes = (transportAllocBytes_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_allocBytes_fnptr", 0L);
        tport->userid = transportAllocBytes(tport, userid_len+1, 0);
        memcpy((char *) tport->userid, userid, userid_len );
        ((char *) tport->userid)[userid_len] = 0;
    }

    if ( expires_in_secs != 0 ) {
        /* Set Expiration time in Security Context
         * Since expTime is in seconds, need to convert to nanoseconds for Transport ExpireTime
         * which is in nanoseconds
         */
        ism_time_t expiry = ( expires_in_secs * 1000000000.00) + ism_common_currentTimeNanos();
        ism_security_context_setOAuthExpirationTime(sContext, expiry);
    }

mod_exit:

    if (acctoken != NULL) {
        ism_common_free(ism_memory_admin_misc,acctoken);
    }

    if (userinfo_buffer != NULL) {
        ism_common_free(ism_memory_admin_misc,userinfo_buffer);
    }

    if ( url != NULL ) {
        ism_common_free(ism_memory_admin_misc,url);
    }

    if ( rc != ISMRC_OK ) {
        char messageBuffer[256];
        /* BEAM suppression: operating on NULL */    LOG(NOTICE, Security, 6164, "%-s%-s%-s%d",
            "The connection is not authorized because OAuth authentication failed. ClientID={0}, AuthorizationToken={1}, Error={2}, RC={3}",
            clientID?clientID:"NULL", accessToken?accessToken:"NULL",
            ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), messageBuffer, 255), rc );
        rc = ISMRC_NotAuthenticated;
    }


    return rc;
}

