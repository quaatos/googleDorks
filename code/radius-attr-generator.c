   /* 
    * Copyright (c) 2010 IETF Trust and the persons identified as
    * authors of the code.  All rights reserved.
    *
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions
    * are met:
    *
    * Redistributions of source code must retain the above copyright
    * notice, this list of conditions and the following disclaimer.
    *
    * Redistributions in binary form must reproduce the above copyright
    * notice, this list of conditions and the following disclaimer in
    * the documentation and/or other materials provided with the
    * distribution.
    *
    * Neither the name of Internet Society, IETF or IETF Trust, nor the
    * names of specific contributors, may be used to endorse or promote
    * products derived from this software without specific prior written
    * permission.
    *
    * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
    * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
    * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    * SUCH DAMAGE.
    *
    *  Author:  Alan DeKok <aland@networkradius.com>
    */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static int      encode_tlv(char *buffer, uint8_t * output, size_t outlen);

static const char *hextab = "0123456789abcdef";

static int
encode_data_string(char *buffer, uint8_t * output, size_t outlen)
{
    int             length = 0;
    char           *p;

    p = buffer + 1;

    while (*p && (outlen > 0)) {
        if (*p == '"') {
            return length;
        }

        if (*p != '\\') {
            *(output++) = *(p++);
            outlen--;
            length++;
            continue;
        }

        switch (p[1]) {
        default:
            *(output++) = p[1];
            break;

        case 'n':
            *(output++) = '\n';
            break;

        case 'r':
            *(output++) = '\r';
            break;

        case 't':
            *(output++) = '\t';
            break;
        }

        outlen--;
        length++;
    }

    fprintf(stderr, "String is not terminated\n");
    return 0;
}

static int
encode_data_tlv(char *buffer, char **endptr, uint8_t * output, size_t outlen)
{
    int             depth = 0;
    int             length;
    char           *p;

    for (p = buffer; *p != '\0'; p++) {
        if (*p == '{')
            depth++;
        if (*p == '}') {
            depth--;
            if (depth == 0)
                break;
        }
    }

    if (*p != '}') {
        fprintf(stderr, "No trailing '}' in string starting "
                "with \"%s\"\n", buffer);
        return 0;
    }

    *endptr = p + 1;
    *p = '\0';

    p = buffer + 1;
    while (isspace((int) *p))
        p++;

    length = encode_tlv(p, output, outlen);
    if (length == 0)
        return 0;

    return length;
}

static int
encode_data(char *p, uint8_t * output, size_t outlen)
{
    int             length;

    if (!isspace((int) *p)) {
        fprintf(stderr, "Invalid character following attribute " "definition\n");
        return 0;
    }

    while (isspace((int) *p))
        p++;

    if (*p == '{') {
        int             sublen;
        char           *q;

        length = 0;

        do {
            while (isspace((int) *p))
                p++;
            if (!*p) {
                if (length == 0) {
                    fprintf(stderr, "No data\n");
                    return 0;
                }

                break;
            }

            sublen = encode_data_tlv(p, &q, output, outlen);
            if (sublen == 0)
                return 0;

            length += sublen;
            output += sublen;
            outlen -= sublen;
            p = q;
        } while (*q);

        return length;
    }

    if (*p == '"') {
        length = encode_data_string(p, output, outlen);
        return length;
    }

    length = 0;
    while (*p) {

        char           *c1, *c2;

        while (isspace((int) *p))
            p++;

        if (!*p)
            break;

        if (!(c1 = memchr(hextab, tolower((int) p[0]), 16)) ||
            !(c2 = memchr(hextab, tolower((int) p[1]), 16))) {
            fprintf(stderr, "Invalid data starting at " "\"%s\"\n", p);
            return 0;
        }

        *output = ((c1 - hextab) << 4) + (c2 - hextab);
        output++;
        length++;
        p += 2;

        outlen--;
        if (outlen == 0) {
            fprintf(stderr, "Too much data\n");
            return 0;
        }
    }

    if (length == 0) {
        fprintf(stderr, "Empty string\n");
        return 0;
    }

    return length;
}

static int
decode_attr(char *buffer, char **endptr)
{
    long            attr;

    attr = strtol(buffer, endptr, 10);
    if (*endptr == buffer) {
        fprintf(stderr, "No valid number found in string "
                "starting with \"%s\"\n", buffer);
        return 0;
    }

    if (!**endptr) {
        fprintf(stderr, "Nothing follows attribute number\n");
        return 0;
    }

    if ((attr <= 0) || (attr > 256)) {
        fprintf(stderr, "Attribute number is out of valid " "range\n");
        return 0;
    }

    return (int) attr;
}

static int
decode_vendor(char *buffer, char **endptr)
{
    long            vendor;

    if (*buffer != '.') {
        fprintf(stderr, "Invalid separator before vendor id\n");
        return 0;
    }

    vendor = strtol(buffer + 1, endptr, 10);
    if (*endptr == (buffer + 1)) {
        fprintf(stderr, "No valid vendor number found\n");
        return 0;
    }

    if (!**endptr) {
        fprintf(stderr, "Nothing follows vendor number\n");
        return 0;
    }

    if ((vendor <= 0) || (vendor > (1 << 24))) {
        fprintf(stderr, "Vendor number is out of valid range\n");
        return 0;
    }

    if (**endptr != '.') {
        fprintf(stderr, "Invalid data following vendor number\n");
        return 0;
    }
    (*endptr)++;

    return (int) vendor;
}

static int
encode_tlv(char *buffer, uint8_t * output, size_t outlen)
{
    int             attr;
    int             length;
    char           *p;

    attr = decode_attr(buffer, &p);
    if (attr == 0)
        return 0;

    output[0] = attr;
    output[1] = 2;

    if (*p == '.') {
        p++;
        length = encode_tlv(p, output + 2, outlen - 2);

    } else {
        length = encode_data(p, output + 2, outlen - 2);
    }

    if (length == 0)
        return 0;
    if (length > (255 - 2)) {
        fprintf(stderr, "TLV data is too long\n");
        return 0;
    }

    output[1] += length;

    return length + 2;
}

static int
encode_vsa(char *buffer, uint8_t * output, size_t outlen)
{
    int             vendor;
    int             attr;
    int             length;
    char           *p;

    vendor = decode_vendor(buffer, &p);
    if (vendor == 0)
        return 0;

    output[0] = 0;
    output[1] = (vendor >> 16) & 0xff;
    output[2] = (vendor >> 8) & 0xff;
    output[3] = vendor & 0xff;

    length = encode_tlv(p, output + 4, outlen - 4);
    if (length == 0)
        return 0;
    if (length > (255 - 6)) {
        fprintf(stderr, "VSA data is too long\n");
        return 0;
    }

    return length + 4;
}

static int
encode_evs(char *buffer, uint8_t * output, size_t outlen)
{
    int             vendor;
    int             attr;
    int             length;
    char           *p;

    vendor = decode_vendor(buffer, &p);
    if (vendor == 0)
        return 0;

    attr = decode_attr(p, &p);
    if (attr == 0)
        return 0;

    output[0] = 0;
    output[1] = (vendor >> 16) & 0xff;
    output[2] = (vendor >> 8) & 0xff;
    output[3] = vendor & 0xff;
    output[4] = attr;

    length = encode_data(p, output + 5, outlen - 5);
    if (length == 0)
        return 0;

    return length + 5;
}

static int
encode_extended(char *buffer, uint8_t * output, size_t outlen)
{
    int             attr;
    int             length;
    char           *p;

    attr = decode_attr(buffer, &p);
    if (attr == 0)
        return 0;

    output[0] = attr;

    if (attr == 26) {
        length = encode_evs(p, output + 1, outlen - 1);
    } else {
        length = encode_data(p, output + 1, outlen - 1);
    }
    if (length == 0)
        return 0;
    if (length > (255 - 3)) {
        fprintf(stderr, "Extended Attr data is too long\n");
        return 0;
    }

    return length + 1;
}

static int
encode_extended_flags(char *buffer, uint8_t * output, size_t outlen)
{
    int             attr;
    int             length, total;
    char           *p;

    attr = decode_attr(buffer, &p);
    if (attr == 0)
        return 0;

    /* output[0] is the extended attribute */
    output[1] = 4;
    output[2] = attr;
    output[3] = 0;

    if (attr == 26) {
        length = encode_evs(p, output + 4, outlen - 4);
        if (length == 0)
            return 0;

        output[1] += 5;
        length -= 5;
    } else {
        length = encode_data(p, output + 4, outlen - 4);
    }
    if (length == 0)
        return 0;

    total = 0;
    while (1) {
        int             sublen = 255 - output[1];

        if (length <= sublen) {
            output[1] += length;
            total += output[1];
            break;
        }

        length -= sublen;

        memmove(output + 255 + 4, output + 255, length);
        memcpy(output + 255, output, 4);

        output[1] = 255;
        output[3] |= 0x80;

        output += 255;
        output[1] = 4;
        total += 255;
    }

    return total;
}

static int
encode_rfc(char *buffer, uint8_t * output, size_t outlen)
{
    int             attr;
    int             length, sublen;
    char           *p;

    attr = decode_attr(buffer, &p);
    if (attr == 0)
        return 0;

    length = 2;
    output[0] = attr;
    output[1] = 2;

    if (attr == 26) {
        sublen = encode_vsa(p, output + 2, outlen - 2);

    } else if ((*p == ' ') || ((attr < 241) || (attr > 246))) {
        sublen = encode_data(p, output + 2, outlen - 2);

    } else {
        if (*p != '.') {
            fprintf(stderr, "Invalid data following " "attribute number\n");
            return 0;
        }

        if (attr < 245) {
            sublen = encode_extended(p + 1, output + 2, outlen - 2);
        } else {

            /* 
             *   Not like the others!
             */
            return encode_extended_flags(p + 1, output, outlen);
        }
    }
    if (sublen == 0)
        return 0;

    if (sublen > (255 - 2)) {
        fprintf(stderr, "RFC Data is too long\n");
        return 0;
    }

    output[1] += sublen;
    return length + sublen;
}

int
main(int argc, char *argv[])
{
    int             lineno;
    size_t          i, outlen;
    FILE           *fp;
    char            input[8192], buffer[8192];
    uint8_t         output[4096];

    if ((argc < 2) || (strcmp(argv[1], "-") == 0)) {
        fp = stdin;
    } else {
        fp = fopen(argv[1], "r");
        if (!fp) {
            fprintf(stderr, "Error opening %s: %s\n", argv[1], strerror(errno));
            exit(1);
        }
    }

    lineno = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char           *p = strchr(buffer, '\n');

        lineno++;

        if (!p) {
            if (!feof(fp)) {
                fprintf(stderr, "Line %d too long in %s\n", lineno, argv[1]);
                exit(1);
            }
        } else {
            *p = '\0';
        }

        p = strchr(buffer, '#');
        if (p)
            *p = '\0';

        p = buffer;

        while (isspace((int) *p))
            p++;
        if (!*p)
            continue;

        strcpy(input, p);
        outlen = encode_rfc(input, output, sizeof(output));
        if (outlen == 0) {
            fprintf(stderr, "Parse error in line %d of %s\n", lineno, input);
            exit(1);
        }

        printf("%s -> ", buffer);
        for (i = 0; i < outlen; i++) {
            printf("%02x ", output[i]);
        }
        printf("\n");
    }

    if (fp != stdin)
        fclose(fp);

    return 0;
}
