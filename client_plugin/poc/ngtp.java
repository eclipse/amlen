/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
import java.io.File;
import java.io.FileInputStream;

public class ngtp {
    /*
     * Get an integer value
     */
    /*
     * Get an integer values.
     * 
     * Testing shows that having separate calls by length is much faster than
     * having a single call which handles multiple lengths.
     */ 
    static int bigInt1(byte [] data, int offset) {
        return data[offset]&0xff;
    }

    /*
     * Get a two byte value
     */
    static int bigInt2(byte [] data, int offset) {
        return ((data[offset]&0xff) << 8) + (data[offset+1&0xff]);
    }
    /*
     * Get a three byte value
     */
    static int bigInt3(byte [] data, int offset) {
        return ((data[offset]&0xff) << 16) + ((data[offset+1]&0xff) << 8) + (data[offset+2]&0xff);
    }
    
    /*
     * Get a four byte value
     */
    static int bigInt4(byte [] data, int offset) {
        return ((data[offset]&0xff) << 24) + ((data[offset+1]&0xff) << 16) + 
                ((data[offset+2]&0xff) << 8) + (data[offset+3]&0xff);
    }
    
    /*
     * Get a five byte value which is used to get a not byte aligned 4 byte value
     */
    static long bigLong5(byte [] data, int offset) {
        return (((long)data[offset]&0xff) << 32) + ((data[offset]&0xff) << 24) + 
            ((data[offset+1]&0xff) << 16) + ((data[offset+2]&0xff) << 8) + (data[offset+3]&0xff);
    }
    
    /*
     * Convert an ATP date into an ISO8601 timestamp
     * @param atptime  Time in ATP encoding
     * @param tzone    Hours east of UTC.  If an invalid value is given, no timezone is shown.
     */
    static String getTimeString(int atptime, int tzone) {
        int year   = (atptime>>26) & 0x3f;
        int month  = (atptime>>22) & 0xf;
        int day    = (atptime>>17) & 0x1f;
        int hour   = (atptime>>12) & 0x1f;
        int minute = (atptime>>6)  & 0x3f;
        int second = atptime & 0x3f;

        /* Just expand this out manually to get the leading zeros */
        char [] date = new char[25];
        int len = 19;
        date[0]  = '2';
        date[1]  = '0';
        date[2]  = (char) ('0'+ (year/10));
        date[3]  = (char) ('0' + (year%10));
        date[4]  = '-';
        date[5]  = (char) ('0' + (month/10));
        date[6]  = (char) ('0' + (month%10));
        date[7]  = '-';
        date[8]  = (char) ('0' + (day/10));
        date[9]  = (char) ('0' + (day%10));
        date[10] = ' ';
        date[11] = (char) ('0' + (hour/10));
        date[12] = (char) ('0' + (hour%10));
        date[13] = ':';
        date[14] = (char) ('0' + (minute/10));
        date[15] = (char) ('0' + (minute%10));
        date[16] = ':';
        date[17] = (char) ('0' + (second/10));
        date[18] = (char) ('0' + (second%10));
        if (tzone == 0) {
            date[19] = 'Z';
            len = 20;
        } else if (tzone > -15 && tzone < 15) {
            if (tzone > 0) {
                date[19] = '+';
            } else {
                date[19] = '-';
                tzone = 0-tzone;
            }
            date[20] = (char) ('0' + (tzone/10));
            date[21] = (char) ('0' + (tzone%10));
            date[22] = ':';
            date[23] = '0';
            date[24] = '0';
            len = 25;
        }
        return new String(date, 0, len);
    }

    /*
     * Parse a periodic data message
     */
    static void parsePeriodic(byte [] data, int offset, int tzone) {
        int  val;
        char [] v = new char [17];
        int date_collect = bigInt4(data, offset);
        int date_sent = bigInt4(data, offset+4);
        
        /* Convert the VIN.  This is a lot more work as it is not byte aligned */
        val = bigInt4(data, offset+11);
        v[0] = (char)((val>>22) & 0xff);
        v[1] = (char)((val>>14) & 0xff);
        v[2] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+14);
        v[3] = (char)((val>>22) & 0xff);
        v[4] = (char)((val>>14) & 0xff);
        v[5] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+17);
        v[6] = (char)((val>>22) & 0xff);
        v[7] = (char)((val>>14) & 0xff);
        v[8] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+20);
        v[9] = (char)((val>>22) & 0xff);
        v[10] = (char)((val>>14) & 0xff);
        v[11] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+23);
        v[12] = (char)((val>>22) & 0xff);
        v[13] = (char)((val>>14) & 0xff);
        v[14] = (char)((val>>6)  & 0xff);
        val = bigInt4(data, offset+26);
        v[15] = (char)((val>>22) & 0xff);
        v[16] = (char)((val>>14) & 0xff);
        String vin = new String(v);
        System.out.println("Periodic vin="+vin + " collected=" + getTimeString(date_collect, tzone) +
                     " sent=" + getTimeString(date_sent, tzone));
    }

    /**
     * Handle a POST request.
     *
     * @param path           The REST request path minus leading alias
     * @param data           byte array of data associated with request.
     */
    static void onHttpDataPOST(String path, byte[] data) {
        String topic = "ngtp";
        String vis1 = "";
        boolean persist = false;
        /* TODO parse and send the message */
        if (data.length > 42) {
            if (data[0] == 0x01) {
                /* parse data from NGTP header */
                vis1 = new String(data, 4, 10);
                int dsp = bigInt2(data, 16);
                int msgid = bigInt2(data, 20);
                int service= bigInt1(data, 24);
                int msg_type = bigInt1(data, 27);
                int tstamp = bigInt4(data, 30);
                String tstring = getTimeString(tstamp, 0);
                int protover = bigInt1(data, 36);
                int datalen = bigInt3(data, 39);
                // if (plugin.isTraceable(8)) {
                    System.out.println("ngtp vin="+vis1+" dsp="+dsp+" msg="+msgid+" service="+service+" msgtype="+msg_type+
                        " time="+tstring+" verion="+protover+" datalen="+datalen);
                // }
                if (msg_type == 54) {
                    parsePeriodic(data, 44, 0); 
                }
            } else if (data[0] == 'A') {
                vis1 = new String(data, 4, 10);
                String vis2 = new String(data, 14, 10);
                String imei = new String(data, 24, 15);
                String imsi = new String(data, 39, 16);
                String lang = new String(data, 55, 2);
                String mcc  = new String(data, 57, 3);
                int    tzone = bigInt1(data, 60) - 12; 
                int    comp_ver = bigInt2(data, 61);
                int    firm_ver = bigInt2(data, 63);
                int    soft_ver = bigInt2(data, 65);
                int    msgcount = bigInt2(data, 67);
                int    msgoffset = 69;
                System.out.println("atb vis1="+vis1+" vis2="+vis2+" imei="+imei+" imsi="+imsi+
                        "\n    lang="+lang+" mcc="+mcc+" timezone="+tzone+
                        "\n    comp_ver="+comp_ver+" firm_ver="+firm_ver+
                        " soft_ver="+soft_ver+" msg_count="+msgcount);
                if (msgcount > 0 && data.length >= msgoffset+4 && data[msgoffset]==0x07) {
                    parsePeriodic(data, msgoffset+4, tzone);    
                }
                /* TODO: parse data from ATP */
            } else {
                System.out.println("Unknown data");
            }
        }
    }

    private static final String hexdigits = "0123456789abcdef";

    public static void main(String [] args) {
        int i;
        int out;
        if (args.length < 1) {
            System.out.println("packet file required");
            return;
        }
        try {
        File f = new File(args[0]);
        FileInputStream fis = new FileInputStream(f);
        byte[] fdata = new byte[(int)f.length()];
        fis.read(fdata);
        fis.close();

        byte [] data = new byte[(fdata.length/2)];
        out = 0;
        for (i=0; i<fdata.length; i+=2) {
            int hex1 = hexdigits.indexOf(fdata[i]);
            int hex2 = hexdigits.indexOf(fdata[i+1]);
            data[out] = (byte)((hex1<<4) + hex2);
            out++;
        }
        onHttpDataPOST("/", data);
        } catch (Exception e) {
             e.printStackTrace(System.err);
        }
    }
}
