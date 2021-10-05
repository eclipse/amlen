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

//****************************************************************************
/// @file  policyInfoRestore.c
/// @brief Functions used during restart/recovery of MessageSight to maintain
///        policy information.
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <ctype.h>

#include "engineInternal.h"
#include "policyInfo.h"
#include "memHandler.h"
#include "engineHashTable.h"    // Hash table functions & constants

//****************************************************************************
/// @brief Information used to map old policy UUIDs (or names) to other policy
/// names
//****************************************************************************
typedef struct tag_iettPolicyNameMapping_t
{
    char *sourceString;
    char *targetString;
    struct tag_iettPolicyNameMapping_t *next;
} iettPolicyNameMapping_t;

static iettPolicyNameMapping_t *policyNameMappingHead = NULL;
static char *policyNameMappingFile = NULL;

//****************************************************************************
/// @brief Reconcile the contents of the policyInfo global structure with
///        the config component's policy definitions.
///
/// @param[in]    policyInfoGlobal  Pointer to the policyInfo global structure
///                                 which will not yet be in ismEngine_serverGlobal
///
/// @remark This function does not take locks, it is expected to be called
///         during restart / recovery and so is expecting no contention.
//****************************************************************************
int32_t iepi_createKnownPoliciesTable(ieutThreadData_t *pThreadData,
                                      iepiPolicyInfoGlobal_t *policyInfoGlobal)
{
    int32_t rc = OK;
    ism_prop_t *props = NULL;

    assert(policyInfoGlobal->knownPolicies == NULL);

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    int osrc = pthread_mutex_init(&policyInfoGlobal->knownPoliciesLock, NULL);

    if (osrc != 0)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, osrc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: failed to initialize knownPoliciesLock (osrc=%d)\n", __func__, osrc);

        goto mod_exit;
    }

    rc = ieut_createHashTable(pThreadData,
                              iepiINITIAL_POLICYINFO_CAPACITY,
                              iemem_policyInfo,
                              &policyInfoGlobal->knownPolicies);

    if (rc != OK) goto mod_exit;


    // Loop through the different policy types
    for(int32_t index=0; iepiPolicyTypeAdminInfo[index].type != ismSEC_POLICY_LAST; index++)
    {
        assert(iepiPolicyTypeAdminInfo[index].name != NULL);

        // Get the policy information using the security component's config handle
        props = ism_config_getPropertiesDynamic(ism_config_getHandle(ISM_CONFIG_COMP_SECURITY, NULL),
                                                iepiPolicyTypeAdminInfo[index].name,
                                                NULL);

        // We have policies of this type....
        if (props)
        {
            const char *propQualifier;
            const char *propFormatBase;
            const char *propertyName = NULL;

            ismSecurityPolicyType_t policyType = iepiPolicyTypeAdminInfo[index].type;

            // Loop through the policies in the set returned for this type ensuring we have an up-to-date
            // view of it.
            for (int32_t i = 0; ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++)
            {
                // We have discovered a messaging policy
                if (strncmp(propertyName,
                            iepiPolicyTypeAdminInfo[index].namePrefix,
                            strlen(iepiPolicyTypeAdminInfo[index].namePrefix)) == 0)
                {
                    propQualifier = propertyName + strlen(iepiPolicyTypeAdminInfo[index].namePrefix);
                    propFormatBase = iepiPolicyTypeAdminInfo[index].propertyFormat;
                }
                else
                {
                    propQualifier = NULL;
                }

                // We found something we're interested in, so process it
                if (propQualifier != NULL)
                {
                    assert((policyType != ismSEC_POLICY_MESSAGING) && (policyType != ismSEC_POLICY_LAST));

                    char *propertyNameFormat = iemem_malloc(pThreadData,
                                                            IEMEM_PROBE(iemem_policyInfo, 5),
                                                            strlen(propFormatBase) +
                                                            strlen(propQualifier) + 1);

                    if (propertyNameFormat == NULL)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    strcpy(propertyNameFormat, propFormatBase);
                    strcat(propertyNameFormat, propQualifier);

                    const char *name = ism_common_getStringProperty(props, propertyName);

                    iepiPolicyInfo_t *pPolicyInfo = NULL;

                    rc = iepi_createPolicyInfoFromProperties(pThreadData,
                                                             propertyNameFormat,
                                                             props,
                                                             policyType,
                                                             true,
                                                             true,
                                                             &pPolicyInfo);

                    if (rc == OK)
                    {
                        char internalPolicyName[strlen(name)+20];

                        // Create a unique identifier from the authority object type and name
                        sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT, policyType, name);

                        // We should not know this policy by this ID already
                        assert(iepi_getKnownPolicyInfo(pThreadData, internalPolicyName, policyInfoGlobal, &pPolicyInfo) == ISMRC_NotFound);

                        // Add the policy by internal name
                        rc = iepi_addKnownPolicyInfo(pThreadData, internalPolicyName, policyInfoGlobal, pPolicyInfo);

                        if (rc == OK)
                        {
                            assert(pPolicyInfo->useCount == 1); // useCount required by context

                            // Set the engine's context for this policy in the security component
                            rc = ismEngine_security_set_policyContext_func(name,
                                                                           policyType,
                                                                           ISM_CONFIG_COMP_ENGINE,
                                                                           pPolicyInfo);

                            if (rc != OK)
                            {
                                if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
                                {
                                    //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                                    ism_admin_setMaintenanceMode(rc, 0);
                                }

                                ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                                               "ismEngine_security_set_policyContext_func failed", rc,
                                               "name", name, strlen(name),
                                               "propQualifier", propQualifier, strlen(propQualifier),
                                               NULL);
                            }
                        }
                    }

                    iemem_free(pThreadData, iemem_policyInfo, propertyNameFormat);

                    if (rc != OK) goto mod_exit;
                }
            }

            ism_common_freeProperties(props);
            props = NULL;
        }
    }

mod_exit:

    // We are finished with the properties now - so release them.
    if (NULL != props) ism_common_freeProperties(props);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Create any Name mappings found in the config directory
///
/// @remark This function does not take locks, it is expected to be called
///         during restart / recovery and so is expecting no contention.
//****************************************************************************
void iepi_loadPolicyNameMappings(ieutThreadData_t *pThreadData)
{
    ieutTRACEL(pThreadData, policyNameMappingFile, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    const char *configDir = ism_common_getStringConfig("ConfigDir");

    if (configDir == NULL)
    {
        ieutTRACEL(pThreadData, configDir, ENGINE_INTERESTING_TRACE, "ConfigDir not found\n");
    }
    else
    {
        char *localPolicyMappingFilename = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,19),strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2);

        sprintf(localPolicyMappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

        FILE *mappingFile = fopen(localPolicyMappingFilename, "r");

        // There is no mapping file - so nothing further to do here.
        if (mappingFile == NULL)
        {
            ieutTRACEL(pThreadData, errno, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT
                       "No policy mapping file named '%s' found\n", __func__, localPolicyMappingFilename);
            ism_common_free(ism_memory_engine_misc,localPolicyMappingFilename);
        }
        // Read through the mapping file
        else
        {
            uint32_t mappingsRead = 0;
            char buffer[4096];

            policyNameMappingFile = localPolicyMappingFilename;

            while(fgets(buffer, sizeof(buffer), mappingFile))
            {
                iettPolicyNameMapping_t *newMapping = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,21),sizeof(iettPolicyNameMapping_t)+strlen(buffer)+1);

                if (newMapping != NULL)
                {
                    memcpy(newMapping+1, buffer, strlen(buffer)+1);
                    newMapping->sourceString = (char *)(newMapping+1);
                    size_t len = strlen(newMapping->sourceString);

                    // Remove leading white space
                    while(len != 0 && isspace(newMapping->sourceString[0]))
                    {
                        newMapping->sourceString += 1;
                        len--;
                    }

                    if (len != 0)
                    {
                        newMapping->targetString = strchr(newMapping->sourceString, ' ');
                        if (newMapping->targetString == NULL)
                        {
                            newMapping->targetString = strchr(newMapping->sourceString, '\t');
                        }

                        if (newMapping->targetString != NULL)
                        {
                            len = strlen(newMapping->targetString);

                            // Strip any leading white space from the target string
                            while(len != 0 && isspace(newMapping->targetString[0]))
                            {
                                *(newMapping->targetString) = '\0';
                                newMapping->targetString += 1;
                                len--;
                            }

                            // Strip any trailing white space from the target string
                            while(len != 0 && isspace(newMapping->targetString[len-1]))
                            {
                                newMapping->targetString[len-1] = '\0';
                                len--;
                            }

                            // Check that the target looks valid, doesn't contain white space,
                            // and doesn't match the source
                            if ((len == 0) ||
                                (strchr(newMapping->targetString, ' ') != NULL) ||
                                (strcmp(newMapping->sourceString, newMapping->targetString) == 0))
                            {
                                newMapping->targetString = NULL;
                            }
                        }
                    }
                    else
                    {
                        newMapping->targetString = NULL;
                    }

                    // We have successfully read in a pair of source and target strings
                    if (newMapping->targetString != NULL)
                    {
                        ieutTRACEL(pThreadData, newMapping, ENGINE_HIGH_TRACE, "Read policy name mapping from '%s' to '%s'\n",
                                   newMapping->sourceString, newMapping->targetString);

                        newMapping->next = policyNameMappingHead;
                        policyNameMappingHead = newMapping;
                        mappingsRead += 1;
                    }
                    else
                    {
                        ieutTRACEL(pThreadData, buffer, ENGINE_INTERESTING_TRACE,
                                   "Unexpected string '%s' read from policy name mapping file, skipping.\n", buffer);
                        ism_common_free(ism_memory_engine_misc,newMapping);
                    }
                }
            }

            fclose(mappingFile);

            ieutTRACEL(pThreadData, mappingsRead, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT
                       "Read %u mappings from '%s'\n", __func__, mappingsRead, localPolicyMappingFilename);
        }
    }

    ieutTRACEL(pThreadData, policyNameMappingHead,  ENGINE_FNC_TRACE, FUNCTION_EXIT
               "policyNameMappingHead=%p\n", __func__, policyNameMappingHead);
}

//****************************************************************************
/// @brief  Map an old messaging policy UUID or name to a policy name
///
/// @param[in]     SourceString        The UUID or name to map from
///
/// @return A policy name or NULL if none was found
//****************************************************************************
char *iepi_findPolicyNameMapping(ieutThreadData_t *pThreadData, char *sourceString)
{
    char *policyName = NULL;

    if (policyNameMappingHead != NULL)
    {
        iettPolicyNameMapping_t *curMapping = policyNameMappingHead;

        while(curMapping)
        {
            if (strcmp(sourceString, curMapping->sourceString) == 0)
            {
                policyName = curMapping->targetString;
                break;
            }
            curMapping = curMapping->next;
        }

        if (policyName != NULL)
        {
            ieutTRACEL(pThreadData, policyName, ENGINE_HIGH_TRACE, FUNCTION_IDENT "Mapping Source '%s' to Target '%s'\n",
                       __func__, sourceString, policyName);
        }
    }

    return policyName;
}

//****************************************************************************
/// @brief  Destroy the policy name mapping information
///
/// @param[in]     keepFile    Whether to keep the file or delete it
//****************************************************************************
void iepi_destroyPolicyNameMappings(ieutThreadData_t *pThreadData, bool keepFile)
{
    ieutTRACEL(pThreadData, policyNameMappingFile, ENGINE_FNC_TRACE, FUNCTION_ENTRY
               "policyNameMappingFile=%p, keepFile=%d\n", __func__, policyNameMappingFile, (int)keepFile);

    if (policyNameMappingFile != NULL)
    {
        iettPolicyNameMapping_t *curMapping = policyNameMappingHead;

        while(curMapping)
        {
            iettPolicyNameMapping_t *nextMapping = curMapping->next;
            ism_common_free(ism_memory_engine_misc,curMapping);
            curMapping = nextMapping;
        }

        policyNameMappingHead = NULL;

        if (!keepFile)
        {
            unlink(policyNameMappingFile);
        }

        ism_common_free(ism_memory_engine_misc,policyNameMappingFile);

        policyNameMappingFile = NULL;
    }
    else
    {
        assert(policyNameMappingHead == NULL);
    }

    ieutTRACEL(pThreadData, 0,  ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}
