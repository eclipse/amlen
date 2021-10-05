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

/*012345678901234567890123456789012345678901234567890123456789012345*/

/* Publish test program for the ISM end of the MQ / ISM bridge. */

/* Command line parameters */
/* 1. Name of the configuration file (optional) */
/*    Defaults to ism_to_mq.cfg */

/* Read the ISM server details from the configuration file, then    */
/* read the list of topics and publish messages to them within the  */
/* ISM server. We expect the topics to be mapped to topics in an MQ */
/* queue manager by the MQ/ISM bridge and other test programs will  */
/* verify that the messages do eventually arrive at the other end.  */

#include <stdlib.h>
#include <string.h>
#include <ismPubTest.h>

#define TOPIC_NAME_LENGTH 1024
#define SERVER_ADDRESS_LENGTH 63
#define CLIENT_ID_LENGTH 24
#define TEN_SECONDS 10000

void readConfigFile(char* pConfigFileName,
                    imbt_config_parameter_t* pImbtConfigParameters,
                    imbt_publish_target_t** ppPublishTargetListHead)
{

   FILE *pConfigFile = NULL;
   imbt_publish_target_t* pCurrentPublishTarget = NULL;
   int lineCount = 0;

   /* Open the configuration file for read */
   printf("Attempting to open %s\n", pConfigFileName);

   pConfigFile = fopen(pConfigFileName, "r");

   if (pConfigFile == NULL)
   {
      printf("Unable to open configuration file %s\n", pConfigFileName);
      exit(EXIT_FAILURE);
   }

   /* Read the address of the ISM server */
   pImbtConfigParameters->serverAddressLength = SERVER_ADDRESS_LENGTH;
   pImbtConfigParameters->pServerAddress =
      (char *) malloc(pImbtConfigParameters->serverAddressLength + 1);
   pImbtConfigParameters->serverAddressSize =
      getline(&(pImbtConfigParameters->pServerAddress),
              (size_t *) &pImbtConfigParameters->serverAddressLength,
              pConfigFile);

   /* Strip off the trailing newline if there is one. */
   if (pImbtConfigParameters->pServerAddress[pImbtConfigParameters->serverAddressSize - 1] == '\n')
   {
      pImbtConfigParameters->pServerAddress[--(pImbtConfigParameters->serverAddressSize)] = 0;
   }

   printf("Server Address -> %s\n", pImbtConfigParameters->pServerAddress);

   /* Read the Client Id */
   pImbtConfigParameters->clientIdLength = CLIENT_ID_LENGTH;
   pImbtConfigParameters->pClientId =
      (char *) malloc(pImbtConfigParameters->clientIdLength + 1);
   pImbtConfigParameters->clientIdSize =
      getline(&(pImbtConfigParameters->pClientId),
              (size_t *) &pImbtConfigParameters->clientIdLength,
              pConfigFile);

   /* Strip off the trailing newline if there is one. */
   if (pImbtConfigParameters->pClientId[pImbtConfigParameters->clientIdSize - 1] == '\n')
   {
      pImbtConfigParameters->pClientId[--(pImbtConfigParameters->clientIdSize)] = 0;
   }

   printf("Client Id -> %s\n", pImbtConfigParameters->pClientId);

   /* Read the remaining lines in the file which are topics to publish to */
   do
   {
      pCurrentPublishTarget =
	 (imbt_publish_target_t*) malloc(sizeof(imbt_publish_target_t));

      pCurrentPublishTarget->topicNameLength = TOPIC_NAME_LENGTH;
      pCurrentPublishTarget->pTopicName =
	 (char *) malloc(pCurrentPublishTarget->topicNameLength + 1);
      pCurrentPublishTarget->topicNameSize =
	 getline(&(pCurrentPublishTarget->pTopicName),
		 (size_t *) &pCurrentPublishTarget->topicNameLength,
		 pConfigFile);

      if (pCurrentPublishTarget->topicNameSize == -1)
      {
	 if (feof(pConfigFile))
	 {
	    break;			/* EOF detected. */
	 }
	 else
	 {
	    printf("Error reading config file %s\n", pConfigFileName);
	    exit(EXIT_FAILURE);
	 }
      }

      /* Strip off the trailing newline if there is one. */
      if (pCurrentPublishTarget->pTopicName[pCurrentPublishTarget->topicNameSize - 1] == '\n')
      {
	 pCurrentPublishTarget->pTopicName[--(pCurrentPublishTarget->topicNameSize)] = 0;
      }
      
      lineCount++;
      printf("Topic Name %d: %s\n", lineCount, pCurrentPublishTarget->pTopicName);

      /* If the first character is a ! the treat the whole line as a */
      /* comment and just discard it. */
      if (pCurrentPublishTarget->pTopicName[0] == '!')
      {
	 free(pCurrentPublishTarget->pTopicName);
	 free(pCurrentPublishTarget);
         continue;
      }

      pCurrentPublishTarget->pNextPublishTarget = *ppPublishTargetListHead;
      *ppPublishTargetListHead = pCurrentPublishTarget;

   } while (1);

   printf("Closing %s\n", pConfigFileName);

   fclose(pConfigFile);

}

void connectionLost(void *context, char *cause)
{
   printf("\nConnection lost\n");
   printf("     cause: %s\n", cause);
}

void messageDelivered(void *context, MQTTClient_deliveryToken dt)
{
   printf("Message with token value %d delivery confirmed\n", dt);
}

int messageArrived(void *context, char *topicName, int topicLength, MQTTClient_message *message)
{
   int i;
   char* payloadptr;

   printf("Message arrived\n");
   printf("     topic: %s\n",topicName);
   printf("   message: ");
   payloadptr = message->payload;
   for(i=0; i<message->payloadlen; i++)
   {
      putchar(*payloadptr++);
   }
   putchar('\n');

   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);

   return 1;
}

#define QOS 2

#define INPUT_BUFFER_LENGTH 1024
#define MESSAGE_BUFFER_LENGTH 1024

int main(int argc, char* argv[])
{

   MQTTClient client;
   MQTTClient_connectOptions connectOptions =
      MQTTClient_connectOptions_initializer;
   MQTTClient_message publishMessage = MQTTClient_message_initializer;
   int rc = MQTTCLIENT_FAILURE;

   char ConfigurationFileName[] = "ism_to_mq.cfg";
   char *pConfigurationFile = NULL;
   imbt_config_parameter_t configurationParameters;

   imbt_publish_target_t *pCurrentPublishTarget = NULL;
   imbt_publish_target_t *pPublishTargetQueueHead = NULL;

   char buffer[MESSAGE_BUFFER_LENGTH];
   int bufferLength;

   printf("Start ISM / MQ bridge publication test, V0.1\n");

   if (argc > 1)
   {
      pConfigurationFile = argv[1];
   }
   else
   {
      pConfigurationFile = ConfigurationFileName;
   }
   printf("pConfigurationFile -> %s\n", pConfigurationFile);

   readConfigFile(pConfigurationFile,
		  &configurationParameters,
		  &pPublishTargetQueueHead);

   printf("Attempting to create MQTT client\n");

   rc = MQTTClient_create(&client,
                          configurationParameters.pServerAddress,
                          configurationParameters.pClientId,
                          MQTTCLIENT_PERSISTENCE_NONE,
                          NULL);
   if (rc != MQTTCLIENT_SUCCESS)
   {
      printf("MQTTClient_create failed. rc = %d (%X)\n", rc, rc);
      exit(EXIT_FAILURE);
   }

   rc = MQTTClient_setCallbacks(client,
                                (void *) pPublishTargetQueueHead,
                                connectionLost,
                                messageArrived,
                                messageDelivered);
   if (rc != MQTTCLIENT_SUCCESS)
   {
      printf("MQTTClient_setCallbacks failed. rc = %d (%X)\n", rc, rc);
      exit(EXIT_FAILURE);
   }

   rc = MQTTClient_connect(client, &connectOptions);
   if (rc != MQTTCLIENT_SUCCESS)
   {

      printf("\nDid you remember to start the ismserver?\n\n");

      printf("MQTTClient_connect failed. rc = %d (%X)\n", rc, rc);
      exit(EXIT_FAILURE);
   }


   /* Iterate through the publish target list, sending a message to */
   /* each topic.                                                   */
   pCurrentPublishTarget = pPublishTargetQueueHead;

   while (pCurrentPublishTarget != NULL)
   {
      sprintf(buffer, "Some text for %s", pCurrentPublishTarget->pTopicName);
      bufferLength = strlen(buffer);
      printf("Message Content: %s\n", buffer);

      publishMessage.payload = buffer;
      publishMessage.payloadlen = bufferLength;
      publishMessage.qos = QOS;
      publishMessage.retained = 0;

      rc = MQTTClient_publishMessage(client,
                                     pCurrentPublishTarget->pTopicName,
                                     &publishMessage,
                                     &pCurrentPublishTarget->deliveryToken);
      if (rc != MQTTCLIENT_SUCCESS)
      {
         printf("MQTTClient_publishMessage failed. rc = %d (%X)\n", rc, rc);
         exit(EXIT_FAILURE);
      }

      pCurrentPublishTarget = pCurrentPublishTarget->pNextPublishTarget;
   }

   /* Iterate through the publish target list, waiting for          */
   /* confirmation of delivery.                                     */
   pCurrentPublishTarget = pPublishTargetQueueHead;

   while (pCurrentPublishTarget != NULL)
   {
      rc = MQTTClient_waitForCompletion(client,
					pCurrentPublishTarget->deliveryToken,
					TEN_SECONDS);
      if (rc != MQTTCLIENT_SUCCESS)
      {
         printf("MQTTClient_waitForCompletion failed. rc = %d (%X)\n", rc, rc);
         exit(EXIT_FAILURE);
      }

      pCurrentPublishTarget = pCurrentPublishTarget->pNextPublishTarget;
   }

   MQTTClient_disconnect(client, TEN_SECONDS);

   MQTTClient_destroy(&client);

   printf("End of ISM / MQ bridge publication test, V0.1\n");

   return EXIT_SUCCESS;
}
