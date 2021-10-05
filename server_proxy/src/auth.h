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

#ifndef AUTH_H
#define AUTH_H

#include <ismutil.h>
#include <pxtransport.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ism_proxy_AuthCallback_t)(int rc, void * callbackParam);

typedef struct authAction_t {
    ism_proxy_AuthCallback_t    callback;
    void *                      callbackParam;
    ism_transport_t *           transport;
    const char *                userID;
    const char *                password;
    double                      authStartTime;
    uint8_t                     which;
    uint8_t                     throttled;
    ism_user_t *                user;
} authAction_t;

XAPI int ism_proxy_doAuthenticate(ism_transport_t * transport, const char * userName, const char * pwd, ism_proxy_AuthCallback_t callback, void * callbackParam);
XAPI int ism_proxy_check_authenticator(ism_transport_t * transport, int which, const char * userID, const char * passwd,
            ism_proxy_AuthCallback_t callback, void * callbackParam);
XAPI int ism_proxyAuthenticate(ism_timer_t key, ism_time_t timestamp, void * userdata);
XAPI ism_tenant_t * ism_proxy_getValidTenant(ism_transport_t * transport);

/*
 * Match the names in the transport object.
 *
 * @param transport  The transport object
 * @return A return code, 0=no match, 1=partial match, 3=full match
 */
XAPI int ism_proxy_matchCertNames(ism_transport_t * transport);

typedef struct px_auth_stats_t {
    double               maxAuthenticationResponseTime;
    double               authenticationResponseTime;
    uint64_t             authenticationRequestsCount;
    uint64_t             authenticationThrottleCount;
} px_auth_stats_t;

XAPI int ism_proxy_getAuthStats(px_auth_stats_t * stats);

/*
 * Extended authorization types.
 * Note: These values must match values defined in
 *       /client_proxy/src/com/ibm/ima/proxy/ImaAuthConnection.java
 */
enum auth_action_e {
    AUTH_Authentication              = 0,  /*Authentication action*/
    AUTH_Authorization				 = 1,  /*Authorization action*/
    AUTH_AuthorizationAutoCreate	 = 2,  /*Device Authorization and Auto Create action*/
	AUTH_AutoCreateOnly            = 3,  /* Device Auto Create Only */
};

/*Authorization Callback*/
typedef void (*ism_proxy_AuthorizeCallback_t)(int rc, const char * reason, void * callbackParam);

/*Async Authorization Structure.*/
typedef struct asyncauth_t {
    ism_transport_t * transport;
    char *            bufbuf;
    int               bufused;
    int               kind;
    uint16_t          action;
    uint32_t          permissions;
    int16_t           count;
    int               authtokenLen;
    const char *      authtoken;
    char *            devtype;
    char *            devid;
    uint64_t          reqTime;
    int               sendSavedData;
    ism_proxy_AuthorizeCallback_t    callback;
    void *                      callbackParam;
    struct asyncauth_t * next;
} asyncauth_t;

/*Complete Authorization function*/
XAPI int ism_proxy_completeAuthorize(void * correlate, int rc, const char * reason);


/*Device Info Structure*/
typedef struct dev_info_t {
    ismHashMap *       pendingAuthRequestTable;
    uint64_t           authTime;
    int                authRC;
    int                createRC;
    char * 	           devtype;
    char *  		       devid;
    ism_time_t		   lastUseTime;		//Time that last use of this object
    pthread_spinlock_t lock;			//Object lock
} dev_info_t;



/**
 * Get the device info from global cache.
 * IMPORTANT: the device info object will be locked.
 * Need to use API ism_proxy_unlockDeviceInfo to unlock the object
 * @param subjectID - the clientID or the full GW Subject ID
 * @param org		- organization
 * @param devtype	- device type
 * @param devid		- device id
 * @return the dev_info_t object
 */
XAPI dev_info_t *  ism_proxy_getDeviceAuthInfo(const char * org, const char * devtype, const char * devid);

/**
 * This function will search and delete the device
 * @param subjectID - the clientID or the full GW Subject ID
 * @param org		- organization
 * @param devtype	- device type
 * @param devid		- device id
 * @return 0 for Success. 1 for Failure
 */
XAPI int  ism_proxy_deleteDeviceAuthInfo(const char * org, const char * devtype, const char * devid);

/**
 * Set the Device Info into cache
 * @param subjectID - the clientID or the full GW Subject ID
 * @param org		- organization
 * @param devtype	- device type
 * @param devid		- device id
 * @param outDevInfo	- the address of the devInfo
 * 					  if not NULL, the new created devInfo will
 * 					  assign to this pointer and lock.
 * @return 0 for Success, 1 for Failure.
 */
XAPI int ism_proxy_setDeviceAuthInfo(const char * org, const char * devtype, const char * devid, dev_info_t ** outDevInfo);


/**
 * Set the pending auth request into the device info object
 * Note: deviceInfo lock will be used
 * @param deviceInfo the device info object
 * @param name of the transport
 * @param async	the pending auth request object
 * @return 0 for success. Otherwise it is the failure.
 */
XAPI int ism_proxy_setDeviceAuthPendingRequest(dev_info_t * deviceInfo, asyncauth_t * async, const char * name);

/**
 * Get device pending auth request chain
 * Note: deviceInfo lock will be used
 * @param deviceInfo device info object
 * @param name of the transport
 * @return the pending auth request chain.
 * 			NULL will be returned if not any
 */
XAPI asyncauth_t * ism_proxy_getDeviceAuthPendingRequest(dev_info_t * deviceInfo, const char * name);

/**
 * Set the RC and Auth time into the device info object.
 * Pending Auth request will be returned to caller, and
 * the pointer to pend auth request will be NULL out.
 * Note: deviceINfo lock will be used.
 * @param deviceInfo	device info object
 * @param authRC		return code from authorization
 * @param createRC      return code from create device
 * @param authtime		the time that the authorization is finished
 * @return the pend auth request if any.
 */
XAPI asyncauth_t * ism_proxy_setDeviceAuthComplete(dev_info_t * deviceInfo, int authRC, int createRC, uint64_t authtime, const char *name);

/**
 * Update the Device Cache from Connector
 * @param parsobj
 * @param where
 * @param name
 */
XAPI int ism_proxy_deviceUpdate(ism_json_parse_t * parseobj, int where, const char * name) ;

/**
 * Check if the device is authorized based last RC and Authorization time
 * If the last RC is not authorized & still in the time limit, it will still
 * be not authorized
 * If the last RC is Authorized, it will return as Authorized
 * IF there are still pending Auth requests, need to continue with the
 * authorization process
 * @param deviceInfo	the device info object
 * @param currTime		the current time
 * @param name			transport name
 * @return -1 for there are still pending auth requests.
 * 			0 for Authorized
 * 			180 for Not Authorized
 */
XAPI int ism_proxy_checkDeviceAuth(dev_info_t * deviceInfo, uint64_t currTime, const char *name) ;

/**
 * Unlock the DeviceInfo Object
 * @param deviceInfo the device info object to be unlocked
 */
XAPI void ism_proxy_unlockDeviceInfo(dev_info_t * deviceInfo);

/**
 * Get ACL key
 * @param transport the transport object
 */
const char * ism_proxy_getACLKey(ism_transport_t * transport);


#ifdef __cplusplus
}
#endif

#endif /* AUTH_H */
