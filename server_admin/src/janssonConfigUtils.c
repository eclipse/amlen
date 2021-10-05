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
 * janssonConfigUtils.c
 */

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include "configInternal.h"
#include <ctype.h>

extern int ism_config_updateCfgFile(ism_prop_t *props, int compType);
extern int ism_config_updateStandbyNode(json_t *objval, int comptype, char *item, char *name, int getConfigLock, int deleteFlag);
extern int ism_config_invokeCallbackOnStandbyFlag(int compType, char *item);

extern const char * configDir;         /* Configuration directory location  */
extern json_t *serverConfigSchema;     /* Server schema JSON object         */
extern schemaItems_t * cfgSchemaItems; /* Schema items                      */
json_t *srvConfigRoot = NULL;          /* Server configuration JSON object  */

#define ISM_TRUSTEDCERT 0
#define ISM_CLIENTCERT  1


const char *jsonTypeStr[] = { "JSON_OBJECT", "JSON_ARRAY", "JSON_STRING", "JSON_INTEGER", "JSON_REAL", "JSON_TRUE", "JSON_FALSE", "JSON_NULL", "JSON_BOOLEAN", "UNKNOWN" };


/* Lock for server configuration JSON object. */
pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
pthread_spinlock_t configSpinLock;
int srvConfigLockInited = 0;

char *serverVersion = NULL;

/*
 * NOTE for schema object: There is no need to create a RW lock for schema because schema JSON object is
 * created at server start time, and it is not updated after server start. Functions just read the
 * content from the schema.
 */


/* Data structure to manage object validation function for composite objects */
struct {
    int                    compType;
    char                 * objectName;
    ismConfigValidate_t    valFNP;
} ismConfigValidationData[] = {
    { ISM_CONFIG_COMP_TRANSPORT,        "AdminEndpoint",            ism_config_validate_AdminEndpoint            },
    { ISM_CONFIG_COMP_TRANSPORT,        "MessageHub",               ism_config_validate_MessageHub               },
    { ISM_CONFIG_COMP_SECURITY,         "ConfigurationPolicy",      ism_config_validate_ConfigurationPolicy      },
    { ISM_CONFIG_COMP_SECURITY,         "ConnectionPolicy",         ism_config_validate_ConnectionPolicy         },
    { ISM_CONFIG_COMP_TRANSPORT,        "CertificateProfile",       ism_config_validate_CertificateProfile       },
    { ISM_CONFIG_COMP_TRANSPORT,        "LTPAProfile",              ism_config_validate_LTPAProfile              },
    { ISM_CONFIG_COMP_TRANSPORT,        "OAuthProfile",             ism_config_validate_OAuthProfile             },
    { ISM_CONFIG_COMP_TRANSPORT,        "SecurityProfile",          ism_config_validate_SecurityProfile          },
    { ISM_CONFIG_COMP_TRANSPORT,        "TrustedCertificate",       ism_config_validate_TrustedCertificate       },
    { ISM_CONFIG_COMP_TRANSPORT,        "Endpoint",                 ism_config_validate_Endpoint                 },
    { ISM_CONFIG_COMP_TRANSPORT,        "TopicMonitor",             ism_config_validate_TopicMonitor             },
    { ISM_CONFIG_COMP_CLUSTER,          "ClusterMembership",        ism_config_validate_ClusterMembership        },
    { ISM_CONFIG_COMP_STORE,            "HighAvailability",         ism_config_validate_HighAvailability         },
    { ISM_CONFIG_COMP_SECURITY,         "Queue",                    ism_config_validate_Queue                    },
    { ISM_CONFIG_COMP_ENGINE,           "AdminSubscription",        ism_config_validate_AdminSubscription        },
    { ISM_CONFIG_COMP_ENGINE,           "DurableNamespaceAdminSub", ism_config_validate_DurableNamespaceAdminSub },
    { ISM_CONFIG_COMP_ENGINE,           "NonpersistentAdminSub",    ism_config_validate_NonpersistentAdminSub    },
    { ISM_CONFIG_COMP_CLUSTER,          "LDAP",                     ism_config_validate_LDAP                     },
    { ISM_CONFIG_COMP_SECURITY,         "TopicPolicy",              ism_config_validate_TopicPolicy              },
    { ISM_CONFIG_COMP_SECURITY,         "QueuePolicy",              ism_config_validate_QueuePolicy              },
    { ISM_CONFIG_COMP_MQCONNECTIVITY,   "QueueManagerConnection",   ism_config_validate_QueueManagerConnection   },
    { ISM_CONFIG_COMP_SECURITY,         "SubscriptionPolicy",       ism_config_validate_SubscriptionPolicy       },
    { ISM_CONFIG_COMP_PROTOCOL,         "Plugin",                   ism_config_validate_Plugin                   },
    { ISM_CONFIG_COMP_SERVER,           "LogLocation",              ism_config_validate_LogLocation              },
    { ISM_CONFIG_COMP_SERVER,           "Syslog",                   ism_config_validate_Syslog                   },
    { ISM_CONFIG_COMP_TRANSPORT,        "ClientCertificate",        ism_config_validate_ClientCertificate        },
    { ISM_CONFIG_COMP_MQCONNECTIVITY,   "DestinationMappingRule",   ism_config_validate_DestinationMappingRule   },
    { ISM_CONFIG_COMP_MQCONNECTIVITY,    "MQCertificate",           ism_config_validate_MQCertificate            },
    { ISM_CONFIG_COMP_TRANSPORT,         "CRLProfile",              ism_config_validate_CRLProfile               },
    { ISM_CONFIG_COMP_ENGINE,            "ResourceSetDescriptor",   ism_config_validate_ResourceSetDescriptor    },
    { ISM_CONFIG_COMP_ENGINE,           "ClusterRequestedTopics",   ism_config_validate_ClusterRequestedTopics   }
};

#define NOVALIDATIONDATA (sizeof(ismConfigValidationData) / sizeof(ismConfigValidationData[0]))

//Insert the string value if it does not exist in the array
static void ism_updateJsonStringArray(json_t *retval, const char *value) {
        int i = 0;
        int dup = 0;
        int rc = 0;

        if (!value)
                return;

        for (i=0; i<json_array_size(retval); i++) {
                json_t *instRoot = json_array_get(retval, i);

                const char *tStr = json_string_value(instRoot);
                if (tStr && !strcmp(value, tStr)) {
                        dup = 1;
                        break;
                }

        }

        if (dup == 0) {
                rc = json_array_append_new(retval, json_string(value));
                TRACE(7, "%s: update the merged json array with %s return %d.\n", __FUNCTION__, value, rc);
        }
        return;
}

static char * ism_getStringValue(json_t *obj, char *object) {
    char *retval = NULL;
    if ( !obj || !object ) return retval;
    json_t *objval = json_object_get(obj, object);
    if ( objval && json_typeof(objval) == JSON_STRING ) {
        retval = (char *)json_string_value(objval);
    }
    return retval;
}

/* Insert the object if it does not exist in the array.
 * This function can handle only TrustedCertificate and ClientCertificate objects
 */
static void ism_updateJsonObjectArray(int objType, json_t *retval, json_t *entry) {
    int i = 0, j = 0;
    int dup = 0;
    int rc = 0;
    char *entProfName = NULL;
    char *entCertName = NULL;
    char *profName = NULL;
    char *certName = NULL;

    if (!retval && !entry)
        return;

    if ( json_typeof(entry) != JSON_ARRAY ) {
        entProfName = ism_getStringValue(entry, "SecurityProfileName");
        entCertName = NULL;
        if ( objType == ISM_TRUSTEDCERT ) {
            entCertName = ism_getStringValue(entry, "TrustedCertificate");
        } else {
            entCertName = ism_getStringValue(entry, "CertificateName");
        }
        for (i=0; i<json_array_size(retval); i++) {
            json_t *instObj = json_array_get(retval, i);

            if (instObj) {
                profName = ism_getStringValue(instObj, "SecurityProfileName");
                certName = NULL;
                if ( objType == ISM_TRUSTEDCERT ) {
                    certName = ism_getStringValue(instObj, "TrustedCertificate");
                } else {
                    certName = ism_getStringValue(instObj, "CertificateName");
                }
                if (entProfName &&  profName && strcmp(entProfName, profName))
                    continue;
                if (entCertName &&  certName && strcmp(entCertName, certName))
                    continue;

                dup = 1;
                break;
            }
        }

        if (dup == 0) {
            rc = json_array_append_new(retval, entry);
            TRACE(7, "%s: update the merged json array with new object, return %d.\n", __FUNCTION__, rc);
        }
    } else {
        for (j=0; j<json_array_size(entry); j++) {
            dup = 0;
            json_t *itemEntry = json_array_get(entry, j);

            if ( !itemEntry )
                break;

            entProfName = ism_getStringValue(itemEntry, "SecurityProfileName");
            entCertName = NULL;
            if ( objType == ISM_TRUSTEDCERT ) {
                entCertName = ism_getStringValue(itemEntry, "TrustedCertificate");
            } else {
                entCertName = ism_getStringValue(itemEntry, "CertificateName");
            }

            for (i=0; i<json_array_size(retval); i++) {
                json_t *instObj = json_array_get(retval, i);

                if (instObj) {
                    profName = ism_getStringValue(instObj, "SecurityProfileName");
                    certName = NULL;
                    if ( objType == ISM_TRUSTEDCERT ) {
                        certName = ism_getStringValue(instObj, "TrustedCertificate");
                    } else {
                        certName = ism_getStringValue(instObj, "CertificateName");
                    }
                    if (entProfName &&  profName && strcmp(entProfName, profName))
                        continue;
                    if (entCertName &&  certName && strcmp(entCertName, certName))
                        continue;

                    dup = 1;
                    break;
                }
            }

            if (dup == 0) {
                rc = json_array_append_new(retval, itemEntry);
                TRACE(7, "%s: Append new object (type=%d) in the array. rc=%d.\n", __FUNCTION__, objType, rc);
            }
        }
    }
    TRACE(9, "%s: Updated object (type=%d) array. size=%d\n", __FUNCTION__, objType, (int)json_array_size(retval));
    return;
}

/**
 * Initialize JSON configuration schema, root configuration object and locks
 * This function is called at server start time.
 */
XAPI void ism_config_json_init(void) {

    /* Initialize schema */
    if (!serverConfigSchema) {
        TRACE(4, "Initialize JSON schema\n");
        ism_config_initSchemaObject(ISM_CONFIG_SCHEMA);
    }

    if ( srvConfigLockInited == 0 ) {
        /* Initialize rwlock to protect server configuration JSON object */
        pthread_rwlockattr_t rwlockattr_init;
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&srvConfiglock, &rwlockattr_init);

        pthread_spin_init(&configSpinLock, 0);
        srvConfigLockInited = 1;
    }

    /* Create a new JSON root node */
    if ( !srvConfigRoot ) {
        pthread_rwlock_wrlock(&srvConfiglock);
        srvConfigRoot = json_object();

        /* set some singleton default values */
        json_object_set_new(srvConfigRoot, "Version", json_string(SERVER_SCHEMA_VERSION));

        /* Set server version to server build version */
        serverVersion = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,242),strlen(ism_common_getVersion()) + strlen(ism_common_getBuildLabel()) + 16);
        sprintf(serverVersion, "%s %s", ism_common_getVersion(), ism_common_getBuildLabel());
        json_object_set_new(srvConfigRoot, "ServerVersion", json_string(serverVersion));
        json_object_set_new(srvConfigRoot, "ServerName", json_string(""));
        pthread_rwlock_unlock(&srvConfiglock);
    }

    return;
}


/**
 * Create JSON object from JSON string
 */
XAPI json_t * ism_config_json_strToObject(const char *text, int *rc)
{
    json_t *root = NULL;
    json_error_t error;

    if ( !text || *text == '\0') {
        TRACE(3, "ism_config_json_strToObject: Invalid argument");
        *rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(ISMRC_BadRESTfulRequest, "%s", text?text:"null");
        return NULL;
    }

    root = json_loads(text, 0, &error);

    if (root) {
        return root;
    }

    *rc = 6001;

    ism_common_setErrorData(6001, "%s%d", error.text, error.line);
    return NULL;
}

/**
 * Create JSON object from a file with JSON fragment
 */
XAPI json_t * ism_config_json_fileToObject(const char *filename) {
    json_t *root;
    json_error_t error;

    if ( !filename || *filename == '\0') {
        TRACE(3, "ism_config_json_fileToObject: Invalid file name");
        return NULL;
    }

    root = json_load_file(filename, 0, &error);

    if (root) {
        return root;
    }

    if ( error.line == -1 ) {
        TRACE(4, "ism_config_json_fileToObject: %s\n", error.text);
    } else {
        TRACE(3, "ism_config_json_fileToObject: JSON error on line %d: %s\n", error.line, error.text);
    }
    return NULL;
}


/**
 * Creates JSON object from value
 */
XAPI json_t * ism_config_json_createObject(int type, void *value) {
    json_t *obj = NULL;

    if ( !value ) {
        obj = json_null();
        return obj;
    }

    switch(type) {
    case JSON_INTEGER:
        obj = json_integer(atoi((char *)value));
        break;

    case JSON_STRING:
        obj = json_string((char *)value);
        break;

    case JSON_TRUE:
        obj = json_true();
        break;

    case JSON_FALSE:
        obj = json_false();
        break;

    case JSON_BOOLEAN_TYPE:
    {
        if ( strcmpi((char *)value, "True") == 0 )
            obj = json_true();
        else
            obj = json_false();

        break;
    }

    default:
        obj = json_null();
    }


    return obj;
}

/**
 * Create and returns JSON object from HTTP payload
 */
XAPI json_t * ism_config_json_createObjectFromPayload(ism_http_t *http, int *rc, int noErrorTrace) {
    json_t * retObj = NULL;
    char *payload;

    /* Get payload from http object */
    if (http && http->content_count > 0) {
        char * content = http->content[0].content;
        int content_len = http->content[0].content_len;

        payload = alloca(content_len + 1);
        memcpy(payload, content, content_len);
        payload[content_len] = '\0';
    } else {
        *rc = ISMRC_BadRESTfulRequest;
        if ( noErrorTrace == 0 ) {
            ism_common_setErrorData(ISMRC_BadRESTfulRequest, "%s", "payload");
        }
        return NULL;
    }

    retObj = ism_config_json_strToObject(payload, rc);
    return retObj;
}

/**
 * Returns pointer to the object instance in json root, else NULL
 */
XAPI json_t * ism_config_json_findObjectInRoot(json_t *root, const char *objName, const char *instName, const char *itemName, int *type) {
    json_t *retval = NULL;
    *type = JSON_NULL;

    if ( !objName || !root )
        return retval;

    /* Find object type */
    json_t *object = json_object_get(root, objName);

    if ( object ) {
        /* check if instance name is specified */
        if ( instName ) {
            /* Instance could be either an object (in case of map of objects)
             * or array of objects
             */
            if ( json_typeof(object) == JSON_ARRAY ) {
                int i = 0;
                /* Loop thru the array and check Name to find the required instance */
                for (i=0; i<json_array_size(object); i++) {
                    json_t *inst = json_array_get(object, i);
                    json_t *nameObj = json_object_get(inst, "Name");
                    if ( nameObj && instName && !strcmp(json_string_value(nameObj), instName)) {
                        /* Check if item name is specified */
                        if ( itemName ) {
                            /* Get value of the required item */
                            json_t *item = json_object_get(inst, itemName);
                            if ( item ) {
                                retval = item;
                                break;
                            }
                        } else {
                            retval = inst;
                            break;
                        }
                    }
                }
            } else if ( json_typeof(object) == JSON_OBJECT ) {
                /* find requested instance from object map */
                json_t *inst = json_object_get(object, instName);
                if ( inst ) {
                    /* if item name is specified, find item */
                    if ( itemName ) {
                        /* Get value of the required item */
                        json_t *item = json_object_get(inst, itemName);
                        if ( item ) {
                            retval = item;
                        }
                    } else {
                        retval = inst;
                    }
                }
            }
        } else {
            /* Singleton object */
            retval = object;
        }
    }

    if ( retval )
        *type = json_typeof(retval);

    return retval;
}

/**
 * Returns copy or pointer of configuration object.
 * Caller should free json object using json_decref() API if requested a copy.
 */
XAPI json_t * ism_config_json_getObject(const char *objName, const char *instName, const char *itemName, int mode, int *type) {
    *type = JSON_NULL;
    json_t *curval = NULL;


    if ( !objName || !srvConfigRoot )
        return curval;

    /* check current configuration */

    *type = JSON_NULL;

    /* Find object type */
    json_t *object = json_object_get(srvConfigRoot, objName);

    if ( object ) {
        /* check if instance name is specified */
        if ( instName ) {
            /* Instance could be either an object (in case of map of objects)
             * or array of objects
             */
            if ( json_typeof(object) == JSON_ARRAY ) {
                int i = 0;
                /* Loop thru the array and check Name to find the required instance */
                for (i=0; i<json_array_size(object); i++) {
                    json_t *inst = json_array_get(object, i);
                    json_t *nameObj = json_object_get(inst, "Name");
                    if ( nameObj && instName && !strcmp(json_string_value(nameObj), instName)) {
                        /* Check if item name is specified */
                        if ( itemName ) {
                            /* Get value of the required item */
                            json_t *item = json_object_get(inst, itemName);
                            if ( item ) {
                                if ( mode == 1 ) {
                                    curval = json_deep_copy(item);
                                } else {
                                    curval = item;
                                }
                                *type = json_typeof(item);
                                break;
                            }
                        } else {
                            /* return instance object */
                            if ( mode == 1 ) {
                                curval = json_deep_copy(inst);
                            } else {
                                curval = inst;
                            }
                            *type = JSON_OBJECT;
                            break;
                        }
                    }
                }
            } else if ( json_typeof(object) == JSON_OBJECT ) {
                /* find requested instance from object map */
                json_t *inst = json_object_get(object, instName);
                if ( inst ) {
                    /* if item name is specified, find item */
                    if ( itemName ) {
                        /* Get value of the required item */
                        json_t *item = json_object_get(inst, itemName);
                        if ( item ) {
                            if ( mode == 1 ) {
                                curval = json_deep_copy(item);
                            } else {
                                curval = item;
                            }
                            *type = json_typeof(item);
                        }
                    } else {
                        /* return instance object */
                        if ( mode == 1 ) {
                            curval = json_deep_copy(inst);
                        } else {
                            curval = inst;
                        }
                        *type = JSON_OBJECT;
                    }
                }
            }
        } else {
            /* Singleton object - set type to object type */
            if ( mode == 1 ) {
                curval = json_deep_copy(object);
            } else {
                curval = object;
            }
            *type = json_typeof(object);
        }
    }

    return curval;
}


/**
 * Merge current and persisted configuration data and returns a copy or pointer.
 * If object exist in the server configuration, the data is returned from server config.
 * If object doesn't exist in server config, the will check if object exist in currRequest.
 * Caller should free json object using json_decref() API.
 */
XAPI json_t * ism_config_json_getMergedObject(const char *objName, const char *instName, const char *itemName, json_t *passedObj, int *type) {
    json_t *retval = NULL;
    *type = JSON_NULL;
    json_t *curval = NULL;


    if ( !objName || !srvConfigRoot )
        return retval;

    /* check currRequest if specified */
    if ( passedObj != NULL ) {
        json_t *tmpObj = NULL;
        int dataType = 0;
        if ( !strcmp(objName, "AdminEndpoint") || !strcmp(objName, "LDAP") ||
             !strcmp(objName, "HighAvailability") || !strcmp(objName, "ClusterMembership") ||
             !strcmp(objName, "Syslog") || !strcmp(objName, "MQCertificate") ||
             !strcmp(objName, "ResourceSetDescriptor"))
        {
            tmpObj = ism_config_json_findObjectInRoot(passedObj, objName, NULL, itemName, &dataType);
        } else {
            tmpObj = ism_config_json_findObjectInRoot(passedObj, objName, instName, itemName, &dataType);
        }

        if ( tmpObj ) {
            retval = json_deep_copy(tmpObj);
        }
    }

    /* get current value */
    json_t *tmpCurObj = json_object_get(srvConfigRoot, objName);
    if ( tmpCurObj ) {
        curval = json_object_get(tmpCurObj, instName);
    }

    /* if currRequest is NULL, return persisted configuration */
    if ( retval == NULL && curval ) {
        retval = json_deep_copy(curval);
        return retval;
    }

    /* If merged option is specified */
    if ( retval && curval ) {
        /* merge data - data in currRequest takes precedence */
        /* Add missing items from persisted configuration */
        void *itemIter = json_object_iter(curval);
        while (itemIter) {
            const char *itemkey = json_object_iter_key(itemIter);
            json_t *itemval = json_object_iter_value(itemIter);

            /* check data in currRequest */
            json_t *tmpobj = json_object_get(retval, itemkey);
            if ( tmpobj == NULL && itemval != NULL ) {
                /* Add data in retval */
                json_t *addval = json_deep_copy(itemval);
                json_object_set_new(retval, itemkey, addval);
            }
            itemIter = json_object_iter_next(curval, itemIter);
        }
    }

    return retval;
}

/**
 * Merge current and persisted configuration array data and returns a copy or pointer.
 * Based on persisted configuration array, if an entry of the current array does not exist
 * in the persisted configuration array, it will be insert into merged array.
 * Caller should free json object using json_decref() API.
 */
XAPI json_t * ism_config_json_getMergedArray(const char *objName, json_t *passedObj, int *rc) {
    json_t *retval = NULL;
    json_t *curval = NULL;

    if ( !srvConfigRoot ) {
        TRACE(3, "%s: server configuration root is null.\n", __FUNCTION__);
        *rc = ISMRC_ObjectNotFound;
        ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", objName, "null");
        goto GET_MERGEARRAY_END;
    }

    /* check currRequest if specified */
    if ( passedObj != NULL ) {
        json_t *tmpObj = json_object_get(passedObj, objName);
        if ( tmpObj ) {
            if (json_typeof(tmpObj) != JSON_ARRAY) {
                TRACE(3, "%s: Configuration object %s in payload is not provided as JSON_ARRAY. it is %s\n", __FUNCTION__, objName, ism_config_json_typeString(json_typeof(tmpObj)));
                *rc = ISMRC_BadPropertyType;
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", objName, "null", "null", ism_config_json_typeString(json_typeof(tmpObj)));
                goto GET_MERGEARRAY_END;
            }
            retval = json_deep_copy(tmpObj);
        }
    }

    /* get current value */
    curval = json_object_get(srvConfigRoot, objName);

    /* if currRequest is NULL, return persisted configuration */
    if ( retval == NULL && curval ) {
        retval = json_deep_copy(curval);
        return retval;
    }

    /* If merged option is specified */
    if ( retval && curval ) {
        if (json_typeof(curval) != JSON_ARRAY) {
            TRACE(3, "%s: Configuration object %s in configuration repository is not defined as JSON_ARRAY. it is %s\n", __FUNCTION__, objName, ism_config_json_typeString(json_typeof(curval)));
            *rc = ISMRC_BadPropertyType;
            ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", objName, "null", "null", ism_config_json_typeString(json_typeof(curval)));
            goto GET_MERGEARRAY_END;
        }

        /* merge data - data in currRequest takes precedence */
        /* Add missing items from persisted configuration */
        int i = 0;
        for (i=0; i<json_array_size(curval); i++) {
            json_t *instRoot = json_array_get(curval, i);

            if (!strcmp(objName, "TopicMonitor") || !strcmp(objName, "ClusterRequestedTopics")) {
                const char *tStr = json_string_value(instRoot);
                ism_updateJsonStringArray(retval, tStr);

            } else if (!strcmp(objName, "TrustedCertificate")) {
                ism_updateJsonObjectArray(ISM_TRUSTEDCERT, retval, instRoot);

            } else if (!strcmp(objName, "ClientCertificate")) {
                ism_updateJsonObjectArray(ISM_CLIENTCERT, retval, instRoot);

            } else {
                TRACE(3, "%s: Configuration object %s is not supported.\n", __FUNCTION__, objName);
                *rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(ISMRC_ArgNotValid, "%s", objName);
                goto GET_MERGEARRAY_END;
            }
        }
    }

GET_MERGEARRAY_END:
    return retval;
}

/**
 * Add or update object, instance of an object or item of an instance
 */
XAPI int ism_config_json_setObject(char *objName, char *instName, char *itemName, int type, void *newvalue) {
    int         rc = ISMRC_OK;
    json_t     *objRoot  = NULL;
    const char *objKey;
    int         objType;
    int         objFound  = 0;
    json_t     *instRoot  = NULL;
    const char *instKey;
    int         itemFound = 0;
    json_t     *newItem = NULL;

    if ( !srvConfigRoot || !objName ) {
        return ISMRC_NullPointer;
    }

    /* Process singleton */

    if ( !instName ) {
        newItem = ism_config_json_createObject(type, newvalue);

        /* check if object exist */
        objRoot = json_object_get(srvConfigRoot, objName);

        if (!objRoot) {
            /* Add object */
            json_object_set_new(srvConfigRoot, objName, newItem);
        } else {
            /* Update object */
            json_object_set(srvConfigRoot, objName, newItem);
        }

        return rc;
    }


    /* Process composite */

    /* Iterate thru the node */
    void *rootIter = json_object_iter(srvConfigRoot);
    while(rootIter)
    {
        objKey = json_object_iter_key(rootIter);
        objRoot = json_object_iter_value(rootIter);
        objType = json_typeof(objRoot);

        /* check key matches item */
        if ( objKey && !strcmp(objKey, objName)) {

            /* Make sure item is an array or object */
            if ( objType != JSON_ARRAY && objType != JSON_OBJECT ) {
                rootIter = json_object_iter_next(srvConfigRoot, rootIter);
                continue;
            }

            objFound = 1;

            /* check if this is the required object */
            if ( json_object_get(objRoot, instName) == NULL ) {
                rootIter = json_object_iter_next(srvConfigRoot, rootIter);
                continue;
            }

            TRACE(9, "Got object: %s\n", instName);

            objFound = 1;

            void *instIter = json_object_iter(objRoot);

            while (instIter) {
                instKey = json_object_iter_key(instIter);
                instRoot = json_object_iter_value(instIter);

                if ( objName && instKey && !strcmp(instKey, objName)) {
                    itemFound = 1;

                    /* Update object instance */
                    TRACE(9, "Update instance %s\n", instKey);

                    /* create new json object for newvalue */
                    newItem = ism_config_json_createObject(type, newvalue);

                    /* Update */
                    if ( json_object_set(instRoot, itemName, newItem) < 0 ) {
                        /* Update failed */
                        TRACE(9, "Update ERROT item %s %s %s\n", objName, instName, itemName);
                    } else {
                        TRACE(9, "Updated item %s %s %s\n", objName, instName, itemName);
                    }

                    break;
                }

                instIter = json_object_iter_next(objRoot, instIter);
            }
            if ( itemFound == 0 ) {
                /* Add Item */
                newItem = ism_config_json_createObject(type, newvalue);
                json_object_set_new(instRoot, itemName, newItem);
                TRACE(9, "Added new item %s %s %s\n", objName, instName, itemName);
                break;
            }
        }


        if ( objFound == 1 )
            break;

        rootIter = json_object_iter_next(srvConfigRoot, rootIter);
    }

    /* if objectName doesn't exist, create an object, add item and
     * add object in the array
     */
    if ( objFound ==  0 ) {
        /* create new instance of object to add items */
        json_t *newinst = json_object();
        json_object_set_new(newinst, itemName, json_string((char *)newvalue));
        /* Add instance as object map in root */
        json_t *newmap = json_object();
        json_object_set_new(newmap, instName, newinst);
        /* Add object map to root */
        json_object_set_new(srvConfigRoot, objName, newmap);
        TRACE(9, "Added MAP: %s %s %s\n", objName, instName, itemName);
    }

    return rc;
}


/**
 * Returns a singleton object
 */
XAPI json_t * ism_config_json_getSingleton(const char *object) {
    json_t *retval = NULL;
    if ( object ) {
        retval = json_object_get(srvConfigRoot, object);
    }

    return retval;
}

/**
 * Returns a composite object in server config structure
 */
XAPI json_t * ism_config_json_getComposite(const char *object, const char *name, int getLock) {
    json_t *retval = NULL;

    if ( getLock == 1 )
        pthread_rwlock_rdlock(&srvConfiglock);

    /* get object node */
    json_t *objRoot = json_object_get(srvConfigRoot, object);
    if ( objRoot ) {
        /* get instance node */
        retval = json_object_get(objRoot, name);
    }

    if ( getLock == 1 )
        pthread_rwlock_unlock(&srvConfiglock);

    return retval;
}


/**
 * Returns a composite object in server config structure
 */
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval) {
    int rc = ISMRC_OK;

    /* get object node */
    json_t *objRoot = json_object_get(srvConfigRoot, object);
    if ( objRoot ) {
        /* set instance node */
        json_object_set_new(objRoot, name, objval);
    } else {
        /* Create object and add instance */
        json_t *newobj = json_object();
        json_object_set_new(srvConfigRoot, object, newobj);
        json_object_set_new(newobj, name, objval);
    }

    return rc;
}

/**
 * Returns a composite object in server config structure
 */
XAPI json_t * ism_config_json_getCompositeItem(const char *object, const char *name, char *item, int getLock) {
    json_t *retval = NULL;

    if ( !object || *object == '\0' || !name || *name == '\0' || !item || *item == '\0' )
        return retval;

    if ( getLock == 1 )
        pthread_rwlock_rdlock(&srvConfiglock);

    /* get object node */
    json_t *objRoot = json_object_get(srvConfigRoot, object);
    if ( objRoot ) {
        /* get instance node */
        json_t *inst = json_object_get(objRoot, name);
        if ( inst ) {
            retval = json_object_get(inst, item);
        }
    }

    if ( getLock == 1 )
        pthread_rwlock_unlock(&srvConfiglock);

    return retval;
}



/* returns JSON data type based on value set for key "Type" */
static int getTypeKeyValFromObject(json_t *obj) {
    int type = JSON_NULL;

    if (obj && json_typeof(obj) == JSON_STRING) {
        const char *val = json_string_value(obj);
        if ( *val == 'S' || *val == 's' )
            type = JSON_STRING;
        else if ( *val == 'E' || *val == 'e' )
            type = JSON_STRING;
        else if ( *val == 'L' || *val == 'l' )
            type = JSON_STRING;
        else if ( *val == 'N' || *val == 'n' ) {
                if (!strcmpi(val, "Number")) {
                        type = JSON_INTEGER;
                } else {
                        type = JSON_STRING;
                }
        } else if ( *val == 'B' || *val == 'b' ) {
            if ( !strcmpi(val, "Boolean")) {
                type = JSON_BOOLEAN_TYPE;
            } else {
                type = JSON_STRING;
            }
        } else if ( *val == 'I' || *val == 'i' ) {
            if ( !strcmpi(val, "IPAddress")) {
                type = JSON_STRING;
            } else {
                type = JSON_INTEGER;
            }
        }
    }

    return type;
}

/* returns JSON data type from configuration schema */
XAPI int ism_config_getJSONObjectTypeFromSchema(char *object, char *item) {

    int type = JSON_NULL;

    if ( object && item ) {
        /* get object node */
        json_t *obj = json_object_get(serverConfigSchema, object);
        if ( obj ) {
            /* get item node */
            json_t *itm = json_object_get(obj, item);
            if ( itm ) {
                /* get Type of item */
                json_t *t = json_object_get(itm, "Type");
                type = getTypeKeyValFromObject(t);
            }
        }
    } else {
        json_t *itm = json_object_get(serverConfigSchema, item);
        if ( itm ) {
            /* get Type of item */
            json_t *t = json_object_get(itm, "Type");
            type = getTypeKeyValFromObject(t);
        }
    }

    return type;
}

/*
 * Add update singleton configuration item in JSON configuration root node.
 * Make sure to get rwlock on srvConfiglock before call this function.
 */
XAPI int ism_config_jsonAddUpdateSingletonProps(char *item, void *newvalue) {
    int rc = ISMRC_OK;

    if ( !srvConfigRoot || !item )
        return -1;

    int schemaObjType = ism_config_getJSONObjectTypeFromSchema(NULL, item);

    /* check for exitsting data */
    json_t * curObj = json_object_get(srvConfigRoot, item);

    if ( curObj ) {
        /* Update value */
        json_t *newobj = ism_config_json_createObject(schemaObjType, newvalue);

        if ( json_object_set(srvConfigRoot, item, newobj) < 0 ) {
            TRACE(4, "Configuration: item update ERROR: %s\n", item);
            return ISMRC_Error;
        } else {
            TRACE(9,  "Configuration: updated Item: %s\n", item);
        }
    } else {
        /* Add a new singleton object */
        json_t *newobj = ism_config_json_createObject(schemaObjType, newvalue);
        if ( json_object_set_new(srvConfigRoot, item, newobj) < 0 ) {
            TRACE(4,  "Configuration: added Item ERROR: %s\n", item);
            return ISMRC_Error;
        } else {
            TRACE(7,  "Configuration: added Item: %s\n", item);
        }
    }

    return rc;
}

/**
 * Add update composite configuration object in JSON configuration root node.
 * Make sure to get rwlock on srvConfiglock before call this function.
 */
XAPI int ism_config_jsonAddUpdateCompositeProps(char *object, char *objname, char *item, void *newvalue, int isJSONValue ) {
    int         rc = ISMRC_OK;
    json_t     *objRoot  = NULL;
    json_t     *instRoot = NULL;
    json_t     *itemRoot = NULL;
    json_t     *newItem = NULL;

    if ( !srvConfigRoot || !item ) {
        return ISMRC_ArgNotValid;
    }

    int schemaObjType = ism_config_getJSONObjectTypeFromSchema(object, item);

    /* check if object exist in root */
    objRoot = json_object_get(srvConfigRoot, object);
    if ( !objRoot ) {
        if ( strcmp( item, "TopicString") ) {
            /* create a new instance of this object */
            json_t *newinst = json_object();
            if ( isJSONValue == 1 ) {
                newItem = (json_t *)newvalue;
            } else {
                newItem = ism_config_json_createObject(schemaObjType, newvalue);
            }
            json_object_set_new(newinst, item, newItem);
            /* Add instance as object map in root */
            json_t *newmap = json_object();
            json_object_set_new(newmap, objname, newinst);
            /* Add object map to root */
            json_object_set_new(srvConfigRoot, object, newmap);
            TRACE(6, "Configuration: created Object: %s %s %s\n", object, objname, item);
            return rc;
        } else {
            /* Add TopicMonitor */
            json_t *newinst = json_array();
            json_array_append_new(newinst, json_string((char *)newvalue));
            json_object_set_new(srvConfigRoot, object, newinst);
            TRACE(6, "Configuration: created Object: %s %s %s\n", object, objname?objname:"NULL", item);
            return rc;
        }
    }

    /* Object is found - check for instance */
    if (objname) {
        instRoot = json_object_get(objRoot, objname);
    } else {
        /* check if TopicMonitor */
        if ( strcmp( item, "TopicString") == 0 ) {
            int i = 0;
            int found = 0;
            for (i=0; i<json_array_size(objRoot); i++) {
                instRoot = json_array_get(objRoot, i);
                char *tStr = (char *)json_string_value(instRoot);
                if ( strcmp(tStr, (char *)newvalue) == 0 ) {
                    found = 0;
                    break;
                }
            }
            if ( found == 0 ) {
                json_array_append_new(objRoot, json_string((char *)newvalue));
            }
        }
        return rc;
    }
    if ( !instRoot ) {
        /* create new instance */
        json_t *newinst = json_object();
        if ( isJSONValue == 1 ) {
            newItem = (json_t *)newvalue;
        } else {
            newItem = ism_config_json_createObject(schemaObjType, newvalue);
        }
        json_object_set_new(newinst, item, newItem);
        /* Add object map to root */
        json_object_set_new(objRoot, objname, newinst);
        TRACE(6, "Configuration: created Object: %s %s %s\n", object, objname, item);
        return rc;
    }

    /* create new json object for newvalue */
    if ( isJSONValue == 1 ) {
        newItem = (json_t *)newvalue;
    } else {
        newItem = ism_config_json_createObject(schemaObjType, newvalue);
    }

    /* Check for item */
    itemRoot = json_object_get(instRoot, item);
    if ( itemRoot ) {
        /* update item value */
        if ( json_object_set(instRoot, item, newItem) < 0 ) {
            /* Update failed */
            TRACE(4, "Configuration: Object update ERROR: %s %s %s %d\n", object, objname, item, schemaObjType);
            rc = ISMRC_Error;   /* TODO: Create a new RC for updated failures */
        } else {
            TRACE(9, "Configuration: Object updated: %s %s %s %d\n", object, objname, item, schemaObjType);
        }
    } else {
        /* create new item */
        json_object_set_new(instRoot, item, newItem);
        TRACE(9, "Configuration: added Item: %s %s %s %d\n", object, objname, item, schemaObjType);
    }

    return rc;
}

/**
 * Add properties to JSON configuration data from a string containing data in key=value pairs
 * as supported in v1.x release.
 */
XAPI int ism_config_convertV1PropsStringToJSONProps(char *propStr, int getConfigLock) {
    int rc = ISMRC_OK;
    char * keyword = NULL;
    char * value = NULL;
    char * more = NULL;
    char * p = NULL;

    if ( !propStr || *propStr == '\0' )
        return rc;

    /* get lock */
    if ( getConfigLock )
        pthread_rwlock_wrlock(&srvConfiglock);

    keyword = ism_common_getToken(propStr, " \t\r\n", "=\r\n", &more);
    if (keyword && keyword[0]!='*' && keyword[0]!='#') {
        char * cp = keyword+strlen(keyword); /* trim trailing white space */
        while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
            cp--;
        *cp = 0;
        value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);

        ism_common_canonicalName(keyword);

        p = keyword;
        while(*p) {
            if (*p == '.')  {
                *p = 0;
                p++;
                keyword = p;
                break;
            }
            p++;
        }

        if (!keyword) {

            TRACE(3, "NULL keyword in dynamic configuration file.\n");
            goto FUNC_END;

        } else {

            int len = strlen(keyword) + 1;
            char *tmpstr = alloca(len);
            memcpy(tmpstr, keyword, len);
            tmpstr[len-1] = 0;
            char *nexttoken = NULL;
            char *object = strtok_r(tmpstr, ".", &nexttoken);
            char *item = strtok_r(NULL, ".", &nexttoken);
            char *tmpname = strtok_r(NULL, ".", &nexttoken);
            char *name = NULL;

            if (object && item && tmpname) {

                /* Process composite object */
                len = strlen(object) + strlen(item) + 2;
                name = (char *)keyword + len;

                if ( ( *item == 'N' && strcmp(item, "Name") == 0 ) ||  ( *item == 'U' && strcmp(item, "UID") == 0 ) ) {
                    goto FUNC_END;
                } else if ( ((*object == 'T' && !strcmp(object, "TopicMonitor")) || (*object == 'C' && !strcmp(object, "ClusterRequestedTopics")))
                           && !strcmp(item, "TopicString") ) {
                    ism_config_jsonAddUpdateCompositeProps(object, NULL, item, (void *)value, 0);
                } else {
                    ism_config_jsonAddUpdateCompositeProps(object, name, item, (void *)value, 0);
                }

            } else {

                if ( value ) {
                    /* Process singleton object */
                    ism_config_jsonAddUpdateSingletonProps((char *)object, (void *)value);
                }

            }
        }
    }

FUNC_END:

    /* Release lock */
    if ( getConfigLock )
        pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}


/**
 * Read a configuration file as keyword=value and convert into JSON object
 */
XAPI int ism_config_convertPropsToJSON(const char * filename) {
    FILE * f = NULL;
    size_t len = 0;
    int    rc = ISMRC_OK;
    char *line = NULL;

    if ( !filename ) {
        ism_common_setError(ISMRC_ArgNotValid);
        return ISMRC_ArgNotValid;
    }

    /* check arguments */
    if ( !filename ) {
        ism_common_setError(ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    /* open and process configuration file in key=value pairs*/
    TRACE(5, "Process configuration file in key=value pair format: %s\n", filename);

    f = fopen(filename, "rb");
    if (!f) {
        TRACE(5, "Configuration file in key=value pair format is not found.\n");
        ism_common_setError(ISMRC_NotFound);
        return ISMRC_NotFound;
    }

    pthread_rwlock_wrlock(&srvConfiglock);

    int getConfigLock = 0;

    rc = getline(&line, &len, f);
    while (rc >= 0) {
        ism_config_convertV1PropsStringToJSONProps(line, getConfigLock);
        rc = getline(&line, &len, f);
    }

    fclose(f);

    if (line)
        ism_common_free_raw(ism_memory_admin_misc,line);

    /* Update configuration file */
    if ( srvConfigRoot != NULL ) {
        rc = ism_config_json_updateFile(getConfigLock);
    }

    pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}


XAPI int ism_config_json_addItemPropToList(int type, const char *objKey, json_t *elem, ism_prop_t *props) {
    int rc = ISMRC_OK;
    ism_field_t var = {0};

    if ( !elem || !props )
        return ISMRC_NullPointer;

    /* set property */
    switch(type) {
    case JSON_STRING:  /* JSON string, value is UTF-8  */
        var.type = VT_String;
        var.val.s = (char *)json_string_value(elem);
        ism_common_setProperty(props, objKey, &var);
        break;
    case JSON_INTEGER: /* Number with no decimal point */
        var.type = VT_Integer;
        var.val.i = json_integer_value(elem);
        ism_common_setProperty(props, objKey, &var);
        break;
    case JSON_REAL:  /* Number which is too big or has a decimal */
        var.type = VT_Double;
        var.val.d = json_real_value(elem);
        ism_common_setProperty(props, objKey, &var);
        break;

    case JSON_TRUE:    /* JSON true */
        var.type = VT_String;
        var.val.s = "true";
        ism_common_setProperty(props, objKey, &var);
        break;
    case JSON_FALSE:   /* JSON false */
        var.type = VT_String;
        var.val.s = "false";
        ism_common_setProperty(props, objKey, &var);
        break;
    case JSON_NULL:
        var.type = VT_Null;
        var.val.s = NULL;
        ism_common_setProperty(props, objKey, &var);
        break;
    default:
        rc = ISMRC_ArgNotValid;
        break;
    }
    return rc;
}

/*
 * Remove spaces around items in a string list and update the jansson object if modified
 *
 * The value must be a copy
 */
int ism_config_fixCommaList(json_t * obj, const char * key, char * value) {
    /* It is most common for a list to contain no spaces, so do nothing */
    if (!strchr(value, ' '))
        return 0;

    int updated = 0;        /* The string has been changed */
    int remove = 1;         /* Remove spaces (after comma or at start) */
    char * op = value;      /* Output pointer */
    char * vp = value;      /* Input pointer */
    char * sp = NULL;       /* Trailing space location */

    /* Loop for each character in the value */
    while (*vp) {
        if (*vp == ',') {   /* If it is a comma */
            if (sp) {
                op = sp;
                updated = 1;
                sp = NULL;
            }
            *op++ = *vp;
            remove = 1;
        } else if (isspace(*vp)) {   /* If it is a space or tab */
            if (!sp)
                sp = op;
            if (!remove) {
                *op++ = *vp;
            } else {
                updated = 1;
            }
        } else {            /* If it is not a space or comma */
            sp = NULL;
            *op++ = *vp;
            remove = 0;
        }
        vp++;
    }
    if (sp) {               /* Remove trailing spaces */
        op = sp;
        updated = 1;
    }
    *op = 0;                /* Null terminate */

    /* If the value has changed and we have a json object, update it */
    if (updated && obj && key) {
        json_t * jvalue = json_string(value);
        json_object_set(obj, key, jvalue);
    }
    return updated;
}

/*
 * Create ISM property from JSON element and add to the property list.
 */
XAPI int ism_config_json_addPropsToList(json_t *elem, const char *objectName, const char *instName, const char *item, ism_prop_t * props, int mode) {
    int rc = ISMRC_OK;
    char *objKey = NULL;
    int objlen = 0;
    int instlen = 0;
    int itemlen = 0;
    int objType = JSON_NULL;

    if ( !elem || !objectName ) {
        return ISMRC_NullArgument;
    }

    /* get object type */
    objType = json_typeof(elem);

    /* set length for key in property */
    objlen = strlen(objectName);
    if ( instName ) instlen = strlen(instName);
    if ( item ) itemlen = strlen(item);
    ism_field_t f;

    if (mode == 2) {
        f.type = VT_Boolean;
        f.val.i = 1;
    }

    if ( instName ) {
        if ( item ) {
            /* need to add property of only one item */
            if ( !instName ) {
                TRACE(5, "NULL instance name: object:%s\n", objectName?objectName:"NULL");
                rc = ISMRC_ArgNotValid;
            } else {
                objKey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,244),objlen + instlen + itemlen + 3);
                if ( mode == 1 ) {
                    if ( !strcmp(objectName, "AdminEndpoint") && !strcmp(item, "ConfigurationPolicies")) {
                        sprintf(objKey, "%s.%s.%s", objectName, "ConnectionPolicies", instName);
                    } else {
                        sprintf(objKey, "%s.%s.%s", objectName, item, instName);
                    }
                } else {
                    strcpy(objKey, item);
                }
                rc = ism_config_json_addItemPropToList(objType, objKey, elem, props);
            }
        } else {
            if (mode == 2) {
                /* Just the name */
                ism_common_addProperty(props, instName, &f);
            } else {
                /* Add the name */
                objKey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,245),objlen + instlen + 7);
                if ( mode == 0 ) {
                    sprintf(objKey, "%s.Name.%s", objectName, instName);
                } else {
                    strcpy(objKey, "Name");
                }
                f.type = VT_String;
                f.val.s = (char *)instName;
                ism_common_setProperty(props, objKey, &f);

                /* need to add all property of the instance */
                void *instIter = json_object_iter(elem);
                while (instIter) {
                    const char * itemKey = json_object_iter_key(instIter);
                    json_t * itemObj = json_object_iter_value(instIter);
                    int itemType = json_typeof(itemObj);

                    /* item can not be JSON_OBJECT or JSON_ARRAY */
                    if ( !itemKey || itemType == JSON_OBJECT || itemType == JSON_ARRAY ) {
                        TRACE(5, "Invalid object type: %d for itemKey:%s object:%s name:%s\n", itemType, itemKey?itemKey:"NULL", objectName?objectName:"NULL", instName?instName:"NULL");
                        rc = ISMRC_ArgNotValid;
                        break;
                    }
                    itemlen = strlen(itemKey);
                    objKey = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,246),objKey, objlen + instlen + itemlen + 3);

                    if ( mode == 0 ) {
                        if ( !strcmp(objectName, "AdminEndpoint") && !strcmp(itemKey, "ConfigurationPolicies")) {
                            sprintf(objKey, "%s.%s.%s", objectName, "ConnectionPolicies", instName);
                        } else {
                            sprintf(objKey, "%s.%s.%s", objectName, itemKey, instName);
                        }
                    } else {
                        strcpy(objKey, itemKey);
                    }
                    rc = ism_config_json_addItemPropToList(itemType, objKey, itemObj, props);
                    if ( rc != ISMRC_OK )
                        break;
                    instIter = json_object_iter_next(elem, instIter);
                }
            }
        }
    } else {
        if ( item ) {
            /* single object */
            objKey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,247),objlen + itemlen + 3);
            sprintf(objKey, "%s.%s", objectName, item);
            rc = ism_config_json_addItemPropToList(objType, objKey, elem, props);
        } else {
            /* elem include complete configuration of object */
            if ( objType == JSON_OBJECT ) {
                void *objIter = json_object_iter(elem);
                while (objIter) {
                    const char * instKey = json_object_iter_key(objIter);
                    json_t * instObj = json_object_iter_value(objIter);
                    int instType = json_typeof(instObj);

                    if ( instType == JSON_OBJECT ) {
                        if (mode == 2) {
                            ism_common_addProperty(props, instKey, &f);
                        } else {
                            /* Add Name */
                            instlen = strlen(instKey);
                            objKey = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,248),objKey, objlen + instlen + 7);
                            if ( mode == 0 ) {
                                sprintf(objKey, "%s.Name.%s", objectName, instKey);
                            } else {
                                strcpy(objKey, "Name");
                            }
                            f.type = VT_String;
                            f.val.s = (char *)instKey;
                            ism_common_setProperty(props, objKey, &f);

                            void *instIter = json_object_iter(instObj);
                            while (instIter) {
                                const char * itemKey = json_object_iter_key(instIter);
                                json_t *itemObj = json_object_iter_value(instIter);
                                int itemType = json_typeof(itemObj);

                                /* item can not be JSON_OBJCET or JSON_ARRAY */
                                if ( itemType == JSON_OBJECT || itemType == JSON_ARRAY ) {
                                    rc = ISMRC_ArgNotValid;
                                    break;
                                }
                                itemlen = strlen(itemKey);
                                objKey = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,249),objKey, objlen + instlen + itemlen + 3);
                                if ( mode == 0 ) {
                                    sprintf(objKey, "%s.%s.%s", objectName, itemKey, instKey);
                                } else {
                                    strcpy(objKey, itemKey);
                                }
                                rc = ism_config_json_addItemPropToList(itemType, objKey, itemObj, props);
                                if ( rc != ISMRC_OK )
                                    break;

                                instIter = json_object_iter_next(instObj, instIter);
                            }
                            if ( rc != ISMRC_OK )
                                break;
                        }
                    }
                    objIter = json_object_iter_next(elem, objIter);
                }
            } else if ( objType == JSON_STRING) {
                if (!strcmp(objectName, "TopicMonitor") || !strcmp(objectName, "ClusterRequestedTopics")) {
                    objKey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,250),objlen + strlen("TopicString") + 4);
                    // Engine code only expect TopicMonitor.TopicString.xx format
                    sprintf(objKey, "%s.%s.1", objectName, "TopicString" );
                    rc = ism_config_json_addItemPropToList(objType, objKey, elem, props);
                } else {
                    rc = ism_config_json_addItemPropToList(objType, objectName, elem, props);
                }
            } else  if ( objType == JSON_ARRAY ) {
                if ( !strcmp(objectName, "TopicMonitor") || !strcmp(objectName, "ClusterRequestedTopics")) {
                    /* loop thru TopicMonitor array and add TopicString to props */
                    int i = 0;
                    objKey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,251),64);
                    for (i=0; i<json_array_size(elem); i++) {
                        json_t *instObj = json_array_get(elem, i);
                        if (instObj) {
                            const char *val = json_string_value(instObj);
                            /* Engine code only expect TopicMonitor.TopicString.xx or ClusterRequestedTopics.TopicString.xx format */
                            sprintf(objKey, "%s.TopicString.%d", objectName, i );
                            f.type = VT_String;
                            f.val.s = (char *)val;
                            ism_common_setProperty(props, objKey, &f);
                        }
                    }
                } else if ( strcmp(objectName, "TrustedCertificate") && strcmp(objectName, "ClientCertificate")) {
                    TRACE(1, "Invalid use of type for object: %s\n", objectName);
                    return ISMRC_ArgNotValid;
                }
            } else {
                /* single object with objectName */
                rc = ism_config_json_addItemPropToList(objType, objectName, elem, props);
            }
        }
    }

    ism_common_free(ism_memory_admin_misc,objKey);
    return rc;
}

/**
 * Check if object exist in root configuration object.
 * If function can not find the object in cached configuration, and requestRoot is not NULL,
 * it will also check requestRoot.
 */
XAPI int ism_config_objectExist(char *objectType, char *objectName, json_t *currRequest) {
    int found = 0;

    if ( !objectType )
        return found;

    /* get object from current configuration */

    /* find object */
    json_t *tmpobj = json_object_get(srvConfigRoot, (const char *)objectType);
    if ( tmpobj ) {
        /* find instance */
        if ( objectName ) {
            json_t *tmpinst = json_object_get(tmpobj, objectName);
            if ( tmpinst ) found = 1;
        } else {
            found = 1;
        }
    }

    /* check requestRoot if not found yet */
    if ( found == 0 && currRequest != NULL ) {
        int dataType = 0;
        json_t *tmpObj = ism_config_json_findObjectInRoot(currRequest, objectType, objectName, NULL, &dataType);
        if ( tmpObj ) found = 1;
    }

    return found;
}



/* Returns index if object is defined.
 * Returns -1 on error.
 */
XAPI int ism_config_getValidationDataIndex(char *objName) {
    int i = 0;

    if ( !objName ) return -1;

    for (i=0; i<NOVALIDATIONDATA; i++) {
        if ( !strcmp(objName, ismConfigValidationData[i].objectName)) {
            return i;
        }
    }

    return -1;
}

/* Returns component type of the object */
XAPI int ism_config_getValidationDataCompType(char *objName) {
    int i = 0;

    if ( !objName ) return ISM_CONFIG_COMP_LAST;

    for (i=0; i<NOVALIDATIONDATA; i++) {
        if ( !strcmp(objName, ismConfigValidationData[i].objectName)) {
            return ismConfigValidationData[i].compType;
        }
    }

    return ISM_CONFIG_COMP_LAST;
}

/*
 * Check for a slash in the object name.
 * Some objects allow slashes
 */
int checkNameSlash(const char * objtype, const char * object) {
    /* If no slash in name, return good */
    if (!object || !strchr(object, '/'))
        return 0;
    /* Check for objects which allow slash in the name */
    if (!strcmp(objtype, "TopicMonitor") ||
        !strcmp(objtype, "Queue") ||
        !strcmp(objtype, "Subscription") ||
        !strcmp(objtype, "AdminSubscription") ||
        !strcmp(objtype, "ClusterRequestedTopics") ||
        !strcmp(objtype, "DurableNamespaceAdminSub") ||
        !strcmp(objtype, "NonpersistentAdminSub")) {
        return 0;
    }
    if (ism_common_validUTF8Restrict(object, -1, UR_NoControl) < 0)
        object = objtype;
    ism_common_setErrorData(ISMRC_BadConfigName, "%s", object);
    return ISMRC_BadConfigName;
}

/* Validates composite objects */
XAPI int ism_config_jsonValidateObject(json_t *post, json_t *objVal, char * objType, char * instName, int action, ism_prop_t *props ) {
    int rc = ISMRC_OK;

    TRACE(9, "Entry %s: json:%p, objType:%s, instName:%s\n",
        __FUNCTION__, objVal?objVal:0, objType?objType:"null", instName?instName:"null");

    /* Get validation data index */
    int index = ism_config_getValidationDataIndex(objType);
    ismConfigValidate_t fn = NULL;

    if ( index != -1 ) {
        rc = checkNameSlash(objType, instName);
        if (!rc) {
            fn = ismConfigValidationData[index].valFNP;
            rc = fn(post, objVal, objType, instName, action, props);
        }
    } else {
        rc = ISMRC_InvalidCfgObject;
        TRACE(3, "Invalid object is specified for validation. Object:%s Name:%s\n", objType?objType:"", instName?instName:"");
        ism_common_setErrorData(rc, "%s", objType?objType:"");
    }

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/* Returns a properties struct for an existing object ONLY */

XAPI ism_prop_t * ism_config_json_getObjectProperties(const char * objectName, const char * instName, int getLock) {
    int rc = ISMRC_OK;
    ism_prop_t * props = NULL;
    json_t *tmpObj = NULL;

    if (!srvConfigRoot || !objectName) {
        return NULL;
    }

    if (getLock)
        pthread_rwlock_rdlock(&srvConfiglock);

    tmpObj = json_object_get(srvConfigRoot, objectName);
    if (tmpObj) {
        if ( instName && *instName != '\0' ) {
            json_t * instObj = json_object_get(tmpObj, instName);
            if (instObj) {
                props = ism_common_newProperties(256);
                rc = ism_config_json_addPropsToList(instObj, objectName, instName, NULL, props, 0);
            }
        } else {
            props = ism_common_newProperties(1);
            rc = ism_config_json_addPropsToList(tmpObj, objectName, NULL, NULL, props, 0);
        }
    }

    if (getLock)
        pthread_rwlock_unlock(&srvConfiglock);

    if (!rc)
        return props;
    else
        ism_common_freeProperties(props);
    return NULL;
}

/*
 * Returns properties belongs to a registered component in ism_prop_t structure
 * Caller should free property list.
 */
XAPI ism_prop_t * ism_config_json_getProperties(ism_config_t *compHandle, const char *objectType, const char *objectName, int *doesObjExist, int mode) {
    ism_prop_t *props = NULL;
    json_t *tmpObj = NULL;
    int rc = ISMRC_OK;

    if ( !compHandle || !srvConfigRoot ) {
        *doesObjExist = 0;
        return NULL;
    }

    if ( objectType ) {
        /* check if object belongs to the specified component */
        int isComposite = 0;
        ism_ConfigComponentType_t compType = ism_config_findSchemaGetComponentType(objectType, &isComposite, NULL);
        if ( compHandle->comptype != compType ) {
            *doesObjExist = 0;
            return NULL;
        }
    }

    props = ism_common_newProperties(256);

    pthread_rwlock_rdlock(&srvConfiglock);

    if ( objectType && !objectName ) {
        /* Return properties of all objects of type objectType */
        tmpObj = json_object_get(srvConfigRoot, objectType);
        if ( tmpObj ) {
            rc = ism_config_json_addPropsToList(tmpObj, objectType, NULL, NULL, props, mode );
        } else {
            rc = ISMRC_NotFound;
        }
    } else if ( objectType && objectName ) {
        /* return requested object instance */
        /* Return properties of all objects of type objectType */
        tmpObj = json_object_get(srvConfigRoot, objectType);
        if ( tmpObj ) {
            json_t *instObj = json_object_get(tmpObj, objectName);
            if ( instObj ) {
                rc = ism_config_json_addPropsToList(instObj, objectType, objectName, NULL, props, mode );
            } else {
                rc = ISMRC_NotFound;
            }
        }
    } else {
        /* Get list of objects of required component, and add to the property list */
        int i = 0;
        for (i = 0; i < cfgSchemaItems->count; i++) {
            schemaItem_t *item = cfgSchemaItems->items[i];
            if ( item->compType == compHandle->comptype ) {
                tmpObj = json_object_get(srvConfigRoot, item->object);
                if ( tmpObj ) {
                    rc = ism_config_json_addPropsToList(tmpObj, item->object, NULL, NULL, props, mode );
                    if ( rc != ISMRC_OK ) {
                        break;
                    }
                }
            }
        }
    }

    if ( rc != ISMRC_OK ) {
        if ( props ) ism_common_freeProperties(props);
        props = NULL;
        *doesObjExist = 0;
    } else {
        *doesObjExist = 1;
    }


    pthread_rwlock_unlock(&srvConfiglock);

    return props;
}

/**
 * Add missing configuration items with default
 */
XAPI int ism_config_addMissingDefaults(char *objectType, json_t *object, ism_config_itemValidator_t * list) {
    int rc = ISMRC_OK;
    int jsonType = 0;

    if ( !objectType || !object || !list ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    /* Iterate thru the reqList, check if object exist in json object, if not add default value */
    int i = 0;
    for (i = 0; i < list->total; i++) {
        char *item = list->name[i];
        if (item && strcmp(item, "Name") && strcmp(item, "Update") && strcmp(item, "Delete"))
        {
            /* check default value */
            char *defval = list->defv[i];
            if (defval) {
                /* find object in json object */
                json_t *jsonItem = json_object_get(object, item);
                if ( !jsonItem || json_is_null(jsonItem)) {
                    /* create json_value */
                    json_t *tmpObj = NULL;
                    /* get json type */
                    jsonType = ism_config_getJSONObjectTypeFromSchema(objectType, item);
                    if ( jsonType == JSON_BOOLEAN_TYPE ) {
                        if ( defval && !strcmp(defval, "True")) {
                            tmpObj = json_true();
                        } else {
                            tmpObj = json_false();
                        }
                    } else {
                        tmpObj = ism_config_json_createObject(jsonType, (void *)defval);
                    }
                    /* add to object */
                    if ( tmpObj ) {
                        json_object_set_new(object, item, tmpObj);
                    }
                }
            }
        }
    }

    return rc;
}

/**
 * Returns JSON type string
 */
XAPI const char * ism_config_json_typeString(int type) {
    if ( type < 0 || type > JSON_BOOLEAN_TYPE ) type = JSON_BOOLEAN_TYPE + 1;
    return jsonTypeStr[type];
}

/**
 * Validates JSON object type
 */
XAPI int ism_config_validate_jsonObjectType(json_t *value, int type, char *object, char *name, char *item) {
    int rc = ISMRC_OK;
    int jsonType = json_typeof(value);

    if ( ( type == jsonType ) ||
         ( type == JSON_BOOLEAN_TYPE && ( jsonType != JSON_TRUE && jsonType != JSON_FALSE )) ) {
        return rc;
    }

    /* Return error */
    ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", object,
                name, item, ism_config_json_typeString(jsonType));
    rc = ISMRC_BadPropertyType;
    return rc;
}


/**
 * Gets object from dynamic configuration and adds it in static config props
 */
XAPI void ism_config_addStaticConfigurationData(char *comp, char *name, int type) {
    char pname[64];

    if ( !comp || !name )
        return;

    sprintf(pname, "%s.%s", comp, name);
    pthread_rwlock_rdlock(&srvConfiglock);
    json_t *obj =  json_object_get(srvConfigRoot, name);
    if ( obj && (type == JSON_BOOLEAN_TYPE || json_typeof(obj) == type) ) {
        ism_field_t var = {0};
        if ( type == JSON_INTEGER ) {
            var.type = VT_Integer;
            var.val.i = json_integer_value(obj);
            TRACE(5, "Add item:%s value:%d\n", pname, var.val.i);
        } else if ( type == JSON_STRING ) {
            char *value = (char *)json_string_value(obj);
            TRACE(5, "Add item:%s value:%s\n", pname, value?value:"");
            if ( !value ) value = "";
            var.type = VT_String;
            var.val.s = value;
        } else if ( type == JSON_BOOLEAN_TYPE ) {
            char *value = "";
            if ( json_typeof(obj) == JSON_TRUE ) value = "true";
            if ( json_typeof(obj) == JSON_FALSE ) value = "false";
            TRACE(5, "Add item:%s value:%s\n", pname, value);
            var.type = VT_String;
            var.val.s = value;
        } else {
            TRACE(5, "Add item:%s value:\n", pname);
            var.type = VT_String;
            var.val.s = "";
        }
        ism_common_setProperty(ism_common_getConfigProperties(), pname, &var);
    }
    pthread_rwlock_unlock(&srvConfiglock);

    return;
}

/*
 * Read store configuration from dynamic data and add in static props
 * - hack for store component
 */
static void ism_config_update_storeData(void) {
    /* Update Store - MemoryType, EnableDiskPersistence, BackupToDisk, RestoreFromDisk */
    ism_config_addStaticConfigurationData("Store", "MemoryType", JSON_INTEGER);
    ism_config_addStaticConfigurationData("Store", "EnableDiskPersistence", JSON_BOOLEAN_TYPE);
    return;
}

XAPI int ism_config_readJSONConfig(char * dynCfgFile) {
    int rc = ISMRC_OK;
    json_error_t error;

    /* make sure that json configuration is initialized */
    ism_config_json_init();

    /* Check if file exist */
    if ( access(dynCfgFile, F_OK) == -1 ) {
        TRACE(5, "JSON Configuration file doesn't exist: %s.\n", dynCfgFile);
        return ISMRC_NotFound;
    }

    /* Read config file */
    pthread_rwlock_wrlock(&srvConfiglock);
    if ( srvConfigRoot ) {
        json_decref(srvConfigRoot);
        srvConfigRoot = NULL;
    }
    srvConfigRoot = json_load_file(dynCfgFile, 0, &error);
    if ( !srvConfigRoot ) {
        rc = 6001;
        TRACE(1, "Unable to read %s: error_text:%s error_line:%d\n", dynCfgFile, error.text, error.line);
        ism_common_setErrorData(6001, "%s%d", error.text, error.line);
    } else {
        /* Update ServerVersion */
        json_object_set(srvConfigRoot, "ServerVersion", json_string(serverVersion));
        ism_config_json_updateFile(0);
    }
    pthread_rwlock_unlock(&srvConfiglock);


    /* Big Hack for store data - because configuration callback for store is not implemented.
     * Add all store configuration in static configuration pops.
     */
    if ( rc == ISMRC_OK ) {
        ism_config_update_storeData();
    }

    return rc;
}

/**
 * Check if object being deleted is not used by other configuration items
 * Caller need to make sure that read lock on configuration is acquired before
 * invoking this function.
 */
XAPI int ism_config_json_findObjectInUse(char *inObject, char *object, char * name, int checkList) {
    int rc = ISMRC_OK;
    char *inName = NULL;

    if ( !inObject || !object || !name ) {
        TRACE(6, "inObject:%s object:%s name:%s\n", inObject?inObject:"NULL", object?object:"NULL", name?name:"NULL");
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    /* Get list of inObject, check if the specified object is referenced */
    int count = 0;
    json_t *cp = json_object_get(srvConfigRoot, inObject);
    if ( !cp ) {
        rc = ISMRC_ObjectNotFound;
        ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", inObject, "");
    } else {
        /* Loop thru ep object */
        json_t *cpval = NULL;
        void *objiter = json_object_iter(cp);
        while (objiter) {
            const char *cpname =json_object_iter_key(objiter);
            cpval = json_object_iter_value(objiter);
            /* Get object in this instance */
            json_t *inst = json_object_get(cpval, object);
            if ( inst && json_typeof(inst) == JSON_STRING ) {
                char *value = (char *)json_string_value(inst);
                if ( value && *value != '\0' ) {
                    if ( checkList == 1 ) {
                        int plen = strlen(value);
                        char *token, *nexttoken = NULL;
                        char *tmpstr = (char *)alloca(plen+1);
                        tmpstr[plen] = 0;
                        memcpy(tmpstr, value, plen);
                        for (token = strtok_r(tmpstr, ",", &nexttoken);
                            token != NULL;
                            token = strtok_r(NULL, ",", &nexttoken))
                        {
                            if ( token && *token != '\0' && !strcmp(token, name)) {
                                if (cpname)
                                    inName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cpname);
                                count = 1;
                                break;
                            }
                        }
                    } else if (!strcmp(value, name)) {
                        if (cpname)
                            inName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),cpname);
                        count = 1;
                        break;
                    }
                }
            }

            objiter = json_object_iter_next(cp, objiter);
        }

        if ( count ) {
            rc = ISMRC_ObjectIsInUse;
            ism_common_setErrorData(rc, "%s%s%s%s", object, name, inObject, inName?inName:"null");
            if (inName)  ism_common_free(ism_memory_admin_misc,inName);
        }
    }

    return rc;
}

/**
 * Check if object being deleted is not used by other configuration items stored as an array.
 * Caller need to make sure that read lock on configuration is acquired before
 * invoking this function.
 */
XAPI int ism_config_json_findArrayInUse(char *inArray, char *object, char *name, int ignoreError) {
    int rc = ISMRC_OK;
    char *inName = NULL;

    if ( !inArray || !object || ( inArray && !name )) {
        TRACE(6, "inArray:%s object:%s name:%s\n", inArray?inArray:"NULL", object?object:"NULL", name?name:"NULL");
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    /* Get list of inObject, check if the specified object is referenced */
    int count = 0;
    json_t *objroot = json_object_get(srvConfigRoot, inArray);
    if ( !objroot ) {
        rc = ISMRC_ObjectNotFound;
        ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", inArray, "");
    } else {

        int i = 0;
        for (i=0; i<json_array_size(objroot); i++) {
            json_t *elemObj = json_array_get(objroot, i);
            if (elemObj) {
                if (!strcmp(inArray, "TrustedCertificate")) {
                    json_t *instObj = json_object_get(elemObj, object);
                    if ( instObj && json_typeof(instObj) == JSON_STRING ) {
                        const char *val = json_string_value(instObj);
                        if ( val && !strcmp(val, name)) {
                            count = 1;
                            break;
                        }
                    }
                }
                else if (!strcmp(inArray, "TopicMonitor") || !strcmp(inArray, "ClusterRequestedTopics")) {
                    if ( json_typeof(elemObj) == JSON_STRING ) {
                        const char *val = json_string_value(elemObj);
                        if (val && !strcmp(val, object)) {
                            count = 1;
                            break;
                        }
                    }
                }
            }
        }

        if ( count ) {
            rc = ISMRC_ObjectIsInUse;
            if ( ignoreError == 0 ) {
                ism_common_setErrorData(rc, "%s%s%s%s", object, name?name:"", inArray, "");
            }
        }

        if (inName)  ism_common_free(ism_memory_admin_misc,inName);
    }

    return rc;
}

/*
 * replace configuration object that has array elements.
 * Such as TopicMonitor, TrustedCertificate, ClusterRequestedTopics
 */
XAPI int ism_config_json_replaceArrayConfig(const char *objName, json_t *newObj) {
        int rc = ISMRC_OK;

        json_t *curObj = json_object_get(srvConfigRoot, objName);
        if (curObj)
                json_object_del(srvConfigRoot, objName);

        json_object_set(srvConfigRoot, objName, newObj);

        return rc;
}

/**
 * Create and set JSON object from payload
 * - used for cunit test cases to create object from payload without any validation
 */
XAPI int ism_config_json_setConfigFromPayload(int isComposite, char *object, char *name, char *payload) {
    int rc = ISMRC_OK;

    if (!payload) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    json_t *objval = ism_config_json_strToObject(payload, &rc);
    if ( objval ) {
        if ( isComposite ) {
            pthread_rwlock_wrlock(&srvConfiglock);
            rc = ism_config_json_setComposite((char *)object, (char *)name, objval);
            pthread_rwlock_unlock(&srvConfiglock);
        }
    }
    return rc;
}

/**
 * Delete a composite object
 * Caller is responsible for delete related validation
 */
int ism_config_json_deleteComposite(char *object, char *inst) {
    int rc = ISMRC_OK;

    /* Get read lock */
    pthread_rwlock_wrlock(&srvConfiglock);

    /* find object */
    json_t *objroot = json_object_get(srvConfigRoot, object);
    if ( objroot ) {
        /* find instance */
        if ( inst ) {
            json_t *instroot = json_object_get(objroot, inst);
            if ( instroot ) {
                json_object_del(objroot, inst);

                int getLock = 0;
                rc = ism_config_json_updateFile(getLock);
            }
        }
    }

    /* Unlock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

//create ism_property and call related component callbacks
XAPI int ism_config_json_callCallback( ism_config_t *handle, const char *object, const char *name, json_t * instObj, int haSync, ism_ConfigChangeType_t cbtype, int queryOption) {
    int rc = ISMRC_OK;

    /* check if standby node and callback invocation is required */
    if ( haSync == 1 ) {
        ism_ConfigComponentType_t compType;
        if ( handle != NULL ) {
            compType = handle->comptype;
        } else {
            if ( !strcmp(object, "DestinationMappingRule") || !strcmp(object, "QueueManagerConnection")) {
                compType = ISM_CONFIG_COMP_MQCONNECTIVITY;
            } else if (!strcmp(object, "Queue") || !strcmp(object, "AdminSubscription") ||
                       !strcmp(object, "DurableNamespaceAdminSub") || !strcmp(object, "NonpersistentAdminSub") ||
                       !strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics") ||
                       !strcmp(object, "ResourceSetDescriptor")) {
                compType = ISM_CONFIG_COMP_ENGINE;
            } else {
                rc = ISMRC_NullPointer;
                return rc;
            }
        }

        if ( ism_config_invokeCallbackOnStandbyFlag(compType, (char *)object) == 0 ) {
            TRACE(8, "Standby Node: Callback is not invoked for %s\n", object);
            return rc;
        }
    }

    /* Create props list for callback */
    ism_prop_t *callBackProps = ism_common_newProperties(32);
    rc = ism_config_json_addPropsToList(instObj, object, name, NULL, callBackProps, 0);
    if ( rc != ISMRC_OK ) {
        goto CALLBACK_END;
    }

    /*
     * QueryOption needs to be translated for Queue, AdminSubscription,
     * DurableNamespaceAdminSub or NonpersistentAdminSub delete.
     */
    if (queryOption) {
        ism_field_t var = {0};
        var.type  = VT_Boolean;
        var.val.i = 1;

        if (!strcmp(object, "Queue")) {
            ism_common_setProperty(callBackProps, "DiscardMessages", &var);
        } else if (!strcmp(object, "AdminSubscription") ||
                   !strcmp(object, "DurableNamespaceAdminSub") ||
                   !strcmp(object, "NonpersistentAdminSub")) {
            ism_common_setProperty(callBackProps, "DiscardSharers", &var);
        }
    }

    if ( handle ) {
        if (ism_config_getValidationDataIndex( (char *)object) != -1) {
            if ( !strcmp(object, "ClusterMembership")) {
                /* No need to invoke callback for clustermembership for now
                 * Dynamic updates are not supported yet
                 */
                rc = ISMRC_OK;
            } else {
                if ( handle->callback == NULL ) {
                    rc = ISMRC_NullPointer;
                    ism_common_setError(rc);
                    goto CALLBACK_END;
                }
                rc = handle->callback((char *)object, (char *)name, callBackProps, cbtype);
                if (rc != ISMRC_OK) {
                    TRACE(3, "%s: call %s callback with name:%s, the return code is: %d\n", __FUNCTION__, object?object:"null", name?name:"null", rc);
                    /* If the callback has set the error, then we don't need to set it here, overwriting any parameters
                     * set in the callback.
                     */
                    if (ISMRC_OK == ism_common_getLastError())
                        ism_common_setError(rc);
                    goto CALLBACK_END;
                }
            }
        }

        /* if callback returns OK, update standby node */
        if ( haSync == 0 ) {
            int getConfigLock = 0; /* lock is already taken */
            int deleteFlag = 0;
            if (cbtype == ISM_CONFIG_CHANGE_DELETE) {
                deleteFlag = 1;
            }
            rc = ism_config_updateStandbyNode(instObj, handle->comptype, (char *)object, (char *)name, getConfigLock, deleteFlag);
            if (rc != ISMRC_OK)
                TRACE(3, "%s: ism_config_updateStandbyNode updating object: %s with name: %s return errorcode: %d\n",
                      __FUNCTION__, object?object:"null", name?name:"null", rc);
        }
    }

CALLBACK_END:

    ism_common_freeProperties(callBackProps);
    return rc;
}


/**
 * Returns key and cert values of a CertificateProfile
 */
XAPI int ism_config_getCertificateProfileKeyCert(char *name, char **cert, char ** key, int getLock) {
    int rc = ISMRC_OK;
    int fileCount = 0;
    char *certStr = NULL;
    char *keyStr = NULL;

    if (!name || *name == 0) {
        TRACE(9, "Invalid CertificateProfile Name: NULL\n");
        return ISMRC_NotFound;
    }

    /* find CertificateProfile */
    if ( getLock )
        pthread_rwlock_rdlock(&srvConfiglock);

    json_t *tmpobj = json_object_get(srvConfigRoot, "CertificateProfile");
    if ( tmpobj ) {
        /* find instance */
        if ( name ) {
            json_t *tmpinst = json_object_get(tmpobj, name);
            if ( tmpinst ) {
                /* Get Certificate and key values */
                json_t *certobj = json_object_get(tmpinst, "Certificate");
                json_t *keyobj  = json_object_get(tmpinst, "Key");
                if ( certobj && json_typeof(certobj) == JSON_STRING ) {
                    certStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(certobj));
                    fileCount += 1;
                }
                if ( keyobj && json_typeof(keyobj) == JSON_STRING ) {
                    keyStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(keyobj));
                    fileCount += 1;
                }
            }
        }
    }

    if ( getLock )
        pthread_rwlock_unlock(&srvConfiglock);

    /* CertificateProfile is valid if both cert and key is configured */
    if ( fileCount < 2 ) {
        TRACE(9, "Could not find CertificateProfile:%s Certificate:%s Key:%s\n", name, certStr?certStr:"NULL", keyStr?keyStr:"NULL");
        if (certStr) ism_common_free(ism_memory_admin_misc,certStr);
        if (keyStr) ism_common_free(ism_memory_admin_misc,keyStr);
        rc = ISMRC_NotFound;

    } else {
        *cert = certStr;
        *key = keyStr;
        TRACE(9, "Found CertificateProfile:%s Certificate:%s Key:%s\n", name, *cert, *key);
    }

    return rc;
}

/**
 * Returns key value of a LTPAProfile
 */
XAPI int ism_config_getLTPAProfileKey(char *name, char ** key, int getLock) {
    int rc = ISMRC_OK;
    int fileCount = 0;
    char *keyStr = NULL;

    if (!name || *name == 0) {
        TRACE(9, "Invalid LTPAProfile Name: NULL\n");
        return ISMRC_NotFound;
    }

    /* find CertificateProfile */
    if ( getLock )
        pthread_rwlock_rdlock(&srvConfiglock);

    json_t *tmpobj = json_object_get(srvConfigRoot, "LTPAProfile");
    if ( tmpobj ) {
        /* find instance */
        if ( name ) {
            json_t *tmpinst = json_object_get(tmpobj, name);
            if ( tmpinst ) {
                /* Get Certificate and key values */
                json_t *keyobj  = json_object_get(tmpinst, "KeyFileName");
                if ( keyobj && json_typeof(keyobj) == JSON_STRING ) {
                    keyStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(keyobj));
                    fileCount += 1;
                }
            }
        }
    }

    if ( getLock )
        pthread_rwlock_unlock(&srvConfiglock);

    /* CertificateProfile is valid if both cert and key is configured */
    if ( fileCount < 1 ) {
        TRACE(9, "Could not find LTPAProfile:%s KeyFileName:%s\n", name, keyStr?keyStr:"NULL");
        if (keyStr) ism_common_free(ism_memory_admin_misc,keyStr);
        rc = ISMRC_NotFound;

    } else {
        *key = keyStr;
        TRACE(9, "Found LTPAProfile:%s KeyFileName:%s\n", name, *key);
    }

    return rc;
}


/**
 * Returns key file of a OAuthProfile
 */
XAPI int ism_config_getOAuthProfileKey(char *name, char ** key, int getLock) {
    int rc = ISMRC_OK;
    int fileCount = 0;
    char *keyStr = NULL;

    if (!name || *name == 0) {
        TRACE(9, "Invalid OAuthProfile Name: NULL\n");
        return ISMRC_NotFound;
    }

    /* find CertificateProfile */
    if ( getLock )
        pthread_rwlock_rdlock(&srvConfiglock);

    json_t *tmpobj = json_object_get(srvConfigRoot, "OAuthProfile");
    if ( tmpobj ) {
        /* find instance */
        if ( name ) {
            json_t *tmpinst = json_object_get(tmpobj, name);
            if ( tmpinst ) {
                /* Get Certificate and key values */
                json_t *keyobj  = json_object_get(tmpinst, "KeyFileName");
                if ( keyobj && json_typeof(keyobj) == JSON_STRING ) {
                    keyStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(keyobj));
                    fileCount += 1;
                }
            }
        }
    }

    if ( getLock )
        pthread_rwlock_unlock(&srvConfiglock);

    /* CertificateProfile is valid if both cert and key is configured */
    if ( fileCount < 1 ) {
        TRACE(9, "Could not find OAuthProfile:%s KeyFileName:%s\n", name, keyStr?keyStr:"NULL");
        if (keyStr) ism_common_free(ism_memory_admin_misc,keyStr);
        rc = ISMRC_NotFound;

    } else {
        *key = keyStr;
        TRACE(9, "Found OAuthProfile:%s KeyFileName:%s\n", name, *key);
    }

    return rc;
}

/**
 * Returns ServerName from Configuration table
 * Caller should free returned memory
 */
XAPI char * ism_config_getServerName(void) {
    char *serverName = NULL;
    pthread_rwlock_rdlock(&srvConfiglock);
    json_t *tmpobj = json_object_get(srvConfigRoot, "ServerName");
    if ( tmpobj && json_typeof(tmpobj) == JSON_STRING ) {
        serverName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(tmpobj));
    }
    pthread_rwlock_unlock(&srvConfiglock);
    return serverName;
}


/**
 * Returns object value from Configuration table as string
 * Caller should free returned memory
 */
XAPI char * ism_config_getStringObjectValue(char *object, char *instance, char *item, int getLock) {
    char *strValue = NULL;
    if ( !object || !instance || !item )
        return strValue;

    if (getLock == 1)
        pthread_rwlock_rdlock(&srvConfiglock);
    json_t *objroot = json_object_get(srvConfigRoot, object);
    if ( objroot && json_typeof(objroot) == JSON_OBJECT ) {
        json_t *instroot = json_object_get(objroot, instance);
        if ( instroot && json_typeof(instroot) == JSON_OBJECT ) {
            json_t *itemroot = json_object_get(instroot, item);
            if ( itemroot && json_typeof(itemroot) == JSON_STRING ) {
                strValue = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(itemroot));
            }
        }
    }
    if (getLock == 1)
        pthread_rwlock_unlock(&srvConfiglock);
    return strValue;
}

