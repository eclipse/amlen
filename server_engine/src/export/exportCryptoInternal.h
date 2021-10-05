/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

//*********************************************************************
/// @file exportCryptoInternal.h
/// @brief Internal datastructures used in reading and writing encrypted files
///
/// Uses OpenSSL (libcrypto) to read and write encrypted data to files
//*********************************************************************

#ifndef EXPORTCRYPTOINTERNAL_H_DEFINED
#define EXPORTCRYPTOINTERNAL_H_DEFINED

#include "engineCommon.h"
#include "memHandler.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

typedef struct tag_ieieEncryptedFile_t {
    char StructId[4]; //IEEF
    iemem_memoryType memType;
    bool Writing;     ///< Are we writing (or reading?)
    EVP_CIPHER_CTX *ctx;
    unsigned char key[32];
    FILE *fp;
    unsigned char    *unencryptedReadBuffer;
    size_t   buffSize;
    unsigned char    *buffCursor;
    size_t   buffUnreadBytes;
    bool     finishedDecrypt;
} ieieEncryptedFile_t;

///  Eyecatcher for ieieEncryptedFile_t  structure
#define IEIE_ENCRYPTEDFILE_STRUCID "IEEF"

typedef struct tag_ieieEncryptedFileHeader_t {
    char StructId[4]; //IEFH
    uint32_t Version;
    unsigned char IV[16]; //128bit initialisation that was randomly generated
} ieieEncryptedFileHeader_t;

///  Eyecatcher for ieieEncryptedFileHeader_t structure
#define IEIE_ENCRYPTEDFILEHEADER_STRUCID "IEFH"

typedef struct tag_ieieEncryptedRecordHeader_t {
    char     EyeCatcher;    //'x'
    uint8_t  Version;       //'1'
    uint16_t DataType;
    uint32_t DataLength;
    uint64_t DataId;
} ieieEncryptedRecordHeader_t;

#define IEIE_ENCRYPTEDRECORDHEADER_EYECATCHER 'x'
#define IEIE_ENCRYPTEDRECORDHEADER_CURRENT_VERSION 1

#endif
