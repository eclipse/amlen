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

/* @file restconfig.c
 */
#define TRACE_COMP Http

#include <ismutil.h>
#include <pxtransport.h>
#include <tenant.h>
#include <imacontent.h>

extern int g_rc;

static int restDisconnect(ism_http_t * http);
static int restDelete(char which, const char * name, const char * name2);
static const char * getQueryProperty(const char * str, const char * name, char * buf, int buflen);
static int restPostConfig(ism_http_t * http, char * * parts, int partcount);
static int restPostConfig(ism_http_t * http, char * * parts, int partcount);
static int restPostSet(const char * name, const char * value);
void ism_proxy_addSelf(ism_acl_t * acl);
void ism_proxy_sendAllACLs(const char * aclsrc, int acllen);
int ism_proxy_deviceUpdate(ism_json_parse_t * parseobj, int where, const char * name) ;
extern const char * ism_transport_makepw(const char * data, char * buf, int len, int dir) ;
extern int ism_common_isProxy(void);
extern int ism_common_isBridge(void);
#ifdef HAS_BRIDGE
typedef struct ism_forwarder_t ism_forwarder_t;
ism_forwarder_t * ism_bridge_getForwarder(const char * name);
int ism_bridge_makeForwarder(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_bridge_getForwarderList(const char * match, ism_json_t * jobj, int json, const char * name);
void ism_bridge_getForwarderJson(ism_json_t * jobj, ism_forwarder_t * forwarder, const char * name);
typedef struct ism_connection_t ism_connection_t;
ism_connection_t * ism_bridge_getConnection(const char * name);
int ism_bridge_makeConnection(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_bridge_getConnectionList(const char * match, ism_json_t * jobj, int json, const char * name);
void ism_bridge_getConnectionJson(ism_json_t * jobj, ism_connection_t * connection, const char * name);
void ism_bridge_lock(void);
void ism_bridge_unlock(void);
void ism_bridge_getDynamicConfig(ism_json_t * jobj);
int  ism_bridge_updateDynamicConfig(int doBackup);
extern const char * g_keystore;
extern const char * g_truststore;
int ism_bridge_deleteAllConnection(const char * match, ism_json_parse_t * parseobj);
int ism_bridge_deleteAllForwarder(const char * match, ism_json_parse_t * parseobj);
int ism_bridge_setLicensedUsage(const char * lictype);
int ism_bridge_startActions(void);
extern int g_licensedUsage;
extern int g_dynamic_license;
extern int g_dynamic_loglevel;
extern int g_dynamic_tracelevel;
#endif

void ism_mhub_getKafkaConnectionJson(ism_json_t * jobj, ism_mhub_t * mhub, const char * name);
int ism_mhub_getKafkaConList(const char * match, ism_tenant_t * tenant, ism_json_t * jobj, const char * name);
ism_mhub_t * ism_mhub_findMhub(ism_tenant_t * tenant, const char * id);
int ism_proxy_getAllActiveClientIDsList(const char * match, ism_json_t * jobj, int json, const char * name) ;


typedef struct ism_protobj_t {
    int16_t            inProgress;
    volatile int        closed;             /* Connection is not is use */
    pthread_spinlock_t  lock;
} ismRestPobj_t;

/*
 * Send the HTTP response after mapping from ISMRC to HTTP status
 *
 * If needed, put out content showing the error
 */
static void httpRestReply (ism_http_t * http, int retcode) {
    int rc = 0;
    ism_transport_t * transport = http->transport;
    ismRestPobj_t * pobj = transport->pobj;

    /* Convert the rc to an http status code */
    switch (retcode) {
        case ISMRC_OK:
        case ISMRC_VerifyTestOK:
        case ISMRC_AsyncCompletion:
            if (http->outbuf.used == 0) {
                ism_common_allocBufferCopy(&http->outbuf, "{ \"Code\":\"CWLNA0000\", \"Message\": \"Success\" }\n");
                http->outbuf.used--;
            }
            rc = 200;
            break;
        case ISMRC_BadAdminPropName:
        case ISMRC_ArgNotValid:
        case ISMRC_PropertyRequired:
        case ISMRC_BadPropertyName:
        case ISMRC_BadPropertyValue:
        default:
            rc = 400;
            break;
        case ISMRC_SingltonDeleteError:
        case ISMRC_DeleteNotAllowed:
            rc = 403;
            break;
        case ISMRC_NotFound:
        case ISMRC_HTTP_NotFound:
        case ISMRC_PropertyNotFound:
        case ISMRC_ObjectNotFound:
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

    if (http->outbuf.used == 0) {
        char * ebuf = alloca(2048);
        if (!ism_common_getLastError())
            ism_common_setError(retcode);
        ism_common_formatLastError(ebuf, 2048);
        sprintf(http->outbuf.buf, "{ \"Code\":\"CWLNA%04u\", ", retcode);
        http->outbuf.used = strlen(http->outbuf.buf);
        ism_json_putBytes(&http->outbuf, "\"Message\":");
        ism_json_putString(&http->outbuf, ebuf);
        ism_common_allocBufferCopy(&http->outbuf, " }\n");
        http->outbuf.used--;
    }

    /* Only trace upto 255 characters */
    TRACEL(5, transport->trclevel, "REST response: connect=%u status=%u len=%u content=%.255s\n",
            transport->index, rc, http->outbuf.used, http->outbuf.buf);

    /* Sent the response */
    transport->endpoint->stats->count[transport->tid].write_msg++;
    if (rc >= 400)
        transport->endpoint->stats->count[transport->tid].lost_msg++;
    if (http->norespond) {      /* Testing mode */
        http->val1 = rc;        /* HTTP status code */
        http->val2 = retcode;   /* ISM return code */
    } else {
        ism_http_respond(http, rc, NULL);

        TRACEL(8, transport->trclevel, "About to close: connect=%u inProgress=%u\n", transport->index, pobj->inProgress);
        if (__sync_sub_and_fetch(&pobj->inProgress, 1) < 0) {
            transport->closed(transport);
        }
    }
}

/*
 * Split the path
 */
static int splitPath(char * path, char * * parts, int partmax) {
    int    partcount = 0;
    char * p = path;

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

/*
 * Get configuration rest APIs
 */
static int restGetConfig(ism_http_t * http, char * * parts, int partcount, concat_alloc_t * buf) {
    ism_endpoint_t * endpoint;
#ifndef NO_PROXY
    ism_tenant_t *   tenant;
    ism_server_t *   server;
#endif
    ism_user_t *     user;
#ifdef HAS_BRIDGE
    ism_forwarder_t *  forwarder;
    ism_connection_t * connection;
#endif
    int rc = 404;

    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 4, 0);

    /*
     *  /admin/config/endpoint
     */
    if (!strcmp(parts[1], "endpoint") || !strcmp(parts[1], "Endpoint")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                if (ism_common_isBridge())
                    jobj->indent = 4;
                rc = ism_transport_getEndpointList(parts[2], jobj, 1, NULL);
            } else {
                ism_tenant_lock();
                endpoint = ism_transport_getEndpoint(parts[2]);
                if (endpoint) {
                    ism_tenant_getEndpointJson(endpoint, jobj, NULL);
                    rc = 0;
                }
                ism_tenant_unlock();
            }
        }
    }
#ifndef NO_PROXY
    /*
     *   /admin/config/tenant
     */
    else if (!strcmp(parts[1], "tenant")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                rc = ism_tenant_getTenantList(parts[2], jobj, 1, NULL);
            } else {
                ism_tenant_lock();
                tenant = ism_tenant_getTenant(parts[2]);
                if (tenant) {
                    ism_tenant_getTenantJson(tenant, jobj, NULL);
                    rc = 0;
                }
                ism_tenant_unlock();
            }
        }
    }

    /*
     *  /admin/config/server
     */
    else if (!strcmp(parts[1], "server")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                rc = ism_tenant_getServerList(parts[2], jobj, 1, NULL);
            } else {
                ism_tenant_lock();
                server = ism_tenant_getServer(parts[2]);
                if (server) {
                    ism_tenant_getServerJson(server, jobj, NULL);
                    rc = 0;
                }
                ism_tenant_unlock();
            }
        }
    }
#endif
    else if (!strcmp(parts[1], "user") || !strcmp(parts[1], "User")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                if (ism_common_isBridge())
                    buf->compact = 0x30;
                rc = ism_tenant_getUserList(parts[2], NULL, jobj, 1, NULL);
            } else {
                ism_tenant_lock();
                user = ism_tenant_getUser(parts[2], NULL, 0);
                if (user) {
                    ism_tenant_getUserJson(user, jobj, NULL);
                    rc = 0;
                }
                ism_tenant_unlock();
            }
        }
    }
#ifndef HAS_BRIDGE
    else if (!strcmp(parts[1], "kafkaconnection") || !strcmp(parts[1], "KafkaConnection")) {
        if (partcount == 3 || partcount == 4) {
            if (partcount == 3)
                parts[3] = "*";
            ism_tenant_lock();
            tenant = ism_tenant_getTenant(parts[2]);
            ism_tenant_unlock();
            if (tenant) {
                if (strchr(parts[3], '*')) {
                    rc = ism_mhub_getKafkaConList(parts[3], tenant, jobj, NULL);
                } else {
                    ism_mhub_t * mhub = ism_mhub_findMhub(tenant, parts[3]);
                    if (mhub) {
                        ism_mhub_getKafkaConnectionJson(jobj, mhub, NULL);
                        rc = 0;
                    }
                }
            }
        }
    }

#endif
#ifdef HAS_BRIDGE
    /*
     *  /admin/config/forwarder
     */
    else if (!strcmp(parts[1], "forwarder") || !strcmp(parts[1], "Forwarder")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                buf->compact = 0x70;
                rc = ism_bridge_getForwarderList(parts[2], jobj, 1, NULL);
            } else {
                ism_bridge_lock();
                forwarder = ism_bridge_getForwarder(parts[2]);
                if (forwarder) {
                    buf->compact = 0x30;
                    ism_bridge_getForwarderJson(jobj, forwarder, NULL);
                    rc = 0;
                }
                ism_bridge_unlock();
            }
        }
    }

    /*
     *  /admin/config/connection
     */
    else if (!strcmp(parts[1], "connection") || !strcmp(parts[1], "Connection")) {
        if (partcount <= 3) {
            if (partcount == 2)
                parts[2] = "*";
            if (strchr(parts[2], '*')) {
                buf->compact = 0x70;
                rc = ism_bridge_getConnectionList(parts[2], jobj, 1, NULL);
            } else {
                ism_bridge_lock();
                connection = ism_bridge_getConnection(parts[2]);
                if (connection) {
                    buf->compact = 0x30;
                    ism_bridge_getConnectionJson(jobj, connection, NULL);
                    rc = 0;
                }
                ism_bridge_unlock();
            }
        }
    }
#endif
#ifndef NO_PROXY
    /*
     *  /admin/config/tuser
     */
    else if (!strcmp(parts[1], "tuser")) {
        if (partcount == 3 || partcount == 4) {
            if (partcount == 3)
                parts[3] = "*";
            ism_tenant_lock();
            tenant = ism_tenant_getTenant(parts[2]);
            if (tenant) {
                if (strchr(parts[3], '*')) {
                    ism_tenant_unlock();
                    rc = ism_tenant_getUserList(parts[3], tenant, jobj, 1, NULL);
                } else {
                    user = ism_tenant_getUser(parts[3], tenant, 1);
                    if (user) {
                        ism_tenant_getUserJson(user, jobj, NULL);
                        rc = 0;
                    }
                    ism_tenant_unlock();
                }
            } else {
            	ism_tenant_unlock();
            }
        }
    }

#endif
    return rc;
}


/*
 * Get Monitor rest APIs
 */
static int restGetMonitor(ism_http_t * http, char * * parts, int partcount, concat_alloc_t * buf) {

#ifndef NO_PROXY
    ism_tenant_t *   tenant;
#endif

    int rc = 404;
#ifndef NO_PROXY
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 4, 0);

    if (!strcmp(parts[1], "activeclients")) {
		if (partcount <= 3) {
			if (partcount == 2)
				parts[2] = "*";
			if (strchr(parts[2], '*')) {
				rc = ism_proxy_getAllActiveClientIDsList(parts[2], jobj, 1, NULL);
			}else{
				//Get active clients for an org
				ism_tenant_lock();
				tenant = ism_tenant_getTenant(parts[2]);
				if(tenant){
					ism_tenant_unlock();
					rc = ism_proxy_getAllActiveClientIDsList(parts[2], jobj, 1, NULL);
				}else{
					ism_tenant_unlock();
				}
			}
		}
	}
#endif
    return rc;
}

/*
 * Do a shutdown.  The rc has aleady been set
 * We do this in a timer so the REST call can complete
 */
static int doShutdown(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(1, "Exit proxy from REST api: rc=%d\n", g_rc);
    pid_t pid = getpid();
    kill(pid, SIGTERM);
    return 0;
}


/*
 * Get info about the proxy/bridge
 */
int ism_proxy_getInfo(concat_alloc_t * buf, const char * which) {
    int rc = 0;

    if (!strcmp(which, "all") || !strcmp(which,"All")) {
        ism_json_t xjobj = {0};
        ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 4, 0);
        ism_json_startObject(jobj, NULL);
        ism_json_putStringItem(jobj, "Version", ism_common_getVersion());
        ism_json_putStringItem(jobj, "Container", ism_common_getProcessName());
        ism_json_putStringItem(jobj, "Hostname", ism_common_getHostnameInfo());
        ism_json_putStringItem(jobj, "ServerName", ism_common_getServerName());
        char * ptf = alloca(strlen(ism_common_getPlatformInfo()) + strlen(ism_common_getKernelInfo()) + 2);
        strcpy(ptf, ism_common_getPlatformInfo());
        strcat(ptf, " ");
        strcat(ptf, ism_common_getKernelInfo());
        ism_json_putStringItem(jobj, "Platform", ptf);
        ism_json_putStringItem(jobj, "Processor", ism_common_getProcessorInfo());
        ism_json_putStringItem(jobj, "BuildLabel", ism_common_getBuildLabel());
        ism_json_endObject(jobj);
    } else if (!strcmp(which, "version")  || !strcmp(which, "Version")) {
        ism_json_putBytes(buf, ism_common_getVersion());
    } else if (!strcmp(which, "container") || !strcmp(which, "Container")) {
        ism_json_putBytes(buf, ism_common_getProcessName());
    } else if (!strcmp(which, "hostname") || !strcmp(which, "Hostname")) {
        ism_json_putBytes(buf, ism_common_getHostnameInfo());
    } else if (!strcmp(which, "servername") || !strcmp(which, "ServerName")) {
        ism_json_putBytes(buf, ism_common_getServerName());
    } else if (!strcmp(which, "platform") || !strcmp(which, "Platform")) {
        ism_json_putBytes(buf, ism_common_getPlatformInfo());
        ism_json_putBytes(buf, " ");
        ism_json_putBytes(buf, ism_common_getKernelInfo());
    } else if (!strcmp(which, "processor") || !strcmp(which, "Processor")) {
        ism_json_putBytes(buf, ism_common_getProcessorInfo());
    } else if (!strcmp(which, "buildlabel") || !strcmp(which, "BuildLabel")) {
        ism_json_putBytes(buf, ism_common_getBuildLabel());
    } else {
        rc = 404;
    }
    ism_common_allocBufferCopyLen(buf, "", 1);
    buf->used--;
    return rc;
}

#ifdef HAS_BRIDGE
/*
 * Delete a store file
 *
 * The file must be a simple file (no slashes)
 */
static int restDeleteStore(const char * path, const char * file) {
    if (!path || !file || strchr(file, '/'))
        return 1;
    char * fname = alloca(strlen(path) + strlen(file) + 2);
    strcpy(fname, path);
    strcat(fname, "/");
    strcat(fname, file);
    unlink(fname);
    return 0;
}

/*
 * Post a store file
 */
static int restPostStore(const char * path, const char * file, ism_http_t * http) {
    char * fname = alloca(strlen(path) + strlen(file) + 2);
    strcpy(fname, path);
    strcat(fname, "/");
    strcat(fname, file);
    unlink(fname);
    if (http->content->content_len >= 32 && memmem(http->content->content, http->content->content_len, "-----BEGIN", 10)) {
        FILE * f = fopen(fname, "wb");
        if (!f) {
            ism_common_setErrorData(ISMRC_FileUpdateError, "%s%d", fname, errno);
            return 400;
        }
        fwrite(http->content->content, 1, http->content->content_len, f);
        fclose(f);
    } else {
        ism_common_setError(400);
        return 400;
    }
    return 0;
}
#endif

/*
 * Get the memory statistics (similar to monitoringutil.c monitoring_modeMemoryDetails but cut down
 * and doesn't try to get the engine statistics)
 */
int restGetMemory(concat_alloc_t * buf) {

    int rc = ISMRC_NotFound;
#ifndef COMMON_MALLOC_WRAPPER
    TRACE(7, "Memory monitor disabled\n");
#else
    ism_MemoryStatistics_t memoryStats;

    rc = ism_common_getMemoryStatistics(&memoryStats);
    if (rc != 0) {
        ism_common_setError(rc);
    } else {
        ism_json_t xjobj = {0};
        ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 4, 0);

        ism_json_startObject(jobj, NULL);

        ism_json_convertMemoryStatistics(jobj,&memoryStats);
        ism_utils_addBufferPoolsDiagnostics(jobj, "BufferPools");

        ism_json_endObject(jobj); // end structure
    }
#endif
    return rc;
}

/*
 * Return from authorization
 */
int ism_proxy_httpRestCall(ism_transport_t * transport, ism_http_t * http, uint64_t async) {
    char *   path2;
    char *   parts[4];
    int      partcount;
    int      rc = 404;
    int      i;
    const char * name;
    char *   content;
    int      content_len = 0;

    char xbuf[2048];
    concat_alloc_t * buf = &http->outbuf;
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 0, JSON_OUT_COMPACT);

    /*
     * Call the correct function based on the sub protocol
     */
    ism_common_setError(0);
    path2 = alloca(strlen(http->user_path)+1);
    strcpy(path2, http->user_path);
    partcount = splitPath(path2, parts, 4);
    transport->endpoint->stats->count[transport->tid].read_msg++;

    if (http->content_count > 0) {
        content_len = http->content[0].content_len;
        content = http->content[0].content;
    } else {
        content = "";
    }
    TRACE(5, "REST request: connect=%u opt=%c path=%s len=%u content=%s\n",
            transport->index, http->http_op, http->user_path, content_len, content);

    if (partcount > 0) {
        switch (http->http_op) {
        /*
         * Handle GET
         */
        case 'G':
            /*
             *  /admin/config - Get config info
             */
            if (!strcmp(parts[0], "config") && partcount > 1) {
                rc = restGetConfig(http, parts, partcount, buf);
                if (http->user_path)
                    http->user_path = strchr(http->user_path+1, '/');
            }
            /*
			*  /admin/monitor - Get monitor items
			*/
            else if (!strcmp(parts[0], "monitor") && partcount > 1) {
			   rc = restGetMonitor(http, parts, partcount, buf);
			   if (http->user_path)
				   http->user_path = strchr(http->user_path+1, '/');
		    }
#ifdef HAS_BRIDGE
            else if (!strcmp(parts[0], "config") && partcount == 1) {
                jobj->indent = 4;
                jobj->compress = 0;
                ism_bridge_getDynamicConfig(jobj);
                rc = 0;
                if (http->user_path)
                    http->user_path = strchr(http->user_path+1, '/');
            }
#endif
            /*
             *  /admin/memory - Get memory data
             */
            else if (!strcmp(parts[0], "memory") && partcount == 1) {
                rc = restGetMemory(buf);
            }
            /*
             *  /admin/endpoint - Get endpoint statistics
             */
            else if (!strcmp(parts[0], "endpoint") || !strcmp(parts[0], "Endpoint")) {
                if (partcount <= 2) {
                    if (partcount == 1)
                        parts[1] = "*";
                    if (ism_common_isBridge()) {
                        jobj->indent = 4;
                        jobj->compress = 0;
                    }
                    rc = ism_tenant_getEndpointStats(parts[1], jobj);
                }
            }
#ifdef HAS_BRIDGE
            /*
             *  /admin/forwarder - Get forwarder statistics
             */
            else if (!strcmp(parts[0], "forwarder") || !strcmp(parts[0], "Forwarder")) {
                if (partcount <= 2) {
                    if (partcount == 1)
                        parts[1] = "*";
                    jobj->indent = 4;
                    jobj->compress = 0;
                    rc = ism_bridge_getForwarderList(parts[1], jobj, 2, NULL);
                }
            }
#endif
            /*
             *  /admin/set
             */
            else if (!strcmp(parts[0], "set")) {
                if (partcount <= 2) {
                    if (partcount == 1)
                        parts[1] = "*";
                    if (!strchr(parts[1], '*')) {
                        const char * value = ism_common_getStringConfig(parts[1]);
                        if (value) {
                            ism_json_putBytes(buf, value);
                            rc = 0;
                        }
                    } else {
                        const char * match = parts[1];
                        jobj->indent = 4;
                        jobj->compress = 0;
                        ism_json_startObject(jobj, NULL);
                        i = 0;
                        while (ism_common_getPropertyIndex(ism_common_getConfigProperties(), i, &name)==0) {
                            if (ism_common_match(name, match)) {
                                if (match[1] != 0 || name[0]!='t' || name[1]!='l' || name[2]!='s') {
                                    ism_field_t field;
                                    ism_common_getProperty(ism_common_getConfigProperties(), name, &field);
                                    if (field.type != VT_Null) {
                                        switch(field.type) {
                                        case VT_String:
                                            ism_json_putStringItem(jobj, name, field.val.s);
                                            break;
                                        case VT_Integer:
                                            ism_json_putIntegerItem(jobj, name, field.val.i);
                                            break;
                                        case VT_Long:
                                            ism_json_putLongItem(jobj, name, field.val.l);
                                            break;
                                        case VT_Boolean:
                                            ism_json_putBooleanItem(jobj, name, field.val.i);
                                            break;
                                        default:
                                            break;
                                        }
                                    }
                                }
                            }
                            i++;
                        }
                        ism_json_endObject(jobj);
                        rc = 0;
                    }
                }
            }

            /*
             *  /admin/time - Get server time
             */
            else if (!strcmp(parts[0], "time")) {
                ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                ism_common_formatTimestamp(ts, xbuf, sizeof(xbuf), 7, ISM_TFF_ISO8601);
                ism_json_putBytes(buf, xbuf);
                ism_common_closeTimestamp(ts);
                rc = 0;
            }

            /*
             *  /admin/info - Get server info
             */
            else if (!strcmp(parts[0], "info")) {
                if (partcount == 1) {
                    rc = ism_proxy_getInfo(buf, "all");
                } else if (partcount == 2) {
                    rc = ism_proxy_getInfo(buf, parts[1]);
                }
            }

            /*
             *  /admin/password/$user/$passwd
             */
            else if (!strcmp(parts[0], "password")) {
                if (partcount == 3) {
                    ism_tenant_createObfus(parts[1], parts[2], xbuf, sizeof xbuf, 1);
                    ism_json_putString(buf, parts[1]);
                    ism_json_putBytes(buf, ": { \"Password\":");
                    ism_json_putString(buf, xbuf);
                    ism_json_putBytes(buf, " },\n");
                    rc = 0;
                }
            }

            /*
             * /admin/connection/password/$passwd
             */
            else if (!strcmp(parts[0], "connection") && !strcmp(parts[1], "password") && partcount==3) {
                ism_transport_makepw(parts[2], xbuf + 1, sizeof xbuf - 1, 0);
                xbuf[0] = '!';
                ism_json_putBytes(buf, xbuf);
                rc = 0;
            }
            break;

        /*
         * Handle POST
         */
        case 'P':
            /*  /admin/config     */
            if (!strcmp(parts[0], "config") || !strcmp(parts[0], "device")) {
                if (http->content_count == 1) {
                    rc = restPostConfig(http, parts, partcount);
                    if (http->user_path)
                        http->user_path = strchr(http->user_path+1, '/');
                } else {
                    rc = 400;
                }
            }

            else if (!strcmp(parts[0], "set")) {
                if (partcount > 2) {
                    const char * value = http->user_path + (parts[2]-path2);
                    rc = restPostSet(parts[1], value);
                }
            }

#ifndef NO_PROXY
            else if (!strcmp(parts[0], "acl")) {
                if (partcount==1) {
                    if (http->content_count != 1) {
                        ism_common_setError(400);
                        return 400;
                    } else {
                        rc = ism_protocol_setACL(http->content[0].content, http->content[0].content_len, 0, ism_proxy_addSelf);
                        if (rc) {
                            ism_common_setError(400);
                            return 400;
                        }
                        ism_proxy_sendAllACLs(http->content[0].content, http->content[0].content_len);
                    }
                }
            }
#endif

            /*
             *  /admin/startmsg - Start messaging
             */
            else if (!strcmp(parts[0], "startmsg") || !strcmp(parts[0], "start")) {
                if (partcount == 1) {
                    ism_transport_startMessaging();
                    rc = 0;
                }
            }

            /*
             *  /admin/quit/rc
             */
            else if (!strcmp(parts[0], "quit")) {
                if (partcount <= 2) {
                    if (partcount > 1)
                        g_rc = atoi(parts[1]);
                    else
                        g_rc = 0;
                    ism_common_setTimerOnce(ISM_TIMER_LOW, doShutdown, NULL, 100000000);
                    rc = 0;
                }
            }
#ifdef HAS_BRIDGE
            /*
             * Implement /admin/accept/license
             */
            else if (partcount == 3 && !strcmp(parts[0], "accept") && !strcmp(parts[1], "license")) {
                int old_license = g_licensedUsage;
                if (ism_bridge_setLicensedUsage(parts[2]) != 0) {
                    rc = 400;
                } else {
                    rc = 0;
                    if (old_license != g_licensedUsage) {
                        ism_bridge_startActions();
                        ism_bridge_updateDynamicConfig(1);
                    }
                }
            }

            /*
             * Implement keystore update
             */
            else if (!strcmp(parts[0], "keystore")) {
                if (partcount == 2) {
                    if (http->content_count != 1 || !g_keystore) {
                        ism_common_setError(400);
                        return 400;
                    }
                    rc = restPostStore(g_keystore, parts[1], http);
                }
            }

            /*
             * Implement truststore update
             */
            else if (!strcmp(parts[0], "truststore")) {
                if (partcount == 2) {
                    if (http->content_count != 1 || !g_keystore) {
                        ism_common_setError(400);
                        return 400;
                    }
                    rc = restPostStore(g_truststore, parts[1], http);
                }
            }
#endif
            break;

        case 'D':
#ifndef NO_PROXY
            if (!strcmp(parts[0], "connection")) {
                if (partcount == 1) {
                    rc = restDisconnect(http);
                }
            } else if(!strcmp(parts[0], "acl")) {
                if (partcount == 2) {
                    /* TODO */
                }
            } else if(!strcmp(parts[0], "device")) {
                if (partcount == 2) {
                    /* TODO */
                }
            } else
#endif
            if (!strcmp(parts[0], "config")) {
                if (!strcmp(parts[1], "endpoint") || !strcmp(parts[2], "Endpoint")) {
                    if (partcount == 3)
                        rc = restDelete('e', parts[2], NULL);
                }
#ifndef NO_PROXY
                else if (!strcmpi(parts[1], "tenant")) {
                    if (partcount == 3)
                        rc = restDelete('t', parts[2], NULL);
                }
                else if (!strcmp(parts[1], "server")) {
                    if (partcount == 3)
                        rc = restDelete('s', parts[2], NULL);
                }
#endif
                else if (!strcmp(parts[1], "user") || !strcmp(parts[1], "User")) {
                    if (partcount == 3)
                        rc = restDelete('u', parts[2], NULL);
                }
#ifdef HAS_BRIDGE
                else if (!strcmp(parts[1], "connection") || !strcmp(parts[1], "Connection")) {
                    if (partcount == 3)
                        rc = restDelete('b', parts[2], NULL);
                }
                else if (!strcmp(parts[1], "forwarder") || !strcmp(parts[1], "Forwarder")) {
                    if (partcount == 3)
                        rc = restDelete('f', parts[2], NULL);
                }
                else if (!strcmp(parts[1], "routingrule") || !strcmp(parts[1], "RoutingRule")) {
                    if (partcount == 4)
                        rc = restDelete('r', parts[2], parts[3]);
                }
#endif
#ifndef NO_PROXY
                else if (!strcmp(parts[1], "tuser")) {
                    if (partcount == 4)
                        rc = restDelete('u', parts[2], parts[3]);
                }

#endif
                if (http->user_path)
                    http->user_path = strchr(http->user_path+1, '/');
            }
#ifdef HAS_BRIDGE
            else if (!strcmp(parts[0], "keystore")) {
                if (partcount == 2 && g_keystore) {
                    restDeleteStore(g_keystore, parts[1]);
                    rc = 0;
                }
            }
            else if (!strcmp(parts[0], "truststore")) {
                if (partcount == 2 && g_truststore) {
                    restDeleteStore(g_truststore, parts[1]);
                    rc = 0;
                }
            }
#endif

            break;

        /*
         * Other methods not supported
         */
        default:
            ;
        }
    }
    if (rc == 1)
        rc = 404;
    if (rc == 404) {
        char * rbuf = alloca(2048);
        TRACEL(5, transport->trclevel, "Send HTTP not found: %s: connect=%u\n", http->user_path, transport->index);
        ism_common_getErrorStringByLocale(ISMRC_HTTP_NotFound, http->locale, rbuf, 2048);
        sprintf(http->outbuf.buf, "{ \"Code\":\"CWLNA%04u\", ", ISMRC_HTTP_NotFound);
        http->outbuf.used = strlen(http->outbuf.buf);
        ism_json_putBytes(&http->outbuf, "\"Message\":");
        if (http->user_path && *http->user_path && http->user_path[1]) {
            ism_common_strlcat(rbuf, ": ", 2048);
            ism_common_strlcat(rbuf, http->user_path+1, 2048);
        }
        ism_json_putString(&http->outbuf, rbuf);
        ism_common_allocBufferCopy(&http->outbuf, " }\n");
        http->outbuf.used--;
    }
    httpRestReply(http, rc);
    return 0;
}

/*
 * Disconnect connections
 * The selction is in the query string
 */
xUNUSED static int restDisconnect(ism_http_t * http) {
    char xbuf [2048];
    char * endpoint = NULL;
    char * server = NULL;
    char * tenant = NULL;
    char * clientid = NULL;
    char * user = NULL;
    const char * ret;
    int   count;

    ret = getQueryProperty(http->query, "endpoint", xbuf, sizeof xbuf);
    if (ret) {
        endpoint = alloca(strlen(ret)+1);
        strcpy(endpoint, ret);
    }
    ret = getQueryProperty(http->query, "server", xbuf, sizeof xbuf);
    if (ret) {
        server = alloca(strlen(ret)+1);
        strcpy(endpoint, ret);
    }
    ret = getQueryProperty(http->query, "tenant", xbuf, sizeof xbuf);
    if (ret) {
        tenant = alloca(strlen(ret)+1);
        strcpy(endpoint, ret);
    }
    ret = getQueryProperty(http->query, "user", xbuf, sizeof xbuf);
    if (ret) {
        user = alloca(strlen(ret)+1);
        strcpy(endpoint, ret);
    }
    ret = getQueryProperty(http->query, "clientid", xbuf, sizeof xbuf);
    if (ret) {
        clientid = alloca(strlen(ret)+1);
        strcpy(endpoint, ret);
    }
    count = ism_transport_closeConnection(clientid, user, NULL, endpoint, tenant, server, 0);

    if (count > 0)
        sprintf(xbuf, "Connection have been closed by request: Count=%u", count);
    else
        sprintf(xbuf, "No connections are selected to be closed");
    ism_json_putBytes(&http->outbuf, xbuf);
    return 0;
}

/*
 * Set a non-complex config setting
 */
static int restPostSet(const char * name, const char * value) {
    int rc = 0;
    char * eos;
    int  ival = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[2] = {{0}};
    parseobj.ent_alloc = 2;
    parseobj.ent_count = 2;
    parseobj.ent = ents;
    parseobj.ent[0].count = 1;
    parseobj.ent[0].objtype = JSON_Object;
    parseobj.ent[1].name = name;
    parseobj.ent[1].value = value;

    if (value && *value) {
        if (!strcmpi(value, "true")) {
            parseobj.ent[1].objtype = JSON_True;
        } else if (!strcmp(value, "false")) {
            parseobj.ent[1].objtype = JSON_False;
        } else {
            ival = strtol(value, &eos, 0);
            if (*eos == 0) {
                parseobj.ent[1].objtype = JSON_Integer;
                parseobj.ent[1].count = ival;
            }
        }
        if (parseobj.ent[1].objtype) {
            rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
            if (rc == 0)
                return 0;
        }
    }
    parseobj.ent[1].objtype = JSON_String;
    rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
    if (rc == ISMRC_NotFound)
        ism_common_setErrorData(ISMRC_NotFound, "%s", name);
#ifdef HAS_BRIDGE
    if (g_dynamic_license || g_dynamic_tracelevel==2 || g_dynamic_loglevel==2) {
        if (g_dynamic_license)
            ism_bridge_startActions();
        ism_bridge_updateDynamicConfig(1);
        g_dynamic_license = 0;
        if (g_dynamic_loglevel == 2)
            g_dynamic_loglevel = 1;
        if (g_dynamic_tracelevel == 2)
            g_dynamic_tracelevel = 1;
    }
#endif
    return rc;
}

/*
 * Delete an object from REST
 */
static int restDelete(char which, const char * name, const char * name2) {
    int rc = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[3];
    parseobj.ent_alloc = 3;
    parseobj.ent_count = 1;
    parseobj.ent = ents;
    memset(parseobj.ent, 0, sizeof(ism_json_entry_t));
    parseobj.ent[0].objtype = JSON_Null;
    switch (which) {
    case 'e':
        ism_tenant_lock();
        rc = ism_proxy_makeEndpoint(&parseobj, 0, name, 0, 0);
        ism_tenant_unlock();
        break;
#ifndef NO_PROXY
    case 't':
        ism_tenant_lock();
        rc = ism_tenant_makeTenant(&parseobj, 0, name);
        ism_tenant_unlock();
        break;
    case 's':
        ism_tenant_lock();
        rc = ism_tenant_makeServer(&parseobj, 0, name);
        ism_tenant_unlock();
        break;
#endif
#ifdef HAS_BRIDGE
    case 'f':   /* Forwarder */
        ism_bridge_lock();
        if (strchr(name, '*')) {
            rc = ism_bridge_deleteAllForwarder(name, &parseobj);
        } else {
            rc = ism_bridge_makeForwarder(&parseobj, 0, name, 0, 0);
        }
        ism_bridge_unlock();
        break;
    case 'b':   /* Bridge connection */
        ism_bridge_lock();
        if (strchr(name, '*')) {
            rc = ism_bridge_deleteAllConnection(name, &parseobj);
        } else {
            rc = ism_bridge_makeConnection(&parseobj, 0, name, 0, 0);
        }
        ism_bridge_unlock();
        break;
    case 'r':   /* Routing rule */
        /* Remove routing rule.  Make JSON: "{ "RoutingRule": { "name2": null } }"   */
        ism_bridge_lock();
        memset(parseobj.ent, 0, sizeof(ism_json_entry_t)*3);
        parseobj.ent[0].objtype = JSON_Object;
        parseobj.ent[0].count = 2;
        parseobj.ent[1].objtype = JSON_Object;
        parseobj.ent[1].count = 1;
        parseobj.ent[1].name = "RoutingRule";
        parseobj.ent[2].objtype = JSON_Null;
        parseobj.ent[2].name = name2;
        rc = ism_bridge_makeForwarder(&parseobj, 0, name, 0, 0);
        ism_bridge_unlock();
        break;
#endif
    case 'u':
        ism_tenant_lock();
        if (name2) {
            ism_tenant_t * tenant = ism_tenant_getTenant(name2);
            if (!tenant) {
                rc = 404;
            } else {
                rc = ism_tenant_makeUser(&parseobj, 0, name, tenant, 0, 0);
            }
        } else {
            rc = ism_tenant_makeUser(&parseobj, 0, name, NULL, 0, 0);
        }
        ism_tenant_unlock();
    }
#ifdef HAS_BRIDGE
    if (!rc) {
        ism_bridge_startActions();
        ism_bridge_updateDynamicConfig(1);
    }
#endif
    return rc;
}

/*
 * Change config
 */
static int restPostConfig(ism_http_t * http, char * * parts, int partcount) {
    int rc = 0;
    int otype;
    xUNUSED ism_tenant_t * tenant;

    if (partcount == 1) {
        otype = 'c';
    } else if (partcount == 3) {
        if (!strcmp(parts[1], "endpoint") || !strcmp(parts[1], "Endpoint") ) {
            otype = 'e';
#ifndef NO_PROXY
        } else if (!strcmp(parts[1], "tenant") || !strcmp(parts[1], "Tenant")) {
            otype = 't';
        } else if (!strcmp(parts[1], "server") || !strcmp(parts[1], "Server")) {
            otype = 's';
#endif
        } else if (!strcmp(parts[1], "user") || !strcmp(parts[1], "User")) {
            otype = 'u';
#ifdef HAS_BRIDGE
        } else if (!strcmp(parts[1], "forwarder") || !strcmp(parts[1], "Forwarder")) {
            otype = 'f';
        } else if (!strcmp(parts[1], "connection") || !strcmp(parts[1], "Connection")) {
            otype = 'b';
#endif
        } else {
           return 404;
       }
#ifndef NO_PROXY
   } else if (partcount == 4) {
       if (!strcmp(parts[1], "user")) {
           otype='v';
       } else {
           return 404;
       }
   } else if (partcount == 2) {
       if (!strcmp(parts[0], "device")) {
           otype = 'd';
       } else {
           return 404;
       }
#endif
   } else {
       return 404;
   }

   /*
    * Parse the JSON file
    */
   ism_json_parse_t parseobj = { 0 };
   ism_json_entry_t ents[500];
   parseobj.ent_alloc = 500;
   parseobj.ent = ents;
   parseobj.source = (char *)http->content[0].content;
   parseobj.src_len = http->content[0].content_len;
   parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
   rc = ism_json_parse(&parseobj);
   if (rc == 0) {
       switch (otype) {
       case 'c':
           rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
#ifdef HAS_BRIDGE
           if (!rc) {
               ism_bridge_startActions();
               ism_bridge_updateDynamicConfig(1);
           }
#endif
           break;
       case 'e':
           rc = ism_proxy_makeEndpoint(&parseobj, 0, parts[2], 0, 0);
           break;
#ifndef NO_PROXY
       case 't':
           rc = ism_tenant_makeTenant(&parseobj, 0, parts[2]);
           break;
       case 's':
           rc = ism_tenant_makeServer(&parseobj, 0, parts[2]);
           break;
       case 'd':
           rc = ism_proxy_deviceUpdate(&parseobj, 0, parts[1]);
           break;
#endif
#ifdef HAS_BRIDGE
       case 'f':
           rc = ism_bridge_makeForwarder(&parseobj, 0, parts[2], 0, 0);
           break;
       case 'b':
           rc = ism_bridge_makeConnection(&parseobj, 0, parts[1], 0, 0);
           break;
#endif
#ifndef NO_PROXY
       case 'v':
           tenant = ism_tenant_getTenant(parts[2]);
           if (!tenant) {
               ism_common_setErrorData(ISMRC_NotFound, "%s%s", "Tenant", parts[2]);
               rc = 400;
           } else {
               rc = ism_tenant_makeUser(&parseobj, 0, parts[3], tenant, 0, 0);
           }
           break;
#endif
       case 'u':
           rc = ism_tenant_makeUser(&parseobj, 0, parts[2], NULL, 0, 0);
           break;
       }
       if (parseobj.free_ent)
           ism_common_free(ism_memory_utils_parser,parseobj.ent);
    } else {
        rc = ISMRC_ParseError;
        ism_common_setErrorData(rc, "%d%u", parseobj.rc, parseobj.line);
        LOG(ERROR, Server, 917, "%s%d%d", "The configuration {0} is not valid JSON at line {2}.",
                "POST", parseobj.rc, parseobj.line);
    }
    return rc;
}

/*
 * Close the admin connection after a short wait
 */
static int doClosed(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport = (ism_transport_t *)userdata;
    if (timer)
        ism_common_cancelTimer(timer);
    transport->closed(transport);
    return 0;
}

/*
 * Close the connection
 */
static int restConfigClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ismRestPobj_t * pobj = transport->pobj;
    if(__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        if(__sync_fetch_and_sub(&pobj->inProgress, 1) == 0)
            ism_common_setTimerOnce(ISM_TIMER_LOW, doClosed, transport, 50000000);  /* 50 ms */
    }
    return 0;
}


/*
 * Receive data
 */
static int restConfigReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
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
    char * version = NULL;
    int    versionlen = 0;
    int    inProgress;
    int    rc = 0;
    char * pw;
    char * passwd;
    int    uidlen;

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

    if (!pos) {
        ism_common_setError(ISMRC_HTTP_NotFound);
        __sync_fetch_and_sub(&pobj->inProgress, 1);
        ism_http_respond(http, 404, NULL);
        return ISMRC_HTTP_NotFound;
    }

    /* Set protocol supported fields in the http object */
    if (versionlen) {
        memcpy(http->version, version, versionlen);
        http->version[versionlen] = 0;
    }
    http->user_path = http->path + (pos-path.val.s);

    /*
     * Authenticate the user
     */
    if (!transport->authenticated) {
        uidlen = transport->userid ? strlen(transport->userid) : 0;
        ism_user_t * user;
        if (transport->userid) {
            pw = passwd = (char *)transport->userid + uidlen + 1;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        } else {
            passwd = NULL;
        }
        user = ism_tenant_getUser(transport->userid, NULL, 0);
        /* Verify password */
        if (!user || !ism_tenant_checkObfus(user->name, passwd, user->password)) {
            TRACEL(6, transport->trclevel, "User authentication failed: user=%s connect=%u client=%s\n", transport->userid, transport->index, transport->name);
            __sync_fetch_and_sub(&pobj->inProgress, 1);
            ism_http_respond(http, 401, NULL);
            rc = 401;
        }
        transport->authenticated = 1;
        if (passwd) {
            pw = passwd;
            while (*pw) {
                *pw++ ^= 0xfd;
            }
        }
    }

    /*
     * Call the function.  This must respond or close
     */
    if (!rc) {
        ism_proxy_httpRestCall(transport, http, 0);
    }
    return rc;
}


/*
 * Initialize rest msg connection
 */
static int restConfigConnection(ism_transport_t * transport) {
    // printf("protocol=%s\n", transport->protocol);
    if (*transport->protocol == '/') {
        if (!strcmp(transport->protocol, "/admin")) {
            ismRestPobj_t * pobj = (ismRestPobj_t *) ism_transport_allocBytes(transport, sizeof(ismRestPobj_t), 1);
            memset((char *) pobj, 0, sizeof(ismRestPobj_t));
            pthread_spin_init(&pobj->lock, 0);
            transport->pobj = pobj;
            transport->receive = restConfigReceive;
            transport->closing = restConfigClosing;
            /* Make protocol constant */
            transport->protocol = "/admin";
            transport->protocol_family = "admin";
            transport->www_auth = transport->endpoint->authorder[0]==AuthType_Basic;
            transport->ready = 1;
            return 0;
        }
    }
    return 1;
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
 * Initialize the rest connection
 */
int ism_proxy_restConfigInit(void) {
    ism_transport_registerProtocol(restConfigConnection);
    return 0;
}

