/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.proxy;

import java.util.Locale;
import java.util.Map;

/*
 * The proxy listener contains an interface provided by the proxy to allow control from Java.
 * This includes both the ability to configure and monitor the proxy.
 * <p>
 * 
 */
public interface ImaProxyListener {

    /**
     * Post a JSON object to the proxy.
     * 
     * The path generally ignored for JSON configuration but is used for error reporting.
     * 
     * The query is used to indicate special processing of the configuration.
     * 
     * @param path    The path portion of the URI
     * @param query   The query portion of the URI
     * @param json    The JSON object as a string
     * @return  A proxy result object 
     */
    public ImaProxyResult postJSON(String path, String query, String json);
    
    /**
     * Post a certificate to the proxy.
     *      
     * The path commonly represents the location of the data.  The first element of the path is
     * replaced with the base location of the data on the file system for file based configuration.
     * 
     * The query is used to indicate special processing of the configuration.
     * 
     * @param path    The path portion of the URI
     * @param query   The query portion of the URI
     * @param cert    The certificate
     * @return  A proxy result object 
     */
    public ImaProxyResult postCertificate(String path, String query, byte [] cert);
    
    /**
     * Get configuration information from the proxy as a JSON object.
     *      *      
     * The path commonly represents the location of the data. This is commonly ignored for
     * JSON configured objects.
     * 
     * The query is used to indicate the objects to get.
     * 
     * @param path   The path portion of the URI giving the location of the certificate
     * @param query  The query portion of the UIR defining any special processing
     * @return A proxy result object.
     */
    public ImaProxyResult getJSON(String path, String query);
    
    /**
     * Get a certificate from the proxy.
     * @param path   The path portion of the URI giving the location of the certificate
     * @param query  The query portion of the UIR defining any special processing
     * @return A proxy result object.
     */
    public ImaProxyResult getCertificate(String path, String query);
    
    /**
     * Indicate to the proxy that it should start normal message processing
     */
    public void startMessaging();
    
    /**
     * Terminate the proxy.
     * Calling this will cause the proxy to terminate.
     * This allows an orderly shutdown of the proxy.
     * @param rc  The return code traced and sent to the operating system.
     */
    public void terminate(int rc);
    
    /**
     * Log a message to the proxy system log.
     * @param msgid  The message identifier.  If this is a message in a catalog known to the proxy
     *               is will use the message from the message catalog instead of the specified 
     *               message format.
     * @param msgformat  The message to log as a Java MessageFormat string.
     * @param repl   Zero or more replacement values for the message format.             
     */
    public void log(String msgid, String msgformat, Object ... repl);
    
    /**
     * Disconnect any connections matching the selected criteria.
     * <p>
     * This form of disconnect is commonly used when on object is to be deleted 
     * to ensure that there are no open connections when the object is deleted.
     * The common sequence is to disable an object which stops new connections,
     * then disconnect any existing connections using that object, and then delete
     * the object.  Users have no disable, but can be deleted while there are active
     * connections, so to delete a user it can be deleted and then all connections 
     * for that user disconnected.
     * <p>
     * To be disconnected a connection must match all specified values.
     * A null match string does not participate in the selection.
     * Any of the selection strings can contain an asterisk which will match 0 or more
     * characters.
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.  
     * 
     * @param endpoint  The name of the endpoint 
     * @param server    The name of the server
     * @param tenant    The name of the tenant
     * @param user      The name of the user
     * @return The count of connections closed   
     */
    public int disconnect(String endpoint, String server, String tenant, String user);
    
    /**
     * Disconnect any connection which matches the clientID specification.
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known clientID.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param clientID  The clientID which can contain asterisks which match 0 or more characters.
     * @return The count of connections closed
     */
    public int disconnect(String clientID);
    
    /**
     * Disconnect any connection which matches the clientID specification.
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known clientID.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param clientID  The clientID which can contain asterisks which match 0 or more characters.
     * @param operationBits the permission bits
     * @return The count of connections closed
     */
    public int disconnect(String clientID, int operationBits) ;
    
    
    /**
     * Disconnect any connection which matches the userid specification for Application
     * <p>
     * This form of disconnect is used to disconnect a particular client or group of clients
     * with a known userid.  
     * <p>
     * This method returns when all of the connections have been selected to be disabled
     * so the connections might still exist when this method returns.
     *  
     * @param userID  The userID or username of the application
     * @param operationBits  The operation bits
     * @return the closed connections
     */
    public int disconnectApp(String userID, int operationBits);
    
    
    
    /**
     * Get a global configuration item.
     * @param name  The name of the config item
     * @return The value of the config item
     * @throws IllegalArgumentException if the requested item is not found.
     */
    public Object getConfigItem(String name);
    
    /**
     * Set a global configuration item.
     * Only a subset of the global configuration items can be set at runtime.
     * @param name   The name of the config item
     * @param value  The value of the config item
     * @return A return code defined in ImaProxyResult. 0=good
     * @throws IllegalArgumentException if the item cannot be set or has an invalid value
     */
    public void setConfigItem(String name, Object value);
    
    /**
     * Get the current state of an endpoint as a Map.
     * @param name   The name of the endpoint.  This can be a wildcard with an asterisk to indicate multiple endpoints.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public Map<String,Object> getEndpoint(String name);

    
    /**
     * Get the current state of a tenant as a Map.
     * @param name   The name of the tenant.  This can be a wildcard with an asterisk to indicate multiple tenants.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public Map<String,Object> getTenant(String name);

    
    /**
     * Get the current state of a server as a Map string.
     * @param name   The name of the server.  This can be a wildcard with an asterisk to indicate multiple servers.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public Map<String,Object> getServer(String name);

    
    /**
     * Get the current state of a user as a Map.
     * @param name   The name of the user.  This can be a wildcard with an asterisk to indicate multiple users.
     * @param tenant The name of the tenant.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public Map<String,Object> getUser(String name, String tenant);

    /**
     * Get a list of the defined endpoints.
     * @param match  A string to match against the endpoint name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of endpoint names
     */
    public String [] getEndpointList(String match);
    
    /**
     * Get a list of the defined tenants.
     * @param match  A string to match against the tenant name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of tenant names
     */
    public String [] getTenantList(String match);
    
    /**
     * Get a list of the defined servers.
     * @param match  A string to match against the sever name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @return An array of server names
     */
    public String [] getServerList(String match);
    
    /**
     * Get a list of the defined users.
     * @param match  A string to match against the user name,  This can contain one or more asterisks (*)
     *               to match 0 or more characters.
     * @param tenant The name of the tenant.  If this is null then global users are returned.               
     * @return An array of user names
     */
    public String [] getUserList(String match, String tenant);
    
    /**
     * Return the error string associated with a return code
     * @param rc    A return code
     * @return A with a string associated with the return code
     */
    public String getErrorString(int rc);
    
    /**
     * Return the error string associated with a return code for a specified locale 
     * @param rc      A return code
     * @param locale  The locale for the message  
     * @return A with a string associated with the return code
     */
    public String getErrorString(int rc, Locale locale);
    
    /**
     * Set an endpoint from a Map.
     * @param name   The name of the endpoint
     * @param value  The name of the value
     * @throws IllegalArgumentExcepton if one of the values is not valid.
     */
    public void setEndpoint(String name, Map<String,Object> value);
    
    /**
     * Set a tenant from a Map.
     * @param name   The name of the endpoint
     * @param value  The name of the value
     * @throws IllegalArgumentExcepton if one of the values is not valid.
     */
    public void setTenant(String name, Map<String,Object> value);
    
    /**
     * Set a server from a Map.
     * @param name   The name of the endpoint
     * @param value  The name of the value
     * @throws IllegalArgumentExcepton if one of the values is not valid.
     */
    public void setServer(String name, Map<String,Object> value);
    
    /**
     * Set a user from a Map.
     * @param name   The name of the endpoint
     * @param value  The name of the value
     * @throws IllegalArgumentExcepton if one of the values is not valid.
     */
    public void setUser(String name, String tenant, Map<String,Object> value);
    
    /**
     * Set ACL data.
     * This method allows ACL data to be set, updated, or deleted.  
     * The ACL source is a set of lines terminated by an combination of CR, LF, and NUL.
     * The first character is the operator and the rest of the line is the data.
     * '/' = comment, the data is ignored
     * '@' = create or update an ACL set, the data is the name of the ACL set
     * '!' = delete an ACL set, the data is the name of the ACL set
     * '+' = add an ACL item to the current set, the data is the item key
     * '-' = delete an ACL item from the current set, the data is the item key
     *  
     * The name of the ACL corresponds to a ClientID of a session.
     * 
     * At the start of the file or after a ! operator there must be a @ operator 
     * before any + or - operators. 
     *
     * In most cases, splitting the ACL data into multiple strings has no affect on the
     * result.  However, if an ACL is deleted but not recreated within a single call 
     * to setACL, the session associated with that ClientID will be disconnected and
     * removed. 
     * 
     * @param data  The data as a string
     */
    public void setACL(String data);
    
    /**
     * Delete an endpoint.
     * It is not an error to ask to delete an endpoint which does not exist, but the
     * return value can be used to determine if the endpoint existed before this call.
     * @param name   The name of the endpoint
     * @param force  Force the deletion even if it is in use
     * @return A boolean to indicate if the endpoint did not exist before this call.
     * @throws IllegalStateException if force=false and the endpoint is in use.
     */
    public boolean deleteEndpoint(String name, boolean force);
    
    /**
     * Delete a tenant.
     * It is not an error to ask to delete a tenant which does not exist, but the
     * return value can be used to determine if the tenant existed before this call.
     * @param name   The name of the tenant
     * @param force  Force the deletion even if it is in use
     * @return A boolean to indicate if the tenant did not exist before this call.
     * @throws IllegalStateException if force=false and the tenant is in use.
     */
    public boolean deleteTenant(String name, boolean force);
    /**
     * Delete a server.
     * It is not an error to ask to delete a server which does not exist, but the
     * return value can be used to determine if the server existed before this call.
     * @param name   The name of the server
     * @param force  Force the deletion even if it is in use
     * @return A boolean to indicate if the server did not exist before this call.
     * @throws IllegalStateException if force=false and the server is in use.
     */
    public boolean deleteServer(String name, boolean force);
    
    /**
     * Delete a user.
     * It is not an error to ask to delete a user which does not exist, but the
     * return value can be used to determine if the user existed before this call.
     * @param name   The name of the user
     * @param tenant The tenant for this user
     * @param force  Force the deletion even if it is in use
     * @return A boolean to indicate if the user did not exist before this call.
     * @throws IllegalStateException if force=false and the user is in use.
     */
    public boolean deleteUser(String name, String tenant, boolean force);
    
    /**
     * Return an obfuscated password for a specified user and password.
     * The proxy normally only stored the obfuscated form of the password.
     * <p>Initially, only type 1 is supported.
     * 
     * @param user      The name of the user
     * @param password  The password in clear text
     * @param type      The type of the output password (1=SHA256)
     */
    public String  getObfuscatedPassword(String user, String password, int type);
    
    /**
     * Get the current statistics of an endpoint as a JSON string.
     * <p>
     * If there are no matching objects an JSON object with no contents will be returned.
     * @param name   The name of the emd[pomt.  This can be a wildcard with an asterisk to indicate multiple endpoints.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public String getEndpointStats(String name);
    
    /**
     * Get the current statistics of a server as a JSON string.
     * <p>
     * If there are no matching objects an JSON object with no contents will be returned.
     * @param name   The name of the sever.  This can be a wildcard with an asterisk to indicate multiple servers.
     * @return  A JSON string or NULL to indicate no data is available.
     */
    public String getServerStats(String name);
    
    /**
     * Register an authenticator.
     * @param auth  The authenticator
     */
    public void setAuthenticator(ImaAuthenticator auth);
    
    /**
     * Set Device Updater
     * @param deviceUpdater
     */
    public void setDeviceUpdater(ImaDeviceUpdater deviceUpdater);
    
    /**
     * Update the Gateway Device
     * @param name the subject Name or gateway full ID
     * @param json JSON string of the device properties
     */
    public void setDevice(String name, String json);
    
    /**
     * Set the MessageHub Bindings
     * @param json
     */
    public void setMHubBindings(String json);
    
    /**
     * Set the MessageHub Bindings with name
     * @param name org id 
     * @param json
     */
    public void setMHubBindings(String name, String json);
    
    /**
     * Set MessageHub Endpoints Credentials
     * @param json
     */
    public void setMHubCredentials(String json);
    
    /**
     * Set MessageHub Endpoints Credentials with name
     * @param name org/serviceid
     * @param json
     */
    public void setMHubCredentials(String name, String json);
    
    /**
     * Set the callback to get the credential for an org or service id
     * @param mHubCredMgr
     */
    public void setMHUBCredentialManager(ImaMHubCredentialManager mHubCredMgr) ;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
