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
*************************************************************************
*                                                                  		*
* Module Name: mult_prod_node_msg.c                               		*
*                                                                  		*
* Description: THIS IS AN UPDATED VERSION OF mult_prod_msg.c			*
* 				this can have multiple sub-nodes.						*
* 				Could do different tests here, but only with single 	*
*				session. Can have multiple producers with each having a	*
*				single topic. If no.prod < no.msgs, then once each prod	*
*				gets a msg assignd, it will asign the nxt msg to the 	*
*				first prod, making its msg count 2. And so on.			*
*                                                                  		*
*************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_options.h"

#define AJ_MSG "AJ-MSG-0"
#define AJ_RETCODE_WRONG_MSG 1313

#define NOMESSAGES 1000000						//Number of Messages to be passed.
#define POINTOFCHECKS 100000					//At every POINTOFCHECKS the message will be verified.
#define NOPUBSUB 100000							//Number of producers and consumers.
#define NOSUBNODE 10							//How many prod and consumers should

typedef struct inputs
{
	int32_t noMessages;
	int32_t pointOfChecks;
	int32_t noProdCons;
	int32_t noSubnodes;
}inputs_t;

int checkLastMsg = 0;
int rc = OK;
int32_t testRetcode = OK;
inputs_t ips;

bool asyncMessageCallback(
                          ismEngine_ConsumerHandle_t      hConsumer,
						  ismEngine_DeliveryHandle_t      hDelivery,
						  ismEngine_MessageHandle_t       hMessage,
						  uint32_t                        deliveryId,
						  ismMessageState_t               state,
						  uint32_t                        destinationOptions,
						  ismMessageHeader_t *            pMsgDetails,
						  uint8_t                         areaCount,
						  ismMessageAreaType_t            areaTypes[areaCount],
						  size_t                          areaLengths[areaCount],
						  void *                          pAreaData[areaCount],
						  void *                          pConsumerContext,
                                                  ismEngine_DelivererContext_t *  _delivererContext);


int main(int argc, char *argv[])
{
    char msgBuffer[256];
    int32_t x;						//For loop count for creating producers and consumers
    size_t areaLength; 				//+1 is for the null terminator character
    char *areaArray[1];
	char topic[256];
	int32_t y = 1, z;					// Counts for creating nodes. Number of producers will be divided by the number of nodes specified.
	int32_t i,j=0,k;					//Counts for creating messages and assigning them to producers and consumers.
    int trclvl = 0;

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) return rc;

	ips.noMessages = NOMESSAGES;
	ips.pointOfChecks = POINTOFCHECKS;
	ips.noProdCons = NOPUBSUB;
	ips.noSubnodes = NOSUBNODE;

	if(argc>1)
	{
		ips.noMessages = atoi(argv[1]);
		ips.pointOfChecks = atoi(argv[2]);
		ips.noProdCons = atoi(argv[3]);
		ips.noSubnodes = atoi(argv[4]);

	}

	ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaType = ismMESSAGE_AREA_PAYLOAD;

    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ProducerHandle_t *hProducer;
    ismEngine_ConsumerHandle_t *hConsumer;
    ismEngine_MessageHandle_t hMessage = NULL;

    hProducer = malloc(ips.noProdCons * sizeof(ismEngine_ProducerHandle_t));
    hConsumer = malloc(ips.noProdCons * sizeof(ismEngine_ConsumerHandle_t));

    printf("\n\n\n\n\nThe test commenced for passing %d messages with checkpoint at every %d message.\n\n", ips.noMessages, ips.pointOfChecks);
    printf("\nWill be having %d producers with a topic each.", ips.noProdCons);

    rc = test_engineInit_DEFAULT;
    if (rc != OK){
    	    // Free the memory to prevent the leakage
    	    free(hConsumer);
    	    free(hProducer);
    	    return rc;
    }

	rc = ism_engine_createClientState("Ashlin-Client",
	                                  testDEFAULT_PROTOCOL_ID,
			                            ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                        NULL, NULL, NULL,
			                            &hClient,
			                            NULL,
			                            0,
			                            NULL);

	rc = ism_engine_createSession(hClient,
			                        ismENGINE_CREATE_SESSION_OPTION_NONE,
			                        &hSession,
			                        NULL,
			                        0,
			                        NULL);


	rc = ism_engine_startMessageDelivery(hSession,
										   ismENGINE_START_DELIVERY_OPTION_NONE,
										   NULL,
										   0,
										   NULL);
	z = ips.noProdCons/ips.noSubnodes;
	printf("\n%d\n",z);
	for(x=1; x<=ips.noProdCons; x++)
	{

		sprintf(topic,"Topic/%d/%d",y,x);

		rc = ism_engine_createProducer(hSession,
										 ismDESTINATION_TOPIC,
										 topic,
										 &hProducer[x-1],
										 NULL,
										 0,
										 NULL);
	    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

	    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
		rc = ism_engine_createConsumer(hSession,
										 ismDESTINATION_TOPIC,
										 topic,
										 &subAttrs,
										 NULL, // Unused for TOPIC
										 topic,
										 strlen(topic)+1,								//+1 for Null reference
										 asyncMessageCallback,
										 NULL,
										 ismENGINE_CONSUMER_OPTION_NONE,
										 &hConsumer[x-1],
										 NULL,
										 0,
										 NULL);
	    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

		if(x%z==0)
		{
			y++;
		}
	}

	k = ips.noMessages/ips.noProdCons;

	for(i=1; i<=ips.noMessages; i++)
	{
		int ref = sprintf(msgBuffer,"%s%d",AJ_MSG, i);
		areaLength = ref + 1;
	    areaArray[0] = msgBuffer;							//Remember its a hack,
															// u r not suppose to know which pointer in the areaArray is the message, but to find it using iteration by checking the areaType
	    rc = ism_engine_createMessage(&header,
	    		                        1,
	    		                        &areaType,
	    		                        &areaLength,
	    		                        (void **)areaArray,
	    		                        &hMessage);
	    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

    	rc = ism_engine_putMessage(hSession,			// Each message should be put and then released before going to next message
									 hProducer[j],
									 NULL,
									 hMessage,
									 NULL,
									 0,
									 NULL);
        if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

		if(i%k==0)
		{
			j++;
		}
	}

	for(x=0; x<ips.noProdCons; x++)
	{

		rc = ism_engine_destroyProducer(hProducer[x],
										  NULL,
										  0,
										  NULL);
	    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

		rc = ism_engine_destroyConsumer(hConsumer[x],
											  NULL,
											  0,
											  NULL);
	    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;
	}
    rc = ism_engine_destroySession(hSession,
								     NULL,
								     0,
								     NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

    rc = ism_engine_destroyClientState(hClient,
                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE,
									NULL,
									0,
									NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion) return rc;

    rc = test_engineTerm(true);
    if (rc != OK) return rc;

    rc = test_processTerm(testRetcode == OK);
    if (rc != OK) return rc;

    if (testRetcode == OK)
    {
    	printf("Test passed.\n");
    }
    else
    {
    	printf("Test failed.\n");
    }

    free(hConsumer);
    free(hProducer);

    return (int)testRetcode;

}

bool asyncMessageCallback(
        ismEngine_ConsumerHandle_t      hConsumer,
		ismEngine_DeliveryHandle_t      hDelivery,
		ismEngine_MessageHandle_t       hMessage,
		uint32_t                        deliveryId,
		ismMessageState_t               state,
		uint32_t                        destinationOptions,
		ismMessageHeader_t *            pMsgDetails,
		uint8_t                         areaCount,
		ismMessageAreaType_t            areaTypes[areaCount],
		size_t                          areaLengths[areaCount],
		void *                          pAreaData[areaCount],
		void *                          pConsumerContext,
                ismEngine_DelivererContext_t *  _delivererContext)
{
    static int countIncomingMessage = 1;
	static int i=1;
	char *reportMsg = NULL;

	if((i)%ips.pointOfChecks==0)
	{
		printf("The message received under the %s was: %.*s\n",
				((char *)pConsumerContext), (int) areaLengths[0], (char *)pAreaData[0]);
	}

    if (rc == OK)
    {
        //AJ- Actual TEST: Compare the message payload with a message we expect and checks each time the message is received.

    	char temp_msg[256];
    	int len;
    	len = sprintf(temp_msg,"%s%d", AJ_MSG, i++);

    	if((len+1)!= areaLengths[0] || strncmp(temp_msg, pAreaData[0], areaLengths[0])!=0)
    	{
    		//The mesage wasn't the message we expected
    		printf("The message we expected was: %s but the message we received was %.*s\n",
    	    temp_msg, (int)areaLengths[0], (char *)pAreaData[0]);
    	    rc = AJ_RETCODE_WRONG_MSG;
		}

    	else
    	{
    		//The message matched the expected format
    	    if((i-1) == ips.noMessages)
    	    {
    	    	checkLastMsg = 1;
    	    	printf("\n\nITS DONE and the last message has length %u and is %s!!\n",
    	    			(int)areaLengths[0], (char *)pAreaData[0]);
    	    }
    	}

        reportMsg = alloca(areaLengths[0]+1);
        if (reportMsg != NULL)
        {
            memcpy(reportMsg, pAreaData[0], areaLengths[0]);
            reportMsg[areaLengths[0]] = '\0';
        }

        ism_engine_releaseMessage(hMessage);
    }
    bool wantAnotherMessage = true;

    if (countIncomingMessage<=ips.noMessages)
    {
    	countIncomingMessage++;
    }
    else
    {
        printf("The Last Message is %s.\n", reportMsg ? reportMsg : "<NULLMSG>");

        //stops message delivery callback will unlock the mutex
        wantAnotherMessage = false;
    }

    if ((rc != OK) && (testRetcode == OK))
    {
        testRetcode = rc;
    }

    return wantAnotherMessage;
}
