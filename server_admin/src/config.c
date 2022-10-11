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

/*
 * This file contains ISM server configuration APIs.
 * The APIs handle both static and dynamic configuration options.
 */

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include <ismutil.h>
#include <sasl_scram.h>
#include "configInternal.h"
#include "validateInternal.h"



extern int32_t ism_ha_admin_set_standby_group(const char *group);
extern int32_t ism_config_validateDeleteOAuthProfile(char * name) ;
extern int32_t ism_config_deleteOAuthKeyFile(char * name);
extern int ism_config_readJSONConfig(char *filename);
extern int ism_config_json_deleteComposite(char *object, char *inst);
extern int ism_config_convertPropsToJSON(const char * filename);
extern void ism_config_json_init(void);
extern int ism_config_json_processObject(json_t *post, char *object, char *httpPath, int haSync, int validate, int restCall, concat_alloc_t *mqcBuffer, int *onlyMqcItems);
extern int ism_config_json_deleteObject(char *object, char *inst, char *subinst, int discardMessages, int standbyCheck, concat_alloc_t *mqcBuffer);
extern int ism_config_migrate_processV1ConfigViaHA(void);
extern ism_prop_t * ism_config_json_getObjectNames(void);
extern int ism_admin_dumpStatus(void);
extern void ism_security_1wayHashAdminUserPassword(const char * password, const char * salt, char * _hash);
extern char * ism_security_createAdminUserPasswordHash(const char * password);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
extern pthread_spinlock_t  configSpinLock;

/* Supported component types, names and property list */
struct compProps_t compProps[] = {
    { ISM_CONFIG_COMP_SERVER,           "Server",           NULL, 0 },
    { ISM_CONFIG_COMP_TRANSPORT,        "Transport",        NULL, 0 },
    { ISM_CONFIG_COMP_PROTOCOL,         "Protocol",         NULL, 0 },
    { ISM_CONFIG_COMP_ENGINE,           "Engine",           NULL, 0 },
    { ISM_CONFIG_COMP_STORE,            "Store",            NULL, 0 },
    { ISM_CONFIG_COMP_SECURITY,         "Security",         NULL, 0 },
    { ISM_CONFIG_COMP_ADMIN,            "Admin",            NULL, 0 },
    { ISM_CONFIG_COMP_MONITORING,       "Monitoring",       NULL, 0 },
    { ISM_CONFIG_COMP_MQCONNECTIVITY,   "MQConnectivity",   NULL, 0 },
    { ISM_CONFIG_COMP_HA,               "HA",               NULL, 0 },
    { ISM_CONFIG_COMP_CLUSTER,          "Cluster",          NULL, 0 },
};


char *adminUser = NULL;
char *adminUserPassword = NULL;

static ismConfigHandles_t *handles = NULL;
static ism_prop_t *ism_s_config_props = NULL;
static int compused = 0;
const char *configDir = NULL;
static int configInited = 0;
static const char *dynFname = NULL;
pthread_mutex_t g_cfgfilelock;
int disableSet = 0;
static char dynCfgFile[1024];
static int fipsEnabled = 0;
static int mqconnEnabled = 0;
static int sslUseBuffersPool = 0;
extern pthread_key_t adminlocalekey;
static int dissconClientNotifInterval = 0;
static int isStandby = 0;
pthread_mutex_t g_cfglock;
int configLocksInited = 0;

static ism_prop_t * ism_config_getPropertiesInternal(ism_config_t * handle, const char * object, const char * name, int mode, int *doesObjExist, int oldConfig, int objectNamesOnly);
static int32_t ism_config_updateFile(const char * fileName, int proctype);
int32_t ism_config_setObjectUID(ism_prop_t * props, char *item, char *name, char *puid, int32_t pType, char **retId);
int ism_config_getUpdateCertsFlag(char *item);

static int ism_config_getJSONString(json_t *objRoot, char *item, char *name, concat_alloc_t *retStr, int getConfigLock, int deleteFlag);
static int ism_config_json_setGlobalConfigVariables(void);



/*
 * Put one character to a concat buf
 */
static void ism_config_bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}


/* get component type */
XAPI int ism_config_getCompType(const char *compname) {
    int i;
    if ( !compname )
        return ISM_CONFIG_COMP_LAST;

    for (i=0; i<ISM_CONFIG_COMP_LAST; i++) {
        if ( strncasecmp(compname, compProps[i].name, strlen(compname)) == 0 ) {
            return compProps[i].type;
        }
    }
    return ISM_CONFIG_COMP_LAST;
}

/* get component string */
XAPI char * ism_config_getCompString(int type) {
    if ( type < 0 || type >= ISM_CONFIG_COMP_LAST )
        return NULL;

    return compProps[type].name;
}

/* get property list of a component */
XAPI ism_prop_t * ism_config_getConfigProps(int comptype) {
    if ( comptype < 0 || comptype >= ISM_CONFIG_COMP_LAST)
        return NULL;

    ism_prop_t *props = compProps[comptype].props;
    return props;
}

/* Add component registration handles to the list */
static int32_t ism_config_addConfigHandle(ism_config_t * handle) {
    int   i;
    if (handles->count == handles->nalloc) {
        ism_config_t ** tmp = NULL;
        int firstSlot = handles->nalloc;
        handles->nalloc = handles->nalloc == 0? 64 : handles->nalloc*2;
        tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_admin_misc,306),handles->handle, sizeof(ism_config_t *) * handles->nalloc);
        if (tmp == NULL) {
            ism_common_setError(ISMRC_AllocateError);
            return ISMRC_AllocateError;
        }
        handles->handle = tmp;
        for ( i=firstSlot; i<handles->nalloc; i++ )
            handles->handle[i] = NULL;
        handles->slots = handles->count;
    }
    if (handles->count == handles->slots) {
        handles->handle[handles->count] = handle;
        handles->id = handles->count;
        handles->count++;
        handles->slots++;
    } else {
        for (i=0; i<handles->slots; i++) {
            if (!handles->handle[i]) {
                handles->handle[i] = handle;
                handles->id = i;
                handles->count++;
                break;
            }
        }
    }
    return ISMRC_OK;
}

/* returns 1 if object has an associated certificate */
int ism_config_getUpdateCertsFlag(char *item) {
    int rc = 0;
    if ( !item )
        return rc;

    if ( !strcmpi(item, "LDAP") ||
         !strcmpi(item, "CertificateProfile") ||
         !strcmpi(item, "SecurityProfile") ||
         !strcmpi(item, "LTPAProfile") ||
         !strcmpi(item, "OAuthProfile") ||
         !strcmpi(item, "TrustedCertificate") ||
         !strcmpi(item, "ClientCertificate") ||
         !strcmpi(item, "MQCertificate") ||
         !strcmpi(item, "PreSharedKey") ||
         !strcmpi(item, "CRLProfile"))
    {
        rc = 1;
    }
    return rc;
}

/**
 * Read and process a configuration file as keyword=value
 *
 * keyword - compname.item.subitem
 * (subitem is optional - used to specify composite objects)
 */
static int ism_config_readDynamicConfigFile(const char * name, int useCompType, int proctype, int standby) {
    FILE * f;
    size_t len = 0;
    int    rc;
    char * line = NULL;
    char * keyword;
    char * value;
    char * more;
    char * compname;
    char * p;
    ism_field_t var = {0};

    /* check arguments */
    if ( !name ) {
        ism_common_setError(ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    /* open and process configuration file */
    TRACE(5, "Process dynamic configuration file: %s\n", name);

    f = fopen(name, "rb");
    if (!f) {
        TRACE(5, "Dynamic configuration file is not found.\n");
        ism_common_setError(ISMRC_NotFound);
        return ISMRC_NotFound;
    }

    rc = getline(&line, &len, f);
    while (rc >= 0) {
        keyword = ism_common_getToken(line, " \t\r\n", "=\r\n", &more);
        if (keyword && keyword[0]!='*' && keyword[0]!='#') {
            char * cp = keyword+strlen(keyword); /* trim trailing white space */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);
            if (!value)
                value = "";
            var.type = VT_String;
            var.val.s = value;
            ism_common_canonicalName(keyword);

            compname = p = keyword;
            while(*p) {
                if (*p == '.')  {
                    *p = 0;
                    p++;
                    keyword = p;
                    break;
                }
                p++;
            }

            if (!keyword) {
                TRACE(3, "NULL keyword in dynamic configuration file.\n");
            } else {
                int comptype = ism_config_getCompType(compname);
                /* on standby node MQC config is managed by imaserver.
                 * Do not read in server config data from mqc dynamic config file
                 */ 
                if ( comptype == ISM_CONFIG_COMP_SERVER && standby == 1 ) {
                    TRACE(9, "Ignore server config in mqcbridge configuration file\n");
                } else {
                  if ( useCompType < 0 || useCompType == comptype ) {
                    ism_prop_t *props = compProps[comptype].props;
                    if (!props) {
                        TRACE(3, "Invalid component: %s\n", compname);
                    } else {
                        ism_common_setProperty(props, keyword, &var);
                    }
                  }
                }
            }
        }
        rc = getline(&line, &len, f);
    }

    fclose(f);

    if (line)
        ism_common_free_raw(ism_memory_admin_misc,line);

    if ( standby == 0 ) {
        /* get SecurityLog and set auditControlLog */
        ism_prop_t *srvprops = compProps[ISM_CONFIG_COMP_SERVER].props;
        ism_field_t fl;
        ism_common_getProperty(srvprops, "SecurityLog", &fl);
        if ( fl.type == VT_String ) {
            int newval = AuxLogSetting_Normal - 1;
            char *tmpval = fl.val.s;
            if ( tmpval ) {
                if ( !strcmpi(tmpval, "MIN"))
                    newval = AuxLogSetting_Min -1;
                else if ( !strcmpi(tmpval, "NORMAL"))
                    newval = AuxLogSetting_Normal -1;
                else if ( !strcmpi(tmpval, "MAX"))
                    newval = AuxLogSetting_Max -1;
            }
            ism_security_setAuditControlLog(newval);
        }

        /*Get SSL FIPS configuration*/    
        ism_prop_t *transProps = compProps[ISM_CONFIG_COMP_TRANSPORT].props;
        fipsEnabled = ism_common_getBooleanProperty(transProps, "FIPS", 0);
        mqconnEnabled = ism_common_getBooleanProperty(transProps, "MQConnectivityEnabled", 0);
        sslUseBuffersPool = ism_common_getIntProperty(ism_common_getConfigProperties(), "SslUseBuffersPool", 0);
    }

    return ISMRC_OK;
}

// TODO: Only necessary until Nir adds registration for Cluster
ism_config_callback_t dummyCallBackCluster(char                    *object,
		char                    *name,
		ism_prop_t              *props,
		ism_ConfigChangeType_t  flag ) {
	return ISMRC_OK;
}

/*
 * Create JSON string of config objects
 */
XAPI int32_t ism_config_getObjectConfig(char *component, char *item, char *name, char *idField, int type, concat_alloc_t *retStr, int getConfigLock, int deleteFlag) {
    int rc = ISMRC_OK;
    int i;

    if ( getConfigLock == 1 )
        pthread_mutex_lock(&g_cfglock);

    int compType = ism_config_getCompType(component);

    ism_config_t *handle = ism_config_getHandle(compType, NULL);
    if ( !handle ) {
        if ( !handle ) {
            pthread_mutex_unlock(&g_cfglock);
            ism_config_register(compType, NULL, NULL, &handle);
            pthread_mutex_lock(&g_cfglock);
            if (!handle) {
                ism_common_setError(ISMRC_ObjectNotValid);
                pthread_mutex_unlock(&g_cfglock);
                return ISMRC_ObjectNotValid;
            }
        }
    }

    ism_prop_t *props = NULL;
    int freeProps = 0;
    if ( idField == NULL ) {
    	int doesObjExist = 0;
        props = ism_config_getPropertiesInternal(handle, item, name, 1, &doesObjExist, 0, 0);
        if ( !props ) {
            ism_common_setError(ISMRC_PropertyNotFound);
            pthread_mutex_unlock(&g_cfglock);
            return ISMRC_PropertyNotFound;
        }
        freeProps = 1;
    } else {
        props = compProps[compType].props;
    }

    const char * propName;
    int propComposite = -1;
    uint64_t nrec = compProps[compType].nrec;
    char ** namePtrs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,308),nrec * sizeof(char *));
    int ntot = 0;

    for (i=0; i<nrec; i++)
        namePtrs[i] = NULL;

    /* if name is not specified, then check for composite object */
    if (!name) {
        ism_common_getPropertyIndex(props, 1, &propName);
        char *ntoken = (char *)propName;
        if ( ntoken && *ntoken != '\0' ) {
            while (*ntoken) {
                if ( *ntoken == '.' )
                    propComposite += 1;
                if (propComposite == 1)
                    break;
                ntoken++;
            }
        }

        if (propComposite == 1) {
            /* get list of names */

            for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
                const char *value = ism_common_getStringProperty(props, propName);

                /* do not return value that is not set */
                if (!value || *value == '\0')
                    continue;

                if ( idField ) {
                	char *p = strstr((char *)propName, idField);
                    if ( p ) {
                    	p = p + strlen(idField) + 1;
                        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),p);
                        ntot += 1;
                    }
                } else {
                    if ( strstr((char *)propName, ".Name.")) {
                        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                        ntot += 1;
                    }
                }
            }
        } else {
            ntot += 1;
        }
    } else {
        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        ntot += 1;
    }

    int itemLoop = 0;
    int found = 0;
    char *lastname = NULL;
    int addComma = 0;
    int cycle = 0;

    for (itemLoop = 0; itemLoop < ntot; itemLoop++) {
        int addStartTag = 1;
        if (ntot >= 1) {
            /* get property of specific object */
            char *iName = namePtrs[itemLoop];
            if ( freeProps ) {
                ism_common_freeProperties(props);
            }
            props = ism_config_getPropertiesDynamic(handle, item, iName);
        }

        if (props) {
            int eol = 0;

            ism_json_putBytes(retStr, "{ \"Component\":\"");
            ism_json_putEscapeBytes(retStr, component, (int) strlen(component));
            ism_config_bputchar(retStr, '"');
            ism_config_bputchar(retStr, ',');

            ism_json_putBytes(retStr, "\"Item\":\"");
            ism_json_putEscapeBytes(retStr, item, (int) strlen(item));
            ism_config_bputchar(retStr, '"');
            ism_config_bputchar(retStr, ',');

            if ( type == 1 ) {
                ism_json_putBytes(retStr, "\"Type\":\"Composite\"");
                ism_config_bputchar(retStr, ',');
            }

            if ( idField ) {
                ism_json_putBytes(retStr, "\"ObjectIdField\":\"");
                ism_json_putEscapeBytes(retStr, idField, (int) strlen(idField));
                ism_config_bputchar(retStr, '"');
                ism_config_bputchar(retStr, ',');
            } else {
                ism_json_putBytes(retStr, "\"Update\":\"True\"");
                ism_config_bputchar(retStr, ',');
            }

            if ( deleteFlag == 1 ) {
                ism_json_putBytes(retStr, "\"Delete\":\"True\"");
                ism_config_bputchar(retStr, ',');
            }

            for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
                const char *value = ism_common_getStringProperty(props, propName);

                /* do not return value that is not set */
                if (!value || *value == '\0')
                     continue;

                /* seams like a composite object  - get key */
                int nameLen = strlen(propName) + 1;
                char *tmpstr = alloca(nameLen);
                memcpy(tmpstr, propName, nameLen);
                tmpstr[nameLen-1] = 0;
                char *nexttoken = NULL;
                char *p1 = strtok_r(tmpstr, ".", &nexttoken);
                char *p2 = strtok_r(NULL, ".", &nexttoken);
                char *p3 = strtok_r(NULL, ".", &nexttoken);

                /* Add composite object items */
                if (p3 && p2 && p1) {
                    nameLen = strlen(p1) + strlen(p2) + 2;
                    char *nameStr = (char *)propName + nameLen;

                    if ( addStartTag == 1 ) {
                        if ( idField && *idField ) {
                            ism_json_putBytes(retStr, "\"UID\":\"");
                        } else {
                            ism_json_putBytes(retStr, "\"Name\":\"");
                        }
                        ism_json_putEscapeBytes(retStr, nameStr, (int) strlen(nameStr));
                        ism_config_bputchar(retStr, '"');
                        addStartTag = 0;
                    }

                    if (strcasecmp(p2, "Name") && strcasecmp(p2, "UID")) {
                           ism_json_putBytes(retStr, ",\"");
                           ism_json_putEscapeBytes(retStr, p2, (int)strlen(p2));
                           ism_json_putBytes(retStr, "\":\"");
                           ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
                           ism_config_bputchar(retStr, '"');
                    }
                    if (idField == NULL && !strcasecmp(p2, "UID")) {
                    	ism_json_putBytes(retStr, ",\"");
					    ism_json_putEscapeBytes(retStr, p2, (int)strlen(p2));
					    ism_json_putBytes(retStr, "\":\"");
					    ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
					    ism_config_bputchar(retStr, '"');
                    }
                    found = 1;
                    continue;
                } else if (!p3 && p2 && p1) {
                    if (cycle == 0 && itemLoop == 0) {
                        cycle = 1;
                    }

                    if (addComma == 1 && eol == 0) {
                        ism_config_bputchar(retStr, ',');
                        addComma = 0;
                    }

                    ism_json_putBytes(retStr, "\"Value\":\"");
                    ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
                    ism_config_bputchar(retStr, '"');
                    addComma = 1;
                    found = 1;
                } else if (!p3 && !p2 && p1) {
                    if (cycle == 0 && itemLoop == 0) {
                        cycle = 1;
                    }

                    if (addComma == 1 && eol == 0) {
                        ism_config_bputchar(retStr, ',');
                        addComma = 0;
                    }

                    ism_json_putBytes(retStr, "\"Value\":\"");
                    ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
                    ism_config_bputchar(retStr, '"');
                    addComma = 1;
                    found = 1;
                }
            }
            if (found == 1) {
            	//put version info
            	ism_config_bputchar(retStr, ',');
            	ism_json_putBytes(retStr, "\"Version\":\"2.0\"");
                ism_json_putBytes(retStr, " } \n");
            }
        }
    }

    if (found == 0) {
        TRACE(7, "Object is not configured yet: Component:%s  Type:%s\n", component, item);
    }

    if ( freeProps ) {
        ism_common_freeProperties(props);
    }

    for (i=0; i<nrec; i++) {
        if ( namePtrs[i] )
            ism_common_free(ism_memory_admin_misc,namePtrs[i]);
    }

    ism_common_free(ism_memory_admin_misc,namePtrs);

    if (lastname)
        ism_common_free(ism_memory_admin_misc,lastname);

    if (rc)
        ism_common_setError(rc);

    if ( getConfigLock == 1 )
        pthread_mutex_unlock(&g_cfglock);

    return rc;
}


XAPI void ism_config_init_locks(void) {
    if ( configLocksInited == 0 ) {
        configLocksInited = 1;
        /* Initialize config locks */
        pthread_mutex_init(&g_cfglock, NULL);
        pthread_mutex_init(&g_cfgfilelock, NULL);
        pthread_mutex_lock(&g_cfglock);
    }
    return;
}

/*
 * This function initializes global configuration list for all supported
 * components.
 */
XAPI uint32_t ism_config_init(void) {
    int i = 0;
    uint32_t rc = ISMRC_OK;

    if ( configInited == 1 ) {
        TRACE(5, "Configuration is already initialized.\n");
        return rc;
    }

    int pType = ism_admin_getServerProcType();

    TRACE(5, "Initialize configuration service: ProcType:%d\n", pType);
    ism_config_init_locks();

    /* Set static config to ism_g_config_props */
    ism_s_config_props = ism_common_getConfigProperties();

    /* get locations of configuration and policy files */
    ism_field_t f;
    configDir = ism_common_getStringProperty(ism_s_config_props, "ConfigDir");
    if ( configDir == NULL ) {
        f.type = VT_String;
        f.val.s = DEFAULT_CONFIGDIR;
        ism_common_setProperty(ism_s_config_props, "ConfigDir", &f);
        configDir = DEFAULT_CONFIGDIR;
    }

    /* Initialize component property list */
    for (i=0; i<ISM_CONFIG_COMP_LAST; i++) {
        compProps[i].props = ism_common_newProperties(20);
        compProps[i].nrec = 512;
        compused += 1;
    }

    /* initialize config rwlock */
    pthread_rwlockattr_t cfgrwlockattr_init;
    pthread_rwlockattr_init(&cfgrwlockattr_init);
    pthread_rwlockattr_setkind_np(&cfgrwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    handles = (ismConfigHandles_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,312),1, sizeof(ismConfigHandles_t));

    if ( pType == ISM_PROTYPE_SERVER ) {
        /* Initialize dynamic configuration properties list and lock */
        dynFname = ism_common_getStringProperty(ism_s_config_props, "DynamicConfigFile");
        if ( dynFname == NULL ) {
            dynFname = DEFAULT_DYNAMIC_CFGNAME;
        }

        /* Read dynamic configuration file (if exist) and add data to component configuration structure */
        snprintf(dynCfgFile, sizeof(dynCfgFile), "%s/%s", configDir, dynFname);

        int rc1 = ISMRC_OK;

        /* Read JSON config file */
        char cfgFile[2048];
        snprintf(cfgFile, sizeof(cfgFile), "%s/%s", configDir, "server_dynamic.json");
        rc1 = ism_config_readJSONConfig(cfgFile);
        if ( rc1 != ISMRC_OK ) {
            TRACE(5, "Failed to read JSON dynamic configuration data: rc=%d\n", rc1);
        }

        ism_config_json_setGlobalConfigVariables();
    }

    configInited = 1;
    pthread_mutex_unlock(&g_cfglock);

    return rc;
}


/*
 * Disable dynamic set
 */
XAPI uint32_t ism_config_disableSet(void) {
    disableSet = 1;
    return ISMRC_OK;
}

/*
 * Enable dynamic set
 */
XAPI uint32_t ism_config_enableSet(void) {
    disableSet = 0;
    return ISMRC_OK;
}

/*
 * Terminate configuration service
 */
XAPI uint32_t ism_config_term(void) {
    configInited = 0;
    return ISMRC_OK;
}

/**
 * Get configuration handle
 */
XAPI ism_config_t * ism_config_getHandle(ism_ConfigComponentType_t comptype, const char *object) {
    ism_config_t *rethandle = NULL;

    if (comptype < 0 || comptype >= ISM_CONFIG_COMP_LAST ) {
        TRACE(2, "Invalid component specified.\n");
        return rethandle;
    }

    if (!handles) {
    	TRACE(2, "Handle array has not been initialized.\n");
    	return rethandle;
    }

    int found = 0, i = 0;
    for (i=0; i<handles->count; i++) {
        ism_config_t *handle = handles->handle[i];
        if ( handle && handle->comptype == comptype ) {
            if ( ( !object && handle->objectname == NULL ) ||
                 ( object && handle->objectname && !strncasecmp(handle->objectname, object, strlen(object))) ) {
                 rethandle = handle;
                found = 1;
                break;
            }
        }
    }
    if ( !found ) {
        if ( object ) {
            TRACE(7, "Component %s (object=%s) is not registered\n", compProps[comptype].name, object);
        } else {
            TRACE(7, "Component %s is not registered\n", compProps[comptype].name);
        }
    }
    return rethandle;
}


/**
 * Get configuration callback
 */
XAPI ism_config_callback_t ism_config_getCallback(ism_ConfigComponentType_t comptype) {
    ism_config_callback_t callback = NULL;

    if (comptype < 0 || comptype >= ISM_CONFIG_COMP_LAST ) {
        TRACE(2, "Invalid component specified.\n");
        return callback;
    }

    int found = 0, i = 0;
    for (i=0; i<handles->count; i++) {
        ism_config_t *handle = handles->handle[i];
        if ( handle && handle->comptype == comptype ) {
            callback = handle->callback;
            found=1;
            break;
        }
    }
    if ( !found ) {
        TRACE(7, "NULL callback, Component %s is not registered\n", compProps[comptype].name);
    }
    return callback;
}

/**
 * Register component
 */
XAPI int32_t ism_config_register(ism_ConfigComponentType_t comptype, const char *object, ism_config_callback_t callback, ism_config_t ** handle) {
    ism_config_t * hdl;
    int rc = ISMRC_OK;
    *handle = NULL;

    if (comptype < 0 || comptype >= ISM_CONFIG_COMP_LAST ) {
        TRACE(2, "Invalid component specified.\n");
        ism_common_setError(ISMRC_ObjectNotValid);
        return ISMRC_ObjectNotValid;
    }

    if ( configInited == 0 ) {
        ism_config_init();
    }

    pthread_mutex_lock(&g_cfglock);

    /* search for handle */
    hdl = ism_config_getHandle(comptype, object);
    if ( hdl ) {
        if ( hdl->callback == NULL || hdl->callback == callback ) {
            hdl->refcount += 1;
            *handle = hdl;
            /* add callback if callback is not set yet */
            if ( hdl->callback == NULL && callback ) {
                hdl->callback = callback;
            }
            TRACE(6, "Component %s is already registered. refcount=%d\n", compProps[comptype].name, hdl->refcount);
            pthread_mutex_unlock(&g_cfglock);
            return ISMRC_OK;
        } else {
            TRACE(3, "Can not re-register with a different callback.\n");
            pthread_mutex_unlock(&g_cfglock);
            ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "callback");
            return ISMRC_ArgNotValid;
        }
    }

    /*
     * Components can register one callback function to receive configuration changes
     * for all objects, by passing NULL for "object". For example, "Transport"
     * component can register one callback to receive changes for all objects like
     * "Endpoint", "websockets" etc.
     *
     * OR
     *
     * Components can register multiple callback functions to receive object specific
     * configuration changes. For example, transport component can register one callback
     * to receive changes for "Endpoint" object and different callback to receive
     * changes for "Websockets" object.
     */

    /* check for invalid registration options */
    if ( object && ism_config_getHandle(comptype, NULL) ) {
        TRACE(2, "Object specific registration is not allowed when component has already registered without object.\n");
        pthread_mutex_unlock(&g_cfglock);
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", object?object:"NULL");
        return ISMRC_ArgNotValid;
    }

    /* If object is NULL, and object specific registration handle exists, return error */
    int i = 0;
    if ( !object ) {
        for (i=0; i<handles->count; i++) {
            ism_config_t *thandle = handles->handle[i];
            if ( thandle && thandle->comptype == comptype ) {
                if ( thandle->objectname ) {
                    TRACE(2, "Object can not be NULL, when component has already registered with a object.\n");
                    pthread_mutex_unlock(&g_cfglock);
                    ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "comptype");
                    return ISMRC_ArgNotValid;
                }
            }
        }
    }

    /* create new handle if not found */
    hdl = (ism_config_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,313),1, sizeof(ism_config_t));
    hdl->comptype = comptype;
    if ( object )
        hdl->objectname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),object);
    if ( callback )
        hdl->callback = callback;
    hdl->refcount += 1;

    /* Add handle in the list */
    if ( (rc = ism_config_addConfigHandle(hdl)) != ISMRC_OK ) {
        ism_common_free(ism_memory_admin_misc,hdl);
        TRACE(5, "Component %s registration failed. rc=%d\n", compProps[comptype].name, rc);
    } else {
        /* return handle */
        TRACE(5, "Component %s is registered. refcount=%d\n", compProps[comptype].name, hdl->refcount);
        *handle = hdl;
    }
    pthread_mutex_unlock(&g_cfglock);
    if (rc)
        ism_common_setError(rc);
    return rc;
}


/**
 * UnRegister component
 */
XAPI int32_t ism_config_unregister(ism_config_t * handle) {
    if ( handle == NULL ) {
        TRACE(2, "ism_config_unregister: cfg handle is NULL");
        ism_common_setError(ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    pthread_mutex_lock(&g_cfglock);
    ism_config_t *hdl = ism_config_getHandle(handle->comptype, handle->objectname);
    if ( !hdl ) {
        TRACE(3, "Component %s is not registered.\n", compProps[handle->comptype].name);
        pthread_mutex_unlock(&g_cfglock);
        ism_common_setError(ISMRC_Failure);
        return ISMRC_Failure;
    }

    hdl->refcount -= 1;

    /* TODO: remove from list
     * For now, just zero out the data in the handle
     */
    if (hdl->refcount == 0) {
        ism_common_free(ism_memory_admin_misc,(char *)hdl->objectname);
        hdl->objectname = NULL;
    	hdl->callback = NULL;
    }

    pthread_mutex_unlock(&g_cfglock);

    TRACE(5, "Component %s is unregistered. refcount=%d\n", compProps[handle->comptype].name, handle->refcount);
    return ISMRC_OK;
}

int ism_config_purgeCompProp(char *compname, int force) {
    int rc = ISMRC_OK;
    if ( !compname )
        return rc;

    int useCompType = ism_config_getCompType(compname);
    ism_prop_t *props = compProps[useCompType].props;

    int currentCount = ism_common_getPropertyCount(props);
    int modval = (currentCount/200 + 0.5);
    int used = currentCount - (modval * 200);
    int pType = ism_admin_getServerProcType();

    if ( force == 1 || used >= 190 ) {
        /* Purge component property data */
        TRACE(4, "Purge configuration of %s\n", compname);
        ism_common_freeProperties(props);
        compProps[useCompType].props = ism_common_newProperties(256);
        /* read from properties file */
        rc = ism_config_readDynamicConfigFile(dynCfgFile, useCompType, pType, 0);
        if ( rc != ISMRC_OK) {
            TRACE(5, "Failed to read dynamic configuration data: %d\n", rc);
        }
    }
    return rc;
}

/**
 * Internal implementation of get configuration properties
 */
static ism_prop_t * ism_config_getPropertiesInternal(ism_config_t * handle, const char * object, const char * name, int mode, int *doesObjExist, int oldConfig, int objectNamesOnly){
    int i = 0;
    int done = 0;
    int found = 0;
    const char * propName;
    char * otype = NULL;
    char * oname = NULL;
    ism_prop_t * retprops = NULL;
    ism_prop_t * props = NULL;

    if (!handle ) {
        TRACE(2, "NULL component configuration handle\n");
        return NULL;
    }

    if (object) {
        otype = alloca(strlen(object)+2);
        sprintf(otype, "%s.", object);
        if (name) {
            oname = alloca(strlen(name)+2);
            sprintf(oname, ".%s", name);
        }
    }

    /* Call API to get config from JSON struct */
    /* TODO: Enable when affected cunit tests are fixed */
    *doesObjExist = 0;
    retprops = ism_config_json_getProperties(handle, object, name, doesObjExist, objectNamesOnly ? 2 : 0);

    if ( *doesObjExist == 0 || !retprops ) {
        if ( retprops ) ism_common_freeProperties(retprops);
        props = compProps[handle->comptype].props;
        retprops = ism_common_newProperties(256);

        /* Get configuration data from dynamic configuration list */
        for (i=0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
            int addProp = 0;
            /* if object and name is NULL return all configuration items of the component */
            if ( !object && !name ) {
                addProp = 1;
            } else if ( object && !name ) {
                /* return property of a configuration object */
                int proplen = strlen(propName);
                char * tPropName = alloca(proplen +1);
                memcpy(tPropName, propName, proplen);
                tPropName[proplen]='\0';
                char * propObjName = ism_common_getToken(tPropName, " \t\r\n", ".", NULL);
                if (propObjName &&   strcasecmp(object, propObjName)==0 ) {
                    addProp = 1;
                    *doesObjExist +=1;
                }
            } else if ( object && name ) {
                /* return configuration of a named object */
                if (propName != NULL) {
                    int tmplen = strlen(propName) + 1;
                    char * tmpstr = alloca(tmplen);
                    memcpy(tmpstr, propName, tmplen - 1);
                    tmpstr[tmplen - 1] = 0;
                    char *nexttoken = NULL;
                    char *p1 = strtok_r((char *)tmpstr, ".", &nexttoken);
                    char *p2 = strtok_r(NULL, ".", &nexttoken);
                    char *p3 = strtok_r(NULL, ".", &nexttoken);
                    if ( p1 && p2 && p3 ) {

                        int nameLen = strlen(p1) + strlen(p2) + 2;
                        char *nameStr = (char *)propName + nameLen;

                        if (!strcmp(p1, object) && !strcmp(nameStr, name)) {
                            addProp = 1;
                            *doesObjExist = 1;
                        }
                    }
                }
            } else if ( !object && name ) {
                /* return configuration of a single configuration item */
                if ( !strncasecmp(propName, name, strlen(name))) {
                    addProp = 1;
                    done = 1;
                }
            } else {
                TRACE(2, "Property is not found: object=%s name=%s\n",
                    object?object:"NULL", name?name:"NULL");
            }
            /* Add property to the return list */
            if ( addProp ) {
                ism_field_t val;
                ism_common_getProperty(props, propName, &val);
                ism_common_setProperty( retprops, propName, &val);
                found = 1;
            }
            if ( done )
                break;
        }

    } else {
        *doesObjExist = 1;
    }

    if ( mode == 1 )
        return retprops;

    /* check static configuration list */
    done = 0;
    for (i=0; ism_common_getPropertyIndex(ism_s_config_props, i, &propName) == 0; i++) {
        int addProp = 0;
        if (objectNamesOnly && strstr(propName, ".Name.") == NULL) continue;
        /* if object and name is NULL return all configuration items of the component */
        if ( !object && !name ) {
            ism_field_t f;
            if (ism_common_getProperty(retprops, propName, &f))
                addProp = 1;
        } else if ( object && !name ) {
            /* return property of a configuration object */
            if ( !strncasecmp(propName, otype, strlen(otype)) ) {
                addProp = 1;
            }
        } else if ( object && name ) {
            /* return configuration of a named object */
            if ( !strncasecmp(propName, otype, strlen(otype)) && strstr(propName, oname)) {
                addProp = 1;
                if ( found == 1 ) {
                       TRACE(9, "Static configuration object %s (name=%s) ignored. Item exists in dynamic configuration list.\n", object, name);
                    addProp = 0;
                }
                *doesObjExist = 1;
            }
        } else if ( !object && name ) {
            /* return configuration of a single configuration item */
            if ( !strncasecmp(propName, name, strlen(name))) {
                addProp = 1;
                done = 1;
                if ( found == 1 ) {
                       TRACE(9, "Static configuration object %s (name=%s) ignored. Item exists in dynamic configuration list.\n", object, name);
                       addProp = 0;
                }
            }
        }

        /* Add property to the return list */
        if ( addProp ) {
            ism_field_t val;
            ism_common_getProperty(ism_s_config_props, propName, &val);
            ism_common_setProperty( retprops, propName, &val);
        }
        if ( done )
            break;
    }

    return retprops;
}

/**
 * Get static and dynamic properties of a configuration objects in a component
 */
XAPI ism_prop_t * ism_config_getProperties(ism_config_t * handle, const char * object, const char * name){
    int mode = 0;
    int doesObjExist = 0;
    return (ism_config_getPropertiesInternal(handle, object, name, mode, &doesObjExist, 0, 0));
}

/**
 * Get instance names of a specific component configuration object type
 */
XAPI ism_prop_t * ism_config_getObjectInstanceNames(ism_config_t * handle, const char * object) {
    int mode = 0;
    int doesObjExist = 0;
    return (ism_config_getPropertiesInternal(handle, object, NULL, mode, &doesObjExist, 0, 1));
}

/**
 * Get dynamic properties of a configuration objects in a component
 */
XAPI ism_prop_t * ism_config_getPropertiesDynamic(ism_config_t * handle, const char * object, const char * name){
    int mode = 1;
    int doesObjExist = 0;
    return (ism_config_getPropertiesInternal(handle, object, name, mode, &doesObjExist, 0, 0));
}

/**
 * Get dynamic properties of a configuration objects in a component.
 * return object exist flag
 */
XAPI ism_prop_t * ism_config_getPropertiesExist(ism_config_t * handle, const char * object, const char * name, int *doesObjExist){
    int mode = 1;
    return (ism_config_getPropertiesInternal(handle, object, name, mode, doesObjExist, 0, 0));
}

/* Check if HA is enabled */
static int32_t ism_config_isGroupUpdateAllowed(const char *group) {
    int isUpdateAllowed = 2; /* update config */

    /* check current server state - if it is in Maintenence mode - allow Group change */
    int state = ism_admin_get_server_state();
    if ( state == ISM_SERVER_MAINTENANCE ) return isUpdateAllowed;

    /* check HA config and role to determine is Group can be updated */
    ism_config_t * hahandle = ism_config_getHandle(ISM_CONFIG_COMP_HA, NULL);
    if ( hahandle == NULL ) {
        TRACE(3, "Could not get HA configuration handle\n");
        isUpdateAllowed = 0; /* update not allowed */
    } else {
        ism_prop_t *props = ism_config_getProperties(hahandle, NULL, NULL);
        if ( props ) {
            /* get current HA role */
            ismHA_View_t haView = {0};
            ism_ha_store_get_view(&haView);

            /* Compare current group with set group */
            const char *groupCFG = ism_common_getStringProperty(props, "HighAvailability.Group.haconfig");
            if ( groupCFG && strcmp(groupCFG, group)) {
                if ( haView.NewRole == ISM_HA_ROLE_PRIMARY && haView.SyncNodesCount == 2 ) {
                    isUpdateAllowed = 1; /* update config and sync */
                } else {
                    /* check EnableHA */
                    const char *retStr = ism_common_getStringProperty(props, "HighAvailability.EnableHA.haconfig");
                    if ( retStr && (*retStr == 't' || *retStr == 'T') ) {
                        if ( haView.NewRole == ISM_HA_ROLE_DISABLED ) {
                               isUpdateAllowed = 2; /* update config */
                        } else {
                            isUpdateAllowed = 0; /* update not allowed */
                        }
                    }
                }
            }

            TRACE(7, "Check Group Update: setGroup:%s confGroup:%s role:%d syncnodes:%d allowed:%d\n",
                    group?group:"", groupCFG?groupCFG:"", haView.NewRole, haView.SyncNodesCount, isUpdateAllowed);
            ism_common_freeProperties(props);
        }
    }
    return isUpdateAllowed;
}

static int isInteger(char *tmpstr) {
    if ( !tmpstr )
        return 0;
    while (*tmpstr) {
        if(!isdigit((int)*tmpstr))
            return 0;
        tmpstr++;
    }
    return 1;
}

/* Set dynamic config from JSON object */
static int32_t ism_config_set_dynamic_internal(ism_json_parse_t *json, int validateData, char *inpbuf, int inpbuflen, int sFlag, int noSend, int storeOnStandby) {
    int rc = ISMRC_OK;
    int i;
    int count =  json->ent[0].count;
    int found = 0;
    /*
     * If the name of the object is 256 UTF-8 characters, then the size of the propName
     * must be at least 256*4*2=2K plus the item name and separators. Will resize, if needed
     */
    int propNameLen = 2048;
    char *propName = alloca(propNameLen);
    int mode = ISM_CONFIG_CHANGE_PROPS;
    int composite = 0;
    int createObject = 1;
    char *objectID = NULL;
    int persistData = 1;
    int isUpdate = 0;
    char *cert = NULL;
    char *key = NULL;
    int purgeCompProp = 0;
    ismConfigVersion_t *pversion = NULL;
    int setAdminMode = 0;
    int isAdminUser = 0;
    int isAdminPasswd = 0;
    char *adminUserName = NULL;
    char *adminUserPWD = NULL;

    TRACE(9, "set dynamic internal: validateData=%d, inpbuf=%s, inpbuflen=%d, sFlag=%d, noSend=%d, storeOnStandby=%d\n",
    		validateData, inpbuf?inpbuf:"null", inpbuflen, sFlag, noSend, storeOnStandby);
    pthread_mutex_lock(&g_cfglock);

    /* check if component is valid and get its handle */
    char *item = (char *)ism_json_getString(json, "Item");
    char *component = NULL;

	ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);
	if (rc != ISMRC_OK) {
		TRACE(3, "Invalid or NULL component: comp=%s \n", component?component:"");
		ism_common_setError(ISMRC_InvalidComponent);
		pthread_mutex_unlock(&g_cfglock);
		ism_common_free(ism_memory_admin_misc,component);
		return ISMRC_InvalidComponent;
	}

    int comptype = ism_config_getCompType(component);
    if ( comptype == ISM_CONFIG_COMP_STORE ) {
         /* STORE objects are not sent on callback - these are internal
          * objects updated unsing advanced-pd-options
          */
        validateData = 0;
    }

    if ( validateData == 1 ) {
        rc = ism_config_validate_set_data(json, &mode);
        if ( rc != ISMRC_OK ) {
            ism_common_setError(rc);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return rc;
        }
    }

    /* get some basic data from set string */

    char *forceStr = (char *)ism_json_getString(json, "StandbyForce");
    int frc = 0;
    if ( forceStr && *forceStr == 'T' ) frc = 1;

    if (!strcmpi(item, "AdminMode")) setAdminMode = 1;

    int isHAConfig = 0;
    int sendHAGroupUpdateToStandby = 0;
    if ( frc == 0 && !strcmpi(component, "HA") && !strcmpi(item, "HighAvailability") ) {
        isHAConfig = 1;
        const char *group = ism_json_getString(json, "Group");
        if ( group && *group != '\0' ) {
            /* get HA config + role */
            sendHAGroupUpdateToStandby = ism_config_isGroupUpdateAllowed(group);
        }

        if ( sendHAGroupUpdateToStandby == 1 ) {
            rc = ism_ha_admin_set_standby_group(group);
            if ( rc != ISMRC_OK ) {
                ism_common_setError(rc);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
        }
    }

    if ( disableSet == 1 && isHAConfig == 0 ) {
        TRACE(2, "Node is running in standby mode. Dynamic set is disabled\n");
        rc = ISMRC_ConfigNotAllowed;
        ism_common_setError(rc);
        pthread_mutex_unlock(&g_cfglock);
        ism_common_free(ism_memory_admin_misc,component);
        return rc;
    }
    
    if(!strcmp(item, "FIPS")){
         validateData=0;
         if (NULL == ism_json_getString(json, "Value")) {
        	 TRACE(9, "No value sent for FIPS update.");
        	 ism_common_setError(ISMRC_SingltonDeleteError);
        	 rc = ISMRC_SingltonDeleteError;
             pthread_mutex_unlock(&g_cfglock);
             ism_common_free(ism_memory_admin_misc,component);
        	 return rc;
         }
    }

    if(!strcmp(item, "ClusterMembership")){
         validateData=0;
    }

    char *oldname = (char *)ism_json_getString(json, "Name");
    char *type = (char *)ism_json_getString(json, "Type");
    char *delete = (char *)ism_json_getString(json, "Delete");
    char *update = (char *)ism_json_getString(json, "Update");
    char *name = oldname;
    char *ObjectIdField = (char *)ism_json_getString(json, "ObjectIdField");
    char *uidStr   = (char *)ism_json_getString(json, "UID");
    char *resultOnPrimary = (char *)ism_json_getString(json, "ResultOnPrimary");

    int hrc = ISMRC_OK;

    ismHA_Role_t harole = ism_admin_getHArole(NULL, &hrc);
    if (hrc == ISMRC_OK)  {
        TRACE(9, "HA role is: %d\n", harole);
    } else {
    	TRACE(3, "Get HArole return error code: %d\n", hrc);
    }

    if (harole == ISM_HA_ROLE_STANDBY) {
        /* get primary version*/
        pversion = alloca(sizeof(ismConfigVersion_t));
        char *versionStr = (char *)ism_json_getString(json, "Version");
        ism_config_convertVersionStr(versionStr, &pversion);

        if (pversion->version == 1 && pversion->release == 1) {
        	TRACE(5, "The request is from a version 1.1 primary\n");

        }
    }

    if (harole == ISM_HA_ROLE_PRIMARY || harole == ISM_HA_ROLE_DISABLED) {
    	isStandby = 0;
    }

    /* flag for result on primary*/
    int deletedOnPrimary = 0;
    int pendingOnPrimary = 0;

#if 0
    /* check for new name and delete options */
    if ( newname && *newname != '\0' ) {
        mode = ISM_CONFIG_CHANGE_NAME;
        name = newname;
    }

    if ( mode == ISM_CONFIG_CHANGE_NAME ) {
        pthread_mutex_unlock(&g_cfglock);
        ism_common_setError(ISMRC_NotImplemented);
        ism_common_free(ism_memory_admin_misc,component);
        return ISMRC_NotImplemented;
    }
#endif

    if ( type && strncasecmp(type,"composite",9) == 0)
        composite = 1;

    ism_config_t * handle = ism_config_getHandle(comptype, NULL);
    if ( !handle && validateData == 0 ) {
        pthread_mutex_unlock(&g_cfglock);
        ism_config_register(comptype, NULL, NULL, &handle);
        pthread_mutex_lock(&g_cfglock);
        if ( !handle ) {
            /* Invalid component - return error */
            ism_common_setError(ISMRC_InvalidComponent);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return ISMRC_InvalidComponent;
        }
    } else if ( !handle )  {
        /* try for object specific handle */
        handle = ism_config_getHandle(comptype, item);
        if ( !handle ) {
            if ( comptype == ISM_CONFIG_COMP_STORE) {
                pthread_mutex_unlock(&g_cfglock);
                ism_config_register(comptype, NULL, NULL, &handle);
                pthread_mutex_lock(&g_cfglock);
            }
            if ( !handle ) {
                /* Invalid component - return error */
                ism_common_setError(ISMRC_InvalidComponent);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return ISMRC_InvalidComponent;
            }
        }
    }

    int objectFound = 0;
    int doesObjExist = 0;
    if ( name && *name != '\0' ) {
        ism_prop_t *p = ism_config_getPropertiesInternal(handle, item, name, 1, &doesObjExist, 0, 0);
        if ( p ) {
            objectFound = 1;
            ism_common_freeProperties(p);
        }
        /* doesObjExist will be checked ONLY is item and name are available*/
        if ( doesObjExist == 0 && delete && strncasecmp(delete, "true", 4) == 0 ) {
            ism_common_setError(ISMRC_NotFound);
            pthread_mutex_unlock(&g_cfglock);
            TRACE(9, "%s: item: %s name: %s does not exist.\n", __FUNCTION__, item, name);
            ism_common_free(ism_memory_admin_misc,component);
            return ISMRC_NotFound;
        }
    }

    /* setup flags for special cases - Subscription */
    int isSubscription = 0;
    char *subName = NULL;
    if (!strcmpi(item, "Subscription")) {
        isSubscription = 1;
        persistData = 0;
    }

    /* setup flags for special cases - MQTTClient */
    int isMQTTClient = 0;
    if (!strcmpi(item, "MQTTClient")) {
        isMQTTClient = 1;
        persistData = 0;
    }

    /* Process object index for a composite item - if set */
    /* Get component property list */
    ism_prop_t *cprops = compProps[comptype].props;

    const char * propertyName = NULL;
    if ( composite == 1 && ObjectIdField && *ObjectIdField != '\0' ) {
        int proplen;
        int itemLen = strlen(item);
        int idLen = strlen(ObjectIdField);
        int len = itemLen + idLen + 2;
        char *keyName = (char *)alloca(len + 2);
        sprintf(keyName, "%s.%s.", item, ObjectIdField);
        char *ObjectIdFieldValue = NULL;

        /* Get passed indexStr value */
        for (i=0; i<=count; i++) {
            ism_json_entry_t * jent = json->ent + i;
            if ( jent->name == NULL )
                continue;
            if ( strcasecmp(jent->name,ObjectIdField) == 0 ) {
                ObjectIdFieldValue = (char *)jent->value;
                break;
            }
        }

        /* check if indexStr is specified in the the input */
        if ( ObjectIdFieldValue == NULL ) {
            ism_common_setError(ISMRC_Error);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return ISMRC_Error;
        }

        /* Loop thru the property and find if indexStr exist */
        for (i=0; ism_common_getPropertyIndex(cprops, i, &propertyName) == 0; i++) {
            proplen = strlen(propertyName);
            if (proplen >= len && !memcmp(propertyName, keyName, len)) {
                char *pval = (char *)ism_common_getStringProperty(cprops, propertyName);
                if ( pval && strcmp(ObjectIdFieldValue, pval) == 0) {
                    /* Get object index from propety name */
                    char *tmpIndexStr = (char *)propertyName + len;
                    objectID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpIndexStr);
                    objectFound = 1;
                } 
            }
        }

        if ( objectFound ) {
            /* check if uidStr is set in query */
            if ( uidStr && strcmpi(uidStr, "autoGenerate")) {
                /* index is specified - match entry */
                if ( strcmpi(objectID, uidStr)) {
                    if ( sFlag == 1 ) {
                        /* This is a Standby node, change uidStr with the one from Primary */
                        TRACE(5, "uidStr mismatch on Standby: objectID=%s uidStr=%s\n",
                                        objectID?objectID:"", uidStr?uidStr:"");
                        ism_common_free(ism_memory_admin_misc,objectID);
                        objectID = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),uidStr);
                    } else {
                        TRACE(5, "uidStr mismatch: objectID=%s uidStr=%s\n",
                            objectID?objectID:"", uidStr?uidStr:"");
                        pthread_mutex_unlock(&g_cfglock);
                        ism_common_setError(ISMRC_Error);
                        ism_common_free(ism_memory_admin_misc,component);
                        return ISMRC_Error;
                    }
                }
            }
        }
    }


    /* Sanity check
     * Create option is allowed only if object doesn't exist.
     * Modify and delete is allowed if object exist.
     */
    if ( update && strncasecmp(update, "true", 4) == 0 ) {
        createObject = 0;
        isUpdate = 1;
    }

    if ( delete && strncasecmp(delete, "true", 4) == 0 ) {
        if (composite == 0) {
            TRACE(8, "Can not delete a singleton item: %s - %s.", item, name);
            ism_common_setErrorData(ISMRC_SingltonDeleteError, "%s", item);
            if ( objectID )
                ism_common_free(ism_memory_admin_misc,objectID);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return ISMRC_SingltonDeleteError;
        }
        mode = ISM_CONFIG_CHANGE_DELETE;
        createObject = 0;

    }

    if ( resultOnPrimary ) {
		if ( *resultOnPrimary == 'd' ) {   			//delete
			deletedOnPrimary = 1;
			TRACE(9, "set deletedOnPrimary to %d\n", deletedOnPrimary);
		} else if ( *resultOnPrimary == 'p' ) {     //pending
			pendingOnPrimary = 1;
			TRACE(9, "set pendingOnPrimary to %d\n", pendingOnPrimary);
		}
    }

    /* parse thru the received data and create a property list to be sent to
     * the component using callback function.
     */
    ism_prop_t *props = ism_common_newProperties(24);

    /*
     * create needs to generate UID
     * update is used by HA. need to get UID
     * delete can skip this part
     */
    if ( composite == 1 && !strcmpi(item, "TopicMonitor")){
    	int uidrc = ISMRC_OK;
    	int32_t pType = ism_admin_getServerProcType();
    	if (createObject == 1) {
			if (uidStr == NULL || !strcasecmp(uidStr, "autoGenerate")) {
				uidrc = ism_config_setObjectUID( props, item, name, NULL, pType, &objectID);
				if (uidrc != ISMRC_OK) {
					TRACE(2, "Failed to set UID for %s.%s .\n", item, name);
					if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
					ism_common_setError(uidrc);
					ism_common_freeProperties(props);
					pthread_mutex_unlock(&g_cfglock);
					ism_common_free(ism_memory_admin_misc,component);
					return uidrc;
				}
				uidStr = objectID;
			} else {
				uidrc = ism_config_setObjectUID( props, item, name, uidStr, pType, &objectID);
				if (uidrc != ISMRC_OK) {
					TRACE(2, "Failed to set %s.%s UID to %s.\n", item, name, uidStr );
					if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
					ism_common_setError(uidrc);
					ism_common_freeProperties(props);
					pthread_mutex_unlock(&g_cfglock);
					ism_common_free(ism_memory_admin_misc,component);
					return uidrc;
				}
			}
    	}
    }

    /* Loop thru passed data and set props to be passed to callback */
    int objIDset = 0;
    for (i=0; i<=count; i++) {
        ism_json_entry_t * jent = json->ent + i;

        if ( jent->name == NULL )
            continue;

        if ( strcasecmp(jent->name,"Action") == 0 ||
            strcasecmp(jent->name, "Component") == 0  ||
            strcasecmp(jent->name, "User") == 0  ||
            strcasecmp(jent->name, "Item") == 0  ||
            strcasecmp(jent->name, "Delete") == 0  ||
            strcasecmp(jent->name, "Update") == 0  ||
            strcasecmp(jent->name, "NewName") == 0  ||
            strcasecmp(jent->name, "Index") == 0  ||
            strcasecmp(jent->name, "UID") == 0  ||
            strcasecmp(jent->name, "ObjectIdField") == 0  ||
            strcasecmp(jent->name, "Version") == 0  ||
            strcasecmp(jent->name, "Type") == 0 ) {
                continue;
        }

        if (composite == 0) {
            ism_field_t f;
            char *value = (char *)ism_json_getString(json, "Value");
            if ( !value ) {
                TRACE(8, "Null value received for property: %s\n", propName);
                break;
            }

//            if (!*value) {
            	char *newValue = NULL;
            	/* For empty string get the default value */
           		rc = ism_config_validate_singletonItem( item, value, 1, &newValue);
           		if (rc) {
                    TRACE(2, "Failed to validate %s\n", item);
                    ism_common_freeProperties(props);
                    if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                    pthread_mutex_unlock(&g_cfglock);
                    ism_common_free(ism_memory_admin_misc,component);
                    return rc;
           		}
           		if (newValue) value = newValue;
//            }


            if ( !strcmpi(item, "SecurityLog")) {
                int securityLog = AuxLogSetting_Min - 1;
                if ( !strcmpi(value, "MIN"))
                    securityLog = AuxLogSetting_Min -1;
                else if ( !strcmpi(value, "NORMAL"))
                    securityLog = AuxLogSetting_Normal -1;
                else if ( !strcmpi(value, "MAX"))
                    securityLog = AuxLogSetting_Max -1;

                ism_security_setAuditControlLog(securityLog);
            }

            if ( !strcmpi(item, "AdminUserPassword")) {
                isAdminPasswd = 1;
                if ( value && *value != '\0' ) {
                    adminUserPWD = ism_security_createAdminUserPasswordHash((char *) value);
                    if ( adminUserPWD == NULL ) {
                        TRACE(2, "Failed to store AdminPassword\n");
                        ism_common_freeProperties(props);
                        if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                        pthread_mutex_unlock(&g_cfglock);
                        ism_common_free(ism_memory_admin_misc,component);
                        return ISMRC_Error;
                    }

                    f.type = VT_String;
                    f.val.s = adminUserPWD;
                } else {
                    TRACE(2, "NULL value is specified for AdminPassword\n");
                    ism_common_freeProperties(props);
                    if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                    pthread_mutex_unlock(&g_cfglock);
                    ism_common_free(ism_memory_admin_misc,component);
                    return ISMRC_BadPropertyValue;
                }
            } else if ( !strcmp(item, "AdminUserID")) {
                isAdminUser = 1;
                if (value && *value != '\0' ) {
                    adminUserName = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                    f.type = VT_String;
                    f.val.s = value;
                } else {
                    TRACE(2, "NULL value is specified for AdminUserID\n");
                    ism_common_freeProperties(props);
                    if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                    pthread_mutex_unlock(&g_cfglock);
                    ism_common_free(ism_memory_admin_misc,component);
                    return ISMRC_BadPropertyValue;
                }
            } else {
                if ( *value != '\0' && isInteger(value) == 1 ) {
                    f.type = VT_Integer;
                    f.val.i = atoi(value);
                } else {
                    f.type = VT_String;
                    f.val.s = value;
                }
            }

            ism_common_setProperty(props, item, &f);
            found = 1;
            break;
        } else {
            ism_field_t f;

            /* Special cases - Subscription */
            if ( isSubscription ) {
            	int l = snprintf(propName, propNameLen, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
            	if (l + 1 > propNameLen) {
            		propNameLen = l + 1;
            		propName = alloca(propNameLen);
            		sprintf(propName, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
            	}
                TRACE(9, "C-ITEM: name:%s  value:%s\n", propName, jent->value);
                if ( !strcmpi(jent->name, "SubscriptionName") ) {
                    char *value = (char *)jent->value;
                    if ( !value ) {
                        TRACE(8,"Null value received for property: %s\n", propName);
                        continue;
                    }
                    int len = strlen(value) + 1;
                    subName = alloca(len);
                    memcpy(subName, value, len - 1);
                    subName[len - 1] = 0;
                    f.type = VT_String;
                    f.val.s = value;
                }

                if ( !strcmpi(jent->name, "ClientID") ) {
                    char *value = (char *)jent->value;
                    if ( !value ) {
                        TRACE(8,"Null value received for property: %s\n", propName);
                        continue;
                    }
                    f.type = VT_String;
                    f.val.s = value;
                }

                if ( !strcmpi(jent->name, "Name") ) {
                    char *value = (char *)jent->value;
                    if ( !value ) {
                        TRACE(8,"Null value received for property: %s\n", propName);
                        continue;
                    }
                    f.type = VT_String;
                    f.val.s = value;
                }
            } else if ( isMQTTClient ) {
            	int l = snprintf(propName, propNameLen, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
            	if (l + 1 > propNameLen) {
            		propNameLen = l + 1;
            		propName = alloca(propNameLen);
            		sprintf(propName, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
            	}

                // TRACE(9, "C-ITEM: name:%s  value:%s\n", propName, jent->value);
                if ( !strcmpi(jent->name, "ClientID") ) {
                    char *value = (char *)jent->value;
                    if ( !value ) {
                        TRACE(8,"Null value received for property: %s\n", propName);
                        continue;
                    }
                    int len = strlen(value) + 1;
                    subName = alloca(len);
                    memcpy(subName, value, len - 1);
                    subName[len - 1] = 0;
                    f.type = VT_String;
                    f.val.s = value;
                }
            } else {
                if ( name != NULL ) {
                	int l = snprintf(propName, propNameLen, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
                	if (l + 1 > propNameLen) {
                		propNameLen = l + 1;
                		propName = alloca(propNameLen);
                		sprintf(propName, "%s.%s.%s", item, jent->name, name);    /* BEAM suppression: passing null object */
                	}
                } else {
                	int l = snprintf(propName, propNameLen, "%s.%s.%s", item, jent->name, objectID);    /* BEAM suppression: passing null object */
                	if (l + 1 > propNameLen) {
                		propNameLen = l + 1;
                		propName = alloca(propNameLen);
                		sprintf(propName, "%s.%s.%s", item, jent->name, objectID);    /* BEAM suppression: passing null object */
                	}

                    if ( ObjectIdField && !strcmpi(jent->name, ObjectIdField)) {
                        objIDset = 1;
                    }
                }

                TRACE(9, "C-ITEM: name:%s  value:%s\n", propName, jent->value);
                if ( !strcmpi(jent->name, "Name") ) {
					f.type = VT_String;
				    f.val.s = name;
                } else {
                    char *value = (char *)jent->value;
                    if ( !value ) {
                        TRACE(8,"Null value received for property: %s\n", propName);
                        continue;
                    }
                    
                    /*If it is a Group Name for HA, consider always a String*/
                    if ( !strcmpi(item, "HighAvailability")  && !strcmpi(jent->name, "Group") ) {
	                    f.type = VT_String;
	                    f.val.s = value;
                    } else if( !strcmpi(item, "TopicPolicy") && !strcmpi(jent->name, "MaxMessageTimeToLive")) {
                    	/* MaxMessageTimeToLive can be unlimited or unsigned long (1-2147483647).
                    	 * Without this check, 2147483647 will be considered as an int and become -1 during atoi.
                    	 */
                    	f.type = VT_String;
                    	f.val.s = value;
                	} else{
	                    if ( *value != '\0' && isInteger(value) == 1 ) {
	                        f.type = VT_Integer;
	                        f.val.i = atoi(value);
	                    } else {
	                        f.type = VT_String;
	                        f.val.s = value;
	                    }
                    }
                }
            }
            ism_common_setProperty(props, propName, &f);
            found = 1;
        }
    }

    /* Add Index and ObjectIdField if not set */
    if ( ObjectIdField != NULL && objectID != NULL ) {
        if ( objIDset == 0 ) {
        	int l = snprintf(propName, propNameLen, "%s.%s.%s", item, ObjectIdField, objectID);    /* BEAM suppression: passing null object */
        	if (l + 1 > propNameLen) {
        		propNameLen = l + 1;
        		propName = alloca(propNameLen);
        		sprintf(propName, "%s.%s.%s", item, ObjectIdField, objectID);    /* BEAM suppression: passing null object */
        	}

            /* get current value from component properties */
            const char *ObjectIdFieldValue = ism_common_getStringProperty(cprops, propName);
            if ( ObjectIdFieldValue ) {
                ism_field_t f;
                f.type = VT_String;
                f.val.s = (char *)ObjectIdFieldValue;
                ism_common_setProperty(props, propName, &f);
                found = 1;
            }
        }
    }

    int isEndpoint = 0;
    if ( strcasecmp(item, "endpoint") == 0 ) {
        isEndpoint = 1;
    }

    /* validate data */
    if ( createObject == 1 && composite == 1 && validateData == 1 ) {
        if ( isEndpoint == 1 ) {
            rc = ism_config_valEndpointObjects(props, name, 1);
            if ( rc != ISMRC_OK ) {
                ism_common_freeProperties(props);
                if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
        } else if ( strcasecmp(item, "connectionpolicy") == 0 || strcasecmp(item, "TopicPolicy") == 0 ) {
            if ( rc != ISMRC_OK ) {
                ism_common_setError(rc);
                ism_common_freeProperties(props);
                if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
        } else if ( strcasecmp(item, "SecurityProfile") == 0) {
			rc = ism_config_validateCertificateProfileExist(json, name, isUpdate);
			if ( rc != ISMRC_OK ) {
				ism_common_freeProperties(props);
				if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
				pthread_mutex_unlock(&g_cfglock);
				ism_common_free(ism_memory_admin_misc,component);
				return rc;
			}

            rc = ism_config_validateLTPAProfileExist(json, name);
            if ( rc != ISMRC_OK ) {
                ism_common_freeProperties(props);
                if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
            rc = ism_config_validateOAuthProfileExist(json, name);
			if ( rc != ISMRC_OK ) {
				ism_common_freeProperties(props);
				if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
				pthread_mutex_unlock(&g_cfglock);
				ism_common_free(ism_memory_admin_misc,component);
				return rc;
			}
        } else if ( strcasecmp(item, "CertificateProfile") == 0) {
            // rc = ism_config_valPolicyOptionalFilter(props, name);
            rc = ism_config_validateCertificateProfileKeyCertUnique(json, name);
            if ( rc != ISMRC_OK ) {
                ism_common_freeProperties(props);
                if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
        }
    } else if ( isEndpoint == 1 && isUpdate == 1 && validateData == 1 ) {
        rc = ism_config_valEndpointObjects(props, name, 0);
        if ( rc != ISMRC_OK ) {
            ism_common_setError(rc);
            ism_common_freeProperties(props);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return rc;
        }
    }

    /* get old certificate and key name before update CertificateProfile*/
    int needDeleteCert= 1;
    int needDeleteKey = 1;
    if (isUpdate) {
        if (strcasecmp(item, "CertificateProfile") == 0) {
            if ( validateData == 1 ) {
                rc = ism_config_validateCertificateProfileKeyCertUnique(json, name);
                if ( rc != ISMRC_OK ) {
                    ism_common_freeProperties(props);
                    pthread_mutex_unlock(&g_cfglock);
                    ism_common_free(ism_memory_admin_misc,component);
                    return rc;
                }

                ism_config_getCertificateProfileKeyCert(name, &cert, &key, 1);
                char *ncertName = (char *)ism_json_getString(json, "Certificate");
                char *nkeyName = (char *)ism_json_getString(json, "Key");
                if (!strcmp(cert, ncertName)) {
                    needDeleteCert = 0;
                }
                if (!strcmp(key, nkeyName)) {
                    needDeleteKey = 0;
                }
            }
        }
        if (strcasecmp(item, "SecurityProfile") == 0) {
			rc = ism_config_validateCertificateProfileExist(json, name, isUpdate);
			if ( rc != ISMRC_OK ) {
				ism_common_freeProperties(props);
				pthread_mutex_unlock(&g_cfglock);
				ism_common_free(ism_memory_admin_misc,component);
				return rc;
			}

            rc = ism_config_validateLTPAProfileExist(json, name);
            if ( rc != ISMRC_OK ) {
                ism_common_freeProperties(props);
                pthread_mutex_unlock(&g_cfglock);
                ism_common_free(ism_memory_admin_misc,component);
                return rc;
            }
            rc = ism_config_validateOAuthProfileExist(json, name);
			if ( rc != ISMRC_OK ) {
				ism_common_freeProperties(props);
				pthread_mutex_unlock(&g_cfglock);
				ism_common_free(ism_memory_admin_misc,component);
				return rc;
			}
        }
        
        
        
    }

    if ( strcasecmp(item, "endpoint") == 0 ) {
    	/*
    	 * On Primary - Update InterfaceName based on configured Interface
    	 * On Standby - Update Interface based on InterfaceName
    	 */
        rc = ism_config_updateEndpointInterface(props, name, (isStandby == 0)?1:0);
        if ( rc != ISMRC_OK ) {
			ism_common_freeProperties(props);
			pthread_mutex_unlock(&g_cfglock);
			ism_common_free(ism_memory_admin_misc,component);
			return rc;
        }
    }


    /* set old name in name is changing */
#if 0
    if ( mode == ISM_CONFIG_CHANGE_NAME ) {
        if ( name && oldname && strcmp(name, oldname) ) {
            ism_field_t f;
            sprintf(propName, "OLD_Name");
            f.type = VT_String;
            f.val.s = oldname;
            ism_common_setProperty(props, propName, &f);
        }
    }
#endif
    /* Don't allow deletion of AdminDefault objects for AdminEndpoint */
    if (mode == ISM_CONFIG_CHANGE_DELETE) {
        if (!strcmp(item, "AdminEndpoint") ) rc = ISMRC_DeleteNotAllowed;
        else if (!strcmp(item, "CertificateProfile") && !strcmp(name, "AdminDefaultCertProf")) rc = ISMRC_DeleteNotAllowed;
    	else if (!strcmp(item, "SecurityProfile") && !strcmp(name, "AdminDefaultSecProfile")) rc = ISMRC_DeleteNotAllowed;
    	else if (!strcmp(item, "ConfigurationPolicy") && !strcmp(name, "AdminDefaultConfigPolicy")) rc = ISMRC_DeleteNotAllowed;
        if ( rc != ISMRC_OK ) {
        	ism_common_setErrorData(rc, "%s", name);
        	ism_common_freeProperties(props);
    		pthread_mutex_unlock(&g_cfglock);
    		ism_common_free(ism_memory_admin_misc,component);
    		return rc;
        }
    }

    /* need to inform component if item is valid and getting deleted */
    if ( (mode == ISM_CONFIG_CHANGE_DELETE) && item && name && persistData && validateData ) {
        TRACE(9, "Delete item %s name %s\n", item?item:"", name?name:"");
        if ( composite == 0 ) {
            TRACE(9, "Cannot delete item %s name %s. Item is not a composite object\n", item?item:"", name?name:"");
            found = 0;
        } else {
            found = 1;
            if ( strcmp(item, "MessageHub") == 0 ) {
                rc = ism_config_valDeleteEndpointObject("MessageHub", name);
            } else if ( strcmp(item, "TopicPolicy") == 0 ) {
               rc = ism_config_valDeleteEndpointObject("TopicPolicies", name);
            } else if ( strcmp(item, "ConnectionPolicy") == 0 ) {
                rc = ism_config_valDeleteEndpointObject("ConnectionPolicies", name);
            } else if ( strcmp(item, "SecurityProfile") == 0 ) {
                rc = ism_config_valDeleteEndpointObject("SecurityProfile", name);
            } else if ( strcmp(item, "CertificateProfile") == 0 ) {
                rc = ism_config_validateDeleteCertificateProfile(name);
            } else if ( strcmp(item, "LTPAProfile") == 0 ) {
                rc = ism_config_validateDeleteLTPAProfile(name);
            } else if ( strcmp(item, "OAuthProfile") == 0 ) {
                rc = ism_config_validateDeleteOAuthProfile(name);
            } else if ( strcmp(item, "Queue") == 0 ) {
                /* Add DiscardMessages property in props list */
                ism_field_t f;
                char *pName = alloca(23 + strlen(name));
                sprintf(pName, "Queue.DiscardMessages.%s", name);
                if ( ism_common_getProperty(props, pName, &f) == 0 ) {
                    if ( f.type == VT_String ) {
                        TRACE(9, "Deleting 'Queue' object: 'Name=%s','DiscardMessages=%s'\n", name, f.val.s);
                    } else {
                        TRACE(9, "Deleting 'Queue' object: 'Name=%s','DiscardMessages=%d'\n", name, f.val.i);
                    }
                    sprintf(propName, "DiscardMessages");
                    ism_common_setProperty(props, propName, &f);
                }
            }
        }
        if ( rc != ISMRC_OK ) {
        	if (rc == ISMRC_SecPolicyInUse) {
        		ism_common_setErrorData(rc, "%s", name);
        	} else {
                ism_common_setError(rc);
        	}
            ism_common_freeProperties(props);
            if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
            pthread_mutex_unlock(&g_cfglock);
            ism_common_free(ism_memory_admin_misc,component);
            return rc;
        }
    }

    /* for server and transport component send config data to callback */
    int useCallback = 0;
    if ( comptype == ISM_CONFIG_COMP_TRANSPORT ) {
        if ( strcasecmp(item, "FIPS") != 0 ) {
            useCallback = 1;
        }
    } else if ( comptype == ISM_CONFIG_COMP_SECURITY ) {
        useCallback = 1;
    } else if ( comptype == ISM_CONFIG_COMP_SERVER ) {
        if ( strcasecmp(item, "LogLevel") == 0    || strcasecmp(item, "ConnectionLog") == 0 ||
             strcasecmp(item, "SecurityLog") == 0 || strcasecmp(item, "AdminLog") == 0 ||
             strcasecmp(item, "TraceLevel") == 0  || strcasecmp(item, "MQConnectivityLog") == 0 ||
             strcasecmp(item, "TraceSelected") == 0  || strcasecmp(item, "TraceConnection") == 0 ||
             strcasecmp(item, "TraceBackup") == 0 )
        {
            useCallback = 1;
        }
    } else if ( comptype == ISM_CONFIG_COMP_ENGINE ) {
        if ( strcasecmp(item, "AdminSubscription") == 0 || strcasecmp(item, "DurableNamespaceAdminSub") ==0 ||
             strcasecmp(item, "NonpersistentAdminSub") == 0 ) {
            useCallback = 1;
        }
    }


    TRACE(9, "ObjectName: %s. SubName: %s. UseCallback:%d. validateData:%d. found:%d\n",
        name, subName?subName:"", useCallback, validateData, found);

    char eptmpbuf[16384];
    concat_alloc_t epbuf = { eptmpbuf, sizeof(eptmpbuf), 0, 0 };
    int rcEPGetConfig = ISMRC_OK;

    if ( found && ( handle->callback || validateData == 0 ) ) {

        /* handle switch for jansson based validation options */
        /* When we switch to jansson based options for all configuration item, this function will be deleted.
         * Till that time, we will enable jansson based validation function and add the configuration item name
         * in the list.
         */
        if (ism_config_getValidationDataIndex(item) == -1) {
            if ( validateData == 1 || useCallback == 1 ) {
                if ( name != NULL ) {
                    TRACE(9, "Invoke config callback: comptype=%d. Item:%s. Name:%s.\n", comptype, item, name?name:"");
                    rc = handle->callback(item, name, props, mode);
                } else if ( subName != NULL ) {
                    TRACE(9, "Invoke config callback: comptype=%d. Item:%s. SubName:%s\n", comptype,item, subName?subName:"");
                    rc = handle->callback(item, subName, props, mode);
                    TRACE(8, "Return code from config callback:%d\n", rc);
                } else {
                    TRACE(9, "Invoke config callback: comptype=%d. Item:%s. ObjectID:%s\n", comptype,item, objectID?objectID:"");
                    rc = handle->callback(item, objectID, props, mode);
                }
            }
        }

        if ( rc == ISMRC_OK ) {
            if ( mode != ISM_CONFIG_CHANGE_DELETE  && mode != ISM_CONFIG_CHANGE_NAME && persistData) {
                /* Add config data to dynamic properties list */
                const char * pName;
                ism_field_t field;
                for (i = 0; ism_common_getPropertyIndex(props, i, &pName) == 0; i++) {
                    ism_common_getProperty(props, pName, &field);
                    if ( ( ObjectIdField !=NULL && objectID != NULL ) || strcmp(pName, "OLD_Name") ) {
                   		ism_common_setProperty(cprops, pName, &field);
                    } else {
                        mode = ISM_CONFIG_CHANGE_DELETE;
                        name = oldname;
                    }
                }
            }

            /* create json string for endpoint object */
            if ( isHAConfig == 0 && ism_ha_admin_isUpdateRequired() == 1 && isEndpoint ) {
                int getConfigLock = 0;
                memset(eptmpbuf, '0', 16384);
                int deleteFlag = 0;
                if ( mode == ISM_CONFIG_CHANGE_DELETE ) deleteFlag = 1;
                rcEPGetConfig = ism_config_getObjectConfig("Transport", "Endpoint", name, NULL, 1, &epbuf, getConfigLock, deleteFlag);
                if ( rcEPGetConfig != ISMRC_OK ) {
                    TRACE(3, "Could not create config JSON string for Endpoint %s. rc=%d", name, rcEPGetConfig);
                }
            }

            if ( (mode == ISM_CONFIG_CHANGE_DELETE) &&  (composite == 1 ) && persistData ) {
                /* delete composite item from dynamic property list */
                int j = 0;
                const char * pName;
                uint64_t nrec = compProps[comptype].nrec;
                char ** namePtrs = alloca(nrec * sizeof(char *));
                int ntot = 0;

                for (i=0; i<nrec; i++)
                    namePtrs[i] = NULL;

                /* if objectID is specified set name to objectID */
                char *useName = NULL;
                if (name)
                	useName = name;
                else  {
                    if ( ObjectIdField != NULL && objectID != NULL ) {
                        useName = objectID;
                    } else {
                        TRACE(3, "Both Name and ObjectID are not set.\n");
                        rc = ISMRC_PropertiesNotValid;
                        ism_common_setError(rc);
                        ism_common_freeProperties(props);
                        pthread_mutex_unlock(&g_cfglock);
                        ism_common_free(ism_memory_admin_misc,component);
                        return rc;
                    }
				}

                char *value;
                for (j = 0; ism_common_getPropertyIndex(cprops, j, &pName) == 0; j++) {
                    if (pName != NULL) {
                        int tmplen = strlen(pName) + strlen(useName) + 1;
                        char * tmpstr = alloca(tmplen);
                        ism_field_t f;
                        ism_common_getProperty(cprops, pName, &f);
                        strcpy(tmpstr, pName);
                        char *nexttoken = NULL;
                        char *p1 = strtok_r((char *)tmpstr, ".", &nexttoken);
                        char *p2 = strtok_r(NULL, ".", &nexttoken);
                        char *p3 = strtok_r(NULL, ".", &nexttoken);
                        if ( p1 && p2 && p3 ) {
                            int nameLen = strlen(p1) + strlen(p2) + 2;
                            char *nameStr = (char *)pName + nameLen;
                            if (!strcmp(p1, item) && !strcmp(nameStr, useName)) {      /* BEAM suppression: passing null object */
                                if (!strcmp(item, "CertificateProfile") && !strcmp(p2, "Certificate")) {
                                    value = (char *)ism_common_getStringProperty(cprops, pName);
                                    cert = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                                } else if (!strcmp(item, "CertificateProfile") && !strcmp(p2, "Key")) {
                                    value = (char *)ism_common_getStringProperty(cprops, pName);
                                    key = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                                } else if (!strcmp(item, "LTPAProfile") && !strcmp(p2, "KeyFileName")) {
                                    /*Security.LTPAProfile.KeyFileName.testLTPAProf*/
                                    value = (char *)ism_common_getStringProperty(cprops, pName);
                                    key = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                                }else if (!strcmp(item, "OAuthProfile") && !strcmp(p2, "KeyFileName")) {
                                    /*Security.LTPAProfile.KeyFileName.testLTPAProf*/
                                    value = (char *)ism_common_getStringProperty(cprops, pName);
                                    key = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                                }

                                /* Do NOT delete the pName directly from cprops here. it will
                                 * skip the next properties inline in this iteration.
                                 */
								namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),pName);
								ntot++;
                                purgeCompProp = 1;
                            }
                        }
                    }
                }
                if (ntot > 0) {
                	for (j = 0; j<ntot; j++) {
                		if (namePtrs[j]) {
                		    ism_common_setProperty(cprops, namePtrs[j], NULL);
                		    TRACE(9, "remove property %s.\n", namePtrs[j]);
                		    ism_common_free(ism_memory_admin_misc,namePtrs[j]);
                		}
                	}
                }
            }

        } else if (rc == ISMRC_PolicyInUse ) {
			TRACE(3, "TopicPolicy %s is in pending Deletion state.\n", name);
        } else {
            TRACE(3, "Configuration change callback failed.: component=%s rc=%d\n", component, rc);
            char buf[1024];
            char *bufptr = buf;
            char *errstr = NULL;
            int inheap = 0;
            int bytes_needed = 0;
            
            const char *locale = ism_common_getLocale();
			if ( !locale ) locale = "en_US";

            if ( comptype == ISM_CONFIG_COMP_ENGINE ) {
                errstr = (char *)ism_common_getErrorStringByLocale(rc, locale, buf, 1024);
            } else {
				int lastError = ism_common_getLastError();
				if ( lastError > 0 ) {
					bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr,1024);
					if (bytes_needed > 1024) {
						bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,365),bytes_needed);
						inheap = 1;
						bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr,bytes_needed);
					}
				}
				if (bytes_needed>0)
				{
					errstr=(char *)&bufptr;
				}else{
					errstr = (char *)ism_common_getErrorStringByLocale(rc, locale, buf, 1024);
				}
            }

            char * oname=NULL;
            char * oname_val = NULL;
            
            if(isSubscription){
                oname="SubscriptionName";
                oname_val = subName;
                
            }else if (isMQTTClient){
                oname = "ClientID";
                oname_val=subName;
            }else{
                oname = "ObjectName";
                oname_val=name;
            }
            
            
            LOG(ERROR, Admin, 6118, "%-s%-s%-s%-s%d", "Configuration Error is detected in {0} configuration item. {1}={2}, Error={3}, RC={4}",
                    item, oname, oname_val?oname_val:"", errstr?errstr:"", rc);

            if (inheap) ism_common_free(ism_memory_admin_misc,bufptr);
        }
    } else {
        if ( validateData == 1 ) {
            rc = ISMRC_PropertiesNotValid;
            ism_common_setError(rc);
        }
    }

    /* update config file if rc is ISMRC_OK */
    int32_t pType = ism_admin_getServerProcType();
    if ( (rc == ISMRC_OK || rc == ISMRC_PolicyInUse) && persistData && ( pType != ISM_PROTYPE_TRACE && pType != ISM_PROTYPE_SNMP)) {
        // if ( dynFname == NULL ) {
        //     dynFname = "server_dynamic.cfg";
        // }

        /* If HA is enabled and node is Primary update Standby */
        if ( setAdminMode == 0 && isHAConfig == 0 && ism_ha_admin_isUpdateRequired() == 1 ) {
            xUNUSED int flag = ism_config_getUpdateCertsFlag(item);
        }

        if ( (rc == ISMRC_OK || rc == ISMRC_PolicyInUse ) && pType != ISM_PROTYPE_TRACE && pType != ISM_PROTYPE_SNMP) {
            if ( storeOnStandby == 1 && comptype == ISM_CONFIG_COMP_MQCONNECTIVITY ) {
                const char *cFname = "mqcbridge_dynamic.cfg";
                pType = ISM_PROTYPE_MQC;
                ism_config_updateFile(cFname, pType);
            } else {
                // ism_config_updateFile(dynFname, pType);
                /* if AdminUserID or AdminUserPasswordis  updated - copy value to adminUser or adminUserPassword */
                if ( isAdminUser == 1 ) {
                    if ( adminUser ) ism_common_free(ism_memory_admin_misc,adminUser);
                    adminUser = adminUserName;
                }
                if ( isAdminPasswd == 1 ) {
                    if ( adminUserPassword ) ism_common_free(ism_memory_admin_misc,adminUserPassword);
                    adminUserPassword = adminUserPWD;
                }
            }
            if ( purgeCompProp == 1 ) {
                int force = 0;
                if ( comptype == ISM_CONFIG_COMP_ENGINE ) {
                    force = 1;
                }
                ism_config_purgeCompProp(component, force);
            }
        }
        if ( ((mode == ISM_CONFIG_CHANGE_DELETE) &&  (composite == 1 )) || (isUpdate && validateData)) {
            if ( strcmp(item, "CertificateProfile") == 0 ) {
                ism_config_deleteCertificateProfileKeyCert(cert, key, needDeleteCert, needDeleteKey);
            }
        }
        if ((mode == ISM_CONFIG_CHANGE_DELETE) &&  (composite == 1 )) {
            if ( strcmp(item, "SecurityProfile") == 0 ) {
                ism_config_deleteSecurityProfile(name);
            } else if ( strcmp(item, "LTPAProfile") == 0 ) {
                ism_config_deleteLTPAKeyFile(key);
            }else if ( strcmp(item, "OAuthProfile") == 0 ) {
                ism_config_deleteOAuthKeyFile(key);
            }
        }
    }

    if (cert)  ism_common_free(ism_memory_admin_misc,cert);
    if (key)   ism_common_free(ism_memory_admin_misc,key);
    if (objectID)  ism_common_free(ism_memory_admin_misc,objectID);
    ism_common_free(ism_memory_admin_misc,component);
    ism_common_freeProperties(props);
    pthread_mutex_unlock(&g_cfglock);

    return rc;
}

/* Set dynamic config from JSON object */
int32_t ism_config_set_dynamic(ism_json_parse_t *json) {
    int validateData = 1;

    /* TODO: this line will be removed when configuration will totally switch from key=value to JSON */
    int rc = ism_config_set_dynamic_internal(json, validateData, NULL, 0, 0, 0, 0);

    /* This is needed for cunit tests to succeed */
    if ( rc == ISMRC_OK ) {
        char *item = (char *)ism_json_getString(json, "Item");
        if ( !item || *item == '\0' )
            return rc;
        if (!strcmp(item, "Subscription")) {
          /* check if deleting object */
          char *delete = (char *)ism_json_getString(json, "Delete");
          if ( delete ) {
            if (!strcmpi(delete, "true")) {
                char *name = (char *)ism_json_getString(json, "Name");
                /* Invoke JSON API to delete config from JSON structure */
                if ( strcmp(item, "Subscription")) {
                    rc = ism_config_json_deleteComposite(item, name);
                }
            }
          }
        } else {
            char *line = json->source;
            int len = json->src_len;

            if ( len != 0 ) {
                int getLock = 1;
                ism_config_convertV1PropsStringToJSONProps(line, getLock);
                // ism_config_json_updateFile(getLock);
            }
        }
    }


    return rc;
}

/* Set dynamic config from JSON object and JSON string */
int32_t ism_config_set_dynamic_extended(int actionType, ism_json_parse_t *json, char *inpbuf, int inpbuflen, char **retbuf) {
	int32_t rc = ISMRC_OK;
    int validateData = 1;

    TRACE(8, "Entry %s: json: %p, inpbuf: %s, inpbuflen: %d\n",
        	__FUNCTION__, json?json:0, inpbuf?inpbuf:"null", inpbuflen);

    /* parse thru the received data and create a property list to be sent to
     * the component using callback function.
     */
    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* Validate object and create configuration properties*/
    rc = ism_config_validate_object(actionType, json, inpbuf, inpbuflen, props);
    int validateRc = rc;
    if ( rc == ISMRC_OK || rc == ISMRC_ClusterRemoveLocalServerNoAck) {
    	char *item = (char *)ism_json_getString(json, "Item");
    	if (item && !strcmpi(item, "ClusterMembership"))
    	    rc = ism_config_set_object(json, validateData, inpbuf, inpbuflen, props, 0);
    	else if (item && !strcmpi(item, "ClientSet"))
    		rc = ism_config_ClientSet(json, validateData, inpbuf, inpbuflen, props, actionType, retbuf);
    	else
            rc = ism_config_set_dynamic_internal(json, validateData, inpbuf, inpbuflen, 0, 0, 0);
    }
    if (rc == ISMRC_OK && validateRc == ISMRC_ClusterRemoveLocalServerNoAck) {
    	rc = validateRc;
    }

    if ( props ) {
        ism_common_freeProperties(props);
    }

    TRACE(8, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 * Dump dynamic configuration data in a file
 */
XAPI uint32_t ism_config_dumpConfig(const char * filepath, int proctype) {
    uint32_t rc = ISMRC_OK;
    int i, j;
    FILE *dest = NULL;
    const char * propName;
    time_t curtime;

    /* open file to dump configuration items */
    dest = fopen(filepath, "w");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    time(&curtime);

    fprintf(dest, "#\n");
    fprintf(dest, "# " IMA_PRODUCTNAME_FULL " Dynamic configuration file\n");
    fprintf(dest, "# Create time: %s", ctime(&curtime));
    fprintf(dest, "#\n");

    for (i=0; i < ISM_CONFIG_COMP_LAST; i++) {
        ism_prop_t *props = compProps[i].props;
        char *compname = compProps[i].name;
        int comptype = compProps[i].type;

        if ( proctype == ISM_PROTYPE_MQC ) {
            if ( comptype != ISM_CONFIG_COMP_SERVER && comptype != ISM_CONFIG_COMP_MQCONNECTIVITY ) continue;
        } else if ( proctype == ISM_PROTYPE_SERVER ) {
            if ( comptype == ISM_CONFIG_COMP_MQCONNECTIVITY ) continue;
        }

        fprintf(dest, "\n# Component: %s\n", compname);
        for (j=0; ism_common_getPropertyIndex(props,j,&propName) == 0; j++) {
            if (propName != NULL) {
                const char *value=ism_common_getStringProperty(props,propName);
                if ( value && *value != '\0' )
                    fprintf(dest, "%s.%s = %s\n", compname, propName, value);
            }
        }
    }
    fclose(dest);
    if (rc)
        ism_common_setError(rc);
    return rc;
}

static int32_t ism_config_updateFile(const char * fileName, int proctype) {
    int32_t rc = ISMRC_OK;
    char bfilepath[1024];
    char cfilepath[1024];
    char ofilepath[1024];
    char tfilepath[1024];

    if (!fileName)  {
        TRACE(2, "A NULL pointer is passed for the configuration file name.\n");
        ism_common_setError(ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    pthread_mutex_lock(&g_cfgfilelock);

    sprintf(cfilepath, "%s/%s", configDir, fileName);
    sprintf(ofilepath, "%s/%s.org", configDir, fileName);
    sprintf(bfilepath, "%s/%s.bak", configDir, fileName);
    sprintf(tfilepath, "%s/%s.tmp", configDir, fileName);

    if ( access(ofilepath, F_OK) == -1 ) {
        TRACE(5, "Make a copy of initial configuration file %s.\n", ofilepath);
        copyFile(cfilepath, ofilepath);
    }

    /* dump content of current configuration to a .temp file */
    if ( (rc = ism_config_dumpConfig(tfilepath, proctype)) == ISMRC_OK ) {
        /* rename current to .bak */
        if ( rename(cfilepath, bfilepath) == 0 ) {
            if ( rename( tfilepath, cfilepath ) != 0 ) {
                TRACE(2, "Could not rename temporary configuration to current. rc=%d\n", errno);
                rc = ISMRC_Error;
            }
        } else {
            TRACE(2, "Could not rename current configuration file to a backup file. rc=%d\n", errno);
            rc = ISMRC_Error;
        }
    }
    if (rc)
        ism_common_setError(rc);

    pthread_mutex_unlock(&g_cfgfilelock);
    return rc;
}

/* Get dynamic config from JSON object */
XAPI int32_t ism_config_get_dynamic(ism_json_parse_t *json, concat_alloc_t *retStr)
{
    int rc = ISMRC_OK;
    int i;
    int listOpt = 0;

    char *component = NULL;
    char *item = (char *)ism_json_getString(json, "Item");
    ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);

    char *name = (char *)ism_json_getString(json, "Name");
    char *ObjectIdField = (char *)ism_json_getString(json, "ObjectIdField");
    char *retList = (char *)ism_json_getString(json, "CLIListOption");

    if ( retList && !strcmpi(retList, "True"))
        listOpt = 1;

    pthread_mutex_lock(&g_cfglock);
    int compType = ism_config_getCompType(component);

    int validateData = 1;
    if ( compType == ISM_CONFIG_COMP_STORE ) {
         /* STORE objects are not sent on callback - these are internal
          * objects updated unsing advanced-pd-options
          */
        validateData = 0;
    }

    ism_config_t * handle = ism_config_getHandle(compType, NULL);
    if ( !handle && validateData == 0 ) {
        pthread_mutex_unlock(&g_cfglock);
        ism_config_register(compType, NULL, NULL, &handle);
        pthread_mutex_lock(&g_cfglock);
        if ( !handle ) {
            /* Invalid component - return error */
            ism_common_setError(ISMRC_InvalidComponent);
            pthread_mutex_unlock(&g_cfglock);
            return ISMRC_InvalidComponent;
        }
    } else if ( !handle )  {
        /* try for object specific handle */
        handle = ism_config_getHandle(compType, item);
        if ( !handle ) {
            if ( compType == ISM_CONFIG_COMP_STORE || compType == ISM_CONFIG_COMP_ENGINE) {
                pthread_mutex_unlock(&g_cfglock);
                ism_config_register(compType, NULL, NULL, &handle);
                pthread_mutex_lock(&g_cfglock);
            }
            if ( !handle ) {
                /* Invalid component - return error */
                ism_common_setError(ISMRC_InvalidComponent);
                pthread_mutex_unlock(&g_cfglock);
                return ISMRC_InvalidComponent;
            }
        }
    }

    ism_prop_t *props = NULL;
    int freeProps = 0;
    if ( ObjectIdField == NULL ) {
    	int doesObjExist = 0;
        props = ism_config_getPropertiesInternal(handle, item, name, 1, &doesObjExist, 0, 0);
        if ( !props ) {
            ism_common_setError(ISMRC_PropertyNotFound);
            pthread_mutex_unlock(&g_cfglock);
            if (component) ism_common_free(ism_memory_admin_misc,component);
            return ISMRC_PropertyNotFound;
        }
        freeProps = 1;
    } else {
        props = compProps[compType].props;
    }

    const char * propName;
    int propComposite = -1;
    uint64_t nrec = compProps[compType].nrec;
    char ** namePtrs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,374),nrec * sizeof(char *));
    int ntot = 0;

    for (i=0; i<nrec; i++)
        namePtrs[i] = NULL;

    /* if name is not specified, then check for composite object */
    if (!name) {
        ism_common_getPropertyIndex(props, 1, &propName);
        char *ntoken = (char *)propName;
        if ( ntoken && *ntoken != '\0' ) {
            while (*ntoken) {
                if ( *ntoken == '.' )
                    propComposite += 1;
                if (propComposite == 1)
                    break;
                ntoken++;
            }
        }

        if (propComposite == 1) {
            /* get list of names */

            for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
                const char *value = ism_common_getStringProperty(props, propName);
                
                /* do not return value that is not set */
                if (!value || (value && *value == '\0') || (value && *value == '!'))
                    continue;

                if ( ObjectIdField ) {
                	char *p = strstr(propName, ObjectIdField);
                    if ( p ) {
                    	p = p + strlen(ObjectIdField) + 1;
                        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),p);
                        ntot += 1;
                    }
                } else { 
                    if ( strstr((char *)propName, ".Name.")) {
                        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
                        ntot += 1;
                    }
                }
            }
        } else {
            ntot += 1;
        }
    } else {
        namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
        ntot += 1;
    }


    int itemLoop = 0;
    int found = 0;
    char *lastname = NULL;
    int addComma = 0;
    int cycle = 0;
    int startBracketAdded = 0;
    
    for (itemLoop = 0; itemLoop < ntot; itemLoop++) {
        int addStartTag = 1;
        if (ntot > 1) {
            /* get property of specific object */
            char *iName = namePtrs[itemLoop];
            if (freeProps == 1) {
            	ism_common_freeProperties(props);
            	props = NULL;
            }
            props = ism_config_getPropertiesDynamic(handle, item, iName);
        }

        if (props) {
            int eol = 0;

            for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
                const char *value = ism_common_getStringProperty(props, propName);

                /* do not return value that is not set */
                if (!value || *value == '\0')
                     continue;

                if (ObjectIdField && *ObjectIdField && strstr(propName, ".UID."))
                	continue;

                if ( itemLoop == 0 && startBracketAdded == 0 ) {
                   ism_json_putBytes(retStr, "[ ");
                   startBracketAdded = 1;
                }

                /* seams like a composite object  - get key */
                int nameLen = strlen(propName) + 1;
                char *tmpstr = alloca(nameLen);
                memcpy(tmpstr, propName, nameLen);
                tmpstr[nameLen-1] = 0;                
                char *nexttoken = NULL;
                char *p1 = strtok_r(tmpstr, ".", &nexttoken);
                char *p2 = strtok_r(NULL, ".", &nexttoken);
                char *p3 = strtok_r(NULL, ".", &nexttoken);
                
                /* Add composite object items */ 
                if (p3 && p2 && p1) {
                    nameLen = strlen(p1) + strlen(p2) + 2;
                    char *nameStr = (char *)propName + nameLen;
                    
                    if ( addStartTag == 1 ) {
                        if ( itemLoop == 0 ) {
                            if ( ObjectIdField && *ObjectIdField ) {
                                ism_json_putBytes(retStr, "{ \"UID\":\"");
                            } else {
                                ism_json_putBytes(retStr, "{ \"Name\":\"");
                            }
                            ism_json_putEscapeBytes(retStr, nameStr, (int) strlen(nameStr));
                            ism_config_bputchar(retStr, '"');
                        } else {
                            if ( ObjectIdField && *ObjectIdField ) {
                                ism_json_putBytes(retStr, "},{ \"UID\":\"");
                            } else {
                                ism_json_putBytes(retStr, "},{ \"Name\":\"");
                            }
                            ism_json_putEscapeBytes(retStr, nameStr, (int) strlen(nameStr));
                            ism_config_bputchar(retStr, '"');
                        }
                        addStartTag = 0;
                    }

                    if (strcasecmp(p2, "Name") ) {
						if ( listOpt == 0 || (listOpt == 1 && ObjectIdField && strcasecmp(p2, ObjectIdField) == 0 )) {
							ism_json_putBytes(retStr, ",\"");
							ism_json_putEscapeBytes(retStr, p2, (int)strlen(p2));
							ism_json_putBytes(retStr, "\":\"");
							ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
							ism_config_bputchar(retStr, '"');
						}
                    }
                    found = 1;
                    continue;
                } else if (!p3 && p2 && p1) {
                    if (cycle == 0 && itemLoop == 0) {
                        ism_json_putBytes(retStr, " { ");
                        cycle = 1;
                    }

                    if (addComma == 1 && eol == 0) {
                        ism_config_bputchar(retStr, ',');
                        addComma = 0;
                    }

                    ism_config_bputchar(retStr, '"');
                    ism_json_putEscapeBytes(retStr, p2, (int)strlen(p2));
                    ism_json_putBytes(retStr, "\":\"");
                    ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
                    ism_config_bputchar(retStr, '"');
                    addComma = 1;
                    found = 1;
                } else if (!p3 && !p2 && p1) {
                    if (cycle == 0 && itemLoop == 0) {
                        ism_json_putBytes(retStr, " { ");
                        cycle = 1;
                    }

                    if (addComma == 1 && eol == 0) {
                        ism_config_bputchar(retStr, ',');
                        addComma = 0;
                    }

                    ism_config_bputchar(retStr, '"');
                    ism_json_putEscapeBytes(retStr, p1, (int)strlen(p1));
                    ism_json_putBytes(retStr, "\":\"");
                    ism_json_putEscapeBytes(retStr, value, (int)strlen(value));
                    ism_config_bputchar(retStr, '"');
                    addComma = 1;
                    found = 1;
                }
            }
        }
    }

    if (found == 1) {
        ism_json_putBytes(retStr, " } ]\n");
    } else {
        rc = ISMRC_NotFound;
    }

    for (i=0; i<nrec; i++) {
        if ( namePtrs[i] )
            ism_common_free(ism_memory_admin_misc,namePtrs[i]);
    }
    ism_common_free(ism_memory_admin_misc,namePtrs);

    if (lastname)
        ism_common_free(ism_memory_admin_misc,lastname);

    if ( freeProps == 1 )
        ism_common_freeProperties(props);

    if (rc)
        ism_common_setError(rc);


    pthread_mutex_unlock(&g_cfglock);
    if (component) ism_common_free(ism_memory_admin_misc,component);
    return rc;
}

/*
 * Get TrustedCertificate and create REST API return JSON payload in http outbuf.
 * TrustedCertificate is not a name object and has to be treated as a special case.
 *
 * @param  secProfile     SecurityProfile name
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_trustedCertificate(char *secProfile, ism_http_t *http) {
    int rc = ISMRC_OK;
    int found = 0;
    struct dirent *dent;
    struct dirent *pent;
    char fpath[1024];
    int certcount = 0;

    char *trustCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");
    DIR* truststoreDir = opendir(trustCertDir);

    if (truststoreDir == NULL) {
    	rc = 6233;
    	ism_common_setError(6233);
    	//DISPLAY_MSG(ERROR,6233, "", "Cannot access " IMA_PRODUCTNAME_FULL " trusted store.");
    	goto GET_TRUSTED_END;
    }

	//Start the return JSON payload
	ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
	ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
	//ism_json_putBytes(&http->outbuf, "\",\"TrustedCertificate\":[{");
	ism_json_putBytes(&http->outbuf, "\",\"TrustedCertificate\":[ ");

	int sameProfile = 0;
    while((dent = readdir(truststoreDir)) != NULL) {
    	struct stat st;

    	if(!strcmp(dent->d_name, ".") ||
    			!strcmp(dent->d_name, "..") ||
    			(strstr(dent->d_name, "_capath") != NULL) ||
    			(strstr(dent->d_name, "_allowedClientCerts") != NULL) ||
    			(strstr(dent->d_name, "_cafile.pem") != NULL))
    		continue;

    	//fprintf(stderr, "entryname: %s\n", dent->d_name);
    	stat(dent->d_name, &st);

    	if (S_ISDIR(st.st_mode) == 0) {
    		//fprintf(stderr, "This is a profile: %s\n", dent->d_name);
    		sprintf(fpath, "%s/%s", trustCertDir, dent->d_name);
    		DIR *profDir = opendir(fpath);
    		if (profDir == NULL) {
    			rc = 6234;
    			ism_common_setErrorData(rc, "%s", dent->d_name);
    			//DISPLAY_MSG(ERROR,6234, "%s", "Cannot open security profile {0}.", dent->d_name);
    	    	return rc;
    		}

    		certcount = 0;
    		while ((pent = readdir(profDir)) != NULL) {
    			struct stat pst;
    			if(!strcmp(pent->d_name, ".") || !strcmp(pent->d_name, ".."))
    			    continue;

    			stat(pent->d_name, &pst);
    			if (S_ISREG(pst.st_mode) == 0) {
    				if (sameProfile == 0) {
    					if (found > 0)
							ism_json_putBytes(&http->outbuf, ", ");
						ism_json_putBytes(&http->outbuf, "{\"SecurityProfileName\":\"");
						ism_json_putBytes(&http->outbuf, dent->d_name);
						ism_json_putBytes(&http->outbuf, "\", \"TrustedCertificate\":[");
						sameProfile = 1;
    				}
    				if (certcount > 0)
    					ism_json_putBytes(&http->outbuf, ", ");
    				ism_json_putBytes(&http->outbuf, "\"");
    				ism_json_putEscapeBytes(&http->outbuf, pent->d_name, (int) strlen(pent->d_name));
    				ism_json_putBytes(&http->outbuf, "\"");
    				//fprintf(stdout, "%s/%s\n", dent->d_name, pent->d_name);
    				certcount++;
    			}
    		}
    		closedir(profDir);
    		found++;
    		if (certcount > 0)
    			ism_json_putBytes(&http->outbuf, " ] }");
    	}
    	sameProfile = 0;
    }
    closedir(truststoreDir);
    //if (found == 0)
    	//DISPLAY_MSG(ERROR,8424, "", "No TrustedCertificate has been found.");
    //    DISPLAY_MSG(ERROR,8048, "%s", "The requested item \"{0}\" is not found.", "TrustedCertificate");
    ism_json_putBytes(&http->outbuf, "] }");

GET_TRUSTED_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}


/*
 * Get ClientCertificate and create REST API return JSON payload in http outbuf.
 * ClientCertificate is not a name object and has to be treated as a special case.
 *
 * @param  secProfile     SecurityProfile name
 * @param  http           ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_clientCertificate(char *secProfile, ism_http_t *http) {
    int    rc = ISMRC_OK;
    struct dirent *pent;
    int    certcount = 0;
    char   dpath[2048];

    char *trustCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");
    sprintf(dpath, "%s/%s_allowedClientCerts", trustCertDir?trustCertDir:"", secProfile?secProfile:"");
    DIR * clientcertstoreDir = opendir(dpath);

    if (clientcertstoreDir == NULL) {
        rc = 6233;
        ism_common_setError(6233);
        goto GET_CLIENT_END;
    }

    // Start the return JSON payload
    ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
    ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
    ism_json_putBytes(&http->outbuf, "\",\"ClientCertificate\":[ ");

    certcount = 0;
    while ((pent = readdir(clientcertstoreDir)) != NULL) {
        struct stat pst;
        if(!strcmp(pent->d_name, ".") || !strcmp(pent->d_name, ".."))
            continue;

        stat(pent->d_name, &pst);
        if (S_ISREG(pst.st_mode) == 0) {
            if (certcount > 0)
                ism_json_putBytes(&http->outbuf, ", ");
            ism_json_putBytes(&http->outbuf, "\"");
            ism_json_putEscapeBytes(&http->outbuf, pent->d_name, (int) strlen(pent->d_name));
            ism_json_putBytes(&http->outbuf, "\"");
            certcount++;
        }
    }
    closedir(clientcertstoreDir);
    ism_json_putBytes(&http->outbuf, "] }");

GET_CLIENT_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}


/*
 * Get singelton object and create REST API return JSON payload in http outbuf.
 *
 * @param  component      component
 * @param  name           specific name
 * @param  http			  ism_http_t object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_get_singletonObject(char *component, char *name, ism_http_t * http)
{

    int rc = ISMRC_OK;

    pthread_mutex_lock(&g_cfglock);
    int compType = ism_config_getCompType(component);

    int validateData = 1;
    if ( compType == ISM_CONFIG_COMP_STORE ) {
         /* STORE objects are not sent on callback - these are internal
          * objects updated unsing advanced-pd-options
          */
        validateData = 0;
    }

    ism_config_t * handle = ism_config_getHandle(compType, NULL);
	if ( !handle && validateData == 0 ) {
		pthread_mutex_unlock(&g_cfglock);
		ism_config_register(compType, NULL, NULL, &handle);
		pthread_mutex_lock(&g_cfglock);
		if ( !handle ) {
			/* Invalid component - return error */
			ism_common_setError(ISMRC_InvalidComponent);
			pthread_mutex_unlock(&g_cfglock);
			return ISMRC_InvalidComponent;
		}
	} else if ( !handle )  {
		/* try for object specific handle */
		handle = ism_config_getHandle(compType, name);
		if ( !handle ) {
			if ( compType == ISM_CONFIG_COMP_STORE || compType == ISM_CONFIG_COMP_ENGINE) {
				pthread_mutex_unlock(&g_cfglock);
				ism_config_register(compType, NULL, NULL, &handle);
				pthread_mutex_lock(&g_cfglock);
			}
			if ( !handle ) {
				/* Invalid component - return error */
				ism_common_setError(ISMRC_InvalidComponent);
				pthread_mutex_unlock(&g_cfglock);
				return ISMRC_InvalidComponent;
			}
		}
	}

	//Start the return JSON payload
	ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
	ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
	ism_json_putBytes(&http->outbuf, "\",");

	ism_prop_t *props = NULL;
	props = ism_config_getPropertiesDynamic(handle, NULL, name);
	if (props) {
		ism_json_putBytes(&http->outbuf, " \"");
		ism_json_putEscapeBytes(&http->outbuf, name, (int) strlen(name));
		ism_json_putBytes(&http->outbuf, "\":\"");
		const char *value = ism_common_getStringProperty(props, name);
		if (!value || *value == '\0') {
			ism_json_putBytes(&http->outbuf, "\" }");
		}else {
			ism_json_putEscapeBytes(&http->outbuf, value, (int) strlen(value));
			ism_json_putBytes(&http->outbuf, "\" }");
		}
	} else {
		rc = ISMRC_PropertyNotFound;
	}

	pthread_mutex_unlock(&g_cfglock);
	return rc;
}

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
XAPI int ism_config_get_compositeObject(char *component, char *item, char *name, ism_http_t *http)
{
    int rc = ISMRC_OK;
    int i;
    char *ObjectIdField = NULL;

    TRACE(5, "%s: component: %s, item: %s, name: %s\n", __FUNCTION__, component?component:"null", item?item:"null", name?name:"null");
    pthread_mutex_lock(&g_cfglock);
    int compType = ism_config_getCompType(component);

    int validateData = 1;
    if ( compType == ISM_CONFIG_COMP_STORE ) {
         /* STORE objects are not sent on callback - these are internal
          * objects updated unsing advanced-pd-options
          */
        validateData = 0;
    }

    ism_config_t * handle = ism_config_getHandle(compType, NULL);
	if ( !handle && validateData == 0 ) {
		pthread_mutex_unlock(&g_cfglock);
		ism_config_register(compType, NULL, NULL, &handle);
		pthread_mutex_lock(&g_cfglock);
		if ( !handle ) {
			/* Invalid component - return error */
			ism_common_setError(ISMRC_InvalidComponent);
			pthread_mutex_unlock(&g_cfglock);
			return ISMRC_InvalidComponent;
		}
	} else if ( !handle )  {
		/* try for item specific handle */
		handle = ism_config_getHandle(compType, item);
		if ( !handle ) {
			if ( compType == ISM_CONFIG_COMP_STORE || compType == ISM_CONFIG_COMP_ENGINE) {
				pthread_mutex_unlock(&g_cfglock);
				ism_config_register(compType, NULL, NULL, &handle);
				pthread_mutex_lock(&g_cfglock);
			}
			if ( !handle ) {
				/* Invalid component - return error */
				ism_common_setError(ISMRC_InvalidComponent);
				pthread_mutex_unlock(&g_cfglock);
				return ISMRC_InvalidComponent;
			}
		}
	}
	if (!strcmpi(item, "TopicMonitor") || !strcmpi(item, "ClusterRequestedTopics")) {
		int tlen = strlen("TopicString");
		ObjectIdField = alloca(tlen +1);
		ObjectIdField = memcpy(ObjectIdField, "TopicString", tlen);
		ObjectIdField[tlen] = '\0';
	}

	ism_prop_t *props = NULL;
	int freeProps = 0;
	const char * propName;
	uint64_t nrec = compProps[compType].nrec;
	char ** namePtrs = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,379),nrec * sizeof(char *));
	int ntot = 0;

	for (i=0; i<nrec; i++)
		namePtrs[i] = NULL;

	/* if name is not specified, then check for composite object */
	// ism_config_getPropertiesDynamic does not handle TopicMonitor correctly.
	// Always search for entire TopicMonitor properties
	if (!name || ObjectIdField) {
		/* get list of names */
		props = ism_config_getPropertiesDynamic(handle, item, NULL);
		//freeProps = 1;
		char *citem = alloca(strlen(item)+7);
		sprintf(citem, "%s.Name.", item);
			for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
				const char *value = ism_common_getStringProperty(props, propName);

				/* do not return value that is not set */
				if (!value || (value && *value == '\0') || (value && *value == '!'))
					continue;

				if ( ObjectIdField ) {
					char *p = strstr(propName, ObjectIdField);
					if ( p ) {
						p = p + strlen(ObjectIdField) + 1;
						namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),p);
						ntot += 1;
					}
				} else {
					//if ( strstr((char *)propName, ".Name.") ) {
					if ( strstr((char *)propName, citem) ) {
						namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),value);
						ntot += 1;
						TRACE(9, "find name: %s\n", value);
					}
				}
			}
	} else {
		    namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),name);
		    ntot += 1;
	}


	int itemLoop = 0;
	int found = 0;
	char *lastname = NULL;

	ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
	ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
	ism_json_putBytes(&http->outbuf, "\",\n  \"");
	ism_json_putEscapeBytes(&http->outbuf, item, (int) strlen(item));
	if (!strcmp(item, "TopicMonitor") || !strcmp(item, "ClusterRequestedTopics")) {
		ism_json_putBytes(&http->outbuf, "\":[ ");
	}else
	    ism_json_putBytes(&http->outbuf, "\":{ ");

	char *iName = NULL;
	for (itemLoop = 0; itemLoop < ntot; itemLoop++) {
		if (ntot > 0) {
			/* get property of specific object */
			iName = namePtrs[itemLoop];
			if (freeProps == 1) {
				ism_common_freeProperties(props);
				props = NULL;
			}
			props = ism_config_getPropertiesDynamic(handle, item, iName);
		}

		TRACE(9, "iName: %s, props: %s\n", iName?iName:"null", props?"Not null":"null");
		if (iName && props) {
			if (itemLoop > 0) {
				if ( !strcmpi(item, "TopicMonitor") && name) {
					TRACE(9, "%s: Try to find single TopicMonitor TopicString: %s\n", __FUNCTION__, name);
				} else if ( !strcmpi(item, "ClusterRequestedTopics") && name) {
					TRACE(9, "%s: Try to find single ClusterRequestedTopics TopicString: %s\n", __FUNCTION__, name);
				} else {
				    ism_json_putBytes(&http->outbuf, ",\n");
				}
			}

			//add object name
			if ( strcmpi(item, "TopicMonitor") && strcmpi(item, "ClusterRequestedTopics") &&
			        strcmpi(item, "HighAvailability") && strcmpi(item, "LDAP") &&
			        strcmpi(item, "ClusterMembership") && strcmpi(item, "AdminEndpoint")) {

				ism_json_putBytes(&http->outbuf, "\n    \"");
				ism_json_putEscapeBytes(&http->outbuf, iName, (int) strlen(iName));
			    ism_json_putBytes(&http->outbuf, "\":{ ");
			}

			int startlen = 0;
			for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {

				const char *value = ism_common_getStringProperty(props, propName);

				/* do not return value that is not set */
				if (!value || *value == '\0') {
					continue;
				}

				if (ObjectIdField && *ObjectIdField && strstr(propName, ".UID."))
					continue;

				if ( (!strcmpi(item, "TopicMonitor") || !strcmpi(item, "ClusterRequestedTopics")) && name) {
					if (strcmp(name, value) ) {
						continue;
					}
				}

				/* seams like a composite object  - get key */
				int nameLen = strlen(propName) + 1;
				char *tmpstr = alloca(nameLen);
				memcpy(tmpstr, propName, nameLen);
				tmpstr[nameLen-1] = 0;
				char *nexttoken = NULL;
				char *p1 = strtok_r(tmpstr, ".", &nexttoken);
				char *p2 = strtok_r(NULL, ".", &nexttoken);
				char *p3 = strtok_r(NULL, ".", &nexttoken);

				/* Add composite object objects */
				if (p3 && p2 && p1) {
					if (strcasecmp(p2, "Name") ) {
						//Only put comma if we have next entry
						if (startlen > 0 ) {
							ism_config_bputchar(&http->outbuf, ',');
						}

						//TopicString present as a value of array
						if (!strcmpi(item, "TopicMonitor") || !strcmpi(item, "ClusterRequestedTopics")) {
							ism_json_putBytes(&http->outbuf, " \"");
							ism_json_putEscapeBytes(&http->outbuf, value, (int)strlen(value));
							ism_config_bputchar(&http->outbuf, '"');
						}else {
							ism_json_putBytes(&http->outbuf, " \"");
							ism_json_putEscapeBytes(&http->outbuf, p2, (int)strlen(p2));
							ism_json_putBytes(&http->outbuf, "\":\"");
							ism_json_putEscapeBytes(&http->outbuf, value, (int)strlen(value));
							ism_config_bputchar(&http->outbuf, '"');
						}
						startlen++;
					}
					found = 1;
					continue;

				}
			}  //end for the item loop
			if ( found == 1 && ObjectIdField && name) {
				break;
			}
		}
		if (strcmpi(item, "TopicMonitor") && strcmpi(item, "ClusterRequestedTopics"))
		    ism_json_putBytes(&http->outbuf, " }");
	}

	if (found == 1) {
		if (!strcmpi(item, "TopicMonitor") || !strcmpi(item, "ClusterRequestedTopics")) {
			ism_json_putBytes(&http->outbuf, "\n  ]");
		}else if ( strcmpi(item, "HighAvailability") && strcmpi(item, "LDAP") &&
			strcmpi(item, "ClusterMembership") && strcmpi(item, "AdminEndpoint")) {
			ism_json_putBytes(&http->outbuf, "\n  }");
		}
		ism_json_putBytes(&http->outbuf, "\n}\n");
	} else {
		if (!name) {
			if (!strcmpi(item, "TopicMonitor") || !strcmpi(item, "ClusterRequestedTopics"))
				ism_json_putBytes(&http->outbuf, "]}");
			else
			    ism_json_putBytes(&http->outbuf, "}}");
		} else {
		    rc = ISMRC_NotFound;
		}
	}

	for (i=0; i<nrec; i++) {
		if ( namePtrs[i] )
			ism_common_free(ism_memory_admin_misc,namePtrs[i]);
	}
	ism_common_free(ism_memory_admin_misc,namePtrs);

	if (lastname)
		ism_common_free(ism_memory_admin_misc,lastname);

	if ( freeProps == 1 )
		ism_common_freeProperties(props);

	if (rc)
		ism_common_setError(rc);


	pthread_mutex_unlock(&g_cfglock);
	return rc;
}

/*
 * Create REST API error return message in http outbuf.
 *
 * @param  http          ism_http_t object
 * @param  retcode       error code of the error message. if has getLocatError, will use lastError error code.
 * @param  repl          replacement string array
 * @param  replSize      replacement string array size.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI void ism_confg_rest_createErrMsg(ism_http_t * http, int retcode, const char **repl, int replSize) {
	int rc = ISMRC_OK;
	int xlen;
	char  msgID[12];
	char  rbuf[1024];
	char  lbuf[1024];
	char *locale = NULL;

    http->outbuf.used = 0;

	char buf[4096];
	char *bufptr = buf;
	char *errstr = NULL;
	int bytes_needed = 0;

	if (http->locale && *(http->locale) != '\0') {
		locale = http->locale;
	} else {
		locale = "en_US";
	}

	if (retcode != ISMRC_OK && retcode != 6011)
	    rc = ism_common_getLastError();

	if (rc > 0) {

		/* Error string in HTTP object is created using client locale */
		ism_admin_getMsgId(rc, msgID);

		bytes_needed= ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
		if (bytes_needed > sizeof(buf)) {
			bufptr = alloca(bytes_needed);
			bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
		}

		errstr=(char *)bufptr;

		ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
		ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
		ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
		ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
		ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
		if ( errstr ) {
			ism_json_putEscapeBytes(&http->outbuf, errstr, (int) strlen(errstr));
		} else {
			ism_json_putEscapeBytes(&http->outbuf, "Unknown", 7);
		}
		ism_json_putBytes(&http->outbuf, "\" }\n");

		/* For LOG file on the system where MessageSight server is running, error message is
		 * created using system locale. If system locale is same as client system locale,
		 * reuse errstr if set.
		 */

		const char * systemLocale = ism_common_getLocale();
		if(http->locale && strcmp(http->locale,  systemLocale)) {
			bufptr = buf;
			bytes_needed = ism_common_formatLastErrorByLocale(systemLocale, bufptr, sizeof(buf));
			if (bytes_needed > sizeof(buf)) {
				bufptr = alloca(bytes_needed);
				bytes_needed = ism_common_formatLastErrorByLocale(systemLocale, bufptr, bytes_needed);
			}
			errstr=(char *)bufptr;
		}

		LOG(ERROR, Admin, 6129, "%d%-s", "The " IMA_SVR_COMPONENT_NAME " encountered an error while processing an administration request. The error code is {0}. The error string is {1}.",
				retcode, errstr?errstr:"Unknown");

	} else {
		ism_admin_getMsgId(retcode, msgID);
		if ( ism_common_getMessageByLocale(msgID, locale, lbuf, sizeof(lbuf), &xlen) != NULL ) {
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, replSize);
		} else {
			rbuf[0]='\0';
		}
		ism_json_putBytes(&http->outbuf, "{ \"Version\":\"");
		ism_json_putEscapeBytes(&http->outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
		ism_json_putBytes(&http->outbuf, "\",\"Code\":\"");
		ism_json_putEscapeBytes(&http->outbuf, msgID, (int) strlen(msgID));
		ism_json_putBytes(&http->outbuf, "\",\"Message\":\"");
		ism_json_putEscapeBytes(&http->outbuf, rbuf, (int) strlen(rbuf));
		ism_json_putBytes(&http->outbuf, "\" }\n");
	}

	return;
}


/*
 * Create REST API error return MQC message in outbuf.
 */
XAPI void ism_confg_rest_createMQCErrMsg(concat_alloc_t *outbuf, const char *locale, int retcode, const char **repl, int replSize) {
    int xlen;
    char  msgID[12];
    char  rbuf[1024];
    char  lbuf[1024];

    if (!locale || *locale == '\0') {
        locale = "en_US";
    }

#if 0

    int rc = ISMRC_OK;
    char buf[4096];
    char *bufptr = buf;
    char *errstr = NULL;
    int inheap = 0;
    int bytes_needed = 0;

    if (retcode != ISMRC_OK && retcode != 6011)
        rc = ism_common_getLastError();

    if (rc > 0) {
        ism_admin_getMsgId(rc, msgID);

        bytes_needed= ism_common_formatLastErrorByLocale(locale, bufptr, sizeof(buf));
        if (bytes_needed > sizeof(buf)) {
            bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,383),bytes_needed);
            inheap=1;
            bytes_needed = ism_common_formatLastErrorByLocale(locale, bufptr, bytes_needed);
        }

        if (bytes_needed > 0) {
            errstr=(char *)bufptr;
        } else {
            errstr = (char *)ism_common_getErrorStringByLocale(rc, locale, buf, sizeof(buf));
        }

        ism_json_putBytes(outbuf, "{ \"Version\":\"");
        ism_json_putEscapeBytes(outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
        ism_json_putBytes(outbuf, "\",\"Code\":\"");
        ism_json_putEscapeBytes(outbuf, msgID, (int) strlen(msgID));
        ism_json_putBytes(outbuf, "\",\"Message\":\"");
        if ( errstr ) {
            ism_json_putEscapeBytes(outbuf, errstr, (int) strlen(errstr));
        } else {
            ism_json_putEscapeBytes(outbuf, "Unknown", 7);
        }
        ism_json_putBytes(outbuf, "\" }\n");
        if(inheap==1) {
            ism_common_free(ism_memory_admin_misc,bufptr);
            bufptr = buf;
        }

        inheap = 0;

        /* Get the Error String in SystemLocale for Logging. */
        /*
         * Check if the System Locale is not the same as the Request locale. If not equal,
         * Get the error message in System Locale
         */

        const char * systemLocale = ism_common_getLocale();
        if (locale && strcmp(locale,  systemLocale)){
            bytes_needed=0;
            if(errstr!=NULL){
                bytes_needed= ism_common_formatLastErrorByLocale(systemLocale, bufptr, sizeof(buf));
                if (bytes_needed > sizeof(buf)) {
                    bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,385),bytes_needed);
                    inheap=1;
                    bytes_needed = ism_common_formatLastErrorByLocale(systemLocale, bufptr, bytes_needed);
                }
            }
            if (bytes_needed > 0) {
                errstr=(char *)&buf;
            } else {
                errstr = (char *)ism_common_getErrorStringByLocale(retcode, systemLocale, buf, sizeof(buf));
            }
        }

        LOG(ERROR, Admin, 6129, "%d%-s", "The " IMA_SVR_COMPONENT_NAME " encountered an error while processing an administration request. The error code is {0}. The error string is {1}.",
                retcode, errstr?errstr:"Unknown");

        if(inheap==1) ism_common_free(ism_memory_admin_misc,bufptr);
    } else {
#endif

        ism_admin_getMsgId(retcode, msgID);
        if ( ism_common_getMessageByLocale(msgID, locale, lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, replSize);
        } else {
            rbuf[0]='\0';
        }
        ism_json_putBytes(outbuf, "{ \"Version\":\"");
        ism_json_putEscapeBytes(outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
        ism_json_putBytes(outbuf, "\",\"Code\":\"");
        ism_json_putEscapeBytes(outbuf, msgID, (int) strlen(msgID));
        ism_json_putBytes(outbuf, "\",\"Message\":\"");
        ism_json_putEscapeBytes(outbuf, rbuf, (int) strlen(rbuf));
        ism_json_putBytes(outbuf, "\" }\n");

#if 0
    }
#endif

    return;
}

/************************************************************************
 *      Config HA functions
 */

/* Set config items to sync with HA standby node */
struct {
    char  * compName;
    char  * itemName;
    char  * idString;
    int     type;
    int     syncData;
    int     callbackOnStandby;
} syncProps [] = {
    { "Server",          "ServerUID",                NULL,          0, 1, 1 },
    { "Server",          "LogLevel",                 NULL,          0, 1, 1 },
    { "Server",          "ConnectionLog",            NULL,          0, 1, 1 },
    { "Server",          "SecurityLog",              NULL,          0, 1, 1 },
    { "Server",          "AdminLog",                 NULL,          0, 1, 1 },
    { "Server",          "MQConnectivityLog",        NULL,          0, 1, 1 },
    { "Server",          "TraceLevel",               NULL,          0, 1, 1 },
    { "Server",          "TraceBackup",              NULL,          0, 0, 1 },
    { "Server",          "TraceBackupCount",         NULL,          0, 0, 1 },
    { "Server",          "TraceBackupDestination",   NULL,          0, 0, 1 },
    { "Server",          "LogLocation",              NULL,          1, 0, 1 },
    { "Server",          "Syslog",                   NULL,          1, 0, 1 },
    { "Server",          "CRLReloadFrequency",       NULL,          0, 1, 1 },
    { "Transport",       "Endpoint",                 NULL,          1, 1, 1 },
    { "Security",        "ConnectionPolicy",         NULL,          1, 1, 1 },
    { "Security",        "LTPAProfile",              NULL,          1, 1, 1 },
    { "Security",        "OAuthProfile",             NULL,          1, 1, 1 },
    { "Transport",       "CertificateProfile",       NULL,          1, 1, 1 },
    { "Transport",       "SecurityProfile",          NULL,          1, 1, 1 },
    { "Transport",       "CRLProfile",               NULL,          1, 1, 1 },
    { "Transport",       "MessageHub",               NULL,          1, 1, 1 },
    { "Security",        "LDAP",                     NULL,          1, 1, 1 },
    { "Transport",       "FIPS",                     NULL,          0, 1, 1 },
    { "Transport",       "MQConnectivityEnabled",    NULL,          0, 1, 1 },
    { "Engine",          "Queue",                    NULL,          1, 1, 0 },
    { "Engine",          "AdminSubscription",        NULL,          1, 1, 0 },
    { "Engine",          "DurableNamespaceAdminSub", NULL,          1, 1, 0 },
    { "Engine",          "NonpersistentAdminSub",    NULL,          1, 1, 0 },
    { "Engine",          "TopicMonitor",             "TopicString", 1, 1, 0 },
    { "MQConnectivity",  "MQConnectivityLog",        NULL,          0, 1, 1 },
    { "MQConnectivity",  "QueueManagerConnection",   NULL,          1, 1, 1 },
    { "MQConnectivity",  "DestinationMappingRule",   NULL,          1, 1, 1 },
    { "HA",              "HighAvailability",         NULL,          1, 0, 1 },
    { "Transport",       "AdminEndpoint",            NULL,          1, 0, 1 },
    { "Security",        "ConfigurationPolicy",      NULL,          1, 1, 1 },
    { "Cluster",         "ClusterMembership",        NULL,          1, 1, 0 },
    { "Security",        "TopicPolicy",              NULL,          1, 1, 1 },
    { "Security",        "QueuePolicy",              NULL,          1, 1, 1 },
    { "Security",        "SubscriptionPolicy",       NULL,          1, 1, 1 },
    { "Engine",          "TopicPolicy",              NULL,          1, 0, 0 },
    { "Engine",          "QueuePolicy",              NULL,          1, 0, 0 },
    { "Engine",          "SubscriptionPolicy",       NULL,          1, 0, 0 },
    { "Transport",       "TrustedCertificate",       NULL,          1, 1, 1 },
    { "Transport",       "ClientCertificate",        NULL,          1, 1, 1 },
    { "Engine",          "ResourceSetDescriptor",    NULL,          1, 1, 0 },
    { "Engine",          "ClusterRequestedTopics",   "TopicString", 1, 1, 0 }
};

#define MAX_SYNC_CONFIG_ITEMS (sizeof(syncProps)/sizeof(syncProps[0]))

static int ism_config_getSyncFlag(char *component, char *item) {
    int i = 0;
    int rc = 0;
    if ( !component || !item )
        return rc;
    for (i=0; i<MAX_SYNC_CONFIG_ITEMS; i++) {
        if ( !strcmpi(component, syncProps[i].compName) &&
             !strcmpi(item, syncProps[i].itemName) ) {
            rc = syncProps[i].syncData;
        }
    }
    return rc;
}



/**
 * Return component name associated with a config item
 */

XAPI char * ism_config_getCompName(char *item) {
    int i = 0;
    if ( !item )
        return NULL;
    for (i=0; i<MAX_SYNC_CONFIG_ITEMS; i++) {
        if ( !strcmpi(item, syncProps[i].itemName) ) {
            return syncProps[i].compName;
        }
    }
    return NULL;
}


/**
 * Returns flag to indicate callback needs to be invoked on standby
 */
XAPI int ism_config_invokeCallbackOnStandbyFlag(int compType, char *item) {
    int i = 0;
    int rc = 0;
    if ( !item )
        return rc;

    if ( compType < 0 || compType > ISM_CONFIG_COMP_LAST )
        return 0;
    char *component = compProps[compType].name;

    for (i=0; i<MAX_SYNC_CONFIG_ITEMS; i++) {
        if ( !strcmpi(component, syncProps[i].compName) &&
             !strcmpi(item, syncProps[i].itemName) ) {
            rc = syncProps[i].callbackOnStandby;
        }
    }

    return rc;
}


/*
 * Dump dynamic configuration data in a file in JSON format
 * - used on primary node
 */
XAPI uint32_t ism_config_dumpJSONConfig(const char * filepath) {
    uint32_t rc = ISMRC_OK;
    int i;
    FILE *dest = NULL;
    int contentAdded = 0;

    /* open file to dump configuration items */
    dest = fopen(filepath, "w");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    for (i=0; i < MAX_SYNC_CONFIG_ITEMS; i++) {
        char tbuf[8192];
        concat_alloc_t output_buffer = { tbuf, sizeof(tbuf), 0, 0 };
        memset(tbuf, '0', 8192);

        /*
         * avoid non-sync object. such as HA to be dumped.
         */
        if (syncProps[i].syncData == 0)
        	continue;

        int rc1 = ism_config_getJSONString(NULL, syncProps[i].itemName, NULL, &output_buffer, 1, 0);

        if ( rc1 == ISMRC_OK && output_buffer.used > 0 ) {
            char *buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,387),output_buffer.used + 1);
            memcpy(buffer, output_buffer.buf, output_buffer.used);
            buffer[output_buffer.used] = 0;
            fprintf(dest, "%s\n", buffer);
            ism_common_free(ism_memory_admin_misc,buffer);
            contentAdded = 1;
        }

        if (output_buffer.inheap)
            ism_common_freeAllocBuffer(&output_buffer);

    }

    fclose(dest);

    if ( contentAdded == 0 )
        rc = ISMRC_PropertyNotFound;

    if (rc)
        ism_common_setError(rc);

    return rc;
}


#if 0
/*
 * Process dynamic configuration data file in JSON format
 * - used on standby node at v1.x level
 */
static int ism_config_processJSONString_V1(int noSend, int msgLen, char * line, ism_prop_t *currList, int storeOnStandby) {
    uint32_t rc = ISMRC_OK;
    int len = 0;
    char *input_buffer = line;

    if ( !line ) {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "line");
        return ISMRC_ArgNotValid;
    }

    len = strlen(line);

    if ( currList ) {
        input_buffer = alloca(len + 1);
        memcpy(input_buffer, line, len);
        input_buffer[len] = 0;
    }

    TRACE(7, "HA Standby - version v1.x - proctype=%d config: %s\n", noSend, input_buffer);

    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[40];
    
    parseobj.ent_alloc = 500;
    parseobj.ent = ents;
    parseobj.source = (char *) input_buffer;
    parseobj.src_len = len + 1;
    ism_json_parse(&parseobj);

    if (parseobj.rc) {
        TRACE(3, "Invalid JSON string input for config SYNC\n");
        return rc;
    }

    char *component = NULL;
    char *item = (char *)ism_json_getString(&parseobj, "Item");
    ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);
    char *value = (char *)ism_json_getString(&parseobj, "Name");
    char *uid   = (char *)ism_json_getString(&parseobj, "UID");
    const char *propName = NULL;
    int pLen = 3;
    if (item && value && *value != '\0') {
        pLen += (strlen(item) + strlen(value) + 4);
		propName = alloca(pLen);
		sprintf((char *)propName, "%s.Name.%s", item, value);
    } else if (item && uid && *uid != '\0' && !strcmp(item, "TopicMonitor")) {
    	 pLen += (strlen(item) + strlen(uid) + 11);
         propName = alloca(pLen);
         sprintf((char *)propName, "%s.TopicString.%s", item, uid);
    }

    int sFlag = ism_config_getSyncFlag(component, item);
    if ( sFlag == 1 || noSend == 1 ) {
        TRACE(7, "Sync config data: comp:%s  item:%s\n", component, item);
        /* No need to validate data - already validated on Primary */
        int validatData = 0;
        isStandby = 1;
        rc = ism_config_set_dynamic_internal(&parseobj, validatData, NULL, 0, sFlag, noSend, storeOnStandby);
        if ( rc != ISMRC_OK ) {
            TRACE(1, "Failed to update config: rc=%d - %s \n", rc, line);
        } else {
               if ( currList ) {
                /* check in current list */
                ism_field_t f;

                /* Defect 68384
                 * For all the TopicMonitor Entry from V1.1 in serverDynamic.primary,
                 * they don't have UIDs so the TopicMonitor entries
                 * will be created blindly which causes duplication. We just need to delete the ones in
                 * currList.
                 */
                if (item && !strcmp(item, "TopicMonitor") && (uid == NULL || (uid && *uid == '\0')) ) {
                	TRACE(9, "This is a update from V1.1 version, all old TopicMonitor entries should be removed.\n");
                } else {
                    ism_common_getProperty(currList, propName, &f);
                    if ( f.type == VT_Integer && f.val.i == 1 ) {
                        f.val.i = 0;
                        ism_common_setProperty(currList, propName, &f);
                    }
                }
            }
        }
    } else {
        TRACE(8, "Sync bypassed for config data: comp:%s  item:%s", component, item);
    }

    if (component) ism_common_free(ism_memory_admin_misc,component);
    return rc;
}
#endif

/*
 * Update prop list based on config items sent from Primary
 */
static void ism_config_updateCurrList(json_t *post, ism_prop_t *currList) {
    json_t *objval = NULL;
    const char *objkey = NULL;
    int found = 0;

    if ( !post ) {
    	TRACE(5, "Standby: no configuration data is received from Primary\n");
    	return;
    }

    if ( !currList ) {
    	TRACE(5, "Standby: no items found in current configuration list on standby\n");
    	return;
    }

    pthread_rwlock_rdlock(&srvConfiglock);

    int nitems = json_object_size(post);
    if ( nitems == 0 ) return;

	json_object_foreach(post, objkey, objval) {

		if ( !strcmp(objkey, "Version")) continue;

		/* get component and object type */
		int itype = 0;
		int otype = 0;
		int comp  = 0;
		json_t *schemaObj = ism_config_findSchemaObject(objkey, NULL, &comp, &otype, &itype);

		TRACE(9, "Standby: update flag in current configuration list: object=%s type=%d component=%d\n", objkey, otype, comp);

		if ( schemaObj && otype == 1 && ( comp == ISM_CONFIG_COMP_TRANSPORT || comp == ISM_CONFIG_COMP_ENGINE || comp == ISM_CONFIG_COMP_SECURITY || comp == ISM_CONFIG_COMP_MQCONNECTIVITY )) {

			if ( !strcmp(objkey, "TopicMonitor") || !strcmp(objkey, "ClusterRequestedTopics")) {

				int j = 0;
				for (j=0; j<json_array_size(objval); j++) {
					json_t *instObj = json_array_get(objval, j);
					if (instObj) {
						const char *val = json_string_value(instObj);
						if ( val && *val != '\0') {
							ism_field_t f;
							char propName[4096];
							sprintf(propName, "%s.%s", objkey, val);
							f.type = VT_Integer;
							f.val.i = 0;
							ism_common_setProperty(currList, propName, &f);
							found += 1;
						}
					}
				}

			} else {

				/* get instance of composite object and validate the object */
				json_t *instval = NULL;
				const char *instkey = NULL;
				int insttyp;
				void *institer = json_object_iter(objval);
				while (institer) {
					instkey = json_object_iter_key(institer);
					instval = json_object_iter_value(institer);
					insttyp = json_typeof(instval);

					if ( insttyp == JSON_OBJECT ) {

						ism_field_t f;
						char propName[2048];
						sprintf(propName, "%s.%s", objkey, instkey);
						f.type = VT_Integer;
						f.val.i = 0;
						ism_common_setProperty(currList, propName, &f);
						found += 1;
					}

					institer = json_object_iter_next(objval, institer);
				}

			}
		}
	}

	if ( found ) {
        TRACE(5, "Standby: updated %d named config item flags in the current list.\n", found);
	}

    pthread_rwlock_unlock(&srvConfiglock);

    return;
}


/*
 * Process message with dynamic configuration data received from primary
 * NOTE - do not use this function on a primary node
 */
XAPI int ism_config_processJSONString(int noSend, int msgLen, char * line, ism_prop_t *currList, int storeOnStandby) {
    int rc = ISMRC_OK;

    if ( !line ) {
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "line");
        return ISMRC_ArgNotValid;
    }

    /* Create jannson structure from sent message */
    json_t *post = ism_config_json_strToObject(line, &rc);

    /* check version - if version is not 2.0 - process data for migration to new version */
    json_t *verObj = json_object_get(post, "Version");
    if ( !verObj ) {
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    	return rc;
    }

    char *verStr = (char *)json_string_value(verObj);
    if ( verStr && !strcmp(verStr, "2.0")) {
        /* Process post */
        int haSync = 1;
        int validate = 1;
        int restCall = 0;

        /* check for delete key */
        json_t *deleteObj = json_object_get(post, "Delete");
        int deleteOnEmpty = 0;

        // Empty ResourceSetDescriptor is effectively a Delete.
        if (!deleteObj) {
            json_t *resourceSetDescriptor = json_object_get(post, "ResourceSetDescriptor");

            if (resourceSetDescriptor && json_typeof(resourceSetDescriptor) == JSON_OBJECT) {
                if (json_object_size(resourceSetDescriptor) == 0) {
                    deleteObj = json_true();
                    deleteOnEmpty = 1;
                }
            }
        }

        if ( deleteObj && deleteObj == json_true()) {
            int discardMessages = 0; /* Queue delete action is not sent to standby node */
            /* Get object ID from json object */
            json_t *objval = NULL;
            const char *objkey = NULL;
            json_object_foreach(post, objkey, objval) {
                if ( !strcmp(objkey, "Version") || !strcmp(objkey, "Delete")) {
                    continue;
                }

                char *inst = NULL;
                char *subinst = NULL;

                if ( !strcmp(objkey, "TrustedCertificate")) {
                    json_t *instObj = json_object_get(objval, "SecurityProfileName");
                    json_t *subinstObj = json_object_get(objval, "TrustedCertificate");
                    if ( instObj && json_typeof(instObj) == JSON_STRING ) {
                        inst = (char *)json_string_value(instObj);
                    }
                    if ( subinstObj && json_typeof(subinstObj) == JSON_STRING ) {
                        subinst = (char *)json_string_value(subinstObj);
                    }
                } else if ( !strcmp(objkey, "ClientCertificate")) {
                    json_t *instObj = json_object_get(objval, "SecurityProfileName");
                    json_t *subinstObj = json_object_get(objval, "CertificateName");
                    if ( instObj && json_typeof(instObj) == JSON_STRING ) {
                        inst = (char *)json_string_value(instObj);
                    }
                    if ( subinstObj && json_typeof(subinstObj) == JSON_STRING ) {
                        subinst = (char *)json_string_value(subinstObj);
                    }
                } else if (!strcmp(objkey, "TopicMonitor") || !strcmp(objkey, "ClusterRequestedTopics")) {
                    json_t *instObj = json_array_get(objval, 0);
                    if ( instObj && json_typeof(instObj) == JSON_STRING ) {
                        inst = (char *)json_string_value(instObj);
                    }
                } else if (!strcmp(objkey, "ResourceSetDescriptor")) {
                    inst = "ResourceSetDescriptor";
                } else {
                    void *institer = json_object_iter(objval);
                    if ( institer ) {
                        inst = (char *)json_object_iter_key(institer);
                    }
                }

                TRACE(5, "Delete object on standby node: object:%s inst:%s subinst:%s\n",
                    objkey, inst?inst:"NULL", subinst?subinst:"NULL");

                int standbyCheck = 1;

                rc = ism_config_json_deleteObject((char *)objkey, inst, subinst, discardMessages, standbyCheck, NULL);

                // This delete was caused by receipt of an empty object, so ignore
                // the discovery of a missing object here too.
                if (rc == ISMRC_NotFound && deleteOnEmpty) {
                    rc = ISMRC_OK;
                }
            }
        } else {
        	int onlyMqcItems = 0;
        	if ( currList ) ism_config_updateCurrList(post, currList);
            rc = ism_config_json_processObject(post, NULL, NULL, haSync, validate, restCall, NULL, &onlyMqcItems);
            if (ISMRC_OK == ism_common_getLastError())
                ism_common_setError(rc);
        }

    } else {
        rc = ISMRC_NotImplemented;
        if (ISMRC_OK == ism_common_getLastError())
            ism_common_setError(rc);
    }

    return rc;
}

static int ism_config_delete_namedObject(ism_prop_t *currList) {
    /* check currItems and delete items that are not updated */
    const char *pName;
    char *nexttoken = NULL;
    int i;
    int flag = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    for (i = 0; ism_common_getPropertyIndex(currList, i, &pName) == 0; i++) {
        flag = ism_common_getIntProperty(currList, pName, 1);
        if (flag == 0) continue;

        /* The property name format is ObjectName.Name */
    	char *object = strtok_r((char *)pName, ".", &nexttoken);
    	char *name = NULL;
    	if (nexttoken && *nexttoken != '\0')
    	    name = nexttoken;

    	if ( !name || *name == '\0' ) continue;

		/* get component and object type */
		int itype = 0;
		int otype = 0;
		int comp  = 0;
		char *component = NULL;
		json_t *schemaObj = ism_config_findSchemaObject(object, NULL, &comp, &otype, &itype);

		if ( schemaObj ) {
			component = ism_config_getCompString(comp);
		} else {
			TRACE(3, "Warning: Could not find the object in schema: object=%s name=%s \n", object, name );
			continue;
		}

        if ( ism_config_getSyncFlag(component, object) == 1 ) {
        	TRACE(5, "Delete config item: object=%s name=%s \n", object, name );
        } else {
        	TRACE(5, "Delete config item: object=%s name=%s - not deleted because the item is not in sync list\n", object, name );
        	continue;
        }

        json_t *objptr = json_object_get(srvConfigRoot, object);

        if ( objptr ) {

            int objType = json_typeof(objptr);

        	if ( objType == JSON_ARRAY && (!strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics"))) {

        		/* Delete item from array */
        		int j = 0;
        		int found = 0;
        		for (j=0; j<json_array_size(objptr); j++) {
        			json_t *instObj = json_array_get(objptr, j);
        			if (instObj) {
       				    const char *val = json_string_value(instObj);
       				    if (val && !strcmp(val, name)) {
        					found = 1;
        					break;
        				}
        			}
        		}

        		if (found == 1) {
        			int rc = json_array_remove(objptr, j);
        			if ( rc ) {
        				TRACE(5, "Could not delete config item: object=%s name=%s \n", object, name );
        			} else {
					    TRACE(5, "Config item is deleted: object=%s name=%s \n", object, name );
        			}
        		}

        	} else {
        		/* Delete item from object */
				json_t *instptr = json_object_get(objptr, name);
				if ( instptr ) {
					int rc = json_object_del(objptr, name);
					if (rc ) {
						TRACE(5, "Could not delete config item: object=%s name=%s \n", object, name );
					} else {
					    TRACE(5, "Config item is deleted: object=%s name=%s \n", object, name );
					}
				}
        	}
        }
    }

    pthread_rwlock_unlock(&srvConfiglock);
    
    return 0;
}

/*
 * Process dynamic configuration data file in JSON format
 * - used on standby node
 */
XAPI uint32_t ism_config_processJSONConfig(const char * filepath) {
    int rc = ISMRC_OK;

    FILE *dest = NULL;
    char linebuf[102400];
    char * line;

    TRACE(5, "Process configuration data sent by Primary node\n");

    /* Identify if data is sent by v1 system */
    int v2System = 0;

    /* open file to read configuration items */
    dest = fopen(filepath, "rb");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    line = fgets(linebuf, sizeof(linebuf), dest);
    while (line) {
        /* Create jannson structure from sent message */
        json_t *post = ism_config_json_strToObject(line, &rc);

        if ( !post ) {
        	line = fgets(linebuf, sizeof(linebuf), dest);
        	continue;
        }

        /* check version - if version is not 2.0 - process data for migration to new version */
        json_t *verObj = json_object_get(post, "Version");
        if ( !verObj ) {
        	rc = ISMRC_NullPointer;
        	ism_common_setError(rc);
        	json_decref(post);
        	fclose(dest);
        	return rc;
        }

        char *verStr = (char *)json_string_value(verObj);
        if ( verStr && !strcmp(verStr, "2.0")) {
        	v2System = 1;
        	json_decref(post);
        	break;
        }
        json_decref(post);

        line = fgets(linebuf, sizeof(linebuf), dest);
    }
    fclose(dest);

    if ( v2System == 0 ) {
        TRACE(5, "Process configuration data sent by Primary node - complete\n");
        rc = ism_config_migrate_processV1ConfigViaHA();
        return rc;
    }


    /* Process V2 configuration */
    ism_prop_t *currList = ism_config_json_getObjectNames();

    dest = fopen(filepath, "rb");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    line = fgets(linebuf, sizeof(linebuf), dest);
    while (line) {
        rc = ism_config_processJSONString(0, 0, line, currList, 0);
        if ( rc != ISMRC_OK ) {
            break;
        }
        line = fgets(linebuf, sizeof(linebuf), dest);
    }

    fclose(dest);

    /* delete items that are not updated
     * delete Endpoint, CertificateProfile, SecurityProfile, 
     * ConnectionPolicies, TopicPolicies, MessageHub,
     */ 
    if ( currList && rc == ISMRC_OK ) {
        TRACE(5, "Delete items that are not in primary node \n");
        ism_config_delete_namedObject(currList);
        ism_common_freeProperties(currList);
    }

    TRACE(5, "Process configuration data sent by Primary node - complete\n");

    return rc;
}



int ism_config_getSecurityLog(void) {
    /* get SecurityLog and set auditControlLog */
    int newval = AuxLogSetting_Normal - 1;

    ism_prop_t *srvprops = compProps[ISM_CONFIG_COMP_SERVER].props;
    ism_field_t fl;
    ism_common_getProperty(srvprops, "SecurityLog", &fl);
    if ( fl.type == VT_String ) {
        char *tmpval = fl.val.s;
        if ( tmpval ) {
            if ( !strcmpi(tmpval, "MIN"))
                newval = AuxLogSetting_Min -1;
            else if ( !strcmpi(tmpval, "NORMAL"))
                newval = AuxLogSetting_Normal -1;
            else if ( !strcmpi(tmpval, "MAX"))
                newval = AuxLogSetting_Max -1;
        }
        ism_security_setAuditControlLog(newval);
    }

    return newval;
}

XAPI int ism_config_getFIPSConfig(void)
{
    return fipsEnabled;
}

XAPI int ism_config_getMQConnEnabled(void)
{
    return mqconnEnabled;
}

XAPI int ism_config_getSslUseBufferPool(void)
{
    if(fipsEnabled)
        return 0;
    return sslUseBuffersPool;
}


XAPI ism_prop_t * ism_config_getComponentProps(ism_ConfigComponentType_t comp)
{
    return compProps[comp].props;
}

/**
 * Set HighAVailablity Group in HA configuration
 */
XAPI int32_t ism_config_setHAGroupID(const char *name, int len, const char *standbyID) {
    int32_t rc = ISMRC_OK;

	
 	char command[1024];
 	char tbuf[1024];
 	concat_alloc_t jBuffer = { tbuf, sizeof(tbuf), 0, 0 };
 	
    

    if ( (!len || !name || (name && *name == '\0')) && (!standbyID || (standbyID && *standbyID == '\0')) ) {
        TRACE(2, "Invalid Group for HA configuration\n");
        return ISMRC_NullArgument;
    }

    /* Set Group */
    if ( standbyID ) {
        sprintf(command, "{ \"Action\":\"set\",\"User\":\"admin\",\"Component\":\"HA\",\"Item\":\"HighAvailability\",\"Type\":\"composite\",\"Update\":\"true\",\"Name\":\"haconfig\",\"StandbyForce\":\"True\", \"Group\":");
        ism_common_allocBufferCopyLen(&jBuffer, command, strlen(command));
        ism_json_putString(&jBuffer, standbyID);
        ism_common_allocBufferCopyLen(&jBuffer, "}", 1);
    } else {
    	/*Need to escape the Name*/
        sprintf(command, "{ \"Action\":\"set\",\"User\":\"admin\",\"Component\":\"HA\",\"Item\":\"HighAvailability\",\"Type\":\"composite\",\"Update\":\"true\",\"Name\":\"haconfig\",\"Group\":");
        ism_common_allocBufferCopyLen(&jBuffer, command, strlen(command));
        ism_json_putString(&jBuffer, name);
        ism_common_allocBufferCopyLen(&jBuffer, "}", 1);
        
    }

    int buflen = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[20];
    char *tmpbuf = NULL;

    buflen = jBuffer.used;
    tmpbuf = (char *)alloca(buflen + 1);
    memcpy(tmpbuf, jBuffer.buf, buflen);
    tmpbuf[buflen] = 0;

    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) tmpbuf;
    parseobj.src_len = buflen;
    ism_json_parse(&parseobj);

    if (parseobj.rc) {
        TRACE(3, "Failed to parse buffer to set Group (rc=%d): %s\n", parseobj.rc, tmpbuf);
        ism_common_free(ism_memory_admin_misc,tmpbuf);
        if (jBuffer.inheap) ism_common_freeAllocBuffer(&jBuffer);
        return ISMRC_ParseError;
    }
    rc = ism_config_set_dynamic(&parseobj);
    
    if (jBuffer.inheap) ism_common_freeAllocBuffer(&jBuffer);

    return rc;
}

/* return disconnected client notification interval */
XAPI int ism_config_getDisconnectedClientNotifInterval(void) {
    if ( dissconClientNotifInterval == 0 ) {
    	dissconClientNotifInterval = ism_common_getIntConfig("DisconnClientNotifInterval", 60);
    }

    return dissconClientNotifInterval;
}


/*
 * get the UID of the composite object
 */
int32_t ism_config_getObjectUID(ism_prop_t * props, char *item, char *name, ism_field_t * field) {
	char * propName = alloca(strlen(item) + strlen(name) + 6);

    sprintf(propName, "%s.UID.%s", item, name);
    return ism_common_getProperty(props, propName, field);
}

/*
 * Base62 is used for the random part of a generated clientID.
 */
char base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

/*
 * Genenrate a UID
 * Caller will have to free returned uuid
 */
XAPI char * ism_config_genUUID(void) {
	unsigned char randID[24];
	unsigned char uuid[24];
	int i=0;
	char *nuid=NULL;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randID, sizeof randID);
#else
    RAND_bytes(randID, sizeof randID);
#endif

    for (i=0; i<sizeof(randID); i++) {
		uuid[i] = base62[(int)(randID[i]%62)];
	}

    nuid = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,391),25);
    memcpy(nuid, uuid, 24);
    nuid[24] = '\0';
    return nuid;
}

/*
 * create a new config object UID
 * sn will be 7 characters
 * uuid will be 24 characters
 * Total 33 characters including ending '\0'
 */
XAPI int ism_config_createObjectUID(char **ouid) {
	const char *sn = NULL;
	char *uuid;
	int rc = ISMRC_OK;

	/*sn will always be 7 characters*/
	/* For cluster work, it is necessary to call ism_common_initServer() to initialize locks.
	 * The ism_common_initServer() invokes ism_common_initPlatformDataFile() and serial number gets generated.
	 * All Cunit tests rely on environment variable ISMSSN.
	 * If ISMSSN is set use this as serial number otherwise call ism_common_getPlatformSerialNumber() to get
	 * serial number.
	 */
	char *epath = getenv("ISMSSN");
	if (epath ) {
		TRACE(5, "System environment variable ISMSSN is %s\n", epath);
		if( strlen(epath) == 7) {
			sn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),epath);
			TRACE(5, "serial number is %s\n", sn);
		} else {
			TRACE(5, "environment variable %s is not a 7-bytes string\n", epath);
		}
	} else {
	    sn = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"XXXXXXX");
	}
	uuid = ism_config_genUUID();

	if (sn == NULL || strlen(sn)!= 7 || uuid == NULL) {
		TRACE(2, "Serial number or UUID is not correct. sn:%s, uuid:%s\n", sn?sn:"null", uuid?uuid:"null");
		ism_common_setError(ISMRC_UUIDConfigError);
		rc = ISMRC_UUIDConfigError;
	} else {
		sprintf(*ouid, "%s-%s", (char *)sn, uuid);
	}

	if (uuid)  ism_common_free(ism_memory_admin_misc,uuid);
	return rc;
}

/*
 * check if generated UID has a duplication
 * It is called from ism_config_setUID, which invoked after
 * g_cfglock has been obtained.
 */
XAPI int ism_config_checkDuplicatedUID(char *ouid) {
    int rc = ISMRC_OK;
    int i;
    int compType;
    char tmp[256*2];

    if (ouid == NULL) {
    	rc = ISMRC_NullPointer;
    	TRACE(3, "exit ism_config_checkDuplicatedUID, rc=%d\n", rc);
    	return rc;
    }

    for (i=0; i < MAX_SYNC_CONFIG_ITEMS; i++) {
        /* Only add index to composite object and not any Server objects */
        if (syncProps[i].type != 1 || !strcmp(syncProps[i].compName, "Server"))
            continue;

        compType = ism_config_getCompType(syncProps[i].compName);
  		if (compType < 0 || compType >= ISM_CONFIG_COMP_LAST ) {
  			/* Invalid component - return error */
  			ism_common_setError(ISMRC_InvalidComponent);
  			TRACE(5, "checkObjectUUID cannot get configuration properties for %s with compType=%d.\n", syncProps[i].compName, compType);
  			return ISMRC_InvalidComponent;
  		}

        ism_prop_t *props = NULL;
  		props = compProps[compType].props;

  		const char * propName;
  		int j= 0;

  		/* get list of composite obj names. has to go through the list twice:
  		 *  1. get name
  		 *  2. check if an UID associated with the name object
  		 */
  		snprintf(tmp, sizeof(tmp), "%s.UID.", syncProps[i].itemName );

  		for (j = 0; ism_common_getPropertyIndex(props, j, &propName) == 0; j++) {

  			/* The props just stored based on component name*/
  			if (strstr((char *)propName, tmp) == NULL )
  				continue;

  			const char *value = ism_common_getStringProperty(props, propName);
  			/* do not return value that is not set */
  			if (!value || (value && *value == '\0') || (value && *value == '!'))
  				continue;

  			TRACE(9, "propName=%s\n", propName);
  			if (ouid && !strcmp(ouid, value)) {
  				rc = ISMRC_ExistingKey;
  				TRACE(9, "Found duplicated UID:%s, used by %s\n", ouid, propName);
  				return rc;
  			}

  		}
    }

    TRACE(9, "exit ism_config_checkDuplicatedUID, rc=%d\n", rc);
    return rc;
}

/*
 * Generate new UID for the object
 * The function should be called after cfglock acquired.
 */
int32_t ism_config_setObjectUID(ism_prop_t * props, char *item, char *name, char *puid, int32_t pType, char **retId ) {
	ism_field_t f;
	int propNameLen = 512;
	char *propName = alloca(propNameLen);
	int l;
	char *ouid;
	int count = 0;
	int rc = ISMRC_OK;

	ouid = (char *)alloca(sizeof(char)*33);
	/*
	 * The only case a UID exists when this function called
	 * is on Standby node.
	 */
	if (puid) {
		snprintf(ouid, 33, "%s", puid);
	} else {
		while (count < 5) {
			rc = ism_config_createObjectUID(&ouid);
			if (rc != ISMRC_OK) {
				//Generate UID error, return and system should be in maintenance mode.
				return rc;
			}

			rc = ism_config_checkDuplicatedUID(ouid);
			if (rc == ISMRC_OK)
				break;
			count++;
		}
		TRACE(9, "check UID duplication rc=%d, count=%d\n", rc, count);
	}

	if (rc != ISMRC_OK ) {
		rc = ISMRC_UUIDConfigError;
	} else {
		if (retId != NULL)
		   *retId = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),ouid);

		if (name) {
			l = snprintf(propName, propNameLen, "%s.UID.%s", item, name);
			if (l + 1 > propNameLen) {
				propNameLen = l + 1;
				propName = alloca(propNameLen);
				sprintf(propName, "%s.UID.%s", item, name);
			}
		} else {
			l = snprintf(propName, propNameLen, "%s.UID.%s", item, ouid);
			if (l + 1 > propNameLen) {
				propNameLen = l + 1;
				propName = alloca(propNameLen);
				sprintf(propName, "%s.UID.%s", item, ouid);
			}
		}
		TRACE(9, "prop=[%s] ouid=[%s]\n", propName, ouid);

		f.type = VT_String;
		f.val.s = ouid;
		ism_common_setProperty(props, propName, &f);
	}
    return rc;
}



void ism_config_convertVersionStr(const char *versionStr, ismConfigVersion_t **pversion) {
	char *nexttoken = NULL;
	char *tmpstr;
	int val = 0;

	TRACE(9, "ism_config_convertVersionStr: versionStr: %s\n", versionStr);
	if (!versionStr || *versionStr=='\0') {
		(* pversion)->version = 1;
		(* pversion)->release = 1;
		(*pversion)->mod     = 0;
		(*pversion)->fixpack = 0;
		return;
	}

	int len = strlen(versionStr);
	tmpstr = alloca(len + 1);
	memcpy(tmpstr, versionStr, len);
	tmpstr[len] = '\0';

	//trim of " Beta" from "1.2 Beta"
	char *vstr = strtok_r((char *)tmpstr, " ", &nexttoken);
	TRACE(9, "vstr: %s\n", vstr);

	nexttoken = NULL;
	char *p1 = strtok_r((char *)vstr, ".", &nexttoken);
	char *p2 = strtok_r(NULL, ".", &nexttoken);
	char *p3 = strtok_r(NULL, ".", &nexttoken);
	char *p4 = strtok_r(NULL, ".", &nexttoken);

	if (p1) {
		val = atoi(p1);
		if (val <= 0)
		    (*pversion)->version = 1;
		else
			(*pversion)->version = val;
	} else {
		(*pversion)->version = 1;
	}

	if (p2) {
		val = atoi(p2);
		if (val <= 0)
			(*pversion)->release = 1;
		else
			(*pversion)->release = val;
	} else {
		(*pversion)->release = 1;
	}

	if (p3) {
		val = atoi(p3);
		if (val <= 0)
			(*pversion)->mod = 0;
		else
			(*pversion)->mod = val;
	} else {
		(*pversion)->mod = 0;
	}

	if (p4) {
		val = atoi(p4);
		if (val <= 0)
			(*pversion)->fixpack = 0;
		else
			(*pversion)->fixpack = val;
	} else {
		(*pversion)->fixpack = 0;
	}

	TRACE(9, "Version: version %d, release %d, mod %d, fixpack %d\n",
			(*pversion)->version,
		    (*pversion)->release,
		    (*pversion)->mod,
		    (*pversion)->fixpack);
	return;
}


static void ism_config_initAdminUserData(void) {
    if ( !adminUser || !adminUserPassword ) {

        char *tmpuserstr = NULL;
        char *tmppasswdstr = NULL;

        pthread_rwlock_rdlock(&srvConfiglock);

        json_t *userObj = json_object_get(srvConfigRoot, "AdminUserID");
        if ( userObj && json_typeof(userObj) == JSON_STRING )
            tmpuserstr = (char *)json_string_value(userObj);

        json_t *passObj = json_object_get(srvConfigRoot, "AdminUserPassword");
        if ( passObj && json_typeof(passObj) == JSON_STRING )
            tmppasswdstr = (char *)json_string_value(passObj);

        pthread_rwlock_unlock(&srvConfiglock);

        if (tmpuserstr && *tmpuserstr != '\0') {
            adminUser = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpuserstr);
        }

        if (tmppasswdstr && *tmppasswdstr != '\0') {
            adminUserPassword = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmppasswdstr);
        }

    }

    return;
}

XAPI char *ism_config_getAdminUserName(void) {
    ism_config_initAdminUserData();
    return adminUser;
}

XAPI char *ism_config_getAdminUserPassword(void) {
    ism_config_initAdminUserData();
    char *decPwd = (char *)ism_security_decryptAdminUserPasswd(adminUserPassword);
    return decPwd;
}

XAPI char *ism_config_getAdminUserPasswordHash(void) {
    ism_config_initAdminUserData();
    return adminUserPassword;
}

XAPI bool ism_config_confirmAdminUserPassword2(char * password,char * encoding) {
    if (strlen(encoding) == 0) {
        return false;
    }
    else if (encoding[0] == '_')
    {
        switch (encoding[1]) {
            case '1': {
                char salt[21];
                char * end;
                strncpy(salt,encoding+3,20);
                salt[20] = '\0';
                uint64_t salt_v = strtoul(salt,&end,10);
                char hash[129];
                strncpy(hash,encoding+24,128);
                hash[128] = '\0';
                char hash2[129];
                ism_security_1wayHashAdminUserPassword(password,(char*)&salt_v,hash2);
                return strcmp(hash,hash2) == 0;
            }
            default: {
                TRACE(2,"Password encoding version too high.\n");
                return false;
            }
        }
    } else {
        TRACE(5,"WARNING: legacy password encoding in use, recomend changing AdminUserPassword.\n");
        char * decrypted = ism_config_getAdminUserPassword();
        if (decrypted != NULL) {
            return strcmp(decrypted,password) == 0;
        }
    }
    return false;
}

XAPI bool ism_config_confirmAdminUserPassword(char * password) {
    char * encoding = ism_config_getAdminUserPasswordHash();
    if ( ism_config_confirmAdminUserPassword2(password,encoding) ) {
        return true;
    }
    return false;
}

XAPI int ism_config_updateCfgFile(ism_prop_t *props, int compType) {
   int rc = ISMRC_OK;

   pthread_mutex_lock(&g_cfglock);

   ism_prop_t *cprops = compProps[compType].props;

   /* Add config data to dynamic properties list */
   int i;
   const char * pName;
   ism_field_t field;
   for (i=0; ism_common_getPropertyIndex(props, i, &pName) == 0; i++) {
       ism_common_getProperty(props, pName, &field);
       ism_common_setProperty(cprops, pName, &field);
   }

   /* Update file */
   rc = ism_config_updateFile( "server_dynamic.cfg", ISM_PROTYPE_SERVER );

   pthread_mutex_unlock(&g_cfglock);

   return rc;
}

/*
 * Create JSON string of configuration objects
 */
static int ism_config_getJSONString(json_t *objRoot, char *item, char *name, concat_alloc_t *retStr, int getConfigLock, int deleteFlag) {
    int rc = ISMRC_OK;

    if ( !item ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto FUNC_END;
    }

    if ( getConfigLock == 1 )
        pthread_rwlock_rdlock(&srvConfiglock);

    /* For ClusterMembership replicate EnableClusterMembership, ClusterName, and DiscoveryServerList */
    if ( !strcmp(item, "ClusterMembership")) {
        if (deleteFlag == 1) {
            /* Invalid action - return ISMRC_DeleteNotAllowed */
            rc = ISMRC_DeleteNotAllowed;
            ism_common_setErrorData(rc, "%s", item);
            goto FUNC_END;
        }
        /* create return node */
        json_t *retval = json_object();
        /* add version */
        json_object_set_new(retval, "Version", json_string("2.0"));
        /* create node for object */
        json_t *obj = json_object();
        /* add node for item */
        json_object_set_new(retval, item, obj);
        /* create node for instance */
        json_t *instObj = json_object();

        /* Get required data from existing configuration and add in the instance node */
        json_t *cluster = NULL;
        if ( objRoot ) {
            cluster = objRoot;
        } else {
            json_t *itemroot = json_object_get(srvConfigRoot, item);
            cluster = json_object_get(itemroot, "cluster");
        }

        json_t *clusterName = json_object_get(cluster, "ClusterName");
        json_t *enabledCM = json_object_get(cluster, "EnableClusterMembership");
        json_t *dsList = json_object_get(cluster, "DiscoveryServerList");

        if ( clusterName ) {
            json_object_set_new(instObj, "ClusterName", clusterName);
        } else {
            json_object_set_new(instObj, "ClusterName", json_string(""));
        }
        if ( enabledCM ) {
            json_object_set_new(instObj, "EnableClusterMembership", enabledCM);
        } else {
            json_object_set_new(instObj, "EnableClusterMembership", json_false());
        }
        if ( dsList ) {
            json_object_set_new(instObj, "DiscoveryServerList", dsList);
        } else {
            json_object_set_new(instObj, "DiscoveryServerList", json_string(""));
        }

        /* add instance */
        json_object_set_new(obj, "cluster", instObj);

        /* dump object in buffer and return */
        char *buf = json_dumps(retval, 0);
        if ( buf ) {
            ism_common_allocBufferCopyLen(retStr, buf, strlen(buf));
            ism_common_free_raw(ism_memory_admin_misc,buf);
        }
        goto FUNC_END;

    } else  if ( !strcmp(item, "TrustedCertificate") || !strcmp(item, "ClientCertificate") ||
                 !strcmp(item, "TopicMonitor") || !strcmp(item, "ClusterRequestedTopics")) {

        /* create return node */
        json_t *retval = json_object();
        /* add version */
        json_object_set_new(retval, "Version", json_string("2.0"));

        if ( deleteFlag == 1 ) {
            json_object_set_new(retval, "Delete", json_true());
        }

        /* Append node for instance */
        if ( objRoot ) {
            /* create node for object */
            json_t *obj = json_array();
            /* add node for item */
            json_object_set_new(retval, item, obj);
            json_array_append(obj, objRoot);
        } else {
            json_t *itemroot = json_object_get(srvConfigRoot, item);
            json_object_set(retval, item, itemroot);
        }

        /* dump object in buffer and return */
        char *buf = json_dumps(retval, 0);
        if ( buf ) {
            ism_common_allocBufferCopyLen(retStr, buf, strlen(buf));
            ism_common_free_raw(ism_memory_admin_misc,buf);
        }
        goto FUNC_END;

    }

    /* check for invalid delete action */
    if ( deleteFlag == 1 && (!strcmp(item, "HighAvailability") || !strcmp(item, "AdminEndpoint") || !strcmp(item, "LDAP"))) {
        /* Invalid action - return ISMRC_DeleteNotAllowed */
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(rc, "%s", item);
        goto FUNC_END;
    }

    /* If objRoot is specified - use it - otherwise get the object from current configuration */
    if ( objRoot ) {
        /* add version to the configuration */
        json_t *retval = json_object();
        /* add version */
        json_object_set_new(retval, "Version", json_string("2.0"));

        if ( deleteFlag == 1 ) {
            json_object_set_new(retval, "Delete", json_true());
        }

        if ( !name ) {
        	json_object_set_new(retval, item, objRoot);
        } else {
			/* create node for object */
			json_t *obj = json_object();
			/* add node for item */
			json_object_set_new(retval, item, obj);
			/* add object */
			json_object_set_new(obj, name, objRoot);
        }

        /* dump object in buffer and return */
        char *buf = json_dumps(retval, 0);
        if ( buf ) {
            ism_common_allocBufferCopyLen(retStr, buf, strlen(buf));
            ism_common_free_raw(ism_memory_admin_misc,buf);
        } else {
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }

    } else {

        json_t *itemroot = json_object_get(srvConfigRoot, item);

        if ( itemroot ) {
            if ( name ) {
                json_t *instroot = json_object_get(itemroot, name);
                if ( instroot ) {
                    /* create return node */
                    json_t *retval = json_object();
                    /* add version */
                    json_object_set_new(retval, "Version", json_string("2.0"));

                    if ( deleteFlag == 1 ) {
                        json_object_set_new(retval, "Delete", json_true());
                    }

                    /* create node for object */
                    json_t *obj = json_object();
                    /* add node for item */
                    json_object_set_new(retval, item, obj);
                    /* add instance */
                    json_object_set_new(obj, name, instroot);

                    /* dump object in buffer and return */
                    char *buf = json_dumps(retval, 0);
                    if ( buf ) {
                        ism_common_allocBufferCopyLen(retStr, buf, strlen(buf));
                        ism_common_free_raw(ism_memory_admin_misc,buf);
                    } else {
                        rc = ISMRC_Error;
                        ism_common_setError(rc);
                    }
                } else {
                    rc = ISMRC_NotFound;
                    ism_common_setError(rc);
                    goto FUNC_END;
                }
            } else {
                /* create return node */
                json_t *retval = json_object();
                /* add version */
                json_object_set_new(retval, "Version", json_string("2.0"));

                if ( deleteFlag == 1 ) {
                    json_object_set_new(retval, "Delete", json_true());
                }

                /* add object */
                json_object_set_new(retval, item, itemroot);

                /* dump object in buffer and return */
                char *buf = json_dumps(retval, 0);
                if ( buf ) {
                    ism_common_allocBufferCopyLen(retStr, buf, strlen(buf));
                    ism_common_free_raw(ism_memory_admin_misc,buf);
                }
            }
        }

    }

FUNC_END:

    if (rc)
        ism_common_setError(rc);

    if ( getConfigLock == 1 )
        pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

/*
 * Dump dynamic configuration data in a file in JSON format
 * - used on primary node
 */
XAPI uint32_t ism_config_dumpJSON(const char * filepath) {
    uint32_t rc = ISMRC_OK;
    int i;
    FILE *dest = NULL;
    int contentAdded = 0;

    /* open file to dump configuration items */
    dest = fopen(filepath, "w");
    if( dest == NULL ) {
        TRACE(2,"Can not open destination file '%s'. rc=%d\n", filepath, errno);
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    /* Loop thru list of configuration items marked for sync */
    for (i=0; i < MAX_SYNC_CONFIG_ITEMS; i++) {
        char tbuf[8192];
        concat_alloc_t output_buffer = { tbuf, sizeof(tbuf), 0, 0 };
        memset(tbuf, '0', 8192);
        int getConfigLock = 1;
        int deleteFlag = 0;

        /*
         * avoid non-sync object. such as HA to be dumped.
         */
        if (syncProps[i].syncData == 0)
            continue;

        TRACE(1, "DEBUG: Add item: %s\n", syncProps[i].itemName);

        int rc1 = ism_config_getJSONString(NULL, syncProps[i].itemName, NULL, &output_buffer, getConfigLock, deleteFlag);

        if ( rc1 == ISMRC_OK && output_buffer.used > 0 ) {
            char *buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,398),output_buffer.used + 1);
            memcpy(buffer, output_buffer.buf, output_buffer.used);
            buffer[output_buffer.used] = 0;
            fprintf(dest, "%s\n", buffer);
            ism_common_free(ism_memory_admin_misc,buffer);
            contentAdded = 1;
        }

        if (output_buffer.inheap)
            ism_common_freeAllocBuffer(&output_buffer);

    }

    fclose(dest);

    if ( contentAdded == 0 )
        rc = ISMRC_PropertyNotFound;

    if (rc)
        ism_common_setError(rc);

    return rc;
}

XAPI int ism_config_updateStandbyNode(json_t *objval, int comptype, char *item, char *name, int getConfigLock, int deleteFlag) {
    int rc = ISMRC_OK;
    /* If HA is enabled and node is Primary - update Standby */
    if ( ism_ha_admin_isUpdateRequired() == 1 ) {
        char *comp = compProps[comptype].name;
        /* check if the item update is required */
        if ( ism_config_getSyncFlag(comp, item) == 1 ) {
            TRACE(7, "ism_config_updateStandbyNode: Update Standby Node: item:%s name:%s\n", item?item:"NULL", name?name:"NULL");
            int flag = ism_config_getUpdateCertsFlag(item);
            char tbuf[4098];
            concat_alloc_t outbuf = { tbuf, sizeof(tbuf), 0, 0 };
            rc = ism_config_getJSONString(objval, item, name, &outbuf, getConfigLock, deleteFlag);
            if ( rc == ISMRC_OK ) {
                rc = ism_ha_admin_update_standby(outbuf.buf, outbuf.used, flag);
            }
        } else {
            TRACE(7, "ism_config_updateStandbyNode: Update Standby Node is not required: item:%s name:%s\n", item?item:"NULL", name?name:"NULL");
        }
    } else {
        TRACE(9, "ism_config_updateStandbyNode: Either HA is not enabled or Node is a standby node. By pass Standby update.\n");
    }

    if ( rc != ISMRC_OK )
        ism_common_setError(rc);

    return rc;
}

/**
 * set initial configuration items *
 */
static int ism_config_json_setGlobalConfigVariables(void) {

    pthread_rwlock_rdlock(&srvConfiglock);

    char *secL = NULL;
    json_t *secLObj = json_object_get(srvConfigRoot, "SecurityLog");
    if ( secLObj && json_typeof(secLObj) == JSON_STRING ) {
        secL = (char *)json_string_value(secLObj);

        if ( secL ) {
            int newval = AuxLogSetting_Normal - 1;
            if ( !strcmpi(secL, "MIN"))
                newval = AuxLogSetting_Min -1;
            else if ( !strcmpi(secL, "NORMAL"))
                newval = AuxLogSetting_Normal -1;
            else if ( !strcmpi(secL, "MAX"))
                newval = AuxLogSetting_Max -1;
            ism_security_setAuditControlLog(newval);
        }
    }

    fipsEnabled = 0;
    json_t *fipsObj = json_object_get(srvConfigRoot, "FIPS");
    if ( fipsObj && json_typeof(fipsObj) == JSON_TRUE ) fipsEnabled = 1;

    mqconnEnabled = 0;
    json_t *mqcObj = json_object_get(srvConfigRoot, "MQConnectivityEnabled");
    if ( mqcObj && json_typeof(mqcObj) == JSON_TRUE ) mqconnEnabled = 1;

    sslUseBuffersPool = ism_common_getIntProperty(ism_common_getConfigProperties(), "SslUseBuffersPool", 0);

    pthread_rwlock_unlock(&srvConfiglock);

    return 0;
}

/**
 * Set mqconnEnabled flag
 */
XAPI void ism_config_setMQConnectivityEnabledFlag(int flag, int wait) {
    pthread_spin_lock(&configSpinLock);
    mqconnEnabled = flag;
    pthread_spin_unlock(&configSpinLock);
    if ( wait == 1 )
        ism_common_sleep(6000000);
}

/*
 * Set adminMode
 */
XAPI void ism_config_setAdminMode(int mode) {
    pthread_spin_lock(&configSpinLock);
    adminMode = mode;
    pthread_spin_unlock(&configSpinLock);
    /* Update status file */
    ism_admin_dumpStatus();
}

/*
 * Get adminMode
 */
XAPI int ism_config_getAdminMode(void) {
	int retval = 0;
    pthread_spin_lock(&configSpinLock);
    retval = adminMode;
    pthread_spin_unlock(&configSpinLock);
    return retval;
}


