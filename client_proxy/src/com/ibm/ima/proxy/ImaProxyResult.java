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

import java.nio.charset.Charset;

public class ImaProxyResult {
    private  int  rc;
    private  String mime;
    private  byte [] bytes;
    private  String  str;
    static Charset utf8 = Charset.forName("UTF-8");
    
 
    /** The result is good */
    public static final int  Good             = 0;
    /** The JSON object is not complete */
    public static final int  JsonIncomplete   = 1;
    /** The JSON object is not valid */
    public static final int  JsonError        = 2;
    /** A required property is missing */
    public static final int  MissingRequired  = 3;
    /** The requested object is not found */
    public static final int  NotFound         = 4;
    /** The requested item is not found */
    public static final int  ItemNotFound     = 5;
    /** The object cannot be deleted because it is in use */
    public static final int  InUse            = 6;
    /** The path or query is not valid */
    public static final int  BadPath          = 7;
    /** The property value is not correct */
    public static final int  BadPropValue     = 8;
    /** The return code is not known */
    public static final int  Unknown          = 9;
    /** An exception occurred */
    public static final int  Exception        = 10;
    
    static final String S_Good            = "";
    static final String S_JsonIncomplete  = "The JSON object is not complete";
    static final String S_JsonError       = "The JSON object is not valid";
    static final String S_MissingRequired = "A required property is missing";
    static final String S_NotFound        = "The requested object is not found";
    static final String S_ItemNotFound    = "The requested item is not found";
    static final String S_InUse           = "The object cannot be deleted because it is in use";
    static final String S_BadPath         = "The path or query is not valid";
    static final String S_BadPropValue    = "The property value is not correct";
    static final String S_Unknown         = "The return code is not known";
    static final String S_Exception       = "An exception occurred";
    
    /**
     * Constructor for binary data response
     */
    public ImaProxyResult(int rc, String mime, byte [] bytes) {
        this.rc = rc;
        this.mime = mime;
        this.bytes = bytes;
    }
    
    /**
     * Constructor for string data response
     */
    public ImaProxyResult(int rc, String mime, String str) {
        this.rc = rc;
        this.mime = mime;
        this.str = str;
    }
    
    /**
     * Constructor from an exceptions
     */
    public ImaProxyResult(Throwable e) {
        this.rc = -1;
        this.mime = "text/plain";
        this.str = e.getMessage();
    }
        
    
    /**
     * Constructor from an error response from JNI
     */
    public ImaProxyResult(String rstr) {
        int colon1 = -1;
        int colon2 = -1;
        this.mime = "text/plain";
        String  reason = "";
        boolean replonly = false;
        if (rstr == null) {
            this.rc = 0;
            this.str = "OK";
        } else {    
            colon1 = rstr.indexOf(':');
            colon2 = rstr.indexOf(':', colon1+1);
            this.rc = Unknown;
            this.mime = "text/plain";
            if (colon1 >= 0 && colon2 > 0) {
                String rc;
                if (rstr.charAt(0)=='+') {
                    rc = rstr.substring(1, colon1);
                } else {
                    replonly = true;
                    rc = rstr.substring(0, colon1);
                }
                try { this.rc = Integer.parseInt(rc);} catch (Exception e) {}
                reason = rstr.substring(colon2+1);
            }
            if (reason.length() > 0) {
                if (replonly)
                    this.str = getErrorString(this.rc) + ": " + reason;
            } else {
                this.str = getErrorString(this.rc);
            }
        }
    }

    /**
     * Get the error string associagted with a return code.
     * @param rc  The return code
     * @return    The error string
     */
    public static String getErrorString(int rc) {
        switch (rc) {
        case Good:            return S_Good;
        case JsonIncomplete:  return S_JsonIncomplete;
        case JsonError:       return S_JsonError;
        case MissingRequired: return S_MissingRequired;
        case NotFound:        return S_NotFound;
        case InUse:           return S_InUse;
        case BadPath:         return S_BadPath;
        case BadPropValue:    return S_BadPropValue;
        case Unknown:         return S_Unknown;
        case Exception:       return S_Exception;
        }
        return "Error: "+rc;
    }
    
    /**
     * Get the return code.
     * @return A return code 0=good, other return codes are HTTP based return codes
     */
    public int getReturnCode() {
        return rc;
    }
    
    /**
     * Get the mime type of the result.
     * If there is no result the mime type is null.
     * @return the mime type or null to indicate there is no result
     */
    public String getMimeType() {
        return mime;
    }
    
    /**
     * Get the result as a string.
     * @return The string representing the value
     */
    public String getStringValue() {
        if (str != null)
            return str;
        if (bytes != null)
            return new String(bytes, utf8);
        return null;
    }
    
    /**
     * Get the result as a byte array.
     * @return the byte array representation of the value.
     */
    public byte [] getBytesValue() {
        if (bytes != null)
            return bytes;
        if (str != null)
            return str.getBytes(utf8);
        return null;
    }
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        if (rc == 0) {
            String mimestr = mime != null ? (mime + ": ") : "";
            if (bytes != null) 
                return mimestr + "len=" + bytes.length;
            if (str == null)
                return mimestr + "null";
            if ("text/plain".equals(mime))
                return str;
            return mimestr + "len=" + str.length();
        }
        return getErrorString(rc) + " (" + rc + ")";
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
}
