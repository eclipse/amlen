<!--
# Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#
-->

<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8" %>	
<%@page import="javax.servlet.*, javax.servlet.http.*, com.ibm.ima.util.Utils"%>
<%@ taglib prefix="c" uri="http://java.sun.com/jsp/jstl/core" %>
<%@ taglib prefix="fmt" uri="http://java.sun.com/jsp/jstl/fmt" %>
<c:set var="imaLocaleCookie" value="${cookie['ima_locale']}"/>
<%
   // don't cache this page
   response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
   response.setHeader("Pragma", "no-cache"); 
   response.setDateHeader("Expires", 0);
   
   // check for locale cookie 
   String actualLocaleString = null;
   Cookie imaLocaleCookie =  (Cookie) pageContext.getAttribute("imaLocaleCookie");
   if (imaLocaleCookie != null) {
      actualLocaleString = imaLocaleCookie.getValue();
      actualLocaleString = actualLocaleString.replace('_', '-');
   }
 
   // check if we found the locale preference in a cookie
   if (actualLocaleString == null) {
      // no cookie - use the locale from the request....
      actualLocaleString = Utils.getLocaleString(request);
   }
   
   pageContext.setAttribute("actualLocaleString", actualLocaleString);      
%>
<c:set var="dojoLocale" value="${actualLocaleString}" scope="session" /> 
<fmt:setLocale value="${actualLocaleString}" />
<fmt:setBundle basename="com.ibm.ima.msgcatalog.IsmUIStringsResourceBundle" />

<html lang="${dojoLocale}">
<head>

<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title><fmt:message key="PRODUCT_NAME" /></title>

<link rel="stylesheet" href="js/dojotoolkit/dojo/resources/dojo.css" />
<link rel="stylesheet" href="js/dojotoolkit/dijit/themes/dijit.css" />
<link rel="stylesheet" href="js/dojotoolkit/dojox/layout/resources/GridContainer.css" />
<link rel="stylesheet" href="js/dojotoolkit/dojox/charting/resources/Legend.css" />
<link rel="stylesheet" href="js/idx/themes/oneui/oneui.css" />
<link rel="stylesheet" href="js/dojotoolkit/gridx/resources/Gridx.css" />
<link rel="stylesheet" href="js/idx/themes/oneui/idx/gridx/Gridx.css" />
<link rel="stylesheet" href="css/ISM.css" />
<link rel="stylesheet" href="css/d3.css" />

<script type="text/javascript" src="js/dojoConfig.js"></script>
<script type="text/javascript" src="js/dojotoolkit/dojo/dojo.js" data-dojo-config="locale: '${dojoLocale}'"></script>