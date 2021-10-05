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

package com.ibm.ima.jms;

import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.Locale;

/**
 * Defines an interface with common extensions to IBM MessageSight JMS client exceptions which
 * allow additional information to be returned for these exceptions.
 * 
 * All objects which implement this interface also extend Throwable and many 
 * of the common Throwable methods are included here for convenience.
 * 
 */
public interface ImaJmsException {   

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /**
     * Gets the error code string.
     * @return The error code which is a character string which uniquely identifies the exception message.
     */
    public String getErrorCode();
    
    /**
     * Gets the message as the string form of a message format.
     * @return The untranslated message format in string form.
     */
    public String getMessageFormat();
    
    /**
     * Gets the message as the string form of a message format in the specified locale.
     * If the locale is null, return the untranslated message format.
     * If the message format is unavailable for the specified locale, use normal locale 
     * search to find the best available version of the message format based on 
     * the locale.
     * @param locale  The locale in which to return the message format.
     * @return The message format in string form in the specified locale.
     */
    public String getMessageFormat(Locale locale);
   
    /**
     * Gets the replacement data for the exception message.
     * @return An array of replacement values for the message.  This can be null
     * in the case there are no replacement values in the message.
     */
    public Object [] getParameters();
    
    /**
     * Gets the error code as an integer.
     * The integer message type is the value of the trailing digits in the error code.
     * @return The integer message type.
     */
    public int getErrorType();
    
    /**
     * Gets the cause of the exception.
     * This is a convenience method with the  same result as the common getCause() of Throwable.
     * @return The cause of the exception or null if the cause is not a Throwable.
     */
    public Throwable getCause();
    
    /**
     * Gets the formatted message.
     * @return A fully formatted message.
     */
    public String getMessage();

    
    /**
     * Gets the formatted message in the specified locale.
     * If the locale is null, return the untranslated formatted message.
     * If the message format is unavailable for the specified locale, use normal locale 
     * search to find the best available version of the message format based on 
     * the locale.
     * @return A fully formatted message.
     */
    public String getMessage(Locale locale);
    
    /**
     * Gets the formatted message in the default locale.
     * If the message format is unavailable for the default locale, use normal locale 
     * search to find the best available version of the message format based on 
     * the locale.
     * @return A fully formatted message.
     */
    public String getLocalizedMessage();
    
    /**
     * Gets the stack trace for this object.
     * Each stack trace element represents an entry on the call stack, with the
     * first value in the array representing the most recent call.
     * @return An array of stack trace elements.
     */
    public StackTraceElement[] getStackTrace();        
    
    /**
     * Prints the backtrace to the specified print stream.
     * @param s PrintStream to use for the output
     */
    public void printStackTrace(PrintStream s);        
    
    /**
     * Prints the backtrace to the specified print writer.
     * @param s PrintWriter to use for the output
     */
    public void printStackTrace(PrintWriter s);
}
