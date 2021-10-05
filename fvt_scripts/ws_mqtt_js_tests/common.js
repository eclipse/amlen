/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
/**
 * Make a request to the DriverSync TestClient
 * @param logfile - the name of the logfile
 * @param args - "request condition [value] [timeout]"
 * @param onComplete - the function to call if the sync request completes 
 * request: 1=INIT, 2=SET, 3=WAIT, 4=RESET, 5=DELETE, 6=GET
 * condition: The variable name
 * value: If this is a set or wait command, the value to set or wait for
 * timeout: The timeout for a wait, in milliseconds.
 * The request will be validated by the DriverSync TestClient. Errors will be logged in sync.log.
 * 
 */
function requestSync(logfile, args, onComplete) {
    var url = php_sync_script;
    var xmlhttp = getXMLHttp();
    var result = -1;
    
    xmlhttp.open("POST", url, true);
    var port = escape(syncport);
    var address = escape(syncaddress);
    var request = escape(args);
    var params = "PORT=" + port + "&ADDRESS=" + address + "&REQUEST=" + request;
    
    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlhttp.setRequestHeader("Content-length", params.length);
    xmlhttp.setRequestHeader("Connection", "close");

    xmlhttp.onreadystatechange = function() {
        //Call a function when the state changes.	
        if (xmlhttp.readyState == 4) {
          if (xmlhttp.status == 200) {
            result = xmlhttp.responseText;
            var date = new Date();
            var h=date.getHours();
            if (h < 10) {
              h='0' + h;
            }
            var m=date.getMinutes();
            if (m < 10) {
              m='0' + m;
            }
            var s=date.getSeconds();
            if (s < 10) {
              s='0' + s;
            }
            writeLog(logfile,'INFO: ' + h + ':' + m + ':' + s + ': Successfully sent DriverSync request \"' + args + '\" with the result: ' + result);
            if (onComplete) {
              onComplete(result);
            }
          }
          else {
            writeLog(logfile,'ERROR: An error occurred on the synchronization request.  The status is: ' + xmlhttp.status);
          }
        }
    }
    
    xmlhttp.send(params);   
    return result;
}

/**
 * Parse the window.location.href, return the value for the specified parameter.
 * For example, http://test.austin.ibm.com/index.html?PARM1=abc&PARM2=12345
 *   getURLParameter('PARM1') will return 'abc'
 *   getURLParameter('PARM2') will return '12345'
 * If parameter is not found in the URL, return null.
 */
function getURLParameter( param )
{
    param = param.replace(/[\[]/,"\\\[").replace(/[\]]/,"\\\]");
    var regexS = "[\\?&]"+param+"=([^&#]*)";
    var regex = new RegExp( regexS );
    var results = regex.exec( window.location.href );
    if( results == null )
        return null;
    else
        return results[1];
}
