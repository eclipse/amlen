/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.plugin;

/**
 * The ImaSubscription object is created using the newSubscription() method of ImaConnection.
 * The configuration of the subscription is read only, and the only actions which can be taken 
 * on the subscription are subscribe() and close().  A subscription object is tied to a 
 * connection.   
 */
public interface ImaSubscription {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Make the subscription.
     * 
     * Some time after making this call, the onComplete() callback of the connection listener will
     * be called with this subscription as its object.
     * 
     * The properties of the subscription are set when the object is created.
     * <p>
     * After the subscribe(), another subscribe cannot be done until after a close() is done.
     */
    public void subscribe();
    
    /**
     * Make the subscription without waiting for an ACK.
     * 
     * <p>
     * After the subscribe(), another subscribe cannot be done until after a close() is done.
     * @throws ImaPluginException if an error occurs while subscribing
     */
    public void subscribeNoAck();
    
    /**
     * Close a subscription.
     * 
     * A subscription is automatically closed when the connection is closed, but can also be closed explicitly. If the
     * subscription type is non-durable, it will be destroyed. After a
     * subscription is closed, another subscribe() can be done.
     * 
     * @throws ImaPluginException if an error occurs while closing a subscription.
     */
    public void close();
    
    /**
     * Returns the connection of the subscription
     * @return the connection object 
     */
    public ImaConnection getConnection();
    
    /**
     * Returns the destination of the subscription
     * @return the destination name
     */
    public String getDestination();
    
    /**
     * Returns the destination type.
     * @return The destination type as topic or queue.
     */
    public ImaDestinationType getDestinationType();

    /**
     * Returns the name of the subscription.
     * @return
     */
    public String getName();
    
    /**
     * Returns the nolocal setting of the subscription.
     * The nolocal setting is ignored for shared subscriptions.
     * @return true if nolocal is selected, false otherwise.
     */
    public boolean getNoLocal();
    
    /**
     * Returns the reliability setting of the subscription
     * @return the reliability setting
     */
    public ImaReliability getReliability();
    
    /**
     * Sets the reliability setting of the subscription
     */
    public void setReliability(ImaReliability qos);
    
    /**
     * Returns the subscription type.
     * @return the subscription type
     */
    public ImaSubscriptionType getSubscriptionType();

//  /**
//   * Returns the selector.
//   * The selector string is based on the JMS selector syntax which is derived from SQL92.
//   * Some restrictions imposed by JMS are not imposed for this selector.
//   * @return The selector string, or null to indicate there is no selector.
//   */
//  public String getSelector();
    
    /**
     * Returns whether the subscription is currently active.
     * @return true if the subscription is active and false otherwise.
     */
    public boolean isSubscribed();

    /**
     * Returns the user data
     * @return user data
     */
	public Object getUserData() ;
	
	/**
     * Sets the user data
     */
	public void setUserData(Object userData) ;
}
