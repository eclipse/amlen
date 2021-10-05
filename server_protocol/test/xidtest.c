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

#include <imacontent.h>
#include <fendian.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

extern int ism_protocol_getXidValue(ism_actionbuf_t * action, ism_field_t * var);

void testXid(void);

CU_TestInfo ISM_Protocol_CUnit_Xid[] = {
    {"XidTest            ", testXid },
    CU_TEST_INFO_NULL
};

/*
 * Xid protocol test
 */
void testXid(void) {
    ism_xid_t xid = {0x1234, 3, 4, "abcdefgh"};
    ism_xid_t xid2, xid3, xid4;

    ism_xid_t fxid = {0x00667764, 23, 2, "RXxy123456_tv654321_12345"};
    int  rc;
    ism_field_t field;
    concat_alloc_t buf = {0};
    char xbuf[1024];
    int  otype;

    ism_common_xidToString(&fxid, xbuf, sizeof xbuf);
    CU_ASSERT(!strcmp(xbuf, "fwd:RX:xy123456_tv654321_12345"));
    ism_common_StringToXid(xbuf, &xid2);
    CU_ASSERT(xid2.formatID = 0x667764);
    CU_ASSERT(xid2.bqual_length == 2);
    CU_ASSERT(xid2.gtrid_length == 23);
    CU_ASSERT(!memcmp(xid2.data, fxid.data, 25));

    ism_common_xidToString(&xid, xbuf, sizeof xbuf);
    CU_ASSERT(strstr(xbuf, ":61626364:") != NULL);
    CU_ASSERT(ism_common_xidFieldLen(&xid) == 13);

    rc = ism_common_fromXid(&xid, &field, (char *)&xid);
    CU_ASSERT(rc == 0);
    CU_ASSERT(field.type == VT_Xid);
    CU_ASSERT(field.val.s == (char *)&xid);
    CU_ASSERT(field.len == 13);
    CU_ASSERT(!memcmp(field.val.s+6, "abcdefg", 7));
    CU_ASSERT(*(uint32_t *)field.val.s == endian_int32(0x1234));

    CU_ASSERT(ism_common_xidFieldLen(&xid));

    rc = ism_common_toXid(&field, &xid2);
    CU_ASSERT(rc == 0);
    CU_ASSERT(xid2.formatID = 0x1234);
    CU_ASSERT(xid2.bqual_length == 4);
    CU_ASSERT(xid2.gtrid_length == 3);
    CU_ASSERT(!memcmp(xid2.data, "abcdefg", 7));

    buf.buf = xbuf;
    buf.len = sizeof(xbuf);
    ism_protocol_putXidValue(&buf, &xid2);
    CU_ASSERT(buf.used == 15);
    CU_ASSERT(buf.buf[0] == S_Xid);
    CU_ASSERT(buf.buf[1] == 13);

    memset(&field, 0, sizeof(field));
    buf.pos = 0;
    otype = buf.buf[0];
    CU_ASSERT(otype == S_Xid);
    ism_protocol_getObjectValue(&buf, &field);
    CU_ASSERT(field.type == VT_Xid);
    CU_ASSERT(field.len == 13);
    CU_ASSERT(field.val.s[4] == 3);
    CU_ASSERT(field.val.s[5] == 4);
    CU_ASSERT(field.val.s[6] == 'a');

    rc = ism_common_toXid(&field, &xid3);
    CU_ASSERT(rc == 0);
    CU_ASSERT(xid3.formatID = 0x1234);
    CU_ASSERT(xid3.bqual_length == 4);
    CU_ASSERT(xid3.gtrid_length == 3);
    CU_ASSERT(!memcmp(xid3.data, "abcdefg", 7));

    memset(buf.buf, 0, 100);
    rc = ism_common_fromXid(&xid3, &field, (char *)&xid);
    buf.used = 0;
    ism_protocol_putXidValueF(&buf, field.val.s, field.len);
    CU_ASSERT(buf.buf[6] == 3);
    CU_ASSERT(buf.buf[7] == 4);

    buf.pos = 1;
    memset(&field, 0, sizeof(field));
    rc = ism_protocol_getXidValue(&buf, &field);
    CU_ASSERT(rc == 0);
    rc = ism_common_toXid(&field, &xid4);
    CU_ASSERT(rc == 0);
    CU_ASSERT(xid4.formatID = 0x1234);
    CU_ASSERT(xid4.bqual_length == 4);
    CU_ASSERT(xid4.gtrid_length == 3);
    CU_ASSERT(!memcmp(xid4.data, "abcdefg", 7));


}
