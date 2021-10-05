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

#ifndef F_ENDIAN_DEFINED
#define F_ENDIAN_DEFINED

#ifndef F_BIG_ENDIAN
#if (defined(AIX) || defined(ZOS)) || defined(HPUX)
    #define F_BIG_ENDIAN
#endif
#if (defined(SOLARIS) && (defined(__sparc__) || defined(__sparc)))
    #define F_BIG_ENDIAN
#endif
#endif

/*
 * Windows is always little endian
 */
#ifndef F_BIG_ENDIAN
#ifdef _WIN32
  #include <stdlib.h>
  #define F_LITTLE_ENDIAN
  #define endian_int16(x)  _byteswap_ushort((uint16_t)(x))
  #define endian_int32(x)  _byteswap_ulong((uint32_t)(x))
  #define endian_int64(x)  _byteswap_uint64((uint64_t)(x))
  __inline float  endian_float(float x)   {union{uint32_t i; float  f;}t;t.f=(x);t.i=_byteswap_ulong(t.i); return t.f;}
  __inline double endian_double(double x) {union{uint64_t i; double f;}t;t.f=(x);t.i=_byteswap_uint64(t.i); return t.f;}
#else

/*
 * SOLARIS x86
 */
#ifdef SOLARIS
  #include <arpa/inet.h>
  #define F_LITTLE_ENDIAN
  #define endian_int16(x)  ntohs(x)
  #define endian_int32(x)  ntohl(x)
  #define endian_int64(x)  (htonl((uint32_t)(x>>32)) | ((uint64_t)htonl((uint32_t)(x)))<<32)


/*
 * Linux can be either endian, so we check
 */
#else
  #include <byteswap.h>
  #if __BYTE_ORDER == __LITTLE_ENDIAN
     #define F_LITTLE_ENDIAN
     #define endian_int16(x)  bswap_16(x)
     #define endian_int32(x)  bswap_32(x)
     #define endian_int64(x)  bswap_64(x)
     __inline__ static float  endian_float(float x)   {union{uint32_t i; float  f;}t;t.f=(x);t.i=bswap_32(t.i); return t.f;}
     __inline__ static double endian_double(double x) {union{uint64_t i; double f;}t;t.f=(x);t.i=bswap_64(t.i); return t.f;}
  #else
     #define F_BIG_ENDIAN
   #endif
#endif
#endif
#endif

/*
 * For big endian, define the converters to do nothing.
 */
#ifdef F_BIG_ENDIAN
  #define endian_int16(x)  x
  #define endian_int32(x)  x
  #define endian_int64(x)  x
  #define endian_float(x)  x
  #define endian_double(x) x
#endif


#endif
