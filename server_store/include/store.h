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

/*********************************************************************/
/*                                                                   */
/* Module Name: store.h                                              */
/*                                                                   */
/* Description: Store component external header file                 */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_DEFINED
#define __ISM_STORE_DEFINED

#include <ismutil.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************/
/* Store External Configuration Parameters                           */
/*********************************************************************/

/* Store.ColdStart                                                             *
 * Indicates if the Store should perform a cold start, i.e., initialize a      *
 * clean Store without recovering data, or perform recovery and initialize the *
 * Store with the data that has been stored in the previous run.               *
 *                                                                             *
 * A value of 2 indicates cold start.                                          *
 * A value of 1 indicates cold start with HA disabled (used for clean store).  *
 * A value of 0 indicates recovery.                                            *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_COLD_START            "Store.ColdStart"

/* Store.BackupToDisk                                                          *
 * Indicates if the Store should write all its content (including the          *
 * Management Generation) to disk as part of the Store (graceful) termination  *
 * procedure.                                                                  *
 *                                                                             *
 * A value of 1 indicates complete backup to disk should be perform.           *
 * A value of 0 indicates complete backup to disk should not be perform.       *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_BACKUP_DISK           "Store.BackupToDisk"

/* Store.RestoreFromDisk                                                       *
 * Indicates if the Store should recover all its data from disk or use the     *
 * combination of NVRAM/SHM memory and disk to recover.                        *
 * Complete restore from disk is possible only if the Store was previously     *
 * terminated with Store.BackupToDisk enabled.                                 *
 *                                                                             *
 * A value of 1 indicates recover everything from disk.                        *
 * A value of 0 indicates recovery from memory.                                *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_RESTORE_DISK          "Store.RestoreFromDisk"

/* Store.GranuleSizeBytes                                                      *
 * Defines the size in bytes of a block size in a regular Generation (not the  *
 * Management).                                                                *
 * Store.GranuleSizeBytes can be modified during run time. If changed, the     *
 * change will affect the next new Generation.                                 *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1024.                                                  */
#define ismSTORE_CFG_GRANULE_SIZE          "Store.GranuleSizeBytes"

/* Store.MgmtMemPercent                                                        *
 * The size of the Management Generation as a percent of the total Store       *
 * memory (which is set by Store.TotalMemSizeMB).                              *
 * Store.MgmtMemPercent can be modified only during start up and only when a   *
 * cold start up is performed.                                                 *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 50.                                                    */
#define ismSTORE_CFG_MGMT_MEM_PCT          "Store.MgmtMemPercent"

/*********************************************************************/
/* Store Development Configuration Parameters                        */
/*********************************************************************/

/* Store.MemoryType                                                            *
 * Define the Store memory type.                                               *
 *                                                                             *
 * A value of ismSTORE_MEMTYPE_SHM indicates Shared-Memory.                    *
 * A value of ismSTORE_MEMTYPE_NVRAM indicates NVRAM.                          *
 * A value of ismSTORE_MEMTYPE_MEM indicates standard memory.                  *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is ismSTORE_MEMTYPE_SHM.                                  */
#define ismSTORE_CFG_MEMTYPE               "Store.MemoryType"

#define ismSTORE_MEMTYPE_SHM               0x0
#define ismSTORE_MEMTYPE_NVRAM             0x1
#define ismSTORE_MEMTYPE_MEM               0x2

/* Store.TotalMemSizeMB                                                        *
 * Defines the total amount of memory available for the Store in Mega Bytes.   *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1024 (1GB).                                            */
#define ismSTORE_CFG_TOTAL_MEMSIZE_MB      "Store.TotalMemSizeMB"


/* Store.TotalMemSizeBytes                                                     *
 * Defines the total amount of memory available for the Store in Bytes.        *
 * Store.TotalMemSizeBytes should be a multiplication of 1MB (1048576 bytes).  *
 *                                                                             *
 * The type of the parameter is uint64_t.                                      *
 * The default value is 0. meaning: "use Store.TotalMemSizeMB"                */
#define ismSTORE_CFG_TOTAL_MEMSIZE_BYTES   "Store.TotalMemSizeBytes"

/* Store.RecoveryMemSizeMB                                                     *
 * The amount in Megabytes of regular memory (not NVRAM) that the Store is     *
 * allowed to consume during recovery. This memory is used only during the     *
 * recovery phase to load Generations from disks.                              *
 * It is recommended to define a size that can hold at least two Generations.  *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 4096 (4GB).                                            */
#define ismSTORE_CFG_RECOVERY_MEMSIZE_MB   "Store.RecoveryMemSizeMB"

/* Store.SharedMemoryName                                                      *
 * The Shared Memory name to use in case the Store is configured to use Shared *
 * Memory instead of NVRAM.                                                    *
 *                                                                             *
 * The type of the parameter is string.                                        *
 * The default value is /com.ibm.ism::<uid>:store                              */
#define ismSTORE_CFG_SHM_NAME              "Store.SharedMemoryName"

/* Store.DiskRootPath                                                          *
 * The path of the root directory at which the Store should save data          *
 * (Generation files).                                                         *
 *                                                                             *
 * The type of the parameter is string.                                        *
 * The default value is /tmp/com.ibm.ism/                                      */
#define ismSTORE_CFG_DISK_ROOT_PATH        "Store.DiskRootPath"

/* Store.ClearDiskFiles                                                        *
 * Indicates if the Store should clear/delete the files on disk while          *
 * initializing the Store. The files are deleted only from the Store root path *
 * (set by Store.DiskRootPath).                                                *
 * The Store is expected to clear disk files only when performing a cold start.*
 *                                                                             *
 * A value of 1 indicates the Store should clear the disk files.               *
 * A value of 0 indicates the Store should not clear the disk files.           *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismSTORE_CFG_DISK_CLEAR            "Store.ClearDiskFiles"

/* Store.EnableDiskPersistence                                                     *
 * Defines whether disk persistence is enabled or disabled.                    *
 *                                                                             *
 * A value of 0 indicates disk persistence is disabled.                        *
 * A value of 1 indicates disk persistence is enabled.                         *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_DISK_ENABLEPERSIST    "Store.EnableDiskPersistence"

/* Store.DiskPersistPath                                                       *
 * The path of the root directory at which the Store write data related to     *
 * disk persistence.                                                           *
 * Note that for performance considerations it is recommended to assign two    *
 * different storage systems to Store.DiskPersistPath and Store.DiskRootPath   *
 *                                                                             *
 * The type of the parameter is string.                                        *
 * The default value is /store/persist/                                        */
#define ismSTORE_CFG_DISK_PERSIST_PATH        "Store.DiskPersistPath"

/* Store.PersistBuffSize                                                       *
 * Size of the buffer maintained for each stream to write persistent           *
 * information to disk.                                                        *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1048576 (1MB).                                         */
#define ismSTORE_CFG_PERSIST_BUFF_SIZE     "Store.PersistBuffSize"

/* Store.PersistFileSizeMB                                                     *
 * Size of each persistence file that is maintained on disk.                   *
 * Four persistence files are maintained on disk.                              *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 16392 (16GB).                                          */
#define ismSTORE_CFG_PERSIST_FILE_SIZE_MB  "Store.PersistFileSizeMB"

/* Store.PersistReuseSHM                                                       *
 * Indicates if the Store may use the optimization of reusing an existing      *
 * shared memory region in case it is found to be valid.                       *
 * This parameter is used only when disk persistence is enabled, that is, when *
 * Store.EnableDiskPersistence is set to 1.                                        *
 * The default is to enable the optimization but testers may want to disable   *
 * it in order to be able to simulate a power failure by killing/terminating   *
 * the server.                                                                 *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 1.                                                     */
#define ismSTORE_CFG_PERSIST_REUSE_SHM        "Store.PersistReuseSHM"

/* Store.PersistThreadPolicy                                                   *
 * Indicates the mechanism by which the shmPersist thread and the IOP threads  *
 * are communicating.                                                          *
 * This parameter is used only when disk persistence is enabled, that is, when *
 * Store.EnableDiskPersistence is set to 1.                                    *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * Possible values are:                                                        *
 * 0 => standard pthread_cond_wait/signal                                      *
 * 1 => using the mutex, but waiting with sched_yield (no cond_wait/signal)    *
 * 2 => using an additional thread to do the signalling                        *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_PERSIST_THREAD_POLICY    "Store.PersistThreadPolicy"

/* Store.PersistAsyncThreads                                                   *
 * Defines the number of threads that the Store uses to invoke Engine          *
 * async complete callbacks.                                                   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 2.                                                     */
#define ismSTORE_CFG_PERSIST_ASYNC_THREADS    "Store.PersistAsyncThreads"

/* Store.PersistHaTxThreads                                                    *
 * Defines the number of threads that the Store uses to send HA data to the    *
 * SB node.                                                                    *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 2.                                                     */
#define ismSTORE_CFG_PERSIST_HATX_THREADS     "Store.PersistHaTxThreads"

/* Store.PersistMaxAsyncCB                                                     *
 * Defines the high water mark of queued async CBs to trigger an event of      *
 * ISM_STORE_EVENT_CBQ_ALERT_ON                                                *
 *                                                                             *
 * The type of the parameter is uint32_t                                       *
 * The default value is 65536.                                                 */
#define ismSTORE_CFG_PERSIST_MAX_ASYNC_CB      "Store.PersistMaxAsyncCB"

/* Store.PersistAsyncOrdered                                                   *
 * Defines whether the Store should maintain a per stream order when invoking  *
 * Engine async complete callbacks. When order is maintained the Store will    *
 * not invoke more than one callback per stream at the same time.              *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * Possible values are:                                                        *
 * 0 => no order is enforced                                                   *
 * 1 => per-stream order is enforced                                           *
 * The default value is 1.                                                     */
#define ismSTORE_CFG_PERSIST_ASYNC_ORDERED    "Store.PersistAsyncOrdered"


/* Store.AsyncCBStatsEnabled                                                   *
 * Defines whether we track how many callbacks each async callback thread      *
 * performs.                                                                   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * Possible values are:                                                        *
 * 0 => Normally stats are disabled, they become enabled only while we have to *
 *      pause the server when they are traced at level 5                       *
 * 1 => Stats are enabled, normally traced at 7, traced at 5 in server pause   *
 * 2 => Stats are always traced at 5                                           *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_ASYNCCB_STATSMODE    "Store.AsyncCBStatsMode"

typedef enum __attribute__ ((__packed__))
{
   ISM_STORE_ASYNCCBSTATSMODE_DURING_SERVERPAUSE = 0,
   ISM_STORE_ASYNCCBSTATSMODE_ENABLED            = 1,
   ISM_STORE_ASYNCCBSTATSMODE_FULL               = 2,
} ismStore_AsyncCBStatsMode_t;

/* Store.PersistAnyUserInGroup                                                 *
 * Defines whether or not any user in the group may write to persisted store   *
 * files and directories.                                                      *
 *                                                                             *
 *                                                                             *
 * The type of the parameter is int.                                           *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_PERSIST_ANY_USER_IN_GROUP   "Store.PersistAnyUserInGroup"

/* Store.PersistedFileMode                                                     *
 * A derived configuration parameter. The default will be used unless the      *
 * configuration parameter  Store.PersistAnyUserInGroup is set to true which   *
 * will result in this paramter being set to 00664                             * 
 *                                                                             *
 *                                                                             *
 * The type of the parameter is mode_t.                                        *
 * The default value is 00644.                                                 */
#define ismSTORE_CFG_PERSISTED_FILE_MODE   "Store.PersistedFileMode"

/* Store.PersistedDirectoryMode                                                *
 * A derived configuration parameter. The default will be used unless the      *
 * configuration parameter  Store.PersistAnyUserInGroup is set to true which   *
 * will result in this paramter being set to 0775                              * 
 *                                                                             *
 *                                                                             *
 * The type of the parameter is mode_t.                                        *
 * The default value is 0755.                                                  */
#define ismSTORE_CFG_PERSISTED_DIRECTORY_MODE   "Store.PersistedDirectoryMode"

/*********************************************************************/
/* Store Advanced Configuration Parameters                           */
/*********************************************************************/

/* Store.NVRAMOffset                                                           *
 * Define the offset of the NVRAM on the machine.                              *
 * This parameter is relevant if and only if the memory type is                *
 * ismSTORE_MEMTYPE_NVRAM.                                                     *
 *                                                                             *
 * The type of the parameter is uint64_t.                                      *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_NVRAM_OFFSET          "Store.NVRAMOffset"

/* Store.CacheFlushMode                                                        *
 * Indicate if the Store should perform a cache flush after each memory update.*
 *                                                                             *
 * A value of ismSTORE_CACHEFLUSH_NORMAL indicates no cache flush.             *
 * A value of ismSTORE_CACHEFLUSH_ADR indicates cache flush.                   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is ismSTORE_CACHEFLUSH_NORMAL.                            */
#define ismSTORE_CFG_CACHEFLUSH_MODE       "Store.CacheFlushMode"

#define ismSTORE_CACHEFLUSH_NORMAL         0x0
#define ismSTORE_CACHEFLUSH_ADR            0x1

/* Store.MgmtSmallGranuleSizeBytes                                             *
 * Defines the size in bytes of a small block size in the Management           *
 * Generation.                                                                 *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 128.                                                   */
#define ismSTORE_CFG_MGMT_SMALL_GRANULE    "Store.MgmtSmallGranuleSizeBytes"

/* Store.MgmtGranuleSizeBytes                                                  *
 * Defines the size in bytes of a large block size in the Management           *
 * Generation.                                                                 *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1024.                                                  */
#define ismSTORE_CFG_MGMT_GRANULE_SIZE     "Store.MgmtGranuleSizeBytes"

/* Store.InMemGensCount                                                        *
 * Number of regular Generations (in addition to the Management Generation)    *
 * that the Store maintains.                                                   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 2. The maximal value is 4                              */
#define ismSTORE_CFG_INMEM_GENS_COUNT      "Store.InMemGensCount"

/* Store.DiskTransferBlockSize                                                 *
 * Size of a block that is used by the Store to write data to disk.            *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 32MB.                                                  */
#define ismSTORE_CFG_DISK_BLOCK_SIZE       "Store.DiskTransferBlockSize"

/* Store.MgmtAlertOnPercent                                                    *
 * Defines the high water mark for an alert of low free memory available for   *
 * management generation pool.                                                 *
 * The value is provided as a percent of the total amount of memory available  *
 * for each management generation pool.                                        *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 90.                                                    */
#define ismSTORE_CFG_MGMT_ALERTON_PCT      "Store.MgmtAlertOnPercent"

/* Store.MgmtAlertOffPercent                                                   *
 * Defines the low water mark for an event that clears the low free memory     *
 * available for management generation pool alert. This event is triggered     *
 * after a low memory alert has been triggered but then sufficient amount of   *
 * free memory became available.                                               *
 * The value is provided as a percent of the total amount of memory available  *
 * for each management generation pool.                                        *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 80.                                                    */
#define ismSTORE_CFG_MGMT_ALERTOFF_PCT     "Store.MgmtAlertOffPercent"

/* Store.GenAlertOnPercent                                                     *
 * Defines the high water mark for an alert of low free memory available for   *
 * store generation. When this alert is triggered the Store will direct new    *
 * store-transactions to the next generation and only allow currently active   *
 * store-transactions to work on the current Generation.                       *
 * The value is provided as a percent of the total amount of memory available  *
 * for the generation.                                                         *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 99.                                                    */
#define ismSTORE_CFG_GEN_ALERTON_PCT       "Store.GenAlertOnPercent"

/* Store.DiskAlertOnPercent                                                    *
 * Defines the high water mark for an alert of low free disk space available   *
 * for the store.                                                              *
 * The value is provided as a percent of the total disk space available for    *
 * the store partition.                                                        *
 * A value of zero indicates alerts are turned off.                            *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 90.                                                    */
#define ismSTORE_CFG_DISK_ALERTON_PCT      "Store.DiskAlertOnPercent"

/* Store.DiskAlertOffPercent                                                   *
 * Defines the low water mark for an event that clears the low free disk space *
 * available for the store alert. This event is triggered after a low disk     *
 * space alert has been triggered but then sufficient amount of free disk      *
 * space became available.                                                     *
 * The value is provided as a percent of the total disk space available for    *
 * the store partition.                                                        *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 80.                                                    */
#define ismSTORE_CFG_DISK_ALERTOFF_PCT     "Store.DiskAlertOffPercent"

/* Store.StreamsCacheSizePercent                                               *
 * Defines the maximum size of the streams cache as a percent of the           *
 * generation memory size.                                                     *
 * The store maintains a local cache of free granules per stream to avoid      *
 * thread contention on the main store pool.                                   *
 *                                                                             *
 * The type of the parameter is double.                                        *
 * The default value is 5.                                                     */
#define ismSTORE_CFG_STREAMS_CACHE_PCT     "Store.StreamsCacheSizePercent"

/* Store.RefCtxtLocksCount                                                     *
 * Defines the number of locks used for reference context objects.             *
 * Multiple reference context objects share the same lock instance because the *
 * number of reference context objects might be huge.                          *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 100.                                                   */
#define ismSTORE_CFG_REFCTXT_LOCKS_COUNT   "Store.RefCtxtLocksCount"

/* Store.StateCtxtLocksCount                                                   *
 * Defines the number of locks used for state context objects.                 *
 * Multiple state context objects share the same lock instance because the     *
 * number of state context objects might be huge.                              *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 100.                                                   */
#define ismSTORE_CFG_STATECTXT_LOCKS_COUNT "Store.StateCtxtLocksCount"

/* Store.RefGenPoolExtElements                                                 *
 * Defines the number of new elements (extension) that are added to the        *
 * RefGenPool when the pool becomes empty.                                     *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 20000.                                                 */
#define ismSTORE_CFG_REFGENPOOL_EXT        "Store.RefGenPoolExtElements"

/* Store.TransactionRsrvOps                                                    *
 * Defines the number of store-transaction operations that is reserved for     *
 * each stream. Once a stream is created, the store makes a reservation for    *
 * the stream to guarantee that the stream can contain at least                *
 * TransactionRsrvOps operations even if the store is full.                    *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 1000.                                                  */
#define ismSTORE_CFG_STORETRANS_RSRV_OPS   "Store.TransactionRsrvOps"

/* Store.CompactMemThresholdMB                                                 *
 * Defines the threshold in Mega Bytes for memory generation compaction        *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 100.                                                   */
#define ismSTORE_CFG_COMPACT_MEMTH_MB      "Store.CompactMemThresholdMB"

/* Store.CompactDiskThresholdMB                                                *
 * Defines the threshold in Mega Bytes for disk generation compaction          *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 500.                                                   */
#define ismSTORE_CFG_COMPACT_DISKTH_MB     "Store.CompactDiskThresholdMB"

/* Store.CompactDiskHighWM                                                     *
 * Defines the high water mark for an indication of low free disk space        *
 * available for the store. Once the free disk space drops under this value,   *
 * the store starts an aggressive compaction procedure, until the the free     *
 * disk space returns back to the normal.                                      *
 * The value is provided as a percent of the total disk space available for    *
 * the store partition.                                                        *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 70.                                                    */
#define ismSTORE_CFG_COMPACT_DISK_HWM      "Store.CompactDiskHighWM"

/* Store.CompactDiskLowWM                                                      *
 * Defines the low water mark for an indication of low free disk space         *
 * available for the store. This event is triggered after a low free disk      *
 * space indication has been triggered, and indicates that the aggressive      *
 * compaction procedure should be stopped.                                     *
 * The value is provided as a percent of the total disk space available for    *
 * the store partition.                                                        *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 60.                                                    */
#define ismSTORE_CFG_COMPACT_DISK_LWM      "Store.CompactDiskLowWM"

/* Store.MgmtSmallGranulesPercent                                              *
 * Defines the size of the pool of the small blocks as a percent of the        *
 * Management Generation memory.                                               *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 70.                                                    */
#define ismSTORE_CFG_MGMT_SMALL_PCT        "Store.MgmtSmallGranulesPercent"

/* Store.OwnerLimitPercent                                                     *
 * Defines the maximum number of owner granules as a percent of the memory     *
 * size of the small granules pool in the management generation.               *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 50.                                                    */
#define ismSTORE_CFG_OWNER_LIMIT_PCT       "Store.OwnerLimitPercent"

/* Store.RecoverFromV12                                                        *
 * Set this flag only in order to recover a store that was produced by an      *
 * early version of MessageSight.                                              *
 * After the server is started successfully it must be stopped gracefully and  *
 * RecoverFromV12 should be reset back to zero before restarting the server.   *
 *                                                                             *
 * The type of the parameter is uint8_t.                                       *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_RECOVER_FROM_V12      "Store.RecoverFromV12"

/* Store.RefSearchCacheSize                                                    *
 * Size of the cache used for createReference linked list search.              *
 * The cache is created in the active refGen for owners with more than 64      *
 * reference chunks.                                                           *
 *                                                                             *
 * The type of the parameter is uint16_t.                                      *
 * The default value is 32.                                                    */
#define ismSTORE_CFG_REFSEARCHCACHESIZE    "Store.RefSearchCacheSize"

/* Store.TrimRefChunkOwnerPercent                                              *
 * The lower limit (in percent) on the portion of reference chunks of an       *
 * owner in the active generation that can trigger a reference chunk trim.     *
 *                                                                             *
 * The type of the parameter is uint8_t.                                      *
 * The default value is 2.                                                     */
#define ismSTORE_CFG_REFCHUNKHWMPCT        "Store.TrimRefChunkOwnerPercent"

/* Store.TrimRefChunkUtilnPercent                                              *
 * The lower limit (in percent) on the utilization of reference chunks of an   *
 * owner in the active generation that can trigger a reference chunk trim.     *
 *                                                                             *
 * The type of the parameter is uint8_t.                                      *
 * The default value is 2.                                                     */
#define ismSTORE_CFG_REFCHUNKLWMPCT        "Store.TrimRefChunkUtilnPercent"

/* Store.PersistRecoveryFlags
 * Persist Recovery robustness flags                                           *
 * bit 1 => create missing generation on the next inMemory gen segment         *
 * bit 2 => continue after failure to convert ST handle to address, skipping   *
 *          the rest of the saved STs                                          *
 *                                                                             *
 * The type of the parameter is uint32_t.                                      *
 * The default value is 0.                                                     */
#define ismSTORE_CFG_PERSISTRECOVERYFLAGS  "Store.PersistRecoveryFlags"

typedef uint32_t  ismStore_StreamHandle_t;
typedef uint64_t  ismStore_Handle_t;
typedef uint16_t  ismStore_GenId_t;

/* Store NULL handle                                                           */
#define ismSTORE_NULL_HANDLE      0x0

#define ISM_STORE_NUM_REC_TYPES 16     /* Must be greater than the number of   */
                                       /* entries in ismStore_RecordType_t     */
typedef enum
{
   ISM_STORE_RECTYPE_SERVER = 0x0001,  /* Server record                        */

   ISM_STORE_RECTYPE_CLIENT = 0x0080,  /* Client record                        */
   ISM_STORE_RECTYPE_QUEUE  = 0x0081,  /* Queue record                         */
   ISM_STORE_RECTYPE_TOPIC  = 0x0082,  /* Topic record                         */
   ISM_STORE_RECTYPE_SUBSC  = 0x0083,  /* Subscription record                  */
   ISM_STORE_RECTYPE_TRANS  = 0x0084,  /* Transaction record                   */
   ISM_STORE_RECTYPE_BMGR   = 0x0085,  /* Bridge queue manager record          */
   ISM_STORE_RECTYPE_REMSRV = 0x0086,  /* Remote server record (for cluster)   */
   ISM_STORE_RECTYPE_MAXOWNER,         /* Marks highest owner                  */

   ISM_STORE_RECTYPE_MSG    = 0x0100,  /* Message record                       */
   ISM_STORE_RECTYPE_PROP   = 0x0101,  /* General property record              */
   ISM_STORE_RECTYPE_CPROP  = 0x0102,  /* Client property record               */
   ISM_STORE_RECTYPE_QPROP  = 0x0103,  /* Queue property record                */
   ISM_STORE_RECTYPE_TPROP  = 0x0104,  /* Topic property record                */
   ISM_STORE_RECTYPE_SPROP  = 0x0105,  /* Subscription property record         */
   ISM_STORE_RECTYPE_BXR    = 0x0106,  /* Bridge XID record                    */
   ISM_STORE_RECTYPE_RPROP  = 0x0107,  /* Remote server property record        */
   ISM_STORE_RECTYPE_MAXVAL            /* Used for array bounds                */
} ismStore_RecordType_t;

typedef struct
{
   ismStore_RecordType_t Type;       /* Record type                            */
   uint32_t              FragsCount; /* Number of fragments from which a record*/
                                     /* is to be assembled. The store          */
                                     /* constructs the record by reassembling  */
                                     /* FragsCount fragments from the pFrags   */
                                     /* array into a single buffer.            */
   char                **pFrags;     /* Record fragments array. A pointer to   */
                                     /* an array of buffers comprising the     */
                                     /* fragments of the record. Only the first*/
                                     /* FragsCount buffers are used. Fragments */
                                     /* are reassembled according to the order */
                                     /* they appear in the pFrags array.       */
   uint32_t             *pFragsLengths; /* Lengths of record fragments in      */
                                     /* pFrags. A pointer to an array of       */
                                     /* integer each holding the length of the */
                                     /* fragment in the corresponding index in */
                                     /* the pFrags array. The length of each   */
                                     /* fragment must be at least 1 byte.      */
   uint32_t              DataLength; /* Total data length in bytes.            */
                                     /* It should be the sum of the first      */
                                     /* FragsCount in *pFragsLenghts array.    */
   uint64_t              Attribute;  /* Attribute of the record                */
   uint64_t              State;      /* State of the record                    */
} ismStore_Record_t;

typedef struct
{
   uint64_t             OrderId;     /* Order for the reference.               *
                                      * It is unique across the owner and can  *
                                      * not be re-used in the same owner.      */
   ismStore_Handle_t    hRefHandle;  /* Store handle of the record being       *
                                      * referred to                            */
   uint32_t             Value;       /* Value of the reference                 */
   uint8_t              State;       /* State of the reference                 */
   uint8_t              Pad[3];      /* Reserved - Alignment pad               */
} ismStore_Reference_t;

typedef struct
{
   uint32_t             Value;       /* Value of the state object              */
} ismStore_StateObject_t;

typedef struct
{
   uint64_t             MinimumActiveOrderId; /* Minimum active order id       */
   uint64_t             HighestOrderId;       /* Highest order id              */
   ismStore_GenId_t     LowestGenId;          /* Lowest generation id          */
   ismStore_GenId_t     HighestGenId;         /* Highest generation id         */
} ismStore_ReferenceStatistics_t;

typedef struct
{
   uint64_t             MemoryTotalBytes;     /* total size of management gen  */
   uint64_t             MemoryFreeBytes;      /* free bytes in management gen  */
   uint64_t             Pool1TotalBytes;      /* Pool1 = small granules pool   */
   uint64_t             Pool1UsedBytes;       /* small granules pool usage     */
   uint64_t             Pool1RecordsLimitBytes; /* owner limit                 */
   uint64_t             Pool1RecordsUsedBytes;  /* memory taken by owners      */
   uint64_t             Pool2TotalBytes;      /* Pool2 = large granules pool   */
   uint64_t             Pool2UsedBytes;       /* large granules pool usage     */
   uint64_t             ClientStatesBytes;    /* Client records                */
   uint64_t             QueuesBytes;          /* Queue  records                */
   uint64_t             TopicsBytes;          /* Topic  records                */
   uint64_t             SubscriptionsBytes;   /* Subscriptions records         */
   uint64_t             TransactionsBytes;    /* Transaction records           */
   uint64_t             MQConnectivityBytes;  /* Bridg Manager records         */
   uint64_t             RemoteServerBytes;    /* Remote Server records         */
   uint64_t             IncomingMessageAcksBytes;  /* QoS2 State blocks        */
   uint32_t             RecordSize;           /* size of owner record          */
   uint8_t              MemoryUsedPercent;    /* management gen usage pct      */
   uint8_t              Pool1UsedPercent;     /* small granules pool usage pct */
   uint8_t              Pool2UsedPercent;     /* large granules pool usage pct */
} ismStore_MemStats_t;

typedef struct
{
   uint32_t             GenerationsCount;     /* Number of generations used by
                                               * the store (Memory + Disk),
                                               * including the management
                                               * generation.                   */
   uint32_t             StreamsCount;         /* Number of active streams      */
   uint32_t             StoreTransRsrvOps;    /* Number of store-transaction
                                               * operations that are reserved
                                               * for each stream.              */
   ismStore_GenId_t     ActiveGenId;          /* Active generation Id          */
   uint8_t              StoreDiskUsagePct;    /* Store disk usage in percent   */
   int8_t               HASyncCompletionPct;  /* HA new node synchronization
                                               * completion in percent. Value
                                               * of -1 means that the HA pair
                                               * is not in a new node
                                               * synchronization procedure.    */
   int8_t               RecoveryCompletionPct;/* Recovery completion in
                                               * percent. Value of -1 means
                                               * that the store is not in
                                               * recovery procedure.           */
   uint64_t             DiskFreeSpaceBytes;   /* Disk free space in bytes      */
   uint64_t             DiskUsedSpaceBytes;   /* Disk space used by the store
                                               * in bytes                      */
   ism_time_t           PrimaryLastTime;      /* The last time this node acted
                                               * as a Primary. A value of zero
                                               * means that the last role the
                                               * node had was not Primary      */
   uint32_t        MgmtSmallGranuleSizeBytes; /* Effective size of management
                                               * generation small granules     */
   uint32_t        MgmtGranuleSizeBytes;      /* Effective size of management
                                               * generation large granules     */
  ismStore_MemStats_t   MemStats;             /* Store memory statistics       */
} ismStore_Statistics_t;

typedef struct
{
   uint64_t             DataLength;  /* Total data length in bytes of the      *
                                      * records included in this reservation   */
   uint32_t             RecordsCount;/* Number of records included in this     *
                                      * reservation                            */
   uint32_t             RefsCount;   /* Number of references included in this  *
                                      * reservation                            */
} ismStore_Reservation_t;

typedef struct ismStore_Iterator_t *ismStore_IteratorHandle;

typedef struct
{
    //First some stats recorded across a time period
    double   statsTime;      /*Normalised TSC @ end of stats collecting period*/
    double   periodLength;   /*Seconds in the  stats collecting period        */
    uint32_t numCallbacks;   /*Number of callbacks executed in the last stats */
                             /*collecting period                              */
    uint32_t numLoops;       /*Number of loops in the last stats period       */

    //Below this point returned "live"
    uint32_t numProcessingCallbacks; /* Number of Callbacks currently processing */
    uint32_t numReadyCallbacks;      /* Number of Callbacks ready for execution  */
    uint32_t numWaitingCallbacks;    /* Number of CBs waiting for store op       */

} ismStore_AsyncThreadCBStats_t;

typedef enum
{
   ISM_STORE_EVENT_MGMT0_ALERT_ON  = 1, /* Management small buffers are filling
                                         * up and have reached a high water
                                         * mark                                */
   ISM_STORE_EVENT_MGMT0_ALERT_OFF = 2, /* Management small buffers are down
                                         * to normal                           */

   ISM_STORE_EVENT_MGMT1_ALERT_ON  = 3, /* Management large buffers are filling
                                         * up and have reached a high water
                                         * mark                                */
   ISM_STORE_EVENT_MGMT1_ALERT_OFF = 4, /* Management large buffers are down
                                         * to normal                           */

   ISM_STORE_EVENT_DISK_ALERT_ON   = 5, /* Store disk space is filling up and
                                         * have reached a high water mark      */
   ISM_STORE_EVENT_DISK_ALERT_OFF  = 6, /* Store disk space is down to normal  */
   ISM_STORE_EVENT_CBQ_ALERT_ON    = 7, /* Store completion callback queue is
                                         * filling up and has reached a high
                                         * water mark                          */
   ISM_STORE_EVENT_CBQ_ALERT_OFF  = 8   /* Store completion callback queue is
                                         * down to normal                      */
} ismStore_EventType_t;

/*********************************************************************/
/* Callback function and arguments used to inform the user on events */
/* related to the store.                                             */
/*                                                                   */
/* @param eventType   Event type                                     */
/* @param pContext    Context provided in                            */
/*                    ism_store_registerEventCallback API            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
typedef int (*ismPSTOREEVENT)(ismStore_EventType_t eventType, void *pContext);

/*********************************************************************/
/* Callback function and arguments used to inform the user that an   */
/* operation completed asynchronously.                               */
/* Used in ism_store_asyncCommit.                                    */
/*                                                                   */
/* @param retcode     Return code for the operation performed        */
/* @param pContext    User context provided for completion callback  */
/*                                                                   */
/* @remark The callback may call other EStore operations             */
/*********************************************************************/
typedef void (*ismStore_CompletionCallback_t)(int retcode, void *pContext);

/*********************************************************************/
/* Store Init                                                        */
/*                                                                   */
/* Performs store init tasks                                         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_init(void);

/*********************************************************************/
/* Store Start                                                       */
/*                                                                   */
/* Performs startup tasks.                                           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_start(void);

/*********************************************************************/
/* Store Shutdown                                                    */
/*                                                                   */
/* Performs shutdown tasks.                                          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_term(void);

/*********************************************************************/
/* Performs HA-Pair shutdown                                         */
/*                                                                   */
/* This API is similar to ism_store_term except that when it is      */
/* invoked on the Primary node of an HA-Pair the standby node will   */
/* also be terminated (if it is running).                            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_termHA(void);

/*********************************************************************/
/* Store Drop                                                        */
/*                                                                   */
/* Drops the store.                                                  */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_drop(void);

/*********************************************************************/
/* Store Event Registration                                          */
/*                                                                   */
/* Registers event callback.                                         */
/* @param callback      Callback function                            */
/* @param pContext      Context to associate with the callback       */
/*                      function                                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int ism_store_registerEventCallback(ismPSTOREEVENT callback, void *pContext);

/*********************************************************************/
/* Store Dump                                                        */
/*                                                                   */
/* Dumps the content of the store.                                   */
/*                                                                   */
/* @param filename      File name of the output file                 */
/*                      If the name is stdout or stderr then the     */
/*                      standard out/error stream is used.           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_dump(char *filename);

/*********************************************************************/
typedef enum
{
   ISM_STORE_DUMP_GENID,
   ISM_STORE_DUMP_REC4TYPE,
   ISM_STORE_DUMP_REF4OWNER,
   ISM_STORE_DUMP_STATE4OWNER,
   ISM_STORE_DUMP_REF,
   ISM_STORE_DUMP_MSG,
   ISM_STORE_DUMP_PROP
} ismStore_DumpType_t;

typedef struct
{
   ismStore_DumpType_t     dataType;
   ismStore_GenId_t        genId;
   ismStore_RecordType_t   recType;
   ismStore_Handle_t       handle;
   ismStore_Handle_t       owner;
   ismStore_Record_t      *pRecord;
   ismStore_Reference_t   *pReference;
   ismStore_StateObject_t *pState;
   int                     readRefHandle;
   /* Output parameter - if set (value != 0) the record (or reference) that    */
   /* this reference points to, will be read                                   */

} ismStore_DumpData_t;

typedef int (*ismPSTOREDUMPCALLBACK)(ismStore_DumpData_t *pDumpData, void *pContext);

XAPI int32_t ism_store_dumpCB(ismPSTOREDUMPCALLBACK callback, void *pContext);
XAPI int32_t ism_store_dumpRaw(char *path);
/*********************************************************************/

/*********************************************************************/
/* Get Generation ID of a store handle                               */
/*                                                                   */
/* Returns the generation ID of a store handle.                      */
/*                                                                   */
/* @param handle        Store handle                                 */
/* @param pGenId        Pointer to the generation id                 */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getGenIdOfHandle(ismStore_Handle_t handle,
                                        ismStore_GenId_t *pGenId);

/*********************************************************************/
/* Get Store Statistics                                              */
/*                                                                   */
/* Returns the store statistics.                                     */
/*                                                                   */
/* @param pStatistics     Pointer to the statistics structure        */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getStatistics(ismStore_Statistics_t *pStatistics);

/*********************************************************************/
/* Open Stream                                                       */
/*                                                                   */
/* Opens a stream. All data access operations are part of a          */
/* store-transaction, and a stream is a sequence of                  */
/* store-transactions.                                               */
/*                                                                   */
/* @param phStream      Handle of newly opened stream                */
/* @param fHighPerf     Indicates if the stream is high-performance  */
/*                      or not                                       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openStream(ismStore_StreamHandle_t *phStream,
                                  uint8_t fHighPerf);

/*********************************************************************/
/* Close Stream                                                      */
/*                                                                   */
/* Closes a stream. Any in-progress store-transaction is rolled      */
/* back.                                                             */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeStream(ismStore_StreamHandle_t hStream);

/*********************************************************************/
/* Reserve resources for a Stream                                    */
/*                                                                   */
/* Ensures that the Stream has enough resources to perform a         */
/* store-transaction without filling up the generation.              */
/* The reservation structure defines the resources that should be    */
/* reserved.                                                         */
/* This API must be called as the first operation of a               */
/* store-transaction, otherwise an ISMRC_StoreTransActive error code */
/* is returned.                                                      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pReservation  Pointer to a reservation structure           */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_reserveStreamResources(ismStore_StreamHandle_t hStream,
                                              ismStore_Reservation_t *pReservation);

/*********************************************************************/
/* Cancel resources reservation for a Stream                         */
/*                                                                   */
/* This function is used to cancel a reservation that was made using */
/* ism_store_reserveStreamResources and to complete the store-       */
/* transaction that this call initiated.                             */
/*                                                                   */
/* This API must be called as the second operation of a store-       */
/* transaction following a call to ism_store_reserveStreamResources. */
/* If any other operations have been performed as part of the store- */
/* transaction an ISMRC_StoreTransActive error code is returned.     */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_cancelResourceReservation(ismStore_StreamHandle_t hStream);


/*********************************************************************/
/* Starts a store-transaction if one is not already started.         */
/* The function does nothing (and returns OK) if a store-transaction */
/* was already active when it was called.                            */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param fIsActive     An output flag indicating if a store-        */
/*                      transaction was already active               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_startTransaction(ismStore_StreamHandle_t hStream, int *fIsActive);


/*********************************************************************/
/* Cancel a store-transaction that was started using                 */
/* ism_store_startTransaction().                                     */
/*                                                                   */
/* This API must be called as the second operation of a store-       */
/* transaction following a call to ism_store_startTransaction.       */
/* If any other operations have been performed as part of the store- */
/* transaction an ISMRC_StoreTransActive error code is returned.     */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_cancelTransaction(ismStore_StreamHandle_t hStream);

/*********************************************************************/
/* Get the number of un-committed operations on the Stream           */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pOpsCount     Pointer to the stream operations count       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_getStreamOpsCount(ismStore_StreamHandle_t hStream,
                                         uint32_t *pOpsCount);

/*********************************************************************/
/* Open Reference Context                                            */
/*                                                                   */
/* Opens a reference context. This is used to manage the creation    */
/* and deletion of references for a specific owner.                  */
/*                                                                   */
/* @param hOwnerHandle  Store handle of owner                        */
/* @param pRefStats     Pointer to a structure where the references  */
/*                      statistics will be copied (output).          */
/* @param phRefCtxt     Handle of newly opened reference context     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openReferenceContext(ismStore_Handle_t hOwnerHandle,
                                            ismStore_ReferenceStatistics_t *pRefStats,
                                            void **phRefCtxt);

/*********************************************************************/
/* Close Reference Context                                           */
/*                                                                   */
/* Closes a reference context.                                       */
/*                                                                   */
/* @param hRefCtxt      Reference context handle                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeReferenceContext(void *hRefCtxt);

#if 0
/*********************************************************************/
/* Clear Reference Context                                           */
/*                                                                   */
/* Clears the content of the reference context (including the        */
/* references) as part of a store-transaction.                       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_clearReferenceContext(ismStore_StreamHandle_t hStream,
                                             void *hRefCtxt);
#endif

/*********************************************************************/
/* Open State Context                                                */
/*                                                                   */
/* Opens a state context. This is used to manage the creation and    */
/* deletion of states for a specific owner.                          */
/*                                                                   */
/* @param hOwnerHandle  Store handle of owner                        */
/* @param phStateCtxt   Handle of newly opened state context         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_openStateContext(ismStore_Handle_t hOwnerHandle,
                                        void **phStateCtxt);

/*********************************************************************/
/* Close State Context                                               */
/*                                                                   */
/* Closes a state context.                                           */
/*                                                                   */
/* @param hStateCtxt    State context handle                         */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_closeStateContext(void *hStateCtxt);

/*********************************************************************/
/* Commit Store-Transaction                                          */
/*                                                                   */
/* Commits a store-transaction. The store-transactions committed to  */
/* a stream are applied to the store in order.                       */
/* This function performs synchronous commit.                        */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_commit(ismStore_StreamHandle_t hStream);

/*********************************************************************/
/* Asynchronous commit of a Store-Transaction                        */
/*                                                                   */
/* Commits a store-transaction. The store-transactions committed to  */
/* a stream are applied to the store in order.                       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pCallback     Completion callback to be invoked when the   */
/*                      Store transaction is committed.              */
/* @param pContext      User context to associate with the operation */
/*                                                                   */
/* @return ISMRC_AsyncCompletion on successful completion or an      */
/*                      ISMRC_ value.                                */
/*                                                                   */
/* @remark  Unless an error is detected early the operation always   */
/*          completes asynchronously and invikes the callback        */
/*********************************************************************/
XAPI int32_t ism_store_asyncCommit(ismStore_StreamHandle_t hStream,
                                   ismStore_CompletionCallback_t pCallback,
                                   void *pContext);

/*********************************************************************/
/* Rollback Store-Transaction                                        */
/*                                                                   */
/* Rolls back a store-transaction.                                   */
/*                                                                   */
/* @param hStream       Stream handle                                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_rollback(ismStore_StreamHandle_t hStream);

/*********************************************************************/
/* Create Record definition                                          */
/*                                                                   */
/* Creates a Record Definition in the Store.                         */
/*                                                                   */
/* The pFrags field in the pRecord structure should point to an      */
/* array of buffers and the pFragsLengths field should indicate the  */
/* length of each buffer in the array. FragsCount should indicate    */
/* the number of buffers to use.                                     */
/* The DataLength field of pRecord should indicate the total number  */
/* of bytes to write.                                                */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param pRecord       Pointer to the record structure              */
/* @param pHandle       Store handle of created record               */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                                    const ismStore_Record_t *pRecord,
                                    ismStore_Handle_t *pHandle);

/*********************************************************************/
/* Update Record definition                                          */
/*                                                                   */
/* Updates a Record Definition in the Store.                         */
/*                                                                   */
/* NOTE: Only records that are stored in the store memory can be     */
/* updated using this API. (i.e., records of type                    */
/* ISM_STORE_RECTYPE_MSG could NOT be updated).                      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param handle        Store record handle                          */
/* @param attribute     new attribute of the record                  */
/* @param state         new state of the record                      */
/* @param flags         Indicates the fields to be updated using     */
/*                      bitwise-or:                                  */
/*                      ismSTORE_SET_ATTRIBUTE - Set attribute field */
/*                      ismSTORE_SET_STATE     - Set state field     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_updateRecord(ismStore_StreamHandle_t hStream,
                                    ismStore_Handle_t handle,
                                    uint64_t attribute,
                                    uint64_t state,
                                    uint8_t  flags);

#define ismSTORE_SET_ATTRIBUTE      0x1
#define ismSTORE_SET_STATE          0x2

/*********************************************************************/
/* Delete Record Definition                                          */
/*                                                                   */
/* Deletes a Record Definition in the Store.                         */
/* All the reference chunks and state objects that are linked (by    */
/* owner) of this record are also deleted.                           */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param handle        Store record handle                          */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteRecord(ismStore_StreamHandle_t hStream,
                                    ismStore_Handle_t handle);

/*********************************************************************/
/* Create Reference                                                  */
/*                                                                   */
/* Creates a Reference from one handle to another in the Store.      */
/* The references are stored in order and best performance is        */
/* achieved if they are created in order, or approximately in order. */
/*                                                                   */
/* NOTE: The order id is unique across the owner, and can not be     */
/* re-used in the same owner.                                        */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param pRef          Pointer to the reference structure           */
/* @param minimumActiveOrderId   Minimum order id for references     */
/* @param pHandle       Store handle of created reference            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       const ismStore_Reference_t *pReference,
                                       uint64_t minimumActiveOrderId,
                                       ismStore_Handle_t *pHandle);
/*********************************************************************/
/* Equivalent to ism_store_createReference followed by sync commit   */
/*********************************************************************/
XAPI int32_t ism_store_createReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       const ismStore_Reference_t *pReference,
                                       uint64_t minimumActiveOrderId,
                                       ismStore_Handle_t *pHandle);

/*********************************************************************/
/* Update Reference                                                  */
/*                                                                   */
/* Updates a Reference from one handle to another in the Store.      */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param handle        Store handle of the reference                */
/* @param orderId       Order Id of the reference (required only for */
/*                      references that are stored on the disk)      */
/* @param state         New state of the reference                   */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_updateReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint8_t state,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Equivalent to ism_store_updateReference followed by sync commit   */
/*********************************************************************/
XAPI int32_t ism_store_updateReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint8_t state,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Delete Reference                                                  */
/*                                                                   */
/* Deletes a Reference from one handle to another in the Store.      */
/* The caller must ensure that the referred-to record is not         */
/* orphaned.                                                         */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param handle        Store handle of the reference                */
/* @param orderId       Order Id of the reference (required only for */
/*                      references that are stored on the disk)      */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteReference(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Equivalent to ism_store_deleteReference followed by sync commit   */
/*********************************************************************/
XAPI int32_t ism_store_deleteReferenceCommit(
                                       ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       ismStore_Handle_t handle,
                                       uint64_t orderId,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Prune References                                                  */
/*                                                                   */
/* Prunes references of an owner in the Store.                       */
/* Pruning references is not performed as a store-transaction and    */
/* does not guarantee that all the references below the provided     */
/* minimumActiveOrderId are deleted once the function returns.       */
/*                                                                   */
/* @param hRefCtxt      Reference context handle                     */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_pruneReferences(void *hRefCtxt,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Set the minimum active order ID for an owner                      */
/*                                                                   */
/* Setting minActiveOrderId is used to allow the Store to prune      */
/* references of an owner in order to free resources.                */
/* Setting minActiveOrderId is not performed as a store-transaction  */
/* and is not affected by commit/rollback. It can be invoked at any  */
/* time while a store-transaction is in progress.                    */
/* Setting minActiveOrderId does NOT guarantee that all the          */
/* references below the provided minimumActiveOrderId are deleted    */
/* once the function returns.                                        */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hRefCtxt      Reference context handle                     */
/* @param minimumActiveOrderId   Minimum order id for references     */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_setMinActiveOrderId(ismStore_StreamHandle_t hStream,
                                       void *hRefCtxt,
                                       uint64_t minimumActiveOrderId);

/*********************************************************************/
/* Create State                                                      */
/*                                                                   */
/* Creates a State object for a specified record in the Store.       */
/* The state objects are stored in order and best performance is     */
/* achieved if they are deleted in order, or approximately in order. */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hStateCtxt    State context handle                         */
/* @param pState        Pointer to the state object structure        */
/* @param pHandle       Store handle of created state                */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_createState(ismStore_StreamHandle_t hStream,
                                   void *hStateCtxt,
                                   const ismStore_StateObject_t *pState,
                                   ismStore_Handle_t *pHandle);

/*********************************************************************/
/* Delete State                                                      */
/*                                                                   */
/* Deletes a State object for a specified record in the Store.       */
/*                                                                   */
/* @param hStream       Stream handle                                */
/* @param hStateCtxt    State context handle                         */
/* @param handle        Store handle of the state                    */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_deleteState(ismStore_StreamHandle_t hStream,
                                   void *hStateCtxt,
                                   ismStore_Handle_t handle);

/*********************************************************************/
/* Stop delivering async complete callbacks to the Engine            */
/*                                                                   */
/* Used by the Engine to instruct the Store to stop delivery of      */
/* async complete callbacks. Any pending callbacks are discarded.    */
/* If the Store cannot stop the async delivery threads it returns    */
/* ISMRC_StoreBusy, in which case the Engine should try again.       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_stopCallbacks(void);

/*********************************************************************/
/* Get Stats about the Callbacks from  async commits completing      */
/*                                                                   */
/* Stats are of two types (see ismStore_AsyncThreadCBStats_t).       */
/* Some stats relate to a period (of ~5mins) e.g. how many CBs were  */
/* completed in this period. Others stats are "live" at the time     */
/* of the API call                                                   */
/*                                                                   */
/* @param pTotalReadyCBs     Callbacks that can be called ASAP       */
/* @param pTotalWaitingCBs   Callbacks waiting for commit to complete*/
/* @param pNumThreads        On input - array length that            */
/*                           pCBThreadStats points to.               */
/*                           On output if rc = ISMRC_ArgNotValid:    */
/*                           then number of threads stats needed for */
/*                           successful output                        */
/*                           On output if rc = OK: Number of thread  */
/*                           stats structure filled out with data    */
/* @param pCBThreadStats     Caller allocated/freed array of         */
/*                           ismStore_AsyncThreadCBStats_t that      */
/*                           this function will fill with data       */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*         If rc is ISMRC_ArgNotValid then recall with the suggested */
/*              pNumThreads of memory allocated for pCBThreadStats   */
/*********************************************************************/
XAPI int32_t ism_store_getAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                       uint32_t *pNumThreads,
                                       ismStore_AsyncThreadCBStats_t *pCBThreadStats);



/* ----------         Start of Recovery Mode APIs         ---------- */

/*********************************************************************/
/* Get next Generation ID                                            */
/*                                                                   */
/* A recovery function to iterate over the available generations.    */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls.                                       */
/* @param pGenId        Pointer to the generation id                 */
/*********************************************************************/
XAPI int32_t ism_store_getNextGenId(ismStore_IteratorHandle *pIterator,
                                    ismStore_GenId_t *pGenId);

/*********************************************************************/
/* Get next Record                                                   */
/*                                                                   */
/* A recovery function to iterate over the available records of a    */
/* given type and a given generation. The first generation that is   */
/* returned by this function is the management generation.           */
/*                                                                   */
/* The pFrags field in the pRecord structure should point to a       */
/* pre-allocated array of buffers and the pFragsLengths field should */
/* indicate the length of each buffer in the array. On input the     */
/* DataLength field of pRecord should indicate the maximal number of */
/* bytes to read (DataLength should not exceed the total size of     */
/* buffers provided in pFrags).                                      */
/* The store copies the data into the buffers, and set DataLength    */
/* to the total number of bytes copied to pFrags. The values of      */
/* pFragsLengths are not modified.                                   */
/* If the buffers are too small, the DataLength is set to the actual */
/* data length, and an ISMRC_StoreBufferTooSmall code is returned.   */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same type and genId.          */
/* @param recordType    Record type                                  */
/* @param genId         Store generation identifier                  */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pRecord       Pointer to the record structure              */
/*********************************************************************/
XAPI int32_t ism_store_getNextRecordForType(ismStore_IteratorHandle *pIterator,
                                            ismStore_RecordType_t recordType,
                                            ismStore_GenId_t genId,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_Record_t *pRecord);

/*********************************************************************/
/* Read a Record                                                     */
/*                                                                   */
/* A recovery function to read the content of a record in the given  */
/* handle.                                                           */
/*                                                                   */
/* The pFrags field in the pRecord structure should point to a       */
/* pre-allocated array of buffers and the pFragsLengths field should */
/* indicate the length of each buffer in the array. On input the     */
/* DataLength field of pRecord should indicate the maximal number of */
/* bytes to read (DataLength should not exceed the total size of     */
/* buffers provided in pFrags).                                      */
/* The store copies the data into the buffers, and set DataLength    */
/* to the total number of bytes copied to pFrags. The values of      */
/* pFragsLengths are not modified.                                   */
/* If the buffers are too small, the DataLength is set to the actual */
/* data length, and an ISMRC_StoreBufferTooSmall code is returned.   */
/* If block is zero the function may return ISMRC_WouldBlock         */
/* to indicate that the record is currently not in memory (pRecord   */
/* is not modified in this case).                                    */
/*                                                                   */
/* @param handle        Handle of the required record                */
/* @param pRecord       Pointer to the record structure              */
/* @param block         If set the record is obtained even if it is  */
/*                      not currently available in memory            */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*         ISMRC_StoreBufferTooSmall indicates a larger buffer is    */
/*         needed to read the record                                 */
/*         ISMRC_WouldBlock indicates the record was not read        */
/*         since it is not in memory (and block is zero)             */
/*********************************************************************/
XAPI int32_t ism_store_readRecord(ismStore_Handle_t  handle,
                                      ismStore_Record_t *pRecord,
                                      uint8_t block);

/*********************************************************************/
/* Get next Owner                                                    */
/*                                                                   */
/* A recovery function to iterate over the available references'     */
/* owners of a given generation.                                     */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same owner and genId.         */
/* @param genId         Store generation identifier                  */
/* @param pOwnerHandle  Pointer to the returned Store owner handle   */
/* @param pRecordRype   Pointer to the record type of the returned   */
/*                      owner.                                       */
/*********************************************************************/
XAPI int32_t ism_store_getNextOwner(ismStore_IteratorHandle *pIterator,
                                    ismStore_GenId_t genId,
                                    ismStore_Handle_t *pOwnerHandle,
                                    ismStore_RecordType_t *pRecordType);

/*********************************************************************/
/* Get next Reference                                                */
/*                                                                   */
/* A recovery function to iterate over the available references of   */
/* a given owner and a given generation.                             */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same owner and genId.         */
/* @param hOwnerHandle  Store handle of the references' owner        */
/* @param genId         Store generation identifier                  */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pReference    Pointer to the reference structure           */
/*********************************************************************/
XAPI int32_t ism_store_getNextReferenceForOwner(ismStore_IteratorHandle *pIterator,
                                                ismStore_Handle_t hOwnerHandle,
                                                ismStore_GenId_t genId,
                                                ismStore_Handle_t *pHandle,
                                                ismStore_Reference_t *pReference);

/*********************************************************************/
/* Get information for a Reference                                   */
/*                                                                   */
/* A recovery function to get the information of a given reference   */
/* handle.                                                           */
/* If block is zero the function may return ISMRC_WouldBlock         */
/* to indicate that the reference is currently not in memory         */
/* (output parameters are not modified in this case).                */
/*                                                                   */
/* This function does not validate that the reference is valid (has  */
/* not been deleted).                                                */
/* If an output parameter (pOwnerHandle, pOwnerRecType, pOrderId) is */
/* NULL it is ignored.                                               */
/*                                                                   */
/* @param handle        Handle of the reference                      */
/* @param pOwnerHandle  Pointer to the returned owner handle         */
/* @param pOwnerRecType Pointer to the returned owner record type    */
/* @param pOrderId      Pointer to the returned reference orderId    */
/* @param block         If set the information is obtained even if   */
/*                      the reference is not currently available in  */
/*                      recovery memory                              */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*         ISMRC_WouldBlock indicates the information was not        */
/*         retrieved since the reference is not in memory (block==0) */
/*         ISMRC_Error indicates that the handle is not valid or     */
/*         the reference is not valid (e.g., already deleted).       */
/*********************************************************************/
XAPI int32_t ism_store_getReferenceInformation(ismStore_Handle_t handle,
                                               ismStore_Handle_t *pOwnerHandle,
                                               ismStore_RecordType_t *pOwnerRecType,
                                               uint64_t *pOrderId,
                                               uint8_t block);

/*********************************************************************/
/* Get next State                                                    */
/*                                                                   */
/* A recovery function to iterate over the available stats of        */
/* a given owner.                                                    */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same owner.                   */
/* @param hOwnerHandle  Store handle of the references' owner        */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pState        Pointer to the state structure               */
/*********************************************************************/
XAPI int32_t ism_store_getNextStateForOwner(ismStore_IteratorHandle *pIterator,
                                            ismStore_Handle_t hOwnerHandle,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_StateObject_t *pState);

/*********************************************************************/
/* Get next owner in a generation                                    */
/*                                                                   */
/* A recovery function to iterate over the new owners, i.e., owners  */
/* that were not yet reported during recovery, of a given owner type */
/* in a generation.                                                  */
/* Important: this function must not be invoked with a genId that    */
/*            already been processed earlierbe.                      */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same generation and recordType*/
/* @param recordType    The record type of the requested owners      */
/* @param genId         Store generation identifier                  */
/* @param pHandle       Pointer to the returned Store record handle  */
/* @param pRecord       Pointer to the record structure              */
/*********************************************************************/
XAPI   int32_t ism_store_getNextNewOwner(ismStore_IteratorHandle *pIterator,
                                         ismStore_RecordType_t recordType,
                                         ismStore_GenId_t genId,
                                         ismStore_Handle_t *pHandle,
                                         ismStore_Record_t *pRecord);

/*********************************************************************/
/* Get next owner that has references in a generation                */
/*                                                                   */
/* A recovery function to iterate over the owners of a given type    */
/* that have references in a generation.                             */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same generation and recordType*/
/* @param recordType    The record type of the requested owners      */
/* @param genId         Store generation identifier                  */
/* @param pOwnerHandle  Pointer to the returned Store owner handle   */
/*********************************************************************/
XAPI int32_t ism_store_getNextRefOwner(ismStore_IteratorHandle *pIterator,
                                       ismStore_RecordType_t recordType,
                                       ismStore_GenId_t genId,
                                       ismStore_Handle_t *pOwnerHandle);

/*********************************************************************/
/* Get next owner that has a property record in a generation         */
/*                                                                   */
/* A recovery function to iterate over the owners of a given type    */
/* that have a property record in a generation.                      */
/* Important: this function must not be invoked with a genId that    */
/*            already been processed earlierbe.                      */
/*                                                                   */
/* @param pIterator     A pointer to an iterator handle.             */
/*                      It should be initialized to NULL on first    */
/*                      call and left unchanged for the rest of the  */
/*                      calls with the same generation and recordType*/
/* @param recordType    The record type of the requested owners      */
/* @param genId         Store generation identifier                  */
/* @param pOwnerHandle  Pointer to the returned Store owner handle   */
/* @param pAttribute    Pointer to the owner's Attribute field. The  */
/*                      Attribute field is used to point to the      */
/*                      property record                              */
/*********************************************************************/
XAPI   int32_t ism_store_getNextPropOwner(ismStore_IteratorHandle *pIterator,
                                          ismStore_RecordType_t recordType,
                                          ismStore_GenId_t genId,
                                          ismStore_Handle_t *pOwnerHandle,
                                          uint64_t *pAttribute);

/*********************************************************************/
/* Comparing two Store handles                                       */
/*                                                                   */
/* This function compares the two Store handles handle1 and handle2. */
/* It sets the pResult to an integer value greater than, equal to or */
/* less than zero if the generation in which handle1 was created is  */
/* found, respectively, to be newer than, equal to or be older than  */
/* the generation in which handle2 was created.                      */
/*                                                                   */
/* Assumptions:                                                      */
/* 1. The API is called before the ism_store_recoveryCompleted is    */
/*    called.                                                        */
/* 2. If an handle points to an object in the management generation  */
/*    then it points to an owner. Otherwise, an ISMRC_ArgNotValid    */
/*    is returned.                                                   */
/*                                                                   */
/* @param handle1       A valid store handle                         */
/* @param handle2       A valid store handle                         */
/* @param pResult       Pointer to an integer where the result will  */
/*                      be copied.                                   */
/*                                                                   */
/* @return ISMRC_OK on successful completion or an ISMRC_ value.     */
/*********************************************************************/
XAPI int32_t ism_store_compareHandles(ismStore_Handle_t handle1,
                                      ismStore_Handle_t handle2,
                                      int *pResult);

/*********************************************************************/
/* Recovery completed                                                */
/*                                                                   */
/* A recovery function to enable release of resources used during    */
/* the recovery process.                                             */
/*********************************************************************/
XAPI int32_t ism_store_recoveryCompleted(void);

/* ----------          End of Recovery Mode APIs          ---------- */

#ifdef __cplusplus
}
#endif

#endif /* __ISM_STORE_DEFINED */

/*********************************************************************/
/* End of store.h                                                    */
/*********************************************************************/
