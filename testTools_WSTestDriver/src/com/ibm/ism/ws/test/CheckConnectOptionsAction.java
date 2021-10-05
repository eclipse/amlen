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
/*
 * Checks desired properties on the CONNACK packet
 * Only works with pahov5 client
 * 
 * 
 * 
 */

package com.ibm.ism.ws.test;

import java.awt.ContainerOrderFocusTraversalPolicy;
import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CheckConnectOptionsAction extends ApiCallAction {
	private final 	String 					_structureID;
	private final	boolean					_isConnected;
	final Long 		_compareSessionExpiryInterval;
	final Boolean 	_hasSessionExpiryInterval;
	final Integer 	_compareReceiveMaximum;
	final Boolean 	_hasReceiveMaximum;
	final Long 		_compareMaximumPacketSize;
	final Boolean 	_hasMaximumPacketSize;
	final Integer 	_compareTopicAliasMaximum;
	final Boolean 	_hasTopicAliasMaximum;
	final String 	_compareAssignedClientIdentifier;
	final String 	_storeAssignedClientID;
	final Boolean 	_hasAssignedClientIdentifier;
	final Boolean 	_compareSessionPresent;
	final Boolean 	_hasSessionPresent;
	final Integer 	_compareMaximumQoS;
	final Boolean 	_hasMaximumQoS;
	final Integer	_compareServerKeepAlive;
	final Boolean 	_hasServerKeepAlive;
	final String 	_compareResponseInformation;
	final Boolean 	_hasResponseInformation;
	final String 	_compareAuthenticationMethod;
	final Boolean 	_hasAuthenticationMethod;
//	final byte[] 	_compareAuthenticationData;
	final String 	_compareReasonString;
	final Boolean 	_hasReasonString;
	final Boolean	_compareIsRetainAvailable;
	final Boolean 	_hasIsRetainAvailable;
	final Boolean 	_compareIsWildcardSubscriptionsAvailable;
	final Boolean 	_hasIsWildcardSubscriptionsAvailable;
	final Boolean 	_compareIsSubscriptionIdentifiersAvailable;
	final Boolean 	_hasIsSubscriptionIdentifiersAvailable;
	final Boolean 	_compareIsSharedSubscriptionsAvailable;
	final Boolean 	_hasIsSharedSubscriptionsAvailable;
	final String	_compareServerReference;
	final Boolean 	_hasServerReference;
	
	

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckConnectOptionsAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("connection_id");
		if (_structureID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		_isConnected = Boolean.parseBoolean(_actionParams.getProperty("isConnected", "true"));
		
		String temp = _apiParameters.getProperty("compareSessionExpiryInterval");
		if(temp == null){
			_compareSessionExpiryInterval = null;
		} else {
			_compareSessionExpiryInterval = new Long(temp);
		}
		
		temp = _apiParameters.getProperty("hasSessionExpiryInterval");
		if(temp == null){
			_hasSessionExpiryInterval = null;
		} else {
			_hasSessionExpiryInterval = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareReceiveMaximum");
		if(temp == null){
			_compareReceiveMaximum = null;
		} else {
			_compareReceiveMaximum = new Integer(temp);
		}
		
		temp = _apiParameters.getProperty("hasReceiveMaximum");
		if(temp == null){
			_hasReceiveMaximum = null;
		} else {
			_hasReceiveMaximum = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareMaximumPacketSize");
		if(temp == null) {
			_compareMaximumPacketSize = null;
		} else {
			_compareMaximumPacketSize = new Long(temp);
		}
		
		temp = _apiParameters.getProperty("hasMaximumPacketSize");
		if(temp == null){
			_hasMaximumPacketSize = null;
		} else {
			_hasMaximumPacketSize = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareTopicAliasMaximum");
		if(temp == null){
			_compareTopicAliasMaximum = null;
		} else {
			_compareTopicAliasMaximum = new Integer(temp);
		}
		
		temp = _apiParameters.getProperty("hasTopicAliasMaximum");
		if(temp == null){
			_hasTopicAliasMaximum = null;
		} else {
			_hasTopicAliasMaximum = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareMaximumQoS");
		if(temp == null){
			_compareMaximumQoS = null;
		} else {
			_compareMaximumQoS = new Integer(temp);
		}
		
		temp = _apiParameters.getProperty("hasMaximumQoS");
		if(temp == null){
			_hasMaximumQoS = null;
		} else {
			_hasMaximumQoS = new Boolean(temp);
		}
		
		_compareAssignedClientIdentifier = _apiParameters.getProperty("compareAssignedClientId");
		
		_storeAssignedClientID = _actionParams.getProperty("storeAssignedClientID");
		
		temp = _apiParameters.getProperty("hasAssignedClientIdentifier");
		if(temp == null){
			_hasAssignedClientIdentifier = null;
		} else {
			_hasAssignedClientIdentifier = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareServerKeepAlive");
		if(temp == null){
			_compareServerKeepAlive = null;
		} else {
			_compareServerKeepAlive = new Integer(temp);
		}
		
		temp = _apiParameters.getProperty("hasServerKeepAlive");
		if(temp == null){
			_hasServerKeepAlive = null;
		} else {
			_hasServerKeepAlive = new Boolean(temp);
		}
		
		_compareResponseInformation = _apiParameters.getProperty("compareResponseInformation");
		
		temp = _apiParameters.getProperty("hasResponseInformation");
		if(temp == null){
			_hasResponseInformation = null;
		} else {
			_hasResponseInformation = new Boolean(temp);
		}
		
		_compareAuthenticationMethod = _apiParameters.getProperty("compareAuthenticationMethod");
		
		temp = _apiParameters.getProperty("hasAuthenticationMethod");
		if(temp == null){
			_hasAuthenticationMethod = null;
		} else {
			_hasAuthenticationMethod = new Boolean(temp);
		}
		
		_compareReasonString = _apiParameters.getProperty("compareReasonString");
		
		temp = _apiParameters.getProperty("hasReasonString");
		if(temp == null){
			_hasReasonString = null;
		} else {
			_hasReasonString = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareIsRetainAvailable");
		if(temp == null){
			_compareIsRetainAvailable = null;
		} else {
			_compareIsRetainAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("hasIsRetainAvailable");
		if(temp == null){
			_hasIsRetainAvailable = null;
		} else {
			_hasIsRetainAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareIsWildcardSubscriptionsAvailable");
		if(temp == null){
			_compareIsWildcardSubscriptionsAvailable = null;
		} else {
			_compareIsWildcardSubscriptionsAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("hasIsWildcardSubscriptionsAvailable");
		if(temp == null){
			_hasIsWildcardSubscriptionsAvailable = null;
		} else {
			_hasIsWildcardSubscriptionsAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareIsSubscriptionIdentifiersAvailable");
		if(temp == null){
			_compareIsSubscriptionIdentifiersAvailable = null;
		} else {
			_compareIsSubscriptionIdentifiersAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("hasIsSubscriptionIdentifiersAvailable");
		if(temp == null){
			_hasIsSubscriptionIdentifiersAvailable = null;
		} else {
			_hasIsSubscriptionIdentifiersAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareIsSharedSubscriptionsAvailable");
		if(temp == null){
			_compareIsSharedSubscriptionsAvailable = null;
		} else {
			_compareIsSharedSubscriptionsAvailable = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("hasIsSharedSubscriptionsAvailable");
		if(temp == null){
			_hasIsSharedSubscriptionsAvailable = null;
		} else {
			_hasIsSharedSubscriptionsAvailable = new Boolean(temp);
		}
		
		_compareServerReference = _apiParameters.getProperty("compareServerReference");
		
		temp = _apiParameters.getProperty("hasServerReference");
		if(temp == null){
			_hasServerReference = null;
		} else {
			_hasServerReference = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("compareSessionPresent");
		if(temp == null){
			_compareSessionPresent = null;
		} else {
			_compareSessionPresent = new Boolean(temp);
		}
		
		temp = _apiParameters.getProperty("hasSessionPresent");
		if(temp == null){
			_hasSessionPresent = null;
		} else {
			_hasSessionPresent = new Boolean(temp);
		}
		
		
		
		
		
	}

	protected boolean invokeApi() throws IsmTestException {
		MyConnection con = (MyConnection)_dataRepository.getVar(_structureID);
		if (null == con) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKCONNECTOPTIONS,
					"Connection "+_structureID+" is not found");
		}
		
		org.eclipse.paho.mqttv5.client.IMqttToken connToken = con.getConnToken();
		System.out.println("connToken: " + connToken);
		if (connToken == null) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKCONNECTOPTIONS,
					"No Connection Token Found");
		}
		
		if (connToken.getResponse() == null) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKCONNECTOPTIONS,
					"No Response from Connection Token");
		}

		if (connToken.getResponse().getProperties() == null) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKCONNECTOPTIONS,
					"No Properties from Connection Token Response");
		}
		
		org.eclipse.paho.mqttv5.common.packet.MqttProperties connProps = connToken.getResponse().getProperties();
		
		System.out.println(connProps);
		System.out.println("maximumQos: " + connProps.getMaximumQoS());
		System.out.println("maximumPacketSize: " + connProps.getMaximumPacketSize());
		System.out.println("assignedClientID: " + connProps.getAssignedClientIdentifier());
		System.out.println("topicAliasMax: " + connProps.getTopicAliasMaximum());
		System.out.println("serverKeepAlive: " + connProps.getServerKeepAlive());
		System.out.println("responseInfo: " + connProps.getResponseInfo());
		System.out.println("sessionExpiryInterval: " + connProps.getSessionExpiryInterval());
		System.out.println("authMethod: " + connProps.getAuthenticationMethod());
		System.out.println("authData: " + connProps.getAuthenticationData());
		System.out.println("reasonString: " + connProps.getReasonString());
		System.out.println("userProperties: " + connProps.getUserProperties());
		System.out.println("isRetainAvailable: " + connProps.isRetainAvailable());
		System.out.println("iswildcardsubavailable: " + connProps.isWildcardSubscriptionsAvailable());
		System.out.println("issubidsavailable: " + connProps.isSubscriptionIdentifiersAvailable());
		System.out.println("isSharedsubsavailable: " + connProps.isSharedSubscriptionAvailable());
		System.out.println("serverReference: " + connProps.getServerReference());
		
		
		if(_compareSessionPresent != null){
			Boolean conSessionPresent = con.getSessionPresent();
			if(!(_compareSessionPresent.equals(conSessionPresent)) || conSessionPresent == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected SessionPresent to be "+ _compareSessionPresent +", actual: "+ conSessionPresent);
			}
		}
		
		if(_hasSessionPresent != null){
			boolean msgHasSessionPresent = false;
			Boolean conHasSessionPresent = con.getSessionPresent();
			
			if(conHasSessionPresent != null){
				msgHasSessionPresent = true;
			}
			
			if(!(_hasSessionPresent.equals(msgHasSessionPresent))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasSessionPresent to be "+ _hasSessionPresent +", actual: "+ msgHasSessionPresent);
			}
		}
		
		if(_compareReceiveMaximum != null){
			Integer msgReceiveMaximum = connProps.getReceiveMaximum();
			if(!(_compareReceiveMaximum.equals(msgReceiveMaximum)) || msgReceiveMaximum == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected ReceiveMaximum to be "+ _compareReceiveMaximum +", actual: "+msgReceiveMaximum);
			}
		}
		
		if(_hasReceiveMaximum != null){
			boolean msgHasReceiveMaximum = false;
			if(connProps.getReceiveMaximum() != null){
				msgHasReceiveMaximum = true;
			}
			if(!(_hasReceiveMaximum.equals(msgHasReceiveMaximum))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasReceiveMaximum to be "+ _hasReceiveMaximum +", actual: "+ msgHasReceiveMaximum);
			}
				
		}
		
		if(_compareMaximumPacketSize != null){
			Long msgMaxPacketSize = connProps.getMaximumPacketSize();
			if(!(_compareMaximumPacketSize.equals(msgMaxPacketSize)) || msgMaxPacketSize == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected MaximumPacketSize to be "+ _compareMaximumPacketSize +", actual: "+msgMaxPacketSize);
			}
		}
		
		if(_hasMaximumPacketSize != null){
			boolean msgHasMaximumPacketSize = false;
			if(connProps.getMaximumPacketSize() != null){
				msgHasMaximumPacketSize = true;
			}
			if(!(_hasMaximumPacketSize.equals(msgHasMaximumPacketSize))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasMaximumPacketSize to be "+ _hasMaximumPacketSize +", actual: "+ msgHasMaximumPacketSize);
			}
		}
		
		if(_compareSessionExpiryInterval != null){
			Long msgSessionExpiryInterval = connProps.getSessionExpiryInterval();

			if(!(_compareSessionExpiryInterval.equals(msgSessionExpiryInterval)) || msgSessionExpiryInterval == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected SessionExpiryInterval to be "+ _compareSessionExpiryInterval +", actual: "+ msgSessionExpiryInterval);
			}
			
		}
		
		if(_hasSessionExpiryInterval != null){
			boolean msgHasSessionExpiryInterval = false;
			if(connProps.getSessionExpiryInterval() != null){
				msgHasSessionExpiryInterval = true;				
			}
			if(!(_hasSessionExpiryInterval.equals(msgHasSessionExpiryInterval))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasSessionExpiryInterval to be "+ _hasSessionExpiryInterval +", actual: "+ msgHasSessionExpiryInterval);
			}
		}
		
		if(_compareTopicAliasMaximum != null){
			Integer msgTopicAliasMax = connProps.getTopicAliasMaximum();

			if(!(_compareTopicAliasMaximum.equals(msgTopicAliasMax)) || msgTopicAliasMax == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected TopicAliasMaximum to be "+ _compareTopicAliasMaximum +", actual: "+ msgTopicAliasMax);
			}

		}
		
		if(_hasTopicAliasMaximum != null){
			boolean msgHasTopicAliasMaximum = false;
			if(connProps.getTopicAliasMaximum() != null){
				msgHasTopicAliasMaximum = true;				
			}
			if(!(_hasTopicAliasMaximum.equals(msgHasTopicAliasMaximum))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasTopicAliasMaximum to be "+ _hasTopicAliasMaximum +", actual: "+ msgHasTopicAliasMaximum);
			}
		}
		
		if(_compareAssignedClientIdentifier != null){
			String msgAssignedClientID = connProps.getAssignedClientIdentifier();
			if(msgAssignedClientID != null){
				if(!(_compareAssignedClientIdentifier.equals(msgAssignedClientID)) || msgAssignedClientID == null){
					throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
							, "Expected AssignedClientIdentifier to be "+ _compareAssignedClientIdentifier +", actual: "+ msgAssignedClientID);
				}
			}
		}
		
		if(_storeAssignedClientID != null){
			String msgAssignedClientID = connProps.getAssignedClientIdentifier();
			if(msgAssignedClientID != null){
				_dataRepository.storeVar(_storeAssignedClientID, msgAssignedClientID);
			}
		}
		
		
		if(_hasAssignedClientIdentifier != null){
			boolean msgHasAssignedClientID = false;
			if(connProps.getAssignedClientIdentifier() != null){
				msgHasAssignedClientID = true;				
			}
			if(!(_hasAssignedClientIdentifier.equals(msgHasAssignedClientID))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasAssignedClientID to be "+ _hasAssignedClientIdentifier +", actual: "+ msgHasAssignedClientID);
			}
		}
		
		
		if(_compareMaximumQoS != null){
			Integer msgMaxQoS = connProps.getMaximumQoS();
			if(msgMaxQoS != null){
				if(!(_compareMaximumQoS.equals(msgMaxQoS)) || msgMaxQoS == null){
					throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
							, "Expected MaximumQoS to be "+ _compareMaximumQoS +", actual: "+ msgMaxQoS);
				}
			}
		}
		
		if(_hasMaximumQoS != null){
			boolean msgHasMaxQoS = false;
			if(connProps.getMaximumQoS() != null){
				msgHasMaxQoS = true;				
			}
			if(!(_hasMaximumQoS.equals(msgHasMaxQoS))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasMaximumQoS to be "+ _hasMaximumQoS +", actual: "+ msgHasMaxQoS);
			}
		}
		
		if(_compareServerKeepAlive != null){
			Integer msgServerKeepAlive = connProps.getServerKeepAlive();
			if(msgServerKeepAlive != null){
				if(!(_compareServerKeepAlive.equals(msgServerKeepAlive))){
					throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
							, "Expected ServerKeepAlive to be "+ _compareServerKeepAlive +", actual: "+ msgServerKeepAlive);
				}
			}
		}
		
		if(_hasServerKeepAlive != null){
			boolean msgHasServerKeepAlive = false;
			if(connProps.getServerKeepAlive() != null){
				msgHasServerKeepAlive = true;				
			}
			if(!(_hasServerKeepAlive.equals(msgHasServerKeepAlive))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasServerKeepAlive to be "+ _hasServerKeepAlive +", actual: "+ msgHasServerKeepAlive);
			}
		}
		
		if(_compareResponseInformation != null){
			String msgResponseInformation = connProps.getResponseInfo();

			if(!(_compareResponseInformation.equals(msgResponseInformation)) || msgResponseInformation == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected ResponseInformation to be "+ _compareResponseInformation +", actual: "+ msgResponseInformation);
			}

		}
		
		if(_hasResponseInformation != null){
			boolean msgHasResponseInformation = false;
			if(connProps.getResponseInfo() != null){
				msgHasResponseInformation = true;				
			}
			if(!(_hasResponseInformation.equals(msgHasResponseInformation))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasResponseInformation to be "+ _hasResponseInformation +", actual: "+ msgHasResponseInformation);
			}
		}
		
		if(_compareAuthenticationMethod != null){
			String msgAuthenticationMethod = connProps.getAuthenticationMethod();

			if(!(_compareAuthenticationMethod.equals(msgAuthenticationMethod)) || msgAuthenticationMethod == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected AuthenticationMethod to be "+ _compareAuthenticationMethod +", actual: "+ msgAuthenticationMethod);
			}

		}
		
		if(_hasAuthenticationMethod != null){
			boolean msgHasAuthenticationMethod = false;
			if(connProps.getAuthenticationMethod() != null){
				msgHasAuthenticationMethod = true;				
			}
			if(!(_hasAuthenticationMethod.equals(msgHasAuthenticationMethod))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasAuthenticationMethod to be "+ _hasAuthenticationMethod +", actual: "+ msgHasAuthenticationMethod);
			}
		}
		
		if(_compareReasonString != null){
			String msgReasonString = connProps.getReasonString();
	
			if(!(_compareReasonString.equals(msgReasonString)) || msgReasonString == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected ReasonString to be "+ _compareReasonString +", actual: "+ msgReasonString);
			}

		}
		
		if(_hasReasonString != null){
			boolean msgHasReasonString = false;
			if(connProps.getReasonString() != null){
				msgHasReasonString = true;				
			}
			if(!(_hasReasonString.equals(msgHasReasonString))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasReasonString to be "+ _hasReasonString +", actual: "+ msgHasReasonString);
			}
		}
		
		if(_compareIsRetainAvailable != null){
			Boolean msgIsRetainAvailable = connProps.isRetainAvailable();

			if(!(_compareIsRetainAvailable.equals(msgIsRetainAvailable)) || msgIsRetainAvailable == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected IsRetainAvailable to be "+ _compareIsRetainAvailable +", actual: "+ msgIsRetainAvailable);
			}

		}
		
		if(_hasIsRetainAvailable != null){
			boolean msgHasIsRetainAvailable = false;
			Boolean msgIsRetainAvailable = connProps.isRetainAvailable();
			
			if(msgIsRetainAvailable != null){
				msgHasIsRetainAvailable = true;
			}
			
			if(!(_hasIsRetainAvailable.equals(msgHasIsRetainAvailable))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasIsRetainAvailable to be "+ _hasIsRetainAvailable +", actual: "+ msgHasIsRetainAvailable);
			}
		}
		
		if(_compareIsWildcardSubscriptionsAvailable != null){
			Boolean msgIsWildcardSubscriptionsAvailable = connProps.isWildcardSubscriptionsAvailable();

			if(!(_compareIsWildcardSubscriptionsAvailable.equals(msgIsWildcardSubscriptionsAvailable)) || msgIsWildcardSubscriptionsAvailable == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected WildcardSubscriptionsAvailable to be "+ _compareIsWildcardSubscriptionsAvailable +", actual: "+ msgIsWildcardSubscriptionsAvailable);
			}

		}
		
		if(_hasIsWildcardSubscriptionsAvailable != null){
			boolean msgHasIsWildcardSubsAvailable = false;
			Boolean msgIsWildcardSubsAvailable = connProps.isWildcardSubscriptionsAvailable();
			
			if(msgIsWildcardSubsAvailable != null){
				msgHasIsWildcardSubsAvailable = true;
			}
			
			if(!(_hasIsWildcardSubscriptionsAvailable.equals(msgHasIsWildcardSubsAvailable))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasIsWildcardSubsAvailable to be "+ _hasIsWildcardSubscriptionsAvailable +", actual: "+ msgHasIsWildcardSubsAvailable);
			}
		}
		
		if(_compareIsSubscriptionIdentifiersAvailable != null){
			Boolean msgIsSubscriptionIdentifiersAvailable = connProps.isSubscriptionIdentifiersAvailable();

			if(!(_compareIsSubscriptionIdentifiersAvailable.equals(msgIsSubscriptionIdentifiersAvailable)) || msgIsSubscriptionIdentifiersAvailable == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected SubscriptionIdsAvailable to be "+ _compareIsSubscriptionIdentifiersAvailable +", actual: "+ msgIsSubscriptionIdentifiersAvailable);
			}

		}
		
		if(_hasIsSubscriptionIdentifiersAvailable != null){
			boolean msgHasIsSubscriptionIdsAvailable = false;
			Boolean msgIsSubscriptionIdsAvailable = connProps.isSubscriptionIdentifiersAvailable();
			
			if(msgIsSubscriptionIdsAvailable != null){
				msgHasIsSubscriptionIdsAvailable = true;
			}
			
			if(!(_hasIsSubscriptionIdentifiersAvailable.equals(msgHasIsSubscriptionIdsAvailable))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasIsSubscriptionIdentifiersAvailable to be "+ _hasIsSubscriptionIdentifiersAvailable +", actual: "+ msgHasIsSubscriptionIdsAvailable);
			}
		}
		
		if(_compareIsSharedSubscriptionsAvailable != null){
			Boolean msgIsSharedSubsAvailable = connProps.isSharedSubscriptionAvailable();

			if(!(_compareIsSharedSubscriptionsAvailable.equals(msgIsSharedSubsAvailable)) || msgIsSharedSubsAvailable == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected SharedSubscriptionsAvailable to be "+ _compareIsSharedSubscriptionsAvailable +", actual: "+ msgIsSharedSubsAvailable);
			}

		}
		
		if(_hasIsSharedSubscriptionsAvailable != null){
			boolean msgHasIsSharedSubsAvailable = false;
			Boolean msgIsSharedSubsAvailable = connProps.isSharedSubscriptionAvailable();
			
			if(msgIsSharedSubsAvailable != null){
				msgHasIsSharedSubsAvailable = true;
			}
			
			if(!(_hasIsSharedSubscriptionsAvailable.equals(msgHasIsSharedSubsAvailable))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasIsSHaredSubscriptionsAvailable to be "+ _hasIsSharedSubscriptionsAvailable +", actual: "+ msgHasIsSharedSubsAvailable);
			}
		}
		
		if(_compareServerReference != null){
			String msgServerReference = connProps.getServerReference();

			if(!(_compareServerReference.equals(msgServerReference)) || msgServerReference == null){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected ServerReference to be "+ _compareServerReference +", actual: "+ msgServerReference);
			}

		}
		
		if(_hasServerReference != null){
			boolean msgHasServerReference = false;
			if(connProps.getServerReference() != null){
				msgHasServerReference = true;				
			}
			if(!(_hasServerReference.equals(msgHasServerReference))){
				throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+2)
						, "Expected HasServerReference to be "+ _hasServerReference +", actual: "+ msgHasServerReference);
			}
		}
		
		
		
		//------------------------

		
		
		
		if (_isConnected != con.isConnected()) {
			throw new IsmTestException("ISMTEST"+(Constants.CHECKCONNECTOPTIONS+1)
					, "Expected connection.isConnected to be "+_isConnected+" but is "+con.isConnected());
		}
		
		return true;
	}
	

}
