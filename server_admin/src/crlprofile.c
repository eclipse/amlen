/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#define TRACE_COMP Admin

#include <curl/curl.h>
#include <errno.h>
#include <sys/wait.h>

#include <crlprofile.h>
#include <janssonConfig.h>
#include <validateInternal.h>

#define SAME_NAME_OVERWRITE    99

extern int ism_config_setApplyCertErrorMsg(int rc, char * item);
extern ism_config_callback_t ism_config_getCallback(ism_ConfigComponentType_t comptype);
extern pthread_rwlock_t srvConfiglock;

typedef int (*ism_transport_revokeConnectionsEndpoint_f)( const char * endpoint);
static ism_transport_revokeConnectionsEndpoint_f revokeConnectionsEndpoint_f = NULL;

typedef struct ism_crlTimerData_t {
    char *      endpointName;
    char *      crlProfileName;
    time_t      lastCheckTime;
    ism_timer_t key;
    struct ism_crlTimerData_t *next;
} ism_crlTimerData_t;

ism_crlTimerData_t * timerData = NULL;

/*
 * Send a CURL request to the URL
 * NOTE: Do not call ism_common_setError() to set error string in this function. ISMRC errors are
 * set in the caller function.
 * ism_common_setErrorData() is called in this function to set cURL errors.
 * This function does not user certificates or any form of user authentication
 */
static size_t crl_write_header_callback(void * ptr, size_t size, size_t nmemb, void * data)
{
  FILE *writehere = (FILE *)data;
  return fwrite(ptr, size, nmemb, writehere);
}

XAPI int ism_config_sendCRLCurlRequest(char * url, char * destFilePath, int * rc) {

    CURLcode cRC = CURLE_OK;
    FILE *headerfile = NULL;
    FILE *destFile = NULL;
    char *bufferp = NULL;
    size_t bufLen = 0;

    /* Initialize curl */
    CURL *curl = curl_easy_init();

    if (curl == NULL) {
        TRACE(3, "Failed to initialize curl.\n");
        /* curl_easy_init() can fail only if system is out of memory */
        *rc = ISMRC_AllocateError;
        ism_common_setError(*rc);
        goto mod_exit;
    }

    cRC = curl_easy_setopt(curl, CURLOPT_URL, url);
    /* enable when https is supported
    if (strstr(url,"https")) {
    	//cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    	//cRC |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }*/

    /* disable progress meter */
    cRC |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    /* complete connection within 10 seconds */
    cRC |= curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    if (cRC != CURLE_OK) {
        TRACE(3, "Failed to configure curl options.\n");
        *rc = ISMRC_CRLServerError;
        ism_common_setErrorData(*rc, "%d", cRC);
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

    destFile = fopen(destFilePath, "w");
    if (!destFile) {
    	TRACE(3, "could not create file on destination path %s, errno=%d\n", destFilePath, errno);
    	*rc = ISMRC_Error;
    	ism_common_setError(*rc);
    	goto mod_exit;
    }

    /*Set header function callback to write header data*/
    cRC = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, crl_write_header_callback);
    /* we want the headers be written to this file handle */
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEHEADER, headerfile);
    /* Set the body response function callback to write response(body) data*/
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    /* Write the response into internal buffer*/
    cRC |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, destFile);

    if (cRC != CURLE_OK) {
        TRACE(3, "Failed to configure curl options.\n");
        *rc = ISMRC_CRLServerError;
        ism_common_setErrorData(*rc, "%d", cRC);
        goto mod_exit;
    }

    /* get it! */
    cRC = curl_easy_perform(curl);
    if(cRC != CURLE_OK) {
        TRACE(3, "curl_easy_perform() failed: %s\n", curl_easy_strerror(cRC));
        *rc = ISMRC_CRLServerError;
        ism_common_setErrorData(*rc, "%d", cRC);
        goto mod_exit;
    }

    /* close the header file */
    fflush(headerfile);

    if (bufferp != NULL && bufLen > 0 && strstr(bufferp, "200 OK") ) {
        TRACE(7, "CRL file downloaded successfully\n");
    } else {
        TRACE(3, "CRL server HTTP response header returned error: %s\n", bufferp);
        *rc = ISMRC_HTTP_BadRequest;
        ism_common_setError(*rc);
    }

mod_exit:
    if (headerfile != NULL) {
        fclose(headerfile);
        headerfile = NULL;
    }

    if (bufferp != NULL) {
        ism_common_free(ism_memory_admin_misc,bufferp);
        bufferp = NULL;
    }

    if (curl != NULL) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }

    if (destFile)
    	fclose(destFile);

    return cRC;
}

/**
 * Deletes a CRL profile
 */
XAPI int ism_config_deleteCRLProfile(const char * crlProfileName) {

    int rc = ISMRC_OK;
    int result = 0;

    char * fileutilsPath  = IMA_SVR_INSTALL_PATH "/bin/imafileutils.sh";

    if ( !crlProfileName || *crlProfileName == '\0' ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    char *CRLDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"CRLDir");
    if ( !CRLDir || *CRLDir == '\0' ) {
        CRLDir = IMA_SVR_DATA_PATH "/data/certificates/truststore/CRL";
    }

    pid_t pid = fork();
    if (pid < 0){
        perror("fork failed");
        return ISMRC_Error;
    }
    if (pid == 0){
        execl(fileutilsPath, "imafileutils.sh", "deleteProfileDir", CRLDir, crlProfileName, NULL);
        int urc = errno;
        TRACE(1, "Unable to run imafileutils.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if (result != 0)  {
        TRACE(3, "%s: failed to delete CRLProfile (%s) files from CRL Directory: %s.\n", __FUNCTION__, crlProfileName, CRLDir);
    }
    return rc;
}

/*
 * Purges only crls associated with a given security profile
 */

XAPI int ism_config_purgeSecurityCRLProfile(const char * secProfileName) {

	int rc = ISMRC_OK;
	int result = 0;
	char * certutilsPath  = IMA_SVR_INSTALL_PATH "/bin/certApply.sh";
	if (!secProfileName || (*secProfileName == '\0')) {
		rc = ISMRC_NullPointer;
		ism_common_setError(rc);
		goto PURGE_END;
	}

    pid_t pid = fork();
    if (pid < 0){
        perror("fork failed");
        return 1;
    }
    if (pid == 0){
        execl(certutilsPath, "certApply.sh", "remove", "REVOCATION", "false", secProfileName, NULL);
        int urc = errno;
        TRACE(1, "Unable to run certApply.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if (result != 0){
        TRACE(5, "%s: call certApply.sh return error code: %d\n", __FUNCTION__, result);
        int xrc = 0;
        xrc = ism_config_setApplyCertErrorMsg(result, "CRLProfile");
        rc = xrc;
    }
PURGE_END:
	return rc;
}

static int apply_CRLToSecProfile(const char * secProfileName, const char * crlProfileName, const char * crlFileName) {
    int rc = ISMRC_OK;

    if (!secProfileName || !crlProfileName || !crlFileName) {
        rc = ISMRC_NullArgument;
        ism_common_setError(rc);
        return rc;
    }

    rc = ism_admin_ApplyCertificate("REVOCATION","apply", "false", (char *) secProfileName, (char *) crlProfileName, (char *) crlFileName);
    if (rc != ISMRC_OK) {
        TRACE(5, "%s: call msshell return error code: %d\n", __FUNCTION__, rc);
        int xrc = 0;
        xrc = ism_config_setApplyCertErrorMsg(rc, "CRLProfile");
        rc = xrc;
    }

    return rc;
}

XAPI int ism_config_updateSecurityCRLProfile(const char * securityProfile, const char * crlProfile) {
    int rc = ISMRC_OK;
    int ptype = 0;
    const char * crlSrc = NULL;

    if (securityProfile) {
        crlSrc = json_string_value(ism_config_json_getObject("CRLProfile", crlProfile, "CRLSource", 0, &ptype));
        if (strstr(crlSrc,"http://")) {
            crlSrc = strrchr(crlSrc,'/');
            crlSrc++;
        }
        rc = apply_CRLToSecProfile(securityProfile, crlProfile, crlSrc);
    }
    return rc;
}

static int callbackToSecurity(char * secProfileName) {
    int rc = ISMRC_OK;

    ism_config_callback_t secCallback;
    ism_prop_t * secProps = NULL;

    secCallback = ism_config_getCallback(ISM_CONFIG_COMP_TRANSPORT);
    secProps = ism_config_json_getObjectProperties("SecurityProfile", secProfileName, 0);
    if (secProps) {
        rc = secCallback("SecurityProfile", secProfileName, secProps, ISM_CONFIG_CHANGE_PROPS);
        if (rc != ISMRC_OK)
            ism_common_setError(rc);
    }
    else {
        rc = ISMRC_ConfigError;
        ism_common_setError(rc);
        TRACE(3, "%s: Could not call back to transport\n", __FUNCTION__);
    }

    if (secProps)
        ism_common_freeProperties(secProps);
    return rc;
}

XAPI int ism_config_updateCRLProfileForSecurity(const char * crlProfileName, const char * crlFileName) {
    int rc = ISMRC_OK;

    int ptype = 0;
    json_t * secProfObjs = NULL;
    json_t * secProfValue = NULL;
    json_t * crlProfStrObj = NULL;

    char * secProfKey = NULL;
    char *crlProfStr = NULL;

    secProfObjs = ism_config_json_getObject("SecurityProfile", NULL, NULL, 0, &ptype);
    void * secProfObjsIter = json_object_iter(secProfObjs);
    while (secProfObjsIter) {
        secProfKey = (char *) json_object_iter_key(secProfObjsIter);
        secProfValue = json_object_iter_value(secProfObjsIter);

        crlProfStrObj = json_object_get(secProfValue, "CRLProfile");
        if ( crlProfStrObj ) crlProfStr = (char *)json_string_value(crlProfStrObj);
        if ( crlProfStr && *crlProfStr != '\0' && !strcmp(crlProfStr, crlProfileName)) {
            rc = apply_CRLToSecProfile(secProfKey, crlProfileName, crlFileName);
            /* do callback to transport as a security profile change to update underlying security context for associated endpoints */
            if (rc == ISMRC_OK) {
                rc = callbackToSecurity(secProfKey);
            }
        }
        if (rc != ISMRC_OK)
            return rc;

        secProfObjsIter = json_object_iter_next(secProfObjs, secProfObjsIter);
    }
    return rc;
}

/* We assume the underlying security context for this endpoint has been updated. There is a possibility that the timerData does not reflect this
 * so the crlprofile could be NULL but that's okay as it will get updated next time it's run.
 */
XAPI int ism_admin_executeCRLRevalidateOptionForSecurityProfile(const char * secProfileName) {
    int rc = ISMRC_OK;

    int ptype = 0;
    json_t * enabledObj = NULL;
    json_t * secProfObj = NULL;

    int adminEndpoint = 0;

    ism_crlTimerData_t *crlTimerData = timerData;
    if (!crlTimerData) {
        return rc;
    }

    while (crlTimerData) {
        if (!strcmp("AdminEndpoint", crlTimerData->endpointName)) {
            secProfObj = ism_config_json_getObject("AdminEndpoint", crlTimerData->endpointName, "SecurityProfile", 0, &ptype);
            adminEndpoint = 1;
        } else {
            secProfObj = ism_config_json_getObject("Endpoint", crlTimerData->endpointName, "SecurityProfile", 0, &ptype);
            enabledObj = ism_config_json_getObject("Endpoint", crlTimerData->endpointName, "Enabled", 0, &ptype);
        }

        if (secProfObj && ((enabledObj && (ptype == JSON_TRUE)) || (adminEndpoint == 1))) {
            char *secProfStr = NULL;
            if (secProfObj) secProfStr = (char *)json_string_value(secProfObj);
            if (secProfStr && *secProfStr != '\0' && !strcmp(secProfStr, secProfileName)) {
                rc = ism_admin_executeCRLRevalidateOptionOneEndpoint(crlTimerData->endpointName);
            }
        }
        adminEndpoint = 0;
        crlTimerData = crlTimerData->next;
    }

    return rc;
}

/* We assume that the underlying security context of this endpoint has been updated. The timer for endpoints is always running once
 * endpoint is enabled - even after later disable calls. We only care about enabled endpoints. It is possible the timerData's crlprofile is NULL
 * because the crlprofile update happened before the timertask is even run. Not a big deal it will get updated eventually but since we have the crl
 * profile we might as well update it now.
 */
XAPI int ism_admin_executeCRLRevalidateOptionAllEndpoints(const char * crlProfileName) {
    int rc = ISMRC_OK;
    int ptype = 0;
    int adminEndpoint = 0;

    char * secProfStr = NULL;
    char * crlProfStr = NULL;

    json_t * enabledObj = NULL;
    json_t * value = NULL;

    ism_crlTimerData_t *crlTimerData = timerData;

    if (!crlTimerData) {
            return rc;
    } else {
        while(crlTimerData) {
            if (!strcmp("AdminEndpoint", crlTimerData->endpointName)) {
                value = ism_config_json_getObject("AdminEndpoint", crlTimerData->endpointName, "SecurityProfile", 0, &ptype);
                adminEndpoint = 1;
            } else {
                value = ism_config_json_getObject("Endpoint", crlTimerData->endpointName, "SecurityProfile", 0, &ptype);
                enabledObj = ism_config_json_getObject("Endpoint", crlTimerData->endpointName, "Enabled", 0, &ptype);
            }
            if (value && ((enabledObj && (ptype == JSON_TRUE)) || (adminEndpoint == 1))) {
                secProfStr = (char *) json_string_value(value);
                if (secProfStr && (*secProfStr != '\0')) {
                    value = ism_config_json_getObject("SecurityProfile", secProfStr, "CRLProfile", 0, &ptype);
                    if (value) {
                        crlProfStr = (char *) json_string_value(value);
                        if (crlProfStr && (*crlProfStr != '\0') && !strcmp(crlProfStr, crlProfileName)) {
                            if (crlTimerData->crlProfileName) {
                                ism_common_free(ism_memory_admin_misc,crlTimerData->crlProfileName);
                            }
                            crlTimerData->crlProfileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),crlProfileName);
                            rc = ism_admin_executeCRLRevalidateOptionOneEndpoint(crlTimerData->endpointName);
                        }
                    }
                }
            }
            adminEndpoint = 0;
            crlTimerData = crlTimerData->next;
        }
    }
    return rc;
}

/* We assume the underlying security context has been updated. There is a possibility the timerData's crlprofile field for this endpoint is NULL
 * since the update could happened before the timer task is even run. We don't care since it will get updated eventually. e
 */
XAPI int ism_admin_executeCRLRevalidateOptionOneEndpoint(const char * epname) {
	int count = 0;

	/* Re-validate connections - if configured */
	if ( revokeConnectionsEndpoint_f == NULL ) {
		revokeConnectionsEndpoint_f = (ism_transport_revokeConnectionsEndpoint_f)(uintptr_t)ism_common_getLongConfig("_ism_transport_revokeConnectionsEndpoint_fnptr", 0L);
	}

	if ( revokeConnectionsEndpoint_f ) {
		count = revokeConnectionsEndpoint_f(epname);
		TRACE(6, "Number of connections to be validated: %d  Endpoint: %s\n", count, epname);
	} else {
		TRACE(3, "CRL revoke connection funtion point is NULL\n");
	}

    return 0;
}

/**
 * Check if it is time to download CRL
 * Loop thru all crlTimerData, and check if it is time to download file for the specified CRL profile.
 * Same CRL profile may be associated with multiple endpoints
 * - Returns 1 if it is time to download new CRL file
 */
static inline int ism_admin_timeToDownloadCRL(ism_crlTimerData_t * crlTimerData, time_t curTime, int crlUpdInterval) {
	int isTime = 0;
	time_t lastProcessTime = crlTimerData->lastCheckTime;
    if ((lastProcessTime == 0 ) || ((curTime - lastProcessTime) >= crlUpdInterval)) {
    	crlTimerData->lastCheckTime = curTime;
    	isTime = 1;
    }
    return isTime;
}

/*
 * Timer Task function to get CRL file from server.
 */
static int ism_admin_CRLTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata) {

	int rc = ISMRC_OK;
	int cRC = CURLE_OK;
	char * crlSource = NULL;

	ism_crlTimerData_t *crlTimerData = (ism_crlTimerData_t *)userdata;

	if (!crlTimerData || !crlTimerData->endpointName)
		return rc;

    /*
     * Loop:
     * - check if endpoint is valid
     * - get security profile name configured for the endpoint
     * - get CRL profile name for the security profile
     * - get source of the CRL profile, if source is URL:
     *   - Get update interval for CRL profile
     *   - Compare current time with lastCheckTime and get CRL file from remote server
     */

	int ptype = 0;
	int adminEndpoint = 0;
	json_t *epObj = NULL;
	json_t *enabledObj = NULL;
	json_t *secProfObj = NULL;
	json_t *crlProfStrObj = NULL;
	json_t *crlProfObj = NULL;
	json_t *crlSrcObj = NULL;
	json_t *crlUpdObj = NULL;
	json_t *crlRevalObj = NULL;

	char *epname = NULL;
	int epnameLen = 0;

	/* get config lock */
	pthread_rwlock_wrlock(&srvConfiglock);


	/* make copy of endpoint name in case we have to free crltimer */
	epnameLen = strlen(crlTimerData->endpointName);
    epname = alloca(epnameLen+1);
    strcpy(epname, crlTimerData->endpointName);
    epname[epnameLen] = '\0';

	/* check if we are admin endpoint */
	if (!strcmp("AdminEndpoint", crlTimerData->endpointName)) {
	    epObj = ism_config_json_getObject("AdminEndpoint", crlTimerData->endpointName, NULL, 0, &ptype);
	    adminEndpoint = 1;
	} else {
	    /* get standard endpoint object */
	    epObj = ism_config_json_getObject("Endpoint", crlTimerData->endpointName, NULL, 0, &ptype);
	}

	if (epObj) {
	    /* get enabled and security profile objects of the endpoint */
	    if (adminEndpoint != 1) {
	        enabledObj = json_object_get(epObj, "Enabled");
	    }
		secProfObj = json_object_get(epObj, "SecurityProfile");
		if ((enabledObj && json_typeof(enabledObj) == JSON_TRUE) || (adminEndpoint == 1)) {
			/* check security profile */
			char *secProfStr = NULL;
			if (secProfObj ) secProfStr = (char *)json_string_value(secProfObj);
			if (secProfStr && (*secProfStr != '\0')) {
				/* get crl profile name for the security profile */
				char *crlProfStr = NULL;
				crlProfStrObj = ism_config_json_getObject("SecurityProfile", secProfStr, "CRLProfile", 0, &ptype);
				if (crlProfStrObj) crlProfStr = (char *)json_string_value(crlProfStrObj);
				if (crlProfStr && (*crlProfStr != '\0')) {
					/* check if crl profile is valid */
					crlProfObj = ism_config_json_getObject("CRLProfile", crlProfStr, NULL, 0, &ptype);
					if (crlProfObj ) {
						/* check if CRL profile is already assigned to this EP timer */
						if (crlTimerData->crlProfileName == NULL ) {
						    crlTimerData->crlProfileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),crlProfStr);
						} else {
							if (strcmp(crlProfStr, crlTimerData->crlProfileName)) {
								ism_common_free(ism_memory_admin_misc,crlTimerData->crlProfileName);
								crlTimerData->crlProfileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),crlProfStr);
							}
						}

						/* get CRL source, interval and revalidate items */
						crlSrcObj = json_object_get(crlProfObj, "CRLSource");
						crlUpdObj = json_object_get(crlProfObj, "UpdateInterval");
						crlRevalObj = json_object_get(crlProfObj, "RevalidateConnection");


						if (crlSrcObj) {
							crlSource = (char *)json_string_value(crlSrcObj);
							if (crlSource && strlen(crlSource) > 7 && !strncmp(crlSource, "http://", 7)) {
								/* Need to download file from remote server */
								int crlUpdInterval = 3600;
								if (crlUpdObj) crlUpdInterval = json_integer_value(crlUpdObj);
                                /* check if it is time to download crl file */
								time_t curTime = time(NULL);
								int isTime = ism_admin_timeToDownloadCRL(crlTimerData, curTime, crlUpdInterval);

								if (isTime == 1) {
									/* get file name from URL */
									char *crlFileName = strrchr(crlSource, '/');
									crlFileName++;
									int pathLength = strlen(USERFILES_DIR) + strlen(crlFileName) + 2;
									char *destFilePath = alloca(pathLength);
									snprintf(destFilePath, pathLength, "%s%s", USERFILES_DIR, crlFileName);
									cRC = ism_config_sendCRLCurlRequest((char *)crlSource, destFilePath, &rc);
									if (rc != ISMRC_OK) {
										TRACE(9, "cURL call to get file %s returned error: %d\n", crlSource, cRC);
										unlink(destFilePath);
										goto TIMER_TASK_END;
									}

									rc = ism_admin_ApplyCertificate("REVOCATION", "staging", "true", crlProfStr, crlFileName, NULL);
									if (rc == SAME_NAME_OVERWRITE)
									    rc = ISMRC_OK;
									if (rc != ISMRC_OK) {
								        TRACE(5, "%s: apply certificate returned error code: %d\n", __FUNCTION__, rc);
								        int xrc = 0;
								        xrc = ism_config_setApplyCertErrorMsg(rc, "CRLProfile");
								        rc = xrc;
								        if ( rc == 6184 )
								            ism_common_setErrorData(rc, "%s", crlFileName);
								        goto TIMER_TASK_END;
									}

									rc = apply_CRLToSecProfile(secProfStr, crlProfStr, crlFileName);
									if (rc != ISMRC_OK)
									    goto TIMER_TASK_END;
									TRACE(3,"New CRL File downloaded from url for CRLProfile: %s, Endpoint: %s\n", crlProfStr, crlTimerData->endpointName);
									/* Revalidate connections - if configured */
								    if (crlRevalObj && json_typeof(crlRevalObj) == JSON_TRUE) {
								    	rc = ism_admin_executeCRLRevalidateOptionOneEndpoint(crlTimerData->endpointName);
								    }
								}
							}
						}
					}
				}
			}
		}
	} else {
	    /* Endpoint got deleted at some point or crlprofile isn't valid for this endpoint i.e. like MQConnectivityEndpoint*/
	    rc = -1;
	}

TIMER_TASK_END:
	pthread_rwlock_unlock(&srvConfiglock);
	if ((rc == ISMRC_OK) || (cRC == CURLE_COULDNT_CONNECT)) {
	    if (cRC == CURLE_COULDNT_CONNECT)
	        TRACE(3,"Unable to establish connection to CRL Server\n"); /* don't fail due to intermittent connection problems */
	    rc = -1;
	} else if (rc == -1) {
	    TRACE(3, "Endpoint %s not found in configuration database. May have been deleted at some point or a not a tcp endpoint. No need for CRL timer. Canceling timer.\n", epname);
	    ism_config_deleteEndpointCRLTimer(epname);
	    rc = 0;
	}
	else {
	    TRACE(3,"An error occurred processing CRL for endpoint: %s. Canceling timer.\n", epname);
	    ism_config_deleteEndpointCRLTimer(epname);
	    rc = 0;
	}

	return rc;
}

/**
 * Start timer for CRL file download - one timer per endpoint
 * - Start timer for all endpoint
 * - Timer function will check endpoint configuration and cancel timer task if not required.
 */
XAPI int ism_config_startEndpointCRLTimer(const char * epname) {
	int rc = ISMRC_OK;
    int delay = 30;
    int interval = 300;  /* minimum allowed for CRL update */
    ism_crlTimerData_t *crlTimerData = NULL;

    if ( !epname || !*epname ) {
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    	return rc;
    }

    /* check for exiting endpoint timer */
    crlTimerData = timerData;
    while (crlTimerData) {
        if (!strcmp(crlTimerData->endpointName, epname)) {
            return rc;
        }
        crlTimerData = crlTimerData->next;
    }

    if (!epname) {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    } else {
    	crlTimerData = (ism_crlTimerData_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,278),sizeof(ism_crlTimerData_t), 1);
        crlTimerData->endpointName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),epname);
        crlTimerData->next = timerData;
        timerData = crlTimerData;
    	ism_timer_t crlTimerTask = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) ism_admin_CRLTimerTask, (void *) crlTimerData, delay, interval, TS_SECONDS);
		if (!crlTimerTask) {
			rc = ISMRC_NullPointer;
			ism_common_setError(rc);
			ism_common_free(ism_memory_admin_misc,crlTimerData->endpointName);
			ism_common_free(ism_memory_admin_misc,crlTimerData);
		} else {
			crlTimerData->key = crlTimerTask;
		}
    }

    return rc;
}

/**
 * Delete endpoint timer for CRL file download
 */
XAPI int ism_config_deleteEndpointCRLTimer(char * epname) {
	int rc = ISMRC_OK;
    ism_crlTimerData_t *crlTimerData = NULL;
    ism_crlTimerData_t *privTimerData = NULL;

    if ( !epname || !*epname ) {
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    	return rc;
    }

    TRACE(5, "Canceling CRL timer for endpoint %s\n", epname);

    /* check for exiting endpoint timer */
    crlTimerData = timerData;
    while (crlTimerData) {
        if (!strcmp(crlTimerData->endpointName, epname)) {
        	break;
        }
        privTimerData = crlTimerData;
        crlTimerData = crlTimerData->next;
    }

	if (crlTimerData) {
		if (privTimerData)
			privTimerData->next = crlTimerData->next;
		else
			timerData = crlTimerData->next;

        /* cancel timer */
    	ism_common_cancelTimer(crlTimerData->key);

		ism_common_free(ism_memory_admin_misc,crlTimerData->endpointName);
		ism_common_free(ism_memory_admin_misc,crlTimerData);

		TRACE(3, "CRL timer for endpoint %s is canceled.\n", epname);
	}

    return rc;
}


