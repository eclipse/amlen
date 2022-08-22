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
 * janssonProcessGet.c
 */

#define TRACE_COMP Admin

#include <fcntl.h>
#include <string.h>
#include <janssonConfig.h>
#include "configInternal.h"

extern json_t * ism_config_listTruststoreCertificate(char *object, char *profileName, char *certName, int *count);

extern json_t *srvConfigRoot;                 /* Server configuration JSON object  */
extern pthread_rwlock_t    srvConfiglock;     /* Lock for servConfigRoot           */

#define CRLFCHAR "\n"

static void replacePassword(char * object, json_t *retval) {
    const char *objkey = NULL;
    json_t * objval = NULL;

    if (!object)
        return;

    if (!strcmp(object, "LDAP")) {
        json_t *ldapObj = json_object_get(retval, "LDAP");
        if (ldapObj && json_typeof(ldapObj) == JSON_OBJECT) {
            json_t *bpwd = json_object_get(ldapObj, "BindPassword");
            if ( bpwd  && json_typeof(bpwd) == JSON_STRING) {
                const char *value = json_string_value(bpwd);
                if (value && *value != '\0') {
                    json_object_set(ldapObj, "BindPassword", json_string("XXXXXX"));
                }
            }
        }
    } else if (!strcmp(object, "LTPAProfile")) {
        json_t *ltpaObj = json_object_get(retval, "LTPAProfile");
        if (ltpaObj && json_typeof(ltpaObj) == JSON_OBJECT) {
            json_object_foreach(ltpaObj, objkey, objval) {
                json_t *bpwd = json_object_get(objval, "Password");
                if ( bpwd  && json_typeof(bpwd) == JSON_STRING) {
                    const char *value = json_string_value(bpwd);
                    if (value && *value != '\0') {
                        json_object_set(objval, "Password", json_string("XXXXXX"));
                    }
                }
            }
        }
    } else if (!strcmp(object, "QueueManagerConnection")) {
        json_t *qmObj = json_object_get(retval, "QueueManagerConnection");
        json_object_foreach(qmObj, objkey, objval) {
            if ( objval  && json_typeof(objval) == JSON_OBJECT ) {
                json_t *bpwd = json_object_get(objval, "ChannelUserPassword");
                if ( bpwd  && json_typeof(bpwd) == JSON_STRING ) {
                    const char *value = json_string_value(bpwd);
                    if (value && *value != '\0') {
                        json_object_set(objval, "ChannelUserPassword", json_string("XXXXXX"));
                    }
                }
            }
        }
    }
    return;
}


/**
 * Process HTTP Get
 */
XAPI int ism_config_json_processGet(ism_http_t *http) {
    int rc = ISMRC_OK;
    int isTC = 0;
    int isArrayType = 0;
    int isPL = 0;
    int found = 1;

    /* Make sure there is no payload */
    if ( http && http->content_count > 0 ) {
        int noErrorTrace = 1;
        json_t * get = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);

        if ( get || (rc != ISMRC_BadRESTfulRequest && rc != ISMRC_OK)) {
            TRACE(4, "ism_config_json_processGet() returned error code: %d\n", rc);
            /* This is a bad request for GET */
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
    char *object = NULL, *more = NULL, *inst = NULL, *subinst = NULL, *tmpstr = NULL, *pluginPropName = NULL;

    if (http->user_path && strlen(http->user_path) > 1) {
        tmpstr = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),http->user_path);
        object = ism_common_getToken(tmpstr, "/", "/", &more );

        if ( !strcmp(object, "TrustedCertificate") || !strcmp(object, "ClientCertificate")) {
            isTC = 1;
            if ( more != NULL ) {
                inst = ism_common_getToken(more, "/", "/", &more );
                if ( more != NULL ) {
                    subinst =  ism_common_getToken(more, "/", "/", &more );
                }
            }
        } else if ( !strcmp(object, "TopicMonitor") || !strcmp(object, "ClusterRequestedTopics")) {
            isArrayType = 1;
            inst = more;
        } else if ( !strcmp(object, "Plugin")) {
        	isPL = 1;
            if ( more != NULL ) {
                inst = ism_common_getToken(more, "/", "/", &more );
                if ( more != NULL ) {
                	pluginPropName = more;
                } else {
                	pluginPropName = more;
                }
            } else {
                inst = more;
            }
        } else if ( !strcmp(object, "Subscription") ||
                    !strcmp(object, "Queue") ||
                    !strcmp(object, "AdminSubscription") ||
                    !strcmp(object, "DurableNamespaceAdminSub") ||
                    !strcmp(object, "NonpersistentAdminSub")) {
            inst = more;
        } else {
            if (more) {
                inst = ism_common_getToken(more, "/", "/", &more );
                if (more && *more) {
                    rc = ISMRC_BadRESTfulRequest;
                    ism_common_setErrorData(rc, "%s", http->user_path);
                    if (tmpstr)
                        ism_common_free(ism_memory_admin_misc,tmpstr);
                    return rc;
                }
            } else {
                inst = more;
            }
        }
    }


    if ( !object || *object == '\0' ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
        return rc;
    }

    /* Find schema object */
    int itype = 0;
    int otype = 0;
    int comp  = 0;

    if ( !strcmp(object, "MQTTClient") || ism_config_findSchemaObject(object, NULL, &comp, &otype, &itype) == NULL ) {
        rc = ISMRC_InvalidCfgObject;
        ism_common_setErrorData(rc, "%s", object);
        if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
        return rc;
    }

    /* For singleton, do not allow inst */
    if ( otype == 0 && inst ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->path);
        TRACE(3, "%s: schemaObj is null or otype = 0 and has inst.\n", __FUNCTION__);
        if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
        return rc;
    }

    /* get object from configuration */
    /* get object from current configuration */
    json_t *objroot = NULL;
    json_t *instroot = NULL;
    json_t *retval = NULL;
    json_t *newval = NULL;
    json_t *tcobj = NULL;
    char *instStr = NULL;

    /* Set inst for AdminEndpoint, LDAP, HighAvailability, ResourceSetDescriptor, Cluster and Syslog */
    int singleObject = 0;
    if ( *object == 'A' && !strcmp(object, "AdminEndpoint")) {
        if ( !inst || *inst == '\0' ) inst = "AdminEndpoint";
        singleObject = 1;
    } else if ( *object == 'S' && !strcmp(object, "Syslog")) {
        if ( !inst || *inst == '\0' ) inst = "Syslog";
        singleObject = 1;
    } else if ( *object == 'L' && !strcmp(object, "LDAP")) {
        if ( !inst || *inst == '\0' ) inst = "ldapconfig";
        singleObject = 1;
    } else if ( *object == 'H' && !strcmp(object, "HighAvailability")) {
        if ( !inst || *inst == '\0' ) inst = "haconfig";
        singleObject = 1;
    } else if ( *object == 'C' && !strcmp(object, "ClusterMembership")) {
        if ( !inst || *inst == '\0' ) inst = "cluster";
        singleObject = 1;
    } else if ( *object == 'M' && !strcmp(object, "MQCertificate")) {
        if ( !inst || *inst == '\0' ) inst = "MQCert";
        singleObject = 1;
    } else if ( *object == 'R' && !strcmp(object, "ResourceSetDescriptor")) {
        if ( !inst || *inst == '\0' ) inst = "ResourceSetDescriptor";
        singleObject = 1;
    }

    /* Get read lock */
    pthread_rwlock_rdlock(&srvConfiglock);

    /* Create return object */
    retval = json_object();
    /* Add API version */
    json_object_set_new(retval, "Version", json_string("v1"));

    /* find object */
    objroot = json_object_get(srvConfigRoot, object);

    if ( objroot ) {

        if ( isTC == 0 && isArrayType == 0 ) {
            if (inst && *inst) {
                instroot = json_object_get(objroot, inst);
                if ( instroot ) {
                    if ( isPL == 1 && pluginPropName ) {
                    	char pName[1024];
                    	sprintf(pName, "Plugin/%s/%s", inst, pluginPropName);
                    	if ( !strcmp(pluginPropName, "PropertiesFile")) {
                   			/* Return contents of properties file */
                   			char filepath[2048];
                   			sprintf(filepath,IMA_SVR_DATA_PATH "/data/config/plugin/plugins/%s/pluginproperties.json", inst);
           					FILE * fd = fopen(filepath, "rb");
           					if (!fd) {
           						TRACE(3, "Could not open Plugin \"%s\" properties file \"%s\". errno: %d\n", inst, "PropertiesFile", errno);
           						if ( errno == 2 ) {
           						    rc = 6198;
           						    ism_common_setErrorData(rc, "%s", pName);
           						} else {
           							rc = ISMRC_Error;
           							ism_common_setError(rc);
           						}

                       		    pthread_rwlock_unlock(&srvConfiglock);
                       		    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
                       		    return rc;
           					}

           					fseek(fd, 0, SEEK_END);
           					long fsize = ftell(fd);
           					fseek(fd, 0, SEEK_SET);
           					char *fcontent = alloca(fsize + 1);

           					long bread = fread(fcontent, fsize, 1, fd);
           					if (bread != 1) {
           						TRACE(9, "The file %s cannot be read successfully. Size=%ld Read=%ld\n", filepath, fsize, bread);
           					}
           					fclose(fd);
           					fcontent[fsize] = 0;

                   			ism_common_allocBufferCopyLen(&http->outbuf, fcontent, fsize);
                   			ism_common_allocBufferCopyLen(&http->outbuf, "\n", 1);

                   		    /* Unlock config lock */
                   		    pthread_rwlock_unlock(&srvConfiglock);

                   		    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
                   		    return rc;

                   		} else {
   						    rc = 6198;
   						    ism_common_setErrorData(rc, "%s", pName);
                   		    /* Unlock config lock */
                   		    pthread_rwlock_unlock(&srvConfiglock);

                   		    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
                   		    return rc;
                   		}

                    } else {

						if ( singleObject == 1 ) {
							json_object_set_new(retval, object, instroot);
						} else {
							json_t *obj = json_object();
							json_object_set_new(retval, object, obj);
							json_object_set_new(obj, inst, instroot);
						}

                    }
                } else {
                    found = 0;
                }
            } else {
                json_object_set_new(retval, object, objroot);
            }

        } else if ( isArrayType == 1 ) {

            if (inst && *inst) {
                /* TopicMonitor / ClusterRequestedTopics objects are array */
                /* Loop thru array and find the required item */
                found = 0;
                int i = 0;
                for (i=0; i<json_array_size(objroot); i++) {
                    json_t *instObj = json_array_get(objroot, i);
                    if (instObj) {
                        const char *val = json_string_value(instObj);
                        if (val && !strcmp(val, inst)) {
                            retval = json_object();
                            json_object_set_new(retval, "Version", json_string("v1"));
                            json_t *array = json_array();
                            json_object_set_new(retval, object, array);
                            json_t *newInstRoot = json_string(inst);
                            json_array_append(array, newInstRoot);
                            found = 1;
                            break;
                        }
                    }
                }
            } else {
                json_object_set_new(retval, object, objroot);
            }

        }

    } else {
        if ( isTC == 1 ) {
            /* Return TrustedCertificate and ClientCertificate objects */
            int count = 0;
            tcobj = ism_config_listTruststoreCertificate(object, inst, subinst, &count);
            if ( count == 0 && (inst || subinst) ) {
                int instlen = 0;
                if ( inst) instlen = strlen(inst);
                if (subinst ) {
                    instlen += strlen(subinst);
                }
                if ( instlen > 0 ) {
                    instStr = (char *)alloca(instlen + 2);
                    if ( subinst ) {
                        sprintf(instStr, "%s/%s", inst, subinst);
                    } else {
                        sprintf(instStr, "%s", inst);
                    }
                }
                found = 0;
            } else {
                json_object_set_new(retval, object, tcobj);
                found = 1;
            }
        } else if ( isArrayType == 1 ) {
            json_t *obj = json_array();
            json_object_set_new(retval, object, obj);
        } else {
            if (inst && *inst) {
                found = 0;
            } else {
                json_t *obj = json_object();
                json_object_set_new(retval, object, obj);
            }
        }
    }

    if ( found == 0 ) {
        rc = ISMRC_ObjectNotFound;
        if ( instStr ) {
            ism_common_setErrorData(rc, "%s%s", object, instStr);
        } else {
            ism_common_setErrorData(rc, "%s%s", object, inst?inst:"NULL");
        }
        if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);
        pthread_rwlock_unlock(&srvConfiglock);
        return rc;
    }

    char *buf = NULL;
    if (!strcmp(object, "LDAP") || !strcmp(object, "LTPAProfile") || !strcmp(object, "QueueManagerConnection")) {
        newval = json_deep_copy(retval);
        replacePassword(object, newval);
        buf = json_dumps(newval, JSON_INDENT(2)|JSON_PRESERVE_ORDER);
    } else {
        buf = json_dumps(retval, JSON_INDENT(2)|JSON_PRESERVE_ORDER);
    }

    /* dump object in buffer and return */
    if ( buf ) {
        ism_common_allocBufferCopyLen(&http->outbuf, buf, strlen(buf));
        ism_common_allocBufferCopyLen(&http->outbuf, CRLFCHAR, strlen(CRLFCHAR));
        // buffer created by jansson
        ism_common_free_raw(ism_memory_admin_misc,buf);
    }

    if ( newval ) json_decref(newval);
    if ( tcobj ) json_decref(tcobj);

    /* Unlock config lock */
    pthread_rwlock_unlock(&srvConfiglock);

    if ( tmpstr ) ism_common_free(ism_memory_admin_misc,tmpstr);

    return rc;
}

/**
 * API to return SNMPEnabled flag
 */
XAPI int ism_admin_isSNMPConfigured(int locked) {
    int enabled = 0;

    if (!locked)
    	pthread_rwlock_rdlock(&srvConfiglock);

    json_t *snmpEnabled = json_object_get(srvConfigRoot, "SNMPEnabled");
    if ( snmpEnabled && json_typeof(snmpEnabled) == JSON_TRUE) {
        enabled = 1;
    }

    if (!locked)
    	pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}

/**
 * API to return ClusterMembership enabled flag
 */
XAPI int ism_admin_isClusterConfigured(void) {
    int enabled = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    json_t *cmObj = json_object_get(srvConfigRoot, "ClusterMembership");
    if ( cmObj ) {
    	json_t *clobj = json_object_get(cmObj, "cluster");
    	if ( clobj ) {
            json_t *ceobj = json_object_get(clobj, "Enabled");
            if ( ceobj && json_typeof(ceobj) == JSON_TRUE) {
            	enabled = 1;
            } else {
                ceobj = json_object_get(clobj, "EnableClusterMembership");
                if ( ceobj && json_typeof(ceobj) == JSON_TRUE) enabled = 1;
            }
        }
    }

    pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}


/**
 * API to return HA enabled flag
 */
XAPI int ism_admin_isHAEnabled(void) {
    int enabled = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    json_t *cmObj = json_object_get(srvConfigRoot, "HighAvailability");
    if ( cmObj ) {
    	json_t *clobj = json_object_get(cmObj, "haconfig");
    	if ( clobj ) {
            json_t *ceobj = json_object_get(clobj, "Enabled");
            if ( ceobj && json_typeof(ceobj) == JSON_TRUE) {
            	enabled = 1;
            } else {
                ceobj = json_object_get(clobj, "EnableHA");
                if ( ceobj && json_typeof(ceobj) == JSON_TRUE) enabled = 1;
            }
        }
    }

    pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}

/**
 * API to return SNMP enabled flag
 */
XAPI int ism_admin_isSNMPEnabled(void) {
    int enabled = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    json_t *cmObj = json_object_get(srvConfigRoot, "SNMPEnabled");
    if ( cmObj && json_typeof(cmObj) == JSON_TRUE) enabled = 1;

    pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}

/**
 * API to return MQConnectivity enabled flag
 */
XAPI int ism_admin_isMQConnectivityEnabled(void) {
    int enabled = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    json_t *cmObj = json_object_get(srvConfigRoot, "MQConnectivityEnabled");
    if ( cmObj && json_typeof(cmObj) == JSON_TRUE) enabled = 1;

    pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}


/**
 * API to return 1 if plugin is configured
 */
XAPI int ism_admin_isPluginEnabled(void) {
    int enabled = 0;

    pthread_rwlock_rdlock(&srvConfiglock);

    json_t *cmObj = json_object_get(srvConfigRoot, "Plugin");
    if ( cmObj ) {
    	const char *objkey;
    	void *objval;
    	json_object_foreach(cmObj, objkey, objval) {
    		json_t *fileobj = json_object_get(objval, "File");
    		if ( fileobj && json_typeof(fileobj) == JSON_STRING ) {
    			enabled = 1;
    			break;
    		}
    		fileobj = json_object_get(objval, "PropertiesFile");
    		if ( fileobj && json_typeof(fileobj) == JSON_STRING ) {
    			enabled = 1;
    			break;
    		}
    	}
    }

    pthread_rwlock_unlock(&srvConfiglock);

    return enabled;
}

/**
 * Returns AdminEndpoint port
 */
XAPI int ism_config_json_getAdminPort(int getLock) {
	int port = 0;

	if ( getLock == 1)pthread_rwlock_wrlock(&srvConfiglock);

	json_t *obj = json_object_get(srvConfigRoot, "AdminEndpoint");
	if ( obj ) {
	    json_t *inst = json_object_get(obj, "AdminEndpoint");
	    if ( inst ) {
	    	json_t *portObj = json_object_get(inst, "Port");
	    	if ( portObj && json_typeof(portObj) == JSON_INTEGER ) {
	    		port = json_integer_value(portObj);
	    	}
	    }
	}

	if ( getLock == 1)pthread_rwlock_unlock(&srvConfiglock);

	return port;
}

/**
 * Returns AllowedAdminActions for the user in GET response
 */
XAPI int ism_config_json_getUserAuthority(ism_http_t *http) {
    return ISMRC_NotImplemented;
}

/**
 * Returns the file contents of the specified file in IMA_SVR_DATA_PATH/userfiles dir
 */
XAPI int ism_config_json_getFile(ism_http_t *http, char *fname) {
    FILE * fd = NULL;
    long bread;
    int rc = ISMRC_OK;
    char *fcontent = NULL;

    /* validate file name - should not have ./, ../, /. or /.. */
    if ( !fname || *fname == '\0' ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        goto CONFIG_END;
    }

    if ( strstr(fname, "./") || strstr(fname, "../") || strstr(fname, "/.") || strstr(fname, "/..") || strlen(fname) > 1024 ) {
        rc = ISMRC_BadRESTfulRequest;
        ism_common_setErrorData(rc, "%s", http->user_path);
        goto CONFIG_END;
    }

    /* Read file into a buffer */
    char filepath[2048];
    sprintf(filepath, IMA_SVR_DATA_PATH "/userfiles/%s", fname);

    fd = fopen(filepath, "rb");
    if (!fd) {
		TRACE(9, "%s: File is not found: %s.\n", __FUNCTION__, filepath);
		rc = ISMRC_NotFound;
		ism_common_setError(rc);
		goto CONFIG_END;
    }

	fseek(fd, 0, SEEK_END);
	long fsize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	fcontent = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,237),fsize + 1);
    if (!fcontent) {
    	fclose(fd);
        TRACE(3, "Unable to allocate memory for file: %s.\n", filepath);
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto CONFIG_END;
    }


	bread = fread(fcontent, fsize, 1, fd);
    if (bread != 1) {
    	fclose(fd);
        TRACE(3, "The file %s cannot be read successfully. Size=%ld Read=%ld\n", filepath, fsize, bread);
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto CONFIG_END;
    }

	fclose(fd);
	fcontent[fsize] = 0;

	ism_common_allocBufferCopyLen(&http->outbuf, fcontent, fsize);

CONFIG_END:

    if ( fcontent ) ism_common_free(ism_memory_admin_misc,fcontent);

    return rc;
}

/**
 * Returns the list of files and dir in IMA_SVR_DATA_PATH/userfiles dir
 */
XAPI int ism_config_json_getFileList(ism_http_t *http, char *fname) {
    int rc = ISMRC_OK;

    struct dirent *dent;
    char *basedir = IMA_SVR_DATA_PATH "/userfiles";

    DIR * bDir = opendir(basedir);

    if (bDir == NULL) {
    	TRACE(3, "Could not open " IMA_SVR_DATA_PATH "/userfiles directory. errno:%d\n", errno);
    	rc = ISMRC_NotFound;
    	ism_common_setError(rc);
    	return rc;
    }

    ism_common_allocBufferCopyLen(&http->outbuf, "List of files:\n", 15);

    char linebuf[4098];

    while ((dent = readdir(bDir)) != NULL) {
    	struct stat st;

    	if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;

    	TRACE(1, "DEBUG: file: %s\n", dent->d_name);

    	stat(dent->d_name, &st);

    	if ( S_ISDIR(st.st_mode) != 0 )
    		continue;

    	sprintf(linebuf, "%s\n", dent->d_name );
    	ism_common_allocBufferCopyLen(&http->outbuf, linebuf, strlen(linebuf));
    }
    closedir(bDir);

    return rc;
}
