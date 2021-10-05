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

/*
 * Note: this file provides global definitions for MessageSight Event MIBs,
 *       as well as their initialization interface. 
 */

#ifndef _IMASNMPEVENTMIB_H_
#define _IMASNMPEVENTMIB_H_

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int 	ima_snmp_init_event_mibs();
       void ima_snmp_reinit_event_mibs();

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPEVENTMIB_H_ */

