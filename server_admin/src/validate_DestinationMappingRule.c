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

#include <validateConfigData.h>
#include "validateInternal.h"
#include <sys/socket.h>
#include <netdb.h>
#include "jansson.h"

extern int ismqmc_validateIPAddress(const char *value, int mode);
extern json_t * ism_config_json_getCompositeItem(const char *object, const char *name, char *item, int getLock);

/* DestinationMappingRules - Queue and Topic validation */
/* Rule types */
#define RULE_TYPE_1_ISMTOPIC_2_MQQUEUE                 1
#define RULE_TYPE_2_ISMTOPIC_2_MQTOPIC                 2
#define RULE_TYPE_3_MQQUEUE_2_ISMTOPIC                 3
#define RULE_TYPE_4_MQTOPIC_2_ISMTOPIC                 4
#define RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE          5
#define RULE_TYPE_6_ISMTOPICSUBTREE_2_MQTOPIC          6
#define RULE_TYPE_7_ISMTOPICSUBTREE_2_MQTOPICSUBTREE   7
#define RULE_TYPE_8_MQTOPICSUBTREE_2_ISMTOPIC          8
#define RULE_TYPE_9_MQTOPICSUBTREE_2_ISMTOPICSUBTREE   9
#define RULE_TYPE_10_ISMQUEUE_2_MQUEUE                 10
#define RULE_TYPE_11_ISMQUEUE_2_MQTOPIC                11
#define RULE_TYPE_12_MQQUEUE_2_ISMQUEUE                12
#define RULE_TYPE_13_MQTOPIC_2_ISMQUEUE                13
#define RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE         14

/*
 * Validate MQ Queue
 * length >= 1 and  <= 48
 * valid chracters: a-z A-Z 0-9  and
 *     backslash(5C), dot(2E), underscore(5F), slash(2F), percent(25)
 * no /+/ or /+
 */
int ismcli_validateMQQueue(const char *queue) {
    int i = 0;
    /* Check for NULL */
    if (!queue)
        return 0;

    /* Check valid UTF8 */
    int len = strlen(queue);
    int count = ism_common_validUTF8(queue, len);

    /* check for length <= 48 */
    if (count < 1 || count > 48)
        return 0;

    /* Check for valid characters */
    int valid = 0;
    for (i=0; i<len; i++) {
        if ((uint8_t)queue[i] == 0x2E || (uint8_t)queue[i] == 0x5F ||
            (uint8_t)queue[i] == 0x2F || (uint8_t)queue[i] == 0x25 ||
            ((uint8_t)queue[i] >= 0x30 && (uint8_t)queue[i] <= 0x39) ||
            ((uint8_t)queue[i] >= 0x41 && (uint8_t)queue[i] <= 0x5A) ||
            ((uint8_t)queue[i] >= 0x61 && (uint8_t)queue[i] <= 0x7A) )
        {
            valid = 1;
        } else {
            valid = 0;
            break;
        }
    }
    if ( valid == 0 )
        return 0;

    /* Do not allow trailing space */
    if (queue[len-1] == ' ')
        return 0;

    /* no /+/ or /+ */
    if ( strstr(queue, "/+/") || strstr(queue, "/+") || strstr(queue, "+/"))
    {
        return 0;
    }

    return 1;
}

/*
 * Validate MQ Topic
 * length >= 1 and  <= 10240
 * no /#/ or /#
 * no /+/ or /+
 */
static int ismcli_validateMQTopic(const char *topic) {
    int i = 0;
    /* Check for NULL */
    if (!topic)
        return 0;

    /* Check valid UTF8 */
    int len = strlen(topic);
    int count = ism_common_validUTF8(topic, len);

    /* check for length <= 10240 */
    if (count < 1 || count > 10240)
        return 0;

    /* Check for control characters */
    for (i=0; i<len; i++) {
        if ((uint8_t)topic[i] < ' ' )
            return 0;
    }

    /* Do not allow trailing space */
    if (topic[len-1] == ' ')
        return 0;

    /* Do not allow single # */
    if ( len == 1 && (*topic == '#' || *topic == '+') )
        return 0;

    /* no /#/ or /# */
    /* no /+/ or /+ */
    if ( strstr(topic, "/#/") ||  strstr(topic, "/+/") )
        return 0;

    if(len==2){
        if ( topic[len-1] == '/' && (topic[len-2] == '#' || topic[len-2] == '+'))
            return 0;


        if ( *topic == '/' && (topic[1] == '#' || topic[1] == '+'))
            return 0;

    }else if ( len > 2){
        /*Do not allow:
        * - Start with #/ or +/
        * - End with /# or /+
        */

        /* End with /# or /+ */
        if ( (topic[len-1] == '#'|| topic[len-1] == '+' ) && (topic[len-2] == '/') )
            return 0;

        /*Start with +/ or #/*/
        if ( (topic[0] == '#' || topic[0] == '+')  && (topic[1] == '/'))
            return 0;
    }

    /* restrict $SYS as root in MQTopic */
    if ( len >= 4 && strncmp(topic, "$SYS", 4) == 0 )
        return 0;

    return 1;
}

///* Starter states for UTF8 */
//static int States[32] = {
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 1,
//};
//
///* Initial byte masks for UTF8 */
//static int StateMask[5] = {0, 0, 0x1F, 0x0F, 0x07};


XAPI int ismcli_validateName(const char *itemName, const char *value) {
    int count;
    int len;

    len = strlen(value);

    count = ism_config_validate_UTF8(value, len, 1, 1);
    if (count<1) {
        TRACE(1, "Name is not a valid.\n");
        return 0;
    }

    if (value[len-1] == ' ')
        return 0;

    if( count > MAX_OBJ_NAME_LEN){
        return 0;
    }
    return 1;
}

/*
 * Validate ISM Queue Name
 */
static int ismcli_validateISMQueue(const char *queue) {
    int match = 1;

    /* Check for NULL */
    if (!queue)
        return 0;

    /* Check valid valid ISM Name */
    match = ismcli_validateName("Queue", queue);
    return match;
}

/*
 * Validate ISM Topic
 * length >= 1 and  <= 65535
 * no /#/ or /#
 * no /+/ or /+
 */
static int ismcli_validateISMTopic(const char *topic) {
    int i = 0;
    /* Check for NULL */
    if (!topic)
        return 0;

    /* Check valid UTF8 */
    int len = strlen(topic);
    int count = ism_common_validUTF8(topic, len);

    /* check for length <= 65535 */
    if (count < 1 || count > 65535)
        return 0;

    /* Check for control characters */
    for (i=0; i<len; i++) {
        if ((uint8_t)topic[i] < ' ' )
            return 0;
    }

    /* Do not allow trailing space */
    if (topic[len-1] == ' ')
        return 0;

    /* Do not allow single # */
    if ( len == 1 && (*topic == '#' || *topic == '+') )
        return 0;

    /* no /#/ or /# */
    /* no /+/ or /+ */
    if ( strstr(topic, "/#/") ||  strstr(topic, "/+/") )
        return 0;

    if(len==2){
        if ( topic[len-1] == '/' && (topic[len-2] == '#' || topic[len-2] == '+'))
            return 0;

        if ( *topic == '/' && (topic[1] == '#' || topic[1] == '+'))
            return 0;
    }else if ( len > 2){
        /*Do not allow:
        * - Start with #/ or +/
        * - End with /# or /+
        */

        /* End with /# or /+ */
        if ( (topic[len-1] == '#'|| topic[len-1] == '+' ) && (topic[len-2] == '/') )
            return 0;

        /*Start with +/ or #/*/
        if ( (topic[0] == '#' || topic[0] == '+')  && (topic[1] == '/'))
            return 0;
    }

    /* restrict $SYS as root in ISMTopic */
    if ( len >= 4 && strncmp(topic, "$SYS", 4) == 0 )
        return 0;

    return 1;
}

/* check destination and topic for different rules */
XAPI int ismcli_validateDMRSourceDest(int type, const char *src, const char *dst) {
    int valid = 1;

    switch (type)
    {
        case RULE_TYPE_1_ISMTOPIC_2_MQQUEUE:
        case RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE:
        {
            if (!ismcli_validateISMTopic(src) || !ismcli_validateMQQueue(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_2_ISMTOPIC_2_MQTOPIC:
        case RULE_TYPE_6_ISMTOPICSUBTREE_2_MQTOPIC:
        case RULE_TYPE_7_ISMTOPICSUBTREE_2_MQTOPICSUBTREE:
        {
            if (!ismcli_validateISMTopic(src) || !ismcli_validateMQTopic(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_3_MQQUEUE_2_ISMTOPIC:
        {
            if (!ismcli_validateMQQueue(src) || !ismcli_validateISMTopic(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_4_MQTOPIC_2_ISMTOPIC:
        case RULE_TYPE_8_MQTOPICSUBTREE_2_ISMTOPIC:
        case RULE_TYPE_9_MQTOPICSUBTREE_2_ISMTOPICSUBTREE:
        {
            if (!ismcli_validateMQTopic(src) || !ismcli_validateISMTopic(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_10_ISMQUEUE_2_MQUEUE:
        {
            if (!ismcli_validateISMQueue(src) || !ismcli_validateMQQueue(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_11_ISMQUEUE_2_MQTOPIC:
        {
            if (!ismcli_validateISMQueue(src) || !ismcli_validateMQTopic(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_12_MQQUEUE_2_ISMQUEUE:
        {
            if (!ismcli_validateMQQueue(src) || !ismcli_validateISMQueue(dst))
                valid = 0;
            break;
        }

        case RULE_TYPE_13_MQTOPIC_2_ISMQUEUE:
        case RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE:
        {
            if (!ismcli_validateMQTopic(src) || !ismcli_validateISMQueue(dst))
                valid = 0;
            break;
        }

        default:
        {
            valid = 0;
            break;
        }
    }

    return valid;
}


XAPI int ismcli_validateRetainedMessageFlag(const char *flag, int ruleType, int noConnections ) {
    if ( flag && strcasecmp(flag, "NONE")) {
        // rules with a queue as the destination are not allowed (1, 5, 10, 12, 13, 14)
        switch (ruleType) {
            case RULE_TYPE_1_ISMTOPIC_2_MQQUEUE:
            case RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE:
            case RULE_TYPE_10_ISMQUEUE_2_MQUEUE:
            case RULE_TYPE_12_MQQUEUE_2_ISMQUEUE:
            case RULE_TYPE_13_MQTOPIC_2_ISMQUEUE:
            case RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE:
                TRACE(9, "Retained message validation failed. Rule %d with queue as the destination is not allowed.\n", ruleType);
                return 1;

            default:
                break;
        }

        // exactly one QueueManagerConnection is required
        if ( noConnections != 1 ) {
            TRACE(9, "Retained message validation failed. Can not be more than one QueueManagerConnection.\n");
            return 2;
        }
    }

    return 0;
}


XAPI int ismcli_validateSubLevel(int ruleType) {
    // rules with a source of mq topic or topic subtree are allowed (4, 8, 9, 13, 14)
    switch (ruleType) {
        case RULE_TYPE_4_MQTOPIC_2_ISMTOPIC:
        case RULE_TYPE_8_MQTOPICSUBTREE_2_ISMTOPIC:
        case RULE_TYPE_9_MQTOPICSUBTREE_2_ISMTOPICSUBTREE:
        case RULE_TYPE_13_MQTOPIC_2_ISMQUEUE:
        case RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE:
            break;
        default:
            TRACE(9, "SubLevel validation failed. Rule %d with non mqtopic as source is not allowed.\n", ruleType);
            return 1;
    }

    return 0;
}

/*
 * Item: DestinationMappingRule
 * Component: MQConnectivity
 *
 * Description:
 *
 * Schema:
 * {
 * 		"DestinationMappingRule": {
 *      	"Name":"string",
 *			"QueueManagerConnection":"string",
 *			"RuleType":"number",
 *			"Source":"string",
 *			"Destination":"string",
 *			"MaxMessages":"number",
 *			"Enabled":"boolean",
 *			"RetainedMessages":"string",
 *			"SubLevel":"number",
 *		}
 *	}
 *
 *
 * QueueManagerConnection: Comma delimited list of Queue Manager Connection objects.
 * RuleType: The mapping rule type.
 * Source: The source of the message. Max 65535 characters.
 * Destination: The destination of the message. Max 65535 characters.
 * MaxMessages: Maximum message count that can be buffered on the destination.
 * Enabled: Specify state of the rule.
 * RetainedMessages: Specify which messages are forwarded to a topic as retained messages.
 *
 * Component callback(s):
 * - MQConnectivity
 * Validation rules:
 *
 * User defined Configuration Item(s):
 * ----------------------------------
 *
 * Internal Configuration Item(s):
 * ------------------------------
 *
 * Unused configuration Item(s):
 * ----------------------------
 *
 * Dependent validation rules:
 * --------------------------
 *
 */
XAPI int32_t ism_config_validate_DestinationMappingRule(json_t *currPostObj, json_t *mergedObj, char * item, char * name, int action, ism_prop_t *props)
{
    int32_t rc = ISMRC_OK;
    ism_config_itemValidator_t * reqList = NULL;
    int compType = -1;
    const char *key;
    json_t *value;
    json_t *currPostInstance = NULL;

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

    /* Validate Name */
    rc = ism_config_validateItemData( reqList, "Name", name, action, props);
    if ( rc != ISMRC_OK ) {
        goto VALIDATION_END;
    }

    /* Validate configuration items */
    int validate = 0;
    int ruleType = 0;
    int oneQMgrConn = 1;
    const char *qMgrConnStr = NULL;
    const char *source = NULL;
    const char *destination = NULL;
    const char *retainedMsgFlag = "None";
    int setEnabledToDefault = 0;


    json_t *currval = NULL;
    int restrictedValUpdated = 0;
    int nonRestrictedValUpdated = 0;
    int newEnabled = 0;
    int subLevelSpecified = 0;

    /* Get currect enabled flag */
    int currEnabled = 0;
	currval = ism_config_json_getCompositeItem("DestinationMappingRule", name, "Enabled", 0);
	if ( currval ) {
		int oType = json_typeof(currval);
		if ( oType != JSON_TRUE && oType != JSON_FALSE && oType != JSON_NULL ) {
			rc = ISMRC_BadPropertyType;
			ism_common_setErrorData(rc, "%s%s%s%s", "DestinationMappingRule", "Enabled", name, ism_config_json_typeString(oType));
			goto VALIDATION_END;
		}
		if ( oType == JSON_TRUE || oType == JSON_NULL ) currEnabled = 1;
	}

	if (action == 1) {
	    currPostInstance = json_object_get(currPostObj, "DestinationMappingRule");
	}

    /* Iterate thru the node */
    void *itemIter = json_object_iter(mergedObj);
    while(itemIter)
    {
        key = json_object_iter_key(itemIter);
        value = json_object_iter_value(itemIter);
        json_type objType = json_typeof(value);

		rc = ism_config_validateItemDataJson( reqList, name, (char *)key, value);
		if (rc != ISMRC_OK) goto VALIDATION_END;

	    /* validate special cases related to this object here*/
		if ( !strcmp(key, "Source") && objType != JSON_NULL ) {
			source = json_string_value(value);
			currval = ism_config_json_getCompositeItem("DestinationMappingRule", name, "Source", 0);
			if ( currval ) {
				int oType = json_typeof(currval);
				if ( oType != JSON_STRING ) {
					rc = ISMRC_BadPropertyType;
					ism_common_setErrorData(rc, "%s%s%s%s", "DestinationMappingRule", "Source", name, ism_config_json_typeString(oType));
					goto VALIDATION_END;
				}
				const char *srcStr = json_string_value(currval);
				if ( srcStr && source && strcmp(source, srcStr)) restrictedValUpdated = 1;
			}
		} else if ( !strcmp(key, "Destination") && objType != JSON_NULL ) {
			destination = json_string_value(value);
			currval = ism_config_json_getCompositeItem("DestinationMappingRule", name, "Destination", 0);
			if ( currval ) {
				int oType = json_typeof(currval);
				if ( oType != JSON_STRING ) {
					rc = ISMRC_BadPropertyType;
					ism_common_setErrorData(rc, "%s%s%s%s", "DestinationMappingRule", "Destination", name, ism_config_json_typeString(oType));
					goto VALIDATION_END;
				}
				const char *destStr = json_string_value(currval);
				if ( destStr && destination && strcmp(destination, destStr)) restrictedValUpdated = 1;
			}
		} else if ( !strcmp(key, "RuleType") && objType == JSON_INTEGER ) {
			ruleType = json_integer_value(value);
			validate = 1;
			currval = ism_config_json_getCompositeItem("DestinationMappingRule", name, "RuleType", 0);
			if ( currval ) {
				int oType = json_typeof(currval);
				if ( oType != JSON_INTEGER ) {
					rc = ISMRC_BadPropertyType;
					ism_common_setErrorData(rc, "%s%s%s%s", "DestinationMappingRule", "RuleType", name, ism_config_json_typeString(oType));
					goto VALIDATION_END;
				}
				int newVal = json_integer_value(currval);
				if ( ruleType != newVal ) restrictedValUpdated = 1;
			}
		} else if ( !strcmp(key, "QueueManagerConnection") && objType != JSON_NULL ) {
			qMgrConnStr = json_string_value(value);
			if ( !qMgrConnStr || '\0' == *qMgrConnStr) {
	    		rc = ISMRC_BadOptionValue;
	    		ism_common_setErrorData(ISMRC_BadOptionValue, "%s%s%s%s"
	   				, "DestinationMappingRule"
					, name
					, "QueueManagerConnection"
					, "\"\"");
	    		goto VALIDATION_END;
			}

			currval = ism_config_json_getCompositeItem("DestinationMappingRule", name, "QueueManagerConnection", 0);
			if ( currval ) {
				int oType = json_typeof(currval);
				if ( oType != JSON_STRING ) {
					rc = ISMRC_BadPropertyType;
					ism_common_setErrorData(rc, "%s%s%s%s", "DestinationMappingRule", "QueueManagerConnection", name, ism_config_json_typeString(oType));
					goto VALIDATION_END;
				}
				const char *qmStr = json_string_value(currval);
				if ( qmStr && qMgrConnStr && strcmp(qMgrConnStr, qmStr)) restrictedValUpdated = 1;
			}

	        if ( strstr(qMgrConnStr, ",")) {
	            oneQMgrConn = 0;
	        }
    		char *token = NULL;
    		char *nexttoken = NULL;
    		char *tmpvalue = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),qMgrConnStr);
    		// Validate the QueueManagerConnection values are valid
    	    for ( token = strtok_r(tmpvalue, ",", &nexttoken); token != NULL;
    	          token = strtok_r(NULL, ",", &nexttoken)) {
    	    	// Make sure there is a QMC named "token"
    	    	value = ism_config_json_getComposite("QueueManagerConnection", token, 0);
    	    	if ( NULL == value ) {
    	    		/* check if QMC is in current POST */
    	    		int found = 0;
    	    		json_t *qmcObj = json_object_get(currPostObj, "QueueManagerConnection");
    	    		if ( qmcObj ) {
    	    			json_t *qmcInst = json_object_get(qmcObj, token);
    	    			if ( qmcInst ) found = 1;
    	    		}
    	    		if ( found == 0 ) {
    	    		    rc = ISMRC_ObjectNotFound;
    	    		    ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "QueueManagerConnection", token);
    	    		    goto VALIDATION_END;
    	    		}
    	    	}
    	    }
		} else if ( !strcmp(key, "RetainedMessages") ) {
			if (objType == JSON_NULL) {
				retainedMsgFlag = "None";
			} else {
				retainedMsgFlag = json_string_value(value);
			}
		} else if ( !strcmp(key, "Enabled") ) {
		    if (objType == JSON_NULL) {
		        setEnabledToDefault = 1;
		        newEnabled = 1;
		    } else if ( objType == JSON_TRUE ) {
		    	newEnabled = 1;
		    }
		} else if ( !strcmp(key, "SubLevel") ) {
		    subLevelSpecified = 1;
		} else if (action == 1){
		    if (currPostInstance) {
		        if (json_object_get(currPostInstance, key)) {
		            nonRestrictedValUpdated++;
		        }
		    }
		}


        itemIter = json_object_iter_next(mergedObj, itemIter);
    }

    if ((currEnabled == 1 && newEnabled == 1) && (restrictedValUpdated == 1 || nonRestrictedValUpdated == 0)) {
    	/* Do not allow DMR configuration if it is enabled or no op config changes */
    	rc = ISMRC_MappingUpdate;
    	ism_common_setError(rc);
    	goto VALIDATION_END;
    }

    int vRC = ismcli_validateRetainedMessageFlag(retainedMsgFlag, ruleType, oneQMgrConn);
    if ( vRC ) {
        if ( vRC == 1 ) {
        	rc = 6211;
        } else {
        	rc = 6212;
        }
        ism_common_setErrorData(rc, "%s", retainedMsgFlag);
        goto VALIDATION_END;
    }

    if (subLevelSpecified)
    {
        vRC = ismcli_validateSubLevel(ruleType);
        if ( vRC ) {
            rc = 6245;
            ism_common_setError(rc);
            goto VALIDATION_END;
        }
    }

    /* Check if required items are specified */
    rc = ism_config_validate_checkRequiredItemList(reqList, 0 );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Add missing default values */
    rc = ism_config_addMissingDefaults(item, mergedObj, reqList);


    if ( validate == 1 ) {
        int valid = ismcli_validateDMRSourceDest(ruleType, source, destination);
        /* display error and return - if not valid */
        if ( valid == 0 ) {
        	rc = 6210;
        	ism_common_setErrorData(6210, "%d", ruleType);
        	goto VALIDATION_END;
        }
    }

    /* validate RetainedMessages */
    vRC = ismcli_validateRetainedMessageFlag(retainedMsgFlag, ruleType, oneQMgrConn );
    if ( vRC != 0 ) {
        if ( vRC == 1 ) {
        	rc = 6211;
        } else {
        	rc = 6212;
        }
        ism_common_setErrorData(rc, "%s", retainedMsgFlag);
        goto VALIDATION_END;
    }

    /* Set Enabled to false if required */
    if ( setEnabledToDefault == 1 ) {
        json_object_set(mergedObj, "Enabled", json_true());
    }

VALIDATION_END:

	TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;

}
