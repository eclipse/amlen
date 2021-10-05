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
/*
 */

#define TRACE_COMP Monitoring

#include <ismutil.h>
#include <ismjson.h>
#include <transport.h>
#include <monitoringutil.h>
#include <monserialization.h>
#include <endpointMonData.h>
#include <validateConfigData.h>
#include <admin.h>



extern ism_ts_t *monEventTimeStampObj;
extern pthread_spinlock_t monEventTimeStampObjLock;
extern ism_config_itemValidator_t * ism_config_validate_getRequiredItemList(int type, char *name, int *rc);
extern void ism_config_validate_freeRequiredItemList(ism_config_itemValidator_t *item);
char *  ism_admin_getStateStr(int type);

/*
 * Put one character to a concat buf
 */
static void ism_monitoring_bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/************************************************Sort Comparators*********************************/

int ism_monitoring_sortComparatorConnTimeBest(const void *data1, const void *data2){
	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;
	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{
		if(conn1->duration < conn2->duration) return   1;
		if(conn1->duration > conn2->duration) return   -1;

	}
	return 0;

}

int ism_monitoring_sortComparatorConnTimeWorst(const void *data1, const void *data2){


	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;
	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{
		if(conn1->duration < conn2->duration) return  -1;
		if(conn1->duration > conn2->duration) return  1;

	}
	return 0;
}

int ism_monitoring_sortComparatorTPutMsgWorst(const void *data1, const void *data2)
{
	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;


	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{

		double tput1 = (conn1->read_msg + conn1->write_msg )/(conn1->duration * 1.0) ;
		double tput2 = (conn2->read_msg + conn2->write_msg )/(conn2->duration * 1.0) ;
		if(tput1 < tput2) return -1;
		if(tput1 > tput2) return 1;
	}
	return 0;
}

int ism_monitoring_sortComparatorTPutMsgBest(const void *data1, const void *data2)
{
	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;

	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{

		double tput1 = (conn1->read_msg + conn1->write_msg )/(conn1->duration * 1.0) ;
		double tput2 = (conn2->read_msg + conn2->write_msg )/(conn2->duration * 1.0) ;
		if(tput1 < tput2) return 1;
		if(tput1 > tput2) return -1;
	}
	return 0;
}

int ism_monitoring_sortComparatorTPutBytesBest(const void *data1, const void *data2)
{
	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;

	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{

		double tput1 = (conn1->read_bytes + conn1->write_bytes )/(conn1->duration * 1.0) ;
		double tput2 = (conn2->read_bytes + conn2->write_bytes )/(conn2->duration * 1.0) ;
		if(tput1 < tput2) return 1;
		if(tput1 > tput2) return -1;
	}
	return 0;
}

int ism_monitoring_sortComparatorTPutBytesWorst(const void *data1, const void *data2)
{
	ism_connect_mon_t * conn1 = *(ism_connect_mon_t * const *) data1;
	ism_connect_mon_t * conn2 = *(ism_connect_mon_t * const *) data2;

	if(conn1==NULL){
		return -1;
	}else if(conn2==NULL){
		return 1;
	}else{

		double tput1 = (conn1->read_bytes + conn1->write_bytes )/(conn1->duration * 1.0) ;
		double tput2 = (conn2->read_bytes + conn2->write_bytes )/(conn2->duration * 1.0) ;
		if(tput1 < tput2) return -1;
		if(tput1 > tput2) return 1;
	}
	return 0;
}


/************************************************End Sort *********************************/

/* Returns time in sec */
ism_snaptime_t ism_monitoring_currentTimeSec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (ism_snaptime_t)tv.tv_sec;
}

/* Returns Message ID */
XAPI void ism_monitoring_getMsgId(ism_rc_t code, char * buffer) {
    sprintf(buffer, "CWLNA%04d", code%10000);
}

/**
 * Get the Monitoring Object Type String
 * @param objType	Object Type
 * @return the object type string
 */
char * ism_monitoring_getMonObjectType(ismMonitoringObjectType_t objType)
{
		/*Get Object Type String*/
		char * objectTypeStr=NULL;

		switch(objType){
			case ismMonObjectType_Server:
				objectTypeStr = "Server";
				break;
			case ismMonObjectType_Endpoint:
				objectTypeStr = "Endpoint";
				break;
			case ismMonObjectType_Connection:
				objectTypeStr = "Connection";
				break;
			case ismMonObjectType_Topic:
				objectTypeStr = "Topic";
				break;
			case ismMonObjectType_Queue:
				objectTypeStr = "Queue";
				break;
			case ismMonObjectType_DestinationMappingRule:
				objectTypeStr = "DestinationMappingRule";
				break;
			case ismMonObjectType_Memory:
				objectTypeStr= "Memory";
				break;
			case ismMonObjectType_Store:
				objectTypeStr="Store";
				break;
            case ismMonObjectType_Plugin:
                objectTypeStr="Plugin";
                break;
			default:
				objectTypeStr="Unknown";

		};

		return objectTypeStr;
}

/*
 * Escape the node name. Replace a space for the following characters
 * +, /, or #
 * Escape the doublequote using a single double quote
 */
/*
 * Copy a byte array with CSV escapes.
 * Escape the doublequote using a single double quote
 */
static void nodeNameEscape(char * to, const char * from, int len) {
    int i;
    for (i=0; i<len; i++) {
        uint8_t ch = (uint8_t)*from++;
        if (ch!='/' && ch!='+' && ch!='#')
				 *to++ = ch;

    }
}

/**
 * Get node Name
 * @param nodeName 		the output of the nodeName
 * @param nodeNameLen	Maximum len of the node name
 * @param escape		Whether to escape the /, +, # characters
 */
void ism_monitoring_getNodeName(char *nodeName, int nodeNameLen, int escape)
{
	if(nodeName!=NULL && nodeNameLen>0){
		memset(nodeName, 0, nodeNameLen);
		gethostname(nodeName, nodeNameLen);
		if(escape){
			char * tnode = alloca(strlen(nodeName));
			nodeNameEscape(tnode, (const char *)nodeName, strlen(nodeName));
			strcpy(nodeName, tnode);
		}

	}

}

/**
 * This function constructs the Topic String for External monitoring or Alert
 * @param objType 		Object Type
 * @param objectName 	Object Name or ID
 * @param outputBuffer	The output buffer
 */
void ism_monitoring_getExtMonTopic(ismMonitoringObjectType_t objType, char *outputBuffer)
{
	if(outputBuffer!=NULL)
	{
		/*Get Object Type String*/
		char * objectTypeStr= ism_monitoring_getMonObjectType(objType);
		sprintf(outputBuffer, "$SYS/ResourceStatistics/%s", objectTypeStr);
	}
}

/**
 * This function constructs the Prefix for the external monitoring or alert message.
 * The prefix will contains the following:
 * -Node name
 * -Timestamp
 * -ObjectType
 * -ObjectName (if not NULL)
 *
 * @param	objType 		Object type
 * @param	objectName		Object Name or ID
 * @param 	outputBuffer	Output buffer
 */
void ism_monitoring_getMsgExternalMonPrefix(ismMonitoringObjectType_t objType,
												 uint64_t currentTime,
												 const char * objectName,
												 concat_alloc_t * outbuf)
{


	if(outbuf!=NULL){
		char tbuf[1024];

		/*Get Object Type String*/
		char * objectTypeStr= ism_monitoring_getMonObjectType(objType);


		/*Get Time String*/
		char *timeValue = NULL;
		char tbuffer[80];
		if ( monEventTimeStampObj != NULL) {
			/*Lock the timestamp object because this function will be called from multiple threads*/
			pthread_spin_lock(&monEventTimeStampObjLock);
            ism_common_setTimestamp(monEventTimeStampObj, currentTime);
            ism_common_formatTimestamp(monEventTimeStampObj, tbuffer, 80, 0, ISM_TFF_ISO8601);
            pthread_spin_unlock(&monEventTimeStampObjLock);
            timeValue = tbuffer;
        }
        /*Get Server version*/
        sprintf((char *)&tbuf,"\"Version\":");
		ism_common_allocBufferCopyLen(outbuf, (char *)&tbuf, strlen(tbuf));
		ism_json_putString(outbuf,ism_common_getVersion());

        /*Get Node Name*/
		char nodeName[1024];
		ism_monitoring_getNodeName((char*)&nodeName, 1024, 0);

		/*Since this is JSON. Use json putstring API to escape characters.*/
		/*Put Node name*/
		sprintf((char *)&tbuf,",\"NodeName\":");
		ism_common_allocBufferCopyLen(outbuf, (char *)&tbuf, strlen(tbuf));
		ism_json_putString(outbuf,(const char *) nodeName);


		/*Put Time Stamp*/
		sprintf((char *)&tbuf,",\"TimeStamp\":");
		ism_common_allocBufferCopyLen(outbuf, (char *)&tbuf, strlen(tbuf));
		ism_json_putString(outbuf, (const char *) timeValue);

		/*Put Time Stamp*/
		sprintf((char *)&tbuf, ",\"ObjectType\":");
		ism_common_allocBufferCopyLen(outbuf, (char *)&tbuf, strlen(tbuf));
		ism_json_putString(outbuf, (const char *) objectTypeStr);


        if(objectName!=NULL){
			sprintf((char *)&tbuf, ",\"ObjectName\":");
			ism_common_allocBufferCopyLen(outbuf, (char *)&tbuf, strlen(tbuf));
			ism_json_putString(outbuf, (const char *) objectName);

		}
	}
}

int ism_monitoring_setReturnCodeAndStringJSON(concat_alloc_t *output_buffer, int rc, const char * returnString)
{

	char tmpbuf[1024];

	sprintf(tmpbuf, "{ \"RC\":\"%d\", \"ErrorString\":", rc);

	ism_common_allocBufferCopyLen(output_buffer, tmpbuf, strlen(tmpbuf));
	/*JSON encoding for the Error String.*/
	ism_json_putString(output_buffer, returnString);

	ism_common_allocBufferCopyLen(output_buffer, "}", 1);

	return ISMRC_OK;

}

/*
 * Get monitoring action type
 */
int ism_monitoring_getStatType(char *actionString) {
    if ( !strcasecmp(actionString, "Store"))
        return ismMON_STAT_Store;
    else if ( !strcasecmp(actionString, "Memory"))
        return ismMON_STAT_Memory;
    else if ( !strcasecmp(actionString, "HA"))
        return ismMON_STAT_HA;
    else if ( !strcasecmp(actionString, "Endpoint"))
        return ismMON_STAT_Endpoint;
    else if ( !strcasecmp(actionString, "Connection"))
        return ismMON_STAT_Connection;
    else if ( !strcasecmp(actionString, "Subscription"))
        return ismMON_STAT_Subscription;
    else if ( !strcasecmp(actionString, "Topic"))
        return ismMON_STAT_Topic;
    else if ( !strcasecmp(actionString, "Queue"))
        return ismMON_STAT_Queue;
    else if ( !strcasecmp(actionString, "MQTTClient"))
        return ismMON_STAT_MQTTClient;
    else if ( !strcasecmp(actionString, "DumpTopic"))
        return ismMON_STAT_AdvEnginePD;
    else if ( !strcasecmp(actionString, "DumpTopictree"))
        return ismMON_STAT_AdvEnginePD;
    else if ( !strcasecmp(actionString, "DumpQueue"))
        return ismMON_STAT_AdvEnginePD;
    else if ( !strcasecmp(actionString, "DumpClient"))
        return ismMON_STAT_AdvEnginePD;
    else if ( !strcasecmp(actionString, "MemoryDetail"))
        return ismMON_STAT_MemoryDetail;
    else if ( !strcasecmp(actionString, "Security"))
        return ismMON_STAT_Security;
    else if ( !strcasecmp(actionString, "Transaction"))
        return ismMON_STAT_Transaction;
    else if ( !strcasecmp(actionString, "Cluster"))
        return ismMON_STAT_Cluster;
    else if ( !strcasecmp(actionString, "Forwarder"))
        return ismMON_STAT_Forwarder;
    else if ( !strcasecmp(actionString, "ResourceSet"))
        return ismMON_STAT_ResourceSet;
    return ismMON_STAT_None;
}

/*
 * This should be a temporary function which will use existing admin validation and monitoring functions.
 * The existing functions require a monitoring JSON string listed below.
 * We should update monitoring function to work with new RESTAPI directly.
 *
 *
 * Handle RESTAPI GET mointoring data actions.
 * The return result or error will be in http->outbuf
 *
 * @param  query_buf    The return buf to hold constructed JSON query string
 * @param  item         The monitoring object, such as Subscription, Queue
 * @param  locale       The query locale
 * @param  list         The query required item list generated from schema and also the enties default value
 * @param  props        The pass in query filter -- from http query parameters.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * { "Action":"Subscription","User":"admin","Locale":"en_US","ResultCount":"50","StatType":"PublishedMsgsHighest","SubName":"*","TopicString":"*","ClientID":"*","MessagingPolicy":"*","SubType":"All" }
 */
int ism_monitoring_restapi_createQueryString(int procType, concat_alloc_t *query_buf, char *item, char *locale, ism_config_itemValidator_t * list, ism_prop_t *props)
{
    int i = 0;
    int rc = ISMRC_OK;
    const char *propName;
    int startlen = 0;

    //special case
	//server stat actuall call Connection
	if (item && !strcmp(item, "Server")) {
		ism_json_putBytes(query_buf, "{ \"Action\":\"Connection\",\"Option\":\"Volume\",\"User\":\"admin\",\"Locale\":\"");
		if (locale)
			ism_json_putBytes(query_buf, locale);
		else
			ism_json_putBytes(query_buf, "en_US");
		ism_json_putBytes(query_buf, "\" }");
		goto CREATEQUERY_END;
	}

    //make sure the list is not empty
    if (!list) {
	   ism_common_setError(ISMRC_PropertiesNotValid);
	   rc = ISMRC_PropertiesNotValid;
	   goto CREATEQUERY_END;
    }

    if ( procType == ISM_PROTYPE_MQC ) {
        ism_json_putBytes(query_buf, "{ \"Monitoring\":");
    }

    ism_json_putBytes(query_buf, "{ \"Action\":\"");
    ism_json_putBytes(query_buf, item);
    ism_json_putBytes(query_buf, "\",\"User\":\"admin\",\"Locale\":\"");
    if (locale)
        ism_json_putBytes(query_buf, locale);
    else
    	ism_json_putBytes(query_buf, "en_US");
    ism_json_putBytes(query_buf, "\",");

    //put specified query filter into query string first
    if (props) {
        for (i=0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
    		const char *propValue = ism_common_getStringProperty(props, propName);

    		if (startlen > 0 ) {
				ism_monitoring_bputchar(query_buf, ',');
			}
    		ism_monitoring_bputchar(query_buf, '"');
			ism_json_putEscapeBytes(query_buf, propName, (int) strlen(propName));
			if (!propValue || *propValue == '\0') {
				ism_json_putBytes(query_buf, "\":\"\"");
			} else {
				ism_json_putBytes(query_buf, "\":\"");
				ism_json_putBytes(query_buf, propValue);
				ism_monitoring_bputchar(query_buf, '"');
			}
			startlen++;
        }
    }

    for (i = 0; i < list->total; i++) {
		/* add required filed if not assigned using default value*/
		if ( list->reqd[i] == 1 && list->assigned[i] == 0 && strcasecmp(list->name[i], "UID") ) {
			/* check if default value is available */
			if (list->defv[i] != NULL ) {
				if (startlen > 0 ) {
					ism_monitoring_bputchar(query_buf, ',');
				}
				ism_monitoring_bputchar(query_buf, '"');
				ism_json_putBytes(query_buf, list->name[i]);
				ism_json_putBytes(query_buf, "\":\"");
				ism_json_putBytes(query_buf, list->defv[i]);
				ism_monitoring_bputchar(query_buf, '"');
                startlen++;
			} else {
				TRACE(5, "%s: %s is required but its default value is null.\n", __FUNCTION__, list->name[i]);
				rc = ISMRC_PropertyRequired;
				ism_common_setErrorData(rc, "%s%s", list->name[i], "null");
			}
		}
    }
    ism_json_putBytes(query_buf, " }");

    if ( procType == ISM_PROTYPE_MQC ) {
        ism_json_putBytes(query_buf, " }");
    }


CREATEQUERY_END:
    TRACE(9, "Exit %s: rc: %d\n", __FUNCTION__, rc);

    return rc;
}

/*
 *
 * Handle RESTAPI get monitoring data actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_monitoring_restapi_stateQuery(ism_http_t *http, ism_rest_api_cb callback)
{
    int32_t rc = ISMRC_OK;
    char *name = NULL;
    char *token, *nexttoken = NULL;
    char buf[1024];
    concat_alloc_t cmd_buf = { buf, sizeof(buf), 0, 0 };
    char rbuf[4096];
    concat_alloc_t out_buffer = { rbuf, sizeof(rbuf), 0, 0 };
    const char * repl[5];
    int replSize = 0;

    ism_config_itemValidator_t * reqList = NULL;
    int isGet = 0;
    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    memset(buf, 0, sizeof(buf));

    if (!http->user_path) {
		TRACE(3, "%s: user path is null\n", __FUNCTION__);
		repl[0] = "user_path";
		repl[1] = http->user_path;
		replSize = 2;
		rc = ISMRC_BadPropertyValue;
		ism_common_setErrorData(rc, "%s%s", "http user path,", "null");
		goto STATQUERY_END;
	}

    char *item = strtok_r((char *)http->user_path, "/", &nexttoken);

    if ( !item || *item == '\0' ) {
    	rc = ISMRC_BadRESTfulRequest;
    	ism_common_setErrorData(rc, "%s", http->path);
    	goto STATQUERY_END;
    } else if( strcmp(item, "Server") ) {
		/* Get list of required items from ISM_MONITORING_SCHEMA schema */
		reqList = ism_config_validate_getRequiredItemList(1, item, &rc);
		if (rc != ISMRC_OK) {
			TRACE(3, "%s: Failed to retrieve required configuration items for %s.\n", __FUNCTION__, item?item:"null");
			goto STATQUERY_END;
		}
    }

    if ( serverState == ISM_SERVER_MAINTENANCE ) {
        rc = 6209;
    	ism_common_setErrorData(rc, "%s%s", "Maintenance", item);
    	goto STATQUERY_END;
    }

    /* Loop thru the query parameters*/
    char *qparam = NULL;
    nexttoken = NULL;

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
            /* ignore all request query parameter */
            if (strncasecmp(key, "request.", 8) != 0 ) {
                TRACE(7, "%s: query parameter: key=%s, value=%s.\n", __FUNCTION__, key?key:"null", value?value:"null");
			    rc = ism_config_validate_checkItemDataType( reqList, key, value, &name, &isGet, 0, props);
			}
			if ( rc != ISMRC_OK ) {
				// Ignore unknown query parameter as requested by GUI 94059
				if (rc == ISMRC_BadAdminPropName) {
					TRACE(5, "%s: The unknown query paramter: %s with value: %s is ignored.\n", __FUNCTION__, key?key:"null", value?value:"null");
					//reset last error and rc
					rc = ISMRC_OK;
					ism_common_setError(0);
				}else {
					TRACE(3, "%s: validate monitoring query parameters failed with rc: %d\n", __FUNCTION__, rc);
					goto STATQUERY_END;
				}
			}

		}
    }

    int procType = ISM_PROTYPE_SERVER;
    if ( !strcmp(item, "DestinationMappingRule")) {
        procType = ISM_PROTYPE_MQC;
    }
    rc = ism_monitoring_restapi_createQueryString(procType, &cmd_buf, item, http->locale, reqList, props);
    if (rc != ISMRC_OK) {
        TRACE(3, "%s: Failed to construct query string.\n", __FUNCTION__);
        goto STATQUERY_END;
    }

    if ( procType == ISM_PROTYPE_MQC ) {
        /* send message to MQC process */
        TRACE(7, "Send monitoring message to MQC process: len=%d msg=%s\n", cmd_buf.used, cmd_buf.buf );
        int persistData = 0;
        rc = ism_admin_mqc_send(cmd_buf.buf, cmd_buf.used, http, callback, persistData, ISM_MQC_LAST, NULL);
        if ( rc == ISMRC_OK || rc == ISMRC_AsyncCompletion) {
            callback = NULL; //Callback will be invoked asynchronously
            rc = ISMRC_OK;
            goto STATQUERY_END;
        }
    }

    TRACE(7, "%s: Call ism_process_monitoring_action with cmd string: %s.\n", __FUNCTION__, cmd_buf.buf);
    memset(rbuf, 0, sizeof(rbuf));
    int xrc = ism_process_monitoring_action(http->transport, cmd_buf.buf, cmd_buf.len, &out_buffer, &rc);
    if (xrc) {
        int state = ism_admin_get_server_state();
        TRACE(3, "%s: query %s monitoring status error: xrc=%d rc=%d state=%s\n", __FUNCTION__, item, xrc, rc, ism_admin_getStateStr(state));
        if (rc < ISMRC_Error) {
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }
        goto STATQUERY_END;
    }
    //1502: If ISMRC_NotFound or no content, need to return empty REST payload instead of error code.
    if (rc) {
    	TRACE(5, "%s: stat %s return error code: %d.\n", __FUNCTION__, item, rc);
    	int statType = ism_monitoring_getStatType(item);
    	switch(statType) {
		case ismMON_STAT_Endpoint:
		{
			if ( rc == ISMRC_EndpointNotFound ) {
				ism_common_setError(0);
				out_buffer.used = 0;
				ism_json_putBytes(&out_buffer, "[ ]");
				rc = ISMRC_OK;
			}
			break;
		}
		case ismMON_STAT_Connection:
		{
			if (  rc == ISMRC_ConnectionNotFound  || rc == ISMRC_NotFound ) {
				ism_common_setError(0);
				out_buffer.used = 0;
				ism_json_putBytes(&out_buffer, "[ ]");
				rc = ISMRC_OK;
			}
			break;
		}
		case ismMON_STAT_Subscription:
		case ismMON_STAT_Topic:
		case ismMON_STAT_Queue:
		case ismMON_STAT_MQTTClient:
		case ismMON_STAT_Transaction:
		case ismMON_STAT_ResourceSet:
		{
			if ( rc == ISMRC_NotFound ) {
				ism_common_setError(0);
				out_buffer.used = 0;
				ism_json_putBytes(&out_buffer, "[ ]");
				rc = ISMRC_OK;
			}
			break;
		}

		case ismMON_STAT_AdvEnginePD:
		case ismMON_STAT_MemoryDetail:
		case ismMON_STAT_DestinationMappingRule:
		case ismMON_STAT_Security:
		case ismMON_STAT_Cluster:
		case ismMON_STAT_Store:
		case ismMON_STAT_HA:
		case ismMON_STAT_Memory:
		default:
			TRACE(5, "%s: %s keep error code: %d.\n", __FUNCTION__, item, rc);
			break;
    	}
		if (rc)
            goto STATQUERY_END;
    }

    //special case for Server
    //remove starting and tailing [ ]
    if (!strcmp(item, "Server")) {
		int i = 0;
		char *p = out_buffer.buf;
		while (i < out_buffer.used ){
			if (p[i] == '[' ) {
				p[i] = ' ';
				break;
			}
			if (p[i] == '{')
				break;
			i++;
		}

		p = out_buffer.buf;
		i = 1;
		while(i < out_buffer.used ) {
			if (p[out_buffer.used -i] == ']') {
				p[out_buffer.used -i] = ' ';
				break;
			}
			if (p[out_buffer.used -i] == '}')
				break;
			i++;
		}
    }

    //create return monitoring data
    ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
	ism_json_putEscapeBytes(&http->outbuf, http->version, (int) strlen(http->version));
	ism_json_putBytes(&http->outbuf, "\", \"");
	ism_json_putBytes(&http->outbuf, item);
	ism_json_putBytes(&http->outbuf, "\": ");
	//ism_json_putBytes(&http->outbuf, out_buffer.buf);
	ism_common_allocBufferCopyLen(&http->outbuf, out_buffer.buf, out_buffer.used);
	ism_json_putBytes(&http->outbuf, " }\n");

STATQUERY_END:
    if (rc ) {
   		ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	}

    ism_config_validate_freeRequiredItemList(reqList);
    if(props)  ism_common_freeProperties(props);
    if (cmd_buf.inheap	)  ism_common_freeAllocBuffer(&cmd_buf);
    if (out_buffer.inheap) ism_common_freeAllocBuffer(&out_buffer);

    if ( callback ) {
        callback(http,rc);
    }

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return ISMRC_OK;
}

#define VALUE_MODE_MEMORYDETAILS         "MemoryDetails"
// Execution modes
typedef enum tag_diagExecMode_t
{
    execMode_INVALID = 0,
    execMode_MEMORYDETAILS,
    execMode_LAST // Add new entries above this
} diagExecMode_t;

#define FUNCTION_ENTRY ">>> %s "
#define FUNCTION_EXIT  "<<< %s "

//****************************************************************************
/// @brief  Memory details
///
/// Return the memory detail information in a JSON format
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t monitoring_modeMemoryDetails(const char *mode,
                               const char *args,
                               char **pDiagnosticsOutput,
                               void *pContext,
                               size_t contextLength,
                               ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    TRACE(7, FUNCTION_ENTRY "\n", __func__);

#ifndef COMMON_MALLOC_WRAPPER
    TRACE(7, "Memory monitor disabled\n");
#else
    ism_MemoryStatistics_t memoryStats;

    rc = ism_common_getMemoryStatistics(&memoryStats);
    if (rc != OK)
    {
        ism_common_setError(rc);
    }
    else
    {
        concat_alloc_t * buf = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,85),1,sizeof(concat_alloc_t));
        ism_common_allocAllocBufferOnHeap(buf,256);
        ism_json_t * jobj = ism_json_newWriter(NULL, buf, 0, JSON_OUT_COMPACT);
        ism_json_startObject(jobj, NULL);

        ism_json_convertMemoryStatistics(jobj , &memoryStats);

        ism_engine_addDiagnostics(jobj, "Engine");

        ism_utils_addBufferPoolsDiagnostics(jobj, "BufferPools");
        ism_json_endObject(jobj);

        *pDiagnosticsOutput = buf->buf;
        buf->buf = NULL;
        ism_common_freeAllocBuffer(buf);
        ism_json_closeWriter(jobj);

        ism_common_free(ism_memory_monitoring_misc,buf);
    }
#endif

    TRACE(7, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
///
/// @brief  Request diagnostics from the entire system
///
/// Requests miscellaneous diagnostic information from the entire system.
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_monitoring_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remark If the return code is OK then diagnosticsOutput will be a string that
/// should be returned to the user and then freed using ism_monitoring_freeDiagnosticsOutput().
/// If the return code is ISMRC_AsyncCompletion then the callback function will be called
/// and if the retcode is OK, the handle will point to the string to show and then free with
/// ism_monitoring_freeDiagnosticsOutput(). This includes the engine diagnostics via ism_engine_diagnostics
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t ism_monitoring_diagnostics(
    const char *                    mode,
    const char *                    args,
    char **                         pDiagnosticsOutput,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    diagExecMode_t execMode = execMode_INVALID;

    // Always pass an empty string for NULL args
    if (args == NULL) args = "";

    // We don't trace arguments for all calls (some contain sensitive information, like passwords)
    const char *traceArgs = args;

    // No mode specified or no pointer for output - trace entry but fail
    if (mode == NULL || pDiagnosticsOutput == NULL)
    {
        TRACE(7, "%s: mode=<NULL> execMode=%d traceArgs='%s' pDiagnosticsOutput=%p pContext=%p contextLength=%lu pCallbackFn=%p\n", __func__,
                execMode, traceArgs, pDiagnosticsOutput, pContext, contextLength, pCallbackFn);
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }
    // MemoryDetails
    else if (mode[0] == VALUE_MODE_MEMORYDETAILS[0] &&
             strcmp(mode, VALUE_MODE_MEMORYDETAILS) == 0)
    {
        execMode = execMode_MEMORYDETAILS;
    }
    // Invalid request type
    else
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }

    if ( execMode != execMode_INVALID)
    {
        TRACE(7,
                FUNCTION_ENTRY "mode='%s' execMode=%d traceArgs='%s' pDiagnosticsOutput=%p pContext=%p contextLength=%lu pCallbackFn=%p\n", __func__,
                mode, execMode, traceArgs, pDiagnosticsOutput, pContext, contextLength, pCallbackFn);

        // Now actually execute the request
        switch(execMode)
        {
            case execMode_MEMORYDETAILS:
            rc = monitoring_modeMemoryDetails(
                    mode,
                    args,
                    pDiagnosticsOutput,
                    pContext, contextLength, pCallbackFn);
            break;
            default:
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            break;
        }
    }

    TRACE(7, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
///
/// @brief  Free the diagnostics output
///
/// @param     diagnosticsOutput      The diagnostics output to free
//****************************************************************************
XAPI void ism_monitoring_freeDiagnosticsOutput(char * diagnosticsOutput)
{
    ism_common_free(ism_memory_alloc_buffer,diagnosticsOutput);
}
