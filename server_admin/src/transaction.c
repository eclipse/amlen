/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
 * This file contains ISM server transaction action APIs.
 */

#define TRACE_COMP Admin

#include "configInternal.h"
#include <engine.h>

/* pointers for Engine APIs */
typedef int (*engineForgetGlobalTransaction_f)(ism_xid_t *pXID, void *pContext, size_t contextLength, void *pCallbackFn);
static engineForgetGlobalTransaction_f engineForgetGlobalTransaction = NULL;
typedef int (*engineCompleteGlobalTransaction_f)(ism_xid_t *pXID, ismTransactionCompletionType_t completionType, void *pContext, size_t contextLength, void *pCallbackFn);
static engineCompleteGlobalTransaction_f engineCompleteGlobalTransaction = NULL;

int ism_config_json_parseServiceTransactionPayload(ism_http_t *http, int actionType, ism_rest_api_cb callback) {
    int rc = ISMRC_OK;
    json_t *post = NULL;

    if ( !http ) {
        rc = ISMRC_NullPointer;
        ism_common_setError(rc);
        goto PROCESSPOST_END;
    } else {
        TRACE(9, "Entry %s: http: %p\n", __FUNCTION__, http);
    }

    /* Create JSON object from HTTP POST Payload */
    int noErrorTrace = 0;
    post = ism_config_json_createObjectFromPayload(http, &rc, noErrorTrace);
    if ( !post || rc != ISMRC_OK) {
        goto PROCESSPOST_END;
    }

    json_t *objval = NULL;
    const char *objkey = NULL;
    const char *xidStr = NULL;

    json_object_foreach(post, objkey, objval) {
        int otype = json_typeof(objval);
    	if ( otype == JSON_STRING && !strcmp(objkey, "XID")) {
    		xidStr = json_string_value(objval);
    	} else {
    		rc = ISMRC_ArgNotValid;
    		ism_common_setErrorData(rc, "%s", objkey?objkey:"");
    		goto PROCESSPOST_END;
    	}
    }

    if (!xidStr || (xidStr && *xidStr == '\0')) {
        TRACE(3, "Invalid or NULL XID: XID=%s \n", xidStr?xidStr:"");
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", "XID", xidStr?xidStr:"");
        goto PROCESSPOST_END;
    }

    ism_xid_t xid;
    memset(&xid, 0, sizeof(ism_xid_t));
    rc = ism_common_StringToXid(xidStr, &xid);
    if (rc) {
        TRACE(3, "Invalid XID: XID=%s \n", xidStr?xidStr:"");
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", "XID", xidStr?xidStr:"");
        goto PROCESSPOST_END;
    }

    switch(actionType) {
    case ISM_ADMIN_ROLLBACK:
    {
        engineCompleteGlobalTransaction = (engineCompleteGlobalTransaction_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_completeGlobalTransaction", 0L);
        rc = (int) engineCompleteGlobalTransaction(&xid,
                ismTRANSACTION_COMPLETION_TYPE_ROLLBACK,
                NULL, 0, NULL);
        if (rc != ISMRC_OK && rc != ISMRC_AsyncCompletion)
            ism_common_setError(rc);
        break;
    }
    case ISM_ADMIN_COMMIT:
    {
        engineCompleteGlobalTransaction = (engineCompleteGlobalTransaction_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_completeGlobalTransaction", 0L);
        rc = engineCompleteGlobalTransaction(&xid,
                  ismTRANSACTION_COMPLETION_TYPE_COMMIT,
                  NULL, 0, NULL);
        if (rc != ISMRC_OK && rc != ISMRC_AsyncCompletion)
            ism_common_setError(rc);
        break;
    }
    case ISM_ADMIN_FORGET:
    {
        engineForgetGlobalTransaction = (engineForgetGlobalTransaction_f)(uintptr_t)ism_common_getLongConfig("_ism_engine_forgetGlobalTransaction", 0L);
        rc = engineForgetGlobalTransaction(&xid,
        		NULL, 0, NULL);
        if (rc != ISMRC_OK && rc != ISMRC_AsyncCompletion)
            ism_common_setError(rc);
        break;
    }
    default:
    {
        ism_common_setError(ISMRC_InvalidParameter);
        rc= ISMRC_InvalidParameter;
        break;
    }
    }

PROCESSPOST_END:

    if ( post )
        json_decref(post);

    return rc;
}

