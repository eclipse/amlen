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
package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

import java.text.MessageFormat;
import java.util.Enumeration;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaIllegalArgumentException;
import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaInvalidClientIDException;
import com.ibm.ima.jms.impl.ImaInvalidDestinationException;
import com.ibm.ima.jms.impl.ImaInvalidSelectorException;
import com.ibm.ima.jms.impl.ImaIOException;
import com.ibm.ima.jms.impl.ImaJmsPropertyException;
import com.ibm.ima.jms.impl.ImaJmsSecurityException;
import com.ibm.ima.jms.impl.ImaMessageEOFException;
import com.ibm.ima.jms.impl.ImaMessageFormatException;
import com.ibm.ima.jms.impl.ImaMessageNotReadableException;
import com.ibm.ima.jms.impl.ImaMessageNotWriteableException;
import com.ibm.ima.jms.impl.ImaNamingException;
import com.ibm.ima.jms.impl.ImaNoSuchElementException;
import com.ibm.ima.jms.impl.ImaNullPointerException;
import com.ibm.ima.jms.impl.ImaNumberFormatException;
import com.ibm.ima.jms.impl.ImaRuntimeException;
import com.ibm.ima.jms.impl.ImaRuntimeIllegalStateException;
import com.ibm.ima.jms.impl.ImaSecurityException;
import com.ibm.ima.jms.impl.ImaTransactionInProgressException;
import com.ibm.ima.jms.impl.ImaTransactionRolledBackException;
import com.ibm.ima.jms.impl.ImaUnsupportedOperationException;
import com.ibm.ima.jms.impl.ImaXAException;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/*
 * Test Pseudo translation messages.
 * 
 * Arguments:
 *   language = "en", "de", "fr", "zh", "ja", etc...
 *   country  = "US", "DE", "FR", "TW", "JP", etc...
 *   defaultLanguage
 *   defaultCountry
 *   
 *   This is more or less just unit testing jms exception classes..
 *   
 *   1. Test all error codes from the bundle.
 *   2. Test all Ima*Exception classes from client_jms and client_ra with valid
 *      error code and matching message from bundle.
 *   
 *   This will need to be updated for any exception classes added to the client
 *   or RA in the future. 
 *        
 *   NOTE: NL locale is used to test a locale that is not supported.
 */
public class PseudoTranslationTester extends Action {
	private final Locale locale;
	private final Locale defaultLocale;
	private final String language;
	private final String country;
	private final String defaultLanguage;
	private final String defaultCountry;

	PseudoTranslationTester(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		defaultLanguage = _actionParams.getProperty("defaultLanguage");
		defaultCountry = _actionParams.getProperty("defaultCountry");
		language = _actionParams.getProperty("language");
		country = _actionParams.getProperty("country");
		if (language != null && country != null) {
			locale = new Locale(language, country);
		} else if (language != null) {
			locale = new Locale(language);
		} else {
			throw new RuntimeException("language must be defined for "
					+ this.toString());
		}
		if (defaultLanguage != null && defaultCountry != null) {
			defaultLocale = new Locale(defaultLanguage, defaultCountry);
		} else if (defaultLanguage != null) {
			defaultLocale = new Locale(defaultLanguage);
		} else {
			defaultLocale = new Locale("en");
		}
	}

	@Override
	protected boolean run() {
		boolean result = true;

		// Set default locale here rather than in the constructor, as multiple
		// PseudoTransTest actions will overwrite the default if set in constructor.
		Locale.setDefault(defaultLocale);
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4000", 
				"Default Locale set to: " + Locale.getDefault());
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4000", 
				"Requesting Locale set to: " + locale);

		// Load error codes and messages from root resource bundle.
		ResourceBundle bundle = ResourceBundle.getBundle("com.ibm.ima.jms.msgcatalog.IsmJmsListResourceBundle", Locale.ROOT);
		Enumeration<String> keys = bundle.getKeys();

		// Loop through all error codes in bundle and verify their translated messages.
		if (checkMessageIDs(result, bundle, keys) == false) {
			result = false;
		}

		// Test each Ima*Exception class with every constructor.
		if (checkExceptionClasses(result, bundle) == false) {
			result = false;
		}

		return result;
	}

	/*
	 * Test all error codes using ImaJmsException
	 */
	private boolean checkMessageIDs(boolean result, ResourceBundle bundle, Enumeration<String> keys) {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4001", "Testing IMA JMS Client Message ID's");

		// Loop through all message ID's from the resource bundle, creating an ImaJmsException
		// for each. Test all Locale related functions on each exception.
		while (keys.hasMoreElements()) {
			String errorcode = keys.nextElement();
			String message = (String) bundle.getObject(errorcode);
			ImaJmsException jmsexceptionA = new ImaJmsExceptionImpl(errorcode, message);
			ImaJmsException jmsexceptionNullmsg = new ImaJmsExceptionImpl(errorcode, null);
			ImaJmsException jmsexceptionB = new ImaJmsExceptionImpl(errorcode, message, (Object[])null);
			ImaJmsException jmsexceptionC = new ImaJmsExceptionImpl(errorcode, new Throwable(message), message, (Object[])null);
			if (checkException(jmsexceptionA, errorcode) == false ||
					checkException(jmsexceptionB, errorcode) == false ||
					checkException(jmsexceptionC, errorcode) == false ||
					checkException(jmsexceptionNullmsg, errorcode) == false) {
				result = false;
			}
		}
		return result;
	}

	/*
	 * Test all Ima*Exception classes besides ImaJmsExceptionImpl,
	 * and all of their constructors.
	 */
	private boolean checkExceptionClasses(boolean result, ResourceBundle bundle) {
		String errorcode = bundle.getKeys().nextElement();
		String message = (String) bundle.getObject(errorcode);

		// Test 1 Message ID with each other type of Ima*Exception
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4015", "Testing IMA JMS Client Exception Classes");

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaInvalidClientIDException");
		ImaInvalidClientIDException clientIDExA = new ImaInvalidClientIDException(errorcode, message);
		ImaInvalidClientIDException clientIDExNullmsg = new ImaInvalidClientIDException(errorcode, null);
		ImaInvalidClientIDException clientIDExB = new ImaInvalidClientIDException(errorcode, message, (Object[])null);
		if (checkException(clientIDExA, errorcode) == false ||
				checkException(clientIDExB, errorcode) == false ||
				checkException(clientIDExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaIllegalStateException");
		ImaIllegalStateException illegalStateExA = new ImaIllegalStateException(errorcode, message);
		ImaIllegalStateException illegalStateExNullsg = new ImaIllegalStateException(errorcode, null);
		ImaIllegalStateException illegalStateExB = new ImaIllegalStateException(errorcode, message, (Object[])null);
		ImaIllegalStateException illegalStateExC = new ImaIllegalStateException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(illegalStateExA, errorcode) == false ||
				checkException(illegalStateExB, errorcode) == false ||
				checkException(illegalStateExC, errorcode) == false ||
				checkException(illegalStateExNullsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaInvalidDestinationException");
		ImaInvalidDestinationException invalidDestExA = new ImaInvalidDestinationException(errorcode, message);
		ImaInvalidDestinationException invalidDestExNullMsg = new ImaInvalidDestinationException(errorcode, null);
		ImaInvalidDestinationException invalidDestExB = new ImaInvalidDestinationException(errorcode, message, (Object[])null);
		ImaInvalidDestinationException invalidDestExC = new ImaInvalidDestinationException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(invalidDestExA, errorcode) == false ||
				checkException(invalidDestExB, errorcode) == false ||
				checkException(invalidDestExC, errorcode) == false ||
				checkException(invalidDestExNullMsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaInvalidSelectorException");
		ImaInvalidSelectorException invalidSelectorExA = new ImaInvalidSelectorException(errorcode, message);
		ImaInvalidSelectorException invalidSelectorExNullmsg = new ImaInvalidSelectorException(errorcode, null);
		ImaInvalidSelectorException invalidSelectorExB = new ImaInvalidSelectorException(errorcode, message, (Object[])null);
		ImaInvalidSelectorException invalidSelectorExC = new ImaInvalidSelectorException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(invalidSelectorExA, errorcode) == false ||
				checkException(invalidSelectorExB, errorcode) == false ||
				checkException(invalidSelectorExC, errorcode) == false ||
				checkException(invalidSelectorExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaJmsPropertyException");
		ImaJmsPropertyException jmsPropertyExA = new ImaJmsPropertyException(errorcode, message);
		ImaJmsPropertyException jmsPropertyExNullMsg = new ImaJmsPropertyException(errorcode, null);
		ImaJmsPropertyException jmsPropertyExB = new ImaJmsPropertyException(errorcode, message, (Object[])null);
		ImaJmsPropertyException jmsPropertyExC = new ImaJmsPropertyException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(jmsPropertyExA, errorcode) == false ||
				checkException(jmsPropertyExB, errorcode) == false ||
				checkException(jmsPropertyExC, errorcode) == false ||
				checkException(jmsPropertyExNullMsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaJmsSecurityException");
		ImaJmsSecurityException jmsSecurityExA = new ImaJmsSecurityException(errorcode, message);
		ImaJmsSecurityException jmsSecurityExNullmsg = new ImaJmsSecurityException(errorcode, null);
		ImaJmsSecurityException jmsSecurityExB = new ImaJmsSecurityException(errorcode, message, (Object[])null);
		ImaJmsSecurityException jmsSecurityExC = new ImaJmsSecurityException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(jmsSecurityExA, errorcode) == false ||
				checkException(jmsSecurityExB, errorcode) == false ||
				checkException(jmsSecurityExC, errorcode) == false ||
				checkException(jmsSecurityExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaMessageEOFException");
		ImaMessageEOFException messageEOFExA = new ImaMessageEOFException(errorcode, message);
		ImaMessageEOFException messageEOFExNullmsg = new ImaMessageEOFException(errorcode, null);
		ImaMessageEOFException messageEOFExB = new ImaMessageEOFException(errorcode, message, (Object[])null);
		ImaMessageEOFException messageEOFExC = new ImaMessageEOFException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(messageEOFExA, errorcode) == false ||
				checkException(messageEOFExB, errorcode) == false ||
				checkException(messageEOFExC, errorcode) == false ||
				checkException(messageEOFExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaMessageFormatException");

		ImaMessageFormatException messageFormatExA = new ImaMessageFormatException(errorcode, message);
		ImaMessageFormatException messageFormatExNullmsg = new ImaMessageFormatException(errorcode, null);
		ImaMessageFormatException messageFormatExB = new ImaMessageFormatException(errorcode, message, (Object[])null);
		ImaMessageFormatException messageFormatExC = new ImaMessageFormatException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(messageFormatExA, errorcode) == false ||
				checkException(messageFormatExB, errorcode) == false ||
				checkException(messageFormatExC, errorcode) == false ||
				checkException(messageFormatExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaMessageNotReadableException");
		ImaMessageNotReadableException messageNotReadableExA = new ImaMessageNotReadableException(errorcode, message);
		ImaMessageNotReadableException messageNotReadableExNullmsg = new ImaMessageNotReadableException(errorcode, null);
		ImaMessageNotReadableException messageNotReadableExB = new ImaMessageNotReadableException(errorcode, message, (Object[])null);
		ImaMessageNotReadableException messageNotReadableExC = new ImaMessageNotReadableException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(messageNotReadableExA, errorcode) == false ||
				checkException(messageNotReadableExB, errorcode) == false ||
				checkException(messageNotReadableExC, errorcode) == false ||
				checkException(messageNotReadableExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaMessageNotWriteableException");
		ImaMessageNotWriteableException messageNotWriteableExA = new ImaMessageNotWriteableException(errorcode, message);
		ImaMessageNotWriteableException messageNotWriteableExNullmsg = new ImaMessageNotWriteableException(errorcode, null);
		ImaMessageNotWriteableException messageNotWriteableExB = new ImaMessageNotWriteableException(errorcode, message, (Object[])null);
		ImaMessageNotWriteableException messageNotWriteableExC = new ImaMessageNotWriteableException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(messageNotWriteableExA, errorcode) == false ||
				checkException(messageNotWriteableExB, errorcode) == false ||
				checkException(messageNotWriteableExC, errorcode) == false ||
				checkException(messageNotWriteableExNullmsg, errorcode) == false) {
			result = false;
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaUnsupportedOperationException");
		ImaUnsupportedOperationException unsupportedOpExA = new ImaUnsupportedOperationException(errorcode, message);
		ImaUnsupportedOperationException unsupportedOpExNullmsg = new ImaUnsupportedOperationException(errorcode, null);
		ImaUnsupportedOperationException unsupportedOpeExB = new ImaUnsupportedOperationException(errorcode, message, (Object[])null);
		if (checkException(unsupportedOpExA, errorcode) == false ||
				checkException(unsupportedOpeExB, errorcode) == false ||
				checkException(unsupportedOpExNullmsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaIllegalArgumentException");
		ImaIllegalArgumentException illegalArgExA = new ImaIllegalArgumentException(errorcode, message);
		ImaIllegalArgumentException illegalArgExNullMsg = new ImaIllegalArgumentException(errorcode, null);
		ImaIllegalArgumentException illegalArgExB = new ImaIllegalArgumentException(errorcode, message, (Object[])null);
		ImaIllegalArgumentException illegalArgExC = new ImaIllegalArgumentException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(illegalArgExA, errorcode) == false ||
				checkException(illegalArgExB, errorcode) == false ||
				checkException(illegalArgExC, errorcode) == false ||
				checkException(illegalArgExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaIOException");
		ImaIOException imaIOExA = new ImaIOException(errorcode, message);
		ImaIOException imaIOExNullMsg = new ImaIOException(errorcode, null);
		ImaIOException imaIOExB = new ImaIOException(errorcode, message, (Object[])null);
		ImaIOException imaIOExC = new ImaIOException(errorcode, new Exception(message), message);
		if (checkException(imaIOExA, errorcode) == false ||
				checkException(imaIOExB, errorcode) == false ||
				checkException(imaIOExC, errorcode) == false ||
				checkException(imaIOExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaNamingException");
		ImaNamingException imaNamingExA = new ImaNamingException(errorcode, message);
		ImaNamingException imaNamingExNullMsg = new ImaNamingException(errorcode, null);
		ImaNamingException imaNamingExB = new ImaNamingException(errorcode, message, (Object[])null);
		if (checkException(imaNamingExA, errorcode) == false ||
				checkException(imaNamingExB, errorcode) == false ||
				checkException(imaNamingExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaNoSuchElementException");
		ImaNoSuchElementException imaNoSuchEleExA = new ImaNoSuchElementException(errorcode, message);
		ImaNoSuchElementException imaNoSuchEleExNullMsg = new ImaNoSuchElementException(errorcode, null);
		ImaNoSuchElementException imaNoSuchEleExB = new ImaNoSuchElementException(errorcode, message, (Object[])null);
		if (checkException(imaNoSuchEleExA, errorcode) == false ||
				checkException(imaNoSuchEleExB, errorcode) == false ||
				checkException(imaNoSuchEleExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaNullPointerException");
		ImaNullPointerException imaNullPtrExA = new ImaNullPointerException(errorcode, message);
		ImaNullPointerException imaNullPtrExNullMsg = new ImaNullPointerException(errorcode, null);
		ImaNullPointerException imaNullPtrExB = new ImaNullPointerException(errorcode, message, (Object[])null);
		if (checkException(imaNullPtrExA, errorcode) == false ||
				checkException(imaNullPtrExB, errorcode) == false ||
				checkException(imaNullPtrExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaNumberFormatException");
		ImaNumberFormatException imaNumFmtExA = new ImaNumberFormatException(errorcode, message);
		ImaNumberFormatException imaNumFmtExNullMsg = new ImaNumberFormatException(errorcode, null);
		ImaNumberFormatException imaNumFmtExB = new ImaNumberFormatException(errorcode, message, (Object[])null);
		if (checkException(imaNumFmtExA, errorcode) == false ||
				checkException(imaNumFmtExB, errorcode) == false ||
				checkException(imaNumFmtExNullMsg, errorcode) == false) {
			result = false;
		}
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaXAException");
    	ImaXAException imaXAExA = new ImaXAException(errorcode, 0, message);
    	ImaXAException imaXAExNullmsg = new ImaXAException(errorcode, 0, null);
    	ImaXAException imaXAExB = new ImaXAException(errorcode, 0, message, (Object[])null);
    	ImaXAException imaXAExC = new ImaXAException(errorcode, 0, new Throwable(message), message, (Object[])null);
    	if (checkException(imaXAExA, errorcode) == false ||
    			checkException(imaXAExB, errorcode) == false ||
    			checkException(imaXAExC, errorcode) == false ||
    			checkException(imaXAExNullmsg, errorcode) == false) {
    		result = false;
    	}
    	
    	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaSecurityException");
    	ImaSecurityException imaSecExA = new ImaSecurityException(errorcode, message);
    	ImaSecurityException imaSecExNullMsg = new ImaSecurityException(errorcode, null);
    	ImaSecurityException imaSecExB = new ImaSecurityException(errorcode, message, (Object[])null);
    	ImaSecurityException imaSecExC = new ImaSecurityException(errorcode, new Throwable(message), message);
    	ImaSecurityException imaSecExD = new ImaSecurityException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(imaSecExA, errorcode) == false ||
				checkException(imaSecExB, errorcode) == false ||
				checkException(imaSecExNullMsg, errorcode) == false ||
				checkException(imaSecExC, errorcode) == false ||
				checkException(imaSecExD, errorcode) == false) {
			result = false;
		}
    	
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaTransactionInProgressException");
		ImaTransactionInProgressException imaTxInProgExA = new ImaTransactionInProgressException(errorcode, message);
		ImaTransactionInProgressException imaTxInProgExNullMsg = new ImaTransactionInProgressException(errorcode, null);
		ImaTransactionInProgressException imaTxInProgExB = new ImaTransactionInProgressException(errorcode, message, (Object[])null);
		ImaTransactionInProgressException imaTxInProgExC = new ImaTransactionInProgressException(errorcode, new Throwable(message), message, (Object[])null);
    	if (checkException(imaTxInProgExA, errorcode) == false ||
				checkException(imaTxInProgExB, errorcode) == false ||
				checkException(imaTxInProgExNullMsg, errorcode) == false ||
				checkException(imaTxInProgExC, errorcode) == false) {
			result = false;
		}
		
    	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaRuntimeIllegalStateException");
    	ImaRuntimeIllegalStateException imaRuntimeIllExA = new ImaRuntimeIllegalStateException(errorcode, message);
    	ImaRuntimeIllegalStateException imaRuntimeIllExNullMsg = new ImaRuntimeIllegalStateException(errorcode, null);
    	ImaRuntimeIllegalStateException imaRuntimeIllExB = new ImaRuntimeIllegalStateException(errorcode, message, (Object[])null);
    	ImaRuntimeIllegalStateException imaRuntimeIllExC = new ImaRuntimeIllegalStateException(errorcode, new Throwable(message), message, (Object[])null);
		if (checkException(imaRuntimeIllExA, errorcode) == false ||
				checkException(imaRuntimeIllExB, errorcode) == false ||
				checkException(imaRuntimeIllExC, errorcode) == false ||
				checkException(imaRuntimeIllExNullMsg, errorcode) == false) {
			result = false;
		}
		
		// ImaTransactionRolledBackException is hard coded to CWLNC0222 and has a slightly different constructor
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaTransactionRolledBackException");
		errorcode = "CWLNC0222";
		ImaTransactionRolledBackException transRBEx = new ImaTransactionRolledBackException(1);
		if (checkException(transRBEx, errorcode) == false) {
			result = false;
		}

		// For runtime exception, test the linked exception
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4014", "ImaRuntimeException");
		Throwable imaRuntimeEx = ImaRuntimeException.mapException(transRBEx);
		Exception e = (Exception) imaRuntimeEx.getCause();
		if (e instanceof ImaJmsException) {
			if (checkException((ImaJmsException)e, errorcode) == false) {
				result = false;
			}
		}

		return result;
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
	private boolean checkException(ImaJmsException jmse, String errorcode) {
		boolean result = true;

		// messageFromBundle is for comparing ImaJmsException.getMessage(Locale) output
		String messageFromBundle = getMessageFromBundle(errorcode, locale);
		if (!(messageFromBundle.contains("~" + locale.getLanguage())) &&
				!(locale.getLanguage().equals("en")) &&
				!(locale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4014", 
					"messageFromBundle is not the correct Locale, " + messageFromBundle);
			result = false;
		}

		// formatFromBundle is for comparing ImaJmsException.getMessageFormat(Locale) output
		String formatFromBundle = getMessageFromBundle(errorcode, locale);
		if (!(formatFromBundle.contains("~" + locale.getLanguage())) &&
				!(locale.getLanguage().equals("en")) &&
				!(locale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4014", 
					"formatFromBundle is not the correct Locale, " + formatFromBundle);
			result = false;
		}

		// defaultFromBundle is for comparing ImaJmsException.getLocalizedMessage() output
		String defaultFromBundle = getMessageFromBundle(errorcode, defaultLocale);
		if (!(defaultFromBundle.contains("~" + defaultLocale.getLanguage())) &&
				!(defaultLocale.getLanguage().equals("en")) &&
				!(defaultLocale.getLanguage().equals("nl"))) {
			// we didn't get the message from the expected bundle
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4014", 
					"defaultFromBundle is not the correct Locale, " + defaultFromBundle);
			result = false;
		}

		// englishFromBundle is for comparing ImaJmsException.getMessage() output
		String englishFromBundle = getMessageFromBundle(errorcode, Locale.ROOT);
		if (englishFromBundle.contains("~~~")) {
			// we didn't get the message from the expected bundle
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4014", 
					"englishFromBundle is not the correct Locale, " + englishFromBundle);
			result = false;
		}

		// englishFormatFromBundle is for comparing ImaJmsException.getMessage() output
		String englishFormatFromBundle = getMessageFromBundle(errorcode, Locale.ROOT);
		if (englishFormatFromBundle.contains("~~~")) {
			// we didn't get the message from the expected bundle
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4014", 
					"englishFormatFromBundle is not the correct Locale, " + englishFormatFromBundle);
			result = false;
		}

		// Special case for ImaTransactionRolledBackException
		if (jmse instanceof ImaTransactionRolledBackException) {
			Object[] replData = { 1 };
			messageFromBundle = new MessageFormat(messageFromBundle).format(replData);
			defaultFromBundle = new MessageFormat(defaultFromBundle).format(replData);
			englishFromBundle = new MessageFormat(englishFromBundle).format(replData);
		}

		if (messageFromBundle.equals(jmse.getMessage(locale))) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4002", 
					"Message " + errorcode + " matched for locale: " + locale);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD4003", 
					"getMessage(Locale): " + jmse.getMessage(locale));
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4004", 
					"FAILED: getMessage(Locale) EXPECTED: " + messageFromBundle);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4004", 
					"FAILED: getMessage(Locale) ACTUAL:   " + jmse.getMessage(locale));
			result = false;
		}

		if (formatFromBundle.equals(jmse.getMessageFormat(locale))) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4005", 
					"Message " + errorcode + " format matched for locale: " + locale);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD4006", 
					"getMessageFormat(Locale): " + jmse.getMessageFormat(locale));
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4007", 
					"FAILED: getMessageFormat(Locale) EXPECTED: " + formatFromBundle);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4007", 
					"FAILED: getMessageFormat(Locale) ACTUAL:   " + jmse.getMessageFormat(locale));
			result = false;
		}

		if (defaultFromBundle.equals(jmse.getLocalizedMessage())) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4008", 
					"Message " + errorcode + " matched for default locale: " + defaultLocale);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD4009", 
					"getLocalizedMessage(): " + jmse.getLocalizedMessage());
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4010", 
					"FAILED: getLocalizedMessage() EXPECTED: " + defaultFromBundle);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4010", 
					"FAILED: getLocalizedMessage() ACTUAL:   " + jmse.getLocalizedMessage());
			result = false;
		}

		if (englishFromBundle.equals(jmse.getMessage())) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4011", 
					"Message " + errorcode + " matched for root locale (en_US)");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD4012", 
					"getMessage(): " + jmse.getMessage());
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4013", 
					"FAILED: getMessage() EXPECTED: " + englishFromBundle);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4013",
					"FAILED: getMessage() ACTUAL:   " + jmse.getMessage());
			result = false;
		}

		if (englishFormatFromBundle.equals(jmse.getMessageFormat())) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD4011", 
					"Message " + errorcode + " format matched for root locale (en_US)");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD4012", 
					"getMessageFormat(): " + jmse.getMessageFormat());
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4013", 
					"FAILED: getMessageFormat() EXPECTED: " + englishFormatFromBundle);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD4013",
					"FAILED: getMessageFormat() ACTUAL:   " + jmse.getMessageFormat());
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
