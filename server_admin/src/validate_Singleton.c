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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <janssonConfig.h>
#include "validateInternal.h"

extern int ism_config_isFileExist( char * filepath);
extern ismHA_Role_t ism_admin_getHArole(concat_alloc_t *output_buffer, int *rc);

extern char *adminUser;
extern char *adminUserPassword;

/* Validate JVM parameters */
static int validatePluginJvmParam(int maxHeapSize, const char * vmArgs) {
    char * argv[64] = {NULL};
    char * env[64] = {NULL};
    int index = 1;
    pid_t pid;
    int err = 0;
    int status;
    char logFile[256];
    argv[0] = IMA_SVR_INSTALL_PATH "/bin/installPlugin.sh";


    if(maxHeapSize) {
        char * data = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,3),32);
        sprintf(data,"%d",maxHeapSize);
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"-x");
        argv[index++] = data;
    }
    if(vmArgs && vmArgs[0]) {
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"-v");
        argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),vmArgs);
    }
    argv[index++] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),"-t");
    pid = vfork();
    if(!pid) {
        //Child process
        sprintf(logFile, IMA_SVR_DATA_PATH "/diag/logs/validatePluginVM.%d",getpid());
        int fd = open("output.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        execve(argv[0], argv, env);
        _exit(errno);
    }
    err = errno;
    for(--index; index > 0; --index) {
        if(argv[index])
            ism_common_free(ism_memory_admin_misc,argv[index]);
    }
    if(pid < 0) {
        ism_common_setErrorData(ISMRC_SysCallFailed, "%s%d%s", "vfork", err, strerror(err));
        return ISMRC_SysCallFailed;
    }
    waitpid(pid, &status, 0);
    err = WEXITSTATUS(status);
    if(WIFEXITED(status) && (err == 0)) {
        TRACE(5, "Plugin JVM parameters validated successfully\n");
        return ISMRC_OK;
    }
    if(err == 255) {
        char * buf;
        int length;
        sprintf(logFile, USERFILES_DIR "/validatePluginVM.%d.log",pid);
        if(ism_common_readFile(logFile, &buf, &length) == ISMRC_OK) {
            ism_common_setErrorData(ISMRC_PluginJvmError, "%s", buf);
            ism_common_free(ism_memory_admin_misc,buf);
        } else {
            ism_common_setErrorData(ISMRC_PluginJvmError, "%s", "Unknown");
        }
        return ISMRC_PluginJvmError;
    }
    ism_common_setErrorData(ISMRC_SysCallFailed, "%s%d%s", "execve", err, strerror(err));
    return ISMRC_SysCallFailed;
}

/*
 * Validate TraceLevel, and TraceSelected
 */
/*
 * Set extended trace level.
 */
static int ism_config_validate_traceLevel(char *name, const char * lvl) {
    char * lp;
    char * eos;
    char * token;
    char * comptoken;
    char * compvaluetoken;
    int    complevel;
    int    compID;
    int    rc = ISMRC_OK;
    int    level = 0;

    /* Null checking has been done already for both but check for empty string for TraceSelected */
    if (!strcmp(name, "TraceSelected") && (*lvl == '\0')) {
        return rc;
    }
    lp = alloca(strlen(lvl)+1);
    strcpy(lp, lvl);

    token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    if (!token) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", name, lvl);
        return rc;
    }

    level = strtoul(token, &eos, 0);
    if (*eos || (level<=0 || level>9)) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", name, lvl);
        return rc;
    }

    token = (char *)ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    while (token) {

        comptoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &compvaluetoken);
        compID = ism_common_getTraceComponentID(comptoken);

        if ( compID == -1 ) {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", name, lvl);
            break;
        }

        if (compvaluetoken && *compvaluetoken) {

            /* Check for tokens that accept non-integer values */
            complevel = strtoul(compvaluetoken, &eos, 0);
            if (*eos) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", name, lvl);
                break;
            }

            if (complevel<=0 || complevel>9) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", name, lvl);
                break;
            }
        } else {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", name, lvl);
            break;
        }
        token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    }
    return rc;
}




/*
 * Object: All Singleton Objects such as LogLevel, TraceLevel, ServerName, ....
 *
 * Schema:
 *
 *   {
 *       "<SingletonObjectName": "<Value>"
 *   }
 *
 * Where:
 *
 * <SingletonObjectName>: Singleton object name
 * <Value>: Value of the object. Type of value may bedifferent for different objects.
 *          For example, LogLevl is a string, whereas FIPS is a boolean
 *
 * Component callback(s):
 * - Depending of Object name, callback could be for Server, Store, and Transport
 *
 */

XAPI int32_t ism_config_validate_Singleton(json_t *currPostObj, json_t *objval, char * item)
{
    int32_t rc = ISMRC_OK;
    int compType = -1;
    int objType;

    TRACE(9, "Entry %s: currPostObj:%p, objval:%p, item:%s\n",
        __FUNCTION__, currPostObj?currPostObj:0, objval?objval:0, item?item:"null");

    if ( !objval || !item ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto VALIDATION_END;
    }

    /* Get list of required items from schema- do not free reqList, this is created at server start time and cached */
    ism_config_itemValidator_t *validator = ism_config_json_getSchemaValidator(ISM_CONFIG_SCHEMA, &compType, item, &rc );
    if (rc != ISMRC_OK) {
        goto VALIDATION_END;
    }

    /* Check if request has any data - delete is not allowed */
    objType = json_typeof(objval);
    if ( strcmp(item, "ServerName") && objType == JSON_NULL ) {
        if ( validator->defv[0] == NULL ) {
            ism_common_setError(ISMRC_SingltonDeleteError);
            rc = ISMRC_SingltonDeleteError;
            goto VALIDATION_END;
        }
    }

    if ( objType == JSON_OBJECT || objType == JSON_ARRAY  ) {
        /* Return error */
        char typeStr[32];
        sprintf(typeStr, "%s", ism_config_json_typeString(objType));
        ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "SingletonObject", item, item, typeStr);
        rc = ISMRC_BadPropertyType;
        goto VALIDATION_END;

    }

    /* Validate configuration items */
    if ( !strcmp(item, "TraceLevel") ||
         !strcmp(item, "TraceSelected") ||
         !strcmp(item, "TraceConnection") ||
         !strcmp(item, "TraceMax") ||
         !strcmp(item, "TraceOptions") ||
         !strcmp(item, "TraceBackupDestination") ||
 		 !strcmp(item, "TraceModuleLocation") ||
 		 !strcmp(item, "TraceModuleOptions"))
    {
    	if ( objType == JSON_NULL) goto VALIDATION_END;
    	if ( objType != JSON_STRING ) {
            /* Return error */
            char typeStr[32];
            sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
            ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
            rc = ISMRC_BadPropertyType;
            goto VALIDATION_END;
        }

        char *value = (char *)json_string_value(objval);

        /* validate trace related configuration items */
        rc = ismcli_validateTraceObject(0, item, value);
        if ( rc != ISMRC_OK ) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", item, value==NULL ? "NULL" : *value ? value : "\"\"");
        }

        if ( !strcmp(item, "TraceLevel") || !strcmp(item, "TraceSelected") ) {
        	rc = ism_config_validate_traceLevel(item, value);
        }

    } else if (!strcmp(item, "PluginMaxHeapSize")) {
        if(objType != JSON_NULL) {
            if ( objType != JSON_INTEGER ) {
                /* Return error */
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }
            int value = (int)json_integer_value(objval);
            if(value < 64) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%d", "PluginMaxHeapSize", value);
                rc = ISMRC_BadPropertyValue;
                goto VALIDATION_END;
            }
            if(value > 64*1024) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%d", "PluginMaxHeapSize", value);
                rc = ISMRC_BadPropertyValue;
                goto VALIDATION_END;
            }
            const char * vmArgs = ism_common_getStringConfig("PluginVMArgs");
            rc = validatePluginJvmParam(value, vmArgs);
            if ( rc != ISMRC_OK ) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%d", "PluginMaxHeapSize", value);
                goto VALIDATION_END;
            }
        }
    } else if (!strcmp(item, "PluginVMArgs")) {
        if(objType != JSON_NULL) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }
            char *value = (char *)json_string_value(objval);
            int maxHeapSize = ism_common_getIntConfig("PluginMaxHeapSize", 512);
            rc = validatePluginJvmParam(maxHeapSize, value);
            if ( rc != ISMRC_OK ) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "PluginVMArgs", ((value==NULL) ? "NULL" : value));
                goto VALIDATION_END;
            }
        }
    } else if (!strcmp(item, "PluginDebugServer")) {
        if(objType != JSON_NULL) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }

            char *value = (char *)json_string_value(objval);

            if(value && value[0]) {
                /* If not empty string - validate validate for a valid IP address */
                rc = ism_config_validate_IPAddress(value, 0);
                if ( rc != ISMRC_OK ) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "PluginDebugServer", ((value==NULL) ? "NULL" : value));
                    goto VALIDATION_END;
                }
            }
        }
    } else if (!strcmp(item, "ServerName")) {
        if (objType != JSON_NULL) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }

            char *value = (char *)json_string_value(objval);

            if (value && *value == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ServerName", ((value==NULL) ? "NULL" : value));
                goto VALIDATION_END;
            }

            /* Check valid UTF8 */
            int len = strlen(value);
            int count = ism_common_validUTF8(value, len);
            if (count < 1) {
                TRACE(3, "%s: Invalid UTF8 string\n", __FUNCTION__);
                rc = ISMRC_ObjectNotValid;
                ism_common_setError(rc);
                goto VALIDATION_END;
            }

            int maxlen = 1024;
       		if ( count > maxlen ) {
        		TRACE(3, "%s: String length check failed. len=%d maxlen=%d\n", __FUNCTION__, count, maxlen);
       		    rc = ISMRC_LenthLimitSingleton;
       		    ism_common_setErrorData(rc, "%s%s", "ServerName", value);
        		goto VALIDATION_END;
        	}
        }
    } else if (!strcmp(item, "PluginServer")) {
        if(objType != JSON_NULL) {
            if ( objType != JSON_STRING ) {
                /* Return error */
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
                goto VALIDATION_END;
            }

            char *value = (char *)json_string_value(objval);

            /* validate trace related configuration items */
            rc = ism_config_validateDataType_IPAddress(value);
            if ( rc != ISMRC_OK ) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "PluginServer", ((value==NULL) ? "NULL" : value));
                goto VALIDATION_END;
            }
        }
    } else if (!strcmp(item, "PreSharedKey")) {
    	if (objType != JSON_NULL) {
             if ( objType != JSON_STRING ) {
                 /* Return error */
                 char typeStr[32];
                 sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                 ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                 rc = ISMRC_BadPropertyType;
                 goto VALIDATION_END;
             }

             const char *pskFile = json_string_value(objval);

             if ( !pskFile || *pskFile == '\0' ) {
                 rc = ISMRC_BadPropertyValue;
                 ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "PreSharedKey", ((pskFile==NULL) ? "NULL" : pskFile));
                 goto VALIDATION_END;
             }

             int32_t errLine = 0, pskEntries = 0;
             const char *PSKdir = ism_common_getStringConfig("PreSharedKeyDir");
             const char *PSKName = "psk.csv";

             char DestPSKFile[strlen(PSKdir)+strlen(PSKName)+2];
             memset(&DestPSKFile, 0, sizeof(DestPSKFile));
             sprintf(DestPSKFile,"%s/%s", PSKdir, PSKName);

             char pskTmpFile[strlen(USERFILES_DIR)+strlen(pskFile)+1];
             memset(&pskTmpFile, 0, sizeof(pskTmpFile));
             sprintf(pskTmpFile,"%s%s", USERFILES_DIR, pskFile);

             /* Validate PSK file, File should have been uploaded to userfiles directory already. */
             if (!ism_config_isFileExist(pskTmpFile)) {
            	 TRACE(4, "The PreSharedKey File %s needs to be uploaded first", pskFile);
            	 rc = 6169;
            	 ism_common_setErrorData(6169, "%s", pskFile);
            	 goto VALIDATION_END;
             }
             rc = ism_ssl_validatePSKfile(pskTmpFile, &errLine, &pskEntries);
             if ( rc != ISMRC_OK ) {
            	 rc = ISMRC_InvalidPSKFile;
            	 TRACE(4,"The PreSharedKey file is not valid at line: %d\n.", errLine);
                 unlink(pskTmpFile);
                 goto VALIDATION_END;
             }
             /* Move it to the permanent PSK directory, remove from /tmp and apply the PSK file	*/
             copyFile(pskTmpFile, DestPSKFile);
             unlink(pskTmpFile);
             rc = ism_ssl_applyPSKfile(DestPSKFile, pskEntries);
             /* Sync file to standby in HA env, if PSK file is good */
             if ( rc == ISMRC_OK ) {
                 ism_admin_ha_syncFile(PSKdir, PSKName);
             }
        }
    } else {

        char *value = NULL;

        /* Convert json value to string */
        if ( objType == JSON_STRING ) {
            value = (char *)json_string_value(objval);
        	if (validator->type[0] == ISM_CONFIG_PROP_NUMBER
        			|| validator->type[0] == ISM_CONFIG_PROP_BOOLEAN) {
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
        		goto VALIDATION_END;
        	} else if ( (validator->type[0] == ISM_CONFIG_PROP_ENUM
        			|| validator->type[0] == ISM_CONFIG_PROP_LIST)
        			&& *value == '\0') {
        		rc = ISMRC_BadPropertyValue;
        		char *tmpvalue = alloca(strlen(value)+3);
        		sprintf(tmpvalue, "\"%s\"", value);
        		ism_common_setErrorData(rc, "%s%s", item, tmpvalue);
        		goto VALIDATION_END;
        	}
        } else if ( objType == JSON_INTEGER ) {
            value = alloca(16);
            ism_common_itoa(json_integer_value(objval), value);
        	if ( validator->type[0] != ISM_CONFIG_PROP_NUMBER ) {
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
        		goto VALIDATION_END;
        	}
        } else if ( objType == JSON_TRUE ) {
            value = "true";
        	if ( validator->type[0] != ISM_CONFIG_PROP_BOOLEAN ) {
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
        		goto VALIDATION_END;
        	}
        } else if ( objType == JSON_FALSE ) {
            value = "false";
         	if ( validator->type[0] != ISM_CONFIG_PROP_BOOLEAN ) {
                char typeStr[32];
                sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
                ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
                rc = ISMRC_BadPropertyType;
        		goto VALIDATION_END;
        	}
       } else if ( objType == JSON_NULL && validator->defv[0] != NULL ) {
        	value = validator->defv[0];
        } else {
            /* Return error */
            char typeStr[32];
            sprintf(typeStr, "InvalidType:%s", ism_config_json_typeString(objType));
            ism_common_setErrorData(ISMRC_BadPropertyType, "%s%s%s%s", "Singleton", item, item, typeStr);
            rc = ISMRC_BadPropertyType;
            goto VALIDATION_END;
        }

        if ( !value ) {
            TRACE(8, "Null value received for property: %s\n", item);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", item, "NULL");
            goto VALIDATION_END;
        }

        char *newValue = NULL;

        /* For empty string get the default value */
        rc = ism_config_validate_singletonItem( item, value, 1, &newValue);
        if (rc) {
            goto VALIDATION_END;
        }
        if (newValue) value = newValue;

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
    }


VALIDATION_END:

    TRACE(9, "Exit %s: rc %d\n", __FUNCTION__, rc);

    return rc;
}

