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

public class IsmUIStringsResourceBundle_ja extends java.util.ListResourceBundle
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
        {LOADING, "読み込み中..."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {PRODUCT_NAME, "${IMA_PRODUCTNAME_NAME_FULL} Web UI"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED, "サポートされないバージョンの Internet Explorer です。 バージョン 9 以降が必要です。"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED_COMPAT_MODE, "Internet Explorer が互換モードで実行されています。このモードはサポートされません。 「ツール」メニューから<em>「互換表示設定」</em>を選択し、使用しているドメインが Web サイトのリストに含まれていないことを確認してください。 互換表示で Web サイトを表示するためのチェック・ボックスと、Microsoft 互換性リストを使用するためのチェック・ボックスが選択されていないことを確認してください。"}, //$NON-NLS-1$        

        // Reviewed by Dev and ID 
        {FF_UNSUPPORTED, "サポートされないバージョンの Firefox です。 バージョン 10 以降が必要です。"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {LICENSE_AGREEMENT_CONTENT, "ご使用条件の内容"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {ERROR_OCCURRED, "エラーが発生しました"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {NOSCRIPT, "${IMA_PRODUCTNAME_FULL_HTML} を管理するには、JavaScript を使用可能にする必要があります。"} //$NON-NLS-1$
    };
}


