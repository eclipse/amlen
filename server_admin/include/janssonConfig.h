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

/*
 * janssonConfig.h
 */

#ifndef JANSSONCONFIG_H_
#define JANSSONCONFIG_H_

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef XAPI
    #define XAPI extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <alloca.h>
#include <jansson.h>
#include <string.h>

#include <ismutil.h>
#include <config.h>
#include <validateConfigData.h>

#define JSON_CONFIG_FNAME      "server_dynamic.json"


/* jansson specifies following data types:
 * JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_INTEGER, JSON_REAL,
 * JSON_TRUE, JSON_FALSE, and JSON_NULL
 *
 * Though to identify boolean i.e. "true" or "false", there are
 * APIs available, but BOOLEAN is not available as a Data type.
 */
#define JSON_BOOLEAN_TYPE (JSON_NULL + 1 )


/* Schema item */
typedef struct {
    int                          compType;
    char                       * object;
    int                          objType;
    char                       * dispList;
    ism_config_itemValidator_t * cfgVal;
    json_t                     * objectSchema;
} schemaItem_t;


/* config schema items */
typedef struct {
    schemaItem_t ** items;
    int             id;
    int             count;
    int             nalloc;
    int             slots;
    int             noSingletons;
    int             noComposites;
} schemaItems_t;


/**
 * Returns JSON object from JSON string
 *
 * @argin  jsonString      JSON formatted string
 */
XAPI json_t * ism_config_json_strToObject(const char *jsonString, int *rc);


/**
 * Returns JSON object from JSON string in a file
 *
 * @argin  jsonString      JSON formatted string
 */
XAPI json_t * ism_config_json_fileToObject(const char *filename);


/**
 * Create and returns JSON object from HTTP payload
 *
 * @argin    http             ISM HTTP object
 * @argout   rc               ISMRC_* code
 * @argin    noErrorTrace     Do not write trace message on error
 *
 * Returns json object or NULL for errors
 *
 */
XAPI json_t * ism_config_json_createObjectFromPayload(ism_http_t *http, int *rc, int noErrorTrace);


/**
 * Returns pointer to the object or a copy of the object in persisted configuration.
 * Caller is responsible for managing configuration lock.
 * Object can be identified using one of the following criteria:
 *
 * 1. Object name, e.g. Endpoint - for all Endpoint
 * 2. Object name and instance name, e.g. Endpoint whose name is DemoEndpoint
 * 3. Object name, instance name and item name, e.g. Port of DemoEndpoint
 *
 * Also sets "type" (passed as an argument) to the type of the JSON object as specified in the
 * configuration schema, e.g.
 * - For Endpoint, type will be JSON_OBJECT
 * - For DemoEndpoint, type will be JSON_OBJECT
 * - For Port, type will be set to JSON_INTEGER
 *
 *
 * @argin  objName    Name of the object
 * @argin  instName   Name of the instance
 * @argin  itemName   Name of the item
 * @argin  mode       If mode is 1, it returns a copy, else pointer in
 *                    configuration structure.
 * @argout type       JSON type of the returned value
 *
 * Returns NULL if object is not found.
 *
 * To get value of a singleton object, set objName and set instName and itemName to NULL.
 *
 * Caller should free returned object by using json_decref() API if mode is 1.
 *
 */
json_t * ism_config_json_getObject(const char *objName, const char *instName, const char *itemName, int mode, int *type);

/**
 * Returns a copy of merged object.
 * Caller is responsible for managing configuration lock.
 * This functions will first get a copy of the object from the persisted configuration
 * data and then merged the data with the passed object. The values of the configuration
 * items in the passed object is retained in the merged object.
 *
 * Object can be identified using one of the following criteria:
 *
 * 1. Object name, e.g. Endpoint - for all Endpoint
 * 2. Object name and instance name, e.g. Endpoint whose name is DemoEndpoint
 * 3. Object name, instance name and item name, e.g. Port of DemoEndpoint
 *
 * Also sets "type" (passed as an argument) to the type of the JSON object as specified in the
 * configuration schema, e.g.
 * - For Endpoint, type will be JSON_OBJECT
 * - For DemoEndpoint, type will be JSON_OBJECT
 * - For Port, type will be set to JSON_INTEGER
 *
 *
 * @argin  objName    Name of the object
 * @argin  instName   Name of the instance
 * @argin  itemName   Name of the item
 * @argin  passedObj  JSON object sent in the current configuration request
 * @argout type       JSON type of the returned value
 *
 * Returns NULL if object is not found in persisted configuration or passed object.
 *
 * To get value of a singleton object, set objName.
 *
 * Caller should free returned object by using json_decref() API.
 *
 */
json_t * ism_config_json_getMergedObject(const char *objName, const char *instName, const char *itemName, json_t *passedObj, int *type);

/**
 * Merge current and persisted configuration array data and returns a copy or pointer.
 * Based on persisted configuration array, if an entry of the current array does not exist
 * in the persisted configuration array, it will be insert into merged array.
 * Caller should free json object using json_decref() API.
 */
XAPI json_t * ism_config_json_getMergedArray(const char *objName, json_t *passedObj, int *rc);

/**
 * Set JSON object
 * Caller is responsible for managing configuration lock.
 * Object can be identified using one of the following criteria:
 * 1. Object Type, e.g. Endpoint - for all Endpoint
 * 2. Object type and instance name, e.g. Endpoint whose name is DemoEndpoint
 * 3. Object type, instance name and item name, e.g. Port of DemoEndpoint
 *
 * @argin  objName   Name or Type of the object
 * @argin  instName  Name of the instance
 * @argin  itemName  Name of the item
 * @argin  type      JSON type of the object
 * @argin  newvalue  New value of the object
 *
 * Set "type" to the type of the JSON object as specified in the
 * configuration schema, e.g.
 * - For Endpoint, type will be JSON_OBJECT
 * - For DemoEndpoint, type will be JSON_OBJECT
 * - For Port, type will be set to JSON_INTEGER
 *
 *
 * Returns NULL if object is not found.
 *
 * This API can be used to set both Singleton and Composite objects.
 * For singleton, do not specify instName and itemName.
 *
 */
XAPI int ism_config_json_setObject(char *objName, char *instName, char *itemName, int type, void *newvalue);


/**
 * Set composite object in configuration
 * Caller is responsible for managing configuration lock.
 *
 * @arg   object    Object
 * @arg   name      Object name
 * @arg   objval    Object value
 *
 * Returns ISMRC_OK or ISMRC_*
 *
 */
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);


/**
 * Get composite object
 * Caller is responsible for managing configuration lock.
 *
 * @arg   object    Object
 * @arg   name      Object name
 * @arg   getLock   Get configuration lock
 *
 * Returns pointer to JSON object or NULL
 *
 */
XAPI json_t * ism_config_json_getComposite(const char *object, const char *name, int getLock);


/**
 * Get singleton object
 * Caller is responsible for managing configuration lock.
 *
 * @arg   object    Object
 *
 * Returns pointer to JSON object or NULL
 *
 */
XAPI json_t * ism_config_json_getSingleton(const char *object);


/**
 * Check if object exist in root configuration object
 * Caller is responsible for managing configuration lock.
 *
 * @argin  objectType      Type of the object e.g. Endpoint
 * @argin  objectName      Name of object. Do not specify objectName for singleton or
 *                         composite objects with fixed name such as AdminEndpoint,
 *                         LDAP, HighAvailability
 * @argin  currRequest     JSON object sent in the current configuration request. Note
 *                         that the current request may include multiple configuration
 *                         items. NOTE: do not send currRequest to check the configuration
 *                         item getting validated.
 *
 * Returns 1 if object exist else 0
 *
 */
XAPI int ism_config_objectExist(char *objectType, char *objectName, json_t *currRequest);


/**
 * Returns pointer to the object instance in specified json root
 *
 * @argin  root      JSON root node
 * @argin  objName   Name or Type of the object
 * @argin  instName  Name of the instance
 * @argin  itemName  Name of the item
 * @argout type      JSON type of the object
 *
 * Returns NULL if can not find the object.
 *
 */
XAPI json_t * ism_config_json_findObjectInRoot(json_t *root, const char *objName, const char *instName, const char *itemName, int *type);

/**
 * Remove spaces around items in a string list and update the jansson object if modified
 * @param  obj   The JSON object
 * @param  key   The item key
 * @param  value The JSON value which must be a modifyable copy
 * @return 1 if updated, 0 if not updated
 */

XAPI int ism_config_fixCommaList(json_t * obj, const char * key, char * value);

/**
 * Validates composite objects and sets ism_prop_t structure that can be sent to components for configuration
 *
 * @argin   root      root object
 * @argin   objVal    Object value in root object
 * @argin   objType   Object type
 * @argin   instName  Instance name
 * @argin   action    Action type - 0 for create, 1 for update
 * @argin   props     Return properties in ism_prop_t structure
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */
XAPI int ism_config_jsonValidateObject(json_t *root, json_t *objVal, char * objType, char * instName, int action, ism_prop_t *props );


/**
 * Updates configuration file in JSON format
 *
 * @argin   getLock    Get Configuration write lock
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */
XAPI int ism_config_json_updateFile(int getLock);

/**
 * Returns properties belonging to an existing an object
 *
 * @argin		objectName		Composite Object name
 * @argin		instName		Instance of object. For Singleton objects set this to NULL.
 * @argin		getLock			Take config lock
 *
 * Returns NULL is object is not configured.
 * Caller is responsible to free properties using ism_common_freeProperties() API
 */
XAPI ism_prop_t * ism_config_json_getObjectProperties(const char * objectName, const char * instName, int getLock);

/**
 * Returns properties belongs to a registered component in ism_prop_t structure
 *
 * @argin      compHandle      Handle of registered component
 * @argin      objectType      Type of the object e.g. Endpoint
 * @argin      objectName      Name of object.
 * @arginout   objectExist     Get set to 1 if object exists
 * @argin      mode            If set to 1, props key name will not be in object.item.name
 *                             format, if set to 2 only key returned is name. The key will be set to item.
 *
 * For singleton object, set objectName to NULL.
 * Returns list of properties in ism_prop_t structure
 * Returns NULL if object doesn't exist or doesn't belongs to the registered component.
 * Caller should free property list using ism_common_freeProperties() API.
 *
 */
XAPI ism_prop_t * ism_config_json_getProperties(ism_config_t *compHandle, const char *objectType, const char *objectName, int *objectExist, int mode);


/**
 * Read a configuration file in keyword=value pairs and update JSON configuration object
 *
 * @argin  filename      Filename containing configuration in key=value pairs
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */
XAPI int ism_config_convertPropsToJSON(const char * filename);


/**
 * Initialize schema object - configuration or monitoring schema
 *
 * @argin    schemaType    Type of schema - ISM_CONFIG_SCHEMA or ISM_MONITORING_SCHEMA
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */
XAPI int ism_config_initSchemaObject(int schemaType);


/**
 * Returns schema validator structure from cached data - used for configuration object validation
 *
 * @argin     schemaType    Schema Type - ISM_CONFIG_SCHEMA or ISM_MONITORING_SCHEMA
 * @argin     compType      Type of component
 * @argin     object        Type of object
 * @argout    rc            Return code
 *
 * Returns ism_confif_itemValidator_t or NULL
 *
 */
XAPI ism_config_itemValidator_t * ism_config_json_getSchemaValidator(int schemaType, int *compType, char *object, int *rc );


/**
 * Returns schema object
 *
 * @argin      object     Object name
 * @argin      item       Configuration item name
 * @argout     component  Component Type
 * @argout     objectType Data type of object
 * @argout     itemType   Data type of Item
 *
 * Returns json_t structure or NULL
 *
 */
XAPI json_t * ism_config_findSchemaObject(const char *object, const char *item, int *component, int *objectType, int *itemType);


/**
 * Add missing configuration items with default
 *
 * @argin     objectType     Type of object e.g. Endpoint
 * @arginout  object         JSON object
 * @argin     list           Schema validation list
 *
 * Returns ISMRC_OK or ISMRC_*
 */
XAPI int ism_config_addMissingDefaults(char *objectType, json_t *object, ism_config_itemValidator_t * list);


/**
 * Return JSON data type string
 *
 * @argin    type          JSON type
 *
 * Returns string representation of JSON type, e.g. for JSON_STRING, returns "string" *
 */
XAPI const char * ism_config_json_typeString(int type);

/**
 * Validates JSON object type
 *
 * @argin     value         Value of JSON to be validated
 * @argin     type          JSON object value should be of type
 * @argin     object        Object type
 * @argin     name          Name of the object
 * @argin     item          Name of the item
 *
 * Returns ISMRC_OK or ISMRC_*
 */
XAPI int ism_config_validate_jsonObjectType(json_t *value, int type, char *object, char *name, char *item);


/**
 * Create a JSON object from type and value
 *
 * @argin     type     Type of object, e.g. JSON_STRING
 * @argin     value    Value of the object
 *
 *  Returns JSON object or NULL
 */
XAPI json_t * ism_config_json_createObject(int type, void *value);

/**
 * Returns component type of an object
 *
 * @argin     object       Name of the object, e.g. Endpoint
 * @arginout  isComposite  Returns 1 if object is a composite object
 * @argout    configObj    If given, returns the pointer to the schema object
 *
 * Returns component type - as specified in ism_ConfigComponentType_t enum.
 */
XAPI ism_ConfigComponentType_t ism_config_findSchemaGetComponentType(const char *object, int *isComposite, json_t **configObj);


/**
 * Check if object being deleted is not used by other configuration items
 * Caller need to make sure that read lock on configuration is acquired before
 * invoking this function.
 *
 * @argin     inObject     Search in object
 * @argin     object       Object to be searched
 * @argin     name         Name of the object to be searched
 * @argin     checkList    Set to one if object can be a comma-separated list. For example
 *                         to search a TopicPolicy in TopicPolicies configuration
 *                         item in Endpoint object, set checkList to 1.
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_json_findObjectInUse(char *inObject, char *object, char * name, int checkList);


/**
 * Add properties to JSON configuration data from a string containing data in key=value pairs
 * as supported in v1.x release.
 *
 * @argin     propStr         JSON string with properties in key=value pairs, separated by comma.
 * @argin     getConfigLock   Takes configuration lock if set to 1.
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_convertV1PropsStringToJSONProps(char *propStr, int getConfigLock);


/**
 * Add update singleton configuration item in JSON configuration root node.
 * Make sure to get rwlock on srvConfiglock before call this function.
 *
 * @argin     item        Configuration item name
 * @argin     newvalue    New value in JSON object - retrieves type from schema
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_jsonAddUpdateSingletonProps(char *item, void *newvalue);

/**
 * Returns validation data index of an item from cache
 *
 * @argin     objName        Object name
 *
 * Returns index number or -1 on error
 */
XAPI int ism_config_getValidationDataIndex(char *objName);

/**
 * Convert Jansson properties to ism properties and call callback
 *
 * @argin     handle         Component handle
 * @argin     object         Object type.
 * @argin     name           Name of the object instance
 * @argin     haSync         Flag to indicate if HA sync required
 * @argin     cbtype         Flag to indicate update or delete
 * @argin     option         Flag to indicate something needs to be added to props, currently only for
 *                           Queue delete to indicate DiscardMessages=true needs to be added to props
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_json_callCallback( ism_config_t *handle, const char *object, const char *name, json_t * instObj, int haSync, ism_ConfigChangeType_t cbtype, int option);


/**
 * Set singleton object from a JSON string
 *
 * @argin     jsonStr        Object key-value pair in JASON string format
 * @argin     object         Object type.
 * @argin     validate       Set to 1 if object value validation is required
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_config_set_fromJSONStr(char *jsonStr, char *object, int validate);

XAPI int ism_config_json_addItemPropToList(int type, const char *objKey, json_t *elem, ism_prop_t *props);

XAPI int ism_config_getLTPAProfileKey(char *name, char ** key, int getLock);

XAPI int ism_config_json_findArrayInUse(char *inArray, char *object, char *name, int ignoreError);

XAPI int ism_config_getOAuthProfileKey(char *name, char ** key, int getLock);

#ifdef __cplusplus
}
#endif

#endif /* JANSSONCONFIG_H_ */
