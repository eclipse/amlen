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

import com.ibm.ima.resources.data.DestinationMappingRule;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;

public class DestinationMappingRuleTest {

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
    public final void testValidate() {
        DestinationMappingRule rule = new DestinationMappingRule();
        rule.setName("test");
        rule.setQueueManagerConnection("any");
        rule.setRuleType(DestinationMappingRule.RULE_TYPE_1_ISMTOPIC_2_MQQUEUE);
        rule.setSource("/good/topic/string");
        rule.setDestination("goodMQname");
        assertTrue(rule.validate().isOK());

        rule.setSource("/contains/invalid/#");
        assertTrue(rule.validate().getResult().equals(VALIDATION_RESULT.INVALID_CHARS));

        rule.setSource("/contains/invalid/+/anywhere");
        assertTrue(rule.validate().getResult().equals(VALIDATION_RESULT.INVALID_CHARS));

        rule.setSource("/good/topic/string");
        rule.setDestination("bad!!!!QueName");
        assertTrue(rule.validate().getResult().equals(VALIDATION_RESULT.INVALID_CHARS));

        rule.setDestination("thishastoomanycharactersforaqueuenamewhichisn'tverylongactually");
        assertTrue(rule.validate().getResult().equals(VALIDATION_RESULT.VALUE_TOO_LONG));

        rule.setDestination("goodMQname");

    }

}
