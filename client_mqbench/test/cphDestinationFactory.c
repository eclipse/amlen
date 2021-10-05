const static char sccsid[] = "%Z% %W% %I% %E% %U%";
/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

#include <string.h>

#include "cphStringBuffer.h"
#include "cphDestinationFactory.h"

/*
** Method: cphDestinationFactoryIni
** 
** Create and initialise the control block for the destination factory
**
** Input Parameters:
**
** Output parameters: A double pointer to the newly created control block
**
*/
void cphDestinationFactoryIni(CPH_DESTINATIONFACTORY **ppDestinationFactory, CPH_CONFIG *pConfig) {
    CPH_DESTINATIONFACTORY *pDestinationFactory = NULL;
    CPH_TRACE *pTrc;
    int mode;
    int destBase;
    int destMax;
    int destNumber;
    char destPrefix[50];
    int nextDest = 0;
    int status = CPHTRUE;
    CPH_SPINLOCK *pDestCreateLock;

    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphDestinationFactoryIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (pConfig != NULL) {

        if (CPHTRUE != cphConfigGetInt(pConfig, &destBase, "db")) {
            cphConfigInvalidParameter(pConfig, "(db) Mult-destination numeric base cannot be retrieved");
            status = CPHFALSE;
        }
        else
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "destBase: %d.", destBase);

        if (CPHTRUE != cphConfigGetInt(pConfig, &destMax, "dx")) {
            cphConfigInvalidParameter(pConfig,  "(dx) Mult-destination numeric maximum cannot be retrieved");
            status = CPHFALSE;
        }
        else
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Multi-destination numeric maximum: %d.", destMax);

        if (CPHTRUE != cphConfigGetInt(pConfig, &destNumber, "dn")) {
            cphConfigInvalidParameter(pConfig, "(dn) Mult-destination numeric range cannot be retrieved");
            status = CPHFALSE;
        }
        else
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Multi-destination numeric range: %d.", destNumber);

        if (CPHTRUE != cphConfigGetString(pConfig, destPrefix, "d")) {
            cphConfigInvalidParameter(pConfig, "(d) Destination Prefix cannot be retrieved");
            status = CPHFALSE;
        }
        else
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Destination prefix: %s.", destPrefix);
    
        if (CPHFALSE == cphSpinLock_Init( &(pDestCreateLock))) {
            cphLogPrintLn(pConfig->pLog, LOGERROR, "Cannot create destination create lock" );
            status = CPHFALSE;
        }

        mode = CPH_DESTINATIONFACTORY_MODE_SINGLE;
		if ( destBase>0 && destMax>0 && destNumber>0 ) {
			
			/* Special case: if *all* elements are set we use -dn as the
			   start point in the loop */
			mode = CPH_DESTINATIONFACTORY_MODE_DIST;
			nextDest = destNumber;
			destNumber = destMax - destBase + 1;
			
			if ( destNumber<destBase || destNumber>destMax ) {
				cphConfigInvalidParameter(pConfig, "-dn must be within the specified bounds of -db and -dx");
                status = CPHFALSE;
			}
			
		} else if ( destBase>0 || destMax>0 || destNumber>0 ) {
			/* If we have specified any the elements ... */
			
			mode = CPH_DESTINATIONFACTORY_MODE_DIST;
			
			if ( destBase==0 ) { 
				/* we need to calculate the base */
				if ( destMax>0 && destNumber>0 ) {
					destBase = destMax - destNumber + 1;
				} else {
					destBase = 1;
				}
			}

			/* We know topicBase is set... */
			nextDest = destBase;
			
			if ( destMax==0 ) {
				/* we need to calculate the max */
				if ( destNumber>0 ) {
					destMax = destNumber + destBase - 1;
				} else {
					/* There is no max !  This implies an ever increasing count */
				}
			}
			
			if ( destNumber==0 ) {
				/* we need to set topicNumber */
				if ( destMax>0 ) {
					destNumber = destMax - destBase + 1;
				} else {
					/* There is no max! */
				}
			}
			
		} /* end if any multi-destination settings */
		
		/* enforce destination constraints */
		//if ( destBase<0 || destMax<destBase ) {
        if ( destBase<0 || ( (destMax > 0) && destMax<destBase ) ) {
			cphConfigInvalidParameter(pConfig, "Destination range is negative." );
            status = CPHFALSE;
		}

        if ( (CPHTRUE == status) &&
             ( NULL != (pDestinationFactory = malloc(sizeof(CPH_DESTINATIONFACTORY)))) ) {
            pDestinationFactory->pConfig = pConfig;

            pDestinationFactory->mode = mode;
            pDestinationFactory->destBase = destBase;
            pDestinationFactory->destMax = destMax;
            pDestinationFactory->destNumber = destNumber;
            pDestinationFactory->nextDest = nextDest;
            strcpy(pDestinationFactory->destPrefix, destPrefix);
            pDestinationFactory->pDestCreateLock = pDestCreateLock;
        }
    }

    *ppDestinationFactory = pDestinationFactory;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
}

/*
** Method: cphDestinationFactoryFree
**
** Free a destinfactory control block
**
** Input Parameters: ppDestinationFactory - double pointer to the destination factory to be freed
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDestinationFactoryFree(CPH_DESTINATIONFACTORY **ppDestinationFactory) {
    int res = CPHFALSE;

    CPH_TRACE *pTrc;

    pTrc = (*ppDestinationFactory)->pConfig->pTrc;

#define CPH_ROUTINE "cphDestinationFactoryFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != *ppDestinationFactory) {
        cphSpinLock_Destroy( &((*ppDestinationFactory)->pDestCreateLock));
        free(*ppDestinationFactory);
		*ppDestinationFactory = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(res);
}

/*
** Method: cphGenerateDestination
**
** Generates a destination name. If using asingle destination the destination is simply set to the
** destination prefix. If using multiple destinations then the prefix is appended with the next
** numeric suffix as calculated by cphDestinationFactoryGetNextDestination.
**
** Input Parameters: pDestinationFactory - pointer to the destination factory
**
** Output Parameters: destString - the string into which the determined destination string should be copied
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDestinationFactoryGenerateDestination(CPH_DESTINATIONFACTORY *pDestinationFactory, char *destString) { 
    CPH_TRACE *pTrc;
    CPH_STRINGBUFFER *pSb;

    pTrc = pDestinationFactory->pConfig->pTrc;

#define CPH_ROUTINE "cphGenerateDestination"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    switch ( pDestinationFactory->mode  ) {

            /* If using single destinations only, then the prefix is the destination */
            case CPH_DESTINATIONFACTORY_MODE_SINGLE:
                strcpy(destString, pDestinationFactory->destPrefix);
                break;

            /* For multiple destinations, determine the next in the sequence determined by the command line options */
            case CPH_DESTINATIONFACTORY_MODE_DIST:
                cphStringBufferIni(&pSb);
                cphStringBufferAppend(pSb, pDestinationFactory->destPrefix);
                cphStringBufferAppendInt(pSb, cphDestinationFactoryGetNextDestination(pDestinationFactory));
                strcpy(destString, cphStringBufferToString(pSb));
                cphStringBufferFree(&pSb);
                break;

            /* If we don't recognise the mode set the generated destination to NULL */
            default: *destString = '\0';

    } /* end switch */

    CPHTRACEMSG(pTrc, "Generated destination: %s.", destString);
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return CPHTRUE;
}

/*
** Method: cphDestinationFactoryGetNextDestination
**
** Determine the next numeric suffix for a multi-destination topic. This is determined in accord with
** the command line options "db", "dn" and "dx".
**
** Input Parameters: pDestinationFactory - pointer to the destination factory
**
** Returns: the next numeric suffix to be used
**
*/
int cphDestinationFactoryGetNextDestination(CPH_DESTINATIONFACTORY *pDestinationFactory) {
    CPH_TRACE *pTrc;
    int num;

    pTrc = pDestinationFactory->pConfig->pTrc;

#define CPH_ROUTINE "cphDestinationFactoryGetNextDestination"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    cphSpinLock_Lock(pDestinationFactory->pDestCreateLock);
    num = pDestinationFactory->nextDest++;

    /* Note loose comparison here allows Max=0 to
    leave an infinite sequence. */
    if ( pDestinationFactory->nextDest == (pDestinationFactory->destMax+1) ) {
        pDestinationFactory->nextDest = pDestinationFactory->destBase; 
    }

    cphSpinLock_Unlock(pDestinationFactory->pDestCreateLock);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return num;
}
