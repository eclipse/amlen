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
 * This file contains RESTAPI functions.
 */

#define TRACE_COMP Admin

#include "validateInternal.h"
#include "configInternal.h"
#include "ltpautils.h"

extern int modeChanged;
extern int modeChangedPrev;
extern int32_t ism_config_set_dynamic_extended(int actionType, ism_json_parse_t *json, char *inpbuf, int inpbuflen, char **retbuf);
extern int ism_admin_restapi_getProtocolsInfo(char *pname, ism_http_t *http);
extern int ism_config_json_processGet(ism_http_t *http);
extern int ism_config_json_processDelete(ism_http_t *, ism_rest_api_cb callback);
extern int ism_config_json_parseServiceImportPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_parseServiceExportPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_getUserAuthority(ism_http_t *http);
extern int ism_config_json_getFile(ism_http_t *http, char *fname);
extern int ism_config_json_getFileList(ism_http_t *http, char *service);
extern int ism_config_json_parseServiceDiagPayload(ism_http_t *http, char * component, ism_rest_api_cb callback);
extern char * ism_admin_getLicenseAccptanceTags(int *licenseStatus);
extern int ism_config_json_parseServiceImportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_parseServiceExportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback);
extern int ism_config_json_deleteClientSet(ism_http_t * http, char * object, char * subinst, int force);
extern int ism_config_json_getClientSetStatus(ism_http_t * http, char * serviceType, char * requestID);
extern int ism_admin_getResourceSetDescriptorStatus(ism_http_t *http, ism_rest_api_cb callback);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */

//The list of unsupported RESTAPI items but exists in schema
static const char * const unsupportList[] ={
		"SSHHost", "WebUIPort", "WebUIHost",
		"MgmtMemPercent", "GranuleSizeBytes", "FixEndpointInterface",
		"LogAutoTransfer", "LogConfig", "AdminMode", "InternalErrorCode"
};

int restTraceDumpFlag = 0;

/* variables to handle build order issues */
typedef int (*removeInactiveClusterMember_f)(ism_http_t *http);
removeInactiveClusterMember_f removeInactiveClusterMember = NULL;

/*
 * Check if the itemName has an associated component in schema and also return
 * the object type.
 */
int ism_config_isItemValid( char * itemName, char **component, ism_objecttype_e *objtype) {
	int rc = ISMRC_OK;
	int count = sizeof(unsupportList)/sizeof(char *);

	if (!itemName || *itemName == '\0' ) {
		rc = ISMRC_PropertyRequired;
		ism_common_setErrorData(rc, "%s%s", "Item", "null");
		return rc;
	}

	rc = ism_config_getComponent(0, itemName, component, objtype);

	/*
	 * check if the itemName is on the unsupported list
	 */
	if (rc == ISMRC_OK) {
		int i = 0;
		for ( i = 0; i < count; i++ ) {
			if (!strcmp(itemName, unsupportList[i])) {
				rc = ISMRC_BadPropertyName;
				ism_common_setErrorData(rc, "%s", itemName);
				TRACE(3, "Configuration Object: %s is not supported.\n", itemName);
			}
		}
	}
	return rc;
}

/* return 0 if not exist*/
static int isFileExist( char * filepath)
{
    if (filepath == NULL)
        return 0;

    if( access( filepath, R_OK ) != -1 ) {
        return 1;
    }
    return 0;
}

/*
 * Call ism_config_set_dynamic_extended with supported JSON string.
 */
int ism_config_restapi_callConfig( char *cmd, int actionType, ism_http_t *http) {
	int rc = ISMRC_OK;
	char *outbuf = NULL;
	int inlen = strlen(cmd);
	char *inbuf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cmd);

	/* Parse input string and create JSON object */
	ism_json_parse_t parseobj = { 0 };
	ism_json_entry_t ents[40];

	parseobj.ent = ents;
	parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
	parseobj.source = (char *) cmd;
	parseobj.src_len = strlen(cmd) + 1;
	ism_json_parse(&parseobj);
	if (parseobj.rc) {
		LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.", cmd, parseobj.rc );
		ism_common_setErrorData(6001, "%s%d", inbuf, parseobj.rc);
		rc = ISMRC_ArgNotValid;
		goto CALLCONFIG_END;
	}

	//The outbuf is only used by delete clientset.
	if (actionType == ISM_ADMIN_RUNMSSHELLSCRIPT) {
		rc = ism_admin_runMsshellScript(&parseobj, &http->outbuf);
	} else {
	    rc = ism_config_set_dynamic_extended(actionType, &parseobj, inbuf, inlen, &outbuf);
		if (outbuf)
			ism_common_allocBufferCopyLen(&http->outbuf, outbuf, strlen(outbuf));
	}


CALLCONFIG_END:
    if (outbuf)  ism_common_free(ism_memory_admin_misc,outbuf);
    if (inbuf)  ism_common_free(ism_memory_admin_misc,inbuf);
	TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

int ism_config_setApplyCertErrorMsg(int rc, char *item) {
	int retcode = 0;

	TRACE(3, "%s: Error code returned from apply %s object certificate is: %d\n", __FUNCTION__, item?item:"NULL", rc);
	switch (rc) {
	case 2:
		ism_common_setError(6184);
		//DISPLAY_MSG(ERROR, 6184, "", "Cannot find the certificate file specified. Make sure it has been uploaded.");
		retcode = 6184;
		break;
	case 3:
		ism_common_setError(6185);
		//DISPLAY_MSG(ERROR,6185, "", "Cannot find the key file specified. Make sure it has been uploaded.");
		retcode = 6185;
		break;
	case 4:
		ism_common_setError(6186);
		//DISPLAY_MSG(ERROR,6186, "", "The certificate already exists. Set Overwrite to true to replace the existing certificate.");
		retcode = 6186;
		break;
	case 5:
		ism_common_setError(6187);
		//DISPLAY_MSG(ERROR,6187, "", "Store the certificate into " IMA_PRODUCTNAME_FULL " keystore failed.");
		retcode = 6187;
		break;
	case 6:
		ism_common_setError(6188);
		//DISPLAY_MSG(ERROR,6188, "", "Store the key file into " IMA_PRODUCTNAME_FULL " keystore failed.");
		retcode = 6188;
		break;
	case 7:
		ism_common_setError(6189);
		//DISPLAY_MSG(ERROR,6189, "", "The certificate and key file do not match.");
		retcode = 6189;
		break;
	case 8:
		ism_common_setError(6190);
		//DISPLAY_MSG(ERROR,6190, "", "An error occurred while verifying the key file.  Error returned: unable to load Private Key.");

		retcode = 6190;
		break;
	case 9:
		ism_common_setError(6191);
		//DISPLAY_MSG(ERROR,6191, "", "The key file specified has a password. need input \"KeyFilePassword\".");
		retcode = 6191;
		break;
	case 10:
		ism_common_setError(6192);
		//DISPLAY_MSG(ERROR,6192, "", "The certificate file might be in an unexpected format, or the certificate password might be incorrect or missing.");
		retcode = 6192;
		break;
	case 11:
		ism_common_setError(6193);
		//DISPLAY_MSG(ERROR,6193, "", "The key file might be in an unexpected format, or the key password might be incorrect or missing.");
		retcode = 6193;
		break;
	case 13:
		ism_common_setError(6194);
		//DISPLAY_MSG(ERROR,6194, "", "The certificate file specified has a password. Need input \"CertFilePassword\".");
		retcode = 6194;
		break;
	case 14:
		ism_common_setError(6180);
		//DISPLAY_MSG(ERROR,6180, "", "The certificate failed the verification.");
		retcode = 6180;
		break;
	case 15:
		ism_common_setError(6181);
		//DISPLAY_MSG(ERROR,6181, "", "The certificate must be in PEM format.");
		retcode = 6181;
		break;
	case 16:
		ism_common_setError(6182);
		//DISPLAY_MSG(ERROR,6182, "", "The certificate must be in PEM format.");
		retcode = 6182;
		break;
	case 17:
		ism_common_setError(6183);
		//DISPLAY_MSG(ERROR,6183, "", "The certificate is not a CA certificate.");

		retcode = 6183;
		break;
	case 12:
	default:
		ism_common_setError(ISMRC_Error);
		retcode = ISMRC_Error;
		break;
	}
	return retcode;
}

//find out ClientID and SubName from the path string ClientID/SubName
//ClientID can be 1024 with any characters.
static void findSubParams(char *substr, char **SubName) {
	char *p, *ptr = NULL;

	if (!substr)
		return;

	ptr = strchr(substr, '/');
	if (ptr == NULL)
		return;
    *ptr = '\0';

	p = ptr +1;
	if ( p != NULL && *p != '\0' ) {
	   *SubName = p;
	}

	return;
}



/* Handle RESTAPI apply LTPA certificate
 *
 * @param  name         The LTPAProfile name
 * @param  keyfile      The ltpa certificate
 * @param  pwd          The ltpa certificate password
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyLTPACert(const char *name, char *keyfile, char *pwd, int overwrite) {
	char * fileutilsPath  = IMA_SVR_INSTALL_PATH "/bin/imafileutils.sh";
    int rc = ISMRC_OK;
    const char *orikey = NULL;
    int profile_found = 0;
    int rollback = 0;


    if (keyfile && pwd) {
        int clen = strlen(USERFILES_DIR) + strlen(keyfile) + 1;
        char *keyfilepath = alloca(clen);

        snprintf(keyfilepath, clen, "%s%s", USERFILES_DIR, keyfile);
        if ( !isFileExist(keyfilepath) ) {
        	TRACE(3, "%s: LTPA Key file: %s cannont be found.\n", __FUNCTION__, keyfilepath);
        	rc = 6185;
        	ism_common_setError(rc);
            goto LTPACERT_END;
        }

        /* need to run perl to convert keyfile*/
        pid_t pid = fork();
        if (pid < 0){
            perror("fork failed");
            return ISMRC_Error;
        }
        if (pid == 0){
            execl(fileutilsPath, "imafileutils.sh", "convert", keyfilepath, NULL);
            int urc = errno;
     		TRACE(1, "Unable to run certApply.sh: errno=%d error=%s\n", urc, strerror(urc));
     		_exit(1);
        }
	    int st;
	    waitpid(pid, &st, 0);
	    int res = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
	    if (res)  {
	    	TRACE(3, "%s: failed to convert: %s into UNIX format.\n", __FUNCTION__, keyfile);
	    	rc = 6202;
	    	ism_common_setErrorData(rc, "%s", keyfile);
		    //DISPLAY_MSG(ERROR,6202, "%s", "The KeyFile \"{0}\" cannot be converted to UNIX format. Convert the file before upload.", keyfile);
	    	goto LTPACERT_END;
	    }

        ismLTPA_t *ltpakey;
        rc = ism_security_ltpaReadKeyfile(keyfilepath, (char *)pwd, &ltpakey);
        if (ltpakey == NULL || rc != ISMRC_OK ) {
        	TRACE(3, "%s: ltpa validation return code: %d\n", __FUNCTION__, rc);
        	rc = 6200;
        	ism_common_setError(rc);
            //DISPLAY_MSG(ERROR,6200, "", "LTPA KeyFile is not valid, or the KeyFile and Password do not match.");
        	goto LTPACERT_END;
        } else {
            ism_security_ltpaDeleteKey(ltpakey);
        }

        /* find object */
        json_t * instObj = ism_config_json_getComposite("LTPAProfile", name, 0);
        if ( instObj ) {
        	profile_found = 1;
        	json_t *item = json_object_get(instObj, "KeyFileName");
			if (item->type != JSON_STRING) {
				ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", "KeyFileName", "Name", "InvalidType");
				rc = ISMRC_BadOptionValue;
				goto LTPACERT_END;
			}
			orikey = json_string_value(item);
			TRACE(5, "%s: Found existing CertificateProfile %s with KeyFileName: %s\n", __FUNCTION__, name?name:"null", orikey?orikey:"null");
        }

        /* check if the keyfile name has been exist and used by other LTPAProfile*/
        char *ltpaCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"LTPAKeyStore");
        int dlen = strlen(ltpaCertDir) + strlen(keyfile) + 2;
        char *destpath = alloca(dlen);
        snprintf(destpath, dlen, "%s/%s", ltpaCertDir, keyfile);

        /* check if any ltpa profile use the same keyfile name*/


        /* If the keyfile exists in LTPAKeyStore, and LTPAProfile is associating with it */
        if (isFileExist(destpath) ) {
            if( (orikey && strcmp(orikey, keyfile)) || profile_found == 0) {
            	rc = 6203;
            	ism_common_setErrorData(rc, "%s%s%s", "KeyFileName", keyfile, "LTPAProfile");
                //DISPLAY_MSG(ERROR,6203, "%s%s%s", "The {0} \"{1}\" is used by another {2}.", "KeyFileName", keyfile, "LTPAProfile");
            	goto LTPACERT_END;
            }
            if (!overwrite) {
            	rc = 6201;
            	ism_common_setErrorData(rc, "%s", keyfile);
            	goto LTPACERT_END;
            	// DISPLAY_MSG(ERROR,6201, "%s", "The \"{0}\" already exists. Set \"Overwrite\":true to update the existing configuration.", keyfile);
            }
        }

        rollback = 1;
        //backup original keyfile
        if (orikey ) {
            int len = 0;
            len = strlen(USERFILES_DIR) + strlen(name) + 1;
            char *tmpdir = alloca(len);
            snprintf(tmpdir, len, "%s%s", USERFILES_DIR, name);
            mkdir(tmpdir, S_IRWXU);

            len = 0;
            len = strlen(ltpaCertDir) + strlen(orikey) + 2;
            char *opath = alloca(len);
            snprintf(opath, len, "%s/%s", ltpaCertDir, orikey);

            len = 0;
            len = strlen(tmpdir) + strlen(orikey) + 2;
            char *tpath = alloca(len);
            snprintf(tpath, len, "%s/%s", tmpdir, orikey);
            if (copyFile(opath, tpath) == 0) {
                unlink(opath);
            }
        }

        // we can copy the keyfile here now
        copyFile(keyfilepath, destpath);
    } else {
    	rc = 6199,
    	ism_common_setError(rc);
        //DISPLAY_MSG(ERROR,6199, "", "You must specify both a KeyFileName and a Password.");
    }

LTPACERT_END:
	if (rollback) ism_config_rollbackCertificate(name, keyfile, rc, LTPA_CERTIFICATE);

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}


/* Handle RESTAPI apply OAuth certificate
 *
 * @param  name         The OAuthProfile name
 * @param  keyfile      The OAuth certificate
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyOAuthCert(const char *name, char *keyfile, int overwrite) {
	char * fileutilsPath  = IMA_SVR_INSTALL_PATH "/bin/imafileutils.sh";
    int rc = ISMRC_OK;
    int inUsersFile = 0;
    int inOAuthDir = 0;
    char *keyfilepath = NULL;

    if (keyfile) {
        int clen = strlen(USERFILES_DIR) + strlen(keyfile) + 1;
        char *ufilepath = alloca(clen);
        snprintf(ufilepath, clen, "%s%s", USERFILES_DIR, keyfile);
        if ( !isFileExist(ufilepath) ) {
            TRACE(7, "%s: Could not find OAuth Key file: %s\n", __FUNCTION__, ufilepath);
        } else {
            inUsersFile = 1;
        }

        /* check if file exists in cert dir */
        char *keyStore = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"OAuthCertificateDir");
        if ( !keyStore ) keyStore = IMA_SVR_DATA_PATH "/data/certificates/OAuth";
        clen = strlen(keyStore) + strlen(keyfile) + 2;
        char *cfilepath = alloca(clen);
        snprintf(cfilepath, clen, "%s/%s", keyStore, keyfile);
        if ( !isFileExist(cfilepath) ) {
            TRACE(7, "%s: Could not find OAuth Key file: %s\n", __FUNCTION__, cfilepath);
        } else {
            inOAuthDir = 1;
        }

        /* check if file exists */
        if ( inUsersFile == 0 && inOAuthDir == 0 ) {
            rc = 6185;
            ism_common_setErrorData(rc, "%s", keyfile);
            goto OAUTHCERT_END;
        }

        /* check for unique keyfile */
        char *orgKeyFileName = NULL;
        json_t *opobj = json_object_get(srvConfigRoot, "OAuthProfile");
        json_t *objval = NULL;
        const char *objkey = NULL;
        json_object_foreach(opobj, objkey, objval) {
            if ( strcmp(objkey, name)) {
                json_t *keyObj = json_object_get(objval, "KeyFileName");
                if ( keyObj && json_typeof(keyObj) == JSON_STRING) {
                    char *kname = (char *)json_string_value(keyObj);
                    if ( kname && !strcmp(kname, keyfile)) {
                        rc = 6203;
                        ism_common_setErrorData(rc, "%s%s%s", "KeyFileName", keyfile, "OAuthProfile");
                        goto OAUTHCERT_END;
                    }
                }
            } else {
            	/* Retain keyfile name from existing object */
                json_t *keyObj = json_object_get(objval, "KeyFileName");
                if ( keyObj && json_typeof(keyObj) == JSON_STRING) {
                    char *tmpstr = (char *)json_string_value(keyObj);
                    if ( tmpstr && *tmpstr != '\0') {
                    	orgKeyFileName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);
                    }
                }
            }
        }

        /* check for overwite key */
        if ( overwrite == 0 && inUsersFile == 1 && inOAuthDir == 1 ) {
            rc = 6201;
            ism_common_setErrorData(rc, "%s", keyfile);
            goto OAUTHCERT_END;
        }

        /* set keyfilepath */
        if ( inUsersFile == 1 ) {
            keyfilepath = ufilepath;

            /* need to run perl to convert keyfile*/
            pid_t pid = fork();
            if (pid < 0){
                perror("fork failed");
                return ISMRC_Error;
            }
            if (pid == 0){
                execl(fileutilsPath, "imafileutils.sh", "convert", keyfilepath, NULL);
                int urc = errno;
                TRACE(1, "Unable to run certApply.sh: errno=%d error=%s\n", urc, strerror(urc));
                _exit(1);
            }
            int st;
            waitpid(pid, &st, 0);
            int res = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
            if (res)  {
                TRACE(3, "%s: failed to convert: %s into UNIX format.\n", __FUNCTION__, keyfile);
                rc = 6202;
                ism_common_setErrorData(rc, "%s", keyfile);
                goto OAUTHCERT_END;
            }

            /* copy file to cert dir */
            copyFile(ufilepath, cfilepath);

            /* delete file from usersfile dir */
            unlink(ufilepath);
        }

        /* If new key file is not same as old, then remove old file if exist */
        if ( keyfile && orgKeyFileName && strcmp(keyfile, orgKeyFileName) ) {

            clen = strlen(keyStore) + strlen(orgKeyFileName) + 2;
            char *ofilepath = alloca(clen);
            snprintf(ofilepath, clen, "%s/%s", keyStore, orgKeyFileName);
            if ( !isFileExist(ofilepath) ) {
                TRACE(7, "%s: Could not find old OAuth Key file: %s\n", __FUNCTION__, ofilepath);
            } else {
            	TRACE(7, "%s: Delete old OAuth Key file: %s\n", __FUNCTION__, ofilepath);
            	unlink(ofilepath);
            }

            ism_common_free(ism_memory_admin_misc,orgKeyFileName);

        }
    } else {
    	rc = ISMRC_PropertyRequired;
    	ism_common_setErrorData(rc, "%s%s", "KeyFileName", "null");
    }

OAUTHCERT_END:

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * This should be a temporary function which will use existing admin validation and config functions.
 * The existing functions require a single configuration JSON string listed below.
 * We should update validation function to work with configSet properties directly.
 *
 * Handle RESTAPI HTTP apply certificate actions for configuration object:
 * 	Certificate
 * 	LDAP
 * 	TrustedCertificate
 * 	ClientCertificate
 * 	QueueManagerConnection
 * 	.
 * The return result or error will be in http->outbuf
 *
 * @param  item         the delete object type
 * @param  props        the configuration properties
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyCert(const char *item, ism_prop_t *props, ism_http_t *http) {
	int rc = ISMRC_OK;
	char command[8096];
	int i = 0;
	const char *propName = NULL;
	int overwrite = 0;


	memset(command, 0, sizeof(command));
	if (!item) {
    	rc = ISMRC_BadPropertyValue;
    	ism_common_setErrorData(rc, "%s%s", "ObjectType", "null");
    	goto APPLYCERT_END;
	}

	if (!strcmpi(item, "CertificateProfile")) {
		const char *name    = ism_common_getStringProperty(props, "Certificate");
		const char *keyname = ism_common_getStringProperty(props, "Key");
		const char *certpwd = ism_common_getStringProperty(props, "CertFilePassword");
		const char *keypwd  = ism_common_getStringProperty(props, "KeyFilePassword");
		overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);

		if (!name) {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", "Certificate", "null");
			goto APPLYCERT_END;
		}
		if (!keyname) {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", "Key", "null");
			goto APPLYCERT_END;
		}

		// args for Server cert - "apply", "Server", overwrite, name, keyname, keypwd, certpwd
		sprintf(command, "{ \"Action\":\"msshell\",\"User\":\"admin\",\"ScriptType\":\"apply\", \"Command\":\"Server\", \"Arg1\":\"%s\", \"Arg2\":\"%s\", \"Arg3\":\"%s\", \"Arg4\":\"%s\", \"Arg5\":\"%s\" }",
		        	    			overwrite?"True":"False", name?name:"", keyname?keyname:"", keypwd?keypwd:"", certpwd?certpwd:"" );
		TRACE(5, "%s:REST API apply cmd: %s\n", __FUNCTION__, command);
	} else if (!strcmpi(item, "QueueManagerConnection")) {
		const char *mqsslkey   = ism_common_getStringProperty(props, "MQSSLKey");
		const char *mqstashpwd = ism_common_getStringProperty(props, "MQStashPassword");
		overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);

		if (!mqsslkey) {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", "MQSSLKey", "null");
			goto APPLYCERT_END;
		}
		if (!mqstashpwd) {
			rc = ISMRC_BadPropertyValue;
			ism_common_setErrorData(rc, "%s%s", "MQStashPassword", "null");
			goto APPLYCERT_END;
		}
		overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);

    	// args for MQ cert - "apply", "MQ", overwrite, mqsslkey, mqstashpwd
    	sprintf(command, "{ \"Action\":\"msshell\",\"User\":\"admin\",\"ScriptType\":\"apply\", \"Command\":\"MQ\", \"Arg1\":\"%s\", \"Arg2\":\"%s\", \"Arg3\":\"%s\" }",
    			overwrite?"True":"False", mqsslkey?mqsslkey:"", mqstashpwd?mqstashpwd:"" );
    	TRACE(5, "%s:REST API apply cmd: %s\n", __FUNCTION__, command);
	} else if (!strcmpi(item, "LDAP")) {
		const char *ldapcert    = ism_common_getStringProperty(props, "Certificate");
		overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);
		if (!ldapcert || !*ldapcert) {
			goto APPLYCERT_END;
		}
		overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);

		// args for LDAP cert - "apply", "LDAP", overwrite, ldapcert
    	sprintf(command, "{ \"Action\":\"msshell\",\"User\":\"admin\",\"ScriptType\":\"apply\", \"Command\":\"LDAP\", \"Arg1\":\"%s\", \"Arg2\":\"%s\" }",
    			overwrite?"True":"False", ldapcert?ldapcert:"" );
    	TRACE(5, "%s:REST API apply cmd: %s\n", __FUNCTION__, command);
	} else if (!strcmpi(item, "TrustedCertificate")) {
		const char *trustedcert = NULL;
		const char *secprofile  = NULL;

		for (i=0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
			if (!propName)
				continue;

			//skip the properties added by init_newCompositeProps
			if (!strcmp(propName, "Type") || !strcmp(propName, "Component") || !strcmp(propName, "Item"))
				continue;

			const char *propValue = ism_common_getStringProperty(props, propName);
			if ( !propValue || !*propValue ) {
				if (!strcmpi(propName, "TrustedCertificate")) {
					rc = ISMRC_BadPropertyValue;
					ism_common_setErrorData(rc, "%s%s", "TrustedCertificate", "null");
					goto APPLYCERT_END;
				} else if (!strcmpi(propName, "SecurityProfileName")) {
					rc = ISMRC_BadPropertyValue;
					ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "null");
					goto APPLYCERT_END;
				}
			}

            if (!strcmpi(propName, "TrustedCertificate")) {;
        	    trustedcert = propValue;
            } else if (!strcmpi(propName, "SecurityProfileName")) {
        	    secprofile = propValue;
            }else if (!strcmpi(propName, "Overwrite")) {
            	overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);
             }else {
             	TRACE(3, "%s: the property: %s does not supported by: %s\n", __FUNCTION__, propName, item);
             	rc = ISMRC_ArgNotValid;
             	ism_common_setErrorData(rc, "%s", propName);
             	goto APPLYCERT_END;
             }
        }


		rc = ism_config_validate_CheckItemExist("Transport", "SecurityProfile", (char *)secprofile );
		if (rc) {
			TRACE(3, "%s: validate SecurityProfile: %s return error code: %d\n", __FUNCTION__, secprofile, rc);
			ism_common_setError(rc);
			goto APPLYCERT_END;
		}

    	// args for Trusted cert - "apply", "TRUSTED", overwrite, secprofile, trustedcert
    	sprintf(command, "{ \"Action\":\"msshell\",\"User\":\"admin\",\"ScriptType\":\"apply\", \"Command\":\"TRUSTED\", \"Arg1\":\"%s\", \"Arg2\":\"%s\", \"Arg3\":\"%s\" }",
    			overwrite?"True":"False", secprofile?secprofile:"", trustedcert?trustedcert:"" );
    	TRACE(5, "%s:REST API apply cmd: %s\n", __FUNCTION__, command);

	}  else if (!strcmpi(item, "ClientCertificate")) {
        const char *clntcert = NULL;
        const char *secprofile  = NULL;

        for (i=0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
            if (!propName)
                continue;

            //skip the properties added by init_newCompositeProps
            if (!strcmp(propName, "Type") || !strcmp(propName, "Component") || !strcmp(propName, "Item"))
                continue;

            const char *propValue = ism_common_getStringProperty(props, propName);
            if ( !propValue || !*propValue ) {
                if (!strcmpi(propName, "CertificateName")) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", "ClientCertificate", "null");
                    goto APPLYCERT_END;
                } else if (!strcmpi(propName, "SecurityProfileName")) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "null");
                    goto APPLYCERT_END;
                }
            }

            if (!strcmpi(propName, "CertificateName")) {;
                clntcert = propValue;
            } else if (!strcmpi(propName, "SecurityProfileName")) {
                secprofile = propValue;
            } else if (!strcmpi(propName, "Overwrite")) {
                overwrite = ism_common_getBooleanProperty(props, "Overwrite", 1);
            } else {
                TRACE(3, "%s: the property: %s does not supported by: %s\n", __FUNCTION__, propName, item);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", propName);
                goto APPLYCERT_END;
            }
        }

        rc = ism_config_validate_CheckItemExist("Transport", "SecurityProfile", (char *)secprofile );
        if (rc) {
            TRACE(3, "%s: validate SecurityProfile: %s return error code: %d\n", __FUNCTION__, secprofile, rc);
            ism_common_setError(rc);
            goto APPLYCERT_END;
        }

        // args for Trusted cert - "apply", "CLIENT", overwrite, secprofile, clntcert
        sprintf(command, "{ \"Action\":\"msshell\",\"User\":\"admin\",\"ScriptType\":\"apply\", \"Command\":\"CLIENT\", \"Arg1\":\"%s\", \"Arg2\":\"%s\", \"Arg3\":\"%s\" }",
                overwrite?"True":"False", secprofile?secprofile:"", clntcert?clntcert:"" );
        TRACE(5, "%s:REST API apply cmd: %s\n", __FUNCTION__, command);

	}

	rc = ism_config_restapi_callConfig( command, ISM_ADMIN_RUNMSSHELLSCRIPT, http);
	if (rc) {
		TRACE(5, "%s: call msshell return error code: %d\n", __FUNCTION__, rc);
		int xrc = 0;
		xrc = ism_config_setApplyCertErrorMsg(rc, (char *)item);
		rc = xrc;
	}

APPLYCERT_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
	return rc;
}


/*
 * Count occurrances of c in s
 */
int strCount(const char * s, char c) {
    int count = 0;
    if (s) {
        while (*s) {
            if (*s == c)
                count++;
            s++;
        }
    }
    return count;
}


/*
 * This should be a temporary function which will use existing admin validation and config functions.
 * The existing functions require a single configuration JSON string listed below.
 * We should update validation function to work with configSet properties directly.
 *
 * Handle RESTAPI HTTP delete configuration object actions.
 * The return result or error will be in http->outbuf
 *
 * @param  componet     the config object's component
 * @param  item         the delete object type
 * @param  name         the delete object name
 * @param  props        the delete options specified in query parameters
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_deleteAnItem( char *component, char *item, char *name, char *secondArg, ism_prop_t *props, ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
	char buf[1024];
	char tbuf[64];
	const char * repl[5];
    int replSize = 0;
	concat_alloc_t cmd_buf = { buf, sizeof(buf), 0, 0 };
	int actionType = ISM_ADMIN_DELETE;
	int invokeOldPath = 0;
	int mqcObject = 0;

	if (!component || !item) {
		TRACE(3, "%s: Failed the null checking. Component: %s, Item: %s \n", __FUNCTION__, component?component:"null", item?item:"null");
		rc = ISMRC_ArgNotValid;
		if (!component)
		    ism_common_setErrorData(rc, "%s", "component");
		else
			ism_common_setErrorData(rc, "%s", "Item");
		goto DELETEANITEM_END;
	} else if (!name) {
        if ( *item == 'A' && !strcmp(item, "AdminEndpoint")) {
            name = "AdminEndpoint";
        } else if ( *item == 'S' && !strcmp(item, "Syslog")) {
                name = "Syslog";
        } else if ( *item == 'L' && !strcmp(item, "LDAP")) {
            name = "ldapconfig";
        } else if ( *item == 'H' && !strcmp(item, "HighAvailability")) {
            name = "haconfig";
        } else if ( *item == 'C' && !strcmp(item, "ClusterMembership")) {
            name = "cluster";
        } else if ( *item == 'M' && !strcmp(item, "MQCertificate")) {
            name = "MQCert";
        } else if ( *item == 'R' && !strcmp(item, "ResourceSetDescriptor")) {
            name = "ResourceSetDescriptor";
        } else if ( strcmp(item, "ClientSet")) {
		    TRACE(3, "%s: Failed the null checking. Name: %s\n", __FUNCTION__, name?name:"null");
		    rc = ISMRC_PropertyRequired;
		    ism_common_setErrorData(rc, "%s%s", "Name", "null");
			repl[0] = item;
			repl[1] = "null";
			replSize = 2;
			ism_confg_rest_createErrMsg(http, rc, repl, replSize);
		    goto DELETEANITEM_END;
        }
	}

	if ( !strcmp(item, "DestinationMappinRule") || !strcmp(item, "QueueManagerConnection") ) {
	    mqcObject = 1;
	}

	ism_json_putBytes(&cmd_buf, "{ \"Action\":\"set\",\"User\":\"admin\",\"Type\":\"Composite\",\"Delete\":\"True\",\"Component\":\"");
	ism_json_putBytes(&cmd_buf, component);
	ism_json_putBytes(&cmd_buf, "\",\"Item\":\"");
	ism_json_putBytes(&cmd_buf, item);
	ism_json_putBytes(&cmd_buf, "\",\"");
	if (!strcmp(item, "Subscription")) {
		ism_json_putBytes(&cmd_buf, "Name\":\"Subscription\",\"SubscriptionName\":\"");
	    ism_json_putEscapeBytes(&cmd_buf, secondArg, (int) strlen(secondArg));
	    ism_json_putBytes(&cmd_buf, "\",\"ClientID\":\"");
	    ism_json_putEscapeBytes(&cmd_buf, name, (int) strlen(name));
	    invokeOldPath = 1;
	} else if (!strcmp(item, "MQTTClient")) {
    	ism_json_putBytes(&cmd_buf, "Name\":\"");
		ism_json_putEscapeBytes(&cmd_buf, name, (int) strlen(name));
		invokeOldPath = 1;
	} else if (!strcmp(item, "ClientSet")) {
	    char * eos = "";
		const char *clientID = ism_common_getStringProperty(props, "ClientID");
		const char *retain = ism_common_getStringProperty(props, "Retain");
		const char *wait = ism_common_getStringProperty(props, "Wait");

		uint32_t waitsec = 60;
		if (wait)
		    waitsec = strtoul(wait, &eos, 10);

		if ((!clientID && !retain) || *eos) {
			rc = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(rc, "%s", http->query);
			goto DELETEANITEM_END;
		}
		if (clientID && *clientID != '\0') {
			ism_json_putBytes(&cmd_buf, "ClientID\":\"");
			ism_json_putEscapeBytes(&cmd_buf, clientID, (int) strlen(clientID));
			if (retain && *retain != '\0') {
				ism_json_putBytes(&cmd_buf, "\",\"Retain\":\"");
				ism_json_putEscapeBytes(&cmd_buf, retain, (int) strlen(retain));
			}
		} else {
			if (retain && *retain != '\0') {
			    ism_json_putBytes(&cmd_buf, "Retain\":\"");
			    ism_json_putEscapeBytes(&cmd_buf, retain, (int) strlen(retain));
			}
		}
		if (wait) {
		    sprintf(tbuf, ",\"Wait\": %u", waitsec);
		    ism_json_putBytes(&cmd_buf, tbuf);
		}
		invokeOldPath = 1;
	} else {
		ism_json_putBytes(&cmd_buf, "Name\":\"");
		ism_json_putEscapeBytes(&cmd_buf, name, (int) strlen(name));
	}
	ism_json_putBytes(&cmd_buf, "\" }");

	//now we have a full cmd.
	//need to copy cmd to a buf for set_dynamic_extend to parse.

	int len = cmd_buf.used;
	char *inbuf = alloca(len + 1);
	memset(inbuf, 0, len+1);
	memcpy(inbuf, cmd_buf.buf, len);
	inbuf[len] = '\0';
	TRACE(5, "Command from RESP API: %s\n", inbuf);

	if ( actionType ==  ISM_ADMIN_RUNMSSHELLSCRIPT || invokeOldPath == 1 ) {
	    rc = ism_config_restapi_callConfig(inbuf, actionType, http);
	}
	if (rc) {
	    TRACE(3, "%s: Failed to delete configuration object: %s, name: %s\n", __FUNCTION__, item, name);
		repl[0] = item;
		repl[1] = name;
		replSize = 2;
		ism_confg_rest_createErrMsg(http, rc, repl, replSize);
		goto DELETEANITEM_END;
	} else {
		if (getenv("CUNIT") == NULL){
			replSize = 0;
			repl[0] = '\0';
			ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
		}

		/* Invoke API to delete from JSON object */
		rc = ism_config_json_processDelete(http, callback);
		/* Try deleting the string exactly as given to account for a slash in the name */
		if ((rc == ISMRC_NotFound || rc == ISMRC_BadRESTfulRequest) && strCount(http->user_path, '/') > 2) {
		    http->outbuf.used = 0;
		    http->val2 = 9;
		    rc = ism_config_json_processDelete(http, callback);
		    if (!rc)
		        ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
		}
		if ( rc == ISMRC_OK ) {
		    if ( mqcObject == 1 ) {
		        TRACE(5, "%s: Delete is initiated for MQConnectivity configuration object: %s, name: %s.\n", __FUNCTION__, item, name);
		    } else {
	            TRACE(5, "%s: Configuration object: %s, name: %s has been deleted successfully.\n", __FUNCTION__, item, name);
		    }
		}
	}

DELETEANITEM_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

/* Handle RESTAPI HTTP delete TrustedCertificate actions.
 * The return result or error will be in http->outbuf
 *
 * @param  certName     the TrustedCertificate name
 * @param  props        the property list has SecurityProfile query parameter specified
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_deleteTrustedCertificate(char *secProfile, char *certName, ism_http_t *http) {
	char * certutilsPath  = IMA_SVR_INSTALL_PATH "/bin/certApply.sh";
    int rc = ISMRC_OK;
	char buf[2048];
    concat_alloc_t cmd_buf = { buf, sizeof(buf), 0, 0 };
	memset(buf, 0, sizeof(buf));
	const char * repl[5];
	int replSize = 0;
	char *item = "TrustedCertificate";

	TRACE(5, "%s: Delete TrustedCertificate: %s from SecurityProfile: %s\n", __FUNCTION__, certName, secProfile);
	if (!secProfile && !certName) {
		TRACE(3, "%s: The TrustedCertificate: %s or SecurityProfileName: %s cannot be null.\n", __FUNCTION__, certName?certName:"null", secProfile?secProfile:"null");
		rc = 6167;
		ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "TrustedCertificate");
		goto DELETE_TRUSTEDCERT;
	}

	rc = ism_config_validate_CheckItemExist("Transport", "SecurityProfile", secProfile );
	if (rc) {
		TRACE(3, "%s: validate SecurityProfile: %s return error code: %d\n", __FUNCTION__, secProfile, rc);
		ism_common_setError(rc);
		goto DELETE_TRUSTEDCERT;
	}

	int result = 0;
	pid_t pid = fork();
    if (pid < 0){
        perror("fork failed");
        return 1;
    }
    if (pid == 0){
    	execl(certutilsPath, "certApply.sh", "remove", "TRUSTED", "delete", secProfile, certName, NULL);
    	int urc = errno;
		TRACE(1, "Unable to run certApply.sh: errno=%d error=%s\n", urc, strerror(urc));
		_exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if (result == 0 ) {
        /* invoke callback on transport to update SecurityProfile */
        /* get SecurityProfile configuration */
        json_t * secProfObj = ism_config_json_getComposite("SecurityProfile", secProfile, 0);
        if (secProfObj) {
            int haSync = 0;
            int discardMsg = 0;
            ism_config_t *handle = ism_config_getHandle(ISM_CONFIG_COMP_TRANSPORT, NULL);
            rc = ism_config_json_callCallback( handle, "SecurityProfile", secProfile, secProfObj, haSync, ISM_CONFIG_CHANGE_PROPS, discardMsg);
        }

    } else {
    	TRACE(5, "%s: call certApply.sh return error code: %d\n", __FUNCTION__, result);
		int xrc = 0;
		xrc = ism_config_setApplyCertErrorMsg(result, item);
		rc = xrc;
		if ( rc == 6184 ) {
			ism_common_setErrorData(rc, "%s", certName?certName:"");
		}
    }

DELETE_TRUSTEDCERT:
    if (cmd_buf.inheap)  ism_common_free(ism_memory_alloc_buffer,cmd_buf.buf);


    if (rc) {
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	} else {
    	ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
	}
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/* Handle RESTAPI HTTP delete ClientCertificate actions.
 * The return result or error will be in http->outbuf
 *
 * @param  certName     the ClientCertificate name
 * @param  props        the property list has SecurityProfile query parameter specified
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_deleteClientCertificate(char *secProfile, char *certName, ism_http_t *http) {
    char * certutilsPath  = IMA_SVR_INSTALL_PATH "/bin/certApply.sh";
    int rc = ISMRC_OK;
    char buf[2048];
    concat_alloc_t cmd_buf = { buf, sizeof(buf), 0, 0 };
    memset(buf, 0, sizeof(buf));
    const char * repl[5];
    int replSize = 0;
    char *item = "ClientCertificate";

    TRACE(5, "%s: Delete ClientCertificate: %s from SecurityProfile: %s\n", __FUNCTION__, certName, secProfile);
    if (!secProfile && !certName) {
        TRACE(3, "%s: The ClientCertificate or SecurityProfileName cannot be null. ClientCertificate:%s SecurityProfileName:%s\n", __FUNCTION__, certName?certName:"null", secProfile?secProfile:"null");
        rc = 6167;
        ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "CertificateName");
        goto DELETE_CLINTCERT;
    }

    rc = ism_config_validate_CheckItemExist("Transport", "SecurityProfile", secProfile );
    if (rc) {
        TRACE(3, "%s: validate SecurityProfile: %s return error code: %d\n", __FUNCTION__, secProfile, rc);
        ism_common_setError(rc);
        goto DELETE_CLINTCERT;
    }

    int result = 0;
    pid_t pid = fork();
    if (pid < 0){
        perror("fork failed");
        return 1;
    }
    if (pid == 0){
        execl(certutilsPath, "certApply.sh", "remove", "CLIENT", "delete", secProfile, certName, NULL);
        int urc = errno;
        TRACE(1, "Unable to run certApply.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if (result == 0) {
        /* invoke callback on transport to update SecurityProfile */
        /* get SecurityProfile configuration */
        json_t * secProfObj = ism_config_json_getComposite("SecurityProfile", secProfile, 0);
        if (secProfObj) {
            int haSync = 0;
            int discardMsg = 0;
            ism_config_t *handle = ism_config_getHandle(ISM_CONFIG_COMP_TRANSPORT, NULL);
            rc = ism_config_json_callCallback( handle, "SecurityProfile", secProfile, secProfObj, haSync, ISM_CONFIG_CHANGE_PROPS, discardMsg);
        }

    } else {
        TRACE(5, "%s: call certApply.sh return error code: %d\n", __FUNCTION__, result);
        int xrc = 0;
        xrc = ism_config_setApplyCertErrorMsg(result, item);
        rc = xrc;
		if ( rc == 6184 ) {
			ism_common_setErrorData(rc, "%s", certName?certName:"");
		}
    }

DELETE_CLINTCERT:
    if (cmd_buf.inheap)  ism_common_free(ism_memory_alloc_buffer,cmd_buf.buf);

    if (rc) {
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	} else {
    	ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
	}

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;

}

/**
 * Get system network interfaces
 * @param http		ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
#define MAX_SSHHOST 100
int ism_admin_restapi_getInterfaces(ism_http_t *http)
{
	char **ipr = NULL;
	int count;

 	ipr = (char **)alloca(MAX_SSHHOST*sizeof(char*));

 	for (int i = 0; i < MAX_SSHHOST; i++ ) {
		ipr[i] = NULL;
	}

 	int needComma = 0;
 	// Following routine is in adminHA.c.... Do we need to move it?
 	if ( ism_admin_getIfAddresses(ipr, &count, 1) != 0 && count > 0) {
		ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
		ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
		ism_json_putBytes(&http->outbuf, "\",\n  \"Interfaces\":[");
 		for (int i=0; i<count; i++) {
 			if (needComma) ism_json_putBytes(&http->outbuf, ", ");
			ism_json_putBytes(&http->outbuf, "\"");
			ism_json_putEscapeBytes(&http->outbuf, ipr[i], (int) strlen(ipr[i]));
			ism_json_putBytes(&http->outbuf, "\"");
			needComma = 1;
 		}
		ism_json_putBytes(&http->outbuf, "]}");

 	 	for (int i = 0; i<count; i++) {
 	 		if (ipr[i]) ism_common_free(ism_memory_admin_misc,ipr[i]);
 	 	}
 	}

	return ISMRC_OK;
}


/*
 *
 * Handle RESTAPI get configuration object actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_getAction(ism_http_t *http, ism_rest_api_cb callback) {
	int retcode = ISMRC_OK;
    char *nexttoken = NULL;
    char *component = NULL;
    char buf[512];

	ism_objecttype_e objtype = ISM_INVALID_OBJTYPE;
	const char * repl[5];
	int replSize = 0;
    char *tmpptr = NULL;

    TRACE(9, "%s: user path is: %s\n", __FUNCTION__, http->user_path?http->user_path:"null");

    if (!http->user_path) {
    	TRACE(3, "%s: user path is null\n", __FUNCTION__);
		repl[0] = "user_path";
		repl[1] = http->user_path;
		replSize = 2;
    	retcode = ISMRC_BadPropertyValue;
    	ism_common_setErrorData(retcode, "%s%s", "http user path,", "null");
    	goto GETACTION_END;
    }

    tmpptr = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),http->user_path);
	char *item = strtok_r(tmpptr, "/", &nexttoken);
	char *name = NULL;
	//For singleton, name is not there
	//char *name = strtok_r(NULL, "/", &nexttoken);
	if (nexttoken && *nexttoken != '\0')
	    name = nexttoken;

	if (!item) {
		TRACE(3, "%s: No configuration object is specified in the path.\n", __FUNCTION__ );
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
		goto GETACTION_END;
	}

    if (!strcmp(item, "Subscription")) {
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
		goto GETACTION_END;
    }
	retcode = ism_config_isItemValid(item, &component, &objtype);
	if (retcode) {
		TRACE(5, "%s: failed to valid the object: %s. retcode:  %d, objtype: %d\n", __FUNCTION__, item, retcode, objtype);
		repl[0] = item;
		replSize = 1;
		goto GETACTION_END;
	}

	/* Do not allow get except for LicensedUsage object, till license is accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 && strcmp(item, "LicensedUsage")) {
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
    	goto GETACTION_END;
    }

    /* Return licensed usage */
    if ( !strcmp(item, "LicensedUsage")) {
    	char retstr[1024];
    	int acceptTag = 4;
    	char *typeStr = ism_admin_getLicenseAccptanceTags(&acceptTag);
   		ism_json_putBytes(&http->outbuf, "{\n  \"Version\":\"");
   		ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
   		ism_json_putBytes(&http->outbuf, "\",\n");
   		sprintf(retstr, "  \"LicensedUsage\": \"%s\",\n", typeStr?typeStr:"Developers");
   		ism_common_allocBufferCopyLen(&http->outbuf, retstr, strlen(retstr));
   		if ( acceptTag == 5 ) {
   	   		sprintf(retstr, "  \"Accept\": true\n");
   	   		ism_common_allocBufferCopyLen(&http->outbuf, retstr, strlen(retstr));
   		} else {
   	   		sprintf(retstr, "  \"Accept\": false\n");
   	   		ism_common_allocBufferCopyLen(&http->outbuf, retstr, strlen(retstr));
   		}
   		ism_json_putBytes(&http->outbuf, "}\n");
   		goto GETACTION_END;
    }

	// Use SERVER_SCHEMA_VERSION for Angel release - revisit this again in next release
	if(!strcmp(item,"AdminMode") && modeChanged==1) {
		memset(buf, 0, 512);
		sprintf(buf, "{ \"Version\":\"%s\",\"AdminMode\":\"%d\" }", SERVER_SCHEMA_VERSION, modeChangedPrev);
		ism_common_allocBufferCopyLen(&http->outbuf, buf, strlen(buf));
		retcode=ISMRC_OK;
		goto GETACTION_END;
	}

	if(!strcmp("Protocol", item)){
		/*Protocol configuration is keep in memory*/
		retcode = ism_admin_restapi_getProtocolsInfo(name, http);
	} else if (!strcmp("Interfaces", item)) {
		retcode = ism_admin_restapi_getInterfaces(http);
	} else if (!strcmp("AdminUserPassword", item)) {
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
	}else{
        /* Call JANSSON based API to return data */
        retcode = ism_config_json_processGet(http);
	}



GETACTION_END:

    if ( tmpptr ) ism_common_free(ism_memory_admin_misc,tmpptr);
    
	if (retcode) {
		ism_confg_rest_createErrMsg(http, retcode, repl, replSize);
	}

	if ( callback ) {
	    callback(http, retcode);
	}

	if (component) ism_common_free(ism_memory_admin_misc,component);

	TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, retcode);
    return ISMRC_OK;
}

/* Delete engine objects */
static int ism_config_deleteEngineObjects(char *item, char *clientID, char *subName) {
    int rc = ISMRC_OK;
    char *name = NULL;

    /* Get configuration handle */
    ism_config_t *handle = ism_config_getHandle(ISM_CONFIG_COMP_ENGINE, NULL);

    if ( !handle || !handle->callback) {
        rc = ISMRC_NullPointer;
        TRACE(3, "%s: Configuration handle (%p) or Callback is not set or for item:%s name:%s\n", __FUNCTION__, handle, item, name);
        ism_common_setError(rc);
        return rc;
    }

    if ( !strcmp(item, "Subscription")) {
        name = "Subscription";
    } else {
        name = clientID;
    }

    ism_prop_t *props = ism_common_newProperties(3);
    ism_field_t var = {0};
    int keyLen = strlen(item) + strlen(name) + 32;

    /* Add Name */
    char *nameKey = (char *)alloca(keyLen);
    snprintf(nameKey, keyLen, "%s.Name.%s", item, name);
    var.type = VT_String;
    var.val.s = name;
    ism_common_setProperty(props, nameKey, &var);

    /* Add ClientID */
    char *clntKey = (char *)alloca(keyLen);
    snprintf(clntKey, keyLen, "%s.ClientID.%s", item, name);
    var.type = VT_String;
    var.val.s = clientID;
    ism_common_setProperty(props, clntKey, &var);

    /* SubscriptionName if not NULL */
    if ( subName ) {
        char *subKey = (char *)alloca(keyLen);
        snprintf(subKey, keyLen, "%s.SubscriptionName.%s", item, name);
        var.type = VT_String;
        var.val.s = subName;
        ism_common_setProperty(props, subKey, &var);
    }

    /* Invoke callback with Delete action */
    rc = handle->callback(item, name, props, ISM_CONFIG_CHANGE_DELETE);
    if (rc != ISMRC_OK) {
        TRACE(3, "%s: call %s callback with name:%s, the return code is: %d\n", __FUNCTION__, item, name, rc);
        /* If the callback has set the error, then we don't need to set it here, overwriting any parameters
         * set in the callback.
         */
        if (ISMRC_OK == ism_common_getLastError())
            ism_common_setError(rc);
    }

    ism_common_freeProperties(props);
    return rc;
}

/*
 *
 * Handle RESTAPI to delete a file from /tmp/usersfile directory
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
static int ism_config_serviceFileDelete(ism_http_t *http) {
	int rc = ISMRC_OK;
    char *nexttoken = NULL;
    char *tmppath = NULL;
    int uplen = 0;

    TRACE(9, "%s: user path is: %s\n", __FUNCTION__, http->user_path?http->user_path:"null");

    if (http->user_path) {
    	uplen = strlen(http->user_path);
    }

    if ( uplen > 1 ) {
    	tmppath = (char *)alloca(uplen + 1);
    	memcpy(tmppath, http->user_path, uplen);
    	tmppath[uplen] = '\0';
    } else {
		 rc = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(rc, "%s", http->path);
    	goto FDELETE_END;
    }

	char *service = strtok_r((char *)http->user_path, "/", &nexttoken);
	char *fname = NULL;
	if (service) {
		fname = strtok_r(NULL, "/", &nexttoken);
	}

    /* validate file name - should not have ./, ../, /. or /.. */
    if ( !fname || *fname == '\0' ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        goto FDELETE_END;
    }

    if ( strstr(fname, "..") || strstr(fname, "./") || strstr(fname, "../") || strstr(fname, "/.") || strstr(fname, "/..") || strlen(fname) > 1024 ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        goto FDELETE_END;
    }

    /* Read file into a buffer */
    char filepath[2048];
    sprintf(filepath, "/tmp/userfiles/%s", fname);

    if ( !isFileExist(filepath) ) {
    	rc = ISMRC_NotFound;
    	ism_common_setError(rc);
        TRACE(9, "%s: Could not find file: %s\n", __FUNCTION__, filepath);
    } else {
    	unlink(filepath);
    }

FDELETE_END:

	return rc;
}


/*
 *
 * Handle RESTAPI delete configuration object actions
 * through http path.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 * @param  pflag        The path flag indicates the calling path.
 * 						0 -- /configuration/
 * 						1 -- /service/
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_deleteAction(ism_http_t *http, int pflag, ism_rest_api_cb callback) {
	int retcode = ISMRC_OK;
	char *token, *nexttoken = NULL;
    char *component = NULL;
    char *tmpstr = NULL;
    ism_prop_t *props = NULL;
    int mqcObjects = 0;

	ism_objecttype_e objtype = ISM_INVALID_OBJTYPE;
	const char * repl[5];
	int replSize = 0;
	int deleteItemCalled = 0;

	/* Do not allow delete if license is not accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 ) {
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
    	goto DELETEACTION_END;
    }

    TRACE(9, "%s: user path is: %s\n", __FUNCTION__, http->user_path?http->user_path:"null");

    ism_common_setError(0);

    if (!http->user_path) {
    	TRACE(3, "%s: user path is null\n", __FUNCTION__);
		repl[0] = "user_path";
		repl[1] = http->user_path;
		replSize = 2;
    	retcode = ISMRC_BadPropertyValue;
    	ism_common_setErrorData(retcode, "%s%s", "HTTP user_path,", "null");
    	goto DELETEACTION_END;
    }

    tmpstr = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),http->user_path);
	char *item = strtok_r(tmpstr, "/", &nexttoken);
	char *name = NULL;
	char *secondArg = NULL;
	char *substr = NULL;

	if (!item || *item == '\0') {
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
		goto DELETEACTION_END;
	}

	/* pflag is set for service call - only ClientSet, import/export of ClientSet, MQTTClient and Subscription are deleted
	 * using service domain.
	 */
	if (strcmp(item, "Subscription") && strcmp(item, "MQTTClient") && strcmp(item, "ClientSet") && strcmp(item, "export") && strcmp(item, "import") && strcmp(item, "file")) {
		if (pflag == 1) {
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", http->path);
			goto DELETEACTION_END;
	    }
	} else {    /* There was match to ONE of the above service call items, these should be not be requested in the config domain */
	    if (pflag == 0) {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto DELETEACTION_END;
	    }
	}

	/* Process service call to delete file from /tmp/usersfile directory */
	if ( !strcmp(item, "file")) {
		retcode = ism_config_serviceFileDelete(http);
		goto DELETEACTION_END;
	}

	if (nexttoken && *nexttoken != '\0') {
		int slen = strlen(nexttoken);
		substr = alloca(slen + 1);
		memcpy(substr, nexttoken, slen);
		substr[slen] = '\0';
	}

    if ( !strcmp(item, "MQTTClient") ) {
        name = substr;
        findSubParams(substr, &secondArg);

        if (!name || secondArg) {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto DELETEACTION_END;
        }
	} else  if ( !strcmp(item, "Subscription") ) {
	    // name - clientID
	    // secondArg - topic string
        name = substr;
        findSubParams(substr, &secondArg);

        if (!name || !secondArg) {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto DELETEACTION_END;
        }
    } else  if (!strcmp(item, "MQTTClient")) {
        // name      -- clientID
        name = substr;
        findSubParams(substr, &secondArg);
        if (!name || secondArg) {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto DELETEACTION_END;
        }

    } else if (!strcmp(item, "TrustedCertificate")) {
		//TrustedCertificate name      -- SecurityProfile
		//                   secondArg -- TrustedCertificate
		name = substr;
		findSubParams(substr, &secondArg);
		retcode = ism_config_json_processDelete(http, callback);
		if (retcode == ISMRC_OK)
			retcode = ism_config_restapi_deleteTrustedCertificate(name, secondArg, http);
	    goto DELETEACTION_END;

	} else if (!strcmp(item, "ClientCertificate")) {
        // ClientCertificate name      -- SecurityProfile
        //                   secondArg -- ClientCertificate
        name = substr;
        findSubParams(substr, &secondArg);
        retcode = ism_config_json_processDelete(http, callback);
        if (retcode == ISMRC_OK)
        	retcode = ism_config_restapi_deleteClientCertificate(name, secondArg, http);
        goto DELETEACTION_END;
	} else if (!strcmp(item, "PreSharedKey")) {
			retcode = ism_config_json_processDelete(http, callback);
			goto DELETEACTION_END;
    } else if (!strcmp(item, "QueueManagerConnection") || !strcmp(item, "DestinationMappingRule")) {
        mqcObjects = 1;
        name = substr;
    } else if (!strcmp(item, "export") || !strcmp(item, "import")) {
        name = substr;
        findSubParams(substr, &secondArg);
        if (!name || strcmp(name, "ClientSet") || !secondArg ) {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto DELETEACTION_END;
        }
    } else {
		name = substr;
	}

	//check query parameters
	/* Loop thru the query parameters*/
	char * qparam = NULL;
	nexttoken = NULL;
	props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

	char * clientID = NULL;
	char * retain = NULL;
	char * wait = NULL;
	uint32_t waitsec = 60;
	int force = 0;

	if (http->query) {
		int len = strlen(http->query);
		qparam = alloca(len + 1);
		memcpy(qparam, http->query, len);
		qparam[len] = '\0';

		for (token = strtok_r(qparam, "&", &nexttoken); token != NULL;
				token = strtok_r(NULL, "&", &nexttoken)) {
			//split token into key,value
			char *key= token;
			char *value = strstr(token, "=");
			if ( value != NULL) {
				char *p = value;
				value = value +1;
				*p = '\0';
			}
			TRACE(5, "%s: query parameter: key=%s, value=%s.\n", __FUNCTION__, key?key:"null", value?value:"null");
			if (key && *key != '\0') {
				ism_field_t f;
				f.type= VT_String;
				f.val.s = value;
				ism_common_setProperty(props, key, &f);
				if (!strcmp(key, "ClientID")) {
					clientID = value;
				} else if (!strcmp(key, "Retain")) {
					retain = value;
				} else if (!strcmp(key, "Wait")) {
				    char * eos;
				    wait = value;
				    waitsec = strtoul(wait, &eos, 10);
				    if (*eos || waitsec > 900000) {
				        retcode = ISMRC_BadRESTfulRequest;
                        ism_common_setErrorData(retcode, "%s", http->path);
                        goto DELETEACTION_END;
				    }
				} else if (!strcmp(key, "Force")) {
				    if (!strcmp(value, "true")) {
				        force = 1;
				    } else {
			            retcode = ISMRC_BadRESTfulRequest;
                        ism_common_setErrorData(retcode, "%s", http->path);
			            goto DELETEACTION_END;
				    }
				}
			}
		}
	}

	if (!strcmp(item, "export") || !strcmp(item, "import")) {
	    retcode = ism_config_json_deleteClientSet(http, item, secondArg, force);
	    if (retcode == ISMRC_OK)
	        deleteItemCalled = 1;
	    goto DELETEACTION_END;
	}
	/* LDAP can be deleted without its default name */
	if (!strcmp(item, "LDAP")) {
		if ( name ) {
			if (strcmp(name, "ldapconfig")) {
				retcode = ISMRC_BadPropertyValue;
				ism_common_setErrorData(retcode, "%s%s", "configuration object name,", name);
				goto DELETEACTION_END;
			}
		} else {
			name = "ldapconfig";
		}
	}

	if ( item ) {
		retcode = ism_config_isItemValid(item, &component, &objtype);
		if (retcode) {
			TRACE(5, "%s: failed to valid the object: %s. retcode:  %d, objtype: %d\n", __FUNCTION__, item, retcode, objtype);
			repl[0] = '\0';
			replSize = 0;
			goto DELETEACTION_END;
		}

		if (objtype == ISM_SINGLETON_OBJTYPE) {
			TRACE(5, "%s: the object: %s is a singleton, cannot be deleted\n", __FUNCTION__, item);
			retcode = ISMRC_SingltonDeleteError;
			ism_common_setErrorData(retcode, "%s", item);
			goto DELETEACTION_END;
		}

	    if ( !strcmp(item, "MQTTClient") ) {
	        retcode = ism_config_deleteEngineObjects(item, name, NULL);
	        goto DELETEACTION_END;
	    } else if ( !strcmp(item, "TopicMonitor") || !strcmp(item, "ClusterRequestedTopics")) {
	        /* Invoke API to delete from JSON object */
	        retcode = ism_config_json_processDelete(http, callback);
		} else if (!strcmp(item, "ClientSet")) {
		    if (name) {
                retcode = ISMRC_BadRESTfulRequest;
                ism_common_setErrorData(retcode, "%s", http->path);
                goto DELETEACTION_END;
		    }
			retcode = ism_config_admin_deleteClientSet(http, callback, clientID, retain, waitsec);
            callback = NULL;   /* Client Set will do its own respond */
		} else {
            retcode = ism_config_restapi_deleteAnItem(component, item, name, secondArg, props, http, callback);
            if ( mqcObjects == 1 && ( retcode == ISMRC_OK || retcode == ISMRC_AsyncCompletion )) {
                /* set callback to NULL - will be invoked asynchronously */
                callback = NULL;
                retcode = ISMRC_OK;
            }
            deleteItemCalled = 1;
		}

	} else {
		TRACE(3, "%s: No configuration object is specified in the path.\n", __FUNCTION__ );
		retcode =ISMRC_PropertyRequired;
		ism_common_setErrorData(retcode, "%s%s", "ObjectType", "null");
	}


DELETEACTION_END:

    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);

	if (retcode) {
		if ( deleteItemCalled == 0 )
		    ism_confg_rest_createErrMsg(http, retcode, repl, replSize);
		if (6197 == retcode || 6196 == retcode || 6195 == retcode) retcode = 0;
	} else {
		replSize = 0;
		repl[0] = '\0';
		if ( deleteItemCalled == 0 )
		    ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
	}

	if ( callback ) {
	    callback(http,retcode);
	}

	if ( component ) ism_common_free(ism_memory_admin_misc,component);

	if ( props ) ism_common_freeProperties(props);

	TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, retcode);
    return ISMRC_OK;
}

/*
 *
 * Handle RESTAPI upload file actions
 *
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_fileUploadAction(ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
	const char * repl[5];
    int replSize = 0;

    int useDiffDir = 0;
    const char * diffDir = NULL;
    const char * dirPath = NULL;
    char * nexttoken = NULL;
    char * content = NULL;
    int    content_len = 0;
    FILE * f = NULL;
    char * name = NULL;

	if (http->user_path ) {
	    const char *p = http->user_path;
	    if (*p == '/')
	    	p = http->user_path + 1;
	    if (strstr(p, "/")) {
	    	TRACE(3, "%s: A unsupported file name: %s has been provided.\n", __FUNCTION__, p);
			rc = ISMRC_PropertyRequired;
			repl[0] = '\0';
			ism_common_setErrorData(rc, "%s%s", "filename", p);
			goto UPLOADFILE_END;
	    }

		name = strtok_r((char *)http->user_path, "/", &nexttoken);
		if (!name || *name == '\0') {
			TRACE(3, "%s: name is required by this action.\n", __FUNCTION__);
			rc = ISMRC_PropertyRequired;
			repl[0] = '\0';
			ism_common_setErrorData(rc, "%s%s", "filename", "null");
			goto UPLOADFILE_END;
		} else {
			//don't allow to have '/', '.', '..', leading or tailing space. The file has to be in tmp/userfiles dir
			int len = strlen(name);
			if (!strcmp(name, ".") || !strcmp(name, "..") ||
					 *name == ' ' || name[len - 1] == ' ') {
				TRACE(3, "%s: A unsupported file name: %s has been provided.\n", __FUNCTION__, name);
				rc = ISMRC_PropertyRequired;
				repl[0] = '\0';
				ism_common_setErrorData(rc, "%s%s", "filename", name);
				goto UPLOADFILE_END;
			}
		}
	}

    /* Parse input string and create JSON object */
	if (http->content_count > 0) {
		content = http->content[0].content;
		content_len = http->content[0].content_len;
		if (content_len == 0) {
			rc = ISMRC_PropertyRequired;
			repl[0] = '\0';
			ism_common_setErrorData(rc, "%s%s", "upload file content,", "null");
			goto UPLOADFILE_END;
		}
	}

	/*
	 * Open the file and read it into memory, check if /tmp/userfiles exists and if we not try to create it, in the instance it
	 * somehow got deleted. Note /tmp/userfiles could be a different filesystem and the new dir which will be a part of /tmp may not
	 * provide adequate disk space but just log this action took place and hope for the best...
	 */
	if (useDiffDir == 1)
	    dirPath = diffDir;
	else
	    dirPath = TMP_USERFILES_DIR;

	if (isFileExist((char *)dirPath) == 0) {
	    int tmp = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	    if (tmp == 0) {
	        LOG(NOTICE, Admin, 6172, "%s", "The {0} directory has been created/recreated.", dirPath);
	    }
	    else {
	        rc = ISMRC_FileUpdateError;
	        ism_common_setErrorData(rc, "%s%d", dirPath, errno);
	        repl[0] = '\0';
	        goto UPLOADFILE_END;
	    }
	}

	int flen = strlen(dirPath) + strlen(name);
	char *fpath = alloca(flen + 2);
	sprintf(fpath, useDiffDir? "%s/%s":"%s%s", dirPath, name);
	f = fopen(fpath, "w");
	if (!f) {
		rc = ISMRC_Error;
		TRACE(2,"%s: Can not open destination file '%s'. rc=%d\n", __FUNCTION__, fpath, errno);
		ism_common_setError(ISMRC_Error);
		goto UPLOADFILE_END;
	}

	fwrite(content, content_len, 1, f);
	fclose(f);

UPLOADFILE_END:
    if (rc)
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	else {
		TRACE(5, "%s: file: %s has been uploaded successfully.\n", __FUNCTION__, name);
		if (getenv("CUNIT") == NULL){
			replSize = 0;
			repl[0] = '\0';
			ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
		}
	}

	callback(http,rc);

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

	return ISMRC_OK;
}

/*
 *
 * Handle RESTAPI get service status actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_serviceGetStatus(ism_http_t *http, ism_rest_api_cb callback) {
	int retcode = ISMRC_OK;
    char *nexttoken = NULL;
    char *tmppath = NULL;
    int uplen = 0;
	const char * repl[5];
	int replSize = 0;

    TRACE(9, "%s: user path is: %s\n", __FUNCTION__, http->user_path?http->user_path:"null");

    if (http->user_path) {
    	uplen = strlen(http->user_path);
    }

    if ( uplen > 1 ) {
    	tmppath = (char *)alloca(uplen + 1);
    	memcpy(tmppath, http->user_path, uplen);
    	tmppath[uplen] = '\0';
    } else {
        retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
    	goto STATUSACTION_END;
    }

	char *status = strtok_r(tmppath, "/", &nexttoken);
	char *service = strtok_r(NULL, "/", &nexttoken);
	char *extraService = NULL;
	char *nextArg = NULL;
	if (service) {
		nextArg = strtok_r(NULL, "/", &nexttoken);
	}

	/* Only allow status till license is accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 && strcmp(status, "status")) {
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
    	goto STATUSACTION_END;
    }

	if (status && !strcmp(status, "status")) {
		if ( nextArg && *nextArg != '\0' ) {
		    if (service && (!strcmp(service,"import") || !strcmp(service,"export"))) {
		        extraService = nextArg;
		        if (!strcmp(extraService, "ClientSet")) {
		            nextArg = strtok_r(NULL, "/", &nexttoken);
		            if (nextArg && *nextArg != '\0') {
		                retcode = ism_config_json_getClientSetStatus(http, service, nextArg);
		            } else {
		                retcode = ISMRC_BadRESTfulRequest;
		                ism_common_setErrorData(retcode, "%s", http->path);
		            }
		        } else {
		            retcode = ISMRC_BadRESTfulRequest;
		            ism_common_setErrorData(retcode, "%s", http->path);
		        }
		    }
		    else {
		        retcode = ISMRC_BadRESTfulRequest;
		        ism_common_setErrorData(retcode, "%s", http->path);
		    }
	    } else if (service && (!strcmp(service, "ResourceSetDescriptor"))) {
	            retcode = ism_admin_getResourceSetDescriptorStatus(http, callback);
	            callback = NULL;
		} else {
			if (!service || !strcmp(service, "Server") || !strcmp(service, "MQConnectivity") || !strcmp(service, "Plugin")
				|| !strcmp(service, "SNMP") || !strcmp(service, "Cluster") || !strcmp(service, "HighAvailability")) {
				retcode = ism_admin_getServerStatus(http, service, 0);
			} else {
				TRACE(3, "%s: the user path is not valid.\n", __FUNCTION__);
				retcode = ISMRC_BadRESTfulRequest;
				ism_common_setErrorData(retcode, "%s", service);
			}
		}
	} else if (status && !strcmp(status, "authority")) {
		if ( service || ( service && *service != '\0') ) {
			retcode = ism_config_json_getUserAuthority(http);
		} else {
			TRACE(3, "%s: the user path is not valid.\n", __FUNCTION__);
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", service);
		}
	} else if (status && !strcmp(status, "file")) {
		if ( service || ( service && *service != '\0') ) {
			retcode = ism_config_json_getFile(http, service);
		} else {
			TRACE(3, "%s: the user path is not valid.\n", __FUNCTION__);
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", service);
		}
	} else if (status && !strcmp(status, "fileList")) {
			retcode = ism_config_json_getFileList(http, service);
	} else {
        retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
	}

STATUSACTION_END:
    if (retcode)
        ism_confg_rest_createErrMsg(http, retcode, repl, replSize);

    if (callback)
        callback(http,retcode);

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, retcode);

    return ISMRC_OK;
}

/*
 *
 * Handle RESTAPI post service actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_servicePostAction(ism_http_t *http, ism_rest_api_cb callback) {
	int retcode = ISMRC_OK;
    char *nexttoken = NULL;

	const char * repl[5];
	int replSize = 0;
	char *user_path = NULL;
	int uplen = 0;
	int isTransaction = 0;

    TRACE(9, "%s: user path is: %s\n", __FUNCTION__, http->user_path?http->user_path:"null");

    if (http->user_path) {
    	uplen = strlen(http->user_path);
    }

    if ( uplen > 1 ) {
    	user_path = (char *)alloca(uplen + 1);
    	memcpy(user_path, http->user_path, uplen);
    	user_path[uplen] = '\0';
    } else {
		 retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
    	goto STATUSACTION_END;
    }

	char *request = strtok_r( user_path, "/", &nexttoken);

	if (!request || *request == '\0') {
		 retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->path);
		goto STATUSACTION_END;
	}
	
	/* Only allow license acceptance action till license is accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 && strcmp(request, "license")) {
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
    	goto STATUSACTION_END;
    }

	if (!strcmp(request, "start")) {
		request = strtok_r(NULL, "/", &nexttoken);
		if (request && *request != '\0') {
		    retcode = ISMRC_BadRESTfulRequest;
		    ism_common_setErrorData(retcode, "%s", http->path);
		    goto STATUSACTION_END;
		}
		TRACE(5, "%s: parse service payload and take start action.\n", __FUNCTION__);
		retcode = ism_config_json_parseServiceStartPayload(http);
		goto STATUSACTION_END;
	} else if (!strcmp(request, "restart")) {
		request = strtok_r(NULL, "/", &nexttoken);
		if (request && *request != '\0') {
		    retcode = ISMRC_BadRESTfulRequest;
		    ism_common_setErrorData(retcode, "%s", http->path);
		    goto STATUSACTION_END;
		}
		TRACE(5, "%s: parse service payload and take restart action.\n", __FUNCTION__);
		retcode = ism_config_json_parseServiceRestartPayload(http);
		goto STATUSACTION_END;
	} else if (!strcmp(request, "stop")) {
		request = strtok_r(NULL, "/", &nexttoken);
		if (request && *request != '\0') {
		    retcode = ISMRC_BadRESTfulRequest;
		    ism_common_setErrorData(retcode, "%s", http->path);
		    goto STATUSACTION_END;
		}
		TRACE(5, "%s: parse service payload and take stop action.\n", __FUNCTION__);
		retcode = ism_config_json_parseServiceStopPayload(http);
		goto STATUSACTION_END;
    } else if (!strcmp(request, "commit")) {
        TRACE(5, "%s: parse service payload to commit an action.\n", __FUNCTION__);
        request = strtok_r(NULL, "/", &nexttoken);
        if (!request || *request == '\0') {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto STATUSACTION_END;
        }

        if (!strcmp(request, "transaction")) {
        	isTransaction = 1;
            TRACE(5, "%s: start - commit a transaction.\n", __FUNCTION__);
            retcode = ism_config_json_parseServiceTransactionPayload(http, ISM_ADMIN_COMMIT, callback);
        } else {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->user_path);
        }

    } else if (!strcmp(request, "rollback")) {
        TRACE(5, "%s: parse service payload to rollback an action.\n", __FUNCTION__);
        request = strtok_r(NULL, "/", &nexttoken);
        if (!request || *request == '\0') {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto STATUSACTION_END;
        }

        if (!strcmp(request, "transaction")) {
        	isTransaction = 1;
            TRACE(5, "%s: start - rollback a transaction.\n", __FUNCTION__);
            retcode = ism_config_json_parseServiceTransactionPayload(http, ISM_ADMIN_ROLLBACK, callback);
        } else {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->user_path);
        }

    } else if (!strcmp(request, "forget")) {
        TRACE(5, "%s: parse service payload to forget an action.\n", __FUNCTION__);
        request = strtok_r(NULL, "/", &nexttoken);
        if (!request || *request == '\0') {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto STATUSACTION_END;
        }

        if (!strcmp(request, "transaction")) {
        	isTransaction = 1;
            TRACE(5, "%s: start - forget a transaction.\n", __FUNCTION__);
            retcode = ism_config_json_parseServiceTransactionPayload(http, ISM_ADMIN_FORGET, callback);
        } else {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->user_path);
        }

	} else if (!strcmp(request, "remove")) {
        TRACE(5, "%s: parse service payload to initiate a remove action.\n", __FUNCTION__);
        request = strtok_r(NULL, "/", &nexttoken);
        if (!request || *request == '\0') {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto STATUSACTION_END;
        }

        if (!strcmp(request, "inactiveClusterMember")) {
            /*Get monitoring function function pointer*/
            removeInactiveClusterMember = (removeInactiveClusterMember_f)(uintptr_t)ism_common_getLongConfig("_ism_monitoring_removeInactiveClusterMember", 0L);
            retcode = removeInactiveClusterMember(http);
        } else {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->user_path);
        }

    } else if (!strcmp(request, "close")) {

        TRACE(5, "%s: parse service payload to initiate a close action.\n", __FUNCTION__);
        request = strtok_r(NULL, "/", &nexttoken);
        if (!request || *request == '\0') {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->path);
            goto STATUSACTION_END;
        }

        if (!strcmp(request, "connection")) {
            retcode = ism_admin_closeConnection(http);
        } else {
            retcode = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(retcode, "%s", http->user_path);
        }

    } else if (!strcmp(request, "trace")) {
		TRACE(5, "%s: parse service payload to take trace action.\n", __FUNCTION__);

		request = strtok_r(NULL, "/", &nexttoken);
		if (!request || *request == '\0') {
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", http->path);
			goto STATUSACTION_END;
		}

		if (!strcmp(request, "flush")) {
			TRACE(5, "%s: parse service payload to perform trace flush action.\n", __FUNCTION__);
			retcode = ism_config_json_parseServiceTraceFlushPayload(http);
		} else if (!strcmp(request, "RESTDumpOn")) {
            TRACE(5, "Dump all REST calls in the trace file: turned on.\n");
            restTraceDumpFlag = 1;
            retcode = 0;
        } else if (!strcmp(request, "RESTDumpLogOff")) {
            TRACE(5, "Dump all REST calls in the trace file: turned off.\n");
            restTraceDumpFlag = 0;
            retcode = 0;
        } else {
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", http->user_path);
		}
	} else if (!strcmp(request, "import") ) {
		/* check if this is client set import */
		request = strtok_r(NULL, "/", &nexttoken);
		if (!request || *request == '\0') {
	        TRACE(5, "%s: parse service payload for importing configuration.\n", __FUNCTION__);
	        retcode = ism_config_json_parseServiceImportPayload(http, callback);
	        callback = NULL;
		} else {
		    if ( !strcmp(request, "ClientSet")) {
				TRACE(5, "%s: parse service payload for importing ClientSet.\n", __FUNCTION__);
				retcode = ism_config_json_parseServiceImportClientSetPayload(http, callback);
				callback = NULL;
		    } else {
				retcode = ISMRC_BadRESTfulRequest;
				ism_common_setErrorData(retcode, "%s", http->path);
				goto STATUSACTION_END;
		    }
		}
    } else if (!strcmp(request, "export") ) {
		/* check if this is client set export */
		request = strtok_r(NULL, "/", &nexttoken);
		if (!request || *request == '\0') {
	        TRACE(5, "%s: parse service payload for exporting configuration.\n", __FUNCTION__);
	        retcode = ism_config_json_parseServiceExportPayload(http, callback);
	        callback = NULL;
		} else {
		    if ( !strcmp(request, "ClientSet")) {
				TRACE(5, "%s: parse service payload for exporting ClientSet.\n", __FUNCTION__);
				retcode = ism_config_json_parseServiceExportClientSetPayload(http, callback);
				callback = NULL;
		    } else {
				retcode = ISMRC_BadRESTfulRequest;
				ism_common_setErrorData(retcode, "%s", http->path);
				goto STATUSACTION_END;
		    }
		}
    } else if (!strcmp(request, "diagnostics") ) {
        TRACE(5, "%s: parse service payload for diagnostics configuration.\n", __FUNCTION__);
		request = strtok_r(NULL, "/", &nexttoken);
		if (!request || *request == '\0') {
			retcode = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(retcode, "%s", http->path);
			goto STATUSACTION_END;
		}
        retcode = ism_config_json_parseServiceDiagPayload(http, request, callback);
        callback = NULL;
    } else {
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", http->user_path);
	}

STATUSACTION_END:
	if (callback) {
	    if (isTransaction == 1 && retcode == ISMRC_AsyncCompletion) {
	    	ism_confg_rest_createErrMsg(http, retcode, repl, replSize);
	    	retcode = ISMRC_OK;
	    } else if ( retcode != ISMRC_OK ) {
	        ism_confg_rest_createErrMsg(http, retcode, repl, replSize);
	    } else {
	        ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
	    }

		callback(http,retcode);
	}

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, retcode);

    return ISMRC_OK;
}

/**
 * Returns flag related to REST call extended trace
 */
XAPI int ism_config_getRESTTraceDumpFlag(void) {
    return restTraceDumpFlag;
}

/* Handle RESTAPI apply MQ certificate
 *
 * @param  MQSSLKey             MQ SSL Key file
 * @param  MQStashPassword      Stash password
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyMQCert(char *keyfile, char *stashfile, int overwrite) {
    int rc = ISMRC_OK;
    int clen = 0;
    int fileInUsersFile = 0;
    int stashInUsersFile = 0;
    int fileInDir = 0;
    int stashInDir = 0;
    char *kufilepath = NULL;
    char *kcfilepath = NULL;
    char *sufilepath = NULL;
    char *scfilepath = NULL;

    char *keyStore = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"MQCertificateDir");
    if ( !keyStore ) keyStore = IMA_SVR_DATA_PATH "/data/certificates/MQC";

    if (keyfile) {

        clen = strlen(USERFILES_DIR) + strlen(keyfile) + 1;
        kufilepath = alloca(clen);
        snprintf(kufilepath, clen, "%s%s", USERFILES_DIR, keyfile);
        if ( !isFileExist(kufilepath) ) {
            TRACE(9, "%s: Could not find MQSSL Key file: %s\n", __FUNCTION__, kufilepath);
        } else {
        	fileInUsersFile = 1;
        }

        /* check if file exists in cert dir */
        clen = strlen(keyStore) + 22;
        kcfilepath = alloca(clen);
        snprintf(kcfilepath, clen, "%s/mqconnectivity.kdb", keyStore);
        if ( !isFileExist(kcfilepath) ) {
            TRACE(9, "%s: Could not find MQSSL Key file: %s\n", __FUNCTION__, kcfilepath);
        } else {
        	fileInDir = 1;
        }

        /* check if file exists */
        if ( fileInUsersFile == 0 && fileInDir == 0 ) {
            rc = 6185;
            ism_common_setErrorData(rc, "%s", keyfile);
            goto MQCERT_END;
        }

        /* check for overwite key */
        if ( overwrite == 0 && fileInUsersFile == 1 && fileInDir == 1 ) {
            rc = 6171;
            ism_common_setErrorData(rc, "%s%s", "MQSSLKey", "MQStashPassword");
            goto MQCERT_END;
        }
    }

    if (stashfile) {

        clen = strlen(USERFILES_DIR) + strlen(stashfile) + 1;
        sufilepath = alloca(clen);
        snprintf(sufilepath, clen, "%s%s", USERFILES_DIR, stashfile);
        if ( !isFileExist(sufilepath) ) {
            TRACE(9, "%s: Could not find MQSSL Stash file: %s\n", __FUNCTION__, sufilepath);
        } else {
        	stashInUsersFile = 1;
        }

        /* check if file exists in cert dir */
        clen = strlen(keyStore) + 22;
        scfilepath = alloca(clen);
        snprintf(scfilepath, clen, "%s/mqconnectivity.sth", keyStore);
        if ( !isFileExist(scfilepath) ) {
            TRACE(9, "%s: Could not find MQSSL Stash file: %s\n", __FUNCTION__, scfilepath);
        } else {
        	stashInDir = 1;
        }

        /* check if file exists */
        if ( stashInUsersFile == 0 && stashInDir == 0 ) {
            rc = 6185;
            ism_common_setErrorData(rc, "%s", stashfile);
            goto MQCERT_END;
        }

        /* check for overwite key */
        if ( overwrite == 0 && stashInUsersFile == 1 && stashInDir == 1 ) {
            rc = 6201;
            ism_common_setErrorData(rc, "%s", keyfile);
            goto MQCERT_END;
        }
    }


    /* set keyfile */
    if ( fileInUsersFile == 1 ) {
        /* copy file to cert dir */
        copyFile(kufilepath, kcfilepath);

        /* delete file from usersfile dir */
        unlink(kufilepath);
    }

    /* set stashfile */
    if ( stashInUsersFile == 1 ) {
        /* copy file to cert dir */
        copyFile(sufilepath, scfilepath);

        /* delete file from usersfile dir */
        unlink(sufilepath);
    }

MQCERT_END:

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

