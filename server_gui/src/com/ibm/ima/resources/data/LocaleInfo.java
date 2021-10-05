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

import com.ibm.icu.util.StringTokenizer;
import com.ibm.icu.util.ULocale;

public class LocaleInfo implements Comparable<LocaleInfo> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    String locale = null;
    String displayName = null;
    
    /**
     * @param locale
     * @param displayName
     */
    public LocaleInfo(String locale, String displayName) {
        super();
        this.locale = locale;
        this.displayName = displayName;
    }
    
    
    
    /**
     * @return the locale
     */
    public String getLocale() {
        return locale;
    }



    /**
     * @param locale the locale to set
     */
    public void setLocale(String locale) {
        this.locale = locale;
    }



    /**
     * @return the displayName
     */
    public String getDisplayName() {
        return displayName;
    }
    
    
    /**
     * @param displayName the displayName to set
     */
    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }
    
    /**
     * Parses a localeStr and returns the locale. 
     * @param localeStr string to parse
     * @return locale from localeStr or default locale if the localeStr cannot be parsed.
     */
    public static ULocale parseLocale(String localeStr) {
        ULocale locale = ULocale.getDefault();
        if (localeStr != null) {
            StringTokenizer st = new StringTokenizer(localeStr, "_");
            if (st.hasMoreTokens()) {
                String sLang = st.nextToken();
                if (st.hasMoreTokens()) {
                    String sCountry = st.nextToken();
                    locale = new ULocale(sLang, sCountry);
                } else {
                    locale = new ULocale(sLang);
                }
            }
        }
        return locale;
        
    }


    @Override
    public int compareTo(LocaleInfo o) {
        String a = displayName;
        String b = o.getDisplayName();
        if (a != null && b!= null) {
            return a.compareTo(b);
        }             
        return 0;
    }
   

}
