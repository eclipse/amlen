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

//****************************************************************************
/// @file  topicTreeUtils.c
/// @brief Utility functions for the topic tree
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

//****************************************************************************
/// @brief Resizes the requested array of pointers to character strings which
///        is expected to have initial size of initialElementCount elements.
///
/// @param[in,out] existingArray        The array to be resized
/// @param[in]     elementCount         The current count of array elements
/// @param[in]     initialElementCount  The initial array size
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Expected to be called by iett_analyseTopicString with an array that is
///         initially on the <em>stack</em> and is initialElementCount size.
///
///         We use this assumption to decide whether, after allocating a bigger
///         array, we should free the previous array - if the elementCount is
///         exactly the initialElementCount, we choose not to free, otherwise
///         we free.
///
/// @see iett_analyseTopicString
//****************************************************************************
int32_t iett_resizeStringArray(ieutThreadData_t *pThreadData,
                               const char **existingArray[],
                               const int elementCount,
                               const int initialElementCount)
{
    int32_t      rc = OK;
    const char **newStringArray;

    newStringArray = iemem_malloc(pThreadData,
                                  IEMEM_PROBE(iemem_topicAnalysis, 1),
                                  (elementCount+initialElementCount)*sizeof(const char **));

    if (NULL == newStringArray)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(newStringArray, *existingArray, elementCount*sizeof(const char **));

    if (elementCount != initialElementCount)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, *existingArray);
    }

    *existingArray = newStringArray;

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Resizes the requested array of pointers to uint32_t values which
///        is expected to have initial size of initialElementCount elements.
///
/// @param[in,out] existingArray        The array to be resized
/// @param[in]     elementCount         The current count of array elements
/// @param[in]     initialElementCount  The initial array size
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Expected to be called by iett_analyseTopicString with an array that is
///         initially on the <em>stack</em> and is initialElementCount size.
///
///         We use this assumption to decide whether, after allocating a bigger
///         array, we should free the previous array - if the elementCount is
///         exactly the initialElementCount, we choose not to free, otherwise
///         we free.
///
/// @see iett_analyseTopicString
//****************************************************************************
int32_t iett_resizeUint32Array(ieutThreadData_t *pThreadData,
                               uint32_t *existingArray[],
                               const int elementCount,
                               const int initialElementCount)
{
    int32_t      rc = OK;
    uint32_t    *newArray;

    newArray = iemem_malloc(pThreadData,
                            IEMEM_PROBE(iemem_topicAnalysis, 2),
                            (elementCount+initialElementCount)*sizeof(uint32_t));

    if (NULL == newArray)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(newArray, *existingArray, elementCount*sizeof(uint32_t));

    if (elementCount != initialElementCount)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, *existingArray);
    }

    *existingArray = newArray;

mod_exit:

    return rc;
}

// Hash algorithm used when calculating a substring hash
#define iettINCREMENT_SUBSTRING_HASH(_hash, _char) \
    (_hash) = (_char) + ((_hash) << 6) + ((_hash) << 16) - (_hash)

//****************************************************************************
/// @brief Analyses the contents of a topic string to determine total length,
///        the positions and count of substrings, the positions and counts of
///        wildcards (single and multi-level), hashes for each substring and a
///        hash for the entire topic string.
///
/// @param[in,out] topic  pre-filled topic object see remarks for more details.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark On entry the topicString, substrings and substringsHashes fields
///         of topic must be filled in. In addition, if wildcard and multicard
///         positions are to be identified these then the wildcards and multicards
///         fields should be initialized.
///
///         Each of the arrays specified should be pointers to arrays on the stack
///         of size topic->initialArraySize, e.g. 'arrayRef' below:
///         <pre>
///         int    initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;
///         char  *arrayName[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
///         char **arrayRef = arrayName;</pre>
///         topic->initialArraySize is assumed to be <b>at least 2</b>.
///         If possible, the results are contained within the arrays on the
///         stack. If this is not possible, storage is allocated and the
///         address passed back.
///
/// @remark Callers <b>must</b> check whether the values returned for
///         array pointers have changed, and if so, free the storage once the
///         data is no longer needed.
//****************************************************************************
int32_t iett_analyseTopicString(ieutThreadData_t *pThreadData,
                                iettTopic_t *topic)
{
    int32_t   rc = OK;

    uint32_t  curSubstringHash = 0;
    int32_t   curSubstringCount = 0;
    int32_t   curWildcardCount = 0;
    int32_t   curMulticardCount = 0;

    ieutTRACEL(pThreadData, topic->topicString,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topic->topicString);

    if (topic->topicStringCopy != NULL) goto mod_exit;

    assert(topic->destinationType == ismDESTINATION_TOPIC || topic->destinationType == ismDESTINATION_REGEX_TOPIC);

    topic->topicStringLength = strlen(topic->topicString);

    topic->topicStringCopy = iemem_malloc(pThreadData,
                                          IEMEM_PROBE(iemem_topicAnalysis, 3),
                                          topic->topicStringLength+1);

    if (topic->topicStringCopy == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(topic->topicStringCopy, topic->topicString, topic->topicStringLength+1);

    // For a regex topic string, we simply call the utility function to compile it
    if (topic->destinationType == ismDESTINATION_REGEX_TOPIC)
    {
        int regex_rc = ism_regex_compile(&topic->regex, topic->topicString);

        if (regex_rc != OK)
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(topic->regex != NULL);
    }
    // Otherwise we perform analysis on the topic string
    else
    {
        char     *curPos = topic->topicStringCopy;
        char     *prevPos = curPos;

        // Loop through the topic string
        do
        {
            char curChar = *curPos;

            // The end of the current substring, update the arrays and counts - if
            // this is the end of the string, finalize them and break.
            if (curChar == '/' || curChar == '\0')
            {
                // If this is a single character wildcard substring, increment the count, and add
                // the position to the appropriate wildcard array.
                if (curPos == prevPos+1)
                {
                    if (*prevPos == '+')
                    {
                        if (NULL != topic->wildcards)
                        {
                            topic->wildcards[curWildcardCount] = prevPos;

                            if (++curWildcardCount % topic->initialArraySize == 0)
                            {
                                rc = iett_resizeStringArray(pThreadData,
                                                            &(topic->wildcards),
                                                            curWildcardCount,
                                                            topic->initialArraySize);
                            }
                        }
                        else // Always count the wildcards
                        {
                            curWildcardCount++;
                        }
                    }
                    else if (*prevPos == '#')
                    {
                        if (NULL != topic->multicards)
                        {
                            topic->multicards[curMulticardCount] = prevPos;

                            if (++curMulticardCount % topic->initialArraySize == 0)
                            {
                                rc = iett_resizeStringArray(pThreadData,
                                                            &(topic->multicards),
                                                            curMulticardCount,
                                                            topic->initialArraySize);
                            }
                        }
                        else // Always count multicards
                        {
                            curMulticardCount++;
                        }
                    }

                    if (rc != OK) break;
                }

                topic->substrings[curSubstringCount] = prevPos;
                topic->substringHashes[curSubstringCount] = (uint32_t)curSubstringHash;

                // Resize the substring and substringHashes arrays if needed.
                if (++curSubstringCount % topic->initialArraySize == 0)
                {
                    rc = iett_resizeStringArray(pThreadData,
                                                &(topic->substrings),
                                                curSubstringCount,
                                                topic->initialArraySize);

                    if (rc == OK)
                    {
                        rc = iett_resizeUint32Array(pThreadData,
                                                    &(topic->substringHashes),
                                                    curSubstringCount,
                                                    topic->initialArraySize);
                    }

                    if (rc != OK) break;
                }

                *curPos++ = '\0';

                // If the start of the topic string is the system topic prefix, note the
                // fact by setting sysTopicEndIndex to the index after it - this gives us
                // a mechanism to require topic strings to match up-to and including the
                // system prefix.
                if (curSubstringCount == 1)
                {
                    if (iettTOPIC_IS_SYSTOPIC(prevPos))
                    {
                        topic->sysTopicEndIndex = curSubstringCount;
                    }
                    else
                    {
                        topic->sysTopicEndIndex = 0;
                    }
                }

                // End of the string, update the length and counts, and add sentinels
                // to each of the arrays.
                if (curChar == '\0')
                {
                    topic->substringCount = curSubstringCount;
                    topic->substrings[curSubstringCount] = NULL;

                    if (NULL != topic->wildcards) topic->wildcards[curWildcardCount] = NULL;
                    if (NULL != topic->multicards) topic->multicards[curMulticardCount] = NULL;

                    topic->wildcardCount = curWildcardCount;
                    topic->multicardCount = curMulticardCount;

                    // Check whether the substring count is > our max
                    if (topic->substringCount > iettMAX_TOPIC_DEPTH)
                    {
                        rc = ISMRC_DestNotValid;
                        ism_common_setErrorData(rc, "%s", topic->topicString);
                    }

                    break; // leave the while loop
                }
                // New substring
                else
                {
                    prevPos = curPos;
                    curSubstringHash = 0;
                }
            }
            // Any character other than '/' or null-terminator.
            else
            {
                iettINCREMENT_SUBSTRING_HASH(curSubstringHash, curChar);
                curPos++;
            }
        }
        while(1);
    }

mod_exit:

    ieutTRACEL(pThreadData, topic->substringCount, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d substrings=%p substringCount=%d\n", __func__,
               rc, topic->substrings, topic->substringCount);

    return rc;
}

//****************************************************************************
/// @brief Generate a hash value for a specified substring
///
/// @param[in]     key  The substring for which to generate a hash value
///
/// @remark This hash should match that calculated in iett_analyseTopicString
///         for a substring, so we use a common macro (iettINCREMENT_SUBSTRING_HASH)
///         in both functions.
///
/// @return The hash value
///
/// @see iett_analyseTopicString
//****************************************************************************
uint32_t iett_generateSubstringHash(const char *key)
{
    uint32_t substringHash = 0;
    char curChar;

    while((curChar = *key++))
    {
        if (curChar == '/') break;
        iettINCREMENT_SUBSTRING_HASH(substringHash, curChar);
    }

    return (uint32_t)substringHash;
}

//****************************************************************************
/// @brief Generate a hash value for a specified topic string for use in the
///        topic tree's sublist caches.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateTopicStringHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}

//****************************************************************************
/// @brief Generate a hash value for a specified subscription name for use in
///        the topic tree.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateSubNameHash(const char *subName)
{
    return iett_generateTopicStringHash(subName);
}

//****************************************************************************
/// @brief Generate a hash value for a specified client Id for use in the
///        topic tree.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateClientIdHash(const char *clientId)
{
    return iett_generateTopicStringHash(clientId);
}

//****************************************************************************
/// @brief Generate a hash value for a specified server UID for use in the
///        origin server hash.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateOriginServerHash(const char *serverUID)
{
    return iett_generateTopicStringHash(serverUID);
}

//****************************************************************************
/// @brief Check whether the specified topic string is valid for either publish
///        or subscribe.
///
/// @param[in]     topicString     Null-terminated topic string to check.
/// @param[in]     validationType  Whether validating for publish, subscribe or
///                                for a topic monitor.
///
/// @remark The string is considered valid for publish unless it contains a
///         wildcard/multicard character, or has more than iettMAX_TOPIC_DEPTH
///         substrings.
///
///         The string is considered valid for a topic monitor if it contains
///         one and only one multicard, at the end of the string.
///
///         The string is considered valid for subscribe unless it has more than
///         iettMAX_TOPIC_DEPTH substrings.
///
/// @remark Note this is an engine specific routine which performs only the
///         validation required by the engine (assuming that maximum string
///         length and UTF-8 correctness have been carried out elsewhere)
///
/// @return true if the string is considered valid by the engine, otherwise false.
//****************************************************************************
bool iett_validateTopicString(ieutThreadData_t *pThreadData,
                              const char *topicString,
                              const iettValidationType_t validationType)
{
    bool retVal = true;
    int32_t curSubstringCount = 0;
    const char *curPos = topicString;
    const char *prevPos = curPos;

    do
    {
        char curChar = *curPos;

        if (curChar == '/' || curChar == '\0')
        {
            if ((curPos == prevPos+1) && (validationType != iettVALIDATE_FOR_SUBSCRIBE))
            {
                if (*prevPos == '+')
                {
                    ieutTRACEL(pThreadData, validationType, ENGINE_HIGH_TRACE, "Validation type %d, topic contains unexpected wildcard\n",
                               validationType);
                    retVal = false;
                    break;
                }
                else if (*prevPos == '#')
                {
                    if (validationType == iettVALIDATE_FOR_PUBLISH || (curChar != '\0'))
                    {
                        ieutTRACEL(pThreadData, validationType, ENGINE_HIGH_TRACE, "Validation type %d, topic contains unexpected multicard\n",
                                   validationType);
                        retVal = false;
                        break;
                    }
                }
            }

            curSubstringCount++;

            if (curChar == '\0')
            {
                if (curSubstringCount > iettMAX_TOPIC_DEPTH)
                {
                    ieutTRACEL(pThreadData, validationType, ENGINE_HIGH_TRACE, "Validation type %d, topic substring count (%d) exceeds max (%d)\n",
                               validationType, curSubstringCount, iettMAX_TOPIC_DEPTH);
                    retVal = false;
                }
                // Make sure a topic monitor ends in a multicard
                else if (validationType == iettVALIDATE_FOR_TOPICMONITOR)
                {
                    if ((curPos != prevPos+1) || *prevPos != '#')
                    {
                        ieutTRACEL(pThreadData, validationType, ENGINE_HIGH_TRACE, "Validation type %d, topic does not end with a multicard\n",
                                   validationType);
                        retVal = false;
                    }
                }
                break;
            }
            else
            {
                prevPos = ++curPos;
            }
        }
        else
        {
            curPos++;
        }
    }
    while(1);

    return retVal;
}
