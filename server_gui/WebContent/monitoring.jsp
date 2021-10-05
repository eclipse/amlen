<!DOCTYPE HTML>
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

<%@include file="header.jsp"%>

<script type="text/javascript">
    require([ 'ism/layers/common___buildLabel__' ], function() {

        require([ 'ism/controller/MonitoringController', 'dojo/domReady!' ],
                function(MonitoringController) {
                    function getUrlVars() {
                        var vars = {};
                        var parts = window.location.href.replace(
                                /[?&]+([^=&]+)=([^&]*)/gi, function(m, key,
                                        value) {
                                    vars[key] = value;
                                });
                        return vars;
                    }
                    var nav = getUrlVars()["nav"];
                    var controller = new MonitoringController({
                        nav : nav,
                        nodeName: "${nodeName}"                        
                    });
                });

    });
</script>
</head>

<%@include file="footer.jsp"%>
