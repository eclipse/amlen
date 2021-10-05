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

package com.ibm.ima.plugin.impl;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaReliability;
import com.ibm.ima.plugin.ImaSubscription;
import com.ibm.ima.plugin.ImaSubscriptionType;

public class ImaSubscriptionImpl implements ImaSubscription {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private ImaDestinationType   desttype;
    private String   dest;
    private String   name;
    private ImaSubscriptionType share;
    private ImaReliability qos = ImaReliability.qos2;
    private boolean  nolocal;
    boolean  isSubscribed;
    private ImaConnectionImpl connect;
    private Object	 userData;
    final boolean               transacted;
    
    /*
     * Constructor
     */
    ImaSubscriptionImpl(ImaDestinationType desttype, String dest, String name, ImaSubscriptionType subType,
            boolean nolocal, boolean transacted, ImaConnectionImpl connect) {
        this.desttype = desttype;
        this.dest     = dest;
        this.name     = name;
        this.share    = subType;
        this.nolocal  = nolocal;
        this.connect  = connect;
        this.transacted = transacted;
        isSubscribed   = false;
        if (this.share == null) 
            this.share = ImaSubscriptionType.NonDurable;
        /*QoS is defaulted to 2 above*/
        /*if (this.qos == null) 
            this.qos = ImaReliability.qos0;*/
    }

    /*
     * Map from share bits to share enum
     */
    static ImaSubscriptionType toSubscriptionType(int share) {
        switch (share) {
        case 0:   return ImaSubscriptionType.NonDurable;
        case 1:   return ImaSubscriptionType.Durable;
 //     case 2:   return ImaSubscriptionType.SharedNonDurable;
 //     case 3:   return ImaSubscriptionType.SharedDurable;
 //     case 6:   return ImaSubscriptionType.GlobalNonDurable;
 //     case 7:   return ImaSubscriptionType.GlobalDurable;
        }
        return ImaSubscriptionType.NonDurable;
    }
    
    /*
     * Map from share enum to share bits
     */
    static int fromSubscriptionType(ImaSubscriptionType share) {
        switch (share) {
        case NonDurable:            return 0;
        case Durable:               return 1;
 //     case SharedNonDurable:      return 2;
 //     case SharedDurable:         return 3;
 //     case GlobalNonDurable:      return 6;
 //     case GlobalDurable:         return 7;
        }
        return 0;                /* Should not happen */
    }

    
    /*
     * @see com.ibm.ima.ext.ImaSubscription#subscribe()
     */
    public void subscribe() {
        if (isSubscribed)
            throw new IllegalStateException("The subscription is already subscribed");
        if (connect.state != ImaConnection.State_Open)
            throw new IllegalStateException("The connection is not open.");
        connect.invokeSubscription(this, true);
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#subscribeNoAck()
     */
    public void subscribeNoAck() {
        if (isSubscribed)
            throw new IllegalStateException("The subscription is already subscribed");
        if (connect.state != ImaConnection.State_Open)
            throw new IllegalStateException("The connection is not open. state=" + connect.state);
        connect.invokeSubscription(this, false);
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#close()
     */
    public void close() {
        if (connect.state == ImaConnection.State_Closed)
            isSubscribed = false;
        if (isSubscribed) {
            connect.closeSubscription(this);
        }    
    }

    public ImaConnection getConnection() {
        return connect;
    }
    
    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getDestination()
     */
    public String getDestination() {
        return dest;
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getName()
     */
    public String getName() {
        return name;
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getNoLocal()
     */
    public boolean getNoLocal() {
        return nolocal;
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getDestinationType()
     */
    public ImaDestinationType getDestinationType() {
        return desttype;
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getShareType()
     */
    public ImaSubscriptionType getSubscriptionType() {
        return share;
    }

    int getShareType() {
        return fromSubscriptionType(share);
    }
    /*
     * @see com.ibm.ima.plugin.ImaSubscription#setReliability()
     */
    public void setReliability(ImaReliability qos) {
        if (transacted && (qos == ImaReliability.qos0))
            throw new RuntimeException("qos0 reliability is not allowed for transacted subscription");
        this.qos = qos;    
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#getReliability()
     */
    public ImaReliability getReliability() {
        return qos;    
    }
    
//  /*
//   * @see com.ibm.ima.plugin.ImaSubscription#getSelector()
//   */
//  public String getSelector() {
//      return selector;
//  }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#isSubscribed()
     */
    public boolean isSubscribed() {
        return isSubscribed;
    }

    /*
     * @see com.ibm.ima.plugin.ImaSubscription#setUserData()
     */
	public Object getUserData() {
		return userData;
	}
	/*
     * @see com.ibm.ima.plugin.ImaSubscription#setUserData()
     */
	public void setUserData(Object userData) {
		this.userData = userData;
	}

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuffer sb = new StringBuffer(512);
        sb.append("ImaSubscription={name=").append(name).append(" dest=").append(dest).append(" type=")
                .append(desttype).append(" qos=").append(qos).append(" nolocal=").append(nolocal)
                .append(" isSubscribed=").append(isSubscribed).append(" connect=").append(String.valueOf(connect));
        if (userData != null)
            sb.append(" userData=").append(userData);
        sb.append('}');
        return sb.toString();
    }

}
