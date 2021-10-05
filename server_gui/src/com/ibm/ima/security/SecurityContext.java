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
package com.ibm.ima.security;

import java.security.Principal;
import java.util.List;
import java.util.Locale;
import java.util.ResourceBundle;


/**
 * Provides security information in the context of a REST service request.
 */
public final class SecurityContext
{
    // @CLASS-COPYRIGHT@
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * The name of the user associated with the thread.
     */
    private final String user;
    
    /**
     * The locale that should be associated with the tread.
     */
    private final Locale locale;
    
    /**
     * The name of the bundle used to confirm if locale selection
     * is acceptable.
     */
    private static final String BUNDLE_NAME = "com.ibm.ima.msgcatalog.IsmUIListResourceBundle";

    /**
     * Thread local containing contexts.
     */
    private static final ThreadLocal<SecurityContext> CONTEXT = new ThreadLocal<SecurityContext>();

    /**
     * Constructor.
     * 
     * @param user
     *            the name of the user to associate with the thread
     * @param locale
     *            the locale to associate with the thread
     */
    private SecurityContext(final String user, final Locale locale)
    {
        this.user = user;
        this.locale = locale;
    }

    /**
     * Sets the context for the current thread based on an HTTP request.
     * 
     * @param principal
     *            the HTTP servlet request
     * @param acceptableLocales
     *            a List of preferred locales from the client/browser
     */
    public static void setContext(final Principal principal, final List<Locale> acceptableLocales)
    {
        if (principal != null) {
            CONTEXT.set(new SecurityContext(principal.getName(), getClientLocale(acceptableLocales)));
        }
    }

    /**
     * Sets the context for the current thread.
     * 
     * @param user
     *            the name of the user
     */
    public static void setContext(final String user, final List<Locale> acceptableLocales)
    {
        CONTEXT.set(new SecurityContext(user, getClientLocale(acceptableLocales)));
    }

    /**
     * Gets the context for the current thread.
     * 
     * @return the security context for the current thread, or <code>null</code> if the current
     *         thread does not represent a REST service request
     */
    public static SecurityContext getContext()
    {
        return CONTEXT.get();
    }

    /**
     * Destroys the context associated with the current thread.
     */
    public static void destroy()
    {
        CONTEXT.remove();
    }

    /**
     * The user associated with the current thread of execution.
     * 
     * @return the user
     */
    public String getUser()
    {
        return user;
    }
    
    /**
     * The locale associated with the current thread of execution.
     * 
     * @return the locale
     */
    public Locale getLocale() 
    {
		return locale;
	}

	/**
     * Determine the client locale based on the httpHeaders
     * 
     * @param httpHeaders The HttpHeaders
     * @return the client locale
     */
    private final static Locale getClientLocale(List<Locale> acceptableLocales ) 
    {
    	
    	// default to English if List is null
        if (acceptableLocales == null) {	
            return Locale.ENGLISH;
        }

        /*
         * The list of locales should be in order of preference. Once
         * a valid match is found then break and return. If no match is
         * found return the default. A match is determined to be found if
         * the locale instance associated with the loaded bundle has a 
         * country String - that is a country string with a length greater
         * than zero.
         */
        
        String language = null;
        String country = null;
        for (Locale desiredLocale : acceptableLocales) {
        	
        	// keep track of the first country preference
        	if (country == null && (desiredLocale.getCountry().length() > 0)) {
        		country = desiredLocale.getCountry();
        	}
        	
         	language = desiredLocale.getLanguage();
        	if (language.length() > 0) {
        	
        		// check if there is a bundle for the language specified
        		ResourceBundle bundle = ResourceBundle.getBundle(BUNDLE_NAME, desiredLocale);
        		String bundleLocaleString = bundle.getLocale().getLanguage();
        		if (language.equals(bundleLocaleString)) {
        			// the bundle loaded matched the language
        			break;
        		}
        	
        	}
        	
        	// reset language 
        	language = null;
        	
        }
        
        // if no supported languages found use English
        if (language == null) {
        	language = "en";
        }
 
        Locale locale = null;
        
        if (country == null) {
        	locale = new Locale(language);
        } else {
        	// use the best country match if one is found
        	locale = new Locale(language, country);
        }
       
        return locale;
        
    }
    
}
