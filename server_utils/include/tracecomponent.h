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

/*
 * Macro to create trace component.
 * This same description is used to create the name and index lookup tables
 */
#ifndef TRACECOMP_SPECIAL
#define TRACECOMP_(name, val) TRACECOMP_##name = val,
enum ismTraceComponents {
#endif

TRACECOMP_(System			,0) 	/* Default or System component */
TRACECOMP_(Admin            ,1)     /* Administration Component */
TRACECOMP_(Engine			,2) 	/* Engine Component */
TRACECOMP_(Http				,3) 	/* Http Component */
TRACECOMP_(Jms				,4) 	/* JMS Component */
TRACECOMP_(Log				,5) 	/* Logging Component */
TRACECOMP_(Mqtt				,6) 	/* MQTT Component */
TRACECOMP_(Protocol			,7) 	/* Protocol Component */
TRACECOMP_(Security			,8) 	/* Security Component */
TRACECOMP_(Store			,9) 	/* Store Component */
TRACECOMP_(Tcp				,10) 	/* TCP Component */
TRACECOMP_(Transport		,11)	/* Transport Component */
TRACECOMP_(Util				,12)	/* Util Component */
TRACECOMP_(MQConn           ,13)    /* MQ Bridge Component */
TRACECOMP_(Monitoring       ,14)    /* Monitoring Component */
TRACECOMP_(TLS              ,15)    /* SSL/TLS */
TRACECOMP_(SSL              ,15)    /* SSL/TLS (synonym of TLS) */
TRACECOMP_(Proxy            ,16)    /* Proxy */
TRACECOMP_(Plugin           ,17)    /* Plug-in */
TRACECOMP_(Cluster          ,18)    /* Cluster */
TRACECOMP_(Forwarder        ,19)    /* Cluster messaging channel */
TRACECOMP_(SpiderCast       ,20)    /* Cluster SpiderCast component */
TRACECOMP_(Kafka            ,21)    /* Kafka protocol */
TRACECOMP_(Mux              ,22)    /* Multiplex protocol */
TRACECOMP_(Sqs              ,23)    /* Sqs */
TRACECOMP_(Routing          ,24)    /* Routing Component */



#ifndef TRACECOMP_SPECIAL
};

/*
 * Total number of components
 */
#define TRACECOMP_MAX	32
/*
 * Default the component to system if the TRACE_COMP
 * is not defined.
 */
#ifndef TRACE_COMP
	#define TRACE_COMP System
#endif
#define TRACECOMP_STR(comp) TRACECOMP_##comp
#define TRACECOMP_XSTR(comp) TRACECOMP_STR(comp)
#endif
