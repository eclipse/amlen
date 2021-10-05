/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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

/**
 * This sample application demonstrates basic usage
 * of the MQTT v3 SSL Client api.
 *
 * It can be run in one of two modes:
 *  - as a publisher, sending a single message to a topic on the server
 *  - as a subscriber, listening for messages from the server
 *
 */


#include <memory.h>
#include <MQTTClient.h>
#include <MQTTClientPersistence.h>
#include <signal.h>
#include <stdio.h>

#if defined(WIN32)
#include <Windows.h>
#define sleep Sleep
#else
#include <stdlib.h>
#include <sys/time.h>
#endif

//Default settings:
struct Options
{
	char* action;
	char* topic;
	char* message ;
	int qos;
	char* broker;
	char* port;
	char* clientKey;
	char* clientPass;
	char* serverKey;
	int enableServerCertAuth;
} options =
{
	"publish",
	NULL,
	"Message from MQTTv3 SSL C client",
	2,
	"localhost",
	"8883",
	NULL,
	NULL,
	NULL,
	0
};


int subscribe(char* brokerUrl, char* clientId, struct Options options);
int publish(char* brokerUrl, char* clientId, struct Options options);

volatile int toStop = 0;
volatile int quietMode = 0;

void printHelp(void)
{
	printf("Syntax:\n\n");
	printf("    MQTTV3SSample [-h] [-a publish|subscribe] [-t <topic>] [-m <message text>]\n");
	printf("         [-s 0|1|2] [-b <hostname|IP address>] [-p <brokerport>] \n\n");
	printf("    -h  Print this help text and quit\n");
	printf("    -q  Quiet mode (default is false)\n");
	printf("    -a  Perform the relevant action (default is publish)\n");
	printf("    -t  Publish/subscribe to <topic> instead of the default\n");
	printf("            (publish: \"MQTTV3SSample/C/v3\", subscribe: \"MQTTV3SSample/#\")\n");
	printf("    -m  Use this message instead of the default (\"Message from MQTTv3 SSL C client\")\n");
	printf("    -s  Use this QoS instead of the default (2)\n");
	printf("    -b  Use this name/IP address instead of the default (localhost)\n");
	printf("    -p  Use this port instead of the default (8883)\n");
	printf("    -k  Use this PEM format keyfile as the client certificate (default none)\n");
	printf("    -w  Passphrase to unlock client certificate\n");
	printf("    -r  Use this PEM format keyfile to verify the server\n");
	printf("    -v  Set server certificate verification; 0 - off (default), 1 - on\n");
	printf("\nDelimit strings containing spaces with \"\"\n");
	printf("\nPublishers transmit a single message then disconnect from the broker.\n");
	printf("Subscribers remain connected to the broker and receive appropriate messages\n");
	printf("until Control-C (^C) is received from the keyboard.\n\n");
}


void handleSignal(int sig)
{
	toStop = 1;
}


/**
 * The main entry point of the sample.
 *
 * This method handles parsing the arguments specified on the
 * command-line before performing the specified action.
 */
int main(int argc, char** argv)
{
	int rc = 0;
	char url[256];
	char clientId[24];

	int i=0;

	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);
	quietMode = 0;
	// Parse the arguments -
	for (i=1; i<argc; i++)
	{
		// Check this is a valid argument
		if (strlen(argv[i]) == 2 && argv[i][0] == '-')
		{
			char arg = argv[i][1];
			// Handle the no-value arguments
			if (arg == 'h' || arg == '?')
			{
				printHelp();
				return 255;
			}
			else if (arg == 'q')
			{
				quietMode = 1;
				continue;
			}

			// Validate there is a value associated with the argument
			if (i == argc - 1 || argv[i+1][0] == '-')
			{
				printf("Missing value for argument: %s\n", argv[i]);
				printHelp();
				return 255;
			}
			switch(arg)
			{
			case 'a': options.action = argv[++i];      break;
			case 't': options.topic = argv[++i];       break;
			case 'm': options.message = argv[++i];     break;
			case 's': options.qos = atoi(argv[++i]);   break;
			case 'b': options.broker = argv[++i];      break;
			case 'p': options.port = argv[++i];        break;
			case 'k': options.clientKey = argv[++i];   break;
			case 'w': options.clientPass = argv[++i];  break;
			case 'r': options.serverKey = argv[++i];   break;
			case 'v': options.enableServerCertAuth = atoi(argv[++i]);  break;
			default:
				printf("Unrecognised argument: %s\n", argv[i]);
				printHelp();
				return 255;
			}
		}
		else
		{
			printf("Unrecognised argument: %s\n", argv[i]);
			printHelp();
			return 255;
		}
	}


	// Validate the provided arguments
	if (strcmp(options.action, "publish") != 0 && strcmp(options.action, "subscribe") != 0)
	{
		printf("Invalid action: %s\n", options.action);
		printHelp();
		return 255;
	}
	if (options.qos < 0 || options.qos > 2)
	{
		printf("Invalid QoS: %d\n", options.qos);
		printHelp();
		return 255;
	}
	if (options.topic == NULL || ( options.topic != NULL && strlen(options.topic) == 0) )
	{
		// Set the default topic according to the specified action
		if (strcmp(options.action, "publish") == 0)
			options.topic = "MQTTV3SSample/C/v3";
		else
			options.topic = "MQTTV3SSample/#";
	}

	// Construct the full broker URL and clientId
	sprintf(url, "ssl://%s:%s", options.broker, options.port);
	sprintf(clientId, "SampleCV3S_%s", options.action);

	if (strcmp(options.action, "publish") == 0)
		rc = publish(url, clientId, options);
	else
		rc = subscribe(url, clientId, options);

	return rc;
}


/**
 * Subscribes to a topic and blocks until Ctrl-C is pressed,
 * or the connection is lost.
 *
 * The sample demonstrates synchronous subscription, which
 * does not use callbacks.
 */
int subscribe(char* brokerUrl, char* clientId, struct Options opt)
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
	int rc;

	// Create the client instance
	rc = MQTTClient_create(&client, brokerUrl, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to create MQTT Client instance\n"); fflush(stdout);
		return rc;
	}

	// Create the connection options
	conn_opts.keepAliveInterval = 20;
	conn_opts.reliable = 0;
	conn_opts.cleansession = 1;

	conn_opts.ssl = &ssl_opts;
	if (opt.serverKey != NULL) conn_opts.ssl->trustStore = opt.serverKey;
	if (opt.clientKey != NULL) conn_opts.ssl->keyStore = opt.clientKey;
	if (opt.clientPass != NULL) conn_opts.ssl->privateKeyPassword = opt.clientPass;
	conn_opts.ssl->enableServerCertAuth = opt.enableServerCertAuth;

	// Connect the client
	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc); fflush(stdout);
		MQTTClient_destroy(&client);
		return rc;
	}

	if (!quietMode){
		printf("Connected to %s\n",brokerUrl); fflush(stdout);
    }

	// Subscribe to the topic
	if (!quietMode){
		printf("Subscribing to topic \"%s\" qos %d\n", opt.topic, opt.qos); fflush(stdout);
    }

	rc = MQTTClient_subscribe(client, opt.topic, opt.qos);

	if (rc != MQTTCLIENT_SUCCESS){
		printf("Failed to subscribe, return code %d\n", rc); fflush(stdout);
    }else{
		long receiveTimeout = 100l;

		while (!toStop)
		{
			char* receivedTopicName = NULL;
			int topicLen;
			MQTTClient_message* message = NULL;

			// Check to see if a message is available
			rc = MQTTClient_receive(client, &receivedTopicName, &topicLen, &message, receiveTimeout);

			if (rc == MQTTCLIENT_SUCCESS && message != NULL)
			{
				// A message has been received
				printf("Topic:\t\t%s\n", receivedTopicName); fflush(stdout);
				printf("Message:\t%.*s\n", message->payloadlen, (char*)message->payload); fflush(stdout);
				printf("QoS:\t\t%d\n", message->qos); fflush(stdout);
				MQTTClient_freeMessage(&message);
				MQTTClient_free(receivedTopicName);
			}
			else if (rc != MQTTCLIENT_SUCCESS)
			{
				printf("Failed to received message, return code %d\n", rc); fflush(stdout);
				toStop = 1;
			}
		}
	}

	// Disconnect the client
	MQTTClient_disconnect(client, 0);

	if (!quietMode){
		printf("Disconnected\n"); fflush(stdout);
    }

	// Free the memory used by the client instance
	MQTTClient_destroy(&client);

	return rc;
}

/**
 * Performs a single publish
 */
int publish(char* brokerUrl, char* clientId, struct Options opt)
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
	MQTTClient_message mqttMessage = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	// Create the client instance
	rc = MQTTClient_create(&client, brokerUrl, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to create MQTT Client instance\n");
		return rc;
	}

	// Create the connection options
	conn_opts.keepAliveInterval = 20;
	conn_opts.reliable = 0;
	conn_opts.cleansession = 1;

	conn_opts.ssl = &ssl_opts;
	if (opt.serverKey != NULL) conn_opts.ssl->trustStore = opt.serverKey;
	if (opt.clientKey != NULL) conn_opts.ssl->keyStore = opt.clientKey;
	if (opt.clientPass != NULL) conn_opts.ssl->privateKeyPassword = opt.clientPass;
	conn_opts.ssl->enableServerCertAuth = opt.enableServerCertAuth;

	// Connect the client
	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
        fflush(stdout);
		MQTTClient_destroy(&client);
		return rc;
	}

	if (!quietMode){
		printf("Connected to %s\n",brokerUrl); fflush(stdout);
    }

	// Construct the message to publish
	mqttMessage.payload = opt.message;
	mqttMessage.payloadlen = strlen(opt.message);
	mqttMessage.qos = opt.qos;
	mqttMessage.retained = 0;

	// Publish the message
	if (!quietMode){
		printf("Publishing to topic \"%s\" qos %d\n", opt.topic, opt.qos); fflush(stdout);
    }

	rc = MQTTClient_publishMessage(client, opt.topic, &mqttMessage, &token);

	if ( rc != MQTTCLIENT_SUCCESS ){
		printf("Message not accepted for delivery, return code %d\n", rc);
        fflush(stdout);
    
	}else {
		// Wait until the message has been delivered to the server
		rc = MQTTClient_waitForCompletion(client, token, 10000L);

		if ( rc != MQTTCLIENT_SUCCESS ){
			printf("Failed to deliver message, return code %d\n",rc); fflush(stdout);
        }
	}

	// Disconnect the client
	MQTTClient_disconnect(client, 0);

	if (!quietMode){
		printf("Disconnected\n"); fflush(stdout);
    }

	// Free the memory used by the client instance
	MQTTClient_destroy(&client);

	return rc;
}
