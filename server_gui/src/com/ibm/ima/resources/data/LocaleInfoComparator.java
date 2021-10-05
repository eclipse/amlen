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
package com.ibm.ima.resources.data;

import java.util.Comparator;
import java.util.Locale;

import com.ibm.icu.text.Collator;

public class LocaleInfoComparator implements Comparator<LocaleInfo> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    private Collator c = null;
    
    public LocaleInfoComparator(Locale locale) {
        c = Collator.getInstance(locale);
    }

    @Override
    public int compare(LocaleInfo object1, LocaleInfo object2) {
        if (object1 != null && object2 != null) {
            String a = object1.getDisplayName();
            String b = object2.getDisplayName();    
            return c.compare(a, b);
        }
        return 0;
    }
    

}
