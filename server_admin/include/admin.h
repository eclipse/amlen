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

#ifndef __ADMIN_DEFINED
#define __ADMIN_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <ismutil.h>
#include <ismxmlparser.h>
#include <transport.h>

#define SERVER_SCHEMA_VERSION  "v1"

/**
 * Server states
 */
typedef enum {
    ISM_SERVER_INITIALIZING       = 0,
    ISM_SERVER_RUNNING            = 1,
    ISM_SERVER_STOPPING           = 2,
    ISM_SERVER_INITIALIZED        = 3,
    ISM_TRANSPORT_STARTED         = 4,
    ISM_PROTOCOL_STARTED          = 5,
    ISM_STORE_STARTED             = 6,
    ISM_ENGINE_STARTED            = 7,
    ISM_MESSAGING_STARTED         = 8,
    ISM_SERVER_MAINTENANCE        = 9,
    ISM_SERVER_STANDBY            = 10,
    ISM_SERVER_STARTING_STORE     = 11,
    ISM_SERVER_STARTING           = 12,
    ISM_SERVER_STOPPED            = 13,
    ISM_SERVER_UNKNOWN            = 14,
    ISM_SERVER_MAINTENANCE_ERROR  = 15,
    ISM_SERVER_STARTING_ENGINE    = 16,
    ISM_SERVER_STARTING_TRANSPORT = 17,
    ISM_SERVER_STATUS_LAST        = 18,
} ism_ServerState_t;

/**
 * Admin action types enumeration
 */
typedef enum  {
    ISM_ADMIN_SET                       = 0,     /** Set                               */
    ISM_ADMIN_GET                       = 1,     /** Get                               */
    ISM_ADMIN_STOP                      = 2,     /** Stop                              */
    ISM_ADMIN_APPLY                     = 3,     /** Apply                             */
    ISM_ADMIN_IMPORT                    = 4,     /** Import                            */
    ISM_ADMIN_EXPORT                    = 5,     /** Export                            */
    ISM_ADMIN_TRACE                     = 6,     /** Trace                             */
    ISM_ADMIN_VERSION                   = 7,     /** Version                           */
    ISM_ADMIN_STATUS                    = 8,     /** Status                            */
    ISM_ADMIN_SETMODE                   = 9,     /** Admin Mode                        */
    ISM_ADMIN_HAROLE                    = 10,    /** HA Node Role                      */
    ISM_ADMIN_CERTSYNC                  = 11,    /** Synchronize certificates          */
    ISM_ADMIN_TEST                      = 13,    /** Test connection                   */
    ISM_ADMIN_PURGE                     = 14,    /** Purge - store/log                 */
    ISM_ADMIN_ROLLBACK                  = 15,    /** Rollback Transaction              */
    ISM_ADMIN_COMMIT                    = 16,    /** Commit Transaction                */
    ISM_ADMIN_FORGET                    = 17,    /** Forget Transaction                */
    ISM_ADMIN_SYNCFILE                  = 18,    /** Sync file with Standby            */
    ISM_ADMIN_CLOSE                     = 19,    /** Close action                      */
    ISM_ADMIN_NOTIFY                    = 20,    /** Notification from other component */
    ISM_ADMIN_RUNMSSHELLSCRIPT          = 21,    /** Run a script                      */
    ISM_ADMIN_CREATE                    = 22,    /** Create object configuration       */
    ISM_ADMIN_UPDATE                    = 23,    /** Update object configuration       */
    ISM_ADMIN_DELETE                    = 24,    /** Delete object configuration       */
    ISM_ADMIN_RESTART                   = 25,    /** Restart server or component       */
    ISM_ADMIN_MONITOR                   = 26,    /** Monitoring requests - statistics  */
    ISM_ADMIN_LAST                      = 27
} ism_AdminActionType_t;


/**
 * Server admin state enumeration
 */
typedef enum  {
    ISM_ADMIN_STATE_PAUSE      = 0,
    ISM_ADMIN_STATE_CONTINUE   = 1,
    ISM_ADMIN_STATE_STOP       = 2,
    ISM_ADMIN_STATE_TERMSTORE  = 3
} ism_adminState_t;


/**
 * Protocol capabilities enumeration
 */
typedef enum  {
    ISM_PROTO_CAPABILITY_USETOPIC     = 0x01,
    ISM_PROTO_CAPABILITY_USESHARED    = 0x02,
    ISM_PROTO_CAPABILITY_USEQUEUE     = 0x04,
    ISM_PROTO_CAPABILITY_USEBROWSE    = 0x08
} ism_protoCapability_t;


/**
 * Server admin restart modes
 */
typedef enum  {
    ISM_ADMIN_MODE_STOP        = 0,
    ISM_ADMIN_MODE_RESTART     = 1,
    ISM_ADMIN_MODE_CLEANSTORE  = 2,
    ISM_ADMIN_MODE_DISKBACKUP  = 3
} ism_adminMode_t;

typedef struct adminRestartTimerData {
    int  isService;
    int  isMaintenance;
    int  isCleanstore;
} adminRestartTimerData_t;


#define MAX_OBJ_NAME_LEN    256
#define USERFILES_DIR  IMA_SVR_DATA_PATH "/userfiles/"
#define DEFAULT_PLUGIN_CONFIG_NAME "plugin.json"
#define DEFAULT_PLUGIN_CONFIG_NAME_LEN 11
#define DEFAULT_PLUGIN_PROPS_FILE_NAME "pluginproperties.json"
#define DEFAULT_PLUGIN_PROPS_FILE_NAME_LEN 21
#define STAGING_INSTALL_DIR  IMA_SVR_DATA_PATH "/data/config/plugin/staging/install/"
#define STAGING_UNINSTALL_DIR IMA_SVR_DATA_PATH "/data/config/plugin/staging/uninstall/"
#define PLUGINS_DIR IMA_SVR_DATA_PATH "/data/config/plugin/plugins/"
#define PSK_DIR IMA_SVR_INSTALL_PATH "/certificates/PSK/"
#define PSK_FILE IMA_SVR_INSTALL_PATH "/certificates/PSK/psk.csv"

#define MAX_PROTOCOL    50

extern int serverState;

extern int storeNotAvailableError;

extern int cleanStore;

extern int adminModeRC;

extern int haSyncInProgress;

extern int restartServer;

extern int resetConfig;

extern int adminMode;

extern int haRestartNeeded;

/**
 * Initialize admin and security service
 */
XAPI int32_t ism_admin_init(int proctype);

/**
 * Terminate admin and security service
 */
XAPI int32_t ism_admin_term(int proctype);

/**
 * Start stop process
 * mode = 0 - stop
 * mode = 1 - restart
 * mode = 2 - clean store and restart
 */
XAPI int ism_admin_init_stop(int mode, ism_http_t *http);

/**
 * Send a signal to stop ISM server process
 */
XAPI int32_t ism_admin_send_stop(int mode);

/**
 * Send a signal to change continue with start process
 */
XAPI int32_t ism_admin_send_continue(void);

/**
 * To make admin and security service pause and wait to receive a signal
 * from CLI or WebUI to stop ISM server process
 */
XAPI int32_t ism_admin_pause(void);

/**
 * Get server state
 */
XAPI int32_t ism_admin_get_server_state(void);

/**
 * Get server mode
 */
XAPI int32_t ism_admin_getmode(void);

/**
 * @brief  Process Admin action (REVISIT - API will change)
 *
 * Called by protocol to process administrative actions like:
 * - set or get configuration data
 * - stop server
 * - check server status
 * - get version
 *
 * @param[in]     transport        Pointer to transport object
 * @param[in]     input_buffer     Input buffer containing object name,
 *                                 action and any required arguments
 * @parm[in]      buffer_len       Length of buffer
 * @param[out]    output_buffer    Returns results
 * @param[out]    rc               Return code
 *
 * Returns 0 on success.
 */
XAPI int ism_process_admin_action(ism_transport_t *transport, const char *input_buffer, int buffer_len, concat_alloc_t *output_buffer, int *rc);

/**
 * Set the last error code and string into the Admin component.  
 * 
 */
XAPI void ism_admin_setLastRCAndError(void); 

/**
 * Set disable HA for clean store
 *
 */
XAPI void ism_admin_ha_disableHA_for_cleanStore(void);

/**
 * Set Mode of the server
 *
 * @param[in]     mode        Mode of the server
 *                            0 - Production
 *                            1 - Maintenance
 * @param[in]     ErrorCode   If Maintenance mode is set by server in case of an internal error.
 *                            RC should be defined in ismrc.h.
 *
 * Returns 0 on success or RC
 */
XAPI int32_t ism_admin_setAdminMode(int mode, int ErrorCode);

/**
 * Update the Capabilities for a protocol.
 *
 * This function combines the capabilities for all plug-ins with
 * the same protocol.
 *
 * @param 	name			Protocol name
 * @param 	capabilities	32-bit value in hex using Bitwise OR.
 * @return 	If successful, the function shall return zero; otherwise,
 *          non zero number shall be returned to indicate the error.
 */
XAPI int32_t ism_admin_updateProtocolCapabilities(const char * name, int capabilities);

/**
 * Get the capabilities for a protocol
 * @param 	name			Protocol name
 * @return the capabilities value which greater 0. If protocol is not found, zero will be returned.
 */
XAPI int ism_admin_getProtocolCapabilities(const char * name);

/**
 * Stop the Plug-in Java Process
 * @return 0 for success. 1 for error.
 */
XAPI int ism_admin_stopPluginProcess(void);

/*
 * Get imaserver status REST API return JSON payload in http outbuf.
 *
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_admin_getServerStatus(ism_http_t *http, const char * service, int fromSNMP);

/*
 * The hook for admin thread to finish existing delete ClientSet
 * after server restart
 */
void ism_config_startClientSetTask();

/**
 * Callback to register with domain socket based server
 * @param    pData     Pointer to data buffer to receive message
 * @param    pDataLen  Size of the received data buffer
 * Returns ISMRC_OK on success or ISMRC_* on error
 */
typedef int (*ism_adminDS_callback_t)( char *pData, int pDataLen );

/**
 * Register a callback with domain socker based server
 * param    callback  Pointer to callback function
 * Returns ISMRC_OK on success or ISMRC_* on error
 */
XAPI int32_t ism_adminDSServer_register_callback(ism_adminDS_callback_t callback);

/**
 * Start Domain socket based server using ism_common_startThread() API.
 * Refer to ism_common_startThread() API in server_utils for details.
 */
XAPI void * ism_admin_dsServer_thread(void * param, void * context, int value);

/**
 * Client function to send message to domain socket based server
 * @param    msg         Message buffer to be sent to server
 * @param    msglen      Message length
 * @param    result      Return result buffer from server
 * @param    resultlen   Length of return buffer
 */
XAPI int32_t ism_admin_dsClient_sendMessage(char *msg, int msglen, char **result, int *resultlen);

/*
 *
 * restart services from RESTAPI service call payload.
 *
 * @param  http            pointer to ism_http_t
 * @param  isService       imaserver is 1, mqconnectivity is 2, -1 is invalid
 * @param  isMaintenance   0 means stop maintenance mode, 1 mean enable maintenance
 * @param  isCleanStore    1 means do cleanstore
 * @param  proctype        Process type as defined in ismutil.h
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
XAPI int ism_admin_restartService(ism_http_t *http, int isService, int isMaintenance, int isCleanstore, int proctype);

/**
 * Function to get configuration data in SNMP Agent process
 */
XAPI int ism_process_adminSNMPRequest(const char *input_buffer, int buffer_len, const char * locale, concat_alloc_t *output_buffer, int *rc);

/**
 * API to get SNMP enabled flag
 * Returns 1 if enabled
 */
XAPI int ism_admin_isSNMPConfigured(int locked);

/**
 * Restart SNMP process
 */
XAPI int ism_admin_restartSNMP(void);

/*
 * Start any CRL timers if necessary
 */
XAPI void ism_admin_startCRLTimers(const char * endpointName, const char * secProfileName);

/**
 * Admin functions related to MQConnectivity process
 */
typedef void (*ism_rest_api_cb)(ism_http_t * http, int retcode);
XAPI int ism_process_adminMQCRequest(const char *input_buffer, int buffer_len, const char * locale, concat_alloc_t *output_buffer, int *rc);
XAPI int ism_admin_init_mqc_channel(void);
XAPI int ism_admin_start_mqc_channel(void);
XAPI int ism_admin_stop_mqc_channel(void);
XAPI void ism_admin_mqc_init(void);
XAPI int ism_admin_mqc_pause(void);
XAPI int ism_admin_mqc_send(char * buff, int length, ism_http_t * http, ism_rest_api_cb callback, int persistData, int objID, char *objName);
XAPI void ism_config_setAdminMode(int mode);
XAPI int ism_config_getAdminMode(void);
XAPI int ism_admin_checkLicenseIsAccepted(void);
XAPI void ism_admin_cunit_acceptLicense(void);

/**
 * Set server mode to Maintenance mode
 *
 * This API can be called by components to take the server down in maintenance mode in
 * case of any unrecoverable error.
 *
 * @param    errorRC     Error code exposed in status call - the error should be one of the
 *                       RC defined in ismrc.h file.
 * @param    restartFlag Set this to 1 if server restart is needed.
 *                       On restart of server, server will start in Maintenance mode.
 *                       Users can fix the problem and restart server in Production mode.
 *
 * Returns ISMRC_OK on success or ISMRC_* on error
 *
 */
XAPI int ism_admin_setMaintenanceMode(int errorRC, int restartFlag);
XAPI int ism_admin_isResourceSetDescriptorDefined(int locked);
XAPI void ism_admin_getActiveResourceSetDescriptorValues(const char **pClientID, const char **pTopic);

#ifdef __cplusplus
}
#endif

#endif

