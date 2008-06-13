/*
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "javautil_stdio.h"

#define LEFT    0x10    /* left justified */

/**
 * Reads number from input buffer, converts it to integer, forwards the buffer
 *
 * @param buf the source buffer
 * @return integer value of the read number
 */
static int skip_atoi(const char **buf) {
    int i = 0;

    while (isdigit(**buf)){
        i = i*10 + *((*buf)++) - '0';
    }
    return i;
}

/**
 * Formats a string and places it in a buffer
 *
 * @param buf buffer to place the result into
 * @param size buffer size
 * @param fmt format string to use
 * @param args string arguments
 *
 * @return number of printed characters
 */
static int javautil_vsnprintf(char *buf, size_t size, const char *fmt,
                              va_list args) {
    int len;
    int i;
    char *str, *end;
    const char *s;
    int flags;      /* flags to number() */
    int field_width;    /* width of output field */
    int is_flag;

    /* calculate upper address delimiter */
    str = buf;
    end = buf + size - 1;

    /* support unlimited buffer size */
    if (end < buf - 1) {
        end = ((char *)-1);
        size = end - buf + 1;
    }

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            if (str <= end){
                *str = *fmt;
            }
            str++;
            continue;
        }

        /* process flags */
        flags = 0;
        for (is_flag = 1; 1 == is_flag;) {
            fmt++;      /* this also skips first '%' */
            switch (*fmt) {
                case '-':
                    flags |= LEFT;
                    break;
                /* Add more flags here */
                default:
                    is_flag = 0;    /* not a recognized flag */
            }
        }

        /* get field width */
        field_width = -1;
        if (isdigit(*fmt)){
            field_width = skip_atoi(&fmt);
        }

        switch (*fmt) {
            /* 's' format support */
            case 's':
                s = va_arg(args, char *);
                if (!s){
                    s = "<NULL>";
                }

                len = strlen(s);

                if (!(flags & LEFT)) {
                    /* alignment to right - pad with spaces */
                    while (len < field_width) {
                        if (str <= end){
                            *str = ' ';
                        }
                        field_width--;
                        str++;
                    }
                }
                for (i = 0; i < len; i++) {
                    if (str <= end){
                        *str = *s;
                    }
                    str++;
                    s++;
                }
                while (len < field_width) {
                    if (str <= end){
                        *str = ' ';
                    }
                    str++;
                    field_width--;
                }
                continue;
            /* Insert additional format supports here */

            default:
                continue;
        }
    }
    if (str <= end){
        *str = '\0';
    }
    else if (size > 0){
        /* don't write out a null byte if the buf size is zero */
        *end = '\0';
    }

    return str - buf;
}


/**
 * Formats a string and places it in a buffer
 *
 * @param buf buffer to place the result into
 * @param fmt format string to use
 * @param args arguments for the format string
 *
 * @return number of printed characters
 */
static int javautil_vsprintf(char *buf, const char *fmt, va_list args) {
    return javautil_vsnprintf(buf, (size_t)-1, fmt, args);
}


/**
 * Formats a string and places it in a buffer
 *
 * @param buf buffer to place the result into
 * @param fmt format string to use
 * @param ... arguments for the format string
 *
 * @return number of printed characters
 */
int javautil_sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    int i;

    va_start(args, fmt);
    i = javautil_vsprintf(buf, fmt, args);
    va_end(args);
    return i;
}

