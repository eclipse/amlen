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
/// @file  engineSplitList.h
/// @brief Defines structures and functions used for split lists
//****************************************************************************
#ifndef __ISM_ENGINE_SPLIT_LIST_DEFINED
#define __ISM_ENGINE_SPLIT_LIST_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "memHandler.h"

//****************************************************************************
/// @brief The link between entries in the split list chains
/// @see ieutSplitListChain_t
//****************************************************************************
typedef struct tag_ieutSplitListLink_t
{
    struct tag_ieutSplitListLink_t *prev;  ///< Link within the previous object in the chain
    struct tag_ieutSplitListLink_t *next;  ///< Link within the next object in the chain
} ieutSplitListLink_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ieutSplitListLink_t(__file)\
    iedm_descriptionStart(__file, ieutSplitListLink_t,,"");\
    iedm_describeMember(ieutSplitListLink_t *, prev);\
    iedm_describeMember(ieutSplitListLink_t *, next);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief A single chain in a split list
/// @see ieutSplitList_t
/// @see ieutSplitListLink_t
//****************************************************************************
typedef struct tag_ieutSplitListChain_t
{
    pthread_mutex_t      lock;  ///< Lock to be used when accessing chain
    ieutSplitListLink_t *head;  ///< Pointer to link within the first object in the chain
} ieutSplitListChain_t;

//****************************************************************************
/// @brief A split list
/// @see ieutSplitListChain_t
//****************************************************************************
typedef struct tag_ieutSplitList_t
{
    size_t                 objectLinkOffset;  ///< The offset of the split list link in objects
    ieutSplitListChain_t  *chains;            ///< Array of chains
    iemem_memoryType       memoryType;        ///< Memory type to use for this list
} ieutSplitList_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ieutSplitList_t(__file)\
    iedm_descriptionStart(__file, ieutSplitList_t,,"");\
    iedm_describeMember(size_t,                  objectLinkOffset);\
    iedm_describeMember(ieutSplitListChain_t *,  chains);\
    iedm_describeMember(iemem_memoryType,        memoryType);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief Return codes from callback
/// @see ieutSplitListChain_t
//****************************************************************************
typedef enum tag_ieutSplitListCallbackAction_t
{
    ieutSPLIT_LIST_CALLBACK_CONTINUE = 0,         ///< Continue traversing the list
    ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT = 1,    ///< Remove the item from the list and continue
    ieutSPLIT_LIST_CALLBACK_STOP = 2,             ///< Stop traversing the list
} ieutSplitListCallbackAction_t;

//****************************************************************************
/// @brief Callback function used when traversing a split list
///
/// @param[in]     pThreadData  Current thread context
/// @param[in]     object       The object for which the callback is being called
/// @param[in]     locked       Whether the split list is locked or not
/// @param[in,out] context      Context supplied to the callback
///
/// @remark Note that the lock of the chain the object is on is held when this
///         call is made.
///
/// @remark The return value indicates what should happen next.
///
/// @see ieutSplitListCallbackAction
//****************************************************************************
typedef ieutSplitListCallbackAction_t (*ieutSplitList_TraverseCallback_t)(ieutThreadData_t *pThreadData,
                                                                          void *object,
                                                                          void *context);

/*********************************************************************/
/* FUNCTION PROTOTYPES                                               */
/*********************************************************************/
int32_t  ieut_createSplitList(ieutThreadData_t *pThreadData,
                              size_t objectLinkOffset,
                              iemem_memoryType memoryType,
                              ieutSplitList_t **list);
void     ieut_traverseSplitList(ieutThreadData_t *pThreadData,
                                ieutSplitList_t *list,
                                ieutSplitList_TraverseCallback_t  callback,
                                void *context);
void     ieut_destroySplitList(ieutThreadData_t *pThreadData, ieutSplitList_t *list);
void     ieut_addObjectToSplitList(ieutSplitList_t *list, void *object);
void     ieut_removeObjectFromSplitList(ieutSplitList_t *list, void *object);

#if 0
void ieut_checkSplitList(ieutSplitList_t *list);
#else
#define ieut_checkSplitList(list)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_SPLIT_LIST_DEFINED */
