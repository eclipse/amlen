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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmqc.h>

#include <mqSubTest.h>

/* Test program for the MQ end of the MQ / ISM bridge. */

/*********************************************************************/
/* How do you run the test?                                          */
/* You need a running queue manager at a version that supports       */
/* pub/sub. This code has been exercised with V7.5 but probably      */
/* works with versions earlier than that. You also need an           */
/* application that will publish messages to the topics subscribed   */
/* by this program and something to catch the messages published by  */
/* this program. The expectation is that the "other" application     */
/* will be connected to an ISM server and from the point of view of  */
/* this code is on the far side of the MQ to ISM bridge.             */
/*********************************************************************/

/* There is minimal validation of user inputs and little error       */
/* handling. Inputs are assumed to be valid and errors are reported  */
/* as soon as they are detected and typically the application stops. */

/* Since this program uses the MQ callback mechanism, it _must_ be   */
/* built using the threaded version of the MQ library (libmqm_r). If */
/* not, the call to MQCTL to start receiving messages will return a  */
/* MQRC_OPERATION_ERROR, which appears to say that the MQOP_START    */
/* value is incorrect and thus isn't very helpful.                   */

/* Command line parameters */
/* 1. Queue manager name */
/* 2. Name of the configuration file */
/*    Defaults to mq_from_ism.cfg */

#define INPUT_BUFFER_SIZE 1024

int messageCount = 0;

/*********************************************************************/
/* Callback function, called when messages arrive for our            */
/* subscriptions.                                                    */
/*********************************************************************/
void messageConsumer(MQHCONN hConn,
                     MQMD *pMsgDesc,
                     MQGMO *pGetMsgOpts,
                     MQBYTE *Buffer,
                     MQCBC *pContext)
{

   MQLONG length;
   imbt_subscription *pCurrentSubscription =
      (imbt_subscription *) pContext->CallbackArea;

   switch(pContext->CallType)
   {

      case MQCBCT_MSG_REMOVED:
	  case MQCBCT_MSG_NOT_REMOVED:

		printf("Entering callback for %s\n",
				(char *) (pCurrentSubscription->sd.ObjectString.VSPtr));

		messageCount++;

		length = pGetMsgOpts -> ReturnedLength;
		if (pContext->Reason) {
			printf("Message Call (%d Bytes) : Reason = %d\n", length,
					pContext->Reason);
		} else {
			printf("Message Call (%d Bytes) :\n", length);
		}

		/* Display each message received                            */
		if (pContext->CompCode != MQCC_FAILED) {
			Buffer[length] = '\0';
			printf("Message: \n<%s>\n", Buffer);
		}
		break;
      default:
         printf("Calltype = %d\n", pContext->CallType);
         break;
   }
}

int main(int argc, char **argv)
{

   MQSD defaultSD = { MQSD_DEFAULT }; /* Subscription Descriptor */
   MQMD defaultMD = { MQMD_DEFAULT }; /* Message Descriptor */
   MQGMO defaultGMO = { MQGMO_DEFAULT }; /* get message options */
   MQCBD cbd = {MQCBD_DEFAULT};   /* Callback Descriptor */
   MQCTLO ctlo = {MQCTLO_DEFAULT}; /* Control options */

   MQHCONN hConn; /* Connection handle */
   MQLONG closeOptions; /* MQCLOSE options */
   MQLONG completionCode; /* Completion code */
   MQLONG subCompletionCode; /* MQSUB completion code */
   MQLONG reasonCode; /* Reason code */

   char ConfigurationFileName[] = "mq_from_ism.cfg";
   char *pQMName = NULL;
   char *pConfigurationFile = NULL;
   char *inString = NULL;
   char inputBuffer[INPUT_BUFFER_SIZE];
   FILE *cfgDataFile = NULL;

   int i = 0;
   int lineLength = 0;
   int lineCount = 0;

   imbt_subscription *subscriptionListHead = NULL;
   imbt_subscription *currentSubscription = NULL;
   char *topicString = NULL;

   printf("Start MQ / ISM bridge test, MQ client V0.1\n");

   printf("argc = %d\n", argc);

   for (i = 0; i < argc; i++) {
      printf("arg[%d]: %s\n", i, argv[i]);
   }

   if (argc < 2) {
      printf("Required parameter missing - queue manager name\n");
      exit(99);
   }

   pQMName = argv[1];
   printf("pQMName -> %s\n", pQMName);

   /******************************************************************/
   /* Connect to queue manager                                       */
   /******************************************************************/
   MQCONN(pQMName, &hConn, &completionCode, &reasonCode);

   /* Report reason and stop if it failed     */
   if (completionCode == MQCC_FAILED)
   {
      printf("MQCONN for %s ended with reason code %d\n",
	     pQMName,
	     reasonCode);
      exit(EXIT_FAILURE);
   }

   /******************************************************************/
   /* Read the topic strings from the configuration file.            */
   /******************************************************************/

   if (argc >= 3) {
      pConfigurationFile = argv[2];
   } else {
      pConfigurationFile = ConfigurationFileName;
   }

   printf("pConfigurationFile -> %s\n", pConfigurationFile);

   cfgDataFile = fopen(pConfigurationFile, "r");

   if (cfgDataFile == NULL) {
      printf("Unable to open configuration file %s\n", pConfigurationFile);
      exit(EXIT_FAILURE);
   }

   do {
      inString = fgets(inputBuffer, INPUT_BUFFER_SIZE, cfgDataFile);
      lineCount++;

      if (inString == 0) {
         if (feof(cfgDataFile)) {
            break; /* EOF detected. */
         } else {
            printf("Error reading configuration file %s\n",
                   pConfigurationFile);
            exit(EXIT_FAILURE); /* Genuine problem detected. */
         }
      }
      lineLength = strlen(inString);

      /* Strip off the trailing newline if there is one. */
      if (inString[lineLength - 1] == '\n') {
         inString[--lineLength] = 0;
      }

      printf("Line %d: %s\n", lineCount, inString);

      /* Create a subscription descriptor for this topic string      */

      topicString = (char *) malloc(lineLength + 1);
      currentSubscription =
	 (imbt_subscription *) malloc(sizeof(imbt_subscription));

      strcpy(topicString, inString);
      memcpy((void *) &(currentSubscription->sd), (void *) &defaultSD,
             sizeof(MQSD));
      memcpy((void *) &(currentSubscription->md), (void *) &defaultMD,
             sizeof(MQMD));
      memcpy((void *) &(currentSubscription->gmo), (void *) &defaultGMO,
             sizeof(MQGMO));
      currentSubscription->hObj = MQHO_NONE;
      currentSubscription->hSub = MQHO_NONE;

      currentSubscription->nextSubscription = subscriptionListHead;
      subscriptionListHead = currentSubscription;

      /***************************************************************/
      /* Subscribe using a managed destination queue                 */
      /***************************************************************/
      (currentSubscription->sd).Options = MQSO_CREATE | MQSO_NON_DURABLE
         | MQSO_FAIL_IF_QUIESCING | MQSO_MANAGED;

      (currentSubscription->sd).ObjectString.VSPtr = topicString;
      (currentSubscription->sd).ObjectString.VSLength = (MQLONG) lineLength;

      MQSUB(hConn,
            &(currentSubscription->sd),
            &(currentSubscription->hObj),
            &(currentSubscription->hSub),
            &subCompletionCode,
            &reasonCode);

      /* Report reason, if any; stop if failed */
      if (reasonCode != MQRC_NONE)
      {
         printf("MQSUB to %s ended with reason code %d\n",
                topicString,
                reasonCode);
      }

      if (subCompletionCode == MQCC_FAILED)
      {
         printf("Unable to subscribe to topic %s\n", topicString);
         exit(EXIT_FAILURE);
      }

      /***************************************************************/
      /*   Register a consumer                                       */
      /***************************************************************/

      cbd.CallbackFunction = messageConsumer;
      cbd.CallbackArea = (MQPTR) currentSubscription;
      (currentSubscription->gmo).Options = MQGMO_NO_SYNCPOINT;

      MQCB(hConn,
           MQOP_REGISTER,
           &cbd,
           currentSubscription->hObj,
           &currentSubscription->md,
           &currentSubscription->gmo,
           &completionCode,
           &reasonCode);
      /* Report reason, if any; stop if failed */
      if (reasonCode != MQRC_NONE)
      {
         printf("MQCB for %s ended with reason code %d\n",
                topicString,
                reasonCode);
      }

      if (completionCode == MQCC_FAILED)
      {
         printf("Unable to define callback for topic %s\n",
                topicString);
         exit(EXIT_FAILURE);
      }

   } while (1);

   /* We assume one message will be published to each topic. If the  */
   /* callbacks work, then we will never need to drain the queues    */
   /* and we only need to wait until all three callbacks have been   */
   /* invoked once before closing everything down.                   */

   /******************************************************************/
   /* Start consumption of messages                                  */
   /******************************************************************/
   MQCTL(hConn,
         MQOP_START,
         &ctlo,
         &completionCode,
         &reasonCode);
   if (completionCode == MQCC_FAILED)
   {
      printf("MQCTL ended with reason code %d\n", reasonCode);
      exit(EXIT_FAILURE);
   }

   /* Iterate through all the topics, collecting one message from    */
   /* each.                                                          */

   {
      int count = 0;
      while (messageCount < 3)
      {
         printf("%d: Waiting for message delivery\n", count);
	 sleep(2);
         count = count + 1;
      }
   }

/*0123456789012345678901234567890123456789012345678901234567890123456*/
   /******************************************************************/
   /* Stop consumption of messages                                   */
   /******************************************************************/
   MQCTL(hConn,
         MQOP_STOP,
         &ctlo,
         &completionCode,
         &reasonCode);
   if (completionCode == MQCC_FAILED)
   {
      printf("MQCTL ended with reason code %d\n", reasonCode);
      exit(EXIT_FAILURE);
   }

   /* Iterate through all the topics again, closing the associated   */
   /* subscription and queue.                                        */

   currentSubscription = subscriptionListHead;

   while (currentSubscription != NULL) {

      /* Close the subscription handle */
      if (subCompletionCode != MQCC_FAILED) {
         closeOptions = MQCO_NONE;

         MQCLOSE(hConn, &currentSubscription->hSub, closeOptions,
                 &completionCode, &reasonCode);

         if (reasonCode != MQRC_NONE) {
            printf("MQCLOSE ended with reason code %d\n", reasonCode);
         }
      }

      /* Close the managed destination queue (if it was opened) */
      if (subCompletionCode != MQCC_FAILED) {
         closeOptions = MQCO_NONE;

         MQCLOSE(hConn, &currentSubscription->hObj, closeOptions,
                 &completionCode, &reasonCode);

         if (reasonCode != MQRC_NONE) {
            printf("MQCLOSE ended with reason code %d\n", reasonCode);
         }
      }

      currentSubscription = currentSubscription->nextSubscription;

   }

#ifdef IMBT_DEBUG

   {

      /* Artificial halt to allow browsing the queue manager state. */

#define TEMP_STRING_LENGTH 64

      int bytes_read = 0;
      char *tempString = NULL;
      int tempStringLength = TEMP_STRING_LENGTH;

      printf("Proceed? ");

      tempString = (char *) malloc(tempStringLength + 1);
      bytes_read = getline(&tempString, (size_t *) &tempStringLength, stdin);

      free(tempString);

   }

#endif

   fclose(cfgDataFile);

   MQDISC(&hConn, &completionCode, &reasonCode);

   /* Report reason, if any     */
   if (reasonCode != MQRC_NONE) {
      printf("MQDISC for %s ended with reason code %d\n", pQMName, reasonCode);
   }

   printf("End of MQ / ISM bridge test, MQ client V0.1\n");
   return (0);
}
