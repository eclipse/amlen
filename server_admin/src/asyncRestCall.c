/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#ifndef TRACE_COMP
#define TRACE_COMP Admin
#endif

#include <adminInternal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <engine.h>


extern json_t * ism_config_json_createObjectFromPayload(ism_http_t *http, int *rc, int noErrorTrace);
extern void ism_confg_rest_createErrMsg(ism_http_t * http, int retcode, const char **repl, int replSize);
extern const char * ism_config_json_typeString(int type);

extern pthread_spinlock_t configSpinLock;

typedef int32_t (*ism_component_diagnostics_f)( const char *mode, const char *args, char ** out, void * pContext, size_t contextLength, void * pCallbackFn);
static ism_component_diagnostics_f engineDiagnostics_f = NULL;
static ism_component_diagnostics_f monitoringDiagnostics_f = NULL;

typedef void (*ism_component_freeDiagnosticsOutput_f)(char * diagnosticsOutput);
static ism_component_freeDiagnosticsOutput_f engineFreeDiagnosticsOutput_f = NULL;
static ism_component_freeDiagnosticsOutput_f monitoringFreeDiagnosticsOutput_f = NULL;


static ismHashMap * restRequestsMap = NULL;
static int restCorrelationID = 1;

typedef struct restRequest_t {
    ism_http_t *http;
    ism_rest_api_cb callback;
    ism_ConfigComponentType_t compType;
    const char *mode;
    const char *args;
} restRequest_t;


void ism_admin_term_async_restProcessor(void) {
    ism_common_HashMapLock(restRequestsMap);
    if(ism_common_getHashMapNumElements(restRequestsMap)) {
        int i = 0;
        ismHashMapEntry ** requests = ism_common_getHashMapEntriesArray(restRequestsMap);
        while(requests[i] != ((void*)-1)) {
            restRequest_t * request = (restRequest_t*) requests[i]->value;
            ism_common_setError(ISMRC_Error);
            ism_confg_rest_createErrMsg(request->http, ISMRC_Error, NULL, 0);
            request->callback(request->http, ISMRC_Error);
            ism_common_removeHashMapElement(restRequestsMap, requests[i]->key, sizeof(int));
            i++;
        }
        ism_common_freeHashMapEntriesArray(requests);

    }
    ism_common_HashMapUnlock(restRequestsMap);
}

static void ism_admin_async_freeRequest(restRequest_t * request) {
	if ( request ) {
		if ( request->args )
			ism_common_free(ism_memory_admin_misc,(void *)request->args);
		if ( request->mode )
			ism_common_free(ism_memory_admin_misc,(void *)request->mode);
		ism_common_free(ism_memory_admin_misc,(void *)request);
	}
}

/*
 * Initialize the Async REST call processing
 */
int ism_admin_init_async_restProcessor(void) {
    int  rc = ISMRC_OK;

    TRACE(4, "Initialize Asynchronous REST call processing option.\n");
    restRequestsMap = ism_common_createHashMap(32, HASH_INT32);

    engineDiagnostics_f = (ism_component_diagnostics_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_diagnostics_fnptr", 0L);
    engineFreeDiagnosticsOutput_f = (ism_component_freeDiagnosticsOutput_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_freeDiagnosticsOutput_fnptr", 0L);

    monitoringDiagnostics_f = (ism_component_diagnostics_f)(uintptr_t)ism_common_getLongConfig("_ism_monitoring_diagnostics_fnptr", 0L);
    monitoringFreeDiagnosticsOutput_f = (ism_component_freeDiagnosticsOutput_f)(uintptr_t)ism_common_getLongConfig("_ism_monitoring_freeDiagnosticsOutput_fnptr", 0L);

    return rc;
}

/* Receive response of the Async call invoked on the component */
int ism_admin_async_restResponse(int rc, void * buf, void * pContext) {
    int correlationID = *((int *)pContext);

    TRACE(7, "ism_admin_async_restResponse: context id: %d\n", correlationID);

    restRequest_t * request = (restRequest_t*)ism_common_removeHashMapElementLock(restRequestsMap, &correlationID, sizeof(int));

    if (request) {
        ism_http_t *http = request->http;
        http->outbuf.used = 0;
        memset(http->outbuf.buf, 0, http->outbuf.len);

        if ( rc != ISMRC_OK ) {
            /* need to update return message before invoking http callback */
            const char * repl[1];
            int replSize = 0;
            ism_confg_rest_createErrMsg(http, rc, repl, replSize);
        } else {
        	char *tmpbuf = (char *)buf;
        	if ( tmpbuf ) {
                ism_common_allocBufferCopyLen(&request->http->outbuf, tmpbuf, strlen(tmpbuf));
        	} else {
        		ism_common_allocBufferCopyLen(&request->http->outbuf, "{}", 2);
        	}

            /* free buffer */
            if ( request->compType == ISM_CONFIG_COMP_ENGINE ) {
            	if ( tmpbuf && engineFreeDiagnosticsOutput_f ) {
            	    engineFreeDiagnosticsOutput_f(tmpbuf);
            	}
            } else if ( request->compType == ISM_CONFIG_COMP_MONITORING ) {
            	if ( tmpbuf && monitoringFreeDiagnosticsOutput_f ) {
            		monitoringFreeDiagnosticsOutput_f(tmpbuf);
            	}
            }
        }


        request->callback(request->http, rc);
        ism_admin_async_freeRequest(request);
    } else {
    	rc = ISMRC_NotFound;
    	ism_common_setError(rc);
    }


    return rc;
}

/* Invoke component function */
static int ism_admin_async_restRequest(ism_http_t * http, ism_rest_api_cb callback, ism_ConfigComponentType_t compType, const char *mode, const char *args) {
    int rc = ISMRC_OK;
    char *resultString = NULL;
	int correlationID;

	if ( restRequestsMap == NULL ) ism_admin_init_async_restProcessor();

	restRequest_t * request = ism_common_malloc(ISM_MEM_PROBE(ism_memory_admin_misc,512),sizeof(restRequest_t));
	if(request == NULL) {
		return ISMRC_AllocateError;
	}
	request->http = http;
	request->callback = callback;
	request->compType = compType;
	if ( mode ) {
	    request->mode = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),mode);
	} else {
		request->mode = NULL;
	}
	if ( args ) {
	    request->args = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),args);
	} else {
		request->args = NULL;
	}

	ism_common_HashMapLock(restRequestsMap);
	correlationID = restCorrelationID++;
	ism_common_putHashMapElement(restRequestsMap, &correlationID, sizeof(int), request, NULL);
	ism_common_HashMapUnlock(restRequestsMap);

    /* invoke component call */
	switch ( compType ) {
	case ISM_CONFIG_COMP_ENGINE:
        if ( engineDiagnostics_f ) {
        	TRACE(7, "Invoke engine diagnostics function: correlationID=%d\n", correlationID);
            rc = engineDiagnostics_f(mode, args, &resultString, &correlationID, sizeof(int), (void *)ism_admin_async_restResponse);

            if ( rc == ISMRC_OK ) {
            	/* received results - send response to REST client */
            	TRACE(9, "Returned buffer:\n%s\n", resultString?resultString:"");
            	rc = ism_admin_async_restResponse(rc, resultString, &correlationID);
            } else if ( rc == ISMRC_AsyncCompletion ) {
            	/* Call is in async mode - response will be send by engine using callback */
            	rc = ISMRC_OK;
            }
        } else {
            rc = ISMRC_NotImplemented;
        }
		break;
	case ISM_CONFIG_COMP_MONITORING:
		if (monitoringDiagnostics_f) {
			TRACE(7,
					"Invoke monitoring diagnostics function: correlationID=%d\n",
					correlationID);
			rc = monitoringDiagnostics_f(mode, args, &resultString,
					&correlationID, sizeof(int),
					(void *) ism_admin_async_restResponse);

			if (rc == ISMRC_OK) {
				/* received results - send response to REST client */
				TRACE(9, "Returned buffer:\n%s\n",
						resultString ? resultString : "");
				rc = ism_admin_async_restResponse(rc, resultString,
						&correlationID);
			} else if (rc == ISMRC_AsyncCompletion) {
				/* Call is in async mode - response will be send by engine using callback */
				rc = ISMRC_OK;
			}
		} else {
			rc = ISMRC_NotImplemented;
		}
		break;

	default:
		rc = ISMRC_NotImplemented;
		break;
	}

	return rc;
}


/**
 * Process Diagnostics payload
 */
XAPI int ism_config_json_parseServiceDiagPayload(ism_http_t *http, char * component, ism_rest_api_cb callback) {
	int rc = ISMRC_OK;
    json_t *post = NULL;
    const char *mode = NULL;
    const char *arguments = NULL;
	const char * repl[5];
	int replSize = 0;

    if ( !http || !component ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto DIAGFUNC_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    int compType = ism_config_getCompType(component);
    if ( rc == ISM_CONFIG_COMP_LAST ) {
    	TRACE(3, "Invalid component %s\n", component);
    	rc = ISMRC_InvalidComponent;
    	ism_common_setError(rc);
    	goto DIAGFUNC_END;
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto DIAGFUNC_END;
    }

    json_t *objval = NULL;
    const char *objkey = NULL;
    json_object_foreach(post, objkey, objval) {
        int jtype = json_typeof(objval);
        if ( !strcmp(objkey, "Mode")) {
            if ( !objval || jtype != JSON_STRING ) {
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Mode", "null", "null", ism_config_json_typeString(jtype));
                goto DIAGFUNC_END;
            }
            mode = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(objval));
            if ( mode && *mode == '\0') {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", "Mode", mode?mode:"NULL");
                goto DIAGFUNC_END;
            }

        } else if ( !strcmp(objkey, "Arguments")) {
            if ( !objval || jtype != JSON_STRING ) {
    			rc = ISMRC_BadPropertyType;
    			ism_common_setErrorData(rc, "%s%s%s%s", "Arguments", "null", "null", ism_config_json_typeString(jtype));
                goto DIAGFUNC_END;
            }

            arguments = ism_common_strdup(ISM_MEM_PROBE(ism_memory_admin_misc,1000),json_string_value(objval));

        } else {
            /* Invalid data in payload */
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", objkey?objkey:"null");
            goto DIAGFUNC_END;
        }
    }

    rc = ism_admin_async_restRequest(http, callback, compType, mode, arguments);


DIAGFUNC_END:

    if ( post ) json_decref(post);
    if ( mode ) ism_common_free(ism_memory_admin_misc,(void *)mode);
    if ( arguments ) ism_common_free(ism_memory_admin_misc,(void *)arguments);

	if (callback) {
	    if (rc) {
	        ism_confg_rest_createErrMsg(http, rc, repl, replSize);
			callback(http, rc);
	    }
	}

    return rc;
}
