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

#include <jni.h>


#ifndef JAVACONFIG_H
#define JAVACONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Constants
 */
#define ImaProxyImpl_config 1
#define ImaProxyImpl_endpoint 2
#define ImaProxyImpl_tenant 3
#define ImaProxyImpl_server 4
#define ImaProxyImpl_user 5
#define ImaProxyImpl_cert 6
#define ImaProxyImpl_item 7
#define ImaProxyImpl_acl  8
#define ImaProxyImpl_device  9
#define ImaProxyImpl_list 0x10

#define PRC_Good             0
#define PRC_JsonIncomplete   1
#define PRC_JsonError        2
#define PRC_MissingRequired  3
#define PRC_NotFound         4
#define PRC_ItemNotFound     5
#define PRC_InUse            6
#define PRC_BadPath          7
#define PRC_BadPropValue     8
#define PRC_Unknown          9
#define PRC_Exception       10


/*
 * Set a JSON config.
 *
 * Method:    setJSONn
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_setJSONn
  (JNIEnv * env, jobject thisobj, jint otype, jstring name, jstring name2, jstring json);

/*
 * Set a binary config.
 *
 * Method:    setBinary
 * Signature: (ILjava/lang/String;Ljava/lang/String;[B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_setBinary
  (JNIEnv * env, jobject thisobj, jint otype, jstring name, jstring name2, jbyteArray);

/*
 * Get a JSON config.
 *
 * Method:    getJSONn
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getJSONn
  (JNIEnv * env, jobject thisobj, jint otype, jstring name, jstring name2);

/*
 * Get a binary config.
 * Method:    getCert
 * Signature: (ILjava/lang/String;Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getCert
  (JNIEnv * env, jobject thisobj, jint otype, jstring name, jstring name2);

/*
 * Delete an object.
 *
 * Method:    deleteObj
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_deleteObj
  (JNIEnv * env, jobject thisobj, jint otype, jstring name, jstring name2, jboolean force);

/*
 * Get statistics.
 *
 * Method:    getStats
 * Signature: (ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getStats
  (JNIEnv * env, jobject thisobj, jint otype, jstring name);

/*
 * Get an obfuscated password.
 *
 * Method:    getObfus
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_getObfus
  (JNIEnv * env, jobject thisobj, jstring user, jstring password, jint type);

/*
 * Start messaging.
 *
 * Method:    startMsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_startMsg
  (JNIEnv * env, jobject thisobj);

/*
 * Terminate the proxy.
 *
 * Method:    quitProxy
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_quitProxy
  (JNIEnv * env, jobject thisobj, jint);

/*
 * Do a disconnect.
 *
 * Method:    doDisconnect
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doDisconnect
  (JNIEnv * env, jobject thisobj, jstring endpoint, jstring server, jstring tenant, jstring user, jstring clientid, jint operationbits);

/*
 * Write an entry in the proxy log.
 *
 * Method:    doLog
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doLog
  (JNIEnv * env, jobject thisobj, jstring msgid, jstring msgformat, jstring fileno, jint lineno, jstring repl);

/**
 * Set the result for the check of device authorization
 *
 * Method: doAuthorized
 * Signature: (JILjava/lang/String;)
 */
JNIEXPORT void JNICALL Java_com_ibm_ima_proxy_impl_ImaProxyImpl_doAuthorized
  (JNIEnv * env, jobject thisobj, jlong correlation, jint rc, jstring reason);

#ifdef __cplusplus
}
#endif
#endif
