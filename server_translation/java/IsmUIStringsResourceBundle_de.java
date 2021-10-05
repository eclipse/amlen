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

public class IsmUIStringsResourceBundle_de extends java.util.ListResourceBundle
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
        {LOADING, "Ladevorgang läuft..."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {PRODUCT_NAME, "Webbenutzerschnittstelle von ${IMA_PRODUCTNAME_NAME_FULL} "}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED, "Nicht unterstützte Version von Internet Explorer. Es wird Version 9 oder höher vorausgesetzt."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED_COMPAT_MODE, "Internet Explorer wird in einem nicht unterstützten Kompatibilitätsmodus ausgeführt. Wählen Sie im Menü Extras die Option <em>Einstellungen der Kompatibilitätsansicht</em> aus, um sicherzustellen, dass Ihre Domäne nicht in der Liste der Websites enthalten ist. Vergewissern Sie sich, dass die Kontrollkästchen für die Anzeige von Websites in der Kompatibilitätsansicht und die Verwendung von Microsoft-Kompatibilitätslisten nicht ausgewählt sind."}, //$NON-NLS-1$        

        // Reviewed by Dev and ID 
        {FF_UNSUPPORTED, "Nicht unterstützte Version von Firefox. Es wird Version 10 oder höher vorausgesetzt."},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {LICENSE_AGREEMENT_CONTENT, "Inhalt der Lizenzvereinbarung"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {ERROR_OCCURRED, "Es ist ein Fehler aufgetreten."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {NOSCRIPT, "Für die Verwaltung von ${IMA_PRODUCTNAME_FULL_HTML} muss JavaScript aktiviert werden. "} //$NON-NLS-1$
    };
}


