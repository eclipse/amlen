/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

import java.nio.charset.Charset;
import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.security.Key;
import java.util.Arrays;

/*
 * 
 */
public class ImaSsoCrypt {
    static final byte[] keybase = {
        (byte)0x0b, (byte)0xbd, (byte)0xd7, (byte)0x89, (byte)0x41, (byte)0xa0, (byte)0xd1, (byte)0xc0,
        (byte)0x74, (byte)0x1b, (byte)0x6d, (byte)0x12, (byte)0x56, (byte)0xa7, (byte)0x50, (byte)0xb9,
        (byte)0x11, (byte)0x4b, (byte)0x64, (byte)0xd9, (byte)0x46, (byte)0x01, (byte)0x49, (byte)0x0b,
        (byte)0xfd, (byte)0xc5, (byte)0xb8, (byte)0xe7, (byte)0xe8, (byte)0xeb, (byte)0xd9, (byte)0xb0,
        (byte)0x48, (byte)0x62, (byte)0x5d, (byte)0x6b, (byte)0xa2, (byte)0x8d, (byte)0xcb, (byte)0xf7,
        (byte)0x5d, (byte)0x4a, (byte)0xe9, (byte)0xde, (byte)0x19, (byte)0x76, (byte)0x1b, (byte)0xb0,
        (byte)0x27, (byte)0x04, (byte)0xac, (byte)0xc5, (byte)0x89, (byte)0xf4, (byte)0x67, (byte)0x84,
        (byte)0x92, (byte)0x8a, (byte)0xa4, (byte)0xd7, (byte)0x1d, (byte)0x70, (byte)0x24, (byte)0xbd,
    };
    static final byte [] iv = {
        (byte)0x7b, (byte)0xea, (byte)0x60, (byte)0x06, (byte)0x66, (byte)0x9f, (byte)0x15, (byte)0x66,
        (byte)0x61, (byte)0xd2, (byte)0xdf, (byte)0x3d, (byte)0xcc, (byte)0x96, (byte)0xee, (byte)0x50,
    };

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";



    /*
     * Encrypt the string with variable key and salt
     */
    public static String encrypt(String s) throws Exception {
        int offset = (int)(System.currentTimeMillis()%43);
        char [] rchar = new char[4];
        for (int i=0; i<4; i++) {
            rchar[i] = b64digit.charAt(keybase[offset+16+i]&0x3F);
        }
        IvParameterSpec ivspec = new IvParameterSpec(iv);
        String  sx = new String(rchar) + ':' + s;
        Key key = generateKey(offset);
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, key, ivspec);
        byte[] inbytes = sx.getBytes(Charset.forName("UTF-8"));
        byte[] blkbytes = new byte[(inbytes.length+16)&~0x0f];
        Arrays.fill(blkbytes, (byte)0);
        System.arraycopy(inbytes, 0, blkbytes, 0, inbytes.length);
        byte[] encbytes = cipher.doFinal(blkbytes);
        byte[] encplus = new byte[encbytes.length + 1];
        encplus[0] = (byte)offset;
        System.arraycopy(encbytes, 0, encplus, 1, encbytes.length);
        return toBase64(encplus);
    }

    /*
     * Decrypt the string with variable key and salt
     */
    public static String decrypt(String s) throws Exception {
        byte[] bindata = fromBase64(s);
        int offset = (bindata[0]&0xff) % 43;
        Key key = generateKey(offset);
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        IvParameterSpec ivspec = new IvParameterSpec(iv);
        cipher.init(Cipher.DECRYPT_MODE, key, ivspec);
        byte[] decbytes = cipher.doFinal(bindata, 1, bindata.length-1);
        String ret = new String(decbytes, Charset.forName("UTF-8"));
        if (ret.length()<5 || ret.charAt(4) != ':')
             throw new IllegalArgumentException("Decrytped value not correct");
        ret = ret.substring(5);
        int pos = ret.indexOf(0);
        if (pos >= 0)
            ret = ret.substring(0, pos);
        return ret;
    }


    /*
     * Generate the key
     */
    static Key generateKey(int offset) throws Exception {
        byte [] keyvalue = new byte[16];
        System.arraycopy(keybase, offset, keyvalue, 0, 16);
        Key key = new SecretKeySpec(keyvalue, "AES");
        return key;
    }


    /*
     * Base64 encoding support
     */
    static private String b64digit = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


    /**
     * Convert a byte array to a base64 string.
     * @param from  The source byte array
     * @return A string in base64.  This is always a multiple of 4 bytes.
     */
    static private String toBase64(byte[] from) {
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
    static private byte b64val [] = {
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

    /*
     * Convert from a base64 string to a byte array.
     *
     * @param from  The input string in base64 which must be a multiple of 4 bytes
     * @return The byte array or null to indicate the string is not valid base64
     */
    static private byte[] fromBase64(String from) {
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

