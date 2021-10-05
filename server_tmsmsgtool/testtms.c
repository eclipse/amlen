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
  LOG(INFO, UTIL, Utilities, 9001, "", "TEST LOG ENTRY FROM UTIL");
  LOG(INFO, UTIL, Utilities, 9002, "", "TEST LOG ENTRY FROM UTIL");
   LOG(INFO, PROTOCOL, PROTOCOL, 2000, "", "TEST LOG ENTRY FROM UTIL");
     LOG(INFO, PROTOCOL, PROTOCOL, 2999, "", "TEST LOG ENTRY FROM UTIL");
           LOG(NOTICE, ENGINE, ENGINE, 3000, "", "TEST LOG ENTRY FROM UTIL");
     LOG(NOTICE, ENGINE, ENGINE, 3999, "", "TEST LOG ENTRY FROM UTIL");
         LOG(CRIT, STORE, STORE, 4000, "", "TEST LOG ENTRY FROM UTIL");
     LOG(CRIT, STORE, STORE, 4999, "", "TEST LOG ENTRY FROM UTIL");
       LOG(EMER, STORE, STORE, 5000, "", "TEST LOG ENTRY FROM UTIL");
     LOG(EMER, ADMIN, ADMIN, 5999, "", "TEST LOG ENTRY FROM UTIL");
  
