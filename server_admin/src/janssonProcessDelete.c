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
 * janssonProcessDelete.c
 */

#define TRACE_COMP Admin

#include <janssonConfig.h>
#include "configInternal.h"
#include <sys/wait.h>
#include "crlprofile.h"

extern int ism_admin_nodeUpdateAllowed(int *rc, ism_ConfigComponentType_t compType, char *object);
extern int ism_config_deleteOAuthKeyFile(char * name);
extern int ism_config_getMQCObjectID(char *objName);
extern int ism_config_checkTrustedCertExist(int type, char *secProfile, char *trustCert, int *isNewFile, int *ncerts);
extern int ism_config_deleteMQCerts(void);
extern int ism_config_markResourceSetDescriptorDeleted(int haSync);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */

static int ism_config_json_deleteArrayEntry(char *item, char *inst, char *subinst) {
    int rc = ISMRC_OK;
    char *tmpstr = NULL;
	int found = 0;

    if (!item) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto DELETEARRAYENTRY_END;
    }

    if (strcmp(item, "TrustedCertificate") && strcmp(item, "TopicMonitor") && strcmp(item, "ClientCertificate") &&
        strcmp(item, "ClusterRequestedTopics")) {
		rc = ISMRC_BadRESTfulRequest;
		int len = strlen(item) + strlen(inst) + 13;
		char *tmp = alloca(len) + 1;
		snprintf(tmp, len, "%s %s with %s", "Delete", item, inst);
		tmp[len] = '\0';
		ism_common_setErrorData(rc, "%s", tmp);
		goto DELETEARRAYENTRY_END;
    }

    if (!strcmp(item, "TrustedCertificate") || !strcmp(item, "ClientCertificate")) {
        char * secProf = inst;
        char * certName = subinst;
        if ( !secProf || *secProf == '\0' || !certName || *certName == '\0' ) {
    		rc = 6167;
            if ( *item == 'T' ) {
        		ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "TrustedCertificate");
            } else {
        		ism_common_setErrorData(rc, "%s%s", "SecurityProfileName", "CertificateName");
            }
            goto DELETEARRAYENTRY_END;
        }

        int isNewFile = 0;
        int ncerts = 0;
        if ( !strcmp(item, "TrustedCertificate")) {
            rc = ism_config_checkTrustedCertExist(0, secProf, certName, &isNewFile, &ncerts);
        } else {
        	rc = ism_config_checkTrustedCertExist(1, secProf, certName, &isNewFile, &ncerts);
        }
        goto DELETEARRAYENTRY_END;
    } else {
        json_t *objroot = json_object_get(srvConfigRoot, item);
        if (!objroot) {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            goto DELETEARRAYENTRY_END;
        }

        int objType = json_typeof(objroot);
        if (objType != JSON_ARRAY) {
            rc = ISMRC_BadPropertyType;
            ism_common_setErrorData(rc, "%s%s%s%s", item, "null", "null", ism_config_json_typeString(objType));
            goto DELETEARRAYENTRY_END;
        }

		int i = 0;
		for (i=0; i<json_array_size(objroot); i++) {
			json_t *instObj = json_array_get(objroot, i);
			if (instObj) {
                if (!strcmp(item, "TopicMonitor") || !strcmp(item, "ClusterRequestedTopics")) {
				    const char *val = json_string_value(instObj);
				    if (val && !strcmp(val, inst)) {
					    found = 1;
					    break;
				    }
				}
			}
		}

		if (found == 1) {
			rc = json_array_remove(objroot, i);
			if (rc) {
				TRACE(3, "%s: Failed to delete configuration object: %s, delete string: %s\n", __FUNCTION__, item, inst);
				rc = ISMRC_DeleteFailure;
				ism_common_setErrorData(rc, "%s%s", item, inst);
			}

		} else {
			TRACE(5, "The item: %s with inst:%s cannot be found\n", item, inst);
		}

    }

DELETEARRAYENTRY_END:
    if (tmpstr)  ism_common_free(ism_memory_admin_misc,tmpstr);
    TRACE(9, "%s: delete obect %s with delete string %s returns %d.\n", __FUNCTION__, item?item:"null", inst?inst:"null", rc);
    return rc;
}

static int ism_config_json_validateDeleteObjects(char *item, char *name) {
    int rc = ISMRC_OK;

    if ( strcmp(item, "MessageHub") == 0 ) {
        rc = ism_config_json_findObjectInUse("Endpoint", "MessageHub", name, 0);
    } else if ( strcmp(item, "TopicPolicy") == 0 ) {
       rc = ism_config_json_findObjectInUse("Endpoint", "TopicPolicies", name, 1);
    } else if ( strcmp(item, "QueuePolicy") == 0 ) {
       rc = ism_config_json_findObjectInUse("Endpoint", "QueuePolicies", name, 1);
    } else if ( strcmp(item, "SubscriptionPolicy") == 0 ) {
       rc = ism_config_json_findObjectInUse("Endpoint", "SubscriptionPolicies", name, 1);
    } else if ( strcmp(item, "ConnectionPolicy") == 0 ) {
        rc = ism_config_json_findObjectInUse("Endpoint", "ConnectionPolicies", name, 1);
    } else if ( strcmp(item, "ConfigurationPolicy") == 0 ) {
            rc = ism_config_json_findObjectInUse("AdminEndpoint", "ConfigurationPolicies", name, 1);
    } else if ( strcmp(item, "SecurityProfile") == 0 ) {
        rc = ism_config_json_findObjectInUse("Endpoint", "SecurityProfile", name, 0);
        if (rc == ISMRC_OK) {
            /* now checking against AdminEndpoint */
            rc = ism_config_json_findObjectInUse("AdminEndpoint", "SecurityProfile", name, 0);
        }
    } else if ( strcmp(item, "CertificateProfile") == 0 ) {
        rc = ism_config_json_findObjectInUse("SecurityProfile", "CertificateProfile", name, 0);
    } else if ( strcmp(item, "LTPAProfile") == 0 ) {
        rc = ism_config_json_findObjectInUse("SecurityProfile", "LTPAProfile", name, 0);
    } else if ( strcmp(item, "OAuthProfile") == 0 ) {
        rc = ism_config_json_findObjectInUse("SecurityProfile", "OAuthProfile", name, 0);
    } else if ( strcmp(item, "CRLProfile") == 0 ) {
    	rc = ism_config_json_findObjectInUse("SecurityProfile", "CRLProfile", name, 0);
    }

    return rc;
}

/**
 * Delete dirs and capath from truststore directory
 */
static int ism_config_json_purgeTrustStore(char *profileName) {
    int rc = ISMRC_OK;
    char * fileutilsPath  = IMA_SVR_INSTALL_PATH "/bin/imafileutils.sh";

    if ( !profileName || *profileName == '\0' ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    char *trustCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");
    if ( !trustCertDir || *trustCertDir == '\0' ) {
        trustCertDir = IMA_SVR_DATA_PATH "/data/certificates/truststore/";
    }

    TRACE(5, "Purge SecurityProfile '%s' is deleted. Purge truststore\n", profileName);

    pid_t pid = fork();
    if (pid < 0){
        perror("fork failed");
        return ISMRC_Error;
    }
    if (pid == 0){
        execl(fileutilsPath, "imafileutils.sh", "deleteProfileDir", trustCertDir, profileName, NULL);
        int urc = errno;
        TRACE(1, "Unable to run imafileutils.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    int st;
    waitpid(pid, &st, 0);
    int res = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
    if (res)  {
        TRACE(3, "%s: failed to delete SecurityProfile (%s) files from TrustStore: %s.\n", __FUNCTION__, profileName, trustCertDir);
    }

    return rc;
}

/**
 * Clean orphaned SecurityProfile dependent objects:
 * - Loop thru list of Trusted and Client Certificates, if
 *   a valid SecurityProfile doesn't exit in the configuration
 *   table, delete Trusted/Client certificates from the
 *   configuration table.
 */
XAPI int ism_config_json_cleanOrphanedSecProfObjects(int getLock, int updateConfigFile) {
    int rc = ISMRC_OK;
    int i = 0;
    int found = 0;
    int updateFlag = 0;
    int count = 0;

    /* get configuration lock */
    if ( getLock ) {
        pthread_rwlock_wrlock(&srvConfiglock);
    }

    /* process TrustedCertificates */
    json_t *tCerts = json_object_get(srvConfigRoot, "TrustedCertificate");
    count = json_array_size(tCerts);
    while ( count ) {
        found = 0;
        for (i=0; i<count; i++) {
            json_t *instObj = json_array_get(tCerts, i);
            if (instObj) {
                json_t *secProfObj = json_object_get(instObj, "SecurityProfileName");
                if ( secProfObj && json_typeof(secProfObj) == JSON_STRING ) {
                    const char *secProfName = json_string_value(secProfObj);
                    /* check if SecurityProfile exits */
                    if ( secProfName && *secProfName != '\0' ) {
                        if ( ism_config_objectExist("SecurityProfile", (char *)secProfName, NULL) == 0 ) {
                            TRACE(5, "Delete orphaned TrustedCertificate associated with SecurityProfile: %s\n", secProfName);
                            json_array_remove(tCerts, i);
                            found = 1;
                            updateFlag = 1;
                            break;
                        }
                    }
                }
            }
        }
        if ( found == 0 ) {
            break;
        }
        count = json_array_size(tCerts);
    }

    /* process ClientCertificates */
    json_t *cCerts = json_object_get(srvConfigRoot, "ClientCertificate");
    count = json_array_size(cCerts);
    while ( count ) {
        found = 0;
        for (i=0; i<count; i++) {
            json_t *instObj = json_array_get(cCerts, i);
            if (instObj) {
                json_t *secProfObj = json_object_get(instObj, "SecurityProfileName");
                if ( secProfObj && json_typeof(secProfObj) == JSON_STRING ) {
                    const char *secProfName = json_string_value(secProfObj);
                    /* check if SecurityProfile exits */
                    if ( secProfName && *secProfName != '\0' ) {
                        if ( ism_config_objectExist("SecurityProfile", (char *)secProfName, NULL) == 0 ) {
                            TRACE(5, "Delete orphaned ClientCertificate associated with SecurityProfile: %s\n", secProfName);
                            json_array_remove(cCerts, i);
                            found = 1;
                            updateFlag = 1;
                            break;
                        }
                    }
                }
            }
        }
        if ( found == 0 ) {
            break;
        }
        count = json_array_size(cCerts);
    }

    /* Update configuration file */
    if ( updateFlag == 1 && updateConfigFile == 1 ) {
        rc = ism_config_json_updateFile(getLock);
    }

    /* Unlock config lock */
    if ( getLock ) {
        pthread_rwlock_unlock(&srvConfiglock);
    }

    return rc;
}


/**
 * Process delete of an object
 */
XAPI int ism_config_json_deleteObject(char *object, char *inst, char *subinst, int queryOption, int standbyCheck, concat_alloc_t *mqcBuffer) {
    int rc = ISMRC_OK;
    int isComposite = 0;

    if ( !object ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        return rc;
    }

    /* Don't allow deletion of AdminDefault objects for AdminEndpoint */
    if ( !strcmp(object, "AdminEndpoint") || !strcmp(object, "Syslog") || !strcmp(object, "LogLocation") || !strcmp(object, "Protocol") ||
         !strcmp(object, "LDAP") || !strcmp(object, "HighAvailability") || !strcmp(object, "ClusterMembership") ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(rc, "%s", object);
        return rc;
    }

    /* Get component type */
    ism_ConfigComponentType_t compType = ism_config_findSchemaGetComponentType((char *)object, &isComposite, NULL);

    /* Don't allow deletion of AdminDefault objects for AdminEndpoint */
    if ( (isComposite == 0) ||
         (!strcmp(object, "CertificateProfile") && !strcmp(inst, "AdminDefaultCertProf")) ||
         (!strcmp(object, "SecurityProfile") && !strcmp(inst, "AdminDefaultSecProfile")) ||
         (!strcmp(object, "ConfigurationPolicy") && !strcmp(inst, "AdminDefaultConfigPolicy")) ) {
        rc = ISMRC_DeleteNotAllowed;
        ism_common_setErrorData(rc, "%s", inst?inst:object);
        return rc;
    }

    /* In HA - check if updates are allowed */
    if ( standbyCheck == 0 && ism_admin_nodeUpdateAllowed(&rc, compType, object) == 0 ) {
        return rc;
    }

    /* Get configuration handle */
    ism_config_t *handle = ism_config_getHandle(compType, NULL);

    /* get object from current configuration */
    json_t *objroot = NULL;
    json_t *instroot = NULL;

    /* Get write lock */
    pthread_rwlock_wrlock(&srvConfiglock);

    int haSync = 0;

    /* validate if the entry of the array can be deleted */
    if (!strcmp(object, "TrustedCertificate") || !strcmp(object, "ClientCertificate")) {
        rc = ism_config_json_deleteArrayEntry(object, inst, subinst);
        goto PROCESSDELETEOBJ_END;
    } else if (!strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics")) {
        rc = ism_config_json_callCallback( handle, object, NULL, json_string(inst), haSync, ISM_CONFIG_CHANGE_DELETE, 0);
        if (rc == ISMRC_OK) {
            rc = ism_config_json_deleteArrayEntry(object, inst, NULL);
        }
        if (rc != ISMRC_OK)
            goto PROCESSDELETEOBJ_END;

        int getLock = 0;
        rc = ism_config_json_updateFile(getLock);
        goto PROCESSDELETEOBJ_END;
    }

    /* validate if the object can be deleted */
    rc = ism_config_json_validateDeleteObjects(object, inst);
    if ( rc == ISMRC_OK ) {
        /* find object */
        objroot = json_object_get(srvConfigRoot, object);
        if ( objroot ) {
            /* find instance */
            if ( inst ) {
                instroot = json_object_get(objroot, inst);
                if ( instroot ) {
                    int isComposite2 = 0;
                    ism_ConfigComponentType_t compType2 = ism_config_findSchemaGetComponentType((char *)object, &isComposite2, NULL);
                    if ( compType2 == ISM_CONFIG_COMP_LAST ) {
                        rc = ISMRC_InvalidCfgObject;
                        ism_common_setErrorData(rc, "%s", object?object:"NULL");
                        goto PROCESSDELETEOBJ_END;
                    }
                    ism_config_t *handle2 = ism_config_getHandle(compType2, NULL);
                    int haSync2 = 0;

                    if ( strcmp(object, "MQCertificate")) {
                        rc = ism_config_json_callCallback( handle2, object, (char *)inst, instroot, haSync2, ISM_CONFIG_CHANGE_DELETE, queryOption);
                    }

                    if (rc != ISMRC_OK)
                        goto PROCESSDELETEOBJ_END;

                    /* Update json config file */
					int getLock = 0;  /* We do have config lock */

					/* If object is CertificateProfile - delete cert and key file */
					if ( strcmp(object, "CertificateProfile") == 0 ) {
						char *cert = NULL;
						char *key = NULL;
						int rc1 = ism_config_getCertificateProfileKeyCert((char *)inst, &cert, &key, getLock);
						if ( rc1 == ISMRC_OK ) {
							ism_config_deleteCertificateProfileKeyCert(cert, key, 1, 1);
						}
					} else if ( strcmp(object, "LTPAProfile") == 0 ) {
						char *key = NULL;
						int rc1 = ism_config_getLTPAProfileKey((char *)inst, &key, getLock);
						if ( rc1 == ISMRC_OK ) {
							ism_config_deleteLTPAKeyFile(key);
						}
					} else if ( strcmp(object, "OAuthProfile") == 0 ) {
						char *key = NULL;
						int rc1 = ism_config_getOAuthProfileKey((char *)inst, &key, getLock);
						if ( rc1 == ISMRC_OK ) {
							ism_config_deleteOAuthKeyFile(key);
						}
					} else if ( strcmp(object, "CRLProfile") == 0) {
						ism_config_deleteCRLProfile((char *) inst);
					} else if ( strcmp(object, "MQCertificate") == 0 ) {
						ism_config_deleteMQCerts();
					} else if ( strcmp(object, "Endpoint") == 0) {
					    /* cancel CRL Timer */
					    ism_config_deleteEndpointCRLTimer(inst);
					}

					if ( !strcmp(object, "DestinationMappingRule") || !strcmp(object, "QueueManagerConnection")) {

						if ( mqcBuffer ) {
							char buf[4098];
							int buflen = snprintf(buf, sizeof(buf),
									"{ \"ConfigurationDelete\": { \"Object\":\"%s\",\"Instance\":\"%s\" }}",
									object, inst);
							ism_common_allocBufferCopyLen(mqcBuffer, buf, buflen);
						}

						if ( standbyCheck == 1 ) {
							json_object_del(objroot, inst);
							rc = ism_config_json_updateFile(0);
						}

					} else {
						json_object_del(objroot, inst);
						rc = ism_config_json_updateFile(getLock);
					}

					if ( !strcmp(object, "SecurityProfile")) {
						if ( rc == ISMRC_OK ) {
							rc = ism_config_purgeSecurityCRLProfile(inst);
							if (rc == ISMRC_OK)
								ism_config_json_purgeTrustStore(inst);
						}
						int updateConfigFile = 1;
						ism_config_json_cleanOrphanedSecProfObjects(getLock, updateConfigFile);
					} else if (!strcmp(object, "ResourceSetDescriptor")) {
					    ism_config_markResourceSetDescriptorDeleted(standbyCheck);
					}

                } else {
                    TRACE(3, "Object:%s Instance:%s is not found.\n", object?object:"NULL", inst?inst:"NULL");
                    rc = ISMRC_NotFound;
                    ism_common_setError(ISMRC_NotFound);
                }
            }
        } else {
        	    if ( object && ( !strcmp(object, "DestinationMappingRule") || !strcmp(object, "QueueManagerConnection"))) {
                TRACE(3, "Object:%s Instance:%s is not found.\n", object?object:"NULL", inst?inst:"NULL");
                rc = ISMRC_NotFound;
                ism_common_setError(ISMRC_NotFound);
        	    }
        }
    }

PROCESSDELETEOBJ_END:

    /* Unlock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

    return rc;
}

/**
 * Process HTTP Delete for Singleton Objects
 */
XAPI int ism_config_json_deleteSingleton(char *object, int deleteValueOnly, int deleteFile, const char *prefixpath) {
	int rc = ISMRC_OK;
	int isComposite = 0;

	json_t *objvalue = NULL;

	if ( !object ) {
	        rc = ISMRC_NullPointer;
	        ism_common_setError(rc);
	        return rc;
	}

    /* Get component type */
    ism_ConfigComponentType_t compType = ism_config_findSchemaGetComponentType((char *)object, &isComposite, NULL);

    if (compType < ISM_CONFIG_COMP_SERVER || compType >= ISM_CONFIG_COMP_LAST || isComposite) {
    	rc = ISMRC_DeleteNotAllowed;
    	goto DELETESINGLETON_END;
    }

    /* Get write lock */
    pthread_rwlock_wrlock(&srvConfiglock);
    /* find object */
    objvalue = json_object_get(srvConfigRoot, object);
    if (objvalue) {
    	if (json_is_string(objvalue)) {
    		if (deleteFile) {
    			const char *filename = json_string_value(objvalue);
    			char fullpathFile[strlen(prefixpath)+strlen(filename)+1];
    			sprintf(fullpathFile,"%s/%s", prefixpath, filename);
    			unlink(fullpathFile);
    		}

    		if (deleteValueOnly) {
    			const char *delStr = "";
    			json_object_set(srvConfigRoot, object, json_string(delStr));
    		} else {
    			json_object_del(srvConfigRoot, object);
    		}
    		rc = ism_config_json_updateFile(0);

    	} else {
    		rc = ISMRC_SingltonDeleteError;
    	}
    } else {
    	rc = ISMRC_NotFound;
    }

DELETESINGLETON_END:

	/* Unlock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

	return rc;
}


/**
 * Process HTTP Delete
 */
XAPI int ism_config_json_processDelete(ism_http_t *http, ism_rest_api_cb callback) {
    int rc = ISMRC_OK;

    if ( !http ) {
        /* This is a bad request for Delete */
        rc = ISMRC_BadRESTfulRequest;
        return rc;
    }

    /* Make sure there is no payload */
    if ( http && http->content_count > 0 ) {
        int noErrorTrace = 1;
        json_t * get = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);

        if ( get || (rc != ISMRC_BadRESTfulRequest && rc != ISMRC_OK)) {
            TRACE(4, "ism_config_json_processDelete() returned error code: %d\n", rc);
            /* This is a bad request for Delete */
            rc = ISMRC_BadRESTfulRequest;
            if (get)
                json_decref(get);
            return rc;
        }
    }
    ism_common_setError(0);
    rc = ISMRC_OK;

    /* check version with current schema version
     * This check is not required in v2.0 release. The check will be required if we update
     * configuration schema in a future release - basically to support multiple versions of
     * REST APIs. */
    char *ver = http->version;
    if ( ver && *ver != '\0' && strcmp(http->version, SERVER_SCHEMA_VERSION)) {
        TRACE(7, "Version is not specified in URI. Use current configuration schema version: %s\n", SERVER_SCHEMA_VERSION);
    }


    /* Get object info from path */
    char *object = NULL, *more = NULL, *inst = NULL, *subinst = NULL, *tmpstr = NULL;

    bool requireRunningServer = false;

    if (http->user_path && strlen(http->user_path) > 1) {
        tmpstr = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),http->user_path);
        object = ism_common_getToken(tmpstr, "/", "/", &more );

        /* Composite Objects */
        if ( !strcmp(object, "TrustedCertificate") || !strcmp(object, "ClientCertificate")) {
            if ( more != NULL ) {
                inst = ism_common_getToken(more, "/", "/", &more );
                if ( more != NULL ) {
                    subinst =  ism_common_getToken(more, "/", "/", &more );
                }
            }
        } else if ( !strcmp(object, "TopicMonitor") || !strcmp(object, "Queue") ||
                    !strcmp(object, "ClusterRequestedTopics") || !strcmp(object, "Subscription")) {
            inst = more;
        } else if ( !strcmp(object, "AdminSubscription") ||
                    !strcmp(object, "DurableNamespaceAdminSub") ||
                    !strcmp(object, "NonpersistentAdminSub") ) {
            requireRunningServer = true;
            inst = more;
        } else {
            if (more && http->val2 != 9) {
                inst = ism_common_getToken(more, "/", "/", &more );
                if (more && *more) {
                    rc = ISMRC_BadRESTfulRequest;
                    ism_common_setErrorData(rc, "%s", http->user_path);
                    goto DELFUNCEND;
                }
            } else {
                inst = more;
            }
        }
    }

    if ( !object || *object == '\0' ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        goto DELFUNCEND;
    }

    if (requireRunningServer && ism_admin_get_server_state() != ISM_SERVER_RUNNING) {
        rc = ISMRC_RequireRunningServer;
        ism_common_setErrorData(rc, "%s", object);
        goto DELFUNCEND;
    }

    if ( !strcmp(object, "MQCertificate") ) {
		if ( inst && *inst != '\0' ) {
			rc = ISMRC_BadRESTfulRequest;
			ism_common_setErrorData(rc, "%s", http->user_path);
			goto DELFUNCEND;
		}
		inst = "MQCert";
    }

    if ( !strcmp(object, "ResourceSetDescriptor") ) {
        if ( inst && *inst != '\0' ) {
            rc = ISMRC_BadRESTfulRequest;
            ism_common_setErrorData(rc, "%s", http->user_path);
            goto DELFUNCEND;
        }
        inst = "ResourceSetDescriptor";
    }

    /* Get values from query parameters */
    int queryOption = 0;

    if (http->query && *http->query) {
        const char *optionName;

        if (!strcmp(object, "Queue")) {
            optionName = "DiscardMessages";
        } else if (!strcmp(object, "AdminSubscription") ||
                   !strcmp(object, "DurableNamespaceAdminSub") ||
                   !strcmp(object, "NonpersistentAdminSub") ) {
            optionName = "DiscardSharers";
        } else {
            optionName = NULL;
        }

        if (optionName != NULL) {
            int len = strlen(http->query);
            char *qparam = alloca(len + 1);
            char *token;
            char *nexttoken;
            memcpy(qparam, http->query, len);
            qparam[len] = '\0';

            for (token = strtok_r(qparam, "&", &nexttoken); token != NULL;
                    token = strtok_r(NULL, "&", &nexttoken)) {
                //split token into key,value
                char *key= token;
                char *value = strstr(token, "=");
                if ( value != NULL) {
                    char *p = value;
                    value = value +1;
                    *p = '\0';
                }
                if (!strcmp(key, optionName) && !strcmpi(value, "true")) {
                    queryOption = 1;
                }
            }
        }
    }

    int standbyCheck = 0;
    char tbuf[2048];
    concat_alloc_t mqcBuffer = { tbuf, sizeof(tbuf), 0, 0 };
    rc = ism_config_json_deleteObject(object, inst, subinst, queryOption, standbyCheck, &mqcBuffer);

    if ( rc == ISMRC_OK && mqcBuffer.used > 0 ) {
        /* buffer contains MQCbridge configuration
         * - Send data to mqc bridge process and do not invoke http callback
         */
        TRACE(7, "Send delete message to MQC process: len=%d msg=%s\n", mqcBuffer.used, mqcBuffer.buf);
        int persistData = 1;
        int objID = ism_config_getMQCObjectID((char *)object);
        rc = ism_admin_mqc_send(mqcBuffer.buf, mqcBuffer.used, http, callback, persistData, objID, inst);
        if ( rc == ISMRC_OK || rc == ISMRC_AsyncCompletion) {
            if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
            return rc;
        }
    }

DELFUNCEND:

    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);

    /* Update error message */
    if ( rc != ISMRC_OK ) {
        const char * repl[1];
        int replSize = 0;
        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
    }

    return rc;
}

