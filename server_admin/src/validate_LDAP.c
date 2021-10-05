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

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include <unistd.h>
#include "configInternal.h"
#include "validateInternal.h"
#include "adminInternal.h"
#include "ldaputil.h"

extern int ism_config_setApplyCertErrorMsg(int rc, char *item);
extern int ism_admin_getCunitEnv(void);

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

/* Handle RESTAPI post verify LDAP object action.
 *
 * @param  props        the config props from POST payload
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
static int verify_LDAPConfig( json_t * cfgObj, int newCert)
{
    int rc = ISMRC_OK;
    char *value=NULL;
    ismLDAPConfig_t tLdapConfig;
    json_t *entry = NULL;

    memset (&tLdapConfig, 0, sizeof(ismLDAPConfig_t));
    if (!cfgObj || json_typeof(cfgObj) != JSON_OBJECT) {
        rc = ISMRC_InvalidLDAP;
        ism_common_setError(rc);
        return rc;
    }

    entry = json_object_get(cfgObj, "Name");
    if (entry && json_typeof(entry) == JSON_STRING)
        value= (char *) json_string_value(entry);

    if(value!=NULL){
        tLdapConfig.name=value;
    }else{
        tLdapConfig.name = "ldapconfig";
    }

    entry = json_object_get(cfgObj, "URL");
    if (entry && json_typeof(entry) == JSON_STRING) {
        tLdapConfig.URL= (char *) json_string_value(entry);
    } else {
        rc = ISMRC_InvalidLDAP;
        ism_common_setError(rc);
        return rc;
    }

    entry = json_object_get(cfgObj, "Certificate");
    if (entry && json_typeof(entry) == JSON_STRING) {
        tLdapConfig.Certificate= (char *) json_string_value(entry);
    }

    entry = json_object_get(cfgObj, "CheckServerCert");
    if (entry && json_typeof(entry) == JSON_STRING) {
        char *tmpCfg = (char *) json_string_value(entry);
        tLdapConfig.CheckServerCert = ismSEC_SERVER_CERT_DISABLE_VERIFY;
        if (tmpCfg && !strcmpi(tmpCfg, "TrustStore")) {
            tLdapConfig.CheckServerCert = ismSEC_SERVER_CERT_TRUST_STORE;
        } else if (tmpCfg && !strcmpi(tmpCfg, "PublicTrust")) {
            tLdapConfig.CheckServerCert = ismSEC_SERVER_CERT_PUBLIC_TRUST;
        }
    }


    entry = json_object_get(cfgObj, "BaseDN");
    if (entry && json_typeof(entry) == JSON_STRING) {
        tLdapConfig.BaseDN= (char *) json_string_value(entry);
    }

    entry = json_object_get(cfgObj, "BindDN");
    if (entry && json_typeof(entry) == JSON_STRING) {
        tLdapConfig.BindDN= (char *) json_string_value(entry);
    }

    entry = json_object_get(cfgObj, "BindPassword");
    if (entry && json_typeof(entry) == JSON_STRING) {
        tLdapConfig.BindPassword= (char *)ism_security_decryptAdminUserPasswd((char *)json_string_value(entry));
    }

    entry = json_object_get(cfgObj, "Timeout");
    if (entry && json_typeof(entry) == JSON_INTEGER) {
        //double dval =json_number_value(entry);
        tLdapConfig.Timeout= (int) json_number_value(entry);
    } else {
        /*Set default timeout*/
        tLdapConfig.Timeout=15;
    }

    /* for verify, set Enable to true */
    tLdapConfig.Enabled= true;

    /* return ISMRC_VerifyTestOK for cunit tests */
    if ( ism_admin_getCunitEnv() == 1 ) return ISMRC_VerifyTestOK;

    /* Validate the object */
    int tryCount = 3;
    rc =  ism_security_validateLDAPConfig(&tLdapConfig, 1, newCert, tryCount);
    if (rc == ISMRC_OK) { /* RTC 222635 */
        rc = ISMRC_VerifyTestOK;
        TRACE(5, "\"The Object: LDAP Name: ldapconfig has been verified successfully.\" (%d)\n", rc);
    } else {
        ism_common_setErrorData(rc, "%s%s", "LDAP", "ldapconfig");
    }

    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Component: Security
 * Item: LDAP
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 * - .....
 *
 * Schema:
 *
 *   {
 *       "LDAP": {
 *       "NestedGroupSearch": "string",
 *       "URL": "string",
 *       "Certificate": "string",
 *       "IgnoreCase": "boolean",
 *       "BaseDN": "string",
 *       "BindDN": "string",
 *       "BindPassword": "string",
 *       "UserSuffix": "string",
 *       "GroupSuffix": "string",
 *       "GroupCacheTimeout": "number",
 *       "UserIdMap": "string",
 *       "GroupIdMap": "string",
 *       "GroupMemberIdMap": "string",
 *       "Timeout": "number",
 *       "EnableCache": "boolean",
 *       "CacheTimeout": "number",
 *       "MaxConnections": "number",
 *       "Enabled": "boolean",
 *       "Verify": "boolean",
 *       "Overwrite": "boolean"
 *       }
 *   }
 *
 *
 */

XAPI int32_t ism_config_validate_LDAP(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;
    int isVerify = 0;
    int hasCertificate = -1;
    int newCertificate = 0;
    int isLdaps = 0;
    int overwrite = 0;
    char *ldapcert = NULL;
    char *bindPassword = NULL;
    int hasBindPwd = 0;
    int isDisabled = 0;
    int isPublicTrust = 0;


    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
    __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props ) {
        TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, validateObj?validateObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    if (!currPostObj) {
        TRACE(5, "Input configuration is empty, no validation is needed.\n");
        goto VALIDATION_END;
    } else {
        //check if new payload has BindPassword specified.
        json_t *inst = json_object_get(currPostObj, "LDAP");
        if (inst && json_typeof(inst) == JSON_OBJECT) {
            json_t *bindpwd = json_object_get(inst, "BindPassword");
            if (bindpwd) {
                TRACE(9, "%s: The new configuration has BindPassword specified.\n", __FUNCTION__);
                hasBindPwd = 1;
            }
        }

        //check if new payload has a new Certificate
        newCertificate = (NULL != json_object_get(inst, "Certificate"));
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(validateObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "LDAP");
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate name - name can only be AdminEndpoint */
    if ( name && strcmp(name, "ldapconfig") ) {
        rc = ISMRC_BadOptionValue;
        ism_common_setErrorData(rc, "%s%s%s%s", "LDAP", "null", "Name", name);
        goto VALIDATION_END;
    }

    /* check items that are not settable */

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

    json_t *enabled = json_object_get(mergedObj, "Enabled");
    if (enabled && json_typeof(enabled) == JSON_FALSE) {
        isDisabled = 1;
        TRACE(9, "%s: LDAP configuration is set to be disabled.\n", __FUNCTION__);
    }else {
        TRACE(9, "%s: LDAP configuration is set to be enabled.\n", __FUNCTION__);
    }

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        objType = json_typeof(value);

        if (!key) {
            itemIter = json_object_iter_next(mergedObj, itemIter);
            continue;
        }

        char *propValue = (char *)json_string_value(value);

        /* Check for public trust case */
        if ( !strcmp(key, "CheckServerCert") ) {
            if (propValue && *propValue != '\0') {
                if ( !strcmp(propValue, "PublicTrust")) {
                    isPublicTrust = 1;
                }
            }
        }

        //skip Overwrite since it is not in schema
        if ( strcmp(key, "Overwrite") ) {
            //if isDisabled, skip validation for required item so they can be set to empty string.
            if ( isDisabled && propValue && *propValue == '\0' && ( !strcmp(key, "URL") || !strcmp(key, "GroupSuffix") || !strcmp(key, "BaseDN")) ) {
                TRACE(5, "%s: skip %s validation since LDAP is disabled and value is empty string.\n", __FUNCTION__, key);
            } else {
                rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
                if (rc != ISMRC_OK)
                    goto VALIDATION_END;
            }
        } else {
            if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s%s", item?item:"null", name, key, ism_config_json_typeString(objType));
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;
            } else if (objType == JSON_TRUE) {
                overwrite = 1;
            }
        }

        if (!strcmp(key, "Certificate") ) {
            if (propValue && *propValue != '\0') {
                int certNeedCopy = 1;
                int clen = strlen(USERFILES_DIR) + strlen(propValue) + 1;
                char *keyfilepath = alloca(clen);

                snprintf(keyfilepath, clen, "%s%s", USERFILES_DIR, propValue);

                if ( ism_admin_getCunitEnv() == 0 && !isFileExist(keyfilepath) ) {
                    if (!strcmp(propValue, "ldap.pem")) {
                        char *storeDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"LDAPCertificateDir");
                        int slen = strlen(storeDir) + strlen("ldap.pem") + 2;
                        char *storePath = alloca(slen);

                        snprintf(storePath, slen, "%s/%s", storeDir, "ldap.pem");
                        if ( !isFileExist(storePath) ) {
                            ism_common_setError(ISMRC_NoCertificate);
                            rc = ISMRC_NoCertificate;
                            goto VALIDATION_END;
                        }
                        certNeedCopy = 0;
                    } else {
                        TRACE(3, "%s: LDAP Key file: %s cannot be found.\n", __FUNCTION__, keyfilepath);
                        rc = 6185;
                        ism_common_setErrorData(rc, "%s", propValue);
                        goto VALIDATION_END;
                    }
                }
                if (certNeedCopy)
                    hasCertificate = 1;

                int mlen = strlen(propValue);
                ldapcert = alloca(mlen + 1);
                memcpy(ldapcert, propValue, mlen);
                ldapcert[mlen] = '\0';
            } else {
                // Certificate has been set to ""
                hasCertificate = 0;
            }
        } else if (!strcmp(key, "URL") && !isDisabled) {
            if (propValue && *propValue != '\0') {
                if (!strncasecmp(propValue, "ldaps://", strlen("ldaps://"))) {
                    isLdaps = 1;
                }
            } else {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "NULL");
                goto VALIDATION_END;
            }
        } else if ( (!strcmp(key, "GroupSuffix") || !strcmp(key, "BaseDN") )  && !isDisabled) {
            if ( !propValue || *propValue == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, propValue?propValue:"NULL");
                goto VALIDATION_END;
            }
        } else if (!strcmp(key, "BindPassword") && hasBindPwd == 1 && propValue && *propValue != '\0') {
            if (!strcmp(propValue, "XXXXXX")) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "XXXXXX");
                goto VALIDATION_END;
            }
            /* Encrypt Password */
            bindPassword = ism_security_encryptAdminUserPasswd(propValue);
            TRACE(5, "BindPassword has been encrypted.\n");
            if ( bindPassword == NULL ) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", key, "NULL");
                goto VALIDATION_END;
            }
        } else if (!strcmp(key, "Verify") && objType == JSON_TRUE) {
            isVerify = 1;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    // update mergedObj with encrypted password
    if (bindPassword) {
        json_object_set(mergedObj, "BindPassword", json_string((const char *)bindPassword));
    }

    int checkMode = 0;
    if ( isDisabled == 1 ) {
        checkMode = 2; /* allow empty string for required items */
    }
    rc = ism_config_validate_checkRequiredItemList(reqList, checkMode );

    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    //If URL is ldaps, ldap certificate is required
    if (isLdaps == 1) {
        if (hasCertificate == -1) {
            // check if the certificate is in LDAP store for update case
            if (action == 1) {
                char *ldapDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"LDAPCertificateDir");
                int dlen = strlen(ldapDir) + strlen("ldap.pem") + 2;
                char *ldpafilepath = alloca(dlen);

                snprintf(ldpafilepath, dlen, "%s/%s", ldapDir, "ldap.pem");
                if ( ism_admin_getCunitEnv() == 0 && !isFileExist(ldpafilepath) ) {
                    ism_common_setError(ISMRC_NoCertificate);
                    rc = ISMRC_NoCertificate;
                    goto VALIDATION_END;
                }
            } else {
                ism_common_setError(ISMRC_NoCertificate);
                rc = ISMRC_NoCertificate;
                goto VALIDATION_END;
            }
        } else if (hasCertificate == 0) {
            if ( isPublicTrust == 0 ) {
                //Certificate cannot be null if ldaps
                TRACE(3, "Validate: for ldaps LDAP Certificate is needed unless CheckServerCert is PublicTrust.\n");
                rc = ISMRC_InvalidLDAPCert;
                ism_common_setError(ISMRC_InvalidLDAPCert);
                goto VALIDATION_END;
            }
        }
    }

    if (ism_admin_getCunitEnv() == 0 && hasCertificate == 1) {
        if (newCertificate && !isVerify) {
            /* move ldap certificate to LDAP store */
            if (overwrite)
                /* args for Trusted cert - "apply", "TRUSTED", overwrite, secprofile, trustedcert */
                rc = ism_admin_ApplyCertificate("LDAP", "true", (char *)ldapcert, NULL, NULL, NULL);
            else
                rc = ism_admin_ApplyCertificate("LDAP", "false", (char *)ldapcert, NULL, NULL, NULL);

            if (rc) {
                TRACE(5, "%s: call msshell return error code: %d\n", __FUNCTION__, rc);
                int xrc = 0;
                xrc = ism_config_setApplyCertErrorMsg(rc, item);
                rc = xrc;
                if ( rc == 6184 ) {
                    ism_common_setErrorData(rc, "%s", ldapcert?ldapcert:"");
                }
                goto VALIDATION_END;
            }
            json_object_set(mergedObj, "Certificate", json_string("ldap.pem"));
        }
    }

    // Add missing default values to mergedObj
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);
    if (rc != ISMRC_OK) {
        TRACE(3, "%s: failed to add default settings into the object: %s.\n", __FUNCTION__, name?name:"null");
        goto VALIDATION_END;
    }

    if ( isVerify || !isDisabled ) {
        if ( !isVerify ) newCertificate = 0;
        rc = verify_LDAPConfig( mergedObj, newCertificate);
        if ( rc == ISMRC_VerifyTestOK ) {
            TRACE(5, "%s: LDAP configuration is verfied successfully.\n", __FUNCTION__);
        } else if ( rc != ISMRC_OK ) {
            TRACE(3, "%s: failed to verify LDAP configuration\n", __FUNCTION__);
        }
    }

    if ( !isDisabled && !isVerify && rc == ISMRC_VerifyTestOK ) rc = ISMRC_OK;

    /* Delete unwanted keys from mergedObj */
    /* Delete Overwrite entry */
    json_object_del(mergedObj, "Overwrite");
    json_object_del(mergedObj, "Verify");

VALIDATION_END:
    if (bindPassword)  ism_common_free(ism_memory_admin_misc,bindPassword);

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
