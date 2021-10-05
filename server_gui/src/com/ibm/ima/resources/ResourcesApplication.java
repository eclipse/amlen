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
package com.ibm.ima.resources;

import com.ibm.ima.ImaRequestFilter;

import java.util.HashSet;
import java.util.Set;

import javax.ws.rs.core.Application;
import javax.ws.rs.ApplicationPath;

import com.ibm.ima.resources.exceptions.GenericExceptionMapper;
import com.ibm.ima.resources.exceptions.WebApplicationExceptionMapper;
import com.ibm.ima.resources.security.CertificateProfilesResource;
import com.ibm.ima.resources.security.FIPSResource;
import com.ibm.ima.resources.security.LibertyCertificateResource;
import com.ibm.ima.resources.security.SSLKeyRepositoryResource;
import com.ibm.ima.resources.security.SecurityProfilesResource;
import com.ibm.ima.util.IsmLogger;

import com.fasterxml.jackson.jaxrs.json.JacksonJaxbJsonProvider;


@ApplicationPath("/rest")
public class ResourcesApplication extends Application {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = ResourcesApplication.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    /**
     * JAX-RS uses this method to obtain details about all the Resource classes that are supported by this application.
     * 
     * A new instance of these classes will be created for each request in which they are used.
     */
    @Override
    public Set<Class<?>> getClasses() {
        logger.traceEntry(CLAS, "getClasses");

        Set<Class<?>> classes = new HashSet<Class<?>>();

        /* add each ReST resource to the list below */
        classes.add(LicenseResource.class);
        classes.add(EndpointResource.class);
        classes.add(UserResource.class);
        classes.add(CertificateProfilesResource.class);
        classes.add(LibertyCertificateResource.class);
        classes.add(SecurityProfilesResource.class);
        classes.add(FIPSResource.class);
        classes.add(MonitoringResource.class);
        classes.add(ConnectionPolicyResource.class);
        classes.add(MessagingPolicyResource.class);
        classes.add(MessageHubResource.class);
        classes.add(PreferencesResource.class);
        classes.add(ApplianceResource.class);
        classes.add(QueueManagerConnectionResource.class);
        classes.add(QueueAdministrationResource.class);
        classes.add(DestinationMappingRuleResource.class);
        classes.add(LogLevelResource.class);
        classes.add(LDAPResource.class);
        classes.add(HAPairResource.class);
        classes.add(HARoleResource.class);
        classes.add(TopicMonitorResource.class);
        classes.add(MQTTClientResource.class);
        classes.add(SSLKeyRepositoryResource.class);
        classes.add(SubscriptionResource.class);
        classes.add(TransactionResource.class);
        classes.add(LoginResource.class);
        classes.add(ProtocolResource.class);
        classes.add(NetworkResource.class);
        classes.add(ClusterMembershipResource.class);
        
        /* add each exception mapper to the list below */
        classes.add(WebApplicationExceptionMapper.class);
        classes.add(GenericExceptionMapper.class);

        //Add the version of Jackson that we bundle
        //per https://openliberty.io/blog/2020/11/11/byo-jackson.html
        classes.add(JacksonJaxbJsonProvider.class);

        return classes;
    }
    
    @Override
    public Set<Object> getSingletons()
    {
        Set<Object> singletons = new HashSet<>();
        singletons.add(new ImaRequestFilter());
        
        return singletons;
    }
}
