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
 * janssonConfigProcessSchema.c
 */

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include <assert.h>

extern ism_ConfigPropType_t ism_config_getConfigPropertyType(const char *propName);
extern char * ism_admin_getSchemaJSONString(int type);
extern ism_ConfigComponentType_t ism_config_getComponentType(const char *compName);
extern char * ism_config_getCompString(int type);

json_t * serverConfigSchema = NULL;    /* Server schema JSON object     */
json_t * monitConfigSchema = NULL;     /* Monitoring schema JSON object */

static int configSchemaInited = 0;
static int monitoringSchemaInited = 0;

/*
 * ISM Configuration property type to JSON type map
 * NOTE: Array index should map to ism_ConfigPropType_t defined in config.h
 * typedef enum {
 *   ISM_CONFIG_PROP_INVALID      = 0,
 *   ISM_CONFIG_PROP_NUMBER       = 1,
 *   ISM_CONFIG_PROP_ENUM         = 2,
 *   ISM_CONFIG_PROP_STRING       = 3,
 *   ISM_CONFIG_PROP_NAME         = 4,
 *   ISM_CONFIG_PROP_BOOLEAN      = 5,
 *   ISM_CONFIG_PROP_IPADDRESS    = 6,
 *   ISM_CONFIG_PROP_URL          = 7,
 *   ISM_CONFIG_PROP_REGEX        = 8,
 *   ISM_CONFIG_PROP_BUFFERSIZE   = 9,
 *   ISM_CONFIG_PROP_LIST         = 10,
 *   ISM_CONFIG_PROP_SELECTOR     = 11,
 *   ISM_CONFIG_PROP_REGEX_SUBEXP = 12
 * } ism_ConfigPropType_t;
 */
const char *propMapTOJSONTypes[] = { "invalid", "integer", "string", "string", "string", "boolean", "string", "string", "string", "string", "string", "string", "string" };


schemaItems_t * cfgSchemaItems = NULL;
schemaItems_t * monSchemaItems = NULL;


/* Add schema item to the list */
static int ism_config_json_addSchemaItem(schemaItems_t * schemaItems, schemaItem_t * item, int type) {
    int i = 0;

    if (schemaItems->count == schemaItems->nalloc) {
        schemaItem_t ** tmp = NULL;
        int firstSlot = schemaItems->nalloc;
        schemaItems->nalloc = schemaItems->nalloc == 0 ? 32 : schemaItems->nalloc * 2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,259),schemaItems->items, sizeof(schemaItem_t *) * schemaItems->nalloc);
        if (tmp == NULL) {
            return ISMRC_AllocateError;
        }
        schemaItems->items = tmp;
        for (i = firstSlot; i < schemaItems->nalloc; i++)
            schemaItems->items[i] = NULL;
        schemaItems->slots = schemaItems->count;
    }
    if (schemaItems->count == schemaItems->slots) {
        schemaItems->items[schemaItems->count] = item;
        schemaItems->count++;
        schemaItems->slots++;
    } else {
        for (i = 0; i < schemaItems->slots; i++) {
            if (!schemaItems->items[i]) {
                schemaItems->items[i] = item;
                schemaItems->id = i;
                schemaItems->count++;
                break;
            }
        }
    }

    switch (type) {
    case ISM_SINGLETON_OBJTYPE:
        schemaItems->noSingletons++;
        break;
    case ISM_COMPOSITE_OBJTYPE:
        schemaItems->noComposites++;
        break;
    default:
        break;
    }

    return ISMRC_OK;
}

/**
 * Returns schema validator structure from cached data
 */
XAPI ism_config_itemValidator_t * ism_config_json_getSchemaValidator(int schemaType, int *compType, char *object, int *rc ) {
    ism_config_itemValidator_t * sv = NULL;
    int i = 0;

    if ( !object ) {
        *rc = ISMRC_NullPointer;
        return sv;
    }

    *rc = ISMRC_NotFound;

    for (i = 0; i < cfgSchemaItems->count; i++) {
        schemaItem_t *item = cfgSchemaItems->items[i];

        if ( !strcmp(object, item->object)) {
            sv = item->cfgVal;
            *compType = item->compType;
            *rc = ISMRC_OK;
            /* Need to clear out the fields from the last validation attempt. */
            for (int j=0; j < (sv->total); j++) {
            	sv->minonevalid[j] = 0;
            	sv->assigned[j] = 0;
            }
            break;
        }
    }

    return sv;
}



/* Returns json data value and type by name */
XAPI json_t * ism_config_getValueAndTypeByName(json_t *root, const char *item, const char *name, int *type) {
    const char *objkey;
    json_t *objval = NULL;
    json_t *retval = NULL;
    *type = JSON_NULL;

    if ( !root && !item ) {
        return NULL;
    }

    retval = json_object_get(root, item);
    if ( !retval )
        return NULL;

    if (!name) {
        *type = json_typeof(retval);
        return retval;
    }

    void *objiter = json_object_iter(retval);
    while(objiter) {
        objkey = json_object_iter_key(objiter);
        objval = json_object_iter_value(objiter);
        int objtyp = json_typeof(objval);

        if ( strcmp(objkey, name) == 0 ) {
            *type = objtyp;
            retval = objval;
            break;
        }

        objiter = json_object_iter_next(retval, objiter);
   }

   return retval;
}

/* Validates and returns value of string object */
static const char * getStringItemValue(json_t *root, const char *item) {
    int type;
    json_t * elem = ism_config_getValueAndTypeByName(root, item, NULL, &type);
    const char *retval = NULL;
    if ( elem && json_typeof(elem) == JSON_STRING ) {
        retval = (char *)json_string_value(elem);
    }
    return retval;
}

/* Initialize schema item */
static schemaItem_t * createSchemaItem(json_t *root, char *object, int objType, int compType) {


    schemaItem_t * si = (schemaItem_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,260),1,sizeof(schemaItem_t));

    si->object = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),object);
    si->compType = compType;
    si->objType = objType;
    si->cfgVal = (ism_config_itemValidator_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,261),1, sizeof(ism_config_itemValidator_t));
    si->objectSchema = json_object();

    return si;
}

/* Update configuration item validation structure */
static void updateItemValidator(json_t *root, char *itemName, int itemIndex, ism_config_itemValidator_t *cfgVal) {
    const char *tmpstr = NULL;
    const char *typestr = NULL;
    int type;

    /* get data type */
    typestr = getStringItemValue(root, "Type");
    type = ism_config_getConfigPropertyType(typestr);

    if ( type != ISM_CONFIG_PROP_INVALID ) {

        if ( itemName )
            cfgVal->name[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),itemName);

        cfgVal->type[itemIndex] = type;

        switch(type) {
            case ISM_CONFIG_PROP_NUMBER:
            case ISM_CONFIG_PROP_BUFFERSIZE:
            {
                tmpstr = getStringItemValue(root, "Minimum");
                if (tmpstr)
                    cfgVal->min[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                tmpstr = getStringItemValue(root, "Maximum");
                if (tmpstr)
                    cfgVal->max[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                break;
            }

            case ISM_CONFIG_PROP_ENUM:
            case ISM_CONFIG_PROP_LIST:
            {
                tmpstr = getStringItemValue(root, "Options");
                if (tmpstr)
                    cfgVal->options[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                break;
            }

            case ISM_CONFIG_PROP_STRING:
            case ISM_CONFIG_PROP_NAME:
            {
                tmpstr = getStringItemValue(root, "MaxLength");
                if (tmpstr)
                    cfgVal->max[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                break;
            }

            case ISM_CONFIG_PROP_IPADDRESS:
            {
                tmpstr = getStringItemValue(root, "Default");
                if (tmpstr) {
                  if (tmpstr && (!strcasecmp(tmpstr, "all") || (*tmpstr == '*'))){
                      cfgVal->tempflag[itemIndex] = 1;
                  }
                }

                break;
            }

            case ISM_CONFIG_PROP_URL:
            {
                tmpstr = getStringItemValue(root, "Options");
                if (tmpstr)
                    cfgVal->options[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                tmpstr = getStringItemValue(root, "MaxLength");
                if (tmpstr)
                    cfgVal->max[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);

                break;
            }

            default:
                break;

        }

        /* check RequiredField */
        tmpstr = getStringItemValue(root, "RequiredField");
        if ( tmpstr && !strcasecmp(tmpstr, "yes")) {
            cfgVal->reqd[itemIndex] = 1;
        }

        const char *defval = getStringItemValue(root, "Default");
        if ( type == ISM_CONFIG_PROP_STRING
        		|| type == ISM_CONFIG_PROP_NAME
        		|| type == ISM_CONFIG_PROP_URL
        		|| type == ISM_CONFIG_PROP_REGEX
        		|| type == ISM_CONFIG_PROP_REGEX_SUBEXP
				|| type == ISM_CONFIG_PROP_IPADDRESS
				|| type == ISM_CONFIG_PROP_SELECTOR) {
            if ( defval ) {
                cfgVal->defv[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),defval);
            } else {
                cfgVal->defv[itemIndex] = NULL;
            }
        } else {
            if ( defval && *defval != '\0' ) {
                cfgVal->defv[itemIndex] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),defval);
            } else {
                cfgVal->defv[itemIndex] = NULL;
            }
        }


        /* check MinOneOption */
        tmpstr = getStringItemValue(root, "MinOneOption");
        if ( tmpstr && !strcasecmp(tmpstr, "yes")) {
            cfgVal->minoneopt[itemIndex] = 1;
        }

        cfgVal->total = itemIndex + 1;
    }
}

/**
 * Initialize schema object - configuration or monitoring schema
 */
XAPI int ism_config_initSchemaObject(int schemaType) {
    int rc = ISMRC_OK;
    char *buf = NULL;

    // Make sure that the mapping between config property types has an entry for all.
    // If a new config property type is added, make sure we have a mapping to JSON Type and update
    // this assert.
    assert((sizeof(propMapTOJSONTypes)/sizeof(propMapTOJSONTypes[0]))-1 == ISM_CONFIG_PROP_REGEX_SUBEXP);

    if ( schemaType == ISM_CONFIG_SCHEMA ) {
        if ( configSchemaInited == 1 ) {
            return rc;
        }
        configSchemaInited = 1;
        buf = ism_admin_getSchemaJSONString(ISM_CONFIG_SCHEMA);
        serverConfigSchema = ism_config_json_strToObject(buf, &rc);
    } else if ( schemaType == ISM_MONITORING_SCHEMA ) {
        if ( monitoringSchemaInited == 1 ) {
            return rc;
        }
        monitoringSchemaInited = 1;
        buf = ism_admin_getSchemaJSONString(ISM_MONITORING_SCHEMA);
        monitConfigSchema = ism_config_json_strToObject(buf, &rc);
    } else {
        TRACE(3, "Invalid Schema type: %d\n", schemaType);
        return ISMRC_ArgNotValid;
    }
    ism_common_free(ism_memory_admin_misc,buf);

    cfgSchemaItems = (schemaItems_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,263),1, sizeof(schemaItems_t));

    /* Process schema */
    int          compType = ISM_CONFIG_COMP_LAST;
    int          objType = -1;
    int          noSingleton = 0;
    int          noComposite = 0;
    int          noTotal = 0;
    const char * objkey = NULL;
    json_t     * objRoot = NULL;
    const char * tmpstr;

    void *objiter = json_object_iter(serverConfigSchema);

    while(objiter) {
        int itemIndex = 0;
        compType = ISM_CONFIG_COMP_LAST;
        objType = -1;

        objkey = json_object_iter_key(objiter);
        objRoot = json_object_iter_value(objiter);

        /* Ignore if it is a JSONComment.... */
        if ( !strncmp("JSONComment", objkey, 11)) {
            objiter = json_object_iter_next(serverConfigSchema, objiter);
            continue;
        }

        tmpstr = getStringItemValue(objRoot, "ObjectType");
        if ( tmpstr ) {
            if ( *tmpstr == 'C' )
                objType = ISM_COMPOSITE_OBJTYPE;
            else if ( *tmpstr == 'S' )
                objType = ISM_SINGLETON_OBJTYPE;
        }

        tmpstr = getStringItemValue(objRoot, "Component");
        if ( tmpstr ) {
            compType = ism_config_getComponentType((char *)tmpstr);
        }

        if ( compType == ISM_CONFIG_COMP_LAST || objType == -1 ) {
            objiter = json_object_iter_next(serverConfigSchema, objiter);
            continue;
        }

        switch(objType) {

        case ISM_SINGLETON_OBJTYPE:
        {
            /* Create schema struct */
            schemaItem_t * si = createSchemaItem(objRoot, (char *)objkey, objType, compType);

            ism_config_itemValidator_t *cfgVal = si->cfgVal;
            updateItemValidator(objRoot, (char *)objkey, itemIndex, cfgVal);

            /* create JSON object of the item */
            /* Add "Version" item of type "string" */
            json_object_set(si->objectSchema, "Version", json_string("string"));
            /* Add singleton object name of type specified in config schema */
            json_object_set(si->objectSchema, (char *)objkey, json_string(propMapTOJSONTypes[cfgVal->type[0]]));

            TRACE(9, "SINGLETON: Object:%s  Component:%d\n", si->object, si->compType);
            ism_config_json_addSchemaItem(cfgSchemaItems, si, ISM_SINGLETON_OBJTYPE);

            noSingleton += 1;
            noTotal += 1;
            break;
        }

        case ISM_COMPOSITE_OBJTYPE:
        {
            /* iterate thru object elements */
            const char *itemkey = NULL;
            json_t *itemval = NULL;
            void *itemiter = json_object_iter(objRoot);
            schemaItem_t * si = createSchemaItem(itemval, (char *)objkey, objType, compType);
            ism_config_itemValidator_t *cfgVal = si->cfgVal;
            cfgVal->item = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),objkey);
            ism_config_json_addSchemaItem(cfgSchemaItems, si, ISM_COMPOSITE_OBJTYPE);

            /* create JSON object of the item */
            /* Add "Version" item of type "string" */
            json_object_set(si->objectSchema, "Version", json_string("string"));
            /* Add new json object to set config items */
            json_t *newarray = json_array();
            json_t *cobjs = json_object();
            json_array_append(newarray, cobjs);
            json_object_set(si->objectSchema, (char *)objkey, newarray);
            json_object_set(cobjs, "Name", json_string("string"));

            while(itemiter) {
                itemkey = json_object_iter_key(itemiter);
                itemval = json_object_iter_value(itemiter);
                int itemtyp = json_typeof(itemval);

                if ( itemtyp == JSON_OBJECT ) {

                    updateItemValidator(itemval, (char *)itemkey, itemIndex, cfgVal);

                    /* add item to schema object */
                    if ( strcasecmp(itemkey, "Update") && strcasecmp(itemkey, "Delete") && strcasecmp(itemkey, "Name") ) {
                        json_object_set(cobjs, itemkey, json_string(propMapTOJSONTypes[cfgVal->type[itemIndex]]));
                    }

                    TRACE(9, "COMPOSITE: Object:%s  Component:%d  Item:%s\n", si->object, si->compType, si->cfgVal->name[itemIndex]);
                    itemIndex += 1;
                }
                itemiter = json_object_iter_next(objRoot, itemiter);
            }

            // Do not remove this - may be used to generate schema of individual objects
            // char objJSONFile[256];
            // fprintf(objJSONFile, "object_schemas/%s.json", object);
            // json_dump_file(si->objectSchema, objJSONFile, JSON_INDENT(4)|JSON_PRESERVE_ORDER);

            noComposite += 1;
            noTotal += 1;
            break;
        }

        default:
            break;
        }

        objiter = json_object_iter_next(serverConfigSchema, objiter);
    }

    TRACE(4, "Configuration objects processed: Singleton:%d Composite:%d Total:%d\n", noSingleton, noComposite, noTotal);
    TRACE(4, "Configuration objects in Schema: Singleton:%d Composite:%d Total:%d\n", cfgSchemaItems->noSingletons, cfgSchemaItems->noComposites, cfgSchemaItems->count);

    return rc;
}


/**
 * Returns schema object
 */
XAPI json_t * ism_config_findSchemaObject(const char *object, const char *item, int *component, int *objtype, int *itemtype) {
    /* Process schema */
    json_t *retval = NULL;

    *component = ISM_CONFIG_COMP_LAST;

    if ( !object )
        return retval;

    /* find object in the schema */
    json_t *objRoot = json_object_get(serverConfigSchema, object);

    if (!objRoot) return retval;

    int jsontype = JSON_NULL;
    *objtype = 0;

    json_t * objTypeElem = ism_config_getValueAndTypeByName(objRoot, "ObjectType", NULL, &jsontype);
    char *tmpstr = (char *)json_string_value(objTypeElem);
    if ( *tmpstr == 'C' ) *objtype = 1;

    tmpstr = (char *)getStringItemValue(objRoot, "Component");
    if ( tmpstr ) {
    	*component = ism_config_getComponentType((char *)tmpstr);
        TRACE(8, "Process schema: objectType=%d component=%s compType=%d\n", *objtype, tmpstr, *component);
    }


    if ( item ) {
        /* iterate thru object elements to find instance of another object */
        const char *objkey = NULL;
        json_t *objval = NULL;
        void *objiter = json_object_iter(objRoot);

        while(objiter) {
            objkey = json_object_iter_key(objiter);
            objval = json_object_iter_value(objiter);
            int objtyp = json_typeof(objval);

            if ( objtyp == JSON_OBJECT ) {
                if ( !strcmp(objkey, item)) {
                    json_t * type = ism_config_getValueAndTypeByName(objval, "Type", NULL, &jsontype);
                    TRACE(8, "Item found: %s Type: %s\n", objkey, json_string_value(type));
                    retval = objval;
                    break;
                }
            }
            objiter = json_object_iter_next(objRoot, objiter);
        }
    } else {
        retval = objRoot;
    }

    return retval;

}

/**
 * Returns schema object
 */
XAPI ism_ConfigComponentType_t ism_config_findSchemaGetComponentType(const char *object, int *isComposite, json_t **schemaObj) {

    char *component = NULL;
    int jsontype;

    /* find object in the schema */
    json_t *objRoot = json_object_get(serverConfigSchema, object);

    if (schemaObj) *schemaObj = objRoot;
    if ( objRoot ) {
        json_t * compName = ism_config_getValueAndTypeByName(objRoot, "Component", NULL, &jsontype);
        component = (char *)json_string_value(compName);

        *isComposite = 0;
        json_t * objTypeElem = ism_config_getValueAndTypeByName(objRoot, "ObjectType", NULL, &jsontype);
        char *tmpstr = (char *)json_string_value(objTypeElem);
        if ( *tmpstr == 'C' ) *isComposite = 1;
    }

    ism_ConfigComponentType_t comptype = ism_config_getCompType(component);

    return comptype;

}

/**
 * Returns list of objects of a specified components
 */
XAPI json_t * ism_config_objectListOfComponent(int compType) {

    char *component = ism_config_getCompString(compType);
    if ( !component )
        return NULL;

    json_t *retval = json_array();

    json_t *objval = NULL;
    void *objiter = json_object_iter(serverConfigSchema);

    while(objiter) {
        objval = json_object_iter_value(objiter);
        int jsontype = 0;
        json_t * compName = ism_config_getValueAndTypeByName(objval, "Component", NULL, &jsontype);
        char *compstr = (char *)json_string_value(compName);

        if ( compstr && !strcmp(component, compstr)) {
            json_t *newobj = json_string(compstr);
            json_array_append(retval, newobj);
        }

        objiter = json_object_iter_next(serverConfigSchema, objiter);
    }

    return retval;
}

