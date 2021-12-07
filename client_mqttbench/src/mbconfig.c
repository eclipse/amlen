/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ismutil.h>
#include <ismjson.h>
#include <dirent.h>
#include <protoex.h>

#include "mbconfig.h"
#include "mqttclient.h"

/* External global variables */
extern uint8_t          g_ClkSrc;
extern mqtt_prop_ctx_t *g_mqttCtx5;
extern int             *g_srcPorts;
extern int 				g_TxBfrSize;
extern int      		g_TotalNumClients;
extern int				g_MostSubsPerClient;
extern char			   *g_predefinedMsgDir;
extern uint8_t 			g_RequestedQuit;
extern pskArray_t 	   *pIDSharedKeyArray;

/* Globals - Initial declaration */
int g_numClientsWithConnTimeout = 0;     /* Total number of clients with a non-zero connection timeout set */
int g_numClientsWithPingInterval = 0;    /* Total number of clients with a non-zero ping interval set */
int g_TotalNumSubscribedTopics = 0;      /* Total number of topics subscribed to. */
int g_LongestClientIDLen = 8;			 /* Keep track of the longest client ID, used for displaying client stats to the screen (min pad length of 8)*/
uint32_t g_TopicID = 1;        			 /* Topic ID, the lower 32 bits of the stream ID (0 is not a valid Topic ID/Stream ID) */
ismHashMap *g_ccertsMap = NULL;          /* Hash Map for the Client Certificates provided via the client list. */
ismHashMap *g_cpkeysMap = NULL;          /* Hash Map for the Client Private Keys provided via the client list. */
ismHashMap *g_dstIPMap 	= NULL;          /* Hostname Hash Map for Destination IP Addresses in the client list. */
ismHashMap *g_srcIPMap  = NULL;          /* Source IP Address Hash Map */

/*
 * mqttbench client list JSON file field names. For field descriptions and how to create
 * a client list file see the Python helper script at clientlists/mqttbenchObjs.py
 */
ism_enumList mbcl_FieldsEnum [] = {
	{ "Fields",						46,								},  // IMPORTANT: you must always update this field, when adding/removing items from enumList
    { "_id",  			 			MB_id,             				},
    { "cleanSession",        		MB_cleanSession, 				},
    { "dst",	           			MB_dst,		    				},
	{ "dstPort",					MB_dstPort,						},
	{ "lingerTimeSecs",				MB_lingerTimeSecs,  			},
	{ "lingerMsgsCount",			MB_lingerMsgsCount,  			},
	{ "password",					MB_password,					},
	{ "publishTopics",				MB_publishTopics,				},
	{ "qos",						MB_qos,							},
	{ "retain",						MB_retain,						},
	{ "topicStr",					MB_topicStr,					},
	{ "reconnectDelayUSecs",		MB_reconnectDelayUSecs,			},
	{ "subscriptions",				MB_subscriptions,				},
	{ "useTLS",						MB_useTLS,						},
	{ "username",					MB_username,					},
	{ "version",					MB_version,						},
	{ "clientCertPath",				MB_clientCertPath,				},
	{ "clientKeyPath",				MB_clientKeyPath,				},
	{ "messageDirPath",				MB_messageDirPath,				},
	{ "lastWillMsg",				MB_lastWillMsg,					},

	// MQTT v5 fields
	{ "cleanStart",					MB_cleanStart,					},
	{ "sessionExpiryIntervalSecs",	MB_sessionExpiryIntervalSecs,	},
	{ "recvMaximum",				MB_recvMaximum,					},
	{ "maxPktSizeBytes",			MB_maxPktSizeBytes,				},
	{ "topicAliasMaxIn",			MB_topicAliasMaxIn,				},
	{ "topicAliasMaxOut",			MB_topicAliasMaxOut,			},
	{ "requestResponseInfo",		MB_requestResponseInfo,			},
	{ "requestProblemInfo",			MB_requestProblemInfo,			},
	{ "userProperties",				MB_userProperties,				},

	// Message Object Fields
	{ "path",						MB_path,						},
	{ "payload",					MB_payload,						},
	{ "payloadSizeBytes",			MB_payloadSizeBytes,			},
	{ "payloadFile",				MB_payloadFile,					},
	{ "topic",						MB_topic,						},
	{ "contentType",				MB_contentType,					},
	{ "expiryIntervalSecs",			MB_expiryIntervalSecs,			},
	{ "responseTopicStr",			MB_responseTopicStr,			},
	{ "correlationData",			MB_correlationData,				},

	// User Properties Fields
	{ "name",						MB_name,						},
	{ "value",						MB_value,						},

	// Topic Fields
	{ "noLocal",					MB_noLocal,						},
	{ "retainAsPublished",			MB_retainAsPublished,			},
	{ "retainHandling",				MB_retainHandling,				},

	{ "connectionTimeoutSecs",		MB_connectionTimeoutSecs,		},
	{ "pingTimeoutSecs",			MB_pingTimeoutSecs,				},
	{ "pingIntervalSecs",			MB_pingIntervalSecs,			},
	// IMPORTANT: you must always update the value of the "Fields" field at the top of this array when adding/removing items from this enumList array, so that it matches
	// the number of fields in the array.  Failure to do so may result in SIGSEGV or INVALID_ENUM.
};

/*
 * The list of valid values that can be specified in the "version" field of the mqttbench client list JSON file
 * and there corresponding MQTT version values.
 *
 * TCP = an MQTT client that connects over TCP or TLS/TCP
 * WS = an MQTT client that connects over WebSockets/TCP or WebSockets/TLS/TCP
 *
 */
ism_enumList mbcl_MQTTVersionsEnum [] = {
	{ "Versions",	6,			},
	{ "TCP3",		MQTT_V3,	},
	{ "WS3",		MQTT_V3,	},
	{ "TCP4",		MQTT_V311,	},
	{ "WS4",		MQTT_V311,	},
	{ "TCP5",		MQTT_V5,	},
	{ "WS5",		MQTT_V5,	},
	// IMPORTANT: you must always update the value of the "Versions" field at the top of this array when adding/removing items from this enumList array, so that it matches
	// the number of fields in the array.  Failure to do say may result in SIGSEGV or INVALID_ENUM.
};

/*
 * Check the JSON entry type is a string type and duplicate/copy the value to the
 * address of the char * pointer dst. This memory must be freed when destroying the
 * mqttclient_t object.
 *
 * @param[in]	ent 		= the JSON entry to be processed as a string object
 * @param[out]  dst			= the destination to duplicate/copy the value of the JSON string
 * @param[out]  len			= length of the string copied to dst
 * @param[in]	maxlen		= optionally provided as an upper bound on string length. If -1, then don't check string length
 * @param[in]   base64      = flag to indicate whether the string value in the JSON entry is base64 encoded and should be decoded
 *
 * @return 0 = OK, otherwise it is a failure
 * */
static int jsonGetString(ism_json_entry_t * ent, char **dst, int64_t *len, int maxlen, int base64) {
	int rc = 0;
	if (ent->objtype == JSON_String && ent->value) {
		int length = strlen(ent->value);
		char *tmp = alloca(length);
		if(base64){
			length = ism_common_fromBase64((char *)ent->value, tmp, length);
			if(length < 0){
				MBTRACE(MBERROR, 1, "The \"%s\" field (value=%s) is expected to be, but is not base64 encoded\n", ent->name, ent->value);
				return RC_BAD_CLIENTLIST;
			}
			*dst = strdup(tmp);  /* BEAM suppression */
		} else {
			*dst = strdup(ent->value);  /* BEAM suppression */
		}
		if (maxlen != -1 && length > maxlen){
			MBTRACE(MBERROR, 1, "\"%s\" field (value=%s) has a string length(%d), which exceeds max length of %d\n", ent->name, *dst, length, maxlen);
			return RC_BAD_CLIENTLIST;
		}
		int count = ism_common_validUTF8(*dst, length);
		if (count < 0) {
			MBTRACE(MBERROR, 1, "The value of the \"%s\" field is not a valid UTF-8 string (value=%s)\n", ent->name, *dst);
			return RC_BAD_UTF8_STRING;
		}
		if(len != NULL)
			*len = length;

	} else {
		MBTRACE(MBERROR, 1, "%s field is not a JSON string OR has a NULL value\n", ent->name);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Check the JSON entry type is an integer type OR number type and has a value that falls within the
 * range between min and max (inclusive).  Copy the value to the address of the int * pointer dst.
 *
 * @param[in]	ent 		= the JSON entry to be processed as an integer or number JSON object
 * @param[out]  dst			= the destination to copy the value of the JSON integer
 * @param[in]   min			= used for range checking, lower acceptable bound(inclusive)
 * @param[in]   max			= used for range checking, upper acceptable bound(inclusive)
 *
 * @return 0 = OK, otherwise it is a failure
 * */
static int jsonGetInteger(ism_json_entry_t * ent, int64_t *dst, int64_t min, int64_t max) {
	int rc = 0;
	int64_t val = 0;
	if (ent->objtype == JSON_Integer) {
		val = ent->count;
	} else if(ent->objtype == JSON_Number) {
		val = strtod(ent->value, NULL);
	} else {
		MBTRACE(MBERROR, 1, "%s field at line %d is not a JSON integer or JSON number (objtype=%d)\n", ent->name, ent->line, ent->objtype);
		return RC_BAD_CLIENTLIST;
	}

	if ((min <= val) && (val <= max)) {
		*dst = val;
	} else {
		MBTRACE(MBERROR, 1, "%s field at line %d is a JSON integer, but has a value(%lld) outside the acceptable range of (min=%lld and max=%lld)\n",
				ent->name, ent->line, (long long int) val, (long long int) min, (long long int ) max);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Check the buffer and ensure that it has enough space to copy bytes of size len into the buffer.
 * Realloc the buffer if necessary, by doubling the size until it can fit len additional bytes
 *
 * @param[in]	buf 		= the buffer object to check for space
 * @param[out]  len			= the additional number bytes needed to copy into the buffer
 *
 * @return 0 = OK, otherwise it is a failure
 * */
static int checkAllocBufSpace(concat_alloc_t *buf, int len){
	int rc = 0;

	if (buf->used + len > buf->len) {
		int newsize = buf->len * 2;
		while (newsize < buf->used + len)
			newsize *= 2;
		if (buf->inheap) {
			char * tmp = realloc(buf->buf, newsize);
			if (tmp)
				buf->buf = tmp;
			else {
				return provideAllocErrorMsg("growing an concat_alloc_t buffer", newsize, __FILE__, __LINE__);
			}

		} else {
			char * tmpbuf = malloc(newsize);
			if (tmpbuf && buf->used)
				memcpy(tmpbuf, buf->buf, buf->used > buf->len ? buf->len : buf->used);
			buf->buf = tmpbuf;
		}
		if (!buf->buf)
			return provideAllocErrorMsg("growing an concat_alloc_t buffer", newsize, __FILE__, __LINE__);;
		buf->inheap = 1;
		buf->len = newsize;
	}

	return rc;
}

/*
 * Parse a JSON file and load contents into memory JSON DOM object
 *
 * The caller of this function must pass the path to an JSON file and an ism_json_parse_t object
 * to store the parsed JSON object.  The caller must allocate the object before calling this function and
 * free it after processing the JSON DOM.
 *
 * @param[in]	path		=	path to the JSON file
 * @param[out]	jsonObj		= 	the object in which to store the parsed JSON object representation of the client list or message file
 *
 * @return 0 on successful completion, or a non-zero value
*/
static int parseJSONFile(const char *path, ism_json_parse_t *jsonObj){
	int rc=0;
	int sizeBytes = 0;
	char *buff;

	rc = readFile(path, &buff, &sizeBytes); // read file contents into buff
	if(rc){
		return rc;
	}

    // allocate max expected number of JSON entry objects that could be found in a client list file
    ism_json_entry_t *ents = calloc(MAX_JSON_ENTS, sizeof(ism_json_entry_t));
    if(ents == NULL){
    	MBTRACE(MBERROR, 1, "Unable to allocate memory for %llu JSON entries to parse the file %s\n", MAX_JSON_ENTS, path);
    	return RC_MEMORY_ALLOC_ERR;
    }
    jsonObj->ent_alloc = MAX_JSON_ENTS;
    jsonObj->ent = ents;
    jsonObj->source = buff;
    jsonObj->src_len = sizeBytes;
    jsonObj->options = JSON_OPTION_COMMENT;   /* Allow C style comments */

    // attempt to parse the file contents as JSON
    rc = ism_json_parse(jsonObj);
    if (rc) {
       	MBTRACE(MBERROR, 1, "The file %s is not valid JSON, (error=%d) at line %d\n", path, rc, jsonObj->line);
    }

    return rc;
}

/*
 * Process the MQTT version field for the client JSON entry in the mqttbench client list file. The version entry
 * encodes both the MQTT protocol version AND the flag to indicate whether to connect over WebSockets or not.
 *
 * @param[in]	ent		= the version JSON string entry
 * @param[out]	client	= the mqttclient_t object to store the version and websocket flag information in
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMQTTVersion(ism_json_entry_t *ent, mqttclient_t *client){
	int rc = 0;
	if (ent->objtype == JSON_String && ent->value) {
		int32_t versionId = ism_common_enumValue(mbcl_MQTTVersionsEnum, ent->value);
		if(versionId == INVALID_ENUM) {
			MBTRACE(MBERROR, 1, "%s is not a valid value for the \"%s\" field at line %d. For a list of valid values see the mqttbenchObjs.py helper script\n", ent->value, ent->name, ent->line);
			rc = RC_BAD_CLIENTLIST;
		} else {
			client->mqttVersion = versionId;
			if(strcasestr(ent->value, "WS"))
				client->useWebSockets = 1;
		}
	} else {
		MBTRACE(MBERROR, 1, "%s field at line %d is not a JSON string OR has a NULL value\n", ent->name, ent->line);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process an MQTT property as a string and store it in the MQTT properties buffer. Also store
 * occurrences of the MQTT property in an array of MQTT property IDs
 *
 * @param[in]	ent				= the JSON entry containing the MQTT property
 * @param[in]	propID			= ID of the MQTT property
 * @param[out]	propsBuf		= buffer to store MQTT properties
 * @param[in]   maxlen          = max string length allowable for the property (inclusive)
 * @param[out]  propIDsArray    = array of Spec-Defined MQTT properties found while processing the current object
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMQTTPropertyString(ism_json_entry_t *ent, mqtt_prop_id_e propID, concat_alloc_t **propsBuf, int maxlen, uint8_t *propIDsArray){
	int rc = 0;

	if (ent->objtype == JSON_String && ent->value) {
		int length = strlen(ent->value);
		if (maxlen != -1 && length > maxlen) {
			MBTRACE(MBERROR, 1,	"%s field (value=%s) has a string length(%d), which exceeds max length of %d\n", ent->name, ent->value, length, maxlen);
			return RC_BAD_CLIENTLIST;
		}

		propIDsArray[propID]++; // record the occurrence of the MQTT property

		if(*propsBuf == NULL){  /* if a property buffer does not exist yet, allocate one, make it big enough that ismutil will most likely not realloc */
			*propsBuf = calloc(1, sizeof(concat_alloc_t));
			if(*propsBuf == NULL){
				return provideAllocErrorMsg("a property buffer (concat_alloc_t)", sizeof(concat_alloc_t), __FILE__, __LINE__);
			}

			(*propsBuf)->buf = calloc(PROPS_BUF_DFLT_SIZE, sizeof(char));
			if ((*propsBuf)->buf == NULL) {
				return provideAllocErrorMsg("a property buffer (char array)", PROPS_BUF_DFLT_SIZE * sizeof(char), __FILE__, __LINE__);
			}
			(*propsBuf)->inheap = 1;
			(*propsBuf)->len = PROPS_BUF_DFLT_SIZE;
		} else {
			rc = checkAllocBufSpace(*propsBuf, strlen(ent->value)); // grow the properties buffer if determined that there is insufficient space available to copy this property
			if(rc)
				return rc;
		}

		rc = ism_common_putMqttPropString(*propsBuf, propID, g_mqttCtx5, ent->value, -1); // insert property into buffer
		if(rc){
			MBTRACE(MBERROR, 1,	"Failed to insert property %s into an MQTT properties buffer\n", ent->name);
			return RC_BAD_CLIENTLIST;
		}
	} else {
		MBTRACE(MBERROR, 1,	"%s field is not a JSON string OR has a NULL value\n", ent->name);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process an MQTT property as an integer and store it in a buffer to be used later by clients.
 *
 * @param[in]	ent				= the JSON entry containing the MQTT property
 * @param[in]	propID			= ID of the MQTT property
 * @param[out]	propsBuf		= buffer to store all the MQTT properties for a given client
 * @param[in]   min             = min value for property (inclusive)
 * @param[in]   max             = max value for property (inclusive)
 * @param[out]  propIDsArray    = array of Spec-Defined MQTT properties found while processing the current object
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMQTTPropertyInteger(ism_json_entry_t *ent, mqtt_prop_id_e propID, concat_alloc_t **propsBuf, int64_t min, int64_t max, uint8_t *propIDsArray){
	int rc = 0;
	int64_t val = 0;

	if (ent) {
		if (ent->objtype == JSON_Integer) {
			val = ent->count;
		} else if(ent->objtype == JSON_Number) {
			val = strtod(ent->value, NULL);
		} else {
			MBTRACE(MBERROR, 1, "%s field at line %d is not a JSON integer or JSON number (objtype=%d)\n", ent->name, ent->line, ent->objtype);
			return RC_BAD_CLIENTLIST;
		}
	} else {
		val = max;  // if JSON entry is NULL, then take value from max
	}

	if (ent && ((val < min) || (val > max))) {
		MBTRACE(MBERROR, 1, "%s field at line %d is a JSON integer, but has a value(%lld) outside the acceptable range of (min=%lld and max=%lld)\n",
				ent->name, ent->line, (long long int) val, (long long int) min, (long long int ) max);
		return RC_BAD_CLIENTLIST;
	}

	propIDsArray[propID]++; // record the occurrence of the MQTT property

	if (*propsBuf == NULL) { /* if a property buffer does not exist yet, allocate one, make it big enough that ismutil will most likely not realloc */
		*propsBuf = calloc(1, sizeof(concat_alloc_t));
		if (*propsBuf == NULL) {
			return provideAllocErrorMsg("a property buffer (concat_alloc_t)", sizeof(concat_alloc_t), __FILE__, __LINE__);
		}

		(*propsBuf)->buf = calloc(PROPS_BUF_DFLT_SIZE, sizeof(char));
		if ((*propsBuf)->buf == NULL) {
			return provideAllocErrorMsg("a property buffer (char array)", PROPS_BUF_DFLT_SIZE * sizeof(char), __FILE__, __LINE__);
		}
		(*propsBuf)->inheap = 1;
		(*propsBuf)->len = PROPS_BUF_DFLT_SIZE;
	} else {
		rc = checkAllocBufSpace(*propsBuf, sizeof(val)); // grow the properties buffer if determined that there is insufficient space available to copy this property
		if (rc)
			return rc;
	}

	rc = ism_common_putMqttPropField(*propsBuf, propID, g_mqttCtx5, val); // insert property into buffer
	if (rc) {
		const char *prop = "";
		if(ent && ent->name){
			prop = ent->name;
		}
		MBTRACE(MBERROR, 1, "Failed to insert property %s into an MQTT properties buffer\n", prop);
		return RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process the JSON list of User-Defined MQTT properties and store them in the concat_alloc_t buffer as
 * well as the array of User-Defined MQTT properties
 *
 * @param[in]	jsonObj				= 	the in-memory JSON object representation of a single User-Defined MQTT property
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[out]	list				= 	the list object to store MQTT User-Defined properties objects in
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMQTTUserDefinedProperty(ism_json_parse_t *jsonObj, int where, ism_common_list *list) {
	int rc = 0;

	ism_json_entry_t * ent = jsonObj->ent + where;
	if (ent->objtype == JSON_Object) {
		int endloc = where + ent->count;	// get the position of the last JSON entry in the UD MQTT property object
		where++;

		mqtt_user_prop_t *prop = calloc(1, sizeof(mqtt_user_prop_t));    /*BEAM suppression: memory leak*/
		if (prop == NULL) {
			return provideAllocErrorMsg("a User-Defined MQTT property object",	sizeof(mqtt_user_prop_t), __FILE__, __LINE__);
		}

		MBTRACE(MBDEBUG, 7, "where=%d name=%s objtype=%d value=%s level=%d count=%d\n", where, ent->name, ent->objtype, ent->value, ent->level, ent->count);
		while (where <= endloc) {	// Process fields in a MQTT User-Defined property JSON object
			rc = 0;
			ent = jsonObj->ent + where;

			int64_t tmpVal = 0;
			int32_t fieldId = ism_common_enumValue(mbcl_FieldsEnum, ent->name);
			switch (fieldId) {
				case MB_name:
					rc = jsonGetString(ent, &prop->name, &tmpVal, MAX_INT16, 0); prop->namelen=tmpVal; break;
				case MB_value:
					rc = jsonGetString(ent, &prop->value, &tmpVal, MAX_INT16, 0); prop->valuelen=tmpVal; break;
			}

			if(rc){
				break;
			}

			// move position in the JSON DOM to the next JSON entry
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}

		if(prop->name != NULL && prop->value != NULL) {
			ism_common_list_insert_tail(list, prop);
		} else {
			free(prop);
			MBTRACE(MBERROR, 1, "Found an invalid MQTT User at line %d, either \"name\" or \"value\" or both are invalid\n", ent->line);
			return RC_BAD_CLIENTLIST;
		}
	} else {
		MBTRACE(MBERROR, 1, "The MQTT User Property at line %d is expected to be, but is not, a JSON object\n", ent->line);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process the JSON list of User-Defined MQTT properties and store them in the concat_alloc_t buffer as
 * well as the array of User-Defined MQTT properties
 *
 * @param[in]	jsonObj				= 	the in-memory JSON object representation of the User-Defined MQTT properties list
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[out]	propsBuf			= 	the buffer in which to store the User-Defined MQTT properties
 * @param[out]	userPropsArray		= 	the array of User-Defined MQTT properties
 * @param[out]	numUserProps		= 	the number of Use-Defined MQTT properties found
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMQTTUserDefinedProperties(ism_json_parse_t *jsonObj, int where, concat_alloc_t **propsBuf, mqtt_user_prop_t **userPropsArray, uint8_t *numUserProps){
	int rc = 0, propCount = 0;

	ism_json_entry_t * ent = jsonObj->ent + where;
	if (ent->objtype == JSON_Array) {
		int endloc = where + ent->count;	// get the position of the last JSON entry in the UD MQTT properties array
		where++;

		/* Allocate a temporary list to store User-Defined properties when processing the JSON array of MQTT User-defined properties.
		 * This lists will later be used to copy the properties to an array */
		ism_common_list *propsList = (ism_common_list *) calloc(1, sizeof(ism_common_list));
		if (propsList) {
			rc = ism_common_list_init(propsList, 0, NULL);
			if(rc) {
				free(propsList);
				MBTRACE(MBERROR, 1, "Failed to initialize the temporary user properties list\n");
				return RC_MEMORY_ALLOC_ERR;
			}
		} else {
			return provideAllocErrorMsg("a temporary user properties list\n", sizeof(ism_common_list), __FILE__, __LINE__);
		}

		while (where <= endloc) {	// Process fields in a JSON array of MQTT User-Defined properties
			rc = 0;
			ent = jsonObj->ent + where;

			rc = processMQTTUserDefinedProperty(jsonObj, where, propsList);
			if(rc){
				MBTRACE(MBERROR, 1, "Unexpected JSON was found inside the MQTT User-Defined properties array at line %d\n", ent->line);
				return rc;
			}

			// move position in the JSON DOM to the next JSON entry
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}

		propCount = ism_common_list_getSize(propsList);
		if(propCount == 0) {
			MBTRACE(MBERROR, 1, "No valid MQTT User-Defined properties were found in the JSON array of MQTT User Properties at line %d\n", ent->line);
			return RC_BAD_CLIENTLIST;
		}

		*numUserProps = propCount;
		userPropsArray = calloc(propCount, sizeof(mqtt_user_prop_t*)); // allocate array of mqtt user-defined properties
			if (userPropsArray == NULL){
				return provideAllocErrorMsg("an array of user-defined MQTT properties", propCount * sizeof(mqtt_user_prop_t*), __FILE__, __LINE__);
		}

		/* Create/Initialize the iterator to go through each client to close the socket. */
		ism_common_listIterator propsListIter;
		ism_common_list_iter_init(&propsListIter, propsList);

		/* Assign pointers from temporary list to array */
		int i = 0;
		ism_common_list_node *currProp;
		while ((currProp = ism_common_list_iter_next(&propsListIter)) != NULL) {
			if (((mqtt_user_prop_t *) currProp->data)) {
				userPropsArray[i] = (mqtt_user_prop_t *) currProp->data;
				i++;
			}
		}

		ism_common_list_destroy(propsList); // don't worry we did not pass a destroy function pointer while initializing this list, so data will remain after destroy

		if (*propsBuf == NULL) { /* if a property buffer does not exist yet, allocate one, make it big enough that ismutil will most likely not realloc */
			*propsBuf = calloc(1, sizeof(concat_alloc_t));
			if (*propsBuf == NULL) {
				free(userPropsArray);
				return provideAllocErrorMsg("a property buffer (concat_alloc_t)", sizeof(concat_alloc_t), __FILE__, __LINE__);
			}

			(*propsBuf)->buf = calloc(PROPS_BUF_DFLT_SIZE, sizeof(char));
			if ((*propsBuf)->buf == NULL) {
				free(userPropsArray);
				free(*propsBuf);
				*propsBuf = NULL;
				return provideAllocErrorMsg("a property buffer (char array)", PROPS_BUF_DFLT_SIZE * sizeof(char), __FILE__, __LINE__);
			}
			(*propsBuf)->inheap = 1;
			(*propsBuf)->len = PROPS_BUF_DFLT_SIZE;
		} else {
			rc = checkAllocBufSpace(*propsBuf, strlen(ent->value)); // grow the properties buffer if determined that there is insufficient space available to copy this property
			if (rc) {
				free(userPropsArray);
				return rc;
			}
		}

		for(int j=0; j < propCount; j++){ /* populate MQTT property buffer with user properties found in the JSON array of user properties */
			if (userPropsArray[j]) {
				/* Insert property into buffer */
				rc = ism_common_putMqttPropNamePair(*propsBuf,
					                                MPI_UserProperty,
												    g_mqttCtx5,
												    userPropsArray[j]->name,
												    userPropsArray[j]->namelen,
												    userPropsArray[j]->value,
												    userPropsArray[j]->valuelen); /* BEAM suppression: operating on NULL */
				if (rc) {
					if ((*propsBuf)->buf)
						free((*propsBuf)->buf);
					if (*propsBuf) {
						free(*propsBuf);
						propsBuf = NULL;
					}
					free(userPropsArray);
					MBTRACE(MBERROR, 1, "Failed to insert name/value User-Defined property %s into an MQTT properties buffer\n", ent->name);
					return RC_BAD_CLIENTLIST;
				}
			}
		}

		free(userPropsArray);
	} else {
		MBTRACE(MBERROR, 1, "The %s field at line %d is expected to be, but is not, a JSON array\n", ent->name, ent->line);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process the Last Will and Testament (LWT) message JSON object from the client list file and
 * create the LWT object for the mqttclient_t object.
 *
 * @param[in]	clientListJSON		= 	the in-memory JSON object representation of the client list file
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[out]	client				= 	the mqttclient_t object to store the LWT message object
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processLastWillMsg(ism_json_parse_t *clientListJSON, int where, mqttclient_t *client){
	int rc = 0;
	uint8_t propIDsArray[MAX_PROPIDS] = {0}; // array of Spec-Defined MQTT properties found while processing this object
	int propCount = 0; 					// number of Spec-Defined MQTT properties found while processing this object

	ism_json_entry_t * ent = clientListJSON->ent + where;
	if (ent->objtype == JSON_Object) {  				// an LWT JSON entry should be of type JSON object
		int endloc = where + ent->count;			    // get the position of the last JSON entry in the LWT JSON object
		where++;

		client->willMsg = (willMessage_t *) calloc(1, sizeof(willMessage_t));
		if (client->willMsg == NULL) {
			MBTRACE(MBERROR, 1, "Unable to allocate memory for the Last Will and Testament (LWT) message structure of the client at line %d\n",	ent->line);
			return RC_MEMORY_ALLOC_ERR;
		}

		while (where <= endloc) {	// Process fields in an LWT message JSON object
			rc = 0;
			ent = clientListJSON->ent + where;
			MBTRACE(MBDEBUG, 7, "where=%d name=%s objtype=%d value=%s level=%d count=%d\n", where, ent->name, ent->objtype, ent->value, ent->level, ent->count);

			int64_t tmpVal = 0;
			int32_t fieldId = ism_common_enumValue(mbcl_FieldsEnum, ent->name);
			switch (fieldId) {
				case MB_topicStr:
					rc = jsonGetString(ent, &client->willMsg->topic, &tmpVal, MQTT_MAX_TOPIC_NAME, 0); client->willMsg->topicLen=tmpVal; break;
				case MB_payload:
					rc = jsonGetString(ent, &client->willMsg->payload, &tmpVal, -1, 0); client->willMsg->payloadLen=tmpVal; break;
				case MB_contentType:
					rc = processMQTTPropertyString(ent, MPI_ContentType, &client->willMsg->propsBuf, -1, propIDsArray); propCount++; break;
				case MB_responseTopicStr:
					rc = processMQTTPropertyString(ent, MPI_ReplyTopic, &client->willMsg->propsBuf, MQTT_MAX_TOPIC_NAME, propIDsArray); propCount++; break;
				case MB_correlationData:
					rc = processMQTTPropertyString(ent, MPI_Correlation, &client->willMsg->propsBuf, -1, propIDsArray); propCount++; break;
				case MB_expiryIntervalSecs:
					rc = processMQTTPropertyInteger(ent, MPI_MsgExpire, &client->willMsg->propsBuf, 0, MAX_UINT32, propIDsArray); propCount++; break;
				case MB_userProperties:
					rc = processMQTTUserDefinedProperties(clientListJSON, where, &client->willMsg->propsBuf, client->willMsg->userPropsArray, &client->willMsg->numUserProps); break;
				case MB_qos:
					rc = jsonGetInteger(ent, &tmpVal, 0, 2); client->willMsg->qos = tmpVal; break;
				case MB_retain:
					client->willMsg->retained = ent->objtype == JSON_True; break;
			}

			if (rc) {
				return rc;
			}

			// move position in the JSON DOM to the next JSON entry
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}

		/* Copy MQTT properties set on topic object from stack memory to heap memory in destTuple */
		if(propCount > 0){
			client->willMsg->numMQTTProps = propCount;
			client->willMsg->propIDsArray = (uint8_t *) calloc(propCount, sizeof(uint8_t));
			if(client->willMsg->propIDsArray == NULL){
				MBTRACE(MBERROR, 1,	"Unable to allocate memory for the MQTT Spec-Defined properties ID array for the Will Message Object for client at line=%d where=%d\n",
						ent->line, where);
				return RC_MEMORY_ALLOC_ERR;
			}
			for (int i=0; i < propCount; i++) {
				if(propIDsArray[i] != 0){
					client->willMsg->propIDsArray[i] = i;
				}
			}
		}

	} else {
		MBTRACE(MBERROR, 1, "The %s field at line %d is expected to be, but is not, a JSON object\n", ent->name, ent->line);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Deep copy a destTuple_t object
 *
 * @param[in]	src		= 	the source dest tuple object (copy from)
 * @param[out]	dst		= 	the destination dest tuple object (copy to)
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int deepCopyTuple(destTuple_t *src, destTuple_t *dst){
	int rc=0, memSize=0;
	if(!src || !dst){
		MBTRACE(MBERROR, 1, "Failed to deep copy dest tuple, src=%p, dst=%p.\n", src, dst);
		return RC_MEMORY_ALLOC_ERR;
	}

	*dst = *src;  // shallow copy

	/* deep copy string pointers and other nested objects */
	if(src->topicName != NULL) { dst->topicName = strdup(src->topicName); memSize+=strlen(src->topicName); }
	if(src->numUserProps != 0){
		dst->userPropsArray = (mqtt_user_prop_t **) calloc(src->numUserProps, sizeof(mqtt_user_prop_t*));
		dst->numUserProps = src->numUserProps;
		for (int i=0; i<src->numUserProps; i++){
			dst->userPropsArray[i]->name = strdup(src->userPropsArray[i]->name);   /* BEAM suppression */
			dst->userPropsArray[i]->namelen = strlen(src->userPropsArray[i]->name);
			dst->userPropsArray[i]->value = strdup(src->userPropsArray[i]->value);   /* BEAM suppression */
			dst->userPropsArray[i]->valuelen = strlen(src->userPropsArray[i]->value);
		}
	}

	if(src->numMQTTProps != 0) {
		dst->propIDsArray = (uint8_t *) calloc(src->numMQTTProps, sizeof(uint8_t));
		dst->numMQTTProps = src->numMQTTProps;
		for (int i=0; i<src->numMQTTProps; i++){
			dst->propIDsArray[i] = src->propIDsArray[i];
		}
	}

	if(src->propsBuf != NULL){
		dst->propsBuf = calloc(1, sizeof(concat_alloc_t));
		dst->propsBuf->buf = calloc(src->propsBuf->len, sizeof(char));
		dst->propsBuf->len = src->propsBuf->len;
		dst->propsBuf->inheap = src->propsBuf->inheap;
	}

	if ( (src->topicName != NULL && dst->topicName == NULL) ||
		 (src->userPropsArray != NULL && dst->userPropsArray == NULL) ||
		 (src->propIDsArray != NULL && dst->propIDsArray == NULL) ) {
		rc = provideAllocErrorMsg("deep copying a destTuple_t object", memSize, __FILE__, __LINE__);
	}

	return rc;
}

/*
 * Process a topic string with the ${COUNT:x-y} variable in it and replicate the destTuple object as
 * many times as are specified by the sequence x to y.  The sequence number is included in
 * the topic name, which is updated in each cloned destTuple object.
 *
 * @param[in/out]	dstTuple		= 	the dest tuple object which describes the publish topic or subscription (update topicName in original destTuple_t object)
 * @param[out]		tmpList			= 	tmpList is the ism_common_list used to temporarily store the list of publish topics or subscriptions
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int replicateTuple(destTuple_t *dstTuple, ism_common_list *tmpList){
	int rc = 0, i=0;
	topicInfo_t *topicInfo = (topicInfo_t *) alloca(sizeof(topicInfo_t));
	char topicStr[1024];

	memset(topicInfo, 0, sizeof(topicInfo_t));
	rc = getTopicVariableInfo(dstTuple->topicName, topicInfo);  // parse the ${COUNT:x-y} variable and record sequence and topic prefix and suffix into topicInfo struct
	if(rc){
		return rc;
	}

	for (i = topicInfo->startNum; i <= topicInfo->endNum; i++) {
		int len = 0;
		destTuple_t *tuple = dstTuple;

		if(i != topicInfo->startNum){ 	// not the original tuple, so clone it
			destTuple_t *clone = calloc(1, sizeof(destTuple_t));
			rc = deepCopyTuple(dstTuple, clone);
			if(rc){
				MBTRACE(MBERROR, 1,"Failed to deep copy (%d) the destTuple_t object for topic string %s\n", i, dstTuple->topicName);
				return rc;
			}
			tuple = clone;
		}

		/* Update topic name with the COUNT variable resolved */
		memset(topicStr, 0, 1024);	// clear string
		sprintf(topicStr,"%s%d%s", topicInfo->topicName_Pfx, i, topicInfo->topicName_Sfx);
		len = strlen(topicStr);
		if(len > MQTT_MAX_TOPIC_NAME){
			MBTRACE(MBERROR, 1,"The replicated topic string %s (len=%lu), is greater than allowed by mqttbench (MQTT_MAX_TOPIC_NAME=%d)\n", topicStr, strlen(topicStr), MQTT_MAX_TOPIC_NAME);
			return RC_BAD_CLIENTLIST;
		} else {
			if (tuple->topicName != NULL) {
				free(tuple->topicName);
			}
			tuple->topicName = strdup(topicStr);  /* BEAM suppression */
			tuple->topicNameLength = len;

			//TODO should we handle ${COUNT:x-y} in the response topic string?  If so, the we should impose requirement that COUNT variable matches in topic str and response topic str
		}

		ism_common_list_insert_tail(tmpList, tuple);
	}

	return rc;
}

/*
 * Process a single topic or subscription JSON entry and create destination tuple object to store the configuration.
 *
 * @param[in]	jsonObj				= 	the in-memory JSON object to process
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[out]	destTuple			= 	the destTuple_t object to store the TX (publish) and RX (subscription) destination object configuration
 * @param[in]	pubOrSub			= 	a flag to indicate the caller context (i.e. calling this function for publish topics or subscriptions)
 * @param[out]	tmpList				= 	a temporary list to store destTuple_t object pointers for publish topics or subscriptions (maybe NULL)
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processTopic(ism_json_parse_t *jsonObj, int where, destTuple_t *dstTuple, int pubOrSub, ism_common_list *tmpList) {
	int rc = 0;
	uint8_t propIDsArray[MAX_PROPIDS] = {0}; // array of Spec-Defined MQTT properties found while processing this object
	int propCount = 0; 					// number of Spec-Defined MQTT properties found while processing this object

	ism_json_entry_t *ent = jsonObj->ent + where;	// get JSON entry at the current position in the JSON object
	if (ent->objtype == JSON_Object) {				// verify that the topic entry is a JSON object
		int endloc = where + ent->count;			// get the position of the last JSON entry in the topic JSON object
		where++;

		MBTRACE(MBDEBUG, 7, "processing %s entry at line=%d where=%d\n", pubOrSub == PUB ? "publish topic" : "subscription", ent->line, where);
		while (where <= endloc) {
			ent = jsonObj->ent + where;// get JSON entry at the current position

			int64_t tmpVal = 0;
			int32_t fieldId = ism_common_enumValue(mbcl_FieldsEnum, ent->name);
			switch (fieldId) {
				case MB_topicStr:
					rc = jsonGetString(ent, &dstTuple->topicName, &tmpVal, MQTT_MAX_TOPIC_NAME, 0); dstTuple->topicNameLength=tmpVal; break;
				case MB_responseTopicStr:
					rc = processMQTTPropertyString(ent, MPI_ReplyTopic, &dstTuple->propsBuf, MQTT_MAX_TOPIC_NAME, propIDsArray); propCount++; break;
				case MB_correlationData:
					rc = processMQTTPropertyString(ent, MPI_Correlation, &dstTuple->propsBuf, -1, propIDsArray); propCount++; break;
				case MB_userProperties:
					rc = processMQTTUserDefinedProperties(jsonObj, where, &dstTuple->propsBuf, dstTuple->userPropsArray, &dstTuple->numUserProps); break;
				case MB_qos:
					rc = jsonGetInteger(ent, &tmpVal, 0, 2); dstTuple->topicQoS=tmpVal; break;
				case MB_retainHandling:
					rc = jsonGetInteger(ent, &tmpVal, 0, 2); dstTuple->retainHandling=tmpVal; break;
				case MB_retain:
					dstTuple->retain = ent->objtype == JSON_True; break;
				case MB_noLocal:
					dstTuple->noLocal = ent->objtype == JSON_True; break;
				case MB_retainAsPublished:
					dstTuple->retainAsPublished = ent->objtype == JSON_True; break;
			}

			// move position in the JSON DOM to the next JSON entry in the topic object
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}

		if (rc)
			return rc; // no point continuing on, since we found a problem on a single field in the topic JSON object

		/* Copy MQTT properties set on topic object from stack memory to heap memory in destTuple */
		if(propCount > 0){
			dstTuple->numMQTTProps = propCount;
			dstTuple->propIDsArray = (uint8_t *) calloc(propCount, sizeof(uint8_t));
			if(dstTuple->propIDsArray == NULL){
				MBTRACE(MBERROR, 1,	"Unable to allocate memory for the MQTT Spec-Defined properties ID array for the %s entry at line=%d where=%d\n",
						pubOrSub == PUB ? "publish topic" : "subscription", ent->line, where);
				return RC_MEMORY_ALLOC_ERR;
			}
			for (int i=0; i < propCount; i++) {
				if(propIDsArray[i] != 0){
					dstTuple->propIDsArray[i] = i;
				}
			}
		}

		/* Validate the dest tuple */
		if (dstTuple->topicName == NULL || dstTuple->topicNameLength == 0) {
			MBTRACE(MBERROR, 1,	"The %s entry at line %d has no \"topicStr\" field or a zero length topic string. This is not a valid topic object.\n",
					pubOrSub == PUB ? "publish topic" : "subscription", ent->line);
			return RC_BAD_CLIENTLIST;
		}

		/* Process substitution variables in topic string (e.g. ${COUNT}) and insert destTuple_t object into the list (if not NULL)
		 * NOTE: this is only from the client list file caller context, not the message file caller context */
		if (tmpList) {
			if (strcasestr(dstTuple->topicName, "${COUNT:")) {
				rc = replicateTuple(dstTuple, tmpList); // insert updated tuple and all its modified clones into the list of dest tuples
				if (rc) {
					MBTRACE(MBERROR, 1,	"Failed to replicate %s entry with COUNT variable at line %d of the client list file.\n",
							pubOrSub == PUB ? "publish topic" : "subscription",	ent->line);
					return rc;
				}
			} else {
				ism_common_list_insert_tail(tmpList, dstTuple); // no ${COUNT:x-y} variable, so insert into the list as-is
			}
		}
	} else {
		MBTRACE(MBERROR, 1, "The %s entry at line %d is expected to be, but is NOT, a JSON object.\n",
				pubOrSub == PUB ? "publish topic" : "subscription", ent->line);
		return RC_BAD_CLIENTLIST;
	}

	if(rc) {
		MBTRACE(MBERROR, 1, "Failed to process the %s entry (line=%d where=%d)\n", pubOrSub == PUB ? "publish topic" : "subscription", ent->line, where);
	}

	return rc;
}

/*
 * Populate the message object payload buffer based on the configuration provided in the mqttbench JSON message configuration file
 *
 * @param[out]	msgObj			= 	the destination message object containing the payload buffer to populate
 * @param[in]	payloadTypes    =	the number of payload configuration options provided in the mqttbench JSON message file
 * @param[in]	msgFileName     =	name of the mqttbench message file to process
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int fillPayload(message_object_t *msgObj, int payloadTypes, const char *msgFileName) {
	int rc = 0;
	uint8_t fill[8] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };

	if(payloadTypes > 1) {
		MBTRACE(MBERROR, 1, "More than one payload option (payloadStr, payloadFilePath, payloadSizeBytes) was set in message file %s. Only one payload option is permitted.\n", msgFileName);
		return RC_BAD_CLIENTLIST;
	}
	if(payloadTypes < 1) {
		MBTRACE(MBERROR, 1, "No payload option (payloadStr, payloadFilePath, or payloadSizeBytes) was set in message file %s. Exactly one payload option MUST be specified.\n", msgFileName);
		return RC_BAD_CLIENTLIST;
	}

	msgObj->payloadBuf = calloc(1, sizeof(concat_alloc_t));
	if (msgObj->payloadBuf == NULL) {
		return provideAllocErrorMsg("a payload buffer (concat_alloc_t)", sizeof(concat_alloc_t), __FILE__, __LINE__);
	}

	if(msgObj->payloadSizeBytes) { // size based payload
		msgObj->payloadBuf->len = msgObj->payloadBuf->used = msgObj->payloadSizeBytes;
		msgObj->payloadBuf->buf = calloc(msgObj->payloadBuf->len, sizeof(char));
		if (msgObj->payloadBuf->buf == NULL && msgObj->payloadBuf->len != 0) {
			return provideAllocErrorMsg("a payload buffer char array", msgObj->payloadBuf->len, __FILE__, __LINE__);
		}
		for (int i=0; i < msgObj->payloadBuf->len; i++){
			msgObj->payloadBuf->buf[i] = fill[i % 8]; // fill message with 0xDEADBEEF
		}
	}

	if(msgObj->payloadStr) { // inline payload in message file
		msgObj->payloadBuf->len = msgObj->payloadBuf->used = strlen(msgObj->payloadStr);
		msgObj->payloadBuf->buf = calloc(msgObj->payloadBuf->len, sizeof(char)); /*BEAM suppression: memory leak*/
		if (msgObj->payloadBuf->buf == NULL && msgObj->payloadBuf->len != 0) {
			return provideAllocErrorMsg("a payload buffer char array", msgObj->payloadBuf->len, __FILE__, __LINE__);
		}
		memcpy(msgObj->payloadBuf->buf, msgObj->payloadStr, msgObj->payloadBuf->len);
	}

	if(msgObj->payloadFilePath) { // payload in a separate payload file
		int bytesRead = 0;
		rc = readFile(msgObj->payloadFilePath, &msgObj->payloadBuf->buf, &bytesRead);
		if(rc){
			MBTRACE(MBERROR, 1, "Failed to read the message payload file %s specified in the message file %s\n", msgObj->payloadFilePath, msgFileName);
			return rc;
		}
		msgObj->payloadBuf->len = msgObj->payloadBuf->used = bytesRead;
	}

	msgObj->payloadBuf->inheap = 1;

	return rc;
}

/*
 * Allocate the initial RX buffer for a client, it will be reallocated later in doData if not large enough
 *
 * @param[in]		client			= 	the client to allocate the initial RX buffer for
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int allocateInitialRxBuffer(mqttclient_t *client) {
	int rc = 0, initialRxPktSize = 0;

	if(client->destRxListCount == 0)
		initialRxPktSize = sizeof(mqttAckMessage); 	// this is a pure producer, so will only receive ACKS
	else
		initialRxPktSize = INITIAL_RX_PKT_SIZE; 	// pick a reasonable initial RX packet size for consumers, partialMsg will be reallocated if a large message is received

	client->partialMsg = ism_allocateByteBuffer(initialRxPktSize);
	if (client->partialMsg == NULL)
    	rc = provideAllocErrorMsg("partial msg buffer", initialRxPktSize, __FILE__, __LINE__);

	return rc;
}

/*
 * Calculate the max TX packet size for a client this will be used to calculate the max TX packet size for all clients
 * which is used to determine the size of the buffers to allocate in the TX buffer pool.
 *
 * @param[in]		client			= 	the client to calculate max TX packet size for
 *
 * @return	max TX packet size in bytes for this client
 */
static int calculateMaxTXPacketSize(mqttclient_t *client,  mqttbenchInfo_t * pMBInfo) {
	int maxTxPktsize = 0, maxPUBsize = 0, maxSUBsize = 0, maxCONsize = 0, maxDISCsize = 0;

	/* Determine max PUBLISH packet size */
	if(client->destTxListCount > 0) { // only need to calculate max PUBLISH packet size if the client is going to publish messages
		int maxTopicLen = 0, maxTopicPropsLen = 0, maxTopicPropsSumLen = 0, maxMsgPropsLen = 0, maxPayloadLen = 0, maxMsgPropsPayloadSumLen = 0;
		int maxLatencySeqNumLen = 0;

		for(int i=0; i < client->destTxListCount; i++){  // iterate through list of publish topics and keep track of max topic name length and topic properties length
			destTuple_t *topic = client->destTxList[i];
			int topicpropslen = topic->propsBuf == NULL ? 0 : topic->propsBuf->used; // update properties length if any MQTT properties were specified in the topic object
			int topiclen = topic->topicNameLength;
			int topicsum = topicpropslen + topiclen;

			if(topicsum > maxTopicPropsSumLen) {   // check the max against the sum of the topic name length AND topic properties
				maxTopicPropsLen = topicpropslen;
				maxTopicLen = topiclen;
				maxTopicPropsSumLen = topicsum;    // update with the larger size
			}
		}

		/* iterate through list of message objects and keep track of max payload length, message properties length, topic name length, and topic properties length */
		for(int i=0; i < client->msgDirObj->numMsgObj; i++ ){
			message_object_t msgObj = client->msgDirObj->msgObjArray[i];
			int payloadlen = msgObj.payloadBuf == NULL ? 0 : msgObj.payloadBuf->used;
			int msgpropslen = msgObj.propsBuf == NULL ? 0 : msgObj.propsBuf->used; // update properties length if any MQTT properties were specified in the message object
			int topiclen = msgObj.topic == NULL ? 0 : ((destTuple_t*)msgObj.topic)->topicNameLength;
			int topicpropslen = (msgObj.topic == NULL || ((destTuple_t*)msgObj.topic)->propsBuf == NULL) ? 0 : ((destTuple_t*)msgObj.topic)->propsBuf->used;

			int msgsum = payloadlen + msgpropslen;
			int topicsum = topiclen + topicpropslen;

			if(msgsum > maxMsgPropsPayloadSumLen) {	// check the max against the sum of the topic name length AND topic properties
				maxMsgPropsLen = msgpropslen;
				maxPayloadLen = payloadlen;
				maxMsgPropsPayloadSumLen = msgsum;  // update with the larger size
			}
			if(topicsum > maxTopicPropsSumLen){
				maxTopicPropsLen = topicpropslen;
				maxTopicLen = topiclen;
				maxTopicPropsSumLen = topicsum;     // update with the larger size
			}
		}

		/* ----------------------------------------------------------------------------
		 * Check if performing Latency and/or Sequence Number.  If so then need to
		 * add 64 bytes for each one since it requires a User Property.
		 * ---------------------------------------------------------------------------- */
		if ((pMBInfo->mbCmdLineArgs->latencyMask & CHKTIMERTT) > 0)
			maxLatencySeqNumLen = 32;	// more than adequate to hold 16 byte hex string representation of uint64_t timestamp + "$TS" name field and length fields
		if ((pMBInfo->mbCmdLineArgs->chkMsgMask & CHKMSGSEQNUM) > 0) {
			maxLatencySeqNumLen += 64;	// more than adequate to hold
										//   - 16 byte hex string representation of uint64_t seqnum + "@SN" name field and length fields
										//   - 16 byte hex string representation of uint64_t streamid + "@SID" name field and length fields
		}
		maxPUBsize = MQTT_PUB_MSG_LEN(maxTopicLen, maxTopicPropsLen + maxMsgPropsLen, maxPayloadLen +
				     maxLatencySeqNumLen);
		MBTRACE(MBDEBUG, 8, "Max PUB packet size = %d\n", maxPUBsize);

		if (maxPUBsize > pMBInfo->mbSysEnvSet->sendBufferSize) {
			MBTRACE(MBINFO, 1, "Resizing SendBufferSize from %d bytes to %d bytes, to accommodate large messages\n",
								pMBInfo->mbSysEnvSet->sendBufferSize, maxPUBsize);
			pMBInfo->mbSysEnvSet->sendBufferSize = maxPUBsize;
		}
	}

	/* Determine max SUBSCRIBE packet size */
	if(client->destRxListCount > 0){
		int multi_subPropsLen = 0, multi_subTopicLen = 0;
		int subsPerSUBPkt = MIN(client->destRxListCount, MQTT_MAX_TOPICS_PER_SUBSCRIBE);
		for(int i=0; i < client->destRxListCount; i++) {
			destTuple_t *sub = client->destRxList[i];
			int subpropslen = sub->propsBuf == NULL ? 0 : sub->propsBuf->used; // update properties length if any MQTT properties were specified in the topic object
			int subtopiclen = sub->topicNameLength + MQTT_TOPIC_NAME_HDR_LEN + SUBOPTIONS_LEN;

			multi_subPropsLen += subpropslen; // add properties for multiple subscription into the same SUBSCRIBE packet, if specified (although this is a little weird)
			multi_subTopicLen += subtopiclen;

			if(i % subsPerSUBPkt == 0) {  	// reached our limit of subs per SUBSCRIBE packet, figure out the total
				maxSUBsize = MAX(maxSUBsize, MQTT_SUB_MSG_LEN(multi_subTopicLen, multi_subPropsLen));
				MBTRACE(MBDEBUG, 8, "Max SUB packet size = %d, multi_subPropsLen=%d, multi_subTopicLen=%d\n", maxSUBsize, multi_subPropsLen, multi_subTopicLen);
				multi_subPropsLen = 0;
				multi_subTopicLen = 0;
			} else {
				maxSUBsize = MAX(maxSUBsize, MQTT_SUB_MSG_LEN(multi_subTopicLen, multi_subPropsLen));
				MBTRACE(MBDEBUG, 8, "Max SUB packet size = %d, multi_subPropsLen=%d, multi_subTopicLen=%d\n", maxSUBsize, multi_subPropsLen, multi_subTopicLen);
			}
		}
	}

	/* Determine max CONNECT packet size */
	int propsBufLen = client->propsBuf == NULL ? 0 : client->propsBuf->used;
	int willMsgLen = 0;
	int varHdrLen = 0;

	switch(client->mqttVersion){
		case MQTT_V5:
			varHdrLen = MQTT_CONNECT_V5_VAR_HDR_LEN; break;
		case MQTT_V311:
			varHdrLen = MQTT_CONNECT_V311_VAR_HDR_LEN; break;
		case MQTT_V3:
			varHdrLen = MQTT_CONNECT_V31_VAR_HDR_LEN; break;
	}

	if(client->willMsg) {
		willMsgLen = MQTT_WILL_MSG_LEN(client->willMsg->topicLen, client->willMsg->payloadLen, client->willMsg->propsBuf == NULL ? 0 : client->willMsg->propsBuf->used);
	}

	maxCONsize = MQTT_CONNECT_MSG_LEN(varHdrLen, willMsgLen, propsBufLen, client->clientIDLen, client->usernameLen, client->passwordLen);


	/* Determine max DISCONNECT packet size */
	maxDISCsize = MQTT_DISCONNECT_MSG_LEN(client->mqttVersion);

	/* calculate the max TX packet size for PUBLISH, SUBSCRIBE, CONNECT, and DISCONNECT packets */
	maxTxPktsize = MAX(MAX(MAX(maxPUBsize, maxSUBsize), maxCONsize), maxDISCsize);

	return maxTxPktsize + client->useWebSockets * MAX_WS_FRAME_LEN; // don't forget to add length of websockets frame if websockets is used
}

/* *************************************************************************************
 *  Check to see if the Client Certificate or the Client Private key exists
 *  in the corresponding HashMap.  If it doesn't  exist then perform the
 *  PEM_Read...() call for either a x509 Certificate or an EVP_PKEY, and store it into
 *  the corresponding HashMap.
 *
 *   @param[in]   hashEntryKey       = the file path of the client certificate or private key
 *   @param[in]   certOrKey		     = a flag to indicate whether file is a certificate or private key
 *   @param[in]	  line				 = the line number in the client list file where this client is specified
 *
 *   @return 0 on successful completion, or a non-zero value
 * *************************************************************************************/
static int processClientCertOrKey(char *hashEntryKey, int certOrKey, int line) {
	int rc = 0;
	FILE *fp;
	X509 *clientCert = NULL;
	EVP_PKEY *clientKey = NULL;

	switch (certOrKey) {
	case CLIENT_CERTIFICATE:
		clientCert = (X509 *) ism_common_getHashMapElement(g_ccertsMap, hashEntryKey, 0);
		if (clientCert == NULL) {
			fp = fopen(hashEntryKey, "rb");
			if (fp) {
				fclose(fp);
				X509 *x509_ClientCert;
				BIO *bio_cert = BIO_new_file(hashEntryKey, "rb");
				x509_ClientCert = PEM_read_bio_X509(bio_cert, NULL, 0, NULL);
				BIO_free(bio_cert);
				if (x509_ClientCert) {
					rc = ism_common_putHashMapElement(g_ccertsMap, hashEntryKey, 0, (void *) x509_ClientCert, NULL);
					if (rc) {
						MBTRACE(MBERROR, 1, "Unable to add client certificate %s for client at line %d to the client certificate hashmap\n", hashEntryKey, line);
						rc = RC_UNABLE_TO_ADD_TO_HASHMAP;
					}
				}
				else {
					MBTRACE(MBERROR, 1, "Failed to read x509 certificate, %s (from client at line %d) is not a well formed x509 certificate\n", hashEntryKey, line);
					rc = RC_FILEIO_ERR;
				}
			} else {
				MBTRACE(MBERROR, 1, "Unable to open the client certificate file %s, from client at line %d\n", hashEntryKey, line);
				rc = RC_FILEIO_ERR;
			}
		}

		break;

	case CLIENT_PRIVATE_KEY:
		clientKey = (EVP_PKEY *) ism_common_getHashMapElement(g_cpkeysMap, hashEntryKey, 0);
		if (clientKey == NULL) {
			fp = fopen(hashEntryKey, "rb");
			if (fp) {
				fclose(fp);
				EVP_PKEY *evpPKey = NULL;
				BIO *bio_key = BIO_new_file(hashEntryKey, "rb");
				evpPKey = PEM_read_bio_PrivateKey(bio_key, NULL, 0, NULL);
				BIO_free(bio_key);
				if (evpPKey) {
					rc = ism_common_putHashMapElement(g_cpkeysMap, hashEntryKey, 0, (void *) evpPKey, NULL);
					if (rc) {
						MBTRACE(MBERROR, 1, "Unable to add client private key %s for client at line %d to the client private key hashmap\n", hashEntryKey, line);
						rc = RC_UNABLE_TO_ADD_TO_HASHMAP;
					}
				} else {
					MBTRACE(MBERROR, 1, "Failed to read EVP_PKEY private key file, %s (from client at line %d) is not a well formed EVP_PKEY file.\n", hashEntryKey, line);
					rc = RC_FILEIO_ERR;
				}
			} else {
				MBTRACE(MBERROR, 1, "Unable to open the client private key file %s, from client at line %d\n", hashEntryKey, line);
				rc = RC_FILEIO_ERR;
			}
		}

		break;
	}

	return rc;
}

/*
 * Process a mqttbench message file and store the configuration in the message object passed into this function
 *
 * @param[out]		msgObj			= 	the destination message object to receive the configuration from the message file
 * @param[in]		mqttbenchInfo   =	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]		msgFileName     =	name of the mqttbench message file to process
 * @param[out]      msgDirObj       =   the message directory object to which this message object belongs to
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processMessageFile(message_object_t *msgObj, ism_json_parse_t *msgFileJSONObj, const char *msgFileName, messagedir_object_t *msgDirObj) {
	int rc=0, where=0, payloadTypes=0;
	uint8_t propIDsArray[MAX_PROPIDS] = {0};  // array of Spec-Defined MQTT properties found while processing this object
	int propCount = 0; 					// number of Spec-Defined MQTT properties found while processing this object

	ism_json_entry_t * ent = msgFileJSONObj->ent + where;
	if (ent->objtype == JSON_Object) { 		// a message file JSON entry should be of type JSON object
		int endloc = where + ent->count;	// get the position of the last JSON entry in the client object
		where++;

		while (where <= endloc) {	// Process fields in an message file JSON object
			rc = 0;
			ent = msgFileJSONObj->ent + where;
			destTuple_t *dstTuple;
			MBTRACE(MBDEBUG, 7, "where=%d name=%s objtype=%d value=%s level=%d count=%d\n",	where, ent->name, ent->objtype, ent->value, ent->level, ent->count);

			int64_t tmpVal = 0;
			int32_t fieldId = ism_common_enumValue(mbcl_FieldsEnum, ent->name);
			switch (fieldId) {
				case MB_payload:
					rc = jsonGetString(ent, &msgObj->payloadStr, NULL, -1, 0); payloadTypes++; break;
				case MB_payloadFile:
					rc = jsonGetString(ent, &msgObj->payloadFilePath, NULL, -1, 0); payloadTypes++; break;
				case MB_payloadSizeBytes:
					rc = jsonGetInteger(ent, &tmpVal, 1, 0x7FFFFFFF); msgObj->payloadSizeBytes = tmpVal; payloadTypes++; break;
				case MB_contentType:
					rc = processMQTTPropertyString(ent, MPI_ContentType, &msgObj->propsBuf, MAX_UINT16, propIDsArray); propCount++;
					if(rc == 0) { // we have to set the payload format indicator to 0x1, when a content type is specified
						rc = processMQTTPropertyInteger(NULL, MPI_PayloadFormat, &msgObj->propsBuf, 0, 1, propIDsArray); propCount++;
					}
					break;
				case MB_expiryIntervalSecs:
					rc = processMQTTPropertyInteger(ent, MPI_MsgExpire, &msgObj->propsBuf, 0, MAX_UINT32, propIDsArray); propCount++; break;
				case MB_userProperties:
					rc = processMQTTUserDefinedProperties(msgFileJSONObj, where, &msgObj->propsBuf, msgObj->userPropsArray, &msgObj->numUserProps); break;
				case MB_topic:
					dstTuple = (destTuple_t *) calloc(1, sizeof(destTuple_t)); // allocate a dest tuple object to store the properties for this publish topic
					if (dstTuple == NULL) {
						MBTRACE(MBERROR, 1,	"Unable to allocate memory for a publish topic object at line %d in message JSON file %s\n", ent->line, msgFileName);
						return RC_MEMORY_ALLOC_ERR;
					}
					rc = processTopic(msgFileJSONObj, where, dstTuple, PUB, NULL);
					if(rc == 0) {
						msgObj->topic = (destTuple *) dstTuple;    		// have to cast to account of typedef redefinition
						((destTuple_t *) msgObj->topic)->fromMD = 1;    // indicate that destTuple is from a message file
					} else {
						free(dstTuple);
					}
					break;
			}

			if (rc) {
				MBTRACE(MBERROR, 1, "An error occured will processing the %s entry at line %d of the message JSON file %s\n", ent->name, ent->line, msgFileName);
				return rc; // no point in validating the client, since we found a problem on a single field in the client JSON object
			}

			// move position in the JSON DOM to the next JSON entry
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}

		/* Copy MQTT properties set from stack memory to heap memory in the message object */
		if(propCount > 0){
			msgObj->numMQTTProps = propCount;
			msgObj->propIDsArray = (uint8_t *) calloc(propCount, sizeof(uint8_t));
			if(msgObj->propIDsArray == NULL){
				MBTRACE(MBERROR, 1,	"Unable to allocate memory for the MQTT Spec-Defined properties ID array in message JSON file %s\n", msgFileName);
				return RC_MEMORY_ALLOC_ERR;
			}
			for (int i=0; i < propCount; i++) {
				if(propIDsArray[i] != 0){
					msgObj->propIDsArray[i] = i;
				}
			}
		}

		msgObj->msgDirObj = msgDirObj;						 // set reference to message directory object
		msgObj->filePath = strdup(msgFileName); 			 // copy file path into the message object
		rc = fillPayload(msgObj, payloadTypes, msgFileName); // populate the message object payload buffer
		if(rc)
			return rc;

	} else {
		MBTRACE(MBERROR, 1, "The first entry in the message file %s is expected to be, but is not, a JSON object\n", msgFileName);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/* *************************************************************************************
 * msgFileFilter
 *
 * Description:  Check that the file entry contains the substring ".json". This is a
 * callback which is invoked from the scandir() call.
 *
 *   @param[in]  entry              = a directory entry to be checked if it should be filtered
 *
 * @return 0          = reject this file
 *         NONZERO    = accept this file
 * *************************************************************************************/
int msgFileFilter (const struct dirent *entry)
{
	char *jsonExt = strstr(entry->d_name, ".json");
	return jsonExt != NULL;
} /* msgFileFilter */

/*
 * Process the message files in the specified directory.  This function will walk through
 * all the JSON files it finds in the specified directory in lexicographic order, parse them as JSON, and
 * process them as mqttbench message files.  The contents of the message files will be stored as message objects
 * and stored in the message directory object which is in then stored in the message directory HashMap.
 * The HashMap will later be used to determine the messages published by each client.
 *
 * @param[in]	msgDirPath		= 	the message directory to process
 * @param[out]	mqttbenchInfo   =	mqttbench user input object (command line params, env vars, etc.)
 *
 * @return	0 = OK, otherwise it is a failure
 */
int processMessageDir(const char *msgDirPath, mqttbenchInfo_t *mqttbenchInfo) {
	int rc=0, numFiles=0;
	ismHashMap *msgDirMap = mqttbenchInfo->mbMsgCfgInfo->msgDirMap;
	struct dirent **direntries;

	messagedir_object_t *msgDirObj = ism_common_getHashMapElement(msgDirMap, msgDirPath, 0);
	if(msgDirObj){
		return rc;
	}

	msgDirObj = calloc(1, sizeof(messagedir_object_t)); /* BEAM suppression: memory leak */
	if(msgDirObj == NULL){
		rc = provideAllocErrorMsg("message directory object", sizeof(messagedir_object_t), __FILE__, __LINE__);
		return rc;
	}

	/* Scan through message dir path and process ALL files contained within as mqttbench message files */
	numFiles = scandir(msgDirPath, &direntries, msgFileFilter, alphasort);
	if(numFiles == 0) {
		MBTRACE(MBERROR, 1, "No mqttbench message JSON files were found in message directory %s\n", msgDirPath);
		return RC_BAD_MSGDIR;
	}
	msgDirObj->numMsgObj = numFiles;
	msgDirObj->msgObjArray = (message_object_t *) calloc(numFiles, sizeof(message_object_t));
	if(msgDirObj->msgObjArray == NULL){
		rc = provideAllocErrorMsg("array of message objects", numFiles * sizeof(message_object_t), __FILE__, __LINE__);
		free(msgDirObj);   /* Free up the msgDirObj */
		return rc;
	}

	for (int i = 0 ; i < numFiles ; i++ ) {
		char filePath[MAX_CHAR_STRING_LEN] = {0};				// path of the message file
		int msgDirPathLen = strlen(msgDirPath);
		memcpy(filePath, msgDirPath, msgDirPathLen);
		if(msgDirPath[msgDirPathLen - 1] != '/') {
			strcat(filePath, "/");
		}
		strcat(filePath, direntries[i]->d_name);

		ism_json_parse_t *msgFileJSONObj = calloc(1, sizeof(ism_json_parse_t));
		rc = parseJSONFile(filePath, msgFileJSONObj);
		if (rc) {
			free(msgDirObj->msgObjArray);
			free(msgDirObj);
			MBTRACE(MBERROR, 1, "Failed to parse mqttbench message file %s as JSON\n", filePath);
			return rc;
		}

		rc = processMessageFile(&msgDirObj->msgObjArray[i], msgFileJSONObj, filePath, msgDirObj); // store data from processed file into message obj array
		if (rc) {
			free(msgDirObj->msgObjArray);
			free(msgDirObj);
			MBTRACE(MBERROR, 1, "Failed to process mqttbench message file %s\n", filePath);
			return rc;
		}

		// we don't need the JSON object anymore
		free(msgFileJSONObj->source);
		free(msgFileJSONObj->ent);
		free(msgFileJSONObj);
		free(direntries[i]);
	}

	if (direntries)
		free(direntries);

	rc = ism_common_putHashMapElement(msgDirMap, msgDirPath, 0, msgDirObj, NULL); // store the newly created message dir object in the hashmap
	if(rc){
		free(msgDirObj);
		MBTRACE(MBERROR, 1, "Failed to put the message directory object into the message directory hashmap\n");
		return RC_MEMORY_ALLOC_ERR;
	}

	return rc;
}

/*
 * Assign the destination tuple pointers in tupleList into the respective TX or RX destination tuple
 * arrays in the client object. Destroy the list after copying the destination tuple pointers.
 *
 * NOTE: since the tupleList was initialized w/o a destroy function pointer the data pointers
 * will remain intact after the list is destroyed.
 *
 * @param[in]		tupleList	   = 	the source dest tuple object list (copy from)
 * @param[out]		client		   = 	the destination client object holding the TX and RX tuple pointer arrays (copy to)
 * @param[in]		pubOrSub	   = 	flag to indicate whether to copy the dest tuple list to the client TX or RX tuple array
 * @param[in]		line		   = 	line number in the client list file from whence this client is declared
 * @param[in]       mqttbenchInfo  =    mqttbench user input object (command line params, env vars, etc.)
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int tupleListToArray(ism_common_list *tupleList, mqttclient_t *client, int pubOrSub, int line, mqttbenchInfo_t * mqttbenchInfo){
	int rc=0;
	int tupleCount = ism_common_list_getSize(tupleList);

	if (tupleCount > 0) {
		ism_common_list_node *currTuple;
		destTuple_t **dstArray = calloc(tupleCount, sizeof(destTuple_t *)); // allocate array of destTuple_t pointers
		if (dstArray == NULL) {
			rc = provideAllocErrorMsg("dest tuple pointer array ", (int) tupleCount * sizeof(destTuple_t *), __FILE__, __LINE__);
			return rc;
		}

		/* Create/Initialize the iterator to go through each client to close the socket. */
		ism_common_listIterator tupleListIter;
		ism_common_list_iter_init(&tupleListIter, tupleList);

		if (pubOrSub == PUB) {
			client->clientType |= PRODUCER;				// mark the client a producer
			client->destTxListCount = tupleCount;
			client->destTxList = dstArray;
		} else {
			client->clientType |= CONSUMER;				// mark the client a consumer
			client->destRxListCount = tupleCount;
			client->destRxList = dstArray;
			g_TotalNumSubscribedTopics += tupleCount; 	// update global variable of the total number of subscriptions configured across all clients
			if(tupleCount > g_MostSubsPerClient)
				g_MostSubsPerClient = tupleCount; 		// keep track of the most subs/client in the client list file

			/* MessageSight imposes a 500 subscription limit per client, check this before starting the client */
			if (tupleCount > MQTT_MAX_SUBSCRIPTIONS_PER_CLIENT) {
				MBTRACE(MBERROR, 1, "Client at line %d exceeded the max # of subscriptions (%d) per client allowed by MessageSight.\n", line, MQTT_MAX_SUBSCRIPTIONS_PER_CLIENT);
				return RC_EXCEEDED_MAX_VALUE;
			}

			if(client->mqttVersion >= MQTT_V5) {
				client->destRxSubIDMap = ism_common_createHashMap(2 * client->destRxListCount, HASH_INT32); // hash on subscription ID
				if(client->destRxSubIDMap == NULL){
					return provideAllocErrorMsg("an ismHashMap", (int) sizeof(client->destRxSubIDMap), __FILE__, __LINE__);
				}
			} else {
				if(!client->usesWildCardSubs){ // do not create the RX subscription map if this client has even one wildcard subscription
					client->destRxSubMap = ism_common_createHashMap(2 * client->destRxListCount, HASH_STRING); // hash on topic name
					if(client->destRxSubMap == NULL){
						return provideAllocErrorMsg("an ismHashMap", (int) sizeof(client->destRxSubMap), __FILE__, __LINE__);
					}
				}
			}
		}

		/* For publish topics we need to first count up the number of destTuple from message files and from the client list file */
		if (pubOrSub == PUB) {
			int numTopicFromCL = 0;
			int numTopicFromMD = 0;
			while ((currTuple = ism_common_list_iter_next(&tupleListIter)) != NULL) {
				if (((destTuple_t *) currTuple->data)) {
					destTuple_t *topic = (destTuple_t *) currTuple->data;
					if(topic->fromMD){
						numTopicFromMD++;
					} else {
						numTopicFromCL++;
					}
				}
			}
			ism_common_list_iter_reset(&tupleListIter); // reset iterator back to the beginning of the list, for the next loop

			client->destTxFromMDCount = numTopicFromMD;
			client->destTxFromCLCount = numTopicFromCL;
			client->destTxFromMD = calloc(client->destTxFromMDCount, sizeof(int)); /* allocate array of indexes into the destTxList array of publish topics
																					  (those topics which came from message objects) */
			client->destTxFromCL = calloc(client->destTxFromCLCount, sizeof(int)); /* allocate array of indexes into the destTxList array of publish topics
																					  (those topics which came from the client list file) */
		}

		/* Assign pointers from temporary list to array */
		int i = 0, numFromMD = 0, numFromCL = 0;
		while ((currTuple = ism_common_list_iter_next(&tupleListIter)) != NULL) {
			if (((destTuple_t *) currTuple->data)) {
				destTuple_t *tuple = (destTuple_t *) currTuple->data;
				dstArray[i] = tuple;

				/* Assign indexes for publish topics based on whether the topic is from a message file or client list */
				if(pubOrSub == PUB) {
					if(tuple->fromMD){
						client->destTxFromMD[numFromMD++] = i;
					} else { // must be from client list file
						client->destTxFromCL[numFromCL++] = i;
					}
				}

				/* Set the topic alias for each publish topic, if MB_topicAliasMaxOut was set for this client */
				if(pubOrSub == PUB && client->topicalias_count_out > 0) {
					tuple->topicAlias = i + 1; // for V5 or later publishers a topic alias of 0 is NOT a valid topic alias
				}

				/* Create the stream ID for publish topics */
				if(pubOrSub == PUB) {
					char streamIDStr[64] = {0};
					uint64_t streamID = (uint64_t) mqttbenchInfo->instanceID << 32 | g_TopicID++;
					tuple->streamID = streamID;
					snprintf(streamIDStr, 64, "%llu", (ULL) streamID);
					tuple->streamIDStr = strdup(streamIDStr);  /* BEAM suppression */
					tuple->streamIDStrLen = strlen(streamIDStr);
				}

				/* check MQTT topic string for wildcards # and +, not permitted */
				if(pubOrSub == PUB) {
					char *wc1 = strstr(tuple->topicName, "#");
					char *wc2 = strstr(tuple->topicName, "+");
					if(wc1 || wc2) {
						MBTRACE(MBERROR, 1, "Client at line %d has a publish topic %s, with an MQTT wildcard character (\"#\" or \"+\") in it, this is an invalid topic\n",
								            line, tuple->topicName);
						return RC_INVALID_TOPIC;
					}
				}

				/* For subscribers check for MQTT version and use corresponding hashmap to store destTuple for fast lookups in onMessage */
				if(pubOrSub == SUB) {
					if(client->mqttVersion >= MQTT_V5) {
						tuple->subId = i + 1; // for V5 or later subscribers set a subscription id - NOTE: 0 is NOT a valid subscription ID
						ism_common_putHashMapElement(client->destRxSubIDMap, &tuple->subId, sizeof(tuple->subId), tuple, NULL); // store the pointer to the dest tuple using the sub ID as the key
					} else {
						if(!client->usesWildCardSubs){ // do not store subcsription destTuples for clients that have even one wildcard subscription
							ism_common_putHashMapElement(client->destRxSubMap, tuple->topicName, strlen(tuple->topicName), tuple, NULL); // store the pointer to the dest tuple using the topic name as the key
						}
					}
				}

				/* Set QoS bit in client QoS bitmap field */
				if(pubOrSub == SUB){
					client->rxQoSBitMap |= (1 << tuple->topicQoS);  /* indicate in the client object which QoS is used for subscriptions */
				} else {
					client->txQoSBitMap |= (1 << tuple->topicQoS);  /* indicate in the client object which QoS is used for publications */
				}

				tuple->client = client;  /* set the client pointer in the dest tuple */
				i++;
			}
		}

	} else {
		MBTRACE(MBDEBUG, 7, "Processed 0 %s tuple entries from the client at line %d\n",
				pubOrSub == PUB ? "publish topic" : "subscription", line);
	}

	ism_common_list_destroy(tupleList); // don't worry we did not pass a destroy function pointer while initializing this list, so data in the list will remain, only destroying list object itself

	return rc;
}

/*
 * Process the publish topic and subscription lists for the client entry in JSON client list file and
 * create the list of TX (publish) and RX (subscription) destinations.
 *
 * @param[in]	clientListJSON		= 	the in-memory JSON object representation of the client list file
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[out]	client				= 	the mqttclient_t object to store the TX (publish) and RX (subscription) destinations
 * @param[in]	pubOrSub			= 	a flag to indicate the caller context (i.e. calling this function for publish topics or subscriptions)
 * @param[out]	tmpList				= 	a temporary list to store destTuple_t object pointers for publish topics or subscriptions)
 *
 * @return	0 = OK, otherwise it is a failure
 */
static int processTopicsAndSubscriptions(ism_json_parse_t *clientListJSON, int where, mqttclient_t *client, int pubOrSub, ism_common_list *tmpList) {
	int rc = 0;

	ism_json_entry_t * ent = clientListJSON->ent + where;
	if (ent->objtype == JSON_Array) { 				// First JSON entry in the subscription or publish topic list should be a JSON array
		int endArrayLoc = where + ent->count;		// get the position of the last JSON entry in the subscription or publish topic JSON array

		if (ent->count) {							// Make sure the list of topics is not empty
			where++;

			while (where < endArrayLoc) {			// loop, each iteration is one entry in the publish topic or subscription list
				ent = clientListJSON->ent + where;	// get JSON entry at the current position (should be a topic JSON object)

				destTuple_t *dstTuple = (destTuple_t *) calloc(1, sizeof(destTuple_t));  // allocate a dest tuple object to store the properties for this publish topic or subscription
				if(dstTuple == NULL){
					MBTRACE(MBERROR, 1, "Unable to allocate memory for a %s object at line %d\n", pubOrSub == PUB ? "publish topic" : "subscription", ent->line);
					return RC_MEMORY_ALLOC_ERR;
				}

				rc = processTopic(clientListJSON, where, dstTuple, pubOrSub, tmpList);
				if(rc) {
					return RC_BAD_CLIENTLIST;
				}

				/* If this client is making an MQTT wildcard subscription, i.e. containing # and +, then set the client flag for wildcard subscriptions */
				if(pubOrSub == SUB) {
					char *wc1 = strstr(dstTuple->topicName, "#");
					char *wc2 = strstr(dstTuple->topicName, "+");
					if(wc1 || wc2) {
						client->usesWildCardSubs = 1; // if this is a wildcard subscription set the flag, do not store or lookup dest tuples by topic name for this client
					}
				}

				// move position in the JSON DOM to the next topic JSON entry
				if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
					where += ent->count + 1;
				else
					where++;
			} // loop through JSON array of publish topics or subscriptions
		} else {
			rc = RC_BAD_CLIENTLIST;
			MBTRACE(MBERROR, 5, "Client at line %d has specified an empty %s list\n",
								ent->line, pubOrSub == PUB ? "publish topic" : "subscription");
		}

		MBTRACE(MBDEBUG, 7, "Processed %lu %s tuple entries from the client at line %d\n",
				(unsigned long ) ism_common_list_getSize(tmpList), pubOrSub == PUB ? "publish topic" : "subscription", ent->line);
	} else {
		MBTRACE(MBERROR, 1, "The %s entry (line=%d where=%d) is expected to be, but is NOT, a JSON array.\n",
				pubOrSub == PUB ? "publish topics list" : "subscription list", ent->line, where);
		rc = RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Check that client is in the range of clients specified by the -crange command line parameter.  The client
 * is "in-range" if its position in the client list JSON file falls within any one of the comma separated list
 * of ranges provided by the -crange command line parameter.
 *
 * @param[in]	clientIdx		=	the index, or position, of the client in the mqttbench client list JSON file
 * @param[out]	crange			= 	comma separated list of ranges of client indexes to include in the test (e.g. 1-20,50-60,90-100)
 *
 * @return 0 	= false (client is not in the requested client range)
 * 		   1 	= true (it is in the range)
 */
static int isClientInCRange(uint32_t clientIdx, const char *crange){
	int inRange = 0;
	char *token, *savePtr1;

	// If no crange is provided on the command line then ACCEPT all clients in the client list file
	if(crange == NULL)
		return 1;

	char *rangeString = strdup(crange);
	token = strtok_r(rangeString, ",", &savePtr1);
	while(token != NULL){
		if(strchr(token, '-')) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
			char *savePtr2;
			char *start = strtok_r(token, "-", &savePtr2); // workaround bug in gcc compiler/glibc impl of strtok_r (https://gcc.gnu.org/bugzilla//show_bug.cgi?id=71701)
			char *end 	= strtok_r(NULL, "-", &savePtr2);
			if(start != NULL && end != NULL){
				char *endPtrStart, *endPtrEnd;
				uint32_t startVal, endVal;
				startVal 	= strtoul(start, &endPtrStart, 10);
				endVal 		= strtoul(end, &endPtrEnd, 10);
				if(!(endPtrStart && *endPtrStart != 0) && !(endPtrEnd && *endPtrEnd != 0) &&
				   startVal <= clientIdx && endVal >= clientIdx){
					  inRange = 1;
				}
			}
#pragma GCC diagnostic pop
		} else {
			if(token != NULL){
				char *endptr;
				uint32_t val;
				val = strtoul(token, &endptr, 10);
				if(!(endptr && *endptr != 0) && val == clientIdx) // check that token is a valid number and equal to client index
					inRange = 1;
			}
		}

		if(inRange)
			break;

		token = strtok_r(NULL, ",", &savePtr1);
	}

	free(rangeString);
	return inRange;
}

/*
 * Perform validation checks on the client to ensure that the client was not improperly configured in the
 * client list JSON file.
 *
 * @param[in]	client				= 	the fully configured mqttbench client object from the client list JSON DOM
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]	line				= 	line number in the client list file where this client is found
 *
 * @return 0 = OK, anything else is an error
 */
static int finalizeAndValidateClient(mqttclient_t *client, mqttbenchInfo_t * mqttbenchInfo, int line) {
	int rc = 0;
	mqttbench_t *submitterThread = client->mbinst;

	/********************************************************************
	 * Updates to submitter thread objects and other program wide objects
	 ********************************************************************/
	mqttbenchInfo->useWebSockets 			|= client->useWebSockets; 	/* if at least one client uses websockets then update the global useWebSockets variable */
	mqttbenchInfo->useSecureConnections     |= client->isSecure;        /* if at least one client uses secured connections then update the global useSecureConnections. */
	mqttbenchInfo->instanceClientType       |= client->clientType;      /* Update the client type for this instance of mqttbench. */
	submitterThread->clientType 			|= client->clientType; 		/* Ensure the mqttbench_t (submitter thread) clientType is consistent with its assigned clients (CONNECTONLY, CONSUMER, PRODUCER, DUAL_CLIENT, etc.*/
	submitterThread->rxQoSBitMap			|= client->rxQoSBitMap;		/* Ensure the mqttbench_t (submitter thread) Rx QoS bit map is the union of all client Rx QoS bitmaps */
	submitterThread->txQoSBitMap			|= client->txQoSBitMap;		/* Ensure the mqttbench_t (submitter thread) Tx QoS bit map is the union of all client Tx QoS bitmaps */
	submitterThread->numSubsPerThread   	+= client->destRxListCount; /* increment the total subscription count for the assigned submitter thread */
	submitterThread->numPubTopicsPerThread	+= client->destTxListCount; /* increment the total publish topic count for the assigned submitter thread */
	submitterThread->numClients         	+= 1;						/* increment client count for the assigned submitter thread */

	if (client->destRxListCount > submitterThread->maxSubsPerClient) {
		submitterThread->maxSubsPerClient = client->destRxListCount;    /* update submitter thread max subscription per client count */
	}

	if (client->connectionTimeoutSecs > 0) {
		g_numClientsWithConnTimeout++;
	}

	/* Enable client trace on this client if it should be enabled for this client */
	if(mqttbenchInfo->mbCmdLineArgs->clientTraceRegex && client->clientIDLen > 0){
		int matched = ism_regex_match(mqttbenchInfo->mbCmdLineArgs->clientTraceRegex, client->clientID);
		client->traceEnabled = (matched == 0) ? 1 : 0;
	}

	/*********************************
	 * Client Validation Checks
	 *********************************/
	if(client->mqttVersion == 0) {
		MBTRACE(MBERROR, 1, "No MQTT version was specified for client at line %d in the client list file. All clients must specify an MQTT version.\n", line);
		return RC_BAD_CLIENTLIST;
	}

	if(client->mqttVersion < MQTT_V5) {
		/* Check if any MQTT v5 fields have been set for an older (< v5) client */
		if(client->cleanStart) {
			MBTRACE(MBERROR, 1, "Client at line %d is not an MQTT v5 client, but is configured to use the cleanStart flag, this is reserved for MQTT v5 clients only\n", line);
			return RC_BAD_CLIENTLIST;
		}
		if(client->propsBuf){
			MBTRACE(MBERROR, 1, "Client at line %d is not an MQTT v5 client, but is configured to use MQTT v5 properties (e.g. session expiry interval, topic aliases, "\
					            "request response info, recv max, etc.), these are reserved for MQTT v5 clients only\n", line);
			return RC_BAD_CLIENTLIST;
		}
		if(strlen(client->clientID) == 0){
			MBTRACE(MBERROR, 1, "Client at line %d is not an MQTT v5 client and does not have a client ID specified, server assigned client IDs is only permitted for MQTT v5 clients\n", line);
			return RC_BAD_CLIENTLIST;
		}
		if( ((mqttbenchInfo->mbCmdLineArgs->latencyMask & CHKTIMERTT) > 0) && client->msgDirObj && (!client->msgDirObj->generated) ) {
			MBTRACE(MBERROR, 1, "Client at line %d is not an MQTT v5 client with message latency measurements (-T 0x1) enabled for predefined messages (-M or \"messageDirPath\" in client list)\n", line);
			return RC_BAD_CLIENTLIST;
		}
	} else {
		/* Check if any MQTT v4 and older fields have been set on a v5 client */
		if(client->cleansession) {
			MBTRACE(MBERROR, 1,	"Client at line %d is a v5 client, but is configured to use the cleanSession flag, this is reserved for MQTT v3.1.1 clients or earlier\n", line);
			return RC_BAD_CLIENTLIST;
		}
	}

	if(client->destTxListCount > 0 && client->msgDirObj == NULL) {
		MBTRACE(MBERROR, 1, "Client at line %d in the client list file was configured to publish messages on one or more topics, " \
						    "but no messages were configured (no -s <min>-<max> command line parameter, no -M <msg dir> command line parameter, "\
							"nor was a message directory specified for this client in the client list file.\n", line);
		return RC_PUB_BUT_NO_MSGS;
	}

	/*********************************
	 * Finish up client allocations
	 *********************************/

	/* Allocate the initial RX buffer for the client */
	rc = allocateInitialRxBuffer(client);
	if(rc) {
		MBTRACE(MBERROR, 1, "Unable to allocate the RX buffer for client at line %d\n", line);
		return rc;
	}

	/* Allocate the inbound topic alias table for MQTT v5 */
	if (client->topicalias_count_in) {
		client->topicalias_in = calloc(client->topicalias_count_in, sizeof(char *)); // allocate the table of inbound topic strings for alias mapping
	}

	/* Update global max TX buffer size, based on this client, all TX buffers are the same size and equal to the size of the max MQTT packet sent by any client in the client list */
	int maxTxPktSize = calculateMaxTXPacketSize(client, mqttbenchInfo);
	if(maxTxPktSize > g_TxBfrSize) {
		MBTRACE(MBINFO, 1, "Client at line %d has increased the max TX pkt size from %d bytes to %d bytes\n", line, g_TxBfrSize, maxTxPktSize);
		g_TxBfrSize = maxTxPktSize; /* keep track of the global max packet size */
	}

	return rc;
}

/*
 * Initialize the transport object of an MQTT client object
 *
 * @param[out]	client				= 	a pointer the mqttclient_t object to initialize the transport object for
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]	line				= 	line number in the client list file where this client is found
 *
 * @return 0 = OK, anything else is an error
 */
static int initClientTransport(mqttclient_t *client, mqttbenchInfo_t * mqttbenchInfo, int line) {
	int rc = 0;

	if(client->server == NULL || client->serverPort == 0) {
		MBTRACE(MBERROR, 1, "Client at line %d does not have a valid dst and dstPort field, these are required fields for every client.\n", line);
		return RC_BAD_CLIENTLIST;
	}

	/*****************************************************************************************
	 * Lookup the server destination in the dstIPList hashmap. If it cannot be found, then
	 * resolve the server destination and set the server destination IP in the client object
	 * from the list of resolved IPv4 addresses.
	 *****************************************************************************************/
	dstIPList_t *dstIPList = (dstIPList_t *) ism_common_getHashMapElement(g_dstIPMap, client->server, 0);
	if(dstIPList == NULL) {
		rc = processServerDestination(client->server, client->serverPort, (destIPList **) &dstIPList, line);
		if(rc || dstIPList == NULL) {
			MBTRACE(MBERROR, 1, "Failed to process the server destination %s for client at line %d in the client list file\n", client->server, line);
			return rc;
		}
	}
	client->serverIP = strdup(dstIPList->dipArrayList[dstIPList->nextDIP]);
	if(client->serverIP == NULL) {
		return provideAllocErrorMsg("a server destination IP address", strlen(dstIPList->dipArrayList[dstIPList->nextDIP]), __FILE__, __LINE__);
	}
	dstIPList->nextDIP = (dstIPList->nextDIP + 1) % dstIPList->numElements; // move index to next resolved destination IP in the dstIPList

	/*****************************************************************************************
	 * Lookup the list of source IPv4 addresses from which the resolved server destination IP
	 * address is reachable and select the next in the list. Assign this source IP to the client
	 * object.
	 *****************************************************************************************/
	srcIPList_t *srcIPList = (srcIPList_t *) ism_common_getHashMapElement(g_srcIPMap, client->serverIP, 0);
	client->srcIP = strdup(srcIPList->sipArrayList[srcIPList->nextSIP]);  /* BEAM suppression */
	if(client->srcIP == NULL) {
		return provideAllocErrorMsg("a source IPv4 address", strlen(srcIPList->sipArrayList[srcIPList->nextSIP]), __FILE__, __LINE__);
	}
	srcIPList->nextSIP = (srcIPList->nextSIP + 1) % srcIPList->numElements; // move index to next source IP in the list

	/* Iterate from SourcePortLo to SourcePortHi trying to find an available source port to connect from, if
	 * SKIP_PORT is returned from preInitializeTransport it means that the source port is already in use and we must try another source port. */
	do {
		if (g_RequestedQuit) {
			rc = RC_EXIT_ON_ERROR;
			break;
		}

		rc = preInitializeTransport(client, client->serverIP, client->serverPort, client->srcIP, g_srcPorts[srcIPList->nextSIP], mqttbenchInfo->mbCmdLineArgs->reconnectEnabled, line);
		if(mqttbenchInfo->mbSysEnvSet->useEphemeralPorts) {
			break; // no need to check for LOCAL_PORT_RANGE_HI when using ephemeral source port selection
		}
		g_srcPorts[srcIPList->nextSIP]++;
	} while ((rc == SKIP_PORT) && (g_srcPorts[srcIPList->nextSIP] < LOCAL_PORT_RANGE_HI));

	if (rc != 0 ) {
		MBTRACE(MBERROR, 1, "Failed to initialize the transport object for client at line %d in the client list file\n", line);
		return rc;
	}

	if(!mqttbenchInfo->mbSysEnvSet->useEphemeralPorts && g_srcPorts[srcIPList->nextSIP] >= LOCAL_PORT_RANGE_HI) {
		MBTRACE(MBERROR, 1, "Exhausted source ports. Exceeded available number of source ports per IPv4 address. More source IP addresses are required "\
							"in the SIPList env variable. If you set the UseEphemeralPorts env variable, ensure that net.ipv4.ip_local_port_range kernel "\
							"parameter is a large range (e.g. 5000 65000)\n");
		return RC_EXHAUSTED_IP_ADDRESSES;
	}

	/* Set client certificate and key if configured for this client */
	if(client->clientCertPath) {
		if(!client->clientKeyPath){
			MBTRACE(MBERROR, 1, "Client at line %d was configured with a client certificate, but not a client private key file.  Both must be configured\n", line);
			return RC_BAD_CLIENTLIST;
		}

		client->trans->ccert = (X509 *)     ism_common_getHashMapElement(g_ccertsMap, client->clientCertPath, strlen(client->clientCertPath));
		client->trans->cpkey = (EVP_PKEY *) ism_common_getHashMapElement(g_cpkeysMap, client->clientKeyPath,  strlen(client->clientKeyPath));
	}

	return rc;
}

/*
 * Initialize an MQTT client object which will then be configured
 *
 * @param[out]	client				= 	a pointer to an allocated, but uninitialized mqttclient_t object
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]	assignedIOPThread  	=	the IOP thread that this client is assigned to
 * @param[in]	assignedIOLThread  	=	the IOL thread that this client is assigned to
 * @param[in]	assignedSubThread  	=	the submitter thread that this client is assigned to
 * @param[in]   clientIdx			=	the index of the client in the overall list of created clients (i.e. those within the accepted crange)
 * @param[in]   line				=   the line in the client list file where this client is defined
 *
 * @return 0 = OK, anything else is an error
 */
static int initializeClient(mqttclient_t *client, mqttbenchInfo_t * mqttbenchInfo, int assignedIOPThread, int assignedIOLThread, int assignedSubThread, int clientIdx, int line) {
	int rc = 0;

	pthread_spin_init(&client->mqttclient_lock, 0);		// Initialize the mqttclient spinlock

	client->clientID = "";								// Initialize the client ID to zero length string (allowed by MQTT v5, but not earlier)
	client->clientIDLen = 0;
	client->line = line;								// set the line number for this client
	client->doData = doData;							// set the callback function when a packet arrives for this client
	client->protocolState = MQTT_UNKNOWN; 				// set the initial protocol state
	client->msgBytesNeeded = 0;   						// initialize the # of bytes for partial msg = 0
	client->maxPacketSize = MAX_UINT32;					// client will accept MQTT messages up to 4GB in size by default, this can be overridden in the client list file

	// Set any client configuration derived from environment variables or command line parameters (some of this maybe overridden from client list file)
	client->currRetryDelayTime_ns 	= client->initRetryDelayTime_ns = mqttbenchInfo->mbSysEnvSet->initConnRetryDelayTime_ns;
	client->lingerTime_ns 			= mqttbenchInfo->mbSysEnvSet->lingerTime_ns;
	client->keepAliveInterval 		= mqttbenchInfo->mbSysEnvSet->mqttKeepAlive;
	client->connectionTimeoutSecs	= MQTT_CONN_DEFAULT_TIMEOUT;
	client->pingTimeoutSecs			= mqttbenchInfo->mbCmdLineArgs->pingTimeoutSecs;   // default is MQTT_PING_DEFAULT_TIMEOUT
	client->pingIntervalSecs		= mqttbenchInfo->mbCmdLineArgs->pingIntervalSecs;  // default is 0, i.e. do not send PINGREQ
	client->lastPingSubmitTime		= 0;
	client->pingWindowStartTime		= 0;
	client->unackedPingReqs			= 0;
	client->pingTimeouts			= 0;
	client->retryInterval 			= MQTT_CONN_RETRIES;

	client->ioProcThreadIdx 		= assignedIOPThread;
	client->ioListenerThreadIdx 	= assignedIOLThread;
	client->mbinst 					= mqttbenchInfo->mbInstArray[assignedSubThread];
	client->connIdx 				= client->mbinst->numClients;
	client->clientType 				= CONNECTONLY; 		// this will later be updated if publish topics or subscriptions are configured for this client

	if(pIDSharedKeyArray && pIDSharedKeyArray->id_Count > 0){
		rc = setMQTTPreSharedKey(client, clientIdx, pIDSharedKeyArray->id_Count, pIDSharedKeyArray);
		if(rc) {
			MBTRACE(MBERROR, 1, "Failed to set the PSK ID/Key pair for client %d\n", clientIdx);
			return rc;
		}
	}
	return rc;
}

/*
 * Process a single client entry in the client list JSON DOM
 *
 * @param[out]	client				=	the mqttbench client object that will be allocated and populated from the current client entry in the JSON DOM
 * @param[in]	clientListJSON		= 	the in-memory JSON object representation of the client list file
 * @param[in]	where				= 	the current entry position in the JSON DOM
 * @param[in]	assignedIOPThread	=	the IOP thread to which this client should be assigned
 * @param[in]	assignedIOLThread	=	the IOL thread to which this client should be assigned
 * @param[in]	assignedSubThread	=	the Submitter thread to which this client should be assigned
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 * @param[in]   clientIdx			=	the index of the client in the overall list of created clients (i.e. those within the accepted crange)
 *
 * @return 0 = OK, anything else is an error
 */
static int processClientEntry(mqttclient_t *client, ism_json_parse_t *clientListJSON, int where, int assignedIOPThread, int assignedIOLThread, int assignedSubThread, mqttbenchInfo_t * mqttbenchInfo, int clientIdx){
	int rc=0;
	uint8_t propIDsArray[MAX_PROPIDS] = {0};  // array of Spec-Defined MQTT properties found while processing this object
	int propCount = 0; 					// number of Spec-Defined MQTT properties found while processing this object

	ism_json_entry_t * ent = clientListJSON->ent + where;
	if (ent->objtype == JSON_Object) {  				// a client JSON entry should be of type JSON object
		int endloc = where + ent->count;			    // get the position of the last JSON entry in the client object
		where++;

		// Initialize an mqttclient_t structure
		rc = initializeClient(client, mqttbenchInfo, assignedIOPThread, assignedIOLThread, assignedSubThread, clientIdx, ent->line);
		if(rc){
			MBTRACE(MBERROR, 1, "failed to initialize mqttclient_t structure\n");
			return rc;
		}

		/* Allocate two temporary destTuple_t lists, one for TX and one for RX. These are used to temporarily store destTuple_t objects when processing
		 * publish topics, subscriptions, or message files with topics defined.  These lists will later be used to copy destTuple_t* to the RX/TX tuple
		 * arrays in the client object. */
		ism_common_list *tmpDestTupleLists = (ism_common_list *) calloc(2, sizeof(ism_common_list));
		if (tmpDestTupleLists) {
			int rc1 = ism_common_list_init(&tmpDestTupleLists[PUB], 0, NULL);
			int rc2 = ism_common_list_init(&tmpDestTupleLists[SUB], 0, NULL);
			if(rc1 || rc2) {
				MBTRACE(MBERROR, 1, "Failed to initialize the temporary destination tuple lists for client entry at line %d\n", ent->line);
				return RC_MEMORY_ALLOC_ERR;
			}
		} else {
			MBTRACE(MBERROR, 1, "Unable to allocate memory for the temporary destination tuple lists for client entry at line %d\n", ent->line);
			return RC_MEMORY_ALLOC_ERR;
		}

		MBTRACE(MBDEBUG, 7, "processing client at line=%d where=%d\n", ent->line, where);
		while(where <= endloc){	// Process fields in an MBClient JSON object
			rc = 0;
			ent = clientListJSON->ent + where;
			MBTRACE(MBDEBUG, 7, "where=%d name=%s objtype=%d value=%s level=%d count=%d\n", where, ent->name, ent->objtype, ent->value, ent->level, ent->count);

			int64_t tmpVal = 0;
			int32_t fieldId = ism_common_enumValue(mbcl_FieldsEnum, ent->name);
			switch(fieldId){
				case MB_version:
					rc = processMQTTVersion(ent, client); break;
				case MB_lastWillMsg:
					rc = processLastWillMsg(clientListJSON, where, client); break;
				case MB_publishTopics:
					rc = processTopicsAndSubscriptions(clientListJSON, where, client, PUB, &tmpDestTupleLists[PUB]); break;
				case MB_subscriptions:
					rc = processTopicsAndSubscriptions(clientListJSON, where, client, SUB, &tmpDestTupleLists[SUB]); break;
				case MB_userProperties:
					rc = processMQTTUserDefinedProperties(clientListJSON, where, &client->propsBuf, client->userPropsArray, &client->numUserProps); break;
				case MB_id:
					rc = jsonGetString(ent, &client->clientID, &tmpVal, MAX_CLIENTID_LEN, 0);
					if(rc == 0) {
						client->clientIDLen = tmpVal;
						if(client->clientIDLen > g_LongestClientIDLen) {
							g_LongestClientIDLen = client->clientIDLen;
						}
					}
					break;
				case MB_username:
					rc = jsonGetString(ent, &client->username, &tmpVal, -1, 0); client->usernameLen = tmpVal; break;
				case MB_password:
					rc = jsonGetString(ent, &client->password, &tmpVal, -1, 1); client->passwordLen = tmpVal; break; // password is base64 encoded and must be decoded
				case MB_messageDirPath:
					rc = jsonGetString(ent, &client->msgDirPath, NULL, -1, 0); break;
				case MB_clientCertPath:
					rc = jsonGetString(ent, &client->clientCertPath, NULL, -1, 0);
					if(rc == 0) {
						rc = processClientCertOrKey(client->clientCertPath, CLIENT_CERTIFICATE, ent->line);
					}
					break;
				case MB_clientKeyPath:
					rc = jsonGetString(ent, &client->clientKeyPath, NULL, -1, 0);
					if(rc == 0) {
						rc = processClientCertOrKey(client->clientKeyPath, CLIENT_PRIVATE_KEY, ent->line);
					}
					break;
				case MB_dst:
					rc = jsonGetString(ent, &client->server, NULL, -1, 0); break;
				case MB_dstPort:
					rc = jsonGetInteger(ent, &tmpVal, 1, MAX_PORT_NUMBER); client->serverPort = tmpVal; break;
				case MB_sessionExpiryIntervalSecs:
					rc = processMQTTPropertyInteger(ent, MPI_SessionExpire, &client->propsBuf, 0, MAX_UINT32, propIDsArray); propCount++; break;
				case MB_recvMaximum:
					rc = processMQTTPropertyInteger(ent, MPI_MaxReceive, &client->propsBuf, 1, MAX_UINT16, propIDsArray); propCount++; break;
				case MB_maxPktSizeBytes:
					rc = processMQTTPropertyInteger(ent, MPI_MaxPacketSize, &client->propsBuf, 1, MAX_UINT32, propIDsArray); propCount++;
					if(rc == 0) {
						client->maxPacketSize = ent->objtype == JSON_Integer ? ent->count : strtod(ent->value, NULL);
					}
					break;
				case MB_topicAliasMaxIn:
					rc = processMQTTPropertyInteger(ent, MPI_MaxTopicAlias, &client->propsBuf, 0, MAX_UINT16, propIDsArray); propCount++;
					if(rc == 0) {
						client->topicalias_count_in = ent->objtype == JSON_Integer ? ent->count : strtod(ent->value, NULL); // store the topic alias inbound max count
					}
					break;
				case MB_topicAliasMaxOut:
					rc = jsonGetInteger(ent, &tmpVal, 1, MAX_UINT16); client->topicalias_count_out = tmpVal; break;
				case MB_requestResponseInfo:
					rc = processMQTTPropertyInteger(ent, MPI_RequestReplyInfo, &client->propsBuf, 0, 1, propIDsArray); propCount++; break;
				case MB_requestProblemInfo:
					rc = processMQTTPropertyInteger(ent, MPI_RequestReason, &client->propsBuf, 0, 1, propIDsArray); propCount++; break;
				case MB_lingerTimeSecs:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_UINT32); client->lingerTime_ns=tmpVal*NANO_PER_SECOND; break;
				case MB_lingerMsgsCount:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_UINT32); client->lingerMsgsCount = tmpVal; break;
				case MB_reconnectDelayUSecs:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_INT32); client->currRetryDelayTime_ns=client->initRetryDelayTime_ns=tmpVal*NANO_PER_MICRO; break;
				case MB_connectionTimeoutSecs:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_INT32); client->connectionTimeoutSecs = tmpVal; break;
				case MB_pingTimeoutSecs:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_INT32); client->pingTimeoutSecs = tmpVal; break;
				case MB_pingIntervalSecs:
					rc = jsonGetInteger(ent, &tmpVal, 0, MAX_INT32); client->pingIntervalSecs = tmpVal; break;
				case MB_cleanSession:
					client->cleansession = ent->objtype == JSON_True; break;
				case MB_cleanStart:
					client->cleanStart = ent->objtype == JSON_True; break;
				case MB_useTLS:
					client->isSecure = ent->objtype == JSON_True; break;
			}

			if(rc){
				return rc; // no point in validating the client, since we found a problem on a single field in the client JSON object
			}

			// move position in the JSON DOM to the next client JSON entry
			if(ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
		    else
		    	where++;
		}

		/* Assign the source of messages to be published by this client according to command line and client list file configuration */
		if(client->msgDirPath) { // client specific message directory is set, so process the JSON message files
			rc = processMessageDir(client->msgDirPath, mqttbenchInfo);
			if(rc){
				MBTRACE(MBERROR, 1, "Failed to process the message directory for client entry at line %d (where=%d) in the mqttbench client list file\n", ent->line, where);
				return rc;
			}
			client->msgDirObj = ism_common_getHashMapElement(mqttbenchInfo->mbMsgCfgInfo->msgDirMap, client->msgDirPath, 0);
			if(client->msgDirObj){
				for(int i=0; i < client->msgDirObj->numMsgObj; i++){
					destTuple_t *topic = (destTuple_t *) client->msgDirObj->msgObjArray[i].topic;
					if(topic){ /* If the message object contains a topic, then replicate it, and insert it into the publish topic list */
						if (strcasestr(topic->topicName, "${COUNT:")) {
							rc = replicateTuple(topic, &tmpDestTupleLists[PUB]); // insert updated tuple and all its modified clones into the list of TX destTuples
							if (rc) {
								MBTRACE(MBERROR, 1,	"Failed to replicate publish topic (from message object) with COUNT variable at line %d of the client list file.\n",
													ent->line);
								return rc;
							}
						} else {
							destTuple_t *clone = calloc(1, sizeof(destTuple_t));
							rc = deepCopyTuple(topic, clone);
							if(rc){
								MBTRACE(MBERROR, 1,"Failed to deep copy publish topic from message object %s for client at line %d\n",
										           client->msgDirObj->msgObjArray[i].filePath, ent->line);
								return rc;
							}
							ism_common_list_insert_tail(&tmpDestTupleLists[PUB], clone); // no ${COUNT:x-y} variable, so insert into the list as-is
						}

						client->clientType  |= PRODUCER;                /* mark this client as a producer */
						client->txQoSBitMap |= (1 << topic->topicQoS);  /* indicate in the client object which QoS is used for publications */
					}
					else {
						if(ism_common_list_getSize(&tmpDestTupleLists[PUB]) == 0) {
							MBTRACE(MBERROR, 1, "Client %s at line %d in the client list file specified a message directory that contained message file %s, "\
									            "which does NOT specify a topic AND no publish topic list was specified for this client, so this message has "\
												"no destination topic to be published to\n",
												client->clientID, ent->line, client->msgDirObj->msgObjArray[i].filePath);
							return RC_BAD_CLIENTLIST;
						}
					}
				}
			}
		} else { // this client does not have a message directory set, so check for -s or -M command line parameters and set this clients message dir object accordingly (remember -s is a special case of msg directory)
			if(mqttbenchInfo->mbCmdLineArgs->msgSizeRangeParmFlag){
				client->msgDirObj = (messagedir_object_t *) ism_common_getHashMapElement(mqttbenchInfo->mbMsgCfgInfo->msgDirMap, PARM_MSG_SIZES, 0);
			}

			if(mqttbenchInfo->mbCmdLineArgs->msgDirParmFlag) {
				client->msgDirObj = (messagedir_object_t *) ism_common_getHashMapElement(mqttbenchInfo->mbMsgCfgInfo->msgDirMap, g_predefinedMsgDir, 0);
			}
		}

		/* Move Publish topics and subscriptions from linked lists to TX and RX arrays, respectively */
		int rc1 = tupleListToArray(&tmpDestTupleLists[PUB], client, PUB, ent->line, mqttbenchInfo); // tmp list is destroyed, but destTuple_t data pointers are retained
		int rc2 = tupleListToArray(&tmpDestTupleLists[SUB], client, SUB, ent->line, mqttbenchInfo);
		if(rc1 || rc2){
			MBTRACE(MBERROR, 1, "Failed to move dest tuple pointers to tuple arrays for client entry at line %d (where=%d) in the mqttbench client list file\n", ent->line, where);
			return RC_MEMORY_ALLOC_ERR;
		}

		/* Copy MQTT properties set from stack memory to heap memory in the client object */
		if(propCount > 0){
			client->numMQTTProps = propCount;
			client->propIDsArray = (uint8_t *) calloc(propCount, sizeof(uint8_t));
			if(client->propIDsArray == NULL){
				MBTRACE(MBERROR, 1,	"Unable to allocate memory for the MQTT Spec-Defined properties ID array for client at line %d\n", ent->line);
				return RC_MEMORY_ALLOC_ERR;
			}
			for (int i=0; i < propCount; i++) {
				if(propIDsArray[i] != 0){
					client->propIDsArray[i] = i;
				}
			}
		}

		/* Initialize the transport object for the client */
		rc = initClientTransport(client, mqttbenchInfo, ent->line);
		if(rc){
			MBTRACE(MBERROR, 1, "Failed to initialize client transport object for client at line %d (where=%d) in the mqttbench client list file\n", ent->line, where);
			return rc;
		}

		/* Validate client object */
		rc = finalizeAndValidateClient(client, mqttbenchInfo, ent->line);
		if(rc){
			MBTRACE(MBERROR, 1, "Failed to validate client entry at line %d (where=%d) in the mqttbench client list file\n", ent->line, where);
			return rc;
		}

	} else {
		MBTRACE(MBERROR, 1, "The client entry at line %d (where=%d) is expected to be, but is NOT, a JSON object.\n", ent->line, where);
		return RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Loop through the client entries in the mqttbench client list JSON in-memory object, process each client entry, and distribute
 * mqttclient_t objects across submitter thread client lists in round robin order.  Exit the loop with a useful error message on
 * the first problem found in the client list file.
 *
 * @param[in]	clientListJSON		=	the in-memory JSON object representation of the client list file
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 *
 * @return 0 on successful completion, or a non-zero value
 *
 */
static int buildClientLists(ism_json_parse_t *clientListJSON, mqttbenchInfo_t * mqttbenchInfo) {
	int rc=0;
	int numSubmitterThreads = mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads;
	int numIOPThreads = mqttbenchInfo->mbSysEnvSet->numIOProcThreads;
	int numIOLThreads = mqttbenchInfo->mbSysEnvSet->numIOListenerThreads;
	char * crange = mqttbenchInfo->mbCmdLineArgs->crange;
	int processedCount=0, skippedCount=0, createdCount=0, numQ12PubClients=0;
	double startTime = ism_common_readTSC();

	if (clientListJSON->ent->objtype == JSON_Array) {  	// First entry in the mqttbench client list should be a JSON array
		uint64_t where=1;								// current position in the JSON DOM
		int endArrayloc = clientListJSON->ent_count;	// total number of entries in the JSON DOM (must not go beyond this point)

		double prevTime = ism_common_readTSC();
		while (where < endArrayloc) {								// big loop, each iteration is one client entry in the JSON DOM
			ism_json_entry_t * ent = clientListJSON->ent + where;	// get JSON entry at the current position (should be a client JSON object)

			if(isClientInCRange(processedCount + 1, crange)) {			 	// verify that this client is in the range of clients specified by the -crange parameter, +1 first client starts at 1
				int assignedIOPThread = createdCount % numIOPThreads;		// assign client to IOP and submitter threads in round robin order
				int assignedIOLThread = createdCount % numIOLThreads;
				int assignedSubmitterThread = createdCount % numSubmitterThreads;

				mqttclient_t *client = calloc(1, sizeof(mqttclient_t)); /*BEAM suppression: memory leak*/
				if (client == NULL) {
					MBTRACE(MBERROR, 1, "Unable to allocate memory for the mqttclient_t structure for client at line %d\n", ent->line);
					return RC_MEMORY_ALLOC_ERR;
				}

				rc = processClientEntry(client, clientListJSON, where, assignedIOPThread, assignedIOLThread, assignedSubmitterThread, mqttbenchInfo, createdCount);
				if(rc) {
					MBTRACE(MBERROR, 1, "An error occurred while processing client JSON entry at line %d (where=%llu) in the mqttbench client list JSON file (%s).\n",
							ent->line, (unsigned long long) where, mqttbenchInfo->mbCmdLineArgs->clientListPath);
					rc=RC_BAD_CLIENTLIST;
					break;
				}

				if((client->txQoSBitMap & (MQTT_QOS_1_BITMASK | MQTT_QOS_2_BITMASK)) > 0) {
					numQ12PubClients++; // need this count to autotune the inflight window allocations per client
				}

				if(client->pingTimeoutSecs > 0) {
					g_numClientsWithPingInterval++;
				}

				// Add client to the client tuples list for the assigned submitter thread
				ism_common_list_insert_tail(mqttbenchInfo->mbInstArray[assignedSubmitterThread]->clientTuples, client);
				createdCount++;
			} else {
				skippedCount++; // increment the skipped client count (client not in crange)
			}
			processedCount++;  // increment the count of clients entries that have been processed,

			double currTime = ism_common_readTSC();
			if(currTime - prevTime > 5) {
				MBTRACE(MBINFO, 1, "Processed %d clients in %.3f seconds. (note: assigning source ports from ephemeral port range can be slow)\n", processedCount, currTime - startTime);
				prevTime = currTime;
			}

			// move position in the JSON DOM to the next client JSON entry
			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;  // this should not be the case, since the mqttbench file is a JSON array of JSON objects
		}
		double endTime = ism_common_readTSC();
		g_TotalNumClients = mqttbenchInfo->clientListNumEntries = createdCount;

		if(rc) {
			MBTRACE(MBERROR, 2, "Failed while processing clients entries from the mqttbench client list file (%s). Processing time before failure, %.3f seconds. "\
					            "Created %d clients and skipped %d clients, before failure\n",
								mqttbenchInfo->mbCmdLineArgs->clientListPath, (endTime - startTime), createdCount, skippedCount);
			return rc;
		}

		/* Autotune/allocate the inflight message window for all clients. Adjust max size based on the number of QoS 1 & 2 publishers in the test
		 *
		 * # QoS>0 Publishers	Tuned Inflight Msg Window Size
		 * 1M 					10
		 * 500K					20
		 * 250K					40
		 * 100K					100
		 * 10K					1K
		 * 1K					10K
		 * 100					64K
		 * 10					64K
		 *
		 * */
		int tunedMsgIdCount = MAX_GLOBAL_MSGID;
		if (numQ12PubClients)
			tunedMsgIdCount /= numQ12PubClients;
		for(int i=0; i < numSubmitterThreads; i++) {
			ism_common_listIterator iter;
			ism_common_list *clients = mqttbenchInfo->mbInstArray[i]->clientTuples;
			ism_common_list_iter_init(&iter, clients);
			ism_common_list_node *currTuple;
			while ((currTuple = ism_common_list_iter_next(&iter)) != NULL) {
				mqttclient_t *client = (mqttclient_t *) currTuple->data;
				/* For consumers (subscribe msgs), or producers that publish at QoS > 0 , allocate a list of inflight message IDs */
				rc = allocateInFlightMsgIDs(client, mqttbenchInfo, tunedMsgIdCount);
				if(rc) {
					MBTRACE(MBERROR, 1, "Unable to allocate inflight message IDs for client at line %d\n", client->line);
					return rc;
				}
			}
		}

		MBTRACE(MBINFO, 2, "Processed %d clients entries from the mqttbench client list file (%s) in %.3f seconds. Created %d clients and skipped %d clients\n",
				processedCount, mqttbenchInfo->mbCmdLineArgs->clientListPath, (endTime - startTime), createdCount, skippedCount);
	} else {
		MBTRACE(MBERROR, 1, "The first element in the mqttbench client list JSON file is expected to be, but is NOT, a JSON array.\n");
		rc=RC_BAD_CLIENTLIST;
	}

	return rc;
}

/*
 * Process the mqttbench configuration files:
 * - the client list JSON file
 * - any mqttbench message JSON files specified in the client list JSON file
 *
 * Create/configure the MQTT clients based on the configuration provided in the client list file and assign
 * the clients in round robin order to the list of submitter and IOP threads.  The number of submitter threads is
 * specified on the command line and the number of IOP threads is specified by env variable.
 *
 * @param[in]	mqttbenchInfo   	=	mqttbench user input object (command line params, env vars, etc.)
 *
 * @return 0 on successful completion, or a non-zero value
 * */
int processConfig(mqttbenchInfo_t *mqttbenchInfo) {
	int rc=0;
	ism_json_parse_t *clientListJSON;	 	// object for storing parsed client list JSON DOM

	// parse the mqttbench client list file as JSON
	clientListJSON = calloc(1, sizeof(ism_json_parse_t));
	rc = parseJSONFile(mqttbenchInfo->mbCmdLineArgs->clientListPath, clientListJSON);
	if(rc) {
		MBTRACE(MBERROR, 1, "Failed to process the mqttbench client list file (%s)\n", mqttbenchInfo->mbCmdLineArgs->clientListPath);
		return rc;
	}

	/* allocate/initialize the MQTT client lists for each mqttbench_t object in the global array of mqttbench_t objects
	 * the size of this array is based on the number of submitter threads */
	for(int i=0 ; i < mqttbenchInfo->mbCmdLineArgs->numSubmitterThreads ; i++) {
		mqttbench_t *mbinst = mqttbenchInfo->mbInstArray[i];

		// Allocate and initialize an mqttbench client list for each submitter thread
		ism_common_list *clientList = (ism_common_list *) calloc(1, sizeof(ism_common_list));
		ism_common_list_init(clientList, 0, NULL);

		// Assign client list to mqttbench_t object for this submitter thread
		mbinst->clientTuples = clientList;

		// Store a reference to the global mqttbench instance information object to each submitter thread
		mbinst->mqttbenchInfo = (mbInfo *) mqttbenchInfo;
	}

	/* Allocate/Initialize global hashmaps for client certificates and keys */
	g_ccertsMap = ism_common_createHashMap(32 * 1024, HASH_STRING);
	g_cpkeysMap = ism_common_createHashMap(32 * 1024, HASH_STRING);
	if(g_ccertsMap == NULL || g_cpkeysMap == NULL)
		return provideAllocErrorMsg("global client certificate and key hashmap objects", sizeof(g_ccertsMap) + sizeof(g_cpkeysMap), __FILE__, __LINE__);

	/* Allocate/Initialize global hashmaps for destination and source IPv4 addresses */
	g_dstIPMap = ism_common_createHashMap(64 * 1024, HASH_STRING);  /* optimized for 100K+ client connections */
	g_srcIPMap = ism_common_createHashMap(64 * 1024, HASH_STRING);  /* optimized for 100K+ client connections */

	// begin processing the client list file and populating the client lists for each submitter thread
	rc = buildClientLists(clientListJSON, mqttbenchInfo);
	if(rc){
		MBTRACE(MBERROR, 1, "An error occurred when building the list of MQTT clients from the client list file\n");
		return rc;
	}

	/* Cleanup dst and src IP hashmaps as well as client cert and key hashmaps. They
	 * are no longer needed as the client have been assigned dst/src IPs and client cert/keys (if configured)*/
    doRemoveHashMaps();

	// we don't need the JSON object anymore
    if (clientListJSON->source)
        free(clientListJSON->source);
    if (clientListJSON->ent)
        ism_common_free(ism_memory_utils_parser, clientListJSON->ent);
    if (clientListJSON) {
        free(clientListJSON);
	}
	return rc;
}
