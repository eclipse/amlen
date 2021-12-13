/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
 * Monitoring related code for the proxy
 */
#ifndef TRACE_COMP
#define TRACE_COMP Tcp
#endif
#include <ismutil.h>
#include <ismjson.h>
#include <pxtransport.h>
#include <pxtcp.h>
#include <protoex.h>
#include <tenant.h>
#include <pxmqtt.h>
#include <selector.h>
#include <imacontent.h>

#define BILLION        1000000000L
xUNUSED static const char * GW_NOTIFY_TOPIC_TEMPLATE = "iot-2/${Org}/type/${Type}/id/${Id}/notify";

typedef struct ism_protobj_t {
	ism_transport_t * 		 transport;
	ism_server_t *    		 server;
	pthread_spinlock_t		 lock;
	volatile ism_serverConnection_State	 state;
} ism_conMonitor_t;

int      g_metering_interval  = 0;
uint64_t g_metering_delta     = 0;
extern int g_useMQTTpx;
int      g_monitor_started    = 0;
ism_timer_t  g_metering_timer = NULL;
int      g_deviceupdatestatus_enabled  = 1;
extern int  g_useKafka;           /* Use kafka metering */
extern int  g_useKafkaTLS;        /* Use TLS for kafka metering **/
extern int  g_useMHUBKafkaConnection;           /* Use kafka metering */



void ism_proxy_makeMonitorTopic(concat_alloc_t * buf, ism_transport_t * transport, const char * monitor_topic);
extern int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command);
extern int ism_transport_startMonitorIOP(void);
extern int  ima_monitor_createConnection(ism_transport_t * transport, ism_server_t *server);
extern void ism_tcp_init_transport(ism_transport_t * transport);
int ism_transport_checkMeteringTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
uint8_t ism_mqtt_getMqttVersion(ism_transport_t * transport);
uint8_t ism_mqtt_getMqttFrame(ism_transport_t * transport);
extern int ism_proxy_deviceStatusUpdate(ism_transport_t * transport, int event,  int ec, const char * reason) ;
int ism_monitor_enableDeviceStatusUpdate(int enable);
int ism_kafka_sendMetering(ism_transport_t * transport, concat_alloc_t * buf);
int ism_kafka_init(int useKafka, int useTLS);
int ism_transport_stopped(void);
const char * ism_mqtt_externalProtocol(ism_transport_t * transport, char * buf);
extern int ism_route_routing_start(void);
extern int ism_mhub_sendMetering(ism_transport_t * transport, concat_alloc_t * buf) ;
/*
 * MQTT commands
 */
#define PUB_QOS_0       0x30     /* PUBLISH */
#define PUB_QOS0_Retain 0x31     /* PUBLISH + retain */
#define PUB_QOS_1       0x32     /* PUBLISH + qos1 */
#define PUBLISH_X       0x3F


#ifndef NO_PROXY
/*
 * Send a connect or disconnect notification to the server.
 */
static int sendNotificationToServer(ism_transport_t * transport, int event, int ec, const char * reason) {
	int rc = 0;
	int loc;
	int topicloc;
    ism_server_t * server = transport->server;
    if (server && server->monitor_topic) {
        int op = PUB_QOS_0;
        char xbuf [4096];
        concat_alloc_t buf = {xbuf, sizeof xbuf};
        buf.used = 16;
        buf.compact = 3;
        const char * montopic = server->monitor_topic;
        if (transport->alt_monitor && server->monitor_topic_alt)
            montopic = server->monitor_topic_alt;
        if (server->mqttProxyProtocol > 2) {
            int  extlen;
            buf.used += 3;
            loc = buf.used;
            ism_common_putExtensionValue(&buf, EXIV_ExpireTTL, (60*60*24*8));   /* 8 days */
            extlen = buf.used-loc;
            buf.buf[17] = (char)(extlen>>8);
            buf.buf[18] = (char)extlen;
        }
        topicloc = buf.used;
        ism_proxy_makeMonitorTopic(&buf, transport, montopic);
        loc = buf.used;
        ism_proxy_jsonMessage(&buf, transport, event, ec, reason);
        TRACE(8, "NotificationMsg: %s\n", buf.buf + loc);
    	pthread_spin_lock(&server->lock);
    	if (server->monitor) {
    		ism_conMonitor_t * pobj = (ism_conMonitor_t *) server->monitor->pobj;
    	  	if (pobj->state == PROTOCOL_CONNECTED) {
    		    if (server->monitor_retain&0x0F) {
    		        switch (event) {
    		        case JM_Connect:
    		            if (transport->protocol[0] != '/')
    		                op = PUB_QOS0_Retain;
    		            break;
    		        case JM_Failed:
    		        case JM_Active:
    		            break;
    		        case JM_Disconnect:
    		            if (server->monitor_retain == 1 && ec != ISMRC_ClientIDReused) {
    		                /* Unset the retained message */
    		                TRACE(8, "Unset retained notification: client=%s\n", transport->name);
  		                    rc = server->monitor->send(server->monitor, buf.buf + topicloc, loc - topicloc,
    		                            PUB_QOS0_Retain, SFLAG_FRAMESPACE);
    		            } else {
    		                if (transport->protocol[0] != '/')
    		                    op = PUB_QOS0_Retain;
    		            }
    		            break;
    		        }
    		    }
                if (server->mqttProxyProtocol > 2) {
    		        buf.buf[16] = (op&0xf) + 0x20;
    		        op = PUBLISH_X;
                }
    			rc = server->monitor->send(server->monitor, buf.buf+16, buf.used-16, op, SFLAG_FRAMESPACE);
    		}
    	}
    	pthread_spin_unlock(&server->lock);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);

    }
	return rc;
}

/*
 * Return a string given an action enumeration
 */
const char * actionName(int action) {
    /* Get the name of the action */
    switch (action) {
    case JM_Connect:     return "Connect";
    case JM_Disconnect:  return "Disconnect";
    case JM_Failed:      return "FailedConnect";
    case JM_Active:      return "Active";
    case JM_Info:        return "Info";
    default:             return "Unknown";
    }
}

static int monitorDumpPobj(ism_transport_t * transport, char * buf, int len) {
    if (len > 0)
        buf[0] = 0;
    return 0;
}

/*
 * Put the monitor topic along with the preceding length bytes into a buffer.
 *
 * This is designed to be used to create the topic into the buffer just before
 * putting in the JSON payload.
 */
void ism_proxy_makeMonitorTopic(concat_alloc_t * buf, ism_transport_t * transport, const char * monitor_topic) {
    const char * mt = monitor_topic;
    const char * endp;
    int  lenloc = ism_protocol_reserveBuffer(buf, 2);

    while (*mt) {
        endp = strchr(mt, '$');
        if (!endp) {
            ism_json_putBytes(buf, mt);
            break;
        } else {
            int len = endp-mt;
            ism_common_allocBufferCopyLen(buf, mt, len);
            mt = endp;
            if (mt[1] == '{') {
                mt += 2;    /* Past ${ */
                endp = strchr(mt, '}');
                if (!endp) {
                    len = 0;
                } else {
                    len = endp-mt;
                }
                if (len==1 && *mt=='$') {
                    ism_json_putBytes(buf, "$");
                    mt = endp + 1;
                } else if ((len==4 && !memcmp(mt, "Type", 4))
                        || (len==4 && !memcmp(mt, "Inst", 4))) {        /* Type */
					if (transport->typeID)
						ism_json_putBytes(buf, transport->typeID);

                    mt = endp + 1;
                } else if ((len==6 && !memcmp(mt, "Device", 6))
                        || (len==2 && !memcmp(mt, "Id", 2)))  {         /* Device portion of ClientID */

                	if (transport->deviceID)
						ism_json_putBytes(buf, transport->deviceID);

                    mt = endp + 1;
                } else if ((len==6 && !memcmp(mt, "Tenant", 6))
                        || (len==3 && !memcmp(mt, "Org", 3))) {         /* Tenant portion of ClientID */
                    if (transport->tenant)
                        ism_json_putBytes(buf, transport->tenant->name);
                    mt = endp + 1;
                } else if (len==8 && !memcmp(mt, "ClientID", 8)) {       /* Whole ClientID */
                    ism_json_putBytes(buf, transport->name);
                    mt = endp + 1;
                } else {      /* Put out the bytes */
                    ism_json_putBytes(buf, "${");
                    ism_common_allocBufferCopyLen(buf, mt, len);
                    mt += len;
                }
            } else {
                ism_common_allocBufferCopyLen(buf, mt, 1);            /* Put the '$' */
                mt++;
            }
        }
    }
    buf->buf[lenloc] = (char)((buf->used-lenloc-2)>>8);
    buf->buf[lenloc+1] = (char)(buf->used-lenloc-2);
}


/*
 * Create an metering connection message
 */
int ism_proxy_meteringMessage(concat_alloc_t * buf, ism_transport_t * transport, char * timest) {
    char xbuf[256];
    char tbuf[32];
    uint64_t  readb;
    uint64_t  writeb;
    /* Format current and connect times */
    if (timest && *timest) {
        timest[19] = 'Z';
        timest[20] = 0;
    } else {
        ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);
        if (!timest)
            timest = tbuf;
        ism_common_setTimestamp(ts, ism_common_currentTimeNanos());
        ism_common_formatTimestamp(ts, timest, sizeof tbuf, 6, ISM_TFF_ISO8601);
        ism_common_closeTimestamp(ts);
    }
    sprintf(xbuf, "{\"T\":\"%s\",\"C\":", timest);
    ism_json_putBytes(buf, xbuf);
    ism_json_putString(buf, transport->name ? transport->name : "");
    readb = transport->read_bytes;
    writeb = transport->write_bytes;
    sprintf(xbuf, ",\"RB\":%lu,\"WB\":%lu}", readb-transport->read_bytes_prev, writeb-transport->write_bytes_prev);
    ism_common_allocBufferCopyLen(buf, xbuf, strlen(xbuf)+1);
    buf->used--;
    transport->read_bytes_prev = readb;
    transport->write_bytes_prev = writeb;
    return 0;
}


/*
 * Create a JSON message for connect/disconnect notification
 */
int ism_proxy_jsonMessage(concat_alloc_t * buf, ism_transport_t * transport, int which, int rc, const char * reason) {
    char tbuf[64];
    char tbuf2[64];
    char proto[16];
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 0, JSON_OUT_COMPACT);

    /* Format current and connect times */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
    ism_common_setTimestamp(ts, transport->connect_time);
    ism_common_formatTimestamp(ts, tbuf2, sizeof tbuf, 7, ISM_TFF_ISO8601);
    ism_common_closeTimestamp(ts);

    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "Action", actionName(which));
    ism_json_putStringItem(jobj, "Time", tbuf);
    ism_json_putStringItem(jobj, "ClientAddr", transport->client_addr);
    if (transport->clientID && *transport->clientID) {
        ism_json_putStringItem(jobj, "ClientID", transport->clientID);
    }
    ism_json_putIntegerItem(jobj, "Port", transport->serverport);
	ism_json_putBooleanItem(jobj, "Secure", transport->secure);
    if (*transport->protocol_family)
        ism_json_putStringItem(jobj, "Protocol", ism_mqtt_externalProtocol(transport, proto));
    if (transport->userid && *transport->userid && strcmp(transport->userid, "use-token-auth"))
        ism_json_putStringItem(jobj, "User", transport->userid);
    if (transport->cert_name && *transport->cert_name)
        ism_json_putStringItem(jobj, "CertName", transport->cert_name);
    ism_json_putStringItem(jobj, "ConnectTime", tbuf2);

    /* Put out closing code */
    if (rc) {
        ism_json_putIntegerItem(jobj, "CloseCode", rc);
        if (reason)
            ism_json_putStringItem(jobj, "Reason", reason);
    }

    /* Put out stats */
    if (which == JM_Disconnect) {
        ism_json_putLongItem(jobj, "ReadBytes",transport->read_bytes);
        ism_json_putLongItem(jobj, "ReadMsg", transport->read_msg);
        ism_json_putLongItem(jobj, "WriteBytes", transport->write_bytes);
        ism_json_putLongItem(jobj, "WriteMsg", transport->write_msg);
    }
    ism_json_endObject(jobj);
    return 0;
}


/*
 * Create a JSON message for Gateway notification
 */
int ism_proxy_jsonGWNotification(concat_alloc_t * buf, ism_transport_t * transport, const char * request,
        const char * topic, const char * typeID, const char * devID, int rc, const char * reason) {
    char tbuf[64];
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, 0, JSON_OUT_COMPACT);

    /* Format current and connect times */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
    ism_common_closeTimestamp(ts);

    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "Request", request);
    ism_json_putStringItem(jobj, "Time", tbuf);
    ism_json_putStringItem(jobj, "Topic", topic);
    if (typeID)
        ism_json_putStringItem(jobj, "Type", typeID);
    if (devID)
        ism_json_putStringItem(jobj, "Id", devID);
    ism_json_putStringItem(jobj, "Client", transport->clientID);
    /* Put out closing code */
    if (rc) {
        ism_json_putIntegerItem(jobj, "RC", rc);
        if (reason)
            ism_json_putStringItem(jobj, "Message", reason);
    }
    ism_json_endObject(jobj);
    return 0;
}


/*
 * Send a metering message
 */
int ism_proxy_meteringMsg(ism_transport_t * transport, char * timest) {
    int   rc = 0;
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    /* Ignore internal and admin connections */
    if (*transport->name == 0 || *transport->protocol_family == 'a')
        return 0;

    if (g_useKafka) {
    		if(g_useMHUBKafkaConnection){
    			//Use MHUB Kafka Connection Configuration
    			rc = ism_mhub_sendMetering(transport, &buf);
    		}else{
    			//Use Legacy Kafka
    			rc = ism_kafka_sendMetering(transport, &buf);
    		}
    }
    if (g_useKafka != 1) {
        ism_server_t * server = transport->server;
        buf.used = 16;
        if (server && server->metering_topic) {
            ism_proxy_makeMonitorTopic(&buf, transport, server->metering_topic);
            int loc = buf.used;
            pthread_spin_lock(&transport->server->lock);
            ism_proxy_meteringMessage(&buf, transport, timest);
            pthread_spin_unlock(&transport->server->lock);
            TRACE(8, "MeteringMsg: %s\n", buf.buf + loc);
            pthread_spin_lock(&server->lock);
            if (server->monitor) {
                rc = server->monitor->send(server->monitor, buf.buf+16, buf.used-16, PUB_QOS_0, SFLAG_FRAMESPACE);
            } else {
                rc = 1;
            }
            pthread_spin_unlock(&server->lock);

        }
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    return rc;
}

/*
 * Send a metering message
 */
int  ism_proxy_GWNotify(ism_transport_t * transport, const char * request, const char * topic,
        const char * typeID, const char * devID, int rc, const char * reason) {
    ism_server_t * server = transport->server;
    if (server) {
        char xbuf [8192];

        concat_alloc_t buf = {xbuf, sizeof xbuf};
        buf.used = 16;
        buf.compact = 3;
        ism_proxy_makeMonitorTopic(&buf, transport, GW_NOTIFY_TOPIC_TEMPLATE);
        int loc = buf.used;
        ism_proxy_jsonGWNotification(&buf, transport, request, topic, typeID, devID, rc, reason);
        TRACE(8, "GWNotification: %s\n", buf.buf + loc);
        pthread_spin_lock(&server->lock);
        if (server->monitor) {
            server->monitor->send(server->monitor, buf.buf+16, buf.used-16, PUB_QOS_0, SFLAG_FRAMESPACE);
        }
        pthread_spin_unlock(&server->lock);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
    }
    return 0;
}

/*
 * Create a JSON connect message
 */
void ism_proxy_connectMsg(ism_transport_t * transport) {
	if (transport->server && (transport->server->monitor_connect&1)) {
	    sendNotificationToServer(transport, JM_Connect, 0, NULL);

		/* Submit to update Device status database */
	    if (g_deviceupdatestatus_enabled)
	    	ism_proxy_deviceStatusUpdate(transport,JM_Connect, 0, NULL);
	}
}

/*
 * Create a notification message for a failed connection.
 * This is not retained.
 */
void ism_proxy_failedMsg(ism_transport_t * transport, int rc, const char * reason) {
    if (transport->server && (transport->server->monitor_connect&2)) {
        sendNotificationToServer(transport, JM_Failed, rc, reason);
    }
}


/*
 * Create a JSON disconnect message
 */
void ism_proxy_disconnectMsg(ism_transport_t * transport, int rc, const char * reason) {
	if (transport->server && (transport->server->monitor_connect&2)) {
	    sendNotificationToServer(transport, JM_Disconnect, rc, reason);
		/* Submit to update Device status database */
	    if (g_deviceupdatestatus_enabled)
	    	ism_proxy_deviceStatusUpdate(transport,JM_Disconnect, rc, reason);
	}
}


extern ism_server_t * ismServers;
static int startServerMonitoring(const char *serverName);
xUNUSED static int stopServerMonitoring(ism_server_t* server);
static int imaServerConnected(ism_transport_t * transport, int rc);
static int imaServerDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason);


/*
 * Update an ACL, but only if the monitoring is connected
 */
int ism_proxy_updateACL(ism_transport_t * transport, void * xtenant, uint64_t whichval) {
    ism_conMonitor_t * pobj = (ism_conMonitor_t *)transport->pobj;
    ism_tenant_t * tenant = (ism_tenant_t *)xtenant;
    char aclname [4] = "_0";
    char xbuf[1024];
    if (pobj->state == PROTOCOL_CONNECTED) {
        int which = (whichval>>1)&0x7;
        int val = whichval&1;
        aclname[1] += which;
        concat_alloc_t buf = {xbuf, sizeof xbuf, 24};
        xbuf[16] = 0;
        xbuf[17] = 1;
        xbuf[18] = EXIV_EndExtension;
        xbuf[19] = '@';
        xbuf[20] = '_';
        xbuf[21] = which + '0';
        xbuf[22] = '\n';
        xbuf[23] = val ? '+' : '-';
        ism_json_putString(&buf, tenant->name);
        ism_common_allocBufferCopyLen(&buf, "\n", 2);
        buf.used--;
        TRACE(8, "Send monitor ACL: %s", buf.buf+19);
        transport->send(transport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
    }
    return 0;
}

/*
 * Send the global ACLs on monitor connect
 */
static void sendGlobalACL(ism_transport_t * transport) {
    ism_acl_t * acl;
    int  found = 0;
    char xbuf [2048];
    char aclname[4] = "_0";
    concat_alloc_t buf = {xbuf, sizeof xbuf, 19 };
    int i;

    xbuf[16] = 0;
    xbuf[17] = 1;
    xbuf[18] = EXIV_EndExtension;
    for (i=0; i<9; i++) {
        aclname[1] = i+'0';
        acl = ism_protocol_findACL(aclname, 0, NULL);
        if (acl) {
            found = 1;
            ism_protocol_getACL(&buf, acl);
            ism_protocol_unlockACL(acl);
        }
    }
    if (found) {
        ism_common_allocBufferCopyLen(&buf, "", 1);
        buf.used--;
        TRACE(8, "Send global ACL: \n%s", buf.buf+19);
        transport->send(transport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
}


/*
 * Receive a CONNACK from the server
 */
static int receiveConAck(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    const char * reason = NULL;
    const char * product = NULL;
    const char * version = NULL;
    const char * details = NULL;
    const char * servername = NULL;
    uint8_t command = (uint8_t)((kind >> 4) & 15);

    TRACE(7, "Receive CONNACK from monitoring server connect=%d command=%d\n", transport->index, command);
    transport->ready = 1;
    ism_server_setLastGoodAddress(transport->server, transport->connect_order);
    if (command == 2) {
    	ism_conMonitor_t * pobj = (ism_conMonitor_t *) transport->pobj;
    	uint8_t * bp = (uint8_t *) inbuf;
    	int rc = bp[1];
    	if (buflen>2) {
    	    int extlen = buflen-2;
    	    const char * ext = inbuf+2;
    	    int protolevel = ism_common_getExtensionValue(ext, extlen, EXIV_ProxyProtLevel, 2);
    	    transport->server->mqttProxyProtocol = protolevel;
            TRACE(8, "Monitoring protocol level=%u\n", protolevel);

            rc = ism_common_getExtensionValue(ext, extlen, EXIV_ServerRC, rc);
            reason = ism_common_getExtensionString(ext, extlen, EXIV_ReasonString, NULL);
            product = ism_common_getExtensionString(ext, extlen, EXIV_ServerProduct, NULL);
            version = ism_common_getExtensionString(ext, extlen, EXIV_ServerVersion, NULL);
            details = ism_common_getExtensionString(ext, extlen, EXIV_ServerDetails, NULL);
            servername  = ism_common_getExtensionString(ext, extlen, EXIV_ServerName, NULL);
            if (rc) {
                TRACE(4, "Monitoring rc=%u reason=%s\n", rc, reason);
            }
    	}
    	if (!rc) {
    		pobj->state = PROTOCOL_CONNECTED;
    		if (servername) {
    		    TRACE(4, "Monitoring server connected: server=%s connect=%d name=%s (%s %s %s)\n",
    		            transport->server->name, transport->index, servername, product, version, details);
    		} else {
    		    TRACE(4, "Monitoring server connected %s: connect=%d\n",
    		            transport->server->name, transport->index);
    	    }
    		sendGlobalACL(transport);
    	} else {
    		//TODO: Handle error
    	}
    	return 0;
    }
    //TODO: Handle error
    return 0;
}

const char * ism_mqtt_mqttCommand(int ixin);
void ism_mqtt_setDumpPobj(ism_transport_t * transport);
/*
 * Start the monitoring server connections
 */
static int startServerMonitoring(const char *serverName) {
	ism_server_t* server = ism_tenant_getServer(serverName) ;
	if (server && !ism_transport_stopped() &&
	    ((server->monitor_connect && server->monitor_topic) || server->metering_topic)) {
	    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
	    ism_tcp_init_transport(transport);
	    transport->originated = 1;
	    transport->protocol = "mqtt4-mon";  //TODO: Do we need to modify this to support MQTTv5
	    transport->protocol_family = "admin";
	    transport->tid = 0;
	    transport->connected = imaServerConnected;
	    ism_conMonitor_t * pobj = (ism_conMonitor_t *) ism_transport_allocBytes(transport, sizeof(ism_conMonitor_t), 1);
	    transport->pobj = pobj;
	    transport->receive = receiveConAck;
	    transport->actionname = ism_mqtt_mqttCommand;
	    transport->closing = imaServerDisconnected;
	    pobj->server = server;
	    pobj->transport = transport;
	    transport->clientID = ism_transport_putString(transport, serverName);
	    transport->name = transport->clientID;
	    transport->dumpPobj = monitorDumpPobj;
	    pthread_spin_lock(&server->lock);
	    pobj->state = TCP_CON_IN_PROCESS;
	    server->monitor = transport;
	    transport->server = server;
	    pthread_spin_unlock(&server->lock);

	    xUNUSED int rc = ima_monitor_createConnection(transport, server);
	    //TODO: Handle error
	}
	return 0;
}

int ima_monitor_getMonitorState(ism_server_t* server) {
    int state = 0;
    pthread_spin_lock(&server->lock);
    if(server->monitor) {
        state = server->monitor->pobj->state;
    }
    pthread_spin_unlock(&server->lock);
    return state;
}

/*
 * Stop server monitoring
 */
static int stopServerMonitoring(ism_server_t* server) {
	return 0;
}

/*
 * Connect message non-proxy protocol
 */
static char connectMsg[14] = {
		16, /* Connect Header */
		12, /* Variable Length */
		0, 4, 'M', 'Q', 'T', 'T', /* Protocol */
		4, /* Protocol version number (3.1.1) */
		2, /* Flags (clean_seesion = 1) */
		0, 0, /* no keep alive */
		0, 0  /* empty client id */
};

/*
 * Connect message for proxy protocol
 */
static char connectMsgPx[20] = "\x10\x12\0\6MQTTpx\x84\2\x10\0\0\0\0\0\1\x13";

/*
 * Start a monitoring timer
 */
static int startMonitoringTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
	char * serverName = userdata;
	startServerMonitoring(serverName);
	ism_common_cancelTimer(timer);
	return 0;
}

/*
 * TCP connection complete on outgoing monitoring connection.
 */
static int imaServerConnected(ism_transport_t * transport, int rc) {
	if (rc == 0) {
		ism_conMonitor_t * pobj = (ism_conMonitor_t *) transport->pobj;
		ism_server_t* server = pobj->server;
        transport->state = ISM_TRANST_Open;
		if (server) {
			pthread_spin_lock(&server->lock);
			pobj->state = PROTOCOL_CON_IN_PROCESS;
			transport->ready = 5;
			pthread_spin_unlock(&server->lock);
			/* If proxy protocol is specified, use it so that the null retained message is not sent */
			if (g_useMQTTpx) {
			    TRACE(7, "send MQTTpx monitor connect: index=%d\n", transport->index);
		        transport->send(transport, connectMsgPx, 20, 0, SFLAG_HASFRAME);
			} else {
                TRACE(7, "send MQTT monitor connect: index=%d\n", transport->index);
			    transport->send(transport, connectMsg, 14, 0, SFLAG_HASFRAME);
			}
		}
		return 0;
	}
	transport->close(transport, rc, 0, "Connection to server was not established");
	return 0;
}

/*
 *
 */
static int imaServerDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason) {
	ism_conMonitor_t * pobj = (ism_conMonitor_t *) transport->pobj;
    pthread_spin_lock(&pobj->server->lock);
    if (pobj->state == TCP_DISCONNECTED) {
    	pthread_spin_unlock(&pobj->server->lock);
    	return 0;
    }
	pobj->state = TCP_DISCONNECTED;
	pobj->server->monitor = NULL;
	transport->ready = 0;
    pthread_spin_unlock(&pobj->server->lock);
    uint64_t retry_time = clean ? 100000000L : 5000000000L;  /* 0.1 sec or 5 sec */
	ism_common_setTimerOnce(ISM_TIMER_HIGH, startMonitoringTimer, (char *)transport->server->name, retry_time);  /* 5 seconds */
	transport->closed(transport);
	return 0;
}

/*
 * Set monitoring interval
 */
int ism_monitor_setMeteringInterval(int interval) {
    g_metering_interval = interval;
    if (g_metering_interval < 0)
        g_metering_interval = 0;
    if (g_metering_interval > 604800)   /* One week */
        g_metering_interval = 604800;
    if (g_metering_timer) {
        ism_common_cancelTimer(g_metering_timer);
        g_metering_timer = NULL;
    }
    if (g_metering_interval) {
        int period = (g_metering_interval * 1000) / 12;
        TRACE(4, "Set metering interval = %d seconds\n", g_metering_interval);
        g_metering_delta = ((uint64_t)g_metering_interval) * BILLION;   /* minutes to nanoseconds */
        g_metering_timer = ism_common_setTimerRate(ISM_TIMER_LOW, ism_transport_checkMeteringTimer, NULL, period, period, TS_MILLISECONDS);
    } else {
        TRACE(4, "Set no metering interval\n");
    }
    return 0;
}
#endif

/*
 * Periodic trim of heap
 */
static int memoryCleanupTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    int rc = malloc_trim(0);
    TRACE(5, "Msproxy Malloc Trim. RC=%d\n", rc);
    return 1;
}


/*
 * Start monitoring
 */
int  ism_monitor_startMonitoring(void) {
    int interval;

	if (g_monitor_started)
		return 0;

#ifndef NO_PROXY
    int useKafkaMetering = 0;;
    int useKafkaMeteringTLS = 0;

	ism_transport_startMonitorIOP();

	/*
	 * Get the metering interval in minutes
	 */
    interval = ism_common_getIntConfig("MeteringInterval", 0);
    if (interval) {
        interval *= 60;
    } else {
        /* For testing allow metering in seconds */
        interval = ism_common_getIntConfig("MeteringIntervalSec", 0);
    }
    ism_monitor_setMeteringInterval(interval);

    /* Select kafka metering */
    useKafkaMetering = ism_common_getIntConfig("UseKafkaMetering", useKafkaMetering);
    useKafkaMeteringTLS = ism_common_getBooleanConfig("UseKafkaMeteringTLS", useKafkaMeteringTLS);
    ism_kafka_init(useKafkaMetering, useKafkaMeteringTLS);

    /* Determine if device status update is to be used */
    int deviceStatusUpdateEnabled = ism_common_getBooleanConfig("DeviceStatusUpdateEnabled", 1);
    ism_monitor_enableDeviceStatusUpdate(deviceStatusUpdateEnabled);

    ism_route_routing_start();
#endif
    /* Run malloc_trim() on a timer */
    interval = ism_common_getIntConfig("MemoryCleanupIntervalSec", 300);
    ism_common_setTimerRate(ISM_TIMER_LOW, memoryCleanupTimer, NULL, interval, interval, TS_SECONDS);

    g_monitor_started = 1;

	return 0;
}

#ifndef NO_PROXY
/*
 * Start monitoring for a server
 */
int  ism_monitor_startServerMonitoring(ism_server_t * server) {
	if (!g_monitor_started){
		ism_monitor_startMonitoring();
	}
	ism_common_setTimerOnce(ISM_TIMER_HIGH, startMonitoringTimer, (char *)server->name, 10000000);  /* 0.01 second */
	return 0;

}

/*
 * Set whether device status update is active
 */
int ism_monitor_enableDeviceStatusUpdate(int enable)
{
	TRACE(5, "Set DeviceUpdateStatus to %d\n", enable);
	g_deviceupdatestatus_enabled=enable;
	return 0;
}
#endif
