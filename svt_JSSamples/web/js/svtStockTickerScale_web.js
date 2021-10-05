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
function connectISMSubscriber(index) {
  console.debug("In connectISMSubscriber");
  var subClientID;
  if (sharedSub == null) {
    subClientID = subClientID_stem + index;
  } else {
    subClientID = "$SharedSubscription/" + sharedSub + "/" + subClientID_stem + index;
  }

  if (maxClientID) {
    fillLen = 1024-subClientID.length;
    var fillstr = new Array(fillLen).join("X");
    subClientID = fillstr + subClientID_stem + index;
  }

  try {    
      if (!zeroLenClientID) {
        subClient[index] = new Messaging.Client( ismServer, ismPort, subClientID );
      } else {
        subClient[index] = new Messaging.Client( ismServer, ismPort, "" );
      }
    console.debug("connectISMSubscriber: created ISM MQTT Subscriber Client "+ subClientID );

    // Register Callback Functions for connect()
    if (!zeroLenClientID) {
        subClient[index].onMessageArrived = onSubClientMessageArrived;
        subClient[index].onConnectionLost = onSubClientConnectionLost;
    } else {
        if (subCount > 3) throw new Error("svtStockTickerScale_web.js does not support more than three anonymous subscribe clients");
        else if (index == 0) {
            subClient[index].onMessageArrived = onSubClientMessageArrived0;
            subClient[index].onConnectionLost = onSubClientConnectionLost0;
    }
    else if (index == 1) {
        subClient[index].onMessageArrived = onSubClientMessageArrived1;
        subClient[index].onConnectionLost = onSubClientConnectionLost1;
    }
    else if (index == 2) {
        subClient[index].onMessageArrived = onSubClientMessageArrived2;
        subClient[index].onConnectionLost = onSubClientConnectionLost2;
    }
  }

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
    // Per MQTT Javascript Messaging API these are the defaults except were noted
    var waitTime = 120;
    connectOptions.timeout = parseInt( waitTime );
    if ( subUser != null ) {  connectOptions.userName = subUser; }
    if ( subPswd != null ) {  connectOptions.password = subPswd; }
    connectOptions.cleanSession = subCleanSession;
    connectOptions.willMessage = willMessage;  // Default is no willMessage
    if (mqttVersion != null) {
        connectOptions.mqttVersion = mqttVersion;  // Default is 3
    }

// JCD Default is 60, 0 is unlimited -- this was to work around defect REMOVE
    connectOptions.keepAliveInterval = parseInt('0');
    // If other clients have connection problems, don't want connected clients to start disconnecting and thrashing on connections
//    connectOptions.keepAliveInterval = parseInt( waitTime*2 );

    if ( subSecure ) { connectOptions.useSSL = subSecure; }
    connectOptions.onSuccess=onSubClientConnect;
    connectOptions.onFailure=onSubClientConnectFailure;
    context = { clientIndex:index };
    connectOptions.invocationContext=context;

    // Connect to the ISM Server - completion status is determined by the callback invoked
    subClientConnect[index]=false;
    subClientOpts[index] = connectOptions ;
    subClient[index].connect( connectOptions );

  } catch (e) {
      var statusMsg = subClientID +" Subscriber encountered an unexpected error. Reason: "+e.message+"\n";
      writeMessage ( "sMessage", statusMsg );

      subClientConnect[index]=false;
      subClient[index].disconnect();

  }
}


// Subscriber Call Back Functions
//-------------------------------------------------------------------------------------------------
// onSubClientConnect - onConnect() Callback for subClientID
//    This function is invoked when ...
//------------------------------------------------------------------------
function onSubClientConnect( context ) {
    index = context.invocationContext.clientIndex;
    console.debug( "In onSubClientConnect for "+ subClient[ index ].clientId +" Context: "+ JSON.stringify(context) ); 

    try {
        subClientConnect[index]=true;

        var subscribeOpts = new Object();
        var context = new Object();

        subscribeOpts.qos = parseInt( subQoS );
        subscribeOpts.onSuccess = onSubscribeSuccess; 
        subscribeOpts.onFailure = onSubscribeFailure;

        context = { clientIndex:index };
        subscribeOpts.invocationContext=context;

        subClient[index].subscribe( subTopic, subscribeOpts );

    } catch ( e ) {
        var statusMsg = "Subscriber client, "+  subClient[ index ].clientId +", no longer has an active connection, Message: "+ e.message +". Try to reconnect.\n";
        writeMessage ( "sMessage", statusMsg );

        subClientConnect[index]=false;
        subClient[index].connect( subClientOpts[index] );          
    }
}


//-------------------------------------------------------------------------------------------------
// onSubClientConnectFailure - onConnect() Callback Failure
//    This function is invoked when ...
//------------------------------------------------------------------------
function onSubClientConnectFailure( context, errorCode, errorMessage) {
    index = context.invocationContext.clientIndex;
    console.debug( "In onSubClientConnectFailure for "+ subClient[ index ].clientId +" Context: "+ JSON.stringify(context) );

    subClientConnect[index]=false;

    if ( pubComplete != true ) {
        var statusMsg = "Subscriber connection failed for client "+ subClient[ index ].clientId +". Reason: "+ errorCode +" Message: "+ errorMessage +". Try to reconnect.\n";
        writeMessage ( "sMessage", statusMsg );

        subClient[index].connect( subClientOpts[index] );

    } else {
        var statusMsg = "Subscriber connection failed for client "+ subClient[ index ].clientId +". Reason: "+ errorCode +" Message: "+ errorMessage +". Publishing is complete, WILL NOT try to reconnect.\n";
        writeMessage ( "sMessage", statusMsg );
    }

}


//-------------------------------------------------------------------------------------------------
// onSubClientMessageArrived - onMessageArrived() Callback for subClientID
//    This function is invoked when ...
//------------------------------------------------------------------------
  function onSubClientMessageArrived( message ) {
  
//        index = getClientIndex( this._localKey );
     index = getClientIndex( this.clientId );    
        if ( subClientFirstMsg[ index ] == null ) {
          var statusMsg = "First Message received for client "+subClient[ index ].clientId+"\n" ;
          writeMessage( "sMessage", statusMsg ); 
          console.log( statusMsg ); 
          subClientFirstMsg[ index ] = new Date().getTime();
          if ( subFirstMsg == null ) { subFirstMsg = subClientFirstMsg[ index ]; }
        }
        subClientMsgCount[ index ]++;    // Count for this client's msgs received
        msgRecv++;                        // Count for all msgs for all clients
        
        try {
            var statusMsg =  this.clientId +" Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadString+"\n" ;
        } catch (e) {
            var statusMsg = this.clientId +" Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadBytes+"\n" ;
        }
        if ( subClientVerbose ) {
            console.log( "In startISMSubscriber.onmessage, " + statusMsg);
            writeMessage ( "sMessage", statusMsg );
        }


        // Client Statistical Check for success        
        if ( expectedMsgsClient == subClientMsgCount[ index ] ) {
            subClientLastMsg[ index ] = new Date().getTime();
            var subClientElapsed = ( subClientLastMsg[ index ] - subClientFirstMsg[ index ] )/1000;
            var subClientRate = parseFloat(subClientMsgCount[ index ]) / parseFloat(subClientElapsed) ;
        
          var statusMsg = "Sub Client "+ this.clientId +" Completed!  Expected Msgs: "+ expectedMsgsClient +"  Received Msgs: "+ subClientMsgCount[ index ] +"  Elapsed Time: "+ subClientElapsed +" sec.  Rate: "+ subClientRate +" msgs/sec. \n" ;
          writeMessage ( "sMessage", statusMsg );
         }

        // Final Statistical Check for Success
        if ( expectedMsgsTotal == msgRecv ) {
            var subElapsed = ( parseFloat(subClientLastMsg[ index ]) - parseFloat(subFirstMsg) )/1000;
            var subRate = parseFloat(expectedMsgsTotal) / parseFloat(subElapsed);
        
            var statusMsg = "Test Successful!  Expected Msgs: "+ expectedMsgsTotal +"  Received Msgs: "+ msgRecv +"  Elapsed Time: "+ subElapsed +" sec.  Rate: "+ subRate  +" msgs/sec. \n" ;
            writeMessage ( "sMessage", statusMsg );
            for ( var i=0; i<subCount; i++ )
            {
                disconnectISMSubscriber(i);
                sleep( 60 );
            }
            // Close the Broswer Window
            if ( winClose  ) {
            	window.setTimeout(function() { window.close(); },1500);
            }
         }
    }

  function onSubClientMessageArrived0( message ) {
      onSubClientMessageArrivedWithIndex(message, 0);
  }
  function onSubClientMessageArrived1( message ) {
      onSubClientMessageArrivedWithIndex(message, 1);
  }
  function onSubClientMessageArrived2( message ) {
      onSubClientMessageArrivedWithIndex(message, 2);
  }
//-------------------------------------------------------------------------------------------------
// onSubClientMessageArrived - onMessageArrived() Callback for subClientID
//    This function is invoked when ...
//------------------------------------------------------------------------
  function onSubClientMessageArrivedWithIndex( message, index ) {
  
        if ( subClientFirstMsg[ index ] == null ) {
          var statusMsg = "First Message received for client "+subClient[ index ].clientId+"\n" ;
          writeMessage( "sMessage", statusMsg ); 
          console.log( statusMsg ); 
          subClientFirstMsg[ index ] = new Date().getTime();
          if ( subFirstMsg == null ) { subFirstMsg = subClientFirstMsg[ index ]; }
        }
        subClientMsgCount[ index ]++;    // Count for this client's msgs received
        msgRecv++;                        // Count for all msgs for all clients
        
        try {
            var statusMsg =  this.clientId +" Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadString+"\n" ;
        } catch (e) {
            var statusMsg = this.clientId +" Message "+msgRecv+" received on topic '"+message.destinationName+"': "+message.payloadBytes+"\n" ;
        }
        if ( subClientVerbose ) {
            console.log( "In startISMSubscriber.onmessage, " + statusMsg);
            writeMessage ( "sMessage", statusMsg );
        }


        // Client Statistical Check for success        
        if ( expectedMsgsClient == subClientMsgCount[ index ] ) {
            subClientLastMsg[ index ] = new Date().getTime();
            var subClientElapsed = ( subClientLastMsg[ index ] - subClientFirstMsg[ index ] )/1000;
            var subClientRate = parseFloat(subClientMsgCount[ index ]) / parseFloat(subClientElapsed) ;
        
          var statusMsg = "Sub Client "+ index +" Completed!  Expected Msgs: "+ expectedMsgsClient +"  Received Msgs: "+ subClientMsgCount[ index ] +"  Elapsed Time: "+ subClientElapsed +" sec.  Rate: "+ subClientRate +" msgs/sec. \n" ;
          writeMessage ( "sMessage", statusMsg );
         }

        // Final Statistical Check for Success
        if ( expectedMsgsTotal == msgRecv ) {
            var subElapsed = ( parseFloat(subClientLastMsg[ index ]) - parseFloat(subFirstMsg) )/1000;
            var subRate = parseFloat(expectedMsgsTotal) / parseFloat(subElapsed);
        
            var statusMsg = "Test Successful!  Expected Msgs: "+ expectedMsgsTotal +"  Received Msgs: "+ msgRecv +"  Elapsed Time: "+ subElapsed +" sec.  Rate: "+ subRate  +" msgs/sec. \n" ;
            writeMessage ( "sMessage", statusMsg );
            for ( var i=0; i<subCount; i++ )
            {
                disconnectISMSubscriber(i);
                sleep( 60 );
            }
            // Close the Broswer Window
            if ( winClose  ) {
            	window.setTimeout(function() { window.close(); },1500);
            }
         }
    }


//-------------------------------------------------------------------------------------------------
// onSubClientConnectionLost - onConnectionLost() Callback for subClientID
//    This function is invoked if the connection with the ISM server has been lost.  
//    The connection can be broken when client initiates a disconnect or server/network failure.
//-------------------------------------------------------------------------------------------------
function onSubClientConnectionLost( reasonCode, reasonMessage ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        // If reason is undefined, it usually means User Initiated disconnect.
        statusMsg = "Subscriber connection closed for client "+ this.clientId ;
    } else {
        statusMsg = "Subscriber connection lost for client "+ this.clientId +". Reason: "+ reasonCode +". Message: "+ reasonMessage ;
    }
    console.debug( "In onSubClientConnectionLost: " + statusMsg); 

    index = getClientIndex( this.clientId );
    subClientConnect[index]=false;

    if ( pubComplete != true ) {
      var statusMsg = statusMsg +". Publishing is active, Try to reconnect.\n";
      writeMessage ( "sMessage", statusMsg );

      subClient[index].connect( subClientOpts[index] );

    } else {
      var statusMsg = statusMsg +". Publishing is complete, WILL NOT try to reconnect.\n";
      writeMessage ( "sMessage", statusMsg );
    }
}

function onSubClientConnectionLost0( reasonCode, reasonMessage) {
nSubClientConnectionLostWithIndex(reasonCode, reasonMessage,0 ); 
}
function onSubClientConnectionLost1( reasonCode, reasonMessage) {
nSubClientConnectionLostWithIndex(reasonCode, reasonMessage,1 ); 
}
function onSubClientConnectionLost2( reasonCode, reasonMessage) {
nSubClientConnectionLostWithIndex(reasonCode, reasonMessage,2 ); 
}

//-------------------------------------------------------------------------------------------------
// onSubClientConnectionLost - onConnectionLost() Callback for subClientID
//    This function is invoked if the connection with the ISM server has been lost.  
//    The connection can be broken when client initiates a disconnect or server/network failure.
//-------------------------------------------------------------------------------------------------
function onSubClientConnectionLostWithIndex( reasonCode, reasonMessage,index ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        // If reason is undefined, it usually means User Initiated disconnect.
        statusMsg = "Subscriber connection closed for client "+ this.clientId ;
    } else {
        statusMsg = "Subscriber connection lost for client "+ this.clientId +". Reason: "+ reasonCode +". Message: "+ reasonMessage ;
    }
    console.debug( "In onSubClientConnectionLost: " + statusMsg); 

    subClientConnect[index]=false;

    if ( pubComplete != true ) {
      var statusMsg = statusMsg +". Publishing is active, Try to reconnect.\n";
      writeMessage ( "sMessage", statusMsg );

      subClient[index].connect( subClientOpts[index] );

    } else {
      var statusMsg = statusMsg +". Publishing is complete, WILL NOT try to reconnect.\n";
      writeMessage ( "sMessage", statusMsg );
    }
}

//-------------------------------------------------------------------------------------------------
// onSubscribeSuccess - onSuccess() Callback for subClientID's subscribe(..)
//    This function is invoked on unsuccessful completion of subscribe(..)
//-------------------------------------------------------------------------------------------------
function onSubscribeSuccess( context ) { 
    index = context.invocationContext.clientIndex;
    console.debug( "In onSubscribeSuccess " + subClient[ index ].clientId +" Context: "+ JSON.stringify(context) ); 

    var statusMsg = subClient[ index ].clientId +" Subscribed to topic: "+ subTopic +" with QoS="+ subQoS +".\n" ;
    writeMessage ( "sMessage", statusMsg );

    // If all the Subscribers have been started and Subscribed, start the Publishers
    if ( subAllStarted() && !publisherStarted ) {
        publisherStarted=true;
        startPublisher();
    }
}

//-------------------------------------------------------------------------------------------------
// onSubscribeFailure - onFailure() Callback for subClientID's subscribe(..)
//    This function is invoked on unsuccessful completion or timeout of subscribe(..)
//-------------------------------------------------------------------------------------------------
function onSubscribeFailure( context, errorCode, errorMessage ) { 
    index = context.invocationContext.clientIndex;
    console.debug( "In onSubscribeFailure " + subClient[ index ].clientId +" Context: "+ JSON.stringify(context)  );
    var statusMsg = subClient[ index ].clientId +" Subscriber failed to complete the subscription. Error Code: "+ errorCode +"  Error Message: "+ errorMessage +"\n";
    writeMessage ( "sMessage", statusMsg );
}

// End of Subscriber Call Back Functions


//-------------------------------------------------------------------------------------------------
// disconnectISMSubscriber - Initiated by pressing the Disconnect Button on the Subscriber HTML.
//    This function enables the Subscriber HTML fields, unsubscribes the topic and disconnects
//    the MQTT Client.
//-------------------------------------------------------------------------------------------------
function disconnectISMSubscriber( index ) {
    try  {
        console.debug( "In disconnectISMSubscriber: subClient.disconnect() " + subClient[index].clientId );
        var statusMsg = subClient[index].clientId +" Subscriber disconnected.\n";
        writeMessage ( "sMessage", statusMsg );

        subClient[index].unsubscribe( subTopic);
        subClient[index].disconnect();
    } catch ( e ) {
        console.debug( "In disconnectISMSubscriber: subClient.disconnect()  subClient["+index+"] already disconnected.  Exception: "+ e.message );
    }
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
function connectISMPublisher(index) {
    console.debug("In connectISMPublisher");
    var pubClientID = pubClientID_stem + index;

    try {
        if (!zeroLenClientID) {
            pubClient[index] = new Messaging.Client( ismServer, ismPort, pubClientID );
        } else {
            pubClient[index] = new Messaging.Client( ismServer, ismPort, "" );
        }
        console.debug("connectISMPublisher: created ISM MQTT Publisher Client "+ pubClientID );

        var statusMsg = pubClientID +" to Publish "+ msgCount +" Qos="+ pubQoS +" messages on topic: "+ pubTopic +".\n";
        writeMessage ( "pMessage", statusMsg );

        // Register Call back functions for connect()
        pubClient[index].onMessageArrived = onPubClientMessageArrived;
        if (!zeroLenClientID) {
            pubClient[index].onConnectionLost = onPubClientConnectionLost;
        } else {
            if (pubCount > 3) throw new Error("svtStockTickerScale_web.js does not support more than three anonymouse publish clients");
            else if (index == 0) {
                pubClient[index].onConnectionLost = onPubClientConnectionLost0;
            } else if (index == 1) {
                pubClient[index].onConnectionLost = onPubClientConnectionLost1;
            } else if (index == 2) {
                pubClient[index].onConnectionLost = onPubClientConnectionLost2;
            }
        }
        if ( pubQoS !== 0 ) {
            pubClient[index].onMessageDelivered = onMessageDelivered;
        }

        // Build the Will Message to be sent by the Server when this client abnormally disconnects
        var willTopic_value = "willTopic";
        var willMessage_value = "R.I.P. client, "+ pubClientID +" terminated abnormally.";
        var willMessage = new Messaging.Message(willMessage_value);
        willMessage.destinationName = willTopic_value;
        willMessage.qos = 1;
        willMessage.retained =  false;

        // Build the Connection Options Object
        var connectOptions = new Object();
        var context = new Object();

        // Per MQTT Javascript Messaging API these are the defaults except were noted
        var waitTime = 120;
        connectOptions.timeout = parseInt( waitTime );
        if ( pubUser != null ) { connectOptions.userName = pubUser; }
        if ( pubPswd != null ) { connectOptions.password = pubPswd; }
        connectOptions.cleanSession = pubCleanSession;
        connectOptions.willMessage = willMessage;  // Default is no willMessage
        if (mqttVersion != null) {
            connectOptions.mqttVersion = mqttVersion;  // Default is 3
        }

// JCD Default is 60, 0 is unlimited -- this was to work around defect REMOVE
connectOptions.keepAliveInterval = parseInt('0');
        // If other clients have connection problems, don't want connected clients to start disconnecting and thrashing on connections
//        connectOptions.keepAliveInterval = parseInt( waitTime*2 );

        if ( pubSecure ) { connectOptions.useSSL = pubSecure; }
    
        connectOptions.onSuccess=onPubClientConnect;
        connectOptions.onFailure=onPubClientConnectFailure;
        context = { clientIndex:index };
        connectOptions.invocationContext=context;

        // Connect to the ISM Server - completion status is determined by the callback invoked
        pubClientConnect[index] = false;
        pubClientOpts[index] = connectOptions;
        pubClient[index].connect( connectOptions );

  } catch (e) {
      var statusMsg = pubClientID +" Publisher encountered an unexpected error. Reason: "+e.message+". Terminating!\n";
      writeMessage ( "pMessage", statusMsg );

      pubClientConnect[index] = false;
      pubClient[index].disconnect();
  }
}

// Publisher Call back functions
//----------------------------------------------------------------------------
// onPubClientConnect - called 
//    
//----------------------------------------------------------------------------
function onPubClientConnect( context ) {
    index = context.invocationContext.clientIndex;
    console.debug( "In onPubClientConnect for "+ pubClient[ index ].clientId +" Context: "+ JSON.stringify(context) ); 

    try {
     // Indicate that this client is connected
        pubClientConnect[index]=true;

        if ( pubRetain ) {
            var statusMsg = pubClient[ index ].clientId +" Data Store: "+ pubRetain +".\n";
            writeMessage ( "pMessage", statusMsg );
        }

          // throttleWaitMSec should be used if/when problems overloading ISMSERVER or downstream subscribers, 
          // Value of 0 is unlimited fast as possible publishing (Default) [units=msgs/sec]
          // Set in svtStockTicker.js   none is DEFAULT: var throttleWaitMSec = 0;  
        var mStartTime = new Date().getTime();

        // Save statistic fields for very FirstMessage and First Message for this client
        if ( pubClientFirstMsg[ index ] == null ) {
            pubClientFirstMsg[ index ] = mStartTime;
            if ( pubFirstMsg == null ) { pubFirstMsg = mStartTime; }
        }
        // If Publisher loses connection and restarts, where to resume count from
        var msgStart=1;
        if ( pubClientMsgCount[ index ] == null ) { 
         pubClientMsgCount[ index ]= 0; 
        } else {
         msgStart = pubClientMsgCount[ index ];
        }

        if (DEBUG) { pubClient[ index ].startTrace(); }
        var message=null;
        for ( var msgSent=msgStart ; msgSent <= msgCount ; msgSent++ ) {
            try {

                var msgPayload = msgText +"::"+ pubClient[ index ].clientId +"::"+ msgSent;
                message = new Messaging.Message( msgPayload);
                message.destinationName = pubTopic;
                pubQoS = parseInt( pubQoS );
                message.qos = pubQoS;
                message.retained = pubRetain;

                if (DEBUG) { console.log( pubClient[ index ].getTraceLog() ); }
                pubClient[index].send( message );
                if (DEBUG) { console.log( pubClient[ index ].getTraceLog() ); }
            
                if ( throttleWaitMSec > 0 ) {
                    var mCurrTime = new Date().getTime();
                    var elapsed = mCurrTime - mStartTime;
                    var projected = ( msgSent / throttleWaitMSec) * 1000.0;
                    if ( elapsed<projected ) {
                        var sleepInterval = projected - elapsed;
                        try {
                            pubThrottle = sleep( sleepInterval );
                        } catch( e ) {
                            console.error ( e );
                        }
                    }
                } 
            } catch( e ) {
                 console.error ("UNEXPECTED ERROR DURING SEND: "+ e +" Message: "+ e.message  );
                 if (DEBUG) { console.log( pubClient[ index ].getTraceLog() ); }
            }
            pubClientMsgCount[ index ]++;
        }

        // THIS pubClient completed, compute and report the statistics
        pubClientLastMsg[ index ] = new Date().getTime();
        pubClientElapsed = pubClientLastMsg[ index ] - pubClientFirstMsg[ index ];
        pubCompleteStatus[index]= true;
        var pubClientElapsed = ( parseFloat(pubClientLastMsg[ index ]) - parseFloat(pubClientFirstMsg[ index ]) )/1000;
        var pubClientRate = parseFloat(pubClientMsgCount[ index ]) / parseFloat(pubClientElapsed) ;
        var statusMsg;
        if (!zeroLenClientID) {
            statusMsg = "Pub Client "+pubClient[index].clientId +" completed. Published "+pubClientMsgCount[ index ]+" msgs. in "+pubClientElapsed+"sec.  Rate: "+pubClientRate+" msgs/sec.\n";
        } else {
            statusMsg = "Pub Client "+index+" completed. Published "+pubClientMsgCount[ index ]+" msgs. in "+pubClientElapsed+"sec.  Rate: "+pubClientRate+" msgs/sec.\n";
        }
        writeMessage ( "pMessage", statusMsg );

        var i;
        // Check through the arrary of publishers to see if all publishers are finished.
        for ( i=0; i<pubCount ; i++ ) {
            if ( pubCompleteStatus[i] != true ) {
                break;
            }
        }
        if ( i == pubCount ) {
            pubComplete = true;
            var pubElapsed = ( parseFloat(pubClientLastMsg[ index ]) - parseFloat(pubFirstMsg) )/1000;
            var pubRate = parseFloat(expectedMsgsClient) / parseFloat(pubElapsed);
            var statusMsg = "ALL Publishers completed. Published "+expectedMsgsClient+" msgs. in "+pubElapsed+"sec. Rate: "+pubRate+" msg/sec.\n";
            writeMessage ( "pMessage", statusMsg );
        }
        if ( pubQoS == "0" ) {
           disconnectISMPublisher( index );
        }        
    } catch ( e ) {

        if (DEBUG) { 
            console.log( pubClient[ index ].getTraceLog() ); 
            pubClient[ index ].stopTrace(); 
        }

        var statusMsg = "Publisher client, "+  pubClient[ index ].clientId +", UNEXPECTED ERROR in onPubClientConnect.  Message: "+ e.message+"\n";
        writeMessage ( "pMessage", statusMsg );

        pubClientConnect[index] = false;
    }
}

//-------------------------------------------------------------------------------------------------
// onPubClientConnectFailure - onConnect() Callback Failure
//    This function is invoked when ...
//------------------------------------------------------------------------
function onPubClientConnectFailure( context, errorCode, errorMessage) {
    index = context.invocationContext.clientIndex;
    console.debug( "In onPubClientConnectFailure for "+ pubClient[ index ].clientId ); 

    pubClientConnect[index]=false;

    // If this client is not publish complete try to reconnect soit can resume
    if ( pubCompleteStatus[ index ] != true ) {
        var statusMsg = "Publisher connection failed for client "+ pubClient[ index ].clientId +". Reason: "+ errorCode +" Message: "+ errorMessage +". Try to reconnect.\n";
        writeMessage ( "pMessage", statusMsg );

        pubClient[index].connect( pubClientOpts[index] );

    } else {
        var statusMsg = "Publisher connection failed for client "+ pubClient[ index ].clientId +". Reason: "+ errorCode +" Message: "+ errorMessage +". Publishing is complete for this client.\n";
        writeMessage ( "pMessage", statusMsg );
    }

}


//----------------------------------------------------------------------------
// onPubClientMessageArrived - called 
//    
//----------------------------------------------------------------------------
function onPubClientMessageArrived( message ) {
    var statusMsg = this.clientId +" PUBLISHER RECEIVED: '"+ message.payload +"' on topic: "+ message.topic +".\n";
    console.log( "In onPubClientMessageArrived, " + statusMsg );
    writeMessage ( "pMessage", statusMsg );
}

//----------------------------------------------------------------------------
// onPubClientConnectionLost - onConnectionLost() Callback for subClientID
//    This function is invoked if the connection with the ISM server has been lost.  
//    The connection can be broken when client initiates a disconnect or server/network failure.
//----------------------------------------------------------------------------
function onPubClientConnectionLost( reasonCode, reasonMessage ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        statusMsg = "Publisher connection closed for client "+ this.clientId ;
    } else {
        statusMsg = "Publisher connection lost for client "+ this.clientId +". Reason: "+ reasonCode +". Message: "+ reasonMessage ;
    }
    console.debug( "In onPubClientConnectionLost: " + statusMsg); 

    index = getClientIndex( this.clientId );

    if ( pubCompleteStatus[ index ] != true ) {
      var statusMsg = statusMsg +". Publishing is NOT complete, Try to reconnect.\n";
      writeMessage ( "pMessage", statusMsg );

      pubClient[index].connect( pubClientOpts[index] );

    } else {
      var statusMsg = statusMsg +". Publishing is complete for this client.\n";
      writeMessage ( "pMessage", statusMsg );
    }
}
    
function onPubClientConnectionLost0( reasonCode, reasonMessage ) { 
    onPubClientConnectionLost( reasonCode, reasonMessage, 0 );
}
function onPubClientConnectionLost1( reasonCode, reasonMessage ) { 
    onPubClientConnectionLost( reasonCode, reasonMessage, 1 );
}
function onPubClientConnectionLost2( reasonCode, reasonMessage ) { 
    onPubClientConnectionLost( reasonCode, reasonMessage, 2 );
}
//    The connection can be broken when client initiates a disconnect or server/network failure.
//----------------------------------------------------------------------------
function onPubClientConnectionLostWithIndex( reasonCode, reasonMessage, index ) { 
    var statusMsg;
    if ( typeof reasonCode === "undefined" ) {
        statusMsg = "Publisher connection closed for client "+ this.clientId ;
    } else {
        statusMsg = "Publisher connection lost for client "+ this.clientId +". Reason: "+ reasonCode +". Message: "+ reasonMessage ;
    }
    console.debug( "In onPubClientConnectionLost: " + statusMsg); 

    if ( pubCompleteStatus[ index ] != true ) {
      var statusMsg = statusMsg +". Publishing is NOT complete, Try to reconnect.\n";
      writeMessage ( "pMessage", statusMsg );

      pubClient[index].connect( pubClientOpts[index] );

    } else {
      var statusMsg = statusMsg +". Publishing is complete for this client.\n";
      writeMessage ( "pMessage", statusMsg );
    }
}
    
//----------------------------------------------------------------------------
// onMessageDelivered - called 
//    
//----------------------------------------------------------------------------
function onMessageDelivered( message ) { 
    console.debug( "In pubClient.onMessageDelivered on Topic: "+ message.destinationName +"' QoS: "+ message.qos +"' Retained: "+ message.retained +"' Duplicate: "+ message.duplicate +"' Text: "+ message.payloadString ); 
   // When QoS 1 or 2, need to count the number of ACKs and wait to receive one for each message sent before disconnecting from the ISM Server
   if ( pubAckReceived < (msgCount * pubCount)) {
      if ( message.duplicate == false ) {
         pubAckReceived++;
      } else {
         console.debug( "In pubClient.onMessageDelivered " + pubClientID + "WARNING: duplicate message Ack received" );
      }
   } else {  
      for ( i=0; i<pubCount ; i++ ) {
         disconnectISMPublisher( i );
      }
   }
}
    
// End of Call back functions


//----------------------------------------------------------------------------
// disconnectISMPublisher - called at the conclusion of a Publish Action to 
//    enable the Publish HTML fields and disconnect the MQTT Client.
//----------------------------------------------------------------------------
function disconnectISMPublisher( index ) {
    try {
        console.debug( "In disconnectISMPublisher: pubClient.disconnect() " + pubClient[index].clientId );
        var statusMsg = pubClient[index].clientId +" Publisher disconnected.\n";
        writeMessage ( "pMessage", statusMsg );

        pubClientConnect[index]=false;
        pubClient[index].disconnect();
    } catch ( e ) {
        console.debug( "In disconnectISMPublisher: pubClient.disconnect()  pubClient["+index+"] already disconnected.  Exception: "+ e.message );
    }
}


//-------------------------------------------------------------------------------------------------
//  writeMessage - Write message to the HTML's textarea, 'messages',  after setting the cursor 
//    position to the bottom of the textarea.
//  Inputs:
//  textArea  - String  ID of the TextArea to write into
//  statusMsg - String  Message text to write in textarea. 
//-------------------------------------------------------------------------------------------------
function writeMessage( textArea, statusMsg ) {
    document.getElementById(textArea).scrollTop = document.getElementById(textArea).scrollHeight;
    document.getElementById(textArea).value += new Date() +":: "+statusMsg+"\n";
    writeLog( logfile, statusMsg+"\n" );
}


function startSubscriber() {
    // On the fly parameters passed in URL ?logfile=logfilename&ismServer=9.3.x.x&ismPort=16902&subSecure=true&pubSecure=true
    var urlParam=getURLParameter("logfile");
    if ( urlParam != null ) {logfile=urlParam; }
    setLog( logfile );

// need unique clientIDs across all browsers
    var urlParam=getURLParameter("clientID");
    if ( urlParam != null ) { 
        subClientID_stem=subClientID_stem + urlParam + "_"; 
        subTopic=subTopic + "/" + urlParam; 
        pubClientID_stem=pubClientID_stem + urlParam + "_"; 
        pubTopic=pubTopic + "/" + urlParam; 
    } else {
        subClientID_stem=subClientID_stem +  "_"; 
        pubClientID_stem=pubClientID_stem +  "_"; 
    }

    var urlParam=getURLParameter("mqttVersion");
    if ( urlParam != null ) { 
        mqttVersion=parseInt(urlParam);
    }

    var urlParam=getURLParameter("subQoS");
    if ( urlParam != null ) { 
        subQoS=parseInt(urlParam);
    }

    var urlParam=getURLParameter("sharedSub");
    if ( urlParam != null ) { 
        sharedSub=urlParam;
    }

    var urlParam=getURLParameter("zeroLenClientID");
    if ( urlParam != null ) { 
        zeroLenClientID=urlParam;
    }

    var urlParam=getURLParameter("maxClientID");
    if ( urlParam != null ) { 
        maxClientID=urlParam;
    }

    var urlParam=getURLParameter("ismServer");
    if ( urlParam != null ) { ismServer=urlParam; }

    var urlParam=getURLParameter("ismPort");
    if ( urlParam != null ) { ismPort=parseInt(urlParam); }

    var urlParam=getURLParameter("subSecure");
    if ( urlParam != null ) { subSecure=urlParam; }

    var urlParam=getURLParameter("pubSecure");
    if ( urlParam != null ) { pubSecure=urlParam; }

    var urlParam=getURLParameter("winClose");
    if ( urlParam != null ) { winClose=urlParam; }

    // Clear the Subscriber Output TextArea
    document.getElementById("sMessage").value="";

    // If no Subscribers are started, manually start the Publisher.  Otherwise Publishers get started only after all Subscribers have successful onConnect()
    if ( subCount > 0 ) {
      for ( var i=0; i<subCount; i++ )
      {
        // How odd, sometimes the Arrays have garbage in them....inital them to null
        subClient[i]= null;
        subClientOpts[i]= null;
        subClientConnect[i]= null;
        subClientFirstMsg[i]= null;
        subClientLastMsg[i]= null;
        subClientMsgCount[i]= null;

        connectISMSubscriber(i);
        sleep( subPace );
      }
    } else {
      startPublisher();
    }
}

function forceCleanSub() {
    document.getElementById("sMessage").value="";
    for ( var i=0; i<subCount; i++ )
    {
      if ( subClientConnect[i] ) {
        subClient[i].disconnect();
        sleep( 1000 );
      }
    }
}

function startPublisher() {
    document.getElementById("pMessage").value="";
    for ( var i=0; i<pubCount; i++ )
    {
        // How odd, sometimes the Arrays have garbage in them....inital them to null
        pubClient[i]= null;
        pubClientOpts[i]= null;
        pubClientConnect[i]= null;
        pubCompleteStatus[i]= null;
        pubClientFirstMsg[i]= null;
        pubClientLastMsg[i]= null;
        pubClientMsgCount[i]= null;

        connectISMPublisher(i);
        sleep( pubPace );
    }
}

function forceCleanPub() {
    document.getElementById("pMessage").value="";
    for ( var i=0; i<pubCount; i++ )
    {
      if ( pubClientConnect[i] ) {
        pubClient[i].disconnect();
        sleep( 1000 );
      }
    }
}

function sleep( msecs ) {
  var tStart = new Date();
  while ((new Date()) - tStart <= msecs) { /* do nothing */}
}

function subAllStarted() {
  for ( var i=0; i<subCount; i++ ) {
    if ( !subClientConnect[i] ) {
    console.debug( "subAllStarted returning false for index "+i ); 
      return false;
    }
  }
  console.debug( "subAllStarted returning true"); 
  return true;
}

function getClientIndex( clientId ) {
// clientId is clientStem_index
  var thisClient = clientId.split("_");
  var index = thisClient[1];
  // remove the trailing ':'   THIS WAS ONLY NECESSARY when I was passing _localKey and clientId looked like IP:PORT:clientStem_index:, will leave it for now...
  index = index.replace(":","");
  return index
}

function setLog( test ) {
    // Create the path for the logfile to make
    // sure it gets in the appropriate directory
    var curURL = window.location.pathname;
    var prefix = curURL.substr(0,curURL.lastIndexOf('/')+1);
    var baseName = test + ".log";
    logfile = prefix + "../" + baseName;
}


if ( document.readyState === "complete" ) {
    startSubscriber();
};
