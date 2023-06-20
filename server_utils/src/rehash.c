/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define TRACE_COMP Util
#include <ismutil.h>
#include <ismregex.h>
#include <unistd.h>
#include <dirent.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

/*
 * @file rehash.c
 *
 * This file implements the openSSL c_rehash function in C.  The original is implemented in perl.
 * In openssl 1.1 this was re-implemented as an app within openssl.
 *
 * The purpose of this is so that the truststore can find certificates and CRLs in the truststore
 * directory using the hash of the subject of the certificate rather than looking at all files
 * in the directory.
 *
 * The hash entries are of the form of 8 hex digits with an extension ending in a number.  CRL entries
 * are the same but have an 'r' at the start of the extension.  Any symbolic link in the directory
 * which does not point to a PEM file in this directory will be deleted when the hash is run.
 *
 * Unlike the original (or the openssl app) this version does allow the PEM file to contain multiple
 * X509 objects.  This matches the way files are loaded when searching the trust store.
 *
 * TODO:
 * * Add more error checking
 * * Process directory without removing existing valid links
 */

/*
 * Structure with the hash and fingerprint of X509 object.
 * If the file contains multiple X509 objects there will be an array of these objects
 */
typedef struct file_hash_t {
    char    hash [10];         /* The has is 8 hex characters - null terminated */
    uint8_t kind;
    uint8_t resv;
    char    fingerprint [20];  /* The fingerprint is 20 binary bytes */
} file_hash_t;

/*
 * Forward and external references
 */
static int doTrustFile(const char * dirpath, const char * name, file_hash_t * * hash) ;

/*
 * Get the contents of a file into a buffer.
 *
 * The invoker can give a max size of the file to read in.  Most PEM files are quite small and if the
 * directory contains large files we do not want to deal with them.
 *
 * We assume this has already been checked to be a normal file
 */
static int getFileContent(const char * path, const char * name, char * * xbuf, int maxsize) {
    FILE * f;
    long   len;
    char * fname;
    char * buf;
    int    bread = 0;

    if (path) {
        fname = alloca(strlen(path)+strlen(name)+2);
        strcpy(fname, path);
        strcat(fname, "/");
        strcat(fname, name);
    } else {
        fname = (char *)name;
    }

    *xbuf = NULL;
    f = fopen(fname, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    len =  ftell(f);
    if (len >= 0 && len < maxsize) {
        buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_rehash,122),len+1);
        if (buf) {
            buf[len] = 0;
            rewind(f);
            *xbuf = buf;
            bread = fread(buf, 1, len, f);
        }
    } else {
        *xbuf = NULL;
        return 0;
    }
    fclose(f);
    return bread;
}


/*
 * Check if the string is a hash link
 * We assume it has already been checked to be a symbolic link
 */
static int isHashLink(const char * name) {
    static ism_regex_t hashlink_p = NULL;
    if (!hashlink_p) {
        ism_regex_compile(&hashlink_p, "^[0-9A-Za-z]{8}\\.r?[0-9]+$");
    }
    return !ism_regex_match(hashlink_p, name);
}


/*
 * Remove a hash link
 */
static void removeHashLink(const char * dirpath, const char * name) {
    char * fname;
    if (dirpath) {
        fname = alloca(strlen(dirpath)+strlen(name)+2);
        strcpy(fname, dirpath);
        strcat(fname, "/");
        strcat(fname, name);
    } else {
        fname = (char *)name;
    }
    remove(fname);
}


/*
 * Link in a hashlink
 */
static int linkHashLink(file_hash_t * hash, const char * dirpath, const char * toName, int verbose) {
    char hashfile [64];
    char fname [2048];
    file_hash_t * targethash;
    int unique = 0;
    int targetcount;
    int i;

    /* Check for duplicates and collisions */

    for (;;) {
        sprintf(hashfile, (hash->kind == 'r' ? "%s.r%d" : "%s.%d"), hash->hash, unique);
        snprintf(fname, sizeof fname, "%s/%s", dirpath, hashfile);
        if (access(fname, R_OK)) {
            removeHashLink(dirpath, hashfile);
            break;
        }
        targetcount = doTrustFile(dirpath, hashfile, &targethash);
        for (i=0; i<targetcount; i++) {
            /* If it already points to a file containing this fingerprint, no link is required */
            if (!memcmp(targethash[i].fingerprint, hash->fingerprint, sizeof(hash->fingerprint))) {
                ism_common_free(ism_memory_utils_rehash,targethash);
                return 0;
            }
        }
        ism_common_free(ism_memory_utils_rehash,targethash);
        unique++;
        sprintf(hashfile, (hash->kind == 'r' ? "%s.r%d" : "%s.%d"), hash->hash, unique);
    }

    /* Create a symbolic link */
    if (verbose&1)
        TRACE(7, "Add hash link: %s to %s\n", fname, toName);
    if (verbose&2)
        printf("Add hash link: %s to %s\n", fname, toName);
    if ( 0 == symlink(toName, fname)) {
        return 1;
    }
    return 0;
}


/*
 * Get the hash and fingerprint for a file or part of file
 */
static int getCertHash(const char * buf, int len, int kind, file_hash_t * hash) {
    BIO * membio = BIO_new_mem_buf(buf, len);
    X509 * cert;
    X509_CRL * crl;
    uint32_t  size = 0;
    uint8_t fingerbuf[64];

    hash->kind = (uint8_t)kind;
    if (kind == 'c') {
        /* Do a certificate or trusted certificate */
        cert = PEM_read_bio_X509(membio, NULL, NULL, NULL);
        if (!cert)
            return 1;
        X509_digest(cert, EVP_sha1(), fingerbuf, &size);
        memcpy(hash->fingerprint, fingerbuf, sizeof(hash->fingerprint));
        sprintf(hash->hash, "%08lx", X509_subject_name_hash(cert));
    } else {
        /* Do a CRL */
        crl = PEM_read_bio_X509_CRL(membio, NULL, NULL, NULL);
        if (!crl)
            return 1;
        X509_CRL_digest(crl, EVP_sha1(), fingerbuf, &size);
        memcpy(hash->fingerprint, fingerbuf, sizeof(hash->fingerprint));
        sprintf(hash->hash, "%08lx", X509_NAME_hash(X509_CRL_get_issuer(crl)));
    }
    return 0;
}

/*
 * Generate a list of hash objects for this file.
 *
 * If the file is not a certificate or CRL return the count of zero.
 * If a non-zero count is returned, it must be freed by the invoker.
 *
 * This only works with PEM files, but it does allow the PEM file to contain multiple
 * certificates.  The max cert file size is 128K
 *
 */
static int doTrustFile(const char * dirpath, const char * name, file_hash_t * * hash) {
    int hashcount = 0;
    char * filebuf = NULL;
    char * begin;
    char * end_pos;
    file_hash_t hashes[100];  /* most certs in a file */
    int    maxhash = 100;

    /* Read the whole file into memory */
    int filelen = getFileContent(dirpath, name, &filebuf, 128*1024);
    if (filelen) {
        /* Break into segments based on the PEM headers.  Ignore lines not in a PEM begin/end sequence */
        begin = strstr(filebuf, "-----BEGIN ");
        while (begin) {
            int kind = 0;
            end_pos = NULL;
            /* This is most likely some kind of PEM file, but check what kind */
            char * endkind = strstr(begin+11, "----");
            if (endkind) {
                int kindlen = endkind-begin-11;
                /* We handle various kinds of Certificates and CRLs.  We do not care about various private keys */
                if (kindlen == 11  && !memcmp(begin+11, "CERTIFICATE", 11)) {
                    kind = 'c';
                } else if (kindlen == 16 && !memcmp(begin+11, "X509 CERTIFICATE", 16)) {
                    kind = 'c';
                } else if (kindlen == 19 && !memcmp(begin+11, "TRUSTED CERTIFICATE", 19)) {
                } else if (kindlen == 8 && !memcmp(begin+11, "X509 CRL", 8)) {
                    kind = 'r';
                }
                end_pos = strstr(endkind, "-----END ");
                if (end_pos) {
                    end_pos = strstr(end_pos+9, "-----");
                    if (end_pos)
                        end_pos += 5;
                }
            }
            if (kind && end_pos) {
                /* Get the set of hash and fingerprint for this PEM segment */
                if (!getCertHash(begin, end_pos-begin, kind, hashes+hashcount)) {
                    if (++hashcount >= maxhash)
                        break;
                }
            } else {
                if (!end_pos)
                    end_pos = begin+11;
            }
            begin = strstr(end_pos, "-----BEGIN ");
        }
    }

    /* If this is a PEM file, return the hash and fingerprint objects we found */
    if (hashcount) {
        file_hash_t * ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_rehash,125),hashcount * sizeof(file_hash_t));
        memcpy(ret, hashes, hashcount * sizeof(file_hash_t));
        *hash = ret;
    }

    /* Free up the buffer containing the file */
    if (filebuf)
        ism_common_free(ism_memory_utils_rehash,filebuf);
    return hashcount;
}


/*
 * Do the openSSL trust store hash for one directory
 *
 * @param dirpath The path to the directory
 * @param leave_links  Remove old links and re-insert
 * @param verbose Bitmask how noisy to be: 0=none, 1=trace, 2=print, 8=very
 * @return 0=good 1=unable to open directory
 *
 * Using leave_links=1 can leave stale links in the directory, but is not harmful otherwise.
 * This is commonly used when the trust store is updated at run time.
 */
int  ism_common_hashTrustDirectory(const char * dirpath, int leave_links, int verbose) {
    DIR * dp = NULL;
    struct dirent * dent;
    file_hash_t * hash;
    int  i;
    int  rmcount = 0;
    int  addcount = 0;

    if (access(dirpath, R_OK | W_OK | X_OK)) {
        TRACE(4, "Unable to update truststore: %s\n", dirpath);
        if (verbose&2)
            printf("Unabled to update truststore: %s\n", dirpath);
        return 1;
    } else {
        TRACE(5, "Hash truststore: %s\n", dirpath);
    }
    dp = opendir(dirpath);
    if (dp) {
        if (!leave_links) {
            dent = readdir(dp);
            while (dent) {
                /* Delete any existing hash links */
                if (dent->d_type == DT_LNK) {
                    if (isHashLink(dent->d_name)) {
                        if (verbose&1)
                            TRACE(7, "Remove hash link: %s/%s\n", dirpath, dent->d_name);
                        if (verbose&2)
                            printf("Remove hash link: %s/%s\n", dirpath, dent->d_name);
                        removeHashLink(dirpath, dent->d_name);
                        rmcount++;
                    }
                }
                dent = readdir(dp);
            }
            rewinddir(dp);
        }
        dent = readdir(dp);
        while (dent) {
            /* Process normal files */
            if (dent->d_type == DT_REG) {
                int hashcount = doTrustFile(dirpath, dent->d_name, &hash);
                if (hashcount) {
                    for (i=0; i<hashcount; i++) {
                        addcount += linkHashLink(hash+i, dirpath, dent->d_name, verbose);
                    }
                    ism_common_free(ism_memory_utils_rehash,hash);
                }
            }
            dent = readdir(dp);
        }
        closedir(dp);
        TRACE(4, "Hash truststore %s: removed=%u added=%u\n", dirpath, rmcount, addcount);
        if (verbose&2)
            printf("Hash truststore %s: removed=%u added=%u\n", dirpath, rmcount, addcount);
        return 0;
    } else {
        TRACE(4, "Unable to open trust store directory: %s\n", dirpath);
        if (verbose&2)
            printf("Unabled to open trust store directory: %s\n", dirpath);
        return 1;
    }
}
