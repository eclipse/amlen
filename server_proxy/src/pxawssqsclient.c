/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
#define TRACE_COMP Sqs
#endif
#include <ismutil.h>
#include <string.h>
#include <stdlib.h>
#include <pxtransport.h>


const char * sqs_region=NULL;
int			 sqs_max_connections=0;
int 		 sqs_enabled = 0;
int			 sqsInited = 0;

/*AWS Gluecode APIs*/
typedef  void  (* aws_api_init_t)(const char * region, int maxConnections);
typedef  void  (* aws_api_shutdown_t)(void);
typedef  int  (* aws_sqs_sendMessage_t)(const char * queue_url, const char * message);
typedef  int  (* aws_sqs_sendMessageAsync_t)(const char * queue_url, const char * message);
aws_api_init_t    aws_api_init = NULL;
aws_api_shutdown_t    aws_api_shutdown = NULL;
aws_sqs_sendMessage_t    aws_sqs_sendMessage = NULL;
aws_sqs_sendMessageAsync_t    aws_sqs_sendMessageAsync = NULL;

/*Global Variables*/
static pthread_mutex_t    sqs_lock = PTHREAD_MUTEX_INITIALIZER;


/**
 * Initialize AWS Service.
 * This function loads the SQS Plugin Library and its functions
 * @param server server object
 */
void ism_proxy_sqs_init(ism_server_t * server)
{
	LIBTYPE  sqsclientmod;
	char  errbuf [512];
	pthread_mutex_lock(&sqs_lock);

	if(sqsInited){
		pthread_mutex_unlock(&sqs_lock);
		return;
	}

	sqs_enabled = ism_common_getIntConfig("SQSEnabled", 0);
	if(sqs_enabled){
		const char * sqs_plugin_lib = ism_common_getStringConfig("SQSPluginLibrary");
		sqs_region = ism_common_getStringConfig("SQSRegion");
		sqs_max_connections = ism_common_getIntConfig("SQSMaxConnections", 25);
		TRACE(5, "SQS Service is enabled.\n");
		sqsclientmod = dlopen(sqs_plugin_lib DLLOAD_OPT);/* Use the base symbolic link if it exists*/
		if (sqsclientmod) {
			aws_api_init =    (aws_api_init_t)    dlsym(sqsclientmod, "aws_api_init");
			aws_api_shutdown =    (aws_api_shutdown_t)    dlsym(sqsclientmod, "aws_api_shutdown");
			aws_sqs_sendMessage =    (aws_sqs_sendMessage_t)    dlsym(sqsclientmod, "aws_sqs_sendMessage");
			aws_sqs_sendMessageAsync =    (aws_sqs_sendMessage_t)    dlsym(sqsclientmod, "aws_sqs_sendMessageAsync");

			if (!aws_api_init || !aws_api_shutdown || !aws_sqs_sendMessage) {
				dlerror_string(errbuf, sizeof(errbuf));
				TRACE(5, "The AWS SQS Client symbols could not be loaded: %s", errbuf);
				sqs_enabled=0;
			}else{
				const char * access_key = ism_common_getStringConfig("SQSAccessKey");
				const char * access_secret = ism_common_getStringConfig("SQSAccessSecret");

				/*Set AWS Credentials if is there */
				if(access_key !=NULL && access_secret != NULL){
					setenv("AWS_ACCESS_KEY_ID", access_key, 1);
					setenv("AWS_SECRET_ACCESS_KEY", access_secret, 1);
				}

				aws_api_init(sqs_region, sqs_max_connections);
				TRACE(5, "The AWS SQS Client symbols are initialized. region=%s maxConnecitons=%d\n",
						sqs_region, sqs_max_connections);
				sqsInited=1;
			}
		} else {
			dlerror_string(errbuf, sizeof(errbuf));
			TRACE(2, "The AWS SQS Client library could not be loaded: %s", errbuf);
			sqs_enabled=0;
		}




	}else{
		TRACE(5, "SQS Service is disabled\n");
	}
	pthread_mutex_unlock(&sqs_lock);

}

/**
 * Send message to AWS SQS
 * @param transport transport object
 * @param message the payload
 * @param message_length the length of the payload
 */
void ism_proxy_sqs_send_message(ism_transport_t * transport, const char * message, int message_len)
{
	pthread_mutex_lock(&sqs_lock);
	transport->endpoint->stats->count[transport->tid].read_msg++;
	if(sqsInited){
		const char * url = transport->server->awssqs_url;
		char * msg = alloca(message_len+1);
		memcpy(msg, message, message_len);
		msg[message_len]='\0';

		int count = ism_common_validUTF8Restrict(msg, message_len,UR_NoNull | UR_NoC0Other | UR_NoNonchar);
		if(count<0){
			TRACE(7, "SQS: Invalid  message.\n");
			pthread_mutex_unlock(&sqs_lock);
			return;
		}
		//Publish Message to AWS SQS
		aws_sqs_sendMessageAsync(url, (const char *)msg);

		//Update Stats
		transport->endpoint->stats->count[transport->tid].write_msg++;
		transport->write_msg++;
		transport->write_bytes += message_len;
	}else{
		TRACE(7, "SQS service is disabled\n");
	}
	pthread_mutex_unlock(&sqs_lock);
}

/**
 * Terminate AWS Service
 */
void ism_proxy_sqs_term(void)
{
	pthread_mutex_lock(&sqs_lock);
	if(sqsInited){
		aws_api_shutdown();
		TRACE(5, "SQS Service is shutdown\n");
	}
	pthread_mutex_unlock(&sqs_lock);
}

