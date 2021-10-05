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

#ifndef STORE_UNIT_TEST_H_
#define STORE_UNIT_TEST_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define SET_NONSHM_MEM_TYPE() { \
   f.type = VT_UByte; \
   f.val.i = test_params.mem_type; \
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MEMTYPE, &f); \
}

#define SET_NONSHM_MEM_TYPE_PLUS_PERSIST() { \
   f.type = VT_UByte; \
   f.val.i = test_params.mem_type; \
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_MEMTYPE, &f); \
   f.type = VT_Boolean; \
   f.val.i = test_params.enable_persist; \
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ENABLEPERSIST, &f); \
   f.type = VT_String; \
   f.val.s = test_params.dir_name; \
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_ROOT_PATH, &f); \
   ism_common_setProperty(ism_common_getConfigProperties(), ismSTORE_CFG_DISK_PERSIST_PATH, &f); \
}

typedef struct
{
   char dir_name[PATH_MAX + 1];                /* Directory of data files        */
   char large_filename[FILENAME_MAX + 1];      /* Large data file name           */
   char small_filename[FILENAME_MAX + 1];      /* Small data file name           */
   char output_filename[FILENAME_MAX + 1];     /* Output file name               */
   char admin_filename[FILENAME_MAX + 1];      /* Admin file name                */
   char remote_disc_nic[128];                  /* Remote discovery NIC           */
   char local_disc_nic[128];                   /* Local discovery NIC            */
   char local_rep_nic[128];                    /* Local replication NIC          */
   char ha_startup[32];                        /* High-Availability startup mode */
   int  store_memsize_mb;                      /* Store memory size MB           */
   int  mem_type;                              /* Store memory type              */
   int  oid_interval;                          /* MinActiveOrderId Interval      */
   int  ha_port;                               /* High-Availability port         */
   int  ha_protocol;                           /* High-Availability protocol     */
   int  enable_persist;                        /* Enable disk persistence        */
} testParameters_t;

void testInitialization(void);
void testCreateMsgs1Thread(void);
void testCreateMsgs4Threads(void);
void testCreateStates(void);
void testDeleteStates(void);
void testStatistics(void);
void testRefsStatistics(void);
void testPruneReferencesPerf(void);
void testReservation(void);
void testAlerts(void);
void testOwnerLimit(void);
void testCompactGeneration(void);

void testCreateMsgs1ThreadHA(void);
void testReadMsgs1ThreadHA(void);
void testCreateMsgs4ThreadsHA(void);
void testCreateStatesHA(void);
void testSendAdminMsgsHA(void);
void testOrphanChunksHA(void);
void testCompactGenerationHA(void);

/**
 * Test array for store tests.
 */
extern CU_TestInfo ISM_store_tests[];
extern CU_TestInfo ISM_store_perf_tests[];
extern CU_TestInfo ISM_storeHA_tests[];

#endif /* STORE_UNIT_TEST_H_ */
