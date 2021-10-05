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
#define TRACE_COMP Kafka
#endif
#include <ismutil.h>
#include <ismjson.h>
#include <pxtransport.h>
#include <pxtcp.h>
#include <tenant.h>
#include <imacontent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pxkafka.h>


/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

int ism_kafka_authenticate(ism_transport_t * transport, int rc)
{

	if (rc == 0) {
		TRACE(5, "ism_kafka_authenticate: connect=%u broker=%s:%u host=%s\n",
				transport->index, transport->server_addr, transport->serverport,
				transport->client_host ? transport->client_host : "");
		ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
		if(pobj->auth_require){
			TRACE(5, "ism_kafka_authenticate: auth is required: connect=%u broker=%s:%u host=%s\n",
							transport->index, transport->server_addr, transport->serverport,
							transport->client_host ? transport->client_host : "");

			if(pobj->authenticator){
				pobj->authenticator(transport, (void *) pobj->authenticator_data);
			}else{
				TRACE(5, "ism_kafka_authenticate: auth failed. No authenticator: connect=%u broker=%s:%u host=%s\n",
											transport->index, transport->server_addr, transport->serverport,
											transport->client_host ? transport->client_host : "");
				pobj->authenticated(1, (void *)transport);
			}

		}else{
			//Auth is not required.
			TRACE(5, "ism_kafka_authenticate: authenticate is completed: connect=%u broker=%s:%u host=%s\n",
										transport->index, transport->server_addr, transport->serverport,
										transport->client_host ? transport->client_host : "");
			pobj->authenticated(0, (void *)transport);
		}

	} else {
		/* The invoker will call close right after this, so nothing to do here */
		TRACE(7, "ism_kafka_authenticate : kafka connecttion failed: connect=%u rc=%u\n", transport->index, rc);
	}
	return 0;
}



int ism_kafka_metadata_conn_authenticator_sasl_plain(ism_transport_t * transport, void * data)
{
	kafkaSASLAuth_t * saslAuth = (kafkaSASLAuth_t *) data;

	char xbuf [1024];
	concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
	concat_alloc_t * buf = &cbuf;

	//Construct the SASL Plain buffer
	bputchar(buf, 0); //authzid is NULL
	ism_common_allocBufferCopyLen(buf, saslAuth->authid, saslAuth->authid_len);  //authid
	bputchar(buf, 0);
	ism_common_allocBufferCopyLen(buf, saslAuth->password,  saslAuth->password_len);//authid password
	bputchar(buf, 0);
	transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);

	//Need to put in timer
	ism_common_sleep(100000);
	ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;

	TRACE(5, "ism_kafka_metadata_conn_authenticator_sasl_plain: authenticate is completed: connect=%u broker=%s:%u host=%s tcp_state=%d\n",
										transport->index, transport->server_addr, transport->serverport,
										transport->client_host ? transport->client_host : "",
										transport->state);
	pobj->authenticated(0, (void *)transport);


	return 0;

}
