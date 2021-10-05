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

package com.ibm.ima.plugin.util;

/**
 * The ImaBase64 class is used to convert byte arrays to and from base64 strings.
 * <p>
 * The ImaBase64 class provides static methods for efficient between byte arrays and
 * base64 strings.
 */
public class ImaBase64 {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    /*
     * Base64 encoding support    
     */
    static String b64digit = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    /*
     * This constructor is not used because all methods in this class are static.
     */
    private ImaBase64() {
    }
    
    /**
     * Convert a byte array to a base64 string.
     * @param from  The source byte array
     * @return A string in base64.  This is always a multiple of 4 bytes.
     */
    static public String toBase64(byte[] from) {
        int j = 0;
        int val = 0;
        int left = from.length;
        int len =  (from.length+2) / 3 * 3;
        char [] to = new char[(len/3)*4];
        int i;

        for (i=0; i<len; i += 3) {
            val = (int)(from[i]&0xff)<<16;
            if (left > 1)
                val |= (int)(from[i+1]&0xff)<<8;
            if (left > 2)
                val |= (int)(from[i+2]&0xff);
            to[j++] = b64digit.charAt((val>>18)&0x3f);
            to[j++] = b64digit.charAt((val>>12)&0x3f);
            to[j++] = left>1 ? b64digit.charAt((val>>6)&0x3f) : '=';
            to[j++] = left>2 ? b64digit.charAt(val&0x3f) : '=';
            left -= 3;
        }
        return new String(to);
    }
    
    /*
     * Map base64 digit to value
     */
    static byte b64val [] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 00 - 0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 10 - 1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,   /* 20 - 2F + / */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,   /* 30 - 3F 0 - 9 = */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   /* 40 - 4F A - O */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,   /* 50 - 5F P - Z */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   /* 60 - 6F a - o */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1    /* 70 - 7F p - z */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 80 - 8F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 90 - 9F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* A0 - AF */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* B0 - BF */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* C0 - CF */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* D0 - DF */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* E0 - EF */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* F0 - FF */
    };
    
    /**
     * Convert from a base64 string to a byte array.
     *
     * @param from  The input string in base64 which must be a multiple of 4 bytes
     * @return The byte array or null to indicate the string is not valid base64
     */
    static public byte[] fromBase64(String from) {
        int i;
        int tolen;
        int j = 0;
        int value = 0;
        int bits = 0;
        if (from.length()%4 != 0) {
            return null;
        }
        if (from.length() == 0)
            return new byte[0];
        
        tolen = from.length()/4 * 3;
        if (from.charAt(from.length()-1) == '=')
            tolen--;
        if (from.charAt(from.length()-2) == '=')
            tolen--;
        byte [] to = new byte[tolen];
        
        for (i=0; i<from.length(); i++) {
            char ch = from.charAt(i);
            byte bval = b64val[ch];
            if (ch == '=' && i+2 < from.length())
                bval = -1;
            if (bval >= 0) {
                value = value<<6 | bval;
                bits += 6;
                if (bits >= 8) {
                    bits -= 8;
                    if (j < to.length)
                        to[j++] = (byte)((value >> bits) & 0xff);
                }
            } else {
                if (bval == (byte)-1)
                    return null;
            }
        }
        return to;
    }
}
