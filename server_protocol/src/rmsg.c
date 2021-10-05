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

#include <transport.h>
#include <engine.h>

#include <protocol.h>
#include "imacontent.h"
#include <ismmessage.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>


/*
 * Protocol states
 */
#define RMSG_NEW            0
#define RMSG_IN_PROGRESS    1
#define RMSG_STOLEN         2
#define RMSG_CONNECTED      3
#define RMSG_DISCONNECTED   4

/*
 * Action structure for async return
 */
typedef struct rmsg_action_t {
    uint8_t        action;
    uint8_t        flags;
    uint32_t       value;
    ism_http_t *   http;
    ism_transport_t * transport;
} rmsg_action_t;

/*
 * Message cache
 */
typedef struct rmsg_cache_t {
    struct rmsg_cache_t * next;
    uint32_t   content_length;
    uint8_t    message_type;
    uint8_t    flags;
    uint16_t   topic_length;
} rmsg_cache_t;

typedef struct rmsg_sub_t {
    struct rmsg_sub_t * next;
    void *              handle;
    uint8_t             durable;
    char                topic [1];
} rmsg_sub_t;

/*
 * The rest protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    struct ism_protobj_t *  next;
    struct ism_protobj_t *  prev;
    void *              client_handle;     /* Client handle            */
    void *              session_handle;    /* Session handle           */
    rmsg_action_t       consumer;
    rmsg_sub_t   *      subs;
    rmsg_cache_t *      cachehead;
    rmsg_cache_t *      cachetail;
    uint32_t            inprogress;
    int32_t             keepAlive;
    volatile int        closed;             /* Connection is not is use */
    pthread_spinlock_t  lock;
    pthread_spinlock_t  sessionlock;
    uint8_t             kind;
    volatile uint8_t    startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t             resv2[2];
    int32_t             state;
    int32_t             suspended;
    ismHashMap *        errors;            /* Errors that were reported already */
} rmsg_pobj_t;



/*
 * Subprotocol actions
 */
#define SUBPROT_Get              1
#define SUBPROT_GetRetain        2
#define SUBPROT_Publish          3
#define SUBPROT_PublishRetain    4
#define SUBPROT_DeleteRetain     5
#define SUBPROT_Subscribe        6
#define SUBPROT_Unsubscribe      7


/*
 * Global variables
 */
static rmsg_pobj_t * clientListHead = NULL;
static rmsg_pobj_t * clientListTail = NULL;
static pthread_mutex_t clientListLock;

/*
 * Forward references
 */
void ism_rmsg_action(int rc, void * handle, void * xact);
void ism_rmsg_replySteal(int32_t reason, ismEngine_ClientStateHandle_t handle, uint32_t options, void * vaction);
void ism_rmsg_putJsonPayloadContent(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray);
void ism_rmsg_removeFromClientList(rmsg_pobj_t * pobj, int lock);
void ism_rmsg_replyCloseClient(ism_transport_t * transport);
static int getBooleanQueryProperty(const char * str, const char * name, int default_val);

/*
 * Scheduled work on completion of authorization (or directly called when already authenticated)
 */
static int httpRestMsgCall(ism_transport_t * transport, void * callbackParam, uint64_t async) {
    ism_http_t * http = (ism_http_t *)callbackParam;
    rmsg_action_t action = {0};
    action.transport = transport;
    action.http = http;
    if (transport->pobj->session_handle) {
        action.action = Action_message;
    } else {
        action.action = Action_createConnection;
    }
    ism_rmsg_action(0, NULL, &action);
    return 0;
}

static void httpReplyError(int rc, int httpRC, ism_http_t * http) {
    ism_transport_t * transport = http->transport;
    rmsg_pobj_t * pobj = (rmsg_pobj_t *) transport->pobj;
    ism_common_setError(rc);
    ism_http_respond(http, httpRC, NULL);
    if (__sync_sub_and_fetch(&pobj->inprogress, 1) < 0) {
        ism_rmsg_replyCloseClient(transport);
    }
}

/*
 * Reply from asynchronous authorization request.
 * This is commonly called in one of the security worker threads, but move further
 * processing to the IOP thread.
 */
static void httpReplyAuth(int authrc, void * callbackParam) {
    ism_http_t * http = * (ism_http_t * *)callbackParam;
    ism_transport_t * transport = http->transport;
    if (authrc) {
        httpReplyError(((authrc != ISMRC_Closed) ? ISMRC_HTTP_NotAuthorized : authrc), 401, http);
    } else {
        transport->authenticated = 1;    /* Do not run auth a second time */
        ism_transport_submitAsyncJobRequest(transport, httpRestMsgCall, http, 1);
    }
}
/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

/*
 * Check if the String is valid.
 * We need to validate some strings, including those which contain topic names.
 * Topic names have additional restrictions including whether the wildcards are
 * allowed as if they are valid.
 */
static int checkTopic(const char * s, int publish, int durable) {
    int i;
    int count;
    int len;
    int check;

    len = s ? strlen(s) : 0;
    check = UR_NoControl | UR_NoNonchar;
    if (publish)
        check |= UR_NoWildcard;

    count = ism_common_validUTF8Restrict(s, len, check);
    if (count < 1) {
        ism_common_setError(ISMRC_BadTopic);
        return 400;
    } else {
        if (*s == '$') {
            if (publish || durable || s[1]!='S' || s[2]!='Y' || s[3]!='S' || s[4]!='/') {
                ism_common_setError(ISMRC_BadSysTopic);
                return 400;
            }
        } else {
            if (!publish) {
                for (i = 0; i < len; i++) {
                    if (s[i] == '#') {
                        if ((i > 0 && s[i - 1] != '/')
                                || (i + 1 != len)) {
                            ism_common_setError(ISMRC_BadTopic);
                            return 400;
                        }
                    } else if (s[i] == '+') {
                        if ((i > 0 && s[i - 1] != '/')
                                || (i + 1 != len && s[i + 1] != '/')) {
                            ism_common_setError(ISMRC_BadTopic);
                               return 400;
                        }
                    }
                }
            }
        }
    }
    return 0;
}


/*
 * Process a message from the engine
 */
bool ism_rmsg_replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction) {
    uint32_t proplen = 0;
    uint32_t bodylen = 0;
    char * propp = NULL;
    char * bodyp = NULL;
    int i;
    bool returncode = true;
    ism_field_t ftopic = {0};
    rmsg_action_t * action;
    ism_transport_t * transport;
    rmsg_pobj_t * pobj;
    concat_alloc_t buf = {NULL, 0};

    action = (rmsg_action_t *) vaction;
    transport = action->transport;
    pobj = transport->pobj;

    /* Find the props and body */
    for (i = 0; i < areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t) areasize[i];
            propp = (char *) areaptr[i];
        } else if (areatype[i] == ismMESSAGE_AREA_PAYLOAD) {
            bodylen = (uint32_t) areasize[i];
            bodyp = (char *) areaptr[i];
        }
    }
    if (proplen) {
        concat_alloc_t pbuf = {propp, proplen, proplen};
        ism_findPropertyNameIndex(&pbuf, ID_Topic, &ftopic);
    }
    switch (hdr->MessageType) {
    case MTYPE_MapMessage:
    case MTYPE_StreamMessage:
        ism_rmsg_putJsonPayloadContent(transport, &buf, bodyp, bodylen, hdr->MessageType != MTYPE_MapMessage);
        bodyp = buf.buf;
        bodylen = buf.used;
        break;
    default:
        break;
    }

    /*
     * If there is a request waiting send the message now
     */
    if (action->action == SUBPROT_GetRetain || action->action == SUBPROT_Get) {
        ism_common_allocBufferCopyLen(&action->http->outbuf, bodyp, bodylen);
        if (ftopic.type == VT_String)
            ism_http_setHeader(action->http, "Topic", ftopic.val.s);
        action->action = 0;
        returncode = false;
    }

    /*
     * Put the message into the cache
     */
    else {
        rmsg_cache_t * cache;
        char * cachedata;
        int topiclen = ftopic.val.s ? strlen(ftopic.val.s)+1 : 0;

        cache = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,2),sizeof(rmsg_cache_t) + topiclen + bodylen);
        cache->content_length = bodylen;
        cache->topic_length = topiclen;
        cache->message_type = hdr->MessageType;
        cache->flags = hdr->Flags;
        cache->next = NULL;
        cachedata = (char *)(cache+1);
        memcpy(cachedata, bodyp, bodylen);
        if (topiclen)
            memcpy(cachedata+bodylen, ftopic.val.s, topiclen);
        pthread_spin_lock(&pobj->lock);
        if (pobj->cachetail == NULL) {
            pobj->cachehead = pobj->cachetail = cache;
        } else {
            pobj->cachetail->next = cache;
            pobj->cachetail = cache;
        }
        pthread_spin_unlock(&pobj->lock);
    }


    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    ism_engine_releaseMessage(msgh);
    return returncode;
}

/*
 * Get a retained message
 */
int ism_rmsg_getRetain(rmsg_action_t * action, ism_http_t * http) {
    int  rc;
    rc = checkTopic(http->user_path+1, 0, 1);
    if (rc)
        return rc;

    action->action = SUBPROT_GetRetain;
    rc = ism_engine_getRetainedMessage(http->transport->pobj->session_handle, http->user_path+1,
            action, sizeof *action, ism_rmsg_replyMessage,
            action, sizeof *action, ism_rmsg_action);
    return rc;
}

/*
 * Get a retained message
 */
int ism_rmsg_get(rmsg_action_t * action, ism_http_t * http, uint32_t timeout) {
    ism_transport_t * transport = action->transport;
    rmsg_pobj_t * pobj = transport->pobj;
    rmsg_cache_t * cache;
    char * cachedata;

    if (pobj->subs == NULL) {
        return ISMRC_NoSubscriptions;
    }

    /*
     * If a message is waiting, send it now
     */
    pthread_spin_lock(&pobj->lock);
    if (pobj->cachehead) {
        cache = pobj->cachehead;
        pobj->cachehead = cache->next;
        if (!pobj->cachehead)
            pobj->cachetail = NULL;
        cachedata = (char *)(cache+1);
        pthread_spin_unlock(&pobj->lock);
        ism_common_allocBufferCopyLen(&action->http->outbuf, cachedata, cache->content_length);
        if (cache->topic_length)
            ism_http_setHeader(action->http, "Topic", cachedata+cache->content_length);
        ism_common_free(ism_memory_protocol_misc,cache);
        return 0;
    }

    /*
     * If a timeout is requested,
     */
    if (action->value == 0) {
        return ISMRC_NoMsgAvail;
    }

#if 1
    /* TODO: Timout is not yet implemented */
    return ISMRC_NoMsgAvail;
#else
    pobj->consumer.action = SUBPROT_Get;
    pthread_spin_unlock(&pobj->lock);
    return ISMRC_AsyncCompletion;
#endif
}

/*
 * We always use two message areas, one for properties and the other for payload
 */
static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};

/*
 * Publish a message
 */
int ism_rmsg_publish(rmsg_action_t * action, ism_http_t * http, int retain) {
    int  rc;
    ism_transport_t * transport = action->transport;
    rmsg_pobj_t * pobj = transport->pobj;
    const char * topic = http->user_path+1;
    uint8_t mtype = MTYPE_MQTT_Binary;
    ismMessageHeader_t hdr = {0};
    ismEngine_MessageHandle_t msgh;
    size_t areasize[2];
    void * areaptr[2];
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int  persist;

    rc = checkTopic(topic, 1, 1);
    if (rc)
        return rc;

    persist = retain ? 1 : getBooleanQueryProperty(http->query, "persist", 0);

    /* Put the topic into the properties */
    ism_protocol_putNameIndex(&buf, ID_Topic);
    ism_protocol_putStringValue(&buf, topic);
    if (retain) {
        ism_protocol_putNameIndex(&buf, ID_ServerTime);
        ism_protocol_putLongValue(&buf, ism_common_currentTimeNanos());
        ism_protocol_putNameIndex(&buf, ID_OriginServer);
        ism_protocol_putStringValue(&buf, ism_common_getServerUID());
    }

    /*
     * Set up the header
     */
    hdr.Persistence = (persist) ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    hdr.Reliability = (persist) ? ismMESSAGE_RELIABILITY_AT_LEAST_ONCE : ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
    hdr.Priority = 4;
    hdr.Flags = retain ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : 0;
    if (http->content_count) {
        int clen = http->content[0].content_type ? strlen(http->content[0].content_type) : 0;
        if (clen > 4 && !memcmp(http->content[0].content_type, "text", 4))
            mtype = MTYPE_MQTT_Text;
        if (clen >= 16 && !memcmp(http->content[0].content_type, "application/json", 16))
            mtype = MTYPE_MQTT_Text;
    }
    hdr.MessageType = mtype;

    /* Set the properties and body area */
    areasize[0] = buf.used;
    areaptr[0] = buf.buf;
    areasize[1] = http->content_count ? http->content[0].content_len : 0;
    areaptr[1] =  http->content_count ? http->content[0].content: NULL;

    /*
     * Create the message
     */
    if (pobj->session_handle) {
        rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    } else {
        transport->listener->stats->count[transport->tid].lost_msg++;
        ism_common_setError(ISMRC_Closed);
        TRACEL(5, transport->trclevel, "The session is closed on a publish: connect=%u client=%s\n", transport->index, transport->name);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
        return ISMRC_Closed;;
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    if (persist) {
        rc = ism_engine_putMessageOnDestination(transport->pobj->session_handle,
            ismDESTINATION_TOPIC, topic, NULL, msgh,
            action, sizeof *action, ism_rmsg_action);
    } else {
        rc = ism_engine_putMessageOnDestination(transport->pobj->session_handle,
                ismDESTINATION_TOPIC, topic, NULL, msgh, NULL, 0, NULL);
    }
    return rc;
}


/*
 * Delete a retained message
 */
int ism_rmsg_deleteRetain(rmsg_action_t * action, ism_http_t * http) {
    int  rc;
    ism_transport_t * transport = action->transport;

    rc = checkTopic(http->user_path+1, 1, 1);
    if (rc)
        return rc;
    rc = ism_engine_unsetRetainedMessageOnDestination(transport->pobj->session_handle,
            ismDESTINATION_TOPIC, http->user_path+1, ismENGINE_UNSET_RETAINED_OPTION_NONE,
            ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME, NULL,
            action, sizeof *action, ism_rmsg_action);
    return rc;
}

/*
 * Find a
 */
rmsg_sub_t * findSubscription(ism_transport_t * transport, const char * topic) {
    rmsg_pobj_t * pobj = transport->pobj;
    rmsg_sub_t * sub = pobj->subs;
    while (sub) {
        if (!strcmp(topic, sub->topic))
            return sub;
        sub = sub->next;
    }
    return NULL;
}


/*
 * Reply on the creation of a consumer
 */
static void replyConsumer(int rc, void * handle, void * xact) {
    rmsg_action_t * action = (rmsg_action_t *)xact;
    ism_http_t * http = action->http;
    ism_transport_t * transport = http->transport;
    rmsg_pobj_t * pobj = transport->pobj;
    const char * topic = http->user_path+1;
    rmsg_sub_t * sub;
    int  len = offsetof(rmsg_sub_t, topic) + strlen(topic) + 1;

    if (!rc) {
        pthread_spin_lock(&pobj->sessionlock);
        sub = (rmsg_sub_t *)ism_transport_allocBytes(transport, len, 1);
        sub->handle = handle;
        sub->next = pobj->subs;
        sub->durable = action->value;
        strcpy(sub->topic, topic);
        pthread_spin_unlock(&pobj->sessionlock);
    }
    action->action = Action_reply;
    ism_rmsg_action(rc, NULL, action);
}

/*
 * Subscribe
 */
int ism_rmsg_subscribe(rmsg_action_t * action, ism_http_t * http) {
    int  rc;
    ism_transport_t * transport = action->transport;
    rmsg_pobj_t * pobj = transport->pobj;
    int durable;
    const char * topic = http->user_path+1;
    rmsg_sub_t * sub;
    ismEngine_ConsumerHandle_t consumerh;

    durable = getBooleanQueryProperty(http->query, "durable", false);
    rc = checkTopic(topic, 0, durable);
    if (rc)
        return rc;

    pthread_spin_lock(&pobj->sessionlock);
    sub = findSubscription(transport, topic);
    pthread_spin_unlock(&pobj->sessionlock);
    if (!sub) {
        action->value = durable;
        if (durable) {
            return ISMRC_NotAuthorized;
            /* TODO */
        } else {
            ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };
            rc = ism_engine_createConsumer(transport->pobj->session_handle,
                 ismDESTINATION_TOPIC, topic,
                 &subAttrs, transport->pobj->client_handle,
                 &pobj->consumer, sizeof(pobj->consumer), ism_rmsg_replyMessage,
                 NULL, ismENGINE_CONSUMER_OPTION_ACK, &consumerh,
                 action, sizeof(*action), replyConsumer);
            if (rc != ISMRC_AsyncCompletion)
                replyConsumer(rc, consumerh, action);
        }
    }
    return 0;
}

/*
 * Reply from a destroy consumer
 */
static void replyUnSubscribe(int rc, void * handle, void * xact) {
    rmsg_action_t * action = (rmsg_action_t *)xact;
    ism_transport_t * transport = action->transport;
    ism_http_t * http = action->http;
    rmsg_pobj_t * pobj = transport->pobj;
    const char * topic = http->user_path+1;
    if (rc == 0) {
        xUNUSED int zrc = ism_engine_destroySubscription(pobj->client_handle,
            topic, pobj->client_handle, NULL, 0, NULL);
    }
    action->action = Action_reply;
    ism_rmsg_action(rc, NULL, action);
}


/*
 * Unsubscribe
 */
int ism_rmsg_unsubscribe(rmsg_action_t * action, ism_http_t * http) {
    int  rc;
    ism_transport_t * transport = action->transport;
    rmsg_pobj_t * pobj = transport->pobj;
    const char * topic = http->user_path+1;
    rmsg_sub_t * sub;
    rmsg_sub_t * prev;
    ismEngine_ConsumerHandle_t consumerh = NULL;

    rc = checkTopic(http->user_path+1, 0, 1);
    if (rc)
        return rc;

    /*
     * If a consumer exists unlink it
     */
    pthread_spin_lock(&pobj->sessionlock);
    sub = findSubscription(transport, topic);
    if (sub) {
        if (pobj->subs == sub) {
            pobj->subs = sub->next;
        } else {
            prev = pobj->subs;
            while (prev->next) {
               if (prev->next == sub) {
                   prev->next = prev->next->next;
                   break;
               }
            }
        }
        pthread_spin_unlock(&pobj->sessionlock);
        rc = ism_engine_destroyConsumer(consumerh,
                action, sizeof(action), replyUnSubscribe);
    } else {
        pthread_spin_unlock(&pobj->sessionlock);
    }
    if (rc != ISMRC_AsyncCompletion)
        replyUnSubscribe(rc, consumerh, action);

    return 0;
}

/*
 * Restartable action processing
 */
void ism_rmsg_action(int rc, void * handle, void * xact) {
    rmsg_action_t * action = (rmsg_action_t *)xact;
    ism_http_t * http = action->http;
    ism_transport_t * transport = http->transport;
    int  httprc = 0;
    ismEngine_ClientStateHandle_t client_handle;
    ismEngine_SessionHandle_t session_handle;
    uint32_t options = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;

    switch (action->action) {
    case Action_createConnection:
        action->action = Action_createSession;
        if (!transport->pobj->client_handle) {
            rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_RMSG, options, transport,
                    ism_rmsg_replySteal, transport->security_context, &client_handle,
                    action, sizeof(*action), ism_rmsg_action);
            if (rc == ISMRC_AsyncCompletion)
                break;
            handle = client_handle;
        }
        /* fall thru */

    case Action_createSession:
        if (rc) {
            ism_common_setError(ISMRC_CreateConnection);
            transport->close(transport, ISMRC_CreateConnection, 0, "Unable to create connection");
            TRACE(4, "Unable to create client state for RMSG \n");
            return;
        } else {
            transport->pobj->client_handle = handle;
            action->action = Action_createSession+1;
            if (!transport->pobj->session_handle) {
                rc = ism_engine_createSession(transport->pobj->client_handle, 0, &session_handle,
                        action, sizeof(*action), ism_rmsg_action);
                if (rc == ISMRC_AsyncCompletion)
                    break;
                handle = session_handle;
            }
        }
        /* fall thru */

    case Action_createSession+1:
        if (rc) {
            ism_common_setError(ISMRC_CreateSession);
            transport->close(transport, ISMRC_CreateSession, 0, "Unable to create connection");
            TRACE(4, "Unable to create session for RMSG \n");
            return;
        } else {
            action->action = Action_message;
            transport->pobj->session_handle = handle;
        }
        /* fall thru */

    case Action_message:
        action->action = Action_reply;
        switch (http->subprot) {
        case SUBPROT_Get:              rc = ism_rmsg_get(action, http, http->val1);     break;
        case SUBPROT_GetRetain:        rc = ism_rmsg_getRetain(action, http);           break;
        case SUBPROT_Publish:          rc = ism_rmsg_publish(action, http, 0);          break;
        case SUBPROT_PublishRetain:    rc = ism_rmsg_publish(action, http, 1);          break;
        case SUBPROT_DeleteRetain:     rc = ism_rmsg_deleteRetain(action, http);        break;
        case SUBPROT_Subscribe:        rc = ism_rmsg_subscribe(action, http);           break;
        case SUBPROT_Unsubscribe:      rc = ism_rmsg_unsubscribe(action, http);         break;
        }
        if (rc == ISMRC_AsyncCompletion)
            break;
        /* fall thru */

    case Action_reply:
        /*
         * Map return code
         */
        switch (rc) {
        case 0:
        case ISMRC_SomeDestinationsFull:
        case ISMRC_NoMatchingDestinations:
        case ISMRC_NoMatchingLocalDestinations:
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
            ism_rmsg_replyCloseClient(transport);
        }
        break;
    }
}


/*
 * Base62 is used for the random part of a generated clientID.
 */
static char base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

/*
 * Generate a client ID.
 */
static const char * generate_cid(ism_transport_t * transport, char * buf) {
    uint64_t rval;
    uint8_t * randbuf = (uint8_t *)&rval;
    char * bp = buf + 1;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randbuf, 8);
#else
    RAND_bytes(randbuf, 8);
#endif
    buf[0] = '_';
    if (transport->client_addr && *transport->client_addr)
        strcpy(bp, transport->client_addr);
    else
        strcpy(bp, "client");
    bp += strlen(bp);
    *bp++ = '_';
    for (int i=0; i<8; i++) {
        *bp++ = base62[(int)(rval%62)];
        rval /= 62;
    }
    *bp++ = 0;
    return buf;
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
        vp++;
    }
    if (vp > value && !*vp)
        return val != 0;
    if (strcmpi(value, "true"))
        return 1;
    if (strcmpi(value, "true"))
        return 0;
    return default_val;
}


/*
 * Receive data
 */
static int restMsgReceive(ism_transport_t * transport, char * data, int datalen, int kind) {
    ism_http_t * http;
    ism_field_t  map;
    ism_field_t  body;
    ism_field_t  query;
    ism_field_t  path;
    ism_field_t  typef;
    ism_field_t  localef;
    ism_field_t  clientf;
    const char * clientID = NULL;
    char  clientid_buf [1024];

    ism_field_t  f;
    char         http_op;
    char localebuf[16];
    concat_alloc_t  hdr = {data, datalen, datalen};
    const char * locale = NULL;
    char * pos = NULL;
    char * subtype;
    int    subprot = 0;
    int    val = 0;
    rmsg_pobj_t * pobj = transport->pobj;
    int inProgress;

    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }
    inProgress = __sync_fetch_and_add(&pobj->inprogress, 1);
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

    if (!transport->authenticated) {
        /* Get the clientID */
        clientID = getQueryProperty(query.val.s, "ClientID", clientid_buf, sizeof(clientid_buf));
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

        if (!clientID && !transport->authenticated) {
            ism_findPropertyName(&hdrs, "]ClientID", &clientf);
            if (clientf.type == VT_String)
                clientID = clientf.val.s;
        }
    }

    /*
     * Set the clientID on first header in the connection
     */
    if (!transport->authenticated) {
        if (!clientID || !*clientID) {
            clientID = generate_cid(transport, clientid_buf);
        }
        transport->name = ism_transport_putString(transport, clientID);
        transport->clientID = transport->name;
    }

    /*
     * Parse the subtype
     */
    pos = strchr(path.val.s+1, '/');
    if (pos) {
        int sublen;
        subtype = pos+1;
        pos = strchr(subtype, '/');
        if (!pos)
            pos = subtype + strlen(subtype);
        sublen = pos-subtype;

        switch (http_op) {

        case 'G':
            if (sublen==7 && !memcmp(subtype, "message", 7) && *pos) {
                subprot = SUBPROT_GetRetain;
            } else {
                if (sublen>0) {
                    val = 0;
                    while (*subtype >= '0' && *subtype <='9') {
                        val += val*10 + (*subtype-'0');
                        subtype++;
                    }
                    if (*subtype == '/' || *subtype == 0)
                        subprot = SUBPROT_Get;
                }
            }
            break;

        case 'P':
            if (sublen == 7 && !memcmp(subtype, "message", 7) && *pos) {
                subprot = SUBPROT_Publish;
            }
            break;

        case 'U':
            if (sublen==7 && !memcmp(subtype, "message", 7) && *pos) {
                subprot = SUBPROT_PublishRetain;
            } else if (sublen == 11 && !memcmp(subtype, "subscription", 11) && *pos) {
                subprot = SUBPROT_Subscribe;
            }
            break;

        case 'D':
            if (sublen==7 && !memcmp(subtype, "message", 7) && *pos) {
                subprot = SUBPROT_DeleteRetain;
            } else if (sublen==11 && !memcmp(subtype, "subscription", 11) && *pos) {
                subprot = SUBPROT_Unsubscribe;
            }
            break;
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
        http->subprot = subprot;


        if (subprot == 0) {
            httpReplyError(ISMRC_HTTP_NotFound, 404, http);
            return ISMRC_HTTP_NotFound;
        }

        http->user_path = http->path + (pos-path.val.s);
        http->val1 = val;

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
            httpRestMsgCall(transport, http, 0);
        }
    }
    return 0;
}

/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_rmsg_replyDoneConnection(int32_t rc, void * handle, void * vaction) {
    rmsg_action_t * action = (rmsg_action_t *) vaction;
    ism_transport_t * transport = action->transport;
    rmsg_pobj_t * pobj = transport->pobj;

    if (pobj->errors) {
        ism_common_destroyHashMap(pobj->errors);
        pobj->errors = NULL;
    }
    pthread_spin_destroy(&pobj->lock);
    pthread_spin_destroy(&pobj->sessionlock);

    /* Tell the transport we are done */
    transport->closed(transport);
}

/*
 * Session closed.  If last one, close the connection
 */
void ism_rmsg_replyCloseClient(ism_transport_t * transport) {
    rmsg_pobj_t * pobj = (rmsg_pobj_t *) transport->pobj;
    void * handle;
    rmsg_action_t action = { 0 };
    action.transport = transport;

    if (!__sync_bool_compare_and_swap(&pobj->closed, 1, 2)) {
        TRACEL(4, transport->trclevel, "ism_rmsg_replyCloseClient called more than once for: connect=%u\n", transport->index);
        return;
    }

    ism_rmsg_removeFromClientList(pobj, 1);

    ism_security_returnAuthHandle(transport->security_context);

    pthread_spin_lock(&pobj->sessionlock);
    pobj->session_handle = 0;

    /* Ensure that we don't call ism_engine_destroyClientState twice. */
    handle = pobj->client_handle;
    pobj->client_handle = 0;

    pthread_spin_unlock(&pobj->sessionlock);

    if (handle) {
        int rc = ism_engine_destroyClientState(handle,
                ismENGINE_DESTROY_CLIENT_OPTION_NONE, &action, sizeof(action),
                ism_rmsg_replyDoneConnection);
        if (rc == ISMRC_AsyncCompletion)
            return;
    }
    ism_rmsg_replyDoneConnection(0, NULL, &action);
}

/*
 * Receive a connection closing notification
 */
static int restMsgClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    rmsg_pobj_t * pobj = (rmsg_pobj_t *) transport->pobj;
    int32_t count;

    TRACEL(8, transport->trclevel, "ism_rmsg_closing: connect=%u client=%s rc=%d clean=%d reason=%s\n",
            transport->index, transport->name, rc, clean, reason);

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
        TRACEL(8, transport->trclevel, "ism_rmsg_closing postponed as there are %d actions/messages in progress: connect=%u client=%s",
                count+1, transport->index, transport->name);
        return 0;
    }

    /* Stop message delivery, destroy session and client state */
    ism_rmsg_replyCloseClient(transport);

    return 0;
}

/*
 * Convert the JMS map or stream message to a JSON payload
 */
void ism_rmsg_putJsonPayload(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray) {
    ism_json_putBytes(buf, ", \"payload\": ");
    ism_rmsg_putJsonPayloadContent(transport, buf, body, bodylen, inarray);
    bputchar(buf, '}');       /* End of message */
    bputchar(buf, 0);         /* Null terminate for debug */
    buf->used--;
}


/*
 * Put out the contents of a map or stream message as the payload contents
 */
void ism_rmsg_putJsonPayloadContent(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray) {
    ism_field_t field;
    int first = 1;
    concat_alloc_t in;
    memset(&in, 0, sizeof(in));
    in.buf = body;
    in.used = bodylen;
    if (in.used < 1 || (uint8_t) * in.buf != (inarray ? 0x9e : 0x9f)) {
        TRACEL(3, transport->trclevel, "Invalid JMS content when converting to RMSG.  The payload is ignored.  clientid=%s index=%d\n",
                transport->name, transport->index);
        return;
    }
    in.pos = 1;
    bputchar(buf, inarray ? '[' : '{');
    while (ism_protocol_getObjectValue(&in, &field) == 0) {
        if (field.type == VT_Name) {
            if (!first)
                bputchar(buf, ',');
            else
                first = 0;
            bputchar(buf, '"');
            ism_json_putEscapeBytes(buf, field.val.s,
                    (int) strlen(field.val.s));
            bputchar(buf, '"');
            bputchar(buf, ':');
        } else {
            if (inarray) {
                if (!first)
                    bputchar(buf, ',');
                else
                    first = 0;
            }
            ism_json_put(buf, NULL, &field, 0);
        }
    }
    bputchar(buf, inarray ? ']' : '}');
    bputchar(buf, 0); /* Null terminate for debug */
    buf->used--;
}

/*
 * Process stolen clientID.
 */
void ism_rmsg_replySteal(int32_t reason, ismEngine_ClientStateHandle_t handle, uint32_t options, void * vaction) {
    ism_transport_t * transport = (ism_transport_t *) vaction;
    rmsg_pobj_t * pobj = (rmsg_pobj_t *) transport->pobj;

    /* TODO: If reason is not ISMRC_ClientIDReused, need to produce a different message */

    /*
     * If connect is not finished yet, indicate that the client state
     * was stolen and needs to be destroyed. Otherwise, destroy the client state.
     */
    if (__sync_bool_compare_and_swap(&pobj->startState, RMSG_IN_PROGRESS, RMSG_STOLEN)) {
        /* Connect in progress, do cleanup when complete */
        return;
    }

    transport->close(transport, ISMRC_ClientIDReused, 0, "The connection is closed because the ClientID is used in another connection");
}
/*
 * Add a connection to the list of clients
 */
void ism_rmsg_addToClientList(rmsg_pobj_t * pobj) {
    TRACE(8, "ism_rmsg_addToClientList: pobj=%p\n", pobj);
    pthread_mutex_lock(&clientListLock);
    if (pobj->keepAlive == -1) {
        pobj->next = NULL;
        pobj->prev = clientListTail;
        if (clientListTail) {
            clientListTail->next = pobj;
        } else {
            clientListHead = pobj;
        }
        clientListTail = pobj;
    }
    if (pobj->keepAlive > -2)
        pobj->keepAlive = 0;

    pthread_mutex_unlock(&clientListLock);
}

/*
 * Remove a connection from the list of clients
 */
void ism_rmsg_removeFromClientList(rmsg_pobj_t * pobj, int lock) {
    TRACE(7, "ism_rmsg_removeFromClientList: pobj=%p lock=%d\n", pobj, lock);
    if (lock)
        pthread_mutex_lock(&clientListLock);
    if (pobj->keepAlive > -1) {
        if (pobj->prev) {
            pobj->prev->next = pobj->next;
        } else {
            clientListHead = pobj->next;
        }
        if (pobj->next) {
            pobj->next->prev = pobj->prev;
        } else {
            clientListTail = pobj->prev;
        }
        pobj->keepAlive = -1;
        pobj->next = pobj->prev = NULL;
    }
    pobj->keepAlive = -2;
    if (lock)
        pthread_mutex_unlock(&clientListLock);
}

/*
 * Initialize rest msg connection
 */
static int restMsgConnection(ism_transport_t * transport) {
    // printf("protocol=%s\n", transport->protocol);
    if (*transport->protocol == '/') {
        if (!strcmp(transport->protocol, "/rmsg")) {
            static const char * myheader = "ClientID";
            rmsg_pobj_t * pobj = (rmsg_pobj_t *) ism_transport_allocBytes(transport, sizeof(rmsg_pobj_t), 1);
            memset((char *) pobj, 0, sizeof(rmsg_pobj_t));
            pthread_spin_init(&pobj->lock, 0);
            pthread_spin_init(&pobj->sessionlock, 0);
            transport->pobj = pobj;
            transport->receive = restMsgReceive;
            transport->closing = restMsgClosing;
            transport->protocol = "/rmsg";
            transport->protocol_family = "rmsg";
            transport->www_auth = transport->listener->usePasswordAuth;
            transport->ready = 1;
            transport->pobj->consumer.transport = transport;
            transport->pobj->consumer.action    = SUBPROT_Get;
            ism_transport_setHeaderList(transport, 1, &myheader);
            return 0;
        }
    }
    return 1;
}

/*
 * Initialize restmsg
 */
int ism_protocol_initRestMsg(void) {
    ism_transport_registerProtocol(NULL, restMsgConnection);

    int capability =  ISM_PROTO_CAPABILITY_USETOPIC;
    ism_admin_updateProtocolCapabilities("rmsg", capability);
    return 0;
}
