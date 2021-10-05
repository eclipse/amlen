// Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
var pubClientID_stem="ISM_PUB_";
//var pubClient="";
var subClientID_stem="ISM_SUB_";
//var subClient="";
var msgRecv=0;

// From the User Input
var ismServer="9.3.179.22";
var ismPort=16102;
var subQoS=0;
var pubQoS=0;
var retained="";
//var timerId;


//-----------------------------------------------
// Functions to respond to user inputs and manage 
// displayed elements.
//----------------------------------------------

function showDiv( divId) {
    document.getElementById(divId).style.display='block';
}

function hideDiv( divId) {
    document.getElementById(divId).style.display='none';
}

function disableElement ( elementName) {
    document.getElementById(elementName).disabled=true;
}
function enableElement ( elementName) {
    document.getElementById(elementName).disabled=false;
}

function do_ldap_validate(inputName,  inputPassword) {
	// ldap authentication goes here
	return true;	
}

var automotiveMainElements =  ["Accounts","Reservations","Unlock"];
function deAllMainElements(myFunc) {
    for (var i = 0; i < automotiveMainElements.length; i++) {
        myFunc(automotiveMainElements[i]);
    }
}
function disableAllMainElements(myFunc) {
    deAllMainElements(disableElement);
}

function enableAllMainElements() {
    deAllMainElements(enableElement);
}

var automotiveDiv =  ["automotiveMain","automotiveLogin"];
function hideAllDiv() {
    for (var i = 0; i < automotiveDiv.length; i++) {
        hideDiv(automotiveDiv[i]);
    }
}

function doButtonAccounts(){
    msgText="ACCOUNT_INFO|" + document.getElementById('ismUID').value;
    //pubReplyTopic="/USER/1"
    pubTopicName="/APP/1/USER/1/USER/1"
    subTopicName="/USER/1/#"
    connectISMSubscriber(msgText,pubTopicName,subTopicName) ;
}

function doButtonUnlock(){
    disableAllMainElements() ;
    msgText="UNLOCK|" + document.getElementById('ismUID').value;
    //pubReplyTopic="/CAR/1"
    pubTopicName="/APP/1/CAR/1/USER/1"
    subTopicName="/USER/1/#"
    connectISMSubscriber(msgText,pubTopicName,subTopicName);

    // This doesn't work , get error _localKey not defined from mqttws31.js
    //writeMessage("Start another one ########################################### ");
    //pubTopicName="/APP/1/USER/2/CAR/2"
    //subTopicName="/USER/2"
    //connectISMSubscriber(msgText,pubTopicName,subTopicName);
}

function doButtonReservations(){
    msgText="RESERVATION_LIST|" + document.getElementById('ismUID').value;
    //pubReplyTopic="/USER/1"
    pubTopicName="/APP/1/USER/1/USER/1"
    subTopicName="/USER/1/#"
    connectISMSubscriber(msgText,pubTopicName,subTopicName);
}

function doButtonLogin(){
    var userName_value = document.getElementById('ismUID').value;
    var password_value = document.getElementById('ismPassword').value;
	if (do_ldap_validate(userName_value,password_value)) {
        writeMessage ("Successfully Logged in.");
        hideAllDiv();
        showDiv('automotiveMain');
	} else {
		alert("Login failed. Please try again.");
	}
}

//-------------------------------------------------------------------------------------------------
// connectISMSubscriber - Initiated by pressing the Subscribe Button on the Subscriber HTML.
//    This function starts by obtaining the IsmMqtt.Client object parameters.  The clientID must be
//    unique and not exceed 23 characters.  The IsmMqtt.Client object stores the clientID and the 
//    connection information to the ISM Server.
//  The connect() call back functions are defined prior to making the connect() call.
//  The subClient.onconnect() call back is called after a successful connection to ISM Server.
//    This function will perform the subscribe() and the client then waits for message to arrive
//    in the subClient.onmessage() call back.
//  The subClient.onconnectionlost() function may be called on a failure during the connect() call
//    or a subsequent dropped connection after a successful connection.  
//  The subClient.connect() will use the information in the IsmMqtt.Client object to connect to the  
//    ISM Server.  When CleanSession=true in the ConnectOptions, the clientID's state is not  
//    persisted should the connection be lost.  
//    If the sample was changed to have CleanSession=false the connection state would be preserved 
//    and could be reconnected.  At some point the saved state is no longer needed.  At that time
//    the application would need to do unsubscribe() on each topic to remove the 
//    persisted session's state.  
//    If a persisted client state is desired, a random clientID is not recommended.
//    The connect() call will return no errors, communication is only through the call back
//    functions onconnect() or onconnectionlost().
//-------------------------------------------------------------------------------------------------
function connectISMSubscriber(msgText,pubTopicName,subTopicName){
    writeMessage("In connectISMSubscriber");
    try{

    //document.getElementById('automotiveMainContent').innerHTML='';

    subClientID = subClientID_stem + Math.floor((Math.random()*999)+1);

    writeMessage("Start a new subscriber for subClientID: " + subClientID);
    
    //timerId=setTimeout('disconnectISMSubscriber(subTopicName,"ERROR: 60 second timeout expired.<br>")', 60000);

    disableAllMainElements() ;
    subClient = new Messaging.Client( ismServer, ismPort, subClientID );
    writeMessage("connectISMSubscriber: created IsmMqtt Subscriber Client "+subClientID);

    subClient.onSuccess = function() {
        writeMessage ("subClient.onSuccess invoked ::::::::::::::::::::::::::::::::");
    }
    
    subClient.onFailure = function() {
        writeMessage ("subscribeFailureCallback invoked:::::::::::::::::::::::::::::");
        disconnectISMSubscriber(subTopicName);
    }
    

    // Call Back Functions
    subClient.onConnect = function() {
        if ( subClient.isConnected ) {
            var subscribeOpts = new Object();
            subscribeOpts.qos = subQoS;
            //subscribeOpts.onSuccess = subscribeCompleteCallback;
            //subscribeOpts.onFailure = subscribeFailureCallback
            subscribeOpts.onSuccess = function() {
                writeMessage ("subscribeCompleteCallback invoked. starting publisher");
                connectISMPublisher(msgText,pubTopicName) ;
            }

            subscribeOpts.onFailure = function() {
                writeMessage ("subscribeFailureCallback invoked:::::::::::::::::::::::::::::");
                disconnectISMSubscriber(subTopicName);
            }

            subClient.subscribe( subTopicName, subscribeOpts );
            var statusMsg = "Subscribed to topic: "+subTopicName+".\n" ;
            writeMessage ( statusMsg );
        } else {
            writeMessage ( "Error: in subClient.onConnect but subClient.isConnected is false for subClientID: " + subClientID);
            disconnectISMSubscriber(subTopicName) ;
        }
    }

    subClient.onMessageArrived = function(message) {
        //clearTimeout(timerId);
        msgRecv++;
        try {
            var statusMsg = "Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadString+"\n" ;
        } catch (e) {
            var statusMsg = "Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadBytes+"\n" ;
        }
        console.log( "In startISMSubscriber.onMessageArrived, " + statusMsg);
        writeMessage ( statusMsg );
        statusMsg = message.payloadString.replace(/\|/g, "<br>");
        console.log( "New status message is:" );
        writeMessage ( statusMsg );
        //document.getElementById('automotiveMainContent').innerHTML=statusMsg;
        document.getElementById('automotiveMainContent').innerHTML+=statusMsg+"<br>";
        writeMessage( "disconnect subscriber " + statusMsg);
        enableAllMainElements() ;
        disconnectISMSubscriber(subTopicName) ;
    }

    subClient.onConnectionLost = function(errReason) { 
        //clearTimeout(timerId);
        writeMessage( "In subClient.onconnectionlost reason: " + errReason + " subClientID: " + subClientID ); 
        var statusMsg = "Subscriber connection lost for client "+subClientID+".\n";
        writeMessage ( statusMsg );
        //document.getElementById('automotiveMainContent').innerHTML+="Error: Connection lost.<br>"
        msgRecv = 0;
        subClientID = "";
        //disconnectISMSubscriber(subTopicName) ;
        enableAllMainElements() ;
        
    }
    // End of Call Back Functions

    var cleanSession_value = true;
    var keepAliveInterval_value = 0;
    var userName_value = "admin";
    var password_value = "admin";
    writeMessage ( "user is: " + userName_value + " pass is: " + password_value);

    // Build the Will Message to be sent by the Server when this client abnormally disconnects
    var willTopic_value = "willTopic";
    var willMessage_value = "R.I.P. client, "+ subClientID +" terminated abnormally.";
    var willMessage = new Messaging.Message(willMessage_value);
    willMessage.destinationName = willTopic_value;
    willMessage.qos = 1;
    willMessage.retained = false;

 
    // Build the Connection Options Object
    var connectOptions = new Object();

    // Per MQTT Javascript Messaging API these are the defaults except where noted
    connectOptions.timeout = parseInt( '30' );
    connectOptions.cleanSession = true;
    connectOptions.userName = userName_value;
    connectOptions.password = password_value;
    connectOptions.keepAliveInterval = parseInt( '60' );
    connectOptions.willMessage = willMessage;  // Default is no willMessage
    //JCD unknown property:    connectOptions.useSSL = false;             //?
 

    subClient.connect( connectOptions );

  } catch (e) {
      var statusMsg = "Subscriber encountered an unexpected error. Reason: "+e.message+"\n";
      writeMessage ( statusMsg );
      disconnectISMSubscriber(subTopicName) ;
  }

  writeMessage("Subscribe call returned +++++++++++++++++++++++" );

}

//-------------------------------------------------------------------------------------------------
// disconnectISMSubscriber - Initiated by pressing the Disconnect Button on the Subscriber HTML.
//    This function enables the Subscriber HTML fields, unsubscribes the topic and disconnects
//    the MQTT Client.
//-------------------------------------------------------------------------------------------------
function disconnectISMSubscriber(subTopicName, errMsg) {
    //clearTimeout(timerId);
    if (arguments.length == 2) {
        document.getElementById('automotiveMainContent').innerHTML+=errMsg;
    }
    writeMessage( "In disconnectISMSubscriber: subClient.disconnect() " + subClientID + " " + ismServer + ":" + ismPort );
    var statusMsg = "Subscriber disconnected.\n";
    writeMessage ( statusMsg );
    msgRecv = 0;
    subClientID = "";

    enableAllMainElements() ;
    subClient.unsubscribe( subTopicName);
    subClient.disconnect();
}


//-------------------------------------------------------------------------------------------------
// connectISMPublisher - Initiate by pressing the Publish Button on the Publisher HTML.
//    This function starts by obtaining the IsmMqtt.Client object parameters.  The clientID must be
//    unique and not exceed 23 characters.  The IsmMqtt.Client object stores the clientID and the 
//    connection information to the ISM Server.
//  The connect() call back functions are defined prior to making the connect() call.
//  The pubClient.onconnect() call back is called after a successful connection to ISM Server.
//    This function will perform the publish() for how many messages were requested.
//  The pubClient.onconnectionlost() function may be called on a failure during the connect() call
//    or a subsequent dropped connection after a successful connection.  
//  The pubClient.connect() will use the IsmMqtt.Client object to connect to the ISM Server.  
//    This call will return no errors, communication is through the call back functions.
//-------------------------------------------------------------------------------------------------
function connectISMPublisher(msgText,pubTopicName) {
 writeMessage("In connectISMPublisher");
 try{

    pubClientID = pubClientID_stem + Math.floor((Math.random()*100)+1);
    pubClient = new Messaging.Client( ismServer, ismPort, pubClientID );
    writeMessage("connectISMPublisher: created IsmMqtt Publisher Client "+pubClientID);

    var statusMsg = "Publishing to Topic " + pubTopicName + ".\n";
    writeMessage ( statusMsg );

    //Call back functions
    pubClient.onConnect = function() {
        console.debug( "In pubClient.onConnect " );
        if ( pubClient.isConnected ) {
            //msgText=pubReplyTopic+"|"+msgText;
            var message = new Messaging.Message( msgText);
            var pubOpts = new Object();
            message.destinationName = pubTopicName;
            message.qos = pubQoS;
            if ( pubQoS !== 0 ) {
                pubOpts.onMessageDelivered = onMessageDelivered;
            }
            message.retained = false;
            writeMessage ( "publishing message: " + msgText + " to topic: " + pubTopicName);
            pubClient.send(message);

        } else {
            writeMessage("Error pubClient is not connected");
        }
        disconnectISMPublisher();
    }

    pubClient.onMessageArrived = function(message) {
        var statusMsg = "PUBLISHER RECEIVED: '"+ message.payload +"' on topic: "+ message.topic +".\n";
        writeMessage( "In pubClient.onmessage, " + statusMsg );
        writeMessage ( statusMsg );
    }

    pubClient.onConnectionLost = function() { 
        writeMessage( "In startISMPublisher.onconnectionlost " + pubClientID ); 
        var statusMsg = "Publisher connection lost for client "+pubClientID+".\n";
        writeMessage ( statusMsg );
    }
    // End of Call back functions

    var cleanSession_value = true;
    var keepAliveInterval_value = 0;
    var userName_value = "admin";
    var password_value = "admin";
    writeMessage ( "user is: " + userName_value + " pass is: " + password_value);

    // Build the Will Message to be sent by the Server when this client abnormally disconnects
    var willTopic_value = "willTopic";
    var willMessage_value = "R.I.P. client, "+ pubClientID +" terminated abnormally.";
    var willMessage = new Messaging.Message(willMessage_value);
    willMessage.destinationName = willTopic_value;
    willMessage.qos = 1;
    willMessage.retained = false;

   // Build the Connection Options Object
    var connectOptions = new Object();

    // Per MQTT Javascript Messaging API these are the defaults except were noted
    connectOptions.timeout = parseInt( '30' );
    connectOptions.cleanSession = true;
    connectOptions.userName = userName_value;
    connectOptions.password = password_value;
    connectOptions.keepAliveInterval = parseInt( '60' );
    connectOptions.willMessage = willMessage;  // Default is no willMessage
//JCD unknown property:    connectOptions.useSSL = false;             //?
 
    pubClient.connect( connectOptions );
  } catch (e) {
      var statusMsg = "Publisher encountered an unexpected error. Reason: "+e.message+"\n";
      writeMessage ( statusMsg );
      disconnectISMPublisher() ;
  }
}

//------------------------------------------------------------------------------
// disconnectISMPublisher - called at the conclusion of a Publish Action to 
//    disconnect the MQTT Client.
//------------------------------------------------------------------------------
function disconnectISMPublisher() {
    writeMessage( "In disconnectISMPublisher: pubClient.disconnect() " + 
        pubClientID + " " + ismServer + ":" + ismPort );
    pubClient.disconnect();
    var statusMsg = "Publish complete. Disconnected from ISM Server.\n";
    writeMessage ( statusMsg );
}


//------------------------------------------------------------------------------
//  writeMessage - 
//  Inputs:
//  statusMsg - String  Message text to write in textarea. 
//------------------------------------------------------------------------------
function writeMessage( statusMsg ) {
    var debug=1;
    if (debug) {
        console.log(statusMsg);
    }
}


