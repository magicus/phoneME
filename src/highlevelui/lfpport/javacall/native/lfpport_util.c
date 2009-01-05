/*
 *
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
 *
 * This source file is specific for Qt-based configurations.
 */

#include <stdlib.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>
#include <pcsl_string.h>
#include <lfpport_error.h>
#include "lfpport_gtk.h"

MidpError gchar_to_pcsl_string(gchar *src, pcsl_string *dst) {
    pcsl_string_status pe;
    if (src == NULL) {
	    *dst = PCSL_STRING_NULL;
    } else if (!strcmp(src, "")) {
	    *dst = PCSL_STRING_EMPTY;
    } else {
        jint length = strlen(src);
        pe = pcsl_string_convert_from_utf8(src, length, dst);
    }
    return KNI_OK;
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}


/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}



char *get_temp_file_name(void) {
    static int i = 0;
    static char buf[100];
    char tmp[4];
    strcpy(buf, tmpFilename);
    strcat(buf, "_");
    itoa(i, tmp);
    strcat(buf, tmp);
    i++;
    return buf;
}
