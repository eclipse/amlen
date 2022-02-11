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

#define TRACE_COMP Protocol
#include <protocol.h>
#include <ismmessage.h>
#include <engine.h>
#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif
xUNUSED static char * version_string = "version: ismprotocol " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

#define LTPA_NAME               "IMA_LTPA_AUTH"
#define LTPA_NAME_LEN   13
#define OAUTH_NAME              "IMA_OAUTH_ACCESS_TOKEN"
#define OAUTH_NAME_LEN  22

extern int ism_protocol_initJMS(void);
extern int ism_protocol_initWSBasic(void);
extern int ism_protocol_initMQTT(void);
extern int ism_protocol_initPlugin(void);
extern int ism_protocol_initRestMsg(void);
extern int ism_protocol_initMux(void);
extern int ism_protocol_startPlugin(void);
extern int ism_protocol_termJMS(void);
extern int ism_protocol_printJMSStats(void);
extern int ism_protocol_initForwarder(void);
extern int ism_protocol_startForwarder(void);
extern int ism_protocol_termForwarder(void);
extern int ism_protocol_initACL(void);
extern int ism_protocol_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        const void *               rule,
        size_t                     rulelen,
        ismMessageSelectionLockStrategy_t * lockStrategy);

static pthread_spinlock_t g_protocol_lock;

int ism_plugin_createPlugin(const char * pluginName, const char * zipFile, const char * propsFile, int overwrite);
int ism_plugin_removePlugin(const char * pluginName);
XAPI int ism_transport_setMaxSocketBufSize(ism_transport_t * transport, int maxSendSize, int maxRecvSize);

/**
 * Protocol Configuration Callback
 */
int ism_protocol_configCallback(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t type) {
    int rc = ISMRC_OK;
    TRACE(8, ">>> ism_protocol_configCallback: object: %s, name: %s, type: %d\n", object, name, type);

    switch (type) {
        case ISM_CONFIG_CHANGE_NAME:
            break;
        case ISM_CONFIG_CHANGE_PROPS:
            if (!strcmp(object, "Plugin")) {
                char propName[1024];
                const char * zipFile = NULL;
                const char * propsFile = NULL;
                int overwrite = 1;
                sprintf(propName,"Plugin.PropertiesFile.%s",name);
                propsFile =  ism_common_getStringProperty(props, propName);
                sprintf(propName,"Plugin.File.%s",name);
                zipFile = ism_common_getStringProperty(props, propName);
                sprintf(propName,"Plugin.Overwrite.%s",name);
                overwrite = ism_common_getBooleanProperty(props, propName, 1);
                rc = ism_plugin_createPlugin(name, zipFile, propsFile, overwrite);
            }
    		break;
    	case ISM_CONFIG_CHANGE_DELETE:
    	    if (!strcmp(object, "Plugin")) {
                rc = ism_plugin_removePlugin(name);
    	    }
     		break;
    }


    TRACE(8, "<<< ism_protocol_configCallback: rc = %d\n", rc);

    return rc;
}
/*
 * Initialize the protocol component
 * @return A return code, 0=good
 */
XAPI int ism_protocol_init(void) {

    pthread_spin_init(&g_protocol_lock, 0);

    if (getenv("CUNIT") == NULL) {
        /*Register Protocol component for configuration.*/
        ism_config_t * conhandle = NULL;
        ism_prop_t * props;
        ism_config_register(ISM_CONFIG_COMP_PROTOCOL, NULL, ism_protocol_configCallback, &conhandle);
        props = ism_config_getProperties(conhandle, NULL, NULL);
        if (NULL != props) ism_common_freeProperties(props);
    }

    ism_protocol_initACL();
    ism_protocol_initPlugin();      /* Register the plug-in protocol checker first */
    ism_protocol_initForwarder();
    ism_protocol_initJMS();
    ism_protocol_initWSBasic();
    ism_protocol_initMQTT();
    if (ism_common_getBooleanConfig("EnableRestMsg", 0))
        ism_protocol_initRestMsg();
    if (ism_common_getIntConfig("Protocol.AllowMqttProxyProtocol", 0) == 2)
        ism_protocol_initMux();
    return 0;
}

/*
 * Required transport object pointer at start of consumer context
 */
typedef struct {
    ism_transport_t * transport;
} consumer_context_t;

/*
 * Handle the delivery failure callback from the engine.
 *
 * NOTE: A requirement for all protocols is that the context passed to ism_engine_createConsumer()
 * must have a transport object pointer as the first item in the context.
 */
void ism_protocol_deliveryFailure(int rc, ismEngine_ClientStateHandle_t hClient,
        ismEngine_ConsumerHandle_t hConsumer, void * context) {
    ism_transport_t * transport = ((consumer_context_t *)context)->transport;
    TRACEL(2, transport->trclevel, "DeliveryFailure: connect=%d client=%s protocol=%s\n", transport->index, transport->name, transport->protocol);
    if (transport->deliveryfailure) {
        transport->deliveryfailure(transport, rc, context);
    } else {
        transport->close(transport, ISMRC_ServerCapacity, 0, "The server capacity is reached");
    }
}

static ismEngine_ClientStateHandle_t g_client_Shared = NULL;
static ismEngine_ClientStateHandle_t g_client_SharedND = NULL;
static ismEngine_ClientStateHandle_t g_client_SharedM = NULL;

static ism_endpoint_t dummyEndpoint = {
    .name        = "!Monitoring",
    .ipaddr      = "",
    .secprof     = "",           /* Security profile name                  */
    .msghub      = "!Monitoring", /* Message hub name                       */
    .conpolicies = "",           /* Connection policies                    */
    .topicpolicies = "",         /* Messaging policies                     */
    .qpolicies   = "",
    .subpolicies = "",
    .transport_type = "none",
    .protomask   = PMASK_Internal,
    .transmask   = TMASK_AnyTrans,
    .thread_count = 1,
};


/*
 * Set the max buffer size based on the size in the securityh context.
 * This must be done after connection policy is complete.
 */
void ism_protocol_setSocketBuffer(struct ism_transport_t * transport) {
    /* Set max IO buffer size */
    int msgRate = (int)ism_security_context_getExpectedMsgRate(transport->security_context);
    int sendbuf;
    int recvbuf;
    switch (msgRate) {
    case EXPECTEDMSGRATE_LOW:     sendbuf = 16*1024;  recvbuf=32*1024;      break;
    default:
    case EXPECTEDMSGRATE_DEFAULT: sendbuf=64*1024;    recvbuf=128*1024;     break;
    case EXPECTEDMSGRATE_HIGH:    sendbuf=1024*1024;  recvbuf=4096*1024;    break;
    case EXPECTEDMSGRATE_MAX:     sendbuf=0;          recvbuf=0;            break;
    }
    ism_transport_setMaxSocketBufSize(transport, sendbuf, recvbuf);

    /* Check now that we have the clientID and original client addr */
    if (ism_common_traceSelectClientID(transport->name) ||
        ism_common_traceSelectClientAddr(transport->client_addr))
        transport->trclevel = &ism_defaultDomain.selected;
}


/*
 * Return the shared client handle.
 */
void * ism_protocol_getSharedClient(int durable) {
    int  rc;
    void * ret = NULL;
    static ismSecurity_t * p_seccontext = NULL;
    static ism_transport_t * p_transport = NULL;

    pthread_spin_lock(&g_protocol_lock);

    if (!p_transport) {
        p_transport = ism_transport_newTransport(&dummyEndpoint, 0, 0);
        p_transport->protocol_family = "!dummy";
        p_transport->protocol = "";
        rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, p_transport, &p_seccontext);
    }

    switch (durable) {
    case 1:
        if (!g_client_Shared) {
            rc = ism_engine_createClientState("__Shared", PROTOCOL_ID_SHARED, ismENGINE_CREATE_CLIENT_OPTION_NONE,
                    NULL, NULL, p_seccontext, &g_client_Shared, NULL, 0, NULL);
            if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed state return code */
                rc = 0;
            }
            if (rc) {
                TRACE(1, "Unable to create __Shared, rc=%d", rc);
            }
        }
        ret = (void *)g_client_Shared;
        break;

    case 0:
        if (!g_client_SharedND) {
            rc = ism_engine_createClientState("__SharedND", PROTOCOL_ID_SHARED, ismENGINE_CREATE_CLIENT_OPTION_NONE,
                    NULL, NULL, p_seccontext, &g_client_SharedND, NULL, 0, NULL);
            if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed state return code */
                rc = 0;
            }
            if (rc) {
                TRACE(1, "Unable to create __SharedND, rc=%d", rc);
            }
        }
        ret = (void *)g_client_SharedND;
        break;

    case 2:
        if (!g_client_SharedM) {
            rc = ism_engine_createClientState("__SharedM", PROTOCOL_ID_SHARED, ismENGINE_CREATE_CLIENT_OPTION_NONE,
                    NULL, NULL, p_seccontext, &g_client_SharedM, NULL, 0, NULL);
            if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed state return code */
                rc = 0;
            }
            if (rc) {
                TRACE(1, "Unable to create __SharedM, rc=%d", rc);
            }
        }
        ret = (void *)g_client_SharedM;
    }
    pthread_spin_unlock(&g_protocol_lock);
    return ret;
}


/*
 * Start the protocol
 * @return A return code, 0=good
 */
int ism_protocol_start(void) {
    ism_engine_registerSelectionCallback(ism_protocol_selectMessage);
    ism_engine_registerDeliveryFailureCallback(ism_protocol_deliveryFailure);
    ism_protocol_startPlugin();
    ism_protocol_startForwarder();
    return 0;
}

/*
 * Terminate the protocol
 * @return A return code, 0=good
 */
int ism_protocol_term(void) {
    ism_protocol_termForwarder();
	ism_protocol_termPlugin();
    return 0;
}

/*
 * Preliminary authentication for LTPA/OAuth.
 */
int ism_protocol_auth(char * username, int isoauth, int isltpa) {
	if (isoauth && isltpa)
		return 1;
	if (isoauth) {
        if (username == NULL) {
            return 1;
        }
	    return strncasecmp(username, OAUTH_NAME,OAUTH_NAME_LEN);
	}
	if (isltpa) {
        if (username == NULL) {
            return 1;
        }
	    return strncasecmp(username,LTPA_NAME,LTPA_NAME_LEN);
	}
	return 1;
}
