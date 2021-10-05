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
 * of the MQTT v3 Asynchronous SSL Client api.
 *
 * It can be run in one of two modes:
 *  - as a publisher, sending a single message to a topic on the server
 *  - as a subscriber, listening for messages from the server
 *
 */


#include <memory.h>
#include <MQTTAsync.h>
#include <MQTTClientPersistence.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#if defined(WIN32)
#include <Windows.h>
#define sleep Sleep
#else
#include <stdlib.h>
#include <sys/time.h>
#endif

volatile int toStop = 0;
volatile int finished = 0;
volatile int quietMode = 0;
volatile MQTTAsync_token deliveredtoken;
static char clientId[24];
struct Options
{
	char* action;
	char* topic;
	char* message;
	int qos;
	char* broker;
	char* port;
	char* privateKey;
	char* keyStore;
	char* clientPass;
	char* serverKey;
	int enableServerCertAuth;
} options =
{
	"publish",
	NULL,
	"Message from MQTTv3 C asynchronous SSL client",
	2,
	"localhost",
	"8883",
	NULL,
	NULL,
	NULL,
	NULL,
	0
};


void printHelp(void)
{
	printf("Syntax:\n\n");
	printf("    MQTTV3AsyncSSample [-h] [-a publish|subscribe] [-t <topic>] [-m <message text>]\n");
	printf("         [-s 0|1|2] [-b <hostname|IP address>] [-p <brokerport>] \n\n");
	printf("    -h  Print this help text and quit\n");
	printf("    -q  Quiet mode (default is false)\n");
	printf("    -a  Perform the relevant action (default is publish)\n");
	printf("    -t  Publish/subscribe to <topic> instead of the default\n");
	printf("            (publish: \"MQTTV3AsyncSSample/C/v3\", subscribe: \"MQTTV3AsyncSSample/#\")\n");
	printf("    -m  Use this message instead of the default (\"Message from MQTTv3 C asynchronous SSL client\")\n");
	printf("    -s  Use this QoS instead of the default (2)\n");
	printf("    -b  Use this name/IP address instead of the default (localhost)\n");
	printf("    -p  Use this port instead of the default (8883)\n");
	printf("    -c  Use this PEM format keyfile as the client certificate private key (default none)\n");
	printf("    -k  Use this PEM format keyfile as the keystore  (default none)\n");
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

int messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    fflush(stdout);

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    fflush(stdout);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed\n");
	finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection Connected to the server\n"); fflush(stdout);
	finished = 1;
}


void onSend(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Message with token value %d delivery confirmed\n", response->token);

	opts.onSuccess = onDisconnect;
	opts.context = client;

	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(-1);
	}
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed\n");
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	printf("Successful connection Connected to the server\n"); fflush(stdout);

	if (strcmp(options.action, "publish") == 0)
	{
		opts.onSuccess = onSend;
		opts.context = client;

		pubmsg.payload = options.message;
		pubmsg.payloadlen = strlen(options.message);
		pubmsg.qos = options.qos;
		pubmsg.retained = 0;
		deliveredtoken = 0;

    	printf("Publishing to topic %s\n", options.topic); fflush(stdout); 
		if ((rc = MQTTAsync_sendMessage(client, options.topic, &pubmsg, &opts))
				!= MQTTASYNC_SUCCESS)
		{
			printf("Failed to start sendMessage, return code %d\n", rc);
			exit(-1);
		}
	}
	else
	{
		printf("Subscribing to topic %s\nfor client %s using QoS%d\n"
			"Press Ctrl+C to quit\n", options.topic, clientId, options.qos);
        fflush(stdout);
		opts.onSuccess = onSubscribe;
		opts.onFailure = onSubscribeFailure;
		opts.context = client;

		deliveredtoken = 0;

		if ((rc = MQTTAsync_subscribe(client, options.topic, options.qos, &opts))
				!= MQTTASYNC_SUCCESS)
		{
			printf("Failed to start subscribe, return code %d\n", rc);
			exit(-1);
		}
	}
}

void connectionLost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
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

	// Default settings:
	int i=0;

	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;

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
			case 'a': options.action = argv[++i]; break;
			case 't': options.topic = argv[++i]; break;
			case 'm': options.message = argv[++i]; break;
			case 's': options.qos = atoi(argv[++i]); break;
			case 'b': options.broker = argv[++i]; break;
			case 'p': options.port = argv[++i]; break;
			case 'k': options.keyStore = argv[++i]; break;
			case 'c': options.privateKey = argv[++i]; break;
			case 'w': options.clientPass = argv[++i]; break;
			case 'r': options.serverKey = argv[++i]; break;
			case 'v': options.enableServerCertAuth = atoi(argv[++i]); break;
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
			options.topic = "MQTTV3AsyncSSample/C/v3";
		else
			options.topic = "MQTTV3AsyncSSample/#";
	}

	// Construct the full broker URL and clientId
	sprintf(url, "ssl://%s:%s", options.broker, options.port);
	sprintf(clientId, "SampleCV3AS_%s", options.action);


	MQTTAsync_create(&client, url, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;

	conn_opts.ssl = &ssl_opts;
	if (options.serverKey != NULL)
		conn_opts.ssl->trustStore = options.serverKey;
	if (options.keyStore != NULL)
		conn_opts.ssl->keyStore  = options.keyStore;
	if (options.privateKey != NULL)
		conn_opts.ssl->privateKey = options.privateKey;
	if (options.clientPass != NULL)
		conn_opts.ssl->privateKeyPassword = options.clientPass;
	conn_opts.ssl->enableServerCertAuth = options.enableServerCertAuth;

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		goto exit;
	}

	while (!finished)
	{
#if defined(WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif
		if (toStop == 1)
		{
			MQTTAsync_disconnectOptions opts =
					MQTTAsync_disconnectOptions_initializer;

			opts.onSuccess = onDisconnect;
			opts.context = client;

			if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
			{
				printf("Failed to start disconnect, return code %d\n", rc);
				exit(-1);
			}
			toStop = 0;
		}
	}

exit:
	MQTTAsync_destroy(&client);
 	return rc;
}
