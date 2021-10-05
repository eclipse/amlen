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
package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * REST representation for setting of the appliance
 * locale.
 *
 */
public class LocaleData extends  AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	@SuppressWarnings("unused")
	private final static String CLAS = LocaleData.class.getCanonicalName();


	@JsonProperty("language")
	private String language;
	
	@JsonProperty("country")
	private String country;
	
	@JsonProperty("locale")
	private String locale;
	
	@JsonProperty("displayName")
	private String displayName;


	/**
	 * Get the language code for the locale
	 * 
	 * @return A String that represents the language
	 */
	@JsonProperty("language")
	public String getLanguage() {
		return language;
	}

	/**
	 * Set the language code for the locale
	 * 
	 * @param language  The language code to set
	 */
	@JsonProperty("language")
	public void setLanguage(String language) {
		this.language = language;
	}

	/**
	 * Get the country code for the locale
	 * 
	 * @return A String that represents the country
	 */
	@JsonProperty("country")
	public String getCountry() {
		return country;
	}

	/**
	 * Set the country code for the locale
	 * 
	 * @param language  The country code to set
	 */
	@JsonProperty("country")
	public void setCountry(String country) {
		this.country = country;
	}

	/**
	 * Get a String that represents the entire locale
	 * 
	 * @return A String that represents the entire locale
	 */
	@JsonProperty("locale")
	public String getLocale() {
		return locale;
	}

	/**
	 * Set the locale using a String specified
	 * 
	 * @param locale  The String to set the locale to
	 */
	@JsonProperty("locale")
	public void setLocale(String locale) {
		this.locale = locale;
	}
	
	/**
	 * Get a human readable String that describes the locale
	 * 
	 * @return  A String that describes the locale
	 */
	@JsonProperty("displayName")
	public String getDisplayName() {
		return displayName;
	}

	/**
	 * Specify a human readable String that describes the locale
	 * 
	 * @param displayName  A human readable description of the locale
	 */
	@JsonProperty("displayName")
	public void setDisplayName(String displayName) {
		this.displayName = displayName;
	}

	@Override
	public String toString() {
		return "LocaleData [language=" + language + " country=" + country + " locale=" + locale + " displayName=" + displayName + "]";
	}
	
	@Override
	public ValidationResult validate() {
	
		ValidationResult vr = new ValidationResult();
		
		// if we have a value for locale validation is good...
		if (locale == null || locale.length() == 0) {
			// make sure we at least have a language if locale is null or empty
			if (language == null || language.length() == 0) {
				vr.setResult(VALIDATION_RESULT.VALUE_EMPTY);
				vr.setParams(new Object[] {"Locale Data"});
			}
		}
		
		return vr;
		
	}
	
}
