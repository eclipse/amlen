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
package com.ibm.ima.jca.servlet;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Scanner;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.ima.jms.test.jca.Utils;

public final class ServletUtils {
	public static int parseURLParam(String params) throws IOException {
		if(params == null)
			return -1;
		Matcher m = Pattern.compile("param=([\\d]+)").matcher(params);
		if(!m.find())
			return -1;
		Integer i = Integer.parseInt(m.group(1));
		return i;
	}
	
	public static String[] getApplianceIPs() throws FileNotFoundException {
		/*
         * If java security is enabled, the properties file needs to be opened
         * in a privileged action
         */
        FileInputStream f = AccessController.doPrivileged(new PrivilegedAction<FileInputStream>() {
            public FileInputStream run() {
                try {
                    return new FileInputStream(Utils.WASPATH() + "/env.file");
                } catch (Exception e) {
                    return null;
                }
            }
        });
        if(f == null)
            throw new FileNotFoundException(Utils.WASPATH() + "/env.file does not exist");
	
        Scanner s = new Scanner(f);
        String a1 = s.nextLine();
        String a2 = s.nextLine();
        s.close();
        
        
        
        return new String[] {a1, a2};
	}
}
