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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>

#include <adminHA.h>
#include "janssonConfig.h"
#include "configInternal.h"

#include <engine.h>

#define MIGRATED_JSON_CONFIG_FNAME "server_dynamic_migrated.json"
#define POLICY_UUID_NAME_MAPFILE   "policyNameMapping.txt"  /* this should match with iepiPOLICY_NAME_MAPPING_FILE defined in server_engine/src/policyInfo.h file */
#define PWKEY "pDm99d30ccF3W8+8ak5CN4jrnCSBh+MLwGKhb3zQX5FrlA62W0Ka/Bw3TPUQF3+VBCbqct2OlARW+FJl6ZG3pqZZIV8o1+KVhTqwAcDkfYA6CRmpv+B+v7zCTHvmlQh+mlfd1yLR+afuZBuShee44gVOzYfhgKn21pW0DvLV5tCApQv0E+Z5dI+zqVegBiycN8z5j6wQqebd6L3noY5VgKvi6+2OQpG8TnCA8G6WdM+pMUc+X3I/Yn9fLhqKSYDPqqYH7hoeA8WGh4AFSkVfHi9wCJMNRhF3DNjZc1TIvP/O2pa87/hzehMHcGGrfngLkmEeQFZxbPqA9FfWq9jOCFXj51URrxWOClN561GsEhRvXLVkSJaTxxzyTZFZ4tJp0w8iaVQC5LAAdkCp8QjUz09QoEfTB8KgTmAdJl3Puq4fH+V9fdziNvfQ9Kv0k9RiXWaKfFLLoCSCjRD6liDJUdjQByQkORuQc1PIzWOho0rSynv3fI9xzo/Evt+hyrnmK1GHxaClNhQQLmMVkzS8FHD1YF7ReWF5znK2IjBH1uVBXyRtnU1+/4/piJTFuxS2Cqh1nmTSkl0UNl6LQTXfURlom7IMR5DrPFrmsYyinXZ3McIEKvENfZeN2t5aqhzpKNxr4+fdsrTGAUILPgcoXb/P05Pr4w4b68cb1Qu75IBHklDefmGDFOW9SXemvEDuP9zGTyd4kR9rhwXjs0GmOYy5cLArsMKuhk867jZl6KRmTH6Zdc7RjZDK147QUfDTvbKjNLHDc812RM8zxk4kqQhGft0QRf9DRi4+0GQDY1s="

#define CLIENTSET_EXPORT 0
#define CLIENTSET_IMPORT 1

extern int ism_config_json_processObject(json_t *post, char *object, char *httpPath, int haSync, int validate, int restCall, concat_alloc_t *mqcBuffer, int *onlyMqcItems);
extern int ism_config_getJSONObjectTypeFromSchema(char *object, char *item);
extern int ism_config_json_updateFile(int getLock);
extern int ism_config_isFileExist( char * filepath);
extern json_t * ism_config_json_fileToObject(const char *filename);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
extern const char * configDir;         /* Configuration directory location  */
extern json_t *serverConfigSchema;     /* Server schema JSON object         */
extern schemaItems_t * cfgSchemaItems; /* Schema items                      */
extern int adminInitError;
extern int ismCUNITEnv;

pthread_rwlock_t newSrvConfigLock;  /* Lock for newConfigRoot                */
int newSrvConfigLockInited = 0;
int backupApplied = 0;

/* Engine ClientSet export/import function def */
typedef int32_t (*ism_engine_exportResources_f)( const char *pClientID, const char *pTtopic, const char *pFilename, const char *pPassword, uint32_t options, uint64_t *pRequestID, void *pContext, size_t contextLength, void *pCallbackFnconst);
static ism_engine_exportResources_f engineExportResources_f = NULL;
typedef int32_t (*ism_engine_importResources_f)( const char *pFilename, const char *pPassword, uint32_t options, uint64_t *pRequestID, void *pContext, size_t contextLength, void *pCallbackFnconst);
static ism_engine_importResources_f engineImportResources_f = NULL;

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


/* ism object to json object type map */
static int ism_config_migrate_getItemType(json_t *typeObj) {
	if ( !typeObj || json_typeof(typeObj) != JSON_STRING ) {
		return JSON_NULL;
	}

    char *typeStr = (char *)json_string_value(typeObj);
    if ( !typeStr || *typeStr == '\0')
        return JSON_NULL;

    if ( !strcasecmp(typeStr, "Number"))
        return JSON_INTEGER;
    else if ( !strcasecmp(typeStr, "Enum"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "List"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "String"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "StringBig"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "Name"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "Boolean"))
        return JSON_BOOLEAN_TYPE;
    else if ( !strcasecmp(typeStr, "IPAddress"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "URL"))
        return JSON_STRING;
    else if ( !strcasecmp(typeStr, "REGEX"))
        return JSON_STRING;
    else if (!strcasecmp(typeStr, "BufferSize"))
    	return JSON_STRING;

    return JSON_NULL;
}

/**
 * Returns backup/restore error code
 */
static int ismcli_getBackupErrorMsg(int result) {
    int rc = ISMRC_OK;
    int rc1 = 0;

    /*
     * ZIP error code
     */
    if (result < 200 && result > 100) {
        rc1 = result - 100;
        switch (rc1) {
        case 2:
        	rc = 6213;
            break;
        case 3:
        	rc = 6214;
            break;
        case 4:
        	rc = 6215;
            break;
        case 8:
        	rc = 6216;
            break;
        case 9:
        	rc = 6217;
            break;
        case 10:
        	rc = 6218;
            break;
        case 11:
        	rc = 6219;
            break;
        case 14:
        	rc = 6220;
            break;
        case 15:
        	rc = 6221;
            break;
        case 18:
        	rc = 6222;
            break;
        case 82:
        	rc = 6231;
            break;
        default:
            rc = ISMRC_Error;
            break;
        }
    }
    /* MD5 error */
    else if (result > 200 && result < 300) {
    	rc = 6232;
    }
    /* Restore error */
    else {
        switch (result) {
        case 1:
        	rc = 6223;
            break;
        case 2:
        	rc = 6224;
            break;
        case 3:
            rc = 6225;
            break;
        case 4:
            rc = 6226;
            break;
        case 5:
            rc = 6227;
            break;
        case 6:
            rc = 6228;
            break;
        case 7:
            rc = 6229;
            break;
        case 9:
            rc = 6230;
            break;
        default:
            rc = ISMRC_Error;
            break;
        }
    }
    return rc;
}


/* Returns JSON data type from configuration schema
 * This is an updated version of ism_config_getJSONObjectTypeFromSchema() to handle MessagingPolicy object
 */
static int ism_config_migrate_getJSONObjectTypeFromSchema(char *object, char *item) {

    int type = JSON_NULL;

    if ( !strcmp(object, "MessagingPolicy")) {
        /* get object node */
        json_t *obj = json_object_get(serverConfigSchema, "TopicPolicy");
        if ( obj ) {
            /* get item node */
            json_t *itm = json_object_get(obj, item);
            if ( itm ) {
                /* get Type of item */
                json_t *t = json_object_get(itm, "Type");
                type = ism_config_migrate_getItemType(t);
            }
        }

        if ( type == JSON_NULL ) {
        	/* handle special cases - like DestinationType, and Destination */
        	if ( !strcmp(item, "DestinationType") || !strcmp(item, "Destination")) {
        		type = JSON_STRING;
        	}
        }

        return type;
    }

    if ( object && item ) {
        /* get object node */
        json_t *obj = json_object_get(serverConfigSchema, object);
        if ( obj ) {
            /* get item node */
            json_t *itm = json_object_get(obj, item);
            if ( itm ) {
                /* get Type of item */
                json_t *t = json_object_get(itm, "Type");
                type = ism_config_migrate_getItemType(t);
            }
        }
    } else {
        json_t *itm = json_object_get(serverConfigSchema, item);
        if ( itm ) {
            /* get Type of item */
            json_t *t = json_object_get(itm, "Type");
            type = ism_config_migrate_getItemType(t);
        }
    }

    return type;
}


/*
 * Add update singleton configuration item in JSON configuration root node.
 * Make sure to get rwlock on newSrvConfigLock before call this function.
 */
static int ism_config_migrate_addUpdateSingletonProps(json_t *newConfigRoot, char *item, void *newvalue) {
    int rc = ISMRC_OK;

    if ( !newConfigRoot || !item )
        return -1;

    int schemaObjType = ism_config_getJSONObjectTypeFromSchema(NULL, item);

    /* check for exitsting data */
    json_t * curObj = json_object_get(newConfigRoot, item);

    if ( curObj ) {
        /* Update value */
        json_t *newobj = ism_config_json_createObject(schemaObjType, newvalue);

        if ( json_object_set(newConfigRoot, item, newobj) < 0 ) {
            TRACE(4, "Configuration: item update ERROR: %s\n", item);
            return ISMRC_Error;
        } else {
            TRACE(9,  "Configuration: updated Item: %s\n", item);
        }
    } else {
        /* Add a new singleton object */
        json_t *newobj = ism_config_json_createObject(schemaObjType, newvalue);
        if ( json_object_set_new(newConfigRoot, item, newobj) < 0 ) {
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
 * Make sure to get rwlock on newSrvConfigLock before call this function.
 */
static int ism_config_migrate_addUpdateCompositeProps(json_t *newConfigRoot, char *object, char *objname, char *item, void *newvalue, int isJSONValue ) {
    int         rc = ISMRC_OK;
    json_t     *objRoot  = NULL;
    json_t     *instRoot = NULL;
    json_t     *itemRoot = NULL;
    json_t     *newItem = NULL;

    if ( !newConfigRoot || !item ) {
        return ISMRC_ArgNotValid;
    }

    if ( object && !strcmp(object, "Endpoint")) {
    	if ( item && !strcmp(item, "MessagingPolicies")) {
    		item = "TopicPolicies";
    	}
    }

    int schemaObjType = ism_config_migrate_getJSONObjectTypeFromSchema(object, item);

    /* check if object exist in root */
    objRoot = json_object_get(newConfigRoot, object);
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
            json_object_set_new(newConfigRoot, object, newmap);
            TRACE(6, "Configuration: migrated Object: %s %s %s %d\n", object, objname, item, schemaObjType);
            return rc;
        } else {
            /* Add TopicMonitor */
            json_t *newinst = json_array();
            json_array_append_new(newinst, json_string((char *)newvalue));
            json_object_set_new(newConfigRoot, object, newinst);
            TRACE(6, "Configuration: migrated Object: %s %s %s %d\n", object, objname?objname:"NULL", item, JSON_STRING);
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
                TRACE(6, "Configuration: migrated Object: %s %s %s %d\n", object, objname?objname:"NULL", item, JSON_STRING);
            }
        }
        return rc;
    }
    if ( !instRoot ) {
        /* create new instance */
        json_t *newinst = json_object();
        newItem = ism_config_json_createObject(schemaObjType, newvalue);
        json_object_set_new(newinst, item, newItem);
        /* Add object map to root */
        json_object_set_new(objRoot, objname, newinst);
        TRACE(6, "Configuration: migrated Object: %s %s %s %d\n", object, objname, item, schemaObjType);
        return rc;
    }

    /* create new json object for newvalue */
    newItem = ism_config_json_createObject(schemaObjType, newvalue);

    /* Check for item */
    itemRoot = json_object_get(instRoot, item);
    if ( itemRoot ) {
        /* update item value */
        if ( json_object_set(instRoot, item, newItem) < 0 ) {
            /* Update failed */
            TRACE(4, "Configuration: migrated Object update ERROR: %s %s %s %d\n", object, objname, item, schemaObjType);
            rc = ISMRC_Error;   /* TODO: Create a new RC for updated failures */
        } else {
            TRACE(6, "Configuration: migrated Object updated: %s %s %s %d\n", object, objname, item, schemaObjType);
        }
    } else {
        /* create new item */
        json_object_set_new(instRoot, item, newItem);
        TRACE(6, "Configuration: migrated object: %s %s %s %d\n", object, objname, item, schemaObjType);
    }

    return rc;
}


/**
 * Add properties to JSON configuration data from a string containing data in key=value pairs
 * as supported in v1.x release.
 */
static int ism_config_migrate_v1PropsStringToJSONProps(char *propStr, json_t *newConfigRoot) {
    int rc = ISMRC_OK;
    char * keyword = NULL;
    char * value = NULL;
    char * more = NULL;
    char * p = NULL;

    if ( !propStr || *propStr == '\0' )
        return rc;

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

            if ((!strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics")) && !strcmp(item, "Locale")) {
            	return rc;
            }

            if (object && item && tmpname) {

                /* Process composite object */
                len = strlen(object) + strlen(item) + 2;
                name = (char *)keyword + len;

                if ( ( *item == 'N' && strcmp(item, "Name") == 0 ) ||  ( *item == 'U' && strcmp(item, "UID") == 0 ) || ( *item == 'L' && strcmp(item, "Locale") == 0 )) {
                    if ( *object == 'M' && !strcmp(object, "MessageHub")) {
                		/* To handle the case when MessageHub Description is not set */
                		char *dummyItem = "Description";
                		char *dummyValue = "";
                		ism_config_migrate_addUpdateCompositeProps(newConfigRoot, object, name, dummyItem, (void *)dummyValue, 0);
                    } else {
                        goto FUNC_END;
                    }
                } else if ( ((*object == 'T' && !strcmp(object, "TopicMonitor")) || ( *object == 'C' && !strcmp(object, "ClusterRequestedTopics")))
                           && !strcmp(item, "TopicString") ) {
                	ism_config_migrate_addUpdateCompositeProps(newConfigRoot, object, NULL, item, (void *)value, 0);
                } else {
                	ism_config_migrate_addUpdateCompositeProps(newConfigRoot, object, name, item, (void *)value, 0);
                }

            } else {

                if ( value ) {
                    /* Process singleton object */
                	ism_config_migrate_addUpdateSingletonProps(newConfigRoot, (char *)object, (void *)value);
                }

            }
        }
    }

FUNC_END:

    return rc;
}

/**
 * Create configuration file in JSON format
 */
XAPI int ism_config_json_createNewConfigFile(int getLock, json_t *newSrvConfigRoot) {
    int rc = ISMRC_OK;
    char cfilepath[1024];

    /* get config lock */
    if ( getLock == 1 )
        pthread_rwlock_wrlock(&newSrvConfigLock);

    sprintf(cfilepath, "%s/%s",     configDir, MIGRATED_JSON_CONFIG_FNAME);

    /* dump content of current configuration to a .temp file */
    int loop = 0;
    for ( loop=0; loop<5; loop++ ) {
        if ( newSrvConfigRoot ) {
            rc = ISMRC_OK;
            errno = 0;
            char *dumpStr = json_dumps(newSrvConfigRoot, JSON_INDENT(2)|JSON_PRESERVE_ORDER|JSON_ENCODE_ANY);
            if ( dumpStr ) {
                FILE *dest = NULL;
                dest = fopen(cfilepath, "w");
                if ( dest ) {
                    fprintf(dest, "%s", dumpStr);
                    fclose(dest);
                    ism_common_free_raw(ism_memory_admin_misc,dumpStr);
                    break;
                } else {
                    TRACE(2, "Failed to open config file: errno:%d\n", errno);
                    rc = ISMRC_Error;
                    ism_common_setError(rc);
                    break;
                }
            } else {
                if ( errno == EAGAIN && loop < 4 ) {
                    TRACE(9, "Update configuration file: try count=%d\n", loop);
                    ism_common_sleep(100000);
                    continue;
                } else {
                    json_t *tmpSrvConfigPtr = json_deep_copy(newSrvConfigRoot);
                    int retval = json_dump_file( tmpSrvConfigPtr, cfilepath, JSON_INDENT(2)|JSON_PRESERVE_ORDER|JSON_ENCODE_ANY );
                    if ( retval == 0 ) {
                        json_t *tmpPtr = newSrvConfigRoot;
                        newSrvConfigRoot = tmpSrvConfigPtr;
                        json_decref(tmpPtr);
                    } else {
                        TRACE(2, "Failed to update configuration file: errno:%d\n", errno);
                        rc = ISMRC_Error;
                        ism_common_setError(rc);
                        break;
                    }

                    continue;
                }
            }
        } else {
            TRACE(2, "Configuration root node is NULL.\n");
            rc = ISMRC_NullPointer;
            ism_common_setError(rc);
            break;
        }
    }

    /* Unlock config lock */
    if ( getLock == 1 )
        pthread_rwlock_unlock(&newSrvConfigLock);

    return rc;
}

/*
 * Updates/maps v1 config items to v2:
 * - Create Topic, Queue and Subscription policies from MessagaingPolicy and update Endpoint
 * - Disable all Endpoint objects
 * - Disable HA object
 */
static int ism_config_migrate_updateResetNewConfig(json_t *newConfigRoot, int disableObjects) {
	int rc = ISMRC_OK;
    json_t *objval = NULL;
    const char *objkey = NULL;

	/* Loop thru all MessagingPolicy */
	json_t *msgPolicyObjects = json_object_get(newConfigRoot, "MessagingPolicy");
	if ( msgPolicyObjects) {
	    json_object_foreach(msgPolicyObjects, objkey, objval) {
	    	/* get DestinationType */
	    	json_t *destTypeObject = json_object_get(objval, "DestinationType");
	    	if ( json_typeof(destTypeObject) == JSON_STRING ) {
	    	    const char *typeStr = json_string_value(destTypeObject);

	    	    if ( *typeStr == 'T' ) {
	    	    	/* Create a TopicPolicy */

	    	    	/* deep copy object */
	    	    	json_t *topicPolicyObject = json_deep_copy(objval);

	    	    	/* rename Destination to Topic */
	    	    	json_t *destObject = json_object_get(objval, "Destination");
	    	    	json_t *topicbject = json_copy(destObject);
	    	    	json_object_set_new(topicPolicyObject, "Topic", topicbject);

	    	    	/* delete PolicyType, Destination, Destinationtype */
	    	    	json_object_del(topicPolicyObject, "PolicyType");
	    	    	json_object_del(topicPolicyObject, "Destination");
	    	    	json_object_del(topicPolicyObject, "DestinationType");

	    	    	/* add object as TopicPolicy */
	    	    	json_t *tPolCurrObject = json_object_get(newConfigRoot, "TopicPolicy");
	    	    	if ( tPolCurrObject ) {
	    	    		/* add new entry */
	    	    		json_object_set(tPolCurrObject, objkey, topicPolicyObject);
	    	    	} else {
	    	    		/* create TopicPolicy object and add an entry in it */
	    	    		json_t *newObj = json_object();
	    	    		json_object_set_new(newConfigRoot, "TopicPolicy", newObj);
	    	    		json_object_set_new(newObj, objkey, topicPolicyObject);
	    	    	}

	    	    } else if ( *typeStr == 'S' ) {
	    	    	/* Create a SubscritionPolicy */

	    	    	/* deep copy object */
	    	    	json_t *subPolicyObject = json_deep_copy(objval);

	    	    	/* rename Destination to Subscription */
	    	    	json_t *destObject = json_object_get(objval, "Destination");
	    	    	json_t *subObject = json_copy(destObject);
	    	    	json_object_set_new(subPolicyObject, "Subscription", subObject);

	    	    	/* delete PolicyType, Destination, DestinationType, DisconnectedClientNotification */
	    	    	json_object_del(subPolicyObject, "PolicyType");
	    	    	json_object_del(subPolicyObject, "Destination");
	    	    	json_object_del(subPolicyObject, "DestinationType");
	    	    	json_object_del(subPolicyObject, "DisconnectedClientNotification");
	    	    	json_object_del(subPolicyObject, "MaxMessageTimeToLive");

	    	    	/* add object as SubscriptionPolicy */
	    	    	json_t *sPolCurrObject = json_object_get(newConfigRoot, "SubscriptionPolicy");
	    	    	if ( sPolCurrObject ) {
	    	    		/* add new entry */
	    	    		json_object_set(sPolCurrObject, objkey, subPolicyObject);
	    	    	} else {
	    	    		/* create SubscriptionPolicy object and add an entry in it */
	    	    		json_t *newObj = json_object();
	    	    		json_object_set_new(newConfigRoot, "SubscriptionPolicy", newObj);
	    	    		json_object_set_new(newObj, objkey, subPolicyObject);
	    	    	}

	    	    } else if ( *typeStr == 'Q' ) {
	    	    	/* Create a QueuePolicy */

	    	    	/* deep copy object */
	    	    	json_t *queuePolicyObject = json_deep_copy(objval);

	    	    	/* rename Destination to Queue */
	    	    	json_t *destObject = json_object_get(objval, "Destination");
	    	    	json_t *queueObject = json_copy(destObject);
	    	    	json_object_set_new(queuePolicyObject, "Queue", queueObject);

	    	    	/* delete PolicyType, DestinationType, MaxMessages, MaxMessagesBehavior, DisconnectedClientNotification */
	    	    	json_object_del(queuePolicyObject, "PolicyType");
	    	    	json_object_del(queuePolicyObject, "Destination");
	    	    	json_object_del(queuePolicyObject, "DestinationType");
	    	    	json_object_del(queuePolicyObject, "MaxMessages");
	    	    	json_object_del(queuePolicyObject, "MaxMessagesBehavior");
	    	    	json_object_del(queuePolicyObject, "DisconnectedClientNotification");

	    	    	/* add object as QueuePolicy */
	    	    	json_t *qPolCurrObject = json_object_get(newConfigRoot, "QueuePolicy");
	    	    	if ( qPolCurrObject ) {
	    	    		/* add new entry */
	    	    		json_object_set(qPolCurrObject, objkey, queuePolicyObject);
	    	    	} else {
	    	    		/* create QueuePolicy object and add an entry in it */
	    	    		json_t *newObj = json_object();
	    	    		json_object_set_new(newConfigRoot, "QueuePolicy", newObj);
	    	    		json_object_set_new(newObj, objkey, queuePolicyObject);
	    	    	}

	    	    }
	    	}
	    }

	    /* Delete MessagingPolicy */
	    json_object_del(newConfigRoot, "MessagingPolicy");
	}

	/* Remove PolicyType from Connection policies */
	/* Loop thru all MessagingPolicy */
	json_t *conPolicyObjects = json_object_get(newConfigRoot, "ConnectionPolicy");
	if ( conPolicyObjects) {
	    json_object_foreach(conPolicyObjects, objkey, objval) {
	    	/* delete PolicyType */
	    	json_t *polTypeObject = json_object_get(objval, "PolicyType");
	    	if ( polTypeObject ) json_object_del(objval, "PolicyType");
	    }
	}

	/* Loop thru Endpoint objects and set Enabled to false */
	json_t *epObjects = json_object_get(newConfigRoot, "Endpoint");
	if ( epObjects ) {
		json_object_foreach(epObjects, objkey, objval) {
			/* set Enabled object to false */
			if ( disableObjects == 1 )
			    json_object_set(objval, "Enabled", json_false());

			/* set MaxMessageSize */
			json_t *maxMsgObj = json_object_get(objval, "MaxMessageSize");
			if ( maxMsgObj && json_typeof(maxMsgObj) == JSON_STRING ) {
				char *maxMsgObjStr = (char *)json_string_value(maxMsgObj);
				if ( !strstr(maxMsgObjStr, "KB") && !strstr(maxMsgObjStr, "MB") && !strstr(maxMsgObjStr, "GB") ) {
					char *newValStr = NULL;
		            if ( strstr(maxMsgObjStr, "K") || strstr(maxMsgObjStr, "M") || strstr(maxMsgObjStr, "G") ) {
		            	newValStr = (char *)alloca(strlen(maxMsgObjStr) + 2);
		            	sprintf(newValStr, "%sB", maxMsgObjStr);
		            }
		            json_object_set(objval, "MaxMessageSize", json_string(newValStr));
				}
			}

			/* delete Security */
			json_object_del(objval, "Security");

			/* Update policy data */
			/* Get TopicPolicies value and validate if all are topic policies */
			json_t *tpol = json_object_get(objval, "TopicPolicies");
			if ( tpol && json_typeof(tpol) == JSON_STRING) {
				char *tpolStr = (char *)json_string_value(tpol);
				if ( tpolStr && *tpolStr != '\0') {
					char *newTpolStr = NULL;
					char *newQpolStr = NULL;
					char *newSpolStr = NULL;

					/* process comma-seperated list
					 * - check policy type and update TopicPolicies, QueuePolicies, and SubscriptionPolicies
					 */

					TRACE(7, "Process Endpoint %s policy string: %s\n", objkey, tpolStr);

		            char *p, *token, *nexttoken = NULL;
		            int len = strlen(tpolStr);
		            char *option = alloca(len + 1);
		            memcpy(option, tpolStr, len);
		            option[len] = '\0';

		            for (token = strtok_r(option, ",", &nexttoken); token != NULL; token = strtok_r(NULL, ",", &nexttoken))
		            {
		                /* remove leading space */
		                while ( *token == ' ' )
		                    token++;
		                if ( *token == 0 )
		                    continue;
		                /* remove trailing space */
		                p = token + strlen(token) - 1;
		                while ( p > token && *p == ' ' )
		                    p--;
		                *(p+1) = 0;

		                if ( !token || *token == '\0') continue;

		                int tklen = strlen(token);
		                int found = 0;

		                TRACE(9, "Find Endpoint policy in TopicPolicy List: %s\n", token);

		                /* check policy type is a TopicPolicy */
		                json_t *tppol = json_object_get(newConfigRoot, "TopicPolicy");
		                if ( tppol ) {
		                	TRACE(9, "Check for TopicPolicy: %s\n", token);
		                	json_t *tpolInst = json_object_get(tppol, token);
		                	if ( tpolInst ) {
		                		TRACE(9, "Found TopicPolicy: %s\n", token);
		                		/* this is a TopicPolicy - add entry in newTpolStr */
		    		            if ( newTpolStr == NULL ) {
		    		            	/* add first entry */
			                		newTpolStr = (char *)alloca(tklen  + 1);
			    		            sprintf(newTpolStr, "%s", token);
		    		            } else {
		    		            	/* append a comma and entry */
		    		            	char *tmpstr = (char *)alloca(strlen(newTpolStr) + tklen + 2);
		    		            	sprintf(tmpstr, "%s,%s", newTpolStr, token);
		    		            	newTpolStr = tmpstr;
		    		            }
		    		            found = 1;
		                	}
		                }

		                if ( found == 1 ) continue;

		                TRACE(9, "Find Endpoint policy in QueuePolicy List: %s\n", token);

		                /* check policy type is a QueuePolicy */
		                json_t *qpol = json_object_get(newConfigRoot, "QueuePolicy");
		                if ( qpol ) {
		                	TRACE(9, "Check for QueuePolicy: %s\n", token);
		                	json_t *qpolInst = json_object_get(qpol, token);
		                	if ( qpolInst ) {
		                		TRACE(9, "Found QueuePolicy: %s\n", token);
		                		/* this is a QueuePolicy - add entry in newQpolStr */
		    		            if ( newQpolStr == NULL ) {
		    		            	/* add first entry */
			                		newQpolStr = (char *)alloca(tklen  + 1);
			    		            sprintf(newQpolStr, "%s", token);
		    		            } else {
		    		            	/* append a comma and entry */
		    		            	char *tmpstr = (char *)alloca(strlen(newQpolStr) + tklen + 2);
		    		            	sprintf(tmpstr, "%s,%s", newQpolStr, token);
		    		            	newQpolStr = tmpstr;
		    		            }
		    		            found = 1;
		                	}
		                }

		                if ( found == 1 ) continue;

		                TRACE(9, "Find Endpoint policy in SubscriptionPolicy List: %s\n", token);

		                /* check policy type is a SubscriptionPolicy */
		                json_t *spol = json_object_get(newConfigRoot, "SubscriptionPolicy");
		                if ( spol ) {
		                	TRACE(9, "Check for SubscriptionPolicy: %s\n", token);
		                	json_t *spolInst = json_object_get(spol, token);
		                	if ( spolInst ) {
		                		TRACE(9, "Found SubscriptionPolicy: %s\n", token);
		                		/* this is a SubscriptionPolicy - add entry in newSpolStr */
		    		            if ( newSpolStr == NULL ) {
		    		            	/* add first entry */
			                		newSpolStr = (char *)alloca(tklen  + 1);
			    		            sprintf(newSpolStr, "%s", token);
		    		            } else {
		    		            	/* append a comma and entry */
		    		            	char *tmpstr = (char *)alloca(strlen(newSpolStr) + tklen + 2);
		    		            	sprintf(tmpstr, "%s,%s", newSpolStr, token);
		    		            	newSpolStr = tmpstr;
		    		            }
		                	}
		                }

		            }

		            /* Update TopicPolices, QueuePolicies and SubscriptionPolicies entry in the endpoint instance object */
		            if ( newTpolStr ) {
		            	json_object_set(objval, "TopicPolicies", json_string(newTpolStr));
		            }
		            if ( newQpolStr ) {
		            	json_object_set(objval, "QueuePolicies", json_string(newQpolStr));
		            }
		            if ( newSpolStr ) {
		            	json_object_set(objval, "SubscriptionPolicies", json_string(newSpolStr));
		            }
				}
			}
		}
	}

	/* Loop thru Certificate profile objects and delete ExpirationDate */
	json_t *certpObjects = json_object_get(newConfigRoot, "CertificateProfile");
	if ( certpObjects ) {
		json_object_foreach(certpObjects, objkey, objval) {
			json_t *expObject = json_object_get(objval, "ExpirationDate");
			if ( expObject ) json_object_del(objval, "ExpirationDate");
		}
	}

	/* Get HA object and set EnabledHA to false */
	json_t *haObject = json_object_get(newConfigRoot, "HighAvailability");
	if ( haObject ) {
		json_t *haInstObject = json_object_get(haObject, "haconfig");
		if ( haInstObject && disableObjects == 1 ) {
			json_object_set(haInstObject, "EnableHA", json_false());
		}
	}

	/* Set DiskPersistence to true */
	json_object_set(newConfigRoot, "EnableDiskPersistence", json_true());

	/* Add PreSharedKey item if PSK file exist */
    if( access( IMA_SVR_DATA_PATH "/data/certificate/PSK/psk.csv", R_OK ) != -1 ) {
    	json_object_set(newConfigRoot, "PreSharedKey", json_string("psk.csv"));
    }


	/* Fix config items related to BufferSize
	 * TraceMax and MaxMessageSize
	 */
	json_t *trmObj = json_object_get(newConfigRoot, "TraceMax");
	if ( trmObj && json_typeof(trmObj) == JSON_STRING ) {
		char *trmObjStr = (char *)json_string_value(trmObj);
		if ( !strstr(trmObjStr, "KB") && !strstr(trmObjStr, "MB") && !strstr(trmObjStr, "GB") ) {
			char *newValStr = NULL;
            if ( strstr(trmObjStr, "K") || strstr(trmObjStr, "M") || strstr(trmObjStr, "G") ) {
            	newValStr = (char *)alloca(strlen(trmObjStr) + 2);
            	sprintf(newValStr, "%sB", trmObjStr);
            }
            json_object_set(newConfigRoot, "TraceMax", json_string(newValStr));
		}
	}

	/* Fix HA object */
	json_t *haObj = json_object_get(newConfigRoot, "HighAvailability");
	if ( haObj ) {
		json_t * haconfigObj = json_object_get(haObj, "haconfig");
		if ( haconfigObj ) {
		    /* make a copy of haconfigObj */
		    json_t * haconfigCopyObj = json_deep_copy(haconfigObj);
		    /* delete AllowSingleNIC */
		    json_object_del(haconfigCopyObj, "AllowSingleNIC");
		    /* delete HighAvailability object and add copy */
		    json_object_del(newConfigRoot, "HighAvailability");
		    json_object_set_new(newConfigRoot, "HighAvailability", haconfigCopyObj);
		}
	}

	/* Fix LDAP object */
	json_t *ldapObj = json_object_get(newConfigRoot, "LDAP");
	if ( ldapObj ) {
		json_t * ldapconfigObj = json_object_get(ldapObj, "ldapconfig");
		if ( ldapconfigObj ) {
			/* make a copy of ldapconfigObj */
			json_t * ldapconfigCopyObj = json_deep_copy(ldapconfigObj);
			/* delete LDAP object and add copy */
			json_object_del(newConfigRoot, "LDAP");
			json_object_set_new(newConfigRoot, "LDAP", ldapconfigCopyObj);
		}
	}

	/* Fix ClusterMembership object */
	json_t *clObj = json_object_get(newConfigRoot, "ClusterMembership");
	if ( clObj ) {
		json_t * clconfigObj = json_object_get(clObj, "cluster");
		if ( clconfigObj ) {
			if ( disableObjects == 1 ) {
				json_object_set(clconfigObj, "EnableClusterMembership", json_false());
			}

		    /* make a copy of clconfigObj */
		    json_t * clconfigCopyObj = json_deep_copy(clconfigObj);
		    /* delete ClusterMembership object and add copy */
		    json_object_del(newConfigRoot, "ClusterMembership");
		    json_object_set_new(newConfigRoot, "ClusterMembership", clconfigCopyObj);
		}
	}

	/* Fix Syslog object */
	json_t *sysObj = json_object_get(newConfigRoot, "Syslog");
	if ( sysObj ) {
		json_t * sysconfigObj = json_object_get(sysObj, "Syslog");
		if ( sysconfigObj ) {
			if ( disableObjects == 1 ) {
				json_object_set(sysconfigObj, "Enabled", json_false());
			}
		    /* make a copy of sysconfigObj */
		    json_t * sysconfigCopyObj = json_deep_copy(sysconfigObj);
		    /* delete Syslog object and add copy */
		    json_object_del(newConfigRoot, "Syslog");
		    json_object_set_new(newConfigRoot, "Syslog", sysconfigCopyObj);
		}
	}

	/* Delete unwanted items */
	json_object_del(newConfigRoot, "InternalErrorCode");
	json_object_del(newConfigRoot, "RestoreFromDisk");
	json_object_del(newConfigRoot, "MemoryType");
	json_object_del(newConfigRoot, "FixEndpointInterface");
	json_object_del(newConfigRoot, "BackupToDisk");
	json_object_del(newConfigRoot, "AdminMode");
	json_object_del(newConfigRoot, "ServerVersion");
	json_object_del(newConfigRoot, "ServerUID");
	json_object_del(newConfigRoot, "AdminUserID");
	json_object_del(newConfigRoot, "AdminUserPassword");
	json_object_del(newConfigRoot, "AdminEndpoint");

	json_t *secPObj = json_object_get(newConfigRoot, "SecurityProfile");
	if ( secPObj ) {
		json_object_del(secPObj, "AdminDefaultSecProfile");
	}

	json_t *certPObj = json_object_get(newConfigRoot, "CertificateProfile");
	if ( certPObj ) {
		json_object_del(certPObj, "AdminDefaultCertProf");
	}

	json_t *pskObj = json_object_get(newConfigRoot, "PreSharedKey");
	if ( pskObj && json_typeof(pskObj) == JSON_STRING ) {
		char *pskstr = (char *)json_string_value(pskObj);
		if ( pskstr && *pskstr == '\0' ) {
			json_object_del(newConfigRoot, "PreSharedKey");
		}
	}

	/* DO not update TraceLevel */
	json_object_del(newConfigRoot, "TraceLevel");
	/* Remove TraceFilter (not supported in v2) */
	json_object_del(newConfigRoot, "TraceFilter");
	/* Remove EnableAdminServer */
	json_object_del(newConfigRoot, "EnableAdminServer");
	/* Remove AuditLogControl */
	json_object_del(newConfigRoot, "AuditLogControl");
	/* Remove LogAutoTransfer defect 199041 */
	json_object_del(newConfigRoot, "LogAutoTransfer");

	return rc;
}


/**
 * Read a configuration file as keyword=value and convert into JSON object
 */
static json_t * ism_config_migrate_propsToJSON(char * filename, json_t *newConfigRoot, int *rc) {
    FILE * f = NULL;
    size_t len = 0;
    char *line = NULL;

    *rc = ISMRC_OK;

    if ( !filename ) {
        ism_common_setError(ISMRC_ArgNotValid);
        *rc = ISMRC_ArgNotValid;
        return newConfigRoot;
    }

    /* check arguments */
    if ( !filename ) {
        ism_common_setError(ISMRC_NullPointer);
        *rc = ISMRC_NullPointer;
        return newConfigRoot;
    }

    /* open and process configuration file in key=value pairs*/
    TRACE(5, "Process configuration file in key=value pair format: %s\n", filename);

    f = fopen(filename, "rb");
    if (!f) {
        TRACE(5, "Configuration file in key=value pair format is not found.\n");
        ism_common_setError(ISMRC_NotFound);
        *rc = ISMRC_NotFound;
        return newConfigRoot;
    }

    /* create UUID policy map file in config dir */
    char uuidMapFile[1024];
    sprintf(uuidMapFile, "%s/%s", configDir, POLICY_UUID_NAME_MAPFILE);
    FILE *mfile = NULL;

    if ( newConfigRoot == NULL ) {
        mfile = fopen(uuidMapFile, "w");
    } else {
    	mfile = fopen(uuidMapFile, "a");
    }

    if ( newConfigRoot == NULL ) {
    	newConfigRoot = json_object();
		if ( newConfigRoot == NULL ) {
			*rc = ISMRC_NullPointer;
			goto MIGRATEEND;
		}
		/* set some singleton default values */
		json_object_set_new(newConfigRoot, "Version", json_string(SERVER_SCHEMA_VERSION));
    }

    int rc1 = 0;

    rc1 = getline(&line, &len, f);
    while ( rc1 >= 0 ) {

    	/* Update UUID map file for MessagingPolicies */
        if ( *line == 'S' && !strncmp(line, "Security.MessagingPolicy.UID.", 29)) {
        	char *tmpstr = line;
        	char *uidMapStr = tmpstr + 29;

            char * keyword = NULL;
            char * value = NULL;
            char * more = NULL;

            keyword = ism_common_getToken(uidMapStr, " \t\r\n", "=\r\n", &more);
            if (keyword) {
                char * cp = keyword+strlen(keyword); /* trim trailing white space */
                while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                    cp--;
                *cp = 0;
                value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);

                ism_common_canonicalName(keyword);

                if ( mfile ) fprintf(mfile, "%s %s\n", value, keyword);
            }
        }

    	ism_config_migrate_v1PropsStringToJSONProps(line, newConfigRoot);
        rc1 = getline(&line, &len, f);
    }

    if ( mfile ) fclose(mfile);

    fclose(f);

    if (line)
        ism_common_free_raw(ism_memory_admin_misc,line);

MIGRATEEND:

    return newConfigRoot;
}

/**
 * Execute a script
 */
static int ism_config_execAndReturnCode(char * fileName, char * bkPassword, int type, int *errorRC ) {
    int pipefd[2];
    if ( pipe(pipefd) == -1 ) {
    	TRACE(2, "pipe returned error\n");
    	return 1;
    }

    pid_t pid = vfork();
    if (pid < 0) {
        perror("fork failed");
        return ISMRC_Error;
    }
    if (pid == 0) {
    	dup2(pipefd[1], STDOUT_FILENO);
    	close(pipefd[0]);
    	close(pipefd[1]);
    	if ( type == 1 ) {
            execl(IMA_SVR_INSTALL_PATH "/bin/restore.sh", "restore.sh", fileName, bkPassword, NULL);
    	} else {
    		execl(IMA_SVR_INSTALL_PATH "/bin/backup.sh", "backup.sh", fileName, bkPassword, NULL);
    	}
        int urc = errno;
        TRACE(1, "Unable to run restore script: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }

    close(pipefd[1]);
    char buffer[4096];
    int size = read(pipefd[0], buffer, sizeof(buffer));
    close(pipefd[0]);

    int st;
    waitpid(pid, &st, 0);
    int res = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    if ( size != 0 ) {
    	if ( type == 1 ) {
    	    TRACE(5, "Script restore.sh execution returned error: %s\n", buffer);
    	} else {
    		TRACE(5, "Script backup.sh execution returned error: %s\n", buffer);
    	}
    	*errorRC = atoi(buffer);

    } else {
   	    TRACE(9, "Script restore.sh execution returned: %d\n", res);
    }
    return res;
}

/**
 * Process MQC objects
 */
static int ism_config_migrate_MQCObjects(json_t *migratedObjects) {
	int rc = ISMRC_OK;
    json_t *objval = NULL;
    const char *objkey = NULL;
    json_t *curObjects = NULL;
    json_t *objCopy = NULL;
    json_t *instCopy = NULL;
    json_t *curInst = NULL;

    /* lock config lock */
    pthread_rwlock_wrlock(&srvConfiglock);

	/* Process Destination mapping rules */
	json_t *dmrObjects = json_object_get(migratedObjects, "DestinationMappingRule");
	if ( dmrObjects ) {
		/* check if objkey exist in current configuration */
		curObjects = json_object_get(srvConfigRoot, "DestinationMappingRule");
		if ( curObjects ) {
			/* loop thru all objects */
			json_object_foreach(dmrObjects, objkey, objval) {
				instCopy = json_copy(objval);
				/* check if new DMR object exists */
				curInst = json_object_get(curObjects, objkey);
				if ( curInst ) {
					/* replace existing */
					json_object_set(curObjects, objkey, instCopy);
				} else {
					/* Add new */
					json_object_set_new(curObjects, objkey, instCopy);
				}
			}

		} else {
			/* Add all objects */
			objCopy = json_deep_copy(dmrObjects);
			json_object_set_new(srvConfigRoot, "DestinationMappingRule", objCopy);
		}
	}

	/* Process Queue Manager Connections */
	json_t *qmcObjects = json_object_get(migratedObjects, "QueueManagerConnection");
	if ( qmcObjects ) {
		/* check if objkey exist in current configuration */
		curObjects = json_object_get(srvConfigRoot, "QueueManagerConnection");
		if ( curObjects ) {
			/* loop thru all objects */
			json_object_foreach(qmcObjects, objkey, objval) {
				instCopy = json_copy(objval);
				/* check if new DMR object exists */
				curInst = json_object_get(curObjects, objkey);
				if ( curInst ) {
					/* replace existing */
					json_object_set(curObjects, objkey, instCopy);
				} else {
					/* Add new */
					json_object_set_new(curObjects, objkey, instCopy);
				}
			}
		} else {
			/* Add all objects */
			objCopy = json_deep_copy(qmcObjects);
			json_object_set_new(srvConfigRoot, "QueueManagerConnection", objCopy);
		}
	}

    /* lock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

	return rc;
}

/**
 * Import configuration data from v1 or v2
 */
XAPI int ism_config_json_parseServiceImportPayload(ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
    json_t *post = NULL;
	const char * repl[5];
	int replSize = 0;

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto PROCESSPOST_END;
    }


    if ( newSrvConfigLockInited == 0 ) {
        /* Initialize rwlock to protect server configuration JSON object */
        pthread_rwlockattr_t rwlockattr_init;
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&newSrvConfigLock, &rwlockattr_init);
        newSrvConfigLockInited = 1;
    }

    /* Validate input arguments */
    char *fileName = NULL;
    char *password = NULL;
    int disableObjects = 1;  /* Default is disable objects */
    int applyConfig = 0;     /* By default imported configuration is not applied, unless this flag is set */
    int cfgFile = 0;

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
    	if ( !strcmp(objkey, "Version")) continue;

    	if ( !strcmp(objkey, "FileName")) {

    		/* get imported file name */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "FileName", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		fileName = (char *)json_string_value(objval);
    	    if (!fileName || (fileName && *fileName == '\0')) {
    	        TRACE(3, "Invalid import file name is specified: %s \n", fileName?fileName:"");
    	        rc = ISMRC_BadPropertyValue;
    	        ism_common_setErrorData(rc, "%s%s", "FileName", fileName?fileName:"");
    	        goto PROCESSPOST_END;
    	    }

    	} else if (!strcmp(objkey, "Password")) {

    		/* get password */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Password", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		password = (char *)json_string_value(objval);
			if (!password || (password && *password == '\0')) {
				TRACE(3, "Invalid password is specified: %s \n", password?password:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "Password", password?password:"");
				goto PROCESSPOST_END;
			}

    	} else if (!strcmp(objkey, "DisableObjects")) {

    		/* get DisableObjects flag */
    		if ( objval && (json_typeof(objval) != JSON_TRUE && json_typeof(objval) != JSON_FALSE) ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "DisabledObjects", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    	    if ( json_typeof(objval) == JSON_FALSE) {
    	    	disableObjects = 0;
    	    }

    	} else if (!strcmp(objkey, "ApplyConfig")) {

    		/* get DisableObjects flag */
    		if ( objval && (json_typeof(objval) != JSON_TRUE && json_typeof(objval) != JSON_FALSE) ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "ApplyConfig", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    	    if ( json_typeof(objval) == JSON_TRUE) {
    	    	applyConfig = 1;
    	    }

    	} else if (!strcmp(objkey, "CFGFile")) {

    		/* get cfgfile flag */
    		if ( objval && (json_typeof(objval) != JSON_TRUE && json_typeof(objval) != JSON_FALSE) ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "CFGFile", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    	    if ( json_typeof(objval) == JSON_TRUE) {
    	    	cfgFile = 1;
    	    }

    	} else {
    		rc = ISMRC_ArgNotValid;
    		ism_common_setErrorData(rc, "%s", objkey);
    		goto PROCESSPOST_END;
    	}

    }

    if ( fileName == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "FileName", "null");
        goto PROCESSPOST_END;
    }

    if ( password == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "Password", "null");
        goto PROCESSPOST_END;
    }

    char *usePasswd = (char *)alloca(strlen(password) + strlen(PWKEY) + 2);
    sprintf(usePasswd, "%s-%s", password, PWKEY);

    if ( cfgFile == 0 ) {
        TRACE(9, "Invoke restore script.\n");
        int rc1 = 0;
        rc = ism_config_execAndReturnCode(fileName, usePasswd, 1, &rc1);
        if ( rc1 != 0 ) {
            rc = ismcli_getBackupErrorMsg(rc1);
	    	if ( rc == 6232 || rc == 6223 || rc == 6224 || rc == 6228 || rc == 6229 ) {
	    		ism_common_setErrorData(rc, "%s", fileName?fileName:"");
	    	}
            goto PROCESSPOST_END;
        }
    }

    int processV2Config = 0;
    char filePath[2048];
    sprintf(filePath, "/tmp/userfiles/.v2Config");
    if ( ism_config_isFileExist(filePath) ) {
    	processV2Config = 1;
    	unlink(filePath);
    }

    /* Create a new JSON root node */
    pthread_rwlock_wrlock(&newSrvConfigLock);

    json_t *newConfigRoot = NULL;

    if ( rc == ISMRC_OK ) {
        if ( processV2Config == 1 ) {
        	sprintf(filePath, "/tmp/userfiles/%s.tmpdir" IMA_SVR_DATA_PATH "/data/config/server_dynamic.json", fileName);
        	newConfigRoot = ism_config_json_fileToObject(filePath);
        	if ( newConfigRoot ) {
        	    sprintf(filePath, "/tmp/userfiles/%s.tmpdir", fileName);
        	    unlink(filePath);
        	}
        } else {
            /* Import server config */
            if ( cfgFile == 1 ) {
                sprintf(filePath, "/tmp/userfiles/%s", fileName);
            } else {
            	sprintf(filePath, "/tmp/userfiles/%s.tmpdir" IMA_SVR_INSTALL_PATH "/config/server_dynamic.cfg", fileName);
            }
			newConfigRoot = ism_config_migrate_propsToJSON(filePath, newConfigRoot, &rc);

			if ( cfgFile == 0 && rc == ISMRC_OK && newConfigRoot != NULL ) {
				/* Import mqcbridge config */
				sprintf(filePath, "/tmp/userfiles/%s.tmpdir" IMA_SVR_INSTALL_PATH "/config/mqcbridge_dynamic.cfg", fileName);
				newConfigRoot = ism_config_migrate_propsToJSON(filePath, newConfigRoot, &rc);
			}
			if ( newConfigRoot ) {
	            if ( cfgFile == 1 ) {
	                sprintf(filePath, "/tmp/userfiles/%s", fileName);
	            } else {
	            	sprintf(filePath, "/tmp/userfiles/%s.tmpdir", fileName);
	            }
	            unlink(filePath);
			}
        }
    } else {
        TRACE(5, "Failed to execute restore script: rc=%d\n", rc);
        rc = ISMRC_Error;
        pthread_rwlock_unlock(&newSrvConfigLock);
        goto PROCESSPOST_END;
    }

    /* Update mew configuration */
    if ( rc == ISMRC_OK && newConfigRoot != NULL ) {
        rc = ism_config_migrate_updateResetNewConfig(newConfigRoot, disableObjects);
        if ( rc != ISMRC_OK ) {
            json_decref(newConfigRoot);
            pthread_rwlock_unlock(&newSrvConfigLock);
    	    goto PROCESSPOST_END;
        }
    } else {
    	if ( newConfigRoot ) json_decref(newConfigRoot);
    	pthread_rwlock_unlock(&newSrvConfigLock);
    	goto PROCESSPOST_END;
    }

    /* Can not apply the config if configuration is not from a backup file */
    if ( cfgFile == 1 ) {
    	/* Can not apply the config */
    	applyConfig = 0;
    }

	if ( applyConfig == 1 ) {
	    int haSync = 0;
	    int validate = 0;
	    int restCall = 1;
	    char tbuf[2048];
	    concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
	    int onlyMqcItems = 0;
	    memset(tbuf, 0, 2048);
	    rc = ism_config_json_processObject(newConfigRoot, NULL, NULL, haSync, validate, restCall, &mqcBuffer, &onlyMqcItems);

	    if ( rc == ISMRC_OK && mqcBuffer.used > 0 ) {
	    	/* Process MQC objects */
	    	rc = ism_config_migrate_MQCObjects(newConfigRoot);
	    }
		if ( rc == ISMRC_OK ) {
			ism_config_json_updateFile(1);
			ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
			adminInitError = ISMRC_RestartNeeded;
			backupApplied = 1;
			serverState = ISM_SERVER_MAINTENANCE;
		}
	} else {
        /* create post string */
        char *dumpStr = json_dumps(newConfigRoot, JSON_INDENT(2)|JSON_PRESERVE_ORDER|JSON_ENCODE_ANY);
        if ( dumpStr ) {
		    ism_common_allocBufferCopyLen(&http->outbuf, dumpStr, strlen(dumpStr));
		    ism_common_allocBufferCopyLen(&http->outbuf, "\n", 1);
		    ism_common_free_raw(ism_memory_admin_misc,dumpStr);
        }
	}

	if ( newConfigRoot)
	    json_decref(newConfigRoot);
    pthread_rwlock_unlock(&newSrvConfigLock);

PROCESSPOST_END:

    if ( post )
        json_decref(post);

    if (callback) {
        if (rc) {
            ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	    }
		callback(http, rc);
	}

	return rc;
}


/**
 * Process v1 configuration sent using HA replication route
 */
XAPI int ism_config_migrate_processV1ConfigViaHA(void) {
	int rc = ISMRC_OK;

    if ( newSrvConfigLockInited == 0 ) {
        /* Initialize rwlock to protect server configuration JSON object */
        pthread_rwlockattr_t rwlockattr_init;
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&newSrvConfigLock, &rwlockattr_init);
        newSrvConfigLockInited = 1;
    }

    /* Create a new JSON root node */
    pthread_rwlock_wrlock(&newSrvConfigLock);

    json_t *newConfigRoot = NULL;

    /* Import server config */
    newConfigRoot = ism_config_migrate_propsToJSON("/ima/config/server_dynamic.cfg", newConfigRoot, &rc);
    if ( rc == ISMRC_OK && newConfigRoot != NULL ) {
    	/* Import mqcbridge config */
    	newConfigRoot = ism_config_migrate_propsToJSON("/ima/config/mqcbridge_dynamic.cfg", newConfigRoot, &rc);
    }

    /* Update mew configuration */
    if ( rc == ISMRC_OK && newConfigRoot != NULL ) {
    	int disableObjects = 1;
        rc = ism_config_migrate_updateResetNewConfig(newConfigRoot, disableObjects);
        if ( rc != ISMRC_OK ) {
            json_decref(newConfigRoot);
            pthread_rwlock_unlock(&newSrvConfigLock);
    	    return rc;;
        }
    } else {
    	if ( newConfigRoot ) json_decref(newConfigRoot);
    	pthread_rwlock_unlock(&newSrvConfigLock);
    	return rc;
    }

    int haSync = 0;
    int validate = 0;
    int restCall = 1;
    char tbuf[2048];
    concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
    int onlyMqcItems = 0;
    memset(tbuf, 0, 2048);
    rc = ism_config_json_processObject(newConfigRoot, NULL, NULL, haSync, validate, restCall, &mqcBuffer, &onlyMqcItems);

	if ( rc == ISMRC_OK ) {
		ism_config_json_updateFile(1);
	}

	if ( newConfigRoot) json_decref(newConfigRoot);
    pthread_rwlock_unlock(&newSrvConfigLock);

   	return rc;
}

/**
 * Export configuration data
 */
XAPI int ism_config_json_parseServiceExportPayload(ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
    json_t *post = NULL;
	const char * repl[5];
	int replSize = 0;

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto EXPPROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    if ( newSrvConfigLockInited == 0 ) {
        /* Initialize rwlock to protect server configuration JSON object */
        pthread_rwlockattr_t rwlockattr_init;
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&newSrvConfigLock, &rwlockattr_init);
        newSrvConfigLockInited = 1;
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto EXPPROCESSPOST_END;
    }


    /* Validate input arguments */
    char *fileName = NULL;
    char *password = NULL;

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
    	if ( !strcmp(objkey, "Version")) continue;

    	if ( !strcmp(objkey, "FileName")) {

			/* get imported file name */
			if ( objval && json_typeof(objval) != JSON_STRING ) {
				/* Invalid type */
				rc = ISMRC_BadPropertyType;
				ism_common_setErrorData(rc, "%s%s%s%s", "FileName", "null", "null", ism_config_json_typeString(json_typeof(objval)));
				goto EXPPROCESSPOST_END;
			}
			fileName = (char *)json_string_value(objval);
			if (!fileName || (fileName && *fileName == '\0')) {
				TRACE(3, "Invalid import file name is specified: %s \n", fileName?fileName:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "FileName", fileName?fileName:"");
				goto EXPPROCESSPOST_END;
			}

		} else if (!strcmp(objkey, "Password")) {

    		/* get password */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Password", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto EXPPROCESSPOST_END;
    		}
    		password = (char *)json_string_value(objval);
			if (!password || (password && *password == '\0')) {
				TRACE(3, "Invalid password is specified: %s \n", password?password:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "Password", password?password:"");
				goto EXPPROCESSPOST_END;
			}

    	} else {
    		rc = ISMRC_ArgNotValid;
    		ism_common_setErrorData(rc, "%s", objkey);
    		goto EXPPROCESSPOST_END;
    	}

    }

    if ( password == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "Password", "null");
        goto EXPPROCESSPOST_END;
    }

    char *usePasswd = (char *)alloca(strlen(password) + strlen(PWKEY) + 2);
    sprintf(usePasswd, "%s-%s", password, PWKEY);

    /* Create a new JSON root node */
    pthread_rwlock_wrlock(&newSrvConfigLock);

    /* Create filepath for backup file */
    char filePath[2048];
    char timeStr[64];
    time_t timer;
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(timeStr, 25, "%H%M%S-%m%d%Y", tm_info);
    sprintf(filePath, "imaBackup.%s.%s", ism_common_getServerUID(), timeStr);

    /* invoke script to create backup file */
    TRACE(9, "Invoke restore script.\n");
    int rc1 = 0;

    if ( fileName ) {
        rc = ism_config_execAndReturnCode(fileName, usePasswd, 2, &rc1);
    } else {
    	rc = ism_config_execAndReturnCode(filePath, usePasswd, 2, &rc1);
    }
    if ( rc1 != 0 ) {
        rc = ismcli_getBackupErrorMsg(rc1);
    	if ( rc == 6232 || rc == 6223 || rc == 6224 || rc == 6228 || rc == 6229 ) {
    		if ( fileName ) {
    		    ism_common_setErrorData(rc, "%s", fileName);
    		} else {
    			ism_common_setErrorData(rc, "%s", filePath);
    		}
    	}
    }

    pthread_rwlock_unlock(&newSrvConfigLock);

EXPPROCESSPOST_END:

    if ( post ) json_decref(post);

	if (callback) {
	    if (rc) {
	        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	    } else {
            ism_confg_rest_createErrMsg(http, 6011, repl, replSize);
        }

		callback(http, rc);
	}

	return rc;
}

/**
 * Create http respose to return client set export/import status
 */
static void ism_config_clientSetExportImportReturnMessage(ism_http_t *http, char * serviceType, int rc, uint64_t requestID) {
	char  msgID[12];
	char *locale = NULL;
	char buf[4096];
	char *bufptr = buf;
	char *errstr = NULL;
	int inheap = 0;
	int bytes_needed = 0;
	char reqStr[1024];

	http->outbuf.used = 0;

	if (http->locale && *(http->locale) != '\0') {
		locale = http->locale;
	} else {
		locale = "en_US";
	}

    if (rc == ISMRC_OK) {
        rc = 6011;
    } else {
        if (rc == ISMRC_AsyncCompletion) {
            ism_common_setError(rc);
        } else if (requestID != 0) {
            char rcbuf[64];
            const char *reason = ism_common_getErrorName(rc, rcbuf, sizeof(rcbuf));

            if (reason == NULL || reason[0] != 'I') reason = "UNKNOWN";

            // Turn requestID into a string to avoid message formatting it with commas
            sprintf(reqStr, "%lu", requestID);

            ism_common_setErrorData(6174, "%s%s%s%d", serviceType, reqStr, reason, rc);
            rc = 6174;
        }

        bytes_needed= ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
        if (bytes_needed > sizeof(buf)) {
            bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,267),bytes_needed);
            inheap=1;
            bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
        }
    }

    if (bytes_needed > 0) {
		errstr=(char *)bufptr;
	} else {
		errstr = (char *)ism_common_getErrorStringByLocale(rc, locale, buf, sizeof(buf));
	}

    ism_admin_getMsgId(rc, msgID);

	ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
	ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
	ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
	ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
	ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
	if ( errstr ) {
		ism_json_putEscapeBytes(&http->outbuf, errstr, (int) strlen(errstr));
	} else {
		ism_json_putEscapeBytes(&http->outbuf, "Unknown", 7);
	}
    ism_json_putBytes(&http->outbuf, "\",");

    sprintf(reqStr, "\"RequestID\":%lu }\n", requestID);
	ism_common_allocBufferCopyLen(&http->outbuf, reqStr, (int)strlen(reqStr));

	if(inheap==1) {
		ism_common_free(ism_memory_admin_misc,bufptr);
		bufptr = buf;
	}

	return;
}

/**
 * Import ClientSet data from a file.
 *
 * This function invokes an engine API:
 *  XAPI int32_t WARN_CHECKRC ism_engine_importResources(
 *       const char *                    pFilename,
 *       const char *                    pPassword,
 *       uint32_t                        options,
 *       uint64_t *                      pRequestId,
 *       void *                          pContext,
 *       size_t                          contextLength,
 *       ismEngine_CompletionCallback_t  pCallbackFn);
 *
 * REST Call Payload schema:
 * {
 *   "FileName":"string"
 *   "Password":"string",
 * }
 */
XAPI int ism_config_json_parseServiceImportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
	int ha_rc = ISMRC_OK;
    json_t *post = NULL;
    uint32_t options = 0;
    uint64_t reqID = 0;

	if ( engineImportResources_f == NULL ) {
		engineImportResources_f = (ism_engine_importResources_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_importResources_fnptr", 0L);
	}

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    if ((ism_admin_getHArole(NULL, &ha_rc) != ISM_HA_ROLE_PRIMARY) &&  (ism_admin_isHAEnabled() == 1)) {
        if (ha_rc == ISMRC_OK) {
            rc = ISMRC_NotAllowedOnStandby;
            ism_common_setErrorData(rc, "%s%s", "import", "ClientSet");
        } else
            rc = ha_rc;
        goto PROCESSPOST_END;
    }

    if ((ismCUNITEnv == 0) && (serverState != ISM_SERVER_RUNNING)) {
        rc = ISMRC_ServerStateProduction;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto PROCESSPOST_END;
    }

    /* Validate input arguments */
    char *fileName = NULL;
    char *password = NULL;

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
    	if ( !strcmp(objkey, "Version")) continue;

    	if ( !strcmp(objkey, "FileName")) {

    		/* get imported file name */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "FileName", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		fileName = (char *)json_string_value(objval);
    	    if (!fileName || (fileName && *fileName == '\0') || strstr(fileName,".status")) {
    	        TRACE(3, "Invalid import file name is specified: %s \n", fileName?fileName:"");
    	        rc = ISMRC_BadPropertyValue;
    	        ism_common_setErrorData(rc, "%s%s", "FileName", fileName?fileName:"");
    	        goto PROCESSPOST_END;
    	    } else if (strlen(fileName) > 255) {
                rc = ISMRC_NameLimitExceed;
                ism_common_setErrorData(rc, "%s%s%d", "FileName", fileName?fileName:"", (int) strlen(fileName));
                goto PROCESSPOST_END;
            }

    	} else if (!strcmp(objkey, "Password")) {

    		/* get password */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Password", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		password = (char *)json_string_value(objval);
			if (!password || (password && *password == '\0')) {
				TRACE(3, "Invalid password is specified: %s \n", password?password:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "Password", password?password:"");
				goto PROCESSPOST_END;
			} else if (strlen(password) > 16) {
                rc = ISMRC_LenthLimitExceed;
                ism_common_setErrorData(rc, "%s%s%d", "Password", "********", (int) strlen(password));
                goto PROCESSPOST_END;
            }

    	} else {
    		rc = ISMRC_ArgNotValid;
    		ism_common_setErrorData(rc, "%s", objkey);
    		goto PROCESSPOST_END;
    	}

    }

    if ( fileName == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "FileName", "null");
        goto PROCESSPOST_END;
    }

    if ( password == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "Password", "null");
        goto PROCESSPOST_END;
    }

    /* Invoke engine call */
    if ( engineImportResources_f ) {
    	rc = engineImportResources_f(fileName, password, options, &reqID, NULL, 0, NULL);
    } else {
    	TRACE(1, "NULL pointer for Engine API to import client set.\n");
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    }


PROCESSPOST_END:

	if ( post ) json_decref(post);

	if (callback) {
		ism_config_clientSetExportImportReturnMessage(http, "import", rc, reqID);
 		callback(http, rc);
	}

	return rc;
}


/**
 * Export ClientSet data - creates a file
 *
 * This function invokes an engine API:
 *   XAPI int32_t WARN_CHECKRC ism_engine_exportResources(
 *       const char *                    pClientId,
 *       const char *                    pTopic,
 *       const char *                    pFilename,
 *       const char *                    pPassword,
 *       uint32_t                        options,
 *       uint64_t *                      pRequestId,
 *       void *                          pContext,
 *       size_t                          contextLength,
 *       ismEngine_CompletionCallback_t  pCallbackFn);
 *
 * REST Call Payload schema:
 * {
 *   "FileName":"string"
 *   "Password":"string",
 *   "ClientID":"string",
 *   "Topic":"string"
 *   "Overwrite": true|false // defaults to false - optional
 * }
 */
XAPI int ism_config_json_parseServiceExportClientSetPayload(ism_http_t *http, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
	int ha_rc = ISMRC_OK;
    json_t *post = NULL;
    uint64_t reqID = 0;

	if ( engineExportResources_f == NULL ) {
		engineExportResources_f = (ism_engine_exportResources_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_exportResources_fnptr", 0L);
	}

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    if ((ism_admin_getHArole(NULL, &ha_rc) != ISM_HA_ROLE_PRIMARY) &&  (ism_admin_isHAEnabled() == 1)) {
        if (ha_rc == ISMRC_OK) {
            rc = ISMRC_NotAllowedOnStandby;
            ism_common_setErrorData(rc, "%s%s", "export", "ClientSet");
        } else
            rc = ha_rc;
        goto PROCESSPOST_END;
    }

    if ((ismCUNITEnv == 0) && (serverState != ISM_SERVER_RUNNING)) {
            rc = ISMRC_ServerStateProduction;
            ism_common_setError(rc);
            goto PROCESSPOST_END;
    }

    /* Validate input arguments */
    char *fileName = NULL;
    char *password = NULL;
    char *clientID = NULL;
    char *topic = NULL;
    uint32_t options = 0;

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto PROCESSPOST_END;
    }

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
    	if ( !strcmp(objkey, "Version")) continue;

    	if ( !strcmp(objkey, "FileName")) {

    		/* get imported file name */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "FileName", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		fileName = (char *)json_string_value(objval);
    	    if (!fileName || (fileName && *fileName == '\0') || strstr(fileName,".status")) {
    	        TRACE(3, "Invalid import file name is specified: %s \n", fileName?fileName:"");
    	        rc = ISMRC_BadPropertyValue;
    	        ism_common_setErrorData(rc, "%s%s", "FileName", fileName?fileName:"");
    	        goto PROCESSPOST_END;
    	    } else if (strlen(fileName) > 255) {
    	        rc = ISMRC_NameLimitExceed;
    	        ism_common_setErrorData(rc, "%s%s%d", "FileName", fileName?fileName:"", (int) strlen(fileName));
    	        goto PROCESSPOST_END;
    	    }

    	} else if (!strcmp(objkey, "Password")) {

    		/* get password */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Password", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		password = (char *)json_string_value(objval);
			if (!password || (password && *password == '\0')) {
				TRACE(3, "Invalid password is specified: %s \n", password?password:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "Password", password?password:"");
				goto PROCESSPOST_END;
			} else if (strlen(password) > 16) {
			    rc = ISMRC_LenthLimitExceed;
                ism_common_setErrorData(rc, "%s%s%d", "Password", "********", (int) strlen(password));
                goto PROCESSPOST_END;
			}

    	} else if (!strcmp(objkey, "ClientID")) {

    		/* get ClientID */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "ClientID", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		clientID = (char *)json_string_value(objval);
			if (!clientID || (clientID && *clientID == '\0')) {
				TRACE(3, "Invalid ClientID is specified: %s \n", clientID?clientID:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "ClientID", clientID?clientID:"");
				goto PROCESSPOST_END;
			}


    	} else if (!strcmp(objkey, "Topic")) {

    		/* get Topic */
    		if ( objval && json_typeof(objval) != JSON_STRING ) {
    			/* Invalid type */
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Topic", "null", "null", ism_config_json_typeString(json_typeof(objval)));
    			goto PROCESSPOST_END;
    		}
    		topic = (char *)json_string_value(objval);
			if (!topic || (topic && *topic == '\0')) {
				TRACE(3, "Invalid Topic is specified: %s \n", topic?topic:"");
				rc = ISMRC_BadPropertyValue;
				ism_common_setErrorData(rc, "%s%s", "Topic", topic?topic:"");
				goto PROCESSPOST_END;
			}


    	} else if (!strcmp(objkey, "Overwrite")) {
    	    if (json_typeof(objval) == JSON_TRUE)
    	        options |= ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE;
    	} else {
    		rc = ISMRC_ArgNotValid;
    		ism_common_setErrorData(rc, "%s", objkey);
    		goto PROCESSPOST_END;
    	}

    }

    if ( fileName == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "FileName", "null");
        goto PROCESSPOST_END;
    }

    if ( password == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "Password", "null");
        goto PROCESSPOST_END;
    }

    if ( clientID == NULL && topic == NULL ) {
        rc = ISMRC_PropertyRequired;
        ism_common_setErrorData(rc, "%s%s", "ClientID and Topic", "null");
        goto PROCESSPOST_END;
    }

    /* Invoke engine call */
    if ( engineExportResources_f ) {
    	rc = engineExportResources_f(clientID, topic, fileName, password, options, &reqID, NULL, 0, NULL);
    } else {
    	TRACE(1, "NULL pointer for Engine API to export client set.\n");
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    }

PROCESSPOST_END:

    	if ( post ) json_decref(post);

    	if (callback) {
    		ism_config_clientSetExportImportReturnMessage(http, "export", rc, reqID);
    		callback(http, rc);
    	}

    	return rc;

}


const char * formatTimestampKeys[] = {
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_STARTTIME,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME,
};

/* The order of the enums should match the above array */
enum ism_ImportExport_Timestamp {
    IE_STATUSSTART  =   0,
    IE_STATUSUPDATE =   1,
    IE_STATUSEND    =   2,
};

const int NUM_TIMESTAMP_KEYS = 3;
const char * notReturnedKeysImportExportCommon[] = {
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERNAME,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERUID,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERNAME,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERUID,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_SERVERINITTIME,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSREAD,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSSTARTED,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSWRITTEN,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSFINISHED,
        "\0"
};

const char * notReturnedKeysExportSpecific[] = {
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSIMPORTED,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSIMPORTED,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSIMPORTED,
        "\0"
};

const char * notReturnedKeysImportSpecific[] = {
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSEXPORTED,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSEXPORTED,
        ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSEXPORTED,
        "\0"
};

const char * notReturnedImportExportArrayKeys[] = {
        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEDATAID,
        "\0"
};
XAPI int ism_config_json_getClientSetStatus(ism_http_t * http, char * serviceType, char * requestID) {
    int rc = ISMRC_OK;

    char * statusPath = NULL;
    int statusPathLen = 0;
    const char * dataDir = NULL;
    const char * statusPrefix = "request_";
    const char * statusSuffix = ".status";
    char * retStrng = NULL;
    json_t * root = NULL;

    int timestampArrayLen = sizeof(formatTimestampKeys)/sizeof(char *);
    long timestampArray[timestampArrayLen];

    if (!strcmp(serviceType, "export")) {
        dataDir = ism_common_getStringConfig("ExportDir");
    } else {
        dataDir = ism_common_getStringConfig("ImportDir");
    }

    statusPathLen = strlen(dataDir) + strlen(statusPrefix) + strlen(requestID) + strlen(statusSuffix) + 2;
    statusPath = alloca(statusPathLen);
    snprintf(statusPath, statusPathLen, "%s/%s%s%s", dataDir, statusPrefix, requestID, statusSuffix);
    if (isFileExist(statusPath) == 0) {
        TRACE(3, "Status File:%s, is not found.\n", statusPath);
        rc = 6178;
        ism_common_setErrorData(rc, "%s%s", serviceType, requestID);
        goto STATUS_EXIT;
    }

    root = ism_config_json_fileToObject(statusPath);
    json_t * objval = NULL;
    const char * objkey = NULL;
    int retcode = 0;
    int statusCode = 0;

    if (!root) {
        TRACE(3, "Status file:%s is not in a valid JSON format\n", statusPath);
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto STATUS_EXIT;
    }

    json_object_foreach(root, objkey, objval) {
        if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE)) {
            retcode = json_integer_value(objval);
        } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS)) {
            statusCode = json_integer_value(objval);
        } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STARTTIME)) {
            timestampArray[IE_STATUSSTART] = json_integer_value(objval);
        } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME)) {
            timestampArray[IE_STATUSUPDATE] = json_integer_value(objval);
        } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME)) {
            timestampArray[IE_STATUSEND] = json_integer_value(objval);
        }
    }

    ism_ts_t * ts = NULL;
    ts = ism_common_openTimestamp(ISM_TZF_UTC);
    if ( ts != NULL) {
        int i = 0;
        char tbuffer[80];
        char * val = NULL;

        while (i < timestampArrayLen) {
            ism_common_setTimestamp(ts, (uint64_t) timestampArray[i]);
            val = ism_common_formatTimestamp(ts, tbuffer, 80, 0, ISM_TFF_ISO8601);
            json_object_set(root, formatTimestampKeys[i], json_string(val));
            memset(tbuffer, 0, 80);
            i++;
        }
        ism_common_closeTimestamp(ts);
    } else {
        TRACE(3, "Could not convert timestamp values to ISO8601. Returning timestamps as only integer values of time since Unix epoch.\n.");
    }
    /* Delete unnecessary keys AND unnecessary keys in array objects */
    int deleteIndex = 0;

    /* Delete objects common to both import and export calls */
    while (*(notReturnedKeysImportExportCommon[deleteIndex]) != '\0') {
        json_object_del(root, notReturnedKeysImportExportCommon[deleteIndex]);
        deleteIndex++;
    }

    /* remove unnecessary object key-value pairs INSIDE each array element common to both import and export calls,
     * only if diagnostics array EXISTS */
    deleteIndex = 0;
    int arrayIndex = 0;
    json_t * json_array_ImportExport = NULL;
    json_array_ImportExport = json_object_get(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS);
    if (json_array_ImportExport) {
        while (*(notReturnedImportExportArrayKeys[deleteIndex]) != '\0') {
            json_t * arrayObjKeyValue = NULL;;
            json_array_foreach(json_array_ImportExport, arrayIndex, arrayObjKeyValue) {
                json_t * arrayObjValue = NULL;
                arrayObjValue = json_object_get(arrayObjKeyValue, notReturnedImportExportArrayKeys[deleteIndex]);
                if (arrayObjValue != NULL) {
                    /* this key-value pair was found inside the array element, delete it */
                    json_object_del(arrayObjKeyValue, notReturnedImportExportArrayKeys[deleteIndex]);
                    /* update the array with the modified element */
                    json_array_set(json_array_ImportExport, arrayIndex, arrayObjKeyValue);
                }
            }
            /* don't need to check to see if we found the entry in any of the array elements, just go to next one */
            deleteIndex++;
            arrayIndex = 0;
        }
        /* update root with the modified array */
        json_object_set(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS, json_array_ImportExport);
    }

    /* Delete objects specific to import or export call */
    if (!strcmp(serviceType, "export")) {
        deleteIndex = 0;
        while (*(notReturnedKeysExportSpecific[deleteIndex]) != '\0') {
            json_object_del(root, notReturnedKeysExportSpecific[deleteIndex]);
            deleteIndex++;
        }
    } else {
        deleteIndex = 0;
        while (*(notReturnedKeysImportSpecific[deleteIndex]) != '\0') {
            json_object_del(root, notReturnedKeysImportSpecific[deleteIndex]);
            deleteIndex++;
        }
    }

    /* If we have an in-progress request then delete the EndTime key */
    if (statusCode == 0) {
        json_object_del(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME);
    }

    retStrng = json_dumps(root, JSON_PRESERVE_ORDER);
    if (!retStrng) {
        TRACE(3, "Status file:%s could not write to file in JSON form\n", statusPath);
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto STATUS_EXIT;
    }

    /* Strip out the leading and trailing braces. */
    retStrng++;
    char * endBrace = strrchr(retStrng,'}');
    *endBrace = '\0';


STATUS_EXIT:
    if (rc == ISMRC_OK) {
        char msgID[12];
        char buf[4096];
        char * bufptr = buf;
        char * errstr = NULL;
        int inheap = 0;
        int bytes_needed = 0;
        char * locale = NULL;

        http->outbuf.used = 0;
        /*Get Request Locale*/
        if (http && http->locale && *(http->locale) != '\0') {
            locale = http->locale;
        } else {
            locale = alloca(strlen("en_US")+1);
            locale = strcpy(locale, "en_US");
        }

        if (retcode == 0) {
            if (statusCode == 0) {
                retcode = 6176;
                ism_common_setErrorData(retcode, "%s%s", serviceType, requestID);
            } else if (statusCode == 1) {
                retcode = 6177;
                ism_common_setErrorData(retcode, "%s%s", serviceType, requestID);
            }
            bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
        } else {
            char rcbuf[64];
            const char *reason = ism_common_getErrorName(retcode, rcbuf, sizeof(rcbuf));

            if (reason == NULL || reason[0] != 'I') reason = "UNKNOWN";

            ism_common_setErrorData(6174, "%s%s%s%d", serviceType, requestID, reason, retcode);
            retcode = 6174;

            bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
            if (bytes_needed > sizeof(buf)) {
                bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,269),bytes_needed);
                inheap=1;
                bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
            }
        }

        if (bytes_needed > 0) {
            errstr=(char *)bufptr;
        } else {
            errstr = (char *)ism_common_getErrorStringByLocale(retcode, locale, buf, sizeof(buf));
        }

        ism_admin_getMsgId(retcode, msgID);

        ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
        ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
        ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
        ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
        ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
        if ( errstr ) {
            ism_json_putEscapeBytes(&http->outbuf, errstr, (int) strlen(errstr));
        } else {
            ism_json_putEscapeBytes(&http->outbuf, "Unknown", 7);
        }
        ism_json_putBytes(&http->outbuf, "\",");
        ism_json_putBytes(&http->outbuf, retStrng);
        ism_json_putBytes(&http->outbuf, "}\n");

        if(inheap==1) {
            ism_common_free(ism_memory_admin_misc,bufptr);
            bufptr = buf;
        }
    }

    if ( root ) json_decref(root);
    if (retStrng) {
        *endBrace = '}';
        retStrng--;
        //retString comes from the json library so doesn't include a memory eyecatcher
        ism_common_free_raw(ism_memory_admin_misc,retStrng);
    }
    return rc;
}

XAPI int ism_config_json_deleteClientSet(ism_http_t * http, char * object, char * subinst, int force) {
    int rc = ISMRC_OK;
    int requestID = atoi(subinst);

    const char * statusDir;
    const char * statusPrefix = "request_";
    const char * statusSuffix = ".status";

    char * statusPath = NULL;
    int statusPathLen;
    json_t * root = NULL;

    if (requestID == 0) {
        TRACE(3, "Object:%s, RequestID:%s is not valid.\n", object?object:"NULL", subinst?subinst:"NULL");
        rc = 6178;
        ism_common_setErrorData(rc, "%s%s", object, subinst);
        goto mod_exit;
    }

    if (!strcmp(object, "export"))
        statusDir = ism_common_getStringConfig("ExportDir");
    else
        statusDir = ism_common_getStringConfig("ImportDir");

    statusPathLen = strlen(statusDir) + strlen(statusPrefix) + strlen(subinst) + strlen(statusSuffix) + 2;
    statusPath =  alloca(statusPathLen);
    sprintf(statusPath, "%s/%s%s%s", statusDir, statusPrefix, subinst, statusSuffix);

    if (isFileExist(statusPath) == 0) {
        TRACE(3, "Status File:%s, is not found.\n", statusPath?statusPath:"NULL");
        rc = 6178;
        ism_common_setErrorData(rc, "%s%s", object, subinst);
        goto mod_exit;
    }

    root = ism_config_json_fileToObject(statusPath);
    json_t * objval = NULL;
    const char * objkey = NULL;

    int requestStatus;
    const char * filePath = NULL;
    int filePathFound = 0;

    if (!root) {
        TRACE(3, "Status file:%s is not in a valid JSON format\n", statusPath?statusPath:"NULL");
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    json_object_foreach(root, objkey, objval) {
        if (!strcmp(objkey,ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS)) {
            requestStatus = json_integer_value(objval);
            if ((requestStatus == ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS) && (force != 1)) {
                rc = 6176;
                ism_common_setErrorData(rc, "%s%s", object, subinst);
                goto mod_exit;
            }
        } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILEPATH)) {
            filePath = json_string_value(objval);
            filePathFound = 1;
        }
    }

    if (rc == ISMRC_OK) {
        /* delete .status file and import/export file. */
        rc = unlink(statusPath);
        if (rc == ISMRC_OK) {
            if (filePathFound == 1) {
                rc = unlink(filePath);
                if (rc != ISMRC_OK) {
                    // The file might already validly have been deleted by the
                    // deletion of another requestID that used the same file
                    // (with Overwrite on an export, or by specifying the same
                    // file name on an import) - so don't treat ENOENT as a failure.
                    if (errno == ENOENT) {
                        rc = ISMRC_OK;
                    } else {
                        rc = ISMRC_SysCallFailed;
                        ism_common_setErrorData(rc, "%s%d%s", "unlink", errno, strerror(errno));
                    }
                }
            } else if (!strcmp(object, "export")) {
                TRACE(3, "No export file found. Only deleted status file\n");
            } else {
                TRACE(3, "Status file:%s does not have required keys\n", statusPath?statusPath:"NULL");
                rc = ISMRC_Error;
                ism_common_setError(rc);
            }
        } else {
            rc = ISMRC_SysCallFailed;
            ism_common_setErrorData(rc, "%s%d%s", "unlink", errno, strerror(errno));
        }
    } else {
        TRACE(3, "Status file:%s does not have required keys\n", statusPath?statusPath:"NULL");
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }

mod_exit:
    if (rc == ISMRC_OK) {
        char msgID[12];
        char buf[4096];
        char * bufptr = buf;
        char * errstr = NULL;
        int inheap = 0;
        int bytes_needed = 0;
        char * locale = NULL;

        http->outbuf.used = 0;
        /*Get Request Locale*/
        if (http && http->locale && *(http->locale) != '\0') {
            locale = http->locale;
        } else {
            locale = alloca(strlen("en_US")+1);
            locale = strcpy(locale, "en_US");
        }

        rc = 6173;
        ism_common_setErrorData(rc, "%s%s", object, subinst);
        ism_admin_getMsgId(rc, msgID);
        bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
        if (bytes_needed > sizeof(buf)) {
            bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,272),bytes_needed);
            inheap=1;
            bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
        }
        if (bytes_needed > 0) {
            errstr=(char *)bufptr;
        } else {
            errstr = (char *)ism_common_getErrorStringByLocale(rc, locale, buf, sizeof(buf));
        }

        ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
        ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
        ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
        ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
        ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
        if ( errstr ) {
            ism_json_putEscapeBytes(&http->outbuf, errstr, (int) strlen(errstr));
        } else {
            ism_json_putEscapeBytes(&http->outbuf, "Unknown", 7);
        }
        ism_json_putBytes(&http->outbuf, "\"}\n");

        if(inheap==1) {
            ism_common_free(ism_memory_admin_misc,bufptr);
            bufptr = buf;
        }
        rc = ISMRC_OK;
    }
    if ( root ) json_decref(root);
    return rc;
}

XAPI int ism_config_cleanupImportExportClientSet(void) {

    int rc = ISMRC_OK;

    const char * cleanupDirs[] = {
        ism_common_getStringConfig("ExportDir"),
        ism_common_getStringConfig("ImportDir"),
        NULL
    };


    /* cleanup status files that on restart that are 'marked in progress' these need to be changed to failed with approprate rc */
    DIR *dirp;
    struct dirent *dentp;
    int i = 0;

    while(cleanupDirs[i] != NULL) {
        if ((dirp = opendir(cleanupDirs[i])) == NULL) {
            if (errno != ENOENT) {
                rc = ISMRC_SysCallFailed;
                ism_common_setErrorData(rc, "%s%d%s", "opendir", errno, strerror(errno));
                goto mod_exit;
            } else
                goto NEXT_DIR;
        }

        do {
            errno = 0;
            if ((dentp = readdir(dirp)) != NULL) {
                if (strstr(dentp->d_name, ".status") == NULL)
                    continue;
                else {
                    json_t * root = NULL;
                    char * statusPath = NULL;
                    int statusPathLen = 0;

                    statusPathLen = strlen(cleanupDirs[i]) + strlen(dentp->d_name) + 2;
                    statusPath = alloca(statusPathLen);
                    sprintf(statusPath, "%s/%s", cleanupDirs[i], dentp->d_name);
                    statusPath[statusPathLen-1] = '\0';

                    root = ism_config_json_fileToObject(statusPath);
                    if (root) {
                        FILE * fp = NULL;

                        const char * objkey = NULL;
                        json_t * objval = NULL;

                        char * retStrng = NULL;

                        int requestStatus = 0;
                        int retCode = 0;

                        int wrtcnt = 0;

                        json_object_foreach(root, objkey, objval) {
                            if (!strcmp(objkey,ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS)) {
                                requestStatus = json_integer_value(objval);
                            } else if (!strcmp(objkey, ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE)) {
                                retCode = json_integer_value(objval);
                            }
                        }

                        if (requestStatus != ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS) {
                            json_decref(root);
                            continue;
                        }

                        requestStatus = ismENGINE_IMPORTEXPORT_STATUS_FAILED;
                        retCode = ISMRC_RequestInProgressAtStartup;
                        json_object_set(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS, json_integer(requestStatus));
                        json_object_set(root, ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE, json_integer(retCode));

                        retStrng = json_dumps(root, JSON_PRESERVE_ORDER);
                        json_decref(root);
                        if (!retStrng) {
                            TRACE(3, "Status file:%s could not be written to in JSON form\n", dentp->d_name);
                            rc = ISMRC_Error;
                            ism_common_setError(rc);
                            goto mod_exit;
                        }

                        fp = fopen(statusPath,"w+");
                        if (fp) {
                            wrtcnt = fwrite(retStrng, strlen(retStrng), 1, fp);
                            if (wrtcnt != 1) {
                                rc = ISMRC_SysCallFailed;
                                ism_common_setErrorData(rc, "%s%d%s", "write", errno, strerror(errno));
                            }
                            fclose(fp);
                        } else {
                            rc = ISMRC_SysCallFailed;
                            ism_common_setErrorData(rc, "%s%d%s", "open", errno, strerror(errno));
                        }

                        ism_common_free_raw(ism_memory_admin_misc,retStrng);
                        if (rc != ISMRC_OK)
                            goto mod_exit;
                    } else {
                        TRACE(3, "Status file:%s is not in a valid JSON format\n", dentp->d_name);
                        rc = ISMRC_Error;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                }
            }
        } while (dentp != NULL);
NEXT_DIR:
        i++;
        if (dirp)
            closedir(dirp);
    }
mod_exit:
    return rc;
}
