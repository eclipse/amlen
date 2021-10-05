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

import java.nio.charset.StandardCharsets;
import java.util.jar.Attributes;

/**
 * The IsmHTTPResponse represents the response on an IsmHTTPConnection.
 * This the status, headers, and content to be parsed from the response.
 */
public class IsmHTTPResponse extends IsmWSMessage {

    int  status = -1;
    byte [] content = null;
    int  content_loc = -1;
    Attributes hdrs = null;
    
    public IsmHTTPResponse() {
    }
    
    /**
     * The normal constructor from a byte array
     */
    public IsmHTTPResponse(byte[] payload, int start, int len) {
        super(payload, start, len, 0);
    }
    
    /**
     * Return the HTTP status code.
     * If the status cannot be found return 0
     * @return The HTTP status
     */
    public int getStatus() {
        int i;
        if (status < 0) {
            status = 0;
            byte [] payload = getPayloadBytes();
            if (payload != null) {
                int len = payload.length;
                for (i=0; i<len; i++) {
                    if (payload[i] == (byte)' ') {
                        for (++i; i<len; i++) {
                            if (payload[i]<'0' || payload[i]>'9')
                                break;
                            status = (status*10) + (payload[i]-'0');
                        }
                        break;
                    }
                }
            }
        }
        return status;
    }
    
    /**
     * Return the content as a byte array. 
     * If there is no content return null
     * @return  The content
     */
    public byte [] getContent() {
        int  i;
        if (content_loc < 0) {
            content_loc = 0;
            byte [] payload = getPayloadBytes();
            if (payload != null) {
                int len = payload.length-3;
                for (i=0; i<len; i++) {
                    if (payload[i] == '\r' && payload[i+2]=='\r' && payload[i+1]=='\n' && payload[i+3]=='\n') {
                        content_loc = i+4;
                        if (content_loc < payload.length) {
                            content = new byte [payload.length-content_loc];
                            System.arraycopy(payload, content_loc, content,  0,  content.length);
                        }    
                    }
                }
            }
        }    
        return content;
    }
    
    /**
     * Get the message payload as a String.
     * If the content does not exist or cannot be converted to a String, return null
     * @return The byte array form of the payload
     */
    public String getPayloadString() {
        try { 
            return new String(getPayloadBytes(), StandardCharsets.UTF_8);
        } catch (Exception e) {
            return null;
        }
    }
    
    /**
     * Get the headers as a string
     * @return The header section of the response as a string
     */
    public String getHeadersString() {
        if (content_loc < 0);
            getContent();
        try {    
            return new String(getPayloadBytes(), 0, content_loc, StandardCharsets.UTF_8); 
        } catch (Exception e) {
            return null;
        }
    }
    
    /**
     * Get the headers as an Attributes object
     * The Attributes object is used because it allows for a case independent map style object.
     * @return The headers
     */
    public Attributes getHeaders() {
        if (hdrs == null) {
            hdrs = new Attributes(50);
            String str = getHeadersString();
            if (str != null) {
                int offset = 0;
                for (;;) {
                    int pos = str.indexOf("\r\n", offset);
                    if (pos < 0 || pos == offset)
                        break;
                    String ent = str.substring(offset, pos);
                    int colon = ent.indexOf(':');
                    if (colon > 0) {
                        int valpos = colon+1;
                        while (valpos < ent.length() && ent.charAt(valpos)==' ')
                            valpos++;
                        // System.out.println("ent="+ent+" len=" + ent.length() + " colon="+colon+" valpos="+valpos);
                        hdrs.putValue(ent.substring(0,colon), ent.substring(valpos));
                    }
                    offset = pos+2;
                }
            }
        }    
        return hdrs;
    }
    
    /**
     * Get the value of an individual header
     * @param name  The name of the header
     * @return The value of the header, or null if it is not set
     */
    public String getHeader(String name) {
        if (hdrs == null)
            getHeaders();
        return hdrs.getValue(name);
    }
}
