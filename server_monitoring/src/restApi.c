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
 */

#define TRACE_COMP Monitoring

#include <monitoring.h>
#include <monitoringutil.h>
#include <perfstat.h>
#include <ismjson.h>
#include <security.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ismutil.h>
#include <transport.h>
#include <admin.h>
#include <config.h>
#include <security.h>

extern int ism_config_getRESTTraceDumpFlag(void);
extern void ism_confg_rest_createErrMsg(ism_http_t * http, int retcode, const char **repl, int replSize);

// return http method string
static char * getHTTPMethod(ism_http_t * http) {
    char *p;

    switch (http->http_op) {
    case HTTP_OP_GET:
        p = "GET"; break;
    case HTTP_OP_POST:
    	p = "POST"; break;
    case HTTP_OP_DELETE:
    	p = "DELETE"; break;
    case HTTP_OP_WS:
    	p = "WebSocket"; break;
    case HTTP_OP_PUT:
    	p = "PUT"; break;
    case HTTP_OP_HEAD:
    	p = "HEAD"; break;
	case HTTP_OP_OPTIONS:
		p = "OPTIONS"; break;
	default:
		p = "UNKNOWN"; break;
    }
    return p;
}

static int processAdminAction(ism_http_t * http, ism_rest_api_cb callback) {
	int retcode = ISMRC_OK;

	if (http->content_count > 0) {
		int rc = ISMRC_OK;
		char * content = http->content[0].content;
		int content_len = http->content[0].content_len;
		ism_process_admin_action(http->transport, content, content_len, &http->outbuf, &rc);
	} else {
	    TRACE(3, "No http content is set for AdminAction\n.");
	}

	if ( callback ) {
	    callback(http, retcode);
	}
	return retcode;
}

/*
 * Configuration HTTP/REST request.
 *
 * This method must respond to the http object.  This can be done either
 * synchronously or asynchronously.  The http object will continue to
 * exist until this response.
 *
 * @param  http        An HTTP object
 */
void ism_config_rest_request(ism_http_t * http, ism_rest_api_cb callback) {
    int retcode = ISMRC_OK;

    /* Clear any previous errors to ensure the last error is reported properly */
    ism_common_setError(0);

    if ( ism_config_getRESTTraceDumpFlag() == 1 ) {
        TRACE(1, "RESTDEBUG: op=%c path='%s' userpath='%s'\n", http->http_op, http->path, http->user_path?http->user_path:"");
        if ( http->content_count > 0 ) {
            TRACE(1, "RESTDEBUG: content:%s\n", http->content[0].content);
        }
    }

	/*
	 * http outbuf initialed with 8000k
	 */

    switch (http->http_op) {
    case HTTP_OP_GET:
    {
        /* check if authorized to View config */
        retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
            ismSEC_AUTH_ACTION_VIEW, ISM_CONFIG_COMP_SECURITY, NULL);
        if ( retcode != ISMRC_OK ) {
            retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
                ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
        }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MANAGE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MONITOR, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
        if ( retcode == ISMRC_OK )
    	    retcode = ism_config_restapi_getAction(http, callback);
    	break;
    }
    case HTTP_OP_POST:
    {
        /* check if authorized to Configure config */
        retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
            ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
        if ( retcode == ISMRC_OK )
    		retcode = ism_config_json_processPost(http, callback);
    	break;
    }
    case HTTP_OP_DELETE:
    {
        /* check if authorized to Configure config */
        retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
            ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
        if ( retcode == ISMRC_OK )
    	    retcode = ism_config_restapi_deleteAction(http, 0, callback);
    	break;
    }
    case HTTP_OP_WS:
    {
    	retcode = processAdminAction(http, callback);
        break;
    }
    case HTTP_OP_PUT:
    case HTTP_OP_HEAD:
    case HTTP_OP_OPTIONS:
    default:
    {
    	TRACE(3, "%s: The http method: %s is not supported\n", __FUNCTION__, getHTTPMethod(http));
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", getHTTPMethod(http));
		break;
    }
    }

    if ( retcode != ISMRC_OK ) {
        ism_confg_rest_createErrMsg(http, retcode, NULL, 0);
        if ( callback ) {
            callback(http, retcode);
        }
    }

    TRACE(7, "Exit %s: retcode %d\n", __FUNCTION__, retcode);
}


/*
 * Monitoring HTTP/REST request.
 *
 * This method must respond to the http object.  This can be done either
 * synchronously or asynchronously.  The http object will continue to
 * exist until this response.
 *
 * @param  http        An HTTP object
 */
void ism_monitoring_rest_request(ism_http_t * http, ism_rest_api_cb callback) {
    int retcode = ISMRC_OK;

    /* Clear any previous errors to ensure the last error is reported properly */
    ism_common_setError(0);

    /* Do not allow to get monitoring data till license is accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 ) {
        const char * repl[1];
        int replSize = 0;
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
        ism_confg_rest_createErrMsg(http, retcode, repl, replSize);
    	callback(http, retcode);
        TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, retcode);
        return;
    }


    switch (http->http_op) {
    case HTTP_OP_GET:
    {
        /* check if authorized to View config */
        retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
            ismSEC_AUTH_ACTION_VIEW, ISM_CONFIG_COMP_SECURITY, NULL);
        if ( retcode != ISMRC_OK ) {
            retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
                ismSEC_AUTH_ACTION_MONITOR, ISM_CONFIG_COMP_SECURITY, NULL);
        }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MANAGE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
        if ( retcode == ISMRC_OK )
        	retcode = ism_monitoring_restapi_stateQuery(http, callback);
    	break;
    }
    case HTTP_OP_POST:
    case HTTP_OP_DELETE:
    case HTTP_OP_WS:
    case HTTP_OP_PUT:
    case HTTP_OP_HEAD:
    case HTTP_OP_OPTIONS:
    default:
    {
    	TRACE(3, "%s: The http method: %s is not supported\n", __FUNCTION__, getHTTPMethod(http));
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", getHTTPMethod(http));
		ism_confg_rest_createErrMsg(http, retcode, NULL, 0);
		break;
    }
    }

    if(retcode)
    	    callback(http, retcode);

    TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, retcode);
}

/*
 * Implement a file request
 */
void ism_rest_file_request(ism_http_t * http, ism_rest_api_cb callback) {
    int  retcode = ISMRC_OK;

    /* Clear any previous errors to ensure the last error is reported properly */
    ism_common_setError(0);

    /* Do not allow to file option till license is accepted */
    if ( ism_admin_checkLicenseIsAccepted() == 0 ) {
    	retcode = ISMRC_LicenseError;
    	ism_common_setError(retcode);
    	ism_confg_rest_createErrMsg(http, retcode, NULL, 0);
    	callback(http, retcode);
        TRACE(7, "Exit %s: rc %d\n", __FUNCTION__, retcode);
        return;
    }

    switch (http->http_op) {
    case HTTP_OP_PUT:
    {
        /* check if authorized to View config */
	    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
			 ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MANAGE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
		if ( retcode == ISMRC_OK )
			 retcode = ism_config_restapi_fileUploadAction(http, callback);
		break;
    }
    case HTTP_OP_GET:
    case HTTP_OP_POST:
    case HTTP_OP_DELETE:
    case HTTP_OP_WS:
    case HTTP_OP_HEAD:
    case HTTP_OP_OPTIONS:
    default:
    {
    	TRACE(3, "%s: The http method: %s is not supported\n", __FUNCTION__, getHTTPMethod(http));
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", getHTTPMethod(http));
		ism_confg_rest_createErrMsg(http, retcode, NULL, 0);
		break;
    }
    }

    if(retcode)
    	callback(http, retcode);
}

/*
 * Implement a service request
 */
void ism_rest_service_request(ism_http_t * http, ism_rest_api_cb callback) {
    int  retcode = ISMRC_OK;

    /* Clear any previous errors to ensure the last error is reported properly */
    ism_common_setError(0);

    switch (http->http_op) {
    case HTTP_OP_GET:
    {
        /* check if authorized to View config */
        retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
            ismSEC_AUTH_ACTION_VIEW, ISM_CONFIG_COMP_SECURITY, NULL);
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MONITOR, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MANAGE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
	    if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
	    }
        if ( retcode == ISMRC_OK )
    	    retcode = ism_config_restapi_serviceGetStatus(http, callback);
    	break;
    }
    case HTTP_OP_POST:
    {
	   /* check if authorized to Configure config */
	   retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
		   ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
	   if ( retcode != ISMRC_OK ) {
		    retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
				 ismSEC_AUTH_ACTION_MANAGE, ISM_CONFIG_COMP_SECURITY, NULL);
	   }
	   if ( retcode == ISMRC_OK )
		   retcode = ism_config_restapi_servicePostAction(http, callback);
	 break;
    }
    case HTTP_OP_DELETE:
    {
	   /* check if authorized to Configure config */
	   retcode = ism_security_validate_policy(http->transport->security_context, ismSEC_AUTH_USER, NULL,
		   ismSEC_AUTH_ACTION_CONFIGURE, ISM_CONFIG_COMP_SECURITY, NULL);
	   if ( retcode == ISMRC_OK )
		retcode = ism_config_restapi_deleteAction(http, 1, callback);
	 break;
    }
    case HTTP_OP_WS:
    case HTTP_OP_PUT:
    case HTTP_OP_HEAD:
	case HTTP_OP_OPTIONS:
	default:
	{
    	TRACE(3, "%s: The http method: %s is not supported\n", __FUNCTION__, getHTTPMethod(http));
		retcode = ISMRC_BadRESTfulRequest;
		ism_common_setErrorData(retcode, "%s", getHTTPMethod(http));
		ism_confg_rest_createErrMsg(http, retcode, NULL, 0);
		break;
	}
    }

    if(retcode)
    	callback(http, retcode);
    return;
}
