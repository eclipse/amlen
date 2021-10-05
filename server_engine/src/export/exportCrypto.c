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

#define TRACE_COMP Engine

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include <openssl/rand.h>

#include "engineInternal.h"
#include "exportCryptoInternal.h"
#include "exportCrypto.h"
#include "memHandler.h"

const unsigned char salt[8] = {'W','O','M','B','L','E','5'};

ieieEncryptedFileHandle_t ieie_createEncryptedFile( ieutThreadData_t *pThreadData
                                                  , iemem_memoryType memType
                                                  , const char *filePath
                                                  , const char *password)
{
    ieutTRACEL(pThreadData, filePath, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "filePath(%s)\n", __func__,
               filePath);

    ieieEncryptedFile_t *hFile = NULL;
    FILE *fp = NULL;
    int error;

    // We want a new file. If the file path already exists, unlink it first so
    // that we can expect open with O_EXCL to give us a new file (unless someone
    // else gets in first).
    int osrc = unlink(filePath);
    error = errno;

    // If the file was unlinked trace that out for information
    if (osrc == 0)
    {
        ieutTRACEL(pThreadData, error, ENGINE_UNUSUAL_TRACE,
                   "filePath(%s) unlinked\n", filePath);
    }
    // If the unlink failed for anything other than ENOENT (meaning it didn't exist)
    // add a trace line, for information but we continue to attempt to open the file.
    else if (errno != ENOENT)
    {
        ieutTRACEL(pThreadData, error, ENGINE_INTERESTING_TRACE,
                   "filePath(%s) unlink failed errno(%d), continuing\n",
                   filePath, error);
    }

    int fd = open(filePath, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR);

    if (fd == -1)
    {
        error = errno;

        ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                    "filePath(%s) open errno(%d)\n", filePath, error);
        goto mod_exit;
    }

    fp = fdopen(fd, "wb");

    if (fp == NULL)
    {
        error = errno;

        ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                    "filePath(%s) fdopen errno(%d)\n", filePath, error);
        close(fd);
        goto mod_exit;

    }

    hFile = iemem_calloc(pThreadData,
                         IEMEM_PROBE(memType, 60500), 1,
                         sizeof(ieieEncryptedFile_t));

    if (hFile == NULL)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) ALLOC ERROR\n", filePath);
        fclose(fp);
        goto mod_exit;
    }

    ismEngine_SetStructId(hFile->StructId, IEIE_ENCRYPTEDFILE_STRUCID);
    hFile->memType = memType;
    hFile->Writing = true;
    hFile->fp = fp;

    ieieEncryptedFileHeader_t fileHeader = {{0},0,{0}};
    ismEngine_SetStructId(fileHeader.StructId, IEIE_ENCRYPTEDFILEHEADER_STRUCID);
    fileHeader.Version = 1;


    //Generate the key bytes and iv from the password.
    const EVP_CIPHER *cipher = NULL;
    const EVP_MD *digest = NULL;


    cipher = EVP_get_cipherbyname("aes-256-cbc");
    if (!cipher)
    {
        ieutTRACEL(pThreadData, hFile, ENGINE_ERROR_TRACE,
                "filePath(%s) Unable to get aes-256-cbc cipher\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    digest=EVP_get_digestbyname("sha256");
    if (!digest)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) ALLOC ERROR\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    if(!EVP_BytesToKey(cipher, digest, salt,
            (unsigned char *) password,
            strlen(password),
            10278,
            hFile->key,
            fileHeader.IV))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) couldn't generate key\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }


    /* Create and initialise the context */
    if(!(hFile->ctx = EVP_CIPHER_CTX_new()))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) couldn't create EVP context\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    /*  We are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_EncryptInit_ex(hFile->ctx,
                               cipher,
                               NULL,
                               hFile->key,
                               fileHeader.IV))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) couldn't initialise encryption\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        EVP_CIPHER_CTX_free(hFile->ctx);
        hFile = NULL;
        goto mod_exit;
    }

    size_t numObjects = fwrite(&fileHeader, sizeof(fileHeader), 1, fp);

    if (1 != numObjects)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) couldn't write initial header\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, memType, hFile);
        EVP_CIPHER_CTX_free(hFile->ctx);
        hFile = NULL;
        goto mod_exit;
    }




mod_exit:
    ieutTRACEL(pThreadData, hFile, ENGINE_FNC_TRACE,
           FUNCTION_EXIT "hFile %p\n", __func__, hFile);
    return hFile;
}

//
int32_t ieie_finishWritingEncryptedFile ( ieutThreadData_t *pThreadData
                                        , ieieEncryptedFileHandle_t file )
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, file, ENGINE_FNC_TRACE,
           FUNCTION_ENTRY "file %p\n", __func__, file);

    assert(file->Writing);
    unsigned char finalBytes[64];
    int len = sizeof(finalBytes);

    int encryptRC = EVP_EncryptFinal_ex(file->ctx, finalBytes, &len);
    if(1 == encryptRC)
    {
        size_t numObjectsWritten = fwrite(finalBytes, len, 1, file->fp);

        if (numObjectsWritten != 1)
        {
            int error = errno;

            ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                    "Failed to write to file. errno = %d\n", error);
            rc = ISMRC_FileCorrupt;
        }
    }
    else
    {
        ieutTRACEL(pThreadData, encryptRC, ENGINE_ERROR_TRACE,
              "Failed to encrypt data for file. rc = %d\n", encryptRC);
        rc = ISMRC_SSLError;
    }
    fclose(file->fp);
    EVP_CIPHER_CTX_free(file->ctx);
    iemem_free(pThreadData, file->memType, file);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE,
           FUNCTION_EXIT "rc %d\n", __func__, rc);

    return rc;
}

//
int32_t ieie_finishReadingEncryptedFile ( ieutThreadData_t *pThreadData
                                        , ieieEncryptedFileHandle_t file )
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, file, ENGINE_FNC_TRACE,
                          FUNCTION_IDENT "file %p\n", __func__, file);

    if (!(file->finishedDecrypt))
    {
        ieutTRACEL(pThreadData, file->buffUnreadBytes, ENGINE_WORRYING_TRACE,
                "Finishing reading but more unread data. Unread Bytes decrypted already %lu\n",
                file->buffUnreadBytes);
    }
    assert(!(file->Writing));
    fclose(file->fp);
    EVP_CIPHER_CTX_free(file->ctx);
    iemem_free(pThreadData, file->memType, file->unencryptedReadBuffer);
    iemem_free(pThreadData, file->memType, file);

    return rc;
}

//Total Length of frags must fit in signed int32_t (with room for blocksize padding)
static inline int32_t ieie_internalExportDataFrags( ieutThreadData_t *pThreadData
                                                  , ieieEncryptedFileHandle_t file
                                                  , ieieDataType_t dataType
                                                  , uint64_t dataId
                                                  , ieieFragmentedExportData_t *pFragsData
                                                  , uint32_t LargestFragSize
                                                  , uint32_t TotalDataLen)
{
    int len = sizeof(ieieEncryptedRecordHeader_t)+32;//Must be at least blocksize bigger than sizeof structure
    unsigned char cipherheader[len];

    assert(LargestFragSize <= INT_MAX);
    int ciphertext_bufsize =  LargestFragSize + 32; //Must be at least blocksize bigger than size of biggest frag
    unsigned char ciphertext[ciphertext_bufsize];

    ieieEncryptedRecordHeader_t recordHdr = {0};
    int32_t rc = OK;

    recordHdr.EyeCatcher = IEIE_ENCRYPTEDRECORDHEADER_EYECATCHER;
    recordHdr.Version    = IEIE_ENCRYPTEDRECORDHEADER_CURRENT_VERSION;

    recordHdr.DataId     = dataId;
    recordHdr.DataLength = TotalDataLen;
    recordHdr.DataType   = dataType;

    int encrypt_rc = EVP_EncryptUpdate(file->ctx, cipherheader, &len, (unsigned char *)&recordHdr, sizeof(recordHdr));
    if(1 == encrypt_rc)
    {
        if (len > 0)
        {
            size_t numObjectsWritten = fwrite(cipherheader, len, 1, file->fp);

            if (numObjectsWritten != 1)
            {
                int error = errno;

                ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                      "Failed to write data to file. rc = %d\n", error);
                rc = ISMRC_FileCorrupt;
                goto mod_exit;
            }
        }
    }
    else
    {
        ieutTRACEL(pThreadData, encrypt_rc, ENGINE_ERROR_TRACE,
              "Failed to encrypt data for file. rc = %d\n", encrypt_rc);
        rc = ISMRC_SSLError;
        goto mod_exit;
    }

    for (uint32_t i=0; i < pFragsData->FragsCount; i++)
    {
        len = ciphertext_bufsize;

        encrypt_rc = EVP_EncryptUpdate( file->ctx
                                      , ciphertext
                                      , &len
                                      , pFragsData->pFrags[i]
                                      , (int)pFragsData->pFragsLengths[i]);
        if(1 == encrypt_rc)
        {
            if (len > 0)
            {
                size_t numObjectsWritten = fwrite(ciphertext, len, 1, file->fp);

                if (numObjectsWritten != 1)
                {
                    int error = errno;

                    ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                            "Failed to write data to file. rc = %d\n", error);
                    rc = ISMRC_FileCorrupt;
                    goto mod_exit;
                }
            }
        }
        else
        {
            ieutTRACEL(pThreadData, encrypt_rc, ENGINE_ERROR_TRACE,
                  "Failed to encrypt data for file. rc = %d\n", encrypt_rc);
            rc = ISMRC_SSLError;
            goto mod_exit;
        }
    }

mod_exit:
    return rc;
}


int32_t ieie_exportData( ieutThreadData_t *pThreadData
                       , ieieEncryptedFileHandle_t file
                       , ieieDataType_t dataType
                       , uint64_t dataId
                       , int dataLen
                       , void *data)
{
    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "dataId %lu dataLen %d\n", __func__,
               dataId, dataLen);

    assert(dataLen > 0);
    ieieFragmentedExportData_t fragsData = {1, &data, (uint32_t *)&dataLen};
    int32_t rc = ieie_internalExportDataFrags( pThreadData
                                             , file
                                             , dataType
                                             , dataId
                                             , &fragsData
                                             , dataLen
                                             , dataLen );

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
                    FUNCTION_EXIT "rc %d\n", __func__, rc);

    return rc;
}

//Total Length of frags must fit in signed int32_t (with room for blocksize padding)
int32_t ieie_exportDataFrags( ieutThreadData_t *pThreadData
                            , ieieEncryptedFileHandle_t file
                            , ieieDataType_t dataType
                            , uint64_t dataId
                            , ieieFragmentedExportData_t *pFragsData)
{
    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "dataId %lu NumFrags %u\n", __func__,
               dataId, pFragsData->FragsCount);

    int LargestFragSize = 0;
    uint64_t  TotalDataLen = 0;
    for (uint32_t i=0; i < pFragsData->FragsCount; i++)
    {
        assert(pFragsData->pFragsLengths[i] <= INT_MAX);
        TotalDataLen    += pFragsData->pFragsLengths[i];
        if (LargestFragSize < pFragsData->pFragsLengths[i])
        {
            LargestFragSize = pFragsData->pFragsLengths[i];
        }
    }
    assert(TotalDataLen <= INT_MAX);

    int  TotalDataLenInt = (int)TotalDataLen;
    int32_t rc = ieie_internalExportDataFrags( pThreadData
                                             , file
                                             , dataType
                                             , dataId
                                             , pFragsData
                                             , LargestFragSize
                                             , TotalDataLenInt);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
                    FUNCTION_EXIT "rc %d\n", __func__, rc);
    return rc;
}

//opens an encrypted file for reading!
ieieEncryptedFileHandle_t ieie_OpenEncryptedFile( ieutThreadData_t *pThreadData
                                                , iemem_memoryType memType
                                                , const char *filePath
                                                , const char *password)
{
    ieutTRACEL(pThreadData, filePath, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "filePath(%s)\n", __func__,
               filePath);

    ieieEncryptedFile_t *hFile = NULL;
    FILE *fp = NULL;

    fp = fopen(filePath, "rb");

    if (fp == NULL)
    {
        int error = errno;

        ieutTRACEL(pThreadData, error, ENGINE_ERROR_TRACE,
                     "filePath(%s) fopen errno(%d)\n", filePath, error);
        goto mod_exit;

    }

    hFile = iemem_calloc(pThreadData,
                         IEMEM_PROBE(memType, 60502), 1,
                         sizeof(ieieEncryptedFile_t));

    if (hFile == NULL)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                "filePath(%s) ALLOC ERROR\n", filePath);
        fclose(fp);
        goto mod_exit;
    }

    ismEngine_SetStructId(hFile->StructId, IEIE_ENCRYPTEDFILE_STRUCID);
    hFile->memType = memType;
    hFile->Writing = false;
    hFile->fp = fp;

    ieieEncryptedFileHeader_t fileHeader = {{0},0,{0}};

    size_t numObjects = fread(&fileHeader, sizeof(fileHeader), 1, fp);

    if (1 != numObjects)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) couldn't read initial header\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        EVP_CIPHER_CTX_free(hFile->ctx);
        hFile = NULL;
        goto mod_exit;
    }


    //Generate the key bytes and iv from the password.
    const EVP_CIPHER *cipher = NULL;
    const EVP_MD *digest = NULL;

    cipher = EVP_get_cipherbyname("aes-256-cbc");
    if (!cipher)
    {
        ieutTRACEL(pThreadData, hFile, ENGINE_ERROR_TRACE,
                   "filePath(%s) Unable to get aes-256-cbc cipher\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    digest=EVP_get_digestbyname("sha256");
    if (!digest)
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) ALLOC ERROR\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }
    unsigned char IV[16];

    if(!EVP_BytesToKey(cipher, digest, salt,
                       (unsigned char *) password,
                       strlen(password),
                       10278,
                       hFile->key,
                       IV))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) couldn't generate key\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    if (memcmp(IV,fileHeader.IV, 16) != 0)
    {
        ieutTRACEL(pThreadData, IV[0], ENGINE_WORRYING_TRACE,
                   "IV generated and IV in  file don't match\n");
    }


    /* Create and initialise the context */
    if(!(hFile->ctx = EVP_CIPHER_CTX_new()))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) couldn't create EVP context\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

    /*  We are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_DecryptInit_ex(hFile->ctx,
                               cipher,
                               NULL,
                               hFile->key,
                               fileHeader.IV))
    {
        ieutTRACEL(pThreadData, sizeof(ieieEncryptedFileHandle_t), ENGINE_ERROR_TRACE,
                   "filePath(%s) couldn't initialise decryption\n", filePath);
        fclose(fp);
        iemem_free(pThreadData, hFile->memType, hFile);
        hFile = NULL;
        goto mod_exit;
    }

mod_exit:
    ieutTRACEL(pThreadData, hFile, ENGINE_FNC_TRACE,
                FUNCTION_EXIT "file %p\n", __func__, hFile);
    return hFile;
}

static inline size_t ieie_freeSpaceInFileBuffer( ieieEncryptedFileHandle_t file)
{
    //Avail = total - bytes we've read - bytes we haven't yet read
    size_t availSpace =   file->buffSize
                        - (file->buffCursor - file->unencryptedReadBuffer)
                        - file->buffUnreadBytes;

    return availSpace;
}
static int32_t ieie_decryptMoreBytes( ieutThreadData_t *pThreadData
                                    , ieieEncryptedFileHandle_t file
                                    , size_t minBytesRequired)
{
    int32_t rc = OK;
    const size_t extraBytes = 1024; // extra bytes must be bigger than blocksize of cipher
    size_t readSize = 10240; //Read in chunks of 10k unless we need to read more...

    if (minBytesRequired > file->buffUnreadBytes)
    {
        //If we need to read more data than our standard read size, adjust
        if( (   minBytesRequired
              + extraBytes
              - file->buffUnreadBytes)
            >= readSize)
        {
            readSize = minBytesRequired + extraBytes - file->buffUnreadBytes;
        }

        unsigned char buffer[readSize];

        if (file->finishedDecrypt)
        {
            //We've read all the file ...
            rc = ISMRC_EndOfFile;
            goto mod_exit;
        }

        //Is there plenty of room in the buffer to decrypt the data?
        if (ieie_freeSpaceInFileBuffer(file) < readSize + extraBytes)
        {
            if (file->buffCursor != file->unencryptedReadBuffer)
            {
                //There are bytes we've already read in still in the buffer
                if (file->buffUnreadBytes > 0)
                {
                    //There are bytes we need to preserve as we've not processed them yet
                    memmove(file->unencryptedReadBuffer, file->buffCursor, file->buffUnreadBytes);
                }
                file->buffCursor = file->unencryptedReadBuffer;
            }

            if (ieie_freeSpaceInFileBuffer(file) < readSize + extraBytes)
            {
                size_t newBufferSize = readSize + extraBytes + file->buffUnreadBytes;
                void *resizedData = iemem_realloc( pThreadData
                                                 , IEMEM_PROBE(file->memType, 60505)
                                                 , file->unencryptedReadBuffer
                                                 , newBufferSize);

                if (resizedData != NULL)
                {
                    file->buffCursor            = resizedData + (file->buffCursor - file->unencryptedReadBuffer);
                    file->unencryptedReadBuffer = resizedData;
                    file->buffSize              = newBufferSize;
                }
                else
                {
                    rc = ISMRC_AllocateError;
                    goto mod_exit;
                }
            }
        }

        size_t bytesRead = fread(buffer, 1, readSize, file->fp);

        if (bytesRead > 0)
        {
            int len = ieie_freeSpaceInFileBuffer(file);
            assert((len+extraBytes) >= bytesRead);

            int decrypt_rc = EVP_DecryptUpdate( file->ctx
                               , file->buffCursor + file->buffUnreadBytes
                               , &len
                               , buffer
                               , bytesRead);

            if(1 != decrypt_rc)
            {
                ieutTRACEL(pThreadData, decrypt_rc, ENGINE_ERROR_TRACE,
                                       "rc=%d\n", decrypt_rc);
                rc = ISMRC_SSLError;
                goto mod_exit;
            }
            file->buffUnreadBytes += len;
        }

        if (bytesRead < readSize)
        {
            int len = ieie_freeSpaceInFileBuffer(file);

            int decrypt_rc = EVP_DecryptFinal_ex( file->ctx
                                , file->buffCursor + file->buffUnreadBytes
                                , &len);

            if(1 != decrypt_rc)
            {
                ieutTRACEL(pThreadData, decrypt_rc, ENGINE_ERROR_TRACE,
                                       "rc=%d\n", decrypt_rc);
                rc = ISMRC_SSLError;
                goto mod_exit;
            }
            file->buffUnreadBytes += len;
            file->finishedDecrypt = true;
        }
    }
mod_exit:
    ieutTRACEL(pThreadData, file->buffUnreadBytes, ENGINE_HIFREQ_FNC_TRACE,
           FUNCTION_IDENT "Unread bytes: %lu, rc=%d\n", __func__,
           file->buffUnreadBytes, rc);
    return rc;
}

//On in, dataLen says size of dataBuffer (which this function may resize so 0 is a valid input size)
//On in, data points to a dataBuffer (or NULL if none)
//On output dataLen is the amount we read (so size of the buffer is max(dataLen for each previous call)

int32_t ieie_importData( ieutThreadData_t *pThreadData
                       , ieieEncryptedFileHandle_t file
                       , ieieDataType_t *pDataType
                       , uint64_t *pDataId
                       , size_t *dataLen
                       , void **data)
{
    ieutTRACEL(pThreadData, *dataLen, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "dataLen %lu\n", __func__,
               *dataLen);

    int32_t rc = OK;

    if (file->buffUnreadBytes < sizeof(ieieEncryptedRecordHeader_t))
    {
        rc = ieie_decryptMoreBytes(pThreadData, file, sizeof(ieieEncryptedRecordHeader_t));

        if (rc != OK)
        {
            //Couldn't decrypt more bytes...
            if (rc == ISMRC_EndOfFile)
            {
                //That's ok... as long as we don't have some trailing nonsense data around
                if (file->buffUnreadBytes != 0)
                {
                   rc = ISMRC_FileCorrupt;
                   goto mod_exit;
                }
            }
            else
            {
                goto mod_exit;
            }
        }
    }

    if (file->buffUnreadBytes >= sizeof(ieieEncryptedRecordHeader_t))
    {
        ieieEncryptedRecordHeader_t hdr;
        memcpy( &hdr, file->buffCursor, sizeof(ieieEncryptedRecordHeader_t));

        file->buffUnreadBytes -= sizeof(ieieEncryptedRecordHeader_t);
        file->buffCursor      += sizeof(ieieEncryptedRecordHeader_t);

        if (    hdr.EyeCatcher != IEIE_ENCRYPTEDRECORDHEADER_EYECATCHER
            ||  hdr.Version    != IEIE_ENCRYPTEDRECORDHEADER_CURRENT_VERSION)
        {
            ieutTRACEL(pThreadData, hdr.EyeCatcher, ENGINE_ERROR_TRACE,
                            "Unexpected record hdr in file. EyeCatcher: %d, version %d\n",
                            hdr.EyeCatcher, hdr.Version);
            rc = ISMRC_FileCorrupt;
            goto mod_exit;
        }
        rc = ieie_decryptMoreBytes(pThreadData, file, hdr.DataLength);

        if (rc != OK)
        {
            if (rc == ISMRC_EndOfFile)
            {
                //It's not ok for the file to end while the record we've already read the header for
                //isn't complete.
                ieutTRACEL(pThreadData, hdr.DataLength, ENGINE_ERROR_TRACE,
                                "Not enough bytes in file for record EyeCatcher: %d, version %d, Length %u\n",
                                hdr.EyeCatcher, hdr.Version, hdr.DataLength);
                rc = ISMRC_FileCorrupt;
            }
            //Couldn't decrypt more bytes...
            goto mod_exit;
        }

        if (file->buffUnreadBytes >= hdr.DataLength)
        {
            if (hdr.DataLength > *dataLen)
            {
                void *resizedData = iemem_realloc(pThreadData, IEMEM_PROBE(file->memType, 60504), *data, hdr.DataLength);

                if (resizedData != NULL)
                {
                    *data = resizedData;
                }
                else
                {
                    rc = ISMRC_AllocateError;
                    goto mod_exit;
                }
            }
            memcpy(*data, file->buffCursor, hdr.DataLength);
            *pDataId   = hdr.DataId;
            *pDataType = hdr.DataType;
            *dataLen   = hdr.DataLength;

            file->buffUnreadBytes -= hdr.DataLength;
            file->buffCursor      += hdr.DataLength;
        }
        else
        {
            //We know the size of the record data but there weren't enough bytes, file truncated
            ieutTRACEL(pThreadData, hdr.DataLength, ENGINE_ERROR_TRACE,
                            "Not enough bytes decrypted in file for record EyeCatcher: %d, version %d, Length %u\n",
                            hdr.EyeCatcher, hdr.Version, hdr.DataLength);
            rc = ISMRC_FileCorrupt;
            goto mod_exit;
        }

    }
    else if (file->buffUnreadBytes > 0)
    {
        //Hmmm we have trailing bytes but not enough for an extra record... corruption!
        ieutTRACEL(pThreadData,file->buffUnreadBytes , ENGINE_ERROR_TRACE,
                        "Extra %lu bytes at end of file\n",
                        file->buffUnreadBytes);
        rc = ISMRC_FileCorrupt;
        goto mod_exit;
    }
    else
    {
        //No more data, that's fine
        rc = ISMRC_EndOfFile;
    }

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE,
           FUNCTION_EXIT "rc=%d id: %lu type: %u dataLen %lu\n", __func__,
           rc, *pDataId, *pDataType, *dataLen);
    return rc;
}

void ieie_freeReadExportedData( ieutThreadData_t *pThreadData
                              , iemem_memoryType memType
                              , void *data)
{
    iemem_free(pThreadData, memType, data);
}
