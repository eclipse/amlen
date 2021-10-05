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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * The ImaJson class is used to parse a JSON string.
 * This is a high speed parser designed to limit the creation of Java objects
 * while parsing.
 * <p>
 * The object can be reused by calling parse with a new string.
 * <p>
 * The ImaJson class also provides a set of helper methods to create JSON.
 */
public class ImaJson {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Create a JSON parser object.
     */
    public ImaJson() {
    }
    
    /**
     * A JSON entry.  There is one for each item in an object or array.
     */
    public class Entry {
        /** The name of the entry.  This is not set for entries in an array */
        public String  name;
        /** The value */
        public String  value;
        /** The count of items in an array, or the value of an integer entry */
        public int     count;
        /** The level in the document tree */
        public int     level;
        /** The line number from the start of the source (first line = 1) */
        public int     line;
        /** The type of the object */
        public JObject objtype;
    }


    /**
     *  JSON object entry type.
     */
    public enum JObject {
        /** JSON string, value is UTF-8              */
        JsonString,    
        /** A number with no decimal point.  The value is in count  */
        JsonInteger,    
        /** Number which is too big or has a fractional part */
        JsonNumber,   
        /** JSON object, count is number of entries  */
        JsonObject,   
        /** JSON array, count is number of entries   */
        JsonArray, 
        /** JSON true, value is not set              */
        JsonTrue,  
        /** JSON false, value is not set             */
        JsonFalse,  
        /** JSON null, value is not set              */
        JsonNull,    
    };

    /*
     * Tokens returned by the JSON tokenizer
     */
    enum JToken {
        Error,         /* Indicates a parsing error          */
        StartObject,   /* Start object {                     */
        EndObject,     /* End object }                       */
        StartArray,    /* Start array [                      */
        EndArray,      /* End array ]                        */
        Colon,         /* Name/value separator :             */
        Comma,         /* List separator                     */
        String,        /* Quoted string                      */
        Number,        /* Decimal number                     */
        Integer,       /* Integer fits in 4 bytes            */
        True,          /* Boolean true                       */
        False,         /* Boolean false                      */
        Null,          /* Null value                         */
        End,           /* End reached                        */
        Incomplete,    /* Token is incomplete                */
    };

    /*
     * Parser states
     */
    enum JState {
        Name,          /* In object, looking for name */
        Value,         /* In object or array, looking for value */
        Comma,         /* In object or array, looking for separator or terminator */
        Done,          /* Done processing due to end of message or error */
    };

    /*
     * Class variables
     */
    int     ent_count;
    int     ent_alloc = 100;
    ImaJson.Entry [] ent = new ImaJson.Entry[ent_alloc];
    
    char [] source;
    int     src_len;
    int     pos;
    int     left;
    int []  where = new int[100];

    String  name;
    String  value;
    int     line;
    boolean comments = false;
    
    /**
     * Allow comments.
     * There is no comment form in JSON, but when JSON is used for configuration it is useful
     * to source comments.  Java and C style comments (slash-star and slash-slash) are allowed 
     * whereever white space is allowed.
     * @param  allowComments  If true allow comments
     */
    public void setAllowComments(boolean allowComments) {
        comments = allowComments;
    }
    
    /**
     * Return the position after the parse.
     * This position can be used when framing of messages is done. 
     * Only the first object or array within the source is parsed and the
     * position after parse indicates the source position after parse.
     * Trailing whitespace but not comments are skipped.
     * #return The source position after parse
     */
    public int getPosition() {
        return pos;
    }
    
    /**
     * Return the line position after the parse.
     * The line is relative to the start of the source and atarts at 1.  
     * This can be used to indicate where an error occurred.
     * @return The line position
     */
    public int getLine() {
        return line;
    }
    
    /**
     * Get the count of entries after a parse. 
     * This is the same value which is returned on the parse() method.
     * @return The number of used entries
     */
    public int getEntryCount() {
        return ent_count;
    }

    public ImaJson.Entry getEntry(int i) {
        if (i < ent_count) {
            return ent[i];
        }
        return null;
    }

    /**
     * Get the array of entries after a parse.
     * These will change in value if parse is called again.
     * The array may be larger than the number of used entries.
     * @return An array of entries.
     */
    public ImaJson.Entry [] getEntries() {
        return ent;
    }

    /**
     * Parse a JSON message.
     * The specified string is parsed as a JSON object or array.
     * If this succeeds the count of entries is returned, otherwise an error is returned.
     * A return of -1 indicates that the JSON object is incomplete.  Therefore by adding more
     * chracters you could get a valid JSON object or array. A return code of -2 indicates that
     * the JSON object is invalid and cannot be fixed by adding additional characters.  This
     * difference is used for framing a stream containing JSON objects deliminated by the end
     * of the object.
     * @param jsonstr  A JSON string containing an object or array
     * @return  The count of entries if zero or positive, -1=incomplete, -2-invalid.
     */
    public int parse(String jsonstr) {
        JToken token;
        JState state;
        int    entnum;
        int    level = 0;
        boolean  inarray = false;
        int    rc = 0;

        line = 1;
        ent_count = 0;
        source  = jsonstr.toCharArray();
        src_len = jsonstr.length();

        /*
         * The initial entry can be an object or an array
         */
        pos = 0;
        left = src_len;
        where[0] = 0;
        token = jsonToken();
        switch (token) {
        case StartObject:  /* Outer object is an object */
            entnum = jsonNewEnt(JObject.JsonObject, null, null, level);
            state = JState.Name;
            break;
        case StartArray:   /* Outer object is an array */
            entnum = jsonNewEnt(JObject.JsonArray, null, null, level);
            state = JState.Value;
            inarray = true;
            name = null;
            break;
        case End:           /* Object is empty */
            level = -1;
            state = JState.Done;
            break;
        case Incomplete:
            return -1;
        default:
            return -2;
        }

        /*
         * Loop thru all content
         */
        while (state != JState.Done) {
            switch (state) {

            /*
             * We are expecting the name within an object
             */
            case Name:
                token = jsonToken();
                name = value;
                switch (token) {
                case EndObject:
                    if (inarray) {
                        rc = -2;
                        state = JState.Done;
                        break;
                    }
                    ent[where[level]].count = ent_count-where[level]-1;
                    if (--level >= 0) {
                        inarray = ent[where[level]].objtype == JObject.JsonArray;
                        state = JState.Comma;
                    } else {
                        state = JState.Done;
                    }
                    break;

                case String:
                    token = jsonToken();
                    if (token != JToken.Colon) {
                        rc = -2;
                        state = JState.Done;
                        break;
                    }
                    state = JState.Value;
                    break;
                default:
                    rc = -2;
                    state = JState.Done;
                    break;
                }
                break;

            /*
             * We are expecting a value
             */
            case Value:
                token = jsonToken();
                switch (token) {
                case String:
                    entnum = jsonNewEnt(JObject.JsonString, name, value, level);
                    state = JState.Comma;
                    break;
                case Integer:
                    entnum = jsonNewEnt(JObject.JsonInteger, name, value, level);
                    if (value.length()<10)
                        ent[entnum].count = Integer.valueOf(value);
                    else
                        ent[entnum].objtype = JObject.JsonNumber;
                    state = JState.Comma;
                    break;
                case Number:
                    entnum = jsonNewEnt(JObject.JsonNumber, name, value, level);
                    state = JState.Comma;
                    break;
                case StartObject:
                    entnum = jsonNewEnt(JObject.JsonObject, name, value, level);
                    where[++level] = entnum;
                    inarray = false;
                    state = JState.Name;
                    break;
                case StartArray:
                    entnum = jsonNewEnt(JObject.JsonArray, name, null, level);
                    where[++level] = entnum;
                    state = JState.Value;
                    inarray = true;
                    name = null;
                    break;
                case True:
                    entnum = jsonNewEnt(JObject.JsonTrue, name, null, level);
                    state = JState.Comma;
                    break;
                case False:
                    entnum = jsonNewEnt(JObject.JsonFalse, name, null, level);
                    state = JState.Comma;
                    break;
                case Null:
                    entnum = jsonNewEnt(JObject.JsonNull, name, null, level);
                    state = JState.Comma;
                    break;
                case EndArray:
                    if (!inarray) {
                        rc = -2;
                        state = JState.Done;
                        break;
                    }
                    ent[where[level]].count = ent_count-where[level]-1;
                    if (--level >= 0) {
                        inarray = ent[where[level]].objtype == JObject.JsonArray;
                        state = JState.Comma;
                    } else {
                        state = JState.Done;
                    }
                    break;
                default:
                    state = JState.Done;
                }
                break;

            /*
             * Comma or end of object
             */
            case Comma:
                token = jsonToken();
                switch (token) {
                /* Comma separator between objects */
                case Comma:
                    state = inarray ? JState.Value : JState.Name;
                    break;

                /* Right brace ends an object */
                case EndObject:
                    if (inarray) {
                        rc = -2;
                        state = JState.Done;
                        break;
                    }
                    ent[where[level]].count = ent_count-where[level]-1;
                    if (--level >= 0) {
                        inarray = ent[where[level]].objtype == JObject.JsonArray;
                        state = JState.Comma;
                    } else {
                        state = JState.Done;
                    }
                    break;

                /* Right bracket ends and array */
                case EndArray:
                    if (!inarray) {
                        rc = -2;
                        state = JState.Done;
                        break;
                    }
                    ent[where[level]].count = ent_count-where[level]-1;
                    if (--level >= 0) {
                        inarray = ent[where[level]].objtype == JObject.JsonArray;
                        state = JState.Comma;
                    } else {
                        state = JState.Done;
                    }
                    break;

                /* End found.  This is good if level is -1 */
                case End:
                    state = JState.Done;
                    break;
                    
                case Incomplete:
                    rc = -1;
                    state = JState.Done;
                    break;

                /* Error */
                default:
                    rc = -2;
                    state = JState.Done;
                    break;
                }
                break;
            }
        }

        /*
         * Check all objects are complete
         */
        if (rc == 0 && level >= 0) {
            rc = -1;    /* Not complete */
        }
        
        /*
         * Skip over trailing white space not including comments
         */
        if (rc == 0) {
            skipWhite();    
        }
        return rc != 0 ? rc : ent_count;
    }

    public void parse(File file) throws Exception {
    	StringBuilder sb = new StringBuilder((int)(file.length() + 1024));
    	try{
        	BufferedReader br = new BufferedReader(new FileReader(file));
	    	do {
	    		String line = br.readLine();
	    		if(line == null)
	    			break;
	    		//line = line.trim();
	    		sb.append(line);
	    	} while(true);
	    	br.close();
    	} catch (Exception ex) {
    		throw new Exception("Failed to parse JSON file: " + file.getName(),ex);
    	}
    	if(parse(sb.toString()) < 0)
    		throw new Exception("Failed to parse JSON file: " + file.getName());
    }

    /*
     * Make a new entry
     */
    int jsonNewEnt(JObject objtype, String name, String value, int level) {
        int entnum;

        if (ent_count >= ent_alloc) {
            int newalloc;
            if (ent_alloc < 25)
                newalloc = 100;
            else
                newalloc = ent_alloc*4;
            ImaJson.Entry [] newent = new ImaJson.Entry[newalloc];
            System.arraycopy(ent, 0, newent, 0, ent_alloc);
            ent = newent;
            ent_alloc = newalloc;
        }
        entnum = ent_count++;
        if (ent[entnum] == null)
            ent[entnum] = new ImaJson.Entry();
        ent[entnum].objtype = objtype;
        ent[entnum].name    = name;
        ent[entnum].value   = value;
        ent[entnum].level   = level;
        ent[entnum].line    = line;
        return entnum;
    }

    /*
     * Skip over white space at end of JSON object.
     * 
     * This is used to skip over any trailing line ends and such to make it more likely that 
     * the position will be at the end of the source.
     * 
     * Comments are not skipped, and should be discouraged when using JSON object based framing.
     */
    void skipWhite() {
        while (left-- > 0) {
            char ch = source[pos++];
            switch (ch) {
            default:
                pos--;
                return;
            case ' ':
            case '\t':
            case '\r':
            case 0x0b:
            case 0x0c:
            case '\n':
                break;
            }    
        }
    }

    /*
     * Return the next token
     */
    JToken jsonToken() {
        while (left-- > 0) {
            char ch = source[pos++];
            switch (ch) {
            case ' ':
            case '\t':
            case '\r':
            case 0x0b:
            case 0x0c:
                break;
            case '\n':
                line++;
                break;
            case '{':
                return JToken.StartObject;
            case '}':
                return JToken.EndObject;
            case '[':
                return JToken.StartArray;
            case ']':
                return JToken.EndArray;
            case ':':
                return JToken.Colon;
            case ',':
                return JToken.Comma;
            case '"':
                return jsonString();
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return jsonNumber();
            case 't':
                return jsonKeyword(JToken.True, "true", 3);
            case 'f':
                return jsonKeyword(JToken.False, "false", 4);
            case 'n':
                return jsonKeyword(JToken.Null, "null", 3);
            case '/':
                if (comments) {
                    if (left-- <= 0)
                        return JToken.Incomplete;
                    ch = source[pos++];
                    if (ch != '*' && ch != '/')
                        return JToken.Error;

                    if (ch == '*') {       /* slash,star comment */
                        while (left-- > 0) {
                            ch = source[pos++];
                            if (ch == '*') {
                                if (left-- <= 0)
                                    return JToken.Incomplete;
                                ch = source[pos++];
                                if (ch == '/')
                                    break;
                            }
                            if (ch == '\n')
                                line++;
                        }
                        if (left < 0)
                            return JToken.Incomplete; /* Error if not terminated */
                    } else {               /* slash,slash comment */
                        while (left-- > 0) {
                            ch = source[pos++];
                            if (ch == '\n')
                                line++;
                            if (ch == '\r' || ch == '\n')
                                break;
                        }
                    }
                } else {
                    return JToken.Error;     /* Comment not allowed */
                }
                break;    
            default:
                return JToken.Error;
            }
        }
        return JToken.End;
    }


    /*
     * Match a keyword
     */
    JToken jsonKeyword(JToken otype, String match, int len) {
        if (left >= len) {
            for (int i=0; i<len; i++) {
                if (source[pos+i] != match.charAt(i+1))
                    return JToken.Error;
            }
            pos += len;
            left -= len;
            value = match;
            return otype;
        }
        return JToken.Incomplete;
    }


    /*
     * Return the value of a hex digit or -1 if not a valid hex digit
     */
    static int hexValue(char ch) {
        if (ch <= '9' && ch >= '0')
            return ch-'0';
        if (ch >='A' && ch <= 'F')
            return ch-'A'+10;
        if (ch >='a' && ch <= 'f')
            return ch-'a'+10;
        return -1;
    }



    /*
     * Match a string
     * A string is any number of bytes ending with an un-escaped double quote.
     * We unescape the string in place.
     */
    JToken jsonString() {
        int    ip = pos;
        int    op = ip;
        int    i;
        int    val;
        int    digit;

        while (left > 0) {
            char ch = source[ip++];
            if (ch == '"') {
                left--;
                value = new String(source, pos, op-pos);
                pos = ip;
                return JToken.String;
            } else if (ch=='\\') {
                if (left < 2)
                    return JToken.Incomplete;
                ch = source[ip++];
                left--;
                switch (ch) {
                case 'b':   ch = 0x08;   break;
                case 'f':   ch = 0x0c;   break;
                case 'n':   ch = '\n';   break;
                case 'r':   ch = '\r';   break;
                case 't':   ch = '\t';   break;

                case 'u':
                    if (left < 4)
                        return JToken.Incomplete;
                    /* Parse the hex value.  This must be four hex digits */
                    val = 0;
                    for (i=0; i<4; i++) {
                        digit = hexValue(source[ip++]);
                        if (digit < 0)
                            return JToken.Incomplete;
                        val = val<<4 | digit;
                    }
                    left -= 4;
                    ch = (char)val;
                    break;

                case '/':
                case '"':
                case '\\':
                    break;
                default:
                    return JToken.Error;
                }
                source[op++] = ch;
            } else{
                source[op++] = ch;
            }
            left--;
        }
        return JToken.Error;
    }


    /*
     * Match a number.
     *
     * The JSON number production is quite specific and a subset of what is allowed in C or Java.
     * We check for any violations of the production. and give an error, even though in many casses
     * the correct JSON behavior would be to end the production at the point of the error.
     * This would just cause the following separator we be in error and give a less usable error code.
     */
    JToken jsonNumber() {
        int    dp = pos-1;
        int    state   = source[dp]=='-' ? 0 : 1;
        boolean  waszero = false;
        int    digits  = 0;
        left++;

        while (left > 0) {
            char ch = source[dp];
            switch (ch) {
            case '0':
                if (state==1 && waszero)
                   return JToken.Error;
                waszero = true;
                if (state == 4)
                    state = 5;
                digits++;
                break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (state == 1)
                    state = 2;
                if (state == 4)
                    state = 5;
                waszero = false;
                digits++;
                break;

            case '.':
                if (state != 1 && state != 2)
                    return JToken.Error;
                state = 3;
                digits = 0;
                break;

            case '-':
                if (state != 0 && state != 4)
                    return JToken.Error;
                state++;
                break;

            case 'E':
            case 'e':
                if (state==0 || state>3)
                    return JToken.Error;
                state = 4;
                digits = 0;
                break;

            case '+':
                if (state != 4)
                    return JToken.Error;
                state = 5;
                digits = 0;
                break;
            default:
                if ((state==3 && digits == 0) || (state==5 && digits == 0))
                    return JToken.Error;
                value = new String(source, pos-1, dp-pos+1);
                pos = dp;
                return state > 2 ? JToken.Number : JToken.Integer;
            }
            dp++;
            left--;
        }
        pos = dp;
        left = 0;
        return JToken.Incomplete;
    }


    /*
     * Compute the extra length needed for JSON escapes of a string.
     * Control characters and the quote and backslash are escaped.
     * No multi-byte characters are escaped.
     */
    static int jsonExtra(String str) {
        int extra = 0;
        for (int i=0; i<str.length(); i++) {
            char ch = str.charAt(i);
            if (ch >= ' ') {                 /* Normal characters */
                if (ch=='"' || ch=='\\')
                    extra++;
            } else {
                switch (ch) {                /* Control characters */
                case '\n':
                case '\r':
                case 0x08:    /* BS */
                case 0x09:    /* HT */
                case 0x0c:    /* FF */
                    extra++;
                    break;
                default:
                    extra += 5;              /* unicode escape */
                }
            }
        }
        return extra;
    }

    static String hexChar = "0123456789ABCDEF";

    /**
     * Copy a string to an output JSON buffer with escapes.
     * <p>
     * Escape the double quote, backslash, and all C0 control characters (0x00 - 0x1F).
     */
    public static StringBuffer escape(StringBuffer to, String from) {
        int i;
        for (i=0; i<from.length(); i++) {
            char ch = from.charAt(i);;
            if (ch >= ' ') {
                if (ch=='"' || ch=='\\')
                    to.append('\\');
                to.append(ch);
            } else {
                to.append('\\');
                switch (ch) {
                case '\n':  to.append('n');    break;
                case '\r':  to.append('r');    break;
                case 0x08:  to.append('b');    break;
                case 0x09:  to.append('t');    break;
                case 0x0c:  to.append('f');    break;
                default:
                    to.append('u');
                    to.append('0');
                    to.append('0');
                    to.append(hexChar.charAt((ch>>4)&0xf));
                    to.append(hexChar.charAt(ch&0xf));
                    break;
                }
            }
        }
        return to;
    }


    /**
     * Put out a set of bytes with JSON string escapes.
     * <p>
     * In JSON all strings are surrounded by double quotes, and the characters 
     * double quote, backslash, and control characters in the C0 range (0x00 - 0x1F)
     * are required to be escaped.
     * @param buf   The buffer to write to
     * @param str   The String to write
     */
     public static void putString(StringBuffer buf, String str) {
    	buf.append('"');
        int extra = jsonExtra(str);
        if (extra == 0) {
            buf.append(str);
        } else {
            escape(buf, str);
        }
        buf.append('"');
    }


    /**
     * Put out a Java object in JSON form.
     * <p>
     * The Java object can be null, or can be an instance of String, Number, or Boolean.
     * Number includes all of the common numeric types.
     * @param buf   The string buffer to write to
     * @param obj   The object to write (null, String, Number, or Boolean).
     */
    public static void put(StringBuffer buf, Object obj) {
    	if (obj == null) {
    		buf.append("null");
    	} else if (obj instanceof String) {
    		putString(buf, (String)obj);
    	} else if (obj instanceof Number) {
    		buf.append(""+(Number)obj);
    	} else if (obj instanceof Boolean) {
    		buf.append((Boolean)obj ? "true" : "false");
    	} else if (obj instanceof String []) {
    	    String [] val = (String[])obj;
    	    buf.append('[');
    	    for (int i=0; i<val.length; i++) {
    	        if (i != 0)
    	            buf.append(',');
    	        putString(buf, val[i]);
    	    }
    	    buf.append(']');
    	} else if (obj instanceof Map) { 
    	    @SuppressWarnings("unchecked")
            Map<String,Object> map = (Map<String,Object>)obj;
            Iterator<String> it = map.keySet().iterator();
            buf.append("{ ");
            boolean isFirst = true;
            while (it.hasNext()) {
                String name = it.next();
                Object value = map.get(name);
                if (!isFirst) {
                    buf.append(", ");
                    isFirst = false;
                }    
                putString(buf, name);
                buf.append(":");
                put(buf, value);
            }
            buf.append(" }");
    	} else {
    		buf.append("null");
    	} 		
    }


    /*
     * Get a field from a JSON object
     */
     int get(int entnum, String name) {
        int maxent;

        if (entnum < 0 || entnum >= ent_count || ent[entnum].objtype != JObject.JsonObject) {
            return -1;
        }
        maxent = entnum + ent[entnum].count;
        entnum++;
        while (entnum <= maxent) {
            if (name.equals(ent[entnum].name)) {
                return entnum;
            }
            if (ent[entnum].objtype == JObject.JsonObject || ent[entnum].objtype == JObject.JsonArray) {
                entnum += ent[entnum].count;
            } else {
                entnum++;
            }
        }
        return -1;
    }


    /**
     * Get a string from a named JSON item.
     * <p>
     * This is used to find simple items within a single level JSON object.
     * It finds the item with the specified name within the outermost object in the parsed 
     * JSON object.  If this item is not an string, it attempts to convert it to a string.
     * If the item is missing or cannot be converted to a string, return null. 
     * @param name  The name of the property
     * @return An string value for the item or null              
     */
    public String getString(String name) {
        int    entnum;

        entnum = get(0, name);
        if (entnum < 0)
            return null;
        switch (ent[entnum].objtype) {
        case JsonTrue:    return "true";
        case JsonFalse:   return "false";
        case JsonNull:    return "null";
        default:      return null;
        case JsonInteger:
        case JsonString:
        case JsonNumber:
            return ent[entnum].value;
        }
    }


    /**
     * Get an integer from a named JSON item.
     * <p>
     * This is used to find simple items within a single level JSON object.
     * It finds the item with the specified name within the outermost object in the parsed 
     * JSON object.  If this item is not an integer, it attempts to convert it to integer.
     * If the item is missing or cannot be converted to integer, return the default value. 
     * @param name  The name of the property
     * @param default_value The default value to return if the named property is missing or cannot be 
     *              converted to a integer.
     * @return An integer value for the item or the default value.             
     */
    public int getInt(String name, int default_value) {
        int    entnum;
        int    val;

        entnum = get(0, name);
        if (entnum < 0)
            return default_value;
        switch (ent[entnum].objtype) {
        case JsonInteger: return ent[entnum].count;
        case JsonTrue:    return 1;
        case JsonFalse:   return 0;
        default:      return default_value;
        case JsonString:
        case JsonNumber:
            try {
                val = Integer.valueOf(ent[entnum].value);
            } catch (Exception e) {
                val = default_value;
            }
            return val;
        }
    }

    
    /**
     * Create JSON object from a Map.
     * @param map  The map to convert to a JSON object
     * @return A String representing the JSON object
     */
    public static String fromMap(Map<String,Object> map) {
        StringBuffer sb = new StringBuffer();
        sb.append("{ ");
        Iterator<String> it = map.keySet().iterator();
        boolean isFirst = true;
        while (it.hasNext()) {
            String name = it.next();
            Object value = map.get(name);
            if (isFirst) {
                isFirst = false;
            } else {    
                sb.append(", ");
            }    
            putString(sb, name);
            sb.append(":");
            put(sb, value);
        }
        sb.append(" }\n");
        return sb.toString();
    }
    
    /**
     * Return the value of an item as an object.
     * @param entnum  The entry number in the parsed object
     * @return The value of the entry as a Java object.
     */
    public Object getValue(int entnum) {
        if (entnum >= ent_count) {
            return null;
        }
        switch (ent[entnum].objtype) {
        case JsonString:  return ent[entnum].value;
        case JsonInteger: return ent[entnum].count;
        case JsonNumber:  return Double.parseDouble(ent[entnum].value);
        case JsonTrue:    return true;
        case JsonFalse:   return false;
        default:      return null;
        }
    }
 
    /**
     * Return a string array from a parsed JSON array.
     * <p>
     * The specified entry in the parsed JSON object must be an array.
     * The value of each object in the array is returned as an array of
     * Strings.  The items in the array are converted as required but must
     * not be objects or arrays.
     * @param entnum  The entry number within the parsed object.
     */
    public String [] toArray(int entnum) {
        int  maxent;
        if (entnum >= ent_count || ent[entnum].objtype != JObject.JsonArray) {
            return null;
        }    
        String [] ret = new String[ent[entnum].count];        
        maxent = ent[entnum].count + entnum + 1;  
        int i;
        int which = 0;
        for (i=entnum+1; i<maxent; i++) {
            switch (ent[i].objtype) {
            case JsonObject:
            case JsonArray:
                return null; 
            case JsonString:
            case JsonNumber:
                ret[which++] = ent[i].value;
                break;
            case JsonInteger:
                ret[which++] = ""+ent[i].count;
                break;
            case JsonTrue:
                ret[which++] = "true";
                break;
            case JsonFalse:
                ret[which++] = "false";
                break;
            case JsonNull:
                ret[which++] = null;
                break; 
            }    
        }
        return ret;
    }
    
    /**
     * Get a boolean from a named JSON item.
     * <p>
     * This is used to find simple items within a single level JSON object.
     * It finds the item with the specified name within the outermost object in the parsed 
     * JSON object.  If this item is not a boolean, it attempts to convert it to boolean.
     * If the item is missing or cannot be converted to boolean, return the default value. 
     * @param name  The name of the property
     * @param default_value The default value to return if the named property is missing or cannot be 
     *              converted to a boolean.
     * @return A boolean value for the item or the default value.            
     */
     public boolean getBoolean(String name, boolean deflt) {
         int    entnum;

         entnum = get(0, name);
         if (entnum < 0)
             return deflt;
         switch (ent[entnum].objtype) {
         case JsonTrue:    return true;
         case JsonFalse:   return false;
         case JsonNull:    return deflt;
         default:      return deflt;
         case JsonInteger: return ent[entnum].count != 0;
         case JsonString:  
             if ("true".equalsIgnoreCase(ent[entnum].value))
                 return true;
             if ("false".equalsIgnoreCase(ent[entnum].value))
                 return false;
             return deflt;    
         }
     }

    /**
     * Make a Java map object from a JSON object.
     * 
     * @param entnum  The entry number in the parsed Java object.  This must be a JSON object.
     * @return A map or null to indicate an error
     */
    public Map<String,Object> toMap(int entnum) {
        int  maxent;
        HashMap<String,Object> map = new HashMap<String,Object>();
        if (entnum >= ent_count || ent[entnum].objtype != JObject.JsonObject) {
            return null;
        }    
        maxent = ent[entnum].count + entnum + 1;
        int i;
        for (i=entnum+1; i<maxent; i++) {
            switch (ent[i].objtype) {
            case JsonObject:
                map.put(ent[i].name, toMap(i));
                i += ent[i].count;
                break;
            case JsonArray:
                String [] sa = new String [ent[i].count];
                for (int j=0; j<ent[i].count; j++)
                    sa[j] = ent[i+j+1].value;
                map.put(ent[i].name, sa);
                i += ent[i].count;
                break;
            case JsonInteger:
                map.put(ent[i].name, ent[i].count);
                break;
            case JsonNumber:
                map.put(ent[i].name, Double.parseDouble(ent[i].value));
                break;
            case JsonString:
                map.put(ent[i].name, ent[i].value);
                break;
            case JsonTrue:
                map.put(ent[i].name, true);
                break;
            case JsonFalse:
                map.put(ent[i].name, false);
                break;
            case JsonNull:
                map.put(ent[i].name, null);
                break;
            }
        }
        return map;
    }
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuffer sb = new StringBuffer();
        toString(sb, 0, 0);
        return sb.toString();
    }

    /*
     * Create a string respresentation of a JSON object
     */
    int toString(StringBuffer sb, int entnum, int options) {
        int  maxent;
        int  startent = entnum;
        boolean  first;

        if (entnum < 0 || entnum >= ent_count) {
            return -1;
        }
        maxent = entnum;
        if (ent[entnum].name != null) {
            sb.append('"');
            escape(sb, ent[entnum].name);
            sb.append("\":");
        }
        switch (ent[entnum].objtype) {
        case JsonObject:
            sb.append('{');
            maxent = entnum + ent[entnum].count;
            break;
        case JsonArray:
            sb.append('[');
            maxent = entnum + ent[entnum].count;
            break;
        case JsonInteger:
            sb.append(""+ent[entnum].count);
            break;
        case JsonNumber:
            sb.append(ent[entnum].value);
            break;
        case JsonString:
            sb.append('"');
            escape(sb, ent[entnum].value);
            sb.append('"');
            break;
        case JsonTrue:
            sb.append("true");
            break;
        case JsonFalse:
            sb.append("false");
            break;
        case JsonNull:
            sb.append("null");
            break;
        }

        first = true;
        entnum++;
        while (entnum > 0 && entnum <= maxent) {
            if (!first)
                sb.append(',');
            else
                first = false;
            entnum = toString(sb, entnum, options);
        }

        /*
         * For composite objects put out the trailing bytes
         */
        switch (ent[startent].objtype) {
        case JsonObject: sb.append('}');  break;
        case JsonArray:  sb.append(']');  break;
        }
        return entnum;
    }

}
