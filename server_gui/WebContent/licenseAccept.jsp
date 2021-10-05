<!DOCTYPE HTML>
<!--
# Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

<%@include file="header.jsp"%>

<script type="text/javascript">	
    require([ 'ism/layers/common___buildLabel__' ], function() {
		require([ 'ism/controller/ServerLicenseController', 'dojo/_base/kernel', 'dojo/domReady!' ],
				function(ServerLicenseController, kernel) {
					function getUrlVars() {
						var vars = {};
						var parts = window.location.href.replace(
								/[?&]+([^=&]+)=([^&]*)/gi, function(m, key,
										value) {
									vars[key] = value;
								});
						return vars;
					}
					
					  var currentLocale = kernel.locale;
					  console.log("current locale is " + currentLocale);
	                  var licenseOnly = getUrlVars()["licenseOnly"];
					var controller = new ServerLicenseController({
 						licenseAccept : true,
                        licenseOnly : licenseOnly
					});
				});

	});
</script>
</head>

<body class="oneui ism">
	<div id="template">
		<div id="loading"><span class="loadingMessage"><fmt:message key="LOADING" /> <img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/></span></div>
		<div id="header"></div>		
		<div id="main" role="main" aria-label="<fmt:message key="LICENSE_AGREEMENT_CONTENT" />" class="licensePresent">
			<div id="mainContent"></div>
		</div>
	</div>
</body>
</html>
