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


#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#include <openssl/rand.h>

#define FNV_OFFSET_BASIS_32 0x811C9DC5
#define FNV_PRIME_32 0x1000193

/*
 * Return the value of a hex digit or -1 if not a valid hex digit
 */
static int hexValue(char ch) {
    if (ch <= '9' && ch >= '0')
        return ch-'0';
    if (ch >='A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >='a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}
static const char * hexdigit = "0123456789abcdef";

/*
 * Convert a byte array string to a hex string.
 * The resulting string is null terminated.
 * The invoker is required to give a large enough buffer for the output.
 * @return  The length of the output
 */
static int toHexString(const char * from, char * to, int fromlen) {
    for (i=0; i<fromlen; i++) {
        *to++ = hexdigit[((*from)>>4)&0x0f];
        *to++ = hexdigit[(*from)&0x0f];
        from++;
    }
    *to = 0;
    return fromlen*2;
}

/*
 * Convert a hex string to a binary array.
 * The invoker is requred to give a large enough buffer for the output.
 * @return The length of the output or -1 to indicate an error
 */
static int fromHexString(const char * from, char * to) {
    int  ret;
    while (*from) {
        int val1 = hexValue(*from++);
        if (val1 >= 0) {
            int val2 = hexValue(*from++);
            if (val2 < 0) {
                return -1; /* Invalid second digit */
            }
            if (to)
                *to++ = (val1<<4) + val2;
            ret++;
        } else {
            return -1;     /* Invalid first digit */
        }
    }
    return ret;
}

/*
 * Get the current time in milliseconds
 */
static uint64_t getMillis(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec/1000);
}

static uint32_t hash_string(const char * in) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    uint8_t * in8 = (uint8_t *) in;
    uint8_t b;
    do {
        b = *(in8++);
        hash ^= b;
        hash *= FNV_PRIME_32;
    } while (b);
    return hash;
}

static int createPSK(FILE * f, int count, int size) {
    char * randbuf = alloca(size);
    char * hexbuf = alloca(size*4);
    struct timeval tv;
    int    counter[1000] = {0};
    int    i;
    int    kind;
    int    which;
    uint64_t before;
    uint64_t after;

    before = getMillis();
    for (i=0; i<count; i++) {
        gettimeofday(&tv, NULL);
        kind = (tv.tv_usec/1000) % 1000;
        which = ++counter[kind];
        RAND_bytes(randbuf, size);
        toHexString(randbuf, hexbuf, size);
        fprintf(f, "sec%03d-%05d,%s\n", kind, which, hexbuf);
    }
    fflush(f);
    after = getMillis();
    printf("create PSK count=%d size=%d time=%lu\n", count, size, after-before);
    return 0;
}

typedef struct slot_t {
    struct slot_t * next;
    uint16_t        idlen;
    uint16_t        keylen;
    uint32_t        hash;
} slot_t;

slot_t * * slots;

static int readPSK(FILE * f, uint32_t count) {
    char  line[4096];
    char  key[4096];
    uint64_t before;
    uint64_t after;
    int      collide = 0;
    int      entries = 0;


    before = getMillis();
    slots = calloc(count, sizeof(slot_t *));

    uint64_t totalmem = count * sizeof(slot_t *);

    while (fgets(line, sizeof(line) - 1, f)) {
        char *   eol;
        int      keylen;
        int      idlen;
        uint32_t which;
        uint32_t hash;
        slot_t * slot;
        char *   slotc;
        char * pos = strchr(line, ',');
        if (pos && pos != line) {
            entries++;
            *pos++ = 0;
            eol = pos + strlen(pos);
            while (eol[-1]=='\n' || eol[-1]=='\r')
                eol--;
            *eol = 0;
            idlen = (int)strlen(line);
            hash = hash_string(line);
            keylen = fromHexString(pos, key);
            if (keylen < 0) {
                printf("Bad hex data [%s] [%s]\n", line, pos);
                continue;
            }
            slot = calloc(1, sizeof(slot_t) + idlen + keylen + 1);
            totalmem += malloc_usable_size(slot);
            slotc = (char *)(slot+1);
            memcpy(slotc, line, idlen+1);
            memcpy(slotc+idlen+1, key, keylen);
            slot->idlen = idlen;
            slot->keylen = keylen;
            slot->hash = hash;
            which = hash%count;
            slot->next = slots[which];
            if (slot->next != NULL)
                collide++;
            slots[which] = slot;
        }
    }
    after = getMillis();
    printf("read PSK entries=%d slots=%d collision=%d memory=%lu time=%lu\n",
            entries, count, collide, totalmem, after-before);
    return 0;
}

static int lookupPSK(FILE * f, uint32_t count) {
    char  line[4096];
    int levels[64] = {0};
    int notfound = 0;
    int maxlevel = 0;
    char  key[4096];
    uint64_t before;
    uint64_t after;
    int        i;
    int        entries = 0;
    double     mean = 0.0;

    before = getMillis();

    while (fgets(line, sizeof(line) - 1, f)) {
        char *   eol;
        uint32_t which;
        uint32_t hash;
        slot_t * slot;
        char *   slotc;
        char * pos = strchr(line, ',');
        if (pos && pos != line) {
            int level = 1;
            int found = 0;
            entries++;
            *pos++ = 0;
            hash = hash_string(line);

            which = hash%count;
            slot = slots[which];
            while (slot) {
                slotc = (char *)(slot+1);
                if (!strcmp(slotc, line)) {
                    found = 1;
                    if (level > maxlevel)
                        maxlevel = level;
                    if (level>63)
                        level = 63;
                    levels[level]++;
                    mean += (double)level;
                    break;
                } else {
                    slot = slot->next;
                    level++;
                }
            }
            if (!found)
                notfound++;
        }
    }
    after = getMillis();
    printf("lookup PSK entries=%d maxlevel=%d mean=%g time=%lu\n", entries, maxlevel, mean/entries, after-before);
    if (maxlevel > 63)
        maxlevel = 63;
    for (i=1; i<=maxlevel; i++) {
        printf("%d=%u ", i, levels[i]);
    }
    printf("\n");
    return 0;
}

int main(int argc, char * * argv) {
    int count = 0;
    int size = 0;
    int slot_count = 0;
    char * filen = NULL;
    FILE * f;

    if (argc > 1)
        count = atoi(argv[1]);
    if (argc > 2)
        size = atoi(argv[2]);
    if (argc > 3)
        filen = argv[3];
    if (argc > 4)
        slot_count = atol(argv[4]);


    if (count < 1)
        count = 1000;
    if (size < 1)
        size = 16;
    if (size > 2048)
        size = 2048;
    if (slot_count < 1)
        slot_count = count;

    if (filen) {
        f = fopen(filen, "wb");
    } else {
        f = stdout;
    }

    createPSK(f, count, size);
    if (filen) {
        fclose(f);
        f = fopen(filen, "rb");
        readPSK(f, slot_count);
        rewind(f);
        lookupPSK(f, slot_count);
        fclose(f);
    }
}

