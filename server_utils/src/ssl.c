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

/*
 * Implement openssl specific methods to centralize the use of openssl APIs
 */
#define TRACE_COMP TLS
#include <ismutil.h>
#include <assert.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/safestack.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>

#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <alloca.h>
#include <stddef.h>
#include <curl/curl.h>

#ifdef OPENSSL_FIPS
#include <openssl/fips.h>
#endif
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

typedef struct dirent DIR_ENTRY;

static int sslUseSpinlocks = 1;
static pthread_mutex_t    sslMutex = PTHREAD_MUTEX_INITIALIZER;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static pthread_spinlock_t *sslLocksS = NULL;
static pthread_mutex_t    *sslLocksM = NULL;
static pthread_mutex_t    sslRandLock;
#endif
int g_cunitCRLcount = 0;
const char * g_keystore;
static int g_keystore_len;
const char * g_truststore;
static const char * g_pskdir;
static int g_pskdir_len;
static int isSSLInited=0;
static int checkCRL = 1;

static DH* dh2048 = NULL;
static DH* dh4096 = NULL;
static DH* dh8192 = NULL;
static EC_KEY * ecdh = NULL;

typedef enum {
    CRL_OK,
    CRL_EXPIRED,
    CRL_FUTURE,
    CRL_INVALID
} crlState_t;

static ism_byteBufferPool pool64B = NULL;
static ism_byteBufferPool pool128B = NULL;
static ism_byteBufferPool pool256B = NULL;
static ism_byteBufferPool pool512B = NULL;
static ism_byteBufferPool pool1KB = NULL;
static ism_byteBufferPool pool2KB = NULL;
static pthread_rwlock_t   pskMapLock;
static ismHashMap * pskMap = NULL;
static ism_psk_notify_t pskNotifer = NULL;
static void sslGatherErr(concat_alloc_t * buf);
static int crlVerifyCB(int good, X509StoreCtx * ctx);
int ism_common_isServer(void);
int ism_common_isProxy(void);
int ism_common_isBridge(void);

#define SSL_ORG_CFG_DISABLED        0
#define SSL_ORG_CFG_ENABLED         1


/*
 * CRL waiter object
 */
typedef struct tlsCrlWait_t {
    struct tlsCrlWait_t * next;
    ima_transport_info_t * transport;
    SSL * transport_ssl;
    void * replyCrlWait_cb;  /* Callback location */
    void * userdata;         /* User data for callback */
    ism_time_t startwait;    /* Time at which we started waiting (for stats) */
    uint32_t rc;             /* ISMRC */
    uint32_t verify_rc;      /* verify_cert return code */
    uint32_t resv;
    int      count;          /* Number of CRL names */
    char *   crls [1];       /* CRL names */
} tlsCrlWait_t;


/*
 * CRL object for proxy orgConfig
 */
typedef struct tlsCrl_t {
    struct tlsCrl_t * next;             /* Linked list */
    const char *      name;             /* CRL URI */
    uint8_t           state;            /* TLSCRL_* */
    uint8_t           inprocess;        /* Being downloaded */
    uint8_t           delta;            /* Is a delta */
    uint8_t           resv[5];
    ism_time_t        valid_ts;         /* Valid until time (next update from CRL)   */
    ism_time_t        last_update_ts;   /* last updated (filetime from dist point) */
    ism_time_t        next_update_ts;   /* next update  */
    int64_t           crlNumber;        /* CRL number */
    int64_t           baseCrlNumber;    /* CRL base number (only for delta) */
} tlsCrl_t;

#define TLSCRL_Never  0      /* Not yet in trust store */
#define TLSCRL_Valid  1      /* In truststore */
#define TLSCRL_Failed 2      /* Failed download, not in trust store */


/*
 * Per org configuration used in proxy
 */
typedef struct tlsOrgConfig_t {
    const char *    name;
    SSL_CTX *       ctx;
    char *          serverCert;
    char *          serverKey;
    char *          trustCerts;
    ism_common_list trustCertsList;
    ism_timer_t     crlUpdateTimer;
    pthread_mutex_t lock;
    uint32_t        generation;
    uint8_t         requireClientCert;
    uint8_t         state;
    uint8_t         resv1;
    int8_t          useCount;
    tlsCrl_t *      crl;
    tlsCrlWait_t *  waiters;
} tlsOrgConfig_t;

/*
 * CA list of trust store
 */
typedef struct sslTrustCertData_t {
    X509 * cert;
    EVP_PKEY * pkey;          /* Public key */
    ism_common_list crlLocations;
    int useCount;
} sslTrustCertData_t;

#define NUM_OF_CU_THREADS 3

/*
 * CRL update thread structure
 */
typedef struct crlUpdateThread_t {
    ism_threadh_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    ism_common_list updateReqsList;
} crlUpdateThread_t;

static crlUpdateThread_t cuThreads[NUM_OF_CU_THREADS];

ismHashMap * orgConfigMap = NULL;
static SSL_CTX * defaultCTX = NULL;
static const char * g_tlsCiphers = NULL;
static int allowLocalCertFiles = 0;
static ism_time_t crlUpdateDelay = 0;

XAPI ism_time_t ism_ssl_convertTime(const ASN1_TIME * ctime);
static int crlUpdateTimer(ism_timer_t key, ism_time_t timestamp, void * userdata);

/*
 * Initialize buffer pools
 */
static void initializeBufferPools(int useBufferPool) {
#ifndef SSL_MALLOC_DEBUG
    pool64B = ism_common_createBufferPool(64,7*1024*useBufferPool,7*1024*useBufferPool,"SSL64B");
    pool128B = ism_common_createBufferPool(128,6*1024*useBufferPool,6*1024*useBufferPool,"SSL128B");
    pool256B = ism_common_createBufferPool(256,2*1024*useBufferPool,2*1024*useBufferPool,"SSL256B");
    pool512B = ism_common_createBufferPool(512,4*1024*useBufferPool,4*1024*useBufferPool,"SSL512B");
    pool1KB = ism_common_createBufferPool(1024,1024*useBufferPool,1024*useBufferPool,"SSL1KB");
    pool2KB = ism_common_createBufferPool(2048,1024*useBufferPool,1024*useBufferPool,"SSL2KB");
#endif
}

/*
 * Strip a line end.
 * This is used for PSK processing
 */
static void stripLineEnd(char * cp) {
    char * eol = cp + strlen(cp);
    while (eol > cp && (eol[-1]=='\n' || eol[-1]=='\r'))
        eol--;
    *eol = 0;
}

/*
 * Parse one CSV field.
 * The field is at the current location, and the location of the next field is returned.
 * @param cp
 * @param expect_comma
 * @return The next field, which might point to a NULL if at end of string.
 *         Return NULL if the next field is not as expected.
 */
static char * csvfield(char * cp, int expect_comma) {
    /* Simple case of unquoted */
    if (*cp != '"') {
        char * pos = strchr(cp, ',');
        if (pos) {
            *pos++ = 0;
            return expect_comma ? pos : NULL;
        } else {
            return expect_comma ? cp + strlen(cp) : NULL;
        }
    }

    /* Complex case of quoted */
    else {
        char * outp = cp;
        cp++;
        while (*cp) {
            if (*cp == '"') {
                /* End followed by comma */
                if (cp[1] == ',') {
                    *outp = 0;
                    return expect_comma ? cp+2 : NULL;
                }
                /* End followed by end of string */
                if (cp[1] == 0) {
                    *outp = 0;
                    return expect_comma ? cp+1 : NULL;
                }
                if (cp[1] == '"') {  /* Two quotes is a quote */
                    cp++;
                } else {             /* Anything else is an error */
                    *outp = 0;
                    return expect_comma ? NULL : cp+1;
                }
            }
            *outp++ = *cp++;
        }
        *outp = 0;
        return expect_comma ? NULL : cp;
    }
}


/*
 * Internal validation of PSK file
 */
static int  validatePSKFile(FILE * fin, uint32_t * pLine, int ignore) {
	char line[1024];
	char * id;
    int    keylen;
    int    idlen;
	int count = 0;
	int lineCounter = 0;

    while (fgets(line, sizeof(line) - 1, fin)) {
		char * key;
    	lineCounter++;

        /* Remove CR and LF */
    	stripLineEnd(line);

        if (!ism_common_validUTF8Restrict(line, strlen(line), UR_NoC0)) {
            ism_common_validUTF8Replace(line, strlen(line), line, UR_NoC0, '?');
            TRACE(4, "The PSK file is not valid on line %d: '%s'\n", lineCounter, line);
            *pLine = lineCounter;
            if (ignore)
                continue;
            count = -1;
            break;
        }

        /* Parse the id and key */
        id = line;
        key = csvfield(id, 1);
		if (key) {
            if (csvfield(key, 0)) {
                key = NULL;
            }
		}

		if (!key || !*key) {
	        TRACE(4, "The PSK file is not valid on line %d:  \"%s\"\n", lineCounter, line);
	        *pLine = lineCounter;
            if (ignore) continue;
	        count = -1;
	        break;
		}

        idlen = strlen(id);
        if (idlen > 255) {
            TRACE(4, "The PSK identity is too long on line %d: \"%s\"\n", lineCounter, id);
            *pLine = lineCounter;
            count = -1;
            break;
        }

		keylen = ism_common_fromHexString(key, NULL);
		if (keylen < 0 || keylen > 256) {
	        TRACE(4, "The PSK key is not valid on line %d:  \"%s\"\n", lineCounter, key);
	        *pLine = lineCounter;
            if (ignore) continue;
	        count = -1;
	        break;
		}
		count++;
    }
    return count;
}


/*
 * Read the PSK file.
 *
 * The file has already been validated at this point, so no extra error checking
 */
static int readPSKFile(FILE * fin, ismHashMap * map) {
	char   line[1024];
	char * id;
	int rc = 0;
	int lineCounter = 0;

    while (fgets(line, sizeof(line) - 1, fin)) {
		char * key;
		char * binkey;
		lineCounter++;

        /* Remove CR and LF */
        stripLineEnd(line);

        /* Parse the id and key */
        id = line;
        key = csvfield(id, 1);
        if (key) {
            if (csvfield(key, 0))
                key = NULL;
        }
		if (key) {
		    int keylen;
		    binkey = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,63),strlen(key)/2 + 1);
		    keylen = ism_common_fromHexString(key, binkey+1);
		    if (keylen < 0 || keylen > 256) {
		        TRACE(8, "PSK key conversion failed on line %d:  \"%s\"\n", lineCounter, key);
		    } else {
		        if (keylen > 0) {
		            binkey[0] = keylen-1;
		            ism_common_putHashMapElement(map, id, 0, binkey, NULL);
		            TRACEX(9, TLS, 0, "Add PSK: identify=%s key=%s\n", id, key);
		        } else {
		            key = ism_common_removeHashMapElement(map, id, 0);
		            if (key) {
		                TRACEX(9, TLS, 0, "Remove PSK: identity=%s\n", id);
		                ism_common_free(ism_memory_utils_sslutils,key);
		            }
		        }
		    }
		}
    }
    return rc;
}


/*
 * Free up the PSK map
 */
static void freePSKMap(ismHashMap * map) {
	ismHashMapEntry **entries = ism_common_getHashMapEntriesArray(map);
	int count = 0;
	while (entries[count] != ((void*) -1)) {
		ism_common_free(ism_memory_utils_sslutils,entries[count]->value);
		count++;
	}
	ism_common_freeHashMapEntriesArray(entries);
	ism_common_destroyHashMap(map);
}


/**
 * Validate a PSK file.
 *
 * Validate that the PSK file is good.  The PSK file must be a valid csv file with two items on each line.
 * <p>
 * The first item is an identity which must be a valid UTF-8 string and not contain C0 control characters
 * (codepoints less than space).  It must be less than 256 bytes in length.
 * <p>
 * The second item is a hex string which must have an even number of hex digits,
 * and not more than 512 hex digits (the resulting binary key must be 0 to 256 bytes in length).  A zero length
 * key disables the use of a PSK cipher for this identity and will result in a disconnect if a PSK cipher is
 * selected.
 * <p>
 * If an error occurs, the line number in the file (starting with 1) is returned and validation fails.
 * If an invalid file is used in apply, any invalid lines will be ignored (but traced).
 *
 * @param name   The name of the PSK file
 * @param line   (output) the line number at which an error occurred or number of PSK Entries.
 * @return An ISMRC return code
 */
XAPI int ism_ssl_validatePSKfile(const char * name, int32_t * line, int32_t *pskEntries) {
	FILE * fin = fopen(name,"r");
	int rc = ISMRC_OK;
	*pskEntries = 0;
    if (fin == NULL) {
        *line = 0;
    	return ISMRC_ConfigError;
    }
    if ((*pskEntries = validatePSKFile(fin, (uint32_t *)line, 0)) < 0) {
    	rc = ISMRC_ConfigError;
    }
    fclose(fin);
    return rc;
}


/*
 * Apply PSK file.
 *
 * Set the identity to PSK mappings from a file.  Any invalid entries will be
 * ignored and thus any checking should be done before calling this method.
 *
 * @param name   The name of the PSK file
 * @return An ISMRC return code
 */
XAPI int  ism_ssl_applyPSKfile(const char * fileName, int mapSize) {
	ismHashMap * newMap = NULL;
	ismHashMap * oldMap = NULL;
	int rc = ISMRC_OK;
	if (fileName && *fileName) {
		uint32_t line = 0;
		FILE * fin = fopen(fileName, "rb");
		if (fin) {
			if (mapSize == 0)
				mapSize = validatePSKFile(fin, &line, 1);
			if (mapSize > 0) {
				newMap = ism_common_createHashMap(mapSize, HASH_STRING);
				rewind(fin);
				if (readPSKFile(fin,newMap) != 0) {
					freePSKMap(newMap);
					newMap = NULL;
			        rc = ISMRC_ConfigError;
				}
			} else {
				if (mapSize < 0) {
				    LOG(ERROR, Server, 980, "%u", "The PreSharedKey file is not valid at line {0}.", line);
			        rc = ISMRC_ConfigError;
				}
			}
			fclose(fin);
		} else {
			if (errno != ENOENT) {
				int err = errno;
		        TRACE(4, "Failed to open PSK file %s: error=%s(%d) \n", fileName, strerror(err),err);
		        rc = ISMRC_ConfigError;
			}
		}
	}
	if (rc == ISMRC_OK) {
		int newCount = (newMap) ? ism_common_getHashMapNumElements(newMap) : 0;
		pthread_rwlock_wrlock(&pskMapLock);
		oldMap = pskMap;
		pskMap = newMap;
		pthread_rwlock_unlock(&pskMapLock);
		if (oldMap) {
			freePSKMap(oldMap);
		}
		if (newCount > 0)
		    LOG(INFO, Server, 981, "%u", "The PreSharedKey map is created with {0} entries.", newCount);
		if (pskNotifer)
			pskNotifer(newCount);
	}
	return rc;
}


/*
 * Add a client cert into the trust store for server
 */
static int addAllowedClientCert(ismHashMap * map, const char * crtFileName) {
    uint64_t hash;
    X509 * cert = NULL;
    FILE * fp = fopen(crtFileName, "r");
    if ( fp == NULL) {
        int err = errno;
        TRACE(4, "Unable to open client certificate file %s: error=%d(%s)\n", crtFileName, err, strerror(errno));
        return 0;
    }
    cert = PEM_read_X509(fp, NULL, NULL, NULL);
    if (cert == NULL) {
        char xbuf[8192];
        concat_alloc_t buf = { 0 };
        buf.buf = xbuf;
        buf.len = sizeof xbuf;
        sslGatherErr(&buf);
        TRACE(4, "Unable to parse client certificate %s: %s\n", crtFileName, buf.buf);
        if (buf.inheap)
            ism_common_free(ism_memory_utils_sslutils,buf.buf);
        return 0;
    }
    hash = X509_subject_name_hash(cert);
    ism_common_putHashMapElementLock(map, &hash, sizeof(hash), cert, NULL);
    return 1;
}

/*
 * OpenSSL callback method to check client certificates on server
 */
XAPI int ism_ssl_checkPreverifiedClient(int preverify_ok, X509_STORE_CTX *storeCTX, ism_getAllowedClientsMap getAllowedClients) {
    SSL * ssl = X509_STORE_CTX_get_ex_data(storeCTX, SSL_get_ex_data_X509_STORE_CTX_idx());
    ima_transport_info_t * transport = (ima_transport_info_t *)SSL_get_app_data(ssl);
    if (transport) {
        int depth = 0;
        ismHashMap * allowedClients = NULL;
        if (transport->originated)
            return preverify_ok;

        depth = X509_STORE_CTX_get_error_depth(storeCTX);
        if (depth > 0) {
            /* We have not yet validated the chain */
        	if ((transport->crtChckStatus & 0x20) == 0)
        		transport->crtChckStatus = (preverify_ok) ? 0x10 : 0x20;
            return 1;
        }

        /* The client is validated and chain is OK */
        if (preverify_ok && (transport->crtChckStatus & 0x10))
            return 1;

        /*
         * Allow the certificate as it is in our trust store.
         * We allow this even if the certificate failed validation
         */
        if (transport->crtChckStatus & 0x01) {
            SSL_set_verify_result(ssl, X509_V_OK);
            X509_STORE_CTX_set_error(storeCTX, X509_V_OK);
            return 1;
        }
        assert((transport->crtChckStatus & 0x02) == 0); //We've verified client cert already and it's bad
        allowedClients = getAllowedClients(transport);
        if (allowedClients) {
            X509 * clientCert = X509_STORE_CTX_get_current_cert(storeCTX);
            uint64_t hash = X509_subject_name_hash(clientCert);
            X509 * allowedCert =  ism_common_getHashMapElementLock(allowedClients, &hash, sizeof(hash));
            if (allowedCert && (X509_cmp(clientCert,allowedCert) == 0)) {
                int ec = X509_STORE_CTX_get_error(storeCTX);

                /* Allow even a bad certificate as it is in our trust store */
                transport->crtChckStatus |= 0x01;
                TRACE(6, "Allow precertified client: connect=%d from=%s:%u valid=%s (%d)\n", transport->index,
                        transport->client_addr, transport->clientport, X509_verify_cert_error_string(ec), ec);
                SSL_set_verify_result(ssl, X509_V_OK);
                X509_STORE_CTX_set_error(storeCTX, X509_V_OK);
                return 1;
            }
        }
    	transport->crtChckStatus |= 0x02;
    	return 0;
    }
    return preverify_ok;
}


/*
 * Read in all certs in the trust store directory for server
 */
XAPI ismHashMap * ism_ssl_initAllowedClientCerts(const char * profileName) {
    ismHashMap * result = NULL;
    int pathLen = strlen(g_truststore) + strlen(profileName) + 32;
    char * crtPath = alloca(pathLen);
    DIR  * dir;
    sprintf(crtPath, "%s/%s_allowedClientCerts", g_truststore, profileName);
    dir = opendir(crtPath);
    if (dir) {
        int maxName = pathconf(crtPath, _PC_NAME_MAX);
        DIR_ENTRY * currEnt = NULL;
        DIR_ENTRY entry = {0};
        char * crtFileName = alloca(pathLen + maxName +1);
        int counter = 0;
        result = ism_common_createHashMap(4096, HASH_INT64);

        do {
            int rc = ISMRC_OK;
            struct stat sb;
            rc = readdir_r(dir, &entry, &currEnt);
            if (rc || (currEnt == NULL))
                break;
            sprintf(crtFileName, "%s/%s", crtPath, currEnt->d_name);
            /* XFS does not support d_type field: DT_REG, replace with stat instead */
            if (!strcmp(".", currEnt->d_name) || !strcmp("..", currEnt->d_name)) /* no need to stat "." and ".." */
            	continue;
            rc = stat(crtFileName, &sb);
            if (rc == ISMRC_OK) {
            	if (S_ISREG(sb.st_mode)) {
            		if (addAllowedClientCert(result, crtFileName))
            			counter++;
            	}
            }
            else
            	break;
        } while (1);
        if (counter == 0) {
            /* No allowed certificates - return NULL */
            ism_common_destroyHashMap(result);
            result = NULL;
        }
    }
    return result;
}

/*
 * Cleanup the map of client certs on shutdown
 */
XAPI void ism_ssl_cleanAllowedClientCerts(ismHashMap *allowedClientsMap) {
    ism_common_destroyHashMapAndFreeValues(allowedClientsMap, (ism_freeValueObject)X509_free);
}


/*
 * Free up all buffer pools on shutdown
 */
static void destroyBufferPools(void) {
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
}


/*
 * Get a buffer from the buffer pool
 */
static inline void *getBuffer(size_t size) {
#ifndef SSL_MALLOC_DEBUG
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
#endif
    /*
    if (size <= (5*1024)) {
        return ism_common_getBuffer(pool5KB, 1)->buf;
    }
    if (size <= (18*1024)) {
        return ism_common_getBuffer(pool18KB, 1)->buf;
    }
    if (size <= (22*1024)) {
        return ism_common_getBuffer(pool22KB, 1)->buf;
    }
    if (size <= (34*1024)) {
        return ism_common_getBuffer(pool34KB, 1)->buf;
    }
    */
    return ism_allocateByteBuffer(size)->buf;
}

/*
 * Return a buffer to the buffer pool
 */
static inline void returnBuffer(void * buff) {
    ism_byteBuffer rb = (ism_byteBuffer)buff;
    rb--;
    ism_common_returnBuffer(rb, __FILE__, __LINE__);
}

/*
 * Get the threadID with an unsigned long type
 * This is used in the openssl locking support
 */
xUNUSED static unsigned long getThreadId(void) {
    return ((unsigned long) pthread_self());
}


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
    char         mbuf[256];
    char         lbuf[2048];

    for (;;) {
        rc = (uint32_t)ERR_get_error_line_data(&file, &line, &data, &flags);
        if (rc == 0)
            break;
        ERR_error_string_n(rc, mbuf, sizeof mbuf);
        if (flags&ERR_TXT_STRING) {
            snprintf(lbuf, sizeof lbuf, "%s:%u %s: %s\n", file, line, mbuf, data);
        } else {
            snprintf(lbuf, sizeof lbuf, "%s:%u %s\n", file, line, mbuf);
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
static void traceSSLErrorInt(const char *errMsg, const char * file, int line) {
	char xbuf[8192];
	concat_alloc_t buf = { 0 };
	buf.buf = xbuf;
	buf.len = sizeof xbuf;
	sslGatherErr(&buf);

	traceFunction(3, 0, file, line, "%s: cause=%s\n", errMsg, buf.buf);
	ism_common_freeAllocBuffer(&buf);
}

/*
 * Log an SSL error
 */
void ism_common_logSSLError(const char * errmsg, const char * file, int line) {
    char xbuf[8192];
    concat_alloc_t buf = { 0 };
    buf.buf = xbuf;
    buf.len = sizeof xbuf;
    sslGatherErr(&buf);

    ism_common_logInvoke(NULL, ISM_LOGLEV_ERROR, 998, "CWLNA0998", LOGCAT_Server, TRACE_DOMAIN, __FUNCTION__, file, line, "%s%s",
            "{0}: {1}", errmsg, buf.buf);
    ism_common_freeAllocBuffer(&buf);
}

#define traceSSLError(errMsg) traceSSLErrorInt(errMsg , __FILE__, __LINE__)

/*
 * External entry point
 */
void ism_common_traceSSLError(const char * errMsg, const char * file, int line) {
    traceSSLErrorInt(errMsg, file, line);
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
/*
 * openssl lock callback for spinlocks
 */
static void sslLockOpsS(int mode, int type, const char * file, int line) {
    if (mode & CRYPTO_LOCK) {
        if (type != CRYPTO_LOCK_RAND)
            pthread_spin_lock(&sslLocksS[type]);
        else
            pthread_mutex_lock(&sslRandLock);
    } else {
        if (type != CRYPTO_LOCK_RAND)
            pthread_spin_unlock(&sslLocksS[type]);
        else
            pthread_mutex_unlock(&sslRandLock);
    }
}


/*
 * openssl lock callback for mutex locks
 */
static void sslLockOpsM(int mode, int type, const char * file, int line) {
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&sslLocksM[type]);
    } else {
        pthread_mutex_unlock(&sslLocksM[type]);
    }
}
#endif

/*
 * Allocate for a TLS object
 */
static void* ssl_malloc(size_t size, const char *fn,  int ln) {
    return getBuffer(size);
}

/*
 * Just use system malloc
 */
static void * my_malloc(size_t size, const char *fn,  int ln) {
    return ism_common_malloc(ISM_MEM_PROBE(ism_memory_ssl_functions,72),size);
}


/*
 * Reallocate for a TLS object
 */
static void * ssl_realloc(void *p, size_t size, const char *fn,  int ln) {
    ism_byteBuffer buff = (ism_byteBuffer)p;
    char* result;
    if (p) {
        buff--;
        if (size <= buff->allocated)
            return p;
    }
    result = getBuffer(size);
    if (buff) {
        memcpy(result,buff->buf,buff->allocated);
        returnBuffer(p);
    }
    return result;
}

/*
 * Just use system realloc
 */
static void * my_realloc(void *p, size_t size, const char *fn,  int ln) {
    return ism_common_realloc(ISM_MEM_PROBE(ism_memory_ssl_functions,72),p,size);
}

/*
 * Free a TLS object
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void  ssl_free(void *p) {
    if (p)
        returnBuffer(p);
}
#else
static void ssl_free(void * p, const char * file, int line) {
    if (p)
        returnBuffer(p);
}
#endif

/*
 * Just use system free
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void my_free(void * p) {
#else
static void my_free(void * p, const char * file, int line) {
#endif
    ism_common_free(ism_memory_ssl_functions,p);
}

/*
 * Initialize SSL locks
 */
static void sslLockInit(void) {
    pthread_rwlockattr_t rw_attr;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    int i;
    pthread_mutexattr_t attr;
    int num = CRYPTO_num_locks();
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    CRYPTO_set_id_callback(getThreadId);

    if (sslUseSpinlocks) {
        sslLocksS = (pthread_spinlock_t*) OPENSSL_malloc(num * sizeof(pthread_spinlock_t));
        for (i = 0; i < num; i++) {
            pthread_spin_init(&sslLocksS[i], 0);
        }
        pthread_mutex_init(&sslRandLock, &attr);
        CRYPTO_set_locking_callback(sslLockOpsS);
    } else {
        sslLocksM = (pthread_mutex_t*) OPENSSL_malloc(num * sizeof(pthread_mutex_t));
        for (i = 0; i < num; i++) {
            if (i != CRYPTO_LOCK_RAND)
                pthread_mutex_init(&sslLocksM[i], NULL);
            else
                pthread_mutex_init(&sslLocksM[i], &attr);
        }
        CRYPTO_set_locking_callback(sslLockOpsM);
    }
    pthread_mutexattr_destroy(&attr);

#endif
    pthread_rwlockattr_init(&rw_attr);
    pthread_rwlockattr_setkind_np(&rw_attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&pskMapLock, &rw_attr);
    pthread_rwlockattr_destroy(&rw_attr);
}


/*
 * Clean up SSL locks
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void sslLockCleanup(void) {
    int i;
    int num = CRYPTO_num_locks();
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    if (sslUseSpinlocks) {
        if (sslLocksS) {
            for (i = 0; i < num; i++) {
                pthread_spin_destroy(&sslLocksS[i]);
            }
            pthread_mutex_destroy(&sslRandLock);
            OPENSSL_free((void*)sslLocksS);
        }
        sslLocksS = NULL;
    } else {
        if (sslLocksM) {
            for (i = 0; i < num; i++) {
                pthread_mutex_destroy(&sslLocksM[i]);
            }
            OPENSSL_free((void*)sslLocksM);
        }
        sslLocksM = NULL;
    }
    pthread_rwlock_destroy(&pskMapLock);
}
#endif


/*
 * Reset openssl locks
 */
XAPI void ism_ssl_locksReset(void) {
    pthread_mutex_lock(&sslMutex);
    if (isSSLInited==1) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        sslLockCleanup();
#endif
        sslLockInit();
    }
    pthread_mutex_unlock(&sslMutex);
    return;
}

/*
 * Return the value of a hex digit or -1 if not a valid hex digit
 */
static int hexValue(char ch) {
    if (ch <= '9' && ch >= '0')
        return ch-'0';
    if (ch >='A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >='a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}


static const char * hexdigit = "0123456789abcdef";
/*
 * Convert a byte array string to a hex string.
 * The resulting string is null terminated.
 * The invoker is required to give a large enough buffer for the output.
 * @return  The length of the output
 */
int ism_common_toHexString(const char * from, char * to, int fromlen) {
    int    i;
    for (i=0; i<fromlen; i++) {
        *to++ = hexdigit[((*from)>>4)&0x0f];
        *to++ = hexdigit[(*from)&0x0f];
        from++;
    }
    *to = 0;
    return fromlen*2;
}

/*
 * Convert a hex string to a binary array.
 * The invoker must give a large enough buffer for the output.
 * The output location can be null to check if the hex string is valid.
 *
 * @return The length of the output or -1 to indicate an error
 */
int ism_common_fromHexString(const char * from, char * to) {
    int   ret = 0;
    while (*from) {
        int val1 = hexValue(*from++);
        if (val1 >= 0) {
            int val2 = hexValue(*from++);
            if (val2 < 0) {
                return -1; /* Invalid second digit */
            }
            if (to)
                *to++ = (val1<<4) + val2;
            ret++;
        } else {
            return -1;     /* Invalid first digit */
        }
    }
    return ret;
}


/*
 * Set the default DH params for 2048 bit
 */
static DH *get_dh2048(void) {
	static unsigned char dh2048_p[] = { 0xD3, 0xD2, 0x50, 0xE4, 0x56, 0x29,
			0x1F, 0x3C, 0x78, 0xDD, 0x3F, 0x15, 0x85, 0xA8, 0x6C, 0x73, 0x92,
			0xEC, 0x22, 0x87, 0x79, 0x2E, 0x3F, 0xBD, 0xB5, 0x1E, 0xF6, 0x5B,
			0x6D, 0x16, 0x7E, 0x5D, 0x47, 0xB7, 0xB9, 0x60, 0x77, 0x88, 0x88,
			0xC1, 0x48, 0x99, 0x7E, 0x66, 0xAA, 0x83, 0x0C, 0x4B, 0xB4, 0xBC,
			0x8B, 0x72, 0xCD, 0x31, 0x1E, 0x90, 0x0F, 0x16, 0x51, 0xC7, 0x33,
			0xEC, 0x0F, 0x01, 0x54, 0xD9, 0x61, 0x87, 0x17, 0x58, 0x7B, 0xCB,
			0x8F, 0x15, 0x20, 0x2C, 0x30, 0xD8, 0xC5, 0x5A, 0x41, 0x3D, 0x6F,
			0xAD, 0x58, 0xB0, 0x17, 0x45, 0xF0, 0xF5, 0xF1, 0x23, 0x8F, 0xDC,
			0xB0, 0x10, 0xEE, 0x85, 0x96, 0x95, 0x75, 0x39, 0x41, 0x44, 0x02,
			0x96, 0x4A, 0xB9, 0x2E, 0x4C, 0xCF, 0x95, 0x24, 0xF9, 0x6A, 0x55,
			0x97, 0xB1, 0x6D, 0x5F, 0x5A, 0x13, 0x38, 0x19, 0xBB, 0x99, 0xFA,
			0x60, 0x7D, 0x9F, 0xA1, 0xE1, 0x94, 0xC8, 0xAF, 0x65, 0xA0, 0x3F,
			0xAC, 0x36, 0x47, 0xBE, 0x0E, 0x73, 0x68, 0xF7, 0x14, 0x03, 0x9C,
			0xC2, 0x15, 0x60, 0x3A, 0xD9, 0xFA, 0xE8, 0xD5, 0xE2, 0x4D, 0xA0,
			0x6E, 0x13, 0xDE, 0x0A, 0x34, 0xEF, 0x63, 0x5F, 0x1C, 0x3A, 0x95,
			0x38, 0xCE, 0xB2, 0x4A, 0x57, 0x96, 0xE1, 0x7D, 0x72, 0xB1, 0x5A,
			0x82, 0x4A, 0xE6, 0x96, 0x0A, 0x20, 0xF0, 0x3B, 0xEA, 0xB9, 0xF1,
			0x8E, 0xA1, 0xB8, 0x76, 0x8B, 0x3F, 0x18, 0x27, 0xEB, 0x0F, 0xCF,
			0x1D, 0x47, 0xAE, 0x1A, 0xCC, 0x44, 0x51, 0x8B, 0x10, 0x34, 0x84,
			0xCF, 0xB1, 0x89, 0xF5, 0x61, 0xD4, 0x5F, 0x3B, 0x5F, 0x7A, 0x8B,
			0xFD, 0xD8, 0xBB, 0x31, 0xEF, 0x40, 0x35, 0xFC, 0x65, 0x89, 0x1A,
			0x62, 0x03, 0x37, 0xF5, 0x59, 0x2F, 0xF7, 0x8F, 0xCB, 0x26, 0x7D,
			0xF5, 0x29, 0x83, 0x62, 0xF1, 0x1F, 0xBF, 0x9B, };
	static unsigned char dh2048_g[] = { 0x02, };
	DH *dh;

	if ((dh = DH_new()) == NULL)
		return (NULL);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	dh->p = BN_bin2bn(dh2048_p, sizeof(dh2048_p), NULL);
	dh->g = BN_bin2bn(dh2048_g, sizeof(dh2048_g), NULL);
	if ((dh->p == NULL) || (dh->g == NULL)) {
		DH_free(dh);
		return (NULL);
	}
#else
    DH_set0_pqg(dh, BN_bin2bn(dh2048_p, sizeof dh2048_p, NULL), NULL, BN_bin2bn(dh2048_g, sizeof dh2048_g, NULL));
#endif
	return (dh);
}

/*
 * Set the default DH params for 4096 bit
 */
static DH *get_dh4096(void) {
	static unsigned char dh4096_p[] = { 0x94, 0xEF, 0x5F, 0x1D, 0x4B, 0xD8,
			0x9B, 0x0B, 0x38, 0x9E, 0xE4, 0xB1, 0xD3, 0xE1, 0x9D, 0xA3, 0xC8,
			0xEF, 0xE3, 0x62, 0xFE, 0x33, 0xF5, 0x08, 0xE6, 0xBB, 0x33, 0xB6,
			0x89, 0x0E, 0x1B, 0x28, 0xBC, 0xDB, 0xEB, 0x71, 0xF8, 0x5D, 0x79,
			0xC1, 0x17, 0x6C, 0x1F, 0x2E, 0xE1, 0xC3, 0xEE, 0xAD, 0xCB, 0xAD,
			0x90, 0xA2, 0xC7, 0x39, 0xB9, 0xA9, 0xF2, 0x0E, 0x80, 0x79, 0x26,
			0x87, 0x86, 0x8B, 0xBE, 0xCA, 0xF2, 0x94, 0x20, 0xA5, 0x11, 0x60,
			0x59, 0x17, 0x14, 0x4C, 0x78, 0x9D, 0x6F, 0xB9, 0x9E, 0x18, 0x1D,
			0xC2, 0x65, 0x7E, 0xD0, 0xF4, 0x36, 0x96, 0x1B, 0x4D, 0x61, 0x37,
			0x69, 0x43, 0x89, 0x0D, 0x6D, 0x76, 0x97, 0x42, 0x19, 0x6F, 0xC0,
			0x93, 0x37, 0xFB, 0xF6, 0x3D, 0xAE, 0x18, 0xE4, 0x3E, 0xD3, 0x6E,
			0xF6, 0x21, 0x99, 0x22, 0xEA, 0x89, 0x63, 0xD2, 0xE8, 0xC2, 0x76,
			0x58, 0xA0, 0x6C, 0x93, 0x47, 0xA0, 0x3A, 0xA1, 0x47, 0xEF, 0xBB,
			0x30, 0xC2, 0x89, 0x2D, 0x69, 0x62, 0xE3, 0xD4, 0x46, 0xBF, 0xBA,
			0xB6, 0xDE, 0x08, 0x37, 0xEE, 0x41, 0xEF, 0xC7, 0xB9, 0xE6, 0xB7,
			0x72, 0x94, 0x25, 0x11, 0x4D, 0xE5, 0xB1, 0x11, 0x11, 0xDB, 0x86,
			0xB6, 0x79, 0xF0, 0xB7, 0x0F, 0xCD, 0x34, 0x87, 0xE1, 0xD1, 0x27,
			0x5D, 0x0E, 0x69, 0x5B, 0x43, 0x05, 0x2C, 0x62, 0x87, 0xDD, 0xA9,
			0x3F, 0xBD, 0x1C, 0xF0, 0x1C, 0x5F, 0x68, 0x66, 0xF2, 0x6A, 0x22,
			0x0E, 0x29, 0x06, 0x38, 0xD2, 0xB8, 0xB2, 0xE8, 0x3A, 0xE5, 0x8A,
			0x43, 0x95, 0x62, 0x5D, 0xEC, 0xD4, 0xCF, 0xE6, 0x83, 0x08, 0x82,
			0xED, 0x50, 0x7F, 0xAD, 0xC6, 0xAD, 0xF3, 0x03, 0xF3, 0x8E, 0x7E,
			0xB4, 0xAA, 0xFF, 0x48, 0x10, 0x4C, 0x8B, 0xAA, 0x48, 0xE8, 0x5B,
			0x16, 0xA5, 0x68, 0x4B, 0x70, 0x9A, 0x3E, 0x40, 0x46, 0x90, 0x96,
			0x69, 0xA4, 0xE5, 0x3D, 0x4C, 0x72, 0xDB, 0xBD, 0xFC, 0x67, 0x4F,
			0x4A, 0x0F, 0x15, 0x5C, 0xB2, 0x01, 0xEA, 0xB9, 0x54, 0x5E, 0x79,
			0xF1, 0x37, 0xA8, 0x3E, 0x6A, 0xEA, 0x14, 0xA9, 0x16, 0xE8, 0xE6,
			0x96, 0x17, 0x98, 0x82, 0xB0, 0x94, 0xFB, 0x7C, 0xAB, 0x70, 0x25,
			0xD9, 0xBC, 0x7F, 0xD9, 0x80, 0x0C, 0x21, 0x84, 0xF2, 0x8E, 0x49,
			0xC6, 0x00, 0x9E, 0xFD, 0x60, 0xB3, 0x98, 0x40, 0x47, 0x3D, 0x14,
			0x26, 0xC8, 0xF5, 0x26, 0x8B, 0x87, 0xAF, 0xB9, 0x11, 0x38, 0x53,
			0x84, 0x03, 0xF2, 0xCD, 0x75, 0x83, 0xE3, 0x93, 0x0D, 0xC6, 0xD5,
			0x41, 0xA8, 0xCB, 0x10, 0x86, 0x7C, 0x6E, 0x3A, 0x91, 0x66, 0x5D,
			0x28, 0x81, 0xA4, 0xA0, 0x01, 0x0D, 0x80, 0x23, 0xED, 0x15, 0x5C,
			0xD7, 0x5F, 0x10, 0xFF, 0x6F, 0x6B, 0xEA, 0x87, 0xB6, 0x9D, 0x1A,
			0xE4, 0xB3, 0x9B, 0x9D, 0x1F, 0x42, 0x56, 0xFA, 0xDC, 0x59, 0x06,
			0x10, 0x4B, 0x8A, 0x56, 0x6C, 0x71, 0x67, 0x45, 0xB6, 0x9F, 0xCC,
			0xCD, 0x0F, 0x71, 0x41, 0xED, 0x7B, 0xA4, 0x5A, 0xD1, 0xD1, 0x36,
			0xD2, 0xAE, 0x1B, 0xB2, 0xD4, 0xAB, 0x8E, 0x97, 0x86, 0x46, 0x40,
			0xC0, 0xCA, 0x1C, 0x97, 0x0F, 0x8F, 0x84, 0x07, 0xC7, 0xF0, 0x68,
			0xD3, 0x5D, 0x8B, 0x41, 0x17, 0x9F, 0x8A, 0x3B, 0xE7, 0x27, 0x9E,
			0xB7, 0xE9, 0x0C, 0xEF, 0xD2, 0x45, 0x99, 0x32, 0x82, 0xC9, 0xD1,
			0xCC, 0x80, 0x47, 0x66, 0x4F, 0x38, 0x15, 0xB5, 0x7E, 0xEE, 0x8A,
			0xCE, 0x43, 0x14, 0x87, 0xD8, 0xD7, 0x54, 0x92, 0xF7, 0xD4, 0x6C,
			0xBA, 0xA9, 0x45, 0x24, 0x75, 0xBF, 0x8A, 0x05, 0xAD, 0xC5, 0xA8,
			0x50, 0x69, 0x69, 0x8C, 0x03, 0x0A, 0xD0, 0x5E, 0x81, 0xD7, 0x13,
			0x57, 0xC2, 0x60, 0xFA, 0x5E, 0x95, 0xFA, 0x01, 0x66, 0xEF, 0x7B, };
	static unsigned char dh4096_g[] = { 0x02, };
	DH *dh;

	if ((dh = DH_new()) == NULL)
		return (NULL);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	dh->p = BN_bin2bn(dh4096_p, sizeof(dh4096_p), NULL);
	dh->g = BN_bin2bn(dh4096_g, sizeof(dh4096_g), NULL);
	if ((dh->p == NULL) || (dh->g == NULL)) {
		DH_free(dh);
		return (NULL);
	}
#else
	 DH_set0_pqg(dh, BN_bin2bn(dh4096_p, sizeof dh4096_p, NULL), NULL, BN_bin2bn(dh4096_g, sizeof dh4096_g, NULL));
#endif
	   return (dh);
}


/*
 * Set the default DH params for 8192 bit
 */
static DH * get_dh8192(void) {
	static unsigned char dh8192_p[] = { 0xCC, 0xCC, 0xB9, 0xAA, 0xC0, 0x74,
			0xB8, 0x19, 0x0B, 0x77, 0xDB, 0x77, 0x56, 0xB8, 0x06, 0xD5, 0xA2,
			0x03, 0x31, 0xA2, 0x13, 0x60, 0xB3, 0x81, 0xC1, 0x30, 0xC2, 0x06,
			0x91, 0xF1, 0x95, 0x50, 0x72, 0x41, 0xC0, 0x39, 0x4F, 0xFE, 0x12,
			0x95, 0x77, 0x35, 0x4A, 0xFA, 0x2E, 0xE3, 0xA9, 0x51, 0x57, 0x59,
			0x43, 0x3A, 0x38, 0x37, 0x2A, 0x4B, 0xA7, 0xC0, 0x61, 0x84, 0x87,
			0xDC, 0x61, 0x86, 0x7E, 0xDA, 0xFE, 0x7E, 0x24, 0x03, 0x3D, 0xB9,
			0x7A, 0xCB, 0xF8, 0x3D, 0x46, 0xE0, 0xF8, 0x1F, 0x41, 0xD3, 0xDC,
			0xBF, 0xD6, 0xBB, 0x6E, 0xDD, 0xAF, 0xC3, 0x4A, 0x0E, 0xDB, 0xD6,
			0x98, 0xF2, 0x05, 0x2A, 0x38, 0x48, 0x38, 0x41, 0x9F, 0x2B, 0x3B,
			0xD9, 0x04, 0x41, 0xD1, 0x77, 0x22, 0x2F, 0x02, 0xA4, 0x9B, 0xD7,
			0xDD, 0x27, 0xE6, 0x7A, 0xC4, 0x9B, 0x66, 0x25, 0x35, 0x09, 0x41,
			0x0D, 0x50, 0xCB, 0x19, 0x9D, 0x0C, 0xE6, 0x20, 0x67, 0x50, 0xB3,
			0xDF, 0xEE, 0xC4, 0xC0, 0xE4, 0x0A, 0x95, 0x37, 0x24, 0x27, 0xDF,
			0x45, 0x2F, 0xD5, 0x89, 0xAF, 0x70, 0x35, 0x5D, 0x5B, 0xA0, 0xFC,
			0xA7, 0xFD, 0x32, 0x8C, 0xAA, 0x8A, 0xF9, 0x9A, 0x5A, 0x0D, 0x8A,
			0x50, 0xB0, 0xF0, 0x50, 0xA8, 0xD4, 0x65, 0x8F, 0x48, 0x5B, 0xF7,
			0x53, 0xE8, 0x59, 0x4C, 0xCE, 0xC3, 0x86, 0x7B, 0x0F, 0x59, 0x32,
			0xBD, 0x33, 0x45, 0x40, 0xBF, 0x79, 0x0F, 0xB5, 0xDD, 0x97, 0x37,
			0x38, 0x14, 0xA6, 0x1B, 0x86, 0x56, 0x63, 0xED, 0x4D, 0xAC, 0xAD,
			0x2D, 0xBB, 0x8C, 0xCE, 0x09, 0x24, 0x14, 0x02, 0x86, 0x3B, 0x17,
			0x4A, 0x8E, 0xE2, 0x26, 0x6A, 0xAB, 0x47, 0x6B, 0x2E, 0xC6, 0xA9,
			0xE1, 0xCC, 0x25, 0xBE, 0x1E, 0x60, 0xD8, 0x94, 0x86, 0x7C, 0xA0,
			0xEB, 0x48, 0x57, 0x56, 0x34, 0x08, 0xA1, 0xAB, 0x0B, 0x29, 0x68,
			0xF3, 0xEF, 0xDF, 0x42, 0xD1, 0x3C, 0x21, 0xAE, 0x93, 0x55, 0xB1,
			0x09, 0x9C, 0x87, 0x59, 0x13, 0xD3, 0x83, 0x0C, 0x4E, 0x2B, 0x2B,
			0x44, 0xAB, 0xD3, 0x11, 0x62, 0xFE, 0xA8, 0xDA, 0x7C, 0xE2, 0x0C,
			0x95, 0xE3, 0xA2, 0x7F, 0x8A, 0x74, 0x85, 0xA5, 0xBF, 0x0D, 0x91,
			0xE4, 0x94, 0xCE, 0xF8, 0xB9, 0xF4, 0x02, 0xDC, 0x1F, 0xC7, 0xE5,
			0x61, 0x9D, 0xE1, 0x3A, 0x83, 0x03, 0x23, 0xC9, 0xA9, 0xB4, 0x53,
			0x4E, 0x08, 0x56, 0xB5, 0x97, 0x00, 0x13, 0x52, 0x86, 0x9E, 0xCB,
			0xCF, 0xEC, 0xF5, 0xA6, 0x68, 0x43, 0x0C, 0x08, 0xB9, 0xAE, 0x5F,
			0xE8, 0xF8, 0x15, 0x72, 0x5B, 0x65, 0x20, 0x57, 0xCB, 0x08, 0x01,
			0xDF, 0x0F, 0xB1, 0x17, 0xC4, 0xE7, 0xA3, 0xD7, 0x02, 0xAD, 0xA8,
			0xC6, 0x70, 0x0D, 0x20, 0x6C, 0x6C, 0xEC, 0xC6, 0xF2, 0x27, 0xC0,
			0x9A, 0x87, 0x00, 0xDB, 0x36, 0x08, 0x9D, 0x60, 0x75, 0xBD, 0x11,
			0xC9, 0x8B, 0x9A, 0xFC, 0x80, 0x55, 0x01, 0x27, 0x09, 0x1D, 0x08,
			0x17, 0xB1, 0xDE, 0xC9, 0x63, 0xCC, 0x75, 0xC8, 0xD0, 0xE1, 0x04,
			0x7D, 0x7B, 0x60, 0xCE, 0x13, 0x11, 0x14, 0x35, 0x16, 0xD8, 0x70,
			0xA1, 0x8F, 0xC4, 0x0E, 0xF3, 0xDB, 0x40, 0x6F, 0x4A, 0xA3, 0x50,
			0xB4, 0xCA, 0x99, 0x12, 0x39, 0xFC, 0x5D, 0x37, 0xC3, 0x13, 0x06,
			0x5F, 0xD3, 0x8C, 0xA9, 0x29, 0x20, 0x83, 0x52, 0xAD, 0x12, 0x81,
			0xAC, 0x75, 0x16, 0xB5, 0xC8, 0x46, 0xC0, 0xAF, 0xD2, 0x85, 0xE9,
			0x2F, 0xC3, 0x63, 0x8D, 0x6A, 0xBD, 0x34, 0xF9, 0x8B, 0x3C, 0x70,
			0x29, 0xD7, 0x5B, 0x28, 0x0F, 0x8A, 0x4B, 0x39, 0x44, 0x35, 0xBD,
			0xA1, 0x95, 0x18, 0xD9, 0xE4, 0xC3, 0xE4, 0xF6, 0x8E, 0xCE, 0xE9,
			0xFB, 0xF0, 0x5C, 0x66, 0x45, 0x3B, 0x78, 0xD4, 0x99, 0x7D, 0xFC,
			0x9C, 0x1B, 0x47, 0xB9, 0x10, 0x26, 0x46, 0xC1, 0x68, 0xEB, 0x5D,
			0x6E, 0xC0, 0xB6, 0x59, 0x88, 0x34, 0xB5, 0xDB, 0x50, 0xDD, 0x70,
			0xB6, 0x75, 0x33, 0xEC, 0x9E, 0x9F, 0x4D, 0x7B, 0xC7, 0x6F, 0x8C,
			0x0D, 0xEC, 0x91, 0xD5, 0x10, 0x87, 0xE6, 0xFD, 0x08, 0x49, 0x53,
			0x6F, 0x2F, 0x09, 0x77, 0x67, 0x4F, 0xB9, 0x41, 0xE3, 0x33, 0x18,
			0xC8, 0x1D, 0x16, 0x89, 0x85, 0xBD, 0xFE, 0x4D, 0x9C, 0xA2, 0x17,
			0x25, 0xBB, 0xE4, 0xD0, 0x40, 0x92, 0x47, 0x6E, 0x46, 0xE6, 0xF7,
			0x3F, 0x5D, 0xD3, 0xC8, 0x45, 0x03, 0xC8, 0xB2, 0xE2, 0x80, 0x64,
			0xE1, 0xDE, 0x22, 0x5E, 0xBB, 0x58, 0xDD, 0x8B, 0x6C, 0x42, 0x02,
			0x51, 0x67, 0x80, 0x3B, 0xED, 0x74, 0xEA, 0xD4, 0x56, 0x4E, 0x47,
			0x6C, 0x18, 0x6E, 0xB6, 0x1D, 0x6F, 0x03, 0x13, 0x50, 0x4E, 0xAD,
			0x02, 0xDF, 0x5A, 0x41, 0x9E, 0x88, 0x24, 0x2B, 0xE8, 0x8A, 0x65,
			0x49, 0x40, 0x36, 0x56, 0xEC, 0x9E, 0xB9, 0x9C, 0x2F, 0x54, 0x9D,
			0xB5, 0xCC, 0x06, 0xFF, 0x77, 0x51, 0xE0, 0x21, 0x21, 0x97, 0xA9,
			0x37, 0xAC, 0xDA, 0x2D, 0x02, 0x44, 0xDE, 0x73, 0x51, 0xA7, 0x6D,
			0x60, 0x7C, 0x07, 0xFD, 0xF0, 0xE8, 0x9B, 0x11, 0x19, 0xA5, 0x8D,
			0x2D, 0x51, 0xBD, 0x49, 0x8B, 0xA1, 0x86, 0x00, 0xCC, 0xF6, 0x38,
			0xE8, 0x1B, 0x9F, 0xEE, 0x49, 0x9A, 0x6E, 0x78, 0x4E, 0x37, 0xDD,
			0x1A, 0x20, 0x4B, 0xC5, 0x70, 0xDC, 0x64, 0xD8, 0x96, 0xA4, 0x46,
			0x09, 0x99, 0x28, 0xAE, 0x57, 0x60, 0x9B, 0x0F, 0xD5, 0x41, 0xA2,
			0x46, 0xBC, 0xDF, 0x45, 0x06, 0x3B, 0x27, 0x5A, 0x83, 0xCC, 0x90,
			0xB2, 0x76, 0x0D, 0x7D, 0xB1, 0x9C, 0x82, 0x1A, 0xFB, 0xDA, 0xF4,
			0xD1, 0xEE, 0x35, 0x3E, 0x93, 0xF4, 0x18, 0x3C, 0xB1, 0xC7, 0x77,
			0xC4, 0xCC, 0x4A, 0x55, 0x01, 0x94, 0x2A, 0x3C, 0x6B, 0x29, 0x7C,
			0x7C, 0x1C, 0x53, 0x91, 0x10, 0x7A, 0x00, 0xBD, 0x8E, 0x09, 0xA2,
			0x68, 0x0B, 0x12, 0x42, 0x63, 0x1D, 0x6D, 0x87, 0x61, 0xDB, 0x2D,
			0x48, 0xD6, 0x75, 0xD7, 0xE2, 0x87, 0xBD, 0x00, 0xAA, 0xCB, 0xED,
			0x34, 0x1F, 0x91, 0xF4, 0x6A, 0xF9, 0x16, 0x23, 0x38, 0xCE, 0x9F,
			0x5D, 0xB8, 0x9B, 0x07, 0xB3, 0x92, 0x33, 0x02, 0xCD, 0x50, 0xED,
			0xCC, 0xE3, 0x2D, 0x08, 0xE5, 0xD1, 0x81, 0xAC, 0x1D, 0x46, 0x40,
			0x60, 0xB4, 0x90, 0x13, 0x39, 0xF6, 0x54, 0x11, 0x64, 0xE8, 0x60,
			0x4E, 0xC7, 0x04, 0x5F, 0xB5, 0x50, 0x99, 0x8E, 0x8A, 0x48, 0xC5,
			0xDF, 0xC4, 0x87, 0xE2, 0xBD, 0xC5, 0xA1, 0xF0, 0xB9, 0x22, 0x6B,
			0x4D, 0x65, 0x09, 0x9E, 0x7A, 0x5B, 0xD4, 0x82, 0xF8, 0xA1, 0xAC,
			0x37, 0xFB, 0xA8, 0xF5, 0x10, 0xC9, 0xBF, 0x1B, 0x64, 0x0E, 0x2A,
			0xE9, 0x03, 0xB8, 0x90, 0xF4, 0x92, 0x17, 0x91, 0xFB, 0xA7, 0xE9,
			0xA6, 0xB0, 0x33, 0xDC, 0x83, 0xEB, 0xCD, 0x29, 0x0C, 0xCA, 0xA2,
			0x16, 0x94, 0x53, 0x89, 0x4C, 0x03, 0x7E, 0xB8, 0x6B, 0xC5, 0x31,
			0x2B, 0x7D, 0x5F, 0xD5, 0x8C, 0x2A, 0x73, 0x63, 0x32, 0x41, 0x6A,
			0x58, 0x85, 0x8B, 0x5B, 0x48, 0x88, 0x3C, 0xC7, 0x15, 0x58, 0xF1,
			0x59, 0x11, 0xC0, 0xA2, 0x10, 0xE2, 0x6E, 0xBF, 0x45, 0x95, 0xC9,
			0x2F, 0x1E, 0x2B, 0x76, 0xE7, 0x87, 0x1A, 0xDA, 0x91, 0x66, 0x90,
			0xC5, 0x2C, 0x50, 0x40, 0xFC, 0xF3, 0x25, 0xDE, 0xB8, 0x1A, 0x47,
			0xF4, 0x4F, 0x92, 0xB8, 0x14, 0x30, 0xCF, 0xB2, 0xD5, 0x4F, 0x65,
			0xCE, 0xE5, 0x33, 0xA6, 0x71, 0x6C, 0xE0, 0x3B, 0x94, 0x14, 0x5E,
			0xB6, 0x5B, 0x2C, 0xDD, 0x1A, 0xB2, 0x92, 0x7D, 0xAA, 0x97, 0xD3,
			0xAF, 0x8C, 0x16, 0x80, 0xA2, 0xD3, };
	static unsigned char dh8192_g[] = { 0x02, };
	DH *dh;

	if ((dh = DH_new()) == NULL)
		return (NULL);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	dh->p = BN_bin2bn(dh8192_p, sizeof(dh8192_p), NULL);
	dh->g = BN_bin2bn(dh8192_g, sizeof(dh8192_g), NULL);
	if ((dh->p == NULL) || (dh->g == NULL)) {
		DH_free(dh);
		return (NULL);
	}
#else
	 DH_set0_pqg(dh, BN_bin2bn(dh8192_p, sizeof dh8192_p, NULL), NULL, BN_bin2bn(dh8192_g, sizeof dh8192_g, NULL));
#endif
	return (dh);
}


/*
 *  openSSL DH callback
 *
 *  According to https://www.openssl.org/docs/ssl/SSL_CTX_set_tmp_dh_callback.html
 *  "Modern servers that do not support export ciphersuites are advised to either
 *  use SSL_CTX_set_tmp_dh() in combination with SSL_OP_SINGLE_DH_USE, or alternatively,
 *  use the callback but ignore keylength and is_export and simply supply at least 2048-bit
 *  parameters in the callback."
 */
static DH * tmpDHCallback(SSL *s, int is_export, int keylength) {
   if (LIKELY(keylength <= 2048))
       return dh2048;
   if (keylength <= 4096)
       return dh4096;
   return dh8192;
}


/*
 * Read DH params from the trust store
 */
static DH * readDHParams(int size) {
    FILE *paramfile = NULL;
    DH * result = NULL;
    char * fileName = alloca(g_keystore_len + 32);
    sprintf(fileName,"%s/dh_param_%d.pem",g_keystore, size);
    paramfile = fopen(fileName, "r");
    if (paramfile) {
        result = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
        fclose(paramfile);
    }
    return result;
}

/*
 * Set to disable CRL.
 * This allows the DisableCRL to be set dynamically
 */
static int g_disableCRL = 0;
void ism_common_setDisableCRL(int disable) {
    g_disableCRL = disable;
}
int ism_common_getDisableCRL(void) {
    return g_disableCRL;
}    


static int (* getDisableCRL)(ima_transport_info_t *);
void ism_common_setDisableCRLCallback(int (* callback)(ima_transport_info_t *)) {
    getDisableCRL = callback;
}

/*
 * Initialize SSL/TLS processing
 */
XAPI void ism_ssl_init(int useFips, int useBufferPool) {
    char * pskFileName;

    pthread_mutex_lock(&sslMutex);
    if (isSSLInited==1) {
        pthread_mutex_unlock(&sslMutex);
        return;
    }
    isSSLInited=1;
    if (useFips) {
#ifdef OPENSSL_FIPS
        if (!FIPS_mode_set(1)) {
            char xbuf[8192];
            concat_alloc_t buf = {0};
            buf.buf = xbuf;
            buf.len = sizeof xbuf;
            ERR_load_crypto_strings();
            sslGatherErr(&buf);
            TRACE(1, "Unable to establish FIPS mode: cause=%s\n", buf.buf);
            LOG(CRIT, Server, 910, "%s", "Unable to establish FIPS mode: Error={0}.", buf.buf);
            FIPS_mode_set(0);       /* Disable FIPS mode */
        } else {
            LOG(NOTICE, Server, 909, NULL, "Running in FIPS mode.");
            TRACE(1, "Running in FIPS mode\n");
            useBufferPool = 0;
        }
#else
        TRACE(1, "FIPS mode is not available\n");
#endif
    }
    if (ism_common_getBooleanConfig("UseSpinLocks", 0))
        sslUseSpinlocks = 1;
    else
        sslUseSpinlocks = 0;

    useBufferPool = ism_common_getBooleanConfig("TlsUseBufferPool", useBufferPool);
    if (useBufferPool) {
        initializeBufferPools(useBufferPool);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        CRYPTO_set_mem_ex_functions(ssl_malloc, ssl_realloc, ssl_free);
#else
        CRYPTO_set_mem_functions(ssl_malloc, ssl_realloc, ssl_free);
#endif
    } else {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        CRYPTO_set_mem_ex_functions(my_malloc, my_realloc, my_free);
#else
        CRYPTO_set_mem_functions(my_malloc, my_realloc, my_free);
#endif
    }
    SSL_load_error_strings();
    SSL_library_init();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();
    sslLockInit();
    pthread_mutex_unlock(&sslMutex);

    g_keystore = ism_common_getStringConfig("KeyStore");
    if (!g_keystore)
        g_keystore = ".";
    g_keystore_len = (int)strlen(g_keystore);
    TRACE(7, "KeyStore = %s\n", g_keystore);

    g_truststore = ism_common_getStringConfig("TrustStore");
    if (!g_truststore)
        g_truststore = ism_common_getStringConfig("TrustedCertificateDir");
    if (!g_truststore)
        g_truststore = ".";
    TRACE(7, "TrustStore = %s\n", g_truststore);

    /*
     * Initialize default DH parameters
     */
    dh2048 = readDHParams(2048);
    if (!dh2048)
        dh2048 = get_dh2048();
    dh4096 = readDHParams(4096);
    if (!dh4096)
        dh4096 = get_dh4096();
    dh8192 = readDHParams(8192);
    if (!dh8192)
        dh8192 = get_dh8192();

    /*
     * Default ECDH curve
     */
    ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

    /*
     * Load the PSK file
     */
    g_pskdir = ism_common_getStringConfig("PreSharedKeyDir");
    if (g_pskdir) {
        g_pskdir_len = (int)strlen(g_pskdir);
        TRACE(7, "PreSharedKeyDir = %s\n", g_pskdir);

        pskFileName = alloca(g_pskdir_len + 32);
        sprintf(pskFileName,"%s/psk.csv",g_pskdir);
        ism_ssl_applyPSKfile(pskFileName, 0);
    }

    /* Set the disable CRL */
    ism_common_setDisableCRL(ism_common_getIntConfig("DisableCRL", g_disableCRL));
}


/*
 * Terminate SSL/TLS processing
 */
void ism_ssl_cleanup(void) {
	ismHashMap * map;
	pthread_rwlock_wrlock(&pskMapLock);
	map = pskMap;
	pskMap = NULL;
	pthread_rwlock_unlock(&pskMapLock);
	if (map)
		freePSKMap(map);
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    sslLockCleanup();
#endif
    destroyBufferPools();
}


/*
 * Log an SSL/TLS error
 */
static void logSSLError(const char *objname) {
    char xbuf[8192];
    concat_alloc_t buf = { 0 };
    buf.buf = xbuf;
    buf.len = sizeof xbuf;
    sslGatherErr(&buf);
    if (objname) {
        LOG(ERROR, Server, 911, "%-s%-s", "Unable to create an SSL/TLS context: Endpoint={0} Error={1}.",
                objname, buf.buf);
    } else {
        TRACE(1, "Unable to create an SSL/TLS context: cause=%s\n", buf.buf);
    }
    ism_common_freeAllocBuffer(&buf);
}


/*
 * Read a certificate revocation file
 * We read the file using openssl BIO support.
 * The file can be in either PEM or DER format
 */
static int processCRLFile(X509_STORE *store, const char * crlFileName) {
    BIO *bio = BIO_new(BIO_s_file());
    if (bio) {
        if (BIO_read_filename(bio, crlFileName)) {
            X509_CRL * crl = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);
            if (!crl) {
                xUNUSED int brc = BIO_reset(bio);
                crl = d2i_X509_CRL_bio(bio, NULL);
                if (!crl) {
                    TRACE(4, "The downloaded CRL file is neither PEM nor DER: crl=%s\n", crlFileName);
                }
            }
            if (crl) {
                X509_STORE_add_crl(store, crl);
                return 0;
            }
        }
    }
    return 1;
}


/*
 * Validate PEM certificate in server
 */
int ism_common_validate_PEM(const char * certFileName) {
	int rc = 0;
	X509 *cert = NULL;
	BIO *bio = BIO_new(BIO_s_file());
	if (bio) {
		if (BIO_read_filename(bio, certFileName)) {
			if (!PEM_read_bio_X509(bio, &cert, 0, NULL)) {
				TRACE(5, "%s is not a valid PEM certificate.\n", certFileName);
				rc = 1;
			} else {
				X509_free(cert);
			}
		} else {
			TRACE(3, "Unable to read certificate file %s\n", certFileName);
			rc = 1;
		}
		BIO_free(bio);
	} else {
		TRACE(3, "BIO_s_file_internal() or BIO_new() failed, cannot validate %s.\n", certFileName);
		rc = 1;
	}
	return rc;
}

/*
 * Update the certificate revocation list (CRL) in server
 */
int ism_common_ssl_update_crl(SSL_CTX * ctx, const char * objname, const char * profileName) {
    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    if (store) {
        int err;
        int pathLen = strlen(g_truststore) + strlen(profileName) + 16;
        char * crlPath = alloca(pathLen);
        DIR  * dir;
        sprintf(crlPath, "%s/%s_crl", g_truststore, profileName);
        dir = opendir(crlPath);
        err = errno;
        if (dir) {
        	int rc = ISMRC_OK;
        	struct stat sb;
        	const char *CRLName = "crl.pem"; /* there is only one consolidated crl file per profile */
            pathLen += strlen(CRLName) + 1;
            char *crlFileName = alloca(pathLen);
            snprintf(crlFileName, pathLen, "%s/%s", crlPath, CRLName);
            rc = stat(crlFileName, &sb);
            err = errno;
            if ((rc != ISMRC_OK) && (err != ENOENT)) {
            	TRACE(5,"stat() system call failed with errno: %d for CRL directory: %s", errno, crlFileName);
            } else {
#if OPENSSL_VERSION_NUMBER < 0x20100000L
            	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
#else
            	X509_VERIFY_PARAM *param = SSL_CTX_get0_param(ctx);
#endif
            	if (err == ENOENT) {
            		err = 0;
            		rc = 0;
              		X509_VERIFY_PARAM_clear_flags(param, X509_V_FLAG_CRL_CHECK);
                    TRACE(5, "CRL folder for security profile %s is empty. Disabling CRL check\n", profileName);
            	}
            	else if (S_ISREG(sb.st_mode)) {
                    if (processCRLFile(store,crlFileName)) {
                        logSSLError(objname);
                        return 1;
                    }
                    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
                    TRACE(5, "CRL file processed for security profile %s. Enabling CRL check\n", profileName);
            	}
#if OPENSSL_VERSION_NUMBER < 0x20100000L
            	SSL_CTX_set1_param(ctx, param);
            	X509_VERIFY_PARAM_free(param);
#endif
            }
            closedir(dir);
            return rc;
        }
        if (err == ENOENT) {
        	err = 0;
            TRACE(5, "CRL folder does not exist for security profile %s\n", profileName);
            return 0;
        } else {
        	TRACE(5, "Could not open CRL folder for security profile %s. errno=%u error=%s\n", profileName, errno, strerror(errno));
        	return 1;
        }
    }
    logSSLError(objname);
    return 1;
}

#ifdef ALLOW_ANY_CERT
int allow_any(X509_STORE_CTX * ctx, void * xx) {
    return 1;
}
#endif

#define V13_FAST_SUITES "TLS_AES_128_GCM_SHA256:"\
                        "TLS_AES_256_GCM_SHA384:"\
                        "TLS_CHACHA20_POLY1305_SHA256:"\
                        "TLS_AES_128_CCM_SHA256:"\
                        "TLS_AES_128_CCM_8_SHA256"
/*
 * Set cipher list including TLSv1.3 ciphersuites
 * If the first AES cipher is not 256 bit (best) change the cipher suites order to put AES128 first.
 */
static int setCtxCiphers(SSL_CTX * ctx, const char * ciphers) {
    int ret = 0;
    SSL_CTX_set_cipher_list(ctx, ciphers);
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    const char * aes;
    	aes = strstr(ciphers, "AES");
	if (aes) {
		aes += 3;
		if (*aes == '_')    /* Allow an underscore between AES and the bit size */
		   aes++;
		if (*aes != '2') {
			SSL_CTX_set_ciphersuites(ctx, V13_FAST_SUITES);
			ret = SSL_OP_PRIORITIZE_CHACHA;
		}
	}

#endif
    return ret;
}

/*
 * Add certs from memory to the TLS context
 */
int  ism_common_addTrustCerts(SSL_CTX * ctx, const char * objname, const char * trustCerts) {
    BIO * bio;
    X509 * x;
    int    rc;

    bio = BIO_new_mem_buf((void *)(trustCerts), -1);
    for (;;) {
        x = PEM_read_bio_X509_AUX(bio, NULL, NULL, "");
        if (x == NULL) {
            if (ERR_GET_REASON(ERR_peek_last_error()) == PEM_R_NO_START_LINE) {
                ERR_clear_error();
                return 0;
            } else {
                logSSLError(objname);
                return ISMRC_CertificateNotValid;
            }
        }
        rc = X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), x);
        if (!rc) {
            logSSLError(objname);
            X509_free(x);
            return ISMRC_CertificateNotValid;
        }
        X509_free(x);
    }
}


/*
 * Create SSL/TLS context for the server with a key password
 * If serverName is set : CAFile = <truststore>/<serverName>_cafile.pem CAPath = <truststore>/<serverName>_capath
 * If serverName null and isServer : CAFile = <truststore>/<profileName>_cafile.pem CAPath = <truststore>/<profileName>_capath
 * If serverName null and !isServer : CAFile = <truststore>/<profileName>_cafile.pem CAPath = <truststore>/client
 */
SSL_CTX * ism_common_create_ssl_ctxpw(const char * objname, const char * methodName, const char * ciphers,
        const char * certFile, const char * keyFile, int isServer, int sslopt, int useClientCert,
        const char * profileName, ism_verifySSLCert verifyCallback, const char * serverName, pem_password_cb getkeypw, void * userdata) {

    char * keyName = NULL;
    char * certName = NULL;
    bool inMemory = false;
    const char * fileName = serverName == NULL ? profileName : serverName;

    SSL_CTX * ctx;
    const SSL_METHOD *method = ((isServer) ? SSLv23_server_method() : SSLv23_client_method());
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    int options = sslopt | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_SINGLE_DH_USE | SSL_OP_NO_COMPRESSION;
#else
    int options = (sslopt & 0x00FF0000) | SSL_OP_ALL | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
#endif


    if (ciphers == NULL)   /* Old list but this should not ever happen */
        ciphers = "AES128-GCM-SHA256:AES128:AESGCM:HIGH:!MD5:!SRP:!aNULL";

    /*
     * Select an SSL/TLS level, but only from those supported by this version of openssl.
     */
    if (methodName) {
#ifdef SSL_OP_NO_TLSv1_3
        if (strcmp(methodName, "TLSv1.3") == 0)
            options |= (SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2);
        else
#endif
        if (strcmp(methodName, SSL_TXT_TLSV1_2) == 0)
            options |= (SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
        else
        if (strcmp(methodName, SSL_TXT_TLSV1_1) == 0)
            options |= (SSL_OP_NO_TLSv1);
    }

    TRACE(5, "Create TLS context for: endpoint=%s profileName=%s methodName=%s"
             " isServer=%d sslopt=%08x useClientCert=%d"
             " certFile=%s keyFile=%s\n",
            objname, profileName, methodName,
            isServer, options, useClientCert,
            ((certFile && *certFile == '&') ? "memory" : certFile),
            ((keyFile && *keyFile == '&') ? "memory" : keyFile));
    TRACE(7, "TLS ciphers: %s\n", ciphers);

    /*
     * Create the SSL/TLS context
     */
    ctx = SSL_CTX_new(method);
    if (ctx) {
        SSL_CTX_set_options(ctx, options);
        options |= setCtxCiphers(ctx, ciphers);
        SSL_CTX_set_tmp_dh_callback(ctx, tmpDHCallback);
#ifdef ALLOW_ANY_CERT
        SSL_CTX_set_cert_verify_callback(ctx, allow_any, NULL);
#endif
        if (ecdh) {
            SSL_CTX_set_tmp_ecdh(ctx, ecdh);
        }
        if (certFile && keyFile) {

            /* Use certificate and key from memory */
            if (*certFile == '&') {
                inMemory = true;

                BIO * certdata = BIO_new_mem_buf((void *)(certFile+1), -1);
                X509 * cert  = PEM_read_bio_X509(certdata, NULL, 0, NULL);
                if (!cert) {
                    TRACE(3, "Certificate could not be read\n");
                    logSSLError(objname);
                    SSL_CTX_free(ctx);
                    BIO_free(certdata);
                    return NULL;
                }
                BIO_free(certdata);

                BIO * keydata = BIO_new_mem_buf((void *)(keyFile+1), -1);
                EVP_PKEY * key = PEM_read_bio_PrivateKey(keydata, NULL, getkeypw, userdata);
                if (!cert || !key) {
                    TRACE(3, "Private key could not be read\n");
                    logSSLError(objname);
                    SSL_CTX_free(ctx);
                    BIO_free(keydata);
                    return NULL;
                }
                BIO_free(keydata);

                SSL_CTX_use_certificate(ctx, cert);
                SSL_CTX_use_PrivateKey(ctx, key);
                if (!SSL_CTX_check_private_key(ctx)) {
                    TRACE(3, "Private key not valid\n");
                    logSSLError(objname);
                    SSL_CTX_free(ctx);
                    return NULL;
                }

            }
            /* Read certificate and key from file */
            else {
                /* Prepend the keystore directory to the specified names */
                keyName = alloca(g_keystore_len + strlen(keyFile) + 2);
                strcpy(keyName, g_keystore);
                strcat(keyName, "/");
                strcat(keyName, keyFile);

                certName = alloca(g_keystore_len + strlen(certFile) + 2);
                strcpy(certName, g_keystore);
                strcat(certName, "/");
                strcat(certName, certFile);
                if (getkeypw) {
                    SSL_CTX_set_default_passwd_cb(ctx, getkeypw);
                    SSL_CTX_set_default_passwd_cb_userdata(ctx, userdata);
                }
                if (!SSL_CTX_use_certificate_chain_file(ctx, certName)
                        || !SSL_CTX_use_PrivateKey_file(ctx, keyName, SSL_FILETYPE_PEM)
                        || !SSL_CTX_check_private_key(ctx)) {
                    logSSLError(objname);
                    SSL_CTX_free(ctx);
                    return NULL;
                }
            }
        }

        SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        if (isServer) {
            if (pskMap)
            	ism_ssl_update_psk(ctx, 1);
            if ( useClientCert ) {
                int mode = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;

                if ( inMemory ) {
                    X509_STORE * store = SSL_CTX_get_cert_store(ctx);
                    BIO * certdata = BIO_new_mem_buf((void *)(certFile+1), -1);
                    X509 * cert  = PEM_read_bio_X509(certdata, NULL, 0, NULL);
                    X509_STORE_add_cert(store,cert);
                }
                else {
                    int len = strlen(g_truststore) + strlen(fileName) + 16;
                    char * CAFile = alloca(len);
                    char * CAPath = alloca(len);
                    sprintf(CAFile, "%s/%s_cafile.pem", g_truststore, fileName);
                    sprintf(CAPath, "%s/%s_capath", g_truststore, fileName);

                    // If there is no cafile then pass NULL into verify locations
                    // if CAPath doesn't exist verify locations doesn't care
                    FILE * f_cafile = fopen(CAFile, "rb");
                    if (!f_cafile ) {
                        CAFile = NULL;
                    }

                    if (SSL_CTX_load_verify_locations(ctx, CAFile, CAPath) == 0) {
                        logSSLError(objname);
                        SSL_CTX_free(ctx);
                        return NULL;
                    }
                    SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(CAFile));
                    if (errno == ENOENT)
                        errno = 0;
                    if (checkCRL) {
                        if (ism_common_ssl_update_crl(ctx, objname, fileName)) {
                            SSL_CTX_free(ctx);
                            return NULL;
                        }
                    }
                }
                if (useClientCert != 2)
                {
                    mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
                }
                else
                {
                    TRACE(3, "%s allows connections without certificates\n", profileName);
                }

                SSL_CTX_set_verify(ctx, mode, verifyCallback);
            } else {
                SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
            }
            if (ism_common_isServer())
                SSL_CTX_set_session_id_context(ctx, (uint8_t *)"\x01imaserver", 10);
            if (ism_common_isProxy())
                SSL_CTX_set_session_id_context(ctx, (uint8_t *)"\x02imaproxy", 9);
            if (ism_common_isBridge())
                SSL_CTX_set_session_id_context(ctx, (uint8_t *)"\x03imabridge", 10);
        } else {

#ifdef HAS_BRIDGE
            SSL_CTX_load_verify_locations(ctx, NULL, g_truststore);
#else
            char * CAFile;
            char * CAPath;

            // If serverName is set then use the same verify location as the server
            if (serverName != NULL) {
                int len = strlen(g_truststore) + strlen(serverName) + 16;
                CAFile = alloca(len);
                sprintf(CAFile, "%s/%s_cafile.pem", g_truststore, fileName);
                CAPath = alloca(len);
                sprintf(CAPath, "%s/%s_capath", g_truststore, fileName);
            } else {
                int len = strlen(g_truststore) + strlen(profileName) + 16;
                CAFile = alloca(len);
                sprintf(CAFile, "%s/%s_cafile.pem", g_truststore, fileName);
                CAPath = alloca(strlen(g_truststore)+16);
                sprintf(CAPath, "%s/client", g_truststore);
            }
            // If there is no cafile then pass NULL into verify locations
            // if CAPath doesn't exist verify locations doesn't care
            FILE * f_cafile = fopen(CAFile, "rb");
            if (!f_cafile ) {
                CAFile = NULL;
            }
            SSL_CTX_load_verify_locations(ctx, CAFile, CAPath);
#endif
            if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
                ism_common_traceSSLError("Failed to set the default verification paths for the context", __FILE__, __LINE__);
            }
        }
    } else {
        logSSLError(objname);
        TRACE(5, "Creation of TLS context failed for: endpoint=%s profileName=%s\n",
                objname, profileName);
    }
    ERR_clear_error();
    return ctx;
}

/*
 * Create SSL/TLS context for the server
 */
SSL_CTX * ism_common_create_ssl_ctx(const char * objname, const char * methodName, const char * ciphers,
        const char * certFile, const char * keyFile, int isServer, int sslopt, int useClientCert,
        const char * profileName, ism_verifySSLCert verifyCallback, const char * serverName ) {
    return ism_common_create_ssl_ctxpw(objname, methodName, ciphers, certFile, keyFile, isServer, sslopt, useClientCert,
            profileName, verifyCallback, serverName, NULL, NULL);
}


/*
 * Free the SSL context
 */
void ism_common_destroy_ssl_ctx(SSL_CTX *ctx) {
    SSL_CTX_free(ctx);
}

/*
 * Compute SHA1
 * Low level API call to digest SHA1 forbidden in FIPS mode!
 * So we are going to use high level digest APIs.
 */
XAPI uint32_t ism_ssl_SHA1(const void *key, size_t keyLen, uint8_t * mdBuf) {
    uint32_t mdLen = 0;
    const EVP_MD *md = EVP_sha1();

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX mdCtx;
    EVP_MD_CTX_init(&mdCtx);
    EVP_DigestInit_ex(&mdCtx, md, NULL);
    EVP_DigestUpdate(&mdCtx, key, keyLen);
    EVP_DigestFinal_ex(&mdCtx, mdBuf, &mdLen);
    EVP_MD_CTX_cleanup(&mdCtx);
#else
    EVP_MD_CTX * mdCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdCtx, md, NULL);
    EVP_DigestUpdate(mdCtx, key, keyLen);
    EVP_DigestFinal_ex(mdCtx, mdBuf, &mdLen);
    EVP_MD_CTX_free(mdCtx);
#endif
    return mdLen;
}


/*
 * Extract information from the peer certificate .
 * This is used to to trace any certificate problems
 *
 * The resulting PUF_MEM object just be freed using BUF_MEM_free()
 */
XAPI BUF_MEM * ism_ssl_getPeerCertInfo(SSL *ssl, int full, int isServer) {
	BIO *mem = BIO_new(BIO_s_mem());
	BUF_MEM *result;
	char buf[1024];
	do {
		STACK_OF(X509) *certChain;
		STACK_OF(X509_NAME) *caList;
		if (isServer) {
			X509 * peerCert = SSL_get_peer_certificate(ssl);
			if (peerCert) {
				BIO_printf(mem,"Client certificate\n");
				X509_NAME_oneline(X509_get_subject_name(peerCert),buf,sizeof(buf));
				BIO_printf(mem,"subject=%s\n",buf);
				X509_NAME_oneline(X509_get_issuer_name(peerCert),buf,sizeof(buf));
				BIO_printf(mem,"issuer=%s\n",buf);
				if (full)
					PEM_write_bio_X509(mem,peerCert);
				X509_free(peerCert);
			} else {
				BIO_printf(mem,"Peer certificate is not available\n");
				break;
			}
		}
		certChain=SSL_get_peer_cert_chain(ssl);
		if (certChain) {
			int i;
			int chainSize = sk_X509_num(certChain);
			BIO_printf(mem,"Peer certificate chain\n");
			for (i = 0; i <chainSize ; i++) {
				X509_NAME_oneline(X509_get_subject_name(sk_X509_value(certChain,i) ), buf, sizeof(buf));
				BIO_printf(mem, "subject_name:%s", buf);
				X509_NAME_oneline(X509_get_issuer_name(sk_X509_value(certChain,i) ), buf, sizeof(buf));
				BIO_printf(mem, "issuer_name:%s\n", buf);
				if (full)
					PEM_write_bio_X509(mem, sk_X509_value(certChain,i) );
			}

		} else {
			BIO_printf(mem,"Peer certificate chain is not available\n");
		}
		caList = SSL_get_client_CA_list(ssl);
		if (caList) {
			int i;
			int listSize = sk_X509_NAME_num(caList);
			BIO_printf(mem,"Client CA list\n");
			for (i = 0; i < listSize; i++) {
				X509_NAME *xn = sk_X509_NAME_value(caList,i);
				X509_NAME_oneline(xn, buf, sizeof(buf));
				BIO_write(mem, buf, strlen(buf));
				BIO_write(mem, "\n", 1);
			}
		} else {
			BIO_printf(mem,"Client CA list is not defined\n");
		}
	} while (0);
	BIO_get_mem_ptr(mem, &result);
	xUNUSED int x = BIO_set_close(mem, BIO_NOCLOSE);            /* So BIO_free() leaves BUF_MEM alone */
	BIO_free(mem);
	return result;
}


/*
 * Get the set of Subject Alternate Names
 * The names are returned as a null separated list in a buffer
 * @param ssl   The SSL object
 * @param buf   The output buffer
 * @return The count of names returned
 */
int ism_ssl_getSubjectAltNames(SSL * ssl, concat_alloc_t * buf) {
    STACK_OF(GENERAL_NAME) *san_names = NULL;
    X509 * peerCert;
    int    count = 0;

    peerCert = SSL_get_peer_certificate(ssl);
    if (peerCert) {
        int sn_count;
        int i;
        san_names = X509_get_ext_d2i(peerCert, NID_subject_alt_name, NULL, NULL);
        if (san_names) {
            sn_count = sk_GENERAL_NAME_num(san_names);
            for (i=0; i<sn_count; i++) {
                const GENERAL_NAME * name = sk_GENERAL_NAME_value(san_names, i);
                if (name->type == GEN_EMAIL) {
                    const char * san =  (const char *)ASN1_STRING_data(name->d.rfc822Name);
                    ism_common_allocBufferCopy(buf, san);
                    count++;
                }
            }
            sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
        }
        X509_free(peerCert);
    }
    return count;
}


/*
 * openssl callback to set PSK
 */
static uint32_t pskServerCallback(SSL * ssl, const char * identity, uint8_t * psk, uint32_t maxPSKLen) {
	uint32_t rc = 0;
	ima_transport_info_t * transport = SSL_get_app_data(ssl);
	if (transport) {
	    pthread_rwlock_rdlock(&pskMapLock);
	    char * ba = (char *)ism_common_getHashMapElement(pskMap, identity, 0);
	    pthread_rwlock_unlock(&pskMapLock);
	    if (LIKELY(ba != NULL)) {
	        rc = (uint8_t)ba[0] + 1;
	        if (LIKELY(rc <= maxPSKLen)) {
	            transport->usePSK = 1;
	            memcpy(psk, ba+1, rc);
	        }
	    }
	}
	return rc;
}


/*
 * Update an SSL context when the PSK file is changed.
 * @param ctx    A server SSL context
 * @param enable Zero if PSK handling should be disabled, non-zero if it should be enabled.
 */
XAPI int ism_ssl_update_psk(struct ssl_ctx_st * ctx, int enable) {
	if (enable) {
		SSL_CTX_set_psk_server_callback(ctx, pskServerCallback);
	} else {
		SSL_CTX_set_psk_server_callback(ctx, NULL);
	}
	return 0;
}

/*
 * Set a method to call when the PSK file is applied.
 *
 * Only one callback is allowed to be set, and this will replace any existing callback.
 * The purpose of this callback is so that we can notify all SSL contexts of the change.
 * The transport component knows about which SSL contexts are in use by endpoints, but
 * the utilities do not.  Normally it is only significant when the PSK file is empty
 * before or after the apply.
 *
 * @param psknotifier  The method to call when the PSK file is applied.
 */
XAPI void ism_common_setPSKnotify(ism_psk_notify_t psknotifer) {
	pskNotifer = psknotifer;
}


typedef struct stack_st_X509   sslCertChain;

/*
 * Revalidates the client certificate on server
 *
 * This is called on the server when the CRL is reloaded
 * @param ssl - SSL handle for the connection
 * @return - 0 if validation failed, 1 if succeeded
 */
XAPI int ism_ssl_revalidateCert(SSL * ssl) {
    int rc = 0;
    if (ssl) {
        SSL_CTX * ctx = SSL_get_SSL_CTX(ssl);
        X509 * cert = SSL_get_peer_certificate(ssl);
        sslCertChain * chain = SSL_get_peer_cert_chain(ssl);
        if (ctx && cert && chain) {
            X509_STORE * store = SSL_CTX_get_cert_store(ctx);
            if (store) {
                X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
                if (store_ctx && X509_STORE_CTX_init(store_ctx, store, cert, chain)) {
                    X509_STORE_CTX_set_depth(store_ctx, 100);
                    rc = X509_verify_cert(store_ctx);
                }
                if (store_ctx)
                    X509_STORE_CTX_free(store_ctx);
            }
        }
        if (cert) {
            X509_free(cert);
        }
    }
    return rc;
}


/*
 * Reverify the client certificate and return the verify return code (proxy)
 *
 * This is called on the server when the CRL is reloaded
 * @param ssl - SSL handle for the connection
 * @return The SSL verify return code
 */
XAPI int ism_ssl_verifyCert(SSL * ssl) {
    int rc = 0;
    if (ssl) {
        SSL_CTX * ctx = SSL_get_SSL_CTX(ssl);
        X509 * cert = SSL_get_peer_certificate(ssl);
        sslCertChain * chain = SSL_get_peer_cert_chain(ssl);
        if (ctx && cert && chain) {
            X509_STORE * store = SSL_CTX_get_cert_store(ctx);
            if (store) {
                X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
                if (store_ctx && X509_STORE_CTX_init(store_ctx, store, cert, chain)) {
                    X509_STORE_CTX_set_verify_cb(store_ctx, crlVerifyCB);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
                    store_ctx->param->flags = ctx->param->flags;
#else
                    X509_STORE_CTX_set_flags(store_ctx, X509_VERIFY_PARAM_get_flags(SSL_CTX_get0_param(ctx)));
#endif
                    X509_STORE_CTX_set_ex_data(store_ctx, SSL_get_ex_data_X509_STORE_CTX_idx(), ssl);
                    if (X509_verify_cert(store_ctx) == 1) {
                        rc = 0;
                    } else {
                        rc = X509_STORE_CTX_get_error(store_ctx);
                    }
                }
                if (store_ctx)
                    X509_STORE_CTX_free(store_ctx);
            }
        }
        if (cert) {
            X509_free(cert);
        }
    }
    return rc;
}

typedef DH *(*tmp_dh_callback)(SSL *ssl, int is_export, int keylength);


/*
 * openssl callback to free certificate
 */
static void freeCertCB(void *data) {
    X509 * cert = (X509*) data;
    if (cert)
        X509_free(cert);
}

/*
 * Free certificate on server
 */
static void freeTrustedCertCB(void *data) {
    sslTrustCertData_t * certData = (sslTrustCertData_t*) data;
    if (certData) {
        certData->useCount--;
        if (certData->useCount < 1) {
            if (certData->cert)
                X509_free(certData->cert);
            if (certData->pkey)
                EVP_PKEY_free(certData->pkey);
            ism_common_list_destroy(&certData->crlLocations);
            ism_common_free(ism_memory_utils_sslutils,certData);
        }
    }
}

/*
 * Local cert files are used for testing only
 */
static inline BIO * createReadBIO(const char * buf, int length, char ** extraBuf) {
    BIO * bio = NULL;
    *extraBuf = NULL;
    if (allowLocalCertFiles && (strncmp("file://", buf, 7) == 0)) { /*For internal testing only */
        int len = 0;
        int rc = ism_common_readFile(buf+7, extraBuf, &len);
        if (rc) {
            TRACE(4, "Failed to read certificate file \"%s\" error=%d\n", buf+7, rc);
            return NULL;
        }
        length = len;
        buf = *extraBuf;
    }
    bio = BIO_new_mem_buf((void *) buf, length);
    return bio;
}

/* Free entry of admin action list */
void ssl_free_data(void *data) {
    ism_common_free(ism_memory_utils_sslutils,data);
}


/*
 * Read certificates into the trust store for server
 */
static int readCerts(const char * certs, int length, ism_common_list * certList, int trusted) {
    BIO * certdata = NULL;
    char * buf = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),certs);
    char * extrabuf = NULL;
    if (trusted) {
        ism_common_list_init(certList, 0, freeTrustedCertCB);
    } else {
        ism_common_list_init(certList, 0, freeCertCB);
    }
    if (buf == NULL) {
        return ISMRC_AllocateError;
    }
    while ((length > 0) && (buf[length-1] == '\n')) {
        length--;
        buf[length] = '\0';
    }

    if (length < 1) {
        ism_common_free(ism_memory_utils_sslutils,buf);
        return ISMRC_CertificateNotValid;
    }

    certdata = createReadBIO(buf, length, &extrabuf);
    if (certdata) {
        unsigned long err = 0;
        int rc = ISMRC_OK;
        while (!BIO_eof(certdata)) {
            if (trusted) {
                sslTrustCertData_t * tcd = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,72),sizeof(sslTrustCertData_t));
                if (tcd) {
                    X509 * cert = PEM_read_bio_X509_AUX(certdata, NULL, NULL, NULL);
                    if (cert) {
                        EVP_PKEY * pkey = X509_get_pubkey(cert);
                        if (pkey) {
                            tcd->cert = cert;
                            tcd->pkey = pkey;
                            tcd->useCount = 1;
                            ism_common_list_init(&tcd->crlLocations, 0, (void*)ssl_free_data);
                            ism_common_list_insert_tail(certList, tcd);
                            continue;
                        } else {
                            traceSSLError("Failed to extract public key ");
                            X509_free(cert);
                            BIO_free(certdata);
                            ism_common_free(ism_memory_utils_sslutils,buf);
                            return ISMRC_CertificateNotValid;
                        }
                    } else {
                        err = ERR_peek_last_error();
                    }
                    ism_common_free(ism_memory_utils_sslutils,tcd);
                    break;
                } else {
                    BIO_free(certdata);
                    ism_common_free(ism_memory_utils_sslutils,buf);
                    return ISMRC_AllocateError;
                }
            } else {
                //DER: x=d2i_X509_bio(certdata,NULL);
                X509 * cert = PEM_read_bio_X509(certdata, NULL, NULL, NULL);
                if (cert) {
                    ism_common_list_insert_tail(certList, cert);
                    continue;
                }
                err = ERR_peek_last_error();
                break;
            }
        }
        if (err) {
            traceSSLError("Unable to parse certificate:");
            rc = ISMRC_CertificateNotValid;
        }
        BIO_free(certdata);
        ism_common_free(ism_memory_utils_sslutils,buf);
        if (extrabuf)
            ism_common_free(ism_memory_utils_sslutils,extrabuf);
        return rc;
    }
    ism_common_free(ism_memory_utils_sslutils,buf);
    if (extrabuf)
        ism_common_free(ism_memory_utils_sslutils,extrabuf);
    return ISMRC_AllocateError;
}


/*
 * Read keys into the trust store for server
 */
static int readKey(const char * buf, int length, EVP_PKEY ** pKey) {
    char * extrabuf = NULL;
    BIO * keydata = createReadBIO(buf, length, &extrabuf);
    if (keydata) {
        int rc = ISMRC_OK;
        EVP_PKEY * key = PEM_read_bio_PrivateKey(keydata, NULL, NULL, NULL);
        if (key == NULL) {
            traceSSLError("Unable to parse private key:");
            rc = ISMRC_CertificateNotValid;
        }
        *pKey = key;
        BIO_free(keydata);
        if (extrabuf)
            ism_common_free(ism_memory_utils_sslutils,extrabuf);
        return rc;
    }
    if (extrabuf)
        ism_common_free(ism_memory_utils_sslutils,extrabuf);
    return ISMRC_AllocateError;
}


#ifndef NO_CURL
/*
 * Read CRL into the strust store for server
 */
static X509_CRL * readCRL(const char * buf, int length) {
    char * extrabuf = NULL;
    X509_CRL * result = NULL;
    BIO * crldata = createReadBIO(buf, length, &extrabuf);
    if (crldata) {
        result = PEM_read_bio_X509_CRL(crldata, NULL, NULL, NULL);
        if (!result) {
            xUNUSED int brc = BIO_reset(crldata);
            result = d2i_X509_CRL_bio(crldata, NULL);
            if (!result)
                traceSSLError("Unable to parse CRL:");
        }
        BIO_free(crldata);
    } else {
        ism_common_setError(ISMRC_AllocateError);
    }
    if (extrabuf)
        ism_common_free(ism_memory_utils_sslutils,extrabuf);
    return result;
}
#endif

static void * crlUpdateProc(void * parm, void * context, int value);

/*
 * Initialize the SNI and create the default context
 *
 * The tlsXXXX properties are normally set from the first configured secure endpoint
 */
int ism_ssl_SNI_init(void) {
    int i;

    orgConfigMap = ism_common_createHashMap(32 * 1024, HASH_STRING);

    crlUpdateDelay = ism_common_getIntConfig("crlUpdateDelay", 3600);    /* default is check every hour */
    crlUpdateDelay *= 1000000000;

    /*
     * Start CRL update threads
     * TODO: why are we not just using non-blocking curl?
     */
    if (!g_disableCRL) {
        for (i = 0; i < NUM_OF_CU_THREADS; i++) {
            char threadName[16];
            sprintf(threadName, "crlUpd.%d", i);
            pthread_mutex_init(&cuThreads[i].lock, NULL);
            pthread_cond_init(&cuThreads[i].cond, NULL);
            ism_common_list_init(&cuThreads[i].updateReqsList, 0, NULL);
            ism_common_startThread(&cuThreads[i].thread, crlUpdateProc, NULL,
            NULL, i, ISM_TUSAGE_LOW, 0, threadName, "CRLUpdateThread");
        }
    }
    return ISMRC_OK;
}


/*
 * Terminate SNI processing
 */
void ism_ssl_SNI_tem(void) {
    SSL_CTX_free(defaultCTX);
    defaultCTX = NULL;
    ism_common_destroyHashMap(orgConfigMap);
}


/*
 * Create the default context for SNI
 */
static void createDefaultCTX(void) {
    pthread_mutex_lock(&sslMutex);
    if (!defaultCTX) {
        int sslOptions = 0x030203FF;
        const char * defaultCertName = ism_common_getStringConfig("tlsCertName");
        const char * defaultKeyName = ism_common_getStringConfig("tlsKeyName");
        const char * tlsMethod = ism_common_getStringConfig("tlsMethod");
        sslOptions = ism_common_getIntConfig("tlsOptions", sslOptions);
        allowLocalCertFiles = ism_common_getIntConfig("allowLocalCertFiles", 1);

        if (!defaultCertName) {
            defaultCertName = ism_common_getStringConfig("DefaultCertName");
            if (!defaultCertName)
                defaultCertName = "defaultCert.pem";
        }
        if (!defaultKeyName) {
            defaultKeyName = ism_common_getStringConfig("DefaultKeyName");
            if (!defaultKeyName)
                defaultKeyName = "defaultKey.pem";
        }
        if (!tlsMethod)
            tlsMethod = "TLSv1.2";
        g_tlsCiphers = ism_common_getStringConfig("tlsCiphers");
        if (!g_tlsCiphers)
            g_tlsCiphers = "AES128-GCM-SHA256:AES128:AESGCM:AES:!SRP:!ADH:!AECDH:!EXP:!RC4";

        /*
         * Create the default context
         */
        defaultCTX = ism_common_create_ssl_ctx("defaultCTX", tlsMethod, g_tlsCiphers, defaultCertName, defaultKeyName, 0,
                sslOptions, 0, "defaultProfile", NULL, NULL);
        if (!defaultCTX) {
            /* Enabling org SNI will fail unless a server certificate is specified for an organization that uses client certificate authentication */
            TRACE(4, "No default TLS context was created for SNI: defaultCertName=%s defaultKeyName=%s\n", defaultCertName, defaultKeyName);
        }
    }
    pthread_mutex_unlock(&sslMutex);
}

/*
 * Create a default context in WIoTP proxy
 * This is used as the base for SNI contexts
 */
static SSL_CTX * createCTXFromDefault(int requireClientCert, const char * orgName, ism_verifySSLCert verifyCallback) {
    SSL_CTX * ctx;

    if (!defaultCTX) {
        createDefaultCTX();
        if (!defaultCTX) {
            return NULL;
        }
    }

    ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx) {
        X509 * cert = NULL;
        int options = 0;
        EVP_PKEY * pkey = NULL;
        STACK_OF(X509) *extraCerts = NULL;
        options = SSL_CTX_get_options(defaultCTX);
        options |= setCtxCiphers(ctx, g_tlsCiphers);
        SSL_CTX_set_options(ctx, options);
        SSL_CTX_set_tmp_dh_callback(ctx, tmpDHCallback);
        if (ecdh) {
            SSL_CTX_set_tmp_ecdh(ctx, ecdh);
        }
        SSL_CTX_set_mode(ctx, SSL_CTX_get_mode(defaultCTX));
        SSL_CTX_set_session_id_context(ctx, (const unsigned char *)orgName, strlen(orgName));
        if (requireClientCert) {
            /* Verify using CRL.  If there is no CRL we allow in the verifyCallback */
            X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
            X509_VERIFY_PARAM_set_flags(param, (X509_V_FLAG_CRL_CHECK | X509_V_FLAG_USE_DELTAS));
            SSL_CTX_set1_param(ctx, param);
            X509_VERIFY_PARAM_free(param);
            /* Set verify */
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verifyCallback);
        }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        {
            SSL *ssl = SSL_new(defaultCTX);
            cert = SSL_get_certificate(ssl);
            pkey = SSL_get_privatekey(ssl);
            SSL_free(ssl);
        }
#else
        cert = SSL_CTX_get0_certificate(defaultCTX);
        pkey = SSL_CTX_get0_privatekey(defaultCTX);
#endif


        SSL_CTX_set_default_verify_paths(ctx);
        SSL_CTX_use_certificate(ctx, cert);
        SSL_CTX_use_PrivateKey(ctx, pkey);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        if (SSL_CTX_get_extra_chain_certs(defaultCTX, &extraCerts)) {
            if (extraCerts) {
                int num = sk_X509_num(extraCerts);
                int i;
                X509 ** certs = alloca(num * sizeof(X509*));
                for(i = 0; i < num; i++) {
                    certs[i] = sk_X509_value(extraCerts,i);
                }
                for( ; i > 0; i--) {
                    X509 * x = certs[i-1];
                    CRYPTO_add(&x->references,1,CRYPTO_LOCK_X509);
                    SSL_CTX_add_extra_chain_cert(ctx, x);
                }
            }
#else
        if (SSL_CTX_get_extra_chain_certs(defaultCTX, &extraCerts)) {
            if (extraCerts && (sk_X509_num(extraCerts) > 0)) {
                if (SSL_CTX_set1_chain(ctx, extraCerts) == 0) {
                    traceSSLError("Unable to set extra chain certs:");
                    SSL_CTX_free(ctx);
                    ctx = NULL;
                }
            }
#endif
        } else {
            traceSSLError("Unable to get extra chain certs:");
            SSL_CTX_free(ctx);
            ctx = NULL;
        }
    } else {
        ism_common_setError(ISMRC_AllocateError);
    }
    return ctx;
}


/*
 * Create an SNI context for WIoTP proxy
 */
static SSL_CTX * createCTX(const char * name, const char * certBuf, const char * keyBuf, int requireClientCert,
        ism_verifySSLCert verifyCallback) {
    int  rc;
    SSL_CTX * ctx = NULL;
    ism_common_list certList;
    EVP_PKEY * key = NULL;
    if (!certBuf || !keyBuf) {
        return createCTXFromDefault(requireClientCert, name, verifyCallback);
    }
    rc = readKey(keyBuf, strlen(keyBuf), &key);
    if (rc) {
        /* TRACE and log */
        return NULL;
    }
    rc = readCerts(certBuf, strlen(certBuf), &certList, 0);
    if (rc) {
        ism_common_list_destroy(&certList);
        EVP_PKEY_free(key);
        ism_common_setError(ISMRC_CertificateNotValid);
        /* trace and log */
        return NULL;
    }

    if (!g_tlsCiphers) {
        g_tlsCiphers = "AES128-GCM-SHA256:AES128:AESGCM:AES:!SRP:!ADH:!AECDH:!EXP:!RC4";
    }

    /*
     * Create a context with a custom server cert
     */
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx) {
        int firstCert = 1;
        int sslOptions = 0x030203FF;
        sslOptions = ism_common_getIntConfig("tlsOptions", sslOptions);
        SSL_CTX_set_options(ctx, sslOptions);
        setCtxCiphers(ctx, g_tlsCiphers);
        SSL_CTX_set_tmp_dh_callback(ctx, tmpDHCallback);
        if (ecdh) {
            SSL_CTX_set_tmp_ecdh(ctx, ecdh);
        }
        SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        SSL_CTX_set_session_id_context(ctx, (const unsigned char*) name, strlen(name));
        if (requireClientCert) {
            /* Verify using CRL.  If there is no CRL we allow in the verifyCallback */
            X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
            X509_VERIFY_PARAM_set_flags(param, (X509_V_FLAG_CRL_CHECK | X509_V_FLAG_USE_DELTAS));
            SSL_CTX_set1_param(ctx, param);
            X509_VERIFY_PARAM_free(param);
            /* Require client certs */
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE , verifyCallback);
        }
        do {
            X509 * cert = NULL;
            ism_common_list_remove_head(&certList, (void**) &cert);
            if (firstCert) {
                firstCert = 0;
                SSL_CTX_use_certificate(ctx, cert);
                X509_free(cert);
            } else {
                SSL_CTX_add_extra_chain_cert(ctx, cert);
            }
        } while (ism_common_list_getSize(&certList));
        SSL_CTX_use_PrivateKey(ctx, key);
        if (!SSL_CTX_check_private_key(ctx)) {
            TRACE(4, "The specified private key is not valid: org=%s\n", name);
            SSL_CTX_free(ctx);
            ctx = NULL;
        }
    }
    EVP_PKEY_free(key);
    ism_common_list_destroy(&certList);
    return ctx;
}


/*
 * Compare two X509 names
 */
static int xname_cmp(const X509_NAME * const * a, const X509_NAME * const * b) {
    return (X509_NAME_cmp(*a, *b));
}


/*
 * Parse the CRL distribution points in a certificate
 */
static int parseCrlLocations(X509 * cert, ism_common_list * crlLocations) {
    int rc = ISMRC_OK;
    if (g_disableCRL)
        return rc;
    STACK_OF(DIST_POINT) * crlDPs = X509_get_ext_d2i(cert, NID_crl_distribution_points, NULL, NULL);
    if (crlDPs) {
        DIST_POINT *dp;
        int i;
        for (i = 0; i < sk_DIST_POINT_num(crlDPs); i++) {
            dp = sk_DIST_POINT_value(crlDPs, i);
            if (dp->distpoint) {
                if (dp->distpoint->type == 0) {
                    /* Full name */
                    if (dp->distpoint->name.fullname) {
                        int j;
                        for (j = 0; j < sk_GENERAL_NAME_num(dp->distpoint->name.fullname); j++) {
                            GENERAL_NAME *gn = sk_GENERAL_NAME_value(dp->distpoint->name.fullname, j);
                            if (gn != NULL && gn->type == GEN_URI) {
                                const char * uri = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),(const char *) gn->d.ia5->data);
                                if (uri == NULL) {
                                    rc = ISMRC_AllocateError;
                                    break;
                                }
                                ism_common_list_insert_tail(crlLocations, uri);
                            }
                        }
                    }
                } else {
                    /* Relative to CRL issuer */
                    if (sk_X509_NAME_ENTRY_num(dp->distpoint->name.relativename) == 1) {
                        X509_NAME * baseName = NULL;
                        if (dp->CRLissuer) {
                            GENERAL_NAME *gn = sk_GENERAL_NAME_value(dp->CRLissuer, 0);
                            assert(gn->type == GEN_DIRNAME);
                            if (gn->type == GEN_DIRNAME)
                                baseName = X509_NAME_dup(gn->d.directoryName);
                        } else {
                            baseName = X509_NAME_dup(X509_get_issuer_name(cert));
                        }
                        if (baseName) {
                            X509_NAME_add_entry(baseName, sk_X509_NAME_ENTRY_value(dp->distpoint->name.relativename, 0),
                                    -1, 0);
                            const char * uri = X509_NAME_oneline(baseName, NULL, 0);
                            if (uri == NULL) {
                                rc = ISMRC_AllocateError;
                                break;
                            }
                            ism_common_list_insert_tail(crlLocations, uri);
                        } else {
                            rc = ISMRC_AllocateError;
                            break;
                        }
                    }
                }
            }
            if (rc)
                break;
        }
        sk_DIST_POINT_pop_free(crlDPs, DIST_POINT_free);
    }
    return rc;
}

/*
 * Create the trust store for an org in WIoTP proxy
 */
static int createTrustStore(tlsOrgConfig_t * orgConfig, ism_common_list * trustedCerts, X509_STORE ** pStore,
        struct stack_st_X509_NAME * caList) {
    char xbuf[2048];
    X509_STORE * store = NULL;
    int rc = ISMRC_OK;

    *pStore = NULL;
    if (trustedCerts) {
        STACK_OF(X509_NAME) * caListTmp = NULL;
        if (caList) {
            caListTmp = sk_X509_NAME_new(xname_cmp);
            if (caListTmp == NULL) {
                ism_common_setError(ISMRC_AllocateError);
                return ISMRC_AllocateError;
            }
        }
        store = X509_STORE_new();
        if (store) {
            ism_common_listIterator it;
            ism_common_list_iter_init(&it, trustedCerts);
            while (ism_common_list_iter_hasNext(&it)) {
                ism_common_list_node * node = ism_common_list_iter_next(&it);
                sslTrustCertData_t * certData = (sslTrustCertData_t *) node->data;
                X509 * cert = certData->cert;
                X509_NAME * xn = X509_get_subject_name(cert);
                int chrc = X509_check_ca(cert);
                if (xn && chrc == 1) {
                    xn = X509_NAME_dup(xn);
                    X509_STORE_add_cert(store, cert);
                    if (sk_X509_NAME_find(caListTmp,xn) >= 0) {
                        X509_NAME_free(xn);
                    } else {
                        sk_X509_NAME_push(caListTmp, xn);
                        sk_X509_NAME_push(caList, xn);
                    }
                } else {
                    char * errbuf = alloca(1024);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
                    uint32_t exflags = cert->ex_flags;
                    uint32_t kusage = cert->ex_kusage;
#else
                    uint32_t exflags = X509_get_extension_flags(cert);
                    uint32_t kusage = X509_get_key_usage(cert);
#endif
                    sprintf(errbuf, "check=%d ", chrc);
                    if ((exflags & EXFLAG_BCONS) == 0) {
                        strcat(errbuf, "No Basic Constraints ");
                    } else {
                        if ((exflags & EXFLAG_CA) == 0) {
                            strcat(errbuf, "Is not a CA");
                        }
                        if ((kusage & KU_KEY_CERT_SIGN) == 0) {
                            strcat(errbuf, "Certificate Sign not in Key Usage");
                        }
                    }

                    if (xn) {
                        X509_NAME_oneline(xn, xbuf, sizeof xbuf);
                    } else {
                        *xbuf = 0;
                        strcat(errbuf, "No Subject Name");
                    }
                    ism_common_setErrorData(ISMRC_CertificateNotValid, "%s%s%s", orgConfig->name, xbuf, errbuf);
                    TRACE(4, "The CA is not valid: org=%s ca=%s reason=%s\n", orgConfig->name, xbuf, errbuf);
                    LOG(ERROR, Server, 988, "%s%-s%-s", "The CA is not valid: Org={0} CA={1} Reason={2}",
                           orgConfig->name, xbuf, errbuf);
                    rc = ISMRC_CertificateNotValid;
                    X509_STORE_free(store);
                    store = NULL;
                    break;
                }
            }
            ism_common_list_iter_destroy(&it);
        } else {
            ism_common_setError(ISMRC_AllocateError);
            rc = ISMRC_AllocateError;
        }
        if (caListTmp) {
            sk_X509_NAME_free(caListTmp);
        }
    }
    *pStore = store;
    return rc;
}


/*
 * Free up an org TLS configuration for WIoTP
 */
static void freeOrgConfig(const char * name) {
    ism_common_HashMapLock(orgConfigMap);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, name, 0);
    if (orgConfig) {
        orgConfig->useCount--;
        if (orgConfig->useCount < 1) {
            if (orgConfig->crlUpdateTimer) {
                ism_common_cancelTimer(orgConfig->crlUpdateTimer);
                orgConfig->crlUpdateTimer = NULL;
            }
            if (orgConfig->ctx) {
                SSL_CTX_free(orgConfig->ctx);
            }
            if (orgConfig->serverCert) {
                ism_common_free(ism_memory_utils_sslutils,orgConfig->serverCert);
            }
            if (orgConfig->serverKey) {
                ism_common_free(ism_memory_utils_sslutils,orgConfig->serverKey);
            }
            if (orgConfig->trustCerts) {
                ism_common_free(ism_memory_utils_sslutils,orgConfig->trustCerts);
                ism_common_list_destroy(&orgConfig->trustCertsList);
            }
            if (orgConfig->name) {
                ism_common_free(ism_memory_utils_sslutils,(char *)orgConfig->name);
            }
            pthread_mutex_destroy(&orgConfig->lock);
            ism_common_free(ism_memory_utils_sslutils,orgConfig);
            ism_common_removeHashMapElement(orgConfigMap, name, 0);
        }
    }
    ism_common_HashMapUnlock(orgConfigMap);
}

typedef struct crlUpdateTask_t {
    const char * orgName;
    uint32_t generation;
} crlUpdateTask_t;


xUNUSED static crlUpdateTask_t * createUpdateCRLTask(const char * orgname, uint32_t generation) {
    crlUpdateTask_t * task = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,88),sizeof(crlUpdateTask_t) + strlen(orgname) + 1);
    if (task) {
        task->orgName = (char *)(task + 1);
        strcpy((char*) task->orgName, orgname);
        task->generation = generation;
    }
    return task;
}

typedef int (* tls_wait_cb_t)(int rc, void * data);

/*
 * Wait for a pending CRL
 *
 * This should only be called when the transport->crtChckStatus==9 which indicates that we previously
 * created a waiter object for this connection.
 *
 * A return value of 0 indicates that there is a waiter and there will be a callback.
 * A return value 0f 1 indicates that there is no waiter and the auth should continue.
 */
int ism_ssl_waitPendingCRL(ima_transport_info_t * transport, const char * org, void * callback, void * data) {
    int ret = 0;    /* wait */
    ism_common_HashMapLock(orgConfigMap);

    TRACE(6, "Wait for pending CRL: org=%s connect=%d\n", org, transport->index);

    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, org, 0);
    if (orgConfig) {
        tlsCrlWait_t * waiter;
        tlsCrlWait_t * oldwaiter = NULL;
        orgConfig->useCount++;
        ism_common_HashMapUnlock(orgConfigMap);
        pthread_mutex_lock(&orgConfig->lock);
        waiter = orgConfig->waiters;
        while (waiter) {
            /* If we have already completed the download, reply now */
            if (waiter->transport == transport) {
                if (waiter->rc != ISMRC_AsyncCompletion) {
                    tls_wait_cb_t waitcb = (tls_wait_cb_t) callback;
                    /* Unlink the waiter */
                    if (oldwaiter)
                        oldwaiter->next = waiter->next;
                    else
                        orgConfig->waiters = waiter->next;
                    transport->crtChckStatus = 0;
                    TRACE(8, "call wait callback: connect=%d rc=%s (%d)\n", transport->index,
                            X509_verify_cert_error_string(waiter->verify_rc), waiter->verify_rc);
                    waitcb(waiter->verify_rc, data);
                    ism_common_free(ism_memory_utils_sslutils,waiter);
                    waiter = NULL;
                }
                break;
            }
            oldwaiter = waiter;
            waiter = waiter->next;
        }

        /* If we found a waiter but it is not complete, set the callback and userdata for when complete */
        if (waiter) {
            waiter->userdata = data;
            waiter->replyCrlWait_cb = callback;
            ret = 1;    /* No waiter, so continue with auth */
        }
        pthread_mutex_unlock(&orgConfig->lock);
        freeOrgConfig(orgConfig->name);    /* Decrement the use count */
    } else {
        ism_common_HashMapUnlock(orgConfigMap);
        ret = 1;        /* No org config, so continue with auth */
    }
    return ret;
}

/*
 * Create a new CRL name
 */
static tlsCrl_t * newCrlObj(const char * crlname) {
    tlsCrl_t * ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,90),1, sizeof(tlsCrl_t) + strlen(crlname)+1);
    ret->name = (const char *)(ret+1);
    strcpy((char *)ret->name, crlname);
    ret->inprocess = 1;
    return ret;
}

/*
 * Add waiter for test only.
 * This does not do any locking so must not be used in production.
 */
int ism_ssl_test_addWaiter(ima_transport_info_t * transport, const char * org) {
    tlsCrlWait_t * waiter;
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, org, 0);
    if (orgConfig) {
        waiter = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,91),1, sizeof(tlsCrlWait_t));
        waiter->transport = transport;
        waiter->transport_ssl = transport->ssl;
        waiter->rc = ISMRC_AsyncCompletion;
        waiter->startwait = ism_common_currentTimeNanos();
        waiter->next = orgConfig->waiters;
        orgConfig->waiters = waiter;
        return 1;
    }
    return 0;
}

/*
 * Check if SSL can set the disable CRL
 */
int ism_ssl_isDisableCRL(ima_transport_info_t * transport) {
    return getDisableCRL(transport);
}

/*
 * We are unable to find a CRL, so create a waiter object and if necessary start a CRL download
 *
 * If this method returns 0, the invoker must at some later time call ism_ssl_waitPendingCRL().
 * @param transport   The transport object
 * @param org         The org name
 * @param cert        The certificate which has a CDP
 * @return 0=download started, 1=crl not available
 */
int ism_ssl_needCRL(ima_transport_info_t * transport, const char * org, X509 * cert) {
    ism_common_list cdpz = {0};
    ism_common_list * cdplist = &cdpz;
    tlsCrlWait_t * waiter;
    tlsCrlWait_t * oldwaiter = NULL;
    tlsCrl_t * crlobj;
    tlsCrl_t * oldCrlobj = NULL;
    int count;
    int len;
    int i;
    int need_download;
    int returncode = 1;
    char commonName [256];

    commonName[0] = 0;
    X509_NAME * name = X509_get_subject_name(cert);
    if (name)
        X509_NAME_get_text_by_NID(name, NID_commonName, commonName, sizeof commonName);
    TRACE(5, "Need CRL: org=%s connect=%d cert=%s\n", org, transport->index, commonName);
    ism_common_HashMapLock(orgConfigMap);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, org, 0);
    if (orgConfig) {
        orgConfig->useCount++;
        ism_common_HashMapUnlock(orgConfigMap);
        pthread_mutex_lock(&orgConfig->lock);

        /* Extract CDP */
        ism_common_list_init(cdplist, 0, (void*)ssl_free_data);
        if (!getDisableCRL || !getDisableCRL(transport))
            parseCrlLocations(cert, cdplist);
        count = ism_common_list_getSize(cdplist);
        if (count) {
            ism_common_listIterator it;

            /* Size the waiter object */
            ism_common_list_iter_init(&it, cdplist);
            len = count;
            while (ism_common_list_iter_hasNext(&it)) {
                ism_common_list_node * node = ism_common_list_iter_next(&it);
                len += strlen(node->data);
            }

            /* Construct the waiter object */
            waiter = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,92),1, sizeof(tlsCrlWait_t) + len + count*sizeof(void *));
            char * data = (char *)(waiter+1);
            data += count * sizeof(void *);
            waiter->transport = transport;
            waiter->transport_ssl = transport->ssl;
            waiter->count = count;
            waiter->rc = ISMRC_AsyncCompletion;
            waiter->startwait = ism_common_currentTimeNanos();

            ism_common_list_iter_init(&it, cdplist);
            for (i=0; ism_common_list_iter_hasNext(&it); i++) {
                ism_common_list_node * node = ism_common_list_iter_next(&it);
                waiter->crls[i] = data;
                strcpy(data, node->data);
                data += strlen(data) + 1;
            }
            ism_common_list_iter_destroy(&it);


            /* Make a CRL object if one is not found */
            need_download = 0;
            for (i=0; i<count; i++) {
                crlobj = orgConfig->crl;
                while (crlobj) {
                    if (!strcmp(waiter->crls[i], crlobj->name)) {
                        break;
                    }
                    oldCrlobj = crlobj;
                    crlobj = crlobj->next;
                }
                if (!crlobj) {
                    crlobj = newCrlObj(waiter->crls[i]);
                    /* Link to end of the list */
                    if (!oldCrlobj)
                        orgConfig->crl = crlobj;
                    else
                        oldCrlobj->next = crlobj;
                    need_download = 1;
                    returncode = 0;
                } else {
                    if (crlobj->state == TLSCRL_Never) {
                        returncode = 0;
                    }
                }
            }

            /* If we are waiting for a download, link the waiter, otherwise free it */
            if (returncode == 0) {
                transport->crtChckStatus = 9;
                /* Link waiter at end of waiter list */
                oldwaiter = orgConfig->waiters;
                if (!oldwaiter) {
                    orgConfig->waiters = waiter;
                } else {
                    while (oldwaiter->next)
                        oldwaiter = oldwaiter->next;
                    oldwaiter->next = waiter;
                }
            } else {
                ism_common_free(ism_memory_utils_sslutils,waiter);
            }

            /* Schedule the update */
            if (need_download) {
                crlUpdateTask_t * task = createUpdateCRLTask(orgConfig->name, orgConfig->generation);
                if (orgConfig->crlUpdateTimer) {
                    ism_common_cancelTimer(orgConfig->crlUpdateTimer);
                    orgConfig->crlUpdateTimer = NULL;
                }
                /* Request the update right now */
                crlUpdateTimer(NULL, ism_common_currentTimeNanos(), task);
            }
        }
        ism_common_list_destroy(cdplist);
        pthread_mutex_unlock(&orgConfig->lock);
        freeOrgConfig(orgConfig->name);    /* Decrement the use count */
    } else {
        ism_common_HashMapUnlock(orgConfigMap);
    }
    TRACE(7, "Return from needPendingCRL: connect=%d rc=%d check=%d\n", transport->index, returncode, transport->crtChckStatus);
    return returncode;
}

#ifndef NO_CURL

/*
 * Curl callback to write data (used in proxy)
 */
static size_t writeCB(char * ptr, size_t size, size_t nmemb, void * param) {
    concat_alloc_t * buf = (concat_alloc_t *) param;
    uint32_t length = (uint32_t)(size * nmemb);
    TRACE(9, "curl writeCB len=%d\n", (int)length);
    if (length) {
        ism_common_allocBufferCopyLen(buf, ptr, length);
    }
    return length;
}


/*
 * Download a CRL using CURL (used in proxy)
 * @param url  The URI to find the CRL
 * @param sinceTime  The time in seconds of the last time we got the file
 * @param fileTime   The time the file was updated
 */
static X509_CRL * downloadCRL(const char * url, const char * org, uint64_t sinceTime, long * fileTime, int * ec) {
    char xbuf [8192];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    X509_CRL * crl = NULL;
    CURL * curl = curl_easy_init();
    *ec = 0;
    if (curl) {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCB);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        if (sinceTime) {
            curl_easy_setopt(curl, CURLOPT_TIMEVALUE, sinceTime);
            curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        }
        curl_easy_setopt(curl, CURLOPT_FILETIME, 1);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            TRACE(3, "Download of CRL failed: org=%s crl=%s reason=%s\n", org, url, curl_easy_strerror(res));
            *ec = 1;
            LOG(ERROR, Server, 986, "%s%s%-s", "A CRL download failed: Org={0} CRL={1} Reason={2}",
                    org, url, curl_easy_strerror(res));
        } else {
            long ft = 0;
            long status = 0;
            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            if (res == CURLE_OK && status >= 400) {
                buf.used = 0;
                TRACE(3, "Download of CRL failed: org=%s crl=%s reason=%s\n", org, url, "Not found");
                *ec = 1;
                LOG(ERROR, Server, 986, "%s%s%-s", "A CRL download failed: Org={0} CRL={1} Reason={2}",
                        org, url, "Not found");
            } else {
                if (status == 304) {
                    TRACE(6, "CRL not download because it has not been updated: org=%s crl=%s\n", org, url);
                }
                res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &ft);
                if (res != CURLE_OK) {
                    TRACE(7, "curl_easy_getinfo no filetime: error=%s (%d) org=%s crl=%s\n", curl_easy_strerror(res), res, org, url);
                    ft = 0;
                }
                if (ft > 0)
                    *fileTime = ft;
            }
        }
        curl_easy_cleanup(curl);

        /*
         * If we got data, process the CRL
         * Note that not getting data could be due to a download failure, or that the file is not newer than
         * the previous downloaded CRL.
         */
        if (buf.used) {
            TRACE(5, "Downloaded CRL: org=%s crl=%s length=%d\n", org, url, (int)buf.used);
            TRACEDATA(9, "CRL Data", 0, buf.buf, buf.used, 4096);
            crl = readCRL(buf.buf, buf.used);
            if (!crl)
                *ec = 2;   /* Not PEM or DER */
        }
    } else {
        TRACE(3, "Failed to init curl\n");
        *ec = 1;
    }
    ism_common_freeAllocBuffer(&buf);
    return crl;
}

/*
 * Used for testing only
 */
int ism_ssl_downloadCRL(char * uri, const char * org, uint64_t sincetime, long * filetime) {
    int ec;
    downloadCRL(uri, org, sincetime, filetime, &ec);
    return ec;
}
#endif


/*
 * Add a job to the CRL update timer (proxy)
 */
static int crlUpdateTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    crlUpdateTask_t * task = (crlUpdateTask_t *) userdata;
    uint32_t index = ism_common_computeHashCode(task->orgName, 0) % NUM_OF_CU_THREADS;
    crlUpdateThread_t * pThread = &cuThreads[index];
    pthread_mutex_lock(&pThread->lock);
    ism_common_list_insert_tail(&pThread->updateReqsList, task);
    pthread_cond_signal(&pThread->cond);
    pthread_mutex_unlock(&pThread->lock);
    if (key)
        ism_common_cancelTimer(key);
    return 0;
}

/*
 * Add a CRL to the trust store (proxy)
 */
static int addCRL(X509_STORE *ctx, X509_CRL *crl) {
    /*TODO: check if there is a better way to remove crl */
    int i;
    int * idx2delete = NULL;
    int idx2deleCount = 0;

    /*
     * Remove the old CRL matching this one
     */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    CRYPTO_w_lock(CRYPTO_LOCK_X509_STORE);
    idx2delete = alloca(sizeof(int) * sk_X509_OBJECT_num(ctx->objs));
    for (i = 0; i < sk_X509_OBJECT_num(ctx->objs); i++) {
        X509_OBJECT * obj = sk_X509_OBJECT_value(ctx->objs, i);
        if (obj->type != X509_LU_CRL) {
            continue;
        }
        if (!X509_CRL_cmp(obj->data.crl, crl)) {
            idx2delete[idx2deleCount++] = i;
        }
    }
    for(i = 0; i < idx2deleCount; i++) {
        sk_X509_OBJECT_delete(ctx->objs, idx2delete[i]);
    }
    CRYPTO_w_unlock(CRYPTO_LOCK_X509_STORE);
#else
    X509_STORE_lock(ctx);
    int objcount = sk_X509_OBJECT_num(X509_STORE_get0_objects(ctx));
    idx2delete = alloca(sizeof(int) * objcount);
    for (i = 0; i < objcount; i++) {
        X509_OBJECT * obj = sk_X509_OBJECT_value(X509_STORE_get0_objects(ctx), i);
        if (X509_OBJECT_get_type(obj) != X509_LU_CRL) {
            continue;
        }
        if (!X509_CRL_cmp(X509_OBJECT_get0_X509_CRL(obj), crl)) {
            idx2delete[idx2deleCount++] = i;
        }
    }
    for(i = 0; i < idx2deleCount; i++) {
        sk_X509_OBJECT_delete(X509_STORE_get0_objects(ctx), idx2delete[i]);
    }
    X509_STORE_unlock(ctx);
#endif

    /*
     * Insert the new CRL
     */
    return X509_STORE_add_crl(ctx, crl);
}


/*
 * Check if the CRL is in the waiter list
 */
static int inWaiterList(const char * name, tlsCrlWait_t * waiter) {
    int i;
    for (i=0; i<waiter->count; i++) {
        if (!strcmp(name, waiter->crls[i]))
            return 1;
    }
    return 0;
}


/*
 * Stop the CRL wait for one connection
 */
int ism_ssl_stopCrlWait(ima_transport_info_t * transport, const char * org) {
    tlsCrlWait_t * waiter;
    tlsCrlWait_t * oldwaiter = NULL;
    tlsCrlWait_t * nextwaiter = NULL;

    int ret = 0;
    if (transport->crtChckStatus == 9 && org) {
        ism_common_HashMapLock(orgConfigMap);
        if (transport->crtChckStatus == 9) {
            tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, org, 0);
            if (orgConfig) {
                orgConfig->useCount++;
                ism_common_HashMapUnlock(orgConfigMap);
                pthread_mutex_lock(&orgConfig->lock);
                waiter = orgConfig->waiters;
                while (waiter) {
                    if (waiter->transport == transport) {
                        TRACE(5, "Free CRL waiter: connect=%d client=%s\n", transport->index, transport->name);
                        if (oldwaiter)
                            oldwaiter->next = waiter->next;
                        else
                            orgConfig->waiters = waiter->next;
                        nextwaiter = waiter->next;
                        ism_common_free(ism_memory_utils_sslutils,waiter);
                        ret++;
                    } else {
                        oldwaiter = waiter;
                        nextwaiter = waiter->next;
                    }
                    waiter = nextwaiter;
                }
                orgConfig->useCount--;
                pthread_mutex_unlock(&orgConfig->lock);
            } else {
                ism_common_HashMapUnlock(orgConfigMap);
            }
        }
    }
    return ret;
}

/*
 * Release the CRL waiters.
 *
 * This is called with the orgConfig locked
 */
static void releaseCrlWaiters(tlsOrgConfig_t * orgConfig, tlsCrl_t * crlobj) {
    tlsCrlWait_t * waiter;
    tlsCrlWait_t * oldwaiter = NULL;
    /* If this crl is valid or if all CRLs have now been processed, complete the waiters */
    if (crlobj->state != TLSCRL_Valid) {
        tlsCrl_t * checkCrl = orgConfig->crl;
        while (checkCrl) {
            if (checkCrl->state == TLSCRL_Never)
                return;
            checkCrl = checkCrl->next;
        }
    }

    /* For all waiters, rerun validation now that we have a CRL */
    waiter = orgConfig->waiters;
    while (waiter) {
        int badwaiter = 0;
        if (crlobj->state != TLSCRL_Valid || inWaiterList(crlobj->name, orgConfig->waiters)) {
            tls_wait_cb_t waitcb = (tls_wait_cb_t) waiter->replyCrlWait_cb;

            /* Rerun verify cert */
            if (waiter->transport && waiter->transport->ssl == waiter->transport_ssl &&
                *SSL_get_version(waiter->transport->ssl) == 'T') {
                waiter->verify_rc = ism_ssl_verifyCert(waiter->transport->ssl);
                waiter->rc = waiter->verify_rc ? ISMRC_CertificateNotValid : 0;
                TRACE(6, "Certificate reverified after CRL update: connect=%d rc=%d\n", waiter->transport->index, waiter->verify_rc);
            } else {
                TRACE(1, "The TLS context is not valid in releaseCrlWaiters: "
                         "    transport=%p ssl=%p waiter_ssl=%p time=%ld count=%u crl=%s\n",
                         waiter->transport, waiter->transport->ssl, waiter->transport_ssl,
                         (ism_common_currentTimeNanos()-waiter->startwait) / 1000000,
                         waiter->count, (waiter->count > 0 ? waiter->crls[0] : ""));
                LOG(ERROR, Server, 999, "%p%p%p%ld%u%s", "The TLS context is not valid in releaseCrlWaiters: "
                         "    waiter  transport={0} ssl={1} waiter_ssl={2} time={3} count={4} crl={5} version={6}",
                         waiter->transport, waiter->transport_ssl, waiter->transport_ssl,
                         (ism_common_currentTimeNanos()-waiter->startwait) / 1000000,
                         waiter->count, (waiter->count > 0 ? waiter->crls[0] : ""));
                waiter->rc = ISMRC_CertificateNotValid;
                badwaiter = 1;
            }

            /* Restart the waiter */
            if (waitcb) {
                if (!badwaiter) {
                    waiter->transport->crtChckStatus = 0;   /* No longer pending */
                    TRACE(8, "release call wait callback: connect=%d rc=%s (%d)\n", waiter->transport->index,
                            X509_verify_cert_error_string(waiter->verify_rc), waiter->verify_rc);
                    waitcb(waiter->verify_rc, waiter->userdata);
                }
                /* Unlink and free the waiter object */
                if (oldwaiter)
                    oldwaiter->next = waiter->next;
                else
                    orgConfig->waiters = waiter->next;
                tlsCrlWait_t * nextwaiter = waiter->next;
                ism_common_free(ism_memory_utils_sslutils,waiter);
                waiter = nextwaiter;
            } else {
                oldwaiter = waiter;
                waiter = waiter->next;
            }
        } else {
            oldwaiter = waiter;
            waiter = waiter->next;
        }
    }
}

/*
 * Enable a CRL and log that it is enabled
 */
static void enableCRL(X509_CRL * crl, const char * org, tlsCrl_t * crlobj) {
    char file_ts [32];
    char valid_ts [32];
    char issuer [512];
    ism_ts_t * ts;
    X509_NAME * issuer_name;

    /* Get info about CRL */
    crlobj->valid_ts = ism_ssl_convertTime(X509_CRL_get_nextUpdate(crl));
    issuer_name = X509_CRL_get_issuer(crl);
    issuer[0] = 0;
    X509_NAME_get_text_by_NID(issuer_name, NID_commonName, issuer, sizeof issuer);
    ts = ism_common_openTimestamp(ISM_TZF_UNDEF);
    ism_common_setTimestamp(ts, crlobj->last_update_ts);
    ism_common_formatTimestamp(ts, file_ts, sizeof file_ts, 6, ISM_TFF_ISO8601|ISM_TFF_FORCETZ );
    if (crlobj->valid_ts) {
        ism_common_setTimestamp(ts, crlobj->valid_ts);
        ism_common_formatTimestamp(ts, valid_ts, sizeof file_ts, 6, ISM_TFF_ISO8601|ISM_TFF_FORCETZ );
    } else {
        strcpy(valid_ts, "NotSet");
        crlobj->valid_ts = ism_common_currentTimeNanos() + (86400 * 1000000000UL);  /* One day */
    }
    ism_common_closeTimestamp(ts);

    ASN1_INTEGER * anum = X509_CRL_get_ext_d2i(crl, NID_crl_number, NULL, NULL);
    if (anum) {
        crlobj->crlNumber = ASN1_INTEGER_get(anum);
        ASN1_INTEGER_free(anum);
    }
    anum = X509_CRL_get_ext_d2i(crl, NID_delta_crl, NULL, NULL);
    if (anum) {
        crlobj->baseCrlNumber = ASN1_INTEGER_get(anum);
        crlobj->delta = 1;
        ASN1_INTEGER_free(anum);
        /* todo: Look for delta crl dp */
    }


    /* Log info */
    LOG(WARN, Server, 985, "%s%-s%ld%ld%-s%s%s", "A CRL is updated in the trust store: Org={0} CRL={1} Number={2} Base={3} Issuer={4} FileTime={5} ValidUntil={6}.",
            org, crlobj->name, crlobj->crlNumber, crlobj->baseCrlNumber, issuer, file_ts, valid_ts);

    /* Mark valid */
    crlobj->state = TLSCRL_Valid;
}


/*
 * Process a CRL update in proxy
 * This task will check all of the CRLs in an org
 */
static int processCRLUpdate(crlUpdateTask_t * task) {
    char * reason = NULL;
    int rc = 0;
    ism_common_HashMapLock(orgConfigMap);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, task->orgName, 0);
    TRACE(6, "Going to update CRLs for org %s: orgConfig=%p\n", task->orgName, orgConfig);
    if (orgConfig) {
        orgConfig->useCount++;
        ism_common_HashMapUnlock(orgConfigMap);
        pthread_mutex_lock(&orgConfig->lock);
        tlsCrl_t * crlobj;
        crlobj = orgConfig->crl;
        while (crlobj) {
            int  ec;
            X509_CRL * crl = NULL;
            long fileTime = 0;
            pthread_mutex_unlock(&orgConfig->lock);
            crlobj->inprocess = 1;
#ifndef NO_CURL
            crl = downloadCRL(crlobj->name, orgConfig->name, crlobj->last_update_ts / 1000000000L, &fileTime, &ec);
#else
            ec = 1;
            crl = NULL;
#endif
            pthread_mutex_lock(&orgConfig->lock);
            crlobj->inprocess = 0;

            /*
             * If we got a CRL mark it valid, log, and release waiters
             */
            if (crl) {
                if (addCRL(SSL_CTX_get_cert_store(orgConfig->ctx), crl)) {
                    crlobj->last_update_ts = fileTime * 1000000000L;     /* Convert to nanos */
                    enableCRL(crl, orgConfig->name, crlobj);
                    crlobj->state = TLSCRL_Valid;
                    releaseCrlWaiters(orgConfig, crlobj);
                } else {
                    X509_CRL_free(crl);
                    crl = NULL;
                    reason = "Unable to add CRL to trust store";
                    ec = 3;
                }
            }

            /*
             * If we did not get a CRL, log the error and conditionally release waiters
             */
            else {
                if (ec == 1)   /* This is already logged */
                    reason = "Unable to download CRL";
                if (ec == 2)
                    reason = "The downloaded CRL is not PEM or DER";
            }
            if (!crl && ec) {
                TRACE(3, "Could not update CRL: org=%s name=%s\n", task->orgName, crlobj->name);
                if (crlobj->state == TLSCRL_Never) {
                    crlobj->state = TLSCRL_Failed;
                    releaseCrlWaiters(orgConfig, crlobj);
                }
                /*
                 * ec==0 means there is no new CRL which is not an error
                 * ec==1 means that the download failed and we have already logg3ed that
                 */
                if (ec > 1) {
                    LOG(ERROR, Server, 987, "%s%s%-s", "A CRL update failed: Org={0} CRL={1} Reason={2}.",
                            orgConfig->name, crlobj->name, reason);
                }
            }
            crlobj = crlobj->next;
        }
        orgConfig->crlUpdateTimer = ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t )crlUpdateTimer, task,
                    crlUpdateDelay);
        pthread_mutex_unlock(&orgConfig->lock);
        freeOrgConfig(task->orgName);    /* Decrement the use count */
    } else {
        rc = 1;
        ism_common_free(ism_memory_utils_sslutils,task);
        ism_common_HashMapUnlock(orgConfigMap);
    }
    return rc;
}


/*
 * CRL update thread
 */
static void * crlUpdateProc(void * parm, void * context, int index) {
    crlUpdateThread_t * pThread = &cuThreads[index];
    pthread_mutex_lock(&pThread->lock);
    do {
        crlUpdateTask_t * task = NULL;
        ism_common_list_remove_head(&pThread->updateReqsList, (void**) &task);
        if (task == NULL) {
            pthread_cond_wait(&pThread->cond, &pThread->lock);
            continue;
        }
        pthread_mutex_unlock(&pThread->lock);
        processCRLUpdate(task);
        pthread_mutex_lock(&pThread->lock);
    } while (1);
    pthread_mutex_unlock(&pThread->lock);
    return NULL;
}


/*
 * Check if the changes to an org config require a new TLS context in WIoTP proxy
 * @return 1=needs new context, 0=use existing context
 */
static int newCtxRequired(tlsOrgConfig_t * orgConfig, const char * serverCert, const char * trustCerts) {

    /* If there is no existing context, create a new context */
    if (orgConfig->ctx == NULL)
        return 1;

    /* If the server cert has changed, create a new context */
    if (orgConfig->serverCert && serverCert) {
        if (strcmp(orgConfig->serverCert, serverCert))
            return 1;
    } else {
        if (orgConfig->serverCert != serverCert)
            return 1;
    }

    /* If the CAs have changed, create a new context */
    if (orgConfig->trustCerts && trustCerts) {
        if (strcmp(orgConfig->trustCerts, trustCerts))
            return 1;
    } else {
        if (orgConfig->trustCerts != trustCerts)
            return 1;
    }

    /* Use the existing context */
    return 0;
}


/*
 * Set the CAs into the TLS context in proxy
 */
static int setCAStore(tlsOrgConfig_t * orgConfig, SSL_CTX * ctx,  const char *trustCerts, ism_common_list * trustCertsList,
        ism_verifySSLCert verifyCallback) {
    X509_STORE * store = NULL;
    int rc = ISMRC_OK;
    STACK_OF(X509_NAME) * caList = sk_X509_NAME_new_null();
    if (caList == NULL) {
        ism_common_setError(ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
    rc = readCerts(trustCerts, strlen(trustCerts), trustCertsList, 1);
    if (rc != ISMRC_OK) {
        ism_common_list_destroy(trustCertsList);
        ism_common_setError(rc);
        return rc;
    }
    rc = createTrustStore(orgConfig, trustCertsList, &store, caList);
    if (rc != ISMRC_OK) {
        ism_common_list_destroy(trustCertsList);
        return rc;
    }
    SSL_CTX_set_client_CA_list(ctx, caList);
    SSL_CTX_set_cert_store(ctx, store);
    return rc;
}

/*
 * CRL verify callback for CRL reverify
 */
int crlVerifyCB(int good, X509StoreCtx * ctx) {
    int ret = good;
    SSL * ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    ima_transport_info_t * transport = SSL_get_app_data(ssl);

    int depth = X509_STORE_CTX_get_error_depth(ctx);
    int err = X509_STORE_CTX_get_error(ctx);
    TRACE(8, "reverify CRL callback connect=%u depth=%d good=%d err=%s (%d)\n", transport->index, depth, good,
            X509_verify_cert_error_string(err), err);
    switch (err) {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:   /* Allow if we cannot find the CRL */
    case X509_V_ERR_CERT_CHAIN_TOO_LONG:     /* This is a somewhat misleading error if we cannot find CRL issuer */
        X509_STORE_CTX_set_error(ctx, 0);
        ret = 1;
        break;
    }
    return ret;
}

/*
 * Set the TLS config for a WIoTP organization in proxy
 *
 * This creates the SSL context used when an SNI with this org is specified,
 * and sets up the trust store.
 * @param name  The name of the SNI context (the org name)
 * @param serverCert  The server certificate
 * @param serverKey   The server key
 * @param keyPassword The password for the server key
 * @param trustCerts  The CAs for the trust store
 * @param requireClientCert Are certs required
 * @param verifyCallback    The openssl cert verify callback
 * @return an ISMRC return code
 */
int ism_ssl_setSNIConfig(const char * name, const char * serverCert, const char * serverKey, const char * keyPassword,
        const char * trustCerts, int requireClientCert, ism_verifySSLCert verifyCallback) {
    int rc = ISMRC_OK;
    if ((!requireClientCert) && (serverCert == NULL))
        return rc;
    TRACE(5, "Set SNI config: name=%s requireClientCert=%d\n", name, requireClientCert);
    if (serverCert) {
        TRACE(9, "Server certificate: name=%s\n%s\n", name, serverCert);
        if (!serverKey) {
            TRACE(3, "Server certificate without server key: name=%s\n", name);
            rc = ISMRC_CreateSSLContext;
            ism_common_setErrorData(rc, "%s", "Server Key is null");
            return rc;
        }
    }
    if (trustCerts) {
        TRACE(9, "Trusted CA: name=%s\n%s\n", name, trustCerts);
    }

    ism_common_HashMapLock(orgConfigMap);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, name, 0);
    if (orgConfig == NULL) {
        orgConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_sslutils,97),1, sizeof(tlsOrgConfig_t));
        pthread_mutex_init(&orgConfig->lock, NULL);
        orgConfig->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),name);
        TRACE(7, "orgConfig was created for org %s: orgConfig=%p\n", name, orgConfig);
        orgConfig->useCount = 1;
        ism_common_putHashMapElement(orgConfigMap, name, 0, orgConfig, NULL);
    } else {
        TRACE(7, "Existing orgConfig was found for org %s: orgConfig=%p\n", name, orgConfig);
    }
    orgConfig->useCount++;
    ism_common_HashMapUnlock(orgConfigMap);
    pthread_mutex_lock(&orgConfig->lock);

    orgConfig->requireClientCert = requireClientCert;

    if (requireClientCert != 9 && newCtxRequired(orgConfig, serverCert, trustCerts)) {
        SSL_CTX * ctx = NULL;
        ism_common_list trustCertsList = {};
        orgConfig->generation++;
        do {   /* Just to allow a break */
            ctx = createCTX(name, serverCert, serverKey, requireClientCert, verifyCallback);
            if (ctx == NULL) {
                rc = ISMRC_CreateSSLContext;
                ism_common_setError(rc);
                break;
            }
            if (serverCert) {
                serverCert = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),serverCert);
                serverKey = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),serverKey);
                if ((serverCert == NULL) || (serverKey == NULL)) {
                    SSL_CTX_free(ctx);
                    if (serverCert)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverCert);
                    if (serverKey)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverKey);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    break;
                }
            }
            if (trustCerts && requireClientCert) {
                trustCerts = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_sslutils,1000),trustCerts);
                if (trustCerts == NULL) {
                    SSL_CTX_free(ctx);
                    if (serverCert)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverCert);
                    if (serverKey)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverKey);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    break;
                }
                rc = setCAStore(orgConfig, ctx, trustCerts, &trustCertsList, verifyCallback);
                if (rc != ISMRC_OK) {
                    SSL_CTX_free(ctx);
                    if (serverCert)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverCert);
                    if (serverKey)
                        ism_common_free(ism_memory_utils_sslutils,(char*)serverKey);
                    ism_common_free(ism_memory_utils_sslutils,(char*)trustCerts);
                    break;
                }
            }
        } while (0);

        /*
         * Delete the old values in the orgConfig
         */
        if (orgConfig->trustCerts) {
            ism_common_free(ism_memory_utils_sslutils,orgConfig->trustCerts);
            ism_common_list_destroy(&orgConfig->trustCertsList);
            orgConfig->trustCerts = NULL;
        }
        if (orgConfig->serverCert) {
            ism_common_free(ism_memory_utils_sslutils,orgConfig->serverCert);
            orgConfig->serverCert = NULL;
            ism_common_free(ism_memory_utils_sslutils,orgConfig->serverKey);
            orgConfig->serverKey = NULL;
        }
        if (orgConfig->crlUpdateTimer) {
            ism_common_cancelTimer(orgConfig->crlUpdateTimer);
            orgConfig->crlUpdateTimer = NULL;
        }
        if (orgConfig->ctx) {
            SSL_CTX_free(orgConfig->ctx);
            orgConfig->ctx = NULL;
        }

        /*
         * Set the new values in the orgConfig
         */
        if (rc == ISMRC_OK) {
            orgConfig->state = SSL_ORG_CFG_ENABLED;
            if (trustCerts && requireClientCert) {
                orgConfig->trustCerts = (char*)trustCerts;
                memcpy(&orgConfig->trustCertsList, &trustCertsList, sizeof(ism_common_list));
            }
            orgConfig->serverCert = (char*)serverCert;
            orgConfig->serverKey = (char*)serverKey;
            orgConfig->ctx = ctx;
            TRACE(6, "New TLS SNI context created: org=%s ctx=%p\n", name, ctx);

            /*
             * For all known CRL distribution points in this org, set the CRL to not loaded
             * and start a download.  This is need because we create a new context which thus
             * no longer holds the CRLs we previously downloaded.
             */
            tlsCrl_t * crl = orgConfig->crl;

            g_cunitCRLcount = 0;
            while (crl) {
                g_cunitCRLcount++;
                crl->state = TLSCRL_Never;
                crl->last_update_ts = 0;
                crl->next_update_ts = 0;
                crl->valid_ts = 0;
                crl = crl->next;
            }
            if (orgConfig->crl) {
                TRACE(5, "Found CRLs when TLS SNI context is updated: org=%s count=%u\n", name, g_cunitCRLcount);
                crlUpdateTask_t * task = createUpdateCRLTask(orgConfig->name, orgConfig->generation);
                crlUpdateTimer(NULL, ism_common_currentTimeNanos(), task);
            }
        } else {
            char * xbuf = alloca(4096);
            ism_common_formatLastError(xbuf, 4096);
            orgConfig->state = SSL_ORG_CFG_DISABLED;
            TRACE(3, "The TLS Context is cannot be created: Org=%s Error=%s (%d)\n", name, xbuf, rc);
            LOG(ERROR, Server, 989, "%s%-s%d", "The TLS SNI Context cannot be created: Org={0} Error={1} ErrorCode={2}.",
                    name, xbuf, rc);
        }
    }
    pthread_mutex_unlock(&orgConfig->lock);
    freeOrgConfig(name);       /* Decrement use count */
    return rc;
}


/*
 * Free up an org config
 */
int ism_ssl_removeSNIConfig(const char * name) {
    TRACE(5, "Remove SNI config: name=%s\n", name);
    freeOrgConfig(name);
    return ISMRC_OK;
}


/*
 * Set the SNI context per org in WIoTP proxy
 */
int ism_ssl_setSniCtx(SSL *ssl, const char * sniName, int * requireClientCert, int * updated) {
    ism_common_HashMapLock(orgConfigMap);
    tlsOrgConfig_t * orgConfig = ism_common_getHashMapElement(orgConfigMap, sniName, 0);
    if (orgConfig) {
        int rc = SSL_TLSEXT_ERR_ALERT_FATAL;
        orgConfig->useCount++;
        ism_common_HashMapUnlock(orgConfigMap);
        pthread_mutex_lock(&orgConfig->lock);
        *requireClientCert = orgConfig->requireClientCert;
        *updated = 1;
        if (orgConfig->ctx && (orgConfig->state == SSL_ORG_CFG_ENABLED)) {
            SSL_CTX * ctx = orgConfig->ctx;
            long options = SSL_get_options(ssl);
            SSL_set_SSL_CTX(ssl, ctx);
            SSL_set_verify(ssl, SSL_CTX_get_verify_mode(ctx), SSL_CTX_get_verify_callback(ctx));
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            X509_VERIFY_PARAM_inherit(ssl->param, ctx->param);
#else
            X509_VERIFY_PARAM_inherit(SSL_get0_param(ssl), SSL_CTX_get0_param(ctx));
#endif
            options |= SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
            SSL_set_options(ssl, options);
//            SSL_set_client_CA_list(ssl, SSL_CTX_get_client_CA_list(ctx));
            rc = SSL_TLSEXT_ERR_OK;
        }
        pthread_mutex_unlock(&orgConfig->lock);
        freeOrgConfig(sniName);
        return rc;
    }
    ism_common_HashMapUnlock(orgConfigMap);
    *requireClientCert = 0;
    return SSL_TLSEXT_ERR_OK;
}

/*
 * Convert an ASN1 time to a nanosecond timestamp
 */
ism_time_t ism_ssl_convertTime(const ASN1_TIME * ctime) {
    ism_time_t ret;
    int len;
    char * data;
    ism_timeval_t t = {0};
    ism_ts_t * ts;
    int good = 0;

    if (!ctime) {
        return 0;
    }
    data = (char *)ctime->data;
    len = ctime->length;
    if (ctime->type == V_ASN1_UTCTIME) {
        if ((len==11 || len==13) && data[len-1] == 'Z') {
            good = 1;
            t.year = (data[0]-'0') * 10 + (data[1]-'0');
            if (t.year < 50)
                t.year += 2000;
            else
                t.year += 1900;
            good = 1;
            if (len==13)
                t.second = (data[10]-'0') * 10 + (data[11]-'0');
            data += 2;
        }
    } else if (ctime->type == V_ASN1_GENERALIZEDTIME) {
        if ((len==13 || len==15) && data[len-1]=='Z') {
            good = 1;
            t.year = (data[0]-'0') * 1000 + (data[1]-'0') * 100 + (data[2]-'0') * 10 + (data[3]-'0');
            if (len==15)
                t.second = (data[12]-'0') * 10 + (data[13]-'0');
            data += 4;
        }
    }
    if (!good) {
        ism_common_setError(ISMRC_Error);
        return 0;
    }

    t.month  = (data[0]-'0') * 10 + (data[1]-'0');
    t.day    = (data[2]-'0') * 10 + (data[3]-'0');
    t.hour   = (data[4]-'0') * 10 + (data[5]-'0');
    t.minute = (data[6]-'0') * 10 + (data[7]-'0');

    /* Convert to binary timestamp */
    ts = ism_common_openTimestamp(ISM_TZF_UNDEF);
    ism_common_setTimestampValues(ts, &t, 0);
    ret = ism_common_getTimestamp(ts);
    ism_common_closeTimestamp(ts);
    return ret;
}
