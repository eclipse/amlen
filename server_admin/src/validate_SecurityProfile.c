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
#include "crlprofile.h"
#include "validateInternal.h"

#include "unicode/utypes.h"
#include "unicode/utf8.h"
#include "unicode/uchar.h"

extern int revalidateEndpointForCRL;
extern char * ism_config_getStringObjectValue(char *object, char *instance, char *item, int getLock);

static int validateAlNum(char *input) {
    if (input == NULL)
        return 1;
    int slen = strlen(input);
    if (slen == 0)
        return 1;

    int32_t i;
    for(i=0; i<slen; ) {
        UChar32 c;
        U8_NEXT(input, i, slen, c);
        if (u_isalnum(c) != TRUE)  {
            TRACE(9, "u_isalnum failed on character \"%d\". i=%d\n", c, i);
            return 0;
        }
    }

    return 1;
}

static int validate_SecurityProfileName(char *value) {
	int rc = ISMRC_OK;

	if (!value) {
		rc = ISMRC_NullArgument;
		ism_common_setError(rc);
		return rc;
	}

    int len = strlen(value);
     int count = ism_config_validate_UTF8(value, len, 0, 3);
     if( count < 0 || count > 32 ){
    	 rc = 6205;
    	 ism_common_setErrorData(rc, "%s%d", "NameOfSecurityProfile", 32);
         return rc;
     }

    if (validateAlNum(value) == 0) {
    	rc = 6206;
    	ism_common_setErrorData(rc, "%s", "NameOfSecurityProfile");
        return rc;
    }
    return rc;
}

/*
 * Component: Transport
 * Item: SecurityProfile
 *
 * Validation rules:
 * - Named object
 * - User can create multiple objects
 * - .....
 *
 * Schema:
 *
 * {
 *     "SecurityProfile": {
 *         "<SecurityProfileName>": {
 *             "TLSEnabled": "Boolean",
 *             "MinimumProtocolMethod": "string",
 *             "UseClientCertificate": "Boolean",
 *             "UsePasswordAuthentication": "Boolean",
 *             "Ciphers": "string",
 *             "CertificateProfile": "string",
 *             "UseClientCipher": "Boolean",
 *             "LTPAProfile": "string",
 *             "OAuthProfile": "string"
 *         }
 *     }
 * }
 *
 *
 * Where:
 *
 * TLSEnabled                 : Use TLS encryption.
 * MinimumProtocolMethod      : Lowest level of protocol method permitted when connecting.
 * UseClientCertificate       : Use Client certificate authentication.
 * UsePasswordAuthentication  : Use User ID and Password Authentication.
 * Ciphers                    : Ciphers.
 * CertificateProfile         : Name of a certificate profile.
 * UseClientCipher            : Use Client's Cipher.
 * LTPAProfile                : Name of an LTPA profile.
 * OAuthProfile               : Name of an OAuth profile.
 * CRLProfile				  : Name of CRLProfile.
 *
 * Component callback(s):
 * - Transport
 *
 */

XAPI int32_t ism_config_validate_SecurityProfile(json_t *currPostObj, json_t *validateObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    char *propValue = NULL;
    int objType;

    int hasLTPAProfile = 0;
    int hasCertProfile = 0;
    int hasCCTrue = 0;
    int tlsEnabled = 1;

    char * crlProfName = NULL;
    char * oldCRLProfName = NULL;

    int hasCRLProfile = 0;
    int crlChanged = 0;
    int allowNullPassword = 0;


    TRACE(9, "Entry %s: currPostObj:%p, validateObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, validateObj?validateObj:0, item?item:"null", name?name:"null", action);

    if ( !validateObj || !props ) {
    	TRACE(3, "%s: validation object: %p or IMA properties: %p is null.\n", __FUNCTION__, validateObj?validateObj:0, props?props:0);
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(validateObj) == JSON_NULL ) {
    	rc = ISMRC_DeleteNotAllowed;
		ism_common_setErrorData(rc, "%s", "SecurityProfile");
		goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    //General name validation
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
	if ( rc != ISMRC_OK ) {
		goto VALIDATION_END;
	}

    /* Validate Name requirement for SecurityProfile */
    rc = validate_SecurityProfileName(name);
	if ( rc != ISMRC_OK ) {
		goto VALIDATION_END;
	}

    /* Validate configuration items */
    json_t * mergedObj = validateObj;

    /* Iterate thru the Composite Object */
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

        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
        if (rc != ISMRC_OK) goto VALIDATION_END;

        if (!strcmp(key, "UseClientCertificate")) {
			if (objType == JSON_TRUE)
				hasCCTrue = 1;
        }

        if (!strcmp(key, "AllowNullPassword")) {
			if (objType == JSON_TRUE)
				allowNullPassword = 1;
        }

        if (!strcmp(key,"CertificateProfile") || !strcmp(key,"LTPAProfile") || !strcmp(key,"OAuthProfile")) {
			propValue = (char *)json_string_value(value);
			if ( propValue && *propValue != '\0' ) {
				rc = ism_config_validateItemData( reqList, (char *)key, propValue, action, props);
				if ( rc != ISMRC_OK ) {
					goto VALIDATION_END;
				}

				//check if the profile exists
				json_t *pobj = ism_config_json_getComposite(key, propValue, 0);
				if (!pobj) {
					rc = ISMRC_ObjectNotFound;
					ism_common_setErrorData(rc, "%s%s", key, propValue);
					TRACE(3, "%s: The specified %s %s cannot be found in the configuration repository.\n", __FUNCTION__, key?key:"null", propValue	);
					goto VALIDATION_END;
				}

				if (!strcmp(key, "LTPAProfile")) {
					if (ism_config_objectExist((char *)key, propValue, currPostObj) == 0 ) {
						rc = ISMRC_ObjectNotFound;
						ism_common_setErrorData(rc, "%s%s", key, propValue);
						TRACE(3, "%s: The specified %s %s cannot be found in the configuration repository.\n", __FUNCTION__, key?key:"null", propValue	);
						goto VALIDATION_END;
					}
					hasLTPAProfile = 1;
				}

				if (!strcmp(key, "CertificateProfile")) {
					//make sure the CertificateProfile specified exists
					if (ism_config_objectExist((char *)key, propValue, currPostObj) == 0 ) {
						rc = ISMRC_ObjectNotFound;
						ism_common_setErrorData(rc, "%s%s", key, propValue);
						TRACE(3, "%s: The specified %s %s cannot be found in the configuration repository.\n", __FUNCTION__, key?key:"null", propValue	);
						goto VALIDATION_END;
					}
					hasCertProfile = 1;
				}
			}
        } else if (!strcmp(key, "CRLProfile")) {
            int ptype = 0;
            propValue = (char *)json_string_value(value);
            if ( propValue && *propValue != '\0' ) {
                //check if the profile exists
                json_t *pobj = ism_config_json_getComposite(key, propValue, 0);
                if (!pobj) {
                    rc = ISMRC_ObjectNotFound;
                    ism_common_setErrorData(rc, "%s%s", key, propValue);
                    TRACE(3, "%s: The specified %s %s cannot be found in the configuration repository.\n", __FUNCTION__, key?key:"null", propValue  );
                    goto VALIDATION_END;
                }
                hasCRLProfile = 1;
                crlProfName = (char *) propValue;
                (void) ism_config_json_getObject("CRLProfile", crlProfName, "RevalidateConnection", 0, &ptype);


            }
            /* Has CRL profile name changed ?. We also need to know if we need to revalidate endpoints using this security profile
             * because of the CRL change. The revalidation is called after the new security context has been updated in the callback to
             * transport component. We do not check if the source of the CRL is a url or filename here. */
            oldCRLProfName = ism_config_getStringObjectValue(item, name, (char *)key, 0);
            if (oldCRLProfName && (*oldCRLProfName != '\0')) {
                if (crlProfName && strcmp(oldCRLProfName, crlProfName)) {
                    crlChanged = 1;
                    if (ptype == JSON_TRUE)
                        revalidateEndpointForCRL = 1;
                } else if (!crlProfName)
                    crlChanged = 1;
            } else {
                crlChanged = 1;
                if (ptype == JSON_TRUE)
                    revalidateEndpointForCRL = 1;
            }
        } else if (!strcmp(key, "TLSEnabled") && objType == JSON_FALSE) {
        	tlsEnabled = 0;
        }

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* UseClientCertificate not set to true in current post object, check if it is true in existing configuration */
    if (!hasCCTrue && hasCRLProfile) {
    	rc = ISMRC_PropertyRequired;
    	ism_common_setErrorData(rc, "%s%s", "UseClientCertificate", hasCCTrue?"null":"false");
    	goto VALIDATION_END;
    }

    //CertificateProfile is needed if TLSEnabled is true
    if (tlsEnabled == 1 && hasCertProfile == 0) {
    	rc = ISMRC_NoCertProfile;
    	ism_common_setError(rc);
    	goto VALIDATION_END;
    }

    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
	   goto VALIDATION_END;
    }

    // Add missing default values to mergedObj
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);
    if (rc != ISMRC_OK) {
    	TRACE(3, "%s: failed to add default settings into the object: %s.\n", __FUNCTION__, name?name:"null");
    	goto VALIDATION_END;
    }

    //If LTPAProfile is set, "UsePasswordAuthentication":true is required
    //If AllowNullPassword is set, "UsePasswordAuthentication":true is required
	if (hasLTPAProfile == 1 || allowNullPassword == 1) {
		json_t *ent = json_object_get(mergedObj, "UsePasswordAuthentication");
		if (json_typeof(ent) != JSON_TRUE ) {
			ism_common_setErrorData(6207, "%s%s", "UsePasswordAuthentication", "false");
			rc = 6207;
			goto VALIDATION_END;
		}
		//DISPLAY_MSG(ERROR, 6207, "%s%s", "The \"{0}\" cannot be set to \"{1}\" because the \"LTPAProfile or AllowNullPassword\" is set.", "UsePasswordAuthentication", "False" );

	}


	/* apply crl certificate to the security profile */
	if ( hasCRLProfile == 1 && crlChanged == 1 ) {
		rc = ism_config_updateSecurityCRLProfile(name, crlProfName);
		if (rc != ISMRC_OK) {
			goto VALIDATION_END;
		}
	} else if (crlChanged == 1) {
	    /* dis-associating old crl profile */
	    if (oldCRLProfName && (*oldCRLProfName != '\0')) {
	        rc = ism_config_purgeSecurityCRLProfile(name);
	        if (rc != ISMRC_OK) {
	            TRACE(3, "Could not purge CRLs for Security Profile Instance: %s", name);
	            goto VALIDATION_END;
	        }
	    }
	}

VALIDATION_END:

	if (oldCRLProfName)
		ism_common_free(ism_memory_admin_misc,oldCRLProfName);

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}
