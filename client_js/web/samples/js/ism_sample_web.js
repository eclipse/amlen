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
var pubClientID="";
var pubClient="";
var subClientID_stem="ISM_SUB_";
var subClientID="";
var subClient="";
var msgRecv=0;
var pubAckReceived=0;
var pStartTime=0;

// From the User Input
var ismServer="";
var ismPort="";
var topicName="";
var msgText="";
var subQoS="";
var pubQoS="";

//-------------------------------------------------------------------------------------------------
// Functions to minimally validate numeric input for ismPort and msgCount input parameters
//-------------------------------------------------------------------------------------------------

function setWSS() {
    var theIP = document.getElementById('ismIP').value;
    var thePort = document.getElementById('ismPort').value;
    if ( document.getElementById('wss').checked ) {
        document.getElementById('fullURL').value = "wss://"+theIP+":"+thePort+"/mqtt";
    } else {
        document.getElementById('fullURL').value = "ws://"+theIP+":"+thePort+"/mqtt";
    }

}

function isNumber(inputStr) {
    for (var i = 0; i < inputStr.length; i++) {
        var oneChar = inputStr.substring(i, i + 1);
        if (oneChar < "0" || oneChar > "9") {
            alert("Please make sure values are numbers only.  '" + oneChar + "' is not a valid character for a positive integer.");
            return false;
        }
    }
    return true;
}

function countIsNumeric( msgCount ) {
    if ( isNumber( msgCount.value ) ) {
    // TRUE, this is good
    } else {
        msgCount.focus();
        msgCount.select();
        console.error( "Message Count must be a numeric value.  " + msgCount.value + "' is not a valid character for a positive integer.");
    }
}

function portIsNumeric( ismPort ) {
    if ( isNumber( ismPort.value ) ) {
    // TRUE, this is good
    } else {
        ismPort.focus();
        ismPort.select();
        console.error( "ISM Port must be a numeric value.  '" + ismPort.value + "' is not a valid character for a positive integer.");
    }
}

function sleep(milliseconds) {
    var mStartTime = new Date().getTime();
    while ( (new Date().getTime() - mStartTime) < milliseconds ) {
    // Do nothing, just loop and check elapsed time again
    }
}

//-------------------------------------------------------------------------------------------------
// connectISMSubscriber - Initiated by pressing the Subscribe Button on the Subscriber HTML.
//    This function starts by obtaining the Messaging.Client object parameters.  The clientID must be
//    unique and not exceed 23 characters.  The Messaging.Client object stores the clientID and the 
//    connection information to the ISM Server.
//  The connect() call back functions are defined prior to making the connect() call.
//  The subClient.onconnect() call back is called after a successful connection to ISM Server.
//    This function will perform the subscribe() and the client then waits for message to arrive
//    in the subClient.onmessage() call back.
//  The subClient.onconnectionlost() function may be called on a failure during the connect() call
//    or a subsequent dropped connection after a successful connection.  
//  The subClient.connect() will use the information in the Messaging.Client object to connect to the  
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
function connectISMSubscriber() {
  console.debug("In connectISMSubscriber");
  try {
    if ( document.getElementById('ismClientId').value != "" ) {
        subClientID = document.getElementById('ismClientId').value;
    } else {
        subClientID = subClientID_stem + Math.floor((Math.random()*999)+1);
    }

    ismServer=document.getElementById('ismIP').value;
    if ( document.getElementById('ismPort').value != "" ) {
        ismPort=parseInt( document.getElementById('ismPort').value );
    } else {
        ismPort=parseInt('1883');
    }
    
    document.getElementById('subConnect').disabled = true;
    document.getElementById('subDisconnect').disabled = false;
    subClient = new Messaging.Client( ismServer, ismPort, subClientID );
    console.debug("connectISMSubscriber: created ISM MQTT Subscriber Client "+subClientID);

    //Register Callback Functions for connect()
    subClient.onMessageArrived = onSubClientMessageArrived;
    subClient.onConnectionLost = onSubClientConnectionLost;


    // Build the Will Message to be sent by the Server when this client abnormally disconnects
    var willTopic_value = "willTopic";
    var willMessage_value = "R.I.P. client, "+ subClientID +" terminated abnormally.";
    var willMessage = new Messaging.Message(willMessage_value);
    willMessage.destinationName = willTopic_value;
    willMessage.qos = 1;
    willMessage.retained = false;

    // Build the Connection Options Object
    var connectOptions = new Object();
    var context = new Object();
    context = { "clientId":subClientID, "ismServer":ismServer, "ismPort":ismPort };
    connectOptions.invocationContext=context;

    // Per MQTT Javascript Messaging API these are the defaults except were noted
    connectOptions.timeout = parseInt( '30' );
    connectOptions.cleanSession = true;
    connectOptions.keepAliveInterval = parseInt( '60' );   
    connectOptions.willMessage = willMessage;  // Default is no willMessage

    if ( !document.getElementById('ismCleanSession').checked ) {
        //  the client and server persistent state is saved across connection.
        connectOptions.cleanSession = false;
    } else {
        //  the client and server persistent state is deleted on succesful connect.
        connectOptions.cleanSession = true;
    }
    if ( document.getElementById('ismUID').value != "" ) {
        connectOptions.userName = document.getElementById('ismUID').value;
    }
    if ( document.getElementById('ismPassword').value != "" ) {
        connectOptions.password = document.getElementById('ismPassword').value;
    }
    if ( document.getElementById('wss').checked ) {
        connectOptions.useSSL = true;
    } else {
        connectOptions.useSSL = false;
    }

    // Callback on Connect()
    connectOptions.onSuccess = onSubClientConnect;
    connectOptions.onFailure = onSubClientConnectFailure;

    // Connect to the ISM Server - completion status is determined by the callback invoked
    subClient.connect( connectOptions );
   
  } catch (e) {
      subDisableButtons ( false );
      var statusMsg = "Subscriber encountered an unexpected error. Reason: "+e.message+"\n";
      writeMessage ( statusMsg );
      subClient.disconnect();

  }
}


// Subscriber Call Back Functions
//-------------------------------------------------------------------------------------------------
// onSubClientConnect - onSuccess() Callback for subClient.connect()
//    This function is invoked when ...
//------------------------------------------------------------------------
  function onSubClientConnect() {
        console.debug( "In onSubClientConnect " ); 
        try {
            subDisableButtons( true );
            if ( document.getElementById('msgTopic').value != "" ) {
                topicName=document.getElementById('msgTopic').value;
            }

            var subscribeOpts = new Object();
            var context = new Object();
            context = { "clientId":subClientID, "ismServer":ismServer, "ismPort":ismPort };
            subscribeOpts.invocationContext=context;

            subscribeOpts.qos = parseInt( document.getElementById('subQoS').value );
            subscribeOpts.onSuccess = onSubscribeSuccess; 
            subscribeOpts.onFailure = onSubscribeFailure;

            subClient.subscribe( topicName, subscribeOpts );

        } catch ( e ) {
            var statusMsg = "Subscriber client, "+ subClientID +", no longer has an active connection.  Reason: "+e.message+"\n";
            writeMessage ( statusMsg );
            subDisableButtons ( false );
            msgRecv = 0;

            subClient.disconnect();          
        }
    }


//-------------------------------------------------------------------------------------------------
// onSubClientConnectFailure - onFailure() Callback for subClient.connect()
//    This function is invoked when ...
//------------------------------------------------------------------------
function onSubClientConnectFailure( context, errorCode, errorMessage ) {
    console.debug( "In onSubClientConnectFailure " ); 
    subDisableButtons( false );
    var statusMsg = "Subscriber connection failed for client "+ context.clientId +". Reason: "+ errorCode +". Message: "+ errorMessage +"\n";
    writeMessage ( statusMsg );
    msgRecv = 0;
    subClientID = "";
}


//-------------------------------------------------------------------------------------------------
// onSubClientMessageArrived - onMessageArrived() Callback for subClientID
//    This function is invoked when ...
//------------------------------------------------------------------------
function onSubClientMessageArrived(message) {
    msgRecv++;
    try {
        var statusMsg = "Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadString+"\n" ;
    } catch (e) {
        var statusMsg = "Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadBytes+"\n" ;
    }
    console.log( "In startISMSubscriber.onmessage, " + statusMsg);
    writeMessage ( statusMsg );
}


//-------------------------------------------------------------------------------------------------
// onSubClientConnectionLost - onConnectionLost() Callback for subClientID
//    This function is invoked if the connection with the ISM server has been lost.  
//    The connection can be broken when client initiates a disconnect or server/network failure.
//-------------------------------------------------------------------------------------------------
function onSubClientConnectionLost( reasonCode, reasonMessage ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        statusMsg = "Subscriber connection closed for client "+ subClientID +"\n";
    } else {
        statusMsg = "Subscriber connection lost for client "+ subClientID +". Reason: "+ reasonCode +". Message: "+ reasonMessage +"\n";
    }
    console.debug( "In onSubClientConnectionLost: " + statusMsg); 
    subDisableButtons ( false );
    writeMessage ( statusMsg );
    msgRecv = 0;
    subClientID = "";
}

//-------------------------------------------------------------------------------------------------
// onSubscribeSuccess - onSuccess() Callback for subClientID's subscribe(..)
//    This function is invoked on unsuccessful completion of subscribe(..)
//-------------------------------------------------------------------------------------------------
function onSubscribeSuccess( context ) { 
    console.debug( "In subClient.onSubscribeSuccess " + subClientID +" Context: "+ JSON.stringify(context) ); 
    var statusMsg = "Subscribed to topic: "+topicName+" with QoS="+ document.getElementById('subQoS').value +".\n" ;
    writeMessage ( statusMsg );
}

//-------------------------------------------------------------------------------------------------
// onSubscribeFailure - onFailure() Callback for subClientID's subscribe(..)
//    This function is invoked on unsuccessful completion or timeout of subscribe(..)
//-------------------------------------------------------------------------------------------------
function onSubscribeFailure( context, errorCode, errorMessage) { 
    console.debug( "In subClient.onSubscribeFailure " + subClientID +" "+ JSON.stringify(context) );
    var statusMsg = "Subscriber failed to complete the subscription. Reason: "+ errorMessage +"  Error Code: "+ errorCode +"\n";
    writeMessage ( statusMsg );
}

// End of Subscriber Call Back Functions


//-------------------------------------------------------------------------------------------------
// unsubscribeISMSubscriber - Initiated by pressing the Unsubscribe Button on the Subscriber HTML.
//    This function enables the Subscriber HTML fields, and unsubscribes the topic but does not 
//    disconnect the MQTT Client.
//-------------------------------------------------------------------------------------------------
function unsubscribeISMSubscriber() {
    console.debug( "In unsubscribeISMSubscriber: subClient.unsubscribe() for Client: " + subClientID );
    subDisableButtons ( false );
    var statusMsg = "Subscriber "+ subClientID +" unsubscribed.\n";
    writeMessage ( statusMsg );
    msgRecv = 0;

    subClient.unsubscribe( topicName);
}

//-------------------------------------------------------------------------------------------------
// disconnectISMSubscriber - Initiated by pressing the Disconnect Button on the Subscriber HTML.
//    This function enables the Subscriber HTML fields, and disconnects the MQTT Client.
//-------------------------------------------------------------------------------------------------
function disconnectISMSubscriber() {
    console.debug( "In disconnectISMSubscriber: subClient.disconnect() for Client: " + subClientID );
    subDisableButtons ( false );
    var statusMsg = "Subscriber "+ subClientID +" disconnected.\n";
    writeMessage ( statusMsg );
    msgRecv = 0;

    subClient.disconnect();
}

//-------------------------------------------------------------------------------------------------
// connectISMPublisher - Initiate by pressing the Publish Button on the Publisher HTML.
//    This function starts by obtaining the Messaging.Client object parameters.  The clientID must be
//    unique and not exceed 23 characters.  The Messaging.Client object stores the clientID and the 
//    connection information to the ISM Server.
//  The connect() call back functions are defined prior to making the connect() call.
//  The pubClient.onconnect() call back is called after a successful connection to ISM Server.
//    This function will perform the publish() for how many messages were requested.
//  The pubClient.onconnectionlost() function may be called on a failure during the connect() call
//    or a subsequent dropped connection after a successful connection.  
//  The pubClient.connect() will use the Messaging.Client object to connect to the ISM Server.  
//    This call will return no errors, communication is through the call back functions.
//-------------------------------------------------------------------------------------------------
function connectISMPublisher() {
  console.debug("In connectISMPublisher");
  try {
    if ( document.getElementById('ismClientId').value != "" ) {
        pubClientID = document.getElementById('ismClientId').value;
    } else {
        pubClientID = pubClientID_stem + Math.floor((Math.random()*100)+1);
    }

    ismServer=document.getElementById('ismIP').value;
    if ( document.getElementById('ismPort').value != "" ) {
        ismPort= parseInt( document.getElementById('ismPort').value );
    } else {
        ismPort= parseInt( '1883' );
    }

    pubDisableButtons( true );
    pubClient = new Messaging.Client( ismServer, ismPort, pubClientID );
    console.debug("connectISMPublisher: created ISM MQTT Publisher Client "+pubClientID);

    var statusMsg = "To Publish "+document.getElementById('msgCount').value+" messages on topic: "+document.getElementById('msgTopic').value+".\n";
    writeMessage ( statusMsg );

   // Register Call back functions for connect()
    pubClient.onMessageArrived = onPubClientMessageArrived;
    pubClient.onConnectionLost = onPubClientConnectionLost;
    if ( parseInt( document.getElementById('pubQoS').value ) !== 0 ) {
        pubClient.onMessageDelivered = onMessageDelivered;
    }

    // Build the Will Message to be sent by the Server when this client abnormally disconnects
    var willTopic_value = "willTopic";
    var willMessage_value = "R.I.P. client, "+ pubClientID +" terminated abnormally.";
    var willMessage = new Messaging.Message(willMessage_value);
    willMessage.destinationName = willTopic_value;
    willMessage.qos = 1;
    willMessage.retained = false;

    // Build the Connection Options Object
    var connectOptions = new Object();
    var context = new Object();
    context = { "clientId":pubClientID, "ismServer":ismServer, "ismPort":ismPort };
    connectOptions.invocationContext=context;

    // Per MQTT Javascript Messaging API these are the defaults except were noted
    connectOptions.timeout = parseInt( '30' );
    connectOptions.keepAliveInterval = parseInt( '60' );
    connectOptions.willMessage = willMessage;  // Default is no willMessage

    if ( !document.getElementById('ismCleanSession').checked ) {
        connectOptions.cleanSession = false;
    } else {
        connectOptions.cleanSession = true;
    }
    if ( document.getElementById('ismUID').value != "" ) {
        connectOptions.userName = document.getElementById('ismUID').value;
    }
    if ( document.getElementById('ismPassword').value != "" ) {
        connectOptions.password = document.getElementById('ismPassword').value;
    }
    if ( document.getElementById('wss').checked ) {
        connectOptions.useSSL = true;
    } else {
        connectOptions.useSSL = false;
    }

    // Callback on Connect()
    connectOptions.onSuccess = onPubClientConnect;
    connectOptions.onFailure = onPubClientConnectFailure;

    // Connect to the ISM Server - completion status is determined by the callback invoked
    pubClient.connect( connectOptions );

  } catch (e) {
      pubDisableButtons( false );
      var statusMsg = "Publisher encountered an unexpected error. Reason: "+e.message+"\n";
      writeMessage ( statusMsg );
      pubClient.disconnect();
  }
}

// Publisher Call back functions
//-------------------------------------------------------------------------------------------------
// onPubClientConnect - onSuccess() Callback for pubClient.connect()
//    This function is invoked when ...
//------------------------------------------------------------------------
function onPubClientConnect() {
      console.debug( "In onPubClientConnect " ); 
      try {
         
      msgText = document.getElementById('msgText').value;
      var message = new Messaging.Message( msgText);
        
              message.destinationName = document.getElementById('msgTopic').value;
      pubQoS = parseInt( document.getElementById('pubQoS').value );
              message.qos = pubQoS;
        
              if ( document.getElementById('retain').checked ) {
                  message.retained = true;
              } else {
                  message.retained = false;
              }
              var statusMsg = "Data Store: "+ message.retained +".\n";
              writeMessage ( statusMsg );

          // throttleWaitMSec should be used if/when problems overloading ISMSERVER or downstream subscribers, 
          // Value of 0 is unlimited fast as possible publishing (Default) [units=msgs/sec]
              var throttleWaitMSec = 0;  
              var mStartTime = new Date().getTime();
              var tStartTime = mStartTime;
        
              for ( var msgSent=1 ; msgSent <= document.getElementById('msgCount').value ; msgSent++ ) {
                  try {
  
                      pubClient.send( message );

                      if ( throttleWaitMSec > 0 ) {
                          var mCurrTime = new Date().getTime();
                          var elapsed = mCurrTime - mStartTime;
                          var projected = ( msgSent / throttleWaitMSec) * 1000.0;
                          if ( elapsed<projected ) {
                              var sleepInterval = projected - elapsed;
                              try {
                                  sleep( sleepInterval );
                              } catch( e ) {
                                  console.error ( e );
                              }
                          }
                      }
                  } catch( e ) {
                      console.error ( e );
              }
          }
          tElapseTime = new Date().getTime() - tStartTime;
          // QoS 1 and 2 need to wait for Acks before disconnecting from ISM Server
          if ( pubQoS == "0" ) {
             disconnectISMPublisher( tElapseTime );
          } else {
             pStartTime = tStartTime;
          }
      } catch ( e ) {
        var statusMsg = "Publisher lost active connection for client "+ pubClientID +".\n";
        writeMessage ( statusMsg );
        pubDisableButtons( false );

        pubClient.disconnect();          
      }
          
}

//-------------------------------------------------------------------------------------------------
// onPubClientConnectFailure - onFailure() Callback for pubClient.connect()
//    This function is invoked when ...
//------------------------------------------------------------------------
function onPubClientConnectFailure( context, errorCode, errorMessage ) {
    console.debug( "In onPubClientConnectFailure " ); 
    pubDisableButtons( false );
    var statusMsg = "Publisher connection failed for client "+ context.clientId +". Reason: "+ errorCode +". Message: "+ errorMessage +"\n";
    writeMessage ( statusMsg );
    pubClientID = "";
}


//-------------------------------------------------------------------------------------------------
// onPubClientMessageArrived - onMessageArrived() Callback for pubClientID
//    This function is invoked when ...
//------------------------------------------------------------------------
function onPubClientMessageArrived(message) {
        var statusMsg = "PUBLISHER RECEIVED: '"+ message.payload +"' on topic: "+ message.topic +".\n";
        console.log( "In pubClient.onmessage, " + statusMsg );
        writeMessage ( statusMsg );
}

//-------------------------------------------------------------------------------------------------
// onPubClientConnectionLost - onConnectionLost() Callback for pubClientID
//    This function is invoked if the connection with the ISM server has been lost.  
//    The connection can be broken when client initiates a disconnect or server/network failure.
//-------------------------------------------------------------------------------------------------
function onPubClientConnectionLost( reasonCode, reasonMessage ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        statusMsg = "Publisher connection closed for client "+ pubClientID +"\n";
    } else {
        statusMsg = "Publisher connection lost for client "+ pubClientID +". Reason: "+ reasonCode +". Message: "+ reasonMessage +"\n";
    }
    console.debug( "In onPubClientConnectionLost " + statusMsg ); 
    pubDisableButtons( false );
    writeMessage ( statusMsg );
    pubClientID = "";
}
   
//-------------------------------------------------------------------------------------------------
// onMessageDelivered
//    This function is invoked when QoS >0 and message has been received by ISM Server.
//    
//-------------------------------------------------------------------------------------------------
function onMessageDelivered( message ) { 
   console.debug( "In pubClient.onMessageDelivered " + pubClientID + "rec'd Acks:" + pubAckReceived ); 
   // When QoS 1 or 2, need to count the number of ACKs and wait to receive one for each message sent before disconnecting from the ISM Server
   if ( pubAckReceived < (document.getElementById('msgCount').value )-1) {
      if ( message.duplicate == false ) {
         pubAckReceived++;
      } else {
         console.debug( "In pubClient.onMessageDelivered " + pubClientID + "WARNING: duplicate message Ack received" );
      }
   } else {  
      pElapseTime = new Date().getTime() - pStartTime;
      disconnectISMPublisher( pElapseTime );
   }
}
   
// End of Call back functions


//----------------------------------------------------------------------------
// disconnectISMPublisher - called at the conclusion of a Publish Action to 
//    enable the Publish HTML fields and disconnect the MQTT Client.
//----------------------------------------------------------------------------
function disconnectISMPublisher( tElapseTime ) {
    console.debug( "In disconnectISMPublisher: pubClient.disconnect() " + pubClientID + " " + ismServer + ":" + ismPort );
    pubDisableButtons( false );
    var statusMsg = "Publish complete in "+tElapseTime+"ms. Disconnected from ISM Server.\n";
    writeMessage ( statusMsg );

    pubClient.disconnect();
}

//-------------------------------------------------------------------------------------------------
//  pubDisableButtons - enable or disable the Publisher HTML fields.
//  When Publish is active, pass the value 'false' to disable fields from being altered
//  When no Publish is active, allow the fields to be edited
//  Inputs:
//  greyed - Boolean 
//-------------------------------------------------------------------------------------------------
function pubDisableButtons( greyed ) {
    document.getElementById('wss').disabled = greyed;
    document.getElementById('ismUID').disabled = greyed;
    document.getElementById('ismPassword').disabled = greyed;
    document.getElementById('ismIP').disabled = greyed;
    document.getElementById('ismPort').disabled = greyed;
    document.getElementById('msgTopic').disabled = greyed;
    document.getElementById('msgCount').disabled = greyed;
    document.getElementById('msgText').disabled = greyed;
    document.getElementById('pubQoS').disabled = greyed;
    document.getElementById('retain').disabled = greyed;
    document.getElementById('pubConnect').disabled = greyed;
}

//-------------------------------------------------------------------------------------------------
//  subDisableButtons - enable or disable the Subscriber HTML fields.
//  When Subscribe is active, pass the value 'false' to disable fields from being altered
//  When no Subscribe is active, allow the fields to be edited, only disable Disconnect Button
//  Inputs:
//  greyed - Boolean 
//-------------------------------------------------------------------------------------------------
function subDisableButtons( greyed ) {
    document.getElementById('wss').disabled = greyed;
    document.getElementById('ismUID').disabled = greyed;
    document.getElementById('ismPassword').disabled = greyed;
    document.getElementById('ismIP').disabled = greyed;
    document.getElementById('ismPort').disabled = greyed;
    document.getElementById('msgTopic').disabled = greyed;
    document.getElementById('subQoS').disabled = greyed;
    document.getElementById('subConnect').disabled = greyed;
    document.getElementById('subDisconnect').disabled = !greyed;
}

//-------------------------------------------------------------------------------------------------
//  writeMessage - Write message to the HTML's textarea, 'messages',  after setting the cursor 
//    position to the bottom of the textarea.
//  Inputs:
//  statusMsg - String  Message text to write in textarea. 
//-------------------------------------------------------------------------------------------------
function writeMessage( statusMsg ) {
    document.getElementById('messages').scrollTop = document.getElementById('messages').scrollHeight;
    document.getElementById('messages').value += statusMsg;
}


