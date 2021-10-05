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

#ifndef __VALIDATECONFIG_DEFINED
#define __VALIDATECONFIG_DEFINED

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef XAPI
#define XAPI extern
#endif

#include <ismutil.h>
#include <ismrc.h>
#include <ismjson.h>
#include <admin.h>
#include <config.h>
#include <security.h>
#include <jansson.h>

/*
 * Validate data type Number.
 * Number can include a qualifier to indicate size, for example 20M means 20 Megabytes.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  min                    The minimum allowed number
 * @param  max                    The maximum allowed number
 * @param  sizeQualifierOptions   Comma-separated list of allowed size qualifiers
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Supported size qualifiers are:
 * k or K for KB
 * m or M for MB
 * g or G for GB
 *
 * If sizeQualifierOptions are not specified, but value has a qualifier, then
 * the function will validate the value for all supported qualifiers.
 *
 */
XAPI int32_t ism_config_validateDataType_number(
    char *name,
    char *value,
    char *min,
    char *max,
    char *sizeQualifierOptions);

/*
 * Validate data type Enum.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  options                Comma-separated list of allowed enum options
 * @param  type                   Either ISM_CONFIG_PROP_ENUM or ISM_CONFIG_PROP_LIST
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_enum(char *name, char *value, char *options, int type);

/*
 * Validate Data type boolean.
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_boolean(char *name, char *value);

/* Validate Data type string
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  type                   Type of the string
 *                                0 - Description
 *                                1 - Topic
 * @param  maxlen                 Maximum length of the string
 * @param  item                   Object type
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 */
XAPI int ism_config_validateDataType_string(
    char *name,
    char *value,
    int type,
    char *maxlen,
	char *item);

/*
 * Validate an configuration object name.
 *
 * @param  name                   Configuration object name
 * @param  value                  Object value to be validated
 * @param  maxlen                 Maximum length of the object name
 * @param  item                   Object type
 *
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Note:
 * There are several special cases for maxlen in 14a code:
 * QueueManagerName             48
 * ChannelName          20
 * UserInfoKey        1024
 * AuthKey            1024
 * ConnectionName     is a comma separated list of IP:port. will be validated using new validateDataType_IPAddress
 */
XAPI int32_t ism_config_validateDataType_name(char *name, char *value, char *maxlen, char *item);

/*
 * Validate a single IP address.
 *
 * @param  value                  An IP address to be validated.
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_IPAddress(char *value);

/*
 * Validate an IP addresses.
 *
 * @param  name                   Object name
 * @param  value                  A comma separated list of IP addresses to be validated
 * @param  mode                   1, validate IP address as an host, allow "*" and "all".
 *                                0, validate IP address as IP.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
XAPI int32_t ism_config_validateDataType_IPAddresses(char *name, char *value, int mode);

/*
 * Validate data type URL
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 * @param  options                The protocol options.
 * @param  item                   Object type
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * URL allows different protocols based upon the property name.
 */
XAPI int32_t ism_config_validateDataType_URL(
    char *name,
    char *value,
    char * maxlen,
    char *options,
	char *item );

/*
 * Validate data type Selector
 *
 * @param  name                   Property name
 * @param  value                  Property value to be validated
 * @param  maxlen                 Maximum length of the string
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * Empty string is valid
 */
XAPI int32_t ism_config_validateDataType_Selector(
    char *name,
    char *value,
    char *maxlen,
    char *item );

/*
 * Generic validation for each passing configuration item.
 * @param  list                   A complete list of defined items of a composite object defined in the JSON schema
 * @param  name                   A configuration item to be validated
 * @param  value                  The value of the configuration item specified by name
 * @param  dupName                The value of "Name" item to be returned.
 * @param  isGet                  The flag to be assigned if "Action" item is "get".
 * @param  isDeleted              The flag to be assigned if "Delete" item is validated.
 * @param  isUpdated              The flag to be assigned if "Update" item is validated.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * NOTE: PLEASE do not add any special cases inside this function.
 * This validation should be ONLY used for generic validating the item against its type,
 * and range defined in the schema. For example, "ConnectionName" is defined as String, this
 * function will only verify the string does not exceed maxlen. To validate each of the
 * IPAddess:port in the string, have special code piece inside valid-QueueManagerConnection API.
 */
XAPI int32_t ism_config_validate_checkItemDataType(
    ism_config_itemValidator_t *list,
    char *name,
    char *value,
    char **dupName,
    int *isGet,
    int action,
    ism_prop_t *props);

/*
 * Generic validation for each passing configuration item.
 * @param  list                   A complete list of defined items of a composite object defined in the JSON schema
 * @param  name                   A configuration item to be validated
 * @param  value                  The json_t value of the configuration item specified by name
 * @param  dupName                The value of "Name" item to be returned.
 * @param  isGet                  The flag to be assigned if "Action" item is "get".
 * @param  isDeleted              The flag to be assigned if "Delete" item is validated.
 * @param  isUpdated              The flag to be assigned if "Update" item is validated.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * NOTE: PLEASE do not add any special cases inside this function.
 * This validation should be ONLY used for generic validating the item against its type,
 * and range defined in the schema. For example, "ConnectionName" is defined as String, this
 * function will only verify the string does not exceed maxlen. To validate each of the
 * IPAddess:port in the string, have special code piece inside valid-QueueManagerConnection API.
 */
XAPI int32_t ism_config_validate_checkItemDataTypeJson(
    ism_config_itemValidator_t *list,
    char *name,
    json_t *value,
    char **dupName,
    int *isGet,
    int action,
    ism_prop_t *props);

/* All composite object validation function has the following signature
 * @param  currPostObj   JSON object received by REST API POST method
 * @param  validateObj   Pointer to JSON object in currePostObj, to be validated
 * @param  objType       Type of object e.g. MessageHub
 * @param  instName      Name of the instance
 * @param  action        Action - 0/1 - create/update
 * @param  props         Object configuration in props list
 *
 * Return ISMRC_OK on success or ISMRC_*
 */
typedef int32_t (*ismConfigValidate_t) (json_t *root, json_t *objRoot, char * objType, char * instName, int action, ism_prop_t *props);


/* Validate an item data based on schema definitions
 * @param  list      Pointer to item validator structure
 * @param  name      Name of the item
 * @param  value     Value of the configuration item
 * @param  action    Action - 0/1 - create/update
 * @param  props     Object configuration in props list
 *
 * Return ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_validateItemData(
    ism_config_itemValidator_t *list,
    char *name,
    char *value,
    int action,
    ism_prop_t *props);


/* Validate an item data passed as JSON object, based on schema definitions
 * @param  list      Pointer to item validator structure
 * @param  name      Name of the item
 * @param  value     JSON object with item configuration data
 *
 * Return ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_validateItemDataJson(
    ism_config_itemValidator_t *list,
    char *item,
    char *name,
    json_t *value);

/*
 * Component: HA
 * Item: HighAvailability
 *
 * Dependent validation rules:
 * --------------------------
 * None
 */
XAPI int32_t ism_config_validate_HighAvailability(json_t *currPostObj,
    json_t *mergedObj,
    char * item,
    char * name,
    int action,
    ism_prop_t *props);


/*
 * Component: Security
 * Item: QueuePolicy
 *
 * Dependent validation rules:
 * --------------------------
 * QueuePolicy can not be deleted if it is used by an Endpoint object.
 */
XAPI int32_t ism_config_validate_QueuePolicy(
		json_t *currPostObj,
		json_t *mergedObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Security
 * Item: SubscriptionPolicy
 *
 * Dependent validation rules:
 * --------------------------
 * SubscriptionPolicy can not be deleted if it is used by an Endpoint object.
 */
XAPI int32_t ism_config_validate_SubscriptionPolicy(
		json_t *currPostObj,
		json_t *mergedObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Security
 * Item: TopicPolicy
 *
 * Dependent validation rules:
 * --------------------------
 * TopicPolicy can not be deleted if it is used by an Endpoint object.
 */
XAPI int32_t ism_config_validate_TopicPolicy(
		json_t *currPostObj,
		json_t *mergedObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Engine
 * Item: Queue
 *
 * Dependent validation rules:
 * --------------------------
 * Queue can not be deleted if it is used by an Endpoint object.
 */
XAPI int32_t ism_config_validate_Queue(
		json_t *currPostObj,
		json_t *mergedObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Engine
 * Item: AdminSubscription
 */
XAPI int32_t ism_config_validate_AdminSubscription(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Engine
 * Item: DurableNamespaceAdminSub
 */
XAPI int32_t ism_config_validate_DurableNamespaceAdminSub(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Engine
 * Item: NonpersistentAdminSub
 */
XAPI int32_t ism_config_validate_NonpersistentAdminSub(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);
/*
 * Component: Server
 * Item: ClientSet
 *
 * Dependent validation rules:
 * --------------------------
 */
XAPI int32_t ism_config_validate_ClientSet(
    ism_json_parse_t *json,
    char *component,
    char *item,
    int actionType,
    char *inpbuf,
    int inpbuflen,
    ism_prop_t *props);

/*
 * Component: Server
 * Item: ClientSet
 * Action: DELETE
 */
XAPI int32_t ism_config_admin_deleteClientSet(ism_http_t *http, ism_rest_api_cb callback, const char *clientID, const char *retain, uint32_t waitsec);


/*
 * Component: Security
 * Item: ConfigurationPolicy
 */
XAPI int32_t ism_config_validate_ConfigurationPolicy(
    json_t *currPostObj,
    json_t *mergedObj,
    char * item,
    char * name,
    int action,
    ism_prop_t *props);

/*
 * Component: Transport
 * Item: Endpoint
 */
XAPI int32_t ism_config_validate_Endpoint(
    json_t *currPostObj,
    json_t *mergedObj,
    char * item,
    char * name,
    int action,
    ism_prop_t *props);

/*
 * Component: Transport
 * Item: AdminEndpoint
 */
int32_t ism_config_validate_AdminEndpoint(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Security
 * Item: ConnectionPolicy
 */
XAPI int32_t ism_config_validate_ConnectionPolicy(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Transport
 * Item: CertificateProfile
 *
 * Dependent validation rules:
 * --------------------------
 * None
 */
XAPI int32_t ism_config_validate_CertificateProfile(
	    json_t *currPostObj,
	    json_t *validateObj,
	    char *item,
	    char *name,
	    int action,
	    ism_prop_t *props);

/*
 * Generic validation of a Singleton item.
 * @param name                    The name of the item
 * @param value                   The value to validate
 * @param action                  The flag indicates the configuration string is 0 - create, 1 - update, 2 - delete.
 * @param newValue                If a default value is substituted, pointer will be placed here
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 */
XAPI int ism_config_validate_singletonItem(
	char *name,
	char *value,
	int action,
	char **newValue);


XAPI int ism_config_getValidationDataCompType(char *objName);

/*
 * Component: Transport
 * Item: MessageHub
 */
XAPI int32_t ism_config_validate_MessageHub(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Cluster
 * Item: ClusterMembership
 */
XAPI int32_t ism_config_validate_ClusterMembership(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Security
 * Item: LTPAProfile
 */
XAPI int32_t ism_config_validate_LTPAProfile(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);


/*
 * Component: Security
 * Item: OAuthProfile
 */
XAPI int32_t ism_config_validate_OAuthProfile(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Server, Store, Transport
 * Items: Singleton objects
 */
XAPI int32_t ism_config_validate_Singleton(
    json_t *currPostObj,
    json_t *objval,
    char * item);
    
/*
 * Component: Transport
 * Item: SecurityProfile
 */
XAPI int32_t ism_config_validate_SecurityProfile(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Transport
 * Item: TrustedCertificate
 */
XAPI int32_t ism_config_validate_TrustedCertificate(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Engine
 * Item: TopicMonitor
 */
XAPI int32_t ism_config_validate_TopicMonitor(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Engine
 * Item: ClusterRequestedTopics
 */
XAPI int32_t ism_config_validate_ClusterRequestedTopics(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Security
 * Item: LDAP
 */
XAPI int32_t ism_config_validate_LDAP(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Protocol
 * Item: Plugin
 */
XAPI int32_t ism_config_validate_Plugin(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: MQConnectivity
 * Item: QueueManagerConnection
 */
XAPI int32_t ism_config_validate_QueueManagerConnection(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: MQConnectivity
 * Item: DestinationMappingRule
 */
XAPI int32_t ism_config_validate_DestinationMappingRule(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);
    
/*
 * Component: Transport
 * Item: ClientCertificate
 */
XAPI int32_t ism_config_validate_ClientCertificate(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Server
 * Item: LogLocation
 */
XAPI int32_t ism_config_validate_LogLocation(
		json_t *currPostObj,
		json_t *validateObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Server
 * Item: Syslog
 */
XAPI int32_t ism_config_validate_Syslog(
		json_t *currPostObj,
		json_t *validateObj,
		char * item,
		char * name,
		int action,
		ism_prop_t *props);

/*
 * Component: Transport
 * Item: ClientCertificate
 */
XAPI int32_t ism_config_validate_ClientCertificate(
    json_t *currPostObj,
    json_t *validateObj,
    char *item,
    char *name,
    int action,
    ism_prop_t *props);

/*
 * Component: Transport
 * Item: CRLProfile
 */
XAPI int32_t ism_config_validate_CRLProfile(
	json_t * currPostObj,
	json_t * validateObj,
	char * objName,
	char * instName,
	int action,
	ism_prop_t * props);

/**
 * Process service request to commit, roll back, or forget a transaction
 */
XAPI int ism_config_json_parseServiceTransactionPayload(ism_http_t *http, int type, ism_rest_api_cb callback);

/*
 * Component: MQConnectivity
 * Item: MQCertificate
 */
XAPI int32_t ism_config_validate_MQCertificate(
		json_t * currPostObj,
		json_t * validateObj,
		char * item,
		char * name,
		int action,
		ism_prop_t * props);

/**
 * Component: Engine
 * Item: ResourceSetDescriptor
 */
XAPI int32_t ism_config_validate_ResourceSetDescriptor(
        json_t * currPostObj,
        json_t * validateObj,
        char * item,
        char * name,
        int action,
        ism_prop_t * props);

#ifdef __cplusplus
}
#endif

#endif

