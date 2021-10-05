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

#ifndef _CPHARRAYLIST
#define _CPHARRAYLIST

/*typedef struct _cphlistiterator CPH_LISTITERATOR; */

/* Following syntax doesn't work on Linux so have to declare as void */
/* typedef struct _cpharraylistitem CPH_ARRAYLISTITEM; */

/* CPH_ARRAYLISTITEM is a struct to define an individual item in the list and to point to the
   next entry */
typedef struct _cpharraylistitem {
    void *item;               /* The address of the item */

    /* CPH_ARRAYLISTITEM *next; */
	void *next;               /* The address of the next item in the list */
} CPH_ARRAYLISTITEM;



/* The CPH_ARRAYLIST structure - this contains a linked list of CPH_ARRAYLISTITEM structures */
typedef struct _cpharraylist {
   int nEntries;              /* The number of entries in the list */
   CPH_ARRAYLISTITEM *items;  /* The address of the first entry in the list */

} CPH_ARRAYLIST;

/* Function prototypes */
void cphArrayListIni(CPH_ARRAYLIST **ppArrayList);
int  cphArrayListFree(CPH_ARRAYLIST **ppArrayList);
int  cphArrayListAdd(CPH_ARRAYLIST *pArrayList, void *anItem);
int  cphArrayListSize(CPH_ARRAYLIST *pArrayList);
int  cphArrayListIndexOf(CPH_ARRAYLIST *pArrayList, void *anItem);
void *cphArrayListGet(CPH_ARRAYLIST *pArrayList, int index);
int  cphArrayListIsEmpty(CPH_ARRAYLIST *pArrayList);

#endif
