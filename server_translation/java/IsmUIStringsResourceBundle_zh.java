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

public class IsmUIStringsResourceBundle_zh extends java.util.ListResourceBundle
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
        {LOADING, "正在加载..."}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {PRODUCT_NAME, "${IMA_PRODUCTNAME_NAME_FULL} Web UI"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED, "不受支持的 Internet Explorer 版本。需要 V9 或更高版本。"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {IE_UNSUPPORTED_COMPAT_MODE, "Internet Explorer 正以不受支持的兼容性方式运行。从“工具”菜单中选择<em>兼容性视图设置</em>以确保您的域不在 Web 站点列表中。确保未选中“在兼容性视图中显示 Web 站点”和“使用 Microsoft 兼容性列表”复选框。"}, //$NON-NLS-1$        

        // Reviewed by Dev and ID 
        {FF_UNSUPPORTED, "不受支持的 Firefox 版本。需要 V10 或更高版本。"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {LICENSE_AGREEMENT_CONTENT, "许可协议内容"},  //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {ERROR_OCCURRED, "发生了错误"}, //$NON-NLS-1$

        // Reviewed by Dev and ID 
        {NOSCRIPT, "必须启用 JavaScript 才能管理 ${IMA_PRODUCTNAME_FULL_HTML}。"} //$NON-NLS-1$
    };
}


