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
 * This file contains internal data, functions functions etc, required by
 * Generic data validation APIs defined in validate_genericData.c file.
 */

#define TRACE_COMP Admin

#include "validateInternal.h"
#include "configInternal.h"

extern int ism_config_getUpdateCertsFlag(char *item);
extern pthread_key_t adminlocalekey;
extern int ism_config_getValidationDataIndex(char *objName);

/*
 * create configuration log
 *
 * @param  item                   The configuration object item
 * @param  name                   The configuration object name
 * @param  rc                     The error code
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 * */
void ism_config_addConfigLog(char *item, char *name, int rc) {

    TRACE(3, "Configuration change callback failed.: item=%s rc=%d\n", item, rc);
    char buf[1024];
    char *bufptr = buf;
    char *errstr = NULL;
    int inheap = 0;

    /*Since the error will be in the LOG file. the error message needs to be in System Locale*/

    if ( ism_common_getLastError() > 0 ) {
		const char * systemLocale = ism_common_getLocale();
		int bytes_needed = 0;
		if(errstr!=NULL){
			bytes_needed= ism_common_formatLastErrorByLocale(systemLocale, bufptr, sizeof(buf));
			if (bytes_needed > sizeof(buf)) {
				bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,302),bytes_needed);
				inheap=1;
				bytes_needed = ism_common_formatLastErrorByLocale(systemLocale, bufptr, bytes_needed);
			}
		}
		if (bytes_needed > 0) {
			errstr=(char *)&buf;
		} else {
			errstr = (char *)ism_common_getErrorStringByLocale(rc, systemLocale, buf, sizeof(buf));
		}
    }

    LOG(ERROR, Admin, 6118, "%-s%-s%-s%-s%d", "Configuration Error is detected in {0} configuration item. {1}={2}, Error={3}, RC={4}",
            item, "ObjectName", name?name:"", errstr?errstr:"", rc);

    if ( inheap == 1 ) ism_common_free(ism_memory_admin_misc,bufptr);

    return;
}

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
 * Returns ISMRC_OK on success, ISMRC_* on failure.
 * */
int ism_config_rollbackCallbacks(char *item, char *name, int callbackList[], ism_prop_t *props, int mode, int action) {
	int rc = ISMRC_OK;
    /*action  0: create
     *        1: update
     *        2: delete
     */
	int rollbackMode = 0;
	if (action == 0) { // action = create, mode = ISM_CONFIG_CHANGE_NAME
		rollbackMode = ISM_CONFIG_CHANGE_DELETE;
	} else if (action == 1) { // action = update, mode = ISM_CONFIG_CHANGE_NAME
		rollbackMode = ISM_CONFIG_CHANGE_NAME;
	} else if (action == 2) { // action = delete, mode = ISM_CONFIG_CHANGE_DELETE
		rollbackMode = ISM_CONFIG_CHANGE_NAME;
	}

	TRACE(7, "Entry %s: item: %s, name: %s, callbackList: %p, mode: %d, props: %p, rollbackMode: %d\n",
	        __FUNCTION__, item?item:"null", name, callbackList, mode, props?props:0, rollbackMode );

	// Calls all callbacks that were completed successfully.
	// The callbacks need to be called in the reverse order when rolling back changes.
	int count;
	for (count = 0; count < ISM_CONFIG_COMP_LAST; count++) {
		if (callbackList[count] == -1) {
			continue;
		}

		int comptype = callbackList[count];

		ism_config_t * compHandle = ism_config_getHandle(comptype, NULL);

		if (compHandle && compHandle->callback) {
			TRACE(8, "Invoke config callback: comptype=%d. Item:%s. Name:%s.\n", comptype, item, name?name:"");
			rc = compHandle->callback(item, name, props, rollbackMode);
			if (rc != ISMRC_OK) {
				// Rollback of callback failed as well. Just error out.
				TRACE(3, "%s: Failed to update repository. Rollback of callback failed for comptype: %d\n", __FUNCTION__, comptype);
				goto ROLLBACK_CALLBACKS_EXIT;
			}
		}
	}

ROLLBACK_CALLBACKS_EXIT:

	return rc;
}

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
int ism_config_multipleCallbacks(ism_config_t *handle, char *item, char *name, ism_prop_t *props, int mode, int action, char *callbackList) {
	int rc = ISMRC_OK;

	TRACE(7, "Entry %s: item: %s, name: %s, mode: %d, props: %p, action: %d\n",
	        __FUNCTION__, item?item:"null", name, mode, props?props:0, action );

	/* Get current properties */
	ism_prop_t *currentProps = ism_config_getProperties(handle, item, name);

	int len = strlen(callbackList);
	int rollbackList[ISM_CONFIG_COMP_LAST];
	int i, count;
	char *tmpstr = (char *)alloca(len+1);
	memcpy(tmpstr, callbackList, len);
	tmpstr[len] = 0;

	// Create reverse callbackList to be used for rollback in case of error
	for (i = 0; i < ISM_CONFIG_COMP_LAST; i++) {
		rollbackList[i] = -1;
	}

	char *token, *nexttoken = NULL;
	for (token = strtok_r(tmpstr, ",", &nexttoken), count = 0; token != NULL;
		token = strtok_r(NULL, ",", &nexttoken), count++)
	{
		/* token == entry from callbackList i.e. Engine, Cluster, Transport etc */
		int comptype = ism_config_getCompType(token);
		comptype = ism_config_getCompType(token);

		ism_config_t * compHandle = ism_config_getHandle(comptype, NULL);

		if (compHandle && compHandle->callback) {
			TRACE(8, "Invoke config callback: comptype=%d. Item:%s. Name:%s.\n", comptype, item, name?name:"");
			rc = compHandle->callback(item, name, props, mode);
			if (rc != ISMRC_OK) {
				TRACE(3, "%s: Failed to update repository. Callback failed for comptype: %d\n", __FUNCTION__, comptype);
				ism_config_addConfigLog(item, name, rc);
				if (count == 0) {
					// Failed on first callback, nothing to rollback
					goto CALL_CALLBACKS_EXIT;
				} else {
					// At least 1 callback was completed successfully. Call rollback
					rc = ism_config_rollbackCallbacks(item, name, rollbackList, currentProps, mode, action);
					goto CALL_CALLBACKS_EXIT;
				}
			}
		}
		// Construct reverse callback list as we go along in case of failure
		rollbackList[ISM_CONFIG_COMP_LAST-count-1] = comptype;
	}

CALL_CALLBACKS_EXIT:

	ism_common_freeProperties(currentProps);

	return rc;
}

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
int ism_config_callCallbacks(ism_config_t *handle, char *item, char *name, int mode, ism_prop_t *props, int action) {
	int rc = ISMRC_OK;
	int callbackListExists = 0;
	char *callbackList = NULL;


	TRACE(7, "Entry %s: item: %s, name: %s, mode: %d, props: %p, action: %d\n",
	        __FUNCTION__, item?item:"null", name, mode, props?props:0, action );

	/* Get Callback list from schema for this item */
	int getCallbacksRC = ism_config_getCallbacks(ISM_CONFIG_SCHEMA, item, &callbackList);
	if (getCallbacksRC == 0) {
		callbackListExists = 1;
	}

	/* send updated configuration to the component(s) */
	if (callbackListExists) {
		TRACE(8, "%s: Invoke config callbacks: Item:%s. Name:%s.\n", __FUNCTION__, item, name );
		rc = ism_config_multipleCallbacks(handle, item, name, props, mode, action, callbackList);
	} else if (handle->callback && name != NULL) {
		TRACE(8, "%s: Invoke config callback: Item:%s. Name:%s.\n", __FUNCTION__, item, name );
		rc = handle->callback(item, name, props, mode);
	} else {
		rc = ISMRC_PropertiesNotValid;
		TRACE(3, "%s: Failed to update repository. Either component callback or item name has not been set.\n", __FUNCTION__);
		ism_common_setError(rc);
	}

    if (callbackList)	ism_common_free(ism_memory_admin_misc,callbackList);

	return rc;
}

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
int ism_config_updateConfigRepository(ism_config_t *handle, char *item, char *name, int mode, ism_prop_t *props,  ism_prop_t *cprops, int *purgeCompProp, int action) {
	int rc = ISMRC_OK;
	int i;
	ism_field_t field;
	const char * pName;

	TRACE(7, "Entry %s: item: %s, name: %s, mode: %d, props: %p, cprops: %p, action: %d\n",
	        __FUNCTION__, item?item:"null", name, mode, props?props:0, cprops?cprops:0, action );

	/* callbacks were already handled for ClusterMembership in new path */
	if (ism_config_getValidationDataIndex(item) == -1) {
		rc = ism_config_callCallbacks(handle, item, name, mode, props, action);
	}
	if (rc != ISMRC_OK) {
		ism_config_addConfigLog(item, name, rc);
		goto UPDATE_END;
	}

	/* Add config data to dynamic properties list */
	if ( mode != ISM_CONFIG_CHANGE_DELETE  && mode != ISM_CONFIG_CHANGE_NAME ) {
		for (i=0; ism_common_getPropertyIndex(props, i, &pName) == 0; i++) {
			if (!pName || !strcmpi(pName, "Delete") || !strcmp(pName, "Update") || !strcmp(pName, "UID")) {
				TRACE(9, "skip pName: %s\n", pName?pName:"null");
				continue;
			}

			ism_common_getProperty(props, pName, &field);
			/*create official name for the property*/
			int plen = strlen(item) + strlen(name) + strlen(pName) + 2;
			char *p = alloca(plen+1);
			sprintf(p, "%s.%s.%s", item, pName, name);
			p[plen] = '\0';
			//TRACE(9, "%s: new property: %s\n", __FUNCTON__, p);

			/* add into the configuration */
			ism_common_setProperty(cprops, p, &field);

			pName = NULL;
		}
	}

	/* delete composite item from dynamic property list */
	if ( mode == ISM_CONFIG_CHANGE_DELETE ) {
		uint64_t nrec = compProps[handle->comptype].nrec;
		char ** namePtrs = alloca(nrec * sizeof(char *));
		int ntot = 0;

		for (i=0; i<nrec; i++)
			namePtrs[i] = NULL;

		for (i = 0; ism_common_getPropertyIndex(cprops, i, &pName) == 0; i++) {
			if (pName != NULL) {
				int tmplen = strlen(pName) + strlen(name) + 1;
				char * tmpstr = alloca(tmplen);
				ism_common_getProperty(cprops, pName, &field);
				strcpy(tmpstr, pName);
				char *nexttoken = NULL;
				char *p1 = strtok_r((char *)tmpstr, ".", &nexttoken);
				char *p2 = strtok_r(NULL, ".", &nexttoken);
				char *p3 = strtok_r(NULL, ".", &nexttoken);
				if ( p1 && p2 && p3 ) {
					int nameLen = strlen(p1) + strlen(p2) + 2;
					char *nameStr = (char *)pName + nameLen;
					if (!strcmp(p1, item) && !strcmp(nameStr, name)) {      /* BEAM suppression: passing null object */
						/* Do NOT delete the pName directly from cprops here. it will
						 * skip the next properties inline in this iteration.
						 */
						namePtrs[ntot] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),pName);
						ntot++;
						*purgeCompProp = 1;
					}
				}
			}
		}
		if (ntot > 0) {
			for (i = 0; i<ntot; i++) {
				if (namePtrs[i]) {
					ism_common_setProperty(cprops, namePtrs[i], NULL);
					TRACE(9, "remove property %s.\n", namePtrs[i]);
					ism_common_free(ism_memory_admin_misc,namePtrs[i]);
				}
			}
		}
	}

UPDATE_END:
    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

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
int ism_config_HASyncUp(char *item, int action, int composite, int admMode, int isHAConfig, char *inpbuf, int inpbuflen, concat_alloc_t epbuf) {
    int rc = ISMRC_OK;

    TRACE(9, "Entry %s: item: %s, action: %d, composite: %d, adminMode: %d, isHAConfig: %d, inpbuf: %s, inpbuflen: %d\n",
        		__FUNCTION__, item?item:"null", action, composite, admMode, isHAConfig, inpbuf?inpbuf:"null", inpbuflen );

    /* If HA is enabled and node is Primary update Standby */
    if ( admMode == 0 && isHAConfig == 0 && ism_ha_admin_isUpdateRequired() == 1 ) {
        int flag = ism_config_getUpdateCertsFlag(item);
		int outbuflen = 0;
		char *outbuf = NULL;

		if ( item && !strcmpi(item, "Endpoint") ) {
			outbuflen = epbuf.used;
			outbuf = epbuf.buf;
		} else {
			outbuflen = inpbuflen;
			outbuf = inpbuf;
		}

		if ( outbuf && *outbuf != '\0' && outbuflen > 2 ) {
			/* add primary version info*/
			const char *versionStr = ism_common_getVersion();
			int vlen = strlen(versionStr);
			char *ntmp = alloca(vlen + 16);

			sprintf(ntmp, ",\"Version\":\"%s\" }", versionStr);
			char *nbuf;
			int nlen = 0;

			nlen = outbuflen + vlen + 16 -1;
			nbuf = alloca(nlen);

			memcpy(nbuf, outbuf, outbuflen-2);  //remove the last " }"
			memcpy((nbuf + outbuflen -2), ntmp, vlen + 15);
			outbuf = nbuf;
			outbuflen = nlen;
		}

		int rc_store = ism_ha_admin_update_standby(outbuf, outbuflen, flag);
		if (rc_store != ISMRC_OK)
			rc = rc_store;
    } else {
    	TRACE(5, "no HA sycn-up is needed.\n");
    }

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

int copyFile(const char *source, const char *destination) {
    int rc = ISMRC_OK;
    FILE *src = NULL;
    FILE *dest = NULL;
    int ch;

    if ( !source || !destination ) {
        TRACE(2, "Can not copy file. NULL is passed for source or destination.\n");
        rc = ISMRC_NullPointer;
    } else {
       src = fopen(source, "r");
       if( src == NULL ) {
          TRACE(2, "Can not copy file. Could not open source file '%s'. rc=%d\n", source, errno);
          rc = ISMRC_Error;
       } else {
          dest = fopen(destination, "w");
          if( dest == NULL ) {
              TRACE(2, "Can not copy file. Could not open destination file '%s'. rc=%d\n", destination, errno);
              rc = ISMRC_Error;
          } else {
              while(( ch = fgetc(src) ) != EOF ) {
                  fputc(ch, dest);
              }
              TRACE(5, "File %s is copied to %s\n", source, destination);
              fclose(dest);
          }
          fclose(src);
       }
    }
    if (rc)
        ism_common_setError(rc);
    return rc;
}

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
int ism_config_rollbackCertificate(const char *profile, const char *keyfile, int rollbackFlag, int authType) {
	int rc = ISMRC_OK;
	struct dirent *dent;
	int len = 0;

	if (!profile) {
		TRACE(3, "%s: profile specified is null.\n", __FUNCTION__);
		return ISMRC_NullArgument;
	}

	//get LTPAKeyStore path
	char *certificateDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
			,0 == authType ? "LTPAKeyStore" : "OAuthCertificateDir");
	if (!certificateDir) {
		TRACE(3, "%s: Failed to get %s keystore path.\n", __FUNCTION__, 0 == authType ? "LTPA" : "OAuth");
		rc = ISMRC_Error;
		ism_common_setError(rc);
		goto ROLLBACK_LTPA_END;
	}

	if ( rollbackFlag && keyfile) {
		//remove keyfile from LTPA keystore
		if (keyfile) {
		    len = strlen(certificateDir) + strlen(keyfile) + 2;
		    char *opath = alloca(len);
		    snprintf(opath, len, "%s/%s", certificateDir, keyfile);
		    unlink(opath);
		}
	} else {
		//remove keyfile from tmp/userfiles
		if (keyfile && *keyfile) {
			len = 0;
		    len = strlen(USERFILES_DIR) + strlen(keyfile) + 1;
		    char *opath = alloca(len);
		    snprintf(opath, len, "%s%s", USERFILES_DIR, keyfile);
		    unlink(opath);
		}
	}

	len = 0;
	len = strlen(USERFILES_DIR) + strlen(profile) + 1;
	char *tmpdir = alloca(len);
	snprintf(tmpdir, len, "%s%s", USERFILES_DIR, profile);
	DIR* certDir = opendir(tmpdir);

	if (certDir == NULL) {
		goto ROLLBACK_LTPA_END;
    }

    while((dent = readdir(certDir)) != NULL) {
	    struct stat st;

	    if(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..") )
	        continue;

	    //fprintf(stderr, "entryname: %s\n", dent->d_name);
	    stat(dent->d_name, &st);
		if (S_ISREG(st.st_mode) == 0) {
			len = 0;
		    len = strlen(tmpdir) + strlen(dent->d_name) + 2;
		    char *tpath = alloca(len);
		    snprintf(tpath, len, "%s/%s", tmpdir, dent->d_name);

			if ( rollbackFlag ) {
				len = 0;
			    len = strlen(certificateDir) + strlen(dent->d_name) + 2;
			    char *opath = alloca(len);
			    snprintf(opath, len, "%s/%s", certificateDir, dent->d_name);
			    copyFile(tpath, opath);
			}
		    unlink(tpath);
		}
    }
    closedir(certDir);
    //remove backupdir created by ism_config_restapi_applyLTPACert
    rmdir(tmpdir);

ROLLBACK_LTPA_END:
    TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
    return rc;
}

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
int ism_config_rollbackCertificateConfig(ism_prop_t *props, int rollbackFlag, int authType) {
	int rc = ISMRC_OK;

	//get LTPAKeyStore path
	char *certificateDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties()
			,0 == authType ? "LTPAKeyStore" : "OAuthCertificateDir");
	if (!certificateDir) {
		TRACE(3, "%s: Failed to get %s keystore path.\n", __FUNCTION__, 0 == authType ? "LTPA" : "OAuth");
		rc = ISMRC_Error;
		ism_common_setError(rc);
		return rc;
	}
	const char *profile = ism_common_getStringProperty(props, "Name");
	const char *keyfile = ism_common_getStringProperty(props, "KeyFileName");

	return ism_config_rollbackCertificate(profile, keyfile, rollbackFlag, authType);
}


/**
 *  Returns component type (ism_ConfigComponentType_t) from component name
 */
XAPI ism_ConfigComponentType_t ism_config_getComponentType(const char *compName) {
    if ( !compName )
        return ISM_CONFIG_COMP_LAST;

    if ( !strcasecmp(compName, "Server"))
        return ISM_CONFIG_COMP_SERVER;
    else if ( !strcasecmp(compName, "Transport"))
        return ISM_CONFIG_COMP_TRANSPORT;
    else if ( !strcasecmp(compName, "Protocol"))
        return ISM_CONFIG_COMP_PROTOCOL;
    else if ( !strcasecmp(compName, "Engine"))
        return ISM_CONFIG_COMP_ENGINE;
    else if ( !strcasecmp(compName, "Store"))
        return ISM_CONFIG_COMP_STORE;
    else if ( !strcasecmp(compName, "Security"))
        return ISM_CONFIG_COMP_SECURITY;
    else if ( !strcasecmp(compName, "Admin"))
        return ISM_CONFIG_COMP_ADMIN;
    else if ( !strcasecmp(compName, "Monitoring"))
        return ISM_CONFIG_COMP_MONITORING;
    else if ( !strcasecmp(compName, "MQConnectivity"))
        return ISM_CONFIG_COMP_MQCONNECTIVITY;
    else if ( !strcasecmp(compName, "HA"))
        return ISM_CONFIG_COMP_HA;
    else if ( !strcasecmp(compName, "cluster"))
        return ISM_CONFIG_COMP_CLUSTER;

    return ISM_CONFIG_COMP_LAST;
}

/* Returns configuration property type - ism_ConfigPropType_t */
XAPI ism_ConfigPropType_t ism_config_getConfigPropertyType(const char *propName) {
    if ( !propName )
        return ISM_CONFIG_PROP_INVALID;

    if ( !strcasecmp(propName, "Number"))
        return ISM_CONFIG_PROP_NUMBER;
    else if ( !strcasecmp(propName, "Enum"))
        return ISM_CONFIG_PROP_ENUM;
    else if ( !strcasecmp(propName, "List"))
        return ISM_CONFIG_PROP_LIST;
    else if ( !strcasecmp(propName, "String"))
        return ISM_CONFIG_PROP_STRING;
    else if ( !strcasecmp(propName, "StringBig"))
        return ISM_CONFIG_PROP_STRING;
    else if ( !strcasecmp(propName, "Name"))
        return ISM_CONFIG_PROP_NAME;
    else if ( !strcasecmp(propName, "Boolean"))
        return ISM_CONFIG_PROP_BOOLEAN;
    else if ( !strcasecmp(propName, "IPAddress"))
        return ISM_CONFIG_PROP_IPADDRESS;
    else if ( !strcasecmp(propName, "URL"))
        return ISM_CONFIG_PROP_URL;
    else if ( !strcasecmp(propName, "REGEX"))
        return ISM_CONFIG_PROP_REGEX;
    else if ( !strcasecmp(propName, "REGEXSUB"))
        return ISM_CONFIG_PROP_REGEX_SUBEXP;
    else if (!strcasecmp(propName, "BufferSize"))
    	return ISM_CONFIG_PROP_BUFFERSIZE;
    else if ( !strcasecmp(propName, "Selector"))
        return ISM_CONFIG_PROP_SELECTOR;

    return ISM_CONFIG_PROP_INVALID;
}


/* return 0 if not exist*/
XAPI int ism_config_isFileExist( char * filepath)
{
    if (filepath == NULL)
        return 0;

    if( access( filepath, R_OK ) != -1 ) {
        return 1;
    }
    return 0;
}


/*
 * Delete TrustedCertificate file from /tmp/userfile dir
 */
XAPI void ism_config_deleteTmpCertFile(char **filelist, int filecount) {
	int i = 0;

	for (i = 0; i < filecount; i++ ) {
        int clen = strlen(USERFILES_DIR) + strlen(filelist[i]) + 1;
        char *cpath = alloca(clen);

        snprintf(cpath, clen, "%s%s", USERFILES_DIR, filelist[i]?filelist[i]:"null");
        if ( ism_config_isFileExist(cpath) ) {
        	if (unlink(cpath) < 0)
			   TRACE(3, "Could not delete TrustedCertificate %s. errno: %d\n", cpath, errno);
        } else {
        	TRACE(9, "The TrustedCertificate %s does not exist in the tmp dir.\n", filelist[i]?filelist[i]:"null");
        }
	}

	return;
}


/*
 * Check for Trusted or Client certificates
 * type = 0 - TrustedCertificate
 * type = 1 - ClientCertificate
 */
XAPI int ism_config_checkTrustedCertExist(int type, char *secProfile, char *trustCert, int *isNewFile, int *noCurrCerts) {
	int rc = ISMRC_OK;
	*noCurrCerts = 0;

	if (!trustCert) {
		rc = ISMRC_PropertyRequired;
		if ( type == 0 ) {
		    ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "TrustedCertificate", "null");
		} else {
			ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "ClientCertificate", "null");
		}
		goto CHECK_CERTEXIST_END;
	}

	if (!secProfile) {
		rc = ISMRC_PropertyRequired;
		ism_common_setErrorData(ISMRC_PropertyRequired, "%s%s", "SecurityProfile", "null");
		goto CHECK_CERTEXIST_END;
    }


    int clen = strlen(USERFILES_DIR) + strlen(trustCert) + 1;
    char *cpath = alloca(clen);

	char *trustedCertDir = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(),"TrustedCertificateDir");
    int tlen = 0;
    char *tpath = NULL;

    snprintf(cpath, clen, "%s%s", USERFILES_DIR, trustCert);
    if ( !ism_config_isFileExist(cpath) ) {
    	if ( type == 0 ) {
        	tlen = strlen(trustedCertDir) + strlen(secProfile) + strlen(trustCert) + 3;
        	tpath = alloca(tlen);
    	    snprintf(tpath, tlen, "%s/%s/%s", trustedCertDir, secProfile, trustCert);
    	} else {
        	tlen = strlen(trustedCertDir) + strlen(secProfile) + strlen(trustCert) + 23;
        	tpath = alloca(tlen);
    	    snprintf(tpath, tlen, "%s/%s_allowedClientCerts/%s", trustedCertDir, secProfile, trustCert);
    	}

    	if ( !ism_config_isFileExist(tpath) ) {
    	    rc = ISMRC_ObjectNotFound;
    	    if ( type == 0 ) {
    	        ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "TrustedCertificate", trustCert);
    	    } else {
    	    	ism_common_setErrorData(ISMRC_ObjectNotFound, "%s%s", "ClientCertificate", trustCert);
    	    }
    	}
    } else {
    	*isNewFile = 1;
    }

    /* Get no of client certs installed for this profile */
    if ( type == 1 ) {
    	tlen = strlen(trustedCertDir) + strlen(secProfile) + 23;
    	tpath = alloca(tlen);
	    snprintf(tpath, tlen, "%s/%s_allowedClientCerts", trustedCertDir, secProfile);
	    struct dirent *dent;
	    DIR * bDir = opendir(tpath);
        if (bDir == NULL) {
        	if ( errno == 2 ) {
        		*noCurrCerts = 0;
        		goto CHECK_CERTEXIST_END;
        	}
    	    TRACE(3, "Could not open %s directory. errno:%d\n", tpath, errno);
    	    rc = ISMRC_NotFound;
    	    ism_common_setError(rc);
    	    return rc;
        }
        int ncerts = 0;
        while ((dent = readdir(bDir)) != NULL) {
        	struct stat st;
        	if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;
        	stat(dent->d_name, &st);
        	if ( S_ISDIR(st.st_mode) != 0 )
        		continue;
        	ncerts += 1;
        }
        closedir(bDir);
        *noCurrCerts = ncerts;
    }


CHECK_CERTEXIST_END:
    return rc;
}
