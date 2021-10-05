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

#ifndef _CPHSTRINGBUFFER

/* The CPH_STRINGBUFFER structure */
typedef struct _cphstringbuffer {
    char *sb;         /* Pointer to a character string containing the data in the buffer */
} CPH_STRINGBUFFER;

/* Function prototypes */
void cphStringBufferIni(CPH_STRINGBUFFER **ppStringBuffer);
int cphStringBufferFree(CPH_STRINGBUFFER **ppStringBuffer);
int cphStringBufferAppend(CPH_STRINGBUFFER *pStringBuffer, char *aString);
int cphStringBufferAppendInt(CPH_STRINGBUFFER *pStringBuffer, int anInt);
int cphStringBufferAppendDouble(CPH_STRINGBUFFER *pStringBuffer, double aDouble);
int cphStringBufferSetLength(CPH_STRINGBUFFER *pStringBuffer, unsigned int newLength);
int cphStringBufferGetLength(CPH_STRINGBUFFER *pStringBuffer);
char *cphStringBufferToString(CPH_STRINGBUFFER *pStringBuffer);

#define _CPHSTRINGBUFFER
#endif
