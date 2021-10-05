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

#include "cphConfig.h"
#include "cphSpinLock.h"

#ifndef _CPHDESTINATIONFACTORY

#define CPH_DESTINATIONFACTORY_MODE_SINGLE 0   /* If "mode" is set to this, it signifies we are not using multiple destinations */
#define CPH_DESTINATIONFACTORY_MODE_DIST 1     /* If "mode" is set to this, then we are using multiple destinations */

/* The destination factory control structure */
typedef struct _cphdestinationfactory {

   CPH_CONFIG *pConfig;   /* Pointer to the cph configuration control structure */

   int mode;              /* Whether we are running in single or multi-destination mode */

   int destBase;          /* The numeric base for multi-destination mode */
   int destMax;           /* The numeric maximum for multi-destination mode */
   int destNumber;        /* The numeric range for multi-destination mode */
   char destPrefix[50];   /* The prefix to the destination - for single destination mode the destination is set to 
                             just the prefix. For multi-destination mode the prefix is appended with the next numeric
                             destination number */
	
   int nextDest;          /* The next destination to be used */
   CPH_SPINLOCK *pDestCreateLock;  /* Lock to be used when generating destinations to protect the next destination number */

} CPH_DESTINATIONFACTORY;

/* Function prototypes */
void cphDestinationFactoryIni(CPH_DESTINATIONFACTORY **ppDestinationFactory, CPH_CONFIG *pConfig);
int cphDestinationFactoryFree(CPH_DESTINATIONFACTORY **ppDestinationFactory);
int cphDestinationFactoryGenerateDestination(CPH_DESTINATIONFACTORY *pDestinationFactory, char *destString);
int cphDestinationFactoryGetNextDestination(CPH_DESTINATIONFACTORY *pDestinationFactory);

#define _CPHDESTINATIONFACTORY
#endif
