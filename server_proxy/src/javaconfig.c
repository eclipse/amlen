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

#define TRACE_COMP Security

#include <ismutil.h>
#include <ismregex.h>
#include <javaconfig.h>
#include <selector.h>
#include <pxtransport.h>
#include <ismjson.h>
#include <log.h>
#include <throttle.h>
#include <pxmqtt.h>
#include <protoex.h>
#include <auth.h>
#include <pxmhub.h>

#define P_LEN 8192
#include <dlfcn.h>
#define CANEXECUTE 1
#define CANREAD 4
#define JVMDLLNAME "libjvm.so"

#define JSON_OPTION_CLEARNULL 0x80

void makeError(int rc, concat_alloc_t * buf);
void logJavaException(JNIEnv *  env, jthrowable ex);
extern int ism_proxy_jsonMessage(concat_alloc_t * buf, ism_transport_t * transport, int which, int rc, const char * reason);
extern int ism_mux_checkServerConnection(ism_server_t * server, int index);
static jclass ImaProxyImpl = NULL;

int ism_mhub_parseBindings(ism_json_parse_t * parseobj, int where, const char * name);
int ism_mhub_parseCredentials(ism_json_parse_t * parseobj, int where, const char * name);

static JNIEnv * g_timer0_env;
extern int g_rc;
jmethodID g_authenticator = NULL;
jobject   g_authobj = NULL;

jmethodID g_devicestatusupdate = NULL;
jobject   g_deviceupdaterobj = NULL;

jmethodID g_getMHubCredential = NULL;
jobject   g_mHubCredMgrObj = NULL;


extern int g_deviceupdatestatus_enabled;
extern int g_AAAEnabled;

static pthread_spinlock_t   authStatLock;
static px_auth_stats_t      authStats = {0};
jmethodID g_checkAuthorization = NULL;
jobject   g_checkAuthorizationObj = NULL;

extern ism_regex_t devIDMatch;
extern int         doDevIDMatch;
extern ism_regex_t devTypeMatch;
extern int         doDevTypeMatch;

extern int         g_checkKafkaIMMsgRouting;
extern int 		  g_useKafkaIMMessaging;

static uint64_t g_authCount;   /* The count of authentications in the last time period */
static double   g_authTimeD;   /* TSC time for authentication in seconds using TSC */
static uint64_t g_toAuthTime;  /* Time from connection create to authentication complete in nanoseconds */

static uint64_t g_lastAuthCount; /* The previous auth count */
static double   g_lastAuthTimeD;
static uint64_t g_lastToAuthTime;
static ism_time_t g_lastStats = 0; /* Time of start of last stats */
static ism_time_t g_nextStats = 0;


/*
 * Return the length of a valid UTF-8 string including surrogate handling.
 * @param str  The string to find the UTF8 size of
 * @returns The length of the string in UTF-8 bytes, or -1 to indicate an input error
 */
static int sizeUTF8(const uint16_t * str, int strlen) {
    int  utflen = 0;
    int  c;
    int  i;

    for (i = 0; i < strlen; i++) {
        c = str[i];
        if (c <= 0x007f) {
            utflen++;
        } else if (c <= 0x07ff) {
            utflen += 2;
        } else if (c >= 0xdc00 && c <= 0xdfff) {   /* High surrogate is an error */
            return -1;
        } else if (c >= 0xd800 && c <= 0xdbff) {   /* Low surrogate */
             if ((i+1) >= strlen) {      /* At end of string */
                 return -1;
             } else {                    /* Check for valid surrogate */
                 c = str[++i];
                 if (c >= 0xdc00 && c <= 0xdfff) {
                     utflen += 4;
                 } else {
                     return -1;
                 }
             }
        } else {
             utflen += 3;
        }
    }
    return utflen;
}

/*
 * Make a valid UTF-8 byte array from a string.
 * Before invoking this method you must call sizeUTF8 using the same string to find the length of the UTF-8
 * buffer and to check the validity of the UTF-8 string.
 */
static const char * makeUTF8(const uint16_t * str, int strlen, char * out) {
    int  c;
    int  i;
    const char * ret = (const char *)out;

    for (i = 0; i < strlen; i++) {
        c = str[i];
        if (c <= 0x007F) {
            *out++ = (char)c;
        } else if (c > 0x07FF) {
            if (c >= 0xd800 && c <= 0xdbff) {
                c = ((c&0x3ff)<<10) + (str[++i]&0x3ff) + 0x10000;
                *out++ = (char) (0xF0 | ((c >> 18) & 0x07));
                *out++ = (char) (0x80 | ((c >> 12) & 0x3F));
                *out++ = (char) (0x80 | ((c >>  6) & 0x3F));
                *out++ = (char) (0x80 | (c & 0x3F));
            } else {
                *out++ = (char) (0xE0 | ((c >> 12) & 0x0F));
                *out++ = (char) (0x80 | ((c >>  6) & 0x3F));
                *out++ = (char) (0x80 | (c & 0x3F));
            }
        } else {
            *out++ = (char) (0xC0 | ((c >>  6) & 0x1F));
            *out++ = (char) (0x80 | (c & 0x3F));
        }
    }
    *out++ = 0;
    return ret;
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
 */
static XINLINE int validSecond(int state, int byte1, int byte2) {
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

/*
 * Fast UTF-8 to char conversion.
 */
xUNUSED static const uint16_t * fromUTF8(const char * buf, int buflen, int * outlen) {
    int  byte1 = 0;
    int  state = 0;
    int  value = 0;
    int  inputsize = 0;
    uint16_t * ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_utils,1),buflen*2);
    uint16_t * out = ret;
    int  i;

    for (i=0; i<buflen; i++) {
        if (state == 0) {
            /* Fast loop in single byte mode */
            for (;;) {
                byte1 = (uint8_t)buf[i];
                if (byte1 >= 0x80)
                    break;
                *out++ = (uint16_t)byte1;
                if (++i >= buflen) {
                    *outlen = out-ret;
                    return ret;
                }
            }
            state = States[(byte1&0xff) >>3];
            value = byte1 = byte1 & StateMask[state];
            inputsize = 1;
            if (state == 1) {
                ism_common_free(ism_memory_proxy_utils,ret);
                return NULL;
            }
        } else {
            int byte2 = (uint8_t)buf[i];
            if ((inputsize==1 && !validSecond(state, byte1, byte2)) ||
                (inputsize > 1 && (byte2 < 0x80 || byte2 > 0xbf))) {
                ism_common_free(ism_memory_proxy_utils,ret);
                return NULL;
            }
            value = (value<<6) | (byte2&0x3f);
            if (inputsize+1 >= state) {
                if (value < 0x10000) {
                    *out++ = (uint16_t)value;
                } else {
                    *out++ = (uint16_t)(((value-0x10000) >> 10) | 0xd800);
                    *out++ = (uint16_t)(((value-0x10000) & 0x3ff) | 0xdc00);
                }
                state = 0;
            } else {
                inputsize++;
            }
        }
    }
    *outlen = out-ret;
    return ret;
}

/*
 * Put one character to a concat buf
 */
xUNUSED static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/*
 * Make a C String from a Java string, overcoming the broken UTF-8 support of JNI
 * The resulting C string is allocated in the heap.
 */
const char * make_javastr(JNIEnv * env, jstring jstr, uint16_t * * juni) {
    char * ret;
    if (!jstr) {
        return NULL;
    } else {
    	//Exception Check
    	if((*env)->ExceptionCheck(env)) return NULL;

        uint32_t  len = (*env)->GetStringLength(env, jstr);
        uint16_t * uni = (uint16_t *)(*env)->GetStringChars(env, jstr, 0);
        *juni = uni;
        int utflen = sizeUTF8(uni, len);
        if (utflen < 0)
            return NULL;
        ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_utils,2),utflen+1);
        if (utflen == 0)
        	ret[0] = '\0';
        else
        	makeUTF8(uni, len, ret);
        return ret;
    }
}

/*
 * Free up a java string
 */
void free_javastr(JNIEnv * env, jstring jstr, const char * cstr, uint16_t * uni) {
    if (jstr) {
        (*env)->ReleaseStringChars(env, jstr, uni);
    }
    if (cstr && *cstr) {
        ism_common_free(ism_memory_proxy_utils,(char *)cstr);
    }
}

extern ism_transport_t * ism_mqtt_getServerTransport(ism_transport_t * transport);

#ifndef NO_PROXY
/*
 * Send an ACL deleted
 */
void sendACLdelete(ism_acl_t * acl) {
    char xbuf[1024];
    if (acl->object) {
        ism_transport_t * transport = (ism_transport_t *)acl->object;
        ism_transport_t * stransport = NULL;
        concat_alloc_t buf = {xbuf, sizeof xbuf, 19};
		xbuf[16] = 0;
		xbuf[17] = 1;
		xbuf[18] = EXIV_EndExtension;
		bputchar(&buf, '!');
		ism_json_putString(&buf, acl->name);
		bputchar(&buf, 0);
		buf.used--;
		if(transport && transport->state == ISM_TRANST_Open) {
			stransport = ism_mqtt_getServerTransport(transport);
			// printf("Send ACL delete: client=%s\n%s\n", stransport->name, buf.buf+16);
			if (stransport && stransport->state == ISM_TRANST_Open) {
				stransport->send(stransport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
			}

        }
		if (buf.inheap)
			ism_common_freeAllocBuffer(&buf);
    }
}

#define DIRECT_KAFKA_UNRELIABLE   0x01  /* Produce direct to Kafka and filter unreliable */
#define DIRECT_KAFKA_RELIABLE     0x02  /* Produce direct to Kafka and filter reliable */
#define ROUTE_UNRELIABLE          0x04  /* Process the route for unreliable */
#define ROUTE_RELIABLE            0x08  /* Process the route for reliable */
#define FILTER_UNRELIABLE_ONLY    0x10  /* Filter unreliable messages */
#define FILTER_RELIABLE_ONLY      0x20  /* Filter reliable messages */

#define FILTER_UNRELIABLE         0x11
#define FILTER_RELIABLE           0x22

int ism_proxy_updateACL(ism_transport_t * transport, void * xtenant, uint64_t whichval);
/*
 *
 */
static void updateACL(ism_tenant_t * tenant, int which, int val) {
    if (tenant->server && tenant->server->monitor) {
        ism_transport_submitAsyncJobRequest(tenant->server->monitor, ism_proxy_updateACL, tenant, (which<<1) | !!val);
    }
}

int ism_proxy_routeMsgEnabled(ism_tenant_t * tenant){
	int msgRouting = tenant->messageRouting;
	if(msgRouting>0){
		if((msgRouting & DIRECT_KAFKA_UNRELIABLE) || (msgRouting & DIRECT_KAFKA_RELIABLE)
			|| (msgRouting & ROUTE_UNRELIABLE) || (msgRouting & ROUTE_RELIABLE) ){
			return 1;
		}
	}
	return 0;

}

int ism_proxy_routeSendMQTT(int msgRouting, int qos){
	int result=1;
	if (qos == 0 && ((msgRouting & DIRECT_KAFKA_UNRELIABLE) || (msgRouting & FILTER_UNRELIABLE)) ) {
		result = 0;
	}else if(qos > 0 && ( (msgRouting & DIRECT_KAFKA_RELIABLE) || (msgRouting & FILTER_RELIABLE)  )){
		result = 0;
	}
	return result;
}

int ism_proxy_routeToKafka(ism_tenant_t * tenant, int qos)
{
	int rc =0;
	if(!g_checkKafkaIMMsgRouting){
		//If no need to enforce messageRouting Per org.
		//allow to PUblish to Kafka for qos 0 only
		if(qos==0)
			return 1;
		else
			if(qos > 0 && g_useKafkaIMMessaging > 1)
				return 1;
			else
				return 0;

	}
	if(qos == 0 && (tenant->messageRouting & DIRECT_KAFKA_UNRELIABLE)){
		rc = 1;
	}else if(qos > 0 &&  ((tenant->messageRouting & DIRECT_KAFKA_RELIABLE)) ){
		rc = 1;
	}

	return rc;
}

/*
 * Change the ACL for routing
 */
void ism_proxy_changeMsgRouting(ism_tenant_t * tenant, int old_msgRouting) {
    int msgroute = tenant->messageRouting;
    ism_acl_t * acl;

    if (msgroute & FILTER_UNRELIABLE) {
        acl = ism_protocol_findACL("_0", 1, NULL);
        ism_protocol_addACLitem(acl, tenant->name);
        ism_protocol_unlockACL(acl);
        updateACL(tenant, 0, 1);
    } else {
        if (old_msgRouting & FILTER_UNRELIABLE) {
            acl = ism_protocol_findACL("_0", 1, NULL);
            ism_protocol_delACLitem(acl, tenant->name);
            ism_protocol_unlockACL(acl);
            updateACL(tenant, 0, 0);
        }
    }
    if (msgroute & FILTER_RELIABLE) {
        acl = ism_protocol_findACL("_1", 1, NULL);
        ism_protocol_addACLitem(acl, tenant->name);
        ism_protocol_unlockACL(acl);
        updateACL(tenant, 1, 1);
    } else {
        if (old_msgRouting & FILTER_RELIABLE) {
            acl = ism_protocol_findACL("_1", 1, NULL);
            ism_protocol_delACLitem(acl, tenant->name);
            ism_protocol_unlockACL(acl);
            updateACL(tenant, 1, 0);
        }
    }
}

/*
 * Add the object itself
 */
void ism_proxy_addSelf(ism_acl_t * acl) {
    char self [512];
    const char * name = acl->name;
    if (acl->hash == NULL) {
        sendACLdelete(acl);
    } else {
        if (name[1]==':') {
            char * pos = strchr(name+2, ':');
            if (pos) {
                ism_common_strlcpy(self, pos+1, sizeof self);
                pos = strchr(self, ':');
                if (pos) {
                    *pos = '/';
                    ism_protocol_addACLitem(acl, self);
                    TRACE(5, "Add ACL item %s to %s\n", self, acl->name);
                }
            }
        }
    }
}


/*
 * Scan the ACL list and update the server ACL
 * TODO: send delta ACL when appropriate
 */
void ism_proxy_sendAllACLs(const char * aclsrc, int acllen) {
    char * aclcopy;
    int    done = 0;
    int    freeit = 0;
    char   xbuf [2048];

    if (aclsrc && acllen < 0)
        acllen = strlen(aclsrc);

    /* Make a copy of the list and change CR or LF to NUL */
    if (acllen < 4091) {
        aclcopy = alloca(acllen+2);
    } else {
        aclcopy = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,3),acllen+2);
        freeit = 1;
    }
    memcpy(aclcopy, aclsrc, acllen);
    aclcopy[acllen] = 0;
    aclcopy[acllen+1] = 0xFF;
    char * ap = aclcopy;
    while (*ap) {
        if (*ap == '\r' || *ap == '\n')
            *ap = 0;
        ap++;
    }
    ap = aclcopy;
    for (;;) {
        while (!*ap)
            ap++;
        switch ((uint8_t)*ap) {
        /* End of ACLs */
        case 0xFF:
            done = 1;
            break;
         /* Comment */
        default:
            break;
         /* New set */
        case '@':
        case ':':
        case '!':
            {
                concat_alloc_t buf = {xbuf, sizeof xbuf, 19};
                ism_acl_t * acl = ism_protocol_findACL(ap+1, 0, NULL);
                if (acl) {
                    if (acl->object) {
                        ism_transport_t * transport = (ism_transport_t *)acl->object;
                        ism_transport_t * stransport = NULL;
                        xbuf[16] = 0;
                        xbuf[17] = 1;
                        xbuf[18] = EXIV_SendNFSubs;
                        ism_protocol_getACL(&buf, acl);
                        bputchar(&buf, 0);
                        buf.used--;
                        // printf("update ACL client=%s:\n%s\n", transport->name, buf.buf+16);
                        if(transport && transport->state == ISM_TRANST_Open ){
							if (transport->hout){      /* HTTP outbound */
							    //Since PINGRESP is not supported for HTTP,
							    //Set the bit 18th to EndExtension so MG will not
							    //Send the PINGRESP for list of topcs (SENDSUBS)
							    xbuf[18] = EXIV_EndExtension;
								stransport = (ism_transport_t *) transport->hout;
							}
							if (!stransport)          /* MQTT */
								stransport = ism_mqtt_getServerTransport(transport);
							if (stransport && stransport->state == ISM_TRANST_Open) {
								stransport->send(stransport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
							}
                        }
                        ism_protocol_unlockACL(acl);
                        if (buf.inheap)
                            ism_common_freeAllocBuffer(&buf);
                    } else {
                        ism_protocol_unlockACL(acl);
                    }
                }
            }
            break;
        }
        if (done)
            break;
        ap += strlen(ap);
    }

    if (freeit && aclcopy)
        ism_common_free(ism_memory_proxy_javaconfig,aclcopy);
}
#endif

#define OTYPE_MHubCredentials 90
#define OTYPE_MHubBindings    91
/*
 * Set a JSON config object.
 *
 * Method:    setJSONn
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_setJSONn(
        JNIEnv * env, jobject inst, jint otype, jstring jname, jstring jname2, jstring jjson) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uname;
    uint16_t * uname2;
    uint16_t * ujson;
    const char * name  = make_javastr(env, jname, &uname);
    const char * name2 = make_javastr(env, jname2, &uname2);
    const char * json  = make_javastr(env, jjson, &ujson);
    const char * oname = name;
    xUNUSED const char * tname = NULL;
    xUNUSED ism_tenant_t * tenant;
    int  rc = 0;

    /*
     * Construct object name for global config
     */
    if (otype == ImaProxyImpl_config) {
        int namelen = name ? strlen(name) : 0;
        int name2len = name2 ? strlen(name2) : 0;
        if (namelen >= 10 && !memcmp(name, "MessageHub", 10)) {
            if (!strcmp(name, "MessageHubBindings") && name2len > 0) {
                otype = OTYPE_MHubBindings;
                oname = name2;
            } else if (!strcmp(name, "MessageHubCredentials") && name2len > 0) {
                otype = OTYPE_MHubCredentials;
                oname = name2;
            } else {
                rc = PRC_BadPath;
            }
        } else if (name2len > 0) {
            if (name2len > 9 && !memcmp(name2, "endpoint=", 9)) {
                otype = ImaProxyImpl_endpoint;
                oname = name2 + 9;
            } else if (name2len >= 7 && !memcmp(name2, "tenant=", 7)) {
                otype = ImaProxyImpl_tenant;
                oname = name2 + 7;
            } else if (name2len > 7 && !memcmp(name2, "server=", 7)) {
                otype = ImaProxyImpl_server;
                oname = name2 + 7;
            } else if (name2len > 5 && !memcmp(name2, "user=", 5)) {
                otype = ImaProxyImpl_user;
                oname = strchr(name2+5, ',');
                if (oname) {
                    *(char *)oname++ = 0;      /* zero out the comma and place at following character */
                    tname = name2+5;
                } else {
                    oname = name2+5;
                }
            } else {
                rc = PRC_BadPath;
            }
        }
    } else {
        oname = name;
        tname = name2;
    }

#ifndef NO_PROXY
    /*
     * Set an ACL
     */
    if (rc == 0 && otype == ImaProxyImpl_acl) {
        int acllen = strlen(json);
        TRACE(7, "Calling ism_protocol_setACL json=%s\n", json);
        rc = ism_protocol_setACL(json, acllen, 0, ism_proxy_addSelf);
        if (rc == 0) {
            ism_proxy_sendAllACLs(json, acllen);
        } else {
        	TRACE(3, "ism_protocol_setACL failed rc=%d\n", rc);
        }
    } else
#endif
    /*
     * Parse the JSON file
     */
    if (rc == 0) {
        ism_json_parse_t parseobj = { 0 };
        ism_json_entry_t ents[500];
        parseobj.ent_alloc = 500;
        parseobj.ent = ents;
        parseobj.source = (char *)json;
        // printf("post: '%s'\n", json);
        parseobj.src_len = strlen(json);
        /* Allow C style comments, and clear selected fields when not specified */
        parseobj.options = JSON_OPTION_COMMENT | JSON_OPTION_CLEARNULL;
        rc = ism_json_parse(&parseobj);
        if (rc == 0) {
            switch (otype) {
            case ImaProxyImpl_config:
                rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
                break;
            case ImaProxyImpl_endpoint:
                rc = ism_proxy_makeEndpoint(&parseobj, 0, oname, 0, 0);
                break;
#ifndef NO_PROXY
            case ImaProxyImpl_tenant:
                rc = ism_tenant_makeTenant(&parseobj, 0, oname);
                break;
            case ImaProxyImpl_server:
                rc = ism_tenant_makeServer(&parseobj, 0, oname);
                break;
            case ImaProxyImpl_device:
				//Process Device Update
				rc = ism_proxy_deviceUpdate(&parseobj, 0, oname);
				break;
#endif
            case ImaProxyImpl_user:
#ifndef NO_PROXY
                if (tname) {
                    tenant = ism_tenant_getTenant(tname);
                    if (!tenant) {
                        rc = PRC_NotFound;
                    } else {
                        rc = ism_tenant_makeUser(&parseobj, 0, oname, tenant, 0, 0);
                    }
                } else
#endif
                {
                    rc = ism_tenant_makeUser(&parseobj, 0, oname, NULL, 0, 0);
                }
                break;
#ifndef HAS_BRIDGE
            case OTYPE_MHubBindings:
                rc = ism_mhub_parseBindings(&parseobj, 0, oname);
                break;
            case OTYPE_MHubCredentials:
                rc = ism_mhub_parseCredentials(&parseobj, 0, oname);
                break;
#endif
            }
        }
        if (parseobj.free_ent)
            ism_common_free(ism_memory_utils_parser,parseobj.ent);
    }
    free_javastr(env, jname,  name,  uname);
    free_javastr(env, jname2, name2, uname2);
    free_javastr(env, jjson,  json,  ujson);
    if (rc) {
        char xbuf[1024];
        concat_alloc_t buf = {xbuf, sizeof xbuf};
        makeError(rc, &buf);
        jstring ret = (jstring)(*env)->NewStringUTF(env, buf.buf);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
        ism_common_freeTLS();
        return ret;
    }
    ism_common_freeTLS();
    return (jstring)NULL;
}

/*
 * Set a binary config object.
 * Method:    setBinary
 * Signature: (ILjava/lang/String;Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_setBinary(
    JNIEnv * env, jobject inst, jint otype, jstring jname, jstring jname2, jbyteArray jbytes) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uname;
    uint16_t * uname2;
    const char * name = make_javastr(env, jname, &uname);
    const char * name2 = make_javastr(env, jname2, &uname2);
    xUNUSED int bytes_len = (*env)->GetArrayLength(env, jbytes);
    const char * bytes = (const char *)(*env)->GetByteArrayElements(env, jbytes, 0);
    /* TODO */
    (*env)->ReleaseByteArrayElements(env, jbytes, (jbyte *)bytes, JNI_ABORT);   /* Do not copy back */
    free_javastr(env, jname, name, uname);
    free_javastr(env, jname2, name2, uname2);
    ism_common_freeTLS();
    return 0;
}

/*
 * Set an Error return code
 */
void setError(concat_alloc_t * buf, int rc, const char * repl) {
    char xbuf[64];
    buf->used = 0;
    sprintf(xbuf, "%d:IllegalArgumentException:", rc);
    ism_json_putBytes(buf, xbuf);
    if (repl)
        ism_json_putBytes(buf, repl);
    ism_common_allocBufferCopyLen(buf, "", 1);
    buf->used--;
}


/*
 * Make an error for set
 */
void makeError(int rc, concat_alloc_t * buf) {
    char xbuf[1024];
    if (rc < 20) {
        sprintf(xbuf, "%d:IllegalArgumentException:", rc);
    } else {
        if (rc == ISMRC_NotFound)
            rc = PRC_NotFound;
        if (rc == ISMRC_Error)
            rc = PRC_Exception;
        if (rc >= 100)
            rc = PRC_BadPropValue;
        sprintf(xbuf, "+%d:IllegalArgumentException:", rc);
        ism_json_putBytes(buf, xbuf);
        ism_common_formatLastError(xbuf, sizeof xbuf);
    }
    ism_common_allocBufferCopy(buf, xbuf);   /* xbuf + null terminator */
    buf->used--;
}

/*
 * Internal portion of getJSONn
 */
int ism_proxy_getJSONn(concat_alloc_t * buf, int otype, const char * name, const char * name2) {
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 4, 0);
    ism_field_t field;
    ism_endpoint_t * endpoint;
#ifndef NO_PROXY
    ism_server_t * server;
#endif
    ism_tenant_t * tenant = NULL;
    ism_user_t * user;
    const char * oname = name;
    const char * tname = NULL;
    int  rc = 0;

    /*
     * Construct object name for global config
     */
    if (otype == ImaProxyImpl_config) {
        otype = 0;
        int name2len = name2 ? strlen(name2) : 0;
        if (name2len > 5 && !memcmp(name2, "list,", 5)) {
            otype = ImaProxyImpl_list;
            name2len -= 5;
            name2 += 5;
            jobj->indent = 0;
        }
        if (name2len > 9 && !memcmp(name2, "endpoint=", 9)) {
            otype |= ImaProxyImpl_endpoint;
            oname = name2 + 9;
#ifndef NO_PROXY
        } else if (name2len >= 7 && !memcmp(name2, "tenant=", 7)) {
            otype |= ImaProxyImpl_tenant;
            oname = name2 + 7;
        } else if (name2len > 7 && !memcmp(name2, "server=", 7)) {
            otype |= ImaProxyImpl_server;
            oname = name2 + 7;
#endif
        } else if (name2len > 5 && !memcmp(name2, "user=", 5)) {
            otype |= ImaProxyImpl_user;
#ifndef NO_PROXY
            oname = strchr(name2+5, ',');
            if (oname) {
                *(char *)oname++ = 0;      /* zero out the comma and place at following character */
                tname = name2+5;
            } else
#endif
            {
                oname = name2+5;
            }
        } else {
            rc = PRC_BadPath;
        }
    } else {
        oname = name;
        tname = name2;
    }
    switch (otype) {
    /* Get properties for an endpoint */
    case ImaProxyImpl_endpoint:
        ism_tenant_lock();
        endpoint = ism_transport_getEndpoint(oname);
        if (!endpoint) {
            setError(buf, PRC_NotFound, oname);
        } else {
            ism_tenant_getEndpointJson(endpoint, jobj, NULL);
        }
        ism_tenant_unlock();
        break;

    /* Get a list of endpoints */
    case ImaProxyImpl_endpoint + ImaProxyImpl_list:
        jobj->indent = 0;
        ism_transport_getEndpointList(oname, jobj, 0, NULL);
        break;

#ifndef NO_PROXY
    /* Get properties for a tenant */
    case ImaProxyImpl_tenant:
        ism_tenant_lock();
        tenant = ism_tenant_getTenant(oname);
        if (!tenant) {
            setError(buf, PRC_NotFound, oname);
        } else {
            ism_tenant_getTenantJson(tenant, jobj, NULL);
        }
        ism_tenant_unlock();
        break;

    /* Get a list of tenants */
    case ImaProxyImpl_tenant + ImaProxyImpl_list:
        jobj->indent = 0;
        ism_tenant_getTenantList(oname, jobj, 0, NULL);
        break;

    /* Get properties for a server */
    case ImaProxyImpl_server:
        ism_tenant_lock();
        server = ism_tenant_getServer(oname);
        if (!server) {
            setError(buf, PRC_NotFound, oname);
        } else {
            ism_tenant_getServerJson(server, jobj, NULL);
        }
        ism_tenant_unlock();
        break;

    /* Get a list of servers */
    case ImaProxyImpl_server + ImaProxyImpl_list:
        jobj->indent = 0;
        ism_tenant_getServerList(oname, jobj, 0, NULL);
        break;
#endif

    /* Get properties for a user */
    case ImaProxyImpl_user:
        user = NULL;
        ism_tenant_lock();

#ifndef NO_PROXY
        if (tname) {
            tenant = ism_tenant_getTenant(tname);
            if (!tenant) {
                setError(buf, PRC_NotFound, tname);
            } else {
                // printf("getUser: %s tenant=%s\n", oname, tenant->name);
                user = ism_tenant_getUser(oname, tenant, 1);
                if (!user) {
                    setError(buf, PRC_NotFound, oname);
                } else {
                    ism_tenant_getUserJson(user, jobj, NULL);
                }
            }
        } else
#endif
        {
            user = ism_tenant_getUser(oname, NULL, 0);
            if (!user) {
                setError(buf, PRC_NotFound, oname);
            } else {
                ism_tenant_getUserJson(user, jobj, NULL);
            }
        }
        ism_tenant_unlock();
        break;

    /* Get a list of users */
    case ImaProxyImpl_list + ImaProxyImpl_user:
        jobj->indent = 0;
        if (tname) {
#ifndef NO_PROXY
            tenant = ism_tenant_getTenant(tname);
            if (!tenant) {
                setError(buf, PRC_ItemNotFound, tname);
            } else
#endif
            {
                ism_tenant_getUserList(oname, tenant, jobj, 0, NULL);
            }
        } else {
            ism_tenant_getUserList(oname, NULL, jobj, 0, NULL);
        }
        break;

    /* Get a single config item */
    case ImaProxyImpl_item:
        rc = ism_common_getProperty(ism_common_getConfigProperties(), oname, &field);
        if (rc) {
            setError(buf, PRC_NotFound, oname);
        } else {
            ism_json_putBytes(buf, "{ ");
            ism_json_put(buf, oname, &field, 0);
            ism_json_putBytes(buf, " }");
        }
        break;
    }
    return rc;
}

/*
 * Get a JSON object.
 *
 * Method:    getJSONn
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getJSONn(
    JNIEnv * env, jobject inst, jint otype, jstring jname, jstring jname2) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uname;
    uint16_t * uname2;
    char xbuf[8182];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    xUNUSED int rc;

    const char * name = make_javastr(env, jname, &uname);
    const char * name2 = make_javastr(env, jname2, &uname2);

    xbuf[0] = 0;

    rc = ism_proxy_getJSONn(&buf, otype, name, name2);

    free_javastr(env, jname, name, uname);
    free_javastr(env, jname2, name2, uname2);

    jstring ret = (jstring)(*env)->NewStringUTF(env, buf.buf);
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    ism_common_freeTLS();
    return ret;
}


/*
 * Get a binary object.
 *
 * Method:    getCert
 * Signature: (ILjava/lang/String;Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getCert(
    JNIEnv * env, jobject inst, jint otype, jstring jname, jstring jname2) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uname;
    uint16_t * uname2;
    const char * name = make_javastr(env, jname, &uname);
    const char * name2 = make_javastr(env, jname2, &uname2);
    /* TODO */
    free_javastr(env, jname, name, uname);
    free_javastr(env, jname2, name2, uname2);
    ism_common_freeTLS();
    return (jbyteArray)NULL;
}

/*
 * Delete object.
 *
 * Method:    deleteObj
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_deleteObj(
    JNIEnv * env, jobject inst, jint otype, jstring jname, jstring jname2, jboolean force) {
    ism_common_makeTLS(512,NULL);
    int  rc = 0;
    uint16_t * uname;
    uint16_t * uname2;
    const char * name = make_javastr(env, jname, &uname);
    const char * name2 = make_javastr(env, jname2, &uname2);
    xUNUSED ism_tenant_t * tenant = NULL;

    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[1];
    parseobj.ent_alloc = 1;
    parseobj.ent = ents;
    memset(parseobj.ent, 0, sizeof(ism_json_entry_t));
    parseobj.ent[0].objtype = JSON_Null;
    switch (otype) {
    case ImaProxyImpl_endpoint:
        rc = ism_proxy_makeEndpoint(&parseobj, 0, name, 0, 0);
        break;
#ifndef NO_PROXY
    case ImaProxyImpl_tenant:
        rc = ism_tenant_makeTenant(&parseobj, 0, name);
        break;
    case ImaProxyImpl_server:
        rc = ism_tenant_makeServer(&parseobj, 0, name);
        break;
#endif
    case ImaProxyImpl_user:
#ifndef NO_PROXY
        if (name2) {
            tenant = ism_tenant_getTenant(name2);
            if (!tenant) {
                rc = PRC_NotFound;
            } else {
                rc = ism_tenant_makeUser(&parseobj, 0, name, tenant, 0, 0);
            }
        } else
#endif
        {
            rc = ism_tenant_makeUser(&parseobj, 0, name, NULL, 0, 0);
        }
    }
    free_javastr(env, jname, name, uname);
    free_javastr(env, jname2, name2, uname2);
    ism_common_freeTLS();
    return (jboolean)(rc == 0);
}


/*
 * Get statistics.
 *
 * Method:    getStats
 * Signature: (ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getStats(
    JNIEnv * env, jobject inst, jint otype, jstring jname) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uname;
    char xbuf[8182];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, &buf, 0, JSON_OUT_COMPACT);

    const char * name = make_javastr(env, jname, &uname);
    const char * match = name ? name : "*";
    switch(otype) {
    case ImaProxyImpl_endpoint:
        ism_tenant_getEndpointStats(match, jobj);
        break;
#ifndef NO_PROXY
    case ImaProxyImpl_server:
        ism_tenant_getServerStats(match, jobj);
        break;
#endif
    default:
        setError(&buf, PRC_NotFound, NULL);
    }
    free_javastr(env, jname, name, uname);
    jstring ret = (jstring)(*env)->NewStringUTF(env, buf.buf);
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    ism_common_freeTLS();
    return ret;
}

/*
 * Get an obfuscated password.
 *
 * Method:    getObfus
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getObfus(
    JNIEnv * env, jobject inst, jstring juserid, jstring jpassword, jint otype) {
    ism_common_makeTLS(512,NULL);
    uint16_t * uuser;
    uint16_t * upassword;
    const char * userid = make_javastr(env, juserid, &uuser);
    const char * password = make_javastr(env, jpassword, &upassword);
    char xbuf[2048];
    ism_tenant_createObfus(userid, password, xbuf, sizeof xbuf, otype);
    free_javastr(env, juserid, userid, uuser);
    free_javastr(env, jpassword, password, upassword);
    ism_common_freeTLS();
    return (*env)->NewStringUTF(env, xbuf);      /* OK as the string is only ASCII7 */
}

/*
 * Start messaging.
 *
 * Method:    startMsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_startMsg(
    JNIEnv * env, jobject inst) {
    ism_common_makeTLS(512,NULL);
    ism_transport_startMessaging();
    ism_common_freeTLS();
    return (jstring)NULL;
}



/*
 * Terminate the proxy.
 *
 * Method:    quitProxy
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_quitProxy(
   JNIEnv * env, jobject thisobj, jint rc) {
    ism_common_makeTLS(512,NULL);
    pid_t pid = getpid();
    TRACE(1, "Exit proxy from Java config: rc=%d\n", rc);
    g_rc = rc;
    ism_common_sleep(100000);
    kill(pid, SIGTERM);
    ism_common_freeTLS();
    return (jstring)NULL;
}

/*
 * Do a disconnect.
 *
 * Method:    doDisconnect
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doDisconnect(
    JNIEnv * env, jobject thisobj, jstring endpoint, jstring server, jstring tenant, jstring user, jstring clientid, jint operationbits) {
    ism_common_makeTLS(512,NULL);
    uint16_t * u_endpoint;
    uint16_t * u_server;
    uint16_t * u_tenant;
    uint16_t * u_user;
    uint16_t * u_clientid;
    const char * c_endpoint  = make_javastr(env, endpoint, &u_endpoint);
    const char * c_server    = make_javastr(env, server,   &u_server);
    const char * c_tenant    = make_javastr(env, tenant,   &u_tenant);
    const char * c_user      = make_javastr(env, user,     &u_user);
    const char * c_clientid  = make_javastr(env, clientid, &u_clientid);
    int permissions = operationbits;

    int count = ism_transport_closeConnection(c_clientid, c_user, NULL, c_endpoint, c_tenant, c_server, permissions);

    free_javastr(env, endpoint, c_endpoint, u_endpoint);
    free_javastr(env, server,   c_server,   u_server);
    free_javastr(env, tenant,   c_tenant,   u_tenant);
    free_javastr(env, user,     c_user,     u_user);
    free_javastr(env, clientid, c_clientid, u_clientid);
    ism_common_freeTLS();
    return (jint)count;
}

#ifndef NO_PROXY
/*
 * Run proxy authenticate in the timer thread
 */
int ism_proxyAuthenticate(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    jthrowable ex = NULL;
    int  rc = 0;
    authAction_t * action = (authAction_t *)userdata;
    ism_transport_t * transport = action->transport;

    ism_common_cancelTimer(key);

    pthread_spin_lock(&transport->lock);
    TRACE(9, "ism_proxyAuthenticate: connect=%u state=%d client=%s\n",
            transport->index, transport->state, transport->clientID);
    //if(UNLIKELY(transport->state == ISM_TRANST_Closing) || UNLIKELY(transport->state == ISM_TRANST_Closed)){
    if(UNLIKELY(transport->state == ISM_TRANST_Closed)){
        pthread_spin_unlock(&transport->lock);
        rc = ISMRC_NotConnected;
        ism_common_setError(rc);
        action->callback(rc, action->callbackParam);
        ism_common_free(ism_memory_proxy_auth,action);
        return 0;
    }
    pthread_spin_unlock(&transport->lock);
    /*Check if Server Connection is available*/
    int server_avail = ism_mux_checkServerConnection(transport->server,
            transport->tid);
    if (!server_avail) {
        TRACE(6,
                "ism_proxyAuthenticate: Server is not available. Set error and return before Auth. connect=%d tid=%d client=%s \n",
                transport->index, transport->tid, transport->name);
        rc = ISMRC_ServerNotAvailable;
        ism_common_setError(ISMRC_ServerNotAvailable);
        action->callback(rc, action->callbackParam);
        ism_common_free(ism_memory_proxy_auth,action);
        return 0;
    }
    action->authStartTime = ism_common_readTSC();

    JNIEnv * env = g_timer0_env;
    rc = (*env)->PushLocalFrame(env, 18);
    if(rc == 0) {
        jstring jclient  = (jstring)(*env)->NewStringUTF(env, transport->clientID);
        jstring juser    = NULL;
        jstring jpwd     = (jstring)(*env)->NewStringUTF(env, action->password);
        jstring jcert    = NULL;
        jstring jissuer  = NULL;
        jstring jaddr    = (jstring)(*env)->NewStringUTF(env, transport->client_addr);
        jlong   jctx     = (jlong)((int64_t)(uintptr_t)action);
        jint    jwhich   = (jint)action->which;
        jint    jcrlStatus = (jint)transport->crlStatus;
        jint    jsecure    = (jint)transport->secure;
        jint    jsgEnabled = (jint) transport->tenant->sgEnabled==1;
        jint    jrlacAppEnabled = (jint) transport->tenant->rlacAppResourceGroupEnabled;
        jint    jrlacAppDefaultGroup = (jint) transport->tenant->rlacAppDefaultGroup;
        jint 	jrmPolicies =  (jint) transport->tenant->rmPolicies;
        jint   jport    = (jint) transport->clientport;
        jlong   jconId   = (jlong) transport->index;

        if(UNLIKELY((jclient == NULL) || (jpwd == NULL) || (jaddr == NULL)))
            rc = ISMRC_AllocateError;

        if (transport->userid && (rc == 0)) {
            juser   = (jstring)(*env)->NewStringUTF(env, action->userID);
            if(UNLIKELY(juser == NULL))
                rc = ISMRC_AllocateError;
        }
        if ((transport->cert_name) && (rc == 0)) {
            jcert   =(jstring)(*env)->NewStringUTF(env, transport->cert_name);
            if(UNLIKELY(jcert == NULL))
                rc = ISMRC_AllocateError;
        }
        if ((transport->certIssuerName) && (rc == 0)) {
            jissuer   =(jstring)(*env)->NewStringUTF(env, transport->certIssuerName);
            if(UNLIKELY(jissuer == NULL))
                rc = ISMRC_AllocateError;
        }
        if(rc == 0) {
            rc = (*env)->CallIntMethod(env, g_authobj, g_authenticator,
                    jclient, juser, jrlacAppEnabled, jrlacAppDefaultGroup, jsecure, jrmPolicies, jsgEnabled, jcert, jissuer, jcrlStatus, jwhich, jaddr, jpwd, jctx, jport, jconId);
            //TODO: Check rc
            rc = 0;
            ex = ((*env)->ExceptionOccurred(env));
            if (ex) {
                logJavaException(env, ex);
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
                rc = ISMRC_Error; //TODO: Should this be another error code
            }
        }
        (*env)->PopLocalFrame(env, NULL);
    } else {
        rc = ISMRC_AllocateError;
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            logJavaException(env, ex);
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, ex);
        }

    }

    if(rc) {
        ism_common_setError(rc);
        action->callback(rc, action->callbackParam);
        ism_common_free(ism_memory_proxy_auth,action);
    }
    return 0;
}

extern int ism_proxy_completeAuthorize(void * correlate, int rc, const char * reason);

static int proxy_checkAuthorizationOnTimer(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	jthrowable ex = NULL;
	int  rc = 0;
	asyncauth_t * asyncJob = (asyncauth_t *)userdata;
	ism_transport_t * transport = asyncJob->transport;

	ism_common_cancelTimer(key);

	TRACE(6, "proxy_checkAuthorizationOnTimer: action=%d, deviceType=%s, deviceId=%s, connect=%u, userid=%s\n",
			asyncJob->action, asyncJob->devtype, asyncJob->devid, transport->index, transport->userid ? transport->userid : "null");


    //validate for Device ID or App id
	if (!rc && doDevIDMatch) {
		/* Make sure clientID matches regex */
		if (ism_regex_match(devIDMatch,  asyncJob->devid)) {
			TRACE(5, "proxy_checkAuthorizationOnTimer: Device or Application id is not valid. DeviceID=%s.\n",  asyncJob->devid);
			rc = ISMRC_BadTopic;
		}
	}
	//Validate the Device Type
	if (!rc && doDevTypeMatch) {
		/* Make sure clientID matches regex */
		if (ism_regex_match(devTypeMatch, asyncJob->devtype)) {
			TRACE(5, "proxy_checkAuthorizationOnTimer: Device Type id is not valid. DeviceType=%s.\n",  asyncJob->devtype);
			rc = ISMRC_BadTopic;
		}
	}

	//Validate authtoken
	if (!rc && asyncJob->authtoken == NULL) {
		TRACE(5, "proxy_checkAuthorizationOnTimer: Internal Error, Authentication token is NULL.\n");
		rc = ISMRC_Error;
	}

	if (!rc && !g_checkAuthorization){
		//Consider it is authorized because the check is not set
		// Since RC = 0, error buffer is not used, provided here just in case.
		char buf[16];
		rc = ISMRC_OK;
		TRACE(5, "proxy_checkAuthorizationOnTimer: Authorization check is not set.\n");
		ism_proxy_completeAuthorize(asyncJob, rc, ism_common_getErrorString(rc, buf, sizeof buf));
		return 0;
	}

	if (rc) {
		char xbuf[4096];
		ism_common_setError(rc);
		ism_proxy_completeAuthorize(asyncJob, rc, ism_common_getErrorString(rc, xbuf, sizeof xbuf));
		return 0;
	}

	JNIEnv * env = g_timer0_env;
	rc = (*env)->PushLocalFrame(env, 16);
	if(rc == 0) {
		jstring 	jclient  = (jstring)(*env)->NewStringUTF(env, transport->clientID);
		jstring jaddr    = (jstring)(*env)->NewStringUTF(env, transport->client_addr);
		jint    	jaction   = asyncJob->action;
		jint    	jpermissions   = asyncJob->permissions;
		jstring 	jdevtype = (jstring)(asyncJob->devtype ? (*env)->NewStringUTF(env, asyncJob->devtype) : NULL);
		jstring 	jdevid = (jstring)(asyncJob->devid ? (*env)->NewStringUTF(env, asyncJob->devid) : NULL);
		jstring 	juid = (jstring)(transport->userid ? (*env)->NewStringUTF(env, asyncJob->transport->userid) : NULL);
		jbyteArray 	jauthToken = NULL;
		if(asyncJob->authtoken){
			jauthToken = (*env)->NewByteArray(env, asyncJob->authtokenLen);
			(*env)->SetByteArrayRegion(env,jauthToken,0,asyncJob->authtokenLen, (jbyte*)asyncJob->authtoken);
		}
		jint  		jauthToken_len = asyncJob->authtokenLen;
		jlong   	jcorrelate = (jlong)((uint64_t)((uintptr_t)asyncJob));

		if(UNLIKELY((jdevtype == NULL) || (jdevid == NULL) || (jauthToken == NULL))) {
			rc = ISMRC_AllocateError;
		}

		if(rc == 0) {
			rc = (*env)->CallIntMethod(env, g_checkAuthorizationObj, g_checkAuthorization,
					jclient, jaddr, jaction, jpermissions, jdevtype, jdevid, juid, jauthToken, jauthToken_len, jcorrelate);
			//TODO: Check rc
			rc = 0;
			ex = ((*env)->ExceptionOccurred(env));
			if (ex) {
				logJavaException(env, ex);
				(*env)->ExceptionDescribe(env);
				(*env)->ExceptionClear(env);
	            (*env)->DeleteLocalRef(env, ex);
				rc = ISMRC_Error; //TODO: Should this be another error code
			}
		}
		//TODO: Handle error
		(*env)->PopLocalFrame(env, NULL);
		return 0;
	}
	//TODO: Handle error
    ex = ((*env)->ExceptionOccurred(env));
    if (ex) {
        logJavaException(env, ex);
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, ex);
    }
    rc = ISMRC_AllocateError;
	ism_common_setError(rc);
    /**
     * TODO: Should callback to async job???
     */
    TRACE(3, "proxy_checkAuthorizationOnTimer: Exit rc=%d.\n", rc);
	return 0;
}

/**
 * Perform authorization check
 * @param 			asyncJob async job object
 * @return zero for success, otherwise, it is failure.
 */
int ism_proxy_checkAuthorization(asyncauth_t *asyncJob)
{

	TRACE(8, "ism_proxy_checkAuthorization: Submit proxy_checkAuthorizationOnTimer asyncJob=%p\n", asyncJob);
	ism_common_setTimerOnce(ISM_TIMER_HIGH, proxy_checkAuthorizationOnTimer, asyncJob, 1);
	return 1;
}
#endif

/**
 * Free MHub Credential Request object
 */
static void freeMHubCredReq(mhub_cred_req_t * req){
	if(req!=NULL){
		if(req->orgId) ism_common_free(ism_memory_proxy_javaconfig,req->orgId);
		if(req->serviceId) ism_common_free(ism_memory_proxy_javaconfig,req->serviceId);
		ism_common_free(ism_memory_proxy_javaconfig,req);
	}
}

/**
 * Call proxyConnector to get the MHub Credential
 */

static int proxy_getMHubCredentialOnTimer(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	jthrowable ex = NULL;
	int  rc = 0;
	mhub_cred_req_t * cred_reg = (mhub_cred_req_t *)userdata;

	ism_common_cancelTimer(key);

	TRACE(6, "proxy_getMHubCredentialOnTimer: OrgId=%s ServiceId=%s.\n", cred_reg->orgId, cred_reg->serviceId);

	JNIEnv * env = g_timer0_env;
	rc = (*env)->PushLocalFrame(env, 16);
	if(rc == 0) {
		jstring 	jorgId  = (jstring)(*env)->NewStringUTF(env,  cred_reg->orgId);
		jstring 	jserviceId  = (jstring)(*env)->NewStringUTF(env, cred_reg->serviceId);

		if(UNLIKELY((jorgId == NULL) ||  (jserviceId == NULL))) {
			rc = ISMRC_AllocateError;
		}

		if(rc == 0) {
			rc = (*env)->CallIntMethod(env, g_mHubCredMgrObj, g_getMHubCredential,
					jorgId, jserviceId);
			//TODO: Check rc
			rc = 0;
			ex = ((*env)->ExceptionOccurred(env));
			if (ex) {
				logJavaException(env, ex);
				(*env)->ExceptionDescribe(env);
				(*env)->ExceptionClear(env);
	            (*env)->DeleteLocalRef(env, ex);
				rc = ISMRC_Error; //TODO: Should this be another error code
			}
		}
		//TODO: Handle error
		(*env)->PopLocalFrame(env, NULL);
		freeMHubCredReq(cred_reg);
		return 0;
	}
	//TODO: Handle error
    ex = ((*env)->ExceptionOccurred(env));
    if (ex) {
        logJavaException(env, ex);
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, ex);
    }
    rc = ISMRC_AllocateError;
	ism_common_setError(rc);
	freeMHubCredReq(cred_reg);
    TRACE(3, "proxy_getMHubCredentialOnTimer: Exit rc=%d.\n", rc);
	return 0;
}

/**
 * Get MHub Credential
 * @param 			name service id or org name
 * @return zero for success, otherwise, it is failure.
 */
int ism_proxy_getMHubCredential(const char * orgId, const char * serviceId )
{
	if(orgId==NULL){

		TRACE(8, "ism_proxy_getMHubCredential: OrgID is null.\n");
		return 1;
	}

	if(serviceId==NULL){

		TRACE(8, "ism_proxy_getMHubCredential: ServiceID is null.\n");
		return 1;
	}

	mhub_cred_req_t * cred_reg = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,226),1, sizeof(mhub_cred_req_t));
	cred_reg->orgId = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,1000),orgId);
	cred_reg->serviceId = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,1000),serviceId);

	TRACE(8, "ism_proxy_getMHubCredential: Submit job to get MHub Credential. orgId=%s serviceId=%s\n", orgId, serviceId);
	ism_common_setTimerOnce(ISM_TIMER_HIGH, proxy_getMHubCredentialOnTimer, cred_reg, 1);
	return 0;
}

extern int ism_proxy_completeAuthorize(void * correlate, int rc, const char * reason);
/**
 * Set the result for the check of device authorization
 *
 * Method: doAuthorized
 * Signature: (JII)
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doAuthorized(JNIEnv * env, jobject thisobj, jlong jcorrelation, jint rc, jstring jreason)
{
    ism_common_makeTLS(512,NULL);
    TRACE(6, "doAuthorized: RC:%d\n ", rc);
	xUNUSED void * correlation = (void *)(uintptr_t *)jcorrelation;

	uint16_t * ureason;
	const char * reason = make_javastr(env, jreason, &ureason);

#ifndef NO_PROXY
	ism_proxy_completeAuthorize(correlation, rc, reason);
#endif

	free_javastr(env, jreason, reason, ureason);
	ism_common_freeTLS();
	return;
}


/*
 * Authenticate a connection.
 * Continue on the rest of the connection
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doAuth(
        JNIEnv * env, jobject thisobj, jlong jtransport, jboolean good, jint rc) {
    ism_common_makeTLS(512,NULL);
    authAction_t * action = (authAction_t *)jtransport;
    int lrc = 0;
    double authCompleteTime = ism_common_readTSC();
    ism_time_t authTime = ism_common_currentTimeNanos();
    TRACE(9, "Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doAuth: connection=%u\n", action->transport->index);

    if (g_AAAEnabled){
			/* Extract RC and Permissions */
		lrc = rc >> 24;
		int iOperations = rc & 0xffffff;
		if (lrc==ISMRC_OK && action!=NULL && action->transport!=NULL) {
			action->transport->auth_permissions = (uint32_t)iOperations;
		}
    } else {
    	lrc = rc;
    }

    if (lrc==ISMRC_OK) {
        if (!good)
            rc = ISMRC_NotAuthenticated;
        else
        	rc = ISMRC_OK;
    } else if (lrc==2) {
        rc = ISMRC_AuthUnavailable;
    } else {
        rc = ISMRC_Error;
    }
    action->callback(rc, action->callbackParam);
    pthread_spin_lock(&authStatLock);
    double d = authCompleteTime - action->authStartTime;
    if (d > authStats.maxAuthenticationResponseTime)
        authStats.maxAuthenticationResponseTime = d;
    authStats.authenticationResponseTime += d;
    authStats.authenticationRequestsCount++;
    g_authCount++;
    g_authTimeD += d;
    g_toAuthTime += authTime - action->transport->connect_time;
    if(action->throttled)
        authStats.authenticationThrottleCount++;
    pthread_spin_unlock(&authStatLock);
    ism_common_free(ism_memory_proxy_auth,action);
    ism_common_freeTLS();
}

/*
 * Update the auth stats by time
 */
void ism_proxy_updateAuth(ism_time_t timestamp) {
    pthread_spin_lock(&authStatLock);
    g_lastAuthCount = g_authCount;
    g_lastAuthTimeD = g_authTimeD;
    g_lastToAuthTime = g_toAuthTime;
    g_lastStats = g_nextStats ? g_nextStats : ism_common_currentTimeNanos();
    g_nextStats = timestamp;
    g_authCount = 0;
    g_authTimeD = 0.0;
    g_toAuthTime = 0;
    pthread_spin_unlock(&authStatLock);
}

static int g_authStatsCount = 0;
int g_shuttingDown = 0;


XAPI int ism_transport_lbcount(void);

/*
 * Wait until we get a #authstats request from load balancer.
 * If we have never received one, or we timeout continue with the shutdown.
 */
void ism_proxy_shutdownWait(void) {
    int lb_count;
    int shutdown_wait = ism_common_getIntConfig("ShutdownWait", 3000);
    pthread_spin_lock(&authStatLock);
    g_shuttingDown = 1;
    lb_count = g_authStatsCount;
    pthread_spin_unlock(&authStatLock);
    if (lb_count == 0 || shutdown_wait <= 0)
        return;

    TRACE(5, "Shutdown wait for LB query for up to %u ms\n", shutdown_wait);
    double endtime = ism_common_readTSC() + ((double)shutdown_wait/1000.0);
    do {
        ism_common_sleep(100000);
        if (ism_transport_lbcount() == 0) {
            ism_common_sleep(2000);  /* 2 ms */
            return;
        }
    } while (ism_common_readTSC() < endtime);
}

/*
 * Delay shutdown as configured.
 * This is to allow short lived connections to complete after we have stopped
 * the load balancer from sending us new connections.
 */
void ism_proxy_shutdownDelay(void) {
    int shutdown_delay = ism_common_getIntConfig("ShutdownDelay", 500);
    if (shutdown_delay > 0) {
        TRACE(5, "Shutdown delay for %u ms\n", shutdown_delay);
        ism_common_sleep(shutdown_delay*1000);
    }
}


/*
 * Return the authrate for the REST interface
 */
void ism_proxy_getAuthLBStats(ism_transport_t * transport, char * buf, int len) {
    ism_time_t now = ism_common_currentTimeNanos();
    double   delta = (double)(now - g_lastStats) / 1e9;
    uint32_t authrate;
    uint32_t toauthrate;
    uint64_t count;
    uint32_t countper;

    pthread_spin_lock(&authStatLock);
    g_authStatsCount++;
    if (g_shuttingDown) {
        *buf = 0;
        transport->endpoint->lb_count = 0;
    } else {
        /* Handle the unexpected case that the time delta is zero */
        transport->endpoint->lb_count++;
        if (delta <= 0.0) {
            ism_common_strlcpy(buf, "0,0,0", len);
        } else {
            count = g_lastAuthCount + g_authCount;
            // printf("count=%ld total=%lu delta=%f\n", count, (g_lastToAuthTime + g_toAuthTime), (delta/1e9));
            authrate = (uint32_t)(((g_lastAuthTimeD + g_authTimeD) * 1e6) / count / delta);
            toauthrate = (uint32_t)(((g_lastToAuthTime + g_toAuthTime) * 1e-3) / count / delta);
            countper = (uint32_t)(count / delta + 0.5);
            snprintf(buf, len, "%u,%u,%u", countper, authrate, toauthrate);
        }
    }
    pthread_spin_unlock(&authStatLock);
}

/*
 * This is called for connections which do not use external authentication.
 * As the external auth stats are used to deal with the performance of the
 * external authenticator, its times are not alterned.
 * @param transport  The transport object
 * @param startime_tsc  The authentication start time from ism_common_readTSC()
 */
void ism_proxy_auth_now(ism_transport_t * transport, double starttime_tsc) {
    double delta = ism_common_readTSC() - starttime_tsc;
    ism_time_t authTime = ism_common_currentTimeNanos();
    int64_t  deltans = authTime - transport->connect_time;

    if (delta < 0.0)
        delta = 0.0;

    pthread_spin_lock(&authStatLock);
    g_authCount++;
    g_authTimeD += delta;
    g_toAuthTime += deltans;
    pthread_spin_unlock(&authStatLock);
}


/*
 * Set the authenticator
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doSetAuth(
        JNIEnv * env, jobject thisobj, jboolean useauth) {
    ism_common_makeTLS(512,NULL);
    jthrowable ex;
    if (useauth) {
        g_authobj = (*env)->NewGlobalRef(env, thisobj);
        g_authenticator = (*env)->GetMethodID(env, ImaProxyImpl, "authenticate",
                "(Ljava/lang/String;Ljava/lang/String;IIIIILjava/lang/String;Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;JIJ)I");
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            logJavaException(env, ex);
            g_authenticator = NULL;
        }
    } else {
        g_authenticator = NULL;
    }
    ism_common_freeTLS();
}

/*
 * Set the authenticator
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doSetDeviceUpdater(
        JNIEnv * env, jobject thisobj, jboolean usedeviceupdater) {
    ism_common_makeTLS(512,NULL);
    jthrowable ex;
    if (usedeviceupdater) {
        g_deviceupdaterobj = (*env)->NewGlobalRef(env, thisobj);
        g_devicestatusupdate = (*env)->GetMethodID(env, ImaProxyImpl, "deviceStatusUpdate",
                "(Ljava/lang/String;)I");
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            logJavaException(env, ex);
            g_deviceupdaterobj = NULL;
        }
    } else {
    	g_deviceupdaterobj = NULL;
    }
    ism_common_freeTLS();
}

/*
 * Set the authenticator
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doSetMHUBCredentialManager(
        JNIEnv * env, jobject thisobj, jboolean useMHubCredMgr) {
    ism_common_makeTLS(512,NULL);
    jthrowable ex;
    if (useMHubCredMgr) {
    		TRACE(1, "doSetMHUBCredentialManager: Set MHub Credential Manager.\n");
    		g_mHubCredMgrObj = (*env)->NewGlobalRef(env, thisobj);
    		g_getMHubCredential = (*env)->GetMethodID(env, ImaProxyImpl, "getMHubCredential",
                "(Ljava/lang/String;Ljava/lang/String;)I");
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            logJavaException(env, ex);
            g_mHubCredMgrObj = NULL;
        }
    } else {
    		g_mHubCredMgrObj = NULL;
    }
    ism_common_freeTLS();
}

/*
 * Set the authenticator
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doSetAuthorize(
        JNIEnv * env, jobject thisobj, jboolean usecheckauth) {
    ism_common_makeTLS(512,NULL);
    jthrowable ex;
    if (usecheckauth) {
    	g_checkAuthorizationObj = (*env)->NewGlobalRef(env, thisobj);
    	g_checkAuthorization = (*env)->GetMethodID(env, ImaProxyImpl, "authorize",
                "(Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BIJ)I");
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            logJavaException(env, ex);
            g_checkAuthorizationObj = NULL;
        }
    } else {
    	g_checkAuthorizationObj = NULL;
    }
    ism_common_freeTLS();
}

#ifndef NO_PROXY
//static int deviceStatusUpdate(ism_timer_t key, ism_time_t timestamp, void * userdata) {
/*
 * Run proxy authenticate in the timer thread
 */
//static deviceStatusUpdate(const char *  clientID, const char * clientAddress, int port,
//		const char * protocol, const char * userID, const char * action, ism_time_t connectedTime) {
static int deviceStatusUpdate(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    jthrowable ex = NULL;
    int  rc = 0;
    char * deviceStatusJson = (char *)userdata;
    JNIEnv * env = g_timer0_env;
	ism_common_cancelTimer(key);

    rc = (*env)->PushLocalFrame(env, 16);
    if(rc == 0) {
        jstring jdeviceStatusJson  		= (jstring)(*env)->NewStringUTF(env,deviceStatusJson);

        if(UNLIKELY((jdeviceStatusJson == NULL)))
            rc = ISMRC_AllocateError;

        if(rc == 0) {
            rc = (*env)->CallIntMethod(env, g_deviceupdaterobj, g_devicestatusupdate,
            		jdeviceStatusJson);
            //TODO: Check rc
            rc = 0;
            ex = ((*env)->ExceptionOccurred(env));
            if (ex) {
                logJavaException(env, ex);
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
                rc = ISMRC_Error; //TODO: Should this be another error code
            }
        }
        (*env)->PopLocalFrame(env, NULL);
    } else {
        rc = ISMRC_AllocateError;
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            logJavaException(env, ex);
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, ex);
        }

    }
    if(rc) {
        ism_common_setError(rc);
    }

    ism_common_free(ism_memory_proxy_javaconfig,deviceStatusJson);
    return 0;
}


/*
 * Submit a authenticator job to the timer thread
 */
int ism_proxy_deviceStatusUpdate(ism_transport_t * transport, int event,  int ec, const char * reason) {

	if (g_deviceupdatestatus_enabled && g_deviceupdaterobj) {

        char xbuf [4096];
    	concat_alloc_t buf = {xbuf, sizeof xbuf};
    	ism_proxy_jsonMessage(&buf, transport, event, ec, reason);

    	char * statusJson = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,229),buf.used + 1);
    	memcpy(statusJson, buf.buf ,buf.used);
    	statusJson[buf.used]='\0';

   		ism_common_setTimerOnce(ISM_TIMER_HIGH, deviceStatusUpdate, statusJson, 1);

    }
    return 0;
}
#endif

/*
 * Get the thread count
 */
JNIEXPORT int JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getThreadCount(JNIEnv * env, jobject thisobj) {
    //Note this does not call ism_common_makeTLS(512,NULL); so must not allocate memory
    return ism_common_getIntConfig("AuthThreadCount", 3);
}

/*
 * Write an entry in the proxy log.
 *
 * Method:    doLog
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doLog(
    JNIEnv * env, jobject thisobj, jstring msgid, jstring msgformat, jstring filen, jint lineno, jstring repl) {
    ism_common_makeTLS(512,NULL);
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[100];
    char types[64];
    const char * r[16] = {0};
    int  count;
    int  i;
    xUNUSED int  rc;
    uint16_t * u_msgid;
    uint16_t * u_msgformat;
    uint16_t * u_repl;
    uint16_t * u_filen;
    const char * c_msgid     = make_javastr(env, msgid,     &u_msgid);
    const char * c_msgformat = make_javastr(env, msgformat, &u_msgformat);
    const char * c_repl      = make_javastr(env, repl,      &u_repl);
    const char * c_filen     = make_javastr(env, filen,     &u_filen);

    /*
     * Parse the repl
     */
    parseobj.ent_alloc = 100;
    parseobj.ent = ents;
    parseobj.source = (char *)c_repl;
    parseobj.src_len = strlen(c_repl);
    rc = ism_json_parse(&parseobj);
    count = parseobj.ent_count - 1;
    if (count > 16)
        count = 16;
    types[0] = 0;
    for (i=0; i<count; i++) {
        switch(parseobj.ent[i+1].objtype) {
        case JSON_String:
            r[i] = parseobj.ent[i+1].value;
            strcat(types, "%-s");
            break;
        case JSON_Integer:
        case JSON_Number:
            r[i] = parseobj.ent[i+1].value;
            strcat(types, "%s");
            break;
        case JSON_True:
            r[i] = "true";
            strcat(types, "%s");
            break;
        case JSON_False:
            r[i] = "false";
            strcat(types, "%s");
            break;
        case JSON_Null:
            r[i] = "null";
            strcat(types, "%s");
            break;
        default:
            i = count;
            break;
        }
    }

    /* Force the log */
    ism_trclevel_t trclvl = {0};
    trclvl.logLevel[LOGGER_SysLog] = AuxLogSetting_Max;

    /*
     * Do the log
     */
    ism_common_logInvoke(NULL, ISM_LOGLEV_NOTICE, 0, c_msgid, LOGCAT_Server, &trclvl, "log",
            c_filen, lineno, types, c_msgformat, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7],
            r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);

    /*
     * Free up the strings
     */
    if (parseobj.free_ent)
        ism_common_free(ism_memory_utils_parser,parseobj.ent);
    free_javastr(env, msgid,     c_msgid,     u_msgid);
    free_javastr(env, msgformat, c_msgformat, u_msgformat);
    free_javastr(env, repl,      c_repl,      u_repl);
    free_javastr(env, filen,     c_filen,     u_filen);
    ism_common_freeTLS();
    return (jstring)NULL;
}


#define JLONG_TO_PTR(x) ((void*)(intptr_t)(x))
#define PTR_TO_JLONG(x) ((jlong)(intptr_t)(x))

/*
 * Handle using older header files
 */
#ifndef JNI_VERSION_1_6
#define JNI_VERSION_1_6 0x00010006
#endif

/*
 * Variables
 */
       JavaVM *        jvm = NULL;             /* The JVM                      */
static JNIEnv *        java_env;               /* Init thread only             */
static int             java_loaded = 0;        /* The JVM loaded flag          */
static const char *    java_home;              /* The Java home directory      */
static const char *    java_jvm;               /* The JVM name                 */
static const char *    java_opt;             /* The JVM options                  */
static const char *    java_classpath;       /* The JVM classpath                */
static const char *    java_config;
static ism_threadh_t   javaThread;
static JavaVMOption * java_options;    /* The java options (init only)     */
static int       option_count = 0;     /* The option count (init only)     */
static int       option_max   = 0;     /* Allocated options slots          */

static pthread_mutex_t  java_lock;     /* Mutex lock                       */


/*
 * Local prototypes
 */
static int   loadjvm(void);
static int   addOption(char * str, void * info);
static void  parseOptions(char * str);
static int   findjvm(char * jvmdll, int len, const char * jvmname);

/*
 * Library level initialization
 */
void javafuncinit(void) {
    pthread_mutex_init(&java_lock, 0);
}

/*
 * Log a java exception
 */
void logJavaException(JNIEnv *  env, jthrowable ex) {
    jclass cls = (*env)->GetObjectClass(env, ex);
    jmethodID mid_toString = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
    jstring msg = (*env)->CallObjectMethod(env, ex, mid_toString);
    const char * cmsg = (*env)->GetStringUTFChars(env, msg, 0);
    (*env)->ReleaseStringUTFChars(env, msg, cmsg);
    LOG(CRIT, Server, 943, "%s", "Exception: {0}", cmsg);
    ism_common_sleep(10000);
}

/*
 * Initialize the java within the proxy process
 */
static void * java_listener_proc(void * vclassname, void * context, int value) {
    jclass    cls;
    jobject   proxy;
    jmethodID constructor = NULL;
    jmethodID run;
    jobject   userinst;
    JNIEnv *  env;
    JavaVMAttachArgs aarg;
    int       rc;
    jthrowable ex;

    aarg.version = JNI_VERSION_1_6;         /* Minimum supported Java level */
    aarg.name = "javaconfig";
    aarg.group = NULL;
    rc = (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void * *)&env, &aarg);
    if (rc != 0) {
        TRACE(2, "Unable to attach Java thread: rc=%d\n", rc);
        return (void *)(uintptr_t)rc;
    }
    TRACE(5, "Attach Java config thread to JVM\n");

    const char * classname = (const char *)vclassname;

    /*
     * Find our implementation class
     */
    cls = (*env)->FindClass(env, "com/ibm/ima/proxy/impl/ImaProxyImpl");
    if (cls) {
        constructor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
    } else {
        constructor = NULL;
    }
    if (!constructor) {
        LOG(ERROR, Server, 944, "%s", "Unable to find Java class: {0}", "ImaProxyImpl");
        TRACE(1, "ImaProxyImpl class not found\n");
        (*jvm)->DetachCurrentThread(jvm);
        ism_common_sleep(100000);
        ism_common_shutdown(0);
    }
    ImaProxyImpl = cls;

    jint aaaEnabled = (jint) ism_common_getIntConfig("AAAEnabled", 0);
    jint sgEnabled = (jint) ism_common_getIntConfig("SGEnabled", 0);

    proxy = (*env)->NewObject(env, cls, constructor, aaaEnabled, sgEnabled);
    ex = ((*env)->ExceptionOccurred(env));
    if (ex) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        logJavaException(env, ex);
        (*jvm)->DetachCurrentThread(jvm);
        ism_common_shutdown(0);
    }

    /*
     * Find the user class
     */
    cls = (*env)->FindClass(env, classname);
    if (cls) {
        constructor = (*env)->GetMethodID(env, cls, "<init>", "(Lcom/ibm/ima/proxy/ImaProxyListener;)V");
    } else {
        constructor = NULL;
    }
    if (!constructor) {
        LOG(ERROR, Server, 944, "%s", "Unable to find Java class: {0}", classname);
        TRACE(1, "JavaConfig class not found: %s\n", classname);
        ism_common_sleep(100000);
        (*jvm)->DetachCurrentThread(jvm);
        ism_common_shutdown(0);
    }

    /*
     * Instantiate the user class.
     * We assume that the Java consturctor is handling its own errors, but if it throws an
     * exception terminate the proxy.
     */
    TRACE(4, "JavaConfig instantiate the configuration class\n");
    userinst = (*env)->NewObject(env, cls, constructor, proxy);
    ex = ((*env)->ExceptionOccurred(env));
    if (ex) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        logJavaException(env, ex);
        (*jvm)->DetachCurrentThread(jvm);
        ism_common_shutdown(0);
    }

    /*
     * Give this thread to the java config class using the run() method
     */
    run = (*env)->GetMethodID(env, cls, "run", "()V");
    if (run) {
        TRACE(4, "JavaConfig start run method\n");
        (*env)->CallVoidMethod(env, userinst, run);
        ex = ((*env)->ExceptionOccurred(env));
        if (ex) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            logJavaException(env, ex);
            (*jvm)->DetachCurrentThread(jvm);
            ism_common_shutdown(0);
        }
    } else {
        TRACE(2, "JavaConfig unable to find run method\n");
    }

    /*
     * If the run method returns, end the java config thread
     */
    return (void *)0;
}

/*
 * Connect timer0 (the high priority timer) to the JVM.
 */
static int timer0_init_jvm(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    JavaVMAttachArgs aarg = {0};
	ism_common_cancelTimer(key);
    aarg.version = JNI_VERSION_1_6;         /* Minimum supported Java level */
    aarg.name = "timer0";
    aarg.group = NULL;
    (*jvm)->AttachCurrentThreadAsDaemon(jvm, (void * *)&g_timer0_env, &aarg);
    return 0;
}

/*
 * Init when we are sure there is no java
 */
void ism_proxy_nojavainit(void) {
    pthread_spin_init(&authStatLock, 0);
}

/*
 * Initialize java configuration if requested
 */
int ism_proxy_javainit(int step) {
    pthread_spin_init(&authStatLock, 0);
    java_config = ism_common_getStringConfig("JavaConfig");
    if(step == 1 ) {
	    if (java_config && *java_config) {
        	/* Get the Java config */
        	java_home = ism_common_getStringConfig("JavaHome");
        	if (!java_home || !*java_home)
            	  java_home = getenv("JAVA_HOME");

                java_jvm = ism_common_getStringConfig("JavaJVM");

                java_opt = ism_common_getStringConfig("JavaOptions");
                if (!java_opt || !*java_opt)
                  java_opt = "-Xrs";

                java_classpath = ism_common_getStringConfig("Classpath");
                if (!java_classpath || !*java_classpath)
                  java_classpath = getenv("CLASSPATH");
                if (!java_classpath || !*java_classpath)
                  java_classpath = ".";

                /* connect to the JVM */
                loadjvm();

                if (java_loaded > 0) {
                  return 0;
                }
                return 1;

            }
            return 0;
    }
    if(java_config && *java_config) {
        ism_common_setTimerOnce(ISM_TIMER_HIGH, timer0_init_jvm, NULL, 1);
        ism_common_sleep(1000);          /* Make sure init gets run */

    	ism_common_startThread(&javaThread, java_listener_proc, (void *)java_config, NULL, 0,
                    ISM_TUSAGE_NORMAL,  0, "javaconfig", "The java configuration thread");
    }
    return 0;
}


/*
 * If the JavaJVM is specified, use it
 * Otherwise look in lib/amd64/classic (or IBM) and then lib/amd64/server (for Oracle)
 */
static int findjvm(char * jvmdll, int len, const char * jvmname) {
    if (!jvmname || !*jvmname) {
        snprintf(jvmdll, len, "%s/jre/lib/amd64/classic/%s", java_home, JVMDLLNAME);
        if (!access(jvmdll, CANREAD))
            return 0;
        snprintf(jvmdll, len, "%s/lib/amd64/classic/%s", java_home, JVMDLLNAME);
        if (!access(jvmdll, CANREAD))
            return 0;
        jvmname = "lib/amd64/server";
    }
    snprintf(jvmdll, len, "%s/jre/%s/%s", java_home, jvmname, JVMDLLNAME);
    if (!access(jvmdll, CANREAD))
        return 0;
    snprintf(jvmdll, len, "%s/%s/%s", java_home, jvmname, JVMDLLNAME);
    if (!access(jvmdll, CANREAD))
        return 0;
    TRACE(4, "JVM not found: %s\n", jvmdll);
    return -1;
}


/*
 * Set the classpath
 */
static void setclasspath(const char * classpath, char * buf, int len) {
    snprintf(buf, len, "-Djava.class.path=%s", classpath);
    addOption(buf, NULL);
}





typedef jint ( JNICALL * createJavaVM_t)(JavaVM * * pvm, void * * penv, void * pargs);

/*
 * Load the JVM
 */
static int loadjvm(void) {
    char jvmpath[P_LEN];
    char   errbuf[300];
    int    rc = 0;
    int    i;
    void * jmodule;
    JavaVMInitArgs args;
    JNIEnv * env;
    createJavaVM_t XCreateJavaVM;

    errbuf[0] = 0;
    pthread_mutex_lock(&java_lock);
    if (java_loaded < 0) {
        pthread_mutex_unlock(&java_lock);
        return java_loaded;
    }


    if (findjvm(jvmpath, P_LEN, java_jvm)) {
        LOG(ERROR, Server, 940, "%s", "Unable to find Java VM: {0}", JVMDLLNAME);
        java_loaded = -1;
        pthread_mutex_unlock(&java_lock);
        return -1;
    }

    XCreateJavaVM = NULL;
    jmodule = dlopen(jvmpath, RTLD_LAZY);
    if (!jmodule) {
        ism_common_strlcpy(errbuf, dlerror(), sizeof(errbuf));
    } else {
        XCreateJavaVM = (createJavaVM_t) dlsym(jmodule, "JNI_CreateJavaVM");
    }

    if (!XCreateJavaVM) {
#pragma GCC diagnostic push
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wformat-truncation="
#endif
        if (!*errbuf)
            snprintf(errbuf, sizeof(errbuf), "JNI_CreateJavaVM entry point not found: %s", jvmpath);
#pragma GCC diagnostic pop
        TRACE(5, "Unable to load the Java VM: %s %s\n", jvmpath, errbuf);
        LOG(ERROR, Server, 941, "%s%s", "Unable to load Java VM: {0}: {1}", jvmpath, errbuf);
        java_loaded = -1;
        pthread_mutex_unlock(&java_lock);
        dlclose(jmodule);
        return -2;
    }

    setclasspath(java_classpath, jvmpath, P_LEN);
    if (java_opt)
        parseOptions(ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,1000),java_opt));

    memset(&args, 0, sizeof(args));
    args.version  = JNI_VERSION_1_6;
    args.nOptions = option_count;
    args.options  = java_options;
    args.ignoreUnrecognized = JNI_FALSE;

    TRACE(5, "JavaVM args: version=0x%08x, Option count=%d\n",
             (int)args.version, (int)args.nOptions);
    for (i = 0; i < option_count; i++) {
        TRACE(5, "   Option[%2d] = '%s'\n", i, args.options[i].optionString);
    }

    rc = XCreateJavaVM(&jvm, (void **)&env, &args);
    for (i=0; i<option_count; i++) {
        if (java_options[i].optionString)
            ism_common_free(ism_memory_proxy_javaconfig,java_options[i].optionString);
    }
    ism_common_free(ism_memory_proxy_javaconfig,java_options);
    option_max = 0;
    if (rc) {
        TRACE(5, "Unable to create Java VM: rc=%d\n", rc);
        LOG(ERROR, Server, 942, "%d", "Unable to create Java VM: rc={0}", rc);
        java_loaded = -1;
        pthread_mutex_unlock(&java_lock);
        dlclose(jmodule);
        return -3;
    }
    java_env = env;

    pthread_mutex_unlock(&java_lock);

    java_loaded = 1;
    return rc;                     /* BEAM suppression:  file leak */
}



/*
 * Add an option
 */
int addOption(char * str, void * info) {
    if (option_count >= option_max) {
        if (java_options == 0) {
            option_max  = 64;
            java_options = (JavaVMOption *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,233),option_max, sizeof(JavaVMOption));
        } else {
            JavaVMOption * tmp;
            option_max *= 4;
            tmp = (JavaVMOption *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,234),option_max, sizeof(JavaVMOption));
            memcpy(tmp, java_options, option_count * sizeof(JavaVMOption));
            ism_common_free(ism_memory_proxy_javaconfig,java_options);
            java_options = tmp;
        }
    }
    java_options[option_count].optionString = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_javaconfig,1000),str);
    java_options[option_count++].extraInfo = info;
    return option_count - 1;
}


/*
 * Parse a string of options into a JVM options structure
 */
void parseOptions(char * str) {
    char * start = NULL;
    for (;;) {
        if (*str==0 || *str==' ' || *str=='\t') {
            if (start) {
                char  save = *str;
                *str = 0;
                addOption(start, 0);
                *str = save;
                start = NULL;
            }
        } else {
            if (!start)
                start = str;
        }
        if (!*str)
            break;
        str++;
    }
}


int ism_proxy_getAuthStats(px_auth_stats_t * stats) {
    pthread_spin_lock(&authStatLock);
    memcpy(stats, &authStats, sizeof(px_auth_stats_t));
    authStats.maxAuthenticationResponseTime = 0;
    authStats.authenticationRequestsCount = 0;
    authStats.authenticationResponseTime = 0;
    authStats.authenticationThrottleCount = 0;
    pthread_spin_unlock(&authStatLock);
    return 0;
}
