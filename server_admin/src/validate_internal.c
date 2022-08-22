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
 * This file contains internal data, functions functions etc, required by
 * Generic data validation APIs defined in validate_genericData.c file.
 */

#define TRACE_COMP Admin

#define ismCFG_FREE(x)                   if (x != NULL) { ism_common_free(ism_memory_admin_misc,(void *)x); x = NULL; }
#define CERTIFICATE_PATH  IMA_SVR_INSTALL_PATH "/certificates"

#include "validateInternal.h"


extern pthread_mutex_t g_cfglock;

extern ism_json_parse_t * ConfigSchema;
extern ism_json_parse_t * MonitorSchema;
extern int isConfigSchemaLoad;
extern int isMonitorSchemaLoad;

extern int32_t ism_config_validateDataType_bufferSize( char *name, char *value, char *min, char *max);

/* get value of an attribute */
XAPI char * ism_config_validate_getAttr(char *name, ism_json_parse_t *json, int pos) {
    int jPos = ism_json_get(json, pos, name);
    if ( jPos != -1 ) {
        ism_json_entry_t * tent = json->ent + jPos;
        return (char *)tent->value;
    }
    return NULL;
}


static int ism_config_validate_initRequiredItemList( ism_config_itemValidator_t **reqdItemList ) {
    int rc = ISMRC_OK;
    int i;
    ism_config_itemValidator_t *tmp = NULL;

    TRACE(9, "Entry %s\n", __FUNCTION__ );

    tmp = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,17),1, sizeof(ism_config_itemValidator_t));
    if ( tmp == NULL ) {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto GET_ITEMINIT_END;
    }

    for (i= 0; i< ISM_VALIDATE_CONFIG_ENTRIES;  i++) {
        tmp->name[i] = NULL;
        tmp->defv[i] = NULL;
        tmp->options[i] = NULL;
        tmp->max[i] = NULL;
        tmp->min[i] = NULL;
        tmp->assigned[i] = 0;
        tmp->reqd[i] = 0;
        tmp->type[i] = ISM_CONFIG_PROP_INVALID;
        tmp->minoneopt[i] = 0;
        tmp->total = 0;
    }
    *reqdItemList = tmp;

GET_ITEMINIT_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

XAPI ism_json_parse_t * ism_config_getSchema(int type) {
    ism_json_parse_t * json = NULL;

    TRACE(9, "Entry %s: type: %d\n", __FUNCTION__, type );

    if (type != ISM_CONFIG_SCHEMA && type != ISM_MONITORING_SCHEMA) {
        goto GET_SCHEMA_END;
    }

    if (type == ISM_CONFIG_SCHEMA) {
        if ( isConfigSchemaLoad == 0) {
            ConfigSchema = ism_admin_getSchemaObject(type);
            isConfigSchemaLoad = 1;
        }
        json = ConfigSchema;
    } else {
        if ( isMonitorSchemaLoad == 0) {
            MonitorSchema = ism_admin_getSchemaObject(type);
            isMonitorSchemaLoad = 1;
        }
        json = MonitorSchema;
    }

GET_SCHEMA_END:
    return json;
}
/* *
 * Returns a complete list of defined items of a composite object defined in the JSON schema
 * The required items has their reqd flag set.
 * */
XAPI ism_config_itemValidator_t * ism_config_validate_getRequiredItemList(int type, char *name, int *rc)
{
    ism_config_itemValidator_t *reqdItemList = NULL;
    ism_json_parse_t * json = ism_config_getSchema(type);

    TRACE(9, "Entry %s: type: %d, name: %s\n", __FUNCTION__, type, name?name:"null" );

    if (NULL == json) {
        goto GET_ITEMLIST_END;
    }

    /* sanity check */
    if ( !json || !name || *name == '\0' )
    {
        *rc = ISMRC_NullPointer;
        ism_common_setError(ISMRC_NullPointer);
        goto GET_ITEMLIST_END;
    }

    /* Find position of the object in JSON object */
    int pos = ism_json_get(json, 0, name);
    if ( pos == -1 )
    {
        ism_common_setErrorData(ISMRC_BadPropertyName, "%s", name);
        *rc = ISMRC_BadPropertyName;
        goto GET_ITEMLIST_END;
    }

    int found = 0;
    int i;
    int count =  json->ent[pos].count;
    int level = -1;
    char *maxlen = NULL;
    char *options = NULL;

    int initrc = ism_config_validate_initRequiredItemList(&reqdItemList);
    if (initrc != ISMRC_OK) {
        *rc = initrc;
        goto GET_ITEMLIST_END;
    }

    /* Traverse thru the object to find required items and build the list */
    for ( i = pos; i <= pos+count; i++ ) {
        ism_json_entry_t * jent = json->ent + i;
        if ( level == -1 )
            level = jent->level;
        int checkLevel = level + 1;
        if ( pos == 0 )
            checkLevel = level;
        if ( jent->objtype == JSON_Object && jent->level == checkLevel ) {
            char *ctype = ism_config_validate_getAttr("Type", json, i);
            if ( ctype && jent->name ) {
                reqdItemList->name[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),jent->name);
                /** assign type                */
                if (!strcmpi(ctype, "Number")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_NUMBER;
                    char *minval = ism_config_validate_getAttr("Minimum", json, i);
                    if (minval)
                        reqdItemList->min[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),minval);
                    char *maxval = ism_config_validate_getAttr("Maximum", json, i);
                    if (maxval)
                        reqdItemList->max[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),maxval);
                } else if (!strcmpi(ctype, "BufferSize")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_BUFFERSIZE;
                    char *minval = ism_config_validate_getAttr("Minimum", json, i);
                    if (minval)
                        reqdItemList->min[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),minval);
                    char *maxval = ism_config_validate_getAttr("Maximum", json, i);
                    if (maxval)
                        reqdItemList->max[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),maxval);
                } else if (!strcmpi(ctype, "Enum")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_ENUM;
                    options = ism_config_validate_getAttr("Options", json, i);
                    if (options)
                        reqdItemList->options[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),options);
                } else if (!strcmpi(ctype, "List")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_LIST;
                    options = ism_config_validate_getAttr("Options", json, i);
                    if (options)
                        reqdItemList->options[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),options);
                } else if (!strcmpi(ctype, "String") || !strcmpi(ctype, "StringBig")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_STRING;
                    maxlen = ism_config_validate_getAttr("MaxLength", json, i);
                    if (maxlen)
                        reqdItemList->max[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),maxlen);
                } else if (!strcmpi(ctype, "Name")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_NAME;
                    maxlen = ism_config_validate_getAttr("MaxLength", json, i);
                    if (maxlen)
                        reqdItemList->max[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),maxlen);
                } else if (!strcmpi(ctype, "Boolean")) {
                       reqdItemList->type[found] = ISM_CONFIG_PROP_BOOLEAN;
                } else if (!strcmpi(ctype, "IPAddress")) {
                    char *defval = ism_config_validate_getAttr("Default", json, i);
                    if (defval && ( !strcmpi(defval, "all") || ( *defval == '*')))
                        reqdItemList->tempflag[found] = 1;
                    reqdItemList->type[found] = ISM_CONFIG_PROP_IPADDRESS;
                } else if (!strcmpi(ctype, "URL")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_URL;
                    options = ism_config_validate_getAttr("Options", json, i);
                    if (options)
                        reqdItemList->options[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),options);

                    maxlen = ism_config_validate_getAttr("MaxLength", json, i);
                    if (maxlen)
                        reqdItemList->max[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),maxlen);
                } else if (!strcmpi(ctype, "Regex")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_REGEX;
                } else if (!strcmpi(ctype, "RegexSub")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_REGEX_SUBEXP;
                } else if (!strcmpi(ctype, "Selector")) {
                    reqdItemList->type[found] = ISM_CONFIG_PROP_SELECTOR;
                }
                else {
                    *rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(ISMRC_BadPropertyName, "%s", ctype);
                    goto GET_ITEMLIST_END;
                }

                /* check RequiredField */
                char *required = ism_config_validate_getAttr("RequiredField", json, i);
                if ( required && !strcasecmp(required, "yes")) {
                    reqdItemList->reqd[found] = 1;
                    char *defval = ism_config_validate_getAttr("Default", json, i);
                    if (!strcmpi(ctype, "String") || !strcmpi(ctype, "StringBig")) {
                        if ( defval ) {
                            reqdItemList->defv[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),defval);
                        } else {
                            reqdItemList->defv[found] = NULL;
                        }
                    } else {
                        if ( defval && *defval != '\0' ) {
                            reqdItemList->defv[found] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),defval);
                        } else {
                            reqdItemList->defv[found] = NULL;
                        }
                    }
                }

                /* check MinOneOption */
                char *minOne = ism_config_validate_getAttr("MinOneOption", json, i);
                if ( minOne && !strcasecmp(minOne, "yes")) {
                    reqdItemList->minoneopt[found] = 1;
                }
                found += 1;
            }

            reqdItemList->total = found;
        }
    }

GET_ITEMLIST_END:
    TRACE(9, "Exit %s: Items found: %d, rc: %d\n", __FUNCTION__, reqdItemList?reqdItemList->total:0, *rc);

    return reqdItemList;
}

/* Free required item list */
XAPI void ism_config_validate_freeRequiredItemList(ism_config_itemValidator_t *item) {
    int k=0;
    if ( !item )
        return;
    if ( item->total == 0 ) {
        ism_common_free(ism_memory_admin_misc,item);
        return;
    }
    for (k=0; k < item->total; k++) {
        if ( item->name[k])    ism_common_free(ism_memory_admin_misc,item->name[k]);
        if ( item->defv[k])    ism_common_free(ism_memory_admin_misc,item->defv[k]);
        if ( item->min[k])     ism_common_free(ism_memory_admin_misc,item->min[k]);
        if ( item->max[k])     ism_common_free(ism_memory_admin_misc,item->max[k]);
        if ( item->options[k]) ism_common_free(ism_memory_admin_misc,item->options[k]);
    }
    ism_common_free(ism_memory_admin_misc,item);
    return;
}


/*
 * Update required item list
 * If the name is on the list, assigned flag will be updated.
 *
 **/
XAPI int ism_config_validate_updateRequiredItemList(ism_config_itemValidator_t *list, char *name )
{
    int i = 0;
    int rc = ISMRC_OK;

    TRACE(9, "Entry %s: name: %s\n",
            __FUNCTION__, name?name:"null" );

    if (!name || !list) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_UPDATE_END;
    }

    if ( list->total < 1)
        goto VAL_UPDATE_END;

    for (i = 0; i < list->total; i++) {
       if ( list->name[i] && !strcmp(list->name[i], name) ) {
           list->assigned[i] = 1;
       }
    }

VAL_UPDATE_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Check The items in required item list.
 * For chkmode == 0 -- creating/updating an config object, check if all items with reqd flag
 *                     have assigned flag marked, except UID
 * For chkmode == 1 -- deleting (or old update) an config object, check if item "name" is provided.
 * For chkmode == 2 -- Allow null value
 *
 */
XAPI int ism_config_validate_checkRequiredItemList(ism_config_itemValidator_t *list, int chkmode)
{
    int i = 0;
    int rc = ISMRC_OK;
    int minOneOpt = 0;
    int hasMinOneOption = 0;

    TRACE(9, "Entry %s: chkmode: %d\n", __FUNCTION__, chkmode );

    if (!list) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    for (i = 0; i < list->total; i++) {
        if (chkmode == 1) {   /*update or delete case*/
            if ( list->name[i] && !strcasecmp(list->name[i], "Name")) {
                if ( list->assigned[i] == 0) {
                    rc = ISMRC_PropertiesNotValid;
                    ism_common_setError(ISMRC_PropertiesNotValid);
                }
                goto VAL_CHECK_END;
            }
        } else {                  /*create*/
            /* check required filed */
            if ( list->reqd[i] == 1 && list->assigned[i] == 0 && strcasecmp(list->name[i], "UID") ) {
                /* check if default value is available */
                if (list->defv[i] == NULL ) {
                    if ( chkmode != 2 ) {
                        rc = ISMRC_PropertyRequired;
                        ism_common_setErrorData(rc, "%s%s", list->name[i], "null");
                        goto VAL_CHECK_END;
                    }
                } else {
                    TRACE(5, "%s: %s will use its default configuration value: %s.\n", __FUNCTION__, list->name[i], list->defv[i]);
                }
            }

            /* check MinOneOptions field */
            if (list->minoneopt[i] == 1) {
                hasMinOneOption += 1;
                if (list->minonevalid[i] == 1)
                    minOneOpt += 1;

            }
        }
    }

    if (hasMinOneOption > 0 && minOneOpt == 0) {
    	char *minList = "Unsupported object";
    	/* Build up a comma separated list of the possible options */
    	if ( !strcmp(list->item, "Endpoint") ) {
    		minList = "TopicPolices,QueuePolicies,SubscriptionPolicies";
    	} else if ( !strcmp(list->item, "ConfigurationPolicy") ) {
    		minList = "ClientAddress,UserID,GroupID,CommonNames";
    	} else if ( !strcmp(list->item, "ConnectionPolicy") ||
    		!strcmp(list->item, "TopicPolicy") ||
    		!strcmp(list->item, "SubscriptionPolicy") ||
    		!strcmp(list->item, "QueuePolicy")) {
    		minList = "ClientID,ClientAddress,UserID,GroupID,CommonNames,Protocol";
    	} else if ( !strcmp(list->item, "ResourceSetDescriptor")) {
    	    minList = "ClientID,Topic";
    	}
    	rc = ISMRC_MinOneOptMissing;
        ism_common_setErrorData(rc, "%s%s", list->item, minList);
        TRACE(3, "%s","Specify one or more of the MinOneOption configuration options.\n");
    }

VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/* Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI int ism_config_validate_CheckItemExist(char *component, char *item, char *name )
{
    int i = 0;
    const char *pName = NULL;
    int rc = ISMRC_NotFound;

    TRACE(9, "Entry %s: component: %s, item: %s, name: %s\n",
            __FUNCTION__, component?component:"null", item?item:"null", name?name:"null" );

    if (!component || !item || !name) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    int comptype = ism_config_getCompType(component);
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        ism_common_setError(ISMRC_InvalidComponent);
        rc = ISMRC_InvalidComponent;
        goto VAL_CHECK_END;
    }

    /* Get properties */
    pthread_mutex_lock(&g_cfglock);
    ism_prop_t *p = ism_config_getProperties(handle, item, name);
    pthread_mutex_unlock(&g_cfglock);

    if ( p ) {
        int len = strlen(item) + strlen(name) + 7;
        char *key = alloca(len);
        sprintf(key, "%s.Name.%s", item, name);
        key[len - 1] = 0;

        for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            if ( strcmp(key, pName) == 0 ) {
                char *value = (char *)ism_common_getStringProperty(p, pName);
                if ( value && *value != '\0' && *value == *name ) {
                     rc = ISMRC_OK;
                     break;
                }
            }
        }
        ism_common_freeProperties(p);
    }

VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/* Starter states for UTF8 */
static int States[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
};


/* Initial byte masks for UTF8 */
static int StateMask[5] = {0, 0, 0x1F, 0x0F, 0x07};


/*
 * Check for valid second byte of UTF-8
 * - copied from utils
 */
static int ism_config_validate_validSecond(int state, int byte1, int byte2, int isName) {
    int ret = 1;

    if (byte2 < 0x80 || byte2 > 0xbf)
        return 0;

    switch (state) {
    case 2:
        if (byte1 < 2)
            ret = 0;
        break;
    case 3:
        if ((byte1== 0 && byte2 < 0xa0) || (byte1 == 13 && byte2 > 0x9f))
            ret = 0;
        break;
    case 4:
        if (byte1 == 0 && byte2 < 0x90)
            ret = 0;
        else if (byte1 == 4 && byte2 > 0x8f)
            ret = 0;
        else if (byte1 > 4)
            ret = 0;
        break;
    }
    return ret;
}

/* Validate char string for UTF8 - used for validating Name and String data types */
XAPI int ism_config_validate_UTF8(const char * s, int len, int checkFirst, int isName)
{
    int  byte1 = 0;
    int  state = 0;
    int  count = 0;
    int  inputsize = 0;
    uint8_t * sp = (uint8_t *)s;
    uint8_t * endp = (uint8_t *)(s+len);
    int firstDone = 0;

    while (sp < endp) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                if (*sp >= 0x80) {
                    firstDone = 1;
                    break;
                }

                if ( isName == 1 ) {
                    if ( checkFirst && !firstDone &&  *sp < 0x41 ) {
                        return -1;
                    }
                    firstDone = 1;
                    if (*sp < ' ' || *sp == '=' || *sp == ',' || *sp == '"' || *sp == '\\' ) {
                        return -1;
                    }
                } else if ( isName == 2 ) {
                    int allow = 0;
                    if ( (*sp > 0x2F && *sp < 0x3A) || (*sp > 0x40 && *sp < 0x5B) || (*sp > 0x60 && *sp < 0x7B) ) {
                        allow = 1;
                    } else {
                        /* allow  -, _, ., and + */
                        if ( *sp == '-' || *sp == '_' || *sp == '.' || *sp == '+') {
                            allow = 1;
                        }else  {
                            allow = 0;
                        }
                    }
                    if ( allow == 0 ) {
                        return 0;
                    }
                } else if ( isName == 3 ) {
                    if ( *sp < 0x20 ) {
                        return 0;
                    }
                } else if ( isName == 4 ) {
                    int allow = 0;
                    if ( (*sp > 0x2F && *sp < 0x3A) || (*sp > 0x40 && *sp < 0x5B) || (*sp > 0x60 && *sp < 0x7B) ) {
                        allow = 1;
                    } else {
                        /* allow  -, _, ., and + */
                        if ( *sp == '#' || *sp == '_' || *sp == '.' || *sp == '+' || *sp == '/' || *sp == '%') {
                            allow = 1;
                        } else  {
                            allow = 0;
                        }
                    }
                    if ( allow == 0 ) {
                        return 0;
                    }
                } else {
                    if (*sp < ' ') {
                        return -1;
                    }
                }

                count++;
                if (++sp >= endp)
                    return count;
            }

            count++;
            state = States[*sp >>3];
            byte1 = *sp & StateMask[state];
            sp++;
            inputsize = 1;
            if (state == 1)
                return -1;
        } else {
            int byte2 = *sp++;
            if ((inputsize==1 && !ism_config_validate_validSecond(state, byte1, byte2, isName)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf)))
                return -1;
            if (inputsize+1 >= state) {
                state = 0;
            } else {
                inputsize++;
            }
        }
    }
    if (state)
        return -1;        /* Incomplete character */
    return count;
}

/* Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI int ism_config_validate_CheckProtocol(char *protocols, int isEndpoint, int capabilities )
{
    int rc = ISMRC_NotFound;
    int icap = 0;
    int found = 0;

    TRACE(9, "Entry %s: protocols: %s, isEndpoint: %d\n",
            __FUNCTION__, protocols?protocols:"null", isEndpoint );

    if (!protocols || *protocols == '\0') {
        rc = ISMRC_OK;
        goto VAL_CHECK_END;
    }

    if(isEndpoint && strcasecmp("all", protocols)==0){
        rc = ISMRC_OK;
        goto VAL_CHECK_END;
    }

    char *opttoken, *optnexttoken = NULL;
    int olen = strlen(protocols);            /* BEAM suppression: passing null object */
    char *opt = (char *)alloca(olen+1);
    memcpy(opt, protocols, olen);
    opt[olen]='\0';

    for (opttoken = strtok_r(opt, ",", &optnexttoken); opttoken != NULL; opttoken = strtok_r(NULL, ",", &optnexttoken)) {
        if ( !opttoken || (opttoken && *opttoken == '\0'))
            continue;

        if (getenv("CUNIT") == NULL){
            icap = ism_admin_getProtocolCapabilities(opttoken);
            if ( capabilities == 0 ) {  //ConnectionPolicy and Endpoint validation
                found = icap;
            } else {    // TopicPolicy validation
                /*If specified protocol is NOT support this capability, return error*/
                found = icap & capabilities;
            }
        } else {
            /*
             * Follow what have done in protocol.c
             * ONLY support JMS,MQTT,JSON4TEST in CUNIT test
             *
             */
            if ( !strcasecmp(opttoken, "JMS")
                    || !strcasecmp(opttoken, "MQTT")
                    || !strcasecmp(opttoken, "JSON4TEST")) {
                found = 1;
            }

        }

        /* return not found */
        if(found==0) {
            TRACE(5, "%s: Cannot find the specified protocol: %s from the protocol list\n", __FUNCTION__, opttoken);
            goto VAL_CHECK_END;
        }

        /* rest found */
        found=0;
    }
    /* validation seems ok*/
    rc = ISMRC_OK;

VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/* Returns ISMRC_OK if the specified Substitution is OK
 *         ISMRC_* for errors.
 */
XAPI int ism_config_validate_PolicySubstitution(char *item, char *name, char *value) {
    int rc = ISMRC_OK;

    TRACE(9, "Entry %s: item: %s, name: %s, value: %s\n",
            __FUNCTION__, item?item:"null", name?name:"null", value?value:"null" );

    if (!item || !name || !value) {
        ism_common_setError(ISMRC_NullPointer);
        rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    if (!strcmp(item, "ConnectionPolicy") ) {
        /* The ClientID filter allows ${UserID}, ${CommonName}.
         * but not ${GroupID} or ${ClientID}*/
        if (!strcmp(name, "ClientID") ) {
            if (strstr(value, "${GroupID}") != NULL || strstr(value, "${ClientID}") != NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
                goto VAL_CHECK_END;
            }

        } else if ( !strcmp(name, "UserID") ||!strcmp(name, "GroupID") || !strcmp(name, "CommonNames") ) {
            /* The UserID, GroupID and CommonNames filter
             * doesn't allows ${UserID}, ${CommonName}, ${GroupID} or ${ClientID}*/
            if (strstr(value, "${ClientID}") != NULL || strstr(value, "${UserID}") != NULL ||
                    strstr(value, "${GroupID}") != NULL || strstr(value, "${CommonName}") != NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
                goto VAL_CHECK_END;
            }
        }
    } else if (!strcmp(item, "TopicPolicy")) {
        /* The ClientID, UserID, GroupID and CommonNames filter
         * doesn't allows ${UserID}, ${CommonName}, ${GroupID} or ${ClientID}*/
        if (!strcmp(name, "ClientID") || !strcmp(name, "UserID") || !strcmp(name, "GroupID") || !strcmp(name, "CommonNames") ) {
            if (strstr(value, "${ClientID}")!= NULL || strstr(value, "${UserID}")!= NULL ||
                    strstr(value, "${GroupID}")!= NULL || strstr(value, "${CommonName}") != NULL) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
                goto VAL_CHECK_END;
            }
        }
    }
VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/* Check * position
 * Only allow one * tailing other valid characters
 * Returns ISMRC_OK if the String meets the rule.
 * ISMRC_* for errors.
 */
XAPI int ism_config_validate_Asterisk(char *name, char *value) {
    int rc = ISMRC_OK;
    int count = 0;
    int i;
    char p;

    TRACE(9, "Entry %s: value: %s\n",
            __FUNCTION__, value?value:"null" );

    if (!name || !value) {
        goto VAL_CHECK_END;
    }

    int len = strlen(value);
    for (i = 0; i<len; i++) {
        p = *(value + i);
        if (p == 0x2A) {
            count++;
            if (i != len -1) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
                goto VAL_CHECK_END;
            }
        }
    }

    if (count > 1) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", name, value);
    }

VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/* Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI ism_prop_t * ism_config_validate_getCurrentConfigProps(char *component, char *item, char *name, int *rc )
{
    *rc = ISMRC_OK;
    ism_prop_t * p = NULL;

    TRACE(9, "Entry %s: component: %s, item: %s, name: %s\n",
            __FUNCTION__, component?component:"null", item?item:"null", name?name:"null" );

    if (!component) {
        ism_common_setError(ISMRC_NullPointer);
        *rc = ISMRC_NullPointer;
        goto VAL_CHECK_END;
    }

    int comptype = ism_config_getCompType(component);
    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle ) {
        ism_common_setError(ISMRC_InvalidComponent);
        *rc = ISMRC_InvalidComponent;
        goto VAL_CHECK_END;
    }

    /* Get properties */
    pthread_mutex_lock(&g_cfglock);
    p = ism_config_getProperties(handle, item, name);
    pthread_mutex_unlock(&g_cfglock);

    if (!p) {
        *rc = ISMRC_NotFound;
    }

VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, *rc);

    return p;
}

/* Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI void * ism_config_validate_getCurrentConfigValue(char *component, char *item, char *name, int *rc )
{
    ism_prop_t *p = NULL;
    void *val = NULL;
    *rc = ISMRC_NotFound;

    TRACE(9, "Entry %s: component: %s, item: %s, name: %s\n",
            __FUNCTION__, component?component:"null", item?item:"null", name?name:"null" );

    p = ism_config_validate_getCurrentConfigProps(component, item, name, rc);
    if (*rc != ISMRC_OK) {
        goto VAL_CHECK_END;
    }

    val = (void *) ism_common_getStringProperty(p, name);
    if (!val) {
        TRACE(5, "%s: failed to find value of the property: %s\n", __FUNCTION__, name);
        *rc = ISMRC_NotFound;
    }


VAL_CHECK_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, *rc);

    return val;
}

XAPI int ism_config_addValueToList(ism_common_list * inList, char * name, int isParam)
{
    int rc=ISMRC_OK;

    if(inList!=NULL && ism_common_list_size(inList) >0){
         ism_common_listIterator iter;
         ism_common_list_iter_init(&iter, inList);

         while (ism_common_list_iter_hasNext(&iter)) {
            ism_common_list_node * node = ism_common_list_iter_next(&iter);
            char * _value = (char *)node->data;
               if(_value!=NULL && !strcmp(_value, name)){

                   rc=ISMRC_PropCreateError;
                   break;
               }
        }
         ism_common_list_iter_destroy(&iter);
         if(rc==ISMRC_PropCreateError){
             if(isParam==1){
                 TRACE(3, "The \"%s\" parameter is duplicated. Parameters can only be used once.", name);
            }else {
                TRACE(3, "The \"%s\" value is duplicated. Values can only be used once.", name);
            }
         }
    }
    ism_common_list_insert_head(inList, (const void *)name);

    return rc;
}

static X509 *
readCertFile(const char * name)
{
    X509 *cert = NULL;
    BIO *in = NULL;
    char *keystore_path = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
			, "KeyStore" );
    char *filename;

    if (!name) {
        TRACE(8, "No Certificate name supplied\n");
        return NULL;
    }

    int len = strlen(name) + strlen(keystore_path) + 2;
    filename = alloca(len);
    memset(filename, '\0', len );
    sprintf(filename, "%s/%s", keystore_path, name);

    in = BIO_new(BIO_s_file());
    if (!in) {
        TRACE(5, "failed to create a BIO object.\n");
        return NULL;
    }
    if ( BIO_read_filename(in, filename) != 1)
    {
        TRACE(5, "failed to read BIO.\n");
        return NULL;
    }
    cert = PEM_read_bio_X509_AUX(in, NULL, NULL, NULL);
    (void) BIO_free_all(in);

    if (cert)
        return cert;
    else
       return NULL;
}

static char * ASN1Time2CTimeStr(ASN1_TIME * time)
{
    struct tm  t;
    char       tmpstr[3];
    time_t     ctime;
    char       ct_str[80];

    memset(&t, 0, sizeof(struct tm));
    memset(tmpstr, '\0', sizeof(tmpstr));
    memset(ct_str, '\0', sizeof(ct_str));

    /* ASN1 time string to struct tm */
    t.tm_zone = 0;
    memcpy(tmpstr, time->data + 10, 2);
    t.tm_sec = atoi(tmpstr);

    memcpy(tmpstr, time->data + 8, 2);
    t.tm_min = atoi(tmpstr);

    memcpy(tmpstr, time->data + 6, 2);
    t.tm_hour = atoi(tmpstr);

    memcpy(tmpstr, time->data + 4, 2);
    t.tm_mday = atoi(tmpstr);

    memcpy(tmpstr, time->data + 2, 2);
    /* Month range in tm structure is 0-11 */
    t.tm_mon = atoi(tmpstr) - 1;

    memcpy(tmpstr, time->data, 2);
    t.tm_year = atoi(tmpstr);

    if (t.tm_year < 70) t.tm_year += 100;

    /* convert to normalize time */
    ctime = timegm(&t);

    /* We store the ExpirationDate in this format: 2023-01-27T21:24.000+00:00 */
    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_UTC);
    ism_common_setTimestamp(ts, ctime * 1000000000);
    ism_common_formatTimestamp(ts, ct_str, sizeof(ct_str), 0, ISM_TFF_ISO8601);
    ism_common_closeTimestamp(ts);

    TRACE(8, "Certificate expirationDate CTime (%ld) is: %s", ctime, ct_str);
    return ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ct_str);
}

/* Returns ISMRC_OK if the cert has an expiration date.
 *         ISMRC_* for errors.
 */
XAPI char * ism_config_getCertExpirationDate(const char * name, int *rc) {
    ASN1_TIME *cert_time;
    char *timestr = NULL;
    X509 *cert;

    *rc = ISMRC_OK;

    cert=readCertFile( name );
    if (cert == NULL ) {
        TRACE(3, "Failed to read certificate file \"%s\".\n", name);
        //DISPLAY_MSG(ERROR,8466, "%s","Failed to read certificate file \"{0}\".", name);
        *rc = ISMRC_ArgNotValid;
        goto VAL_CHECK_END;
    }

    cert_time = X509_get_notAfter(cert);
    timestr = ASN1Time2CTimeStr(cert_time);
    if (timestr) {
        TRACE(9, "%s: Cert ExpirationDate notAfter is: %s\n", __FUNCTION__, timestr);
    } else {
        TRACE(9, "%s: No ExpirationDate notAfter in the cert.\n", __FUNCTION__);
    }

VAL_CHECK_END:
    return timestr;
}

/* return 0 if not exist*/
static int ism_config_validateFileExist( char * filepath)
{
    int rc = 0;

    if (filepath == NULL)
        return rc;

    if( access( filepath, R_OK ) != -1 ) {
        rc = 1;
    }
    return rc;
}

/* return 0 if not exist*/
XAPI int ism_config_validateCertificateFileExist( char * name, int kind)
{
    char fpath[1024];

    memset(fpath, '\0', sizeof(fpath));
    /* keystore*/
    if (kind == 1) {
         snprintf(fpath, sizeof(fpath), "%s/keystore/%s", CERTIFICATE_PATH, name);
    }
    /* MQC*/
    else if (kind == 2) {
        snprintf(fpath, sizeof(fpath), "%s/MQC/%s", CERTIFICATE_PATH, name);
    }
    /* LDAP*/
    else if (kind == 3) {
        snprintf(fpath, sizeof(fpath), "%s/LDAP/%s", CERTIFICATE_PATH, name);
    }
    /* truststore*/
    else if (kind == 4) {
        snprintf(fpath, sizeof(fpath), "%s/truststore/%s", CERTIFICATE_PATH, name);
    }
    /* ltpakeyfile */
    else if (kind == 5) {
        snprintf(fpath, sizeof(fpath), "%s/%s", USERFILES_DIR, name);
    }
    else {
        return 0;
    }
    TRACE(9, "valid certificate path [%s].\n", fpath);
    return ism_config_validateFileExist(fpath);
}

/*Returns ISMRC_OK if the cert and key are match.
 *        ISMRC_* for errors.
 *
 */

XAPI int ismcli_validateKeyCertMatch(char * cert, char *key) {
    int rc = ISMRC_OK;
    char *keystorepath = IMA_SVR_INSTALL_PATH "/certificates/keystore/";

    if (key && cert) {
        int clen = strlen(keystorepath) + strlen(cert) + 1;
        int klen = strlen(keystorepath) + strlen(key) + 1;
        char *certpath = alloca(clen);
        char *keypath  = alloca(klen);
        certpath[clen -1] = '\0';
        keypath[klen -1] = '\0';

        snprintf(certpath, clen, "%s%s", keystorepath, cert);
        snprintf(keypath, klen, "%s%s", keystorepath, key);
        /*
         * call matchkeycert.sh to run openssl utils to validate the key, cert match.
         */
        pid_t pid = fork();
        if (pid < 0){
            rc = ISMRC_Failure;
            TRACE(3, "%s: Failed to fork a process to run \"%s\" script\n", __FUNCTION__, "matchkeycert");
            goto VAL_CHECK_END;
        }
        if (pid == 0){
            execl(IMA_SVR_INSTALL_PATH "/bin/matchkeycert.sh", "matchkeycert.sh", certpath, keypath, NULL);
            int urc = errno;
            TRACE(1, "Unable to run matchkeycert.sh: errno=%d error=%s\n", urc, strerror(urc));
            _exit(1);
        }
        int st;
        waitpid(pid, &st, 0);
        int result = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
        if (result) {
            TRACE(3, "%s: The certificate and key file do not match.\n", __FUNCTION__);
            //DISPLAY_MSG(ERROR,8283, "", "The certificate and key file do not match.");
            rc = ISMRC_CertKeyMisMatch;  /* key and cert does not match */
            ism_common_setErrorData(ISMRC_CertKeyMisMatch, "%s%s", cert, key);
            goto VAL_CHECK_END;
        }
    } else {
        //DISPLAY_MSG(ERROR,8465, "", "You must specify both a Certificate and a Key.");
        TRACE(3, "%s: You must specify both a Certificate and a Key.\n", __FUNCTION__);
    }


VAL_CHECK_END:
    ismCFG_FREE(key);
    ismCFG_FREE(cert);
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 * Validate specific trace related configuration items
 */
extern int ism_common_getTraceComponentID(const char * component);
XAPI int32_t ismcli_validateTraceObject(int ignoreEmptyValue, const char *itemName, const char *value) {
    int32_t rc = ISMRC_OK;

    if ( ignoreEmptyValue == 1 && (value==NULL || *value=='\0'))
        return rc;

    int  level = 0;
    char * lp;
    char * eos;
    char * token;
    char * comptoken;
    char * compvaluetoken;
    int    complevel;
    int    compID;
    char * conntoken;
    char * connvaluetoken;

    if (!value)
        return ISMRC_BadPropertyValue;

    lp = alloca(strlen(value)+1);
    strcpy(lp, value);

    if ( !strcmp(itemName, "TraceLevel") || !strcmp(itemName, "TraceSelected") ) {
        token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
        if (!token) {
        	if ( !strcmp(itemName, "TraceLevel") ) {
        		return ISMRC_BadPropertyValue;
        	}
            return rc;
        }

        /*Check if there comma in the string and nothing after it. Consider bad value*/
        if(strchr(value, ',')!=NULL && (lp ==NULL || *lp=='\0') ){
                return ISMRC_BadPropertyValue;
        }

        level = strtoul(token, &eos, 0);
        if (*eos)
            return ISMRC_BadPropertyValue;

           if ( level < 1 || level > 9 )
            return ISMRC_BadPropertyValue;

           token = (char *)ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
           while (token) {
               comptoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &compvaluetoken);
               if (compvaluetoken && *compvaluetoken) {
                   complevel = strtoul(compvaluetoken, &eos, 0);
                   if (!*eos) {
                       compID = ism_common_getTraceComponentID(comptoken);

                       if (compID >=0 && compID <TRACECOMP_MAX) {
                           if (complevel < 1 || complevel > 9 ) {
                               return ISMRC_BadPropertyValue;
                           }
                       } else {
                           return ISMRC_BadPropertyValue;
                       }
                   }
               } else {
                   return ISMRC_BadPropertyValue;
               }
               token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
           }
    } else if ( !strcmp(itemName, "TraceConnection") ) {
           token = (char *)ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
           while (token) {
               conntoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &connvaluetoken);
               if (connvaluetoken && *connvaluetoken) {
                   if ( strcmpi(conntoken, "endpoint") && strcmpi(conntoken, "clientaddr") && strcmpi(conntoken, "clientid") ) {
                   return ISMRC_BadPropertyValue;
                   }
               } else {
                   return ISMRC_BadPropertyValue;
               }
               token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
           }
    } else if ( !strcmp(itemName, "TraceMax") ) {
    	char *TRACEMAX_MIN = "256000";   //250 * 1024;		   //250K minimum
    	char  *TRACEMAX_MAX = "524288000";    //500 * 1024 * 1024;  //500M maximum
    	if (lp && *lp == '\0') {
    		return ISMRC_BadPropertyValue;
    	}
    	rc = ism_config_validateDataType_bufferSize("TraceMax", lp, TRACEMAX_MIN, TRACEMAX_MAX);
    } else if ( !strcmp(itemName, "TraceOptions")) {
        if (lp && strlen(lp)) {
            token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
            while (token) {
                if (!strcmpi(token, "time") || !strcmpi(token, "thread")
                		|| !strcmpi(token, "where") || !strcmpi(token, "append")) {
                    break;
                } else {
                    TRACE(2, "The trace options are not valid: %s\n", value);
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", "TraceOptions", value);
                }
                token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
            }
        }
    } else if ( !strcmp(itemName, "TraceBackupDestination")) {
    	if (lp && *lp == '\0') {
    		return ISMRC_OK;
    	}

    	rc = ism_config_validateDataType_URL("TraceBackupDestination", lp, "2048", "ftp://,scp://", NULL);
    } else if ( !strcmp(itemName, "TraceModuleLocation")) {
    	// Any string
    } else if ( !strcmp(itemName, "TraceModuleOptions")) {
    	// Any string
	}else {
        rc = ISMRC_BadPropertyName;
    }

    return rc;
}


