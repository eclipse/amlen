<!DOCTYPE html>
<!--
# Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
   
   Utils.checkWasRequestURL(request,response);  
%>
<c:set var="dojoLocale" value="${actualLocaleString}" scope="session" />
<fmt:setLocale value="${actualLocaleString}" />
<fmt:setBundle basename="com.ibm.ima.msgcatalog.IsmUIStringsResourceBundle" />
<c:set var="ieUnsupp"><fmt:message key="IE_UNSUPPORTED" /></c:set>
<c:set var="ieUnsuppCM"><fmt:message key="IE_UNSUPPORTED_COMPAT_MODE" /></c:set>
<c:set var="ffUnsupp"><fmt:message key="FF_UNSUPPORTED" /></c:set>

<html lang="${dojoLocale}">
<head>

<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title><fmt:message key="PRODUCT_NAME" /></title>

<script type="text/javascript">
    var ieAgent = "MSIE ";
    var ieIndex = navigator.userAgent.indexOf(ieAgent);
    var ffAgent = "Firefox/";
    var ffIndex = navigator.userAgent.indexOf(ffAgent);
    if (ieIndex > 0) {    
        var vers = parseFloat(navigator.userAgent.substring(ieIndex + ieAgent.length));
        if (vers < 9) {
            var message = "<div style='width: 100%; text-align: center; margin-top: 50px;'><h1>${ieUnsupp}</h1></div>";
            // check to see if we are in the proper browser version but it's running in compatibility mode
            var tridentVersionIndex = navigator.userAgent.indexOf("Trident/");
            if (tridentVersionIndex > -1) {
                vers = parseFloat(navigator.userAgent.substring(tridentVersionIndex + "Trident/".length));
                if (vers >= 5) {
                    // we're running on IE 9 or greater but in compatibility mode < 9
                    message = "<div style='width: 95%; text-align: center; margin: 10px 20px; font-size: 1.2em;'>${ieUnsuppCM}</div>";                    
                } 
            } 
            document.write(message);
        }
    } else if (ffIndex > 0) {
        var vers = parseFloat(navigator.userAgent.substring(ffIndex + ffAgent.length));
        if (vers < 10) {
            document.write("<div style='width: 100%; text-align: center; margin-top: 50px;'><h1>${ffUnsupp}</h1></div>");
        }
    }
</script>
<link rel="stylesheet" href="js/dojotoolkit/dojo/resources/dojo.css" />
<link rel="stylesheet" href="js/dojotoolkit/dijit/themes/dijit.css" />
<link rel="stylesheet" href="js/idx/themes/oneui/oneui.css" />
<link rel="stylesheet" href="css/ISM.css" />

<script type="text/javascript" src="js/dojoConfig.js"></script>
<script type="text/javascript" src="js/dojotoolkit/dojo/dojo.js" data-dojo-config="locale: '${dojoLocale}'"></script>
<script type="text/javascript">
	require([ 'ism/layers/common___buildLabel__' ], function() {

		require([ 'ism/widgets/LoginForm', 'dojo/domReady!' ], function(
				LoginForm, domConstruct) {
			var login = new LoginForm({nodeName: "${nodeName}"}, "loginFormContainer");
			login.startup();
		});

	});
</script>
</head>
<body class="loginBody oneui">
    <noscript>
        <h1 style="padding:50px;"><fmt:message key="NOSCRIPT" /></h1>
    </noscript>
	<div id="loginFormContainer"></div>
</body>
</html>
