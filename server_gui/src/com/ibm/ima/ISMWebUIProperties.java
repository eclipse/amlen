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
package com.ibm.ima;

import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.URL;
import java.util.Properties;

public class ISMWebUIProperties implements InvocationHandler {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /** Location of the properties file within the java packages */
    private static final String PROPERTIES_PATH = "com/ibm/ima/";
    /** Name of the .properties file */
    private static final String PROPERTIES_FILE_NAME = "ismwebui.properties";
    /** Prefix used by all properties within the .properties file. */
    private static final String PROPERTY_PREFIX = "com.ibm.ima.";

    /**
     * Reference to the Props instance using the Singleton pattern. Other methods will see this as an implementation of Props whereas in
     * actuality this is a facade over the ISMWebUIProperties class using the invocation handler method.
     */
    private static Props instance = null;

    /**
     * The properties interface. At runtime, the ISMWebUIProperties will dynamically create a proxy instance to this interface and
     * instantiate it. Calls to these methods will be automatically resolved into lookups of the matching named property in the properties
     * file.
     * 
     * "Matching named" means that the "get" prefix is removed from the method name, the next character is changed to lowercase, and the
     * string PROPERTY_PREFIX is inserted at the beginning. The resulting name is looked up in the properties file.
     * 
     * The DefaultSystemProperty annotation is used to provide default values to some properties. If the associated property does not have a
     * provided value then the value within DefaultSystemProperty is used to look up an appropriate value via System.getProperty(...)
     * 
     */
    public interface Props {

        /**
         * Reference to the location of the data directory on the file system. Directory must be exist and be writable at the time the app
         * is deployed. Must end in a slash.
         */
        @DefaultSystemProperty("java.io.tmpdir")
        String getDataDir();

        /**
         * The URL to be used for the on-the-box LDAP.
         */
        String getOnBoardLDAPURL();

        /**
         * The Authentication mode to use when connecting to the on-the-box LDAP.
         */
        String getOnBoardLDAPAuthType();

        /**
         * The Principal ID to use when authenticating with the on-the-box LDAP.
         */
        String getOnBoardLDAPPrincipalID();

        /**
         * The password to use when authenticating with the on-the-box LDAP.
         */
        String getOnBoardLDAPPassword();

        String getOnBoardLDAPWebUiUsersContext();

        String getOnBoardLDAPWebUiGroupsContext();

        String getOnBoardLDAPMessagingUsersContext();

        String getOnBoardLDAPMessagingGroupsContext();

        /**
         * The fully qualified path to the system logs directory
         * @return
         */
        String getLogsDirectory();

        /**
         * The fully qualified path to the liberty logs directory
         * @return
         */
        String getWlpLogsDirectory();
        
        /**
         * The fully qualified path to the liberty utilities directory
         * @return
         */
        String getWlpInstallDirectory();

        /**
         * The fully qualified path to the webui install directory
         * @return
         */
        String getWebUIInstallDirectory();

        /**
         * The fully qualified path to the server install directory
         *  (The bits of WebUI that need to know this only work if
         *   installed alongside the server)
         * @return
         */
        String getServerInstallDirectory();

        /**
         * The fully qualified path to the server data directory
         *  (The bits of WebUI that need to know this only work if
         *   installed alongside the server)
         * @return
         */
        String getServerDataDirectory();

        /**
         * The fully qualified path to the platform logs directory
         * @return
         */
        String getPlatformLogsDirectory();


        /**
         * The fully qualified path to the Liberty config directory
         * @return
         */
        String getLibertyConfigDirectory();

        /**
         * The ism product Name
         * @return
         */
        String getProductName();

        /**
         * The ism version
         */
        String getVersion();
    }

    /**
     * Obtain a Props instance populated from the ismwebui.properties file.
     * 
     * @return an implementation of the Props interface.
     */
    public static Props instance() {
        if (instance == null) {
            instance = (Props) Proxy.newProxyInstance(ISMWebUIProperties.class.getClassLoader(), new Class[] { Props.class }, new ISMWebUIProperties());
        }
        return instance;
    }

    private Properties properties;

    private ISMWebUIProperties() {
        properties = new Properties();
        load();
    }

    /**
     * This method is called by the dynamic proxy system whenever the property methods in Props are invoked. Other code should not call
     * this.
     */
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        String methodName = method.getName();

        String propertyName = toPropertyName(methodName);

        String result = properties.getProperty(propertyName);

        if (method.getReturnType() == String.class) {
            if ((result == null || result.equalsIgnoreCase("")) && method.isAnnotationPresent(DefaultSystemProperty.class)) {
                result = System.getProperty(method.getAnnotation(DefaultSystemProperty.class).value());
            }
            return result;
        }

        if (method.getReturnType() == int.class || method.getReturnType() == Integer.class) // Might be Integer if we want to support nulls.
        {
            if (result == null)
                return null;

            return Integer.parseInt(result);
        }

        if (method.getReturnType() == boolean.class) {
            return Boolean.parseBoolean(result);
        }

        // TODO: FAILED. LOG/THROW EXCEPTION.
        return null;
    }

    /**
     * Converts the provided method name into a property name. This is achieved by lowercasing the fourth character, dropping the first
     * three characters (which should be 'get') and appending the PROPERTY_PREFIX.
     * 
     * No checking is performed to ensure that the provided method name is in the format getXxx
     * 
     * @param methodName The method name to convert.
     * @return The resulting property name
     */
    private String toPropertyName(String methodName) {
        // character after the "get" must be lowercased
        char firstChar = Character.toLowerCase(methodName.charAt(3));
        // substring lops off the preceding "get"
        String propertyName = PROPERTY_PREFIX + firstChar + methodName.substring(4);
        return propertyName;
    }

    /**
     * Read the properties file, this is located in the root com.ibm.ima package.
     */
    private void load() {
        URL propertiesFile = this.getClass().getClassLoader().getResource(PROPERTIES_PATH + PROPERTIES_FILE_NAME);

        try {
            properties.load(propertiesFile.openStream());
        } catch (IOException e) {

        }
    }

    /**
     * Annotation used to indicate that a default value can be obtained from the system property is no value has been provided in the
     * .properties file. The Retention annotation is required so that it reamins and can be checked after the class has been converted to
     * byte code.
     * 
     */
    @Retention(RetentionPolicy.RUNTIME)
    private @interface DefaultSystemProperty {

        String value();
    }
}
