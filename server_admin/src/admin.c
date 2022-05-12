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

#define TRACE_COMP Admin

#include "configInternal.h"
#include "cluster.h"
#include "ha.h"
#include "store.h"

typedef int32_t  (*getStatistics_f)(const ismCluster_Statistics_t *pStatistics);

typedef void (*ism_snmpStop_f) (void);
static ism_snmpStop_f snmpStop_f= NULL;

typedef int (*ism_snmpStart_f) (void);
static ism_snmpStart_f snmpStart_f = NULL;

typedef int32_t (*ism_snmpRunning_f) (void);
static ism_snmpRunning_f snmpRunning_f = NULL;

extern size_t ism_common_formatMessage(char * msgBuff, size_t msgBuffSize, const char * msgFormat, const char * * replData, int replDataSize);
extern int32_t ism_config_set_dynamic_extended(int actionType, ism_json_parse_t *json, char *inpbuf, int inpbuflen, char **retbuf);
extern int32_t ism_config_get_dynamic(ism_json_parse_t *json, concat_alloc_t *retStr);
extern char * ism_admin_get_harole_string(ismHA_Role_t role);

extern int32_t ism_adminHA_term(void);

extern int ism_admin_getProtocolsInfo(ism_json_parse_t *json, concat_alloc_t *output_buffer);
extern int ism_admin_getPluginsInfo(ism_json_parse_t *json, concat_alloc_t *output_buffer);
extern int ism_admin_processPSKNotification(ism_json_parse_t *json, concat_alloc_t *output_buffer);
extern void ism_adminHA_init_locks(void);
extern int ism_adminHA_term_thread(void);
extern void ism_config_init_locks(void);
extern void ism_security_init_locks(void);
extern char * ism_config_getServerName(void);
extern char * ism_config_getStringObjectValue(char *object, char *instance, char *item, int getLock);
extern int ism_config_json_processMQCRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int rc1);
extern void ism_config_initMQCConfigLock(void);
extern int ism_admin_getPluginStatus(void);
extern int ism_admin_getMQCStatus(void);
extern int ism_admin_startPlugin(void);
extern int ism_admin_stop_mqc_channel(void);
extern int ism_admin_start_mqc_channel(void);
extern int ism_server_config(char * object, char * namex, ism_prop_t * props, ism_ConfigChangeType_t flag);
extern const char * ism_config_json_haRCString(int type);
extern char * ism_admin_getStatusStr(int type);
extern void ism_config_setServerName(int getLock, int setDefault );
extern int ism_config_json_getAdminModeRC(void);
extern int ism_config_json_getAdminPort(int getLock);
extern int ism_config_isFileExist( char * filepath);
extern int ism_config_json_setAdminMode(int flag);
extern int ism_config_cleanupImportExportClientSet(void);
extern int ism_admin_dumpStatus(void);
extern void ism_admin_startDumpStatusTimerTask(void);

static void getDockerContainerUUID(char **buf, size_t *len);


extern char *remoteServerName;
extern char *serverVersion;
extern int ldapstatus;
extern json_t *srvConfigRoot;
extern int backupApplied;
extern pthread_rwlock_t    srvConfiglock;

int serverProcType = ISM_PROTYPE_SERVER;
int serverState = ISM_SERVER_INITIALIZING;
int adminMode = 0;
int adminModeRC;
int 			 adminModeRCReplDataCount = 0;
concat_alloc_t   * adminModeRCReplDataBuf=NULL;
int cleanStore=0;
int modeChanged=0;
int modeChangedValue=0;
int modeChangedPrev=-1;
int modeFirst=-1;
int adminPauseState = ISM_ADMIN_STATE_PAUSE;
int ismAdminPauseInited = 0;
int adminInitError = ISMRC_OK;
int adminInitWarn = ISMRC_OK;
uint64_t serverStartTime = 0;
int haSyncInProgress = 0;
int storePurgeState = 0;
int restartServer = 0;
int restartNeeded = 0;
ism_json_parse_t * ConfigSchema;
ism_json_parse_t * MonitorSchema;
int isConfigSchemaLoad  = 0;
int isMonitorSchemaLoad = 0;
char *container_uuid = NULL;
int ismCUNITEnv = 0;
int haRestartNeeded = 0;
int lastAminPort = 9089;
int licenseIsAccepted = 0;

/* This lock and condition variable is to control execution of ISM server
 * process running in daemon mode.
 */
static pthread_mutex_t adminLock;
static pthread_cond_t  adminCond;
static pthread_mutex_t adminFileLock;
static ismSecurity_t *adminSContext;
static int fSecurityInit = 0;
static int fAdminSecurityContext = 0;
static int fConfigInit = 0;
static int fAdminHAInit = 0;
static int fMQCInit = 0;
static int secProtocolNextId=0;
static char *licensedUsage = NULL;
static ism_timer_t enable_status_timer = 0;

ismHashMap * ismSecProtocolMap;

/*Request Locale Specific*/
pthread_key_t adminlocalekey;
pthread_once_t adminkey_once = PTHREAD_ONCE_INIT;

static void make_key(void)
{
	(void) pthread_key_create(&adminlocalekey, NULL);
}

XAPI int ism_admin_getStorePurgeState(void) {
	return storePurgeState;
}

int ism_admin_getAdminModeRCStr(char * outbuf, int outbuf_len, int errorCode)
{
	const char * replData[64];
	int i = 0;
	int rc = adminModeRC;

	if ( errorCode == 0 ) {
		concat_alloc_t conbuf = {0};
		conbuf.buf = adminModeRCReplDataBuf->buf;
		conbuf.used = adminModeRCReplDataBuf->used;
		for (i=0; i<adminModeRCReplDataCount; i++) {
			replData[i] = ism_common_getReplacementDataString(&conbuf);
		}
	} else {
		rc = errorCode;
	}

	char errstr [1024];

	ism_common_getErrorStringByLocale(rc,  ism_common_getRequestLocale(adminlocalekey), errstr, sizeof(errstr));

	return ism_common_formatMessage(outbuf, outbuf_len, errstr, replData, i);
}

/**
 * Retruns License type and tag from accepted.json file
 * Caller should check and free returned License type string
 */
#ifdef ACCEPT_LICENSE
//Really check/set license status
XAPI char * ism_admin_getLicenseAccptanceTags(int *licenseStatus) {
	int newFile = 0;

	/* accepted.json file in configuration directory */
	char cfilepath[1024];
	sprintf(cfilepath, IMA_SVR_INSTALL_PATH "/config/accepted.json");

	/* get current status and LicensedUsage from json file */
	char *licenseType = NULL;
	*licenseStatus = 0;

	if ( ism_config_isFileExist( cfilepath )) {
		/* File exists - check contents */
		json_t *root = NULL;
	    json_error_t error;
	    root = json_load_file(cfilepath, 0, &error);
	    if (root) {
	        json_t *objval = NULL;
	        const char *objkey = NULL;
	        json_object_foreach(root, objkey, objval) {
	        	if ( !strcmp(objkey, "Status")) {
	        		*licenseStatus = json_integer_value(objval);
	        		TRACE(5, "License acceptance mode is %d\n", *licenseStatus);
		    		if (*licenseStatus != 4 && *licenseStatus != 5 ) {
		    			newFile = 1;
		    		}
	        	} else if ( !strcmp(objkey, "LicensedUsage")) {
	        		char *tmpstr = (char *)json_string_value(objval);
		    		if ( !strcmp(tmpstr, "Developers") || !strcmp(tmpstr, "Production") || !strcmp(tmpstr, "Non-Production") || !strcmp(tmpstr, "IdleStandby") ) {
		    		    licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),tmpstr);
		    		    TRACE(5, "License type is %s\n", licenseType);
		    		} else {
		    			newFile = 1;
		    		}
	        	}
	        }
	    	json_decref(root);
	    } else {
		    if ( error.line == -1 ) {
		        TRACE(2, "Error in accepted license file: %s\n", error.text);
		    } else {
		    	TRACE(2, "Error in accepted license file: JSON error on line %d: %s\n", error.line, error.text);
		    }
	    	newFile = 1;
	    }
	} else {
		newFile = 1;
	}

	if ( newFile == 1 ) {
		/* File doesn't exist - create a default file */
	    /* Create license string */
		char buffer[512];
		FILE *dest = NULL;

		sprintf(buffer, "{ \"Status\": 4, \"Language\":\"en\", \"LicensedUsage\":\"Developers\" }");

		dest = fopen(cfilepath, "w");
		if ( dest ) {
			fprintf(dest, "%s", buffer);
			fclose(dest);

			*licenseStatus = 4;
			licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Developers");
		} else {
			ism_common_setErrorData(ISMRC_FileUpdateError, "%s%d", cfilepath, errno);
		}
	}

	return licenseType;
}
#else //ACCEPT_LICENSE not set
//There is no license the user has to accept, just return that we are good
XAPI char * ism_admin_getLicenseAccptanceTags(int *licenseStatus) {
	*licenseStatus = 5;
	char *licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Production");

	return licenseType;
}
#endif

/**
 * Initialize admin and security service
 *
 * For imaserver process:
 * - init locks and condition variables
 * - init config
 * - init security
 * - init admin HA
 * - check results of uEFI, Raid and nvDIMM tests
 *
 * For mqcbridge process
 * - init locks and condition variables
 * - init config
 * - init security
 * - init admin HA
 *
 */
XAPI int32_t ism_admin_init(int proctype) {
	int rc = ISMRC_OK;
	char * compError = NULL;
	size_t len;
	ism_prop_t *props = NULL;

	adminModeRC = ISMRC_OK;
	adminInitError = ISMRC_OK;
	adminInitWarn = ISMRC_OK;

	TRACE(5, "Initializing administration services.\n");

	adminMode = 0;

	/* check for CUNIT tests */
	if ( getenv("CUNIT") != NULL ) {
		ismCUNITEnv = 1;
	}

	serverStartTime = (uint64_t)ism_common_readTSC();
	serverProcType = proctype;

	/* Initialize admin lock and condvar */
	pthread_mutex_init(&adminLock, 0);
	pthread_cond_init(&adminCond, 0);
	pthread_mutex_init(&adminFileLock, 0);

	/* Set container_uuid, if running in docker container */
	len = 0;

	if ( ism_common_getPlatformType() == PLATFORM_TYPE_DOCKER ) {
	    getDockerContainerUUID(&container_uuid, &len);
	}

	/* Initialize configuation locks */
	ism_config_init_locks();

	/* Initialize platform data file - needed for non-docker environment */
	rc = ism_common_initPlatformDataFile();
	if (  rc != ISMRC_OK ) {
		TRACE(2, "Failed to create platform data file: rc=%d\n", rc);
		compError="Configuration";
		adminInitError = rc;
		goto ADMIN_INIT_END;
	}

	ismSecProtocolMap = ism_common_createHashMap(256, HASH_STRING);

	/* Initialize configuration service */
	rc = ism_config_init();
	if (  rc != ISMRC_OK ) {
		TRACE(2, "Could not initialize configuration service: rc=%d\n", rc);
		if ( compError == NULL ) compError="Configuration";
		if ( adminInitError == ISMRC_OK) adminInitError = rc;
		if ( rc == ISMRC_UUIDConfigError ) goto ADMIN_INIT_END;
	}

	fConfigInit = 1;

	/* Register for the server callbacks.  We do this here to break a utils/admin order issue. */
	ism_config_t * serverhandle = NULL;
	ism_config_register(ISM_CONFIG_COMP_SERVER, NULL, ism_server_config, &serverhandle);
	if ( serverhandle == NULL ) {
		TRACE(2, "Failed to get the server config handle\n");
		compError="Configuration";
		adminInitError = ISMRC_Error;
		goto ADMIN_INIT_END;
	}

	props = ism_config_getProperties(serverhandle, NULL, NULL);

	if (proctype == ISM_PROTYPE_SERVER) {
		/* Initialize loggers */
		ism_common_initLoggers(props, 0);

		/* check if License is accepted */
		int licenseStatus = 0;
		char *licenseType = NULL;

		if ( ismCUNITEnv == 1 ) {
			licenseType = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Developers");
			licenseStatus = 5;
		} else {
		    licenseType = ism_admin_getLicenseAccptanceTags(&licenseStatus);
		}

		/* Initialize license type */
		if ( licensedUsage ) ism_common_free(ism_memory_admin_misc,licensedUsage);

		if ( licenseType )
		    licensedUsage = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),licenseType);
		else
			licensedUsage = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"Developers");

		ism_common_setPlatformLicenseType(licensedUsage);

		LOG(INFO, Server, 900, "%s %s", "Start the " IMA_SVR_COMPONENT_NAME_FULL " ({0} {1}).", ism_common_getVersion(), ism_common_getBuildLabel());
		LOG(NOTICE, Admin, 900, "%s %s", "Start the " IMA_SVR_COMPONENT_NAME_FULL " ({0} {1}).", ism_common_getVersion(), ism_common_getBuildLabel());

		/* set ServerName if not set */
		ism_config_setServerName(1, 0);

		/* get current Admin port */
		lastAminPort = ism_config_json_getAdminPort(1);

		/* initialize adminHA locks */
		ism_adminHA_init_locks();

		/* Initlizaize security locks */
		ism_security_init_locks();

		/* Print licensed usage */
		LOG(NOTICE, Admin,6163, "%s", "The " IMA_SVR_COMPONENT_NAME_FULL " license usage is {0}.", licensedUsage);

		rc = ism_security_init();
		if ( rc != ISMRC_OK ) {
			if ( compError == NULL ) compError="Security";
			if ( adminInitError == ISMRC_OK) adminInitError = rc;
			TRACE(2, "Could not initialize security: rc=%d\n", rc);
		}

		fSecurityInit = 1;

		/* init security context for admin component */
		if ( rc == ISMRC_OK ) {
			rc = ism_security_create_context(ismSEC_POLICY_CONFIG, NULL, &adminSContext);
			if ( rc != ISMRC_OK ) {
				if ( compError == NULL ) compError="SecurityContext";
				if ( adminInitError == ISMRC_OK) adminInitError = rc;
				TRACE(2, "Could not set security context for admin account: RC=%d\n", rc);
			}
		}

		fAdminSecurityContext = 1;

		/* Initialize HA admin */
		if ( rc == ISMRC_OK ) {
			/* Initialize HA admin */
			rc = ism_adminHA_init();

			if ( rc != ISMRC_OK ) {
				if ( compError == NULL ) compError="HighAvailability";
				if ( adminInitError == ISMRC_OK) adminInitError = rc;
			}

			fAdminHAInit = 1;
		}

		rc = ism_admin_init_mqc_channel();
		if ( rc != ISMRC_OK ) {
			if ( adminInitError == ISMRC_OK) adminInitError = rc;
			if ( compError == NULL ) compError="InitMQC";
			goto ADMIN_INIT_END;
		}
		fMQCInit = 1;

        if ( licenseStatus != 5 ) {
        	rc = ISMRC_LicenseError;
    		TRACE(2, "Server license is not yet accpted. License type is %s. Setting server is Maintenance mode: rc=%d\n", licenseType?licenseType:"", rc);
    		if ( compError == NULL ) compError="Configuration";
    		if ( adminInitError == ISMRC_OK) adminInitError = rc;
    		if ( licenseType ) ism_common_free(ism_memory_admin_misc,licenseType);
    		goto ADMIN_INIT_END;
        } else {
        	licenseIsAccepted = 1;
        }
        if ( licenseType ) ism_common_free(ism_memory_admin_misc,licenseType);

	} else if (proctype == ISM_PROTYPE_MQC) {

		ism_config_initMQCConfigLock();
		ism_admin_mqc_init();

	}


ADMIN_INIT_END:

    /* Despite any other admin errors as long as config is set do any special actions related to config:
     * Current items:
     * 1. ResourceSetDesriptor,
     * 2. Cleanup import/export files with respect to clientset
     */
    if (fConfigInit == 1) {
        (void) ism_admin_isResourceSetDescriptorDefined(0);
        rc = ism_config_cleanupImportExportClientSet();
        // Don't fail admin start because of this, just log error and continue
        if (rc != ISMRC_OK) {
            LOG(WARN, Admin, 6175, NULL, "Could not cleanup import/export files marked in progress for client set move requests. Please check trace.");
            rc = ISMRC_OK;
        }
    }
    if (props)
	    ism_common_freeProperties(props);

	/*Init Locale Thread Specific Key*/
	(void) pthread_once(&adminkey_once, make_key);

	/* If admin error is set, LOG error - server will start in maintenance mode - check is done in main.c */
	if (adminInitError != ISMRC_OK) {
		if ( compError && !strcmp(compError, "VMConfigCheck") && adminInitError != ISMRC_VMCheckAdminLog )
			return ISMRC_OK;

		ism_admin_setLastRCAndError();
		char buf[1024];
		char * errstr = (char *)ism_common_getErrorString(adminInitError, buf, 1024);
		LOG(ERROR, Admin, 6119, "%s%-s%d", "Configuration Error was detected in \"{0}\". Error String: {1}. RC: {2}.", compError?compError:"NULL",errstr, adminInitError);
	} else {
		LOG(NOTICE, Admin, 6013, "%d", "Administration service is initialized: RunMode={0}.", adminMode);
	}


    /*
     * Start timer thread to dump status - for kubernetes probes - run the timer thread only in imaserver process
     */
    if (proctype == ISM_PROTYPE_SERVER) {
        ism_admin_startDumpStatusTimerTask();
    }

	return ISMRC_OK;
}

/**
 * Set adminInitError if failed to configure external ldap
 */
XAPI void ism_admin_setAdminInitErrorExternalLDAP(int rc) {
    /* check for LDAP errors - ldap init is invoked by ism_security_init() */
    if ( rc != ISMRC_OK  && adminInitError == ISMRC_OK ) {
        adminInitError=ldapstatus;
        adminMode = 1;
        char buf[1024];
        char * errstr = (char *)ism_common_getErrorString(adminInitError, buf, 1024);
        LOG(ERROR, Admin, 6119, "%s%-s%d", "Configuration Error was detected in \"{0}\". Error String: {1}. RC: {2}.", "LDAP",errstr, adminInitError);
    }
}

/**
 * Returns server runmode
 * - Returns Production, Maintenance or CleanStore mode based on following data
 *   - Check value of AdminMode config item in server dynamic config
 *   - Check adminInitError variable (gets set during server start)
 */
XAPI int32_t ism_admin_getmode(void) {
	json_t *adminModeJSON =  ism_config_json_getSingleton("AdminMode");
	if ( adminModeJSON && json_typeof(adminModeJSON) == JSON_INTEGER ) {
		adminMode = json_integer_value(adminModeJSON);
		TRACE(8, "Server mode: %d\n", adminMode);
		if ( adminMode ==  ISM_MODE_MAINTENANCE ) {
			if ( cleanStore == 1 ) {
				adminMode = ISM_MODE_CLEANSTORE;
			}
		}
		if ( adminMode == ISM_MODE_PRODUCTION) {
			if ( adminInitError != ISMRC_OK ) {
				adminMode = ISM_MODE_MAINTENANCE;
			}
		}
	}
	return adminMode;
}

/**
 * Return current value of EnableDiskPersistence configuration item
 */
XAPI int32_t ism_admin_getDiskMode(void) {
	ism_prop_t * storeProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
	char * curval = (char *) ism_common_getStringProperty(storeProps, "EnableDiskPersistence");
	if ( !curval || (curval && *curval == '\0')) {
		return 0;
	}
	if ( *curval == 'T' || *curval == 't' || *curval == '1' ) {
		return 1;
	}
	return 0;
}

/**
 * Returns server AdminErrorCode
 */
XAPI int32_t ism_admin_getInternalErrorCode(void) {
	int rc = ISMRC_OK;
	ism_config_t * serverhandle = ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);
	if ( serverhandle == NULL ) {
		TRACE(3, "Could not get server config handle\n");
		rc = ISMRC_Error;
	} else {
		ism_prop_t *props = ism_config_getProperties(serverhandle, NULL, NULL);
		const char *adminErrorStr = ism_common_getStringProperty(props, "InternalErrorCode");
		// TRACE(8, "Server InternalErrorCode: %s\n", adminErrorStr);
		if ( adminErrorStr && *adminErrorStr != '\0') {
			rc = atoi(adminErrorStr);
		}
		if ( props ) {
			ism_common_freeProperties(props);
		}
	}
	return rc;
}

/**
 * Terminate admin and security service
 */
XAPI int32_t ism_admin_term(int proctype) {
	if (fConfigInit) {
		ism_config_term();
		fConfigInit=0;
	}
	if (fAdminSecurityContext) {
		ism_security_destroy_context(adminSContext);
		fAdminSecurityContext=0;
	}
	if (fSecurityInit) {
		ism_security_term();
		fSecurityInit=0;
	}
	if (fAdminHAInit) {
		ism_adminHA_term_thread();
		ism_adminHA_term();
		fAdminHAInit = 0;
	}

	if(adminModeRCReplDataBuf!=NULL){
		ism_common_freeAllocBuffer(adminModeRCReplDataBuf);
		ism_common_free(ism_memory_admin_misc,adminModeRCReplDataBuf);
		adminModeRCReplDataBuf = NULL;
	}
	if(fMQCInit) {
		ism_admin_stop_mqc_channel();
		fMQCInit = 0;
	}
	adminModeRCReplDataCount=0;

	ism_common_destroyHashMap(ismSecProtocolMap);

	if (container_uuid) {
		ism_common_free_raw(ism_memory_admin_misc,container_uuid);
	}

	return ISMRC_OK;
}

/**
 * To make admin and security service wait to receive a signal to
 * stop imaserver process - invoked by main
 */
XAPI int32_t ism_admin_pause(void) {
	int32_t rc = ISMRC_OK;
	ismAdminPauseInited = 1;
	TRACE(3, "In ism_admin_pause(): admnPauseState=%d adminMode=%d\n", adminPauseState, adminMode);
	/*
	 * read QueuedTask from configDir if exist
	 * and start delete ClientSet thread.
	 */
	if (adminMode == 0 ) {
		ism_config_startClientSetTask();
	}

	for (;;) {
		pthread_mutex_lock(&adminLock);

		if ( adminPauseState == ISM_ADMIN_STATE_TERMSTORE ) {
			rc = ISMRC_StoreNotAvailable;
			adminPauseState = ISM_ADMIN_STATE_PAUSE;
			pthread_mutex_unlock(&adminLock);
			break;
		} else if ( adminPauseState == ISM_ADMIN_STATE_STOP ) {
			rc = ISMRC_Closed;
			pthread_mutex_unlock(&adminLock);
			break;
		}

		pthread_cond_wait(&adminCond, &adminLock);

		if ( adminPauseState == ISM_ADMIN_STATE_CONTINUE ) {
			TRACE(5, "ism_admin_pause: state is ADMIN_STATE_CONTINUE.\n");
			/* This state is set on standby node - to move from standby to primary.
			 * Set adminPauseState to ADMIN_STATE_PAUSE and break from look, so that engine
			 * and messaging can start. ADMIN_STATE_PAUSE mode is required so that we can
			 * handle imaserver stop.
			 */
			adminPauseState = ISM_ADMIN_STATE_PAUSE;
			pthread_mutex_unlock(&adminLock);
			break;
		}

		if ( adminPauseState == ISM_ADMIN_STATE_TERMSTORE ) {
			TRACE(2, "ism_admin_pause: state is ADMIN_STATE_TERMSTORE. adminMode=%d\n", adminMode);
			/* check adminMode: if already in maintenance - ignore the signal */
			if ( adminMode != 0 ) {
				TRACE(2, "ism_admin_pause: adminMode=%d - Ignore ADMIN_STATE_TERMSTORE.\n", adminMode);
				pthread_mutex_unlock(&adminLock);
				continue;
			}
			rc = ISMRC_StoreNotAvailable;
			adminPauseState = ISM_ADMIN_STATE_PAUSE;
			pthread_mutex_unlock(&adminLock);
			break;
		}

		if ( adminPauseState == ISM_ADMIN_STATE_STOP ) {
			TRACE(2, "ism_admin_pause: imaserver stop is initiated.\n");
			rc = ISMRC_Closed;
			pthread_mutex_unlock(&adminLock);
			break;
		}

		TRACE(5, "ism_admin_pause is signaled. PauseState %d is not processed.\n", adminPauseState);
		pthread_mutex_unlock(&adminLock);
	}
	ismAdminPauseInited = 0;
	return rc;
}

/**
 * Returns server initialization state
 */
XAPI int32_t ism_admin_get_server_state(void) {
	return serverState;
}

/**
 * Send a signal to stop ISM server process
 */
XAPI int32_t ism_admin_send_stop(int mode) {
	pthread_mutex_lock(&adminLock);
	adminPauseState = mode;
	pthread_cond_signal(&adminCond);
	pthread_mutex_unlock(&adminLock);
	return ISMRC_OK;
}

/**
 * Send a signal to continue with startup process
 */
XAPI int32_t ism_admin_send_continue(void) {
	pthread_mutex_lock(&adminLock);
	adminPauseState = ISM_ADMIN_STATE_CONTINUE;
	pthread_cond_signal(&adminCond);
	pthread_mutex_unlock(&adminLock);
	return ISMRC_OK;
}

/**
 * Start stop process
 * mode = 0 - stop
 * mode = 1 - restart
 * mode = 2 - clean store and restart
 */
XAPI int ism_admin_init_stop(int mode, ism_http_t *http) {
	int rc = ISMRC_OK;

	LOG(NOTICE, Admin, 6005, NULL, "The " IMA_SVR_COMPONENT_NAME_FULL " is stopping.");

	if ( mode != ISM_ADMIN_MODE_STOP ) {
		if ( mode == ISM_ADMIN_MODE_CLEANSTORE ) {
			/* Clean store */
			/* set clean store and restart server */
			TRACE(1, "======== Server CleanStore is initiated ========\n");
			pthread_mutex_lock(&adminLock);
			cleanStore = 1;
			pthread_mutex_unlock(&adminLock);
		}

		TRACE(1, "======== Server Restart is initiated ========\n");

		/* Server restart */
		restartServer = 1;

		pthread_mutex_lock(&adminFileLock);
		FILE *dest = NULL;
		dest = fopen("/tmp/.restart_inited", "w");
		if( dest == NULL ) {
			TRACE(1,"Can not open /tmp/.restart_inited file: errno=%d\n", errno);
			rc = ISMRC_Error;
			pthread_mutex_unlock(&adminFileLock);
			goto INITSTOP_END;
		} else {
			ism_admin_stop_mqc_channel();
			fprintf(dest, "Restart has been initialed");
			fclose(dest);
		}
		pthread_mutex_unlock(&adminFileLock);
	}

	pthread_mutex_lock(&adminFileLock);
	FILE *dest = NULL;
	dest = fopen("/tmp/imaserver.stop", "w");
	if( dest == NULL ) {
		TRACE(1,"Can not open /tmp/imaserver.stop file: errno=%d\n", errno);
		rc = ISMRC_Error;
		pthread_mutex_unlock(&adminFileLock);
		goto INITSTOP_END;
	} else {
		fprintf(dest, "STOP");
		fclose(dest);
	}
	pthread_mutex_unlock(&adminFileLock);

	/* Send signal to stop, if pause is enabled.
	 * For all other cases exit out via a timer thread
	 */

	TRACE(1, "======== Server STOP is initiated ========\n");

	if ( ismAdminPauseInited == 1) {
		ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
	} else {
		TRACE(1, "Set Timer Task to STOP server\n");
		ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) ism_admin_stopTimerTask, NULL, 10, 30, TS_SECONDS);
	}

	INITSTOP_END:
	return rc;
}


/* Admin function to purge or clean store with auto restart of server */
static int32_t ism_admin_purgeStore(concat_alloc_t *output_buffer) {
	char  rbuf[1024];
	char  lbuf[1024];
	const char * repl[5];
	int rc = ISMRC_OK;

	int curmode = ism_admin_getmode();
	char *mstr = "production";
	if (curmode == 1) mstr = "maintenance";

	if ( curmode > 1 ) {
		char *mStr = "2";
		TRACE(5, "Changing server runmode from %d to %s.\n", serverState, mStr);
		ism_admin_change_mode(mStr, output_buffer);
		return rc;
	}

	TRACE(1, "Purge store is invoked. System will restart after cleaning the store.\n");

	/* set clean store and restart server */
	__sync_val_compare_and_swap( &cleanStore, cleanStore, 1);
	TRACE(2, "The cleanStore variable is set. The store will be cleaned on the next start.\n");
	pthread_mutex_lock(&adminFileLock);
	FILE *dest = NULL;
	dest = fopen("/tmp/imaserver.stop", "w");
	if( dest == NULL ) {
		TRACE(2,"Can not open /tmp/imaserver.stop file: errno=%d\n", errno);
	} else {
		fprintf(dest, "STOP");
		fclose(dest);
	}
	pthread_mutex_unlock(&adminFileLock);

	int xlen;
	char  msgID[12];
	ism_admin_getMsgId(6125, msgID);
	if (ism_common_getMessageByLocale(msgID,ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL) {
		repl[0] = mstr;
		repl[1] = "clean_store";
		repl[2] = mstr;
		ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 3);
	} else {
		sprintf(rbuf,
				"The " IMA_SVR_COMPONENT_NAME_FULL " is currently in \"%s\" mode with \"%s\" action pending. \nWhen it is restarted, it will be in \"%s\" mode.\n",
				mstr, "clean_store", mstr);
	}

	if ( output_buffer ) {
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
	}

	storePurgeState = 1;
	ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
	return rc;
}

/**
 * Admin API that exposes various actions to supported exposed to WebUI/CLI
 * NOTE: Though this function return error code, but the calling function
 *       from protocol ignores the return code. On error this function return 1
 */
XAPI int ism_process_admin_action(ism_transport_t *transport, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc) {
	char  rbuf[1024];
	char  lbuf[1024];
	const char * repl[5];
	*rc = ISMRC_OK;

	/* sanity check - input buffer */
	if ( inpbuf == NULL ) {
		return 1;
	}
	/* sanity check - if input buffer is a JSON string */
	if ( *inpbuf != '{' ) {
		return 1;
	}

	/* Clear any previous errors to ensure the last error is reported properly */
	ism_common_setError(0);

	/* make a copy of what is needed and null terminate input buffer */
	char *input_buffer = alloca(inpbuflen + 1);
	memcpy(input_buffer, inpbuf, inpbuflen);
	input_buffer[inpbuflen] = 0;

	/* hide any password from logging */
	char *input_logBuffer = input_buffer;
	char *maskBuffer = NULL;
	if (strstr(input_buffer, "BindPassword")!=NULL){
		maskBuffer = alloca(inpbuflen + 1);
		memset(maskBuffer, 0, inpbuflen + 1);
		input_logBuffer = maskBuffer;
		ism_admin_maskJSONField(input_buffer, "BindPassword", input_logBuffer);
	} else if (strstr(input_buffer, "Password")!=NULL){
		maskBuffer = alloca(inpbuflen + 1);
		memset(maskBuffer, 0, inpbuflen + 1);
		input_logBuffer = maskBuffer;
		ism_admin_maskJSONField(input_buffer, "Password", input_logBuffer);
	}

	LOG(INFO, Admin, 6014, "%-s", "Processing administrative request buffer: {0}", input_logBuffer);

	/* Parse input string and create JSON object */
	ism_json_parse_t parseobj = { 0 };
	ism_json_entry_t ents[40];
	char *pInpbuf = input_buffer;

	parseobj.ent = ents;
	parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
	parseobj.source = (char *) input_buffer;
	parseobj.src_len = inpbuflen + 1;
	ism_json_parse(&parseobj);

	memset(rbuf, 0, sizeof(rbuf));
	memset(lbuf, 0, sizeof(lbuf));

	memset(output_buffer->buf, 0, output_buffer->len);

	if (parseobj.rc) {

		LOG(ERROR, Admin, 6001, "%-s%d", "Failed to parse administrative request {0}: RC={1}.",
				input_logBuffer, parseobj.rc );

		int xlen;
		char  msgID[12];
		ism_admin_getMsgId(6001, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			char abf[12];
			repl[0] = pInpbuf;
			repl[1] = ism_common_itoa(parseobj.rc, abf);
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 2);
		} else {
			sprintf(rbuf, "Failed to parse administrative request %s: RC=%d.\n", input_logBuffer, parseobj.rc);
		}
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		return 1;
	}

	/* Get action and user */
	char *user = (char *)ism_json_getString(&parseobj, "User");
	char *action = (char *)ism_json_getString(&parseobj, "Action");
	//char *component = (char *)ism_json_getString(&parseobj, "Component");
	char *item = (char *)ism_json_getString(&parseobj, "Item");
	char *component = NULL;
	if ( strcmpi(action, "msshell")) {
		ism_config_getComponent(ISM_CONFIG_SCHEMA, item, &component, NULL);
	}

	/*Get Request Locale*/
	const char *locale = (const char *)ism_json_getString(&parseobj, "Locale");

	if(locale==NULL){
		/*Set to the server default locale*/
		locale = (const char *)ism_common_getLocale();
	}
	/*Set to Thread Specific*/
	ism_common_setRequestLocale(adminlocalekey, locale);

	int32_t actionType = ism_admin_getActionType(action);

	/* check if action is valid */
	if ( !user || ( actionType < 0 || actionType >= ISM_ADMIN_LAST )) {
		LOG(ERROR, Admin, 6128, "%-s%-s%-s", "Invalid administrative action {0} from the user: UserID={1} RequestBuffer={2}", action?action:"NULL", user?user:"NULL", input_logBuffer);

		int xlen;
		char  msgID[12];
		ism_admin_getMsgId(6128, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			char *npt="NULL";
			repl[0] = action?action:npt;
			repl[1] = user?user:npt;
			repl[2] = input_logBuffer;
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 3);
		} else {
			sprintf(rbuf, "Invalid administrative action %s from the user: UserID=%s RequestBuffer=%s\n", action?action:"NULL", user?user:"NULL", input_logBuffer);
		}

		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		if (component) ism_common_free(ism_memory_admin_misc,component);
		return 1;
	}

	if ( actionType == ISM_ADMIN_STATUS ) {
		LOG(INFO, Admin, 6004, "%-s%-s", "Processing administrative action {0} from the user: UserID={1}.", action, user);
	} else {
		LOG(NOTICE, Admin, 6004, "%-s%-s", "Processing administrative action {0} from the user: UserID={1}.", action, user);
	}

	/*If the Server is in StandBy, Only Allow server and HA component*/
	int isHaEnabled = 0;
	if (serverProcType == ISM_PROTYPE_SERVER) {
		int haRoleRC;
		ismHA_Role_t role = ism_admin_getHArole(NULL, &haRoleRC);
		isHaEnabled = 1;
		if (role == ISM_HA_ROLE_DISABLED) {
			isHaEnabled = 0;
		} else if (role == ISM_HA_ROLE_STANDBY) {
			if (component != NULL && (strcmp(component, "HA") && strcmp(component, "Server"))) {

				/*Set Error*/
				LOG(ERROR, Admin, 6121, "%s%s", "The " IMA_SVR_COMPONENT_NAME " status is \"{0}\". Any actions to \"{1}\" are not allowed.",
						"Standby", item);
				int xlen;
				char msgID[12];
				char tmpbuf[128];
				ism_admin_getMsgId(6121, msgID);
				if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf,
						sizeof(lbuf), &xlen) != NULL) {
					repl[0] = "Standby";
					repl[1] = item;
					ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 2);
				} else {
					sprintf( tmpbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " status is \"%s\". Any actions to \"%s\" are not allowed.",
							"Standby", item);
				}
				ism_admin_setReturnCodeAndStringJSON(output_buffer, ISMRC_ArgNotValid, tmpbuf);
				if(component) ism_common_free(ism_memory_admin_misc,component);
				return 1;
			}
		}
	}


	/* process action */
	int retcode = ISMRC_OK;

	/* check if authorized to take action only for imaserver process */
	if (serverProcType == ISM_PROTYPE_SERVER) {
		int secActionType = configuration_policy_action_lookup[actionType];
		retcode = ism_security_validate_policy(transport->security_context, ismSEC_AUTH_USER, NULL, secActionType, ISM_CONFIG_COMP_SECURITY, NULL);
		if ( retcode != ISMRC_OK ) {
			goto ENDADMINACTION;
		}
	}


	switch(actionType) {
	case ISM_ADMIN_SET:
	case ISM_ADMIN_CREATE:
	case ISM_ADMIN_UPDATE:
	case ISM_ADMIN_DELETE:
	{
		int haRoleRC;
		ismHA_Role_t role = ism_admin_getHArole(NULL, &haRoleRC);
		TRACE(8, "ism_process_admin_action: server_state: %d, isHaEnabled: %d, harole: %d, haRoleRC: %d\n", ism_admin_get_server_state(), isHaEnabled, role, haRoleRC);

		if ( serverProcType == ISM_PROTYPE_SERVER && isHaEnabled == 1 && role == ISM_HA_ROLE_PRIMARY ) {

			int pret = haSyncInProgress;
			int allow = 0;
			TRACE(4, "HA Node - check if synchronization is in progress: %d\n", pret);
			if( (item!=NULL && !strcasecmp(item,"LDAP") ) ||
					( component!=NULL && ( !strcmp(component,"Server") || !strcmp(component,"HA") )) )
			{
				allow = 1;
			}
			if (pret != 0 && allow == 0) {
				char errstr [512];
				ism_common_getErrorStringByLocale(ISMRC_HAInSync,  ism_common_getRequestLocale(adminlocalekey), errstr, sizeof(errstr));
				sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", ISMRC_HAInSync, errstr);
				LOG(ERROR, Admin, 353, NULL, "HighAvalibility node synchronization process is in progress. Configuration changes are not allowed at this time.");
				break;
			}
		}

		char *retbuf = NULL;
		retcode = ism_config_set_dynamic_extended(actionType, &parseobj, (char *)inpbuf, inpbuflen, &retbuf);

		if ( retcode == ISMRC_OK  || retcode == ISMRC_ClusterRemoveLocalServerNoAck) {
			int xlen;
			char  msgID[12];
			char tmpbuf[256];
			int msgnum = (retcode == ISMRC_OK ? 6011 : 6015);
			retcode = ISMRC_OK;
			ism_admin_getMsgId(msgnum, msgID);
			if (!retbuf) {
				if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
					repl[0] = 0;
					ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
				} else if (6011 == msgnum){
					sprintf(tmpbuf, "The requested configuration change has completed successfully." );
				} else {
					sprintf(tmpbuf, "This cluster member, which is the last server that is connected in the cluster\nhas left the cluster. The server received no acknowledgment of this action." );
				}
			}
			ism_admin_setReturnCodeAndStringJSON(output_buffer,retcode, retbuf ? retbuf : tmpbuf);
			if (retbuf) ism_common_free(ism_memory_admin_misc,retbuf);

			/* If Item is EnableDiskPersistence, then restart server if CleanStore is set */
			if ( component && item &&
					!strcasecmp(component, "Store") &&
					!strcasecmp(item, "EnableDiskPersistence") ) {
				char *cleanstore = (char *)ism_json_getString(&parseobj, "CleanStore");
				if ( cleanstore && ( *cleanstore == 'T' || *cleanstore == 't' || *cleanstore == '1') ) {
					/* Invoke clean store action with server restart */
					ism_admin_purgeStore(NULL);
				}
			}
		}
		break;
	}

	case ISM_ADMIN_GET:
	{
		if(item!=NULL && !strcmp(item,"AdminMode") && modeChanged==1) {
			sprintf(rbuf, "[  { \"AdminMode\":\"%d\" } ]",modeChangedPrev);
			retcode=ISMRC_OK;
			break;
		}

		if(item!=NULL && !strcmp("Protocol", item)){
			/*Protocol configuration is keep in memory*/
			retcode = ism_admin_getProtocolsInfo(&parseobj, output_buffer);
		}if(item!=NULL && !strcmp("Plugin", item)){
			/*Protocol configuration is keep in memory*/
			retcode = ism_admin_getPluginsInfo(&parseobj, output_buffer);
		}else{
			retcode = ism_config_get_dynamic(&parseobj, output_buffer);
		}
		break;
	}

	case ISM_ADMIN_STOP:
	{
		LOG(NOTICE, Admin, 6005, NULL, "The " IMA_SVR_COMPONENT_NAME_FULL " is stopping.");

		char *modeStr = (char *)ism_json_getString(&parseobj, "Mode");
		if ( modeStr && *modeStr == '1' ) {
			restartServer = 1;
		}

		pthread_mutex_lock(&adminFileLock);
		FILE *dest = NULL;
		dest = fopen("/tmp/imaserver.stop", "w");
		if( dest == NULL ) {
			TRACE(1,"Can not open /tmp/imaserver.stop file: errno=%d\n", errno);
		} else {
			fprintf(dest, "STOP");
			fclose(dest);
		}
		pthread_mutex_unlock(&adminFileLock);

		/* Send signal to stop, if pause is enabled.
		 * For all other cases exit out via a timer thread
		 */
		TRACE(1, "======== Server STOP invoked by %s ========\n", user);

		if ( restartServer == 1 ) {
			ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
			break;
		}

		if ( ismAdminPauseInited == 1) {
			ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
		} else {
			TRACE(1, "Set Timer Task to STOP server\n");
			ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) ism_admin_stopTimerTask, NULL, 10, 10, TS_SECONDS);
		}

		int xlen;
		char  msgID[12];
		ism_admin_getMsgId(6005, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0] = user;
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 1);
		} else {
			sprintf(rbuf, "The " IMA_SVR_COMPONENT_NAME_FULL " is stopping.\n");
		}

		break;
	}

	case ISM_ADMIN_SETMODE:
	{
		char *modeStr = (char *)ism_json_getString(&parseobj, "Mode");
		TRACE(5, "Changing server runmode from %d to %s.\n", serverState, modeStr?modeStr:"null");

		ism_admin_change_mode(modeStr, output_buffer);

		break;
	}

	case ISM_ADMIN_TRACE:
	{
		char *dbglog = (char *)ism_json_getString(&parseobj, "DBGLOG");
		if ( dbglog && *dbglog != '\0' ) {
			TRACE(1, "DEBUG_MESSAGE: %s\n", dbglog);
		} else {
			TRACE(1, "Trace buffer is flushed.\n");
		}
		int xlen;
		char  msgID[12];
		ism_admin_getMsgId(6007, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0] = 0;
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 0);
		} else {
			sprintf(rbuf, "Trace buffer is flushed.\n" );
		}
		break;
	}

	case ISM_ADMIN_STATUS:
	{
		int state = ism_admin_get_server_state();

		/* If there is a pending admin init error - set state to maintenance
		 * and adminModeRC to adminInitError
		 */
		if ( adminInitError != ISMRC_OK ) {
			state = ISM_SERVER_MAINTENANCE;
			//adminModeRC = adminInitError;
			TRACE(1,"Server is in maintenance mode. AdminInitError=%d\n", adminModeRC);
		}

		int xlen;
		char  msgID[12];
		char statusStr[128];
		sprintf(statusStr, "Status = %s", ism_admin_getStateStr(state));

		if ( serverProcType == ISM_PROTYPE_MQC ) {
			sprintf(rbuf, "{ \"RC\":\"0\",\"ServerState\":\"%d\",\"ServerStateStr\":\"%s\" }", state, statusStr);
			ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
			break;
		}

		/* get HA config */
		int isHAEnabled = ism_admin_isHAEnabled();

		/* invoke store ha API to get current mode */
		ismHA_View_t haView = {0};
		ismStore_Statistics_t storeStats = {0};
		int pctSyncCompletion = 0;
		ism_time_t primaryLastTime = 0;
		char *timeValue = NULL;
		char tbuffer[80];
		ism_ha_store_get_view(&haView);
		if ( ism_store_getStatistics(&storeStats) == ISMRC_OK ) {
			pctSyncCompletion = storeStats.HASyncCompletionPct;
			primaryLastTime = storeStats.PrimaryLastTime;

			//Convert to ISO8601
			ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
			if (primaryLastTime && ts != NULL) {
				ism_common_setTimestamp(ts, primaryLastTime);
				ism_common_formatTimestamp(ts, tbuffer, 80, 6, ISM_TFF_ISO8601);
				timeValue = tbuffer;
			}
			if (ts) {
				ism_common_closeTimestamp(ts);
			}
		}
		if (timeValue == NULL ) {
			timeValue = "";
		}

		uint64_t currenttime = (uint64_t) ism_common_readTSC() ;

		/*Get number of seconds that the server had been up*/
		uint64_t serverUpTime = currenttime - serverStartTime ;

		/*Convert to Days Hours Minutes Seconds format*/
		int days, hours, minutes, seconds;
		days = serverUpTime / SECS_PER_DAY;
		hours = (serverUpTime % SECS_PER_DAY) / SECS_PER_HOUR;
		minutes = ((serverUpTime % SECS_PER_DAY) % SECS_PER_HOUR) / SECS_PER_MIN;
		seconds = ((serverUpTime % SECS_PER_DAY) % SECS_PER_HOUR) % SECS_PER_MIN;

		char serverUPTimeStr[256];
		memset(&serverUPTimeStr, 0, 256);

		ism_admin_getMsgId(6131, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			char daysBuf[128];
			char hoursBuf[128];
			char minutesBuf[128];
			char secondsBuf[128];

			repl[0] = ism_common_itoa(days, daysBuf);
			repl[1] = ism_common_itoa(hours, hoursBuf);
			repl[2] = ism_common_itoa(minutes, minutesBuf);
			repl[3] = ism_common_itoa(seconds, secondsBuf);

			ism_common_formatMessage(serverUPTimeStr, sizeof(serverUPTimeStr), lbuf, repl, 4);

		} else {
			sprintf(serverUPTimeStr, "%d days %d hours %d minutes %d seconds",days, hours, minutes, seconds );
		}


		/* Note:
		 * The WebUI will get the serverUpTime field, and format it to fit into the panel/page.
		 * The CLI will get the serverUpTimeStr, and just display it.
		 */
		int confmode = ism_admin_getmode();
		int internalErrorCode = ism_admin_getInternalErrorCode();
		if (modeChanged) {
			sprintf(rbuf, "{ \"RC\":\"0\",\"ServerState\":\"%d\",\"ServerStateStr\":\"%s\",\"CurrentMode\":\"%d\",\"ConfiguredMode\":\"%d\",\"ModeChanged\":\"1\",\"PendingMode\":\"%d\",\"AdminStateRC\":\"%d\",\"ServerUPTime\":\"%ld\",\"ServerUPTimeStr\":\"%s\",\"EnableDiskPersistence\":\"%d\",",
					state, statusStr, modeChangedPrev, confmode, modeChangedValue,
					internalErrorCode?internalErrorCode:adminModeRC, serverUpTime,
							serverUPTimeStr, ism_admin_getDiskMode());
		} else {
			sprintf(rbuf, "{ \"RC\":\"0\",\"ServerState\":\"%d\",\"ServerStateStr\":\"%s\",\"CurrentMode\":\"%d\",\"ConfiguredMode\":\"%d\",\"ModeChanged\":\"0\",\"PendingMode\":\"0\",\"AdminStateRC\":\"%d\",\"ServerUPTime\":\"%ld\",\"ServerUPTimeStr\":\"%s\",\"EnableDiskPersistence\":\"%d\",",
					state, statusStr, confmode, confmode,
					internalErrorCode?internalErrorCode:adminModeRC, serverUpTime,
							serverUPTimeStr,ism_admin_getDiskMode());
		}
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));

		/*Add AdminRCStr*/
		sprintf(rbuf,"\"AdminStateRCStr\":");
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		if (adminModeRC != ISMRC_OK || internalErrorCode != ISMRC_OK ){

			char tmpbuf[2048];

			ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), internalErrorCode);

			ism_json_putString(output_buffer, tmpbuf);

		}else{
			ism_common_allocBufferCopyLen(output_buffer, "\"\"", 2);
		}

		ism_common_allocBufferCopyLen(output_buffer, ",", 1);
		char *pReasonParam = (char *)(haView.pReasonParam);
		if (!isHAEnabled) {
			strcpy(rbuf, "\"EnableHA\":false");
		} else {
			if ((modeChangedPrev != - 1 && confmode == modeChangedPrev && confmode == ISM_MODE_MAINTENANCE) ||
					(modeChangedPrev == -1 && confmode == ISM_MODE_MAINTENANCE) ) {
				sprintf(rbuf,
						"\"EnableHA\":true,\"NewRole\":\"UNKNOWN\",\"OldRole\":\"UNKNOWN\",\"ActiveNodes\":\"%d\",\"SyncNodes\":\"%d\",\"PrimaryLastTime\":\"%s\",\"PctSyncCompletion\":\"%d\",\"ReasonCode\":\"%d\",\"ReasonString\":\"%s\"",
						haView.ActiveNodesCount, haView.SyncNodesCount, primaryLastTime?timeValue:"", pctSyncCompletion, haView.ReasonCode,
								pReasonParam? pReasonParam:"" );
			} else {
				sprintf(rbuf,
						"\"EnableHA\":true,\"NewRole\":\"%s\",\"OldRole\":\"%s\",\"ActiveNodes\":\"%d\",\"SyncNodes\":\"%d\",\"PrimaryLastTime\":\"%s\",\"PctSyncCompletion\":\"%d\",\"ReasonCode\":\"%d\",\"ReasonString\":\"%s\"",
						ism_admin_get_harole_string(haView.NewRole), ism_admin_get_harole_string(haView.OldRole),
						haView.ActiveNodesCount, haView.SyncNodesCount, primaryLastTime?timeValue:"", pctSyncCompletion, haView.ReasonCode,
								pReasonParam? pReasonParam:"" );
			}
		}
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		// cluster information
		getStatistics_f getStats = (getStatistics_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_getStatistics_fnptr", 0L);

		if (getStats) {
			ismCluster_Statistics_t stats;
			int statsRC = getStats(&stats);
			if (!statsRC) {
				char *cstate = "Unknown";
				switch (stats.state) {
				case ISM_CLUSTER_LS_STATE_INIT:
					cstate = "Initializing";
					break;
				case ISM_CLUSTER_LS_STATE_DISCOVER:
					cstate = "Discover";
					break;
				case ISM_CLUSTER_LS_STATE_ACTIVE:
					cstate = "Active";
					break;
				case ISM_CLUSTER_LS_STATE_STANDBY:
					cstate = "Standby";
					break;
				case ISM_CLUSTER_LS_STATE_REMOVED:
					cstate = "Removed";
					break;
				case ISM_CLUSTER_LS_STATE_ERROR:
					cstate = "Error";
					break;
				case ISM_CLUSTER_LS_STATE_DISABLED:
					cstate = "Disabled";
					break;
				case ISM_CLUSTER_RS_STATE_ACTIVE:
				case ISM_CLUSTER_RS_STATE_CONNECTING:
				case ISM_CLUSTER_RS_STATE_INACTIVE:
					TRACE(2, "Unexpected state returned by ism_cluster_getStatistics: %d.", stats.state);
				}
				sprintf(rbuf, ", \"ClusterState\": \"%s\", \"ClusterName\": \"%s\", \"ServerName\": \"%s\", \"ActiveServers\": \"%d\", \"InactiveServers\": \"%d\""
						, cstate, stats.pClusterName ? stats.pClusterName : "NULL"
								, stats.pServerName ? stats.pServerName : "NULL"
										, stats.connectedServers, stats.disconnectedServers);
				ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
			} else {
				char *status = NULL;
				switch (statsRC) {
				case ISMRC_ClusterDisabled:
					status = "Disabled";
					break;
				case ISMRC_ClusterNotAvailable:
					status = "Unavailable";
					break;
				case ISMRC_NotImplemented:
					status = "NotImplemented";
					break;
				}
				if (status) {
					sprintf(rbuf, ", \"ClusterState\": \"%s\"", status);
					ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
				} else {
					TRACE(2, "Unexpected return code from ism_cluster_getStatistics: rc=%d\n", statsRC);
				}
			}
		}
		sprintf(rbuf, "}");

		break;
	}

	case ISM_ADMIN_VERSION:
	{
		int xlen;
		char  msgID[12];
		ism_admin_getMsgId(6010, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0] = ism_common_getVersion();
			repl[1] = ism_common_getBuildLabel();
			repl[2] = ism_common_getBuildTime();
			ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 3);
		} else {
			sprintf(rbuf, IMA_PRODUCTNAME_FULL " version is %s %s %s", ism_common_getVersion(), ism_common_getBuildLabel(),
					ism_common_getBuildTime());
		}
		break;
	}

	case ISM_ADMIN_HAROLE:
	{
		/* check if user is authorized to invoke HAMODE */
		TRACE(5, "" IMA_SVR_COMPONENT_NAME_FULL " HAMode invoked by %s. \n", user);

		ismHA_View_t haView = {0};
		ism_ha_store_get_view(&haView);

		/* Check if it doesn't impact performance - get WebUI info from standby, for now set this to cached data */
		char *remServer = remoteServerName;
		sprintf(rbuf,
				"{ \"NewRole\":\"%s\",\"OldRole\":\"%s\",\"ActiveNodes\":\"%d\",\"SyncNodes\":\"%d\",\"ReasonCode\":\"%d\",\"ReasonString\":\"%s\",\"LocalReplicationNIC\":\"%s\",\"LocalDiscoveryNIC\":\"%s\",\"RemoteDiscoveryNIC\":\"%s\",\"RemoteServerName\":\"%s\" }",
				ism_admin_get_harole_string(haView.NewRole), ism_admin_get_harole_string(haView.OldRole),
				haView.ActiveNodesCount, haView.SyncNodesCount, haView.ReasonCode,
				haView.pReasonParam? haView.pReasonParam:"",
						haView.LocalReplicationNIC?haView.LocalReplicationNIC:"",
								haView.LocalDiscoveryNIC?haView.LocalDiscoveryNIC:"",
										haView.RemoteDiscoveryNIC?haView.RemoteDiscoveryNIC:"",
												remServer?remServer:"");

		break;
	}

	case ISM_ADMIN_CERTSYNC:
	{
		int32_t lrc = ISMRC_OK;
		/* check current HA role */
		ismHA_View_t haView = {0};
		ism_ha_store_get_view(&haView);
		if ( haView.NewRole == ISM_HA_ROLE_PRIMARY ) {
			lrc = ism_admin_ha_syncMQCertificates();
			TRACE(5, "MQConnectivity Certificate sync is initiated: RC=%d\n", lrc);
			sprintf(rbuf, "Certificate Sync is initialized\n");
		} else {
			sprintf(rbuf, "It is not a Primary node. Certificate Sync is ignored.\n");
		}
		break;
	}

	case ISM_ADMIN_SYNCFILE:
	{
		/* check current HA role */
		ismHA_View_t haView = {0};
		ism_ha_store_get_view(&haView);
		if ( haView.NewRole == ISM_HA_ROLE_PRIMARY ) {
			const char *dirname = ism_json_getString(&parseobj, "DIRName");
			const char *filename = ism_json_getString(&parseobj, "FILEName");
			ism_admin_ha_syncFile(dirname, filename);
			sprintf(rbuf, "File sync is initiated\n");
		} else {
			sprintf(rbuf, "It is not a Primary node. File Sync is ignored.\n");
		}
		break;
	}

	case ISM_ADMIN_NOTIFY:
	{
		if(item!=NULL && !strcmp("Plugin", item)){
			retcode = ism_admin_processPluginNotification(&parseobj, output_buffer);
		}else if(item!=NULL && !strcmp("PreSharedKey", item)){
			retcode = ism_admin_processPSKNotification(&parseobj, output_buffer);
		}
		break;
	}


	/* Enable to implement purge option */
	case ISM_ADMIN_PURGE:
	{
		char *modeStr = (char *)ism_json_getString(&parseobj, "Object");
		TRACE(5, "" IMA_SVR_COMPONENT_NAME_FULL " purge %s invoked by %s.\n", modeStr?modeStr:"null", user);

		/* check if cleaning store */
		if (modeStr && !strcasecmp(modeStr, "Store")) {
			char *force = (char *)ism_json_getString(&parseobj, "Force");
			if ( force && !strcasecmp(force, "true")) {
				retcode = ism_admin_purgeStore(output_buffer);
			} else {
				char *mStr = "2";
				TRACE(5, "Changing server runmode from %d to %s.\n", serverState, mStr);
				ism_admin_change_mode(mStr, output_buffer);
			}
		} else {
			/* Unsupported option */
			ism_common_getErrorStringByLocale(6008, ism_common_getRequestLocale(adminlocalekey), rbuf, sizeof(rbuf));
			break;
		}

		break;
	}

	case ISM_ADMIN_RUNMSSHELLSCRIPT:
	{
		retcode = ism_admin_runMsshellScript(&parseobj, output_buffer);
		break;
	}

	default:
	{
		/* Unsupported option */
		ism_common_getErrorStringByLocale(6008, ism_common_getRequestLocale(adminlocalekey), rbuf, sizeof(rbuf));
		break;
	}
	}

	ENDADMINACTION:

	if ( retcode != ISMRC_OK ) {
		char buf[1024];
		char tbuf[1024];
		char *bufptr = buf;
		char *errstr = NULL;
		int inheap = 0;
		int bytes_needed = 0;
		const char * requestLocale = ism_common_getRequestLocale(adminlocalekey);

		if (ism_common_getLastError() > 0) {
			bytes_needed= ism_common_formatLastErrorByLocale(requestLocale, bufptr, sizeof(buf));
			if (bytes_needed > sizeof(buf)) {
				bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,574),bytes_needed);
				inheap=1;
				bytes_needed = ism_common_formatLastErrorByLocale(requestLocale, bufptr, bytes_needed);
			}
		}
		if (bytes_needed > 0) {
			errstr=(char *)&buf;
		} else {
			errstr = (char *)ism_common_getErrorStringByLocale(retcode,requestLocale, buf, sizeof(buf));
		}
		if ( errstr ) {

			sprintf(tbuf, "{ \"RC\":\"%d\", \"ErrorString\":", retcode);

			ism_common_allocBufferCopyLen(output_buffer, tbuf, strlen(tbuf));
			/*JSON encoding for the Error String.*/
			ism_json_putString(output_buffer, errstr);

			ism_common_allocBufferCopyLen(output_buffer, "}", 1);

		} else {
			sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Unknown\" }", retcode);
		}
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
		if(strcmp(requestLocale,  systemLocale)){
			bytes_needed=0;
			if(errstr!=NULL){
				bytes_needed= ism_common_formatLastErrorByLocale(systemLocale, bufptr, sizeof(buf));
				if (bytes_needed > sizeof(buf)) {
					bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,576),bytes_needed);
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
	}

	if ( actionType != ISM_ADMIN_TEST )
		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));

	if (component) ism_common_free(ism_memory_admin_misc,component);
	if ( output_buffer->used > 0 ) {
		return 0;
	} else {
		*rc = ENODATA;
		return 1;
	}
}



/**
 * Set the last error code and string into the Admin component.  
 * 
 */
XAPI void ism_admin_setLastRCAndError(void)
{
	/*Get the Last Error RC*/
	int rc = ism_common_getLastError();

	__sync_val_compare_and_swap( &adminModeRC, adminModeRC, rc);
	if(rc>ISMRC_OK){
		char buf[2048];
		char *bufptr=buf;
		int inheap=0;
		int bytes_needed =0;

		concat_alloc_t * tmpconfbuf = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,579),1, sizeof(concat_alloc_t));
		concat_alloc_t * tmpadminModeRCReplDataBuf = adminModeRCReplDataBuf;

		adminModeRCReplDataCount = ism_common_getLastErrorReplData(tmpconfbuf);

		__sync_bool_compare_and_swap( &adminModeRCReplDataBuf, adminModeRCReplDataBuf, tmpconfbuf);

		if(tmpadminModeRCReplDataBuf!=NULL){
			ism_common_freeAllocBuffer(tmpadminModeRCReplDataBuf);
			ism_common_free(ism_memory_admin_misc,tmpadminModeRCReplDataBuf);
			tmpadminModeRCReplDataBuf=NULL;
		}

		/*Get the Last Error String*/
		/*Since this is for Logging. Use System Locale*/
		bytes_needed= ism_common_formatLastErrorByLocale(ism_common_getLocale(), bufptr,1024);
		if (bytes_needed > 1024) {
			bufptr = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,581),bytes_needed);
			inheap=1;
			ism_common_formatLastErrorByLocale(ism_common_getRequestLocale(adminlocalekey), bufptr,bytes_needed);
		}

		LOG(ERROR, Admin, 6120, "%d%-s", IMA_PRODUCTNAME_FULL " detects an error. The error code is {0}. The error string is {1}.", adminModeRC, bufptr);

		if(inheap){
			if(bufptr!=NULL) ism_common_free(ism_memory_admin_misc,bufptr);
		}



	}else{

		adminModeRCReplDataCount=0;
	}
}

/* return server process type */
XAPI int32_t ism_admin_getServerProcType(void) {
	return serverProcType;
}


static void ism_admin_toLowerStr(char * lstring)
{
	if(lstring){
		char *tmpstr = lstring;
		while (*tmpstr) {
			*tmpstr = tolower(*tmpstr);
			tmpstr++;
		}
	}
}


/**
 * Update the Capabilities for a protocol.
 *
 */
XAPI int32_t ism_admin_updateProtocolCapabilities(const char * name, int capabilities)
{
	if(name!=NULL && capabilities!=-1){
		int len = strlen(name);

		char * tmpname = (char *)alloca(len+1);
		memcpy(tmpname, name, len);
		tmpname[len]='\0';
		ism_admin_toLowerStr(tmpname);

		ismSecProtocol * secprotocol = (ismSecProtocol *) ism_common_getHashMapElement(ismSecProtocolMap, (void *)tmpname, len);
		if(secprotocol==NULL){
			secprotocol = ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,583),1, sizeof(ismSecProtocol));
			secprotocol->id = secProtocolNextId;
			secProtocolNextId++; //Increase the next id
			ism_common_putHashMapElement(ismSecProtocolMap, name, len, (void *)secprotocol, NULL);
			ism_security_updatePoliciesProtocol();
		}
		secprotocol->capabilities |= capabilities;
		return 0;
	}else if(capabilities<0){
		if(name!=NULL && (strcasecmp(name,"mqtt")!=0) &&  (strcasecmp(name,"jms")!=0)){
			ism_common_removeHashMapElement(ismSecProtocolMap, name, strlen(name));
		}
	}
	return -1;
}

/**
 * Get the Capabilities for a protocol.
 *
 */
XAPI int ism_admin_getProtocolCapabilities(const char * name)
{
	if (name != NULL) {
		int len = strlen(name);
		char * tmpname = (char *)alloca(len+1);
		memcpy(tmpname, name, len);
		tmpname[len]='\0';
		ism_admin_toLowerStr(tmpname);


		ismSecProtocol * secprotocol = (ismSecProtocol *)ism_common_getHashMapElement(ismSecProtocolMap, (void *)tmpname, len);
		if(secprotocol!=NULL){
			return secprotocol->capabilities;
		}

	}
	return 0;
}

/**
 * Get the ID for a protocol.
 *
 */
int ism_admin_getProtocolId(const char * name)
{
	if (name != NULL) {
		int len = strlen(name);
		char * tmpname = (char *)alloca(len+1);
		memcpy(tmpname, name, len);
		tmpname[len]='\0';
		ism_admin_toLowerStr(tmpname);

		ismSecProtocol * secprotocol = (ismSecProtocol *)ism_common_getHashMapElement(ismSecProtocolMap, (void *)tmpname, len);
		if(secprotocol!=NULL){
			return secprotocol->id;
		}

	}
	return -1;
}

/*
 * Returns cluster status string
 */
XAPI char * ism_config_getClusterStatusStr(void) {
	int statsRC = 0;
	char *cstate = "Unknown";
	getStatistics_f getStats = (getStatistics_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_getStatistics_fnptr", 0L);

	if (getStats) {
		ismCluster_Statistics_t stats;
		statsRC = getStats(&stats);
		if (! statsRC) {
			switch (stats.state) {
			case ISM_CLUSTER_LS_STATE_INIT:
				cstate = "Initializing";
				break;
			case ISM_CLUSTER_LS_STATE_DISCOVER:
				cstate = "Discover";
				break;
			case ISM_CLUSTER_LS_STATE_ACTIVE:
				cstate = "Active";
				break;
			case ISM_CLUSTER_LS_STATE_STANDBY:
				cstate = "Standby";
				break;
			case ISM_CLUSTER_LS_STATE_REMOVED:
				cstate = "Removed";
				break;
			case ISM_CLUSTER_LS_STATE_ERROR:
				cstate = "Error";
				break;
			case ISM_CLUSTER_LS_STATE_DISABLED:
				cstate = "Inactive";
			case ISM_CLUSTER_RS_STATE_ACTIVE:
			case ISM_CLUSTER_RS_STATE_CONNECTING:
			case ISM_CLUSTER_RS_STATE_INACTIVE:
				TRACE(5, "%s: Remote server status %d is ignored during local status call.\n", __FUNCTION__, statsRC);
			}

		} else {

			switch ( statsRC) {
			case ISMRC_ClusterDisabled:
				cstate = "Inactive";
				break;
			case ISMRC_ClusterNotAvailable:
				cstate = "Unavailable";
				break;
			case ISMRC_NotImplemented:
				cstate = "NotImplemented";
				break;
			}
			if ( !strcmp(cstate, "Unknown") ) {
				TRACE(2, "%s: Unexpected return code from ism_cluster_getStatistics: rc=%d\n", __FUNCTION__, statsRC);
			}
		}
	}

	return cstate;
}

/*
 * Get imaserver status REST API return JSON payload in http outbuf.
 *
 * @param  http			  ism_http_t object.
 * @param service         the service for which to retrieve status
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 */
int ism_admin_getServerStatus(ism_http_t *http, const char * service, int fromSNMP) {
	int rc = ISMRC_OK;
	char  rbuf[4096];
	char  lbuf[2048];
	const char * repl[5];
	int replSize = 0;
	char *locale = NULL;
    char tbuf[2048];
    concat_alloc_t outbuf = { tbuf, sizeof(tbuf), 0, 0 };

	/*Get Request Locale*/
	if (http && http->locale && *(http->locale) != '\0') {
		locale = http->locale;
	} else {
		locale = "en_US";
	}

	/*Set to Thread Specific*/
	ism_common_setRequestLocale(adminlocalekey, locale);

	int state = ism_admin_get_server_state();
	int lastAdminModeRC = ism_config_json_getAdminModeRC();

	/* If there is a pending admin init error - set state to maintenance
	 * and adminModeRC to adminInitError
	 */
	if ( adminInitError != ISMRC_OK  || lastAdminModeRC == ISMRC_LicenseError ) {
		state = ISM_SERVER_MAINTENANCE;
		// adminModeRC = adminInitError;
		TRACE(1,"Server is in maintenance mode. AdminInitError=%d AdminModeRC=%d\n", adminInitError, adminModeRC?adminModeRC:lastAdminModeRC);
	}

	/* This function can not be invoked on mqcbridge process */
	int	lserverProcType = ism_admin_getServerProcType();
	if ( lserverProcType == ISM_PROTYPE_MQC ) {
		TRACE(3, "%s: MQConnectivity status is not supported at this time.\n", __FUNCTION__);
		rc = ISMRC_ArgNotValid;
		ism_common_setErrorData(rc, "%s", "MQConnectivity");
		goto GETSTATUS_END;
	}

	int xlen;
	char  msgID[12];
	char statusStr[128];
	char stateStr[128];
	sprintf(statusStr, "%s", ism_admin_getStatusStr(state));
	sprintf(stateStr, "%s", ism_admin_getStateStr(state));
	ism_json_putBytes(&outbuf, "{\n  \"Version\":\"");
	ism_json_putEscapeBytes(&outbuf, SERVER_SCHEMA_VERSION, (int) strlen(SERVER_SCHEMA_VERSION));
	ism_json_putBytes(&outbuf, "\"");

	if (!service || strcmp(service, "Server") == 0 || strcmp(service, "imaserver") == 0 ) {

		ism_json_putBytes(&outbuf, ",\n  \"Server\": {\n");

		uint64_t currenttime = (uint64_t) ism_common_readTSC() ;

		/*Get number of seconds that the server had been up*/
		uint64_t serverUpTime = currenttime - serverStartTime ;

		/*Convert to Days Hours Minutes Seconds format*/
		int days, hours, minutes, seconds;
		days = serverUpTime / SECS_PER_DAY;
		hours = (serverUpTime % SECS_PER_DAY) / SECS_PER_HOUR;
		minutes = ((serverUpTime % SECS_PER_DAY) % SECS_PER_HOUR) / SECS_PER_MIN;
		seconds = ((serverUpTime % SECS_PER_DAY) % SECS_PER_HOUR) % SECS_PER_MIN;

		char serverUPTimeStr[256];
		memset(&serverUPTimeStr, 0, 256);

		ism_admin_getMsgId(6131, msgID);
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(adminlocalekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			char daysBuf[128];
			char hoursBuf[128];
			char minutesBuf[128];
			char secondsBuf[128];

			repl[0] = ism_common_itoa(days, daysBuf);
			repl[1] = ism_common_itoa(hours, hoursBuf);
			repl[2] = ism_common_itoa(minutes, minutesBuf);
			repl[3] = ism_common_itoa(seconds, secondsBuf);

			ism_common_formatMessage(serverUPTimeStr, sizeof(serverUPTimeStr), lbuf, repl, 4);

		} else {
			sprintf(serverUPTimeStr, "%d days %d hours %d minutes %d seconds",days, hours, minutes, seconds );
		}

		/* Get current server time */
	    char serverTime[32];
	    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
	    ism_common_setTimestamp(ts, ism_common_currentTimeNanos());
	    ism_common_formatTimestamp(ts, serverTime, sizeof(serverTime), 0, ISM_TFF_ISO8601);
	    ism_common_closeTimestamp(ts);

		/* Note:
		 * The WebUI will get the serverUpTime field, and format it to fit into the panel/page.
		 * The CLI will get the serverUpTimeStr, and just display it.
		 */
		int internalErrorCode = ism_admin_getInternalErrorCode();

	    if ( internalErrorCode == 0 && adminInitError == ISMRC_RestartNeeded && backupApplied == 1 ) {
	    	internalErrorCode = ISMRC_RestartNeeded;
	    }

		char *sName = ism_config_getServerName();
		char *sUID = (char *)ism_common_getServerUID();

		sprintf(rbuf, "    \"Name\": \"%s\",\n", sName?sName:"");
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"UID\": \"%s\",\n", sUID?sUID:"");
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"Status\": \"%s\",\n", statusStr);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"State\": %d,\n", state);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"StateDescription\": \"%s\",\n", stateStr);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"ServerTime\": \"%s\",\n", serverTime);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"UpTimeSeconds\": %ld,\n", serverUpTime);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"UpTimeDescription\": \"%s\",\n", serverUPTimeStr);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		sprintf(rbuf, "    \"Version\": \"%s\",\n", serverVersion);
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

		if ( sName ) ism_common_free(ism_memory_admin_misc,sName);

		if ( adminInitError == ISMRC_LicenseError ) {
			sprintf(rbuf, "    \"ErrorCode\": %d,\n", ISMRC_LicenseError);
			ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
			sprintf(rbuf, "    \"ErrorMessage\": \"");
			ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

			char tmpbuf[2048];
			ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), ISMRC_LicenseError);
			sprintf(rbuf,"%s", tmpbuf);
			ism_json_putEscapeBytes(&outbuf, rbuf, strlen(rbuf));
			ism_json_putBytes(&outbuf, "\"\n");

		} else if ( haRestartNeeded != ISMRC_OK ) {
		    sprintf(rbuf, "    \"ErrorCode\": %d,\n", haRestartNeeded);
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		    sprintf(rbuf, "    \"ErrorMessage\": \"");
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

			char tmpbuf[2048];
			ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), ISMRC_RestartNeeded);
			sprintf(rbuf,"%s", tmpbuf);
			ism_json_putEscapeBytes(&outbuf, rbuf, strlen(rbuf));
			ism_json_putBytes(&outbuf, "\"\n");

		} else if (adminModeRC != ISMRC_OK || internalErrorCode != ISMRC_OK ) {

		    sprintf(rbuf, "    \"ErrorCode\": %d,\n", internalErrorCode?internalErrorCode:adminModeRC);
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		    sprintf(rbuf, "    \"ErrorMessage\": \"");
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

			char tmpbuf[2048];
			if ( internalErrorCode == 1100 ) internalErrorCode = ISMRC_Error;
			if ( internalErrorCode == 1001 ) internalErrorCode = ISMRC_AllocateError;
			ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), internalErrorCode);
			sprintf(rbuf,"%s", tmpbuf);
			ism_json_putEscapeBytes(&outbuf, rbuf, strlen(rbuf));
			ism_json_putBytes(&outbuf, "\"\n");

		} else if ( lastAdminModeRC != ISMRC_OK ) {
		    sprintf(rbuf, "    \"ErrorCode\": %d,\n", lastAdminModeRC);
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		    sprintf(rbuf, "    \"ErrorMessage\": \"");
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

			char tmpbuf[2048];
			ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), lastAdminModeRC);
			sprintf(rbuf,"%s", tmpbuf);
			ism_json_putEscapeBytes(&outbuf, rbuf, strlen(rbuf));
			ism_json_putBytes(&outbuf, "\"\n");
		} else {
		    sprintf(rbuf, "    \"ErrorCode\": 0,\n");
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		    sprintf(rbuf, "    \"ErrorMessage\": \"");
		    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
			ism_json_putBytes(&outbuf, "\"\n");
		}

		/* Add container UUID, if running in Docker container */
		if ( container_uuid ) {
			sprintf(rbuf, "  },\n  \"Container\": {\n    \"UUID\":\"%s\"\n", container_uuid);
			ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		}

		ism_json_putBytes(&outbuf, "  }");
	}

	if ( !service || strcmp(service, "HighAvailability") == 0 ) {
		ism_json_putBytes(&outbuf, ",\n  \"HighAvailability\": {\n");

		/* get HA config */
		int isHAEnabled = ism_admin_isHAEnabled();

		/* invoke store ha API to get current mode */
		ismHA_View_t haView = {0};
		ismStore_Statistics_t storeStats = {0};
		int pctSyncCompletion = 0;
		ism_time_t primaryLastTime = 0;
		char *timeValue = NULL;
		char tbuffer[80];

		ism_ha_store_get_view(&haView);
		if ( ism_store_getStatistics(&storeStats) == ISMRC_OK ) {
			pctSyncCompletion = storeStats.HASyncCompletionPct;
			primaryLastTime = storeStats.PrimaryLastTime;

			//Convert to ISO8601
			ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
			if (primaryLastTime && ts != NULL) {
				ism_common_setTimestamp(ts, primaryLastTime);
				ism_common_formatTimestamp(ts, tbuffer, 80, 6, ISM_TFF_ISO8601);
				timeValue = tbuffer;
			}
			if (ts) {
				ism_common_closeTimestamp(ts);
			}
		}

		if (timeValue == NULL ) {
			timeValue = "";
		}

		int confmode = ism_admin_getmode();
		char *haGroupName = ism_config_getStringObjectValue("HighAvailability", "haconfig", "Group", 1);
		int haNodeCount = haView.ActiveNodesCount;
		int haSyncNode = haView.SyncNodesCount;
		int haRC = haView.ReasonCode;
		char *haRS = (char *)haView.pReasonParam;
		if ( haRC < 0 || haRC > 8 ) haRC = 8;
		char *haRCStr = (char *)ism_config_json_haRCString(haRC);

		char *haEnableKey = "Enabled";
		if ( fromSNMP ) {
			haEnableKey = "EnableHA";
		}

		if (!isHAEnabled && haNodeCount == 0) {
			if (fromSNMP) {
				strcpy(rbuf, "    \"Status\": \"Inactive\",\n    \"EnableHA\": false,\n    \"NewRole\": \"UNKNOWN\",\n    \"OldRole\": \"UNKNOWN\",\n    \"ActiveNodes\": 0,\n    \"SyncNodes\": 0,\n    \"PrimaryLastTime\": \"N/A\",\n    \"PctSyncCompletion\": 0,\n    \"ReasonCode\": 0,\n    \"RemoteServerName\":\"\"\n");
			} else {
				strcpy(rbuf, "    \"Status\": \"Inactive\",\n    \"Enabled\": false\n");
			}
		} else {
			if ((modeChangedPrev != - 1 && confmode == modeChangedPrev && confmode == ISM_MODE_MAINTENANCE) ||
					(modeChangedPrev == -1 && confmode == ISM_MODE_MAINTENANCE) ) {
				if (haRC) {
					sprintf(rbuf,
							"    \"Status\": \"Active\",\n    \"%s\": %s,\n    \"Group\": \"%s\",\n    \"NewRole\": \"UNKNOWN\",\n    \"OldRole\": \"UNKNOWN\",\n    \"ActiveNodes\": %d,\n    \"SyncNodes\": %d,\n    \"PrimaryLastTime\": \"%s\",\n    \"PctSyncCompletion\": %d,\n    \"ReasonCode\": %d,\n    \"ReasonString\": \"%s - %s\",\n    \"RemoteServerName\":\"%s\"\n",
							haEnableKey, isHAEnabled?"true":"false", haGroupName?haGroupName:"", haNodeCount, haSyncNode, primaryLastTime?timeValue:"", pctSyncCompletion, haRC, haRS? haRS:"", haRCStr, remoteServerName?remoteServerName:"");
				} else {
					sprintf(rbuf,
							"    \"Status\": \"Active\",\n    \"%s\": %s,\n    \"Group\": \"%s\",\n    \"NewRole\": \"UNKNOWN\",\n    \"OldRole\": \"UNKNOWN\",\n    \"ActiveNodes\": %d,\n    \"SyncNodes\": %d,\n    \"PrimaryLastTime\": \"%s\",\n    \"PctSyncCompletion\": %d,\n    \"ReasonCode\": %d,\n    \"RemoteServerName\":\"%s\"\n",
							haEnableKey, isHAEnabled?"true":"false", haGroupName?haGroupName:"", haNodeCount, haSyncNode, primaryLastTime?timeValue:"", pctSyncCompletion, haRC, remoteServerName?remoteServerName:"");
				}
			} else {
				char *haStatusStr = "Active";
				char *nRole = ism_admin_get_harole_string(haView.NewRole);
				char *oRole = ism_admin_get_harole_string(haView.OldRole);

				if ( haView.NewRole == ISM_HA_ROLE_DISABLED ) haStatusStr = "Inactive";

				if (haRC) {
					sprintf(rbuf,
							"    \"Status\": \"%s\",\n    \"%s\": %s,\n    \"Group\": \"%s\",\n    \"NewRole\": \"%s\",\n    \"OldRole\": \"%s\",\n    \"ActiveNodes\": %d,\n    \"SyncNodes\": %d,\n    \"PrimaryLastTime\": \"%s\",\n    \"PctSyncCompletion\": %d,\n    \"ReasonCode\": %d,\n    \"ReasonString\": \"%s - %s\",\n    \"RemoteServerName\":\"%s\"\n",
							haStatusStr, haEnableKey, isHAEnabled?"true":"false", haGroupName?haGroupName:"", nRole, oRole, haNodeCount, haSyncNode, primaryLastTime?timeValue:"", pctSyncCompletion, haRC, haRS? haRS:"", haRCStr,  remoteServerName?remoteServerName:"");
				} else {
					sprintf(rbuf,
							"    \"Status\": \"%s\",\n    \"%s\": %s,\n    \"Group\": \"%s\",\n    \"NewRole\": \"%s\",\n    \"OldRole\": \"%s\",\n    \"ActiveNodes\": %d,\n    \"SyncNodes\": %d,\n    \"PrimaryLastTime\": \"%s\",\n    \"PctSyncCompletion\": %d,\n    \"ReasonCode\": %d,\n    \"RemoteServerName\":\"%s\"\n",
							haStatusStr, haEnableKey, isHAEnabled?"true":"false", haGroupName?haGroupName:"", nRole, oRole, haNodeCount, haSyncNode, primaryLastTime?timeValue:"", pctSyncCompletion, haRC,  remoteServerName?remoteServerName:"");
				}
			}
		}

		if ( haGroupName ) ism_common_free(ism_memory_admin_misc,haGroupName);

		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
		ism_json_putBytes(&outbuf, "  }");

	}

	if ( !service || strcmp(service, "Cluster") == 0 ) {
		ism_json_putBytes(&outbuf, ",\n  \"Cluster\": {\n");

		int cmEnabled = ism_admin_isClusterConfigured();

		getStatistics_f getStats = (getStatistics_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_getStatistics_fnptr", 0L);

		if (getStats) {
			ismCluster_Statistics_t stats;
			int statsRC = getStats(&stats);
			if (!statsRC) {
				char *cstate = "Unknown";
				switch (stats.state) {
				case ISM_CLUSTER_LS_STATE_INIT:
					cstate = "Initializing";
					break;
				case ISM_CLUSTER_LS_STATE_DISCOVER:
					cstate = "Discover";
					break;
				case ISM_CLUSTER_LS_STATE_ACTIVE:
					cstate = "Active";
					break;
				case ISM_CLUSTER_LS_STATE_STANDBY:
					cstate = "Standby";
					break;
				case ISM_CLUSTER_LS_STATE_REMOVED:
					cstate = "Removed";
					break;
				case ISM_CLUSTER_LS_STATE_ERROR:
					cstate = "Error";
					break;
				case ISM_CLUSTER_LS_STATE_DISABLED:
					cstate = "Inactive";
				case ISM_CLUSTER_RS_STATE_ACTIVE:
				case ISM_CLUSTER_RS_STATE_CONNECTING:
				case ISM_CLUSTER_RS_STATE_INACTIVE:
					TRACE(5, "%s: Remote server status are ignored during local status call.\n", __FUNCTION__);
				}
				sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Name\": \"%s\",\n    \"Enabled\": %s,\n    \"ConnectedServers\": %d,\n    \"DisconnectedServers\": %d\n  }",
						cstate, stats.pClusterName ? stats.pClusterName : "NULL",
						cmEnabled?"true":"false", stats.connectedServers, stats.disconnectedServers);
				ism_json_putBytes(&outbuf, rbuf);
			} else {
				char *status = NULL;
				switch (statsRC) {
				case ISMRC_ClusterDisabled:
					status = "Inactive";
					break;
				case ISMRC_ClusterNotAvailable:
					status = "Unavailable";
					break;
				case ISMRC_NotImplemented:
					status = "NotImplemented";
					break;
				}
				if (status) {
					sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Enabled\": %s\n  }", status, cmEnabled?"true":"false");
					ism_json_putBytes(&outbuf, rbuf);
				} else {
					TRACE(2, "%s: Unexpected return code from ism_cluster_getStatistics: rc=%d\n", __FUNCTION__, statsRC);
				}
			}
		}
	}

	if (!service || strcmp(service, "Plugin") == 0) {
		int plEnabled = ism_admin_isPluginEnabled();
		ism_json_putBytes(&outbuf, ",\n  \"Plugin\": {\n");
		int plstat = ism_admin_getPluginStatus();
		sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Enabled\": %s\n  }", plstat?"Active":"Inactive", plEnabled?"true":"false");
		ism_json_putBytes(&outbuf, rbuf);
	}

	if (!service || strcmp(service, "MQConnectivity") == 0) {
		int mqEnabled = ism_admin_isMQConnectivityEnabled();
		ism_json_putBytes(&outbuf, ",\n  \"MQConnectivity\": {\n");
		char *status = (ism_admin_getMQCStatus()) ? "Active" : "Inactive";
		sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Enabled\": %s\n  }", status, mqEnabled?"true":"false");
		ism_json_putBytes(&outbuf, rbuf);
	}

	if (!service || strcmp(service, "SNMP") == 0) {
		int snmpEnabled = ism_admin_isSNMPEnabled();
		ism_json_putBytes(&outbuf, ",\n  \"SNMP\": {\n");
		if (!snmpRunning_f)
			snmpRunning_f = (ism_snmpRunning_f)(uintptr_t) ism_common_getLongConfig("_ism_snmp_running_fnptr", 0L);
		char *status = (snmpRunning_f()) ? "Active" : "Inactive";
		sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Enabled\": %s\n  }", status, snmpEnabled?"true":"false");
		ism_json_putBytes(&outbuf, rbuf);
	}

	ism_json_putBytes(&outbuf, "\n}\n");

	http->outbuf.used = 0;
    ism_common_allocBufferCopyLen(&http->outbuf, outbuf.buf, outbuf.used);

	GETSTATUS_END:
	if (rc) {
		ism_confg_rest_createErrMsg(http, rc, repl, replSize);
	}
	TRACE(9, "%s: Exit with rc: %d\n", __FUNCTION__, rc);
	return rc;

}

int ism_admin_restartServer( int serverflag) {
	int rc = ISMRC_OK;

	if (serverflag != 1) {
		rc = ISMRC_ArgNotValid;
		ism_common_setErrorData(rc, "%d", serverflag);
		goto RESTART_END;
	}
	// set restart flag
	pthread_mutex_lock(&adminFileLock);
	FILE *dest = NULL;
	dest = fopen("/tmp/.restart_inited", "w");
	if( dest == NULL ) {
		TRACE(1,"Can not open /tmp/.restart_inited file: errno=%d\n", errno);
		rc = ISMRC_Error;
	} else {
		fprintf(dest, "Restart has been initialed");
		fclose(dest);
	}
	dest = NULL;
	dest = fopen("/tmp/imaserver.stop", "w");
	if( dest == NULL ) {
		TRACE(2,"Can not open /tmp/imaserver.stop file: errno=%d\n", errno);
		rc = ISMRC_Error;
	} else {
		fprintf(dest, "STOP");
		fclose(dest);
	}
	pthread_mutex_unlock(&adminFileLock);

	if (rc) {
		ism_common_setError(rc);
		goto RESTART_END;
	}

	restartServer = 1;
	TRACE(2, "%s: imaserver will be restarted.\n", __FUNCTION__);
	rc = ism_admin_send_stop(ISM_ADMIN_STATE_STOP);

	RESTART_END:
	TRACE(5, "%s: restart service %d has return code: %d\n", __FUNCTION__, serverflag, rc);
	return rc;
}

int ism_admin_cleanStore(void) {
	int rc = ISMRC_OK;

	// set restart flag
	pthread_mutex_lock(&adminFileLock);
	FILE *dest = NULL;
	dest = fopen("/tmp/.restart_inited", "w");
	if( dest == NULL ) {
		TRACE(2,"Can not open /tmp/.restart_inited file: errno=%d\n", errno);
	} else {
		fprintf(dest, "Clean store has been initialed");
		fclose(dest);
	}
	dest = NULL;
	dest = fopen("/tmp/imaserver.stop", "w");
	if( dest == NULL ) {
		TRACE(2,"Can not open /tmp/imaserver.stop file: errno=%d\n", errno);
		rc = ISMRC_Error;
	} else {
		fprintf(dest, "STOP");
		fclose(dest);
	}
	pthread_mutex_unlock(&adminFileLock);
	if (rc) {
		ism_common_setError(rc);
		return rc;
	}

	/* set clean store and restart server */
	__sync_val_compare_and_swap( &cleanStore, cleanStore, 1);
	restartServer = 1;

	TRACE(2, "%s: The cleanStore variable is set. The store will be cleaned and imaserver will be restarted.\n", __FUNCTION__);
	ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
	return rc;
}

int ism_admin_maintenance(int flag, ism_http_t *http) {
	int rc = ISMRC_OK;

	TRACE(5, "%s: %s server maintenance.\n", __FUNCTION__, flag?"Start":"Stop");

    /* set AdminMode if isMaintenance is 1 */
    if ( flag == 1 ) {
    	ism_config_json_setAdminMode(1);
    } else if ( flag == 0 ) {
    	ism_config_json_setAdminMode(0);
    }

	TRACE(1, "%s: The maintenance mode has been changed. The imaserver will be restarted.\n", __FUNCTION__);
	int mode = 1; /* restart mode */
	rc = ism_admin_init_stop(mode, http);

	TRACE(5, "%s: flag:%s RC=%d\n", __FUNCTION__, flag?"stop":"start", rc);
	return rc;
}

/*
 *
 * restart services from RESTAPI service call payload.
 *
 * @param  http            http object pointer
 * @param  isService       imaserver is 1, mqconnectivity is 2, -1 is invalid
 * @param  isMaintenance   0 means stop maintenance mode, 1 mean enable maintenance
 * @param  isCleanStore    1 means do cleanstore
 * @param  proctype        Process type
 *
 * Returns ISMRC_OK on success, ISMRC_* on error
 *
 * */
XAPI int ism_admin_restartService(ism_http_t *http, int isService, int isMaintenance, int isCleanstore, int proctype) {
	int rc = ISMRC_OK;

	if (isCleanstore && proctype == ISM_PROTYPE_SERVER) {
		int mode = ISM_ADMIN_MODE_CLEANSTORE; /* clean store mode */
		rc = ism_admin_init_stop(mode, http);
	} else if (isMaintenance != -1 && proctype == ISM_PROTYPE_SERVER) {
		rc = ism_admin_maintenance(isMaintenance, http);
	} else if (isService) {
		if ( proctype == ISM_PROTYPE_SERVER ) {
			int mode = ISM_ADMIN_MODE_RESTART; /* restart mode */
			rc = ism_admin_init_stop(mode, http);
		} else if ( proctype == ISM_PROTYPE_PLUGIN ) {
			rc = ism_admin_startPlugin();
		} else if ( proctype == ISM_PROTYPE_SNMP) {
			rc = ism_admin_restartSNMP();
		} else if ( proctype == ISM_PROTYPE_MQC ) {
			rc = ism_admin_stop_mqc_channel();
			rc = ism_admin_start_mqc_channel();
		} else {
			rc =  ISMRC_ArgNotValid;
			ism_common_setErrorData(rc, "%s", "parameters");
		}
	} else {
		rc =  ISMRC_ArgNotValid;
		ism_common_setErrorData(rc, "%s", "parameters");
	}

	return rc;
}


XAPI int ism_process_adminMQCRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int *rc) {
	int rc1 = ISMRC_OK;

	if ( inpbuf && inpbuflen > 0 ) {
		rc1 = ism_config_json_processMQCRequest(inpbuf, inpbuflen, locale, output_buffer, *rc);
	}

	if ( rc1 == -1 ) {
	    /* MQConnectivity monitoring data - output_buffer is already set */
	    *rc = 0;
	    return 0;
	}

    const char * repl[1];
    int replSize = 0;
    if (rc1)
        ism_confg_rest_createMQCErrMsg(output_buffer, locale, rc1, repl, replSize);
    else
        ism_confg_rest_createMQCErrMsg(output_buffer, locale, 6011, repl, replSize);

    if ( rc1 != ISMRC_OK )
        *rc = rc1;
    else
        *rc = 0;

    return 0;
}


XAPI int ism_process_adminSNMPRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int *rc) {
	ism_common_allocBufferCopy(output_buffer,"{\"rc\": 0, \"reply\": \"OK\"}");
	*rc = 0;
	return 0;
}


/**
 * Check if configuration updates are allowed on the node
 *
 * If the Server is in StandBy, Only Allow server, HA and Cluster component
 *
 * Returns 1 if allowed.
 *
 */
XAPI int ism_admin_nodeUpdateAllowed(int *rc, ism_ConfigComponentType_t compType, char *object) {
	*rc = ISMRC_OK;

	if (serverProcType == ISM_PROTYPE_SERVER) {
		int haRoleRC;
		ismHA_Role_t role = ism_admin_getHArole(NULL, &haRoleRC);
		if (role == ISM_HA_ROLE_STANDBY) {
			if (compType != ISM_CONFIG_COMP_HA && compType != ISM_CONFIG_COMP_SERVER && compType != ISM_CONFIG_COMP_CLUSTER) {

				/*Set Error*/
				ism_common_setErrorData(6121, "%s%s", "Standby", object);
				LOG(ERROR, Admin, 6121, "%s%s", "The " IMA_SVR_COMPONENT_NAME " status is \"{0}\". Any actions to \"{1}\" are not allowed.",
						"Standby", object);
				*rc = 6121;
				return 0;
			}
		}
	}

	return 1;
}

static void getDockerContainerUUID(char **buf, size_t *len) {
    FILE * in;
    ssize_t actualLen;

    if (buf == NULL || len == NULL) {
    	    return;
    }

    /* in = popen("cat /proc/self/mountinfo | grep containers | grep hostname | cut -d'/' -f6", "r"); */
    in = popen(IMA_SVR_INSTALL_PATH "/bin/container_uuid.sh", "r");
    if (!in) {
    	    *buf = NULL;
    	    *len = 0;
    	    return;
    }

    actualLen = getline(buf, len, in);
    if (actualLen < 0) {
    	    TRACE(5, "Failed to read container UUID.\n");
    	    *buf = NULL;
    	    *len = 0;
    } else {
        	(*buf)[actualLen - 1] = 0;
    }

    pclose(in);
}

/* Timer task to Restart imaserver */
int ism_admin_restartTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    int rc = ISMRC_OK;
    TRACE(1, "Server is Restarted by Admin action.\n");
    adminRestartTimerData_t *rdata = (adminRestartTimerData_t *)userdata;
    int isService = rdata->isService;
    int isMaintenance = rdata->isMaintenance;
    int isCleanstore = rdata->isCleanstore;

    rc = ism_admin_restartService(NULL, isService, isMaintenance, isCleanstore, ISM_PROTYPE_SERVER);

    return rc;
}

/**
 * Restart server
 */
XAPI int ism_admin_initRestart(int delay) {
    TRACE(1, "Set Timer Task to Restart the server.\n");
    adminRestartTimerData_t *userdata = (adminRestartTimerData_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,586),sizeof(adminRestartTimerData_t));
    userdata->isCleanstore = 0;
    userdata->isMaintenance = -1;
    userdata->isService = 1;
    ism_common_setTimerRate(ISM_TIMER_HIGH, (ism_attime_t) ism_admin_restartTimerTask, (void *)userdata, delay, 60, TS_SECONDS);
    return 0;
}

/**
 * Process stop snmp service request.
 */
XAPI int ism_admin_stopSNMP(void)
{
	if (ism_admin_isSNMPConfigured(0)) {
		if (!snmpStop_f)
			snmpStop_f = (ism_snmpStop_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_stop_fnptr", 0L);
		snmpStop_f();
		return ISMRC_OK;
	}
	else {
		ism_common_setError(ISMRC_SNMPNotConfigured);
		return ISMRC_SNMPNotConfigured;
	}
}

/**
 * Process start snmp request.
 */
XAPI int ism_admin_startSNMP(void)
{
	if (ism_admin_isSNMPConfigured(0)) {
		if (!snmpStart_f)
			snmpStart_f = (ism_snmpStart_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_start_fnptr", 0L);
		return snmpStart_f();
	}
	else {
		ism_common_setError(ISMRC_SNMPNotConfigured);
		return ISMRC_SNMPNotConfigured;
	}
}

/**
 * Process restart snmp request.
 */
XAPI int ism_admin_restartSNMP(void)
{
	if (ism_admin_isSNMPConfigured(0)) {
		if (!snmpStop_f)
			snmpStop_f = (ism_snmpStop_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_stop_fnptr", 0L);
		snmpStop_f();
		sleep(1);
		if (!snmpStart_f)
			snmpStart_f = (ism_snmpStart_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_start_fnptr", 0L);
		return snmpStart_f();
	}
	else {
		ism_common_setError(ISMRC_SNMPNotConfigured);
		return ISMRC_SNMPNotConfigured;
	}
}

/*
 * Returns 1 if cluster is either not configured or disabled
 */
XAPI int ism_admin_isClusterDisabled(void) {

	int cmEnabled = ism_admin_isClusterConfigured();
	getStatistics_f getStats = (getStatistics_f)(uintptr_t)ism_common_getLongConfig("_ism_cluster_getStatistics_fnptr", 0L);

	if ( cmEnabled == 0 && !getStats ) return 1;

	int statValue = 0;

	if (getStats) {
		ismCluster_Statistics_t stats;
		int statsRC = getStats(&stats);
		if (!statsRC) {
			switch (stats.state) {
			case ISM_CLUSTER_LS_STATE_DISABLED:
				statValue = 1;
				break;
			default:
				break;
			}
		} else {
			switch (statsRC) {
			case ISMRC_ClusterDisabled:
			case ISMRC_NotImplemented:
				statValue = 1;
				break;
			default:
				break;
			}
		}
	}

	if ( statValue == 1 ) return 1;

	return 0;
}

/**
 * Returns licenseIsAccepted flag value
 */
XAPI int ism_admin_checkLicenseIsAccepted(void) {
	return licenseIsAccepted;
}


/* Internal call for cunit tests to accept license */
XAPI void ism_admin_cunit_acceptLicense(void) {
	licenseIsAccepted = 1;
}

/*
 * Internal call to dump server status in a file if server is running in a docker container.
 * Needed for Kubernetes, to check the status of imaserver from LivenessProbes and ReadinessProbes.
 *
 * This function will dump the following Status information (only in english) in 
 * a file named "imaserver.status" in config directory:
 *
 * {
 *   "Server": {
 *     "Name": "fa797958597e:9089",
 *     "ContainerUID": "fa797958597e02f57ad38f2cb78eaadf3240a873eff858fda18410862c1622cd",
 *     "StateDescription": "Running (production)",
 *     "ServerTime": "2018-08-02T13:48:04.959Z",
 *     "Version": "2.0.0.2 20180618-2257",
 *     "ErrorCode": 0,
 *     "ErrorMessage": ""
 *   },
 *   "HighAvailability": {
 *     "Status": "Active",
 *     "Enabled": true,
 *     "Group": "HAGROUP",
 *     "NewRole": "PRIMARY",
 *     "OldRole": "PRIMARY"
 *   }
 * }
 *
 */
XAPI int ism_admin_dumpStatus(void) {
    int rc = ISMRC_OK;
	char rbuf[2048];
    char tbuf[2048];
    concat_alloc_t outbuf = { tbuf, sizeof(tbuf), 0, 0 };

    if ( container_uuid == NULL ) return ISMRC_NotImplemented;

    /* Get current server time */
	char serverTime[32];
	ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
	ism_common_setTimestamp(ts, ism_common_currentTimeNanos());
	ism_common_formatTimestamp(ts, serverTime, sizeof(serverTime), 0, ISM_TFF_ISO8601);
	ism_common_closeTimestamp(ts);

	/* Get server name and state information */
	char statusStr[128];
	int state = ism_admin_get_server_state();
	char *sName = ism_config_getServerName();
	int lastAdminModeRC = ism_config_json_getAdminModeRC();
	if ( adminInitError != ISMRC_OK  || lastAdminModeRC == ISMRC_LicenseError ) {
		state = ISM_SERVER_MAINTENANCE;
	}
	int internalErrorCode = ism_admin_getInternalErrorCode();
    if ( internalErrorCode == 0 && adminInitError == ISMRC_RestartNeeded && backupApplied == 1 ) {
	    	internalErrorCode = ISMRC_RestartNeeded;
	}
    sprintf(statusStr, "%s", ism_admin_getStateStr(state));

    /* Get current error code and message */
    int errorCode = 0;
    	if ( adminInitError == ISMRC_LicenseError ) {
        errorCode = ISMRC_LicenseError;
	} else if ( haRestartNeeded != ISMRC_OK ) {
        errorCode = ISMRC_RestartNeeded;
	} else if (adminModeRC != ISMRC_OK || internalErrorCode != ISMRC_OK ) {
		if ( internalErrorCode == 1100 ) {
		    internalErrorCode = ISMRC_Error;
		} else if ( internalErrorCode == 1001 ) {
		    internalErrorCode = ISMRC_AllocateError;
		}
        errorCode = internalErrorCode?internalErrorCode:adminModeRC;
	} else if ( lastAdminModeRC != ISMRC_OK ) {
        errorCode = lastAdminModeRC;
    }

    /* Add server status info in json object */


    ism_json_putBytes(&outbuf, "{\n  \"Server\": {\n");
	sprintf(rbuf, "    \"Name\": \"%s\",\n", sName?sName:"");
	ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	sprintf(rbuf, "    \"ContainerUID\": \"%s\",\n", container_uuid?container_uuid:"");
	ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	sprintf(rbuf, "    \"StateDescription\": \"%s\",\n", statusStr);
	ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	sprintf(rbuf, "    \"ServerTime\": \"%s\",\n", serverTime);
	ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	sprintf(rbuf, "    \"Version\": \"%s\",\n", serverVersion);
	ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

	if ( sName ) ism_common_free(ism_memory_admin_misc,sName);

	if ( errorCode == 0 ) {
		sprintf(rbuf, "    \"ErrorCode\": 0,\n");
	    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	    	sprintf(rbuf, "    \"ErrorMessage\": \"\",\n");
	    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	} else {
        char tmpbuf[2000];
        	sprintf(rbuf, "    \"ErrorCode\": %d,\n", errorCode);
	    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	    ism_admin_getAdminModeRCStr(tmpbuf, sizeof(tmpbuf), errorCode);
	    sprintf(rbuf, "    \"ErrorMessage\": \"%s\"\n", tmpbuf);
	    ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	}
	
	ism_json_putBytes(&outbuf, "  },\n  \"HighAvailability\": {\n");

	/* get HA config */
	int isHAEnabled = ism_admin_isHAEnabled();

    if ( isHAEnabled ) {
		/* invoke store ha API to get current mode */
		ismHA_View_t haView = {0};
		int confmode = ism_admin_getmode();
		char *haGroupName = ism_config_getStringObjectValue("HighAvailability", "haconfig", "Group", 1);

		ism_ha_store_get_view(&haView);

		if ((modeChangedPrev != - 1 && confmode == modeChangedPrev && confmode == ISM_MODE_MAINTENANCE) || (modeChangedPrev == -1 && confmode == ISM_MODE_MAINTENANCE) ) {
			sprintf(rbuf, "    \"Status\": \"Active\",\n    \"Enabled\": true,\n    \"Group\": \"%s\",\n    \"NewRole\": \"UNKNOWN\",\n    \"OldRole\": \"UNKNOWN\"\n",
			    haGroupName?haGroupName:"");
		} else {
			char *haStatusStr = "Active";
			char *nRole = ism_admin_get_harole_string(haView.NewRole);
			char *oRole = ism_admin_get_harole_string(haView.OldRole);

			if ( haView.NewRole == ISM_HA_ROLE_DISABLED ) haStatusStr = "Inactive";

			sprintf(rbuf, "    \"Status\": \"%s\",\n    \"Enabled\": true,\n    \"Group\": \"%s\",\n    \"NewRole\": \"%s\",\n    \"OldRole\": \"%s\"\n",
				haStatusStr, haGroupName?haGroupName:"", nRole, oRole);
		}

		if ( haGroupName ) ism_common_free(ism_memory_admin_misc,haGroupName);

		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));

	} else {
		strcpy(rbuf, "    \"Status\": \"Inactive\",\n    \"Enabled\": false\n");
		ism_common_allocBufferCopyLen(&outbuf, rbuf, strlen(rbuf));
	}

	ism_json_putBytes(&outbuf, "  }\n}\n");

	if ( getenv("CUNIT") != NULL ) {
		if (outbuf.inheap)
            ism_common_freeAllocBuffer(&outbuf);
		return rc;
	}

    /* get config lock, before creating status file */
    if ( outbuf.used > 0 ) {
        	char sfilepath[1024];
        	const char *configDir = ism_common_getStringProperty(ism_common_getConfigProperties(), "ConfigDir");
	    sprintf(sfilepath, "%s/imaserver.status", configDir?configDir:IMA_SVR_DATA_PATH "/data/config");
	    pthread_rwlock_wrlock(&srvConfiglock);
	    char *buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,589),outbuf.used + 1);
        memcpy(buffer, outbuf.buf, outbuf.used);
        buffer[outbuf.used] = 0;
        FILE *dest = NULL;
        dest = fopen(sfilepath, "w");
        if ( dest ) {
            fprintf(dest, "%s", buffer);
            fclose(dest);
        }
        ism_common_free(ism_memory_admin_misc,buffer);
        pthread_rwlock_unlock(&srvConfiglock);
	}

	/* free buffer */
	if (outbuf.inheap)
        ism_common_freeAllocBuffer(&outbuf);

    return rc;
}

/* Timer task to periodically dump server status in a file */
static int ism_admin_dumpStatusTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    ism_admin_dumpStatus();
    return 1;
}


/* setup a timer thread to write status on a file once in 30 seconds (configurable) - only if server is running in a container */
XAPI void ism_admin_startDumpStatusTimerTask(void) {
    if ( getenv("CUNIT") == NULL ) {

        if ( container_uuid == NULL ) return;

        if ( !enable_status_timer ) {
            int statusDumpInterval = ism_common_getIntProperty(ism_common_getConfigProperties(), "ServerStatusDumpInterval", 30);
            TRACE(9, "Admin: Start timer to dump server status in a file for kubernetes. Interval:%d\n", statusDumpInterval);
            enable_status_timer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) ism_admin_dumpStatusTimerTask, NULL, 15, statusDumpInterval, TS_SECONDS);
        }
    }
}
