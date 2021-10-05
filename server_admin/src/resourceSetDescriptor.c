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

#include "validateInternal.h"
#include <assert.h>

int resourceSetInited = 0;
pthread_spinlock_t resourceSetSpinLock;
const char *resourceSetDefaultClientID = NULL;
const char *resourceSetDefaultTopic = NULL;

/* Protected by resourceSetSpinLock  */
typedef struct ism_config_ResourceSetDescriptor_t
{
    char *    clientID;        /* field is set only by admin init */
    char *    newClientID;
    char *    topic;           /* field is set only by admin init */
    char *    newTopic;
    int       deleted;
} ism_config_ResourceSetDescriptor_t;

ism_config_ResourceSetDescriptor_t * resourceSetDescriptorInfo = NULL;

XAPI void ism_config_updateResourceSetDescriptor(json_t *mergedObj, int haSync)
{
    const char *newClientID = json_string_value(json_object_get(mergedObj, "ClientID"));
    const char *newTopic = json_string_value(json_object_get(mergedObj, "Topic"));

    assert(resourceSetInited == 1);
    assert(newClientID != NULL || newTopic != NULL);

    /* Allocate or update changes to the resourceSetDescriptor */
    pthread_spin_lock(&resourceSetSpinLock);
    if (!resourceSetDescriptorInfo) {
        resourceSetDescriptorInfo = (ism_config_ResourceSetDescriptor_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,51),1, sizeof(ism_config_ResourceSetDescriptor_t));
        assert(resourceSetDescriptorInfo != NULL);
        if (haSync) {
            resourceSetDescriptorInfo->clientID = newClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newClientID) : NULL;
            resourceSetDescriptorInfo->topic = newTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newTopic) : NULL;
        } else {
            resourceSetDescriptorInfo->newClientID = newClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newClientID) : NULL;
            resourceSetDescriptorInfo->newTopic = newTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newTopic) : NULL;
        }
    } else {
        /* Updates to the config update the 'new' fields if haSync is 0, and the
         * base fields if haSync is 1.
         */
        if (resourceSetDescriptorInfo->newClientID) {
            ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->newClientID);
            resourceSetDescriptorInfo->newClientID = NULL;
        }
        if (resourceSetDescriptorInfo->newTopic) {
            ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->newTopic);
            resourceSetDescriptorInfo->newTopic = NULL;
        }

        /* Only want to indicate the descriptor has changed if the strings are actually
         * different from those being used now
         */
        if ((newClientID && (resourceSetDescriptorInfo->clientID == NULL ||
             strcmp(resourceSetDescriptorInfo->clientID, newClientID))) ||
            (newTopic && (resourceSetDescriptorInfo->topic == NULL ||
             strcmp(resourceSetDescriptorInfo->topic, newTopic)))) {
            if (haSync) {
                ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->clientID);
                resourceSetDescriptorInfo->clientID = newClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newClientID) : NULL;
                ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->topic);
                resourceSetDescriptorInfo->topic = newTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newTopic) : NULL;
            } else {
                resourceSetDescriptorInfo->newClientID = newClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newClientID) : NULL;
                resourceSetDescriptorInfo->newTopic = newTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),newTopic) : NULL;
            }
        }
        resourceSetDescriptorInfo->deleted = 0;
    }
    pthread_spin_unlock(&resourceSetSpinLock);

    return;
}

/*
 * Item: ResourceSetDescriptor
 * Component: Engine
 *
 * Description:
 *      ResourceSetDescriptor which provides matching rules for gathering statistics for MQTT Clients.
 *      Default object created with empty strings for ClientID and TopicString
 * Schema:
 * {
 *      "ResourceSetDescriptor": {
 *          "Name": " String", - is set by server
 *          "ClientID": "RegexSub",
 *          "Topic": "RegexSub"
 *      }
 *  }
 */

XAPI int32_t ism_config_validate_ResourceSetDescriptor(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props) {
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;

    int clientIDEmpty = 0;
    int topicEmpty = 0;


    TRACE(9, "Entry %s: currPostObj:%p, mergedObj:%p, item:%s, name:%s action:%d\n",
        __FUNCTION__, currPostObj?currPostObj:0, mergedObj?mergedObj:0, item?item:"null", name?name:"null", action);

    if ( !mergedObj || !props || !name ) {
        rc = ISMRC_NullPointer;
        goto VALIDATION_END;
    }


    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    reqList = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    if ( json_typeof(mergedObj) == JSON_NULL || json_object_size(mergedObj) == 0 ) {
        rc = ISMRC_NotImplemented;
        ism_common_setErrorData(rc, "%s", "ConfiguationPolicy");
        goto VALIDATION_END;
    }

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        const char * jsonStrng = NULL;


        rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);

        if (!strcmp(key, "ClientID")) {
            jsonStrng = json_string_value(value);
            if (jsonStrng && (*jsonStrng == '\0')) {
                clientIDEmpty = 1;
            }
        } else if( !strcmp(key, "Topic")) {
            jsonStrng = json_string_value(value);
            if (jsonStrng && (*jsonStrng == '\0')) {
                topicEmpty = 1;
            }
        }
        if (rc != ISMRC_OK) goto VALIDATION_END;

        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    /* Check if required items are specified */
    int chkMode = 0;
    rc = ism_config_validate_checkRequiredItemList(reqList, chkMode );

    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* ClientID and Topic cannot be BOTH Empty Strings */
    if (clientIDEmpty && topicEmpty) {
        rc = ISMRC_MinOneOptMissing;
        ism_common_setErrorData(rc, "%s%s","ResourceSetDescriptor", "ClientID, Topic");
    }

    ism_config_updateResourceSetDescriptor(mergedObj, 0);

VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

/**
 * Create http respose to return client set export/import status
 */
static void ism_admin_resourceSetDescriptorReturnMessage(ism_http_t *http, int rc) {
    char  msgID[12];
    char *locale = NULL;
    char buf[4096];
    char *bufptr = buf;
    char *errstr = NULL;
    int inheap = 0;
    int bytes_needed = 0;

    char clientID[512] = {0};
    char topic[512] = {0};
    char postClientID[512] = {0};
    char postTopic[512] = {0};

    http->outbuf.used = 0;

    if (http->locale && *(http->locale) != '\0') {
        locale = http->locale;
    } else {
        locale = "en_US";
    }

    ism_common_setError(rc);
    bytes_needed= ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
    if (bytes_needed > sizeof(buf)) {
        bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,56),bytes_needed);
        inheap=1;
        bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
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


    if (resourceSetDescriptorInfo)
        pthread_spin_lock(&resourceSetSpinLock);
    switch(rc)
    {
        case 6240:
        {
            ism_json_putBytes(&http->outbuf, "\",");
            sprintf(clientID, "\"ClientID\":\"%s\"", resourceSetDescriptorInfo->clientID);
            sprintf(topic, ",\"Topic\":\"%s\" }\n", resourceSetDescriptorInfo->topic);
            break;
        }
        case 6241:
        {
            ism_json_putBytes(&http->outbuf, "\",");
            sprintf(postClientID, "\"ClientIDPostRestart\":\"%s\"", resourceSetDescriptorInfo->newClientID);
            sprintf(postTopic, ",\"TopicPostRestart\":\"%s\" }\n", resourceSetDescriptorInfo->newTopic);
            break;
        }
        case 6242:
        {
            ism_json_putBytes(&http->outbuf, "\",");
            sprintf(clientID, "\"ClientID\":\"%s\"", resourceSetDescriptorInfo->clientID);
            sprintf(topic, ",\"Topic\":\"%s\"", resourceSetDescriptorInfo->topic);
            sprintf(postClientID, ",\"ClientIDPostRestart\":\"%s\"", resourceSetDescriptorInfo->newClientID);
            sprintf(postTopic, ",\"TopicPostRestart\":\"%s\" }\n", resourceSetDescriptorInfo->newTopic);
            break;
        }
        case 6243:
        {
            ism_json_putBytes(&http->outbuf, "\",");
            sprintf(clientID, "\"ClientID\":\"%s\"", resourceSetDescriptorInfo->clientID);
            sprintf(topic, ",\"Topic\":\"%s\" }\n", resourceSetDescriptorInfo->topic);
            break;
        }
        case 6244:
            ism_json_putBytes(&http->outbuf, "\" }\n");
            break;
    }
    if (resourceSetDescriptorInfo)
        pthread_spin_unlock(&resourceSetSpinLock);

    if (*clientID) {
        ism_common_allocBufferCopyLen(&http->outbuf, clientID, (int)strlen(clientID));
    }
    if (*topic) {
        ism_common_allocBufferCopyLen(&http->outbuf, topic, (int)strlen(topic));
    }
    if (*postClientID) {
        ism_common_allocBufferCopyLen(&http->outbuf, postClientID, (int)strlen(postClientID));
    }
    if (*postTopic) {
        ism_common_allocBufferCopyLen(&http->outbuf, postTopic, (int)strlen(postTopic));
    }

    if(inheap==1) {
        ism_common_free(ism_memory_admin_misc,bufptr);
        bufptr = buf;
    }
    return;
}

XAPI int ism_admin_getResourceSetDescriptorStatus(ism_http_t *http, ism_rest_api_cb callback) {
    int rc = ISMRC_OK;
    if (!resourceSetDescriptorInfo) {
        rc = 6244;
    } else {
        pthread_spin_lock(&resourceSetSpinLock);
        if (resourceSetDescriptorInfo->deleted) {
            if (resourceSetDescriptorInfo->clientID || resourceSetDescriptorInfo->topic) {
                rc = 6243;
            } else {
                rc = 6244;
            }
        } else {
            if (resourceSetDescriptorInfo->clientID || resourceSetDescriptorInfo->topic) {
                if (resourceSetDescriptorInfo->newClientID || resourceSetDescriptorInfo->newTopic) {
                    rc = 6242;
                } else {
                    rc = 6240;
                }
            } else {
                rc = 6241;
            }
        }
        pthread_spin_unlock(&resourceSetSpinLock);
    }
    ism_admin_resourceSetDescriptorReturnMessage(http, rc);
    if (callback) {
        callback(http, rc);
    }
    return ISMRC_OK;
}

XAPI void ism_admin_getActiveResourceSetDescriptorValues(const char **pClientID, const char **pTopic) {
    const char *clientID = NULL;
    const char *topic = NULL;
    if (resourceSetDescriptorInfo) {
        clientID = resourceSetDescriptorInfo->clientID;
        topic = resourceSetDescriptorInfo->topic;
    }
    if (pClientID) *pClientID = clientID;
    if (pTopic) *pTopic = topic;
}

/* Caller needs to indicate if server config lock is already held (always called at least once: admin_init() during startup) */
XAPI int ism_admin_isResourceSetDescriptorDefined(int locked) {
    int rc = ISMRC_OK;
    int defined;

    json_t *resourceSetJSON = NULL;
    const char *clientID;
    const char *topic;

    if (!resourceSetInited) {
        resourceSetDefaultClientID = ism_common_getStringConfig("Server.DefaultResourceSetDescriptorClientID");
        resourceSetDefaultTopic = ism_common_getStringConfig("Server.DefaultResourceSetDescriptorTopic");
        pthread_spin_init(&resourceSetSpinLock, 0);
        resourceSetInited = 1;
    }

    resourceSetJSON = ism_config_json_getComposite("ResourceSetDescriptor", "ResourceSetDescriptor", locked);
    if (resourceSetJSON) {
        clientID = json_string_value(json_object_get(resourceSetJSON, "ClientID"));
        topic = json_string_value(json_object_get(resourceSetJSON, "Topic"));
    } else {
        clientID = resourceSetDefaultClientID;
        topic = resourceSetDefaultTopic;
    }

    if (clientID || topic) {
        defined = 1;

        if (!resourceSetDescriptorInfo) {
            resourceSetDescriptorInfo = (ism_config_ResourceSetDescriptor_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,58),1, sizeof(ism_config_ResourceSetDescriptor_t));
            if (!resourceSetDescriptorInfo) {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            } else {
                pthread_spin_lock(&resourceSetSpinLock);
                if (clientID) resourceSetDescriptorInfo->clientID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),clientID);
                if (topic) resourceSetDescriptorInfo->topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),topic);
                pthread_spin_unlock(&resourceSetSpinLock);
            }
        }
    } else {
        defined = 0;
    }

    return defined;
}

XAPI void ism_config_markResourceSetDescriptorDeleted(int haSync) {
    pthread_spin_lock(&resourceSetSpinLock);
    if (resourceSetDescriptorInfo) {
        ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->newClientID);
        resourceSetDescriptorInfo->newClientID = NULL;
        ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->newTopic);
        resourceSetDescriptorInfo->newTopic = NULL;

        // If this delete causes us to drop back to defaults, changing newClientID and newTopic if haSync is 0
        // and clientID and topic if haSync is 1.
        if (resourceSetDefaultClientID || resourceSetDefaultTopic) {
            if ((resourceSetDefaultClientID && (!resourceSetDescriptorInfo->clientID ||
                 strcmp(resourceSetDescriptorInfo->clientID, resourceSetDefaultClientID))) ||
                (resourceSetDefaultTopic && (!resourceSetDescriptorInfo->topic ||
                 strcmp(resourceSetDescriptorInfo->topic, resourceSetDefaultTopic)))) {
                if (haSync) {
                    ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->clientID);
                    resourceSetDescriptorInfo->clientID = resourceSetDefaultClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),resourceSetDefaultClientID) : NULL;
                    ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->topic);
                    resourceSetDescriptorInfo->topic = resourceSetDefaultTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),resourceSetDefaultTopic) : NULL;
                } else {
                    resourceSetDescriptorInfo->newClientID = resourceSetDefaultClientID ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),resourceSetDefaultClientID) : NULL;
                    resourceSetDescriptorInfo->newTopic = resourceSetDefaultTopic ? ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),resourceSetDefaultTopic) : NULL;
                }
            }
        } else {
            if (haSync) {
                ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->clientID);
                ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo->topic);
                ism_common_free(ism_memory_admin_misc,resourceSetDescriptorInfo);
                resourceSetDescriptorInfo = NULL;
            } else {
                resourceSetDescriptorInfo->deleted = 1;
            }
        }
    }
    pthread_spin_unlock(&resourceSetSpinLock);
}
