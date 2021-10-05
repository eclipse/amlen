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

/**********************************************************************/
/* MQTT client request / response - MQTT client                       */
/*                                                                    */
/* Usage: mqttrequest <topic> <message> [nummessages]                 */
/*                                                                    */
/* Publishes <message> to <topic> [nummessages] times (default once)  */
/* Subscribes to <topic>/REPLY/ClientId and waits for response(s)     */
/* Messages contain REPLYTOTOPIC:<topic>/REPLY/ClientId               */
/* Intended to be used with mqresponse.c                              */
/**********************************************************************/

/**********************************************************************/
/* Compile Windows:                                                   */
/*   CL /Zi mqttrequest.c "C:\Program Files (x86)\IBM\WebSphere MQ\mqxr\SDK\clients\c\windows_ia32\mqttv3c.lib" -I "C:\Program Files (x86)\IBM\WebSphere MQ\mqxr\SDK\clients\c\include" */
/* Compile Linux:                                                     */
/*   gcc -I /opt/mqm/mqxr/SDK/clients/c/include -L /opt/mqm/mqxr/SDK/clients/c/linux_ia64 -lmqttv3c -pthread -Wl,-rpath /opt/mqm/mqxr/SDK/clients/c/linux_ia64 -o mqttrequest mqttrequest.c */
/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <pthread.h>
#endif

#ifdef _WIN32
  #define ADDRESS "tcp://mjcamp:16102"
#else
  #define ADDRESS "tcp://127.0.0.1:16102"
#endif

#ifdef _WIN32
  static HANDLE hEvent;
#else
  static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static int consumed = 0;
static int nummessages = 1;

int consumer(void * context, char * topic, int len, MQTTClient_message * message)
{
  printf("Reply:     %.*s\n", message->payloadlen, (char *)message->payload);

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);

  if (++consumed == nummessages)
  {
#ifdef _WIN32
    SetEvent(hEvent);
#else
    pthread_mutex_unlock(&Mutex);
#endif
  }

  return 1;
}

int main(int argc, char* argv[])
{
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  int rc, len;
  char * replytotopic, * message;
  time_t now;
  char clientid[24];
  int i;

  if (argc < 3)
  {
    printf("Usage: mqttrequest <topic> <message> [nummessages]\n");
    rc = 1;
    goto MOD_EXIT;
  }

  if (argc > 3)
  {
    nummessages = atoi(argv[argc-1]);
  }

#ifdef _WIN32
  hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
  pthread_mutex_lock(&Mutex);
#endif

  sprintf(clientid, "mqtt.%ld", time(&now));

  MQTTClient_create(&client, ADDRESS, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);

  MQTTClient_setCallbacks(client, NULL, NULL, consumer, NULL);

  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
     printf("Failed to connect, return code %d\n", rc);
     rc = 1;
     goto MOD_EXIT;
  }

  replytotopic = malloc(strlen(argv[1]) + strlen("/REPLY/") + strlen(clientid) + 1);
  sprintf(replytotopic, "%s/REPLY/%s", argv[1], clientid);

  printf("\nSubscribe: %s\n", replytotopic);
  rc = MQTTClient_subscribe(client, replytotopic, 0);

  len = strlen("REPLYTOTOPIC:") + strlen(replytotopic) + 1 + strlen(argv[2]);
  message = malloc(len);
  sprintf(message, "REPLYTOTOPIC:%s", replytotopic);
  memcpy(&message[strlen(message) + 1], argv[2], strlen(argv[2]));

  pubmsg.payload    = message;
  pubmsg.payloadlen = len;

  printf("Publish:   %s\n\n", argv[1]);
  for (i = 0; i < nummessages; i++)
  {
    MQTTClient_publishMessage(client, argv[1], &pubmsg, &token);
  }

#ifdef _WIN32
  WaitForSingleObject(hEvent, INFINITE);
#else
  pthread_mutex_lock(&Mutex);
#endif

  MQTTClient_disconnect(client, 10000);

  MQTTClient_destroy(&client);

MOD_EXIT:
  return rc;
}

