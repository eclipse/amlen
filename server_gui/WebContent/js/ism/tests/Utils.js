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

define([], function() {
	
	/**
	 * This Utilities class should not contain anything MessageSight specific.  Use IsmUtils instead.
	 */

	var _getBaseUri = function() {
        var baseUri = location.pathname.split("/").slice(0,1).join("/");
        if (baseUri.length === 0 || baseUri[baseUri.length-1] !== "/") {
            baseUri += "/"; 
        }
        return baseUri;
    };
	
	var Utils = {
	    
		getBaseUri: function () {
			return _getBaseUri();
		},
		
	    compareObjects: function(expected, actual) {
	        if (!expected || !actual) {
	            return "expected or actual are not defined";
	        } 
	        for (var prop in expected) {
	            if (expected[prop] !== actual[prop]) {
	                return "Mismatch for prop="+prop+"; expected: " + expected[prop] + ", actual: " + actual[prop];
	            }
	        }
	        return "OK";
	    },
	    
	    // create a string of the specified length
	    createString: function(length) {
	        var s = "abcdefghijklmnopqrstuvwxyz_";
	        while (s.length < length) {
	            s = s + s;
	        }
	        s = s.substring(0, length);
	        return s;
	    },
	    
	    validationErrors: {
	        VALUE_EMPTY:                   "CWLNA5012",
	        VALUE_TOO_LONG:                "CWLNA5013",
	        INVALID_CHARS:                 "CWLNA5014",
	        INVALID_START_CHAR:            "CWLNA5015",
	        LEADING_OR_TRAILING_BLANKS:    "CWLNA5016",
	        REFERENCED_OBJECT_NOT_FOUND:   "CWLNA5017",
	        INVALID_ENUM:                  "CWLNA5028",
	        VALUE_OUT_OF_RANGE:            "CWLNA5050",
	        UNIT_INVALID:                  "CWLNA5053",
	        NOT_AN_INTEGER:                "CWLNA5054",
	        INVALID_WILDCARD:              "CWLNA5094",
	        VALUE_TOO_LONG_CHARS:          "CWLNA5100",
	        INVALID_SYS_TOPIC:             "CWLNA5108",
	        INVALID_TOPIC_MONITOR:         "CWLNA5109",
	        MUST_BE_EMPTY:                 "CWLNA5128",
	        INVALID_URL_SCHEME:            "CWLNA5139",
	        INCOMPATIBLE_PROTOCOL:         "CWLNA5141",
	        INVALID_VARIABLES:             "CWLNA5143"  
	    }

	};

	return Utils;
});

