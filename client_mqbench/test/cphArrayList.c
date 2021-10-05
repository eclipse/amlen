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

#include "cphUtil.h"
#include "cphArrayList.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* The functions in this module provide a set of routines for maintaining a linked list of pointers to
   data structures. The interface is loosely based on the Java ArrayList class. In cph, the routines are used
   to maintain a list of worker thread structures and to maintain a list of configuration property bundles */

/*
** Method: cphArrayListIni
**
** Initialise a new CPH_ARRAYLIST structure
**
** Input/Output Parameters: ppArrayList - double pointer for the new array list control block
**
*/
void cphArrayListIni(CPH_ARRAYLIST **ppArrayList) {
   CPH_ARRAYLIST *pArrayList;

   pArrayList = malloc(sizeof(CPH_ARRAYLIST));
   if (NULL != pArrayList) {
	   pArrayList->nEntries = 0;
	   pArrayList->items = NULL;
   }

   *ppArrayList = pArrayList;
}

/*
** Method: cphArrayListFree
**
** Dispose of an cphArrayList control block
**
** Input/Output Parameters: ppArrayList - double pointer to the array list control block. The pointer 
**                          is set to null after the array list has been disposed of.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphArrayListFree(CPH_ARRAYLIST **ppArrayList) {
    CPH_ARRAYLISTITEM *i, *i1;

	if (NULL != *ppArrayList) {
        if (NULL != (*ppArrayList)->items) {
            i = (*ppArrayList)->items;
            while(i) {
                i1 = i->next;
                free(i);
                i = i1;
            }
        }
		
        free(*ppArrayList);
		*ppArrayList = NULL;
		return(CPHTRUE);
	}
	return(CPHFALSE);
}

/*
** Method: cphArrayListAdd
**
** Adds a new item to the array list.
**
** Input Parameters: pArrayList - pointer to the CPH_ARRAYLIST control block
**                   anItem - void pointer to an item to be added
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphArrayListAdd(CPH_ARRAYLIST *pArrayList, void *anItem) {

	CPH_ARRAYLISTITEM *item1;
	CPH_ARRAYLISTITEM *item = (CPH_ARRAYLISTITEM *) malloc(sizeof(CPH_ARRAYLISTITEM));

	if (NULL != item) {
        item->item = anItem;
		item->next = NULL;
	}

    if (pArrayList->items == NULL) {
		pArrayList->items = item;
    }
	else
	{
		item1 = pArrayList->items;
		while (item1->next) item1 = item1->next;
		item1->next = item;
	}

	pArrayList->nEntries++;

   return(CPHTRUE);
}

/*
** Method: cphArrayListSize
**
** Return the number of entries in the array list. This is known by the "nEntries" member of the
** control block
**
** Input Parameters: pArrayList - pointer to the CPH_ARRAYLIST control block
**
** Returns: the number of entries in the array list.
**
*/
int  cphArrayListSize(CPH_ARRAYLIST *pArrayList) {
   return(pArrayList->nEntries);
}

/*
** Method: cphArrayListIndexOf
**
** Seaches a list for a matching entry. It returns the position of an item in the list according 
** to the address of the item in the list. (The first item in the list is at position 0).
**
** Input Parameters: pArrayList - pointer to the CPH_ARRAYLIST control block
**                   aRequiredItem - the address of the item to look for in the list
**
** Returns: the position of the item in the list (int)
**
*/
int  cphArrayListIndexOf(CPH_ARRAYLIST *pArrayList, void *aRequiredItem) {
    void *anItem;
    int n=-1;
    int res=CPHFALSE;
    
    if (NULL == pArrayList->items) res = CPHFALSE;

    n = 0;
    anItem = pArrayList->items;
    do {
       if (anItem == aRequiredItem)
          {
          res = CPHTRUE;
          break;
          }
       anItem = pArrayList->items->next;
       n++;
    } while (NULL != anItem);

    if (res == CPHTRUE) return n;
    return -1;
}

/*
** Method: cphArrayListGet
**
** Returns the item at a specified index in the list. (The first item in the list
** is at position zero).
**
** Input Parameters: pArrayList - pointer to the CPH_ARRAYLIST control block
**                   index - the list item is retrieved from this position in the list
**
** Returns: a void pointer to the list item or NULL if there is no item at the specified
**          position
**
*/
void *cphArrayListGet(CPH_ARRAYLIST *pArrayList, int index) {
    void *requiredItem = NULL;
    void *anItem;
    int n=0;

    if ( (NULL != pArrayList->items) &&
         (index >= 0) ){

        anItem = pArrayList->items;
        for (n=0; n<=index; n++)
        {
            if (NULL == anItem) break;
            if (index == n) requiredItem = anItem;
            anItem = pArrayList->items->next;
        }
    }

    return(requiredItem);
}

/*
** Method: cphArrayListIsEmpty
**
** Identifies whteher or not an array list is empty
**
** Input Parameters: pArrayList - pointer to the CPH_ARRAYLIST control block
**
** Returns: CPHTRUE if the list if empty and CPHFALSE if it isn't.
**
*/
int cphArrayListIsEmpty(CPH_ARRAYLIST *pArrayList) {
    int status = CPHFALSE;

    if (0 == pArrayList->nEntries) status = CPHTRUE;
    return(status);
}
