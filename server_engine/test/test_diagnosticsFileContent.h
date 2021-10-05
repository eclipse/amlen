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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_diagnosticsFileContent.h                        */
/*                                                                   */
/* Description: CUnit test contents of engine diagnostic files       */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_DIAGNOSTICSFILECONTENT_DEFINED
#define __TEST_DIAGNOSTICSFILECONTENT_DEFINED

typedef void (*checkDiagnosticFile_AssertFail_t)(const char *function, int lineNumber, unsigned long int rc);

typedef enum tag_displayType
{
    displayType_NONE = 0,
    displayType_RAW = 1,
    displayType_PARSED = 2,
} displayType;

void test_checkDumpClientStatesFile(const char *filePath,
                                    const char *password,
                                    checkDiagnosticFile_AssertFail_t failFunc,
                                    displayType display);

#endif /* __TEST_CLIENTSTATE_DEFINED */

/*********************************************************************/
/* End of test_diagnosticsFileContent.h                              */
/*********************************************************************/
