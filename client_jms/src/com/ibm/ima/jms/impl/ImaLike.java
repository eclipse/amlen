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
package com.ibm.ima.jms.impl;

import javax.jms.JMSException;

/*
 * Implement a like rule for the IBM MessageSight JMS client
 */
public class ImaLike {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    String  likestr;             /* Original like string */
    char [] rule;                /* Compiled like rule   */
    int     rulelen;             /* Length of the compiled rule */
    
   /*
    * Create a like rule from a like string.
    * A like string contains any character, but the characters '%', '_', have special meaning.
    * <ul>  
    * <li>The '%' will match zero or more characters.
    * <li>The '_' will match exactly one character.
    * </ul>
    * @param likestr  The like string.
    */
    public ImaLike (String s) throws JMSException {
        this(s, (char)0xffff);
    }
    
    /*
     * Create a like rule from a like string with an escape.
     * A like string contains any character, but the characters '%', '_', and the
     * specified escape character are treated differently.
     * <ul>  
     * <li>The '%' will match zero or more characters.
     * <li>The '_' will match exactly one character.
     * <li>The specified escape character means that the following character is treated as a literal.
     * </ul>
     * @param likestr  The like string.
     * @param escape   The escape character, or 0 if there is no escape character
     */
    public  ImaLike(String likestr, char escape) throws JMSException {
        this.likestr = likestr;
        rule = new char[matchlen()];
        int inpos = 0;
        int outpos = 0;
        int lenpos = -1;
        boolean inescape = false;
        
        /*
         * Convert the like string to a rule character array
         */
        while (inpos < likestr.length()) {
            char kind = 0;
            char ch = likestr.charAt(inpos++);
            /*
             * Since we use 0xffff and 0xfffe which are invalid Unicode characters
             * as specials, make sure they are not in the string.
             */
            if (ch == 0xffff || ch == 0xfffe) {
            	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0290", "A request to create a JMS message consumer failed because the LIKE expression in the selection rule contains a code point which is not a valid Unicode character.");
                ImaTrace.traceException(2, jex);
                throw jex;
            }
            if (inescape) {
                inescape = false;
            } else {
                if (ch == escape ) {
                    inescape = true;
                    continue;
                }
                if (ch == '%') {
                    kind = 0xffff;
                } else if (ch == '_') {
                    kind = 0xfffe;
                }    
            }
            if (kind != 0) {
                if (kind == 0xffff && lenpos==-1 && outpos > 0 && rule[outpos-1]==0xffff)
                    outpos--;
                rule[outpos++] = kind;
                lenpos = -1;
            } else {
                if (lenpos == -1) {
                    lenpos = outpos++;
                    rule[lenpos] = 0;
                } 
                rule[outpos++] = ch;
                rule[lenpos]++;
            }
        }
        rule[outpos++] = 0;
        rulelen = outpos;
    }

    /*
     * Match a string to the rule.
     * @param str   The string to match
     * @return true if the string matches the like rule
     */
    public boolean match(String str) {
        int clen = 0;
        int pos = 0;
        int lastpos = -1;
        int lastmatchpos = -1;
        int lastlen = 0;
        int matchpos = 0;
        int len = str.length();
        
        /*
         * Loop for all elements in the rule
         */
        for (;;) {
            if (rule[matchpos]==0) {
                if (len == 0 || lastmatchpos<0)
                    break;
                matchpos = lastmatchpos;
            }
            
            /*
             * Process a generic (%)
             */
            if (rule[matchpos] == 0xffff) {
                lastmatchpos = matchpos++;
                clen = rule[matchpos];
                lastpos = -1;
                if (clen == 0xffff || clen == 0xfffe) {
                    if (clen == 0xfffe && pos < str.length()) {
                        lastlen = len-1;
                        lastpos = pos+1;
                    }
                    continue;
                }
                matchpos++;
                if (clen==0)
                    return true;
                while (len > 0) {
                    if (str.charAt(pos) == rule[matchpos]) {
                        if (len >= clen && strEquals(str, pos, matchpos, clen)) {
                            matchpos += clen;
                            break;
                        }
                    }
                    len--;
                    pos++;
                }
                if (len == 0)
                    return false;
                len -= clen;
                pos += clen;
            } 
            
            /*
             * Process a single character
             */
            else if (rule[matchpos] == 0xfffe) {
                if (len == 0)
                    return false;
                /* We should handle surrogates here */
                len--;
                pos++;
                matchpos++;
            }
            
            /*
             * Process a constant string
             */
            else {
                clen = rule[matchpos++];
                if (len < clen || !strEquals(str, pos, matchpos, clen )) {
                    matchpos += clen;
                    if (lastmatchpos >= 0) {
                        matchpos = lastmatchpos;
                        if (lastpos >= 0) {
                            len = lastlen;
                            pos = lastpos;
                        }
                        continue;
                    }
                    return false;
                }
                len -= clen;
                pos += clen;
                matchpos += clen;
            }
        }    
        return len == 0;
    }
    
    /*
     * Check if a range of a string matches a range within the rule
     */
    boolean strEquals(String s, int pos, int matchpos, int len) {
        while (len-- > 0) {
            if (s.charAt(pos++) != rule[matchpos++])
                return false;
        }
        return true;
    }

    
    /*
     * Determine the size needed for the compiled rule
     */
    int matchlen() {
        int len = likestr.length()+2;
        for (int i=0; i<likestr.length(); i++) {
            if (likestr.charAt(i)=='%' || likestr.charAt(i)=='_')
                len++;
        }
        return len;
    }
    
    
    /*
     * Dump a rule for debugging
     */
    String dumpLike() {
        StringBuffer b = new StringBuffer();
        for (int i=0; i<rulelen; i++) {
            if (rule[i]==0xffff) 
                b.append('%');
            else if (rule[i]==0xfffe)
                b.append('_');
            else {
                int count = rule[i++];
                b.append("["+count+"]");
                while (count-- > 0) 
                    b.append(rule[i++]);
                i--;
            }    
        }
        return b.toString();
    }
    
    /*
     * Use the original like string as the object name.
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return likestr;
    }
}
