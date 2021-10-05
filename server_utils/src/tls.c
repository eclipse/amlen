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
#ifndef TRACE_COMP
#define TRACE_COMP TLS
#endif
#ifndef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel
#endif
#include <ismutil.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <arpa/inet.h>
#ifdef OPENSSL_FIPS
#include <openssl/fips.h>
#endif

/*
 * This file contains items used for trace and problem determination of TLS connections.
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

/*
 * The table of ciphers is declared later
 */
typedef struct cipher_name_t {
    uint32_t     id;
    const char * name;
} cipher_name_t;

/*
 * Get the name of a TLS version
 */
static const char * versionName(int version) {
    const char * sVer;
    switch (version) {
    case TLS1_VERSION:
        sVer = "TLS 1.0";
        break;
    case TLS1_1_VERSION:
        sVer = "TLS 1.1";
        break;
    case TLS1_2_VERSION:
        sVer = "TLS 1.2";
        break;
    case 0x0304  :
        sVer = "TLS 1.3";
        break;
    case SSL3_VERSION:
        sVer = "SSL 3.0";
        break;
    case DTLS1_VERSION:
        sVer = "DTLS 1.0";
        break;
    case SSL2_VERSION:
        sVer = "SSL 2.0";
        break;
    case DTLS1_BAD_VER:
        sVer = "DTLS 1.0 (bad)";
        break;
    default:
        sVer = "???";
        break;
    }
    return sVer;
}

/*
 * Check if a version is supported
 */
xUNUSED static int versionSupported(int version) {
    switch (version) {
    case TLS1_VERSION:
#ifdef TLS1_1_VERSION
    case TLS1_1_VERSION:
    case TLS1_2_VERSION:
#endif
    case SSL3_VERSION:
        return 1;
    default:
        return 0;
    }
}

/*
 * Trace data types
 */
enum trcdata_type_e {
    TRDT_None         = 0,         /* Text only */
    TRDT_Data         = 1,         /* Data only */
    TRDT_ClientHello  = 2,         /* Client hello */
    TRDT_ServerHello  = 3,         /* Server hello */
    TRDT_Encrypted    = 4          /* Encrypted extensions (TLS 1.3) */
};

/*
 * Put out the characters in a data dump
 */
static int putchars(uint8_t * line, const uint8_t * data, int len) {
    int       i;
    uint8_t * cp = line;
    *cp++ = ' ';
    *cp++ = '[';
    for (i=0; i<len; i++) {
        if (data[i]<0x20 || data[i]>=0x7f)
            *cp++ = '.';
        else
            *cp++ = data[i];
    }
    *cp++ = ']';
    *cp++ = '\n';
    *cp   = 0;
    return cp-line;
}

/*
 * Format a set of bytes
 */
void formatBytes(concat_alloc_t * buf, uint8_t * data, size_t len, int chars, int linesize) {
    static const char * hexdigitx = "0123456789abcdef";
    uint8_t   line [4096];
    uint8_t * cp;
    int       pos;
    int       i;
    int       mod;

    if (linesize <= 16)
        linesize = 32;
    if (linesize > 1024)
        linesize = 1024;

    if (len <= linesize) {
        cp = line;
        mod = 0;
        for (pos=0; pos<len; pos++) {
            *cp++ = hexdigitx[(data[pos]>>4)&0x0f];
            *cp++ = hexdigitx[data[pos]&0x0f];
            if (mod++ == 3) {
                *cp++ = ' ';
                mod = 0;
            }
        }
        if (chars)
            cp += putchars(cp, data, len);
        else
            *cp++ = '\n';
        ism_common_allocBufferCopyLen(buf, (const char *)line, cp-line);
    } else {
        pos = 0;
        while (pos < len) {
            sprintf((char *)line, "%05u: ", pos);
            cp = line + strlen((char *)line);
            mod = 0;
            for (i=0; i<linesize; i++, pos++) {
                if (pos >= len) {
                    *cp++ = ' ';
                    *cp++ = ' ';
                } else {
                    *cp++ = hexdigitx[(data[pos]>>4)&0x0f];
                    *cp++ = hexdigitx[data[pos]&0x0f];
                }
                if (mod++ == 3) {
                    *cp++ = ' ';
                    mod = 0;
                }
            }
            if (chars) {
                cp += putchars(cp, data+pos-32, pos<=len ? 32 : len-pos+32);
            } else {
                *cp++ = '\n';
            }
            ism_common_allocBufferCopyLen(buf, (const char *)line, cp-line);
        }
    }
    ism_common_allocBufferCopyLen(buf, "", 1);
    buf->used--;
}

/*
 * Get the name of an extension
 */
static const char * extension_name(int ext) {
    const char * ret = NULL;
    switch (ext) {
    case 0:   ret = "server_name";               break;
    case 1:   ret = "max_fragment";              break;
    case 2:   ret = "client_cert_url";           break;
    case 3:   ret = "trusted_ca_keys";           break;
    case 4:   ret = "truncated_hmac";            break;
    case 5:   ret = "status_request";            break;
    case 6:   ret = "user_mapping";              break;
    case 7:   ret = "client_authz";              break;
    case 8:   ret = "server_authz";              break;
    case 9:   ret = "cert_type";                 break;
    case 10:  ret = "elliptic_curves";           break;
    case 11:  ret = "point_formats";             break;
    case 12:  ret = "srp";                       break;
    case 13:  ret = "signature_alg";             break;
    case 14:  ret = "use_srtp";                  break;
    case 15:  ret = "heartbeat";                 break;
    case 16:  ret = "app_protocol";              break;
    case 17:  ret = "status_request_v2";         break;
    case 18:  ret = "cert_timestamp";            break;
    case 19:  ret = "client_cert_type";          break;
    case 20:  ret = "server_cert_type";          break;
    case 21:  ret = "padding";                   break;
    case 22:  ret = "encryprt_then_mac";         break;
    case 23:  ret = "ext_secrect";               break;
    case 35:  ret = "session_ticket";            break;
    case 41:  ret = "pre_shared_key";            break;
    case 42:  ret = "early_data";                break;
    case 43:  ret = "supported_versions";        break;
    case 44:  ret = "cookie";                    break;
    case 45:  ret = "psk_key_exchange_modes";    break;
    case 47:  ret = "certificate_authorities";   break;
    case 48:  ret = "oid_filters";               break;
    case 49:  ret = "post_handshake_auth";       break;
    case 50:  ret = "signature_algorithms_cert"; break;
    case 51:  ret = "key_share";                 break;
    case 0x0a0a: ret = "interop0";    break;
    case 0x1a1a: ret = "interop1";    break;
    case 0x2a2a: ret = "interop2";    break;
    case 0x3a3a: ret = "interop3";    break;
    case 0x4a4a: ret = "interop4";    break;
    case 0x5a5a: ret = "interop5";    break;
    case 0x6a6a: ret = "interop6";    break;
    case 0x7a7a: ret = "interop7";    break;
    case 0x8a8a: ret = "interop8";    break;
    case 0x9a9a: ret = "interop9";    break;
    case 0xaaaa: ret = "interopA";    break;
    case 0xbaba: ret = "interopB";    break;
    case 0xcaca: ret = "interopC";    break;
    case 0xdada: ret = "interopD";    break;
    case 0xeaea: ret = "interopE";    break;
    case 0xfafa: ret = "interopF";    break;
    case 65281:  ret = "renegotiation_info";  break;
    }
    return ret;
}

/*
 * Get the name of an extension
 */
static int  extension_data(int ext) {
    int ret = 0;
    switch (ext) {
    case 0:    ret = 1;  /*  server_name  */             break;
    case 1:    ret = 0;  /*  max_fragment  */            break;
    case 2:    ret = 0;  /*  client_cert_url  */         break;
    case 3:    ret = 0;  /*  trusted_ca_keys  */         break;
    case 4:    ret = 0;  /*  truncated_hmac  */          break;
    case 5:    ret = 0;  /*  status_request  */          break;
    case 6:    ret = 0;  /*  user_mapping  */            break;
    case 7:    ret = 0;  /*  client_authz  */            break;
    case 8:    ret = 0;  /*  server_authz  */            break;
    case 9:    ret = 0;  /*  cert_type  */               break;
    case 10:   ret = 3;  /*  elliptic_curves  */         break;
    case 11:   ret = 0;  /*  point_formats  */           break;
    case 12:   ret = 0;  /*  srp  */                     break;
    case 13:   ret = 5;  /*  signature_alg  */           break;
    case 14:   ret = 0;  /*  use_srtp  */                break;
    case 15:   ret = 0;  /*  heartbeat  */               break;
    case 16:   ret = 0;  /*  app_protocol  */            break;
    case 17:   ret = 0;  /*  status_request_v2  */       break;
    case 18:   ret = 0;  /*  cert_timestamp  */          break;
    case 19:   ret = 0;  /*  client_cert_type  */        break;
    case 20:   ret = 0;  /*  server_cert_type  */        break;
    case 21:   ret = 4;  /*  padding  */                 break;
    case 22:   ret = 0;  /*  encryprt_then_mac  */       break;
    case 23:   ret = 0;  /*  ext_secrect  */             break;
    case 35:   ret = 0;  /*  session_ticket  */          break;
    case 41:   ret = 0;  /*  pre_shared_key */           break;
    case 42:   ret = 0;  /*  early_data */               break;
    case 43:   ret = 6;  /*  supported_versions */       break;
    case 44:   ret = 0;  /*  cookie */                   break;
    case 45:   ret = 0;  /*  psk_key_exchange_modes */   break;
    case 47:   ret = 0;  /*  certificate_authorities */ break;
    case 48:   ret = 0;  /*  oid_filters */             break;
    case 49:   ret = 0;  /*  post_handshake_auth */     break;
    case 50:   ret = 5;  /*  signature_algorithms_cert*/break;
    case 51:   ret = 0;  /*  key_share */               break;
    case 65281:   ret = 0;  /*  regotiation_info  */    break;
    }
    return ret;
}



/*
 * Return the name of a curve
 */
static const char * curve_name(int cid) {
    const char * ret = NULL;
    switch (cid) {
    case 1:  ret = "sect163k1";       break;   /*  [RFC4492]  */
    case 2:  ret = "sect163r1";       break;   /*  [RFC4492]  */
    case 3:  ret = "sect163r2";       break;   /*  [RFC4492]  */
    case 4:  ret = "sect193r1";       break;   /*  [RFC4492]  */
    case 5:  ret = "sect193r2";       break;   /*  [RFC4492]  */
    case 6:  ret = "sect233k1";       break;   /*  [RFC4492]  */
    case 7:  ret = "sect233r1";       break;   /*  [RFC4492]  */
    case 8:  ret = "sect239k1";       break;   /*  [RFC4492]  */
    case 9:  ret = "sect283k1";       break;   /*  [RFC4492]  */
    case 10: ret = "sect283r1";       break;   /*  [RFC4492]  */
    case 11: ret = "sect409k1";       break;   /*  [RFC4492]  */
    case 12: ret = "sect409r1";       break;   /*  [RFC4492]  */
    case 13: ret = "sect571k1";       break;   /*  [RFC4492]  */
    case 14: ret = "sect571r1";       break;   /*  [RFC4492]  */
    case 15: ret = "secp160k1";       break;   /*  [RFC4492]  */
    case 16: ret = "secp160r1";       break;   /*  [RFC4492]  */
    case 17: ret = "secp160r2";       break;   /*  [RFC4492]  */
    case 18: ret = "secp192k1";       break;   /*  [RFC4492]  */
    case 19: ret = "secp192r1";       break;   /*  [RFC4492]  */
    case 20: ret = "secp224k1";       break;   /*  [RFC4492]  */
    case 21: ret = "secp224r1";       break;   /*  [RFC4492]  */
    case 22: ret = "secp256k1";       break;   /*  [RFC4492]  */
    case 23: ret = "secp256r1";       break;   /*  [RFC4492]  */
    case 24: ret = "secp384r1";       break;   /*  [RFC4492]  */
    case 25: ret = "secp521r1";       break;   /*  [RFC4492]  */
    case 26: ret = "brainpoolP256r1"; break;   /*  [RFC7027]  */
    case 27: ret = "brainpoolP384r1"; break;   /*  [RFC7027]  */
    case 28: ret = "brainpoolP512r1"; break;   /*  [RFC7027]  */
    /* TLS 1.3 */
    case 29: ret = "x25519";          break;
    case 30: ret = "x448";            break;
    case 31: ret = "brainpoolP256r1"; break;   /* Brainpool 1.3 */
    case 32: ret = "brainpoolP384r1"; break;
    case 33: ret = "brainpoolP512r1"; break;
    case 34: ret = "GC256A";          break;   /* GOST */
    case 35: ret = "GC256B";          break;
    case 36: ret = "GC256C";          break;
    case 37: ret = "GC256D";          break;
    case 38: ret = "GC512A";          break;
    case 39: ret = "GC512B";          break;
    case 40: ret = "GC512C";          break;

    case 0x100: ret = "ffdhe2048";    break;
    case 0x101: ret = "ffdhe3072";    break;
    case 0x102: ret = "ffdhe2096";    break;
    case 0x103: ret = "ffhde6144";    break;
    case 0x104: ret = "ffdhe8192";    break;
    case 65281: ret = "arbitrary_prime";  break;
    case 65282: ret = "arbitrary_char2";  break;
    case 0x0a0a: ret = "interop0";    break;  /* GREASE */
    case 0x1a1a: ret = "interop1";    break;
    case 0x2a2a: ret = "interop2";    break;
    case 0x3a3a: ret = "interop3";    break;
    case 0x4a4a: ret = "interop4";    break;
    case 0x5a5a: ret = "interop5";    break;
    case 0x6a6a: ret = "interop6";    break;
    case 0x7a7a: ret = "interop7";    break;
    case 0x8a8a: ret = "interop8";    break;
    case 0x9a9a: ret = "interop9";    break;
    case 0xaaaa: ret = "interopA";    break;
    case 0xbaba: ret = "interopB";    break;
    case 0xcaca: ret = "interopC";    break;
    case 0xdada: ret = "interopD";    break;
    case 0xeaea: ret = "interopE";    break;
    case 0xfafa: ret = "interopF";    break;
    }
    return ret;
}

/*
 * Return the name of a signature
 */
static const char * sig_name(int cid) {
    const char * ret = NULL;
    switch (cid) {
    case 0x201:  ret = "rsa_pkcs1_sha1";          break;
    case 0x203:  ret = "ecdsa_sha1";              break;
    case 0x401:  ret = "rsa_pkcs1_sha256";        break;
    case 0x403:  ret = "ecdsa_secp256r1_sha256";  break;
    case 0x501:  ret = "rsa_pkcs1_sha384";        break;
    case 0x503:  ret = "ecdsa_secp256r1_sha384";  break;
    case 0x601:  ret = "rsa_pkcs1_sha512";        break;
    case 0x603:  ret = "ecdsa_secp256r1_sha512";  break;
    case 0x704:  ret = "eccsi_sha256";            break;  /* Public key with ibc */
    case 0x705:  ret = "iso_ibs1";                break;
    case 0x706:  ret = "iso_ibs2";                break;
    case 0x707:  ret = "iso_chinese_ibs";         break;
    case 0x804:  ret = "rsa_pss_rsae_sha256";     break;
    case 0x805:  ret = "rsa_pss_rsae_sha384";     break;
    case 0x806:  ret = "rsa_pss_rsae_sha512";     break;
    case 0x807:  ret = "ed25519";                 break;
    case 0x808:  ret = "ed448";                   break;
    case 0x809:  ret = "rsa_pss_pss_sha256";      break;
    case 0x80A:  ret = "rsa_pss_pss_sha384";      break;
    case 0x80B:  ret = "rsa_pss_pss_sha512";      break;
    case 0x81A:  ret = "ecdsa_brainpoolP256r1_sha256";      break;   /* Brainpool 1.3 */
    case 0x81B:  ret = "ecdsa_brainpoolP384r1_sha384";      break;
    case 0x81C:  ret = "ecdsa_brainpoolP512r1_sha512";      break;
    }
    return ret;
}


/*
 * Format a client or server hello.
 * The client and server hello are very similar, except that the server hello gives a single
 * cipher and compression which indicates which was selected.
 * The buffer already has the header info.
 *
 * openSSL has already checked that the hello is valid, so our length checks are we should not
 * ever see
 */
void formatHello(concat_alloc_t * buf, int isServer, ima_transport_info_t * transport, const void * xdata, size_t len) {
    uint8_t * data = (uint8_t *)xdata + 4;
    uint8_t * datap = data;
    int       version = 0x304;   /* The encrypted extensions are only in TLS 1.3 */
    char line[1030];

    if (isServer < 2) {
        if (len < 36)
            return;

        /*
         * Put out version
         */
        version = ((((int)data[0])<<8) | data[1]);
        sprintf(line, "legacy_version=%04x %s\n", version, versionName(version));
        ism_common_allocBufferCopy(buf, line);
        buf->used--;

        /*
         * Put out random (including 4 timestamp bytes)
         */
        ism_common_allocBufferCopy(buf, "random=");
        buf->used--;
        formatBytes(buf, data+2, 32, 0, 0);

        /*
         * Put out the sessionID.
         */
        int sessionlen = data[34];
        datap = data+35+sessionlen;
        ism_common_allocBufferCopy(buf, "sessionID=");
        buf->used--;
        formatBytes(buf, data+35, sessionlen, 1, 0);

        /*
         * Put out the cipher list
         */
        uint16_t * cipherlist = (uint16_t *)(datap);
        int        cipherlen = 1;
        if (!isServer) {
            cipherlen = ntohs(*cipherlist++) / 2;    /* Length is in bytes */
        }
        datap = (uint8_t *)(cipherlist+cipherlen);
        if (datap <= data+len) {
            int cid;
            if (cipherlen == 1) {
                cid = ntohs(*cipherlist);
                sprintf(line, "cipher=%04x %s\n", cid, ism_common_getCipherName(cid));
                ism_common_allocBufferCopy(buf, line);
                buf->used--;
            } else {
                sprintf(line, "ciphers=%u\n", cipherlen);
                ism_common_allocBufferCopy(buf, line);
                buf->used--;
                while (cipherlen) {
                    cid = ntohs(*cipherlist++);
                    sprintf(line, "    %04x %s\n", cid, ism_common_getCipherName(cid));
                    ism_common_allocBufferCopy(buf, line);
                    buf->used--;
                    cipherlen--;
                }
            }
        } else {
            ism_common_allocBufferCopy(buf, "length error\n");
            buf->used--;
        }

        /*
         * Put out compression
         */
        int complen = *datap;
        if (isServer) {
            sprintf(line, "compression=%02x\n", complen);
            ism_common_allocBufferCopy(buf, line);
            buf->used--;
            datap++;
        } else {
            strcpy(line, "compression=");
            ism_common_allocBufferCopy(buf, line);
            buf->used--;
            formatBytes(buf, datap+1, complen, 0, 0);
            datap += (1+complen);
        }
    }

    /*
     * Put out the extension
     */
    if (datap < data+len) {
        int      extdata;
        int      extcount;
        uint16_t totallen;
        uint16_t exttype;
        uint16_t extlen;
        const char * extname;
        const char * curvename;
        const char * signame;
        const char * sVer;
        memcpy((char *)&extlen, (char *)datap, 2);
        totallen = ntohs(extlen);
        datap += 2;
        if (datap + totallen <= data + len) {
            while (totallen >= 4) {
                memcpy((char *)&exttype, (char *)datap, 2);
                memcpy((char *)&extlen, (char *)(datap+2), 2);
                exttype = ntohs(exttype);
                extlen  = ntohs(extlen);
                datap += 4;
                totallen -= (extlen + 4);
                extname = extension_name(exttype);
                if (exttype == 10 && version >= 0x0304)
                    extname = "supported_groups";
                if (extname)
                    sprintf(line, "%s=", extname);
                else
                    sprintf(line, "ext_%04x", exttype);
                ism_common_allocBufferCopy(buf, line);
                buf->used--;

                extdata = extension_data(exttype);
                switch (extdata) {
                case 0:  /* Binary */
                    if (extlen > 64)
                        ism_common_allocBufferCopyLen(buf, "\n", 1);
                    formatBytes(buf, datap, extlen, 0, 64);
                    break;
                case 1:  /* With char data */
                    if (extlen > 64)
                        ism_common_allocBufferCopyLen(buf, "\n", 1);
                    formatBytes(buf, datap, extlen, 1, 64);
                    break;
                case 2:
                    if (extlen > 32)
                        ism_common_allocBufferCopyLen(buf, "\n", 1);
                    formatBytes(buf, datap, extlen, 0, 32);
                    break;
                case 3:  /* curve names */
                    extcount = extlen/2;
                    while (extcount-- > 0) {
                        memcpy((char *)&exttype, (char *)datap, 2);
                        datap += 2;
                        curvename = curve_name(ntohs(exttype));
                        if (curvename) {
                            ism_common_allocBufferCopy(buf, curvename);
                        } else {
                            sprintf(line, "curve_%04x", exttype);
                            ism_common_allocBufferCopy(buf, line);
                        }
                        buf->used--;
                        ism_common_allocBufferCopy(buf, extcount ? "," : "\n");
                        buf->used--;
                    }
                    extlen = 0;
                    break;

                case 4:    /* Length only */
                    sprintf(line, "%d\n", extlen);
                    ism_common_allocBufferCopy(buf, line);
                    buf->used--;
                    break;
                case 5:  /* signature names */
                    extcount = extlen/2;
                    while (extcount-- > 0) {
                        memcpy((char *)&exttype, (char *)datap, 2);
                        datap += 2;
                        signame = sig_name(ntohs(exttype));
                        if (signame) {
                            ism_common_allocBufferCopy(buf, signame);
                        } else {
                            sprintf(line, "signature_%04x", exttype);
                            ism_common_allocBufferCopy(buf, line);
                        }
                        buf->used--;
                        ism_common_allocBufferCopy(buf, extcount ? "," : "\n");
                        buf->used--;
                    }
                    extlen = 0;
                    break;
                case 6:  /* versions */
                    if (extlen < 2)
                        break;
                    char * verdata = (char *)datap;
                    if (extlen == 2) {      /* server_hello */
                        extcount = 1;
                    } else {                /* client_hello */
                        if ((uint8_t)*datap != extlen-1)
                            break;
                        extcount = (extlen-1)/2;
                        verdata++;
                    }

                    while (extcount-- > 0) {
                        exttype = BIGINT16(verdata);
                        verdata += 2;
                        sVer = versionName(exttype);
                        if (*sVer != '?') {
                            ism_common_allocBufferCopy(buf, sVer);
                        } else {
                            sprintf(line, "version:%04x", exttype);
                            ism_common_allocBufferCopy(buf, line);
                        }
                        buf->used--;
                        ism_common_allocBufferCopy(buf, extcount ? "," : "\n");
                        buf->used--;
                    }
                }
                datap += extlen;
            }
        } else {
            sprintf(line, "length error: extlen=%d pos=%d len=%d\n", totallen, (int)(datap-data), (int)len);
            ism_common_allocBufferCopy(buf, line);
            buf->used--;
        }
    }
}


/*
 * Debug callback from openssl
 */
void ism_common_sslProtocolDebugCallback(int direction, int version, int contentType, const void * buf, size_t len, SSL * ssl, void * arg) {
    ima_transport_info_t * transport = (ima_transport_info_t *) arg;
    const char *sVer;
    const unsigned char * msg = (const unsigned char*) buf;
    char trcbuf[8192];
    char * trcp;
    int maxDataSize;
    int trctype = TRDT_None;   /* No data */
    xUNUSED int dlen = 0;
    int  ret;

    /*
     * This method is now (openssl >= 1.0.2) called for each data record sent or received.
     * This is done with version=0 and contentType = 256.  We ignore this for now, but might
     * want to count them.
     */
    if (!arg)
        return;
    if (direction)
        transport->tlsWriteBytes += (uint32_t)len;
    else
        transport->tlsReadBytes += (uint32_t)len;

    if (!SHOULD_TRACE(9))
        return;

    if (contentType >= 256) {
#ifdef DEBUG
        if (len >= 5 && msg[0]==23) {
            contentType = msg[0];
            version = BIGINT16(msg+1);
            dlen = BIGINT16(msg+3);
        } else {
            return;
        }
#else
       return;
#endif
    }

    sVer = versionName(version);

    if (direction)
        strcpy(trcbuf, "TLS send ");
    else
        strcpy(trcbuf, "TLS receive ");
    trcp = trcbuf+strlen(trcbuf);


    switch (contentType) {
    case 20:
        strcpy(trcp, "ChangeCipherSpec ");
        break;
    case 21:    /* Alert.  This also shows up in info */
        ret = BIGINT16(msg);
        sprintf(trcp, "alert %u %s(%s) ", (uint8_t)ret, SSL_alert_type_string_long(ret),
                SSL_alert_desc_string_long(ret));
        trcp += strlen(trcp);
        break;
    case 22:
        strcpy(trcp, "Handshake");
        trctype = TRDT_Data;     /* Dump data */
        trcp += strlen(trcp);
        if (len > 0) {
            switch (((const unsigned char*) buf)[0]) {
                case 0:
                    strcpy(trcp, " (HelloRequest): ");
                    break;
                case 1:
                    strcpy(trcp, " (ClientHello): ");
                    if (len >= 36)
                        trctype = TRDT_ClientHello;   /* Format */
                    break;
                case 2:
                    strcpy(trcp, " (ServerHello): ");
                    if (len >= 26)
                        trctype = TRDT_ServerHello;   /* Format */
                    break;
                case 3:
                    strcpy(trcp, " (HelloVerifyRequest): ");
                    break;
                case 4:
                    strcpy(trcp, " (NewSessionTicket): ");
                    break;
                case 5:
                    strcpy(trcp, " (EndOfEarlyData): ");
                    break;
                case 8:
                    strcpy(trcp, " (EncryptedExtensions): ");
                    trctype = TRDT_Encrypted;
                    break;
                case 11:
                    strcpy(trcp, " (Certificate): ");
                    break;
                case 12:
                    strcpy(trcp, " (ServerKeyExchange): ");
                    break;
                case 13:
                    strcpy(trcp, " (CertificateRequest): ");
                    break;
                case 14:
                    strcpy(trcp, " (ServerHelloDone): ");
                    break;
                case 15:
                    strcpy(trcp, " (CertificateVerify): ");
                    break;
                case 16:
                    strcpy(trcp, " (ClientKeyExchange): ");
                    break;
                case 20:
                    strcpy(trcp, " (Finished): ");
                    break;
                case 21:
                    strcpy(trcp, " (CertificateURL): ");
                    break;
                case 22:
                    strcpy(trcp, " (CertificateStatus): ");
                    break;
                case 23:
                    strcpy(trcp, " (SupplementalData): ");
                    break;
                case 24:
                    strcpy(trcp, " (KeyUpdate): ");
                    break;
                case 254:
                    strcpy(trcp, " (MessageHash): ");
                    break;
                default:
                    strcpy(trcp, " (UNKNOWN): ");
                    break;
            }
        } else {
            strcpy(trcp, " (UNKNOWN): ");
        }
        break;
    case 23:
        sprintf(trcbuf+strlen(trcbuf), "AppData len=%u ", dlen);
        trctype = TRDT_None;
        break;
    case 24:
        strcat(trcbuf, "Heartbeat");
        trcp += strlen(trcp);
        trctype = TRDT_Data;
        if (len > 0) {
            switch (msg[0]) {
            case 1:
                strcat(trcbuf, " (HeartbeatRequest): ");
                break;
            case 2:
                strcat(trcbuf, " (HeartbeatResponse): ");
                break;
            default:
                strcat(trcbuf, " (UNKNOWN): ");
                break;
            }
        } else {
            strcat(trcbuf, " (UNKNOWN): ");
        }
        break;
    default:
        sprintf(trcbuf+strlen(trcbuf), "type=%u len=%u ", contentType, dlen);
        // strcat(trcbuf, "UNKNOWN ");
        break;
    }
    trcp += strlen(trcp);

    /*
     * Add in other info
     */
    snprintf(trcp, sizeof(trcbuf) - (trcp-trcbuf), "connect=%u version=%s",
            transport->index, sVer);

    /*
     * Trace it
     */
    maxDataSize = ism_common_getTraceMsgData();
    if (maxDataSize < 256)
        maxDataSize = 256;
    switch (trctype) {
    case TRDT_None:   /* Text only */
        TRACE(9, "%s\n", trcbuf);
        break;
    case TRDT_Data:   /* With data */
        traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buf, len, maxDataSize);
        break;
    default:          /* Formatted */
        /* If TraceMessageData is zero, do not format */
        if (maxDataSize == 64) {
            traceDataFunction(trcbuf, 0, __FILE__, __LINE__, buf, len, maxDataSize);
        } else {
            concat_alloc_t abuf = {trcbuf, sizeof trcbuf, strlen(trcbuf)};
            ism_common_allocBufferCopyLen(&abuf, "\n", 1);
            switch (trctype) {
            case TRDT_ClientHello:
                formatHello(&abuf, 0, transport, buf, len);
                break;
            case TRDT_ServerHello:
                formatHello(&abuf, 1, transport, buf, len);
                break;
            case TRDT_Encrypted:
                formatHello(&abuf, 2, transport, buf, len);
                break;
            }
            TRACE(9, "%s", abuf.buf);
            if (abuf.inheap)
                ism_common_freeAllocBuffer(&abuf);
        }
        break;
    }
    return;
}

static inline int getCipherId(const SSL * ssl) {
#ifdef TLS1_1_VERSION
    const SSL_CIPHER * cipher = SSL_get_current_cipher(ssl);
    return ((cipher) ? (SSL_CIPHER_get_id(cipher)&0xffff) : -1);
#else
    return 1;
#endif

}

/*
 * Trace TLS info callback
 */
void ism_common_sslInfoCallback(const struct ssl_st * ssl, int where, int ret) {
    ima_transport_info_t *transport = (ima_transport_info_t *)SSL_get_app_data(ssl);
    const char *op = "UNDEFINED";
    int w = where & ~SSL_ST_MASK;

    if (!transport)
        return;
    if (!SHOULD_TRACE(9)) {
        if (where & SSL_CB_ALERT) {
            op = (where & SSL_CB_READ) ? "receive" : "send";
            const char * alert_type = SSL_alert_type_string_long(ret);
            if (*alert_type == 'f') {    /* Lower trace level for fatal alerts */
                TRACE(5, "TLS %s alert %u %s(%s): connect=%u\n", op, (uint8_t)ret,
                    alert_type, SSL_alert_desc_string_long(ret), transport->index);
            } else {
                TRACE(7, "TLS %s alert %u %s(%s): connect=%u\n", op, (uint8_t)ret,
                    alert_type, SSL_alert_desc_string_long(ret), transport->index);
            }
            return;
        }
    }

    if (!SHOULD_TRACE(7))
        return;

    if (where & SSL_CB_HANDSHAKE_START) {
        TRACE(7, "TLS handshake started: connect=%u From=%s:%d endpoint=%s\n",
                transport->index, transport->client_addr, transport->clientport, transport->endpoint_name);
        return;
    }
    if (where & SSL_CB_HANDSHAKE_DONE) {
        int id = getCipherId(ssl);
        TRACE(7, "TLS handshake finished: connect=%u cipher=%04x %s\n",
                transport->index, id, ism_common_getCipherName(id));
        return;
    }


    if (w & SSL_ST_CONNECT) {
        op = "connect";
    } else {
        if (w & SSL_ST_ACCEPT) {
            op = "accept";
        }
    }

    if (where & SSL_CB_LOOP) {
        return;
    }
    if (where & SSL_CB_EXIT) {
        if (ret == 0) {
            TRACE(7, "TLS handshake(%s) failed in \"%s\": connect=%u\n", op, SSL_state_string_long(ssl), transport->index);
            return;
        }
    }
}

/*
 * We declare the names strings here to get the external names
 */
cipher_name_t ism_cipher_names[] = {
    {    0x0000,  "TLS_NULL_WITH_NULL_NULL"                   },
    {    0x0001,  "TLS_RSA_WITH_NULL_MD5"                     },
    {    0x0002,  "TLS_RSA_WITH_NULL_SHA"                     },
    {    0x0003,  "TLS_RSA_EXPORT_WITH_RC4_40_MD5"            },
    {    0x0004,  "TLS_RSA_WITH_RC4_128_MD5"                  },
    {    0x0005,  "TLS_RSA_WITH_RC4_128_SHA"                  },
    {    0x0006,  "TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5"        },
    {    0x0007,  "TLS_RSA_WITH_IDEA_CBC_SHA"                 },
    {    0x0008,  "TLS_RSA_EXPORT_WITH_DES40_CBC_SHA"         },
    {    0x0009,  "TLS_RSA_WITH_DES_CBC_SHA"                  },
    {    0x000A,  "TLS_RSA_WITH_3DES_EDE_CBC_SHA"             },
    {    0x000B,  "TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA"      },
    {    0x000C,  "TLS_DH_DSS_WITH_DES_CBC_SHA"               },
    {    0x000D,  "TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA"          },
    {    0x000E,  "TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA"      },
    {    0x000F,  "TLS_DH_RSA_WITH_DES_CBC_SHA"               },
    {    0x0010,  "TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA"          },
    {    0x0011,  "TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA"     },
    {    0x0012,  "TLS_DHE_DSS_WITH_DES_CBC_SHA"              },
    {    0x0013,  "TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA"         },
    {    0x0014,  "TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA"     },
    {    0x0015,  "TLS_DHE_RSA_WITH_DES_CBC_SHA"              },
    {    0x0016,  "TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA"         },
    {    0x0017,  "TLS_DH_Anon_EXPORT_WITH_RC4_40_MD5"        },
    {    0x0018,  "TLS_DH_Anon_WITH_RC4_128_MD5"              },
    {    0x0019,  "TLS_DH_Anon_EXPORT_WITH_DES40_CBC_SHA"     },
    {    0x001A,  "TLS_DH_Anon_WITH_DES_CBC_SHA"              },
    {    0x001B,  "TLS_DH_Anon_WITH_3DES_EDE_CBC_SHA"         },
    {    0x001C,  "SSL_FORTEZZA_KEA_WITH_NULL_SHA"            },
    {    0x001D,  "SSL_FORTEZZA_KEA_WITH_FORTEZZA_CBC_SHA"    },
    {    0x001E,  "TLS_KRB5_WITH_DES_CBC_SHA"                 },
    {    0x001F,  "TLS_KRB5_WITH_3DES_EDE_CBC_SHA"            },
    {    0x0020,  "TLS_KRB5_WITH_RC4_128_SHA"                 },
    {    0x0021,  "TLS_KRB5_WITH_IDEA_CBC_SHA"                },
    {    0x0022,  "TLS_KRB5_WITH_DES_CBC_MD5"                 },
    {    0x0023,  "TLS_KRB5_WITH_3DES_EDE_CBC_MD5"            },
    {    0x0024,  "TLS_KRB5_WITH_RC4_128_MD5"                 },
    {    0x0025,  "TLS_KRB5_WITH_IDEA_CBC_MD5"                },
    {    0x0026,  "TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA"       },
    {    0x0027,  "TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA"       },
    {    0x0028,  "TLS_KRB5_EXPORT_WITH_RC4_40_SHA"           },
    {    0x0029,  "TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5"       },
    {    0x002A,  "TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5"       },
    {    0x002B,  "TLS_KRB5_EXPORT_WITH_RC4_40_MD5"           },
    {    0x002C,  "TLS_PSK_WITH_NULL_SHA"                     },
    {    0x002D,  "TLS_DHE_PSK_WITH_NULL_SHA"                 },
    {    0x002E,  "TLS_RSA_PSK_WITH_NULL_SHA"                 },
    {    0x002F,  "TLS_RSA_WITH_AES_128_CBC_SHA"              },
    {    0x0030,  "TLS_DH_DSS_WITH_AES_128_CBC_SHA"           },
    {    0x0031,  "TLS_DH_RSA_WITH_AES_128_CBC_SHA"           },
    {    0x0032,  "TLS_DHE_DSS_WITH_AES_128_CBC_SHA"          },
    {    0x0033,  "TLS_DHE_RSA_WITH_AES_128_CBC_SHA"          },
    {    0x0034,  "TLS_DH_Anon_WITH_AES_128_CBC_SHA"          },
    {    0x0035,  "TLS_RSA_WITH_AES_256_CBC_SHA"              },
    {    0x0036,  "TLS_DH_DSS_WITH_AES_256_CBC_SHA"           },
    {    0x0037,  "TLS_DH_RSA_WITH_AES_256_CBC_SHA"           },
    {    0x0038,  "TLS_DHE_DSS_WITH_AES_256_CBC_SHA"          },
    {    0x0039,  "TLS_DHE_RSA_WITH_AES_256_CBC_SHA"          },
    {    0x003A,  "TLS_DH_Anon_WITH_AES_256_CBC_SHA"          },
    {    0x003B,  "TLS_RSA_WITH_NULL_SHA256"                  },
    {    0x003C,  "TLS_RSA_WITH_AES_128_CBC_SHA256"           },
    {    0x003D,  "TLS_RSA_WITH_AES_256_CBC_SHA256"           },
    {    0x003E,  "TLS_DH_DSS_WITH_AES_128_CBC_SHA256"        },
    {    0x003F,  "TLS_DH_RSA_WITH_AES_128_CBC_SHA256"        },
    {    0x0040,  "TLS_DHE_DSS_WITH_AES_128_CBC_SHA256"       },
    {    0x0041,  "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA"         },
    {    0x0042,  "TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA"      },
    {    0x0043,  "TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA"      },
    {    0x0044,  "TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA"     },
    {    0x0045,  "TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA"     },
    {    0x0046,  "TLS_DH_Anon_WITH_CAMELLIA_128_CBC_SHA"     },
    {    0x0047,  "TLS_ECDH_ECDSA_WITH_NULL_SHA"              },
    {    0x0048,  "TLS_ECDH_ECDSA_WITH_RC4_128_SHA"           },
    {    0x0049,  "TLS_ECDH_ECDSA_WITH_DES_CBC_SHA"           },
    {    0x004A,  "TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA"      },
    {    0x004B,  "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA"       },
    {    0x004C,  "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA"       },
    {    0x0060,  "TLS_RSA_EXPORT1024_WITH_RC4_56_MD5"        },
    {    0x0061,  "TLS_RSA_EXPORT1024_WITH_RC2_CBC_56_MD5"    },
    {    0x0062,  "TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA"       },
    {    0x0063,  "TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA"   },
    {    0x0064,  "TLS_RSA_EXPORT1024_WITH_RC4_56_SHA"        },
    {    0x0065,  "TLS_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA"    },
    {    0x0066,  "TLS_DHE_DSS_WITH_RC4_128_SHA"              },
    {    0x0067,  "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256"       },
    {    0x0068,  "TLS_DH_DSS_WITH_AES_256_CBC_SHA256"        },
    {    0x0069,  "TLS_DH_RSA_WITH_AES_256_CBC_SHA256"        },
    {    0x006A,  "TLS_DHE_DSS_WITH_AES_256_CBC_SHA256"       },
    {    0x006B,  "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256"       },
    {    0x006C,  "TLS_DH_Anon_WITH_AES_128_CBC_SHA256"       },
    {    0x006D,  "TLS_DH_Anon_WITH_AES_256_CBC_SHA256"       },
    {    0x0080,  "TLS_GOSTR341094_WITH_28147_CNT_IMIT"       },
    {    0x0081,  "TLS_GOSTR341001_WITH_28147_CNT_IMIT"       },
    {    0x0082,  "TLS_GOSTR341094_WITH_NULL_GOSTR3411"       },
    {    0x0083,  "TLS_GOSTR341001_WITH_NULL_GOSTR3411"       },
    {    0x0084,  "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA"         },
    {    0x0085,  "TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA"      },
    {    0x0086,  "TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA"      },
    {    0x0087,  "TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA"     },
    {    0x0088,  "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA"     },
    {    0x0089,  "TLS_DH_Anon_WITH_CAMELLIA_256_CBC_SHA"     },
    {    0x008A,  "TLS_PSK_WITH_RC4_128_SHA"                  },
    {    0x008B,  "TLS_PSK_WITH_3DES_EDE_CBC_SHA"             },
    {    0x008C,  "TLS_PSK_WITH_AES_128_CBC_SHA"              },
    {    0x008D,  "TLS_PSK_WITH_AES_256_CBC_SHA"              },
    {    0x008E,  "TLS_DHE_PSK_WITH_RC4_128_SHA"              },
    {    0x008F,  "TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA"         },
    {    0x0090,  "TLS_DHE_PSK_WITH_AES_128_CBC_SHA"          },
    {    0x0091,  "TLS_DHE_PSK_WITH_AES_256_CBC_SHA"          },
    {    0x0092,  "TLS_RSA_PSK_WITH_RC4_128_SHA"              },
    {    0x0093,  "TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA"         },
    {    0x0094,  "TLS_RSA_PSK_WITH_AES_128_CBC_SHA"          },
    {    0x0095,  "TLS_RSA_PSK_WITH_AES_256_CBC_SHA"          },
    {    0x0096,  "TLS_RSA_WITH_SEED_CBC_SHA"                 },
    {    0x0097,  "TLS_DH_DSS_WITH_SEED_CBC_SHA"              },
    {    0x0098,  "TLS_DH_RSA_WITH_SEED_CBC_SHA"              },
    {    0x0099,  "TLS_DHE_DSS_WITH_SEED_CBC_SHA"             },
    {    0x009A,  "TLS_DHE_RSA_WITH_SEED_CBC_SHA"             },
    {    0x009B,  "TLS_DH_Anon_WITH_SEED_CBC_SHA"             },
    {    0x009C,  "TLS_RSA_WITH_AES_128_GCM_SHA256"           },
    {    0x009D,  "TLS_RSA_WITH_AES_256_GCM_SHA384"           },
    {    0x009E,  "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256"       },
    {    0x009F,  "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384"       },
    {    0x00A0,  "TLS_DH_RSA_WITH_AES_128_GCM_SHA256"        },
    {    0x00A1,  "TLS_DH_RSA_WITH_AES_256_GCM_SHA384"        },
    {    0x00A2,  "TLS_DHE_DSS_WITH_AES_128_GCM_SHA256"       },
    {    0x00A3,  "TLS_DHE_DSS_WITH_AES_256_GCM_SHA384"       },
    {    0x00A4,  "TLS_DH_DSS_WITH_AES_128_GCM_SHA256"        },
    {    0x00A5,  "TLS_DH_DSS_WITH_AES_256_GCM_SHA384"        },
    {    0x00A6,  "TLS_DH_Anon_WITH_AES_128_GCM_SHA256"       },
    {    0x00A7,  "TLS_DH_Anon_WITH_AES_256_GCM_SHA384"       },
    {    0x00A8,  "TLS_PSK_WITH_AES_128_GCM_SHA256"           },
    {    0x00A9,  "TLS_PSK_WITH_AES_256_GCM_SHA384"           },
    {    0x00AA,  "TLS_DHE_PSK_WITH_AES_128_GCM_SHA256"       },
    {    0x00AB,  "TLS_DHE_PSK_WITH_AES_256_GCM_SHA384"       },
    {    0x00AC,  "TLS_RSA_PSK_WITH_AES_128_GCM_SHA256"       },
    {    0x00AD,  "TLS_RSA_PSK_WITH_AES_256_GCM_SHA384"       },
    {    0x00AE,  "TLS_PSK_WITH_AES_128_CBC_SHA256"           },
    {    0x00AF,  "TLS_PSK_WITH_AES_256_CBC_SHA384"           },
    {    0x00B0,  "TLS_PSK_WITH_NULL_SHA256"                  },
    {    0x00B1,  "TLS_PSK_WITH_NULL_SHA384"                  },
    {    0x00B2,  "TLS_DHE_PSK_WITH_AES_128_CBC_SHA256"       },
    {    0x00B3,  "TLS_DHE_PSK_WITH_AES_256_CBC_SHA384"       },
    {    0x00B4,  "TLS_DHE_PSK_WITH_NULL_SHA256"              },
    {    0x00B5,  "TLS_DHE_PSK_WITH_NULL_SHA384"              },
    {    0x00B6,  "TLS_RSA_PSK_WITH_AES_128_CBC_SHA256"       },
    {    0x00B7,  "TLS_RSA_PSK_WITH_AES_256_CBC_SHA384"       },
    {    0x00B8,  "TLS_RSA_PSK_WITH_NULL_SHA256"              },
    {    0x00B9,  "TLS_RSA_PSK_WITH_NULL_SHA384"              },
    {    0x00BA,  "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256"      },
    {    0x00BB,  "TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256"   },
    {    0x00BC,  "TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256"   },
    {    0x00BD,  "TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256"  },
    {    0x00BE,  "TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256"  },
    {    0x00BF,  "TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256"  },
    {    0x00C0,  "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256"      },
    {    0x00C1,  "TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA256"   },
    {    0x00C2,  "TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA256"   },
    {    0x00C3,  "TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256"  },
    {    0x00C4,  "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256"  },
    {    0x00C5,  "TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA256"  },
    {    0x00FF,  "TLS_EMPTY_RENEGOTIATION_INFO_SCSV"         },
    {    0x1301,  "TLS_AES_128_GCM_SHA256"                    },  /* TSLv1.3 */
    {    0x1302,  "TLS_AES_256_GCM_SHA384"                    },  /* TLSv1.3 */
    {    0x1303,  "TLS_CHACHA20_POLY1305_SHA256"              },  /* TLSv1.3 */
    {    0x1304,  "TLS_AES_128_CCM_SHA256"                    },  /* TLSv1.3 */
    {    0x1305,  "TLS_AES_128_CCM_8_SHA256"                  },  /* TLSv1.3 */
    {    0x5600,  "TLS_FALLBACK_SCSV"                         },
    {    0xC001,  "TLS_ECDH_ECDSA_WITH_NULL_SHA"              },
    {    0xC002,  "TLS_ECDH_ECDSA_WITH_RC4_128_SHA"           },
    {    0xC003,  "TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA"      },
    {    0xC004,  "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA"       },
    {    0xC005,  "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA"       },
    {    0xC006,  "TLS_ECDHE_ECDSA_WITH_NULL_SHA"             },
    {    0xC007,  "TLS_ECDHE_ECDSA_WITH_RC4_128_SHA"          },
    {    0xC008,  "TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA"     },
    {    0xC009,  "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA"      },
    {    0xC00A,  "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA"      },
    {    0xC00B,  "TLS_ECDH_RSA_WITH_NULL_SHA"                },
    {    0xC00C,  "TLS_ECDH_RSA_WITH_RC4_128_SHA"             },
    {    0xC00D,  "TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA"        },
    {    0xC00E,  "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA"         },
    {    0xC00F,  "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA"         },
    {    0xC010,  "TLS_ECDHE_RSA_WITH_NULL_SHA"               },
    {    0xC011,  "TLS_ECDHE_RSA_WITH_RC4_128_SHA"            },
    {    0xC012,  "TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA"       },
    {    0xC013,  "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA"        },
    {    0xC014,  "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA"        },
    {    0xC015,  "TLS_ECDH_Anon_WITH_NULL_SHA"               },
    {    0xC016,  "TLS_ECDH_Anon_WITH_RC4_128_SHA"            },
    {    0xC017,  "TLS_ECDH_Anon_WITH_3DES_EDE_CBC_SHA"       },
    {    0xC018,  "TLS_ECDH_Anon_WITH_AES_128_CBC_SHA"        },
    {    0xC019,  "TLS_ECDH_Anon_WITH_AES_256_CBC_SHA"        },
    {    0xC01A,  "TLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA"         },
    {    0xC01B,  "TLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA"     },
    {    0xC01C,  "TLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA"     },
    {    0xC01D,  "TLS_SRP_SHA_WITH_AES_128_CBC_SHA"          },
    {    0xC01E,  "TLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA"      },
    {    0xC01F,  "TLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA"      },
    {    0xC020,  "TLS_SRP_SHA_WITH_AES_256_CBC_SHA"          },
    {    0xC021,  "TLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA"      },
    {    0xC022,  "TLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA"      },
    {    0xC023,  "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256"   },
    {    0xC024,  "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384"   },
    {    0xC025,  "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256"    },
    {    0xC026,  "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384"    },
    {    0xC027,  "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256"     },
    {    0xC028,  "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384"     },
    {    0xC029,  "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256"      },
    {    0xC02A,  "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384"      },
    {    0xC02B,  "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256"   },
    {    0xC02C,  "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384"   },
    {    0xC02D,  "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256"    },
    {    0xC02E,  "TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384"    },
    {    0xC02F,  "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256"     },
    {    0xC030,  "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384"     },
    {    0xC031,  "TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256"      },
    {    0xC032,  "TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384"      },
    {    0xC033,  "TLS_ECDHE_PSK_WITH_RC4_128_SHA"            },
    {    0xC034,  "TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA"       },
    {    0xC035,  "TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA"        },
    {    0xC036,  "TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA"        },
    {    0xC037,  "TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256"     },
    {    0xC038,  "TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384"     },
    {    0xC039,  "TLS_ECDHE_PSK_WITH_NULL_SHA"               },
    {    0xC03A,  "TLS_ECDHE_PSK_WITH_NULL_SHA256"            },
    {    0xC03B,  "TLS_ECDHE_PSK_WITH_NULL_SHA384"            },
    {    0xC03B,  "TLS_ECDHE_PSK_WITH_NULL_SHA384"            },
    {    0xC03D,  "TLS_RSA_WITH_ARIA_256_CBC_SHA384"          },
    {    0xC03E,  "TLS_DH_DSS_WITH_ARIA_128_CBC_SHA256"       },
    {    0xC03F,  "TLS_DH_DSS_WITH_ARIA_256_CBC_SHA384"       },
    {    0xC040,  "TLS_DH_RSA_WITH_ARIA_128_CBC_SHA256"       },
    {    0xC041,  "TLS_DH_RSA_WITH_ARIA_256_CBC_SHA384"       },
    {    0xC042,  "TLS_DHE_DSS_WITH_ARIA_128_CBC_SHA256"      },
    {    0xC043,  "TLS_DHE_DSS_WITH_ARIA_256_CBC_SHA384"      },
    {    0xC044,  "TLS_DHE_RSA_WITH_ARIA_128_CBC_SHA256"      },
    {    0xC045,  "TLS_DHE_RSA_WITH_ARIA_256_CBC_SHA384"      },
    {    0xC046,  "TLS_DH_anon_WITH_ARIA_128_CBC_SHA256"      },
    {    0xC047,  "TLS_DH_anon_WITH_ARIA_256_CBC_SHA384"      },
    {    0xC048,  "TLS_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256"  },
    {    0xC049,  "TLS_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384"  },
    {    0xC04A,  "TLS_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256"   },
    {    0xC04B,  "TLS_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384"   },
    {    0xC04C,  "TLS_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256"    },
    {    0xC04D,  "TLS_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384"    },
    {    0xC04E,  "TLS_ECDH_RSA_WITH_ARIA_128_CBC_SHA256"     },
    {    0xC04F,  "TLS_ECDH_RSA_WITH_ARIA_256_CBC_SHA384"     },
    {    0xC050,  "TLS_RSA_WITH_ARIA_128_GCM_SHA256"          },
    {    0xC051,  "TLS_RSA_WITH_ARIA_256_GCM_SHA384"          },
    {    0xC052,  "TLS_DHE_RSA_WITH_ARIA_128_GCM_SHA256"      },
    {    0xC053,  "TLS_DHE_RSA_WITH_ARIA_256_GCM_SHA384"      },
    {    0xC054,  "TLS_DH_RSA_WITH_ARIA_128_GCM_SHA256"       },
    {    0xC055,  "TLS_DH_RSA_WITH_ARIA_256_GCM_SHA384"       },
    {    0xC056,  "TLS_DHE_DSS_WITH_ARIA_128_GCM_SHA256"      },
    {    0xC057,  "TLS_DHE_DSS_WITH_ARIA_256_GCM_SHA384"      },
    {    0xC058,  "TLS_DH_DSS_WITH_ARIA_128_GCM_SHA256"       },
    {    0xC059,  "TLS_DH_DSS_WITH_ARIA_256_GCM_SHA384"       },
    {    0xC05A,  "TLS_DH_anon_WITH_ARIA_128_GCM_SHA256"      },
    {    0xC05B,  "TLS_DH_anon_WITH_ARIA_256_GCM_SHA384"      },
    {    0xC05C,  "TLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256"  },
    {    0xC05D,  "TLS_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384"  },
    {    0xC05E,  "TLS_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256"   },
    {    0xC05F,  "TLS_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384"   },
    {    0xC030,  "TLS_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256"    },
    {    0xC061,  "TLS_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384"    },
    {    0xC062,  "TLS_ECDH_RSA_WITH_ARIA_128_GCM_SHA256"     },
    {    0xC063,  "TLS_ECDH_RSA_WITH_ARIA_256_GCM_SHA384"     },
    {    0xC064,  "TLS_PSK_WITH_ARIA_128_CBC_SHA256"          },
    {    0xC065,  "TLS_PSK_WITH_ARIA_256_CBC_SHA384"          },
    {    0xC066,  "TLS_DHE_PSK_WITH_ARIA_128_CBC_SHA256"      },
    {    0xC067,  "TLS_DHE_PSK_WITH_ARIA_256_CBC_SHA384"      },
    {    0xC068,  "TLS_RSA_PSK_WITH_ARIA_128_CBC_SHA256"      },
    {    0xC069,  "TLS_RSA_PSK_WITH_ARIA_256_CBC_SHA384"      },
    {    0xC06A,  "TLS_PSK_WITH_ARIA_128_GCM_SHA256"          },
    {    0xC06B,  "TLS_PSK_WITH_ARIA_256_GCM_SHA384"          },
    {    0xC06C,  "TLS_DHE_PSK_WITH_ARIA_128_GCM_SHA256"      },
    {    0xC06D,  "TLS_DHE_PSK_WITH_ARIA_256_GCM_SHA384"      },
    {    0xC06E,  "TLS_RSA_PSK_WITH_ARIA_128_GCM_SHA256"      },
    {    0xC06F,  "TLS_RSA_PSK_WITH_ARIA_256_GCM_SHA384"      },
    {    0xC070,  "TLS_ECDHE_PSK_WITH_ARIA_128_CBC_SHA256"    },
    {    0xC071,  "TLS_ECDHE_PSK_WITH_ARIA_256_CBC_SHA384"    },
    {    0xC072,  "TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256"},
    {    0xC073,  "TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384"},
    {    0xC074,  "TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256" },
    {    0xC075,  "TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384" },
    {    0xC076,  "TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256"  },
    {    0xC077,  "TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384"  },
    {    0xC078,  "TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256"   },
    {    0xC079,  "TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384"   },
    {    0xC07A,  "TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256"        },
    {    0xC07B,  "LS_RSA_WITH_CAMELLIA_256_GCM_SHA384"         },
    {    0xC07C,  "TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256"    },
    {    0xC07D,  "TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384"    },
    {    0xC07E,  "TLS_DH_RSA_WITH_CAMELLIA_128_GCM_SHA256"     },
    {    0xC07F,  "TLS_DH_RSA_WITH_CAMELLIA_256_GCM_SHA384"     },
    {    0xC080,  "TLS_DHE_DSS_WITH_CAMELLIA_128_GCM_SHA256"    },
    {    0xC081,  "TLS_DHE_DSS_WITH_CAMELLIA_256_GCM_SHA384"    },
    {    0xC082,  "TLS_DH_DSS_WITH_CAMELLIA_128_GCM_SHA256"     },
    {    0xC083,  "TLS_DH_DSS_WITH_CAMELLIA_256_GCM_SHA384"     },
    {    0xC084,  "TLS_DH_anon_WITH_CAMELLIA_128_GCM_SHA256"    },
    {    0xC085,  "TLS_DH_anon_WITH_CAMELLIA_256_GCM_SHA384"    },
    {    0xC086,  "TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256"},
    {    0xC087,  "TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384"},
    {    0xC088,  "TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256" },
    {    0xC089,  "TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384" },
    {    0xC08A,  "TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256"  },
    {    0xC08B,  "TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384"  },
    {    0xC08C,  "TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256"   },
    {    0xC08E,  "TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384"   },
    {    0xC08E,  "TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256"        },
    {    0xC08F,  "TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384"        },
    {    0xC090,  "TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256"    },
    {    0xC091,  "TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384"    },
    {    0xC092,  "TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256"    },
    {    0xC093,  "TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384"    },
    {    0xC094,  "TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256"        },
    {    0xC095,  "TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384"        },
    {    0xC096,  "TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256"    },
    {    0xC097,  "TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384"    },
    {    0xC098,  "TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256"    },
    {    0xC099,  "TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384"    },
    {    0xC09A,  "TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256"  },
    {    0xC09B,  "TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384"  },
    {    0xC09C,  "TLS_RSA_WITH_AES_128_CCM"                  },
    {    0xc09d,  "TLS_RSA_WITH_AES_256_CCM"                  },
    {    0xc09e,  "TLS_DHE_RSA_WITH_AES_128_CCM"              },
    {    0xc09f,  "TLS_DHE_RSA_WITH_AES_256_CCM"              },
    {    0xc0a0,  "TLS_RSA_WITH_AES_128_CCM_8"                },
    {    0xc0a1,  "TLS_RSA_WITH_AES_256_CCM_8"                },
    {    0xc0a2,  "TLS_DHE_RSA_WITH_AES_128_CCM_8"            },
    {    0xc0a3,  "TLS_DHE_RSA_WITH_AES_256_CCM_8"            },
    {    0xc0a4,  "TLS_PSK_WITH_AES_128_CCM"                  },
    {    0xc0a5,  "TLS_PSK_WITH_AES_256_CCM"                  },
    {    0xc0a6,  "TLS_DHE_PSK_WITH_AES_128_CCM"              },
    {    0xc0a7,  "TLS_DHE_PSK_WITH_AES_256_CCM"              },
    {    0xc0a8,  "TLS_PSK_WITH_AES_128_CCM_8"                },
    {    0xc0a9,  "TLS_PSK_WITH_AES_256_CCM_8"                },
    {    0xc0aa,  "TLS_PSK_DHE_WITH_AES_128_CCM_8"            },
    {    0xc0ab,  "TLS_PSK_DHE_WITH_AES_256_CCM_8"            },
    {    0xc0ac,  "TLS_ECDHE_ECDSA_WITH_AES_128_CCM"          },
    {    0xc0ad,  "TLS_ECDHE_ECDSA_WITH_AES_256_CCM"          },
    {    0xc0ae,  "TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8"        },
    {    0xc0af,  "TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8"        },
    {    0xc0b0,  "TLS_ECCPWD_WITH_AES_128_GCM_SHA256",       },
    {    0xc0b1,  "TLS_ECCPWD_WITH_AES_256_GCM_SHA256",       },
    {    0xc0b2,  "TLS_ECCPWD_WITH_AES_128_CCM_SHA256",       },
    {    0xc0b3,  "TLS_ECCPWD_WITH_AES_128_CCM_SHA256",       },
    {    0xc0b4,  "TLS_SHA256_SHA256",                        },   /* TLS 1.3 authentication only */
    {    0xc0b5,  "TLS_SHA384_SHA384",                        },   /* TLS 1.3 authentication only */
    {    0xc100,  "TLS_GOSTR341112_256_WITH_KUZNYECHIK_CTR_OMAC",  },
    {    0xc101,  "TLS_GOSTR341112_256_WITH_MAGMA_CTR_OMAC",       },
    {    0xc102,  "TLS_GOSTR341112_256_WITH_28147_CNT_IMIT",       },
    {    0xCC13,  "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305"      },   /* Unregistered, used by chrone */
    {    0xCC14,  "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305"    },   /* Unregistered, used by chrome */
    {    0xCCA8,  "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA56"    },
    {    0xCCA9,  "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256" },
    {    0xCCAA,  "TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256"     },
    {    0xCCAB,  "TLS_PSK_WITH_CHACHA20_POLY1305_SHA256"         },
    {    0xCCAC,  "TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256"   },
    {    0xCCAD,  "TLS_DHE_PSK_WITH_CHACHA20_POLY1305_SHA256"     },
    {    0xCCAE,  "TLS_RSA_PSK_WITH_CHACHA20_POLY1305_SHA256"     },
    {    0xD001,  "TLS_ECDHE_PSK_WITH_AES_128_GGM_SHA256"     },
    {    0xD002,  "TLS_ECDHE_PSK_WITH_AES_256_GGM_SHA384"     },
    {    0xD003,  "TLS_ECDHE_PSK_WITH_AES_128_CCM_8_SHA256"   },
    {    0xD005,  "TLS_ECDHE_PSK_WITH_AES_128_CCM_SHA256"     },
    {    0xFEFE,  "SSL_RSA_FIPS_WITH_DES_CBC_SHA"             },
    {    0xFEFF,  "SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA"        },
    {    0xFFE0,  "SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA"        },
    {    0xFFE1,  "SSL_RSA_FIPS_WITH_DES_CBC_SHA"             },
    {    0x0a0a,  "Interoperability Test 0"                   },   /* GREASE */
    {    0x1a1a,  "Interoperability Test 1"                   },
    {    0x2a2a,  "Interoperability Test 2"                   },
    {    0x3a3a,  "Interoperability Test 3"                   },
    {    0x4aba,  "Interoperability Test 4"                   },
    {    0x5a5a,  "Interoperability Test 5"                   },
    {    0x6a6a,  "Interoperability Test 6"                   },
    {    0x7a7a,  "Interoperability Test 7"                   },
    {    0x8a8a,  "Interoperability Test 8"                   },
    {    0x9a9a,  "Interoperability Test 9"                   },
    {    0xaaaa,  "Interoperability Test A"                   },
    {    0xbaba,  "Interoperability Test B"                   },
    {    0xcaca,  "Interoperability Test C"                   },
    {    0xdada,  "Interoperability Test D"                   },
    {    0xeaea,  "Interoperability Test E"                   },
    {    0xfafa,  "Interoperability Test F"                   },
    {    0,       NULL,                                       },
};


/*
 * Get the cipher name given its index.
 * The name is is the TLS specification syntax and not openSSL syntax.
 * @param id  The index of the cipher.
 * @return the naem of the cipher in spec format
 */
inline const char * ism_common_getCipherName(int id) {
    cipher_name_t * cp;
    if (id == -1) {
        return "NONE";
    } else {
        cp = ism_cipher_names;
        while (cp->name) {
            if (cp->id == (uint32_t)id) {
                return cp->name;
            }
            cp++;
        }
    }
    return "UNKNOWN";
}

/*
 * Get the cipher id given its name.
 * The name is is the TLS specification syntax and not openSSL syntax.
 * @param name  The name of the cipher in spec format
 * @return the index of the cipher or -1 to indicate no cipher or -2 to indicate unknown cipher
 */
int ism_common_getCipherId(const char * name) {
    cipher_name_t * cp;
    if (!name || !strcmpi(name, "none")) {
        return -1;
    } else {
        cp = ism_cipher_names;
        while (cp->name) {
            if (!strcmpi(cp->name, name)) {
                return cp->id;
            }
            cp++;
        }
    }
    return -2;
}



/*
 * Get the cipher id for the current transport.
 * @param transport   The transport object
 * @return the cipher ID or -1 to indicate there is no cipher
 */
int ism_common_getCipher(ima_transport_info_t * transport) {
    SSL_CIPHER * cipher = ism_common_getCipherObject(transport);
    if (!cipher) {
        return -1;
    } else {
#ifdef TLS1_1_VERSION
        return SSL_CIPHER_get_id(cipher)&0xffff;
#else
        return 1;
#endif
    }
}


/*
 * Get the cipher object for the current transport.
 * @param transport  The transport object
 * @return the cipher object or NULL to indicate there is no cipher object
 */
struct ssl_cipher_st * ism_common_getCipherObject(ima_transport_info_t * transport) {
    if (transport->ssl == NULL)
        return NULL;
    return (struct ssl_cipher_st *)SSL_get_current_cipher(transport->ssl);
}
