// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
function getXMLHttp()
{
  var xmlHttp;

  try
  {
    //Firefox, Opera 8.0+, Safari
    xmlHttp = new XMLHttpRequest();
  }
  catch(e)
  {
    //Internet Explorer
    try
    {
      xmlHttp = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch(e)
    {
      try
      {
        xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
      }
      catch(e)
      {
        alert("Your browser does not support AJAX!")
        return false;
      }
    }
  }
  return xmlHttp;
}

/**
 * Write a message to the specified log file.
 */
function writeLog(filename, message){
	var url = php_write_log_script;
	xmlhttp = getXMLHttp();
	
	xmlhttp.open("POST", url, true);
	var data = encodeURIComponent(message + "\n");
	params = "DATA=" + data + '&FILENAME=' + filename;
	xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xmlhttp.setRequestHeader("Content-length", params.length);
	xmlhttp.setRequestHeader("Connection", "close");

	xmlhttp.onreadystatechange = function() {
		//Call a function when the state changes.	
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			//Do nothing!!!
		}
	}
	xmlhttp.send(params);
	//Echo the message back to the browser
	document.body.appendChild(document.createElement("br"));
	document.body.appendChild(document.createTextNode(message));
}

/**
 * Read and return the entire log file.
 */
function readLog(filename){
	var url = php_read_log_script;
	xmlhttp = getXMLHttp();
	var params = "FILENAME=" + filename;
	var response = "";
	xmlhttp.open("GET", url + "?" + params, false);
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			response = xmlhttp.responseText;
		} else {
			message = "ERROR: " + xmlhttp.statusText;
			//Echo the message back to the browser
			document.body.appendChild(document.createElement("br"));
			document.body.appendChild(document.createTextNode(message));			
		}
	}
	xmlhttp.send(null);
	return response;
}

/**
 * Write the property-value pair to spefied file.
 */
function writeProperty(filename, property, value) {
	if (property.length > 0 && value.length > 0) {
		writeLog(filename, escape(property + "=" + value));
	}
}

/**
 * Return the value of the property if exists in the specfied file.
 * The timeout value is in milliseconds. Note that the caller should check
 * the return string. A 0 length string means the property has not been set.
 */
function readProperty(filename, property, timeout) {
	var url = php_read_log_script;
	xmlhttp = getXMLHttp();
	params = "PROPERTY=" + property + "&TIMEOUT=" + timeout + '&FILENAME=' + filename;
	var response = "";
	xmlhttp.open("GET", url + "?" + params, false);
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			response = xmlhttp.responseText;
		} else {
			message = "ERROR: " + xmlhttp.statusText;
			//Echo the message back to the browser
			document.body.appendChild(document.createElement("br"));
			document.body.appendChild(document.createTextNode(message));			
		}
	}
	xmlhttp.onerror = function() {
		message = "ON ERROR: " + xmlhttp.statusText;
		//Echo the message back to the browser
		document.body.appendChild(document.createElement("br"));
		document.body.appendChild(document.createTextNode(message));		
	}
	xmlhttp.send(null);
	return response;
}

/**
 * This function tests the writeProperty function.  To use this function,
 * the test html file should include this log.js file and invoke this
 * function upon an action (e.g. button clicked).  This function expects
 * 2 elements (e.g. text input). The value of the first element is the
 * name of the property. The value of the second element is the value
 * of the property.
 */
function testWriteProperty(propElement, valueElement) {
	var prop = propElement.value;
	var value = valueElement.value;
	
	//Use the html filename for the properties filename with ".properties" extension
	var curURL = window.location.pathname;
	var propFile = curURL + ".properties";
	
	var message = "About to write to " + propFile +  " " + prop + "=" + value;
	alert(message);
	writeProperty(propFile, prop, value);
}

/**
 * This function tests the readProperty function.  To use this function,
 * the test html file should include this log.js file and invoke this
 * function upon an action (e.g. button clicked).  This function expects
 * 2 elements (e.g. text input). The value of the first element is the
 * name of the property. If the property exists, the second element will
 * updated with the returned value of the property.  The default timeout
 * is 15 seconds.  If the property does not exists, an alert message
 * will be displayed on the browser.
 */
function testReadProperty(propElement, valueElement) {
	var prop = propElement.value;
	valueElement.value = "";
	
	//Use the html filename for the properties filename with ".properties" extension
	var curURL = window.location.pathname;
	var propFile = curURL + ".properties";
	
	var message = "Will try to get " + prop + " from " + propFile;
	alert(message);
	var value = readProperty(propFile, prop, 15000);
	valueElement.value = value;
	if (value.length === 0) {
		message =  prop + " has not been set";
		alert(message);
	} else {
		message = prop + "=" + value;
	}
	//Echo the message back to the browser
	document.body.appendChild(document.createElement("br"));
	document.body.appendChild(document.createTextNode(message));
}
