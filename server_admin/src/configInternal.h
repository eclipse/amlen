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

#ifndef __ISM_CONFIGINTERNAL_DEFINED
#define __ISM_CONFIGINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <time.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#include <config.h>
#include <ismjson.h>
#include <engine.h>
#include <janssonConfig.h>

#include "adminInternal.h"




/**
 * Defines used in config functions
 */
#define MAX_JSON_ENTRIES 1000
#define DEFAULT_CONFIGDIR   "./"
#define DEFAULT_POLICYDIR   "./policies"
#define ISM_UUID_LENGTH 25

/* Component configuration handle */
struct ism_config_t {
    int                   comptype;
    const char *          objectname;
    ism_config_callback_t callback;
    int                   refcount;
};

/* configuration handles */
typedef struct {
    int                 type;
    ism_config_t **     handle;
    int                 id;
    int                 count;
    int                 nalloc;
    int                 slots;
} ismConfigHandles_t;

/* ism version */
typedef struct {
    int                 version;
    int                 release;
    int                 mod;
    int                 fixpack;
} ismConfigVersion_t;

/* Supported component types, names and property list */
struct compProps_t {
    int          type;
    char       * name;
    ism_prop_t * props;
    uint64_t     nrec;
};

extern struct compProps_t compProps[];

typedef struct {
	int        comp_type;
	char       * name;
	ism_prop_t **plist;
	int        ncount;
}configSet_t ;

typedef struct ismAdmin_DeleteClientSetMonitor_t ismAdmin_DeleteClientSetMonitor_t;

/**
 * Internal function prototypes
 */
int32_t ism_config_getAdminLog(void);
int32_t ism_config_validate_set_data(ism_json_parse_t *json, int *mode);
int32_t ism_config_updateEndpointInterface(ism_prop_t * props, char * name, int mode);
int32_t ism_config_valEndpointObjects(ism_prop_t * props, char * name, int isCreate);
int32_t ism_config_valDeleteEndpointObject(char *object, char * name);
//int32_t ism_config_valPolicyOptionalFilter(ism_prop_t * props, char * name);
int32_t ism_config_validateDeleteCertificateProfile(char * name);
int32_t ism_config_deleteCertificateProfileKeyCert(char * cert, char * key, int needDeleteCert, int needDeleteKey);
int32_t ism_config_deleteSecurityProfile(char * name);
int32_t ism_config_deleteLTPAKeyFile(char * name);
int32_t ism_config_validateCertificateProfileExist(ism_json_parse_t *json, char * name, int isUpdate);
int32_t ism_config_validateLTPAProfileExist(ism_json_parse_t *json, char * name);
int32_t ism_config_validateOAuthProfileExist(ism_json_parse_t *json, char * name);
int32_t ism_config_validateCertificateProfileKeyCertUnique(ism_json_parse_t *json, char * name);
int32_t ism_config_getCertificateProfileKeyCert(char *name, char **cert, char ** key, int getLock);
int32_t ism_config_validateDeleteLTPAProfile(char * name);
int32_t ism_config_getSecurityLog(void);
int32_t ism_config_set_dynamic(ism_json_parse_t *json);
void ism_config_convertVersionStr(const char *versionStr, ismConfigVersion_t **pversion);
int32_t ism_config_validate_object(int actionType, ism_json_parse_t *json, char *inpbuf, int inpbuflen, ism_prop_t *props);


void    ism_security_setAuditControlLog(int newval);

int32_t ism_ha_admin_update_standby(char *inpbuf, int inpbuflen, int flag);
int     ism_ha_admin_isUpdateRequired(void);

/*
 * create configuration log
 *
 * @param  item                   The configuration object item
 * @param  name                   The configuration object name
 * @param  rc                     The error code
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
void ism_config_addConfigLog(char *item, char *name, int rc);

/*
 * Rollback each successful callback that was called
 * for a multiple callback configuration update.
 *
 * @param item                   The configuration object item
 * @param name                   The configuration object name
 * @param callbackList           The list of callbacks specified in the schema for this item
 * @param props                  The original configuration properties in the component
 * @param mode                   The flag to callback
 * @param action                 The action to take
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int32_t ism_config_rollbackCallbacks(char *item, char *name, int callbackList[], ism_prop_t *props, int mode, int action);

/*
 * Calls each callback specified in an items Callback property from
 * the schema. If a callback fails, rollback previous callbacks.
 *
 * @param handle                 The configuration handle for the component
 * @param item                   The configuration object item
 * @param name                   The configuration object name
 * @param props                  The configuration properties created from input string
 * @param mode                   The flag to callback
 * @param action                 The action to take
 * @param callbackList           The list of callbacks for the configuration object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int32_t ism_config_multipleCallbacks(ism_config_t *handle, char *item, char *name, ism_prop_t *props, int mode, int action, char *callbackList);

/*
 * Call the callback methods for the updated configuration item.
 *
 * @param  handle                 The configuration handle for the component
 * @param  item                   The configuration object item
 * @param  name                   The configuration object name
 * @param  mode                   The flag to callback
 * @param  props                  The configuration properties created from input string
 * @param  action                 The action to take
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_callCallbacks(ism_config_t *handle, char *item, char *name, int mode, ism_prop_t *props, int action);

/*
 * update/delete configuration repository
 *
 * @param  handle                 The configuration handle for the component
 * @param  item                   The configuration object item
 * @param  name                   The configuration object name
 * @param  mode                   The flag to callback
 * @param  props                  The configuration properties created from input string
 * @param  cprops                 The current configuration properties in the component
 * @param  purgeCompProp          The pruge flag will be returned
 * @param  action                 The action to take
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_updateConfigRepository(ism_config_t *handle, char *item, char *name, int mode, ism_prop_t *props, ism_prop_t *cprops, int *purgeCompProp, int action);

/*
 * HA sync-up between primary and standby for the configuration change
 *
 * @param  item                   The configuration object item
 * @param  action                 Operation action. Only support 0 - create, 1- update.
 * @param  composite              The flag indicates if the item is a composite object
 * @param  adminMode              The current imaserver admin mode.
 * @param  isHAConfig             The flag indicate if HA has been enabled
 * @param  inpbuf                 The original input string
 * @param  inpbuflen              The input string length
 * @param  epbuf                  The special buffer string for an endpoint config string.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_HASyncUp(char *item, int action, int composite, int adminMode, int isHAConfig, char *inpbuf, int inpbuflen, concat_alloc_t epbuf);

int copyFile(const char *source, const char *destination);

/*
 * Handle the ClientSet commands "delete"
 *
 * @param  json                   The pased json for the command
 * @param  validateData           Boolean whether to validate the data
 * @param  inpbuf                 Original input buffer
 * @param  inpbuflen              and length
 * @param  props                  The parsed properties
 * @param  retbuf                 pointer to store pointer to return string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int32_t ism_config_ClientSet(ism_json_parse_t *json, int validateData, char *inpbuf, int inpbuflen, ism_prop_t *props, int actionType, char **retbuf);

XAPI ism_json_parse_t * ism_config_json_loadJSONFromFile(const char * name);

XAPI int ism_config_json_createClientSetConfig(ism_json_parse_t * parseobj, int where);

XAPI int ism_config_json_parseServiceTraceFlushPayload(ism_http_t *http);

/*
 *
 * Add a clientset into the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_addClientSetToList(const char * clientID, const char * retain);

/*
 *
 * Delete a clientset from the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_deleteClientSetFromList(const char * clientID, const char * retain);

/*
 *
 * Update the clientset status in the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 * @param  status           The enum value of the clientset status
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
int ism_config_updateClientSetStatus(const char * clientID, const char * retain, ismClientSetState_t status);

/*
 *
 * Get the clientset status from the deleting list.
 *
 * @param  clientID         The regular expression of clientID
 * @param  retain           The regular expression of retained message. Can be NULL.
 * @param  clientSet        A pointer to ismAdmin_DeleteClientSetMonitor_t * to hold return status
 *
 * Returns enum value of ismClientSetState.
 * */
ismClientSetState_t ism_config_getClientSetStatus(const char *clientID, const char *retain, ismAdmin_DeleteClientSetMonitor_t ** clientSet, int eel);

/*
 * CUNIT test only function
 */
int ism_validate_ClientSetStatus(const char *clientID, const char *retain, ismClientSetState_t status);

/*
 *
 * parse http service start POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
int ism_config_json_parseServiceStartPayload(ism_http_t *http);

/*
 *
 * parse http service restart POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
int ism_config_json_parseServiceRestartPayload(ism_http_t *http);

/*
 *
 * parse http service stop POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
int ism_config_json_parseServiceStopPayload(ism_http_t *http);

#define LTPA_CERTIFICATE  0
#define OAUTH_CERTIFICATE 1
/*  Rollback LTPA keyfile if configuration change failed
 *
 * @param  props         the LTPA configuration properties
 * @param  rollbackFlag  the rollback flag indicate the stage failure occured.
 * 		                 0 -- keyfile, password validation. Certificate is still in /tmp/userfiles dir
 * 		                 1 -- configuration failure. Certificate has been moved to LTPA keystore
 * @param  authType      the authority type
 *                       0 -- LTPA
 *                       1 -- OAuth
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_rollbackCertificateConfig(ism_prop_t *props, int rollbackFlag, int authType);

/*  Rollback LTPA/OAuth keyfile if configuration change failed
 *
 * @param  profile       the LTPA/OAuth profile name
 * @param  keyfile       the original certificate file name
 * @param  rollbackFlag  the rollback flag indicate the stage failure occured.
 * 		                 0 -- keyfile, password validation. Certificate is still in /tmp/userfiles dir
 * 		                 1 -- configuration failure. Certificate has been moved to LTPA keystore
 * @param  authType      the authority type
 *                       0 -- LTPA
 *                       1 -- OAuth
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_rollbackCertificate(const char *profile, const char *keyfile, int rollbackFlag, int authType);


/**
 * Returns component type defined by ism_ConfigComponentType_t enum.
 *
 * @param  compName    Component name
 *
 */
XAPI ism_ConfigComponentType_t ism_config_getComponentType(const char *compName);

/**
 * Returns configuration property type defined by ism_ConfigPropType_t enum
 *
 * @param  propName    Property type string
 *
 */
XAPI ism_ConfigPropType_t ism_config_getConfigPropertyType(const char *propName);

/**
 *  Returns monitoring or configuration schema JSON formated string
 *
 *  @param  type    Schema type
 *
 */
XAPI char * ism_admin_getSchemaJSONString(int type);

/**
 * Get dynamic properties of a configuration objects in a component.
 * return object exist flag
 */
XAPI ism_prop_t * ism_config_getPropertiesExist(ism_config_t * handle, const char * object, const char * name, int *doesObjExist);

/**
 * Get dynamic properties of a configuration objects in a component from JSON config file
 * return object exist flag
 */
XAPI ism_prop_t * ism_config_getPropertiesExistNew(ism_config_t * handle, const char * object, const char * name, int *doesObjExist);

/* Handle RESTAPI post verify LDAP object action.
 *
 * @param  props        the config props from POST payload
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int ism_security_testLDAPConfig( ism_prop_t *props);

/*
 *
 * Check if Certificate and Key file specified in payload
 * is unique to the CertificateProfile.
 * If the Certificate and Key file name has been used by the CertificateProfile
 * other than the name specified, this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 * @param  keyName      The Key file name
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateCertificateProfileKeyCertUnique(char * name, char *certName, char *keyName);

/*
 *
 * Check if LTPA Certificate specified in payload
 * is unique to the LTPAProfile.
 * If the Certificate  file name has been used by the other LTPAProfile,
 * this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateLTPACertUnique(char * name, char *certName);

/*
 *
 * Check if OAuth Certificate specified in payload
 * is unique to the OAuthProfile.
 * If the Certificate  file name has been used by the other OAuthProfile,
 * this checking will fail.
 *
 * @param  name         The CertificateProfile name.
 * @param  certName     The Certificate file name.
 *
 * Returns ISMRC_OK on passing the checking, ISMRC_* on error
 * */
int32_t ism_config_json_validateOAuthCertUnique(char * name, char *certName);

/* Handle RESTAPI apply LTPA certificate
 *
 * @param  name         The LTPAProfile name
 * @param  keyfile      The ltpa certificate
 * @param  pwd          The ltpa certificate password
 * @param  overwrite    If the existing OAuth certificate can be overwritten if the file name is the same.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyLTPACert(const char *name, char *keyfile, char *pwd, int overwrite);

/* Handle RESTAPI apply OAuth certificate
 *
 * @param  name         The OAuthProfile name
 * @param  keyfile      The OAuth certificate
 * @param  overwrite    If the existing OAuth certificate can be overwritten if the file name is the same.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_config_restapi_applyOAuthCert(const char *name, char *keyfile, int overwrite);
#ifdef __cplusplus
}
#endif

#endif
