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

public class IsmUIStringsResourceBundle_fr extends java.util.ListResourceBundle
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
        {LOADING, "Chargement..."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {PRODUCT_NAME, "Interface utilisateur Web d''${IMA_PRODUCTNAME_NAME_FULL}"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED, "Version d''Internet Explorer non prise en charge. La version 9 ou ultérieure est requise."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED_COMPAT_MODE, "Internet Explorer s''exécute dans un mode compatibilité non pris en charge. Sélectionnez <em>Compatibility View Settings</em> dans le menu Outils pour vous assurer que votre domaine n''est pas dans la liste des sites Web. Vérifiez que les cases pour l''affichage des sites Web dans la vue de compatibilité et pour l''utilisation de listes de compatibilité Microsoft ne sont pas sélectionnées."}, //$NON-NLS-1$        

        // Reviewed by Dev and ID 
        {FF_UNSUPPORTED, "Version de Firefox non prise en charge. La version 10 ou ultérieure est requise."},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {LICENSE_AGREEMENT_CONTENT, "Contenu du contrat de licence"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {ERROR_OCCURRED, "Une erreur s''est produite."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {NOSCRIPT, "JavaScript doit être activé pour gérer ${IMA_PRODUCTNAME_FULL_HTML}."} //$NON-NLS-1$
    };
}


