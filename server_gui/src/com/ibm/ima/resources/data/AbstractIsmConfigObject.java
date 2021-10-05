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

package com.ibm.ima.resources.data;

import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.icu.lang.UCharacter;
import com.ibm.icu.text.UTF16;
import com.ibm.ima.util.IsmLogger;

/**
 * The REST representation of a certificate response
 *
 */
public abstract class AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = AbstractIsmConfigObject.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    protected final static Pattern NAME_INVALID_CHAR_PATTERN = Pattern.compile("[\\u0000-\\u001F,=\\u0022\\u005C]", Pattern.CANON_EQ); // No characters < 0x20 allowed
    protected final static int NAME_MINIMUM_INITIAL_CHAR = 0x41; // First characters must be > 0x40 ('A')
    protected static final Pattern INVALID_FILTER_VARIABLES = Pattern.compile("\\$\\{UserID\\}|\\$\\{GroupID\\}|\\$\\{ClientID\\}|\\$\\{CommonName\\}");
    protected static final Pattern INVALID_CP_CLIENTID_FILTER_VARIABLES = Pattern.compile("\\$\\{GroupID\\}|\\$\\{ClientID\\}");

    protected final static int NAME_MAX_LENGTH = 256;
    protected final static int DEFAULT_MAX_LENGTH = 1024;
    protected final static int IP_ADDRESS_MAX_LENGTH = 39;
    protected final static int FQ_HOSTNAME_MAX_LENGTH = 255;
    protected final static int PORT_MIN = 1;
    protected final static int PORT_MAX = 65535;

    public enum VALIDATION_RESULT {
        OK                          (0,   ""),
        VALUE_EMPTY                 (1,   "CWLNA5012"),
        VALUE_TOO_LONG              (2,   "CWLNA5013"),
        INVALID_CHARS               (3,   "CWLNA5014"),
        INVALID_START_CHAR          (4,   "CWLNA5015"),
        LEADING_OR_TRAILING_BLANKS  (5,   "CWLNA5016"),
        REFERENCED_OBJECT_NOT_FOUND (7,   "CWLNA5017"),
        INVALID_ENUM                (8,   "CWLNA5028"),
        VALUE_OUT_OF_RANGE          (9,   "CWLNA5050"),
        UNIT_INVALID                (10,  "CWLNA5053"),
        NOT_AN_INTEGER              (11,  "CWLNA5054"),
        INVALID_WILDCARD            (12,  "CWLNA5094"),
        VALUE_TOO_LONG_CHARS        (13,  "CWLNA5100"),
        INVALID_SYS_TOPIC           (14,  "CWLNA5108"),
        INVALID_TOPIC_MONITOR       (15,  "CWLNA5109"),
        MUST_BE_EMPTY               (16,  "CWLNA5128"),
        INVALID_URL_SCHEME          (17,  "CWLNA5139"),
        INCOMPATIBLE_PROTOCOL       (18,  "CWLNA5141"),
        INVALID_VARIABLES           (19,  "CWLNA5143"),
        UNEXPECTED_ERROR            (100, "CWLNA5000");

        private final int rc;
        private final String messageID;

        private VALIDATION_RESULT(final int rc, final String messageID) {
            this.rc = rc;
            this.messageID = messageID;
        }

        /**
         * @return the rc
         */
        @JsonProperty("RC")
        public int getRc() {
            return rc;
        }

        /**
         * @return the messageID
         */
        public String getMessageID() {
            return messageID;
        }
    }
    
    /**
     * Constructor
     */
    public AbstractIsmConfigObject() {

    }
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(this.getClass().getCanonicalName(), "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }

    /**
     * Validates the configuration object
     * @return {@link ValidationResult}}
     */
    public abstract ValidationResult validate();


    /**
     * Validates the name. If the name is null, contains only blanks,
     * is too long, or contains invalid characters, returns a ValidationResult object.
     * @param name The name to validate
     * @param propertyName The name of the property being validated
     * @return ValidationResult
     */
    public ValidationResult validateName(String name, String propertyName)  {
        ValidationResult result = new ValidationResult();
        if (propertyName == null) {
            propertyName = "name";
        }
        String trimmedName = trimUtil(name);
        if (trimmedName == null || trimmedName.length() == 0) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {propertyName});
        } else if (!trimmedName.equals(name)) {
            result.setResult(VALIDATION_RESULT.LEADING_OR_TRAILING_BLANKS);
            result.setParams(new Object[] {propertyName});
        } else if (!checkUtfLength(name, NAME_MAX_LENGTH)) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, NAME_MAX_LENGTH});
        } else if (!isFirstCharValid(name, NAME_MINIMUM_INITIAL_CHAR)){
            result.setResult(VALIDATION_RESULT.INVALID_START_CHAR);
            result.setParams(new Object[] {propertyName, ""});
        } else {
            String invalidChar = containsChars(name, NAME_INVALID_CHAR_PATTERN);
            if (invalidChar != null) {
                result.setResult(VALIDATION_RESULT.INVALID_CHARS);
                result.setParams(new Object[] {propertyName, invalidChar});
            }
        }

        return result;
    }


    /**
     * Checks to see if the specified value starts with a character >= minChar
     * @param value The value to check
     * @param minChar The minimum valid character
     * @return true if the first character is valid
     */
    protected static boolean isFirstCharValid(String value, int minChar) {
        if (value != null) {
            int firstChar = UTF16.charAt(value, 0);
            return ( firstChar >= minChar);
        }
        return false;
    }

    /**
     * Checks the value against the pattern. If the pattern matches any portion of
     * the value, returns the first match.
     * @param value
     * @param pattern
     * @return first matching char or null if none
     */
    protected static String containsChars(String value, Pattern pattern) {
        logger.traceEntry(CLAS, "containsChars", new Object[]{value,pattern});
        String result = null;
        if (value != null && pattern != null) {
            Matcher m = pattern.matcher(value);
            if (m.find()) {
                int charIndex = m.start();
                result = value.substring(charIndex, charIndex+1);
            }
        }
        logger.traceExit(CLAS, "containsChars", result);
        return result;
    }

    /**
     * Checks the value against the pattern. If the pattern matches 
     * the value, returns true.
     * @param value
     * @param pattern
     * @return true if matches
     */
    protected static boolean matches(String value, Pattern pattern) {
        if (value != null && pattern != null) {
            Matcher m = pattern.matcher(value);
            return m.matches();
        }
        return false;
    }

    /**
     * Returns a string with both leading and trailing white spaces removed.
     * This method is applicable for both single and double byte characters.
     * 
     * @param untrimmedString
     *            The string which leading and trailing space is to be removed.
     * @return Returns the string with leading and trailing white space removed.
     *         If the string contains no leading or trailing white space then
     *         the unmodified string is returned. If a null or empty string is
     *         passed then the null or empty string is returned.
     */
    public static String trimUtil(String untrimmedString) {

        String trimmedString = null;
        if (untrimmedString == null || untrimmedString.equals("")) {
            trimmedString = untrimmedString;
        } else {
            //lets the regular string trim method too
            trimmedString = untrimmedString.trim();
            if ( !trimmedString.equals("") ) {
                int beginPos = 0;
                int endPos = 0;

                int cp = UTF16.charAt(untrimmedString, beginPos);
                if (UCharacter.isWhitespace(cp)) {
                    beginPos += UTF16.getCharCount(cp);
                    while (beginPos < untrimmedString.length()) {
                        cp = UTF16.charAt(untrimmedString, beginPos);
                        if (!UCharacter.isWhitespace(cp)) {
                            break;
                        }
                        beginPos += UTF16.getCharCount(cp);
                    }
                }

                //endPos = beginPos;
                endPos = untrimmedString.length() - 1;
                while (endPos > beginPos) {
                    cp = UTF16.charAt(untrimmedString, endPos);
                    if (!UCharacter.isWhitespace(cp)) {
                        break;
                    }
                    endPos -= UTF16.getCharCount(cp);
                }
                trimmedString = untrimmedString.substring(beginPos, endPos+1);//add one to include the char at that pos

            }
        }
        return trimmedString;
    }
    
    public static boolean checkUtfLength(String s, int maxLength) {
    	logger.traceEntry(CLAS, "checkUtfLength", new Object[] {s, ""+maxLength});
    	boolean ok = true;
    	if (s != null && !s.isEmpty()) {
    		int codePointCount = s.codePointCount(0, s.length());
    		logger.trace("CLAS", "checkUtfLength", "codePointCount=" + codePointCount);
    		ok = codePointCount <= maxLength;
    	}
    	logger.traceExit(CLAS, "checkUtfLength", ok);
    	return ok;
    }
    
    /**
     * If the specified string is longer than the specified length, set the validation result 
     * appropriately and return false;  Otherwise, return true.
     * @param s The string to check (if null, returns true)
     * @param maxLength maximum length
     * @param name Property name to include in message
     * @param result ValidationResult object to update
     * @return true if OK, false otherwise
     */
    public static boolean checkUtfLength(String s, int maxLength, String name, ValidationResult result) {
    	if (s == null) {
    		return true;
    	}
        if (!checkUtfLength(s, maxLength)) {
        	if (result != null) {
        		result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
        		result.setParams(new Object[] {name, maxLength});
        	}
        	return false;
        }    	
        return true;
    }
    
    /**
     * If the specified string representation of the integer is out of range, or is a string with length > 0
     * and is not a valid integer, set the validation result appropriately and return false;  Otherwise, return true.
     * @param iString The string representation of the integer to check
     * @param min The minimum, inclusive
     * @param max The maximum, inclusive
     * @param name Property name to include in message
     * @param result ValidationResult object to update
     * @return true if OK, false otherwise
     */
    public static boolean checkRange(String iString, int min, int max, String name, ValidationResult result) {
    	if (iString == null || iString.isEmpty()) {
    		return true;
    	}
    	int i = 0;
    	
    	try {
    		i = Integer.parseInt(iString);
    	} catch (NumberFormatException nfe) {
            result.setResult(VALIDATION_RESULT.NOT_AN_INTEGER);
            result.setParams(new Object[] {name});
            return false;    		
    	}
    	
    	if (i < min || i > max) {
        	if (result != null) {
        		result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
        		result.setParams(new Object[] {name, min, max});
        	}
        	return false;
        }    	
        return true;
    }



    /**
     *
     */
    public static class ValidationResult {

        private VALIDATION_RESULT result = VALIDATION_RESULT.OK;
        private Object[] params = null;
        private String errorMessage = null;  // Can be set in lieu of using the VALIDATION_RESULT error message
        private String errorMessageID = null;// Can be set in lieu of using the VALIDATION_RESULT error message

        public ValidationResult () {
        }

        /**
         * @param result
         * @param params
         */
        public ValidationResult(VALIDATION_RESULT result, Object[] params) {
            this.result = result;
            this.params = params;
        }

        /**
         * @return the result
         */
        public VALIDATION_RESULT getResult() {
            return result;
        }

        /**
         * @param result the result to set
         */
        public void setResult(VALIDATION_RESULT result) {
            this.result = result;
        }

        /**
         * Alternative to using VALIDATION_RESULT message.
         * @param errorMessage the errorMessage to set
         */
        public void setErrorMessage(String errorMessage) {
            this.errorMessage = errorMessage;
        }

        /**
         * Alternative to using VALIDATION_RESULT message.
         * @param errorMessageID the errorMessageID to set
         */
        public void setErrorMessageID(String errorMessageID) {
            this.errorMessageID = errorMessageID;
        }


        /**
         * @return the errorMessageID
         */
        public String getErrorMessageID() {
            return errorMessageID != null ? errorMessageID : result.getMessageID();
        }

        /**
         * @return the params
         */
        public Object[] getParams() {
            return params;
        }

        /**
         * @param params the params to set
         */
        public void setParams(Object[] params) {
            this.params = params;
        }

        /**
         * Gets the error message based on the following:
         * 
         * If an error message was set via {link {@link #setErrorMessage(String)},
         * that message will be returned.
         * 
         * Else if an error message ID was set via {@link #setErrorMessageID(String)}
         * a message will be built using the specified locale and any parameters set
         * via {@link #setParams(Object[])}.
         * 
         * Else if if the result is not VALIDATION_RESULT.OK, a message will be built
         * based on {@link #getResult()} and any parameters set via {@link #setParams(Object[])}.
         * 
         * @param locale The locale to get the error message in
         * @return The error message or null if none
         */
        public String getErrorMessage(Locale locale) {

            if (errorMessage != null) {
                return errorMessage;
            }

            String messageID = getErrorMessageID();

            if (messageID == null) {
                return null;
            }

            return logger.getFormattedMessage(result.getMessageID(), getParams(), locale, true);
        }

        /**
         * Convenience check to see if everything was OK.
         */
        public boolean isOK() {
            return result == VALIDATION_RESULT.OK;
        }


    }



}
