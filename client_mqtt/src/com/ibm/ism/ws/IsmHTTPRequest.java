/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.ws;

import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.Set;
import java.util.jar.Attributes;

/*
 * The IsmHTTPRequest object represents a single request within an HTTP connection.
 * The HTTP connection is defined by a IsmHTTPConnection object.
 */
public class IsmHTTPRequest {
    IsmHTTPConnection hcon;
    String op;
    String path;
    String query;
    String uri;
    String host;
    String basicauth;
    byte [] content;
    String content_type;
    boolean keepalive = true;
    int verbose;
    boolean bindump;
    Attributes hdrs;
    
    /**
     * Consturctor
     * @param hcon   The connection object
     * @param op     The op as "GET", "POST", "PUT", etc.
     * @param path   The path portion of the URL
     * @param query  The query portion of the URL
     */
    public IsmHTTPRequest(IsmHTTPConnection hcon, String op, String path, String query) {
        this.hcon = hcon;
        this.op = op;
        this.path = path;
        this.query = query;
        this.verbose = hcon.verbose;
    }
    
    /**
     * Set the verbose setting
     * This is used for problem determination.
     * 0 = no data output 
     * 1 = Show the request path and result status
     * 2 = Show the request path and result status and content
     * 3 = show the request path and content and the result status and content
     * 4 = Show the entire HTTP request and HTTP response
     * 
     * The content is shown in text or binary based on the setting of verbose in
     * the IsmHTTPConnection object.
     * 
     * If verbose is not set for a request, the verbose setting of the connection is used.
     * 
     * @param verbose  The verbose setting for this request
     * @return This connection object
     */
    public IsmHTTPRequest setVerbose(int verbose) {
        this.verbose = verbose;
        return this;
    }
    
    /**
     * Get the verbose setting
     * #return The verbose setting
     */
    public int getVerbose() {
        return verbose;
    }
    
    /**
     * Set the user and password
     * @param username  The username
     * @param pw        The password
     * #return This object
     */
    public IsmHTTPRequest setUser(String username, String pw) {
        String auth = username + ':' + pw;
        basicauth = IsmWSConnection.toBase64(auth);
        return this;    
    }
    
    /**
     * Set the content as a byte array.
     * This will automatically set the content-type to either JSON or text.
     * @param content  The content as a String
     * #return This object
     */
    public IsmHTTPRequest setContent(String content) {
        if (content == null)
            return this;
        this.content = content.getBytes(StandardCharsets.UTF_8);
        if (content_type == null) {
            if (this.content.length > 0 && this.content[0] == '{') 
                content_type = "application/json";
            else
                content_type = "text/plain; charset=utf-8";
        }
        return this;    
    }
    
    /**
     * Return the content byte arrary
     * @return The content
     */
    public byte [] getContent() {
        return content;
    }
    
    /**
     * Set the content as a byte array.
     * This method does not set the content type
     * @param content The content as a byte array
     * #return This object
     */
    public IsmHTTPRequest setContent(byte [] content) {
        this.content = content;
        return this;
    }
    
    /**
     * Set the content type
     * Setting the content type to null will cause no content-type header
     * to be sent on the HTTP request
     * #param content_type  The content type
     * #return This object
     */
    public IsmHTTPRequest setContentType(String content_type) {
        this.content_type = content_type;
        return this;
    }
    
    /**
     * Set whether to request the connection to close.
     * The default if this is not alled is keepalive=true
     * @param keealive true if the connection should be kept alive
     * @return this object
     */
    public IsmHTTPRequest setKeepAlive(boolean keepalive) {
        this.keepalive = keepalive;
        return this;
    }
    
    /**
     * Add a header field
     * @param name  The name of the header
     * @param value The value of the header
     * @return this object
     */
    public IsmHTTPRequest setHeader(String name, String value) {
        if ("content-type".equalsIgnoreCase(name)) {
            content_type = value;
        } else if (!"authorization".equalsIgnoreCase(name) &&
            !"host".equalsIgnoreCase(name) &&   
            !"connection".equalsIgnoreCase(name) &&   
            !"content-length".equalsIgnoreCase(name)) {            
            if (hdrs == null) {
                hdrs = new Attributes(50);
            }
            hdrs.putValue(name,  value);
        }    
        return this;
    }
    
    /**
     * Get the non-standard headers as an Attributes object
     * The standard headers authorization, connection, content-length, and host 
     * are not returned by this method.
     * @return The non-standard heasers
     */
    public Attributes getHeaders() {
        return hdrs;
    }
    
    /**
     * Get a single header\
     * @param name  The name of the header field
     * @return The value of the header field
     */
    public String getHeader(String name) {
        if (name == null)
            return null;
        if ("host".contentEquals(name))
            return hcon.ip;
        if ("host".contentEquals(name))
            return basicauth;
        if ("connection".contentEquals(name))
            return basicauth;
        if ("content-length".equalsIgnoreCase(name))
            return (content != null) ? ""+content.length : "0";
        if ("content-type".equalsIgnoreCase(name))
            return content_type;
        if (hdrs == null)
            return null;
        return hdrs.getValue(name);
    }
    

    /** 
     * Make an HTTP request and return a response
     * This will show the request and response as defined by the serVerbose().
     * @return The response message
     */
    public IsmHTTPResponse request() throws IOException {
        String request;
        URI xpath;
        try {
            xpath = new URI(null, null, path, query, null);
        } catch (Exception e) {
            throw new RuntimeException("Unable to creawte URI", e);
        }
        request = op + ' ' + xpath + " HTTP/1.1\r\n";
        request += "host: " + hcon.ip + "\r\n";
        if (basicauth != null) 
            request += "authorization: basic " + basicauth + "\r\n";
        if (keepalive) {
            request += "connection: keep-alive\r\n";
        } else {
            request += "connection: close\r\n";
        }
        if (hdrs != null) {
            Set<Object> map = hdrs.keySet();
            Iterator<Object> it = map.iterator();
            while (it.hasNext()) {
                String hdr = ""+it.next();
                request += hdr + ": " + hdrs.getValue(hdr) + "\r\n";
            }
        }
        if (content != null) {
            if (content_type != null)
                request += "content-type: " + content_type + "\r\n";
            request += "content-length: " + content.length + "\r\n";
        }
        request += "\r\n";
        byte [] requestb = request.getBytes(StandardCharsets.UTF_8);
        if (content != null) {
            byte [] fullreq = new byte [requestb.length + content.length];
            System.arraycopy(requestb, 0, fullreq, 0, requestb.length);
            System.arraycopy(content, 0, fullreq, requestb.length, content.length);
            requestb = fullreq;
        }    
        if (verbose > 3) {
            hcon.dumpData("send request", requestb);
        } else if (verbose != 0) {
            hcon.dumpData("send request: " + op + " " + path + ((query != null) ? ("?" + query) : ""), 
                    verbose == 3 ? content : null);
        }
        hcon.send(requestb);
        
        IsmHTTPResponse ret = (IsmHTTPResponse) hcon.receive();
        if (ret != null) {
            if (verbose > 3) {
                hcon.dumpData("receive response",  ret.getPayloadBytes());
            } else if (verbose == 1) {
                hcon.dumpData(("receive response status=" + ret.getStatus()), null);
            } else if (verbose == 2 || verbose == 3) {
                hcon.dumpData(("receive response status=" + ret.getStatus()), ret.getContent());
            }
        }
        return ret;
    }
}
