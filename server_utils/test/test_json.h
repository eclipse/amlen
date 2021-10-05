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



/*   (programming note 1)
 (programming note 11)
 Array that carries the basic tests for JSON APIs to CUnit framework.
 */
CU_TestInfo ISM_Utils_CUnit_JsonBasic[] = {
    { "JsonParsing", jsonparsetest },
    { "Utf8", utf8test },
    { "Utf8Restrict", utf8Restrict_test },
    { "Utf8Replace", utf8Replace_test },
    { "Match", testMatch },
    { "Regex", testRegex },
    { "JsonNumber" , testValidNumber },
    CU_TEST_INFO_NULL
};


