/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.msgcatalog;

// NLS_ENCODING = UTF-8
// NLS_MESSAGEFORMAT_ALL

public class IsmUIStringsResourceBundle extends java.util.ListResourceBundle
{
	 // START NON-TRANSLATABLE 
    /** COPYRIGHT */
    public static final String COPYRIGHT= "************************************************************* Copyright (c) 2014-2021 Contributors to the Eclipse Foundation. See the NOTICE file(s) distributed with this work for additional information regarding copyright ownership. This program and the accompanying materials are made available under the terms of the Eclipse  Public License 2.0 which is available at http://www.eclipse.org/legal/epl-2.0 SPDX-License-Identifier: EPL-2.0. *************************************************************"; //$NON-NLS-1$

    /** This static member defines the class name */
    public static final String CLASS_NAME = "com.ibm.ima.msgcatalog.IsmUIStringsResourceBundle"; //$NON-NLS-1$

    /*********************************************************
    * Implements java.util.ListResourceBundle.getContents().
    *
    * @see java.util.ListResourceBundle#getContents()
    * @return Object[][]
    **********************************************************/
    public Object[][] getContents()
    {
        return CONTENTS;
    }


    /*********************************************************
    * Keys for translatable text
    **********************************************************/

    /** This member defines a static reference to a key */ 
    public static final String LOADING = "LOADING"; //$NON-NLS-1$
    public static final String PRODUCT_NAME = "PRODUCT_NAME"; //$NON-NLS-1$
    public static final String IE_UNSUPPORTED = "IE_UNSUPPORTED"; //$NON-NLS-1$
    public static final String IE_UNSUPPORTED_COMPAT_MODE = "IE_UNSUPPORTED_COMPAT_MODE"; //$NON-NLS-1$    
    public static final String FF_UNSUPPORTED = "FF_UNSUPPORTED"; //$NON-NLS-1$
    public static final String LICENSE_AGREEMENT_CONTENT = "LICENSE_AGREEMENT_CONTENT"; //$NON-NLS-1$
    public static final String ERROR_OCCURRED ="ERROR_OCCURRED"; //$NON-NLS-1$
    public static final String NOSCRIPT = "NOSCRIPT"; //$NON-NLS-1$

 // END NON-TRANSLATABLE
    /*********************************************************
    * Resource table
    **********************************************************/
    private final static Object[][] CONTENTS=
    {
        // Reviewed by Dev and ID 
        {LOADING, "Loading..."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {PRODUCT_NAME, "${IMA_PRODUCTNAME_FULL} WebUI"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED, "Unsupported version of Internet Explorer. Version 9 or later is required."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED_COMPAT_MODE, "Internet Explorer is running in an unsupported compatibility mode. " + //$NON-NLS-1$ 
            "Select <em>Compatibility View Settings</em> from the Tools menu to ensure that your domain is not in the list of websites. " + //$NON-NLS-1$ 
            "Ensure that the check boxes for displaying websites in Compatibility View, and using Microsoft compatibility lists, are not selected."}, //$NON-NLS-1$        

        // Reviewed by Dev and ID 
        {FF_UNSUPPORTED, "Unsupported version of Firefox. Version 10 or later is required."},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {LICENSE_AGREEMENT_CONTENT, "License Agreement Content"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {ERROR_OCCURRED, "An error occured"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {NOSCRIPT, "JavaScript must be enabled to administer ${IMA_PRODUCTNAME_FULL_HTML}."} //$NON-NLS-1$
    };
}


