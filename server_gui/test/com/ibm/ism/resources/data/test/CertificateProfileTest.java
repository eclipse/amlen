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
package com.ibm.ism.resources.data.test;

import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.security.CertificateProfile;

public class CertificateProfileTest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @Before
    public void setUp() throws Exception {}

    @After
    public void tearDown() throws Exception {}

    @Test
    public final void testValidateName() {
        CertificateProfile profile = new CertificateProfile();

        ValidationResult result = profile.validateName("AValidName", "name");
        assertTrue("Test a valid name " + result.getResult(),result.getResult() == VALIDATION_RESULT.OK);

        result = profile.validateName("This Name Is Max Length! kkl;asdkkf  aslsoei__ fjk11@# 1 the end", "name");
        assertTrue("Test a valid name of max length",result.getResult() == VALIDATION_RESULT.OK);

        result = profile.validateName(" leading blank", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("Test a leading blank",result.getResult() == VALIDATION_RESULT.LEADING_OR_TRAILING_BLANKS);

        result = profile.validateName("trailing blank ", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("Test a trailing blank " + result.getResult(),result.getResult() == VALIDATION_RESULT.LEADING_OR_TRAILING_BLANKS);

        result = profile.validateName("some mixed \u30A0 \u30F0 \u30FF characters", "name");
        assertTrue("Test some mixed characters",result.getResult() == VALIDATION_RESULT.OK);

        result = profile.validateName("some mixed \u30A0 \u30F0 \u30FF characters", "name");
        assertTrue("Test some mixed characters",result.getResult() == VALIDATION_RESULT.OK);

        result = profile.validateName("!some invalid characters at the beginning", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("s a control character",result.getResult() == VALIDATION_RESULT.INVALID_START_CHAR);

        result = profile.validateName("some control characters \u0002 \u0010 in the middle", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("contains a control character",result.getResult() == VALIDATION_RESULT.INVALID_CHARS);

        result = profile.validateName("equal = equal", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("contains an equal",result.getResult() == VALIDATION_RESULT.INVALID_CHARS);

        result = profile.validateName("comma,comma", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("contains an comma",result.getResult() == VALIDATION_RESULT.INVALID_CHARS);

        result = profile.validateName("quote\"quote", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("contains an quote",result.getResult() == VALIDATION_RESULT.INVALID_CHARS);

        result = profile.validateName("backslash \\ backslash", "name");
        System.err.println(result.getErrorMessage(null));
        assertTrue("contains an backslash",result.getResult() == VALIDATION_RESULT.INVALID_CHARS);

    }

}
