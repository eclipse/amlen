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
void ism_engine_threadInit(uint8_t isStoreCrit) {}
void ism_engine_threadTerm(void) {}
int ism_cluster_setHaStatus(int haStatus) {return 0;}
int ism_cluster_getStatistics(void *p) {return 0;}
int ism_transport_getNumActiveConns(void){return 0;}
