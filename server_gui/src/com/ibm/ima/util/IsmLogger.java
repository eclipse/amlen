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

package com.ibm.ima.util;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.text.MessageFormat;
import java.util.HashMap;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * General logger to help insulate the server components from the logging implementation.
 * 
 * The current implementation uses java.util.logging, which Liberty intercepts and manages.
 * Liberty logging properties may be controlled via server.xml file.
 * 
 * See
 * http://pic.dhe.ibm.com/infocenter/wasinfo/v8r5/index.jsp?topic=%2Fcom.ibm.websphere.wlp.nd.doc%2Ftopics%2Frwlp_logging.html
 * 
 * 
 *
 */
public class IsmLogger {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static HashMap<String, IsmLogger> loggers = new HashMap<String, IsmLogger>();

    private final static String CLAS = IsmLogger.class.getCanonicalName();

    /**
     * The component name to use for the Web UI component
     */
    public final static String WEB_UI_COMPONENT_NAME = "com.ibm.ima.gui";

    /**
     * The ResourceBundle name to use for the Web UI component
     */
    public final static String WEB_UI_RESOURCE_BUNDLE_NAME = "com.ibm.ima.msgcatalog.IsmUIListResourceBundle";


    /**
     * The Log Levels being used by ISM. Use the trace methods for trace messages.
     */
    public static enum LogLevel {
        LOG_CRIT     ('C', Level.SEVERE),  // Critical conditions
        LOG_ERR      ('E', Level.SEVERE),  // Error conditions
        LOG_WARNING  ('W', Level.WARNING), // Warning conditions
        LOG_NOTICE   ('N', Level.INFO),    // Normal but significant condition
        LOG_INFO     ('I', Level.INFO);    // Informational

        public final char severityChar;
        public final Level javaLogLevel;
        LogLevel(char severityChar, Level javaLogLevel) {
            this.severityChar = severityChar;
            this.javaLogLevel = javaLogLevel;
        }
    }

    private Logger logger = null;
    private ResourceBundle resourceBundle = null;
    private String resourceBundleName = null;

    // Current implementation creates a java.util.Logger
    private IsmLogger (String componentName, String resourceBundleName) {
        try {
            this.resourceBundleName = resourceBundleName;
            logger = Logger.getLogger(componentName, null);
            resourceBundle = ResourceBundle.getBundle(resourceBundleName);
        } catch (MissingResourceException mre) {
            if (logger != null) {
                logger.logp(Level.WARNING, CLAS, "getLogger", "Could not load MessageBundle " + resourceBundleName, mre);
            } 
        } catch (Throwable t) {
            System.err.println("Could not get the logger for " + componentName);
        }

    }

    /**
     * Convenience method to get the Logger for the Web UI component.
     * @return The logger for the Web UI component
     */
    public static IsmLogger getGuiLogger() {
        return IsmLogger.getLogger(IsmLogger.WEB_UI_COMPONENT_NAME, IsmLogger.WEB_UI_RESOURCE_BUNDLE_NAME);
    }

    /**
     * Get a logger for the specified component and resource bundle
     * @param componentName The name of the component to get the logger for.
     * @param resourceBundleName The name of the resource bundle for the component.
     * @return The logger for the specified component
     */
    public static IsmLogger getLogger(String componentName, String resourceBundleName) {
        IsmLogger logger = loggers.get(componentName);
        if (logger == null) {
            try {
                logger = new IsmLogger(componentName, resourceBundleName);
            } catch (MissingResourceException mre) {
                logger = new IsmLogger(componentName, null);
                logger.log(LogLevel.LOG_WARNING, CLAS, "getLogger", "Could not load MessageBundle " + resourceBundleName, mre);
            } catch (Throwable t) {
                System.err.println("Could not get the logger for " + componentName);
            }
            loggers.put(componentName, logger);
        }
        return logger;
    }

    /*
     * Determine if the specified LogLevel is loggable, based on the current logging level
     * for this logger.
     * @param logLevel the LogLevel to check
     * @return true if a message at that level would get logged, false otherwise
     */
    public boolean isLoggable(LogLevel logLevel) {
        return logger.isLoggable(logLevel.javaLogLevel);
    }

    /**
     * Log a message
     * @param logLevel     The level for the message
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param messageID    The message ID in the resource bundle
     */
    public void log(LogLevel logLevel, String sourceClass, String sourceMethod, String messageID) {
        Level level = logLevel != null ? logLevel.javaLogLevel : Level.WARNING;
        logger.logp(level, sourceClass, sourceMethod, formatMessage(sourceClass,sourceMethod, messageID, null));
    }

    /**
     * Log a message with variables
     * @param logLevel     The level for the message
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param messageID    The message ID in the resource bundle
     * @param params       The substitution variables for the message
     */
    public void log(LogLevel logLevel, String sourceClass, String sourceMethod, String messageID, Object[] params) {
        Level level = logLevel != null ? logLevel.javaLogLevel : Level.WARNING;
        logger.logp(level, sourceClass, sourceMethod, formatMessage(sourceClass,sourceMethod, messageID, params));
    }

    /**
     * Log a message with an exception
     * @param logLevel     The level for the message
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param messageID    The message ID in the resource bundle
     * @param thrown       The exception
     */
    public void log(LogLevel logLevel, String sourceClass, String sourceMethod, String messageID, Throwable thrown) {
        this.log(logLevel, sourceClass, sourceMethod, messageID, null, thrown);
    }

    /**
     * Log a message with an exception
     * @param logLevel     The level for the message
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param messageID    The message ID in the resource bundle
     * @param params       The substitution variables for the message
     * @param thrown       The exception
     */
    public void log(LogLevel logLevel, String sourceClass, String sourceMethod, String messageID, Object[] params, Throwable thrown) {
        Level level = logLevel != null ? logLevel.javaLogLevel : Level.WARNING;
        logger.logp(level, sourceClass, sourceMethod, formatMessage(sourceClass,sourceMethod, messageID, params), thrown);
        if (isTraceEnabled()) {
            try {
                // only output exceptions to the trace log for security reasons
                trace(sourceClass, sourceMethod, getStackTrace(thrown));
            } catch (IOException ioe) { /* eat it */ }
        }
    }


    /**
     * Send a trace entry message to the trace log
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     */
    public void traceEntry(String sourceClass, String sourceMethod) {
        logger.entering(sourceClass, sourceMethod);
    }

    /**
     * Send a trace entry message to the trace log
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param params       Parameters to log
     */
    public void traceEntry(String sourceClass, String sourceMethod, Object[] params) {
        logger.entering(sourceClass, sourceMethod, params);
    }

    /**
     * Send a trace exit message to the trace log
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     */
    public void traceExit(String sourceClass, String sourceMethod) {
        logger.exiting(sourceClass, sourceMethod);
    }

    /**
     * Send a trace exit message to the trace log
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param result       Result to log
     */
    public void traceExit(String sourceClass, String sourceMethod, Object result) {
        logger.exiting(sourceClass, sourceMethod, result);
    }

    /**
     * Send a trace message to the trace log
     * @param sourceClass  The source Class issuing the message
     * @param sourceMethod The source method issuing the message
     * @param message      Message to log
     */
    public void trace(String sourceClass, String sourceMethod, String message) {
        logger.fine("[" + sourceClass + "][" + sourceMethod + "] "+ message);
    }

    /**
     * Determine if trace is enabled.
     * 
     * @return true if enabled, false otherwise
     */
    public boolean isTraceEnabled() {
        return logger.isLoggable(Level.FINE);
    }

    /**
     * Gets the messageID from the resourceBundle for the specified locale and formats
     * it with any specfied parameters. If prefixMessageID is true, prefixes it with the messageID.
     * @param messageID  The messageID
     * @param params Optional array of parameters
     * @param locale Locale to retrieve
     * @param prefixMessageID whether or not to add the messageID prefix to the message.
     * @return The message or null if not found.
     */
    public String getFormattedMessage(String messageID, Object[] params, Locale locale, boolean prefixMessageID) {
        String message = null;
        if (locale == null) {
            locale = Locale.getDefault();
        }
        ResourceBundle bundle = null;
        try {
            bundle = ResourceBundle.getBundle(resourceBundleName, locale);
            message = bundle.getString(messageID);
        } catch (MissingResourceException e) {
            if (bundle == null) {
                log(LogLevel.LOG_WARNING, CLAS, "getFormattedMessage", "Could not load MessageBundle " + resourceBundleName, e);
            } else {
                log(LogLevel.LOG_WARNING, CLAS, "getFormattedMessage", "CWLNA5008", new Object[] {messageID, resourceBundleName, locale} , e);
            }
        }

        if (message != null && params != null) {
            message = message.replace("'", "''");
        	message = new MessageFormat(message).format(params) ;
            if (prefixMessageID) {
                message = messageID + ": " + message;
            }
        }
        return message;
    }

    private String getStackTrace(Throwable cause) throws IOException {
        final Writer writer = new StringWriter();
        final PrintWriter printWriter = new PrintWriter(writer);
        cause.printStackTrace(printWriter);
        String result = writer.toString();
        printWriter.close();
        writer.close();
        return result;
    }


    /**
     * Adds the message ID in front of the message and formats the message with any parameters
     * @param sourceClass The source class
     * @param sourceMethod The source method
     * @param messageID  The message ID
     * @param params     The parameters, or null if there aren't any
     * @return The formatted string with message ID and severity character
     */
    private String formatMessage(String sourceClass, String sourceMethod, String messageID, Object[] params)
    {
        String message = messageID + ":  ";

        if (resourceBundle != null) {
            try {
                String messageFromBundle = resourceBundle.getString(messageID);
                messageFromBundle = messageFromBundle.replace("'", "''");
                message += messageFromBundle;
            }
            catch (MissingResourceException e)  {
                // we'll just use the messageID (no severity since it could be a random string)
            }
        }

        if (params != null) {
            message = new MessageFormat(message).format(params);
        }

        // TODO can we get liberty to include this by default? I don't see how to do it. May need to create a logger for every class...
        message += " [" + sourceClass + "." + sourceMethod + "]";

        return message;
    }

}
