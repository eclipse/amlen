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

#ifndef _CPHNAMEVAL

#include <stdio.h>

/* Linked list type for storing name and value configuration properties */
typedef struct _cphnameval {
	char name[80];
	char value[1024];
	void *next;
} CPH_NAMEVAL;

int cphNameValAdd(CPH_NAMEVAL **ppList, char *name, char *value);
int cphNameValGet(CPH_NAMEVAL *pList, char *name, char *value);
int cphNameValFree(CPH_NAMEVAL **ppList);
int cphNameValNext(CPH_NAMEVAL **ppList);

#ifdef CPH_TESTING
int cphNameValList(CPH_NAMEVAL *pList);
#endif

#define _CPHNAMEVAL
#endif
