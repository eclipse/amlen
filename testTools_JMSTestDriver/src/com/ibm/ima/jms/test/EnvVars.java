/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.jms.test;

import java.util.Map;
import java.util.HashMap;

/**
 */

public final class EnvVars {

    private EnvVars() {
    }
    
    public static String replace(String orig) throws Exception {
        Map<String, String> envVars = new HashMap<String, String>();
        String temp = orig;
        while(true) {
            int begin = temp.indexOf("``");
            if(begin < 0)
                break;
            if(begin >= temp.length()-2)
                break;
            int end = temp.indexOf("``", begin + 1);
            if(end < 0)
                break;
            String alias = temp.substring(begin+2, end);
            String envValue = System.getenv(alias);
            if(envValue == null)
                throw new Exception("NonExistentEnvironmentVariable");
            envVars.put(alias, envValue);
            temp = temp.substring(end+2);            	
        }
        
        String s = new String(orig);

        for(String alias : envVars.keySet())
            s = s.replaceAll("``" + alias + "``", envVars.get(alias));

        return s;
    }
}
