/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

import java.text.MessageFormat;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

import javax.jms.JMSException;

import com.ibm.ima.jms.ImaJmsException;

/**
 *
 */
public class ImaJmsExceptionImpl extends JMSException implements ImaJmsException{

    private static final long serialVersionUID = -3003473974517039239L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /**
     * The ResourceBundle name to use for the JMS client and RA components
     */
    public final static String JMS_RESOURCE_BUNDLE_NAME = "com.ibm.ima.jms.msgcatalog.IsmJmsListResourceBundle";
    
    private static String resourceBundleName = JMS_RESOURCE_BUNDLE_NAME;
    
    private Object [] repl  = null;
    //private Object [] repllist = null;
    private String msgfmt = null;

    /*
     * Create a JMSException with a reason string with no replacement data.
     * @param errorcode  The error code for the exception
     * @param reason     The reason as a Java message format
     */
    public ImaJmsExceptionImpl(String errorcode, String reason ) {
        super(formatException(reason, null, errorcode), errorcode);
        msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reason);
    }
    

    /*
     * Create a JMSException with a reason message.
     * @param errorcode  The error code for the exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaJmsExceptionImpl(String errorcode, String reasonmsg, Object ... repl ) {
        super(formatException(reasonmsg, repl, errorcode), errorcode);
        //this.repllist = repl;
        this.repl = repl;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reasonmsg);
    }
    
    /*
     * Create a JMSException with a cause and reason message.
     * @param errorcode  The error code for the exception
     * @param cause      An exception to link to this exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaJmsExceptionImpl(String errorcode, Throwable cause, String reasonmsg, Object ... repl) {
        super(formatException(reasonmsg, repl, errorcode), errorcode);
        //this.repllist = repl;
        this.repl = repl;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reasonmsg);
        if (cause != null) {
            if (cause instanceof Exception)
                setLinkedException((Exception)cause);
            initCause(cause);
        }    
    }
    
    /*
     * Format the message into a reason string.
     * @param reasonmsg  The reason as a Java message format
     * @param repl       A single replacement data
     */
    /*
    public static String formatException(String msg, Object repl) {
        try {
            return MessageFormat.format(msg, new Object [] {repl});
        } catch (Exception e) {
            return msg;
        }
    }
    */
    /*
     * Format the message into a reason string.
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public static String formatException(String msg, Object [] repl, String errCode) {
    	String message = getMessageFromBundle(Locale.ROOT, errCode, msg);
        try {
            return MessageFormat.format(message, repl);
        } catch (Exception e) {
            return message;
        }    
    }
    
    /*
     * Get the replacement parameters.
     * @return An array of objects which are used as the replacement data for the exception.
     * This can be null if there is no replacement data. 
     */
    public Object [] getParameters() {
    	/*
        if (repllist == null && repl != null) {
            repllist = new Object [] {repl};
        }
        return repllist;
        */
    	return repl;
    }
    
    /*
     * Common implementation method
     */
    public static int errorType(String ec) {
        int  code = 0;
        if (ec != null) {
            for (int pos=5; pos<ec.length(); pos++) {
                char ch = ec.charAt(pos);
                if (ch >= '0' && ch <= '9') 
                    code = code * 10 + (ch-'0');
                else
                    break;
            }
        }    
        return code; 
    }
    
    /*
     * Common implementation method
     */
    public static String getMsg(ImaJmsException lje, Locale locale) {
        String message = null;
        
        message = getMsgFmt(lje, locale);
        
        if (message != null && lje.getParameters() != null) {
            message = new MessageFormat(message).format(lje.getParameters()) ;
        }
        
        return message;
    }
    
    /*
     * Common implementation method
     */
    public static String getMsgFmt(ImaJmsException lje, Locale locale) {
        String message = null;
        if (locale == null) {
            locale = Locale.getDefault();
        }
        
        ResourceBundle bundle = null;
        try {
            bundle = ResourceBundle.getBundle(ImaJmsExceptionImpl.resourceBundleName, locale);
            if (lje.getErrorCode() != null) {
	            message = bundle.getString(lje.getErrorCode());
	            message = message.replace("'", "''"); 
            }
        } catch (MissingResourceException e) {
        	message = lje.getMessageFormat();
        }
        
        if (message == null)
        	message = lje.getMessageFormat();
        
        return message;
    }       

    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsExceptionImpl.errorType(getErrorCode());
    }    
    
    /*
     * Return the detail message for the exception.
     * @see com.ibm.ima.jms.ImaJmsException#getMessage()
     */
    public String getMessage() {
    	return getMessage(Locale.ROOT);
    }

    /*
     * Return the detail message for the exception in the specified locale.
     * @see com.ibm.ima.jms.ImaJmsException#getMessage(java.util.Locale)
     */
    public String getMessage(Locale locale) {
    	return getFormattedMessage(locale);
    }
    
    /*
     * Return the detail message for the exception in the default locale.
     * @see com.ibm.ima.jms.ImaJmsException#getMessage(java.util.Locale)
     */
    public String getLocalizedMessage() {
    	return getMessage(Locale.getDefault());
    }
    

    /*
     * Return the untranslated message format.
     * @see com.ibm.ima.jms.ImaJmsException#getMessageFormat()
     */
    public String getMessageFormat() {
        return msgfmt;
    }

    /*
     * Return the message format for the specified locale.
     * @see com.ibm.ima.jms.ImaJmsException#getMessageFormat(java.util.Locale)
     */
    public String getMessageFormat(Locale locale) {
        return getFormat(locale);
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "JMSException: " + getErrorCode() + " " + getMessage();
    }
    
    /*
     * Helper method that retrieves error messages from the appropriate catalog.  
     * If the input locale is null, the current default locale is used.  If the default
     * locale is not supported, the message data is retrieved from the base message catalog.
     * If the message does not exist in the message catalog or if the message catolog is not found, 
     * the hard coded message provided in the source code is used.
     */
    private String getFormattedMessage(Locale locale) {
        String message = null;
        
        message = getFormat(locale);
        
        if (message != null && repl != null) {
            message = new MessageFormat(message).format(repl) ;
        }
        
        return message;
    }
    
    /*
     * Helper method that retrieves error messages from the appropriate catalog.  
     * If the input locale is null, the current default locale is used.  If the default
     * locale is not supported, the message data is retrieved from the base message catalog.
     * If the message does not exist in the message catalog or if the message catolog is not found, 
     * the hard coded message provided in the source code is used.
     */
    private String getFormat(Locale locale) {
        String message = null;
        if (locale == null) {
            locale = Locale.getDefault();
        }
        
        message = getMessageFromBundle(locale, this.getErrorCode(), msgfmt);
        
        return message;
    }
    
    public static String getMessageFromBundle(Locale locale, String msgID, String msg) {
    	String message = null;
        ResourceBundle bundle = null;
        try {
            bundle = ResourceBundle.getBundle(resourceBundleName, locale);
            if(msgID != null) {
                message = bundle.getString(msgID);
                message = message.replace("'", "''");
            }
        } catch (MissingResourceException e) {
        	message = msg;
        }
        
        return message;
    }
}
