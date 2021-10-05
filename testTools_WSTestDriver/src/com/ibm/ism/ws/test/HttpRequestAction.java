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

package com.ibm.ism.ws.test;

import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.Set;
import java.util.Vector;
import java.util.jar.Attributes;

import org.w3c.dom.Element;

import com.ibm.ism.ws.IsmHTTPConnection;
import com.ibm.ism.ws.IsmHTTPRequest;
import com.ibm.ism.ws.IsmHTTPResponse;
import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class HttpRequestAction extends ApiCallAction {
    protected final String  connectionID;
    protected final String  action;
    protected final String  path;
    protected final String  query;
    protected final String  username;
    protected final String  password;
    protected final String  contentType;
    protected final String  content;
    protected final Attributes header;
    protected final int     verbose;
    protected final int     status;
    protected final boolean keepalive;
    protected final boolean stillalive;
    protected final String []    responses;
    protected final Attributes outheader;
                    int gotstatus;
                    

    /*
     * Make an HTTP Request
     */
    public HttpRequestAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        connectionID = _actionParams.getProperty("connection_id");
        if (connectionID == null) {
            throw new RuntimeException("connection_id is not defined for " + this);
        }
        action    = _apiParameters.getProperty("action", "GET");   /* HTTP action: GET, PUT, POST, HEAD, etc */
        path      = _apiParameters.getProperty("path", "/");       /* HTTP path */           
        query     = _apiParameters.getProperty("query");           /* HTTP query (after ? */
        username  = _apiParameters.getProperty("username");       
        password  = _apiParameters.getProperty("password");
        contentType = _apiParameters.getProperty("contentType");   
        content   = _apiParameters.getProperty("content");
        status    = Integer.parseInt(_apiParameters.getProperty("status", "0"));
        verbose   = Integer.parseInt(_apiParameters.getProperty("verbose", "0"));
        keepalive = Boolean.parseBoolean(_apiParameters.getProperty("keepalive", "true"));
        stillalive = Boolean.parseBoolean(_apiParameters.getProperty("stillalive", "false"));
        header    = getHeaders("header");
        outheader = getHeaders("outheader");
        responses = getStrings("response");
    }
        
    protected boolean invokeApi() throws IsmTestException {
        CreateHttpConnectionAction conact = (CreateHttpConnectionAction)_dataRepository.getVar(connectionID);
        if (conact == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3612",
                    "Action {0}: Failed to locate the Connection object ({1}).",_id, connectionID);
            return false;
        }
        IsmHTTPConnection connect = conact.getConnect();
        if (connect == null || !connect.isConnected()) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3613",
                    "Action {0}: The connection is closed ({1}).",_id, connectionID);
            return false;    
        }
        IsmHTTPRequest request = new IsmHTTPRequest(connect, action, path, query);
        request.setVerbose(verbose);
        request.setUser(username,  password);
        request.setContent(content);
        if (header != null) {
            Set<Object> map = header.keySet();
            Iterator<Object> it = map.iterator();
            while (it.hasNext()) {
                String hdr = ""+it.next();
                request.setHeader(hdr, header.getValue(hdr));
            }
        }
        request.setContentType(contentType);
        request.setKeepAlive(keepalive);
        IsmHTTPResponse resp;
        try {
            resp = request.request();
        } catch (Exception e) {
            throw new IsmTestException("ISMTEST3614", "Request failed: " + this + ": " + e);
        }
        
        /*
         * Check thes status 
         */
        gotstatus = resp.getStatus();
        if (status != 0 && gotstatus != status) {
            throw new IsmTestException("ISMTEST3615", "Action " + _id + ": Expected status " + status + " but got " + gotstatus); 
        }
        
        /*
         * Check the output headers
         */
        if (outheader != null) {
            Set<Object> map = outheader.keySet();
            Iterator<Object> it = map.iterator();
            while (it.hasNext()) {
                String hdr = ""+it.next();
                String gotval = resp.getHeader(hdr);
                String expval = outheader.getValue(hdr);
                if (expval.length()==0) {
                    if (gotval != null && gotval.length() > 0) {
                        throw new IsmTestException("ISMTEST3616", "Action " + _id + ": Expected header " + hdr + " to be empty but got " + gotval);
                    }
                } else {
                    /* Compare case independent as most header values may be an any case */
                    if (gotval.toLowerCase().indexOf(expval.toLowerCase()) < 0) {
                        throw new IsmTestException("ISMTEST3617", "Action " + _id + ": Expected header " + hdr + " to contain \"" +
                            expval + "\" but got \"" + gotval + "\"");     
                    }
                }
            }
        }
        
        /*
         * Check the output content 
         */
        if (responses != null) {
            byte [] bcontent = resp.getContent();
            String scontent = bcontent == null ? "" : new String(bcontent, StandardCharsets.UTF_8);
            for (int i=0; i<responses.length; i++) {
                if (scontent.indexOf(responses[i]) < 0) {
                    throw new IsmTestException("ISMTEST3618", "Action " + _id + ": Expected contents to contain \"" +
                            responses[i] + "\" but got \"" + scontent + "\"");     
                }
            }
        }
        
        /*
         * Check if the connection is as we expect
         */
        if (stillalive) {
            if (connect.isConnected() != keepalive) {
                throw new IsmTestException("ISMTEST3619", "Action " + _id + ": Expected connection to be " +
                        (keepalive ? "open" : "closed") + " but it is not");
            }
        }
        
        return true;
    }
    
    /*
     * Get an array of strings
     */
    String [] getStrings(String name) {
        Vector<String> list = null;
        for (int index = 0; ; index++) {
            String item = name + '.' + index;
            String entry = _apiParameters.getProperty(item);
            if (entry == null)
                break;
            if (list == null)
                list  = new Vector<String>();
            list.add(entry);
        }
        return list == null ? null : list.toArray(new String[0]);
    }
    
    /*
     * Get the headers as a Attributes object
     */
    Attributes getHeaders(String name) {
        Attributes list = null;
        for (int index = 0; ; index++) {
            String item = name + '.' + index;
            String entry = _apiParameters.getProperty(item);
            if (entry == null)
                break;
            int pos = entry.indexOf('=');
            if (pos <= 0) {             
                _trWriter.writeTraceLine(LogLevel.LOGLEV_WARN, "ISMTEST3611",
                        "Action {0}: Value for header {1} is not correct.", _id, entry);
                break;
            }
            if (list == null)
                list  = new Attributes(50);
            list.putValue(entry.substring(0, pos), entry.substring(pos+1));
        }
        return list;
    }

    /**
     * @see com.ibm.ism.ws.test.Action#toString()
     */
    public String toString() {
        String ret = "HTTPRequest name=" + _id + " action=" + action + " path=" + path + " query=" + query;
        if (gotstatus != 0) {
            ret += " status=" + gotstatus;
        }
        return ret;
    }
}
