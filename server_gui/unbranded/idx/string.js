/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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

define(["dojo/_base/lang","idx/main","dojo/string"], function (dLang,iMain,dString)
{
    /**
 	 * @name idx.string
 	 * @namespace Provides Javascript utility methods in addition to those provided by Dojo.
 	 */
	var iString = dLang.getObject("string", true, iMain);
	var regexChars = "\\[]()^.+*{}?!=-";
	var regexCache = {};
	var regexCacheCount = 0;
	var regexCacheMax = 50;

	/**
	 * @public 
	 * @function
	 * @name idx.string.unescapedIndexOf
	 * @description Searches the specified text for the first unescaped index of
	 *              a character contained in the specified string of characters.
	 * @param {String} text The text to search.
	 * @param {String} chars The characters for which to search (any of these).
	 * @param {String} escaper The optional character to use for escaping (defaults to "\\").
	 * @param {Number} limit The optional limit to the number of indexes desired in case only
	 *                 the first or first few are needed (use null, undefined, zero, false, or
	 * 	               a negative number to indicate no limit).
	 * @returns {Number[]} The array of indexes at which one of the specified characters
	 *                     is found or an empty array if not found.
	 */
	iString.unescapedIndexesOf = function(/*string*/ text, 
										  /*string*/ chars, 
										  /*string*/ escaper,
										  /*number*/ limit) {
		if (!chars || (chars.length == 0)) {
			throw "One or more characters must be provided to search for unescaped characters: "
				  + " chars=[ " + chars + " ], escaper=[ " + escaper + " ], text=[ " + text + " ]";
		}
		if (!escaper || escaper.length == 0) {
			escaper = "\\";
		}
		if (escaper.length > 1) {
			throw "The escaper must be a single character for unescaped character search: "
				  + " escaper=[ " + escaper + " ], chars=[ " + chars
				  + " ], text=[ " + text + " ]";
		}
		if (chars.indexOf(escaper) >= 0) {
			throw "The escaping character cannot also be a search character for unescaped character search: "
				  + " escaper=[ " +  escaper + " ], separators=[ " + separators
				+ " ] text=[ " + text + " ]";
			
		}
		if ((limit === null) || (limit === undefined) || (limit === 0) || (limit === false)) {
			limit = -1;
		}
		if (limit < 0) limit = 0;
		var result = [];
		var index = 0;
		var escaping = false;
		var current = null;
		for (index = 0; index < text.length; index++) {
			current = text.charAt(index);
			if (escaping) {
				escaping = false;
				continue;
			}
			if (current == escaper) {
				escaping = true;
				continue;
			}
			if (chars.indexOf(current) >= 0) {
				result.push(index);
				if (limit && (result.length == limit)) break;
			}
		}
		return result;
	};

	/**
	 * @public 
	 * @function
	 * @name idx.string.unescapedSplit
	 * @description Similar to iString.parseTokens, this function will split the
	 * 	            the specified text on unescaped separator characters, but will 
	 *              NOT remove any escaping in the original text so that the elements
	 *              in the returned array will still contain any original escaping.
	 * @param {String} text The text to split.
	 * @param {String} separators The optional string containing the separator 
	 *                 characters (defaults to ",")
	 * @param {String} escaper The optional character to use for escaping (defaults to "\\").
	 * @param {Number} limit The optional limit to the number of tokens desired in case only
	 *                 the first or first few are needed (use null, undefined, zero, false, or
	 * 	               a negative number to indicate no limit).
	 * @returns {String[]} Return an array of string parts.
	 */
	iString.unescapedSplit = function(/*string*/  text, 
	                                  /*string*/  separators, 
								      /*string*/  escaper,
								  	/*number*/ limit) {
		if (!separators || (separators.length == 0)) {
			separators = ",";
		}
		if (!escaper || escaper.length == 0) {
			escaper = "\\";
		}
		if (escaper.length > 1) {
			throw "The escaper must be a single character for escaped split: "
				  + " escaper=[ " + escaper + " ], separators=[ " + separators
				  + " ], text=[ " + text + " ]";
		}
		if (separators.indexOf(escaper) >= 0) {
			throw "The escaping character cannot also be a separator for escaped split: "
				  + " escaper=[ " +  escaper + " ], separators=[ " + separators
				+ " ] text=[ " + text + " ]";
			
		}
		if ((limit === null) || (limit === undefined) || (limit === 0) || (limit === false)) {
			limit = -1;
		}
		if (limit < 0) limit = 0;
		var result = [];
		var index = 0;
		var escaping = false;
		var current = null;
		var start = 0;
		for (index = 0; index < text.length; index++) {
			current = text.charAt(index);
			if (escaping) {
				escaping = false;
				continue;
			}
			if (current == escaper) {
				escaping = true;
				continue;
			}
			if (separators.indexOf(current) >= 0) {
				if (start == index) {
					result.push("");
				} else {
					result.push(text.substring(start,index));
				}
				start = index + 1;
				if (limit && (result.length == limit)) break;
			}
		}
		if ((!limit) || (limit && result.length < limit)) {
			if (start == text.length) {
				result.push("");
			} else {
				result.push(text.substring(start));
			}
		}
		return result;						
	};
	
	/**
	 * @public 
	 * @function
	 * @name idx.string.parseTokens
	 * @description Similar to iString.unescapedSplit, this function will split the
	 * 	            the specified text on unescaped separator characters, but will 
	 *              also remove any escaping in the original text when creating the
	 *              tokens unless those escaped characters are included in the optional
	 *              "specialChars" parameter (in which case they remain escaped if 
	 *              they are escaped in the original).
	 * @param {String} text The text to split.
	 * @param {String} separators The optional string containing the separator 
	 *        characters (defaults to ",")
	 * @param {String} escaper The optional character to use for escaping (defaults to "\\").
	 * @param {String} specialChars The optional string of special characters that if escaped
	 *                              in the original text should remain escaped in the tokens.
	 * @returns {String[]} Return an array of string parts.
	 */
	iString.parseTokens = function(/*string*/  text, 
	                               /*string*/  separators, 
								   /*string*/  escaper,
								   /*string*/  specialChars) {
		if (!separators || (separators.length == 0)) {
			separators = ",";
		}
		if (!escaper || escaper.length == 0) {
			escaper = "\\";
		}
		if (escaper.length > 1) {
			throw "The escaper must be a single character for token parsing: "
				  + " escaper=[ " + escaper + " ], separators=[ " + separators
				  + " ], text=[ " + text + " ]";
		}
		if (separators.indexOf(escaper) >= 0) {
			throw "The escaping character cannot also be a separator for token parsing: "
				  + " escaper=[ " +  escaper + " ], separators=[ " + separators
				+ " ] text=[ " + text + " ]";
			
		}
		if (specialChars && specialChars.length == 0) {
			specialChars = null;
		}
		var index = 0;
		var result = [];
		var start = 0;
		var escaping = false;
		var part = "";
		var end = -1;
		var current = null;
		for (index = 0; index < text.length; index++) {
			current = text.charAt(index);
			end = -1;
			if (!escaping && current==escaper) {
				if (specialChars && ((index+1)<text.length) 
				    && (specialChars.indexOf(current)>=0)) {
					// we have an escaped special character
					index++;
					continue;
				}
				
				// take the characters up to and excluding the current
				part = part + text.substring(start, index);
				
				// set the starting position to the next character
				start = index + 1;
				escaping = true;
				continue;
			}
			// check if we are escaping
			if (escaping) {
				escaping = false;
				continue;
			}
			// check if the current character is a separator
			if (separators.indexOf(current) >= 0) {
				end = index;
			}
							
			if (end >= 0) {
				part = part + text.substring(start, end);
				start = end + 1;
				result.push(part);
				part = "";
			}
		}
		
		// get the last part
		if ((end < 0) && (start < text.length)) {
			part = part + text.substring(start);
			result.push(part);
		} else if ((end >= 0)||escaping) {
			result.push(part);
		}
				
		return result;
	};
		
	/**
	 * @public 
	 * @function
	 * @name idx.string.escapedChars
	 * @description Escapes the specified characters in the specified text using the 
	 *              specified escape character.  The escape character is also escaped
	 *              using itself.
	 * @param {String} text The text to be escaped.
	 * @param {String} The string containing the characters to be escaped.
	 * @param {String} escaper The optional character to use for escaping (defaults to "\\").
	 * @returns {String[]} Return the escaped text.
	 */
	iString.escapeChars = function(/*string*/ text, /*string*/ chars, /*string*/ escaper) {
		if (!chars || (chars.length == 0)) {
			chars = "";
		}
		if (!escaper || escaper.length == 0) {
			escaper = "\\";
		}
		if (escaper.length > 1) {
			throw "The escaper must be a single character for escaped split: "
				  + " escaper=[ " + escaper + " ], chars=[ " + chars
				  + " ], text=[ " + text + " ]";
		}
		if (chars.indexOf(escaper) >= 0) {
			throw "The escaping character cannot also be a separator for escaped split: "
				  + " escaper=[ " +  escaper + " ], chars=[ " + chars
				+ " ] text=[ " + text + " ]";
			
		}
		var chars = chars + escaper;
		var regex = regexCache[chars];
		var pattern = "";
		if (! regex) {
			pattern = "([";
			for (index = 0; index < chars.length; index++) {
				if (regexChars.indexOf(chars.charAt(index)) >= 0) {
					pattern = pattern + "\\";
				}
				pattern = pattern + chars.charAt(index);
			}
			pattern = pattern + "])";
			try {
				regex = new RegExp(pattern, "g");
			} catch (e) {
				console.log("Pattern: " + pattern);
				throw e;
			}
			if (regexCacheCount < regexCacheMax) {
				regexCache[chars] = regex;
				regexCacheCount++;
			}
		}
		return text.replace(regex, escaper + "$1");
	};
		
	/**
	 * @public 
	 * @function
	 * @name idx.string.escapedJoin
	 * @description Joins the array into a single string separated by the specified 
	 *              separator and using the specified escaper to escape the separator.
	 * @param {String[]} arr The array of objects to join as a string.
	 * @param {String} separator The single-character string containing the separator character.
	 * @param {String} escaper The optional single character to use for escaping (defaults to "\\")
	 * @returns {String[]} Return an array of string parts.
	 */
	iString.escapedJoin = function(/*array*/ arr, /*string*/ separator, /*string*/ escaper) {
		if (!separator || (separator.length == 0)) {
			separator = ",";
		}
		if (separator.length > 1) {
			throw "Only one separator character can be used in escapedJoin: "
					" separator=[ " + separator + " ], text=[ " + text + " ]";
		}
		if (!escaper || escaper.length == 0) {
			escaper = "\\";
		}
		if (escaper.length > 1) {
			throw "The escaper must be a single character for escaped split: "
				  + " escaper=[ " + escaper + " ], separator=[ " + separator
				  + " ], text=[ " + text + " ]";
		}
		if (separator.indexOf(escaper) >= 0) {
			throw "The escaping character cannot also be a separator for escaped split: "
				  + " escaper=[ " +  escaper + " ], separator=[ " + separator
				+ " ] text=[ " + text + " ]";
			
		}
	
		var index = 0;
		var result = "";
		var part = null;
		var prefix = "";
		
		for (index = 0; index < arr.length; index++) {
			part = arr[index];
			part = iString.escapeChars(part, separator, escaper);
			result = result + prefix + part;
			prefix = separator;
		}
		
		return result;
	};
	
	/**
	 * @public 
	 * @function
	 * @name idx.string.startsWith
	 * @description Checks if the specified text starts with the specified prefix.
	 * @param {String} text The text to look at.
	 * @param {String} prefix The prefix to check for.
	 * @returns {Boolean} Return true if string "text" starts with "prefix"
	 */
	iString.startsWith = function(/*string*/ text, /*string*/ prefix){
		return (dLang.isString(text) && dLang.isString(prefix) && text.indexOf(prefix) === 0); // Boolean
	};
	
	/**
	 * @public 
	 * @function
	 * @name idx.string.endsWith
	 * @description Checks if the specified text ends with the specified suffix.
	 * @param {String} text The text to look at.
	 * @param {String} suffix The suffix to check for.
	 * @returns {Boolean} Return true if string "text" ends with "suffix"
	 */
	iString.endsWith = function(/*string*/ text, /*string*/ suffix){
		return (dLang.isString(text) && dLang.isString(suffix) && text.lastIndexOf(suffix) === text.length - suffix.length); // Boolean
	};

	/**	
	 * @public 
	 * @function
	 * @name idx.string.equalsIgnoreCase
	 * @description Case insensitive check for string equality.
	 * @param {String} s1 The first string.
	 * @param {String} s2 The second string.
	 * @returns {Boolean} Return true if string "s1" equals to "s2" with ignoring case
	 */
	iString.equalsIgnoreCase = function(/*string*/ s1, /*string*/ s2){
		return (dLang.isString(s1) && dLang.isString(s2) && s1.toLowerCase() === s2.toLowerCase()); // Boolean
	};
	
	/**	
	 * @public 
	 * @function
	 * @name idx.string.isNumber
	 * @description Checks if the specified parameter is a number and is finite.
	 * @param {Number} n The value to check.
	 * @returns {Boolean} Return true if 'n' is a Number
	 */
	iString.isNumber = function(/*number*/ n){
		return (typeof n == "number" && isFinite(n)); // Boolean
	};
	
	/**
	 * @public 
	 * @function
	 * @name idx.string.nullTrim
	 * @description Trims the specified string, and if it is empty after trimming, returns null.  
	 *              If the specified parameter is null or undefined, then null is returned.
	 * @param {String} str the string to trim
	 * @returns {String} Trimmed string or null if nothing left
	 */
    iString.nullTrim = function(/*String*/ str) {
            if (! str) return null;
            var result = dString.trim(str);
            return (result.length == 0) ? null : result;
        };
        
     /**
	 * @public 
	 * @function
	 * @name idx.string.propToLabel
	 * @description Converts a property name that is typically separated by camel case into a
	 *              psuedo label (i.e.: one that is not translated but based off the property
	 *              name but formatted nicer).  This method converts dots/periods into slashes. 
	 * @param {String} propName The property name to convert.
	 * @returns {String} The converted psuedo-label.
      *
      */
     iString.propToLabel = function(/*String*/ propName) {
     	if (!propName) return propName;
     	
	    // split the property name at any case switches or underscores
	    propName = dString.trim(propName);
	    var upperProp = propName.toUpperCase();
	    var lowerProp = propName.toLowerCase();
	    var index = 0;
	    var result = "";
	    var prevResult = "";
	    var prevUpper = false;
	    var prevLower = false;

		for (index = 0; index < propName.length; index++) {
	    	var upperChar = upperProp.charAt(index);
	        var lowerChar = lowerProp.charAt(index);
	        var origChar  = propName.charAt(index);

	        var upper = ((upperChar == origChar) && (lowerChar != origChar));
	        var lower = ((lowerChar == origChar) && (upperChar != origChar));

	        // check for spaces or underscores
	        if ((origChar == "_") || (origChar == " ")) {
	        	if (prevResult == " ") continue;
	         	prevResult = " ";
	            prevUpper  = upper;
	            prevLower  = lower;
	            result = result + prevResult;
	            continue;
	        }
	           
	        // check for dot notation
	        if (origChar == ".") {
	        	prevResult = "/";
	        	prevUpper = upper;
	        	prevLower = lower;
	        	result = result + " " + prevResult + " ";
	        	continue;
	        }

	        // check if this is the first character
	        if ((index == 0) || (prevResult == " ")) {
	        	prevResult = upperChar; 
	            prevUpper  = upper;
	            prevLower  = lower;
	            result = result + prevResult;
	            continue;
	        }

	        if ((!upper) && (!lower)) {
	        	if (prevUpper || prevLower) {
	            	result = result + " ";
	            }

	            // the character is not alphabetic, and neither is this one
	            prevUpper = upper;
	            prevLower = lower;
	            prevResult = origChar;

	            result = result + prevResult;
	            continue;
	        }

	        if ((!prevUpper) && (!prevLower)) {
	        	// previous character was non-alpha, but this one is alpha
	        	var prevSlash = (prevResult == "/");
	            prevUpper = upper;
	            prevLower = lower;
	            prevResult = upperChar;
	            if (prevSlash) {
	            	result = result + prevResult;
	            } else {
	            	result = result + " " + prevResult;
	            }
	            continue;
	        }

	        // if we get here then both the previous and current character are
	        // alphabetic characters so we need to detect shifts in case
	        if (upper && prevLower) {
	        	// we have switched cases
	            prevResult = upperChar;
	            prevUpper  = upper;
	            prevLower = lower;
	            result = result + " " + prevResult;
	            continue;
	        }

	        // if we get here then we simply use the lower-case version
	        prevResult = lowerChar;
	        prevUpper  = upper;
	        prevLower  = lower;
	        result = result + prevResult;
	 	}
	    
	    // return the result
	    return result;        
     };
     
     return iString;
});
