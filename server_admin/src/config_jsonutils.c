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
 * This file contains JSON parse functions.
 */

#define TRACE_COMP Admin

#include "validateInternal.h"
#include "configInternal.h"

extern int ism_admin_init_stop(int mode, ism_http_t *http);
extern int ism_admin_startPlugin(void);
extern int ism_admin_stopPlugin(void);
extern int ism_admin_start_mqc_channel(void);
extern int ism_admin_stop_mqc_channel(void);
extern int ism_admin_startSNMP(void);
extern int ism_admin_stopSNMP(void);
extern json_t * ism_config_getSingleton(const char *object);
extern int ism_admin_restartTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata);
extern int ism_admin_isClusterDisabled(void);
extern void ism_config_json_resetAdminModeRC(void);
extern int ism_admin_setMaintenanceMode(int errorRC, int restartFlag);
extern char * ism_admin_getLicenseAccptanceTags(int *licenseStatus);
extern int ism_admin_initRestart(int delay);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
extern const char * configDir;
extern int adminInitError;
extern int licenseIsAccepted;


int resetConfig = 0;

/*
 *
 * parse a json formated file into a ism_json_parse_t object.
 *
 * @param  name                   The full path of a json formated file
 *
 * Returns ism_json_parse_t object on success, NULL on error
 * */
XAPI ism_json_parse_t * ism_config_json_loadJSONFromFile(const char * name) {
    FILE * f = NULL;
    int  len;
    char * buf = NULL;
    int    bread;

    int    rc = ISMRC_OK;
    ism_json_parse_t *parseobj = NULL;
    int buflen = 0;

    if (name == NULL) {
    	rc = ISMRC_ArgNotValid;
    	ism_common_setErrorData(ISMRC_ArgNotValid, "%s", name?name:"null");
        goto CONFIG_END;
    }

    /*
     * Open the file and read it into memory
     */
    f = fopen(name, "rb");
    if (!f) {
		TRACE(9, "%s: The configuration file is not found: %s.\n", __FUNCTION__, name);
		goto CONFIG_END;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,294),len+2);
    if (!buf) {
        TRACE(3, "Unable to allocate memory for config file: %s.\n", name);
        rc = ISMRC_AllocateError;
        ism_common_setError(ISMRC_NotFound);
        goto CONFIG_END;
    }
    rewind(f);

    /*
     * Read the file
     */
    bread = fread(buf, 1, len, f);
    buf[bread] = 0;
    if (bread != len) {
        TRACE(3, "The configuration file cannot be read: %s.\n", name);
        rc = ISMRC_Error;
        ism_common_setError(ISMRC_Error);
        goto CONFIG_END;
    }

    /*
     * Parse the JSON file
     */
    parseobj = (ism_json_parse_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,295),1, sizeof(ism_json_parse_t));
    memset(parseobj, 0, sizeof(ism_json_parse_t));

    /* Parse policy buffer and create rule definition */
    buflen = strlen(buf);

    parseobj->src_len = buflen;
    parseobj->source = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,296),buflen+1);
    memcpy(parseobj->source, buf, buflen);
    parseobj->source[buflen] = 0;   /* null terminate for debug */
    parseobj->options = JSON_OPTION_COMMENT;   /* Allow C style comments */
    rc = ism_json_parse(parseobj);
    if (rc) {
		TRACE(3, "%s: The configuration file is not valid JSON encoding: File=%s Error=%d Line=%d\n",
				__FUNCTION__, name, rc, parseobj->line);
		ism_common_setError(rc);
    }

CONFIG_END:
    if (f)  fclose(f);
    if (buf) ism_common_free(ism_memory_admin_misc,buf);
	TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

	if (rc) {
	   /* Clean up */
		if (parseobj && parseobj->free_ent)
			ism_common_free(ism_memory_utils_parser,parseobj->ent);
		parseobj = NULL;
	}

	return parseobj;
}

int ism_config_json_updateConfigSet(configSet_t *configSet, char *component, ism_prop_t *props) {
	ism_field_t var;
	const char *item;
	int rc = ISMRC_OK;


	item = ism_common_getStringProperty(props, "Item");
	if (!item) {
		TRACE(3, "%s: the properties has no item entry, it is not valid.\n", __FUNCTION__ );
		rc = ISMRC_BadRESTfulRequest;
	    return rc;
	}

	//initial component type property list array if it has not been initialed.
	int comptype = ism_config_getCompType(component);
	if (configSet[comptype].plist == NULL) {
		configSet[comptype].plist = (ism_prop_t **)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,299),MAX_JSON_ENTRIES*sizeof(ism_prop_t *));
	}

	//add default name to special objects.
	if ( item && !strcmpi(item, "HighAvailability")) {
		var.type = VT_String;
		var.val.s = "haconfig";
		ism_common_setProperty(props, "Name", &var);
	} else if ( item && !strcmpi(item, "LDAP")) {
		var.type = VT_String;
		var.val.s = "ldapconfig";
		ism_common_setProperty(props, "Name", &var);
	} else if ( item && !strcmpi(item, "ClusterMembership")) {
		var.type = VT_String;
		var.val.s = "cluster";
		ism_common_setProperty(props, "Name", &var);
	} else if ( item && !strcmpi(item, "AdminEndpoint")) {
		var.type = VT_String;
		var.val.s = "AdminEndpoint";
		ism_common_setProperty(props, "Name", &var);
	} else if ( item && !strcmpi(item, "Syslog")) {
		var.type = VT_String;
		var.val.s = "Syslog";
		ism_common_setProperty(props, "Name", &var);
	}
	configSet[comptype].plist[configSet[comptype].ncount] = props;
	configSet[comptype].ncount++;

	return rc;
}


/*
 *
 * parse http service POST JSON payload to start a service - MQConnectivity, SNMP, Plugin
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
int ism_config_json_parseServiceStartPayload(ism_http_t *http) {
    int rc = ISMRC_OK;
    const char * repl[5];
    int replSize = 0;
    char *payload = NULL;

    char * content = NULL;
    int    content_len = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[2000];

    /* Parse input string and create JSON object */
    if (http->content_count > 0) {
        content = http->content[0].content;
        content_len = http->content[0].content_len;

        payload = alloca(content_len + 1);
        memcpy(payload, content, content_len);
        payload[content_len] = '\0';
    }

    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) content;
    parseobj.src_len = content_len;
    ism_json_parse(&parseobj);
    if (parseobj.rc) {
        LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.", payload?payload:"null", parseobj.rc );
        ism_common_setErrorData(6001, "%s%d", payload?payload:"null", parseobj.rc);
        rc = ISMRC_ArgNotValid;
        goto PARSEPAYLOD_END;
    }

    int entloc = 1;
    int proctype = 0;
    int count = 0;

    //parse the payload
    while (entloc < parseobj.ent_count) {
        ism_json_entry_t * ent = parseobj.ent+entloc;

        //add entries into props
        switch(ent->objtype) {
        case JSON_String:  /* JSON string, value is UTF-8  */
            if (ent->name && !strcmp(ent->name, "Service")) {
                if ( ent->value && (!strcmp(ent->value, "Plugin") || !strcmp(ent->value, "plugin"))) {
                    proctype = ISM_PROTYPE_PLUGIN;
                    count += 1;
                } else if ( ent->value && (!strcmp(ent->value, "SNMP") || !strcmp(ent->value, "snmp"))) {
                    proctype = ISM_PROTYPE_SNMP;
                    count += 1;
                } else if ( ent->value && (!strcmp(ent->value, "MQConnectivity") || !strcmp(ent->value, "mqconnectivity"))) {
                    proctype = ISM_PROTYPE_MQC;
                    count += 1;
                } else if ( ent->value && (!strcmp(ent->value, "Server") || !strcmp(ent->value, "imaserver"))) {
                    if ( serverState == ISM_SERVER_STOPPING ) {

                    }
                } else {
                    rc = ISMRC_PropertyRequired;
                    ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
                    goto PARSEPAYLOD_END;
                }
                if ( count > 1 ) {
                    rc = ISMRC_PropertyRequired;
                    ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
                    goto PARSEPAYLOD_END;
                }
            } else {

                TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
                goto PARSEPAYLOD_END;
            }

            entloc++;
            break;
        case JSON_Integer: /* Number with no decimal point             */
        case JSON_Number:  /* Number which is too big or has a decimal */
        case JSON_Object:  /* JSON object, count is number of entries  */
        case JSON_Array:   /* JSON array, count is number of entries   */
        case JSON_Null:
            TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
            goto PARSEPAYLOD_END;
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
            if (!ent->name || strcmp(ent->name, "CleanStore") != 0) {
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
                goto PARSEPAYLOD_END;
            }
            entloc++;
            break;
        }
    }

    TRACE(5, "%s: start the service./n", __FUNCTION__);

    if ( proctype == ISM_PROTYPE_PLUGIN ) {
        rc = ism_admin_startPlugin();
    } else if ( proctype == ISM_PROTYPE_SNMP) {
    	rc = ism_admin_startSNMP();
    } else if ( proctype == ISM_PROTYPE_MQC ) {
        rc = ism_admin_start_mqc_channel();
    } else {
        rc = ISMRC_NotImplemented;
        ism_common_setError(rc);
        goto PARSEPAYLOD_END;
    }

PARSEPAYLOD_END:
    if (rc)
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}



/*
 *
 * parse http service POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
//imaserver: { "Service":"imaserver","Maintenance":"start|stop","CleanStore":true/false }
//mqconnectivity
int ism_config_json_parseServiceRestartPayload(ism_http_t *http) {
	int rc = ISMRC_OK;
	const char * repl[5];
    int replSize = 0;
    char *payload = NULL;

    char * content = NULL;
    int    content_len = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[2000];

    /* Parse input string and create JSON object */
	if (http->content_count > 0) {
		content = http->content[0].content;
		content_len = http->content[0].content_len;

		payload = alloca(content_len + 1);
		memcpy(payload, content, content_len);
		payload[content_len] = '\0';
	}

	parseobj.ent = ents;
	parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
	parseobj.source = (char *) content;
	parseobj.src_len = content_len;
	ism_json_parse(&parseobj);
    if (parseobj.rc) {
        LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.", payload?payload:"null", parseobj.rc );
        ism_common_setErrorData(6001, "%s%d", payload?payload:"null", parseobj.rc);
        rc = ISMRC_ArgNotValid;
        goto PARSEPAYLOD_END;
    }

	int entloc = 1;
	int proctype = 1000;
	int count = 0;
	int isService = -1;
    int isMaintenance = -1;
    int isCleanstore = 0;

    //parse the payload
	while (entloc < parseobj.ent_count) {
		ism_json_entry_t * ent = parseobj.ent+entloc;

		//add entries into props
		switch(ent->objtype) {
		case JSON_String:  /* JSON string, value is UTF-8  */
			if (ent->name && !strcmp(ent->name, "Service")) {
                if ( ent->value && !strcmp(ent->value, "Server")) {
                    proctype = ISM_PROTYPE_SERVER;
                    count += 1;
                } else if ( ent->value && !strcmp(ent->value, "Plugin")) {
                    proctype = ISM_PROTYPE_PLUGIN;
                    count += 1;
                } else if ( ent->value && !strcmp(ent->value, "SNMP")) {
                    proctype = ISM_PROTYPE_SNMP;
                    count += 1;
                } else if ( ent->value && !strcmp(ent->value, "MQConnectivity")) {
                    proctype = ISM_PROTYPE_MQC;
                    count += 1;
                } else {
                    rc = ISMRC_PropertyRequired;
                    ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
                    goto PARSEPAYLOD_END;
                }
				if ( count > 1 ) {
					rc = ISMRC_PropertyRequired;
					ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
					goto PARSEPAYLOD_END;
				} else {
				    isService = 1;
				}
			} else if (ent->name && !strcmp(ent->name, "Maintenance")) {
				if (ent->value) {
					if (!strcmpi(ent->value, "start")) {
						isMaintenance = 1;
					} else if (!strcmpi(ent->value, "stop")) {
						isMaintenance = 0;
					} else {
						TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
						rc = ISMRC_PropertyRequired;
						ism_common_setErrorData(rc, "%s%s", ent->name, ent->value);
						goto PARSEPAYLOD_END;
					}
				} else {
					rc = ISMRC_PropertyRequired;
					ism_common_setErrorData(rc, "%s%s", ent->name, "null");
					goto PARSEPAYLOD_END;
				}
            } else if (ent->name && !strcmp(ent->name, "Reset")) {
                    if (ent->value && !strcmp(ent->value, "config"))
                        resetConfig = 1;
                    else {
                            rc = ISMRC_PropertyRequired;
                            ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
                            goto PARSEPAYLOD_END;
                    }
            } else {

				TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
				rc = ISMRC_ArgNotValid;
				ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
				goto PARSEPAYLOD_END;
			}

			entloc++;
			break;
		case JSON_Integer: /* Number with no decimal point             */
		case JSON_Number:  /* Number which is too big or has a decimal */
		case JSON_Object:  /* JSON object, count is number of entries  */
		case JSON_Array:   /* JSON array, count is number of entries   */
		case JSON_Null:
		    TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
		    rc = ISMRC_ArgNotValid;
		    ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
		    goto PARSEPAYLOD_END;
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
            if (ent->name && !strcmp(ent->name, "CleanStore")) {
            	isCleanstore = ent->objtype == JSON_True;

            	/* clean store is not allowed if cluster is enabled and active */
            	if ( ism_admin_isClusterDisabled() == 0 ) {
            		rc = ISMRC_UpdateNotAllowed;
            		ism_common_setErrorData(rc, "%s%s", "ClusterMembership", "CleanStore");
            		goto PARSEPAYLOD_END;
            	}
            }else {
            	rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
                goto PARSEPAYLOD_END;
            }
            entloc++;
            break;
        }
	}

	/* clean store is implied with a resetConfig request */
	if (resetConfig && isCleanstore != 0) {
	    isCleanstore = 0;
	}

    if ( (isService == -1) ||
         (isMaintenance != -1 && isCleanstore) ||
         (resetConfig && isMaintenance != -1) )
    {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", payload);
        goto PARSEPAYLOD_END;
    }

    if ( proctype == ISM_PROTYPE_SERVER ) {
        TRACE(1, "Set Timer Task to Restart the server: Maintenance:%d CleanStore:%d.\n", isMaintenance, isCleanstore );

        adminRestartTimerData_t *userdata = (adminRestartTimerData_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,300),sizeof(adminRestartTimerData_t));
        userdata->isCleanstore = isCleanstore;
        userdata->isMaintenance = isMaintenance;
        userdata->isService = isService;

        /* Set server state and reset AdminModeRC */
        serverState = ISM_SERVER_STOPPING;
        ism_config_json_resetAdminModeRC();

        ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) ism_admin_restartTimerTask, (void *)userdata, 3, 60, TS_SECONDS);
    } else {
        rc = ism_admin_restartService(http, isService, isMaintenance, isCleanstore, proctype);
    }

PARSEPAYLOD_END:

    if (rc) {
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
        resetConfig = 0;
    }
    else
        ism_confg_rest_createErrMsg(http, 6011, repl, replSize);


    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}


/*
 *
 * parse http service POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
//imaserver: { "Service":"imaserver","Maintenance":"start|stop","CleanStore":true/false }
//mqconnectivity
int ism_config_json_parseServiceStopPayload(ism_http_t *http) {
	int rc = ISMRC_OK;
	const char * repl[5];
    int replSize = 0;
    char *payload = NULL;

    char * content = NULL;
    int    content_len = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[2000];

    /* Parse input string and create JSON object */
	if (http->content_count > 0) {
		content = http->content[0].content;
		content_len = http->content[0].content_len;

		payload = alloca(content_len + 1);
		memcpy(payload, content, content_len);
		payload[content_len] = '\0';
	}

	parseobj.ent = ents;
	parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
	parseobj.source = (char *) content;
	parseobj.src_len = content_len;
	ism_json_parse(&parseobj);
    if (parseobj.rc) {
    	LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.", payload?payload:"null", parseobj.rc );
        ism_common_setErrorData(6001, "%s%d", payload?payload:"null", parseobj.rc);
        rc = ISMRC_ArgNotValid;
        goto PARSEPAYLOD_END;
    }

	int entloc = 1;
	int proctype = 1000;
	int count = 0;

    //parse the payload
	while (entloc < parseobj.ent_count) {
		ism_json_entry_t * ent = parseobj.ent+entloc;

		//add entries into props
		switch(ent->objtype) {
		case JSON_String:  /* JSON string, value is UTF-8  */
			if (ent->name && !strcmp(ent->name, "Service")) {
			    if ( ent->value && (!strcmp(ent->value, "imaserver") || !strcmp(ent->value, "Server"))) {
			        proctype = ISM_PROTYPE_SERVER;
			        count += 1;
			    } else if ( ent->value && (!strcmp(ent->value, "plugin") || !strcmp(ent->value, "Plugin"))) {
			        proctype = ISM_PROTYPE_PLUGIN;
			        count += 1;
			    } else if ( ent->value && (!strcmp(ent->value, "snmp") || !strcmp(ent->value, "SNMP"))) {
			        proctype = ISM_PROTYPE_SNMP;
			        count += 1;
			    } else if ( ent->value && (!strcmp(ent->value, "mqconnectivity") || !strcmp(ent->value, "MQConnectivity"))) {
			        proctype = ISM_PROTYPE_MQC;
			        count += 1;
			    } else {
					rc = ISMRC_PropertyRequired;
					ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
					goto PARSEPAYLOD_END;
				}

			    if ( count == 0 || count > 1 ) {
			        rc = ISMRC_ArgNotValid;
			        ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
			        goto PARSEPAYLOD_END;
			    }

			} else {
				TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
				rc = ISMRC_ArgNotValid;
				ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
				goto PARSEPAYLOD_END;
			}

			entloc++;
			break;
		case JSON_Integer: /* Number with no decimal point             */
		case JSON_Number:  /* Number which is too big or has a decimal */
		case JSON_Object:  /* JSON object, count is number of entries  */
		case JSON_Array:   /* JSON array, count is number of entries   */
		case JSON_Null:
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
        default:
			rc = ISMRC_ArgNotValid;
			ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
			goto PARSEPAYLOD_END;
        }
	}

	TRACE(5, "Process admin request to stop the service. Proctype=%d\n", proctype);
	int mode = 0;   /* stop mode */
	if ( proctype == ISM_PROTYPE_SERVER ) {
	    rc = ism_admin_init_stop(mode, http);
	} else if ( proctype == ISM_PROTYPE_PLUGIN ) {
	    rc = ism_admin_stopPlugin();
	} else if ( proctype == ISM_PROTYPE_SNMP) {
		rc = ism_admin_stopSNMP();
	} else if ( proctype == ISM_PROTYPE_MQC ) {
        rc = ism_admin_stop_mqc_channel();
    } else {
	    rc = ISMRC_NotImplemented;
	    ism_common_setError(rc);
	    goto PARSEPAYLOD_END;
	}

PARSEPAYLOD_END:
    if (rc)
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

/*
 *
 * parse http POST payload.
 *
 * @param  json_t  *post
 * @param  int     getLock *
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 */
int ism_config_json_processLicensePayload(json_t *post, int getLock) {
    int rc = ISMRC_OK;
    int accept = 4;
    char *usage = NULL;
    int licenseAccepted = 0;
    int adminRestartFlag = 0;
	int updateLicenseFile = 0;
	char buffer[1024];
	int licenseStatus = 0;
	char cfilepath[1024];
	char *licenseType = NULL;

    if ( !post ) {
    	rc = ISMRC_NullPointer;
    	ism_common_setError(rc);
    	return rc;
    }

    if ( getLock == 1 ) pthread_rwlock_rdlock(&srvConfiglock);

    json_t *acceptObject = json_object_get(post, "Accept");
    json_t *licenseObject = json_object_get(post, "LicensedUsage");

	/* get current config from accepted.json file in configuration directory */
	sprintf(cfilepath, IMA_SVR_INSTALL_PATH "/config/accepted.json");
	licenseType= ism_admin_getLicenseAccptanceTags(&licenseStatus);
	if ( licenseType == NULL ) {
#ifdef ACCEPT_LICENSE
		licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Developers");
		licenseStatus = 4;
		updateLicenseFile = 1;
#else
        licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Production");
        licenseStatus = 5;
        updateLicenseFile = 1;
#endif
	}

    /* process new config */
    if ( licenseObject ) {
        int jtype = json_typeof(licenseObject);
        int nullType = 0;
        if ( jtype == JSON_NULL ) {
        	nullType = 1;
        } else if ( jtype != JSON_STRING ) {
    		rc = ISMRC_BadPropertyType;
    		ism_common_setErrorData(rc, "%s%s%s%s", "LicensedUsgae", "null", "null", ism_config_json_typeString(jtype));
            goto LICENSEPOST_END;
        }

        if ( nullType == 1 ) {
        	usage = "Developers";
        } else {
            usage = (char *)json_string_value(licenseObject);
        }

        if ( usage && (!strcmp(usage, "Developers") || !strcmp(usage, "Non-Production") || !strcmp(usage, "Production") || !strcmp(usage, "IdleStandby"))) {
            /* Invoke script to copy TAG file */
            char *ltype = "NONPROD";
            if ( !strcmp(usage, "Production") ) {
            	ltype = "PROD";
            } else if ( !strcmp(usage, "IdleStandby") ) {
               	ltype = "STANDBY";
            } else if ( !strcmp(usage, "Developers") ) {
               	ltype = "DEVELOPERS";
            }

            /* Set TAG files */

			char *version = (char *)ism_common_getVersion();
			char *scrpath = IMA_SVR_INSTALL_PATH "/bin/setILMTTagFile.sh";

			pid_t pid = fork();
			if (pid < 0){
				perror("fork failed");
				return ISMRC_Error;
			}
			if (pid == 0){
				execl(scrpath, "setILMTTagFile.sh", ltype, version, NULL);
				int urc = errno;
				TRACE(1, "Unable to run setILMTTagFile.sh: errno=%d error=%s\n", urc, strerror(urc));
				_exit(1);
			}
			int st;
			waitpid(pid, &st, 0);
			int res = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
			if (res)  {
				TRACE(3, "%s: failed to Set ILMT Tag file: LicenseType:%s  Version:%s\n", __FUNCTION__, ltype, version);
				rc = ISMRC_ILMTTagUpdatetError;
				ism_common_setErrorData(rc, "%s%s", ltype, version);
				goto LICENSEPOST_END;
			}

        } else {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", "LicensedUsage", usage?usage:"NULL");
            goto LICENSEPOST_END;
        }
    }

    if ( acceptObject ) {
        int jtype = json_typeof(acceptObject);
        if ( jtype != JSON_TRUE ) {
    		rc = ISMRC_BadPropertyType;
    		ism_common_setErrorData(rc, "%s%s%s%s", "Accept", "null", "null", ism_config_json_typeString(jtype));
            goto LICENSEPOST_END;
        }

        licenseAccepted = 1;
    }

	/* checked with passed value */
	if ( !usage ) {
		usage = licenseType;
	}

	/* if usage is same as before, compare old and new accepted flag */
	if ( !strcmp(usage, licenseType)) {

		if ( licenseStatus == 5 && updateLicenseFile == 0 ) {
			goto LICENSEPOST_END;
		}

		/* check if license is accepted */
	    if ( licenseAccepted == 1 ) {
			accept = 5;
			updateLicenseFile = 1;
			adminRestartFlag = 1;
	    } else {
	    	accept = 4;
	    	updateLicenseFile = 1;
	    	licenseIsAccepted = 0;
	    	adminInitError = ISMRC_LicenseError;
	    }

	} else {

		/* check if license is accepted */
        if ( licenseAccepted == 1 ) {
		    accept = 5;
		    updateLicenseFile = 1;
		    adminRestartFlag = 1;
    	} else {
    		accept = 4;
    		updateLicenseFile = 1;
    		adminInitError = ISMRC_LicenseError;
    		licenseIsAccepted = 0;
    	}
    }


    /* Update accepted.json file in configuration directory */
    if ( updateLicenseFile == 1 ) {

		sprintf(buffer, "{ \"Status\": %d, \"Language\":\"en\", \"LicensedUsage\":\"%s\" }", accept, usage);
		TRACE(5, "Update license file: LicenseType:%s AcceptTag:%d\n", usage, accept);

		errno = 0;
		FILE *dest = NULL;
		dest = fopen(cfilepath, "w");
		if ( dest ) {
			fprintf(dest, "%s", buffer);
			fclose(dest);
		} else {
			rc = ISMRC_FileUpdateError;
			ism_common_setErrorData(rc, "%s%d", cfilepath, errno);
			if ( licenseType ) ism_common_free(ism_memory_admin_misc,licenseType);
			goto LICENSEPOST_END;
		}
    }

    /* check if restart flag or admin mode needs to be set */
    if ( adminRestartFlag == 1 ) {
    	/* schedule restart of server */
    	rc = ism_admin_initRestart(5);
    	licenseIsAccepted = 1;
    }

LICENSEPOST_END:

    if ( getLock == 1 ) pthread_rwlock_unlock(&srvConfiglock);

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}


/*
 *
 * parse a ism_json_parse_t object and call related APIs.
 *
 * @param  parseobj        ism_json_parse_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
XAPI int ism_config_json_parseConfig(ism_json_parse_t * parseobj) {
	int rc = ISMRC_ArgNotValid;
	int entloc = 1;

	while (entloc < parseobj->ent_count) {
		ism_json_entry_t * ent = parseobj->ent+entloc;
		switch(ent->objtype) {
		/* Ignore simple objects */
		case JSON_String:  /* JSON string, value is UTF-8  */
			/* Process runtime changes */
			entloc++;
			break;
		case JSON_Integer: /* Number with no decimal point             */
			/* Process runtime changes */
			entloc++;
			break;
		case JSON_Number:  /* Number which is too big or has a decimal */
		case JSON_True:    /* JSON true, value is not set              */
		case JSON_False:   /* JSON false, value is not set             */
		case JSON_Null:
			entloc++;
			break;
		case JSON_Object:  /* JSON object, count is number of entries  */
			entloc += ent->count + 1;
			break;
		case JSON_Array:   /* JSON array, count is number of entries   */
			if (ent->name) {
				if (!strcmpi(ent->name, "ClientSetDelete")) {
			        rc = ism_config_json_createClientSetConfig(parseobj, entloc);
				} else {
                    TRACE(5, "The JSON array with name: %s is not supported.\n", ent->name);
				}
			} else {
				TRACE(8, "The JSON array has no name.\n");
			}
			entloc += ent->count + 1;
			break;
		}
	}

	TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
	return rc;
}

/*
 *
 * parse a ism_json_parse_t object for DeleteClientSet and call clientSet APIs.
 *
 * @param  parseobj         ism_json_parse_t object
 * @param  where            the location of current json object.
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
XAPI int ism_config_json_createClientSetConfig(ism_json_parse_t * parseobj, int where) {
    int endloc;
    int   rc = ISMRC_OK;

    /*Sanity check*/
    if (!parseobj) {
    	rc = ISMRC_NullPointer;
    	ism_common_setError(ISMRC_NullPointer);
        goto CLIENTSET_END;
    }
    if (where > parseobj->ent_count) {
		rc = ISMRC_ArgNotValid;
		ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "where" );
		goto CLIENTSET_END;
	}

    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    //where++;

    while (where < endloc) {
        if (ent->objtype == JSON_Object ) {
            char *ClientID = ism_config_validate_getAttr("ClientID", parseobj, where);
            char *Retained = ism_config_validate_getAttr("Retain", parseobj, where);
            if (getenv("CUNIT") == NULL){
                TRACE(9, "ClientID: %s, Retain: %s\n", ClientID?ClientID:"null", Retained?Retained:"null");
            }else {
            	printf("ClientID: %s, Retain: %s\n", ClientID?ClientID:"null", Retained?Retained:"null");
            }

            rc = ism_config_addClientSetToList(ClientID, Retained);
            if (rc) {
            	TRACE(3, "%s: Failed to add clientID: %s, retain: %s into client set List.\n",
            					__FUNCTION__, ClientID?ClientID:"null", Retained?Retained:"null");
            	ism_common_setError(rc);
            	break;
            }
            where += ent->count + 1;
        } else {
        	TRACE(5, "The JSON type %d is not supported in ClientSetDelete.\n", ent->objtype);
            where++;
        }
        ent = parseobj->ent + where;
    }

CLIENTSET_END:
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;

}

/*
 *
 * parse http service POST JSON payload.
 *
 * @param  http        ism_http_t object
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
//imaserver: { "Service":"imaserver" }
//mqconnectivity
int ism_config_json_parseServiceTraceFlushPayload(ism_http_t *http) {
	int rc = ISMRC_OK;
	const char * repl[5];
    int replSize = 0;
    char *payload = NULL;

    char * content = NULL;
    int    content_len = 0;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[2000];

    /* Parse input string and create JSON object */
	if (http->content_count > 0) {
		content = http->content[0].content;
		content_len = http->content[0].content_len;

		payload = alloca(content_len + 1);
		memcpy(payload, content, content_len);
		payload[content_len] = '\0';
	}

	parseobj.ent = ents;
	parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
	parseobj.source = (char *) content;
	parseobj.src_len = content_len;
	ism_json_parse(&parseobj);
    if (parseobj.rc) {
    	LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.", payload?payload:"null", parseobj.rc );
        ism_common_setErrorData(6001, "%s%d", payload?payload:"null", parseobj.rc);
        rc = ISMRC_ArgNotValid;
        goto PARSEPAYLOD_END;
    }

	int entloc = 1;
	const char * dbglog = NULL;

    //parse the payload
	while (entloc < parseobj.ent_count) {
		ism_json_entry_t * ent = parseobj.ent+entloc;

		//add entries into props
		switch(ent->objtype) {
		case JSON_String:  /* JSON string, value is UTF-8  */
			if (ent->name && !strcmp(ent->name, "Service")) {
				if (ent->value && strcmp(ent->value, "imaserver")) {
					rc = ISMRC_PropertyRequired;
					ism_common_setErrorData(rc, "%s%s", ent->name, ent->value?ent->value:"null");
					goto PARSEPAYLOD_END;
				}
			} else if (ent->name && !strcmp(ent->name, "DBGLOG")) {
				dbglog = ent->value;
			} else {
				TRACE(3, "%s: The JSON entry: %s is not valid.\n", __FUNCTION__, ent->name);
				rc = ISMRC_ArgNotValid;
				ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
				goto PARSEPAYLOD_END;
			}

			entloc++;
			break;
		case JSON_Integer: /* Number with no decimal point             */
		case JSON_Number:  /* Number which is too big or has a decimal */
		case JSON_Object:  /* JSON object, count is number of entries  */
		case JSON_Array:   /* JSON array, count is number of entries   */
		case JSON_Null:
        case JSON_True:    /* JSON true, value is not set              */
        case JSON_False:   /* JSON false, value is not set             */
        default:
			rc = ISMRC_ArgNotValid;
			ism_common_setErrorData(rc, "%s", ent->name?ent->name:"null");
			goto PARSEPAYLOD_END;
        }
	}

	if ( dbglog && *dbglog != '\0' ) {
		TRACE(1, "%s: DEBUG_MESSAGE: %s\n", __FUNCTION__, dbglog);
	} else {
        TRACE(1, "%s: Trace buffer is flushed.\n", __FUNCTION__);
	}
	rc = ISMRC_OK;

PARSEPAYLOD_END:
    if (rc)
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);
    return rc;
}

