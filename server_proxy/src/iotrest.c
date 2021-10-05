/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

/* @file iotrest.c
 * The iotrest protocol takes in MQTT requests and sends MQTT packets to a MessageSight
 * server over the MqtProxyProtocol.
 * For publish, iotrest creates a set of connections (on the order of the number of IOP
 * threads in the server). The connection to use is selected by
 *
 * For get, iotrest creates a connection for each HTTP connection which has done a GET.
 * The clientID is derived from the HTTP username.
 */
#define TRACE_COMP Http

#include <iotrest.h>
#include <selector.h>
#ifndef NO_PXACT
#include <pxactivity.h>
#endif
#include <pxtransport.h>
#include <pxtcp.h>
#include <imacontent.h>
#include <alloca.h>
#include <pxmqtt.h>
#include <protoex.h>
#include <tenant.h>
#include <throttle.h>
#include <auth.h>
#include <ismregex.h>

#define PostEvent          1
#define PostCommand        2
#define PostCommandRequest 3

#define TYPE_WILDCARD    0x01
#define DEVICE_WILDCARD  0x02
#define COMMAND_WILDCARD 0x04
#define CONTENT_TYPE_TEXT 0x10
#define CONTENT_TYPE_JSON 0x20
#define CONTENT_TYPE_BIN  0x40
#define CONTENT_TYPE_XML  0x80

enum pxaction_e {
    Action_message                = 1,
    Action_reply                  = 9
};

/*
 * Protocol states
 */
#define IOTREST_NEW            0
#define IOTREST_IN_PROGRESS    1
#define IOTREST_CONNECTED      3
#define IOTREST_DISCONNECTED   4

xUNUSED static int iotrest_unit_test = 0;
xUNUSED static int iotrest_unit_test_last_http_rc = 0;
xUNUSED static int iotrest_unit_test_last_wait_time_secs = 0;

extern int g_AAAEnabled;
static int g_HTTPOutboundEnabled = 0;
extern uint64_t g_gwCleanupTime;

px_http_stats_t httpStats = {};
static px_msgsize_stats_t httpMsgSizeStats = {0};

static int  CMD_SEND_AUTH_MASK = 0;
static int  EVT_SEND_AUTH_MASK = 0;

/*
 * Action structure for async return
 */
typedef struct iotrest_action_t {
    uint8_t        action;
    uint8_t        flags;
    uint32_t       value;
    ism_http_t *   http;
    ism_transport_t * transport;
} iotrest_action_t;

/*
 * The iot rest protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    volatile uint32_t   inprogress;
    volatile int        closed;             /* Connection is not is use */
    volatile int        subID;             /* Subscription ID */
    pthread_spinlock_t  lock;
    pthread_spinlock_t  sessionlock;
    ismHashMap *        knownDevsMap;
    uint8_t             kind;
    volatile uint8_t    startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t             resv1[2];
    int32_t             state;
    int32_t             suspended;
    uint64_t            msgsPublished;
    uint64_t            msgsAcked;
    uint64_t            msgsBad;
} iotrest_pobj_t;

extern ism_regex_t devIDMatch;
extern int         doDevIDMatch;
extern ism_regex_t devTypeMatch;
extern int         doDevTypeMatch;

extern int      g_metering_interval;
extern uint64_t g_metering_delta;

/*
 * Forward references
 */
static int iotrestConnection(ism_transport_t * transport);
void ism_iotrest_replyCloseClient(ism_transport_t * transport);
void ism_iotrest_startMessaging(void);
xUNUSED static int getBooleanQueryProperty(const char * str, const char * name, int default_val);
const char * ism_mqtt_mqttCommand(int ix);
void ism_iotrest_doneConnection(int32_t rc, ism_transport_t * transport);

extern int ism_proxy_getAuthMask(const char * name);
extern int ism_proxy_setMsgSizeStats(px_msgsize_stats_t * msgSizeStat, int size, int originated);
extern int ism_route_mhubMessage(ism_transport_t * transport, mqtt_pmsg_t * pmsg);
int ism_hout_init(void);

/*
 * Get iotrest inprogress from pobj
 */
int ism_iotrest_getInprogress(ism_transport_t *transport) {
	return ((iotrest_pobj_t*)transport->pobj)->inprogress;
}


/**
 * Enable or disable HTTP Messaging (Outbound)
 */
void ism_iotrest_setHTTPOutboundEnable(int enabled) {
	g_HTTPOutboundEnabled = enabled;
	TRACE(5, "g_HTTPOutboundEnabled=%d\n", g_HTTPOutboundEnabled);
}


/*
 * Put one character to a concat buf
 * TODO: this function was copied from pxmqtt.c.  It should be a common function.
 */
static inline void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/*
 * Initialize the iot_rest protocol
 */
int ism_iotrest_init(int step) {
    int mask = 0;
    switch(step) {
    case 1:
        g_HTTPOutboundEnabled = ism_common_getIntConfig("HTTPOutboundEnabled", 1);
        TRACE(3,"ism_iotrest_init: HTTPOutboundEnabled = %u\n", g_HTTPOutboundEnabled);
        ism_transport_registerProtocol(iotrestConnection);
        break;
    case 2:
        mask = ism_proxy_getAuthMask("cmd_send");
        if(mask != -1) {
            CMD_SEND_AUTH_MASK = mask;
        }
        mask = ism_proxy_getAuthMask("event_send");
        if(mask != -1) {
            EVT_SEND_AUTH_MASK = mask;
        }
        break;
    default:
        break;
    }
    ism_hout_init();
    return 0;
}


/*
 * Start messaging for the iot_rest protocol
 */
void ism_iotrest_startMessaging(void) {
}


/*
 * Get a query parameter
 */
static const char * getQueryProperty(const char * str, const char * name, char * buf, int buflen) {
    char * search;
    const char * value = NULL;
    int    namelen;
    int    str_len;

    if (!name || !str)
        return NULL;

    namelen = (int)strlen(name);
    str_len = (int)strlen(str);
    if (str_len <= namelen)
        return NULL;

    if (str_len > namelen && !memcmp(str, name, namelen) && str[namelen]=='=') {
        value = str+namelen+1;
    } else {
        search = alloca(namelen+3);
        search[0] = '&';
        strcpy(search+1, name);
        search[namelen+1] = '=';
        search[namelen+2] = 0;
        value = strstr(str, search);
        if (value)
            value += namelen+2;
    }
    if (value) {
        const char * pos = strchr(value, '&');
        if (pos)
            str_len = pos-value;
        else
            str_len = strlen(value);
        if (str_len >= buflen)
            return NULL;
        memcpy(buf, value, str_len);
        buf[str_len] = 0;
        return buf;
    } else {
        return NULL;
    }
}

/*
 * Get a boolean query property
 */
static int getBooleanQueryProperty(const char * str, const char * name, int default_val) {
    char xbuf [256];
    const char * value;
    const char * vp;
    int   val;

    value = getQueryProperty(str, name, xbuf, sizeof xbuf);
    if (!value || !*value)
        return default_val;
    val = 0;
    vp = value;
    while (*vp >= '0' && *vp <= '9') {
        val = val * 10 + (*vp-'0');
    }
    if (vp > value && !*vp)
        return val != 0;
    if (strcmpi(value, "true"))
        return 1;
    if (strcmpi(value, "true"))
        return 0;
    return default_val;
}


static void updateHttpClientActivity(ism_http_t *http, int line)
{
#ifndef NO_PXACT
    if (!ism_pxactivity_is_started())
        return;

    int arc;
    ism_transport_t *ctransport = http->transport;
	ismPXACT_Client_t cl[1];

	if (!ctransport->tenant->pxactEnabled)
		return;

	if (ctransport->client_class == 'A' && ctransport->genClientID)
		return;

	memset(cl,0,sizeof(ismPXACT_Client_t));
	cl->pClientID = ctransport->clientID;
	cl->pOrgName = ctransport->tenant->name;
	cl->pDeviceType = ctransport->typeID;
	cl->pDeviceID = ctransport->deviceID;
	switch (ctransport->client_class) {
		case 'd': cl->clientType = PXACT_CLIENT_TYPE_DEVICE ; break;
		case 'a': cl->clientType = PXACT_CLIENT_TYPE_APP ; break;
		case 'A': cl->clientType = PXACT_CLIENT_TYPE_APP_SCALE ; break;
		case 'g': cl->clientType = PXACT_CLIENT_TYPE_GATEWAY ; break;
		case 'c': cl->clientType = PXACT_CLIENT_TYPE_CONNECTOR ; break;
		default : cl->clientType = PXACT_CLIENT_TYPE_OTHER ; break;
	}
	
	switch (http->subprot) {
		case PostEvent:   cl->activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT; break;
		case PostCommand: cl->activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_COMMAND; break;
		case PostCommandRequest: cl->activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_COMMAND_REQ; break;
		default: TRACE(1," Unrecognized http->subprot (=%d)\n",http->subprot); return;
	}
	
	arc = ism_pxactivity_Client_Activity(cl);
	if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
		TRACE(1, "ism_pxactivity_Client_Activity (%d) failed, rc= %d, caller=%d\n",cl->activityType,arc,line);
	}
#endif
}


/*
 * Break a path into components.
 *
 * Multiple slashes are the same as a single slash, and a leading slash is
 * ignored.
 *
 * @param path  A copy of the path which is modified by this call
 * @param parts The output array of components
 * @param partmax The size of the output array
 * @return The number of components in the path (which might be larger than partmax)
 */
int ism_http_splitPath(char * path, char * * parts, int partmax) {
    int    partcount = 0;
    char * p = path;

    memset(parts, 0, partmax*sizeof(char *));
    while (*p) {
        while (*p && *p=='/')    /* Ignore leading slashes */
            p++;
        if (*p) {
            if (partcount < partmax)
                parts[partcount] = p;
            partcount++;
            while (*p && *p!='/')
                p++;
            if (*p)
                *p++ = 0;
        }
    }
    return partcount;
}

/**
 * Create the topic and place it in the result buffer.
 * The first 2 bytes in the result buffer will be used to store the length of the topic.
 * Note that the constructed topic name is not null terminated.
 *
 * @param org Organization name
 * @param type Device type
 * @param id Device id
 * @param action HTTP action
 * @param actionName Event or command name
 * @param fmt format
 * @return length of the constructed topic string
 */
int ism_http_createTopic(concat_alloc_t * buf, const char * org, const char * type, const char *id, uint8_t action, const char *actionName, const char *fmt) {
    char * topic = alloca(8192);
    const char * actionStr = (action == PostEvent) ? "evt" : "cmd";
    int len = snprintf(topic, 8192, "iot-2/%s/type/%s/id/%s/%s/%s/fmt/%s",
            org, type, id, actionStr, actionName, fmt);
    if(len >= 8192) {
    	len += 32;
        topic = alloca(len);
        len = snprintf(topic, len, "iot-2/%s/type/%s/id/%s/%s/%s/fmt/%s",
                org, type, id, actionStr, actionName, fmt);
    }
    TRACE(9, "createPublishTopic: %s\n", topic);
    bputchar(buf, (char)(len>>8));
    bputchar(buf, (char)len);
    ism_common_allocBufferCopyLen(buf, topic, len);
    return len;
}


/*
 * Check if path components match an expected production
 *
 */
int checkParts(char * * parts, int more, int count, ...) {
    va_list valist;
    int     i;
    int     ret = 1;

    va_start(valist, count);

    for (i=0; i<count; i++) {
        char * match = va_arg(valist, char *);
        if (match && strcmp(parts[i], match)) {
            ret = 0;
            break;
        }
    }
    va_end(valist);
    if (ret) {
        if (!more && parts[i])
            ret = 0;
    }
    return ret;
}

/*
 * Restartable action processing
 */
void ism_iotrest_action(int rc, void * handle, void * xact) {
    iotrest_action_t * action = (iotrest_action_t *)xact;
    ism_http_t * http = action->http;
    ism_transport_t * transport = http->transport;
    int  httprc = 0;
    TRACEL(8, transport->trclevel, "ism_iotrest_action: connect=%u inprogress=%d\n", transport->index, transport->pobj->inprogress);

    /* TODO: */
    switch(action->action) {
    case Action_message:
        action->action = Action_reply;
        /* fall thru */

    case Action_reply:
        /*
         * Map return code
         */
        switch (rc) {
        case 0:
            httprc = 200;
            if (http->outbuf.used == 0 && http->http_op == HTTP_OP_GET)
                httprc = 204;
            break;
        case ISMRC_NotAuthorized:
            httprc = 403;
            break;
        case ISMRC_NoSubscriptions:
            httprc = 205;
            break;
        case ISMRC_NotFound:
        case ISMRC_NoMsgAvail:
            httprc = 204;
            http->outbuf.used = 0;
            break;
        case ISMRC_HTTP_BadRequest:
        case ISMRC_HTTP_NotAuthorized:
        case ISMRC_HTTP_NotFound:
            httprc = rc;
            break;
        default:
            httprc = 500;
        }
        ism_http_respond(http, httprc, NULL);

        if (__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0) {
            ism_iotrest_replyCloseClient(transport);
        }
        break;
    }
}

/*
 * Send an HTTP response.
 * Note: ism_http_respond() will free http object.
 */
void ism_http_Reply(int rc, int httpRC, ism_http_t * http, const char * content_type) {
    ism_transport_t * transport = http->transport;
    iotrest_pobj_t * pobj = (iotrest_pobj_t *) transport->pobj;

    TRACEL(8, transport->trclevel, "ism_http_Reply: subprot=%d connect=%u inprogress=%d rc=%d httpRC=%d\n",
    		http->subprot, transport->index, transport->pobj->inprogress, rc, httpRC);
    if (rc)
        ism_common_setError(rc);
    /*
     * Map return code
     */
    if (httpRC == 0) {
        switch (rc) {
        case 0:
            httpRC = 200;
            if (http->outbuf.used == 0 && http->http_op == HTTP_OP_GET)
                httpRC = 204;
            break;
        case ISMRC_NotAuthorized:
            httpRC = 403;
            break;
        case ISMRC_NoSubscriptions:
            httpRC = 205;
            break;
        case ISMRC_NotFound:
        case ISMRC_NoMsgAvail:
        case ISMRC_ClientIDReused:
            httpRC = 204;
            http->outbuf.used = 0;
            break;
        case ISMRC_ServerCapacity:
            httpRC = 503;
        break;
        case ISMRC_NotConnected:
            httpRC = 503;
        break;
        case ISMRC_MsgTooBig:
            httpRC = 413;
            break;
        case ISMRC_HTTP_BadRequest:
        case ISMRC_HTTP_NotAuthorized:
        case ISMRC_HTTP_NotFound:
        case ISMRC_HTTP_Unsupported:
            httpRC = rc;
            break;
        default:
            httpRC = 500;
        }
    }
    if (httpRC >= 400)
        http->will_close = 1;
    ism_http_respond(http, httpRC, content_type);
    int inProgress = __sync_sub_and_fetch(&pobj->inprogress, 1);
    TRACEL(8, transport->trclevel, "ism_http_Reply: connect=%u inprogress=%d rc=%d\n", transport->index, inProgress, rc);
    if (inProgress < 0) {
        ism_iotrest_replyCloseClient(transport);
    }
}

extern void ism_http_free(ism_http_t * http);

/**
 * Handle processing error
 *
 * @param rc Internal return code
 * @param httpRC HTTP response code
 * @param obj Decrement inprogress count if not NULL
 * @param http Send http reply if not NULL
 * @param transport Close transport if not NULL
 * @param reason Reason for transport closing
 */
static int iotrestHandleError(uint64_t rc, int httpRC, iotrest_pobj_t * obj, ism_http_t * http, ism_transport_t * transport, const char *reason) {

	if (iotrest_unit_test) {
		iotrest_unit_test_last_http_rc = httpRC;
		if (http) {
			ism_http_free(http);
		}
		if (obj) {
			__sync_fetch_and_sub(&obj->inprogress, 1);
		}
		return rc;
	}

	if (obj && !http) {
		__sync_fetch_and_sub(&obj->inprogress, 1);
	}
	if (http) {
		ism_http_Reply(rc, httpRC, http, NULL);
	}
	if (transport) {
		transport->close(transport, rc, 0, reason);
	}
	return rc;
}

static const char * iotrest_getContentType(uint32_t type) {
	if (type == 0)
		return NULL;
	if (type & CONTENT_TYPE_JSON) {
		return "application/json";
	}
	if (type & CONTENT_TYPE_TEXT) {
		return "text/plain;charset=utf-8";
	}
	if (type & CONTENT_TYPE_BIN) {
		return "application/octet-stream";
	}
	if (type & CONTENT_TYPE_XML) {
		return "application/xml";
	}
	return NULL;
}

/**
 * This async job will send a reply back to the HTTP client.
 * This function must be initiated after authentication or authorization callback.
 * Note: http object will be freed.
 *
 * @param transport transport object
 * @param callbackParam http object
 * @param rc ISMRC code
 */
static int httpSendReply(ism_transport_t * transport, void * callbackParam, uint64_t rc) {
	ism_http_t * http = (ism_http_t *) callbackParam;
	__sync_add_and_fetch(&httpStats.httpP2CMsgsSent, 1);
	if (rc != 0) {
		transport->pobj->msgsBad++;
	}
	const char * content_type = iotrest_getContentType(http->val2);
	ism_http_Reply(rc, 0, http, content_type);
	return 0;
}

extern int ism_server_mqttPublish(ism_http_t * http);


static void httpGetMessageCallback(ism_http_t * http, int rc, const char * reason, int close,
             int subID, const char * topic, const char * payload, int len) {
	ism_transport_t * transport = http->transport;

	int pendCommandGetRequests = __sync_sub_and_fetch(&httpStats.httpPendingGetCommandRequests, 1);

	TRACEL(8, transport->trclevel, "httpGetMessageCallback: connect=%u client=%s clientID=%s rc=%d subID=%d subprot=%d pendGetCommandRequests=%d\n",
			transport->index, transport->name, transport->clientID, rc, subID, http->subprot, pendCommandGetRequests);

	if (rc) {
    	TRACEL(7, transport->trclevel, "httpGetMessageCallback: rc=%d connect=%u subID=%d reason=%s close=%d\n",
    			rc, transport->index, subID, reason, close);
    	if (close) {
    		ism_common_setError(rc);
    		iotrestHandleError(rc, 0, transport->pobj, http, transport, reason);
    	} else {
    		ism_common_setError(rc);
    		iotrestHandleError(rc, 0, transport->pobj, http, NULL, reason);
    	}
    } else {
        if (!payload) {
        	TRACEL(7, transport->trclevel, "httpGetMessageCallback: subID=%d topic=%s payload is NULL\n", subID, topic);
        	ism_common_setError(ISMRC_Error);
        	iotrestHandleError(ISMRC_Error, 0, transport->pobj, http, transport, "Command payload is NULL");
        	return;
        }
        char * tempTopic = alloca(strlen(topic)+1);
        char * parts[16];
    	strcpy(tempTopic, topic);
    	int count = ism_http_splitPath(tempTopic, parts, 16);
    	// a command topic : iot-2/<org>/type/<type>/id/<id>/cmd/<command>/fmt/<format_string>
    	if (count != 10) {
    		TRACEL(3, transport->trclevel, "httpGetMessageCallback: connect=%u subID=%d invalid topic=%s parts count=%d\n",
    		        transport->index, subID, topic, count);
    		iotrestHandleError(ISMRC_Error, 0, transport->pobj, http, transport, "Invalid command topic");
    		return;
    	}
    	uint32_t wildcards = http->val2;
    	http->val2 = 0;
    	if (!strcmp(parts[9], "json")) {
    		http->val2 |= CONTENT_TYPE_JSON;
    	} else if (!strcmp(parts[9], "text")) {
    		http->val2 |= CONTENT_TYPE_TEXT;
    	} else if (!strcmp(parts[9], "bin")) {
    		http->val2 |= CONTENT_TYPE_BIN;
    	} else if (!strcmp(parts[9], "xml")) {
    		http->val2 |= CONTENT_TYPE_XML;
    	} else {
    		TRACEL(5, transport->trclevel, "httpGetMessageCallback: connect=%u subID=%d invalid payload format topic=%s\n", transport->index, subID, topic);
    	}

    	if (wildcards & TYPE_WILDCARD)
    		ism_http_setHeader(http, "X-deviceType", parts[3]);
    	if (wildcards & DEVICE_WILDCARD)
    		ism_http_setHeader(http, "X-deviceId", parts[5]);
    	if (wildcards & COMMAND_WILDCARD)
    		ism_http_setHeader(http, "X-commandId", parts[7]);

        ism_common_allocBufferCopyLen(&http->outbuf, payload, len);
        //Determine format
        ism_transport_submitAsyncJobRequest(transport, httpSendReply, http, rc);
    }
}

/**
 * Parse http path and subscribe
 */
static int httpSubscribe(ism_transport_t * transport, ism_http_t * http) {
	int rc;
	//iotrest_pobj_t * pobj = transport->pobj;
	char * parts[16];
	char * path = alloca(strlen(http->path)+1);
	char *filter = NULL;

	strcpy(path, http->path);
	ism_http_splitPath(path, parts, 16);
    char *devType = parts[4];
    char *devId = parts[6];
    char *commandId = parts[8];
	char tbuf[8192];
	concat_alloc_t buf = {tbuf, sizeof tbuf};
	// Subscribe to all formats
	int len = ism_http_createTopic(&buf,transport->org, devType, devId, http->subprot, commandId, "+");
	char *topic = buf.buf + (buf.used - len);
	bputchar(&buf, '\0');
	//int subID = __sync_fetch_and_add(&pobj->subID, 1);

	int timeout = (int)http->val2;
	uint32_t wildcards = 0;

    if ((devType[0] == '+') && (devType[1] == '\0')) {
    	wildcards |= TYPE_WILDCARD;
    }

    if ((devId[0] == '+') && (devId[1] == '\0')) {
    	wildcards |= DEVICE_WILDCARD;
    }

	if (wildcards && transport->has_acl) {
		/**
		 * For a wildcard subscription the filter is set to:
		 *    JMS_Topic aclcheck('g:org:type:id',3,5) is not false
		 * where the first parameter of aclcheck (in single quotes) is the ClientID of the gateway
		 */
		filter = alloca(strlen(transport->clientID) + 32);
		sprintf(filter, "JMS_Topic aclcheck('%s',3,5)", transport->clientID);
	}

	if ((commandId[0] == '+') && (commandId[1] == '\0')) {
		wildcards |= COMMAND_WILDCARD;
	}

	//Save wildcards for callback
	http->val2 = wildcards;

	int pendCommandGetRequests = __sync_add_and_fetch(&httpStats.httpPendingGetCommandRequests, 1);

	TRACEL(8, transport->trclevel, "httpSubscribe: client_class=%c subprot=%d connect=%u inprogress=%d client=%s clientID=%s subscribe subId=%d timeout=%d topic=%s filter=%s pendCommandGetRequests=%d\n",
							transport->client_class, http->subprot,
							transport->index, transport->pobj->inprogress, transport->name, transport->clientID,
							0, timeout, topic, (filter != NULL) ? filter : "NULL", pendCommandGetRequests);
	updateHttpClientActivity(http, __LINE__); //PXACT
	rc = iot_getMessage(http, 0, topic, timeout, 0, filter, httpGetMessageCallback);
	ism_common_freeAllocBuffer(&buf);
	return rc;
}

/**
 * This async job will send data.
 * This function must be initiated after authentication or authorization callback.
 *
 * @param transport transport object
 * @param callbackParam http object
 * @param rc ISMRC code
 */
static int httpSendData(ism_transport_t * transport, void * callbackParam, uint64_t rc) {
	ism_http_t * http = (ism_http_t *) callbackParam;

	if (!rc) {
		TRACEL(8, transport->trclevel, "httpSendData: client_class=%c subprot=%d connect=%u client=%s\n",
						transport->client_class, http->subprot,
						transport->index, transport->name);
        if (transport->server) {
            if (transport->client_class == 'a') {
                updateHttpClientActivity(http, __LINE__); //PXACT
                rc = ism_server_mqttPublish(http);
            } else {
                if (http->subprot == PostCommandRequest) {
                    rc = httpSubscribe(transport, http);
                } else {
                    updateHttpClientActivity(http, __LINE__); //PXACT
                    rc = ism_server_mqttPublish(http);
                }
            }
        } else {

        }
	}

	if (rc) {
		httpSendReply(transport, http, rc);
	} else {
    	/*
    	 * Will send response in ism_rest_handlePubAck()
    	 */
		transport->pobj->msgsPublished++;
		__sync_add_and_fetch(&httpStats.httpP2SMsgsSent, 1);
	}
	return 0;
}

/*
 * Get the ACLs and put in the transport object.
 */
static int httpFindACL(ism_transport_t * transport) {
	char xbuf [2048];
	concat_alloc_t buf = {xbuf, sizeof xbuf, 19};
	const char * aclKey = ism_proxy_getACLKey(transport);
	ism_acl_t * acl = ism_protocol_findACL(aclKey, 0);

	xbuf[16] = 0;
    xbuf[17] = 1;
    xbuf[18] = EXIV_EndExtension;
    if (acl) {
        ism_protocol_getACL(&buf, acl);
        ism_protocol_unlockACL(acl);
        transport->has_acl = 1;
    } else {
        transport->has_acl = 0;
    }
	return 0;
}

/*
 * Create an async Authorization job
 */
static asyncauth_t * httpCreateAsyncAuthorizationJob(ism_http_t * http, uint16_t action,
		const char * devtype, const char * devid, ism_proxy_AuthorizeCallback_t callback) {
	ism_transport_t * transport = http->transport;
	int devtypeLen = (devtype) ? (strlen(devtype) + 1) : 0;
	int devidLen = (devid) ? (strlen(devid) + 1) : 0;
	/*
	 * Allocate space for asyncauth_t, device type and device ID.
	 */
	size_t length = sizeof(asyncauth_t) + devtypeLen + devidLen;
	asyncauth_t * result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_auth,5),length);
	memset(result , 0, length);
	char * ptr = (char*)(result+1);

	if (transport->channel) {
		result->authtoken = transport->channel;
		result->authtokenLen = strlen(result->authtoken);
	}

	/*
	 * Need to get back http object in httpAuthorizeCallback
	 */
	result->bufbuf = (char*)http;
	result->transport = transport;
	result->action = action;
	result->permissions = transport->auth_permissions;

	result->callback = callback;
	result->callbackParam = result;

	if (devtypeLen) {
		memcpy(ptr, devtype, devtypeLen);
		result->devtype = ptr;
		ptr += devtypeLen;
	}

	if (devidLen) {
		memcpy(ptr, devid, devidLen);
		result->devid = ptr;
	}

    result->reqTime = (uint64_t) (ism_common_readTSC() * 1000);
    return result;
}

/**
 * Callback from authorization request.
 * Complete the publish or reply with error.
 */
static void httpAuthorizeCallback(int rc, const char * reason, void * callbackParam) {
	asyncauth_t * async = (asyncauth_t *) callbackParam;
	asyncauth_t * pendingAuthRequest = NULL;
	ism_transport_t * transport = async->transport;
	ism_http_t * http = (ism_http_t *)(async->bufbuf);

	TRACEL(7, transport->trclevel, "httpAuthorizeCallback: connect=%u rc=%u, reason=%s\n",
			transport->index, rc, (reason != NULL) ? reason : "NULL");

	__sync_sub_and_fetch(&httpStats.httpPendingAuthorizationRequests, 1);

	/*
	 * Sanity check
	 */
	if (http->transport != transport) {
		TRACEL(3, transport->trclevel, "httpAuthorizeCallback: authorization failed sanity check: connect=%u client=%s rc=%u reason=%s\n",
				transport->index, transport->name, rc, reason ? reason : "");
		return;
	}

	dev_info_t * devInfo = ism_proxy_getDeviceAuthInfo(transport->tenant ? transport->tenant->name : NULL, async->devtype, async->devid);

	pendingAuthRequest = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);

	//if (devInfo && (devInfo->pendingAuthRequest == async)) {
	if (devInfo && (pendingAuthRequest == async)) {
		uint64_t authtime = (uint64_t) (ism_common_readTSC() * 1000.0);
		int authRC, createRC;

		TRACEL(7, transport->trclevel, "httpAuthorizeCallback: connect=%u client=%s org=%s devType=%s devId=%s authRC=%d createRC=%d authTime=%llu\n",
				transport->index, transport->name, transport->org, async->devtype, async->devid, devInfo->authRC, devInfo->createRC, (ULL)devInfo->authTime);
		//devInfo->pendingAuthRequest = NULL;
		if (async->action == AUTH_AutoCreateOnly) {
			createRC = rc;
			authRC = devInfo->authRC;
		} else {
			authRC = rc;
			createRC = devInfo->createRC;
		}
		ism_proxy_setDeviceAuthComplete(devInfo, authRC, createRC, authtime, transport->name);

		while (async) {
			asyncauth_t * currAuthInfo = async;
			http = (ism_http_t *)(async->bufbuf);
			if (!rc) {
				ism_transport_submitAsyncJobRequest(transport, httpSendData, http, rc);
			} else {
		        ism_transport_submitAsyncJobRequest(transport, httpSendReply, http, rc);
			}
			async = async->next;
			ism_common_free(ism_memory_proxy_auth,currAuthInfo);
		}
	} else {
		/*
		 * Ignore old authorization request.
		 */
		while (async) {
			TRACEL(5, transport->trclevel, "httpAuthorizeCallback: Ignore old requests client_class=%c connect=%u client=%s devType=%s devId=%s\n",
						transport->client_class, transport->index, transport->name, async->devtype, async->devid);
			asyncauth_t * currAuthInfo = async;
			async = async->next;
		    ism_common_free(ism_memory_proxy_auth,currAuthInfo);
		}
	}
	ism_proxy_unlockDeviceInfo(devInfo);
}

extern int ism_proxy_checkAuthorization(asyncauth_t *asyncJob);

/**
 * Check that the gateway device can publish on behalf of the device.
 * Return 0, ISMRC_NotAuthorized or ISMRC_AsyncCompletion
 */
static int httpCheckPublishAuth(ism_http_t * http, int rcCheckACL) {
	int rc = 0;
	int asyncAction;
	ism_transport_t * transport = http->transport;
    char * parts[16];
    char * path = alloca(strlen(http->path)+1);
    strcpy(path, http->path);
    ism_http_splitPath(path, parts, 16);
    char *devtype = parts[4];
    char *devid = parts[6];

    dev_info_t * devInfo = ism_proxy_getDeviceAuthInfo(transport->tenant ? transport->tenant->name : NULL, devtype, devid);

    if (devInfo) {

    	/*
    	 * Already submitted authorization check and if no pending auth request,
    	 * return either 0 or ISMRC_NotAuthorized
    	 */
    	asyncauth_t * pendingAuthRequest = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);
    	uint64_t currTime = (uint64_t) (ism_common_readTSC() * 1000.0);
    	TRACEL(7, transport->trclevel, "httpCheckPublishAuth: connect=%d client=%s org=%s devType=%s devId=%s pendingAuthRequest=%p authRC=%d authTime=%llu currTime=%llu gwCleanupTime=%llu\n",
    	                    transport->index, transport->name, transport->org, devtype, devid,
							pendingAuthRequest, devInfo->authRC, (ULL)devInfo->authTime,
    	                    (ULL)currTime, (ULL)g_gwCleanupTime);

    	int drc = ism_proxy_checkDeviceAuth(devInfo, currTime, transport->name);
		switch (drc) {
		case -1: // cache timed out
			break;
		case 0: // already authorized according to the cache
			if (rcCheckACL) {
				// Check ACL failed before, so will authorize again
				TRACEL(7, transport->trclevel, "httpCheckPublishAuth: connect=%d client=%s org=%s devID=%s devType=%s rcCheckACL=%d authRC=%d createRC=%d checkDevAuthRC=%d\n",
							transport->index, transport->name, transport->org, devid, devtype, rcCheckACL, devInfo->authRC, devInfo->createRC, drc);
				break;
			} else {
				ism_proxy_unlockDeviceInfo(devInfo);
				return 0;
			}
		default:
			ism_proxy_unlockDeviceInfo(devInfo);
			return drc;
		}

    } else {
    	ism_proxy_setDeviceAuthInfo(transport->org, devtype, devid, &devInfo);
    	TRACEL(7, transport->trclevel, "httpCheckPublishAuth: new device connect=%u client=%s org=%s devType=%s devId=%s\n",
            transport->index, transport->name, transport->org, devtype, devid);

    }

    asyncAction = transport->has_acl ? AUTH_AutoCreateOnly : AUTH_AuthorizationAutoCreate;
    asyncauth_t * async = httpCreateAsyncAuthorizationJob(http, asyncAction, devtype, devid, httpAuthorizeCallback);


    if(devInfo){
		//asyncauth_t * currInfo = devInfo->pendingAuthRequest;
		asyncauth_t * currInfo = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);


		/*
		 * FIXME: pxmqtt.c : not sure why this value is set to -1.
		 * async->count = -1;
		 */
		if (currInfo) {
			/*
			 * There is a pending auth request already.
			 * Add current request to the end of requests list
			 */
			while (currInfo->next)
				currInfo = currInfo->next;
			currInfo->next = async;
			ism_proxy_unlockDeviceInfo(devInfo);
			return ISMRC_AsyncCompletion;
		} else {
			ism_proxy_setDeviceAuthPendingRequest(devInfo,async, transport->name);
		}
		ism_proxy_unlockDeviceInfo(devInfo);
    }


    __sync_add_and_fetch(&httpStats.httpPendingAuthorizationRequests, 1);
    if (ism_proxy_checkAuthorization(async)) {
    	rc = ISMRC_AsyncCompletion;
    }

	return rc;
}


/**
 * For devices and applications, this function will publish immediately.
 * For gateways, ACL and authorization checking will be performed.
 *
 * @param http the http object
 * @param publishNow If 0, the function will schedule an async job to send data.
 *
 * @return 0 if the message has been sent to the server.
 */
static int httpPublish(ism_http_t * http, int sendDataNow) {
	int rc = 0;
	ism_transport_t *transport = http->transport;

	if ((transport->client_class == 'a') || (transport->client_class == 'g')) {
	    char * parts[16];
	    char * path = alloca(strlen(http->path)+1);
	    strcpy(path, http->path);
	    ism_http_splitPath(path, parts, 16);
	    char *devType = parts[4];
	    char *devId = parts[6];
	    int dtLen = strlen(devType);
	    int idLen = strlen(devId);

		if (transport->has_acl) {
			int checkACL = 0;
			int rcCheckACL;
			if (http->subprot != PostCommandRequest) {
				checkACL = 1;
			} else {
				uint32_t wildcards = 0;
			    if ((devType[0] == '+') && (devType[1] == '\0')) {
			    	wildcards |= TYPE_WILDCARD;
			    }

			    if ((devId[0] == '+') && (devId[1] == '\0')) {
			    	wildcards |= DEVICE_WILDCARD;
			    }
			    if (!wildcards)
			    	checkACL = 1;
			}
			if (checkACL) {
				TRACEL(7, transport->trclevel, "httpPublish: checking ACL connect=%u user=%s client=%s devType=%s devId=%s subprot=%d\n",
							transport->index, transport->userid, transport->name, devType, devId, http->subprot);
				const char * aclKey;
				char *key = alloca(dtLen + idLen + 2);
		        memcpy(key, devType, dtLen);
		        key[dtLen] = '/';
		        memcpy(key+dtLen+1, devId, idLen+1);
		        aclKey = ism_proxy_getACLKey(transport);
		        rcCheckACL = ism_protocol_checkACL(key, aclKey);
				if (rcCheckACL != 0) {
					rcCheckACL = ISMRC_NotAuthorized;
					TRACEL(6, transport->trclevel, "httpPublish: ACL check failed connect=%u user=%s client=%s devType=%s devId=%s subprot=%d, rcCheckACL=%d\n",
								transport->index, transport->userid, transport->name, devType, devId, http->subprot, rcCheckACL);
					if (transport->auth_permissions & 0x200) {
						rc = httpCheckPublishAuth(http, rcCheckACL);
						if (rc == ISMRC_AsyncCompletion) {
							TRACEL(7, transport->trclevel, "httpPublish: submitted authorization request connect=%u inprogress=%d client=%s devType=%s, devId=%s\n",
										transport->index, transport->pobj->inprogress, transport->name, devType, devId);
							return 0;
						}
					} else {
						rc = ISMRC_NotAuthorized;
					}
				}
			}
		} else {
			/*
			 * If the gateway has no ACLs, there is no authorization for outbound.
			 * The concept is that the gateway will still only receive commands for devices which exist.
			 * For HTTP inbound we need to add the authorization and auto-registration checks.
			 */
			if ((transport->client_class == 'g') && (http->subprot == PostEvent)) {
				rc = httpCheckPublishAuth(http, 0);
				if (rc == ISMRC_AsyncCompletion) {
					TRACEL(7, transport->trclevel, "httpPublish: submitted authorization request connect=%u inprogress=%d client=%s devType=%s, devId=%s\n",
								transport->index, transport->pobj->inprogress, transport->name, devType, devId);
					return 0;
				}
			}
		}
	}

	if (!rc) {
		if (sendDataNow) {
			TRACEL(7, transport->trclevel, "httpPublish: client_class=%c subprot=%d connect=%u inprogress=%d client=%s\n",
						transport->client_class, http->subprot,
						transport->index, transport->pobj->inprogress, transport->name);
			updateHttpClientActivity(http, __LINE__); //PXACT
			rc = ism_server_mqttPublish(http);
			if (!rc) {
			       transport->pobj->msgsPublished++;
			        __sync_add_and_fetch(&httpStats.httpP2SMsgsSent, 1);
			}
		} else {
			ism_transport_submitAsyncJobRequest(transport, httpSendData, http, rc);
		}
	}

    if (rc) {
        ism_transport_submitAsyncJobRequest(transport, httpSendReply, http, rc);
    }
    return 0;
}

/*
 * Scheduled work on completion of authorization (or directly called when already authenticated)
 */
static int httpIoTAuthComplete(ism_transport_t * transport, void * callbackParam, uint64_t rc) {
    ism_http_t * http = (ism_http_t *) callbackParam;
    __sync_sub_and_fetch(&httpStats.httpPendingAuthenticationRequests, 1);
    if(g_AAAEnabled && (rc == ISMRC_OK)) {
        if(((http->subprot == PostCommand) && CMD_SEND_AUTH_MASK && ((transport->auth_permissions & CMD_SEND_AUTH_MASK) == 0)) ||
           ((http->subprot == PostEvent) && EVT_SEND_AUTH_MASK && ((transport->auth_permissions & EVT_SEND_AUTH_MASK) == 0)))     {
            TRACEL(6, transport->trclevel, "httpIoTAuthComplete: %s does not have permission to post %s auth_permissions=0x%x connect=%u client=%s\n",
                    ((transport->client_class == 'a') ? "application" : "device"),
                    ((http->subprot == PostCommand) ? "command" : "event"),  transport->auth_permissions,
                    transport->index, transport->name);
            rc = ISMRC_HTTP_NotAuthorized;
        }
    }
    if(rc) {
        ism_common_setError((int)rc);
        ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
        transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed");
        return 0;
    }
    transport->authenticated = 1;
    transport->ready = 6;
    if (g_metering_interval && transport->client_class) {
        transport->metering_time = transport->connect_time + (g_metering_delta/2);
    }
    ism_proxy_connectMsg(transport);
    TRACEL(7, transport->trclevel, "httpIoTAuthComplete: client_class=%c subprot=%d connect=%u inprogress=%d client=%s\n",
    			transport->client_class, http->subprot,
				transport->index, transport->pobj->inprogress, transport->name);

	if (transport->client_class == 'g' || transport->client_class == 'a') {
		httpFindACL(transport);
	}

    httpPublish(http, 0);
    return 0;
}

/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_iotrest_replyDoneConnection(int32_t rc, void * handle, void * vaction) {
    iotrest_action_t * action = (iotrest_action_t *) vaction;
    ism_transport_t * transport = action->transport;
    iotrest_pobj_t * pobj = transport->pobj;

    pthread_spin_destroy(&pobj->lock);
    pthread_spin_destroy(&pobj->sessionlock);
    int httpConnections = __sync_sub_and_fetch(&httpStats.httpConnections, 1);
    TRACEL(6, transport->trclevel, "ism_iotrest_replyDoneConnection: connect=%u client=%s httpConnections=%d\n",
            transport->index, transport->name, httpConnections);

    if(transport->client_class == 'd') {
        __sync_sub_and_fetch(&httpStats.httpConnectedDev, 1);
    } else if(transport->client_class == 'g') {
        __sync_sub_and_fetch(&httpStats.httpConnectedGWs, 1);
    } else {
        if((transport->client_class == 'a') || (transport->client_class == 'A')) {
            __sync_sub_and_fetch(&httpStats.httpConnectedApp, 1);
        }
    }

    /* Tell the transport we are done */
    transport->closed(transport);
}

/*
 * Session closed.  If last one, close the connection
 */
void ism_iotrest_replyCloseClient(ism_transport_t * transport) {
    iotrest_pobj_t * pobj = (iotrest_pobj_t *) transport->pobj;
    iotrest_action_t action = { 0 };
    action.transport = transport;

    TRACEL(6, transport->trclevel, "ism_iotrest_replyCloseClient: connect=%u client=%s closed=%d\n",
            transport->index, transport->name, pobj->closed);

    if (!__sync_bool_compare_and_swap(&pobj->closed, 1, 2)) {
        TRACEL(4, transport->trclevel, "ism_iotrest_replyCloseClient called more than once for: connect=%u client=%s\n",
                transport->index, transport->name);
        return;
    }

    ism_iotrest_replyDoneConnection(0, NULL, &action);
}

static void httpAuthCallback(int rc, void * callbackParam) {
    ism_http_t * http = (ism_http_t *) callbackParam;
    ism_transport_t * transport = http->transport;
    ism_throttle_setConnectReqInQ(transport->clientID,0);
    if(rc == 0) {
        /*Remove any previous failed delay object if any*/
        if(ism_throttle_isEnabled())
            ism_throttle_removeThrottleObj(transport->name);
    }
    ism_transport_submitAsyncJobRequest(transport,httpIoTAuthComplete, http, rc);
}

static const char * contentType2iotFormat(const char * contentType, ism_transport_t * transport) {
    if (contentType) {
        int  len = strlen(contentType);
        if (len>=16 && !memcmp("application/json", contentType, 16))
            return "json";
        if (len>=15 && !memcmp("application/xml", contentType, 15))
            return "xml";
        if (len>5 && !memcmp("text/", contentType, 5))
            return "text";
        if (len>=24 && !memcmp("application/octet-stream", contentType, 24))
            return "bin";
    }
    if (transport) {
        TRACEL(5, transport->trclevel, "contentType2iotFormat: unknown content type %s: connect=%u client=%s\n",
                contentType ? contentType : "", transport->index, transport->name);
    }
    return NULL;
}

/*
 * Expose for test
 */
const char * ism_proxy_contentTypeMap(const char * contentType) {
    return contentType2iotFormat(contentType, NULL);
}

static int validatePathElement(const char * element, int matchRegex, ism_regex_t regexMatch) {
    if((element == NULL) || (element[0] == '\0'))
        return 1;
    if(strchr(element, ' '))
        return 1;
    if (matchRegex && ism_regex_match(regexMatch, element)) {
        return 1;
    }
    return 0;
}

/*
 * For gateways, transport->userid contains the username from the HTTP header.
 * The format is g-orgId-typeId-devId. Whereas, the transport->clientID format is g:orgId-typeId-devId
 *
 * This function compares the tokens separated by the two delimiters.
 *
 * @param s1 String #1
 * @param delim1 Delimiter in String #1
 * @param count Expected number of delimiters in String #1
 * @param s2 String #2
 * @param delim2 Delimiter in String #2
 * @return 0 if tokens are equal
 */
int ism_iotrest_ids_cmp(const char *s1, char delim1, int count, const char *s2, char delim2) {
	int rc = 0;
	const char *p1;
	const char *p2;
	int n;

	if (count <= 0)
		return 1;

	p1 = s1;
	while (count > 0) {
		while (*p1 && (*p1 != delim1))
			p1++;
		n = p1 - s1;
		if (n == 0) {
			rc = 1;
			break;
		}
		p2 = s2 + n;
		if (*p2 != delim2) {
			rc = 1;
			break;
		}
		rc = strncmp(s1, s2, n);
		if (rc) {
			break;
		}
		s1 = p1 + 1;
		s2 = p2 + 1;
		count--;
		p1 = s1;
	}

	if (count)
		return count;

	if (!rc) {
		rc = strcmp(s1, s2);
	}
	return rc;
}

/**
 * Receive data with HTTP outbound support
 * @TODO: Once HTTP outbound has been fully tested, rename this function to iotrestReceive()
 */
/**
 * Receive data with HTTP outbound support
 * @TODO: Once HTTP outbound has been fully tested, rename this function to iotrestReceive()
 */
static int iotrestReceiveWithOutbound(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_http_t * http;
    ism_field_t  map;
    ism_field_t  body;
    ism_field_t  query;
    ism_field_t  path;
    ism_field_t  typef;
    ism_field_t  localef;
    ism_field_t  host;
    ism_field_t  f;
    char         http_op;
    char localebuf[16];
    concat_alloc_t  hdr = {data, datalen, datalen};
    const char * locale = NULL;
    iotrest_pobj_t * pobj = transport->pobj;
    int inProgress;
    char * path2;
    char * parts[16];
    int    partcount;
    char * orgID = NULL;
    int orgLen = 0;
    char errorMsg[1024];
    uint32_t wildcards = 0;

    uint8_t client_class;
    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    inProgress = __sync_fetch_and_add(&pobj->inprogress, 1);
    TRACEL(8, transport->trclevel, "iotrestReceiveWithOutbound: connect=%u client=%s inprogress=%d\n",
    		transport->index, transport->name, inProgress);
    if (inProgress) {
        if (inProgress < 0) {
        	ism_common_setError(ISMRC_Closed);
        	return iotrestHandleError(ISMRC_Closed, 0, pobj, NULL, NULL, NULL);
        }
        if (inProgress > 0) {
        	ism_common_setError(ISMRC_BadRESTfulRequest);
        	return iotrestHandleError(ISMRC_BadRESTfulRequest, 0, pobj, NULL, transport, "Pending HTTP request");
        }
    }


    /*
     * Decode the concise encoding of the http data
     */
    ism_protocol_getObjectValue(&hdr, &f);          /* connect ID, not used */
    ism_protocol_getObjectValue(&hdr, &f);          /* HTTP op */
    if (f.type == VT_Byte)
        http_op = (char)f.val.i;
    else
        http_op = 'W';
    ism_protocol_getObjectValue(&hdr, &path);       /* Path */
    if (path.type != VT_String)
        path.val.s = "";
    ism_protocol_getObjectValue(&hdr, &query);      /* Query */
    if (query.type != VT_String)
        query.val.s = NULL;
    ism_protocol_getObjectValue(&hdr, &map);        /* Headers */
    if (map.type != VT_Map)
        map.val.s = NULL;
    ism_protocol_getObjectValue(&hdr, &body);       /* Content */
    if (body.type != VT_ByteArray) {
        body.val.s = NULL;
        body.len = 0;
    }

    /*
     * If we have headers, parse the content type and locale
     */
    if (map.val.s) {
        concat_alloc_t hdrs = { map.val.s, map.len, map.len };
        if (http_op == HTTP_OP_POST) {
            ism_findPropertyName(&hdrs, "]Content-Type", &typef);
            if (typef.type != VT_String) {
                typef.type = VT_Null;
                typef.val.s = NULL;
            }
        } else {
            typef.type = VT_Null;
            typef.val.s = NULL;
        }

        ism_findPropertyName(&hdrs, "]Accept-Language", &localef);
        if (localef.type == VT_String) {
            locale = ism_http_mapLocale(localef.val.s, localebuf, sizeof localebuf);
        }
        ism_findPropertyName(&hdrs, "]Host", &host);
        if (host.type == VT_String) {
            char * ptr =  strchr(host.val.s, '.');
            orgLen = ptr-host.val.s;
            if(ptr && orgLen) {
                orgID = alloca(orgLen+1);
                memcpy(orgID, host.val.s, orgLen);
                orgID[orgLen] = 0;
            } else {
            	ism_common_setError(ISMRC_BadRESTfulRequest);
            	return iotrestHandleError(ISMRC_BadRESTfulRequest, 0, pobj, NULL, transport, "Invalid hostname");
            }
        } else {
        	ism_common_setError(ISMRC_BadRESTfulRequest);
        	return iotrestHandleError(ISMRC_BadRESTfulRequest, 0, pobj, NULL, transport, "Unkown hostname");
        }
    } else {
    	ism_common_setError(ISMRC_BadRESTfulRequest);
        return iotrestHandleError(ISMRC_BadRESTfulRequest, 0, pobj, NULL, transport, "Unknown hostname");
    }

    /*
     * Construct the HTTP object
     */
    TRACEL(9, transport->trclevel, "iotrestReceiveWithOutbound: construct http object connect=%u http_op=%c path=%s query=%s locale=%s body.len=%d datatype=%s\n",
        		transport->index,
				http_op,
				path.val.s ? path.val.s : "NULL",
				query.val.s ? query.val.s : "NULL",
				locale ? locale : "NULL",
				body.len,
				typef.val.s ? typef.val.s : "NULL");

    http = ism_http_newHttp(http_op, path.val.s, query.val.s, locale, body.val.s, body.len, typef.val.s,
            map.val.s, map.len, 8000);
    if (!http) {
    	ism_common_setError(ISMRC_AllocateError);
        return iotrestHandleError(ISMRC_AllocateError, 0, pobj, NULL, transport, "Unable to allocate HTTP object");
    }
    http->transport = transport;
    http->user_path = "";
    http->val1 = 0;
    http->val2 = 0;

    if (http_op != HTTP_OP_POST) {
    	ism_common_setError(ISMRC_HTTP_Unsupported);
    	return iotrestHandleError(ISMRC_HTTP_Unsupported, 405, pobj, http, transport, "HTTP operation is not supported");
    }

    if(body.len > transport->endpoint->maxMsgSize) {
    	TRACEL(5, transport->trclevel, "iotrestReceiveWithOutbound: connect=%u length=%u maxMsgSize=%d\n", transport->index, body.len, transport->endpoint->maxMsgSize);
    	ism_common_setError(ISMRC_MsgTooBig);
        return iotrestHandleError(ISMRC_MsgTooBig, 413, pobj, http, transport, "Message too big");
    }

    path2 = alloca(strlen(path.val.s)+1);
    strcpy(path2, path.val.s);
    TRACEL(8, transport->trclevel, "iotrestReceiveWithOutbound: connect=%u client=%s path=%s\n",
    		transport->index, transport->name, path2);
    partcount = ism_http_splitPath(path2, parts, 16);


    /**
     * Here are the path rules.
     * HTTP inbound: (part count is 9)
     *    Application:
     *      /api/v0002/application/types/{deviceType}/devices/{deviceId}/commands/{command}
     *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/events/{event}
     *    Gateway and device:
     *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/events/{event}
     * HTTP outbound: (part count is 10)
     *    Gateway and device:
     *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/commands/{command}/request
     */

    if((partcount < 9) || (partcount > 10) ||
    		strcmp(parts[0], "api") || strcmp(parts[1], "v0002") ||
            strcmp(parts[3], "types") || strcmp(parts[5],"devices") ||
    		(partcount == 10 && !strcmp(parts[2], "application")) ||
    		(partcount == 10 && (strcmp(parts[7], "commands") || strcmp(parts[9], "request")))) {
        snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
        ism_common_setError(ISMRC_HTTP_NotFound);
        return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
    }

    if (!transport->org) {
        transport->org = ism_transport_putString(transport, orgID);
    }
    if (!transport->tenant) {
        ism_proxy_getValidTenant(transport);
    }
    if (partcount > 9 && strcmp(parts[9], "request")) {
        if (!transport->server) {
            ism_common_setError(ISMRC_HTTP_NotFound);
            return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, "request not allowed");
        }
    }

    /* Check for wildcard */
    if ((parts[4][0] == '+') && (parts[4][1] == '\0')) {
    	wildcards |= TYPE_WILDCARD;
    }

    if ((parts[6][0] == '+') && (parts[6][1] == '\0')) {
    	wildcards |= DEVICE_WILDCARD;
    }

    if ((parts[8][0] == '+') && (parts[8][1] == '\0')) {
    	wildcards |= COMMAND_WILDCARD;
    }


    /* Do not validate when wildcard is in the path */
    if ( ( !(wildcards & TYPE_WILDCARD) && validatePathElement(parts[4], doDevTypeMatch, devTypeMatch) ) ||
         ( !(wildcards & DEVICE_WILDCARD) && validatePathElement(parts[6], doDevIDMatch, devIDMatch) ) ) {
    	snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
    	ism_common_setError(ISMRC_HTTP_NotFound);
        return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
    }

    if (strcmp(parts[7], "events") == 0) {
        http->subprot = PostEvent;
    } else {
        if (strcmp(parts[7], "commands") == 0) {
        	if (partcount == 10) {
        		/* Already verified (!strcmp(parts[9], "request") above */
        		http->subprot = PostCommandRequest;
        	} else {
        		http->subprot = PostCommand;
        	}
        } else {
            snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
            ism_common_setError(ISMRC_HTTP_NotFound);
            return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
        }
    }

    /* Only command subscription API supports wildcards */
    if ( (http->subprot != PostCommandRequest) && wildcards) {
    	snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
    	ism_common_setError(ISMRC_HTTP_NotFound);
    	return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
    }

    /* Only command subscription API allows 0 length content */
    if (http->subprot != PostCommandRequest) {
        if (body.len == 0) {
        	ism_common_setError(ISMRC_HTTP_BadRequest);
            return iotrestHandleError(ISMRC_HTTP_BadRequest, 400, pobj, http, transport, "Request body is empty");
        }
        /* All HTTP inbound requests must have valid Content-Type header */
        http->single_content.format = contentType2iotFormat(http->single_content.content_type, http->transport);
        if (http->single_content.format == NULL) {
        	ism_common_setError(ISMRC_HTTP_Unsupported);
        	return iotrestHandleError(ISMRC_HTTP_Unsupported, 415, pobj, http, transport, "Unsupported content type");
        }
    }

    if (http->subprot == PostCommandRequest) {
    	/**
    	 * If content is empty, the default timeout is 0.
    	 * Otherwise, the content must be a valid JSON object. Within that object
    	 * can be 0 or 1 instances of a field waitTimeSecs which must be
    	 * of type number which must be an integer and betwen 0 and 3600 (inclusive).
    	 * It is an error for any other field to exist within the object.
    	 * Any of these errors generates an http response with status code 400.
    	 */
    	int timeout = 0;
    	if (body.len > 0) {
			ism_json_parse_t parseobj = { 0 };
			ism_json_entry_t ents[10];
			char *source = alloca(body.len + 1);
			memcpy(source, body.val.s, body.len);
			source[body.len] = 0;


			parseobj.ent = ents;
			parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
			parseobj.source = source;
			parseobj.src_len = body.len;
			ism_json_parse(&parseobj);

			if (parseobj.rc) {
				ism_common_setError(ISMRC_HTTP_BadRequest);
				return iotrestHandleError(ISMRC_HTTP_BadRequest, 400, pobj, http, transport, "Invalid request body (invalid JSON)");
			}

			if (parseobj.ent_count > 2) {
				ism_common_setError(ISMRC_HTTP_BadRequest);
				return iotrestHandleError(ISMRC_HTTP_BadRequest, 400, pobj, http, transport, "Invalid request body (too many entries)");
			}

			if (parseobj.ent_count == 2) {
				timeout = ism_json_getInteger(&parseobj, "waitTimeSecs", -1);
				if (timeout < 0 || timeout > 60 * 60) {
					ism_common_setError(ISMRC_HTTP_BadRequest);
					return iotrestHandleError(ISMRC_HTTP_BadRequest, 400, pobj, http, transport, "Invalid timeout value");
				}
			}
    	} else {
    		TRACEL(9, transport->trclevel, "iotrestReceiveWithOutbound: connect=%u name=%s PostCommandRequest no content 0 length.\n",
    				transport->index, transport->name);
    	}

    	if (iotrest_unit_test) {
    		iotrest_unit_test_last_wait_time_secs = timeout;
    	}

        http->val2 = (uint32_t)(timeout * 1000);
    } else {
    	/* Only command subscription API allows 0 length content */
        if (body.len == 0) {
        	ism_common_setError(ISMRC_HTTP_BadRequest);
            return iotrestHandleError(ISMRC_HTTP_BadRequest, 400, pobj, http, transport, "Request body is empty");
        }
        /* Only command subscription API supports wildcards */
        if (wildcards) {
        	snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
        	ism_common_setError(ISMRC_HTTP_NotFound);
        	return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
        }
    }

    if (strcmp(parts[2], "device") == 0) {
    	if (transport->authenticated) {
    		/* Already authenticated, client_class is either 'g' or 'd' */
    		client_class = transport->client_class;
    	} else {
    		/**
    		 * For devices, userid must be "use-token-auth"
    		 * For gateways, userid must be in this format: "g/{org}/{type}/{id}" or "g-{org}-{type}-{id}"
    		 */
    		if (transport->userid && (*(transport->userid) == 'g')) {
    			client_class = 'g';
    		} else {
    			client_class = 'd';
    		}
    	}
    } else {
        if(strcmp(parts[2], "application") == 0) {
            client_class = 'a';
        } else {
            snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
            ism_common_setError(ISMRC_HTTP_NotFound);
            return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
        }
    }

    if (client_class == 'd' && (http->subprot == PostCommandRequest) && (wildcards & (TYPE_WILDCARD | DEVICE_WILDCARD) )) {
    	snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
    	ism_common_setError(ISMRC_HTTP_NotFound);
        return iotrestHandleError(ISMRC_HTTP_NotFound, 404, pobj, http, transport, errorMsg);
    }

    if (iotrest_unit_test) {
    	iotrest_unit_test_last_http_rc = 200;
        TRACE(5, "iotrestReceiveWithOutbound: Unit test is successful.\n");
        __sync_fetch_and_sub(&pobj->inprogress, 1);
    	return 0;
    }


    transport->read_msg++;
    __sync_add_and_fetch(&httpStats.httpC2PMsgsReceived, 1);

    // @TODO Should be increase messaging size statistic for for HTTP outbound?
    if ((http->subprot != PostCommandRequest) && (http->content != NULL)) {
    	ism_proxy_setMsgSizeStats(&httpMsgSizeStats,http->content->content_len, 0 );
    }

    if (!transport->authenticated) {
        char * pw;
        char * passwd;
        int    uidlen = transport->userid ? strlen(transport->userid) : 0;
        int doauth = transport->endpoint->authorder[0]==AuthType_Basic;
        transport->org = ism_transport_allocBytes(transport, orgLen+1, 0);
        strcpy((char*)transport->org, orgID);

        transport->client_class = client_class;
        if (transport->client_class == 'd' ) {
            int typeLen = strlen(parts[4]);
            int idLen = strlen(parts[6]);
            char * typeID = ism_transport_allocBytes(transport, typeLen+1, 0);
            char * devID = ism_transport_allocBytes(transport, idLen+1, 0);
            strcpy(typeID, parts[4]);
            strcpy(devID, parts[6]);
            transport->clientID = ism_transport_allocBytes(transport, idLen+typeLen+orgLen+16, 0);
            transport->deviceID = devID;
            transport->typeID = typeID;
            sprintf((char*)transport->clientID,"d:%s:%s:%s", orgID, typeID, devID);
            __sync_add_and_fetch(&httpStats.httpConnectedDev, 1);
        } else {
            if (transport->client_class == 'a') {
                __sync_add_and_fetch(&httpStats.httpConnectedApp, 1);
                if (uidlen > 0) {
                	// For HTTP applications, set client ID to api key, replace '-' with ':'
                	char *tmp;
                	int count = 0;
                	transport->clientID = ism_transport_allocBytes(transport, uidlen+1, 0);
                	tmp = (char *)memcpy((char *)transport->clientID, transport->userid, uidlen+1);
                	while ((count < 2) && ((tmp = strchr(tmp, '-')) != NULL)) {
                		*tmp = ':';
                		count++;
                	}
                } else {
                	// Let the authentication logic handle user and password not being set.
                	// Set client ID to "a:org:http_proxy"
                	transport->clientID = ism_transport_allocBytes(transport, orgLen+16, 0);
                	sprintf((char*)transport->clientID,"a:%s:http_proxy", orgID);
                }

                transport->deviceID = "http_proxy";
                transport->typeID = "application";
                transport->alt_monitor = 1;
                if (uidlen > 4) {
                    //Check that org in api key (e.g. a-org_id-a84ps90Ajs) is the same as in URL
                    int match = 0;
                    const char * ptr1 = transport->userid+2;
                    const char * ptr2 = strchr(ptr1, '-');
                    if (ptr2) {
                        int len = ptr2-ptr1;
                        char * keyOrg = alloca(len+1);
                        memcpy(keyOrg, ptr1,len);
                        keyOrg[len] = '\0';
                        match = !strcmp(keyOrg, transport->org);
                     }
                    if(!match) {
                    }
                }
            } else {
            	int rc = ISMRC_OK;
            	char * errReason = NULL;
                /*
                 * For gateways, minimum size of userid is 7 and
                 * delimiter is either '/' : g/OOOOOO/T/D
                 *                  or '-' : g-OOOOOO-T-D
                 */
                if (transport->client_class == 'g' && uidlen > 6) {
                    const char * orgPtr = transport->userid+2;
                    char * p = strchr(orgPtr, '/');
                    char gwDelimiter;
                    if (p) {
                        gwDelimiter = '/';
                    } else {
                        p = strchr(orgPtr, '-');
                        if (p) {
                            gwDelimiter = '-';
                        }
                    }
                    if (p) {
                        if (strncmp(orgPtr, orgID, orgLen)) {
                            rc = ISMRC_HTTP_BadRequest;
                            errReason = "Organization mismatch";
                        } else {
                            const char * typePtr = p+1;
                            p = strchr(typePtr, gwDelimiter);
                            if (p) {
                                int typeLen = p - typePtr;
                                const char * idPtr = p+1;
                                int idLen = strlen(idPtr);
                                if (typeLen && idLen) {
                                    char * typeID = ism_transport_allocBytes(transport, typeLen+1, 0);
                                    char * devID = ism_transport_allocBytes(transport, idLen+1, 0);
                                    int clientIdLen = idLen+typeLen+orgLen+6;
                                    strncpy(typeID, typePtr, typeLen);
                                    strcpy(devID, idPtr);
                                    transport->clientID = ism_transport_allocBytes(transport, clientIdLen, 0);
                                    transport->deviceID = devID;
                                    transport->typeID = typeID;
                                    sprintf((char*)transport->clientID,"g:%s:%s:%s", orgID, typeID, devID);
                                    __sync_add_and_fetch(&httpStats.httpConnectedGWs, 1);
                                } else {
                                    rc = ISMRC_HTTP_BadRequest;
                                    errReason = "Invalid client ID";
                                }
                            } else {
                                rc = ISMRC_HTTP_BadRequest;
                                errReason = "Invalid client ID";
                            }
                        }
                    } else {
                        rc = ISMRC_HTTP_BadRequest;
                        errReason = "Invalid client ID";
                    }
                } else {
                    /* client_class is not 'g' or userid is too short */
                    rc = ISMRC_HTTP_BadRequest;
                    errReason = "Invalid client ID";
                }

            	if (rc != ISMRC_OK && errReason != NULL) {
            		ism_common_setError(rc);
            		ism_http_Reply(rc, 403, http, NULL);
            	    transport->close(transport, rc, 0, errReason);
            	    return rc;
            	}
            }
        }

        transport->name = transport->clientID;
        ism_tenant_t * tenant = ism_proxy_getValidTenant(transport);

        if (!tenant) {
            ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->clientID);
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "No valid tenant");
            return ISMRC_HTTP_NotAuthorized;
        }

        if (transport->userid) {
            pw = passwd = (char *)transport->userid + uidlen + 1;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        } else {
            passwd = NULL;
        }

        /* Check client certificate name match */
        if (transport->cert_name && *transport->cert_name) {
            int name_match = ism_proxy_matchCertNames(transport);
            if (name_match)
                transport->secure = name_match == 3 ? 7 : 3;
        }

        /*
         * Call async authenticate
         */
        TRACEL(7, transport->trclevel, "Authentication: submit HTTP authentication: connect=%u user=%s client=%s cert=%s required=%u\n",
                transport->index, transport->userid, transport->name, transport->cert_name, doauth);

        __sync_add_and_fetch(&httpStats.httpPendingAuthenticationRequests, 1);
        int rc = ism_proxy_doAuthenticate(transport, transport->userid, passwd, httpAuthCallback, http);
        if (passwd) {
            pw = passwd;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        }
        if(rc) {
            if(rc == ISMRC_AsyncCompletion)
                return 0;
            __sync_sub_and_fetch(&httpStats.httpPendingAuthenticationRequests, 1);
            ism_common_setError(rc);
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed");
            return ISMRC_HTTP_NotAuthorized;
        }
        httpAuthCallback(0, http);
        return 0;
    } else {
        if(strcmp(orgID, transport->org)) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different organization");
            return ISMRC_HTTP_NotAuthorized;
        }
        if(transport->client_class != client_class) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different client class");
            return ISMRC_HTTP_NotAuthorized;
        }
        if((transport->client_class == 'd') &&
           (strcmp(transport->typeID, parts[4]) || strcmp(transport->deviceID, parts[6]))) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different device");
            return ISMRC_HTTP_NotAuthorized;
        }
        if (http->subprot == PostCommandRequest) {
        	httpSubscribe(transport, http);
        } else {
        	httpPublish(http, 1);
        }

    }
    return 0;
}

/**
 * Receive data without HTTP outbound support
 * @TODO: Once HTTP outbound has been fully tested, delete this function.
 */
static int iotrestReceiveNoOutbound(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_http_t * http;
    ism_field_t  map;
    ism_field_t  body;
    ism_field_t  query;
    ism_field_t  path;
    ism_field_t  typef;
    ism_field_t  localef;
    ism_field_t  host;
    ism_field_t  f;
    char         http_op;
    char localebuf[16];
    concat_alloc_t  hdr = {data, datalen, datalen};
    const char * locale = NULL;
    iotrest_pobj_t * pobj = transport->pobj;
    int inProgress;
    char * path2;
    char * parts[16];
    int    partcount;
    char * orgID = NULL;
    int orgLen = 0;

    uint8_t client_class;
    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    inProgress = __sync_fetch_and_add(&pobj->inprogress, 1);
    TRACEL(9, transport->trclevel, "iotrestReceive: connect=%u client=%s inprogress=%d\n",
    		transport->index, transport->name, inProgress);
    if (inProgress) {
        if (inProgress < 0) {
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            ism_common_setError(ISMRC_Closed);
            return ISMRC_Closed;
        }
        if (inProgress > 0) {
            ism_common_setError(ISMRC_BadRESTfulRequest);
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            transport->close(transport, ISMRC_BadRESTfulRequest, 0, "Pending HTTP request");
            //Another request is still in progress
            return ISMRC_BadRESTfulRequest;
        }
    }


    /*
     * Decode the concise encoding of the http data
     */
    ism_protocol_getObjectValue(&hdr, &f);          /* connect ID, not used */
    ism_protocol_getObjectValue(&hdr, &f);          /* HTTP op */
    if (f.type == VT_Byte)
        http_op = (char)f.val.i;
    else
        http_op = 'W';
    ism_protocol_getObjectValue(&hdr, &path);       /* Path */
    if (path.type != VT_String)
        path.val.s = "";
    ism_protocol_getObjectValue(&hdr, &query);      /* Query */
    if (query.type != VT_String)
        query.val.s = NULL;
    ism_protocol_getObjectValue(&hdr, &map);        /* Headers */
    if (map.type != VT_Map)
        map.val.s = NULL;
    ism_protocol_getObjectValue(&hdr, &body);       /* Content */
    if (body.type != VT_ByteArray) {
        body.val.s = NULL;
        body.len = 0;
    }

    /*
     * If we have headers, parse the content type and locale
     */
    if (map.val.s) {
        concat_alloc_t hdrs = { map.val.s, map.len, map.len };
        if (http_op == HTTP_OP_POST) {
            ism_findPropertyName(&hdrs, "]Content-Type", &typef);
            if (typef.type != VT_String) {
                typef.type = VT_Null;
                typef.val.s = NULL;
            }
        } else {
            typef.type = VT_Null;
            typef.val.s = NULL;
        }

        ism_findPropertyName(&hdrs, "]Accept-Language", &localef);
        if (localef.type == VT_String) {
            locale = ism_http_mapLocale(localef.val.s, localebuf, sizeof localebuf);
        }
        ism_findPropertyName(&hdrs, "]Host", &host);
        if (host.type == VT_String) {
            char * ptr =  strchr(host.val.s, '.');
            orgLen = ptr-host.val.s;
            if(ptr && orgLen) {
                orgID = alloca(orgLen+1);
                memcpy(orgID, host.val.s, orgLen);
                orgID[orgLen] = 0;
            } else {
                ism_common_setError(ISMRC_BadRESTfulRequest);
                __sync_fetch_and_sub(&pobj->inprogress, 1);
                transport->close(transport, ISMRC_BadRESTfulRequest, 0, "Invalid hostname");
                return ISMRC_BadRESTfulRequest;
            }
        } else {
            ism_common_setError(ISMRC_BadRESTfulRequest);
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            transport->close(transport, ISMRC_BadRESTfulRequest, 0, "Unknown hostname");
            return ISMRC_BadRESTfulRequest;
        }
    } else {
        ism_common_setError(ISMRC_BadRESTfulRequest);
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        transport->close(transport, ISMRC_BadRESTfulRequest, 0, "Unknown hostname");
        return ISMRC_BadRESTfulRequest;
    }

    /*
     * Construct the HTTP object
     */
    http = ism_http_newHttp(http_op, path.val.s, query.val.s, locale, body.val.s, body.len, typef.val.s,
            map.val.s, map.len, 8000);
    if (!http) {
        ism_common_setError(ISMRC_AllocateError);
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        transport->close(transport, ISMRC_AllocateError, 0, "Unable to allocate HTTP object");
        return ISMRC_AllocateError;
    }
    http->transport = transport;
    http->user_path = "";
    http->val1 = 0;
    http->val2 = 0;

    if (http_op != HTTP_OP_POST) {
    	ism_common_setError(ISMRC_HTTP_Unsupported);
    	ism_http_Reply(ISMRC_HTTP_Unsupported, 405, http, NULL);
    	transport->close(transport, ISMRC_HTTP_Unsupported, 0, "HTTP operation is not supported");
    	return ISMRC_HTTP_Unsupported;
    }

    if(body.len > transport->endpoint->maxMsgSize) {
        ism_http_Reply(ISMRC_MsgTooBig, 413, http, NULL);
        transport->close(transport, ISMRC_MsgTooBig, 0, "Message too big");
        return ISMRC_MsgTooBig;
    }
    if(body.len == 0) {
        ism_http_Reply(ISMRC_HTTP_BadRequest, 400, http, NULL);
        transport->close(transport, ISMRC_HTTP_BadRequest, 0, "Request body is empty");
        return ISMRC_HTTP_BadRequest;
    }

    http->single_content.format = contentType2iotFormat(http->single_content.content_type, transport);
    if(http->single_content.format == NULL) {
        ism_http_Reply(ISMRC_HTTP_Unsupported, 415, http, NULL);
        transport->close(transport, ISMRC_HTTP_Unsupported, 0, "Unsupported content type");
        return ISMRC_HTTP_Unsupported;
    }

    path2 = alloca(strlen(path.val.s)+1);
    strcpy(path2, path.val.s);
    partcount = ism_http_splitPath(path2, parts, 16);
    if((partcount != 9) || strcmp(parts[0], "api") || strcmp(parts[1], "v0002") ||
        strcmp(parts[3], "types") || strcmp(parts[5],"devices")) {
        char errorMsg[1024];
        snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
        ism_http_Reply(ISMRC_HTTP_NotFound, 404, http, NULL);
        transport->close(transport, ISMRC_HTTP_NotFound, 0, errorMsg);
        return ISMRC_HTTP_NotFound;
    }

    if(validatePathElement(parts[4], doDevTypeMatch, devTypeMatch) ||
       validatePathElement(parts[6], doDevIDMatch, devIDMatch)) {
        char errorMsg[1024];
        snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
        ism_http_Reply(ISMRC_HTTP_NotFound, 404, http, NULL);
        transport->close(transport, ISMRC_HTTP_NotFound, 0, errorMsg);
        return ISMRC_HTTP_NotFound;
    }

    if(strcmp(parts[7], "events") == 0) {
        http->subprot = PostEvent;
    } else {
        if(strcmp(parts[7], "commands") == 0) {
            http->subprot = PostCommand;
        } else {
            char errorMsg[1024];
            snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
            ism_http_Reply(ISMRC_HTTP_NotFound, 404, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotFound, 0, errorMsg);
            return ISMRC_HTTP_NotFound;
        }
    }

    if(strcmp(parts[2], "device") == 0) {
    	if (transport->authenticated) {
    		/* Already authenticated, client_class is either 'g' or 'd' */
    		client_class = transport->client_class;
    	} else {
    		/* Currently, devices and gateways are not authorized to send a command*/
    		if(http->subprot == PostCommand) {
    			ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
    			transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Device is not authorized to send a command");
    			return ISMRC_HTTP_NotAuthorized;
    		}
    		/**
    		 * For devices, userid must be "use-token-auth"
    		 * For gateways, userid must be in this format: "g/{org}/{type}/{id}" or "g-{org}-{type}-{id}"
    		 */
    		if (transport->userid && (*(transport->userid) == 'g')) {
    			client_class = 'g';
    		} else {
    			client_class = 'd';
    		}
    	}
    } else {
        if(strcmp(parts[2], "application") == 0) {
            client_class = 'a';
        } else {
            char errorMsg[1024];
            snprintf(errorMsg,sizeof(errorMsg), "Invalid path: %s", path.val.s);
            ism_http_Reply(ISMRC_HTTP_NotFound, 404, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotFound, 0, errorMsg);
            return ISMRC_HTTP_NotFound;
        }
    }
    transport->read_msg++;
   __sync_add_and_fetch(&httpStats.httpC2PMsgsReceived, 1);
   ism_proxy_setMsgSizeStats(&httpMsgSizeStats,http->content->content_len, 0 );

    if (!transport->authenticated) {
        char * pw;
        char * passwd;
        int    uidlen = transport->userid ? strlen(transport->userid) : 0;
        int doauth = transport->endpoint->authorder[0]==AuthType_Basic;
        transport->org = ism_transport_allocBytes(transport, orgLen+1, 0);
        strcpy((char*)transport->org, orgID);

        transport->client_class = client_class;
        if (transport->client_class == 'd' ) {
            int typeLen = strlen(parts[4]);
            int idLen = strlen(parts[6]);
            char * typeID = ism_transport_allocBytes(transport, typeLen+1, 0);
            char * devID = ism_transport_allocBytes(transport, idLen+1, 0);
            strcpy(typeID, parts[4]);
            strcpy(devID, parts[6]);
            transport->clientID = ism_transport_allocBytes(transport, idLen+typeLen+orgLen+16, 0);
            transport->deviceID = devID;
            transport->typeID = typeID;
            sprintf((char*)transport->clientID,"d:%s:%s:%s", orgID, typeID, devID);
            __sync_add_and_fetch(&httpStats.httpConnectedDev, 1);
        } else {
            if(transport->client_class == 'a' ) {
            	transport->clientID = ism_transport_allocBytes(transport, orgLen+16, 0);
                __sync_add_and_fetch(&httpStats.httpConnectedApp, 1);
                sprintf((char*)transport->clientID,"a:%s:http_proxy", orgID);
                transport->deviceID = "http_proxy";
                transport->typeID = "application";
                transport->alt_monitor = 1;
                if(uidlen > 4) {
                    //Check that org in api key (e.g. a-org_id-a84ps90Ajs) is the same as in URL
                    int match = 0;
                    const char * ptr1 = transport->userid+2;
                    const char * ptr2 = strchr(ptr1, '-');
                    if(ptr2) {
                        int len = ptr2-ptr1;
                        char * keyOrg = alloca(len+1);
                        memcpy(keyOrg, ptr1,len);
                        keyOrg[len] = '\0';
                        match = !strcmp(keyOrg, transport->org);
                     }
                    if(!match) {
                        ism_common_setError(ISMRC_HTTP_NotAuthorized);
                        ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
                        transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Organization mismatch");
                        return ISMRC_HTTP_NotAuthorized;
                    }
                }
            } else {
            	int rc = ISMRC_OK;
            	char * errReason = NULL;
                /*
                 * For gateways, minimum size of userid is 72 and
                 * delimiter is either '/' : g/OOOOOO/T/D
                 *                  or '-' : g-OOOOOO-T-D
                 */
                if (transport->client_class == 'g' && uidlen > 6) {
                    const char * orgPtr = transport->userid+2;
                    char * p = strchr(orgPtr, '/');
                    char gwDelimiter;
                    if (p) {
                        gwDelimiter = '/';
                    } else {
                        p = strchr(orgPtr, '-');
                        if (p) {
                            gwDelimiter = '-';
                        }
                    }
                    if (p) {
                        if (strncmp(orgPtr, orgID, orgLen)) {
                            rc = ISMRC_HTTP_BadRequest;
                            errReason = "Organization mismatch";
                        } else {
                            const char * typePtr = p+1;
                            p = strchr(typePtr, gwDelimiter);
                            if (p) {
                                int typeLen = p - typePtr;
                                const char * idPtr = p+1;
                                int idLen = strlen(idPtr);
                                if (typeLen && idLen) {
                                    char * typeID = ism_transport_allocBytes(transport, typeLen+1, 0);
                                    char * devID = ism_transport_allocBytes(transport, idLen+1, 0);
                                    strncpy(typeID, typePtr, typeLen);
                                    strcpy(devID, idPtr);
                                    transport->clientID = ism_transport_allocBytes(transport, idLen+typeLen+orgLen+6, 0);
                                    transport->deviceID = devID;
                                    transport->typeID = typeID;
                                    sprintf((char*)transport->clientID,"g:%s:%s:%s", orgID, typeID, devID);
                                    __sync_add_and_fetch(&httpStats.httpConnectedGWs, 1);
                                } else {
                                    rc = ISMRC_HTTP_BadRequest;
                                    errReason = "Invalid client ID";
                                }
                            } else {
                                rc = ISMRC_HTTP_BadRequest;
                                errReason = "Invalid client ID";
                            }
                        }
                    } else {
                        rc = ISMRC_HTTP_BadRequest;
                        errReason = "Invalid client ID";
                    }
                } else {
                    /* client_class is not 'g' or userid is too short */
                    rc = ISMRC_HTTP_BadRequest;
                    errReason = "Invalid client ID";
                }
            	if (rc != ISMRC_OK && errReason != NULL) {
            		ism_common_setError(rc);
            		ism_http_Reply(rc, 403, http, NULL);
            	    transport->close(transport, rc, 0, errReason);
            	    return rc;
            	}
            }
        }

        transport->name = transport->clientID;
        ism_tenant_t * tenant = ism_proxy_getValidTenant(transport);

        if (!tenant) {
            ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->clientID);
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "No valid tenant");
            return ISMRC_HTTP_NotAuthorized;
        }

        if (transport->userid) {
            pw = passwd = (char *)transport->userid + uidlen + 1;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        } else {
            passwd = NULL;
        }

        /* Check client certificate name match */
        if (transport->cert_name && *transport->cert_name) {
            int name_match = ism_proxy_matchCertNames(transport);
            if (name_match)
                transport->secure = name_match == 3 ? 7 : 3;
        }

        /*
         * Call async authenticate
         */
        TRACEL(7, transport->trclevel, "Authentication: submit HTTP authentication: connect=%u user=%s client=%s cert=%s required=%u\n",
                transport->index, transport->userid, transport->name, transport->cert_name, doauth);

        __sync_add_and_fetch(&httpStats.httpPendingAuthenticationRequests, 1);
        int rc = ism_proxy_doAuthenticate(transport, transport->userid, passwd, httpAuthCallback, http);
        if (passwd) {
            pw = passwd;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        }
        if(rc) {
            if(rc == ISMRC_AsyncCompletion)
                return 0;
            __sync_sub_and_fetch(&httpStats.httpPendingAuthenticationRequests, 1);
            ism_common_setError(rc);
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed");
            return ISMRC_HTTP_NotAuthorized;
        }
        httpAuthCallback(0, http);
        return 0;
    } else {
        if(strcmp(orgID, transport->org)) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different organization");
            return ISMRC_HTTP_NotAuthorized;
        }
        if(transport->client_class != client_class) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different client class");
            return ISMRC_HTTP_NotAuthorized;
        }
        if((transport->client_class == 'd') &&
           (strcmp(transport->typeID, parts[4]) || strcmp(transport->deviceID, parts[6]))) {
            ism_http_Reply(ISMRC_HTTP_NotAuthorized, 403, http, NULL);
            transport->close(transport, ISMRC_HTTP_NotAuthorized, 0, "Authentication failed- different device");
            return ISMRC_HTTP_NotAuthorized;
        }
        httpPublish(http, 1);
    }
    return 0;
}

/**
 * Receive data.
 *
 * @TODO: Once HTTP outbound has been fully tested,
 * move the content of iotrestReceiveWithOutbound() here
 */
static int iotrestReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
	if (g_HTTPOutboundEnabled) {
		return iotrestReceiveWithOutbound(transport, data, datalen, kind);
	} else {
		return iotrestReceiveNoOutbound(transport, data, datalen, kind);
	}
}


/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_iotmsg_doneConnection(int32_t rc, ism_transport_t * transport) {
    iotrest_pobj_t * pobj = (iotrest_pobj_t *) transport->pobj;
    char xbuf[256];

    if (rc == 0) {
        strcpy(xbuf, "The connection has completed normally");
    } else {
        ism_common_getErrorString(rc, xbuf, sizeof xbuf);
    }

    TRACEL(7, transport->trclevel, "close IoTRest MQTT connection: connect=%u client=%s\n",
    		transport->index, transport->name);

    /* Remove transport from ACL */
    if(transport->has_acl){
		const char * aclKey = ism_proxy_getACLKey(transport);
		ism_acl_t * acl = ism_protocol_findACL(aclKey, 0);
		if (acl) {
			TRACEL(8, transport->trclevel, "Remove connection from ACL: connect=%u name=%s rc=%u was=%p\n",
					transport->index, aclKey, rc, acl->object);
			if (acl->object == (void *)transport)
				acl->object = NULL;
			ism_protocol_unlockACL(acl);
		}
    }

    pthread_spin_destroy(&pobj->lock);

    /* Tell the transport we are done */
    transport->closed(transport);

}

/*
 * Receive a connection closing notification
 */
static int iotrestClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    iotrest_pobj_t * pobj = (iotrest_pobj_t *) transport->pobj;
    int32_t count;
    int traceLevel = 6;

    if (rc && (rc != ISMRC_ClosedByClient) && reason && !strstr(reason, "by the client")) {
    	traceLevel = 5;
    }
    TRACEL(traceLevel, transport->trclevel, "iotrestClosing: connect=%u client=%s closed=%d inprogress=%d rc=%d reason=%s msgsPublished=%llu, msgsAcked=%llu, msgsBad=%llu\n",
            transport->index, transport->name, pobj->closed, pobj->inprogress, rc, reason, (ULL)pobj->msgsPublished, (ULL)pobj->msgsAcked, (ULL)pobj->msgsBad);

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }

    /* Subtract the "in progress" indicator. If it becomes negative,
     * no actions are in progress, so it is safe to clean up protocol data
     * and close the connection. If it is non-negative, there are
     * actions in progress. The action that sets this value to 0
     * would re-invoke closing().
     */
    count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACEL(traceLevel, transport->trclevel, "ism_iotrest_closing postponed as there are %d actions/messages in progress: connect=%u client=%s\n",
                count+1, transport->index, transport->name);
        return 0;
    }

    if (transport->hout) {
        TRACEL(traceLevel, transport->trclevel, "iotrestClosing: Closing outbound connection connect=%u client=%s closed=%d inprogress=%d subID=%d rc=%d reason=%s\n",
                transport->index, transport->name, pobj->closed, pobj->inprogress, pobj->subID, rc, reason);
        pobj->subID = -1;
    	iot_doneConnection(transport, rc, reason);
    }

    ism_iotmsg_doneConnection(rc, transport);

    /* Stop message delivery, destroy session and client state */
    ism_iotrest_replyCloseClient(transport);

    return 0;
}

/*
 * Starting with a HTTP iotrest object, parse it for routing
 * TODO: use this
 */
xUNUSED static int routeMhubMessage(ism_http_t * http) {
    ism_transport_t * transport = http->transport;
    mqtt_pmsg_t pmsg = {0};
    char * parts[16];
    int  pubCount;

    char * path = alloca(strlen(http->path)+1);
    strcpy(path, http->path);
    ism_http_splitPath(path, parts, 16);
    pmsg.type = parts[4];
    pmsg.id = parts[6];
    pmsg.event = parts[8];
    pmsg.format = http->content->format;
    pmsg.payload_len = http->content->content_len;
    pmsg.payload = http->content->content;
    /* Note that the topic is not set */

    pubCount = ism_route_mhubMessage(transport, &pmsg);
    return pubCount;
}

/*
 * Initialize iotrest connection
 */
static int iotrestConnection(ism_transport_t * transport) {
    // printf("protocol=%s\n", transport->protocol);
    if (*transport->protocol == '/') {
        if (!strcmp(transport->protocol, "/api")) {
            iotrest_pobj_t * pobj = (iotrest_pobj_t *) ism_transport_allocBytes(transport, sizeof(iotrest_pobj_t), 1);
            memset((char *) pobj, 0, sizeof(iotrest_pobj_t));
            pthread_spin_init(&pobj->lock, 0);
            pthread_spin_init(&pobj->sessionlock, 0);
            transport->pobj = pobj;
            transport->receive = iotrestReceive;
            transport->closing = iotrestClosing;
            /* Make protocol constant */
            transport->protocol = "/api";
            transport->protocol_family = "iotrest";
            transport->www_auth = transport->endpoint->authorder[0]==AuthType_Basic;
            transport->ready = 1;
            transport->auth_permissions = 0xffffff;
            int httpConnections = __sync_add_and_fetch(&httpStats.httpConnections, 1);
            uint64_t httpConnectionsTotal = __sync_add_and_fetch(&httpStats.httpConnectionsTotal, 1);
            TRACEL(6, transport->trclevel, "iotrestConnection: connect=%u httpConnections=%d httpConnectionsTotal=%llu\n",
                    transport->index, httpConnections, (ULL) httpConnectionsTotal);
            return 0;
        }
    }
    return 1;
}

void ism_rest_handlePubAck(ism_http_t * httpReq, int rc, const char * reason) {
    if (httpReq) {
    	TRACEL(9, httpReq->transport->trclevel, "ism_rest_handlePubAck: connect=%u client=%s org=%s path=%s rc=%d\n",
    	       httpReq->transport->index, httpReq->transport->name, httpReq->transport->org, httpReq->path, rc);
        httpReq->transport->pobj->msgsAcked++;
        ism_http_Reply(rc, 0, httpReq, NULL);
    }
}

/**
 * Get HTTP Stats
 */
int ism_proxy_getHTTPStats(px_http_stats_t * stats) {
    memcpy(stats, &httpStats, sizeof(px_http_stats_t));
    return 0;
}

/**
 * Get HTTP Messages with Size Stats
 */
int ism_proxy_getHTTPMsgSizeStats(px_msgsize_stats_t * stats) {
    memcpy(stats, &httpMsgSizeStats, sizeof(px_msgsize_stats_t));
    return 0;
}

void test_setUnitTest(int test) {
	iotrest_unit_test = test;
}

int test_iotrestReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
	return iotrestReceive(transport, data, datalen, kind);
}

int test_getLastHttpRC(void) {
	return iotrest_unit_test_last_http_rc;
}

void test_setLastHttpRC(int rc) {
	iotrest_unit_test_last_http_rc = rc;
}

int test_getLastWaitTimeSecs(void) {
	return iotrest_unit_test_last_wait_time_secs;
}

void test_setLastWaitTimeSecs(int waitTimeSecs) {
	iotrest_unit_test_last_wait_time_secs = waitTimeSecs;
}

int test_getInprogress(ism_transport_t * transport) {
	iotrest_pobj_t * pobj = transport->pobj;
	return pobj->inprogress;
}

int test_iotrestConnection(ism_transport_t * transport) {
	return iotrestConnection(transport);
}

