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

#ifndef __CONFIG_DEFINED
#define __CONFIG_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <ismutil.h>
#include <ismxmlparser.h>
#include <ismjson.h>
#include <transport.h>
#include "admin.h"



/**
 * The "Configuration Service" in ISM implements a set of APIs for the
 * components to register with the "Configuration Service" and get
 * component configuration data. There are two types of configuration items:
 *
 * - Static configuration - These configuration items can not be modified
 *   by the users using WebUI or CLI options. These items are stored in
 *   ismserver.cfg file. For examples:
 *   ConfigDir = /opt/ibm/imaserver/config
 *   LogDir = /opt/ibm/imaserver/logs
 *   ....
 *
 * - Dynamic configuration - These configuration items are created, modified or
 *   deleted by users using WebUI or CLI. These items are stored in
 *   ismserver_dynamic.cfg file. For examples:
 *   LogLevel = INFO
 *   TraceLevel = 7
 *   Listener.Port.MQTTListener = 16102
 *   Listener.Security.MQTTListener = true
 *   ....
 */


#define ISM_CONFIG_SCHEMA        0
#define ISM_MONITORING_SCHEMA    1

typedef enum {
    ISM_INVALID_OBJTYPE    = 0,   /* Invalid object */
    ISM_SINGLETON_OBJTYPE  = 1,   /* Singleton object */
    ISM_COMPOSITE_OBJTYPE  = 2    /* Composite object */
}ism_objecttype_e;

/** Opaque structure for configuration object handle */
typedef struct ism_config_t ism_config_t;

/**
 * component types enumeration
 */
typedef enum  {
    ISM_CONFIG_COMP_SERVER          = 0,     /** Server         */
    ISM_CONFIG_COMP_TRANSPORT       = 1,     /** Transport      */
    ISM_CONFIG_COMP_PROTOCOL        = 2,     /** Protocol       */
    ISM_CONFIG_COMP_ENGINE          = 3,     /** Engine         */
    ISM_CONFIG_COMP_STORE           = 4,     /** Store          */
    ISM_CONFIG_COMP_SECURITY        = 5,     /** Security       */
    ISM_CONFIG_COMP_ADMIN           = 6,     /** Admin          */
    ISM_CONFIG_COMP_MONITORING      = 7,     /** Monitoring     */
    ISM_CONFIG_COMP_MQCONNECTIVITY  = 8,     /** MQConnectivity */
    ISM_CONFIG_COMP_HA              = 9,     /** HA */
    ISM_CONFIG_COMP_CLUSTER         = 10,    /** Cluster */
    ISM_CONFIG_COMP_LAST            = 11
} ism_ConfigComponentType_t;

#define ISM_VALIDATE_CONFIG_ENTRIES        32

/**
 * Configuration property types enumeration
 */
typedef enum {
    ISM_CONFIG_PROP_INVALID      = 0,   /** type invalid                   */
    ISM_CONFIG_PROP_NUMBER       = 1,   /** type Number                    */
    ISM_CONFIG_PROP_ENUM         = 2,   /** type Enum                      */
    ISM_CONFIG_PROP_STRING       = 3,   /** type String and StringBig      */
    ISM_CONFIG_PROP_NAME         = 4,   /** type Name                      */
    ISM_CONFIG_PROP_BOOLEAN      = 5,   /** type Boolean                   */
    ISM_CONFIG_PROP_IPADDRESS    = 6,   /** type IPAddress                 */
    ISM_CONFIG_PROP_URL          = 7,   /** type URL                       */
    ISM_CONFIG_PROP_REGEX        = 8,   /** type Regular Expression        */
    ISM_CONFIG_PROP_BUFFERSIZE   = 9,   /** type buffer size               */
    ISM_CONFIG_PROP_LIST         = 10,  /** type List                      */
    ISM_CONFIG_PROP_SELECTOR     = 11,  /** type Selector string           */
    ISM_CONFIG_PROP_REGEX_SUBEXP = 12   /** type RegEx with SubExpressions */
} ism_ConfigPropType_t;

/**
 * Object used for schema validation
 */
typedef struct ism_config_itemValidator {
	char *item;                                     /* Composite object name   */
    char *name[ISM_VALIDATE_CONFIG_ENTRIES];        /* item name               */
    char *defv[ISM_VALIDATE_CONFIG_ENTRIES];        /* default value           */
    char *options[ISM_VALIDATE_CONFIG_ENTRIES];     /* options value           */
    int   reqd[ISM_VALIDATE_CONFIG_ENTRIES];        /* required filed          */
    int   type[ISM_VALIDATE_CONFIG_ENTRIES];        /* ism_ConfigPropType_t    */
    char *min[ISM_VALIDATE_CONFIG_ENTRIES];         /* minimum value           */
    char *max[ISM_VALIDATE_CONFIG_ENTRIES];         /* maximum value           */
    int   minoneopt[ISM_VALIDATE_CONFIG_ENTRIES];   /* MinOneOption            */
    int   minonevalid[ISM_VALIDATE_CONFIG_ENTRIES]; /* Validly set for MinOneOpt */
    int   assigned[ISM_VALIDATE_CONFIG_ENTRIES];    /* item assigned           */
    int   total;                                    /* total number            */
    int   tempflag[ISM_VALIDATE_CONFIG_ENTRIES];    /* Can be used at run time */
} ism_config_itemValidator_t;

/**
 * Change types enumeration
 */
typedef enum  {
    ISM_CONFIG_CHANGE_PROPS     = 0,     /** Modified properties                  */
    ISM_CONFIG_CHANGE_NAME      = 1,     /** Modified properties and object name  */
    ISM_CONFIG_CHANGE_DELETE    = 2,     /** Object is deleted                    */
} ism_ConfigChangeType_t;

/*
 * Delete ClientSet enumeration
 */
typedef enum
{
	ismCLIENTSET_WAITING           = 0, ///< waiting to start
    ismCLIENTSET_START             = 1, ///< starting up
    ismCLIENTSET_HAVECLIENTLIST    = 2, ///< Have the client list
    ismCLIENTSET_DELETINGCLIENTS   = 3, ///< Currently deleting clients
    ismCLIENTSET_CLIENTSCOMPLETE   = 4, ///< Currently deleting clients
    ismCLIENTSET_DELETINGMSGS      = 5, ///< Currently delete RETAINed messages
    ismCLIENTSET_DONE              = 6, ///< Done
    ismCLIENTSET_RESTARTING        = 7,
    ismCLIENTSET_NOTFOUND          = 8, ///< Does not exist
} ismClientSetState_t;


/* MQC Object IDs */
typedef enum  {
    ISM_MQC_QMC      = 0,    /* QueueManagerConnection */
    ISM_MQC_DMR      = 1,    /* DestinationMappingRule */
    ISM_MQC_FLAG     = 2,    /* MQConnectivityEnabled  */
    ISM_MQC_LAST     = 3,
} ism_MQCObjectIDs_t;


/**
 * To receive dynamic configuration data from WebUI or CLI, components register
 * a callback function with the "Configuration Service". The callback function
 * prototype to receive changes (add, delete, or modify) is as follows:
 *
 * @param[in]  Object   Configuration object
 * @param[in]  name     Name of the configuration object
 * @param[in]  props    A properties object (contains modified properties only)
 * @param[in]  flag     Change flag - refer to ism_ConfigChangeType_t
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 * The "object" could be a composite object (e.g. "Listener" in "Transport"
 * component) or a configuration item (e.g. "LogLevel"). The "name" filed
 * is sent for composite objects only to identify the composite object in
 * the properties (for example "AdminListener" is returned for
 * "Listener.AdminListener").
 *
 * If flag is ISM_CONFIG_CHANGE_NAME, then old name will also be added in 'props'.
 * The name of the property will be "OLD_Name"
 *
 */
typedef int (*ism_config_callback_t)(
		char                    *object,
		char                    *name,
		ism_prop_t              *props,
		ism_ConfigChangeType_t  flag );


/**
 * Register a component with configuration service.
 *
 * To get configuration items from configuration service, components will have to
 * register with Configuration service, passing a callback function to receive
 * dynamic configuration changes (for the configurations that can be added, deleted
 * or modified by the users from WebUI or CLI). On successful registration, this API
 * returns a configuration object handle.
 *
 * @param[in]     type        Component type (specified in ism_ConfigComponentTypes_t)
 * @param[in]     object      A configuration object
 * @param[in]     callback    Callback function to receive configuration changes
 * @param[out]    handle      configuration object handle
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 * There are two mutually exclusive options available for callback registration:
 * - Components can register one callback function to receive configuration changes
 *   for all objects, by passing NULL for "object". For example, "Transport"
 *   component can register one callback to receive changes for all objects like
 *   "Listener", "websockets" etc.
 * - Components can register multiple callback functions to receive object specific
 *   configuration changes. For example, transport component can register one callback
 *   to receive changes for "Listener" object and different callback to receive
 *   changes for "Websockets" object.
 *
 * The configuration object could be a composite configuration object (e.g. "Listener"
 * in Transport component) or one configuration item (e.g. LogLevel, TraceLevel).
 *
 */
XAPI int32_t ism_config_register(
		ism_ConfigComponentType_t    type,
		const char *                 object,
		ism_config_callback_t        callback,
		ism_config_t **              handle );


/**
 * Get a properties object of a component.
 * Note: Some of the configuration objects returned could be from
 * from static or dynamic configuration list. For example, the "Listener" 
 * configuration in Transport component could be in static list 
 * (Admin Listener) and dynamic list (Listeners configured by the users).
 * 
 * @param[in]  handle  Component configuration handle
 * @param[in]  object  Composite object.
 * @param[in]  name    Configuration item name.
 *
 * Returns a properties object on success or NULL
 *
 *
 * Examples:
 *
 * - To get configuration of a specific composite object, pass a valid
 *   composite object for "object" and NULL for "name"
 *   in ism_config_getProperties() API. For example, to get configuration
 *   items of "Listener" composite object whose name is "AdminListener" in
 *   the "Transport" component:
 *
 * ism_config_t *handle = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, cfgCallbackTransport);
 * ism_prop_t *props = ism_config_getProperties(handle, "Listener", "AdminListener");
 * The props structure will have configuration that could be represented by
 * the following key=value pairs:
 *
 * Listener.Port.AdminListener = 16102
 * Listener.Security.AdminListener = true
 * Listener.Protocol.AdminListener = MQTT
 * ....
 *
 *
 * - To get configuration items of all Listeners, pass a valid composite object
 *   for "object" and NULL for "name" in ism_config_getProperties() API.
 *   For example, to get configuration items of all "Listener" composite objects
 *   in the "Transport" component:
 *
 * ism_config_t *handle = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, cfgCallbackTransport);
 * ism_prop_t *props = ism_config_getProperties(handle, "Listener", NULL);
 * The props structure will have configuration that could be represented
 * by the following key=value pairs:
 *
 * Listener.Port.MQTTListener = 16102
 * Listener.Security.MQTTListener = true
 * Listener.Protocol.MQTTListener = MQTT
 * .......
 * Listener.Port.JMSListener = 17855
 * Listener.Security.JMSListener = false
 * .........
 *
 *
 * - To get one configuration item, pass NULL for "object", and a valid
 *   configuration item name for "name" in ism_config_getProperties() API.
 *   For example, to get configuration item whose name is "FIPS" in the
 *   "Transport" component.
 *
 * ism_config_t *handle = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, cfgCallbackTransport);
 * ism_prop_t *props = ism_config_getProperties(handle, NULL, "FIPS");
 * The props structure will have configuration that could be represented by the
 * following key=value pair:
 *
 * FIPS = false
 *
 */
XAPI ism_prop_t * ism_config_getProperties(
		ism_config_t * handle,
		const char *   object,
		const char *   name );

/**
 * Get instance names of a specific component configuration object type
 *
 * Returns properties whose name is the instance name and value is a simple boolean 'true'.
 */
XAPI ism_prop_t * ism_config_getObjectInstanceNames(
        ism_config_t * handle,
        const char *   object);

/**
 * Get dynamic properties of a configuration objects in a component
 *
 * Returns properties only from dynamic configuration list.
 */
XAPI ism_prop_t * ism_config_getPropertiesDynamic(
		ism_config_t * handle,
		const char * object,
		const char * name );


/**
 * UnRegister a component with configuration service.
 *
 * @param[in]    handle      configuration object handle
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */
XAPI int32_t ism_config_unregister(
		ism_config_t *  handle );


/**
 * Get configuration handle by component name
 *
 * @param[in]   type     Component type (specified in ism_ConfigComponentTypes_t)
 * @param[in]   object   A configuration object
 *
 * Returns valid pointer on success or NULL if component is not
 * registered with the configuration service.
 */
XAPI ism_config_t * ism_config_getHandle(
		ism_ConfigComponentType_t   type,
		const char *                object );


/**
 * Initialize configuration service.
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI uint32_t ism_config_init(void);

/*
 * Terminate configuration service
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI uint32_t ism_config_term(void);

/*
 * Disable dynamic set - used for HA pair
 */
XAPI uint32_t ism_config_disableSet(void);

/*
 * Enable dynamic set - used for HA pair
 */
XAPI uint32_t ism_config_enableSet(void);

/*
 * Dump dynamic configuration to a file 
 */
XAPI uint32_t ism_config_dumpConfig(const char *filepath, int proctype);

/*
 * Dump dynamic configuration to a file in JSON format
 * - One object per line
 */
XAPI uint32_t ism_config_dumpJSONConfig(const char * filepath);

/*
 * Sync dynamic configuration from a file in JSON format
 * on a standby node
 */
XAPI uint32_t ism_config_processJSONConfig(const char * filepath);

/* Get SSL FIPS configuration*/
XAPI int ism_config_getFIPSConfig(void);

/* Get MQConnectivityEnabled configuration */
XAPI int ism_config_getMQConnEnabled(void);

/* Returns 1 is SSL is configured to use buffer pool */
XAPI int ism_config_getSslUseBufferPool(void);

/* Returns all properties of a component */
XAPI ism_prop_t * ism_config_getComponentProps(ism_ConfigComponentType_t comp);

/* Return component type from name */
XAPI int32_t ism_config_getCompType(const char *compname);

/* Set HighAVailablity Group in HA configuration */
XAPI int32_t ism_config_setHAGroupID(const char *name, int len, const char *standbyID);

/* return disconnected client notification interval */
XAPI int ism_config_getDisconnectedClientNotifInterval(void);

/*
 *
 * Set dynamic config from ism_prop_t object
 *
 * @param  json                   The JSON string for set configuration
 * @param  validateData           The flag indicates get handle action
 * @param  inpbuf                 The original input string
 * @param  inpbuflen              The input string length
 * @param  props                  The configuration properties will be created from input string
 * @param  storeOnStandby         The flag about store status
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_set_object(ism_json_parse_t *json, int validateData, char *inpbuf, int inpbuflen, ism_prop_t *props, int storeOnStandby);

/*
 * create/delete/update MessageHub configuration
 * Set dynamic config from ism_prop_t object
 *
 * @param  handle                 The configuration handle for the component
 * @param  item                   The configuration object item
 * @param  action                 Operation action. 0 - create, 1- update, 2 - delete
 * @param  mode                   The flag to callback
 * @param  props                  The configuration properties created from input string
 * @param  purgeCompProp          The pruge flag will be returned
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_set_MessageHub(ism_config_t *handle, char *item, int action, int mode, ism_prop_t *props, int *purgeCompProp);

XAPI int32_t ism_config_getObjectConfig(char *component, char *item, char *name, char *idField, int type, concat_alloc_t *retStr, int getConfigLock, int deleteFlag);

/*
 * Get singelton object and create REST API return JSON payload in http outbuf.
 *
 * @param  component      component
 * @param  name           specific name
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_singletonObject(char *component, char *name, ism_http_t * http);

/*
 * Get composite object and create REST API return JSON payload in http outbuf.
 *
 * @param  component      component
 * @param  item           query object
 * @param  name           specific name of the object. Can be NULL
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_compositeObject(char *component, char *item, char *name, ism_http_t *http);

/*
 * Get TrustedCertificate and create REST API return JSON payload in http outbuf.
 * TrustedCertificate is not a name object and has to be treated as a special case.
 *
 * @param  secProfile     SecurityProfile name
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_trustedCertificate(char *secProfile, ism_http_t *http);

/*
 * Create REST API error return message in http outbuf.
 *
 * @param  http          ism_http_t object
 * @param  retcode       error code to be return
 * @param  repl          replacement string array
 * @param  replSize      replacement string array size.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI void ism_confg_rest_createErrMsg(ism_http_t * http, int retcode, const char **repl, int replSize);

/*
 *
 * Handle RESTAPI get configuration object actions.
 * The return result will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_getAction(ism_http_t *http, ism_rest_api_cb callback);

/*
 *
 * Handle RESTAPI delete configuration object actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 * @param  pflag        The path flag indicates the calling path.
 * 						0 -- /configuration/
 * 						1 -- /service/
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_deleteAction(ism_http_t *http, int pflag, ism_rest_api_cb callback);

/*
 *
 * Handle RESTAPI upload file actions
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_fileUploadAction(ism_http_t *http, ism_rest_api_cb callback);

/*
 *
 * Handle RESTAPI get service status actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_serviceGetStatus(ism_http_t *http, ism_rest_api_cb callback);

/*
 *
 * Handle RESTAPI post service actions.
 * The return result or error will be in http->outbuf
 *
 * @param  http         ism_htt_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_restapi_servicePostAction(ism_http_t *http, ism_rest_api_cb callback);

/*
 * Process HTTP Post request
 *
 * @param  http          ism_http_t object
 * @param  httpCallback  HTTP callback invoked to sent result asynchronously
 *
 * The POST process results are sent back in http->outbuf.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_json_processPost(ism_http_t *http, ism_rest_api_cb httpCallback);

/*
 * Get ClientCertificate and create REST API return JSON payload in http outbuf.
 * ClientCertificate is not a name object and has to be treated as a special case.
 *
 * @param  secProfile     SecurityProfile name
 * @param  http           ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_clientCertificate(char *secProfile, ism_http_t *http);

/*
 * Create REST API error return MQC message in outbuf.
 */
XAPI void ism_confg_rest_createMQCErrMsg(concat_alloc_t *outbuf, const char *locale, int retcode, const char **repl, int replSize);


#ifdef __cplusplus
}
#endif

#endif

