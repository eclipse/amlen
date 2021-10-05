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

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.ra.ImaRARuntimeException;
import com.ibm.ima.ra.common.ImaCommException;
import com.ibm.ima.ra.common.ImaException;
import com.ibm.ima.ra.common.ImaInvalidPropertyException;
import com.ibm.ima.ra.common.ImaResourceException;

/**
 * This servlet tests mock translation by creating each of the
 * different type of Ima*Exceptions in the IMA Resource Adapter
 * and calling the following API's for each:
 * 
 * 	getMessage()
 * 	getMessage(Locale locale)
 * 	getMessageFormat(Locale locale)
 * 	getLocalizedMessage()
 * 
 * The messages are compared directly against the resource bundle
 * of the specified locale.
 * 
 * URL Syntax: http://ip:port/appName/PseudoTranslationServlet?ln,CO,dLn,dCO,END
 * 
 * where ln = language ("en", "de", "ja", etc.)
 *       CO = country  ("US", "DE", "JP", etc.)
 *       dLn = default language
 *       dCO = default country
 *       
 * ,END is at the end of the parameters so that an empty string can be set for dCO.
 * For example, de,,de,,END would result in locale=de and defaultLocale=de
 * while de,DE,de,DE,END would result in locale=de_DE and defaultLocale=de_DE.
 * 
 * Verify success by searching for SUCCESS in output.
 * 
 */
@WebServlet("/PseudoTranslationServlet")
public class PseudoTranslationServlet extends HttpServlet {
    private static final long serialVersionUID = 1L;
    
    private Locale locale;
    private Locale defaultLocale;
    private Locale englishLocale;
    
    private PrintWriter o;
    
    /**
     * @see HttpServlet#HttpServlet()
     */
    public PseudoTranslationServlet() {
        super();
    }
    
    /**
     * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        response.setCharacterEncoding("UTF-8");
        response.setContentType("text/html; charset=UTF-8");
        o = response.getWriter();
        o.println("[PseudoTranslationServlet] Processing GET request ... ");                       

        try {
			parseURLParam(request.getQueryString());
	       	Locale.setDefault(defaultLocale);
			runPseudoTranslationTester();
			
			// Set default Locale back to english just to make sure we don't 
			// screw something up in WAS...
			Locale.setDefault(englishLocale);
        } catch (Exception e) {
			o.println(e.getMessage());
			o.println("FAILURE");
			e.printStackTrace();
		}
    }
    
    private void parseURLParam(String params) throws Exception {
        String[] locales = params.split("[,]");
        
        if (locales.length != 5) {
        	throw new Exception("Params must be \"language,country,defaultLanguage,defaultCountry,END\"");
        }
        
        // locales[0] = locale.language, locales[1] = locale.country
        if (locales[0].length() > 0 && locales[1].length() > 0) {
        	locale = new Locale(locales[0], locales[1]);
        } else if (locales[0].length() != 0) {
        	locale = new Locale(locales[0]);
        } else {
        	throw new Exception("At least locale must be specified");
        }
        
        // locales[2] = defaultLocale.language, locales[3] = defaultLocale.country
        if (locales[2].length() > 0 && locales[3].length() > 0) {
        	defaultLocale = new Locale(locales[2], locales[3]);
        } else if (locales[2].length() != 0) {
        	defaultLocale = new Locale(locales[2]);
        } else {
        	defaultLocale = new Locale("en", "US");
        }
        o.println("Requesting Locale is: " + locale.toString());
        o.println("Default Locale is: " + defaultLocale.toString());
        englishLocale = new Locale("en", "US");
    }
    
    private void runPseudoTranslationTester() throws IOException {
    	boolean result = true;
    	
    	ResourceBundle bundle = ResourceBundle.getBundle("com.ibm.ima.jms.msgcatalog.IsmJmsListResourceBundle", englishLocale);
    	String errorcode = bundle.getKeys().nextElement();
    	String message = (String) bundle.getObject(errorcode);
    	
    	// Resource Adapter Exceptions
    	
    	o.println("ImaCommException");
    	ImaCommException commExA = new ImaCommException(errorcode, message);
    	ImaCommException commExNullmsg = new ImaCommException(errorcode, null);
    	ImaCommException commExB = new ImaCommException(errorcode, message, null, (Object[])null);
    	ImaCommException commExC = new ImaCommException(errorcode, message, new Throwable(message), (Object[])null);
    	if (checkException(commExA, errorcode) == false ||
    			checkException(commExB, errorcode) == false ||
    			checkException(commExC, errorcode) == false ||
    			checkException(commExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	o.println("ImaException");
    	ImaException imaExA = new ImaException(errorcode, message);
    	ImaException imaExNullmsg = new ImaException(errorcode, null);
    	ImaException imaExB = new ImaException(errorcode, message, (Object[])null);
    	if (checkException(imaExA, errorcode) == false ||
    			checkException(imaExB, errorcode) == false ||
    			checkException(imaExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	o.println("ImaInvalidPropertyException");
    	ImaInvalidPropertyException imaInvalidPropExA = new ImaInvalidPropertyException(errorcode, message);
    	ImaInvalidPropertyException imaInvalidPropExNullmsg = new ImaInvalidPropertyException(errorcode, null);
    	ImaInvalidPropertyException imaInvalidPropExB = new ImaInvalidPropertyException(errorcode, message, (Object[])null);
    	if (checkException(imaInvalidPropExA, errorcode) == false ||
    			checkException(imaInvalidPropExB, errorcode) == false ||
    			checkException(imaInvalidPropExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	o.println("ImaResourceException");
    	ImaResourceException imaResourceExA = new ImaResourceException(errorcode, message);
    	ImaResourceException imaResourceExNullmsg = new ImaResourceException(errorcode, null);
    	ImaResourceException imaResourceExB = new ImaResourceException(errorcode, message, (Object[])null);
    	ImaResourceException imaResourceExC = new ImaResourceException(errorcode, new Throwable(message), message);
    	ImaResourceException imaResourceExD = new ImaResourceException(errorcode, new Throwable(message), message, (Object[])null);
    	if (checkException(imaResourceExA, errorcode) == false ||
    			checkException(imaResourceExB, errorcode) == false ||
    			checkException(imaResourceExC, errorcode) == false ||
    			checkException(imaResourceExD, errorcode) == false ||
    			checkException(imaResourceExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	// Check a runtime exception without a linked exception
    	o.println("ImaRARuntimeException");
    	ImaRARuntimeException imaRuntimeExA = new ImaRARuntimeException(errorcode, message);
    	ImaRARuntimeException imaRuntimeExNullmsg = new ImaRARuntimeException(errorcode, null);
    	ImaRARuntimeException imaRuntimeExB = new ImaRARuntimeException(errorcode, message, (Object[])null);
    	if (checkException(imaRuntimeExA, errorcode) == false ||
    			checkException(imaRuntimeExB, errorcode) == false ||
    			checkException(imaRuntimeExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	// Check a runtime exception with a linked exception, both the runtime and the linked exception.
    	o.println("ImaRARuntimeException linked");
    	imaRuntimeExA = new ImaRARuntimeException(errorcode, imaResourceExA, message);
    	if (checkException(imaRuntimeExA, errorcode) == false) {
    		result = false;
    	}
    	if (imaRuntimeExA.getCause() instanceof ImaJmsException) {
    		if (checkException((ImaJmsException)imaRuntimeExA.getCause(), errorcode) == false) {
    			result = false;
    		}
    	}
    	
    	if (result) {
    		o.println("SUCCESS");
    	} else {
    		o.println("FAILURE");
    	}
	}
    
    /*
     * Compare exception output with message from resource bundle.
	 * 
	 * Start by pulling the expected message for each API from the bundle,
	 * and check that they contain the proper locale string.
	 * 
	 * If locale is something other than "en", the message will contain
	 * something like "~~~~~~zh" or "~~~~~~de", etc.
	 * If locale is "en", we should not see a bunch of "~~~" in the message.
	 * 
	 * This check is mainly to make sure that we have the right messages to
	 * use in the comparisons later.
	 * 
	 * Next, actually compare the output of the exception API's with the
	 * messages from the bundles.
	 * 
	 * NOTE: "nl" is the locale used for testing a locale that is not supported.
	 * 
     * Returns true on success.
     */
	private boolean checkException(ImaJmsException jmse, String errorcode) throws IOException {
		boolean result = true;
		
		// messageFromBundle is for comparing ImaJmsException.getMessage(Locale) output
		String messageFromBundle = getMessageFromBundle(errorcode, locale);
		if (!(messageFromBundle.contains("~" + locale.getLanguage())) &&
				!(locale.getLanguage().equals("en")) &&
				!(locale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			o.println("messageFromBundle is not the correct Locale, " + messageFromBundle);
			result = false;
		}
		
		if (messageFromBundle.equals(jmse.getMessage(locale))) {
			o.println("Message " + errorcode + " matched for locale: " + locale);
			o.println("getMessage(Locale): " + jmse.getMessage(locale));
		} else {
			o.println("FAILED: getMessage(Locale) EXPECTED: " + messageFromBundle);
			o.println("FAILED: getMessage(Locale) ACTUAL:   " + jmse.getMessage(locale));
			result = false;
		}
		
		// formatFromBundle is for comparing ImaJmsException.getMessageFormat(Locale) output
		String formatFromBundle = getMessageFromBundle(errorcode, locale);
		if (!(formatFromBundle.contains("~" + locale.getLanguage())) &&
				!(locale.getLanguage().equals("en")) &&
				!(locale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			o.println("formatFromBundle is not the correct Locale, " + formatFromBundle);
			result = false;
		}
		
		if (formatFromBundle.equals(jmse.getMessageFormat(locale))) {
			o.println("Message " + errorcode + " format matched for locale: " + locale);
			o.println("getMessageFormat(Locale): " + jmse.getMessageFormat(locale));
		} else {
			o.println("FAILED: getMessageFormat(Locale) EXPECTED: " + formatFromBundle);
			o.println("FAILED: getMessageFormat(Locale) ACTUAL:   " + jmse.getMessageFormat(locale));
			result = false;
		}
		
		// defaultFromBundle is for comparing ImaJmsException.getLocalizedMessage() output
		String defaultFromBundle = getMessageFromBundle(errorcode, defaultLocale);
		if (!(defaultFromBundle.contains("~" + defaultLocale.getLanguage())) &&
				!(defaultLocale.getLanguage().equals("en")) &&
				!(defaultLocale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			o.println("defaultFromBundle is not the correct Locale, " + defaultFromBundle);
			result = false;
		}
		
		if (defaultFromBundle.equals(jmse.getLocalizedMessage())) {
			o.println("Message " + errorcode + " matched for default locale: " + defaultLocale);
			o.println("getLocalizedMessage(): " + jmse.getLocalizedMessage());
		} else {
			o.println("FAILED: getLocalizedMessage() EXPECTED: " + defaultFromBundle);
			o.println("FAILED: getLocalizedMessage() ACTUAL:   " + jmse.getLocalizedMessage());
			result = false;
		}
		
		// englishMessage is for comparing ImaJmsException.getMessage() output
		String englishFromBundle = getMessageFromBundle(errorcode, Locale.ROOT);
		if (englishFromBundle.contains("~~~")) {
			// we didn't get the message from the expected bundle
			o.println("englishFromBundle is not the correct Locale, " + englishFromBundle);
			result = false;
		}
		
		if (englishFromBundle.equals(jmse.getMessage())) {
			o.println("Message " + errorcode + " matched for root locale (en_US)");
			o.println("getMessage(): " + jmse.getMessage());
		} else {
			o.println("FAILED: getMessage() EXPECTED: " + englishFromBundle);
			o.println("FAILED: getMessage() ACTUAL:   " + jmse.getMessage());
			result = false;
		}
		
		// englishFormatFromBundle is for comparing ImaJmsException.getMessageFormat() output
		String englishFormatFromBundle = getMessageFromBundle(errorcode, Locale.ROOT);
		if (englishFormatFromBundle.contains("~~~")) {
			// we didn't get the message from the expected bundle
			o.println("englishFormatFromBundle is not the correct Locale, " + englishFormatFromBundle);
			result = false;
		}
		
		if (englishFormatFromBundle.equals(jmse.getMessageFormat())) {
			o.println("Message " + errorcode + " format matched for root locale (en_US)");
			o.println("getMessageFormat(): " + jmse.getMessageFormat());
		} else {
			o.println("FAILED: getMessageFormat() EXPECTED: " + englishFormatFromBundle);
			o.println("FAILED: getMessageFormat() ACTUAL:   " + jmse.getMessageFormat());
			result = false;
		}
		
		return result;
	}

	/*
	 * Load message for error code errorCode from the jms client resource bundle
	 * with the specified Locale.
	 * 
	 * Return null if no matching message is found.
	 */
	private String getMessageFromBundle(String errorCode, Locale locale) {
		String resourceBundleName = "com.ibm.ima.jms.msgcatalog.IsmJmsListResourceBundle";
		ResourceBundle bundle = null;
		String message = null;
        try {
            bundle = ResourceBundle.getBundle(resourceBundleName, locale);
            if(errorCode != null) {
                message = bundle.getString(errorCode);
                message = message.replace("'", "''");
            }
        } catch (MissingResourceException e) {
        	return null;
        }
        return message;
	}
    
}
