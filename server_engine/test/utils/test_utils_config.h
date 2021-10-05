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
/// @file  test_utils_config.h
/// @brief Utility functions for testing config
//****************************************************************************
#ifndef __ISM_TEST_UTILS_CONFIG_H_DEFINED
#define __ISM_TEST_UTILS_CONFIG_H_DEFINED

//****************************************************************************
/// @brief  Provide properties to add to and remove from the dynamic properties
///         returned by ism_config_getPropertiesDynamic.
///
/// @param[in]     additions  ism_prop_t containing additional properties
/// @param[in]     removals   ism_prop_t whose named properties will be removed
//****************************************************************************
void test_setConfigDynamicPropsChanges(ism_prop_t *additons, ism_prop_t *removals);


//****************************************************************************
/// @brief  Make a call to ism_config_set_dynamic with an input JSON string
///
/// @param[in]     inputJSON    JSON string to be parsed and passed to config
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_configProcessPost(const char *inputJSON);

//****************************************************************************
/// @internal
///
/// @brief  Make a call to ism_config_json_processDelete for an object
///
/// @param[in]     objType      Type of object being referred to
/// @param[in]     objName      Name of object being referred to
/// @param[in]     queryParams  Parameters in key=value format
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_configProcessDelete(const char *objType, const char *objName, const char *queryParams);

#endif //end ifndef __ISM_TEST_UTILS_CONFIG_H_DEFINED
