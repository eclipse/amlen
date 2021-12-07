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

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <netdb.h>

#include "mbconstants.h"
#include "tcpclient.h"

#define MAX_PING_RETRIES 3
#define WS_SERVER_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_CLIENT_KEY_LEN 16
#define WEBSOCKET_CLIENT_REQUEST_HEADER "\
GET %s HTTP/1.1\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n\
Host: %s\r\n\
Sec-WebSocket-Version: 13\r\n\
Sec-WebSocket-Protocol: %s\r\n\
Sec-WebSocket-Key: %s\r\n\r\n"

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_ClkSrc;
extern uint8_t g_DoDisconnect;
extern uint8_t g_RequestedQuit;
extern uint8_t g_StopIOPProcessing;
extern uint8_t g_TCPConnLatOnly;         /* declared in env.c */

extern int g_Equiv1Sec_Conn;             /* conversion of g_UnitsConn to 1 sec. */
extern int g_LatencyMask;
extern int g_MaxRetryAttempts;
extern int g_MBErrorRC;
extern int g_MBTraceLevel;

extern double g_UnitsConn;
extern double g_StartTimeConnTest;

extern mqttbenchInfo_t *g_MqttbenchInfo;
extern pthread_spinlock_t connTime_lock;
extern pskArray_t *pIDSharedKeyArray;   /* declared in env.c - used for preshared key. */
extern ism_threadh_t g_thrdHandleDoCommands;

extern unsigned char *g_alpnList;       /* declared in env.c - ALPN protocol list - https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_alpn_protos.html */
extern unsigned int g_alpnListLen;      /* declares in env.c - ALPN protocol list length */


/* ******************************** GLOBAL VARIABLES ********************************** */
/* Statics */
static SSL_CTX **g_SSLCtx;
static pthread_mutex_t *openSSLLocks;   /* Used by OpenSSL */

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Initial declarations */
ioProcThread_t **ioProcThreads;          /* ioProcessor threads */
ioListenerThread_t **ioListenerThreads;  /* ioListener threads */
uint32_t g_TotalAttemptedRetries = 0;
uint64_t bfAddrInUse = 0;                /* Bind - Address In Use Counter */
uint64_t bfAddrNotAvail = 0;             /* Bind - Address Not Available Counter */
uint64_t cfAddrNotAvail = 0;             /* Connect - Address Not Available Counter */

/* Function prototypes for functions unique to this file. */
static void * ioListenerThread (void * threadArg, void * context, int value);
static void * ioProcessorThread (void * threadArg, void * context, int value);
static int stopIOListening (transport_t * trans);
static unsigned int psk_client_cb (SSL *, const char *, char *, unsigned int, unsigned char *, unsigned int);
static void scheduleReconnect (transport_t *, const char *file, int line);

int addTransport (ioListenerThread_t *, ioProcThread_t *, transport_t *);

/* Client callbacks */
int onConnect(transport *trans);
int onShutdown(transport *trans);

/* SSL Buffer Pool */
static ism_byteBufferPool pool64B = NULL;
static ism_byteBufferPool pool128B = NULL;
static ism_byteBufferPool pool256B = NULL;
static ism_byteBufferPool pool512B = NULL;
static ism_byteBufferPool pool1KB = NULL;
static ism_byteBufferPool pool2KB = NULL;

struct randomRStruct *ioProcRandData = NULL;


/* *************************************************************************************
 * initializeBufferPools
 *
 * Description:  Initialize buffer pools
 *
 * Note:         This is the mqttbench version of the initializeBufferPools() in server_utils.
 *
 *   @param[in]  mqttbenchInfo       = Structure which holds pointers to other structures
 *   @param[in]  pSSLBfrEnv          = SSL Buffer Pool Info
 *   @param[out]	                 = x
 * *************************************************************************************/
static void initializeBufferPools (mqttbenchInfo_t *mqttbenchInfo, int useBufferPool)
{
	uint64_t sslMemBfrPoolSize = 0;
	mbSSLBufferInfo_t *pSSLBfrEnv = mqttbenchInfo->mbSSLBfrEnv;

	if (pSSLBfrEnv->sslBufferPoolMemory > 0)
		sslMemBfrPoolSize = pSSLBfrEnv->sslBufferPoolMemory;
	else  /* division by 1024 is equivalent to shifting right by 10 */
		sslMemBfrPoolSize = (mqttbenchInfo->mbSysEnvSet->totalSystemMemoryMB >> 10) * 4000;

	pSSLBfrEnv->pool64B_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool64B_totalSize = (uint64_t)(64 * sslMemBfrPoolSize * useBufferPool);

	pSSLBfrEnv->pool128B_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool128B_totalSize = (uint64_t)(128 * sslMemBfrPoolSize * useBufferPool);

	pSSLBfrEnv->pool256B_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool256B_totalSize = (uint64_t)(256 * sslMemBfrPoolSize * useBufferPool);

	pSSLBfrEnv->pool512B_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool512B_totalSize = (uint64_t)(512 * sslMemBfrPoolSize * useBufferPool);

	pSSLBfrEnv->pool1KB_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool1KB_totalSize = (uint64_t)(1024 * sslMemBfrPoolSize * useBufferPool);

	pSSLBfrEnv->pool2KB_numBfrs = (uint32_t)(sslMemBfrPoolSize * useBufferPool);
	pSSLBfrEnv->pool2KB_totalSize = (uint64_t)(2048 * sslMemBfrPoolSize * useBufferPool);

	pool64B = ism_common_createBufferPool (64, pSSLBfrEnv->pool64B_numBfrs, pSSLBfrEnv->pool64B_numBfrs, "pool64B");
	pool128B = ism_common_createBufferPool (128, pSSLBfrEnv->pool128B_numBfrs, pSSLBfrEnv->pool128B_numBfrs, "pool128B");
	pool256B = ism_common_createBufferPool (256, pSSLBfrEnv->pool256B_numBfrs, pSSLBfrEnv->pool256B_numBfrs, "pool256B");
	pool512B = ism_common_createBufferPool (512, pSSLBfrEnv->pool512B_numBfrs, pSSLBfrEnv->pool512B_numBfrs, "pool512B");
	pool1KB = ism_common_createBufferPool (1024, pSSLBfrEnv->pool1KB_numBfrs, pSSLBfrEnv->pool1KB_numBfrs, "pool1KB");
	pool2KB = ism_common_createBufferPool (2048, pSSLBfrEnv->pool2KB_numBfrs, pSSLBfrEnv->pool2KB_numBfrs, "pool2KB");
} /* initializeBufferPools */

/* *************************************************************************************
 * destroyBufferPools
 *
 * Description:  Destroy the SSL Buffer Pool
 *
 * Note:         This is taken directly from server_utils.
 * *************************************************************************************/
static void destroyBufferPools (void)
{
    if (pool64B) {
        ism_common_destroyBufferPool(pool64B);
        pool64B = NULL;
    }
    if (pool128B) {
        ism_common_destroyBufferPool(pool128B);
        pool128B = NULL;
    }
    if (pool256B) {
        ism_common_destroyBufferPool(pool256B);
        pool256B = NULL;
    }
    if (pool512B) {
        ism_common_destroyBufferPool(pool512B);
        pool512B = NULL;
    }
    if (pool1KB) {
        ism_common_destroyBufferPool(pool1KB);
        pool1KB = NULL;
    }
    if (pool2KB) {
        ism_common_destroyBufferPool(pool2KB);
        pool2KB = NULL;
    }

    return;
} /* destroyBufferPools */

/* *************************************************************************************
 * getBuffer
 *
 * Description:  Get a buffer for the particular SSL Buffer Pool.
 *
 * Note:         Inline function which is taken directly from server_utils.
 *
 *   @param[in]  size                = Size of buffers requested.
 *
 *   @return     =
 * *************************************************************************************/
static inline void * getBuffer (size_t size)
{
    if (size <= 64) {
        return ism_common_getBuffer(pool64B, 1)->buf;
    }
    if (size <= 128) {
        return ism_common_getBuffer(pool128B, 1)->buf;
    }
    if (size <= 256) {
        return ism_common_getBuffer(pool256B, 1)->buf;
    }
    if (size <= 512) {
        return ism_common_getBuffer(pool512B, 1)->buf;
    }
    if (size <= 1024) {
        return ism_common_getBuffer(pool1KB, 1)->buf;
    }
    if (size <= 2048) {
        return ism_common_getBuffer(pool2KB, 1)->buf;
    }

    return ism_allocateByteBuffer(size)->buf;
} /* getBuffer */

/* *************************************************************************************
 * returnBuffer
 *
 * Description:  Return an SSL Buffer to the pool.
 *
 * Note:         This is taken directly from server_utils.
 *
 *   @param[in]  bfr                 = An SSL Buffer to be returned to pool.
 * *************************************************************************************/
static inline void returnBuffer (void *bfr)
{
    ism_byteBuffer rb = (ism_byteBuffer)bfr;
    rb--;
    ism_common_returnBuffer(rb,__FILE__,__LINE__);
} /* returnBuffer */

/* *************************************************************************************
 *
 * OpenSSL thread ID callback used to identify threads making calls to OpenSSL
 *
 * *************************************************************************************/
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void threadIdCB(CRYPTO_THREADID *tid)
{
    CRYPTO_THREADID_set_numeric(tid, (unsigned long)pthread_self());
}
#endif

/*
 * Return a buffer with any errors from openssl
 * The openssl error handling is not very good, but gives us some clues about what happened.
 */
static void sslGatherErr(concat_alloc_t * buf) {
    uint32_t     rc;
    const char * file;
    int          line;
    int          flags;
    const char * data;
    char *       pos;
    char         mbuf[256];
    char         lbuf[2048];

    for (;;) {
        rc = (uint32_t)ERR_get_error_line_data(&file, &line, &data, &flags);
    	if (rc == 0)
            break;
        ERR_error_string_n(rc, mbuf, sizeof mbuf);
        pos = strchr(mbuf, ':');
        if (!pos)
            pos = mbuf;
        else
            pos++;
        if (flags & ERR_TXT_STRING) {
            snprintf(lbuf, sizeof lbuf, "%s: %s\n", pos, data);
        } else {
            snprintf(lbuf, sizeof lbuf, "%s\n", pos);
        }
        ism_common_allocBufferCopy(buf, lbuf);
        buf->used--;
    }
    if (buf->used && buf->buf[buf->used-1]=='\n') {
        buf->buf[buf->used-1] = 0;
    } else {
        ism_common_allocBufferCopyLen(buf, "", 1);
    }
}

/*
 * Trace an SSL/TLS error
 */
static void traceSSLError(const char *errMsg, int trcLevel, const char *file, int line) {
	char xbuf[8192];
    concat_alloc_t buf = { 0 };
    buf.buf = xbuf;
    buf.len = sizeof xbuf;
    sslGatherErr(&buf);

    MBTRACE(MBERROR, trcLevel, "%s (%s:%d) => cause=%s\n", errMsg, file, line, buf.buf);
    ism_common_freeAllocBuffer(&buf);
}

/* *************************************************************************************
 * sslLockOps
 *
 * Description: x
 *
 *   @param[in]	mode                = x
 *   @param[in]	type                = x
 *   @param[in]	file                = x
 *   @param[in]	line                = x
 *
 * *************************************************************************************/
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static uint64_t			 *sslLockStats = NULL;
static double             prevTime = 0;

static void sslLockOps(int mode, int type, const char *file, int line)
{
	if (ism_common_getTraceLevel() > 5) {
		__sync_add_and_fetch(&sslLockStats[type], 1);
		double currTime = ism_common_readTSC();
		if(currTime - prevTime > 5) {
			prevTime = currTime;
			int numLocks = CRYPTO_num_locks();
			char buf[4096] = {0};
			for(int i=0; i < numLocks; i++) {
				char xbuf[64] = {0};
				snprintf(xbuf, 64, "lockType=%d count=%llu, ", i, (unsigned long long) sslLockStats[i]);
				strcat(buf, xbuf);
			}
			MBTRACE(MBDEBUG, 5, "SSL Lock stats: %s\n", buf);
		}
	}

    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&openSSLLocks[type]);
    else
        pthread_mutex_unlock(&openSSLLocks[type]);
}
/* end OpenSSL locking callbacks */
#endif

/* *************************************************************************************
 * ssl_malloc
 *
 * Description:  Allocate for a TLS object
 *
 * Note:         This is taken directly from server_utils.
 *
 *   @param[in]  size                = x
 *   @param[in]  fn                  = x
 *   @param[in]  ln                  = x
 * *************************************************************************************/
static void * ssl_malloc (size_t size, const char *fn,  int ln)
{
    return getBuffer(size);
}

/* *************************************************************************************
 * ssl_realloc
 *
 * Description:  Reallocate for a TLS object
 *
 * Note:         This is taken directly from server_utils.
 *
 *   @param[in]  size                = x
 *   @param[in]  fn                  = x
 *   @param[in]  ln                  = x
 * *************************************************************************************/
static void * ssl_realloc (void *p, size_t size, const char *fn,  int ln)
{
    char *result;
    ism_byteBuffer buff = (ism_byteBuffer)p;

    if (p) {
        buff--;
        if (size <= buff->allocated)
            return p;
    }

    result = getBuffer(size);
    if (buff) {
        memcpy(result, buff->buf, buff->allocated);
        returnBuffer(p);
    }

    return result;
} /* ssl_realloc */

/* *************************************************************************************
 * ssl_free
 *
 * Description:  Free a TLS object
 *
 * *************************************************************************************/
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void ssl_free (void *p)
{
    if (p)
        returnBuffer(p);
}
#else
static void ssl_free(void * p, const char * file, int line) {
    if (p)
        returnBuffer(p);
}
#endif

/* *************************************************************************************
 * sslLockInit
 *
 * Description:  Initialize locks for OpenSSL
 *
 * Note:         This is taken directly from server_utils.
 *
 *   @return 0   = Successful Completion
 *           1   = An error/failure occurred.
 * *************************************************************************************/
static int sslLockInit (void)
{
	int rc = 0;
	int i;
	int num = CRYPTO_num_locks();

    do {
	    openSSLLocks = (pthread_mutex_t *) OPENSSL_malloc(num * sizeof(pthread_mutex_t));
	    if (!openSSLLocks) {
		    rc = 1;
            break;
        }

	    for ( i = 0 ; i < num ; i++ ) {
	    	pthread_mutex_init(&openSSLLocks[i], NULL);
	    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	    CRYPTO_THREADID_set_callback(threadIdCB);
	    CRYPTO_set_locking_callback(sslLockOps);

	    sslLockStats = (uint64_t *) OPENSSL_malloc(num * sizeof(uint64_t));
	    memset(sslLockStats, 0, num * sizeof(uint64_t));
#endif
    } while(0);

	return rc;
} /* sslLockInit */

/* *************************************************************************************
 * sslLockCleanup
 *
 * Description:  Cleanup the SSL Locks.
 *
 * Note:         This is the mqttbench version of the server_utils:  sslLockCleanup()
 * *************************************************************************************/
static void sslLockCleanup (void)
{
	int i;
	int num = CRYPTO_num_locks();

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

	for ( i = 0 ; i < num ; i++ ) {
		pthread_mutex_destroy(&openSSLLocks[i]);
	}

	OPENSSL_free(openSSLLocks);
} /* sslLockCleanup */

/* *************************************************************************************
 * sslInit
 *
 * Description:  Initialize OpenSSL
 *
 * Note:         This is the mqttbench version of the server_utils:  ism_ssl_init()
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful Completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
static int sslInit (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;

	uint8_t useBufferPool = mqttbenchInfo->mbSysEnvSet->mqttbenchTLSBfrPool; /* Set the buffer pool flag. */

	/* --------------------------------------------------------------------------------
	 * Check to see who is managing the TLS Buffer Pool:
	 *   0 = Server Utils / OpenSSL
	 *   1 = mqttbench internally
	 * -------------------------------------------------------------------------------- */
	if (useBufferPool) {
		initializeBufferPools(mqttbenchInfo, useBufferPool);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        CRYPTO_set_mem_ex_functions(ssl_malloc, ssl_realloc, ssl_free);
#else
        CRYPTO_set_mem_functions(ssl_malloc, ssl_realloc, ssl_free);
#endif
	}

	SSL_load_error_strings();
	SSL_library_init();
	ERR_load_BIO_strings();
	ERR_load_CRYPTO_strings();

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();
	rc = sslLockInit();
	if (rc)
		MBTRACE(MBERROR, 1, "Failed to initialize OpenSSL locks (rc: %d).\n", rc);

	return rc;
} /* sslInit */

/* *************************************************************************************
 * sslCleanup
 *
 * Description:  Terminate SSL/TLC processing.
 *
 * Note:         This is the mqttbench version of the server_utils:  ism_ssl_cleanup()
 * *************************************************************************************/
void sslCleanup (void)
{
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    sslLockCleanup();
    destroyBufferPools();
} /* sslCleanup */

/* *************************************************************************************
 * initSSLCtx
 *
 * Description:  Initialize an OpenSSL context
 *
 *   @param[in]  sslClientMethod     = SSL Client Method to be used.
 *   @param[in]  ciphers             = a list of ciphers to use during negotiation
 *
 *   @return ctx = A valid Open SSL Context.
 *
 * *************************************************************************************/
SSL_CTX * initSSLCtx (char *sslClientMethod, const char *ciphers)
{
    SSL_CTX *ctx = NULL;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    const SSL_METHOD *method = TLSv1_2_client_method();
    do {
    	if (strcmp(sslClientMethod,"SSLv23") == 0 ) {
    		method = SSLv23_client_method();
    		break;
    	}
    	if (strcmp(sslClientMethod,"TLSv1") == 0 ) {
    		method = TLSv1_client_method();
    		break;
    	}
    	if (strcmp(sslClientMethod,"TLSv11") == 0 ) {
    		method = TLSv1_1_client_method();
    		break;
    	}
    	if (strcmp(sslClientMethod,"TLSv12") == 0 ) {
    		method = TLSv1_2_client_method();
    		break;
    	}
    } while(0);
#else
    const SSL_METHOD *method = TLS_client_method();  // version specific protocols are deprecated started in OpenSSL 1.1.0
#endif

    /*
	 * Create the SSL/TLS context
	 */
    ctx = SSL_CTX_new((SSL_METHOD *) method);
    if (ctx) {
    	int rc=0;
#if OPENSSL_VERSION_NUMBER < 0x10101000L
        rc=SSL_CTX_set_cipher_list(ctx, ciphers);
#else
        if(g_alpnListLen > 0) {
            SSL_CTX_set_alpn_protos(ctx, g_alpnList, g_alpnListLen);  /* set application level protocols if configured by env variable */
		}
        rc=SSL_CTX_set_ciphersuites(ctx, ciphers);
#endif
        if(!rc) {
        	traceSSLError("Failed to set the TLS cipher list/suites", 5, __FILE__, __LINE__);
        	return NULL;
        }

        /* ----------------------------------------------------------------------------
    	 * Check to see if the PreShared Keys is being used and if so set the callback.
    	 * This must occur prior to the connect call.
    	 * ---------------------------------------------------------------------------- */
    	if (pIDSharedKeyArray)
    		SSL_CTX_set_psk_client_callback(ctx, psk_client_cb);

        SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

        if((!SSL_CTX_load_verify_locations(ctx, NULL, "certs/")) ||
           (!SSL_CTX_set_default_verify_paths(ctx)))
        {
        	traceSSLError("Failed to load trust store locations", 5, __FILE__, __LINE__);
        	return NULL;
        }
    } else {
    	traceSSLError("Failed to create TLS context", 5, __FILE__, __LINE__);
    }

    return ctx;
} /* initSSLCtx */

/* *************************************************************************************
 * sslDestroyCtx
 *
 * Description:  Cleanup the OpenSSL context
 *
 *   @param[in]  sslCtxt             = OpenSSL Context to be destroyed.
 * *************************************************************************************/
static void sslDestroyCtx (SSL_CTX *sslCtxt)
{
	SSL_CTX_free(sslCtxt);
}

/* *************************************************************************************
 * tcpInit
 *
 * Description:  Initialize a TCP transport
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return 0   = Successful Completion.
 *         123   = The SSL Context is NULL.
 * *************************************************************************************/
int tcpInit (mqttbenchInfo_t *mqttbenchInfo)
{
	int rc = 0;
	int numIOP = (int) mqttbenchInfo->mbSysEnvSet->numIOProcThreads;
	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;
	MBTRACE(MBINFO, 2, "Initializing the tcpclient module\n");

	/* --------------------------------------------------------------------------------
	 * For secure clients the options are to use a client cert or anonymous SSL.  In
	 * either case a single SSL context is used to create SSL sessions.  Each connection
	 * gets its own SSL session
	 * -------------------------------------------------------------------------------- */
	if (mqttbenchInfo->useSecureConnections) {
		rc = sslInit(mqttbenchInfo);

		/* ----------------------------------------------------------------------------
		 * Initialize a TLS context per IOP thread to avoid lock contention on the TLS context object.
		 * All TLS contexts are configured the same */
		g_SSLCtx = (SSL_CTX **) calloc(numIOP, sizeof(SSL_CTX *));
		if(g_SSLCtx) {
			for(int i=0; i < numIOP; i++) {
				g_SSLCtx[i] = initSSLCtx(pSysEnvSet->sslClientMethod, pSysEnvSet->sslCipher);
				if (g_SSLCtx[i] == NULL) {
					rc = RC_SSL_CONTEXT_NULL;
					break;
				}
			}
		} else {
			rc = provideAllocErrorMsg("Array of TLS context objects", (int) (numIOP * sizeof(SSL_CTX *)), __FILE__, __LINE__);
		}
	}

	return rc;
} /* tcpInit */

/* *************************************************************************************
 * startIOThreads
 *
 * Description:  Allocate and start I/O Processor and Listener threads.
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures.
 *   @param[in]  bfrSize             = Number of buffers requested at initialization of
 *                                     the buffer pool.
 *
 *   @return 0   = Successful Completion.
 *         <>0   = An error/failure occurred.
 * *************************************************************************************/
int startIOThreads (mqttbenchInfo_t *mqttbenchInfo, int bfrSize)
{
	int i = 0;
	int k = 0;
	int rc = 0;
	int numIOL = (int)mqttbenchInfo->mbSysEnvSet->numIOListenerThreads;
	int numIOP = (int)mqttbenchInfo->mbSysEnvSet->numIOProcThreads;

	mbMsgConfigInfo_t *pMsgCfgInfo = mqttbenchInfo->mbMsgCfgInfo;
	environmentSet_t  *pSysEnvSet  = mqttbenchInfo->mbSysEnvSet;
	mbCmdLineArgs_t   *cmdLineArgs = mqttbenchInfo->mbCmdLineArgs;

	/* --------------------------------------------------------------------------------
	 * Initialize the number of random_data structures for random_r based on the number of
	 * I/O Processor threads.
	 * -------------------------------------------------------------------------------- */
	ioProcRandData = (randomRStruct *)calloc(1, sizeof(randomRStruct));
	if (ioProcRandData) {
		ioProcRandData->randomIOProcData = calloc(numIOP, sizeof(struct random_data));
		if (ioProcRandData->randomIOProcData) {
			ioProcRandData->randomIOProcStateBfrs = calloc(numIOP, PRNG_BUFSZ);
			if (ioProcRandData->randomIOProcStateBfrs == NULL) {
				rc = provideAllocErrorMsg("random data state buffer",
										  (int)(numIOP * PRNG_BUFSZ),
										  __FILE__, __LINE__);
				free(ioProcRandData->randomIOProcData);
				free(ioProcRandData);
				free(ioListenerThreads);
				free(ioProcThreads);
				return rc;
			}
		} else {
			rc = provideAllocErrorMsg("random data structure",
									  (int)(sizeof(struct random_data) * numIOP),
									  __FILE__, __LINE__);
			free(ioProcRandData);
			return rc;
		}
	} else {
		rc = provideAllocErrorMsg("I/O random data structure",
								 (int)sizeof(randomRStruct),
								 __FILE__, __LINE__);
		return rc;
	}

	/* Create and Start I/O Listener threads */
	for ( i = 0 ; i < numIOL ; i++ ) {
		int affMapLen;
		char *envAffStr;
		char  affMap[64] = {0};
		char  envAffName[64];
		char  threadName[MAX_THREAD_NAME];

		snprintf(threadName, MAX_THREAD_NAME, "iol%d", i);
		snprintf(envAffName, 64, "Affinity_%s", threadName);
		envAffStr = getenv(envAffName);
		affMapLen = ism_common_parseThreadAffinity(envAffStr, affMap);

		ioListenerThread_t *iol = (ioListenerThread_t *)calloc(1, sizeof(ioListenerThread_t));
		if (iol == NULL) {
	    	rc = provideAllocErrorMsg("I/O Listener thread", (int)sizeof(ioListenerThread_t), __FILE__, __LINE__);
			return rc;
		}

		pthread_spin_init(&(iol->cleanupReqListLock),0);
		iol->efd = epoll_create(INITIAL_NUM_EPOLL_EVENTS);
		if (iol->efd == -1) {
			MBTRACE(MBERROR, 1, "The thread %s failed to create an epoll object: error(%s)\n", threadName, strerror(errno));
			return RC_EPOLL_ERROR;
		}

		ioListenerThreads[i] = iol;   /* store the I/O listener thread into the list of I/O listener threads */

		rc = ism_common_startThread(&iol->threadHandle,
				                    ioListenerThread,
				                    iol,
				                    NULL,
				                    0,
				                    ISM_TUSAGE_NORMAL,
				                    0,
				                    threadName,
				                    NULL);
		if (rc) {
			MBTRACE(MBERROR, 1, "Unable to start I/O Listener thread (%s)\n", threadName);
			return RC_IOLISTENER_FAILED;
		}

		if (affMapLen)
			ism_common_setAffinity(iol->threadHandle,affMap,affMapLen);
	} /* for ( i = 0 ; i < numIOL ; i++ ) */

	MBTRACE(MBINFO, 1, "Started %d I/O Listener threads\n", numIOL);

	/* Create and Start I/O Processor threads */
	for ( i = 0 ; i < numIOP ; i++ ) {
		int minPoolSize;
		int maxPoolSize;
		int affMapLen;
		char *envAffStr;
		char  affMap[64];
		char  envAffName[64];
		char  threadName[MAX_THREAD_NAME];

		snprintf(threadName, MAX_THREAD_NAME, "iop%d", i);
		snprintf(envAffName, 64, "Affinity_%s", threadName);
		envAffStr = getenv(envAffName);
		affMapLen = ism_common_parseThreadAffinity(envAffStr, affMap);

		ioProcThread_t * iop = (ioProcThread_t *)calloc(1, sizeof(ioProcThread_t));
		if (iop == NULL) {
	    	rc = provideAllocErrorMsg("I/O Processor thread", (int)sizeof(ioProcThread_t), __FILE__, __LINE__);
			return rc;
		}

		pthread_spin_init(&(iop->jobListLock),0);
		pthread_spin_init(&(iop->counterslock),0);
		iop->jobsList[0].used = 0;
		iop->jobsList[0].allocated = IO_PROC_THREAD_MAX_JOBS;
		iop->jobsList[0].jobs = calloc(iop->jobsList[0].allocated, sizeof(ioProcJob));
		iop->jobsList[1].used = 0;
		iop->jobsList[1].allocated = IO_PROC_THREAD_MAX_JOBS;
		iop->jobsList[1].jobs = calloc(iop->jobsList[1].allocated, sizeof(ioProcJob));
		iop->currentJobs = &(iop->jobsList[0]);
		iop->nextJobs = &(iop->jobsList[1]);

		/* Clear the message counters. */
		memset(iop->currRxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));
		memset(iop->currTxMsgCounts, 0, (sizeof(uint64_t) * MQTT_NUM_MSG_TYPES));

		iop->currRxQoSPublishCounts[0] = 0;
		iop->currRxQoSPublishCounts[1] = 0;
		iop->currRxQoSPublishCounts[2] = 0;

		/* Set the RX minPoolSize and maxPoolSIze */
		if (pMsgCfgInfo->maxMsg < MSG_SIZE_10MB) {
			minPoolSize = IO_PROC_RX_POOL_SIZE_MIN;
			maxPoolSize = IO_PROC_RX_POOL_SIZE_MAX;
		}  else {
			minPoolSize = LG_IO_PROC_RX_POOL_SIZE_MIN;
			maxPoolSize = LG_IO_PROC_RX_POOL_SIZE_MAX;
		}

		/* Create the RX Buffer Pool */
		iop->recvBuffPool = ism_common_createBufferPool(pSysEnvSet->recvBufferSize, minPoolSize, maxPoolSize,"RecvBufferPool");
		if (!(iop->recvBuffPool)) {
	    	rc = provideAllocErrorMsg("the RX Buffer Pool",
	    			                  pSysEnvSet->recvBufferSize * maxPoolSize,
									  __FILE__, __LINE__);
			return rc;
		}

		/* Set the TX minPoolSize and maxPoolSIze */
		if (pMsgCfgInfo->maxMsg < MSG_SIZE_10MB) {
			minPoolSize = IO_PROC_TX_POOL_SIZE_MIN;
			maxPoolSize = IO_PROC_TX_POOL_SIZE_MAX;
		}  else {
			minPoolSize = LG_IO_PROC_TX_POOL_SIZE_MIN;
			maxPoolSize = LG_IO_PROC_TX_POOL_SIZE_MAX;
		}

		/* If the maxTXBuffers is specified then reset the min and max based on the value. */
		if (pSysEnvSet->maxNumTXBfrs) {
			maxPoolSize = pSysEnvSet->maxNumTXBfrs;

			if (pSysEnvSet->maxNumTXBfrs < minPoolSize)
				minPoolSize = pSysEnvSet->maxNumTXBfrs;
		}

		pSysEnvSet->tx_NumAllocatedBfrs = maxPoolSize;

		/* ----------------------------------------------------------------------------
		 * Create TX Buffer Pool which initializes the pool with minPoolSize # buffers
		 * with size bfrSize.
		 * ---------------------------------------------------------------------------- */
		iop->sendBuffPool = ism_common_createBufferPool(bfrSize, minPoolSize, maxPoolSize, "SendBufferPool");
		if (!(iop->sendBuffPool)) {
	    	rc = provideAllocErrorMsg("TX buffer pools on I/O Processor thread", (bfrSize * maxPoolSize),
	    			                  __FILE__, __LINE__);
			return rc;
		}

		/* Obtain TX Buffers from the newly created TX Buffer Pool for the IOP to use. */
		iop->numBfrs = pSysEnvSet->buffersPerReq;
		iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 0);
		if (iop->txBuffer == NULL) {
	    	rc = provideAllocErrorMsg("TX buffer pools on I/O Processor thread", iop->numBfrs,
	    			                  __FILE__, __LINE__);
	    	return rc;
		}

		/* Allocate the socket error array for this IOP. */
		iop->socketErrorArray = calloc(MAX_NUM_SOCKET_ERRORS, sizeof(char *));
		if (iop->socketErrorArray) {
			for ( k = 0 ; k < MAX_NUM_SOCKET_ERRORS ; k++ ) {
				iop->socketErrorArray[k] = calloc(1, (sizeof(char) * MAX_ERROR_LINE_LENGTH));
				if (iop->socketErrorArray[k] == NULL)
			    	rc = provideAllocErrorMsg("socket error array", (int)(sizeof(char) * MAX_ERROR_LINE_LENGTH),
											  __FILE__, __LINE__);
			}
		} else {
	    	rc = provideAllocErrorMsg("socket error array", (int)(sizeof(char *) * MAX_NUM_SOCKET_ERRORS),
									  __FILE__, __LINE__);
			return rc;
		}

		if((cmdLineArgs->chkMsgMask & CHKMSGSEQNUM) > 0) { // user requested checking for ordered message delivery, allocate per IOP hash maps for streams
			iop->streamIDHashMap = ism_common_createHashMap(1024 * 1024, HASH_INT64);
			if(iop->streamIDHashMap == NULL){
				return provideAllocErrorMsg("an ismHashMap", (int) sizeof(iop->streamIDHashMap), __FILE__, __LINE__);
			}
		}

		ioProcThreads[i] = iop;   /* store the I/O processor thread into the list of I/O processor threads */

		/* random_r state buffer must be initialized to NULL prior to utilizing */
		initstate_r(random(), &ioProcRandData->randomIOProcStateBfrs[i], PRNG_BUFSZ, &ioProcRandData->randomIOProcData[i]);
		iop->ioProcRandomData = &ioProcRandData->randomIOProcData[i];

		rc = ism_common_startThread(&iop->threadHandle,
				                    ioProcessorThread,
				                    iop,
									NULL,
				                    0,
				                    ISM_TUSAGE_NORMAL,
				                    0,
				                    threadName,
				                    NULL);
		if (rc == 0) {
			if (affMapLen)
				ism_common_setAffinity(iop->threadHandle,affMap,affMapLen);
		} else {
			MBTRACE(MBERROR, 1, "Unable to start I/O Processor thread (%s)\n", threadName);
			return RC_IOPROCESSOR_FAILED;
		}
	} /* for ( i = 0 ; i < numIOP ; i++ ) */

	MBTRACE(MBINFO, 1, "Started %d I/O Processor threads\n", numIOP);

	return rc;
} /* startIOThreads */

/* *************************************************************************************
 * stopIOThreads
 *
 * Description:  Stop I/O Processor and Listener threads
 *
 *   @param[in]  mqttbenchInfo       = Structure holding pointers to various structures
 *
 *   @return     =
 * *************************************************************************************/
int stopIOThreads (mqttbenchInfo_t *mqttbenchInfo)
{
	environmentSet_t *pSysEnvSet = mqttbenchInfo->mbSysEnvSet;

	int rc = 0;
	int i = 0;
	int j = 0;
	int numIOL = (int)pSysEnvSet->numIOListenerThreads;
	int numIOP = (int)pSysEnvSet->numIOProcThreads;

	/* Stop the I/O Listener threads */
	for ( i = 0 ; i < numIOL ; i++ ) {
		ioListenerThread_t *iol = ioListenerThreads[i];
		iol->isStopped = 1;
		ism_time_t timer = NANO_PER_SECOND * 2; // wait at most 2 seconds to join the thread
		rc += ism_common_joinThreadTimed(iol->threadHandle, NULL, timer);
		if (rc)
			MBTRACE(MBERROR, 1, "Could not join I/O Listener thread (idx=%d)\n", i);
	}

	/* Stop the I/O Processor threads */
	for ( i = 0 ; i < numIOP ; i++ ) {
		ioProcThread_t *iop = ioProcThreads[i];
		iop->isStopped = 1;
		ism_time_t timer = NANO_PER_SECOND * 2; // wait at most 2 seconds to join the thread
		rc += ism_common_joinThreadTimed(iop->threadHandle, NULL, timer);
		if (rc) {
			MBTRACE(MBERROR, 1, "Could not join I/O Processor thread (idx=%d)\n", i);
		} else {
			if (iop->jobsList[0].jobs) {
				free(iop->jobsList[0].jobs);
				iop->jobsList[0].jobs = NULL;
			}
			if (iop->jobsList[1].jobs) {
				free(iop->jobsList[1].jobs);
				iop->jobsList[1].jobs = NULL;
			}

			if (iop->socketErrorArray) {
				for ( j = 0 ; j < MAX_NUM_SOCKET_ERRORS ; j++ ) {
					if (iop->socketErrorArray[j] != NULL)
						free(iop->socketErrorArray[j]);
				}

				free(iop->socketErrorArray);
			}
		} /* if (rc) */
	}

	/* Free up the random_r structure memory */
	if (ioProcRandData) {
		if (ioProcRandData->randomIOProcData)
			free(ioProcRandData->randomIOProcData);
		if (ioProcRandData->randomIOProcStateBfrs)
			free(ioProcRandData->randomIOProcStateBfrs);

		free(ioProcRandData);
	}

	return rc;
} /* stopIOThreads */

/* *************************************************************************************
 * preInitializeTransport
 *
 * Description:  Create (allocate memory), Initialize and Bind to a client's transport
 *
 *   @param[in]  client              = the mqttclient_t object to allocate/initialize the transport object for
 *   @param[in]  serverIP            = server/destination IP Address
 *   @param[in]  port                = server/destination Port
 *   @param[in]  clientIP            = client/source IP Address
 *   @param[in]  srcport             = client/source Port
 *   @param[in]  reconnectEnabled    = a flag to indicate whether this client should reconnect if the connection is closed
 *   @param[in]  line				 = line in the client list file where this client is defined
 *
 *   @return 0   = Successful Completion.
 *           2   (SKIP_PORT) - Unable to connect using the Source Port specified.
 *          >2   = An error/failure occurred.
 * *************************************************************************************/
int preInitializeTransport (mqttclient_t *client, char *serverIP, int port, char *clientIP, int srcport, int reconnectEnabled, int line) {
	int rc = 0;
	transport_t *trans;
	struct sockaddr_in clntSockAddr;
	struct sockaddr_in srvrSockAddr;

	/* Configure the server addr */
	srvrSockAddr.sin_addr.s_addr = inet_addr(serverIP);
	srvrSockAddr.sin_port = htons(port);
	srvrSockAddr.sin_family = AF_INET;

	/* Configure the client addr */
	clntSockAddr.sin_family = AF_INET;
	/* If the clientIP isn't NULL then set the Client IP in the clntSockAddr */
	if (clientIP)
		clntSockAddr.sin_addr.s_addr = inet_addr(clientIP);

	do {
		/* Allocate memory for the transport for the client. */
		trans = (transport_t *)calloc(1, sizeof(transport_t));
		if (trans == NULL) {
			rc = provideAllocErrorMsg("client transport", (int)sizeof(transport_t), __FILE__, __LINE__);
			break;
		}

		trans->serverIP = (char *)calloc(1, (strlen(serverIP) + 1));
		if (trans->serverIP == NULL) {
			rc = provideAllocErrorMsg("send buffer for client transport", (int)(strlen(serverIP) + 1), __FILE__, __LINE__);
			free(trans);
			trans = NULL;
			break;
		}

		trans->client = client;		         /* provide the transport a reference to the mqttclient */
		memcpy(trans->serverIP, serverIP, strlen(serverIP));
		trans->serverPort = port;

		/* ------------------------------------------------------------------------
		 * Copy the server(dst) IPv4 address and destination port information configured
		 * in the client list file to the server socket object
		 * ------------------------------------------------------------------------ */
		memcpy(&(trans->serverSockAddr), &srvrSockAddr, sizeof(struct sockaddr));

		if ((trans->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {		/* Create a socket object */
			MBTRACE(MBERROR, 1, "Failed to create a socket with error: %s (errno=%d)\n",
								strerror(errno), errno);
			rc = -1;
			free(trans->serverIP);
			free(trans);
			trans = NULL;
			break;
		}

		/* ------------------------------------------------------------------------
		 * In some tests we will need to reuse connections in TIME_WAIT state,
		 * must set SO_REUSEADDR socket options.
		 * ------------------------------------------------------------------------ */
		int reuse = 1;

		if (setsockopt(trans->socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
			MBTRACE(MBERROR, 1, "Unable to set the SO_REUSEADDR socket option %d: %s (%d)\n",
								reuse, strerror(errno), errno);

			rc = RC_SO_REUSEADDR_ERROR;
			close(trans->socket);
			free(trans->serverIP);
			free(trans);
			trans = NULL;
			break;
		}

		/* ------------------------------------------------------------------------
		 * Copy the client(src) IPv4 address and destination port information configured
		 * in the client list file to the server socket object
		 * ------------------------------------------------------------------------ */
		memcpy(&(trans->clientSockAddr), &clntSockAddr, sizeof(struct sockaddr_in));

		if(!g_MqttbenchInfo->mbSysEnvSet->useEphemeralPorts)
			trans->clientSockAddr.sin_port = htons(srcport); // we are choosing the source port, not the OS

		if (bind(trans->socket, &trans->clientSockAddr, sizeof(struct sockaddr_in)) < 0) {
			int err = errno;

			if (err == EADDRINUSE)
				__sync_add_and_fetch(&bfAddrInUse,1);
			else {
				if (err == EADDRNOTAVAIL)
					__sync_add_and_fetch(&bfAddrNotAvail,1);

				MBTRACE(MBERROR, 1, "Unable to bind socket to interface %s, src port=%d, %s (errno=%d). "\
									"Verify requested ip %s is valid local address.\n",
									inet_ntoa(trans->clientSockAddr.sin_addr),
									ntohs(trans->clientSockAddr.sin_port),
									strerror(errno),
									errno,
									inet_ntoa(trans->clientSockAddr.sin_addr));
			}

			close(trans->socket);
			free(trans->serverIP);
			free(trans);
			trans = NULL;
			return SKIP_PORT;
		} else {
			trans->srcPort = ntohs(trans->clientSockAddr.sin_port);
		}

		/* Set the socket for non-blocking I/O */
		int flags = fcntl(trans->socket, F_GETFL) | O_NONBLOCK;
		if (fcntl(trans->socket, F_SETFL, flags) == -1) {
			TRACE(1, "Unable to set a socket to non-blocking.");
			rc = RC_SET_NONBLOCKING_ERROR;
			close(trans->socket);
			free(trans->serverIP);
			free(trans);
			trans = NULL;
			break;
		}

		pthread_spin_init(&(trans->slock),0);

		trans->ioProcThread = ioProcThreads[client->ioProcThreadIdx]; /* assign I/O processor thread to client */
		trans->ioListenerThread = ioListenerThreads[client->ioListenerThreadIdx];  /* assign I/O listener thread to client */
		trans->state = SOCK_NEED_CONNECT;

		/* ------------------------------------------------------------------------
		 * Use the setting of the client state for Security since it was set in
		 * createMQTTClient(), which is what calls this routine.   The setting is
		 * based the setting for each client in the client list.
		 * ------------------------------------------------------------------------ */
		if (client->isSecure) {
			trans->doRead = doSecureRead;
			trans->doWrite = doSecureWrite;
		} else {
			trans->doRead = doRead;
			trans->doWrite = doWrite;
		}

		/* Set the state change callbacks */
		trans->onConnect = onConnect;
		trans->onShutdown = onShutdown;

		/* Set the reconnect flag on the trans based on global flag. */
		trans->reconnect = (uint8_t)reconnectEnabled;

		client->trans = trans;

		if(client->useWebSockets) {
			rc = initWebSockets(client, line);
			if(rc) {
				MBTRACE(MBERROR, 1, "Failed to initialize WebSockets for client at line %d\n", line);
				return rc;
			}
		}

	} while(0);

	return rc;
} /* preInitializeTransport */

/* *************************************************************************************
 * createSocket
 *
 * Description:  Re-create a socket, which was previously created.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport.
 *           1   (IOP_TRANS_SHUTDOWN) = Stop processing for this transport and perform
 *                                      a transport shutdown.
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int createSocket (transport_t *trans)
{
	int rc = IOP_TRANS_CONTINUE;

	do {
		/* Create a socket object. */
	    if ((trans->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	    	MBTRACE(MBERROR, 1, "Failed to create a socket with error: %s (errno=%d)\n",
	    						strerror(errno), errno);
	    	if (trans->reconnect == 0) {
	    		free(trans->serverIP);
	    		free(trans);
	    		trans = NULL;
		    	rc = IOP_TRANS_SHUTDOWN;
	    	} else {
	    		rc = IOP_TRANS_IGNORE;
	    	}

	    	break;
	    }

	    /* ----------------------------------------------------------------------------
	     * In some tests we will need to reuse connections in TIME_WAIT state, must set
	     * SO_REUSEADDR socket options
	     * ---------------------------------------------------------------------------- */
	    int sockOptReuse = 1;
	    if (setsockopt(trans->socket, SOL_SOCKET, SO_REUSEADDR, &sockOptReuse, sizeof(sockOptReuse)) < 0) {
	    	char displayLine[MAX_DISPLAYLINE_LEN];
	    	displayLine[0] = '\0';
	    	snprintf(displayLine, MAX_DISPLAYLINE_LEN, "Unable to set the SO_REUSEADDR socket option %d: %s (%d) %s:%d\n",
	    			                                    sockOptReuse,
	    			                                    strerror(errno),
	    			                                    errno,
	    			                                    __FILE__,
	    			                                    __LINE__);
	    	MBTRACE(MBWARN, 1, "%s", displayLine);
	    	fprintf(stdout, "(w) %s", displayLine);
	    	fflush(stdout);

	    	if (trans->reconnect == 0) {
	    		close(trans->socket);
	    		free(trans->serverIP);
	    		free(trans);
	    		trans = NULL;
	    		rc = IOP_TRANS_SHUTDOWN;
	    	} else {
	    		rc = IOP_TRANS_IGNORE;
	    	}
	    	break;
	    }

	    if(g_MqttbenchInfo->mbSysEnvSet->useEphemeralPorts) {
	    	trans->clientSockAddr.sin_port = 0; // be sure to set source port to 0, when user has requested OS ephemeral source port selection
	    	trans->srcPort = 0;
	    }

	    /* Bind the socket */
    	if (bind(trans->socket, &trans->clientSockAddr, sizeof(struct sockaddr_in)) < 0) {
    		MBTRACE(MBERROR, 1, "Unable to bind socket to interface %s; src port=%d, errno=%d (%s:%d)\n",
                                inet_ntoa(trans->clientSockAddr.sin_addr),
                                ntohs(trans->clientSockAddr.sin_port),
                                errno,
                                __FILE__,
                                __LINE__);

    		trans->bindFailures++;
    		if (trans->reconnect == 0) {
				close(trans->socket);
				free(trans->serverIP);
				free(trans);
				trans = NULL;
				rc = IOP_TRANS_SHUTDOWN;
			} else {
				rc = IOP_TRANS_IGNORE;
			}
    		break;
    	}

        /* Set the socket otherwise set to Non-Blocking. */
       	int flags = fcntl(trans->socket, F_GETFL) | O_NONBLOCK;
       	if (fcntl(trans->socket, F_SETFL, flags) == -1) {
       		MBTRACE(MBERROR, 1, "Unable to set a socket to non-blocking.");

       		if (trans->reconnect == 0) {
       			close(trans->socket);
       			free(trans->serverIP);
       			free(trans);
       			trans = NULL;
           		rc = IOP_TRANS_SHUTDOWN;
       		} else {
       			rc = IOP_TRANS_IGNORE;
       		}
       		break;
       	}
	} while(0);

	/* --------------------------------------------------------------------------------
	 * If successfully created TCP Socket then set the trans state to SOCK_NEED_CONNECT
	 * rc = IOP_TRANS_CONTINUE == 0
	 * -------------------------------------------------------------------------------- */
	if (rc == IOP_TRANS_CONTINUE)
		trans->state = SOCK_NEED_CONNECT; /* Need to create TCP Connection */

	return rc;
} /* createSocket */ /* BEAM suppression: file not closed */

/* *************************************************************************************
 * transportCleanup
 *
 * Description:  Cleanup the transport.  This will need to check if this is a reconnect
 *               since it must not clear specific pointers in order to be able to perform
 *               a reconnect.
 *
 *   @param[in]  trans               = Client transport
 * *************************************************************************************/
static void transportCleanup (transport_t *trans)
{
	pthread_spin_lock(&trans->slock);      /* Obtain the transport lock */

	if (trans->sslHandle) {
		SSL_free(trans->sslHandle);
		trans->sslHandle = NULL;
		trans->bioHandle = NULL;
	}

	if (trans->currentRecvBuff) {
		ism_common_returnBuffer(trans->currentRecvBuff,__FILE__,__LINE__);
		trans->currentRecvBuff = NULL;
	}

	if (trans->currentSendBuff) {
		ism_common_returnBuffer(trans->currentSendBuff,__FILE__,__LINE__);
		trans->currentSendBuff = NULL;
	}

	while (trans->pendingSendListHead) {
		ism_byte_buffer_t * bb = trans->pendingSendListHead;
		trans->pendingSendListHead = trans->pendingSendListHead->next;
		ism_common_returnBuffer(bb,__FILE__,__LINE__);
	}

	trans->pendingSendListTail = NULL;

	/* --------------------------------------------------------------------------------
	 * If reconnect is NOT enabled then set the ioProcThread and ioListenerThread
	 * to NULL to prevent it from trying to execute and further.
	 * -------------------------------------------------------------------------------- */
	if (trans->reconnect == 0) {
		trans->ioProcThread = NULL;
		trans->ioListenerThread = NULL;
	}

	trans->next = NULL;

	/* Close the socket and then set to -1. */
	close(trans->socket);
	trans->socket = -1;
	trans->sockInited = 0;  /* reset socket initialized flag */

	pthread_spin_unlock(&trans->slock);      /* Release the transport lock */
} /* transportCleanup */

/* *************************************************************************************
 * connectionShutdown
 *
 * Description:  Shutdown function for secure and non-secure connections.  This performs
 *               very limited amount of work in order to support reconnect.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport.
 *           1   (IOP_TRANS_SHUTDOWN) = Stop processing for this transport and perform
 *                                      a transport shutdown.
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int connectionShutdown (transport_t *trans)
{
	int rc;
	mqttclient_t *client = trans->client;

	if (!client->isSecure) {
		stopIOListening(trans);  	/* Delete the epoll events from the I/O Listener for this transport */
		return IOP_TRANS_SHUTDOWN;
	}

	if (trans->sslHandle && !SSL_in_init(trans->sslHandle)) {
		rc = SSL_shutdown(trans->sslHandle);
		/* OpenSSL docs say you should check whether the "close notify" alert was sent to peer,
		 * but in practice with Non-blocking I/O this fails and it is not worth checking or logging the
		 * error since the connection is going down, given that we don't retry the SSL_shutdown() call anyway
		 *
		 * https://www.openssl.org/docs/man1.0.2/ssl/SSL_shutdown.html
		 *
		 * */
		int err = errno;
		if (rc == 0) {
			return IOP_TRANS_CONTINUE;
		}
		if (rc < 0) {
			int ec = SSL_get_error(trans->sslHandle, rc);
			char errString[MAX_ERROR_LINE_LENGTH] = {0};
			snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to shutdown TLS session (ec=%d, errno=%d)",
													   client->clientID, client->line, ec, err);
			traceSSLError(errString, 5, __FILE__, __LINE__);
		}
	}

	stopIOListening(trans);  /* Delete the epoll events from the I/O Listener for this transport */
	return IOP_TRANS_SHUTDOWN;
} /* connectionShutdown */

/* *************************************************************************************
 * Generate the client's WebSocket handshake message and accept string to verify the server's
 * WebSocket response.
 *
 * @param[in/out]	client 		= the client object to initialize the WebSocket for
 * @param[in]       line		= line number of client in the client list file
 *
 * @return 0 = OK, otherwise it is a failure
 * *************************************************************************************/
int initWebSockets(mqttclient *client, int line){
	int rc = 0;
	transport_t *trans = client->trans;

	/* --------------------------------------------------------------------------------
	 * Randomly generate and Base64 encode the client key
	 * -------------------------------------------------------------------------------- */
	char clientKey[WS_CLIENT_KEY_LEN + 1] = {0};
	char clientKeyB64[32] = {0}; // 32 bytes is more than adequate to base64 encode a 16 byte string
	char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int charsetlen = strlen(charset);
	for(int i=0; i<WS_CLIENT_KEY_LEN; i++){
		int idx = rand() % charsetlen;
		clientKey[i] = charset[idx];
	}
	trans->wsClientKey = strdup(clientKey);

	int clientKeyB64Len = ism_common_toBase64(clientKey, clientKeyB64, strlen(clientKey));
	if (clientKeyB64Len < 0) {
		MBTRACE(MBERROR, 1, "Failed to base64 encode the WebSocket client key for client at line %d\n", line);
		return RC_BAD_CLIENTLIST;
	}

	/* --------------------------------------------------------------------------------
	 * Prepare the WebSocket Accept string to validate WS response from the server
	 * Accept string = clientKeyB64 + WS_SERVER_GUID
	 * -------------------------------------------------------------------------------- */
	char sha1Input[128] = {0};
	char sha1Digest[SHA_DIGEST_LENGTH] = {0};
	char acceptStrB64[64] = {0}; // 64 bytes is more than adequate to base64 encode a 20 byte SHA1 digest

	sprintf(sha1Input, "%s%s", clientKeyB64, WS_SERVER_GUID);
	SHA1((unsigned char *) sha1Input, strlen(sha1Input), (unsigned char *) sha1Digest);
	int acceptStrB64Len = ism_common_toBase64(sha1Digest, acceptStrB64, SHA_DIGEST_LENGTH);
	if(acceptStrB64Len < 0) {
		MBTRACE(MBERROR, 1, "Failed to base64 encode the WebSocket server accept string for client at line %d\n", line);
		return RC_BAD_CLIENTLIST;
	}
	trans->wsAcceptStr = strdup(acceptStrB64);

	/* --------------------------------------------------------------------------------
	 * Prepare the WebSocket client handshake message
	 * -------------------------------------------------------------------------------- */
	char *resource = "/";
	char *protocol = "mqtt";
	if(client->mqttVersion == MQTT_V3)
	  	protocol = "mqttv3.1";
	char handshakeMsg[1024] = {0};

    /* Create the handshake message with all the data. */
    sprintf(handshakeMsg, WEBSOCKET_CLIENT_REQUEST_HEADER, resource, client->serverIP, protocol, clientKeyB64);
    trans->wsHandshakeMsg = strdup(handshakeMsg);
    trans->wsHandshakeMsgLen = strlen(handshakeMsg);

	return rc;
} /* initWebSockets */

/* *************************************************************************************
 * verifyServerWebSocketResponse
 *
 * Description:  Verify the Server Response to the Create WebSocket.
 *
 *   @param[in]  srvrBfr             = x
 *   @param[in]  numBytes            = x
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int verifyServerWebSocketResponse (transport_t *trans, char *srvrBfr, int numBytes) {
	const char *wsAcceptMsg = "Sec-WebSocket-Accept: ";

	int rc = IOP_TRANS_CONTINUE;
	int statusCode;

	char *startPtr = srvrBfr;
	char *endPtr = srvrBfr + numBytes;

    /* Get HTTP status code */
    if (sscanf(startPtr, "HTTP/1.1 %d Switching Protocols\r\n", &statusCode) == 1) {
    	startPtr = strstr(startPtr, "\r\n") + 2;

    	/* ----------------------------------------------------------------------------
    	 * Loop thru the received tokens from the server response and verify
    	 * the client accept string matches to indicate WebSockets is established.
    	 * ---------------------------------------------------------------------------- */
        while (startPtr < endPtr && startPtr[0] != '\r' && startPtr[1] != '\n') {
            if (memcmp(startPtr, wsAcceptMsg, strlen(wsAcceptMsg)) == 0) {
            	int len;
            	char *acceptStr = NULL;
            	char *testPtr = NULL;

            	/* Move pointer past the WebSocket Accept Message to get the value. */
            	testPtr = startPtr + strlen(wsAcceptMsg);
            	/* Check to ensure there is some data. */
            	len = strstr(testPtr, "\r\n") - testPtr + 1;
            	if (len > 0) {
            		acceptStr = alloca(len);
            		if (acceptStr != NULL) {
            			memcpy(acceptStr, testPtr, (len-1));
            			acceptStr[len-1] = 0;
            			if (strcmp(acceptStr, trans->wsAcceptStr) == 0) {
            				rc = IOP_TRANS_CONTINUE;
            			} else {
            				MBTRACE(MBERROR, 1, "WebSocket Accept responses don't match.\n");
                    		rc = IOP_TRANS_IGNORE;
            			}

            			break;
            		} else {
            	    	rc = provideAllocErrorMsg("token pointer", 0, __FILE__, __LINE__);
                		break;
            		}
            	} else {
            		MBTRACE(MBERROR, 1, "There is no data from the WebSocket handshake.\n");
            		rc = IOP_TRANS_IGNORE;
            		break;
            	}
            }

            /* move start pointer to end of line */
            startPtr = strstr(startPtr, "\r\n") + 2;
        } /* while */
    } else
    	return IOP_TRANS_IGNORE;

    return rc;
} /* verifyServerWebSocketResponse */

/* *************************************************************************************
 * onWSConnect
 *
 * Description:  Verify the Server Response to the Create WebSocket.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int onWSConnect (transport_t *trans)
{
	int rc = IOP_TRANS_CONTINUE;
	int ec = 0;

    char server_buffer[1024];

	/* Initialize the 1st byte of the buffer to be read to NULL */
   	server_buffer[0] = 0;
   	mqttclient_t *client = trans->client;

   	if (client->isSecure) {
   		rc = SSL_read(trans->sslHandle, server_buffer, sizeof(server_buffer));
   		ec = SSL_get_error(trans->sslHandle, rc);

   		switch (ec) {
   			case SSL_ERROR_NONE:
   				if (rc > 32)
   					server_buffer[rc] = 0;
   				else
   					return IOP_TRANS_CONTINUE;
   				break;
   			case SSL_ERROR_WANT_READ:
   				trans->state |= READ_WANT_READ;
   				trans->state &= ~SOCK_CAN_READ;
   				trans->currentRecvBuff = NULL;
   				return IOP_TRANS_CONTINUE;
   			case SSL_ERROR_WANT_WRITE:
   				trans->state |= READ_WANT_WRITE;
   				trans->state &= ~SOCK_CAN_WRITE;
   				trans->currentRecvBuff = NULL;
   				return IOP_TRANS_CONTINUE;
   			case SSL_ERROR_ZERO_RETURN:
   			default:
   				if ((ec == SSL_ERROR_ZERO_RETURN) && SSL_get_shutdown(trans->sslHandle))
   					trans->state |= SHUTDOWN_IN_PROCESS;
   				else
   					trans->state |= SOCK_ERROR;

   				trans->currentRecvBuff = NULL;

   				if ((g_RequestedQuit == 0) && ((trans->state & SOCK_ERROR) > 0))
   					traceSSLError("SSL read failed on WebSocket handshake", 5, __FILE__, __LINE__);

   				rc = connectionShutdown(trans);
   				return IOP_TRANS_IGNORE;
   		} /* switch (ec) */
   	} else {
   		/* Verify handshake by reading the server response. */
   		rc = read(trans->socket, server_buffer, 1024);
   		if (rc > 32) {
   			server_buffer[rc] = 0;
   		} else {
   			if ((rc < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
   				return IOP_TRANS_CONTINUE;
   			} else {   /* Failed to read with error */
   				trans->currentRecvBuff = NULL;
   	   			rc = connectionShutdown(trans);
   				return IOP_TRANS_IGNORE;
   			}
   		}
    }

   	/* --------------------------------------------------------------------------------
   	 * Verify the response from the Server pertaining to the request for WebSockets by
   	 * calling verifyServerWebSocketResponse with the buffer which was NULL terminated
   	 * by the length of the data which is stored in rc.
   	 *
   	 * Return codes from verifyServerWebSocketResponse
   	 * rc == 0  - received successful WebSocket creation indicated by the server
   	 * rc != 0  - failed
   	 * -------------------------------------------------------------------------------- */
	rc = verifyServerWebSocketResponse(trans, server_buffer, rc);

	if (rc == 0) {
		/* Check if performing latency testing. */
		if ((g_LatencyMask & LAT_TCP_COMBO) > 0) {
			uint32_t latency = (uint32_t)(((g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) -
				client->tcpConnReqSubmitTime) * g_UnitsConn);
			mqttbench_t *mbinst = client->mbinst;

			if ((g_LatencyMask & CHKTIMETCPCONN) > 0) {
				volatile ioProcThread_t * iop = trans->ioProcThread;

				if (iop->tcpConnHist) {
					iop->tcpConnHist->totalSampleCt++;   /* Update Total # of Samples Received. */

					if (latency > iop->tcpConnHist->max)   /* Update max latency if applicable. */
						iop->tcpConnHist->max = latency;

					/* ----------------------------------------------------------------
					 * Check whether the latency falls within the scope of the histogram.
					 * If not, then increment the counter for larger than histogram size
					 * and update the 1 sec and 5 sec counters if applicable.
					 * ---------------------------------------------------------------- */
					if (latency < iop->tcpConnHist->histSize)
						iop->tcpConnHist->histogram[latency]++;  /* Update histogram since within scope. */
					else {
						iop->tcpConnHist->big++;   /* Update larger than histogram. */

						if (latency >= g_Equiv1Sec_Conn) {  /* Update if larger than 1 sec. */
							iop->tcpConnHist->count1Sec++;
							if (latency >= (g_Equiv1Sec_Conn * 5))  /* Update if larger than 5 sec. */
								iop->tcpConnHist->count5Sec++;
						}
					}
				}
			} else
				client->tcpLatency = latency;

			__sync_add_and_fetch(&(mbinst->numTCPConnects),1);
			if (mbinst->numTCPConnects == mbinst->numClients) {
				mbinst->tcpConnTime = getCurrTime() - mbinst->startTCPConnTime;
			}
		}

		if(client->traceEnabled || g_MBTraceLevel == 9) {
			MBTRACE(MBDEBUG, 5, "%s client @ line%d, ID=%s has completed the WebSocket handshake\n",
								"WS CONN:", client->line, client->clientID);
		}

		/* ----------------------------------------------------------------------------
		 * If successfully verified the Server WebSocket Response then turn off the
		 * trans->state for WebSocket in process.
		 * ---------------------------------------------------------------------------- */
	    trans->state &= ~SOCK_WS_IN_PROCESS;

	    if (g_TCPConnLatOnly == 0)
			rc = submitMQTTConnect(trans);
	} else
		MBTRACE(MBERROR, 1, "Verify server response failed (rc: %d).\n", rc);

    return rc;
} /* onWSConnect */

/* *************************************************************************************
 * createWebSocket
 *
 * Description:  Create a WebSocket Connection to the ISM server.  This is performed
 *               after getting TCP Connection and SSL (if requested connection type).
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int createWebSocket (transport_t *trans)
{
    int rc;
    int ec;

    if (trans->client->isSecure) {
    	rc = SSL_write(trans->sslHandle, trans->wsHandshakeMsg, trans->wsHandshakeMsgLen);
    	ec = SSL_get_error(trans->sslHandle, rc);

        switch (ec) {
        	case SSL_ERROR_NONE:
        		break;
        	case SSL_ERROR_WANT_READ:
        		trans->state |= WRITE_WANT_READ;
        		trans->state &= ~SOCK_CAN_READ;
        		return IOP_TRANS_CONTINUE;
        	case SSL_ERROR_WANT_WRITE:
        		trans->state |= WRITE_WANT_WRITE;
        		trans->state &= ~SOCK_CAN_WRITE;
        		return IOP_TRANS_CONTINUE;
        	case SSL_ERROR_ZERO_RETURN:
        	default:
        		if ((ec == SSL_ERROR_ZERO_RETURN) && SSL_get_shutdown(trans->sslHandle))
        			trans->state |= SHUTDOWN_IN_PROCESS;
        		else
        			trans->state |= SOCK_ERROR;

        		if ((g_RequestedQuit == 0) && (trans->state & SOCK_ERROR))
        			traceSSLError("SSL write failed on WebSocket handshake", 5, __FILE__, __LINE__);

        		rc = connectionShutdown(trans);
        		return IOP_TRANS_IGNORE;
        } /* switch (ec) */
    } else {
    	rc = write(trans->socket, trans->wsHandshakeMsg, trans->wsHandshakeMsgLen);
    	if (rc < 0) {
    		MBTRACE(MBERROR, 1, "WebSocket :: write failed: %s\n", strerror(errno));
    		close(trans->socket);
    		return IOP_TRANS_IGNORE;
    	}
    }

   	trans->state |= SOCK_WS_IN_PROCESS;
   	trans->onWSConnect = onWSConnect;

	return IOP_TRANS_CONTINUE;
} /* createWebSocket */

/* *************************************************************************************
 * psk_client_cb
 *
 * Description:  The callback for the PreShared Key.
 *
 *   @param[in]  ssl                 = x
 *   @param[in]  hint                = x
 *   @param[in]  identity            = x
 *   @param[in]  max_identity_len    = x
 *   @param[in]  psk                 = x
 *   @param[in]  max_psk_len         = x
 *
 *   @return 0   =
 * *************************************************************************************/
static unsigned int psk_client_cb (SSL *ssl,
		                           const char *hint,
		                           char *identity,
		                           unsigned int max_identity_len,
		                           unsigned char *psk,
		                           unsigned int max_psk_len)
{
	static int warned = 0;

	mqttclient_t *client;
	transport_t *trans;

	if (!hint) {
		/* No ServerKeyExchange message was received from the server */
		if (!warned) {
			MBTRACE(MBDEBUG, 6, "No PSK identity hint was provided by the server\n");
			warned = 1;
		}
	}

	/* --------------------------------------------------------------------------------
	 * Get the app data that was set when setting the callback so that the client
	 * information can be obtained to complete the callback call.
	 * -------------------------------------------------------------------------------- */
	trans = (transport_t *)(SSL_get_app_data(ssl));
	client = trans->client;

	/* If the Client PSK ID length is > max_identity_len then need to exit. */
	if (client->psk_id_len > max_identity_len) {
		MBTRACE(MBERROR, 1, "PSK ID length (%d) > max identity len (%d).\n", client->psk_id_len, max_identity_len);
		return 0;
	}

	/* String copy the ID to the passed in variable (identity). */
	strcpy(identity, client->psk_id);

	/* If the Client PSK Key length is > max_key_len then need to exit. */
	if (client->psk_key_len > max_psk_len) {
		MBTRACE(MBERROR, 1, "PSK Key length (%d) > max key len (%d).\n", client->psk_key_len, max_psk_len);
		return 0;
	}

	/* Copy the client key to the passed in key (psk) via memcpy. */
	memcpy(psk, client->psk_key, client->psk_key_len);

	/* Return the key length. */
	return client->psk_key_len;
} /* psk_client_cb */

/* *************************************************************************************
 * sslConnect
 *
 * Description:  Performs the SSL Connection for a secured connection.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int sslConnect (transport_t *trans)
{
	int rc = IOP_TRANS_CONTINUE;
	int ec;
	int iopIdx = trans->client->ioProcThreadIdx;

	static volatile int firstTLSHandshake = 0;
	environmentSet_t *pSysEnvSet = g_MqttbenchInfo->mbSysEnvSet;
	mqttclient_t *client = trans->client;
	char errString[MAX_ERROR_LINE_LENGTH] = {0};

	if (trans->sslHandle == NULL) {
		trans->sslHandle = SSL_new(g_SSLCtx[iopIdx]);
		if (trans->sslHandle == NULL) {
			snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to create a TLS session", client->clientID, client->line);
			traceSSLError(errString, 5, __FILE__, __LINE__);
			return IOP_TRANS_IGNORE;
		}

		/* For clients which have a client certificate to send */
		if (trans->ccert) {
			/* ------------------------------------------------------------------------
			 * Load x509 Certificate into client SSL Session  The ccert is a
			 * pointer to X509 * Certificate Object (stored in global cert map
			 * ------------------------------------------------------------------------ */
			rc = SSL_use_certificate(trans->sslHandle, trans->ccert);
			if (rc != 1) {
				snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to load its client certificate", client->clientID, client->line);
				traceSSLError(errString, 5, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			}

			/* ------------------------------------------------------------------------
			 * Load private key into client SSL Session.  The cpkey is a pointer to
			 * EVP_PKEY * private key object (stored in global private key map)
			 * ------------------------------------------------------------------------ */
			rc = SSL_use_PrivateKey(trans->sslHandle, trans->cpkey);
			if (rc != 1) {
				snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to load its client private key", client->clientID, client->line);
				traceSSLError(errString, 5, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			}

			/* ------------------------------------------------------------------------
			 * Verify the consistency of the x509 and private key loaded into the
			 * client SSL session.
			 * ------------------------------------------------------------------------ */
			rc = SSL_check_private_key(trans->sslHandle);
			if (rc != 1) {
				snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to verify the consistency of the client certificate and key", client->clientID, client->line);
				traceSSLError(errString, 5, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			}
		}

		if (client->server) {
			if (!SSL_set_tlsext_host_name(trans->sslHandle, client->server)) {
				snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to set the SNI indicator on the TLS session", client->clientID, client->line);
				traceSSLError(errString, 5, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			}
		}

		/* ----------------------------------------------------------------------------
		 * Need to create a BIO for handling Certificates.
		 *
		 * NOTE::  BIO_CLOSE fails to allow reconnect.
		 * ---------------------------------------------------------------------------- */
		trans->bioHandle = BIO_new_socket(trans->socket, BIO_NOCLOSE);
		if (trans->bioHandle) {
			SSL_set_bio(trans->sslHandle, trans->bioHandle, trans->bioHandle);  /* use same BIO handle for read and write */
			SSL_set_app_data(trans->sslHandle, (char *)trans);
		} else {
			snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to allocate a BIO handle for reads/writes", client->clientID, client->line);
			traceSSLError(errString, 5, __FILE__, __LINE__);
			return IOP_TRANS_IGNORE;
		}
	}

	rc = SSL_connect(trans->sslHandle);
	if (rc == 0) {
		client->tlsErrorCtr++; /* Increment special TLS Error Counter */
		if(client->traceEnabled || g_MBTraceLevel == 9) {
			snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d), the server closed the connection during TLS handshake", client->clientID, client->line);
			traceSSLError(errString, 5, __FILE__, __LINE__);
		}
		return IOP_TRANS_IGNORE;
	}
	if (rc < 0) {
		ec = SSL_get_error(trans->sslHandle, rc);
		switch (ec) {
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				return IOP_TRANS_CONTINUE_TLS;
			default:
			{
				client->tlsErrorCtr++; /* Increment special TLS Error Counter */
				snprintf(errString, MAX_ERROR_LINE_LENGTH, "Client %s (line=%d) failed to complete TLS handshake", client->clientID, client->line);
				traceSSLError(errString, 5, __FILE__, __LINE__);
				trans->state |= SOCK_ERROR;
				return IOP_TRANS_IGNORE;
			}
		} /* switch (ec) */
	} else {
		/* Completed the TLS handshake */
		client->currRetryDelayTime_ns = client->initRetryDelayTime_ns;
		rc = SSL_get_verify_result(trans->sslHandle);
		if ((rc != X509_V_OK) && (rc != X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)) {
			MBTRACE(MBERROR, 1, "Unable to verify the server certificate for MQTT server at %s:%d, client at line %d in the client list file (SSL error: %s). "\
					            "Check the following trust store locations (%s, %s). Check file permissions.\n",
		            			trans->client->server, trans->client->serverPort, trans->client->line, X509_verify_cert_error_string(rc), X509_get_default_cert_dir(), "certs/");
			g_MBErrorRC = RC_SSL_TRUSTSTORE_ERROR;
			if(g_thrdHandleDoCommands)
				ism_common_cancelThread(g_thrdHandleDoCommands);  // interrupt the read from the interactive command line thread
			g_RequestedQuit = 1; // this is a fatal error, we should stop the test at this point and let the user know there trust store needs updating
			return g_MBErrorRC;
		}
	}

	if (!firstTLSHandshake || client->traceEnabled || g_MBTraceLevel == 9) {
		firstTLSHandshake = 1;
		const char *sslReturn = SSL_get_cipher(trans->sslHandle);
		if(client->traceEnabled || g_MBTraceLevel == 9) {
			MBTRACE(MBDEBUG, 5, "%10s client @ line=%d, ID=%s has completed the TLS handshake: requested cipher=%s, negotiated cipher=%s\n",
								"TLS CONN:", client->line, client->clientID, pSysEnvSet->sslCipher, sslReturn);
		} else {
			MBTRACE(MBINFO, 5, "Requested/Default TLS Cipher: %s\n", pSysEnvSet->sslCipher);
			MBTRACE(MBINFO, 5, "Negotiated TLS Cipher: %s (server %s)\n", sslReturn, client->server);
		}

		if(pSysEnvSet->negotiated_sslCipher) {
			char *ref = pSysEnvSet->negotiated_sslCipher;
			pSysEnvSet->negotiated_sslCipher = strdup(sslReturn);
			free(ref);
		} else {
			pSysEnvSet->negotiated_sslCipher = strdup(sslReturn);
		}
	}

	/* --------------------------------------------------------------------------------
	 * Set the transport state to be SOCK_CONNECTED, SOCK_CAN_READ and
	 * SOCK_CAN_WRITE to indicate the secure connection has been established.
	 * -------------------------------------------------------------------------------- */
	trans->state = (SOCK_CONNECTED | SOCK_CAN_READ | SOCK_CAN_WRITE);
	client->tlsConnCtr++;  /* Increment special TLS Connection Counter */

	/* --------------------------------------------------------------------------------
	 * If measuring TCP Connection then time to get the t2 for the latency and
	 * update the histogram.
	 * -------------------------------------------------------------------------------- */
	if ((g_LatencyMask & LAT_TCP_COMBO) > 0) {
		/* ----------------------------------------------------------------------------
		 * Only collect TCP Connection latency if WebSockets were not requested.  If
		 * WebSockets were requested then the TCP Connection latency will be collected
		 * in onWSConnect()
		 * ---------------------------------------------------------------------------- */
		uint32_t latency = (uint32_t)(((g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) - client->tcpConnReqSubmitTime) * g_UnitsConn);
		mqttbench_t * mbinst = trans->client->mbinst;

		if ((g_LatencyMask & CHKTIMETCPCONN) > 0) {
			volatile ioProcThread_t * iop = trans->ioProcThread;

			if (iop->tcpConnHist) {
				iop->tcpConnHist->totalSampleCt++;   /* Update Total # of Samples Received. */

				if (latency > iop->tcpConnHist->max)   /* Update max latency if applicable. */
					iop->tcpConnHist->max = latency;

				/* --------------------------------------------------------------------
				 * Check whether the latency falls within the scope of the histogram.
				 * If not, then increment the counter for larger than histogram size
				 * and update the 1 sec and 5 sec counters if applicable.
				 * -------------------------------------------------------------------- */
				if (latency < iop->tcpConnHist->histSize) {
					iop->tcpConnHist->histogram[latency]++;  /* Update histogram since within scope. */
				} else {
					iop->tcpConnHist->big++;   /* Update larger than histogram. */

					if (latency >= g_Equiv1Sec_Conn) {  /* Update if larger than 1 sec. */
						iop->tcpConnHist->count1Sec++;
						if (latency >= (g_Equiv1Sec_Conn * 5))  /* Update if larger than 5 sec. */
							iop->tcpConnHist->count5Sec++;
					}
				}
			} else {
				return IOP_TRANS_IGNORE;
			}
		} else {
			trans->client->tcpLatency = latency;
		}

		__sync_add_and_fetch(&(mbinst->numTCPConnects),1);
		if (mbinst->numTCPConnects == mbinst->numClients) {
			mbinst->tcpConnTime = getCurrTime() - mbinst->startTCPConnTime;
		}
	} else {
		/* ----------------------------------------------------------------------------
		 * If reconnection is enabled then reset 2 client variables:
		 *     - connection retries count
		 *     - current connection delay
		 * ---------------------------------------------------------------------------- */
		if (trans->reconnect) {
			client->connRetries = 0;
			client->currRetryDelayTime_ns = client->initRetryDelayTime_ns;
		}
	}

	/* --------------------------------------------------------------------------------
	 * Check to see if performing TCP connection test only.  If not then perform either
	 * a creation of a WebSocket or perform a MQTT Connect.
	 * -------------------------------------------------------------------------------- */
	if (g_TCPConnLatOnly == 0) {
		if (trans->client->useWebSockets) {
			rc = createWebSocket(trans);
			if (rc) {
				MBTRACE(MBERROR, 1, "Client %s (line=%d) is unable to create a WebSocket (rc: %d).\n",
						             client->clientID, client->line, rc);
				exit (1);
			}
		} else
			rc = submitMQTTConnect(trans);
	}

	return IOP_TRANS_CONTINUE;
} /* sslConnect */

/* *************************************************************************************
 * createConnection
 *
 * Description:  Called by processIORequest to create a TCP connection to the ISM server
 *
 *   @param[in]  client              = Specific mqttclient to be use.
 *
 *   @return RC_SUCCESSFUL 	 		 = Successfully created a connection (or "in-progress" EINPROGRESS)
 *           (non-zero)  			 = Failed to create a connection
 * *************************************************************************************/
int createConnection (mqttclient_t *client)
{
	int rc = RC_SUCCESSFUL;
	transport_t *trans = client->trans;
	client->tcpConnReqSubmitTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

	if ((g_LatencyMask & LAT_TCP_COMBO) > 0) {
		pthread_spin_lock(&connTime_lock);       /* Lock the spinlock */
		if (client->mbinst->startTCPConnTime == 0) {
			client->mbinst->startTCPConnTime = getCurrTime();
			g_StartTimeConnTest = client->mbinst->startTCPConnTime;
		}
		pthread_spin_unlock(&connTime_lock);     /* Unlock the spinlock */
	} else if ((g_LatencyMask & (CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE)) > 0) {
		/* If requested TCP Connection, MQTT Connection or Subscription time, set the timer. */
		pthread_spin_lock(&connTime_lock);       /* Lock the spinlock */
		if (g_StartTimeConnTest == 0.0)
			g_StartTimeConnTest = getCurrTime();
		pthread_spin_unlock(&connTime_lock);     /* Unlock the spinlock */
	}

	/* --------------------------------------------------------------------------------
	 * non-blocking connect (see initializeTransport)
	 * connect returns -1 for non-blocking and 0 for blocking
	 * -------------------------------------------------------------------------------- */
	rc = connect(trans->socket, &(trans->serverSockAddr), sizeof(struct sockaddr_in)); /* establish a TCP connection (send TCP_SYN) */
	int connectErr = errno;

	trans->client->tcpConnCtr++;  /* increment TCP Connection Counter */

	if(client->traceEnabled || g_MBTraceLevel == 9) {
		MBTRACE(MBDEBUG, 5, "%10s client %s (line=%d) initiated a TCP connect from %s to %s:%d, rc=%d and error=%s (errno=%d)\n",
							"TCP CONN:", client->clientID, client->line, client->srcIP, trans->serverIP, trans->serverPort, rc, strerror(connectErr), connectErr);
	}

	if (rc < 0 && connectErr != EINPROGRESS) { /* EINPROGRESS = connection in progress and connection should move to HANDSHAKE_IN_PROCESS state (in addTransport)*/
		client->connRetries++;
		__sync_add_and_fetch(&(client->mbinst->connRetries),1);
		if (connectErr == EADDRNOTAVAIL)
			__sync_add_and_fetch(&cfAddrNotAvail,1);

		if (client->connRetries >= g_MaxRetryAttempts) {
			if (client->stopLogging == 0) {
				volatile ioProcThread_t * iop = trans->ioProcThread;
				char errString[MAX_ERROR_LINE_LENGTH] = {0};

				snprintf(errString, MAX_ERROR_LINE_LENGTH,
								   "Transport (fd=%d, state=0x%X) for client %s exceeded %d max number of connection attempts " \
								   "to server %s:%d with rc=%d and error=%s (errno=%d)",
								   trans->socket,
								   trans->state,
								   client->clientID,
								   g_MaxRetryAttempts,
								   trans->serverIP,
								   trans->serverPort,
								   rc,
								   strerror(connectErr),
								   connectErr);

				MBTRACE(MBERROR, 1, "%s\n", errString);

				/* Store the socket error in the IOP socket error array. */
				iop->socketErrorArray[iop->nextErrorElement][0] = '\0';
				strcpy(iop->socketErrorArray[iop->nextErrorElement++], errString);
				/* If reached max element reset to zero */
				if (iop->nextErrorElement == MAX_NUM_SOCKET_ERRORS)
					iop->nextErrorElement = 0;

				client->stopLogging = 1;
			}
		}

		static uint64_t connAttempts = 0;
		if ((__sync_add_and_fetch(&connAttempts,1) % 1000) == 999) /* add a small delay every 1000 connection attempts to throttle a bit */
			ism_common_sleep(10);

		trans->state |= SOCK_ERROR;
		return connectErr;
	}

	/* --------------------------------------------------------------------------------
	 * connect call was successful (or "in-progress"), add the transport for this client
	 * to the target I/O listener and I/O processor threads.
	 * -------------------------------------------------------------------------------- */
	rc = addTransport(ioListenerThreads[client->ioListenerThreadIdx], ioProcThreads[client->ioProcThreadIdx], trans);
	client->stopLogging = 0; /* re-enable connect() logging for this client, since is now connecting again */
	return rc;
} /* createConnection */

/* *************************************************************************************
 * doSecureWrite
 *
 * Description:  Send data for secure connections.
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  bb                  = Byte Buffer containing data to be written to a
 *                                     transport socket.
 *   @param[in]  numBytesToWrite     = Number of bytes to be copied from the byte buffer
 *                                     and written to the transport SSL Handle.
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 * *************************************************************************************/
HOT int doSecureWrite (transport_t *trans, ism_byte_buffer_t *bb, int numBytesToWrite)
{
    int rc = SSL_write(trans->sslHandle, bb->getPtr, numBytesToWrite);
    int ec = (rc > 0) ? SSL_ERROR_NONE : SSL_get_error(trans->sslHandle, rc);
    if(trans->client && (trans->client->traceEnabled || g_MBTraceLevel == 9)) {
    	MBTRACE(MBDEBUG, 5, "%10s client @ line=%d, ID=%s SSL_write of %d bytes returned with rc=%d and ec=%d\n",
    			            "WRITE:", trans->client->line, trans->client->clientID, numBytesToWrite, rc, ec);
    }
    switch (ec) {
    	case SSL_ERROR_NONE:
    		bb->getPtr += rc;
    		bb->used -= rc;
    		if (bb->used == 0) {
    			bb->putPtr = bb->buf;
    			bb->getPtr = bb->buf;
    		}
    		return IOP_TRANS_CONTINUE;
    	case SSL_ERROR_WANT_READ:
    		trans->state |= WRITE_WANT_READ;
    		trans->state &= ~SOCK_CAN_READ;
    		return IOP_TRANS_CONTINUE;
    	case SSL_ERROR_WANT_WRITE:
    		trans->state |= WRITE_WANT_WRITE;
    		trans->state &= ~SOCK_CAN_WRITE;
    		return IOP_TRANS_CONTINUE;
    	default:
    		// Error should be discovered by next read
    		trans->state |= SOCK_CAN_READ;
    		return IOP_TRANS_CONTINUE;
    } /* switch (ec) */
} /* doSecureWrite */

/* *************************************************************************************
 * doWrite
 *
 * Description:  Send data for non-secure connections
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  bb                  = Byte Buffer containing data to be written to a
 *                                     transport socket.
 *   @param[in]  numBytesToWrite     = Number of bytes to be copied from the byte buffer
 *                                     and written to the transport socket.
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 * *************************************************************************************/
int doWrite (transport_t *trans, ism_byte_buffer_t *bb, int numBytesToWrite)
{
	int rc = write(trans->socket, bb->getPtr, numBytesToWrite);
	if(trans->client && (trans->client->traceEnabled || g_MBTraceLevel == 9)) {
		MBTRACE(MBDEBUG, 5, "%10s client @ line=%d, ID=%s write of %d bytes returned with rc=%d\n",
				            "WRITE:", trans->client->line, trans->client->clientID, numBytesToWrite, rc);
	}
	if (rc > 0) {
		bb->used -= rc;
		bb->getPtr += rc;
		if (bb->used == 0) {
			bb->putPtr = bb->buf;
			bb->getPtr = bb->buf;
		}
		return IOP_TRANS_CONTINUE;
	}

	if ((rc <= 0) && (errno == EAGAIN)) {
		trans->state |= WRITE_WANT_WRITE;
		trans->state &= ~SOCK_CAN_WRITE;
		return IOP_TRANS_CONTINUE;
	} else {
		MBTRACE(MBERROR, 1, "Failed to write errno=%d, %s:%d\n", errno, __FILE__, __LINE__);
		trans->currentRecvBuff = NULL;

		/* Error should be discovered by next read. */
		trans->state &= ~SOCK_CAN_WRITE;
		trans->state |= SOCK_CAN_READ;
		return IOP_TRANS_CONTINUE;
	}
} /* doWrite */

/* *************************************************************************************
 * onConnect
 *
 * Description:  Callback which is invoked when the SSL pr Non-SSL Handshake is completed
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *          -1   (IOP_TRANS_IGNORE)  = Remove this transport from the joblist queue,
 *                                     and wait for event.
 *************************************************************************************/
int onConnect (transport *trans)
{
	static int NagleMsgDisplayed = 0;

	int rc = IOP_TRANS_CONTINUE;   /* rc = 0 */
	int sock = trans->socket;
	int soerror = 0;
	int optFlags = 1;
	socklen_t sockoptlen = sizeof(int);
	mqttclient_t *client = trans->client;

	environmentSet_t *pSysEnvSet = g_MqttbenchInfo->mbSysEnvSet;

	/* onConnect can be called multiple times. For example, in the case of TLS connections. As such, check
	 * if the socket has already been initialized to avoid set socket options more than once.  sockInited is reset
	 * during transport cleanup */
	if(!trans->sockInited) {
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)&soerror, &sockoptlen) == 0) {
			if (soerror) {
				MBTRACE(MBERROR, 1,	"Client %s (line=%d) failed to connect to server %s:%d with error: %s (errno=%d)\n",
									client->clientID, client->line, client->serverIP,
									client->serverPort, strerror(soerror), soerror);

				trans->state = SOCK_ERROR;
				return IOP_TRANS_IGNORE;
			} else {
				/* ------------------------------------------------------------------------
				 * Connection was established successfully, fall through to setting socket
				 * options on the connection.
				 *
				 * Obtain the assigned source port by the OS ephemeral source port selection
				 * algorithm and store it in the transport_t object.
				 * ------------------------------------------------------------------------ */
				socklen_t inaddr_len = sizeof(struct sockaddr_in);
				struct sockaddr_in sockAddr;
				if (getsockname(sock, (struct sockaddr *) &sockAddr, &inaddr_len) == -1) {
					MBTRACE(MBERROR, 1,	"Unable to get the source interface and port information client at line %d in client list file (errno=%d)\n",
										client->line, errno);

					trans->state = SOCK_ERROR;
					return IOP_TRANS_IGNORE;
				} else {
					trans->srcPort = ntohs(sockAddr.sin_port);
				}
			}
		} else {
			int err = errno;
			MBTRACE(MBERROR, 1, "Could not read SO_ERROR socket option for transport %p:  sock: %d  errno=%d\n", trans, sock, err);

			trans->state = SOCK_ERROR;
			return IOP_TRANS_IGNORE;
		}

		/* Set socket options */
		if (pSysEnvSet->recvSockBuffer > 0) { /* Set the receive buffer */
			int bufsize = pSysEnvSet->recvSockBuffer;
			if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
				MBTRACE(MBERROR, 1, "Unable to set the tcp receive buffer size to %d: %s (%d)\n", bufsize, strerror(errno), errno);

				trans->state = SOCK_ERROR;
				return IOP_TRANS_IGNORE;
			}
		}

		if (pSysEnvSet->sendSockBuffer > 0) { /* Set the send buffer size */
			int bufsize = pSysEnvSet->sendSockBuffer;
			if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0) {
				MBTRACE(MBERROR, 1, "Unable to set the tcp send buffer size to %d: %s (%d)\n", bufsize, strerror(errno), errno);

				trans->state = SOCK_ERROR;
				return IOP_TRANS_IGNORE;
			}
		}

		/* --------------------------------------------------------------------------------
		 * Set TCP_NODELAY option based on environment variable
		 * -------------------------------------------------------------------------------- */
		if (pSysEnvSet->useNagle == 0) {
			if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&optFlags, sizeof(optFlags))) {
				MBTRACE(MBERROR, 1, "Unable to set TCP nodelay.\n");

				trans->state = SOCK_ERROR;
				return IOP_TRANS_IGNORE;
			} else {
				if (NagleMsgDisplayed == 0) {
					NagleMsgDisplayed++;
					MBTRACE(MBDEBUG, 7, "Nagle's Algorithm has been disabled.\n");
				}
			}
		}

		/* Enable TCP keep alive */
		if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&optFlags, sizeof(optFlags))) {
			MBTRACE(MBERROR, 1, "Unable to enable TCP keep-alive\n");

			trans->state = SOCK_ERROR;
			return IOP_TRANS_IGNORE;
		}

		trans->sockInited = 1;  /* mark the socket initialized */
	}

	/* --------------------------------------------------------------------------------
	 * Check to see if the connection is using a secured environment.  If so then call
	 * sslConnect to handle.
	 * -------------------------------------------------------------------------------- */
	if (client->isSecure) {
		rc = sslConnect(trans);
	} else {
		trans->state = (SOCK_CONNECTED | SOCK_CAN_READ | SOCK_CAN_WRITE);
		mqttbench_t * mbinst = client->mbinst;

		if(client->traceEnabled || g_MBTraceLevel == 9) {
			MBTRACE(MBDEBUG, 5, "%10s client @ line=%d, ID=%s has completed the TCP handshake\n",
					            "TCP CONN:", client->line, client->clientID);
		}

		/* ----------------------------------------------------------------------------
		 * If measuring TCP Connection and not secure connection then time to get
		 * the t2 for the latency and update the histogram.
		 * ---------------------------------------------------------------------------- */
		if ((g_LatencyMask & LAT_TCP_COMBO) > 0) {
			/* ------------------------------------------------------------------------
			 * Only collect TCP Connection latency if WebSockets were not requested.
			 * If WebSockets were requested then the TCP Connection latency will be
			 * collected in onWSConnect()
			 * ------------------------------------------------------------------------ */
			if (client->useWebSockets == 0) {
				uint32_t latency = (uint32_t)(((g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime()) -
					client->tcpConnReqSubmitTime) * g_UnitsConn);

				if ((g_LatencyMask & CHKTIMETCPCONN) > 0) {
					volatile ioProcThread_t * iop = trans->ioProcThread;

					if (iop->tcpConnHist) {
						iop->tcpConnHist->totalSampleCt++;   /* Update Total # of Samples Received. */

						if (latency > iop->tcpConnHist->max)   /* Update max latency if applicable. */
							iop->tcpConnHist->max = latency;

						/* ------------------------------------------------------------
						 * Check whether the latency falls within the scope of the
						 * histogram.  If not, then increment the counter for larger
						 * than histogram size and update the 1 sec and 5 sec counters
						 * if applicable.
						 * ------------------------------------------------------------ */
						if (latency < iop->tcpConnHist->histSize)
							iop->tcpConnHist->histogram[latency]++;  /* Update histogram since within scope. */
						else {
							iop->tcpConnHist->big++;   /* Update larger than histogram. */

							if (latency >= g_Equiv1Sec_Conn) {  /* Update if larger than 1 sec. */
								iop->tcpConnHist->count1Sec++;
								if (latency >= (g_Equiv1Sec_Conn * 5))  /* Update if larger than 5 sec. */
									iop->tcpConnHist->count5Sec++;
							}
						}
					}
				} else
					client->tcpLatency = latency;

				__sync_add_and_fetch(&(mbinst->numTCPConnects),1);
				if (mbinst->numTCPConnects == mbinst->numClients)
					mbinst->tcpConnTime = getCurrTime() - mbinst->startTCPConnTime;
			}
		} else {
			/* ------------------------------------------------------------------------
			 *  If reconnection is enabled then reset 2 client variables:
			 *     - connection retries count
			 *     - current connection delay
			 * ------------------------------------------------------------------------ */
			if (trans->reconnect) {
				client->connRetries = 0;
				client->currRetryDelayTime_ns = client->initRetryDelayTime_ns;
			}
		}

		if (client->useWebSockets) {
			rc = createWebSocket(trans);
			if (rc) {
				MBTRACE(MBERROR, 1, "Unable to create a WebSocket (rc: %d).\n", rc);
				exit (1);
			}
		} else
			rc = submitMQTTConnect(trans);
	} /* if (trans->isSecure) */

	return rc;
} /* onConnect */

/* *************************************************************************************
 * submitMQTTConnect
 *
 * Description:  Create a MQTTConnect message for a specific transport and submit it to
 *               the IOP assigned to handle this transport.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   =
 * *************************************************************************************/
int submitMQTTConnect(transport *trans)
{
	int rc = 0;

	mqttclient_t *client = trans->client;
	mqttbench_t *mbinst = client->mbinst;
	ism_byte_buffer_t *txBuf;
	volatile ioProcThread_t *iop = trans->ioProcThread;

	/* --------------------------------------------------------------------------------
	 * Obtain a TX Buffer from the buffer pool to send the MQTTConnect message.  If
	 * there aren't any available then the getBuffer will perform a pthread_spin_lock
	 * until one is available.
	 * -------------------------------------------------------------------------------- */
	txBuf = iop->txBuffer;
	if (txBuf == NULL) {
		iop->txBuffer = getIOPTxBuffers(iop->sendBuffPool, iop->numBfrs, 1);
		txBuf = iop->txBuffer;

#ifdef _DEBUG_BUFFERS
		assert(client->trans->ioProcThread == ioProcThreads[client->ioProcThreadIdx]);
//		assert(txBuf->used == 0);
#endif /* _DEBUG_BUFFERS */
	}

	iop->txBuffer = txBuf->next;

	/* Need to send the MQTT Connect Message for this client to the server. */
	createMQTTMessageConnect(txBuf, client, client->mqttVersion);

	/* --------------------------------------------------------------------------------
	 * If measuring MQTT Connection latency then take t1 timestamp and store
	 * in the client.
 	 * -------------------------------------------------------------------------------- */
	client->mqttConnReqSubmitTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
	if ((g_LatencyMask & (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE)) > 0) {
		if (mbinst->startMQTTConnTime == 0)
			mbinst->startMQTTConnTime = client->mqttConnReqSubmitTime;
	}

	if(client->traceEnabled || g_MBTraceLevel == 9) {
		char obuf[256] = {0};
		sprintf(obuf, "DEBUG: %10s client @ line=%d, ID=%s submitting an MQTT CONNECT packet: MQTT version=%d",
				      "MQTT CONN:", client->line, client->clientID, client->mqttVersion);
		traceDataFunction(obuf, 0, __FILE__, __LINE__, txBuf->buf, txBuf->used, 512);
	}
	client->protocolState = MQTT_CONNECT_IN_PROCESS;

	/* Submit job to I/O Thread and check return code. */
	rc = submitIOJob(trans, txBuf);
	if (rc == 0) {
		client->currTxMsgCounts[CONNECT]++;
		iop->currConsConnectCounts++;
		__sync_add_and_fetch(&(client->mbinst->currConnectMsgCount),1);
	} else {
		MBTRACE(MBERROR, 1, "Failure sending MQTT Connection message (rc: %d).\n", rc);
		client->protocolState = MQTT_UNKNOWN;
	}

	return rc;
} /* submitMQTTConnect */

/* *************************************************************************************
 * onShutdown
 *
 * Description:  I/O processor received a cleanup job for this client transport to
 *               Shutdown the transport
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 * *************************************************************************************/
int onShutdown (transport *trans)
{
	//fprintf(stderr,"(w) transport for client %s is shutting down\n",trans->client->clientID);
	transportCleanup(trans);
	return IOP_TRANS_CONTINUE;
}

/* *************************************************************************************
 * doSecureRead
 *
 * Description:  Receive data for secure connections
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  bb                  = Byte Buffer where the SSL Handle Socket data is
 *                                     copied to.
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport.
 *           1   (IOP_TRANS_SHUTDOWN) = Stop processing for this transport and perform
 *                                      a transport shutdown.
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
HOT int doSecureRead (transport_t *trans, ism_byte_buffer_t *bb) {
	mqttclient_t *client = trans->client;

	int rc = SSL_read(trans->sslHandle,bb->putPtr,bb->allocated);
	int ec = (rc > 0) ? SSL_ERROR_NONE : SSL_get_error(trans->sslHandle, rc);
	switch (ec) {
		case SSL_ERROR_NONE:
			if (rc) {
				bb->used = rc;
				bb->putPtr += rc;
				trans->currentRecvBuff = NULL;
				client->doData(client,bb);
			}
			return IOP_TRANS_CONTINUE;
		case SSL_ERROR_WANT_READ:
			trans->state |= READ_WANT_READ;
			trans->state &= ~SOCK_CAN_READ;
			trans->currentRecvBuff = NULL;
			ism_common_returnBuffer(bb,__FILE__,__LINE__);
			return IOP_TRANS_CONTINUE;
		case SSL_ERROR_WANT_WRITE:
			trans->state |= READ_WANT_WRITE;
			trans->state &= ~SOCK_CAN_WRITE;
			trans->currentRecvBuff = NULL;
			ism_common_returnBuffer(bb,__FILE__,__LINE__);
			return IOP_TRANS_CONTINUE;
		case SSL_ERROR_ZERO_RETURN:
		default:
			if ((ec == SSL_ERROR_ZERO_RETURN) && (SSL_get_shutdown(trans->sslHandle)))
				trans->state |= SHUTDOWN_IN_PROCESS; // connection completed normally
			else
				trans->state |= SOCK_ERROR; // connection was closed by the server

			trans->currentRecvBuff = NULL;
			ism_common_returnBuffer(bb,__FILE__,__LINE__);

			/* ------------------------------------------------------------------------
			 * If user requested quit OR time expired and the transport state isn't
			 * SOCK_ERROR then quiesce the output from stderr.
			 * ------------------------------------------------------------------------ */
			if ((g_RequestedQuit == 0) && (trans->state & SOCK_ERROR) && (trans->reconnect == 0)) {
				traceSSLError("SSL read failed", 5, __FILE__, __LINE__);
				trans->client->protocolState = MQTT_UNKNOWN;
			}

			/* Ensure the connection is shutdown prior to scheduling reconnect if enabled. */
			rc = connectionShutdown(trans);
			if ((rc == 0) && (trans->sslHandle))
				return rc;
			else
				return IOP_TRANS_IGNORE;
	} /* switch (ec) */
} /* doSecureRead */

/* *************************************************************************************
 * doRead
 *
 * Description:  Receive data for non-secure connections
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  bb                  = Byte Buffer where the Transport Socket data is
 *                                     copied to.
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 *           1
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
int doRead (transport_t *trans, ism_byte_buffer_t *bb)
{
	mqttclient_t *client = trans->client;
	int rc = read(trans->socket, bb->putPtr, bb->allocated);

	if (rc > 0) {
		bb->used = rc;
		bb->putPtr += rc;
		trans->currentRecvBuff = NULL;
		client->doData(client,bb);
		return IOP_TRANS_CONTINUE;
	}

	if ((rc < 0) && (errno == EAGAIN)) {
		trans->state |= READ_WANT_READ;
		trans->state &= ~SOCK_CAN_READ;
		trans->currentRecvBuff = NULL;
		ism_common_returnBuffer(bb,__FILE__,__LINE__);
		return IOP_TRANS_CONTINUE;
	} else {  /* Failed to read with error */
		trans->currentRecvBuff = NULL;
		trans->state |= SOCK_ERROR;
		trans->client->protocolState = MQTT_DISCONNECTED;
		ism_common_returnBuffer(bb,__FILE__,__LINE__);

		rc = connectionShutdown(trans);
		return IOP_TRANS_IGNORE;
	}
} /* doRead */

/* *************************************************************************************
 * readData
 *
 * Description:  Handle a read I/O request
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 * *************************************************************************************/
static int readData (transport_t *trans)
{
	if (trans->currentRecvBuff == NULL) {
		/* do not wait to get receive buffer */
		trans->currentRecvBuff = ism_common_getBuffer(trans->ioProcThread->recvBuffPool,0);
	}

	ism_byte_buffer_t *bb = trans->currentRecvBuff;
	if (bb) {
		trans->state &= ~(READ_WANT_WRITE | READ_WANT_READ);
		trans->doRead(trans, bb);
	}

	return IOP_TRANS_CONTINUE;
} /* readData */

/* *************************************************************************************
 * writeData
 *
 * Description:  Handle a write I/O request
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport
 * *************************************************************************************/
static int writeData (transport_t *trans)
{
	int rc = IOP_TRANS_CONTINUE;
	int sendBufferSize = g_MqttbenchInfo->mbSysEnvSet->sendBufferSize;
	int buffcount = 0;

	ism_byte_buffer_t *bb = NULL;
	ism_byte_buffer_t *freeListHead = NULL;
	ism_byte_buffer_t *freeListTail = NULL;

    assert(trans->socket > 0);

	if (trans->currentSendBuff == NULL) {
		trans->currentSendBuff = ism_allocateByteBuffer(g_MqttbenchInfo->mbSysEnvSet->sendBufferSize);
		if (trans->currentSendBuff == NULL) {
	    	rc = provideAllocErrorMsg("send buffer for client transport",
	    			                  g_MqttbenchInfo->mbSysEnvSet->sendBufferSize,
									  __FILE__, __LINE__);
			exit(1);
		}
	}

	bb = trans->currentSendBuff;

	/* --------------------------------------------------------------------------------
	 * Copy submitted messages into the send buffer of the client transport.
	 *
	 * NOTE:   This was set to #if 0 after the initial development.
	 * -------------------------------------------------------------------------------- */
#if 0
	pthread_spin_lock(&trans->slock);
	if (bb->used == 0) {
		freeListTail = trans->pendingSendListHead;
		while(trans->pendingSendListHead){
			ism_byte_buffer_t * tmp = trans->pendingSendListHead;
			if((bb->allocated - (bb->putPtr - bb->buf)) < tmp->used)
				break;
			trans->pendingSendListHead = tmp->next;
			tmp->next = freeListHead;
			freeListHead = tmp;
			buffcount++;
	    	/* Need to use the getPtr since WebSockets can have padding in the beginning of the buffer. */
			memcpy(bb->putPtr,tmp->getPtr, tmp->used);
			bb->used += tmp->used;
			bb->putPtr += tmp->used;
			if(trans->pendingSendListHead == NULL)
				trans->pendingSendListTail = NULL;
			//fprintf(stderr,"writeData(%p): %d(%d) %d\n",trans,bb->bufSizeUsed,bb->bufSizeAllocated,tmp->bufSizeUsed);
		}
	}
	pthread_spin_unlock(&trans->slock);
#else
   if (bb->used == 0) {
       register int dataSize;
       ism_byte_buffer_t *tmp;
       pthread_spin_lock(&trans->slock);       
       dataSize = 0;

       while (trans->pendingSendListHead) {
           tmp = trans->pendingSendListHead;
           dataSize += tmp->used;
           if (dataSize < sendBufferSize) {
        	   trans->pendingSendListHead = tmp->next;

        	   if (freeListTail == NULL)
        		   freeListHead = tmp;
        	   else
        		   freeListTail->next = tmp;

        	   freeListTail = tmp;
        	   tmp->next = NULL;
           } else {
        	   MBTRACE(MBDEBUG, 9, "SendBufferSize (%d) is smaller than dataSize (%d), no more messages can be batched for this send operation.\n",
        			               sendBufferSize,
								   dataSize);
        	   break;
           }
       }

       if (trans->pendingSendListHead == NULL)
           trans->pendingSendListTail = NULL;

       pthread_spin_unlock(&trans->slock);
  #ifdef _DEBUG_DOWRITE
       for ( dataSize = 0, tmp = freeListHead ; tmp != NULL ; tmp = tmp->next, buffcount++ ) {
    	   dataSize += tmp->used;
           assert(dataSize <= sendBufferSize);
  #else
       for ( tmp = freeListHead ; tmp != NULL ; tmp = tmp->next, buffcount++ ) {
  #endif /* _DEBUG_DOWRITE */
    	   /* -------------------------------------------------------------------------
    	    * Need to use the getPtr since WebSockets can have padding in the beginning
    	    *  of the buffer.
    	    * ------------------------------------------------------------------------- */
           memcpy(bb->putPtr, tmp->getPtr, tmp->used);
           bb->used += tmp->used;
           bb->putPtr += tmp->used;
       }
   }
#endif
    /* --------------------------------------------------------------------------------
     * Return the buffers back to the buffer pool.
     *
     * The original code was using a linked list.   The current implementation is to use
     * a buffer count and ism_common_returnBuffersList.
     *
     * NOTE:   This was set to #if 0 after the initial development.
     * -------------------------------------------------------------------------------- */
#if 0
	while(freeListHead){
		ism_byte_buffer_t * tmp = freeListHead;
		freeListHead = tmp->next;
		ism_common_returnBuffer(tmp,__FILE__,__LINE__);
	}
#else
	if (buffcount > 0)
		ism_common_returnBuffersList(freeListHead, freeListTail, buffcount);
#endif

	/* Call secure or non-secure write function pointer (doWrite) */
	if (bb->used) {
		trans->state &= ~(WRITE_WANT_WRITE|WRITE_WANT_READ);
		int toWrite = bb->allocated - (bb->getPtr - bb->buf);
		if (toWrite > bb->used)
			toWrite = bb->used;

		rc = trans->doWrite(trans, bb, toWrite);
	}

	return rc;
} /* writeData */

/* *************************************************************************************
 * Zeroes out a subset of the stats for the IOP thread passed to this IOP job callback function
 *
 *   @param[in]  dataIOP             = the IOP thread object passed to this callback function
 *   @param[in]  dataTrans           = the transport object passed to this callback function (may be NULL)
 *
 * *************************************************************************************/
void resetStats(void *dataIOP, void *dataTrans) {
	int i;

	volatile ioProcThread_t *iop = (ioProcThread_t *) dataIOP;

	for (i = 0; i < MQTT_NUM_MSG_TYPES; i++) {
		iop->prevRxMsgCounts[i] = 0;
		iop->currRxMsgCounts[i] = 0;
		iop->prevTxMsgCounts[i] = 0;
		iop->currTxMsgCounts[i] = 0;
	} /* end for loop of I/O processor threads*/
} /* resetStats */

/* *************************************************************************************
 * Add a job to each IOP thread with a callback function to reset the message statistics.
 *
 * *************************************************************************************/
void submitResetStatsJob (void)
{
	int i;

	for ( i = 0 ; i < g_MqttbenchInfo->mbSysEnvSet->numIOProcThreads ; i++ ) {
		if (ioProcThreads[i] != NULL)
			addJob2IOP(ioProcThreads[i], NULL, 0, (void *)resetStats);
	}
} /* submitResetLatencyJob */

/* *************************************************************************************
 * resetLatencyStats
 *
 * Description:  Reset the latency statistics for the specified iop.  This will clear out:
 *                 - Histogram
 *                 - The largest latency found, which will probably be larger than the histogram.
 *                 - The maximum latency contained in the histogram
 *                 - Count greater than 1 sec
 *                 - Count greater than 5 sec
 *
 *   @param[in]  dataIOP             = x
 *   @param[in]  dataTrans           = x
 *
 *   @return 0   =
 * *************************************************************************************/
void resetLatencyStats (void *dataIOP, void *dataTrans)
{
	int i;
	int mask;

	latencystat_t *latencyStats;

	volatile ioProcThread_t *iop = (ioProcThread_t *)dataIOP;

	/* Determine the latency histogram(s) that need to be cleared based on the latencyMask. */
	for ( i = 0, mask = 0x1 ; i < LAT_MASK_SIZE ; mask <<= 1, i++ ) {
		if (((mask & g_LatencyMask) > 0) && (iop)) {
			switch (mask) {
				case 0x1:
					latencyStats = iop->rttHist;
					break;
				case 0x2:
					latencyStats = iop->sendHist;
					break;
				case 0x4:
					latencyStats = iop->tcpConnHist;
					break;
				case 0x8:
				case 0x20:
					latencyStats = iop->mqttConnHist;
					break;
				case 0x10:
				case 0x40:
					latencyStats = iop->subscribeHist;
					break;
				default:
					continue;
					break;
			} /* switch (mask) */

			if(!latencyStats)
				continue;

			/* Clear the specific fields in the latency structure for this IOP */
			memset(latencyStats->histogram, 0, (latencyStats->histSize * sizeof(uint32_t)));
			latencyStats->big = 0;
			latencyStats->max = 0;
			latencyStats->totalSampleCt = 0;
			latencyStats->count1Sec = 0;
			latencyStats->count5Sec = 0;

			/* Set the time that this reset occurred. */
			latencyStats->latResetTime = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());
		}
	}
} /* resetLatencyStats */

/* Callback function scheduled from the client scan timer task and invoked from ioProcessorThread */
void scheduleReconnectCallback (void *dataIOP, void *dataTrans) {
	scheduleReconnect(dataTrans, __FILE__, __LINE__);
}

/* Callback function scheduled from the client ping timer task and invoked from ioProcessorThread */
void schedulePingCallback (void *dataIOP, void *dataTrans) {
	submitPing(((transport_t*) dataTrans)->client);
}

/* *************************************************************************************
 * submitResetLatencyJob
 *
 * Description:  Add a job to each IOP to reset the latency structures.
 * *************************************************************************************/
void submitResetLatencyJob (void)
{
	int i;

	for ( i = 0 ; i < g_MqttbenchInfo->mbSysEnvSet->numIOProcThreads ; i++ ) {
		if (ioProcThreads[i] != NULL)
			addJob2IOP(ioProcThreads[i], NULL, 0, (void *)resetLatencyStats);
	}
} /* submitResetLatencyJob */

/* *************************************************************************************
 * addJob4Processing
 *
 * Description:  Add the transport to the list of current jobs for the assigned
 *               I/O processor thread
 *
 *   @param[in]  ioph                = x
 *   @param[in]  trans               = Client transport
 *   @param[in]  events              = x
 *   @param[in]  callbackFunc        = x
 * *************************************************************************************/
HOT static inline void addJob4Processing (volatile ioProcThread_t *ioph,
                               transport_t *trans,
						       uint32_t events,
						       iopjob_callback_t callbackFunc)
{
	if (ioph) {
		pthread_spin_lock(&ioph->jobListLock);
		ioProcJobsList *jobsList = ioph->currentJobs;

		if (__builtin_expect((jobsList->used == jobsList->allocated),0)) {
			jobsList->allocated *= 2;
			jobsList->jobs = realloc(jobsList->jobs, jobsList->allocated * sizeof(ioProcJob));
		}

		ioProcJob *job = jobsList->jobs + jobsList->used;
		if (job) {
			job->trans = trans; /* BEAM suppression */
			job->events = events;
			job->callback = callbackFunc;
			jobsList->used++;
            if (trans) {
                trans->jobIdx = jobsList->used;
            }
		}
		pthread_spin_unlock(&ioph->jobListLock);
	}
}

/* *************************************************************************************
 * addJob2IOP
 *
 * Description:  Add a job to the assigned I/O processor thread to perform a callback.
 *
 *   @param[in]  ioph                = x
 *   @param[in]  trans               = Client transport
 *   @param[in]  events              = x
 *   @param[in]  callbackFunc        = x
 * *************************************************************************************/
void addJob2IOP (volatile ioProcThread_t *ioph,
		         transport_t *trans,
				 uint32_t events,
				 iopjob_callback_t callbackFunc)
{
	addJob4Processing(ioph, trans, events, callbackFunc);
}

/* *************************************************************************************
 * addJob4Cleanup
 *
 * Description:  Add a job to the assigned I/O processor thread to cleanup the transport
 *
 *   @param[in]  trans               = Client transport
 * *************************************************************************************/
static void addJob4Cleanup (transport_t *trans)
{
	pthread_spin_lock(&trans->ioProcThread->jobListLock);	/* same lock being used for transport processing and transport cleanup job lists */
	trans->next = trans->ioProcThread->cleanupReqList;
	trans->ioProcThread->cleanupReqList = trans;
	pthread_spin_unlock(&trans->ioProcThread->jobListLock);
}

/* *************************************************************************************
 * addTransport
 *
 * Description:  API to assign an MQTT client transport to an I/O listener and an
 *               I/O processor thread
 *
 *   @param[in]  ioListenerThrd      = Pointer to an I/O Listener Thread
 *   @param[in]  ioProcThrd          = Pointer to an I/O Processor Thread
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 *         <>0   = Unable to add socket to epoll.
 * *************************************************************************************/
int addTransport (ioListenerThread_t *ioListenerThrd, ioProcThread_t *ioProcThrd, transport_t *trans)
{
	trans->ioProcThread = ioProcThrd;
	trans->ioListenerThread = ioListenerThrd;
	struct epoll_event event = {0};
	event.data.ptr = trans;
	event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
	trans->state = HANDSHAKE_IN_PROCESS;
	trans->isProcessing = 0;
	mqttclient_t *client = trans->client;
	if (epoll_ctl(ioListenerThrd->efd, EPOLL_CTL_ADD, trans->socket, &event) == -1) {
		MBTRACE(MBERROR, 1, "client %s (line=%d) unable to add socket to epoll list (errno=%d).\n",
				            client->clientID, client->line, errno);
		return RC_EPOLL_ERROR;
	}
	trans->epollCount++;

	if ((client && client->traceEnabled) || g_MBTraceLevel == 9) {
        MBTRACE(MBDEBUG, 5, "client %s (line=%d) socket %d was added to epoll list (epollCount=%jd)\n",
				             client->clientID, client->line, trans->socket, trans->epollCount);
	}

	return RC_SUCCESSFUL;
} /* addTransport */

/* *************************************************************************************
 * removeTransportFromIOThread
 *
 * Description:  API to remove an MQTT client transport from the assigned I/O listener
 *               and I/O processor threads
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int removeTransportFromIOThread (transport_t *trans)
{
	if (trans && ((trans->state & SOCK_DISCONNECTED) == 0)) {
		pthread_spin_lock(&trans->slock);
		if (trans->ioProcThread)
			addJob4Processing(trans->ioProcThread, trans, APP_DISCONN_REQ, (void *)NULL);
		pthread_spin_unlock(&trans->slock);
	}

	return 0;
} /* removeTransportFromIOThread */

/* *************************************************************************************
 * submitIOJob
 *
 * Description:  API to send an MQTT message
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  bb                  =
 *
 *   @return 0   = Successful Completion.
 *          >0   = An error/failure occurred.
 * *************************************************************************************/
HOT int submitIOJob (transport_t *trans, ism_byte_buffer_t *bb)
{
	int rc = 0;
	bb->next = NULL;
	pthread_spin_lock(&trans->slock);

	if ((trans->state & (SOCK_ERROR | SOCK_DISCONNECTED | SHUTDOWN_IN_PROCESS)) == 0) {
		if (trans->pendingSendListTail) {					/* append message to pending message list ; I/O processor is not servicing this client transport fast enough */
			trans->pendingSendListTail->next = bb;
			trans->pendingSendListTail = bb;
		} else {											/* pending message list is empty ; I/O processor is keeping up ; append message */
			trans->pendingSendListHead = trans->pendingSendListTail = bb;
			pthread_spin_unlock(&trans->slock);
			addJob4Processing(trans->ioProcThread, trans, 0, NULL);
			return rc;
		}
	} else {
		char errString[MAX_ERROR_LINE_LENGTH] = {0};

		snprintf(errString, MAX_ERROR_LINE_LENGTH,
				            "submitIOJob(%p): Connection is in invalid state: 0x%X, fd: %d, clientID: %s, " \
				            "src addr: %s, src port: %d\n",
		                    trans,
		                    trans->state,
		                    trans->socket,
		                    trans->client->clientID,
		                    (&(trans->clientSockAddr.sin_addr) != NULL ? inet_ntoa(trans->clientSockAddr.sin_addr) : ""),
		                    trans->srcPort);

		MBTRACE(MBERROR, 1, "%s\n", errString);

		ism_common_returnBuffer(bb,__FILE__,__LINE__);         /* Return the buffer back to the pool */
		rc = RC_UNABLE_TO_SUBMIT_JOB;
	}

	pthread_spin_unlock(&trans->slock);
	return rc;
} /* submitIOJob */

/* *************************************************************************************
 * submitJob4IOP
 *
 * Description:  Submit a job to the IOP to perform a job for the Transport.  The type
 *               of job is based on what the trans->state is set to prior to calling
 *               this function.
 *
 *               Currently called to perform the following:
 *               Called by:       Description:
 *               ---------------  -----------------------------------------------------------
 *               doHandleClients  Cleanup the transport on a disconnect when using zombie time
 *               doHandleClients  Submit a socket create job after waking up from zombie time
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int submitJob4IOP (transport_t *trans)
{
	addJob4Processing(trans->ioProcThread, trans, 0, NULL);
	return 0;
} /* submitJob4IOP */

/* *************************************************************************************
 * submitCreateSocketJob
 *
 * Description:  Submit a job to the IOP for Reinitialize transport.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int submitCreateSocketJob (transport_t *trans)
{
	addJob4Processing(trans->ioProcThread, trans, 0, NULL);
	return 0;
} /* submitCreateSocketJob */

/* *************************************************************************************
 * submitCreateConnectionJob
 *
 * Submit a job to the IOP for creating the connection.
 *
 * Description:  x
 *
 *   @param[in]	 trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int submitCreateConnectionJob (transport_t *trans)
{
	int rc = RC_SUCCESSFUL;
	trans->state = SOCK_NEED_CONNECT;
	if(trans->ioProcThread == NULL)
		trans->ioProcThread = ioProcThreads[trans->client->ioProcThreadIdx];

	addJob4Processing(trans->ioProcThread, trans, 0, NULL);
	return rc;
} /* submitCreateConnectionJob */

/* *************************************************************************************
 * submitMQTTUnSubscribeJob
 *
 * Description:  Submit a job to the IOP to perform a MQTT UnSubscribe for the client
 *               associated with the transport.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int submitMQTTUnSubscribeJob (transport_t *trans)
{
	if (trans->client->protocolState == MQTT_PUBSUB)
		addJob2IOP(trans->ioProcThread, trans, 0, (void *)performMQTTUnSubscribe);

	return 0;
} /* submitMQTTUnSubscribeJob */

/* *************************************************************************************
 * performMQTTUnSubscribe
 *
 * Description:  Submit a job to the IOP to perform a MQTT UnSubscribe for the client
 *               associated with the transport.
 *
 *   @param[in]  dataIOP             = x
 *   @param[in]  dataTrans           = x
 * *************************************************************************************/
void performMQTTUnSubscribe (void *dataIOP, void *dataTrans)
{
	transport_t *trans = (transport_t *)dataTrans;
	if (trans) {
		trans->client->protocolState = MQTT_DOUNSUBSCRIBE;
		addJob4Processing(trans->ioProcThread, trans, 0, (void *)NULL);
	}
} /* performMQTTUnSubscribe */

/* *************************************************************************************
 * submitMQTTDisconnectJob
 *
 * Description:  Submit a job to the IOP to perform a MQTT Disconnect for the client
 *               associated to the transport.
 *
 *   @param[in]  trans               = Client transport
 *   @param[in]  deferJob            = a flag to indicate whether to defer the disconnect
 *                                     for this transport and allow a pending job for this
 *                                     transport to be processed prior to disconnect. For
 *                                     example, a client that had a pending PUBLISH message
 *                                     to be transmitted.
 *
 *   @return 0   =
 * *************************************************************************************/
int submitMQTTDisconnectJob (transport_t *trans, int deferJob)
{
	if (trans->client->protocolState == MQTT_PUBSUB || trans->client->protocolState == MQTT_CONNECTED) {
		if(deferJob) {
			trans->state |= DEFERRED_SHUTDOWN;
			addJob4Processing(trans->ioProcThread, trans, 0, (void *) NULL);
		} else {
			addJob2IOP(trans->ioProcThread, trans, 0, (void *) performMQTTDisconnect);
		}
	}

	return 0;
} /* submitMQTTDisconnectJob */

/* *************************************************************************************
 * performMQTTDisconnect
 *
 * Description:  This is the callback for disconnecting a particular client.  This is
 *               accomplished by calling doDisconnectMQTTClient.
 *
 *   @param[in]  dataIOP             = x
 *   @param[in]  dataTrans           = x
 * *************************************************************************************/
void performMQTTDisconnect (void *dataIOP, void *dataTrans)
{
	transport_t *trans = (transport_t *)dataTrans;
	if (trans) {
		/* ----------------------------------------------------------------------------
		 * Check to see if the Client associated with this transport has a disconnect
		 * return code.  If so then update the disconnect counter due to errors
		 * (currDiscconnectErrorCount) for this IOP.   This will provide a way to notify
		 * the user on the stats screen that there are disconnects occurring.
		 * ---------------------------------------------------------------------------- */
		if (trans->client->disconnectRC) {
			volatile ioProcThread_t *iop = NULL;
			iop = trans->ioProcThread;
			iop->currBadTxRCCount++;
			trans->client->badTxRCCount++;
		}

		doDisconnectMQTTClient(trans->client);

	}
} /* performMQTTDisconnect */


/* *************************************************************************************
 * processIORequest
 *
 * Description:  Process requests in this order for the given mqttclient:
 *                  - For secure connections establish SSL handshake if not established
 *                  - Check if ready for read and if so, read
 *                  - Check if ready for write and if so, write
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   (IOP_TRANS_CONTINUE) = Continue processing requests for this transport.
 *           1   (IOP_TRANS_SHUTDOWN) = Stop processing for this transport and perform
 *                                      a transport shutdown.
 *          -1   (IOP_TRANS_IGNORE)   = Remove this transport from the joblist queue,
 *                                      and wait for event.
 * *************************************************************************************/
HOT static inline int processIORequest (transport_t *trans)
{
	int rc = 0;

	/* --------------------------------------------------------------------------------
	 * Handle the case where the transport state has either SOCK_ERROR or
	 * SHUTDOWN_IN_PROCESS set.
	 * -------------------------------------------------------------------------------- */
	if ((trans->state & SOCK_ERROR) || (trans->state & SOCK_DISCONNECTED) || (trans->state & SHUTDOWN_IN_PROCESS)) {
		/* If the user specified ExitOnFirstDisconnect (-efd option) and transport
		 * state has SOCK_ERROR set then exit. */
		if ((g_MqttbenchInfo->mbCmdLineArgs->exitOnFirstDisconnect) && (trans->state & SOCK_ERROR)) {
			MBTRACE(MBINFO, 1, "Shutting down mqttbench, exit on first disconnect (-efd) command line option was enabled and "\
					           "at least one connection was closed\n");
			g_StopIOPProcessing = 1;
			g_RequestedQuit = 1;
			g_MBErrorRC = RC_EXIT_ON_ERROR;
			return IOP_TRANS_SHUTDOWN;
		}

		/* Test if reconnect is enabled. */
		if (trans->reconnect) {
			/* Reschedule this transport since reconnect was enabled. */
			if (g_RequestedQuit == 0 && (trans->state & SOCK_NEED_CREATE) == 0) {
				scheduleReconnect(trans, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			}
		} else {
			/* Reconnect is not enabled so permanently shutting down connection */
			rc = connectionShutdown(trans);
			if ((rc == 0) && (trans->sslHandle))
				return rc;
			else
				return IOP_TRANS_IGNORE;
		}
	}

	/* --------------------------------------------------------------------------------
	 * See if the transport state has HANDSHAKE_IN_PROCESS set and if so then
	 * call onConnect to see if the connection completed.
	 * -------------------------------------------------------------------------------- */
	if (trans->state & HANDSHAKE_IN_PROCESS) {

		/* Complete/continue the TCP/TLS handshake */
		rc = trans->onConnect(trans);

		/* ----------------------------------------------------------------------------
		 * If non-zero RC then check -efd command line flag and if reconnect is enabled
		 * on the client
		 * ---------------------------------------------------------------------------- */
		if (rc) {
			if(rc == IOP_TRANS_CONTINUE_TLS)
				return IOP_TRANS_IGNORE;

			if (g_MqttbenchInfo->mbCmdLineArgs->exitOnFirstDisconnect) {
				MBTRACE(MBINFO, 1, "Shutting down mqttbench, exit on first disconnect (-efd) command line option was enabled and "\
								   "at least one connection was closed\n");
				g_StopIOPProcessing = 1;
				g_RequestedQuit = 1;
				g_MBErrorRC = RC_EXIT_ON_ERROR;
				rc = IOP_TRANS_SHUTDOWN;
			}

			if (trans->reconnect) {
				scheduleReconnect(trans, __FILE__, __LINE__);
				rc = IOP_TRANS_IGNORE;
			} else {
				/* Reconnect is not enabled so permanently shutting down connection */
				rc = connectionShutdown(trans);
				if ((rc == 0) && (trans->sslHandle))
					return rc;
				else
					return IOP_TRANS_IGNORE;
			}
		}

		return rc;
	}

	/* Check if the Socket needs to be created. */
	if (trans->state & SOCK_NEED_CREATE) {
		/* Create the Socket. */
		rc = createSocket(trans);

		/* ----------------------------------------------------------------------------
		 * Check to see if rc NOT = 0 and reconnect is enabled.   If this is true, then
		 * need to schedule reconnect since there will be NO EPOLL events to disconnect
		 * the socket since this is the point where the socket is being created.
		 * ---------------------------------------------------------------------------- */
		if ((rc) && (trans->reconnect)) {
			scheduleReconnect(trans, __FILE__, __LINE__);
			return IOP_TRANS_IGNORE;
		} else
			return rc;
	}

	/* --------------------------------------------------------------------------------
	 * Check to see if transport state indicates WebSockets is in process if so then
	 * read data.
	 * -------------------------------------------------------------------------------- */
	if (trans->state & SOCK_WS_IN_PROCESS)
		return trans->onWSConnect(trans);

	/* Check if the Socket needs a connection. */
	if (trans->state & SOCK_NEED_CONNECT) {
		/* ----------------------------------------------------------------------------
		 * Create the connection.
		 *
		 * createConnection is called to perform create the connection.  Upon return
		 * the return codes can be:
		 *
		 *    0   == There are 3 conditions for this to occur:
		 *              a) Successfully created the connection and the transport for
		 *                 this client is added to the target I/O listener and
		 *                 I/O processor threads.
		 *              b) Received a socket error: EINPROGRESS
		 *              c) Reconnect is: enabled, socket error is: EADDRNOTAVAIL,
		 *                 maxRetries (10K) was NOT exceeded and the client's
		 *                 currRetryDelayTime_ns is > 0, then createConnection added
		 *                 the transport to a time task to attempt again.
		 *           After returning from createConnection the return code will be
		 *           set to:  IOP_TRANS_IGNORE   so that when it returns to the
		 *           IOProcessorThread it will be removed from the IOP Job List. Once
		 *           removed it requires an EPOLL error or a job from a timer task.
		 *
		 * 0xDEAD == Reconnect is NOT enabled and received a socket error which is
		 *           neither EINPROGRESS nor EADDRNOTAVAIL.  In order to shutdown
		 *           this connection the return code is set to:  IOP_TRANS_SHUTDOWN
		 *           so that IOProcessorThread will perform the necessary steps
		 *           to shut the connection down.
		 *
		 *     -1 == Reconnect is enabled, the socket error was EADDRNOTAVAIL and
		 *           it exceeded the max retries.  SOCK_ERROR was 'OR' to the
		 *           transport->state.  Will reschedule the connection here and set
		 *           the rc = IOP_TRANS_IGNORE.
		 *
		 *     -1 == Reconnect is not enabled.  Therefore shutdown the connection
		 *           by setting the rc = IOP_TRANS_SHUTDOWN.
		 * ---------------------------------------------------------------------------- */

		/* Create the Connection.  If successful (rc == 0) then return -1 (IOP_TRANS_IGNORE) */
		rc = createConnection(trans->client);
		if (rc == 0) {
			return IOP_TRANS_IGNORE;
		} else {
			if (trans->reconnect) {
				/* --------------------------------------------------------------------
				 * rc is not = 0 at this point.  So, check to see if reconnect is
				 * enabled.  If true, then need to schedule reconnect since there will
				 * be NO EPOLL events to disconnect since this is trying to create the
				 * connection with the server.  The timer task will put a job on the
				 * list in order to start the reconnect process.
				 * -------------------------------------------------------------------- */
				scheduleReconnect(trans, __FILE__, __LINE__);
				return IOP_TRANS_IGNORE;
			} else {
				trans->state &= ~SOCK_NEED_CONNECT;
				return IOP_TRANS_SHUTDOWN;
			}
		}
	}

	/* --------------------------------------------------------------------------------
	 * See if the Client's protocol state is in MQTT_DOUNSUBSCRIBE and if so, then need
	 * to send a MQTT UNSUBSCRIBE message to start the shutdown.
	 * -------------------------------------------------------------------------------- */
	if (trans->client->protocolState == MQTT_DOUNSUBSCRIBE) {
		trans->client->protocolState = MQTT_UNSUBSCRIBE_IN_PROCESS;
		submitUnSubscribe(trans->client);
		return IOP_TRANS_CONTINUE;
	}

	// Check if we can read
	if (CAN_READ(trans->state)){
		rc = readData(trans);
		if (rc)
			return rc;
	}

	// Check if we can write
	if (CAN_WRITE(trans->state)){
		rc = writeData(trans);
		if (rc)
			return rc;
	}

	if (trans->state & DEFERRED_SHUTDOWN) {
		performMQTTDisconnect((ioProcThread_t *) trans->ioProcThread, trans);
		return IOP_TRANS_IGNORE;
	}

	if (CAN_READ(trans->state) ||
		((trans->pendingSendListHead ||	(trans->currentSendBuff && trans->currentSendBuff->used)) && CAN_WRITE(trans->state)))
		return IOP_TRANS_CONTINUE;

	return IOP_TRANS_IGNORE;
} /* processIORequest */

/* *************************************************************************************
 * scheduleReconnect
 *
 * Description:  Perform the necessary steps for cleaning up the transport and the
 *               client to allow the connection to attempt a reconnect.   The reconnect
 *               is scheduled by placing a job on the timer thread.
 *
 *   @param[in]  trans               = Client's transport
 *
 * *************************************************************************************/
static void scheduleReconnect (transport_t *trans, const char *file, int line)
{
	mqttclient_t *client = trans->client;

	if(client->traceEnabled || g_MBTraceLevel == 9) {
		MBTRACE(MBDEBUG, 5, "DEBUG: client @ line=%d, ID=%s scheduling a reconnect, initiated from %s:%d\n",
					        client->line, client->clientID, file, line);
	}

	/* --------------------------------------------------------------------------------
	 * Check to make sure that the transport state isn't already in SOCK_NEED_CREATE.
	 * If it is then return.   The concern here is that a job may already be in the
	 * timer or the IOP job list to handle.
	 * -------------------------------------------------------------------------------- */
	if ((trans->state & SOCK_NEED_CREATE) == 0) {
		trans->state = SOCK_NEED_CREATE;

		/* ----------------------------------------------------------------------------
		 * Free up the linked list of send buffers associated with the transport and
		 * close the TLS session (for secure connections) and socket.
		 * ---------------------------------------------------------------------------- */
		transportCleanup(trans);

		/* Set the client protocol state to UNKNOWN since doing a reset.*/
		trans->client->protocolState = MQTT_UNKNOWN;

		/* Perform the Client cleanup. */
		resetMQTTClient(trans->client);

		/* timerfd support does not work well (observed missed timer event) with timers less that 50 microseconds
		 * ensure a minimum of 50 microseconds for reconnect delay timer */
		uint64_t reconnectDelayTime = MAX(MIN_RECONNECT_DELAY_NANOS, trans->client->currRetryDelayTime_ns);

		/* Add this transport to the timer task that will call onDelayTimeout. */
		addDelayTimerTask(g_MqttbenchInfo, trans, reconnectDelayTime);
	}

} /* scheduleReconnect */

/* *************************************************************************************
 * ioProcessorThread
 *
 * Description:  Thread entry point for the I/O Processor threads
 *
 *   @param[in]  threadArg           = I/O Processor's Thread information
 *   @param[in]  context             = x
 *   @param[in]  value               = x
 * *************************************************************************************/
static void * ioProcessorThread (void *threadArg, void *context, int value)
{
	static int numPrintClientClosed = 0;
	static int numPrintServerClosed = 0;

	int i;
	int numEpollErr = 0;
	int batchingDelay = g_MqttbenchInfo->mbSysEnvSet->batchingDelay;

	char tname[32] = {0};

	uint32_t currentAllocated = 16 * 1024;
	uint32_t currentJobSize, nextJobSize = 0;

	ioProcThread_t *iop = (ioProcThread_t *)threadArg;

	transport_t *cleanupReqList;
	transport_t **localJobsList = calloc(currentAllocated, sizeof(transport_t*));

	/* Get a copy of the thread name */
	ism_common_getThreadName(tname, sizeof(tname));

	while (!iop->isStopped) {
		int rc;
		ioProcJobsList *currentJobsList;

		pthread_spin_lock(&iop->jobListLock);            /* Move jobs to local jobs list and work off the local list */
		currentJobsList = iop->currentJobs;
		iop->currentJobs = iop->nextJobs;
		iop->nextJobs = currentJobsList;
		pthread_spin_unlock(&iop->jobListLock);

		for ( i = 0 ; i < currentJobsList->used ; i++ ) {
			mqttclient_t *client = NULL;

			ioProcJob * job = currentJobsList->jobs + i;
			uint32_t events = job->events;
			transport_t * trans = job->trans;
			if (trans) {
				client = trans->client;

				if ((client && client->traceEnabled) || g_MBTraceLevel == 9) {
					const char *clientID = "N/A";
					int line = -1;
					int pstate = 0xDEAD;
					if (client) {
						clientID = client->clientID;
						line = client->line;
						pstate = client->protocolState;
					}

					ism_byte_buffer_t *data = trans->pendingSendListHead;
					MBTRACE(MBDEBUG, 5, "processing job for client %s (line=%d) and trans=%p events=0x%x tstate=0x%x pstate=0x%x pendingSendListHead=%d bytes callback=%p\n",
					            	     clientID, line, trans, events, trans->state, pstate, data ? data->used : 0, job->callback);
				}
			}

			if (events != APP_DISCONN_REQ) {
				if (LIKELY(trans != NULL)) {
					if (events & EPOLLIN)
						trans->state |= SOCK_CAN_READ;

					if (events & EPOLLOUT)
						trans->state |= SOCK_CAN_WRITE;

					if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
						int getSockOptRC = 0;
						int error = 0;
						socklen_t errlen = sizeof(error);
						client->socketErrorCt++;

						if (events & EPOLLERR) {
							if (!(client->isSecure) && (trans->state & (SHUTDOWN_IN_PROCESS | SOCK_DISCONNECTED))== 0) {
								volatile ioProcThread_t * ioProc = trans->ioProcThread;
								char errString[MAX_ERROR_LINE_LENGTH] = {0};

								getSockOptRC = getsockopt(trans->socket, SOL_SOCKET, SO_ERROR, (void*)&error, &errlen);
								client->lastErrorNo = (getSockOptRC == 0 ? error : -1);
								if(numPrintClientClosed % CONSTANT_5K == 0 || client->traceEnabled) {
									snprintf(errString, MAX_ERROR_LINE_LENGTH,
												        "Client closed connection %s (dst:%s:%d src:%s:%d), fd: %d, " \
												        "epoll events: 0x%x, trans->state: 0x%X, errno: %d",
												        client->clientID,
												        trans->serverIP,
												        trans->serverPort,
												        inet_ntoa(trans->clientSockAddr.sin_addr),
												        trans->srcPort,
												        trans->socket,
												        events,
												        trans->state,
												        client->lastErrorNo);
									MBTRACE(MBERROR, 5, "%s\n", errString);
									numPrintClientClosed++;
								}

								/* Store the socket error in the IOP socket error array. */
								ioProc->socketErrorArray[iop->nextErrorElement][0] = '\0';
								strcpy(ioProc->socketErrorArray[ioProc->nextErrorElement++], errString);
								/* If reached max element reset to zero */
								if (ioProc->nextErrorElement == MAX_NUM_SOCKET_ERRORS)
									ioProc->nextErrorElement = 0;
							}
						} else { // EPOLLHUP | EPOLLRDHUP
							if (numEpollErr < EPOLLERR_MAX_PRINT || client->traceEnabled) {
								volatile ioProcThread_t * ioProc = trans->ioProcThread;
								char errString[MAX_ERROR_LINE_LENGTH] = {0};

								getSockOptRC = getsockopt(trans->socket, SOL_SOCKET, SO_ERROR, (void*)&error, &errlen);
								client->lastErrorNo = (getSockOptRC == 0 ? error : -1);
								if(numPrintServerClosed % CONSTANT_5K == 0 || client->traceEnabled) {
									snprintf(errString, MAX_ERROR_LINE_LENGTH,
												        "Server closed connection %s (dst:%s:%d src:%s:%d), fd: %d, epoll events: 0x%x, " \
													    "trans->state: 0x%X, errno: %d.",
												        client->clientID,
													    trans->serverIP,
												        trans->serverPort,
												        inet_ntoa(trans->clientSockAddr.sin_addr),
												        trans->srcPort,
												        trans->socket,
												        events,
												        trans->state,
												        (getSockOptRC == 0 ? error : -1));

									if (g_RequestedQuit == 0) {
										MBTRACE(MBERROR, 5, "%s %s Printing this message a limited number of times, "\
							                                "see TE count in stats.\n",
								                            errString, (g_DoDisconnect ? " Shutting down connection." : ""));
									}

									numPrintServerClosed++;
								}

								/* Store the socket error in the IOP socket error array. */
								iop->socketErrorArray[iop->nextErrorElement][0] = '\0';
								strcpy(ioProc->socketErrorArray[iop->nextErrorElement++], errString);
								/* If reached max element reset to zero */
								if (ioProc->nextErrorElement == MAX_NUM_SOCKET_ERRORS)
									ioProc->nextErrorElement = 0;
							} /* if (numEpollErr < EPOLLERR_MAX_PRINT) */

							trans->client->protocolState = MQTT_DISCONNECTED;
						} /* else (EPOLLHUP | EPOLLRDHUP)*/

						numEpollErr++;
						trans->state |= SOCK_ERROR;
					} /*if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) */

					if (events & ~(EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLIN | EPOLLOUT )) {
						MBTRACE(MBERROR, 1, "Unhandled EPOLL event: 0x%x\n", events);
						exit (-1);
					}
				} /* if (trans) */
			} else {
				if (trans)
					trans->state |= SHUTDOWN_IN_PROCESS;
			}

			if (job->callback) {
				job->callback((void *)iop, (void *)trans);
			} else {
				if (!trans->isProcessing) {
					if (nextJobSize == currentAllocated) {
						currentAllocated *= 2;
						localJobsList = realloc(localJobsList, currentAllocated * sizeof(transport_t *));
					}

					localJobsList[nextJobSize++] = trans;
					trans->isProcessing = 1;
					trans->processCount++;
				}
			}
		} /* for ( i = 0 ; i < currentJobsList->used ; i++ ) */

		currentJobsList->used = 0;
		currentJobSize = nextJobSize;
		nextJobSize = 0;

		for ( i = 0 ; i < currentJobSize ; i++ ) {		/* Process the jobs from the local list */
			transport_t* trans = localJobsList[i];
			localJobsList[i] = NULL;
			rc = processIORequest(trans);
			if (rc == 0) {
				/* --------------------------------------------------------------------
				 * The return code is = 0 (TRANS_IOP_CONTINUE) so put the transport back
				 * on the local job list.   This is either due to the fact that it was
				 * unable to process the I/O request or there is a state change.
				 * -------------------------------------------------------------------- */
				localJobsList[nextJobSize++] = trans;
			} else {
				if (rc < 0)
					trans->isProcessing = 0;
				else {
					if (trans->onShutdown)
						trans->onShutdown(trans);
				}
			}
		}

		pthread_spin_lock(&iop->jobListLock);
		cleanupReqList = iop->cleanupReqList;
		iop->cleanupReqList = NULL;
		pthread_spin_unlock(&iop->jobListLock);

		while (cleanupReqList) {
			transport_t *trans = cleanupReqList;
			cleanupReqList = cleanupReqList->next;
			trans->state |= SOCK_DISCONNECTED;
			iop->currDisconnectCounts++;
			trans->state &= ~(SHUTDOWN_IN_PROCESS | SOCK_CONNECTED);
			addJob4Processing(trans->ioProcThread, trans, 0, NULL);
		} /* while (cleanupReqList) */

		/* ----------------------------------------------------------------------------
		 * If batchingDelay > 0 then perform usecs sleeps. If <= 0 then 3 sched_yields
		 * are performed.  This is needed for latency testing to minimize the client
		 * portion.
		 * ---------------------------------------------------------------------------- */
		if (batchingDelay)
			ism_common_sleep(batchingDelay);
		else {
			sched_yield(); sched_yield(); sched_yield();
		}
	} /* while(!iop->isStopped) */

	/* The iop has stopped so free up the local jobs list. */
	free(localJobsList);

	return NULL;
} /* ioProcessorThread */

/* *************************************************************************************
 * ioListenerThread
 *
 * Description:  Thread entry point for the I/O listener threads
 *
 *   @param[in]  threadArg           = I/O Listener Thread's information.
 *   @param[in]  context             = x
 *   @param[in]  value               = x
 * *************************************************************************************/
static void * ioListenerThread (void *threadArg, void *context, int value)
{
	int efd;
	int events_found = 0;
	int max_events = INITIAL_NUM_EPOLL_EVENTS;

	/* Intercommunication between threads. */
	int pipefd[2];
	xUNUSED int rc = pipe2(pipefd, O_NONBLOCK);

	struct epoll_event *epoll_events = NULL;

	ioListenerThread_t *iol = (ioListenerThread_t *)threadArg;

	/* Get a copy of the thread name */
	char tname[20] = {0};
	ism_common_getThreadName(tname, sizeof(tname));

	efd = iol->efd;

	epoll_events = calloc(max_events, sizeof(struct epoll_event));

	/* --------------------------------------------------------------------------------
	 * There are 2 ends to the pipes where pipedf[0] is the read end and pipefd[1] is
	 * the write end.
	 * -------------------------------------------------------------------------------- */
	epoll_events[0].data.fd = pipefd[0];
	epoll_events[0].events = EPOLLIN | EPOLLRDHUP | EPOLLET;
	rc = epoll_ctl(efd, EPOLL_CTL_ADD, pipefd[0], &epoll_events[0]);
	assert (rc != -1);

	iol->pipe_wfd = pipefd[1];  /* write pipe */

	/* --------------------------------------------------------------------------------
	 * Main processing loop
	 *
	 * Wait for an epoll event
	 * Loop through epoll events
	 *   If EPOLLIN event:
	 * 	   set CAN_READ state
	 *   If EPOLLOUT event:
	 *     check if there is a tx buffer ready to be sent for the connection on which
	 *     the epoll event was raised.  if yes, then this connection can transmit
	 *     data doWrite() or doSecureWrite()
	 * -------------------------------------------------------------------------------- */
	while (!iol->isStopped) {
		transport_t *cleanupReqList;

		/* Add a job for connection cleanup */
		pthread_spin_lock(&iol->cleanupReqListLock);
		cleanupReqList = iol->cleanupReqList;
		iol->cleanupReqList = NULL;
		pthread_spin_unlock(&iol->cleanupReqListLock);

		while (cleanupReqList) {
			transport_t *trans = cleanupReqList;
			cleanupReqList = cleanupReqList->next;
			epoll_ctl(efd, EPOLL_CTL_DEL, trans->socket, NULL);
			trans->epollCount--;

			if ((trans->client && trans->client->traceEnabled) || g_MBTraceLevel == 9) {
				mqttclient_t *client = trans->client;
				MBTRACE(MBDEBUG, 5, "client %s (line=%d) socket %d was removed from epoll list (epollCount=%jd), adding cleanup job for IOP thread\n",
									 client->clientID, client->line, trans->socket, trans->epollCount);
			}
			addJob4Cleanup(trans);
		}

		events_found = epoll_wait(efd, epoll_events, max_events, 1000); /* 1 second timeout, may need to decrease this timeout */
		if (LIKELY(events_found > 0)) {
			int i;

			/* ------------------------------------------------------------------------
			 * Go through list of epoll events, check transport state for client
			 * connection, and submit job to processing thread.
			 * ------------------------------------------------------------------------ */
			for ( i = 0 ; i < events_found ; i++ ) {
				struct epoll_event * event = &epoll_events[i];

				if (LIKELY(event->data.fd != pipefd[0])) {
					transport_t *trans = event->data.ptr;
					if (LIKELY(trans != NULL)) {
						mqttclient_t *client = trans->client;
						const char *clientID = "N/A";
						int line = -1;
						int pstate = 0xDEAD;
						if ((client && client->traceEnabled) || g_MBTraceLevel == 9) {
							clientID = client->clientID;
							line = client->line;
							pstate = client->protocolState;

							MBTRACE(MBDEBUG, 5, "epoll event for client %s (line=%d) and trans=%p fd=%d events=0x%x tstate=0x%x pstate=0x%x\n",
												 clientID, line, trans, trans->socket, event->events, trans->state, pstate);
						}
						addJob4Processing(trans->ioProcThread, trans, event->events, NULL);
						continue;
					}
				} else {
					char c;
					while (read(pipefd[0], &c, 1) > 0) {
						continue;
					}
				}
			}

			/* Increase epoll list size if not big enough */
			if (UNLIKELY(events_found == max_events)) {
				int more_events = events_found * 2; /* grow epoll event count by 2X every time we hit the current limit */
				/* --------------------------------------------------------------------
				 * TODO
				 *    - Take the following line out once we tune the rate at which we
				 *      grow the epoll event.
				 * -------------------------------------------------------------------- */
				double ts = (g_ClkSrc == 0 ? ism_common_readTSC() : getCurrTime());

				/* --------------------------------------------------------------------
				 * TODO
				 *    - If the following messages shows up then need to change to malloc
				 *      and free.
				 * -------------------------------------------------------------------- */
				if (g_RequestedQuit == 0)
					MBTRACE(MBERROR, 1, "epoll events_found: %d max_events: %d growing max_events.  ts=%f\n",
						                events_found,
							            max_events,
							            ts);

				struct epoll_event *tmp = realloc(epoll_events, (more_events) * sizeof(struct epoll_event));
				if (tmp) {
					epoll_events = tmp;
					max_events = more_events;
				}
			}
		} else {
			/* ------------------------------------------------------------------------
			 * If errno == EINTR then don't print out error message. errno = EINTR
			 * occurs with gdb.
			 * ------------------------------------------------------------------------ */
			if ((events_found) && (errno != EINTR))
				MBTRACE(MBERROR, 1, "epoll_wait() failed: errno=%d\n", errno);
		}
	} /* while (!iol->isStopped) */

	free(epoll_events);
	close(iol->efd);
	close(pipefd[0]);
	close(pipefd[1]);
	return NULL;
} /* ioListenerThread */

/* *************************************************************************************
 * stopIOListening
 *
 * Description:  Add client transport to cleanup list for the I/O listener thread to
 *               remove from epoll list.
 *
 *   @param[in]  trans               = Client transport
 *
 *   @return 0   = I/O Listener Thread is NULL.
 *           1   = Successful Completion.
 * *************************************************************************************/
static int stopIOListening (transport_t *trans)
{
	ioListenerThread_t *iol = trans->ioListenerThread;
	char  c = 'C';

	if (iol == NULL)
		return 0;

	pthread_spin_lock(&iol->cleanupReqListLock);
	trans->next = iol->cleanupReqList;
	iol->cleanupReqList = trans;
	trans->ioListenerThread = NULL;
	pthread_spin_unlock(&iol->cleanupReqListLock);

	if ((trans->client && trans->client->traceEnabled) || g_MBTraceLevel == 9) {
		mqttclient_t *client = trans->client;
		MBTRACE(MBDEBUG, 5, "client %s (line=%d tstate=0x%x pstate=0x%x) added to cleanup list for removal from IOL thread\n",
							client->clientID, client->line, trans->state, client->protocolState);
	}

	return write(iol->pipe_wfd, &c, 1);
} /* stopIOListening */

/* *************************************************************************************
 * tcpCleanup
 *
 * Description:  Cleanup the TCP connection along with any security and/or websocket
 *               information associated to the connection.
 * *************************************************************************************/
void tcpCleanup (void)
{
	if (g_MqttbenchInfo->useSecureConnections) {
		sslLockCleanup();
		int numIOP= g_MqttbenchInfo->mbSysEnvSet->numIOProcThreads;
		for(int i=0; i < numIOP; i++) {
			if(g_SSLCtx && g_SSLCtx[i]) {
				sslDestroyCtx(g_SSLCtx[i]);
			}
		}
	}

} /* tcpCleanup */

/* *************************************************************************************
 * initIOPTxBuffers
 *
 * Description:  Set up the pointers in the mbinst to the iop send buffer pool.
 *
 *   @param[in]  mbinst              = mqttbench instance.
 *   @param[in]  numBfrs             = Number of buffers to initialize it to.
 *
 *   @return 0   = Successful Completion.
 *         <>0   = Failed to allocate the number of buffers requested.
 * *************************************************************************************/
int initIOPTxBuffers (mqttbench_t *mbinst, int numBfrs)
{
	int rc = 0;
	int i;

	volatile ioProcThread_t *iop;

	for ( i = 0 ; i < g_MqttbenchInfo->mbSysEnvSet->numIOProcThreads ; i++ ) {
		iop = ioProcThreads[i];
		mbinst->txBfr[i] = getIOPTxBuffers(iop->sendBuffPool, numBfrs, 0);
		if (mbinst->txBfr[i] == NULL) {
			MBTRACE(MBERROR, 1, "Failure allocating TX Buffers for IOP\n.");
			rc = RC_MEMORY_ALLOC_ERR;
			break;
		}
	}

	mbinst->numBfrs = numBfrs;
	MBTRACE(MBINFO, 5, "Submitter thread %d retrieved %d buffers from each IOP thread TX buffer pool for transmitting MQTT messages\n",
			           mbinst->id, numBfrs);

	return rc;
} /* initIOPTxBuffers */

/* *************************************************************************************
 * getIOPTxBuffers
 *
 * Description:  Get a specific set of buffers from the TX Pool
 *
 *   @param[in]  pool                = A Buffer Pool to request buffers from.
 *   @param[in]  numBfrs             = Number of buffers to request.
 *   @param[in]  force               = Action to perform if buffer pool is exhausted:
 *                                       0 = Wait until enough buffers are available.
 *                                       1 = Force a request for additional buffers.
 *
 *   @return a group of IOP TX byte buffers from the buffer pool.
 * *************************************************************************************/
ism_byteBuffer getIOPTxBuffers (ism_byteBufferPool pool, int numBfrs, int force)
{
	int retryCtr = 0;

	ism_byteBuffer txPoolBuffers = NULL;

	do {
		txPoolBuffers = ism_common_getBuffersList(pool, numBfrs, force);
		if (txPoolBuffers)
			break;

		/* ----------------------------------------------------------------------------
		 * In the case that the submitter thread tries to get buffers and fails then
		 * retry 3 times and return NULL.  This will allow the submitter thread to
		 * continue processing other clients.
		 * ---------------------------------------------------------------------------- */
		if (++retryCtr == NUM_RETRIES_GETBUFFER)
			break;

		sched_yield();sched_yield();sched_yield();
	} while (1);

	return txPoolBuffers;
} /* getIOPTxBuffers */

/* *************************************************************************************
 * Attempt to connect to the provided destination IP address and port from the provided
 * client sourc IP address
 *
 * @param[in]  pClientSrcIPAddr    = src ip address to bind to and connect from
 * @param[in]  srvrSockAddr        = server sockaddr_in with destination IP address and port number
 * @param[in]  displaySuccessConn  = flag to indicate whether to print connection test results to stdout
 *
 * @return 0   =
 *        -1   = An error/failure occurred.
 * *************************************************************************************/
int testConnectSrcToDest (char *pClientSrcIPAddr, struct sockaddr_in *srvrSockAddr, uint8_t displaySuccessConn)
{
	int rc = -1;
	int pollRC = 0;
	int fd = 0;
	int pollWait =  1000; // 1 second poll() timeout
	int sockReuse = 1;
	int flags = 0;
	int soerror = 0;
	int retries = 0;
	int failedToConnect = 1;

	char pServerDestIPAddr[64];
	struct pollfd pollFDS[1];

	socklen_t sockoptlen = sizeof(int);
	socklen_t inaddr_len;

	struct sockaddr_in *clntSockAddr = NULL;

	/* --------------------------------------------------------------------------------
	 * Allocate memory for the client socket address structure to be used to make
	 * a connection with the source ip addresses and server destination ip addrs.
	 * -------------------------------------------------------------------------------- */
	clntSockAddr = calloc(1, sizeof(struct sockaddr_in));
	if (clntSockAddr) {
		do {
			/* Create a socket object */
			if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
				MBTRACE(MBERROR, 1, "Failed to create a temporary socket to test connections from src ip %s errno=%d\n", pClientSrcIPAddr, errno);
				break;
			}

			/* ------------------------------------------------------------------------
			 * In some tests we will need to reuse connections in TIME_WAIT state, must
			 * set SO_REUSEADDR socket options.
			 * ------------------------------------------------------------------------ */
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockReuse, sizeof(sockReuse)) < 0) {
				MBTRACE(MBERROR, 1, "Unable to set the SO_REUSEADDR socket option %d: errno=%d\n", sockReuse, errno);
				break;
			}

			/* Set the socket for non-blocking I/O */
			flags = fcntl(fd, F_GETFL) | O_NONBLOCK;
			if (fcntl(fd, F_SETFL, flags) == -1) {
				MBTRACE(MBERROR, 1, "Unable to set a socket to non-blocking.\n");
				break;
			}

			/* ----------------------------------------------------------------------------
		 	 * Attempt a bind to each source IP in the environment variable SIPList.
		 	 * ---------------------------------------------------------------------------- */
			clntSockAddr->sin_family = AF_INET;
			clntSockAddr->sin_addr.s_addr = inet_addr(pClientSrcIPAddr);

			if (bind(fd, clntSockAddr, sizeof(struct sockaddr_in)) < 0) {
				MBTRACE(MBERROR, 1, "Unable to bind client socket to interface %s: errno=%d (%s:%d)\n",
                                    pClientSrcIPAddr,
                                    errno,
                                    __FILE__,
                                    __LINE__);
				break;
			}

			/* --------------------------------------------------------------------------------
			 * Copy the Server IP address to the local stack variable.
			 * -------------------------------------------------------------------------------- */
			pServerDestIPAddr[0] = '\0';
			strcpy(pServerDestIPAddr, inet_ntoa(srvrSockAddr->sin_addr));

			/* To confirm that destination is valid establish a TCP connection */
			if (connect(fd, srvrSockAddr, sizeof(struct sockaddr_in)) < 0) {
				if (errno != EINPROGRESS) {
					char errString[MAX_ERROR_LINE_LENGTH] = {0};
					snprintf(errString, MAX_ERROR_LINE_LENGTH, "Unable to connect to %s:%d from source IP address %s (errno=%d)\n",
														       pServerDestIPAddr,
															   ntohs(srvrSockAddr->sin_port),
															   pClientSrcIPAddr,
		                                                       errno);
					MBTRACE(MBWARN, 6, "%s", errString);
					break;
				}
			}

			/* ------------------------------------------------------------------------
			 * Replaced the select() call with a call to poll(). select() only supports
			 * up to 1024 connects, where poll() uses a structure that supports larger
			 * than 1024.
			 * ------------------------------------------------------------------------ */
			memset(pollFDS, 0, sizeof(pollFDS));  /* Reset/clear pollFDS */
			pollFDS[0].fd = fd;                   /* Set the fd field to the current file descriptor */
			pollFDS[0].events = POLLOUT;          /* Set the events to POLLOUT. */

			/* ------------------------------------------------------------------------
			 * Test using poll() to see if the connection is successful.  Will attempt
			 * to try 10 times along with the poll command waiting up to 5 seconds.
			 * ------------------------------------------------------------------------ */
			do {
				pollRC = poll(pollFDS, 1, pollWait);
				/* --------------------------------------------------------------------
				 * Check the return code and if errno isn't equal to EINTR then provide
				 * a message that it failed to make a connection between the source IP
				 * and the destination IP.
				 * -------------------------------------------------------------------- */
				if (pollRC < 0) {
					if (errno != EINTR) {
						MBTRACE(MBERROR, 1, "poll() failed to connect to ip address %s(fd=%d): errno=%d\n",
							                pServerDestIPAddr,
		                                    fd,
		                                    errno);
						break;
					}
				} else {
					if (pollRC == 0) {
						MBTRACE(MBWARN, 6, "Timed out trying to connect to %s:%d from source IP address %s (errno=%d). Retrying...\n",
											pServerDestIPAddr,
											ntohs(srvrSockAddr->sin_port),
											pClientSrcIPAddr,
											errno);
						retries++;
						continue;
					}

					if ((pollFDS[0].revents & POLLOUT) == 0) {
						retries++;
						continue;
					}
				}

				/* --------------------------------------------------------------------
				 * Performs a getsockopt() and if the connection is successful
				 * (soerror = 0) then set the failedToConnect flag = 0, rc = 0 and
				 * break out of the do loop.
				 * -------------------------------------------------------------------- */
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&soerror, &sockoptlen) == 0) {
					if (soerror == 0) {
						failedToConnect = 0;
						rc = 0;
						break; /* successfully connected */
					}

					/* ----------------------------------------------------------------
					 * If the soerror == EINPROGRESS then increment the retry counter
					 * and continue.   If it is not EINPROGRESS then it was unsuccessful
					 * and need to break out.
					 * ---------------------------------------------------------------- */
					if (soerror == EINPROGRESS) {
						retries++;
						continue;
					} else {
						MBTRACE(MBERROR, 1, "Unable to connect to %s:%d from source IP address %s (errno=%d)\n",
					                        pServerDestIPAddr,
											ntohs(srvrSockAddr->sin_port),
							                pClientSrcIPAddr,
			                                soerror);
						break;
					}
				} else {
					/* Unable to read the SO_ERROR for the connection, so break out of the loop. */
					MBTRACE(MBERROR, 1, "Could not read the SO_ERROR socket option for connection to ip address %s(fd=%d): errno=%d\n",
							            pServerDestIPAddr,
							            fd,
		                                errno);
					break;
				}
			} while(retries < MAX_PING_RETRIES);

			/* ------------------------------------------------------------------------
			 * Check the failedToConnect flag which is set to:
			 *   0 = Successfully connected.
			 *   1 = Failed to connect.
			 *
			 * If it failed provide a failure message and get out.
			 * ------------------------------------------------------------------------ */
			if (failedToConnect) {
				MBTRACE(MBWARN, 1, "Unable to connect to %s:%d from source IP address %s. Try pinging %s and make sure the destination is listening on tcp port %d. " \
									"Looking for a local IPv4 addresses from which a connection can be made to the server\n",
									pServerDestIPAddr,
									ntohs(srvrSockAddr->sin_port),
									pClientSrcIPAddr,
						            pServerDestIPAddr,
						            ntohs(srvrSockAddr->sin_port));
				break;
			}

			/* ------------------------------------------------------------------------
			 * Obtain the current address to which the socket is bound to.  Set up the
			 * inaddr_len variable to the size of the sockaddr_in.  If the rc = -1 the
			 * provide back a failure message.
			 * ------------------------------------------------------------------------ */
			inaddr_len = sizeof(struct sockaddr_in);
			if (getsockname(fd,(struct sockaddr *) clntSockAddr, &inaddr_len) == -1) {
				MBTRACE(MBERROR, 1, "Unable to get the source interface and port information from the socket connected to the " \
	                                "destination at ip address %s: errno=%d\n",
						            pServerDestIPAddr,
	                                errno);
				rc = RC_TCPIP_ERROR;
				break;
			}

			/* ------------------------------------------------------------------------
			 * Check to see if the successful connection should be displayed, which
			 * is determined by the calling routine.
			 * ------------------------------------------------------------------------ */
			if (displaySuccessConn == DISPLAY_GOOD_CONN) {
				/* Print src ip and dst ip */
				fprintf(stdout, "(i) Connected to dst ip: %s from src ip: %s\n",
				                pServerDestIPAddr,
                                pClientSrcIPAddr);
				fflush(stdout);
			}
		} while(0);

		/* ----------------------------------------------------------------------------
		 * If the connect() > 0 then need to close the file descriptor.  Set the value
		 * to -1 to ensure that is closed and not valid.
		 * ---------------------------------------------------------------------------- */
		if (fd > 0) {
			close(fd);
			fd = -1;
		}

		/* Free the memory for the client socket address structure. */
		free(clntSockAddr);
	} else
		rc = provideAllocErrorMsg("the array of client sockaddr_in", sizeof(struct sockaddr_in), __FILE__, __LINE__);

	return rc;
} /* testConnectSrcToDest */

/* *************************************************************************************
 * Populate the srcIPMapEntry list with source IPs that are able to connect
 * to the destination IP address provided.
 *
 * @param[in]  dstIPAddr           = destination IP Address to attempt to connect to.
 * @param[in]  dstPort             = destination Port to attempt to connect to
 * @param[in]  srcIPMapEntry       = the list of source IP addresses to build from the connection tests
 * @param[in]  arraySrcIPs         = array of source IP addresses to test
 * @param[in]  numSrcIPs           = number of source IP addresses to test
 *
 * @return 0   =
 *        -1   = An error/failure occurred.
 * *************************************************************************************/
int createSIPArrayList (char *dstIPAddr, int dstPort, srcIPList_t *srcIPMapEntry, char **arraySrcIPs, int numSrcIPs)
{
	int rc = 0;
	int connectCtr = 0;
	struct sockaddr_in srvrSockAddr;
	struct hostent *dest = NULL;

	dest = gethostbyname(dstIPAddr);	/* Set up the server sockaddr structure */
	if (dest) {
		srvrSockAddr.sin_addr = *(struct in_addr *)dest->h_addr_list[0];
		srvrSockAddr.sin_port = htons(dstPort);
		srvrSockAddr.sin_family = AF_INET;
	} else {
		MBTRACE(MBERROR, 1, "Unable to gethostbyname() for dstIPAddr %s while testing source IP addresses\n", dstIPAddr);
		return RC_UNABLE_TO_RESOLVE_DNS;
	}

	for (int i = 0 ; i < numSrcIPs ; i++ ) {
		rc = testConnectSrcToDest(arraySrcIPs[i], &srvrSockAddr, DONT_DISPLAY_GOOD_CONN);
		if (rc == 0) {
			srcIPMapEntry->sipArrayList[connectCtr] = strdup(arraySrcIPs[i]);
			if(srcIPMapEntry->sipArrayList[connectCtr] == NULL) {
				return provideAllocErrorMsg("srcIPList array element", strlen(arraySrcIPs[i]), __FILE__, __LINE__);
			}
			connectCtr++;
		}
	}

	if (connectCtr > 0) { // the destination IP and port must be reachable from at least 1 of the provided source IP
		srcIPMapEntry->numElements = connectCtr;
		rc = 0;
	} else {
		MBTRACE(MBERROR, 1, "Failed to reach destination IP %s and port %d from any of the provided source IP Addresses in SIPList.\n", dstIPAddr, dstPort);
		rc = RC_DEST_UNREACHABLE;
	}

	return rc;
} /* createSIPArrayList */

/* *************************************************************************************
 * getBufferSize
 *
 * Description:  Get a buffer size.  The buffer size is a string containing an optional
 *               suffix of K or M to indicate thousands or millions.
 *
 *   @param[in]  ssize               = x
 *   @param[in]  defaultSize         = x
 *
 *   @return 0   = Unable to determine the size of the buffer requested.
 *           x   = Size of buffer in bytes.
 * *************************************************************************************/
int getBufferSize (const char *ssize, const char *defaultSize)
{
    int val;
    char *eos;

    if (!ssize)
        ssize = defaultSize;

    val = strtoul(ssize, &eos, 10);
    if (eos) {
        while (*eos == ' ')
            eos++;

        if (*eos == 'k' || *eos == 'K')
            val *= 1024;
        else if (*eos == 'm' || *eos == 'M')
            val *= (1024 * 1024);
        else if (*eos)
            val = 0;
    }

    /* The default is zero, but anything else which resolves to zero is an error */
    if ((val == 0) && strcmp(ssize, "0"))
        MBTRACE(MBWARN, 1, "The buffer size %s is not correct and is ignored.\n", ssize);

    return val;
} /* getBuffSize */

/* *************************************************************************************
 * countConnections
 *
 * Description:  Count the connection states based on the specified requested state.
 *               Caller must call this function with a single state.
 *
 *   @param[in]  mbinst              = A pointer to a submitter thread's mqttbench_t.
 *   @param[in]  clientType          = Client type (Consumer, Producer or Dual Client)
 *   @param[in]  reqState            = A particular connection status that is requested
 *                                     to be counted.
 *   @param[in]  stateType           = TCP or MQTT state.
 *
 *   @return x   = Number of connections that are in the state requested.
 * *************************************************************************************/
int countConnections (mqttbench_t *mbinst, int clientType, int reqState, int stateType, int *connStats)
{
	int rc = 0;
	int i, j;
	int stateCt = 0;
	int totalCt = CONN_MQTT_CIP;

	int getStatsClientStateValues[] = { HANDSHAKE_IN_PROCESS, \
                                        SOCK_CONNECTED, \
                                        SHUTDOWN_IN_PROCESS, \
                                        SOCK_DISCONNECTED, \
                                        SOCK_ERROR, \
                                        SOCK_NEED_CREATE, \
                                        MQTT_CONNECT_IN_PROCESS, \
 	                                    MQTT_CONNECTED, \
 	                                    MQTT_PUBSUB, \
                                        MQTT_DISCONNECT_IN_PROCESS, \
                                        MQTT_DISCONNECTED, \
	                                    MQTT_UNSUBSCRIBED};

    ism_common_listIterator *pIter;
    ism_common_list_node *currNode;

    mqttclient_t *client;

    if (mbinst->clientTuples) {
    	/* Create the iterator to walk each of the consumers and producers to get stats. */
    	pIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
    	if (pIter) {
    		ism_common_list_iter_init(pIter, mbinst->clientTuples);

    		/* ------------------------------------------------------------------------
    		 *
    		 * ------------------------------------------------------------------------ */
    		if (stateType == STATE_TYPE_UNIQUE) {
    			if (reqState == ALL_CONNECT_STATES) {
    				while ((currNode = ism_common_list_iter_next(pIter)) != NULL) {
    					client = (mqttclient_t *)(currNode->data);

						for ( i = 0 , j=totalCt ; i < totalCt ; i++, j++ ) {
							if ((client->trans->state & getStatsClientStateValues[i]) > 0)
								connStats[i] += 1;

							if (j < NUM_CONNECT_STATES) {
								if ((client->protocolState & getStatsClientStateValues[j]) > 0)
									connStats[j] += 1;
							}
						}
					}
				} else if (reqState == ALL_TOPICS_UNSUBACK) {
					while ((currNode = ism_common_list_iter_next(pIter)) != NULL) {
						client = (mqttclient_t *)(currNode->data);

						if (((mqttclient_t *)(currNode->data))->allTopicsUnSubAck)
							stateCt++;
					}
				}
    		} else {
    			/* Check if requesting a TCP state. */
    			if (stateType == STATE_TYPE_TCP) {
   					while ((currNode = ism_common_list_iter_next(pIter)) != NULL) {
   						if ((((mqttclient_t *)(currNode->data))->trans->state & reqState) > 0) {
   							stateCt++;
    					}
    				}
    			} else if (stateType == STATE_TYPE_MQTT) {   /* Requesting a MQTT State */
    				while ((currNode = ism_common_list_iter_next(pIter)) != NULL) {
    					if ((((mqttclient_t *)(currNode->data))->protocolState & reqState) > 0) {
    						stateCt++;
    					}
   					}
    			}
    		}

    		ism_common_list_iter_destroy(pIter);
    		free(pIter);
    	} else {
    		rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
    		exit (rc);
    	}
    }

	return stateCt;
} /* countConnections */

/* *************************************************************************************
 * forceDisconnect
 *
 * Description:  Force a disconnect for all the clients associated with the specified
 *               thread.  If the client->trans->state doesn't = SOCK_DISCONNECT then
 *               call removeTransportFromIOThread.
 *
 *   @param[in]  mbinst              = A pointer to a submitter thread's mqttbench_t.
 * *************************************************************************************/
void forceDisconnect (mqttbench_t *mbinst)
{
	int rc = 0;
	ism_common_listIterator *pIter;
    ism_common_list_node *currNode;

    /* Create the iterator to walk each of the consumers and producers to get stats. */
    pIter = (ism_common_listIterator *)calloc(1, sizeof(ism_common_listIterator));
    if (pIter) {
    	ism_common_list_iter_init(pIter, mbinst->clientTuples);

       	while ((currNode = ism_common_list_iter_next(pIter)) != NULL) {
       		if ((((mqttclient_t *)(currNode->data))->trans->state & SOCK_DISCONNECTED) == 0)
       			removeTransportFromIOThread(((mqttclient_t *)(currNode->data))->trans);
    	}

        ism_common_list_iter_destroy(pIter);
    	free(pIter);
    } else {
    	rc = provideAllocErrorMsg("an iterator", (int)sizeof(ism_common_listIterator), __FILE__, __LINE__);
   		exit (rc);
    }
} /* forceDisconnect */

/* *************************************************************************************
 * Resolve the DNS name to a list of IPv4 addresses
 *
 *   @param[in]  dnsName      = Destination Server DNS name.
 *   @param[in]  dstIPList    = the pointer used to return the allocated dstIPList_t object (list of resolved IPv4 addresses)
 *
 *   @return 0   = Successfully resolved the DNS name
 *          >0   = Failed to resolve the DNS name
 * *************************************************************************************/
int resolveDNS2IPAddress (char *dnsName, destIPList **dstIPList)
{
	int rc = 0;
    struct addrinfo hints;
    struct addrinfo *result = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;                  /* Return IPv4 choices */
    hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(dnsName, NULL, &hints, &result);
    if(rc) {
		MBTRACE(MBERROR, 1, "Unable to resolve the DNS Name: %s (rc: %d)\n", dnsName, rc);
		return RC_UNABLE_TO_RESOLVE_DNS;
    }

	struct addrinfo *ai;
	int dnsRecordCount = 0;
	char dnsRecords[MAX_DNS_RECORDS][INET_ADDRSTRLEN];

	/* Iterate through list of DNS records returned by getaddrinfo and get numerical IP DNS records */
	for(ai = result; (ai != NULL) && (dnsRecordCount < MAX_DNS_RECORDS); ai = ai->ai_next){
		rc = getnameinfo(ai->ai_addr, ai->ai_addrlen, dnsRecords[dnsRecordCount], INET_ADDRSTRLEN, NULL, 0 , NI_NUMERICHOST);
		if(rc){
			freeaddrinfo(result);
			MBTRACE(MBERROR, 1, "Failed to get numerical IP DNS records returned for DNS name %s (errorCode=%d)\n", dnsName, rc);
			return RC_UNABLE_TO_RESOLVE_DNS;
		}
		dnsRecordCount++;
	}

    if (result) {
    	freeaddrinfo(result);
    }

	if(dnsRecordCount == 0) {
		MBTRACE(MBERROR, 1, "No DNS records were returned for DNS name %s\n", dnsName);
		return RC_UNABLE_TO_RESOLVE_DNS;
	}

	// Allocate dstIPList_t object to hold resolved numerical IP DNS records
	*dstIPList = (destIPList *) calloc(1, sizeof(dstIPList_t));
	if(*dstIPList == NULL) {
		return provideAllocErrorMsg("dstIPList_t object", sizeof(dstIPList_t), __FILE__, __LINE__);
	}

	((dstIPList_t *) *dstIPList)->dipArrayList = (char **) calloc(dnsRecordCount, sizeof(char*));
	if(((dstIPList_t *) *dstIPList)->dipArrayList == NULL) {
		return provideAllocErrorMsg("dstIPList_t dipArrayList", sizeof(char *) * dnsRecordCount, __FILE__, __LINE__);
	}
	((dstIPList_t *) *dstIPList)->numElements = dnsRecordCount;

	/* Copy numerical IP DNS records from stack memory to heap */
	for(int i=0; i<dnsRecordCount; i++){
		((dstIPList_t *) *dstIPList)->dipArrayList[i] = strdup(dnsRecords[i]);
		if(((dstIPList_t *) *dstIPList)->dipArrayList[i] == NULL){
			return provideAllocErrorMsg("dstIPList_t dipArrayList object", sizeof(dnsRecords[i]), __FILE__, __LINE__);
		}
	}

    return rc;
}

/* *************************************************************************************
 * updateDNS2IPAddr - Re-Resolve the DNS name to a list of IPv4 addresses
 *
 *   @param[in]  dnsName      = Destination Server DNS name.
 *   @param[in]  dstIPList    = the pointer used to return the allocated dstIPList_t object (list of resolved IPv4 addresses)
 *
 *   @return 0   = Successfully resolved the DNS name
 *          >0   = Failed to resolve the DNS name
 * *************************************************************************************/
int updateDNS2IPAddr (char *dnsName, char *serverIPAddr)
{
	int rc = 0;
    struct addrinfo hints;
    struct addrinfo *result = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;                  /* Return IPv4 choices */
    hints.ai_socktype = SOCK_STREAM;            /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(dnsName, NULL, &hints, &result);
    if(rc) {
		MBTRACE(MBERROR, 1, "Unable to resolve the DNS Name: %s (rc: %d)\n", dnsName, rc);
		return RC_UNABLE_TO_RESOLVE_DNS;
    }

	struct addrinfo *ai;
	char dnsRecords[INET_ADDRSTRLEN];

	/* Iterate through list of DNS records returned by getaddrinfo and get 1st numerical IP DNS records */
	for (ai = result; ai != NULL ; ai = ai->ai_next){
		rc = getnameinfo(ai->ai_addr, ai->ai_addrlen, dnsRecords, INET_ADDRSTRLEN, NULL, 0 , NI_NUMERICHOST);
		if (rc) {
		    freeaddrinfo(result);
			MBTRACE(MBERROR, 1, "Failed to get numerical IP DNS records returned for DNS name %s (errorCode=%d)\n", dnsName, rc);
			return RC_UNABLE_TO_RESOLVE_DNS;
		} else {
			strcpy(serverIPAddr, dnsRecords);
			break;
		}
	}

	if (result)
    	freeaddrinfo(result);

	if (serverIPAddr[0] == '\0') {
		MBTRACE(MBERROR, 1, "No DNS records were returned for DNS name %s\n", dnsName);
		return RC_UNABLE_TO_RESOLVE_DNS;
	}

    return rc;
}

/* *************************************************************************************
 * obtainSocketErrors
 *
 * Description:  Obtain the socket errors, and copy into the arraySocketErrors, which is
 *               used by doCommands.
 *
 *   @param[in]  arraySocketErrors   = Array containing the socket errors for an IOP
 *   @param[in]  iopNum              = Number of IOPs
 *
 *   @return 0   = Successful Completion.
 * *************************************************************************************/
int obtainSocketErrors (char **arraySocketErrors, int iopNum)
{
	int rc = 0;
	int i;
	int indxError;

	ioProcThread_t *iop = ioProcThreads[iopNum];
	indxError = iop->nextErrorElement;

	for ( i = 0 ; i < MAX_NUM_SOCKET_ERRORS ; i++ ) {
		if (indxError == 0)
			indxError = MAX_NUM_SOCKET_ERRORS - 1;
		else
			indxError--;

		arraySocketErrors[i] = iop->socketErrorArray[indxError];
	}

    return rc;
} /* obtainSocketErrors */

/*
 * Create a TCP socket and perform a blocking connect to the destination IP and port provided
 * and send the buffer provided to the destination server
 *
 * @param[in]	dstIP  				=	the IPv4 address of the server to connect to
 * @param[in]	dstPort				= 	the port number that the server is listening on
 * @param[in]	buf					=   the buffer to send
 * @param[in]	buflen				=   the length of the buffer to send
 *
 * @return 0 = OK, anything else is an error
 */
int basicConnectAndSend(char *dstIP, uint16_t dstPort, char *buf, int buflen) {
    int rc = 0;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1) {
		MBTRACE(MBERROR, 1, "Failed to create client socket for connection to %s:%u\n", dstIP, dstPort);
		return RC_TCP_SOCK_ERROR;
	}

	struct hostent *entry;
	struct sockaddr_in server;

	if( (entry = gethostbyname(dstIP)) == NULL ) {
		MBTRACE(MBERROR, 1, "Unable to resolve server name %s\n", dstIP);
		close(fd);
		return RC_TCPIP_ERROR;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	memcpy(&server.sin_addr.s_addr, entry->h_addr_list[0], entry->h_length);
	server.sin_port = htons(dstPort);

	/* Connect retry loop */
	int retryCount = 0;
	do {
		if (connect(fd, (struct sockaddr*)&server, sizeof(server)) != 0) {
			rc = RC_TCPIP_ERROR;
		} else {
			rc = 0;
			break;  // successfully connected
		}

		retryCount++;
	} while(retryCount < MAX_CONN_BASIC_RETRIES);

	if(rc) {
		MBTRACE(MBERROR, 1, "Unable to connect to %s:%u\n", dstIP, dstPort);
		close(fd);
		return rc;
	}

	/* Send retry loop */
	int bytesSent = 0;
	int remainingLen = buflen;
	char *pBuf = buf;

	while(remainingLen > 0) {
		bytesSent = send(fd, pBuf, remainingLen, 0);
		if(bytesSent == -1) {
			rc = RC_TCPIP_ERROR;
			break;
		}

		pBuf += bytesSent;
		remainingLen -= bytesSent;
	}

	if(rc) {
		MBTRACE(MBERROR, 1, "Unable to write %d out of %d bytes to %s:%u\n", remainingLen, buflen, dstIP, dstPort);
	}

	close(fd);
	return rc;
}
