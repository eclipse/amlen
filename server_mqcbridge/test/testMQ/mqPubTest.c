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

#include <cmqc.h>

#include <mqPubTest.h>

/* Test program for the MQ end of the MQ / ISM bridge. */

/*********************************************************************/
/* How do you run the test?                                          */
/* You need a running queue manager at a version that supports       */
/* pub/sub. This code has been exercised with V7.5 but probably      */
/* works with versions earlier than that. You also need an           */
/* application that will receive the messages published by this      */
/* program. The expectation is that the "other" application will be  */
/* connected to an ISM server and from the point of view of this     */
/* code is on the far side of the MQ to ISM bridge.                  */
/*********************************************************************/

/* There is minimal validation of user inputs and little error       */
/* handling. Inputs are assumed to be valid and errors are reported  */
/* as soon as they are detected and typically the application stops. */

/* Command line parameters */
/* 1. Queue manager name */
/* 2. Name of the configuration file */
/*    Defaults to mq_to_ism.cfg */

#define MESSAGE_BUFFER_LENGTH 1024
#define INPUT_BUFFER_LENGTH 1024

int main(int argc, char **argv)
{

   MQMD defaultMD = { MQMD_DEFAULT };  /* Message Descriptor */
   MQPMO defaultPMO = {MQPMO_DEFAULT}; /* put message options */
   MQMD md = { MQMD_DEFAULT };	       /* Message Descriptor */
   MQPMO pmo = {MQPMO_DEFAULT};	       /* put message options */
   MQOD od = {MQOD_DEFAULT};	       /* Object Descriptor */
   MQHOBJ hObj;			       /* object handle */

   MQHCONN hConn; /* Connection handle */
   MQLONG completionCode; /* Completion code */
   MQLONG reasonCode; /* Reason code */

   char buffer[MESSAGE_BUFFER_LENGTH];
   MQLONG bufferLength; /* Buffer length */

   char ConfigurationFileName[] = "mq_to_ism.cfg";
   char *pQMName = NULL;
   char *pConfigurationFile = NULL;
   char *inString = NULL;
   char inputBuffer[INPUT_BUFFER_LENGTH];

   FILE *cfgDataFile = NULL;

   int i = 0;
   int lineLength = 0;
   int lineCount = 0;

   printf("Start MQ / ISM bridge publication test, V0.1\n");

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

   do
   {
      inString = fgets(inputBuffer, INPUT_BUFFER_LENGTH, cfgDataFile);

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

      /* If the first character is a ! the treat the whole line as a */
      /* comment and just discard it. */
      if (inString[0] == '!')
      {
         continue;
      }

      /* The line we have just read is a topic string to which we    */
      /* publish. Open the target topic for output.                  */

      od.ObjectString.VSPtr = inString;
      od.ObjectString.VSLength = (MQLONG) lineLength;

      od.ObjectType = MQOT_TOPIC;
      od.Version = MQOD_VERSION_4;

      MQOPEN(hConn,
	     &od,		/* object descriptor for topic */
	     MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING, /* open options */
	     &hObj,		/* object handle */
	     &completionCode,
	     &reasonCode);

      if (reasonCode != MQRC_NONE)
      {
	 printf("MQOPEN for %s ended with reason code %d\n",
		inString,
		reasonCode);
      }
      if (completionCode == MQCC_FAILED)
      {
	 printf("Unable to open topic %s for publish\n", inString);
	 exit(EXIT_FAILURE);
      }


/*0123456789012345678901234567890123456789012345678901234567890123456*/
      /***************************************************************/
      /* Publish a message to the topic                              */
      /***************************************************************/

      memcpy((void *) &md, (void *) &defaultMD, sizeof(MQMD));
      memcpy((void *) &pmo, (void *) &defaultPMO, sizeof(MQPMO));

      memcpy(md.Format,
             MQFMT_STRING,
             (size_t) MQ_FORMAT_LENGTH);

      pmo.Options = MQPMO_FAIL_IF_QUIESCING | MQPMO_NO_SYNCPOINT;

      sprintf(buffer, "Some text for %s", inString);
      bufferLength = strlen(buffer);
      printf("Message Content: %s\n", buffer);

      MQPUT(hConn,
            hObj,
            &md,
            &pmo,
            bufferLength,	/* message length */
            buffer,		/* message buffer */
            &completionCode,
            &reasonCode);

      if (reasonCode != MQRC_NONE)
      {
         printf("MQPUT to %s ended with reason code %d\n",
                inString,
                reasonCode);
	 exit(EXIT_FAILURE);
      }

      /***************************************************************/
      /* Close the target topic                                      */
      /***************************************************************/
      MQCLOSE(hConn,
	      &hObj,
	      MQCO_NONE,
	      &completionCode,
	      &reasonCode);

      if (reasonCode != MQRC_NONE)
      {
	 printf("MQCLOSE ended with reason code %d\n",
		reasonCode);
      }
   } while (1);

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
