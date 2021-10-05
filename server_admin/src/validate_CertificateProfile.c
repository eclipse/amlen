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
#include "configInternal.h"
#include "validateInternal.h"


extern int ism_config_setApplyCertErrorMsg(int rc, char *item);
/*
 * Component: Transport
 * Item: CertificateProfile
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 * - .....
 *
 * Schema:
 *
 * {
 *     "CertificateProfile": {
 *         "<CertificateProfileName>": {
 *             "Certificate": "string",
 *             "Key": "string",
 *             "CertFilePassword": "string",
 *             "KeyFilePassword": "string",
 *             "Overwrite": "Boolean"
 *         }
 *     }
 * }
 *
 *
 * Where:
 *
 * CertificateProfileName          : The name of this certificate profile.
 * Certificate    : The name of the server certificate that is used by this CertificateProfile
 * Key            : The name of the private key that is associated with the Certificate
 *
 * Component callback(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_CertificateProfile(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    int objType;
    int overwrite = 0;
    char *certName = NULL;
    char *keyName = NULL;
    char *certPwd = NULL;
    char *keyPwd = NULL;

    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props ) {
        TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, mergedObj?mergedObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(ISMRC_DeleteNotAllowed, "%s", "CertificateProfile");
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* check items that are not settable */

    /* Validate configuration items */
    char *expDateStr = NULL;

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
        if ( !strcmp(key, "Certificate") ) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;

            } else {
                if ( propValue && *propValue != '\0' ) {
                    rc = ism_config_validateItemData( reqList, (char *)key, propValue, action, props);
                    if ( rc != ISMRC_OK ) {
                        goto VALIDATION_END;
                    }
                    int clen = strlen(propValue) + 1;
                    certName = alloca(clen + 1 );
                    memset(certName, 0, clen+1);
                    memcpy(certName, propValue, clen);
                    certName[clen] = '\0';
                }
            }
        }else if ( !strcmp(key, "Key") ) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;

            } else {
                if ( propValue && *propValue != '\0' ) {
                    rc = ism_config_validateItemData( reqList, (char *)key, propValue, action, props);
                    if ( rc != ISMRC_OK ) {
                        goto VALIDATION_END;
                    }
                    int clen = strlen(propValue) + 1;
                    keyName = alloca(clen + 1 );
                    memset(keyName, 0, clen+1);
                    memcpy(keyName, propValue, clen);
                    keyName[clen] = '\0';
                }
            }
        }else if (!strcmp(key, "Overwrite") ) {
            if ( objType != JSON_TRUE && objType != JSON_FALSE ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;

            } else {
                if ( objType == JSON_TRUE ) {
                    overwrite = 1;
                }
            }

        }else if (!strcmp(key, "CertFilePassword")) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;
            } else {
                if ( propValue && *propValue != '\0' ) {
                    int clen = strlen(propValue) + 1;
                    certPwd = alloca(clen + 1 );
                    memset(certPwd, 0, clen+1);
                    memcpy(certPwd, propValue, clen);
                    certPwd[clen] = '\0';
                }
            }

        }else if (!strcmp(key, "KeyFilePassword")) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s", item?item:"null", key, "InvalidType");
                rc = ISMRC_BadOptionValue;
                goto VALIDATION_END;
            } else {
                if ( propValue && *propValue != '\0' ) {
                    int klen = strlen(propValue) + 1;
                    keyPwd = alloca(klen + 1 );
                    memset(keyPwd, 0, klen+1);
                    memcpy(keyPwd, propValue, klen);
                    keyPwd[klen] = '\0';
                }
            }

        } else if ( !strcmp(key, "ExpirationDate") ) {
            /* Just ignore this now. Most probably from existing data...
             * Might want to make sure it is not in the incoming JSON
             */
            if ( objType != JSON_STRING ) {
                TRACE(3, "%s: The configuration item: %s is not supported.\n", __FUNCTION__, key);
                ism_common_setErrorData(ISMRC_ArgNotValid, "%s", key);
                rc = ISMRC_ArgNotValid;
                goto VALIDATION_END;
            }

            if ( propValue && *propValue != '\0' ) {
                int slen = strlen(propValue) + 1;
                expDateStr = alloca(slen + 1 );
                memset(expDateStr, 0, slen+1);
                memcpy(expDateStr, propValue, slen);
                expDateStr[slen] = '\0';
            }

        } else {
            TRACE(3, "%s: The configuration item: %s is not supported.\n", __FUNCTION__, key);
            ism_common_setErrorData(ISMRC_ArgNotValid, "%s", key);
            rc = ISMRC_ArgNotValid;
            goto VALIDATION_END;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Delete unwanted keys from mergedObj */
    /* Delete Overwrite entry */
    json_object_del(mergedObj, "Overwrite");
    json_object_del(mergedObj, "KeyFilePassword");
    json_object_del(mergedObj, "CertFilePassword");

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    //Check if certificate/key have been used by other CertificateProfile
    rc = ism_config_json_validateCertificateProfileKeyCertUnique(name, certName, keyName);
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    //apply certificate if there is one
    /* args for Server cert - "apply", "Server", overwrite, name, keyname, keypwd, certpwd */
    if (overwrite)
        rc = ism_admin_ApplyCertificate("Server", "true", certName, keyName, keyPwd, certPwd);
    else
        rc = ism_admin_ApplyCertificate("Server", "false", certName, keyName, keyPwd, certPwd);

    /*
     * Need to get the original cert and key names and delete them from the directory if they
     * were changed to new names.
     */
    if (rc) {
        TRACE(5, "%s: ism_admin_ApplyCertificate() returned error=%d\n", __FUNCTION__, rc);
        int xrc = 0;
        xrc = ism_config_setApplyCertErrorMsg(rc, item);
        rc = xrc;
        if ( rc == 6184 ) {
            ism_common_setErrorData(rc, "%s", certName?certName:"");
        } else if ( rc == 6185 ) {
            ism_common_setErrorData(rc, "%s", keyName?keyName:"");
        }
    } else if ( certName && *certName ) {
        /* Get the Expiration date and add it to the parameters */
        char *expireDate = ism_config_getCertExpirationDate(certName, &rc);;
        if (ISMRC_OK == rc && expireDate != NULL ) {
            json_object_set(mergedObj, "ExpirationDate", json_string(expireDate));
            ism_common_free(ism_memory_admin_misc,expireDate);
        }
    }
    /*
     * At this point, if rc is zero, check if we have changed/uploaded any new certificate/key
     * and if so, remove the old one.
     */
    if (rc == ISMRC_OK) {
        json_t *certProf = ism_config_json_getComposite(item, name, 0);
        if (certProf) {
            json_t *oldCertJson = json_object_get(certProf, "Certificate");
            char *certDir = NULL;
            char buffer[100];
            const char *oldCert = NULL;
            if (oldCertJson) {
                oldCert = json_string_value(oldCertJson);
                if (strcmp(oldCert, certName)) {
                    /* New certificate, delete the old */
                    certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
                            , "KeyStore" );
                    sprintf(buffer, "%s/%s", certDir, oldCert);
                    unlink(buffer);
                }
            }
        
            json_t *oldKeyJson = json_object_get(certProf, "Key");
            if (oldKeyJson) {
                const char *oldKey = json_string_value(oldKeyJson);
                if (strcmp(oldKey, keyName)) {
                    if (strcmp(oldKey, certName)) {
                        /* New key, delete the old */
                        if (NULL == certDir)
                            certDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
                                    , "KeyStore" );
                        sprintf(buffer, "%s/%s", certDir, oldKey);
                        unlink(buffer);
                    }
                }
            }
        }
    }

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

