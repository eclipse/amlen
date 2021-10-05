/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.proxy;

/**
 * The ImaAuthConnection interface defines the surrogate for the connection used
 * during authentication.
 */
public interface ImaAuthConnection {
	public enum CrlState {
		NONE(0), 
		OK(1), 
		EXPIRED(2), 
		FUTURE(3),
		INVALID(4),
		UNAVAILABLE(5);
		private final int state;
		private CrlState(int i) {
			state = i;
		}
		static public CrlState valueOf(int state) {
			switch (state) {
			case 0:
				return CrlState.NONE;
			case 1:
				return CrlState.OK;
			case 2:
				return CrlState.EXPIRED;
			case 3:
				return CrlState.FUTURE;
			case 4:
				return CrlState.INVALID;
			case 5:
				return CrlState.UNAVAILABLE;
			default:
				throw new RuntimeException("Unknown CRL state:" + state);
			}
		}
		public int intValue() {
			return state;
		}
	}
	/*Authentication Only Action*/
	public static final int ACTION_AUTHENTICATION=0;
	
	/*Authorization Only Action*/
	public static final int ACTION_AUTHORIZATION=1;
	
	/*Device Authorization and Auto Create.*/
	public static final int ACTION_AUTHORIZATIONAUTOCREATE=2;
	
	/* Device Auto Create Only */
	public static final int ACTION_AUTOCREATEONLY=3;
	
	/*AUTHORIZED*/
	public static final int AUTHORIZED=0;
	
	/*NOT AUTHORIZED result*/
	public static final int NOT_AUTHORIZED=1;

    /*
     * Gets the client ID of the connection.
     * @return the client ID
     */
    public String getClientID();
    
    /**
     * Gets the user ID of the connection.
     * @return the user ID.  If the user ID is not set, return a zero length string.
     */
    public String getUserID();
    
    /**
     * Gets the user ID of the connection as specified by the protocol.
     * @return the protocol user ID.  If the client does not supply a userID this will be null.
     */
    public String getProtocolUserID();    
    
    /**
     * Gets the password of the connection.
     * @return the password.  If the password is not set, return a zero length string.
     */
    public String getPassword();
    
    /**
     * Gets the certificate name the connection.
     * @return the certificate common name.  If there is not certificate or the certificate does not have a name this will be null.
     */
    public String getCertificateName();

    /**
     * Gets the connection certificate issuer.
     * @return the certificate issuer common name if exists.  null otherwise.
     */
    public String getCertificateIssuer();
    
    /**
     * Gets the connection secure status.
     * @return true if connection is over TLS protocol.
     */
    public boolean isSecure();
    
    /**
     * Gets whether the ClientID matches the certificate names.
     * @return true if the CommonName or SubjectAltName of the client cert matches the clientID.
     */
    public boolean isNameMatch();
    
    /**
     * Gets whether the ClientID exactly matches the certificate names
     * @return true if the CommonName or SubjectAltName exactly matches the ClientID (both type and id)
     */
    public boolean isExactNameMatch();
    
    /**
     * Gets the connection certificate CRL state.
     * @return connection certificate CRL state.
     */
    public CrlState getCrlState();
    
    /**
     * Gets the client IP address of the connection.
     * @return the client IP address.  If this is IPv6 is will be within square brackets.
     */
    public String getClientAddress();
    
    
    
    /**
     * Get Device ID
     * @return device ID
     */
    public String getDeviceId();
    
    /**
     * Get DeviceType
     * @return device type
     */
    public String getDeviceType();
    
    
    /**
     * Get auth token bytes
     * @return token
     */
    public byte[] getAuthToken();
    
    /**
     * Get auth token length
     * @return token length
     */
    public int getAuthTokenLenghth();
    
    /**
     * Get action 
     * @return action
     */
    public int getAction();
    
    public int getPermissions();
    
    /**
     * Get Aaa Enabled value
     * @return true if AAA service is enabled. False otherwise
     */
    public boolean getAaaEnabled();
    
    /**
     * Gets the Risk Management Policy Bits
     *  ISM_TENANT_RMPOLICY_REQSECURE	0x0001
     *  ISM_TENANT_RMPOLICY_REQCERT      0x0002
     *  ISM_TENANT_RMPOLICY_REQUSER      0x0004
     *  ISM_TENANT_RMPOLICY_REQSNI       0x0008
     * @return policy bits
     * @see com.ibm.ima.proxy.ImaAuthConnection#getCertificateName()
     */
    public int getRMPolicies();
    
    /**
     * Authenticate the connection.
     * <p>
     * A connection must be authenticated once and only once.  The authenticator must keep
     * this object and invoke the authenticate method to indicate either success or failure
     * of the authentication. 
     * 
     * @param good  true if the connection is authenticated, false otherwise
     * @throws IllegalStateException if authenticate is called more than once
     */
    public void authenticate(boolean good);
    
    

    /**
     * Authenticate the connection with return code. 
     * <p>
     * A connection must be authenticated once and only once.  The authenticator must keep
     * this object and invoke the authenticate method to indicate either success or failure
     * of the authentication. 
     * 
     * @param good  true if the connection is authenticated, false otherwise
     * @param rc the return code
     * @throws IllegalStateException if authenticate is called more than once
     */
    public void authenticate(boolean good, int rc);
    
    
    /**
     * Authorize the connection with rc and result.
     *
     * @param rc  the return code
     * @param result the result value 
     */
    public void authorize(int rc, String result);
    
    /**
     * Authorize the connection with return code.
     *
     * @param rc  the return code
     */
    public void authorize(int rc);
    
    /**
     * Get SecGuardian Enabled property
     * @return True for SecGuardian is Enabled. Otherwise, it is false
     **/
    public boolean getSecGuardianEnabled();
    
    /**
     * Get RLAC App Enabled property
     * @return true for RLAC for application is enabled. Otherwise, it is false.
     */
    public boolean getRLACAppEnabled();
    
    /**
     * Get RLAC for Applicaiton Default Group Use
     * @return True if Use Default Group. Otherwise, it is false.
     */
    public boolean getRLACAppDefaultGroup();
    
    /**
     * Get Client port property
     * @return client port
     */
    public int getClientPort();
    
    /**
     * Get Connection Id property
     * @return connection Id
     */
    public long getConnectionId();

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";
    
    
}
