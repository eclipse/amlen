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

#ifndef __ISM_ADMININTERNAL_DEFINED
#define __ISM_ADMININTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <ismutil.h>
#include <ismjson.h>
#include <admin.h>
#include <config.h>
#include <log.h>
#include <security.h>
#include <transport.h>
#include <store.h>
#include <ha.h>
#include <adminHA.h>
#include <assert.h>
#include <ctype.h>
#include <jansson.h>


/**
 * Admin modes
 */
#define ISM_MODE_PRODUCTION    0
#define ISM_MODE_MAINTENANCE   1
#define ISM_MODE_CLEANSTORE    2
#define ISM_MODE_STOREBACKUP   3


/* Security Protocol object */
typedef struct {
    const char *		name;
    int				id;
    int				capabilities;
} ismSecProtocol;


/**
 * Defines for time conversions
 */
#define SECS_PER_DAY  86400
#define SECS_PER_HOUR 3600
#define SECS_PER_MIN  60

#define DEFAULT_DYNAMIC_CFGNAME "server_dynamic.cfg"

/**
 * imaserver configuration check errors
 */
#define VM_ERROR_SIZE   0x1
#define VM_ERROR_VCPU   0x2
#define VM_ERROR_SYSMEM 0x4


/**
 * Internal function prototypes
 */
int32_t ism_admin_checkVMConfigCheck(void);
void    ism_admin_getMsgId(ism_rc_t code, char * buffer);
int32_t ism_admin_getActionType(const char *actionStr);
int32_t ism_admin_getServerProcType(void);
char *  ism_admin_getStateStr(int type);
int32_t ism_admin_stopTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata);
void    ism_admin_change_mode(char *modeStr, concat_alloc_t *output_buffer);
int32_t ism_admin_maskJSONField(const char * line, char * fieldName, char * retValue);
int32_t ism_admin_setReturnCodeAndStringJSON(concat_alloc_t *output_buffer, int rc, const char * returnString);
int32_t ism_admin_getServerProcType(void);
int32_t ism_admin_ha_send_client_configMsg(char *configMsg, int msgLen, int force);
int32_t ism_security_testClientAuthority(char *testObject, ism_json_parse_t *json, concat_alloc_t *output_buffer);
char *  ism_config_getComponentItemField(ism_prop_t * allProps, char *item, char * name, const char * fieldName);
int     ism_admin_closeConnection(ism_http_t *http);
int     ism_admin_processPluginNotification(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer);
int     ism_admin_getProtocolId(const char * name);
int32_t ism_admin_runMsshellScript(ism_json_parse_t *parseobj, concat_alloc_t *output_buffer);
int     ism_admin_getIfAddresses(char **ip, int *count, int includeIPv6);
int     ism_admin_isHAEnabled(void);
int     ism_admin_isSNMPEnabled(void);
int     ism_admin_isMQConnectivityEnabled(void);
int     ism_admin_isClusterConfigured(void);
int     ism_admin_isPluginEnabled(void);
int     ism_admin_testObject(const char *objtype, json_t *objval, concat_alloc_t *output_buffer);

/*
 * Returns monitoring or configuration schema JSON object
 *
 * @param  type     Type of schema
 *                  Supported types are:
 *                  ISM_CONFIG_SCHEMA and
 *                  ISM_MONITORING_SCHEMA
 *
 * Returns JSON obect on success or NULL or failure.
 *
 */
ism_json_parse_t * ism_admin_getSchemaObject(int type);

/* Get the callback list from schema for the specified object
 *
 * @param  type         The schema type
 * @param  name         The configuration item name
 * @param  callbacks    The pointer to return callback list
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * callbackList string has to be freed by caller
 *
 * */

int ism_config_getCallbacks(int type, char *itemName, char **callbacks);

/* Get the component from schema for the specified object
 *
 * @param  type         The schema type
 * @param  name         The configuration item name
 * @param  component    The pointer to return component name
 * @param  objtype      The pointer to return ObjectType
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * component string has to be freed by caller
 *
 * */

int ism_config_getComponent(int type, char *itemName, char **component, ism_objecttype_e *objtype);

/*
 * get an attribute value from JSON object
 */
char * ism_admin_getAttr(ism_json_parse_t *json, int pos, char *name);

/*
 * Check if the itemName has an associated component in schema and also return
 * the object type.
 */
int ism_config_isItemValid( char * itemName, char **component, ism_objecttype_e *objtype);

/*
 * Call certApply.sh to validate certificate and move it to keystore
 */
int32_t ism_admin_ApplyCertificate(char *action, char *arg1, char *arg2, char *arg3, char *arg4, char *arg5);

#ifdef __cplusplus
}
#endif

#endif
