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
#ifndef __VALIDATE_INTERNALS_DEFINED
#define __VALIDATE_INTERNALS_DEFINED

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <ismutil.h>
#include <ctype.h>
#include <ismjson.h>
#include <config.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/objects.h>
#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <validateConfigData.h>
#include "configInternal.h"
#include "adminInternal.h"

#define MAX_PATH_LEN	1024
/*
 * Returns the JSON schema
 *
 * @param  type     Type of schema
 *                  Supported types are:
 *                  ISM_VALIDATE_CONFIG_SCHEMA and
 *                  ISM_VALIDATE_MONITORING_SCHEMA
 *
 * Returns the JSON schema requested or returns NULL if failure.
 *
 */
XAPI ism_json_parse_t * ism_config_getSchema(int type);

/*
 * Returns list of required items of a composite object defined in the JSON schema
 *
 * @param  type     Type of schema
 *                  Supported types are:
 *                  ISM_VALIDATE_CONFIG_SCHEMA and
 *                  ISM_VALIDATE_MONITORING_SCHEMA
 * @param  name     Name of the conmposite object
 * @param  rc       return code to pass back.
 *
 * Returns ism_config_itemValidator list on success or returns NULL of failure.
 *
 * Caller should invoke ism_config_validate_freeRequiredItemList() to free memory.
 *
 */
XAPI ism_config_itemValidator_t * ism_config_validate_getRequiredItemList(int type, char *name, int *rc);


/*
 * Free ism_config_itemValidator created by ism_config_validate_getRequiredItemList() API
 *
 * @param  item     Pointer to ism_config_itemValidator returned by
 *                  ism_config_validate_getRequiredItemList() API.
 *
 */
XAPI void ism_config_validate_freeRequiredItemList(ism_config_itemValidator_t *item);


/*
 * Update assigned flag on required items in the required list.
 *
 * @param list      Pointer to ism_config_itemValidator returned by
 *                  ism_config_validate_getRequiredItemList() API.
 * @param name      The name of a configuration item.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 **/
XAPI int ism_config_validate_updateRequiredItemList(ism_config_itemValidator_t *list, char *name );

/*
 * Check all items in required item list has assigned flag marked.
 *
 * @param list      Pointer to ism_config_itemValidator returned by
 *                  ism_config_validate_getRequiredItemList() API.
 * @param chkmode   A flag indicate if the check is for create or update/delete
 * @param props     use existing item props if available
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 **/
XAPI int ism_config_validate_checkRequiredItemList(ism_config_itemValidator_t *list, int chkmode);

/*
 * Returns properties of a configuration item
 *
 * @param componet   The configuration component
 * @param item       The configuration item
 * @param name       The configuation object name
 * @param rc         Return value
 *
 * Returns props of the confuration item if item exist, or return NULL (check rc for returned
 *         error code).
 *
 */
XAPI int ism_config_validate_CheckItemExist(char *component, char *item, char *name );


XAPI int ism_config_validate_UTF8(const char * s, int len, int checkFirst, int isName);

/* Returns ISMRC_OK if the specified protocol has been found
 *         ISMRC_NotFound if the specified protocol cannot be found
 *         ISMRC_* for errors.
 */
XAPI int ism_config_validate_CheckProtocol(char *protocols, int isEndpoint, int capabilities );

/* Returns ISMRC_OK if the specified Substitution is OK
 *         ISMRC_* for errors.
 */
XAPI int ism_config_validate_PolicySubstitution(char *item, char *name, char *value);

/* Check * position
 * Only allow one * tailing other valid characters
 * Returns ISMRC_OK if the String meets the rule.
 * ISMRC_* for errors.
 */
XAPI int ism_config_validate_Asterisk(char *name, char *value);

/* Wrap function to get configuration properties specified by item and name
 * Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI ism_prop_t * ism_config_validate_getCurrentConfigProps(char *component, char *item, char *name, int *rc );

/* Returns ISMRC_OK if the specified object has been found
 *         ISMRC_NotFound if the specified object cannot be found
 *         ISMRC_* for errors.
 */
XAPI void * ism_config_validate_getCurrentConfigValue(char *component, char *item, char *name, int *rc );

/* Returns ISMRC_OK if the specified name can be stored into the list
 *         ISMRC_* for duplicate.
 */
XAPI int ism_config_addValueToList(ism_common_list * inList, char * name, int isParam);

/* Returns ISMRC_OK if the cert has an expiration date.
 *         ISMRC_* for errors.
 */
XAPI char * ism_config_getCertExpirationDate(const char * name, int *rc);

XAPI char * ism_config_validate_getAttr(char *name, ism_json_parse_t *json, int pos);

XAPI ism_prop_t * ism_config_getConfigProps(int comptype);

XAPI int ism_config_validate_IPAddress(char *ip, int checkLocal);

XAPI int32_t ismcli_validateTraceObject(int ignoreEmptyValue, const char *itemName, const char *value);

/* Encrypt/Decrypt Password */
XAPI char * ism_security_encryptAdminUserPasswd(const char * src);
XAPI char * ism_security_decryptAdminUserPasswd(const char * src);

/* Set global HA flag */
XAPI void ism_config_json_disableHACheck(int flag);


#ifdef __cplusplus
}
#endif

#endif

