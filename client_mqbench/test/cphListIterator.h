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

#ifndef _CPHLISTITERATOR

#include "cphArrayList.h"

/* The CPH_LISTITERATOR structure */
typedef struct _cphlistiterator {
	CPH_ARRAYLISTITEM *listStart;    /* The address of the first item in the list */
	CPH_ARRAYLISTITEM *currentItem;  /* The address of the current item in the list */
} CPH_LISTITERATOR;

/* Function prototypes */
void cphListIteratorIni(CPH_LISTITERATOR **ppListIterator, CPH_ARRAYLIST *pArrayList);
int cphListIteratorFree(CPH_LISTITERATOR **ppListIterator);
int cphListIteratorHasNext(CPH_LISTITERATOR *pListIterator);
CPH_ARRAYLISTITEM* cphListIteratorNext(CPH_LISTITERATOR *pListIterator);
CPH_LISTITERATOR* cphArrayListListIterator(CPH_ARRAYLIST *pArrayList);

#define _CPHLISTITERATOR
#endif
