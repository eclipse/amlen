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

#include <transport.h>

#define ACTION_WS_ECHO_NAMES
#define ACTION_WS_MONITORING_NAMES
#include <protocol.h>
#undef ACTION_WS_ECHO_NAMES
#undef ACTION_WS_MONITORING_NAMES
#include "imacontent.h"

#include <admin.h>
#include <config.h>
#include <monitoring.h>


static pthread_mutex_t     bcastlock;
static ism_transport_t * * bcast_list;
static int                 bcast_alloc;

/*
 * The rest protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    pthread_spinlock_t      lock;
    uint8_t                 kind;
    int8_t                  inProgress;
    uint8_t                 closed;
    uint8_t                 resv;
} ism_protobj_t;

typedef ism_protobj_t   ismRestPobj_t;

/*
 * Copy the input buffer to the output buffer, and send back.  This function is
 * used by both the echo and broadcast protocols.
 */
static int wsEchoReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    // TRACE(9, "wsReceive: protocol = %s, buffer = %s\n", transport->protocol, buf);
    transport->send(transport, buf, buflen, kind, 0);
    return 0;
}


/*
 * Send the received buffer to all broadcast connections
 */
static int wsBroadcastReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    int  i;

    pthread_mutex_lock(&bcastlock);
    for (i=0; i<bcast_alloc; i++) {
        if (bcast_list[i]) {
            bcast_list[i]->send(bcast_list[i], buf, buflen, kind, 0);
        }
    }
    pthread_mutex_unlock(&bcastlock);
    return 0;
}


/*
 * Receive and process an admin operation, then return the result.
 */
static int wsAdminReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    int rc;
    char tbuf[8192];
    concat_alloc_t output_buffer = { tbuf, sizeof(tbuf), 0, 0 };

    // TRACE(9, "wsReceive: protocol = %s, buffer = %s\n", transport->protocol, buf);
    ism_process_admin_action(transport, buf, buflen, &output_buffer, &rc);

    transport->send(transport, output_buffer.buf, output_buffer.used, kind, 0);
    if (output_buffer.inheap)
        ism_common_freeAllocBuffer(&output_buffer);

    return 0;
}


/*
 * Receive and process an admin operation, then return the result.
 */
static int wsMonitoringReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    int rc;
    char tbuf[8192];
    concat_alloc_t output_buffer = { tbuf, sizeof(tbuf), 0, 0 };
    memset(&tbuf, 0, sizeof(tbuf));

    // TRACE(9, "wsReceive: protocol = %s, buffer = %s\n", transport->protocol, buf);
    ism_process_monitoring_action(transport, buf, buflen, &output_buffer, &rc);

    transport->send(transport, output_buffer.buf, output_buffer.used, kind, 0);
    if (output_buffer.inheap)
        ism_common_freeAllocBuffer(&output_buffer);

    return 0;
}


/*
 * Receive a connection closing notification for the WS protocol.
 */
static int wsClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    transport->closed(transport);
    return 0;
}


/*
 * Receive a connection closing notification for the WS protocol.
 */
static int wsBroadcastClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    int  i;
    pthread_mutex_lock(&bcastlock);
    for (i=0; i<bcast_alloc; i++) {
        if (bcast_list[i] == transport) {
            bcast_list[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&bcastlock);
    transport->closed(transport);
    return 0;
}


/*
 * Process a new WS ibm.ism.broadcast connection.
 */
static int wsBroadcastConnection(ism_transport_t * transport) {
    int  found = 0;
    int  i;

    if (!strcmpi(transport->protocol, "broadcast.ism.ibm.com")) {
        pthread_mutex_lock(&bcastlock);
        transport->receive = wsBroadcastReceive;
        transport->closing = wsBroadcastClosing;
        transport->protocol = "broadcast.ism.ibm.com";
        transport->protocol_family = "admin";
        transport->ready = 1;

        /* Find an open slot */
        for (i=0; i<bcast_alloc; i++) {
            if (bcast_list[i] == NULL) {
                bcast_list[i] = transport;
                found = 1;
                break;
            }
        }
        /* Expand the list */
        if (!found) {
            int newsize = bcast_alloc ? bcast_alloc*4 : 20;
            bcast_list = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,1),bcast_list, newsize*sizeof(ism_transport_t *));
            if (bcast_alloc) {
                memset(bcast_list+bcast_alloc, 0, (newsize-bcast_alloc)*sizeof(ism_transport_t *));
            }
            bcast_list[bcast_alloc] = transport;
            bcast_alloc = newsize;
        }
        pthread_mutex_unlock(&bcastlock);
        return 0;
    }
    return 1;
}


/*
 * Process a new WS ibm.ism.echo connection.
 */
static int wsEchoConnection(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "echo.ism.ibm.com")) {
        transport->receive = wsEchoReceive;
        transport->closing = wsClosing;
        transport->protocol = "echo.ism.ibm.com";
        transport->protocol_family = "admin";
        transport->ready = 1;
        return 0;
    }
    return 1;
}


/*
 * Process a new WS ibm.ism.admin connection.
 */
static int wsAdminConnection(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "admin.ism.ibm.com")) {
        transport->receive = wsAdminReceive;
        transport->closing = wsClosing;
        transport->protocol = "admin.ism.ibm.com";
        transport->protocol_family = "admin";
        transport->ready = 1;
        return 0;
    }
    return 1;
}

/*
 * Process a new WS ibm.ism.admin connection.
 */
static int wsMonitoringConnection(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "monitoring.ism.ibm.com")) {
        transport->receive = wsMonitoringReceive;
        transport->closing = wsClosing;
        transport->protocol = "monitoring.ism.ibm.com";
        transport->protocol_family = "admin";
        transport->ready = 1;
        return 0;
    }
    return 1;
}


void ism_config_rest_request(ism_http_t * http, ism_rest_api_cb callback);
void ism_monitoring_rest_request(ism_http_t * http, ism_rest_api_cb callback);
void ism_rest_file_request(ism_http_t * http, ism_rest_api_cb callback);
void ism_rest_service_request(ism_http_t * http, ism_rest_api_cb callback);


#ifndef SIMPLE_TCP

#define SUBPROT_Admin      1
#define SUBPROT_Monitoring 2
#define SUBPROT_File       3
#define SUBPROT_Service    4

static void httpRestReply(ism_http_t * http, int retcode) {
	int rc = 0;
    ism_transport_t * transport = http->transport;
    ismRestPobj_t * pobj = transport->pobj;

    /* Convert the rc to an http status code */
    switch (retcode) {
		case ISMRC_OK:
		case ISMRC_VerifyTestOK:
		    rc = 200;
		    break;
		case ISMRC_AsyncCompletion:
			rc = 202;
			break;
        case ISMRC_BadAdminPropName:
        case ISMRC_ArgNotValid:
        case ISMRC_PropertyRequired:
        default:
            rc = 400;
            break;
		case ISMRC_SingltonDeleteError:
		case ISMRC_DeleteNotAllowed:
		    rc = 403;
		    break;
        case ISMRC_NotFound:
        case ISMRC_PropertyNotFound:
        case ISMRC_ObjectNotFound:
        case ISMRC_BadPropertyName:
        case 6178:
            rc = 404;
            break;
		case ISMRC_Error:
        case ISMRC_AllocateError:
        case ISMRC_ServerCapacity:
            rc = 500;
		    break;
        case ISMRC_NotImplemented:
            rc = 501;
            break;
        case ISMRC_MonDataNotAvail:
            rc = 503;
            break;
    }
    /* Sent the response */
    ism_http_respond(http, rc, NULL);

    if (__sync_sub_and_fetch(&pobj->inProgress, 1) < 0) {
    	transport->closed(transport);
    }
}


/*
 * Return from authorization
 */
static int httpRestCall(ism_transport_t * transport, void * callbackParam, uint64_t async) {
    ism_http_t * http = callbackParam;
    /*
     * Call the correct function based on the sub protocol
     */
    switch (http->subprot) {
    case SUBPROT_Admin:
        ism_config_rest_request(http, httpRestReply);
        break;
    case SUBPROT_Monitoring:
        ism_monitoring_rest_request(http, httpRestReply);
        break;
    case SUBPROT_File:
        ism_rest_file_request(http, httpRestReply);
        break;
    case SUBPROT_Service:
        ism_rest_service_request(http, httpRestReply);
        break;
    default:
        break;
    }
    return 0;
}


/*
 * Reply from asynchronous authorization request.
 * This is commonly called in one of the security worker threads, but move further
 * processing to the IOP thread.
 */
static void httpReplyAuth(int authrc, void * callbackParam) {
    ism_http_t * http = * (ism_http_t * *)callbackParam;
    ism_transport_t * transport = http->transport;
    ismRestPobj_t * pobj = transport->pobj;
    if (authrc) {
    	if(authrc != ISMRC_Closed) {
            ism_common_setError(ISMRC_HTTP_NotAuthorized);
    	}
        ism_http_respond(http, 401, NULL);
        if (__sync_sub_and_fetch(&pobj->inProgress, 1) < 0) {
        	transport->closed(transport);
        }
    } else {
        transport->authenticated = 1;    /* Do not run auth a second time */
        ism_transport_submitAsyncJobRequest(transport, httpRestCall, http, 1);
    }
}


static int restClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ismRestPobj_t * pobj = transport->pobj;
    if(__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
    	if(__sync_fetch_and_sub(&pobj->inProgress, 1) == 0)
    		transport->closed(transport);
    }

    return 0;
}

/*
 * Receive data
 */
static int restReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_http_t * http;
    ism_field_t  map;
    ism_field_t  body;
    ism_field_t  query;
    ism_field_t  path;
    ism_field_t  typef;
    ism_field_t  localef;

    ism_field_t  f;
    char         http_op;
    char localebuf[16];
    concat_alloc_t  hdr = {data, datalen, datalen};
    const char * locale = NULL;
    char * pos = NULL;
    char * subtype;
    char * version = NULL;
    int    versionlen = 0;
    int    subprot = 0;
    int    inProgress;

    ismRestPobj_t * pobj = transport->pobj;

    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    inProgress = __sync_fetch_and_add(&pobj->inProgress, 1);
    if (inProgress < 0) {
    	__sync_fetch_and_sub(&pobj->inProgress, 1);
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    if (inProgress > 0) {
        ism_common_setError(ISMRC_BadRESTfulRequest);
        __sync_fetch_and_sub(&pobj->inProgress, 1);
        transport->close(transport, ISMRC_BadRESTfulRequest, 0, "Pending HTTP request");
        //Another request is still in progress
        return ISMRC_BadRESTfulRequest;
    }

    /*
     * Decode the concise encoding of the http data
     */
    ism_protocol_getObjectValue(&hdr, &f);
    ism_protocol_getObjectValue(&hdr, &f);
    if (f.type == VT_Byte)
        http_op = (char)f.val.i;
    else
        http_op = 'W';
    ism_protocol_getObjectValue(&hdr, &path);       /* Path */
    if (path.type != VT_String)
        path.val.s = NULL;
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
        if (http_op == HTTP_OP_POST  || http_op == HTTP_OP_PUT) {
            ism_findPropertyName(&hdrs, "]Content-Type", &typef);
            if (typef.type != VT_String)
                typef.val.s = NULL;
        } else {
            typef.type = VT_Null;
            typef.val.s = NULL;
        }

        ism_findPropertyName(&hdrs, "]Accept-Language", &localef);
        if (localef.type == VT_String) {
            locale = ism_http_mapLocale(localef.val.s, localebuf, sizeof localebuf);
        }
    } else {
        typef.type = VT_Null;
        typef.val.s = NULL;
    }

    /*
     * Parse the version
     */
    if (path.val.s) {
        pos = strchr(path.val.s+1, '/');
        if (pos && pos[1]=='v') {
            version = ++pos;
            pos = strchr(version, '/');
            versionlen = pos ? (int)(pos-version) : strlen(version);
            if (versionlen >= sizeof(http->version))
                versionlen = sizeof(http->version)-1;
        } else {
            version = "v1";       /* Default version if not specified */
            versionlen = 2;
        }
    }

    /*
     * Parse the subtype
     */
    if (pos) {
        int sublen;
        subtype = pos+1;
        pos = strchr(subtype, '/');
        if (!pos)
            pos = subtype + strlen(subtype);
        sublen = pos-subtype;
        switch (sublen) {
        case 4:
            if (!memcmp(subtype, "file", 4))
                subprot = SUBPROT_File;
            break;
#ifdef COMPAT_HTTP
        case 5:
            if (!memcmp(subtype, "admin", 5))
                subprot = SUBPROT_Admin;
            break;
#endif
        case 7:
            if (!memcmp(subtype, "monitor", 7))
                subprot = SUBPROT_Monitoring;
            if (!memcmp(subtype, "service", 7))
                subprot = SUBPROT_Service;
            break;
#ifdef COMPAT_HTTP
        case 10:
            if (!memcmp(subtype, "monitoring", 10))
                subprot = SUBPROT_Monitoring;
            break;
#endif
        case 13:
            if (!memcmp(subtype, "configuration", 13))
                subprot = SUBPROT_Admin;
            break;
        }
    }

    /*
     * Construct the HTTP object
     */
    http = ism_http_newHttp(http_op, path.val.s, query.val.s, locale, body.val.s, body.len, typef.val.s,
            map.val.s, map.len, 8000);
    if (!http) {
        ism_common_setError(ISMRC_AllocateError);
        __sync_fetch_and_sub(&pobj->inProgress, 1);
        transport->close(transport, ISMRC_AllocateError, 0, "Unable to allocate HTTP object");
        return ISMRC_AllocateError;
    }
    http->transport = transport;
    http->subprot = subprot;


    if (!pos || subprot == 0) {
        ism_common_setError(ISMRC_HTTP_NotFound);
        ism_http_respond(http, 404, NULL);
        if (__sync_sub_and_fetch(&pobj->inProgress, 1) < 0) {
            transport->closed(transport);
        }
        return ISMRC_HTTP_NotFound;
    }

    /* Set protocol supported fields in the http object */
    if (versionlen) {
        memcpy(http->version, version, versionlen);
        http->version[versionlen] = 0;
    }
    http->user_path = http->path + (pos-path.val.s);


#if 0
    /*
     * multipart not allowed
     */
    if (typef.val.s && (*typef.val.s == 'm' || *typef.val.s == 'M') {
        pos = strstr(typef.val.s+11, "boundary=");
        if (pos) {
            int    boundary_len = strlen(pos+9) + 4;
            char * boundary = alloca(boundary_len + 1);
            memcpy(boundary, "\r\n--", 4);
            strcpy(boundary+4, pos+9);

        }

    }
#endif

    /*
     * Authenticate the user
     */
    if (!transport->authenticated) {
        char * pw;
        char * passwd;
        int    uidlen = transport->userid ? strlen(transport->userid) : 0;
        int    pwlen;
        int doauth = transport->listener->usePasswordAuth;
        if (transport->userid) {
            pw = passwd = (char *)transport->userid + uidlen + 1;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
            pwlen = pw-passwd;
        } else {
            passwd = NULL;
            pwlen = 0;
        }

        /*
         * Call async authenticate
         */
        TRACEL(7, transport->trclevel, "Authentication: submit HTTP authentication: connect=%u user=%s cert=%s required=%u\n",
                transport->index, transport->userid, transport->cert_name, doauth);
        ism_security_authenticate_user_async(transport->security_context,
            transport->userid, uidlen, passwd, pwlen,
            &http, sizeof http, httpReplyAuth, doauth, 0);
        if (passwd) {
            pw = passwd;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        }
    } else {
        /*
         * Continue processing
         */
        httpRestCall(transport, http, 0);
    }

    return 0;
}


/*
 * Initialize rest connection
 */
static int restAdminConnection(ism_transport_t * transport) {
    // printf("protocol=%s\n", transport->protocol);
    if (*transport->protocol == '/') {
        if (!strcmp(transport->protocol, "/ima")) {
            ism_protobj_t * pobj = (ism_protobj_t *) ism_transport_allocBytes(transport, sizeof(ism_protobj_t), 1);
            memset((char *) pobj, 0, sizeof(ism_protobj_t));
            pthread_spin_init(&pobj->lock, 0);
            transport->pobj = pobj;
            transport->receive = restReceive;
            transport->closing = restClosing;
            transport->protocol = "/ima";
            transport->protocol_family = "admin";
            transport->www_auth = transport->listener->usePasswordAuth;
            transport->ready = 1;
            return 0;
        }
    }
    return 1;
}
#endif


/*
 * Initialize the basic WS protocols
 */
int ism_protocol_initWSBasic(void) {
    pthread_mutex_init(&bcastlock, NULL);
    if (ism_common_getIntConfig("EnableInternalProtocols", 0)) {
        ism_transport_registerProtocol(NULL, wsAdminConnection);
        ism_transport_registerProtocol(NULL, wsMonitoringConnection);
        ism_transport_registerProtocol(NULL, wsEchoConnection);
        ism_transport_registerProtocol(NULL, wsBroadcastConnection);
    }
#ifndef SIMPLE_TCP
    ism_transport_registerProtocol(NULL, restAdminConnection);
#endif
    return 0;
}
