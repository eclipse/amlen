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

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include "configInternal.h"

extern int ism_config_json_addPropsToList(json_t *elem, const char *objectName, const char *instName, const char *item, ism_prop_t * props, int mode);
extern int ism_admin_mqc_send_signal(int type, int mode);
extern char * ism_security_decryptAdminUserPasswd(char * src);
extern char * ism_security_encryptAdminUserPasswd(char * src);
extern int ism_config_updateStandbyNode(json_t *objval, int comptype, char *item, char *name, int getConfigLock, int deleteFlag);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */
extern int mqcAdminPauseInited;

pthread_rwlock_t mqcConfiglock;     /* Lock for MQC configuration */
int mqcConfigLockInited = 0;
int mqcPausedSignaled = 0;
int MQConnectivityEnabled = 1;

typedef int (*monitoringAction_f)(const char * locale, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc);
static monitoringAction_f monitoringAction = NULL;


/* Define MQC configuration object processing order */
/* MQC process only support objects of Server and MQConnectivity components */
struct {
    int    defined;
    char * objectName;
    int    compType;
} mqcConfigOrderList[] = {
    { 0, "MQConnectivityLog", ISM_CONFIG_COMP_MQCONNECTIVITY },
    { 0, "TraceLevel", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceSelected", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceMax", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceConnection", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceOptions", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceMessageData", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceBackup", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceBackupDestination", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceBackupCount", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceModuleLocation", ISM_CONFIG_COMP_SERVER },
    { 0, "TraceModuleOptions", ISM_CONFIG_COMP_SERVER },
    { 0, "LogLocation", ISM_CONFIG_COMP_SERVER },
    { 0, "Syslog", ISM_CONFIG_COMP_SERVER },
    { 0, "QueueManagerConnection", ISM_CONFIG_COMP_MQCONNECTIVITY },
    { 0, "DestinationMappingRule", ISM_CONFIG_COMP_MQCONNECTIVITY }
};

#define NOMQCCFGITEMS (sizeof(mqcConfigOrderList)/sizeof(mqcConfigOrderList[0]))

/* Returns component type */
static int ism_config_json_getMQCCompType(const char *objectName) {
    int i = 0;
    if (!objectName || *objectName == '\0' )
        return ISM_CONFIG_COMP_LAST;

    for ( i=0; i<NOMQCCFGITEMS; i++ ) {

        if ( !strcmp(objectName, mqcConfigOrderList[i].objectName)) {
            return mqcConfigOrderList[i].compType;
        }
    }

    return ISM_CONFIG_COMP_LAST;
}

static int ism_config_json_processOneMQCObject(json_t *post, const char *objkey, json_t *objval, int fromMQC, concat_alloc_t *output_buffer) {
    int rc = ISMRC_OK;
    int compType = ism_config_json_getMQCCompType(objkey);

    if ( compType == ISM_CONFIG_COMP_LAST ) {
        rc = ISMRC_Error;
        return rc;
    }

    /* no need to re-process objects of any other components except MQConnectivity component if this
     * function is invoked on imaserver process.
     */
    if (fromMQC == 0 && compType != ISM_CONFIG_COMP_MQCONNECTIVITY ) {
        return rc;
    }

    char *compName = "Server";
    if ( compType == ISM_CONFIG_COMP_MQCONNECTIVITY ) {
        compName = "MQConnectivity";
    }

    ism_config_t *handle = NULL;
    handle = ism_config_getHandle(compType, NULL);

    /* Call is invoked in MQC process */
    if ( fromMQC == 1 ) {

        if ( !handle ) {
            TRACE(2, "Component %s is not registered with configuration service.\n", compName);
            rc = ISMRC_Error;
            return rc;
        }

		if ( json_typeof(objval) != JSON_OBJECT ) {
			ism_prop_t *callBackProps = ism_common_newProperties(32);
			rc = ism_config_json_addPropsToList(objval, objkey, NULL, NULL, callBackProps, 0);
			if ( rc != ISMRC_OK ) {
				if ( callBackProps ) ism_common_freeProperties(callBackProps);
				goto PROCESSONEMQC_END;
			}

			rc = handle->callback((char *)objkey, NULL, callBackProps, ISM_CONFIG_CHANGE_PROPS);
			if (rc != ISMRC_OK) {
				TRACE(3, "%s: call %s callback, the return code is: %d\n", __FUNCTION__, objkey?objkey:"null", rc);
				/* If the callback has set the error, then we don't need to set it here, overwriting any parameters
				 * set in the callback.
				 */
				if (ISMRC_OK == ism_common_getLastError())
					ism_common_setError(rc);
			}

			if ( callBackProps ) ism_common_freeProperties(callBackProps);
			goto PROCESSONEMQC_END;
		}

    }

    /* get instance of composite object and validate the object */
    json_t *instval = NULL;
    const char *instkey = NULL;
    int insttyp;
    void *institer = json_object_iter(objval);
    while (institer) {
        instkey = json_object_iter_key(institer);
        instval = json_object_iter_value(institer);
        insttyp = json_typeof(instval);

        /* check if the object is QueueManagerConnection and Verify is set to true i.e. test connection case */
        int invokeTestFunction = 0;
        int qmcObject = 0;
        if ( !strcmp(objkey, "QueueManagerConnection")) {
        	qmcObject = 1;
			/* get verify flag */
			json_t * verifyObj = json_object_get(instval, "Verify");

			if ( verifyObj && json_typeof(verifyObj) == JSON_TRUE ) {
				/* verify is set - run test QueueManagerConnection, do not update configuration */
				invokeTestFunction = 1;
			}
        }

        if ( insttyp == JSON_OBJECT ) {

            /* if call is invoked on imaserver process, then just update configuration */
            if ( fromMQC == 0 && invokeTestFunction == 0 ) {
                /* make a copy of instval, to persist data in configuration */
                json_t *newval = json_deep_copy(instval);

                /* Encrypt password if specified */
                if ( qmcObject == 1 ) {
    				json_t *passwdObj = json_object_get(newval, "ChannelUserPassword");
    				if ( passwdObj && json_typeof(passwdObj) == JSON_STRING ) {
    					char *pwdStr = (char *)json_string_value(passwdObj);
    					if ( pwdStr && *pwdStr != '\0' ) {
    						TRACE(9, "Encrypt ChannelUserPassword: %s\n", pwdStr);
    						char *encPassword = ism_security_encryptAdminUserPasswd((char *)pwdStr);
    						json_object_set(newval, "ChannelUserPassword", json_string(encPassword));
    					}
    				}
                }

                /* get current config and replace with newval */
                json_t *currConfig = ism_config_json_getComposite(objkey, instkey, 0);
                if ( currConfig) {
                    /* update with new value */
                    if ( json_object_update(currConfig, newval) < 0 ) {
                        rc = ISMRC_Error;
                        TRACE(4, "Failed to update configuration of Object:%s Name:%s\n", objkey?objkey:"NULL", instkey?instkey:"NULL");
                        goto PROCESSONEMQC_END;
                    }
                } else {
                    /* Add new object */
                    rc = ism_config_json_setComposite(objkey, instkey, newval);
                }

                if ( rc != ISMRC_OK ) {
                    goto PROCESSONEMQC_END;
                }
                
                /* Update standby node */
	            int getConfigLock = 0; /* lock is already taken */
	            int deleteFlag = 0;
	            rc = ism_config_updateStandbyNode(newval, ISM_CONFIG_COMP_MQCONNECTIVITY, (char *)objkey, (char *)instkey, getConfigLock, deleteFlag);
	            if (rc != ISMRC_OK)
	            		TRACE(3, "%s: ism_config_updateStandbyNode updating object: %s return errorcode: %d\n", __FUNCTION__, objkey?objkey:"null", rc);

                institer = json_object_iter_next(objval, institer);
                continue;
            }

            if ( fromMQC == 1 ) {

            	/* check if test function needs to be invoked */
            	if ( invokeTestFunction == 1 ) {
            		rc = ism_admin_testObject(objkey, instval, output_buffer);
            		goto PROCESSONEMQC_END;
            	}

				/* process object configuration on mqcbridge process */
				/* Create props list for callback */
				ism_prop_t *callBackProps = ism_common_newProperties(32);
				rc = ism_config_json_addPropsToList(instval, objkey, instkey, NULL, callBackProps, 0);
				if ( rc != ISMRC_OK ) {
					goto PROCESSONEMQC_END;
				}

				rc = handle->callback((char *)objkey, (char *)instkey, callBackProps, ISM_CONFIG_CHANGE_PROPS);
				if (rc != ISMRC_OK) {
					TRACE(3, "%s: call %s callback with name:%s, the return code is: %d\n", __FUNCTION__, objkey?objkey:"null", instkey?instkey:"null", rc);
					/* If the callback has set the error, then we don't need to set it here, overwriting any parameters
					 * set in the callback.
					 */
					if (ISMRC_OK == ism_common_getLastError())
						ism_common_setError(rc);
					goto PROCESSONEMQC_END;
				}

            }
        } else if ( insttyp == JSON_NULL ) {
            rc = ISMRC_BadRESTfulRequest;
            int erlen = strlen(instkey) + 9;
            char *erstr = alloca(erlen + 1);
            sprintf(erstr, "\"%s\":null", instkey);
            ism_common_setErrorData(rc, "%s", erstr);
            goto PROCESSONEMQC_END;
        } else {
            rc = ISMRC_PropertiesNotValid;
            ism_common_setError(rc);
            goto PROCESSONEMQC_END;
        }

        institer = json_object_iter_next(objval, institer);
    }

PROCESSONEMQC_END:
    if ( rc != ISMRC_OK ) {
        TRACE(3, "MQC Object='%s' configuration error.\n", objkey?objkey:"NULL");
    }

    return rc;
}

/**
 * Process POST Payload of MQC JSON configuration objects in MQCBridge process
 * - The object is partly validated in imaserver process, remaining validation
 *   can be done by MQC config callback function. Create properties and invoke
 *   config callback.
 */
static int ism_config_json_processMQCConfigObjects(json_t *post, int fromMQC, concat_alloc_t *output_buffer ) {
    int rc = ISMRC_OK;
    json_t *objval = NULL;
    const char *objkey = NULL;
    int contLoop = 0;

    /* check no of configuration items in the post */
    int nitems = json_object_size(post);

    TRACE(5, "MQC Config: From MQCProcess:%d - process POST Payload. Number of items: %d\n", fromMQC, nitems);

    ism_common_setError(0);

    /* No need to process if POST payload has no item */
    if ( nitems == 0 ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", "payload");
        goto PROCESSPOST_END;
    }

    /* Check and process one item */
    if ( nitems == 1 ) {
        json_object_foreach(post, objkey, objval) {
            rc = ism_config_json_processOneMQCObject(post, objkey, objval, fromMQC, output_buffer);
        }
        goto PROCESSPOST_END;
    }

    /* POST payload includes multiple items
     * - loop thru post json objects to check for invalid items
     */
    int i = 0;
    for (i=0; i<NOMQCCFGITEMS; i++) {
        mqcConfigOrderList[i].defined = 0;
    }
    json_object_foreach(post, objkey, objval) {
        contLoop = 0;

        /* Validate item from ordered list */
        for (i=0; i<NOMQCCFGITEMS; i++) {
            char *oname = mqcConfigOrderList[i].objectName;
            if ( !strcmp(objkey, oname)) {
                contLoop = 1;
                mqcConfigOrderList[i].defined = 1;
                break;
            }
        }
        if ( contLoop == 0 ) {
            /* Invalid object is specified - return error */
            rc = ISMRC_BadPropertyName;
            ism_common_setErrorData(rc, "%s", objkey?objkey:"NULL");
            goto PROCESSPOST_END;
        }
    }

    /* loop thru the ordered list */
    for ( i=0; i<NOMQCCFGITEMS; i++ ) {
        if ( mqcConfigOrderList[i].defined == 0 )
            continue;

        objkey = mqcConfigOrderList[i].objectName;

        /* find item in POST payload and process each item */
        objval = json_object_get(post, objkey);

        rc = ism_config_json_processOneMQCObject(post, objkey, objval, fromMQC, output_buffer);

        if ( rc != ISMRC_OK )
           goto PROCESSPOST_END;
    }

PROCESSPOST_END:

    return rc;
}

/**
 * Process MQC object delete
 */
static int ism_config_json_processMQCObjectDelete(char *object, char *inst) {
    int rc = ISMRC_PropertiesNotValid;

    /* only DestinationMappingRule and QueueManagerConnection can be deleted */
    if ( object && inst && (!strcmp(object, "DestinationMappingRule") || !strcmp(object, "QueueManagerConnection"))) {

        ism_config_t *handle = ism_config_getHandle(ISM_CONFIG_COMP_MQCONNECTIVITY, NULL);

        if ( !handle ) {
            TRACE(2, "MQConnectivity is not registered with configuration service.\n");
            rc = ISMRC_Error;
            return rc;
        }

        TRACE(9, "Delete object:'%s' Instance:'%s'", object, inst?inst:"");

        ism_prop_t *props = ism_common_newProperties(2);

        /* Add object name to props list */
        char keyword[4096];
        ism_field_t var = {0};
        sprintf(keyword, "%s.Name.%s", object, inst);
        ism_common_canonicalName(keyword);
        var.type = VT_String;
        var.val.s = (char *)inst;
        ism_common_setProperty(props, keyword, &var);

        rc = handle->callback(object, inst, props, ISM_CONFIG_CHANGE_DELETE);
    }

    return rc;
}

/**
 * Process initial configuration (already available in the configuration file)
 */
static int ism_config_json_processMQCInitConfig(json_t *inpobj) {

    /* This function is called when mqc process is started,
     * add a small delay for pause function to get started in mqcbridge process
     */
	ism_common_sleep(500000);

    if ( !inpobj ) {
        /* Signal main process to continue, if mqc pause is initialized */
        if ( mqcAdminPauseInited == 1 ) ism_admin_mqc_send_signal(ISM_ADMIN_STATE_CONTINUE, 0);
        mqcPausedSignaled = 1;
    	return ISMRC_NullPointer;
    }

    /* process inpobj and update props */
    json_t *objval = NULL;
    const char *objkey = NULL;
    const char *itemkey = NULL;
    void *itemiter = NULL;
    json_t *itemval = NULL;
    char keyword[4098];
    ism_prop_t *props = NULL;

    TRACE(1, "Process Initial configuration sent by imaserver.\n");

    json_object_foreach(inpobj, objkey, objval) {
        int compType = ism_config_json_getMQCCompType(objkey);
        if ( compType == ISM_CONFIG_COMP_LAST ) continue;

        props = compProps[ISM_CONFIG_COMP_SERVER].props;

        if ( compType == ISM_CONFIG_COMP_MQCONNECTIVITY ) {
            props = compProps[ISM_CONFIG_COMP_MQCONNECTIVITY].props;
        }

        if ( json_typeof(objval) == JSON_OBJECT ) {
            itemiter = json_object_iter(objval);
            while (itemiter) {
                itemkey = json_object_iter_key(itemiter);
                itemval = json_object_iter_value(itemiter);

                /* Add object name to props list */
                ism_field_t var = {0};
                sprintf(keyword, "%s.Name.%s", objkey, itemkey);
                ism_common_canonicalName(keyword);
                var.type = VT_String;
                var.val.s = (char *)itemkey;
                ism_common_setProperty(props, keyword, &var);

                /* Add rest of the items */
                const char *elemkey = NULL;
                void *elemiter = NULL;
                json_t *elemval = NULL;
                elemiter = json_object_iter(itemval);
                while (elemiter) {
                    elemkey = json_object_iter_key(elemiter);
                    elemval = json_object_iter_value(elemiter);

                    sprintf(keyword, "%s.%s.%s", objkey, elemkey, itemkey);
                    ism_common_canonicalName(keyword);

                    TRACE(1, "Process property: %s\n", keyword);

                    int type = json_typeof(elemval);
                    if ( type == JSON_STRING ) {
                        char *dPasswd = NULL;
                        char *charval = (char *)json_string_value(elemval);
                        var.type = VT_String;
                        var.val.s = charval;
                        if ( !strcmp(elemkey, "ChannelUserPassword") && charval && *charval != '\0' ) {
                            dPasswd = ism_security_decryptAdminUserPasswd(charval);
                            if ( dPasswd && *dPasswd != '\0' ) {
                                var.val.s = dPasswd;
                            }
                        }
                        ism_common_setProperty(props, keyword, &var);
                        ism_common_free(ism_memory_admin_misc,dPasswd);
                    } else if ( type == JSON_INTEGER ) {
                        int  intval = json_integer_value(elemval);
                        var.type = VT_Integer;
                        var.val.i = intval;
                        ism_common_setProperty(props, keyword, &var);
                    } else if ( type == JSON_TRUE || type == JSON_FALSE ) {
                        int bval = 0;
                        if ( type == JSON_TRUE ) bval = 1;
                        var.type = VT_Boolean;
                        var.val.i = bval;
                        ism_common_setProperty(props, keyword, &var);
                    }

                    elemiter = json_object_iter_next(itemval, elemiter);
                }

                itemiter = json_object_iter_next(objval, itemiter);
            }
        } else {
            /* process all singleton objects */
            sprintf(keyword, "%s", objkey);
            ism_common_canonicalName(keyword);

            TRACE(1, "Process property: %s\n", keyword);

            ism_field_t var = {0};
            int type = json_typeof(objval);
            if ( type == JSON_STRING ) {
                char *charval = (char *)json_string_value(objval);
                var.type = VT_String;
                var.val.s = charval;
                ism_common_setProperty(props, keyword, &var);
            } else if ( type == JSON_INTEGER ) {
                int  intval = json_integer_value(objval);
                var.type = VT_Integer;
                var.val.i = intval;
                ism_common_setProperty(props, keyword, &var);
            } else if ( type == JSON_TRUE || type == JSON_FALSE ) {
                int bval = 0;
                if ( type == JSON_TRUE ) bval = 1;
                var.type = VT_Boolean;
                var.val.i = bval;
                ism_common_setProperty(props, keyword, &var);
            }
        }
    }

    TRACE(1, "Done processing initial configuration sent by imaserver.\n");

    /* Signal main process to continue, if mqc pause is initialized */
    if ( mqcAdminPauseInited == 1 ) ism_admin_mqc_send_signal(ISM_ADMIN_STATE_CONTINUE, 0);
    mqcPausedSignaled = 1;

    return 0;
}

/**
 * Process MQC Monitoring calls
 */
static int ism_config_json_processMQCMonitoring(json_t *objval, const char * locale, concat_alloc_t *output_buffer) {
    int rc = ISMRC_OK;

    /* convert info JSON string and invoke monitoring action API */
    char *buf = json_dumps(objval, JSON_ENCODE_ANY);
    if ( buf ) {
        int buflen = strlen(buf);
        TRACE(5, "%s: Call ism_process_mqc_monitoring_action with cmd string: %s.\n", __FUNCTION__, buf);

        monitoringAction = (monitoringAction_f)(uintptr_t)ism_common_getLongConfig("_ism_process_mqc_monitoring_action_fnptr", 0L);
        if ( monitoringAction ) {

            int xrc = monitoringAction(locale, buf, buflen, output_buffer, &rc);
            if (xrc > 0 ) {
                TRACE(3, "%s: MQ connectivity monitoring status return error code: %d.\n", __FUNCTION__, xrc);
                rc = xrc;
                ism_common_setError(rc);
            } else if ( xrc == -1 ) {
                rc = -1;
            }
        } else {
            ism_common_free_raw(ism_memory_admin_misc,buf);
            rc = ISMRC_NotImplemented;
            ism_common_setError(rc);
        }
    } else {
        rc = ISMRC_NotFound;
        ism_common_setError(rc);
    }

    return rc;
}

/**
 * Send continue signal to MQC process
 */
static int ism_config_json_processMQCProcessContinue(void) {
    return ism_admin_mqc_send_signal(ISM_ADMIN_STATE_CONTINUE, 0);
}

/**
 * Send stop signal to MQC process
 */
static int ism_config_json_processMQCProcessStop(int mode) {
    return ism_admin_mqc_send_signal(ISM_ADMIN_STATE_STOP, mode);
}

/**
 * Set MQConnectivityEnabled flag value in mqc process
 */
static int ism_config_json_setMQCEnabledFlag(json_t *objval) {
    int rc = ISMRC_OK;

    int enabled = 1;

    if ( objval && json_typeof(objval) == JSON_FALSE ) enabled = 0;

    /* set global flag */
    MQConnectivityEnabled = enabled;

    TRACE(1, "MQConnectivityEnabled flag is set to %d\n", MQConnectivityEnabled);

    return rc;
}


/**
 * Process configuration of MQC objects forwarded by imaserver process.
 * This function is executed in mqcbridge process.
 */
XAPI int ism_config_json_processMQCRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int rc) {
    rc = ISMRC_OK;

    if ( ism_admin_getServerProcType() != ISM_PROTYPE_MQC ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setError(rc);
        return rc;
    }
    if ( !inpbuf ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }
    if ( *inpbuf == '\0') return rc;

    /* inputbuf is a json string, parse it */

    TRACE(5, "Received MQC Admin request. Buffer length=%d\n", inpbuflen);

    /* Get read lock */
    pthread_rwlock_wrlock(&mqcConfiglock);

    char *cfgObjStr = alloca(inpbuflen +1);
    memcpy(cfgObjStr, inpbuf, inpbuflen);
    cfgObjStr[inpbuflen] = 0;

    TRACE(9, "MQC Admin request buffer:\n%s\n", cfgObjStr);

    json_t *cfgObj = ism_config_json_strToObject(cfgObjStr, &rc);

    if ( !cfgObj || rc != ISMRC_OK ) {
        pthread_rwlock_unlock(&mqcConfiglock);
        return rc;
    }


    /* traverse thru the object and take action
     * Object may include initial configuration,
     * dynamic configuration, service or monitoring request
     */
    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(cfgObj, objkey, objval) {
        TRACE(5, "Process MQC Admin request type: %s\n", objkey);
        if ( !strcmp(objkey, "Configuration")) {
            /* Process configuration payload */
            int fromMQC = 1;
            rc = ism_config_json_processMQCConfigObjects(objval, fromMQC, output_buffer);
        } else if ( !strcmp(objkey, "ConfigurationDelete")) {
            /* process delete method */
            json_t *objectJson = json_object_get(objval, "Object");
            json_t *instJson = json_object_get(objval, "Instance");
            char *object = (char *)json_string_value(objectJson);
            char *inst = (char *)json_string_value(instJson);
            rc = ism_config_json_processMQCObjectDelete(object, inst);
        } else if ( !strcmp(objkey, "ConfigurationInitial")) {
            rc = ism_config_json_processMQCInitConfig(objval);
        } else if ( !strcmp(objkey, "ProcessContinue")) {
            rc = ism_config_json_processMQCProcessContinue();
        } else if ( !strcmp(objkey, "ProcessStop")) {
            json_t *modeObj = json_object_get(objval, "Mode");
            int mode = json_integer_value(modeObj);
            rc = ism_config_json_processMQCProcessStop(mode);
        } else if ( !strcmp(objkey, "Monitoring")) {
            rc = ism_config_json_processMQCMonitoring(objval, locale, output_buffer);
        } else if ( !strcmp(objkey, "MQConnectivityEnabled")) {
        	rc = ism_config_json_setMQCEnabledFlag(objval);
        }
    }

    json_decref(cfgObj);

    /* Unlock config lock */
    pthread_rwlock_unlock(&mqcConfiglock);

    return rc;
}

/**
 * Initialize MQC configuration lock
 */
XAPI void ism_config_initMQCConfigLock(void) {

    if ( mqcConfigLockInited == 0 ) {
        /* Initialize rwlock to protect server configuration JSON object */
        pthread_rwlockattr_t rwlockattr_init;
        pthread_rwlockattr_init(&rwlockattr_init);
        pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        pthread_rwlock_init(&mqcConfiglock, &rwlockattr_init);
        mqcConfigLockInited = 1;
    }
}


/**
 * Add initial configuration to the buffer
 * This function is invoked on imaserver process when MQC admin channel get connected to
 * imaserver process.
 */
XAPI int ism_config_addInitialMQCConfiguration(concat_alloc_t *buf) {
    pthread_rwlock_rdlock(&srvConfiglock);
    json_t *curConfig = json_deep_copy(srvConfigRoot);
    pthread_rwlock_unlock(&srvConfiglock);

    if ( curConfig ) {
        json_t *retObj = json_object();
        json_object_set_new(retObj, "ConfigurationInitial", curConfig);
        char *cfgStr = json_dumps(retObj, 0);

        if ( cfgStr ) {
            ism_common_allocBufferCopyLen(buf, cfgStr, strlen(cfgStr));
            ism_common_free_raw(ism_memory_admin_misc,cfgStr);
        }

        json_decref(curConfig);
    }

    return 0;
}

/**
 * Delete Object after receiving success from mqcbridge process and before invoking http callback
 */
XAPI int ism_config_json_delMQCObject(int objID, char *objName) {
    int rc = ISMRC_OK;

    json_t *rootObj = NULL;
    json_t *instObj = NULL;

    if ( !srvConfigRoot || !objName ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    TRACE(5, "Delete MQConnectivity object: ID:%d Name:%s\n", objID, objName);

    pthread_rwlock_rdlock(&srvConfiglock);

    if ( objID == ISM_MQC_DMR ) {
        rootObj = json_object_get(srvConfigRoot, "DestinationMappingRule");
    } else if ( objID == ISM_MQC_QMC ) {
        rootObj = json_object_get(srvConfigRoot, "QueueManagerConnection");
    } else {
        TRACE(3, "Received a request to delete an unsupported MQConnectivity Object. ID:%d\n", objID);
    }

    if ( rootObj ) {
        instObj = json_object_get(rootObj, objName);
        if ( instObj ) {
            
	        /* Update standby node */
	        int getConfigLock = 0;
	        int deleteFlag = 1;
	        int rc1 = ISMRC_OK;
	        if ( objID == ISM_MQC_DMR ) {
	            	rc1 = ism_config_updateStandbyNode(instObj, ISM_CONFIG_COMP_MQCONNECTIVITY, "DestinationMappingRule", (char *)objName, getConfigLock, deleteFlag);
	        } else {
	            	rc1 = ism_config_updateStandbyNode(instObj, ISM_CONFIG_COMP_MQCONNECTIVITY, "QueueManagerConnection", (char *)objName, getConfigLock, deleteFlag);
            }	        
	        	if (rc1 != ISMRC_OK)
	        	    	TRACE(3, "%s: ism_config_updateStandbyNode updating object: %s return errorcode: %d\n", __FUNCTION__, objName?objName:"null", rc1);
	        	    	
	        json_object_del(rootObj, objName);

	    }
	            	
    } else {
        TRACE(5, "Could not find MQConnectivity object ID=%d\n", objID);
    }

    pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

/**
 * Returns ism_MQCObjectIDs_t
 */
XAPI int ism_config_getMQCObjectID(char *objName) {
    if (!objName ) return ISM_MQC_LAST;
    if ( !strcmp(objName, "QueueManagerConnection")) {
        return ISM_MQC_QMC;
    } else if ( !strcmp(objName, "DestinationMappingRule")) {
        return ISM_MQC_DMR;
    }
    return ISM_MQC_LAST;
}

/**
 * Add Object after receiving success from mqcbridge process, and before invoking http callback
 */
XAPI int ism_config_json_addMQCObject(char *cfgObjStr) {
    int rc = ISMRC_OK;

    TRACE(9, "_addMQCObject: Add configuration from buffer:\n%s\n", cfgObjStr);

    json_t *cfgObj = ism_config_json_strToObject(cfgObjStr, &rc);

    if ( !cfgObj || rc != ISMRC_OK ) {
        ism_common_setError(rc);
        return rc;
    }


    /* traverse thru the object and update configuration */
    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(cfgObj, objkey, objval) {
        TRACE(5, "_addMQCObject: Process MQC Admin request type: %s\n", objkey);
        if ( !strcmp(objkey, "Configuration")) {
            /* Process configuration payload */
            int fromMQC = 0;
            char tbuf[2048];
            concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
            rc = ism_config_json_processMQCConfigObjects(objval, fromMQC, &mqcBuffer);
        }
    }
    json_decref(cfgObj);
    return rc;
}

/**
 * Send MQConnectivityFlag to MQC process
 */
XAPI int ism_config_json_sendMQConnectvityFlagValue(void) {
	int rc = ISMRC_OK;
	int persistData = 0;
	int enabled = 0;

    char buf[1024];
    int buflen = 1024;
    concat_alloc_t outbuf = {0};
    outbuf.buf = buf;
    outbuf.len = buflen;
    memset(outbuf.buf, 0, buflen);

    ism_http_t *http;
    http = alloca(sizeof(ism_http_t));
    memset(http->version, 0, 16);
    memcpy(http->version, "2.0", 3);
    http->outbuf = outbuf;
    http->locale = "en_US";

	char tbuf[2048];
	concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
	memset(tbuf, 0, 2048);

    enabled = ism_config_getMQConnEnabled();

	ism_common_allocBufferCopyLen(&mqcBuffer, "{\"MQConnectivityEnabled\":", 25);
	if ( enabled == 1 ) {
		ism_common_allocBufferCopyLen(&mqcBuffer, "true", 4);
	} else {
		ism_common_allocBufferCopyLen(&mqcBuffer, "false", 5);
	}
	ism_common_allocBufferCopyLen(&mqcBuffer, "}", 1);

	TRACE(5, "Send MQConnectivityEnabled flag to MQC process: len=%d msg=%s\n", mqcBuffer.used, mqcBuffer.buf );

	rc = ism_admin_mqc_send(mqcBuffer.buf, mqcBuffer.used, http, NULL, persistData, ISM_MQC_FLAG, NULL);

	return rc;
}
