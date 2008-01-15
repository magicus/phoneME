
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

#include <stdio.h>
#include <stdarg.h>
#include "javacall_logging.h"
#include "javautil_printf.h"

static char* convertInt2String(int inputInt, char *buffer);
static char* convertHexa2String(int inputHex, char *buffer);
static void  javautil_putstring(const char *outputString);
static void  javautil_putchar(const char outputChar);

//#define USING_64_BIT_INTEGER

/* Supported types are signed integer, character, string, hexadecimal format integer: */
/* d,u,i = int,c = char, s = string (char *), x = int in hexadecimal format. */

#ifdef USING_64_BIT_INTEGER

/* The longest 64 bit integer could be 21 characters long including the '-' and '\0' */
/* MAX_INT_64   0x7FFFFFFFFFFFFFFF = 9223372036854775807  */
/* MAX_UINT_64  0xFFFFFFFFFFFFFFFF = 18446744073709551615 */
/* MIN_INT_64   0x8000000000000000 = -9223372036854775808 */
#define STRIP_SIGNIFICANT_BIT_MASK  0x0FFFFFFFFFFFFFFF
#define MIN_NEGATIVE_INT (-9223372036854775808)

#define CONVERSION_BUFFER_SIZE      21
#else /* NOT USING_64_BIT_INTEGER */

/* The longest 32 bit integer could be 12 characters long including the '-' and '\0' */
/* MAX_INT_32   0x7FFFFFFF = 2147483647  */
/* MAX_UINT_32  0xFFFFFFFF = 4294967295  */
/* MIN_INT_32   0x80000000 = -2147483648 */
#define MIN_NEGATIVE_INT (-2147483648)

#define STRIP_SIGNIFICANT_BIT_MASK  0x0FFFFFFF
#define CONVERSION_BUFFER_SIZE      12
#endif /* USING_64_BIT_INTEGER */




void javautil_printf(int severity, int channelID, char *message, ...) {
    va_list list;
    va_start(list, message);
    javautil_vsprintf(severity, channelID, 1, message, list);
    va_end(list);

}

void javautil_vprintf(int severity, int channelID, int isolateID, char *msg, va_list vl) {

    char *str = 0;
    char tempBuffer[CONVERSION_BUFFER_SIZE];
    union Printable_t {
        int     i;
        int     x;
        char    c;
        char   *s;
    } Printable;

    if(msg == NULL) {
        return;
    }
    /* 
       msg is the last argument specified; all
       others must be accessed using the variable-
       argument macros.
    */

    while(*msg) {

        if((*msg == '%') && (*(msg+1) == '%')) {
            msg++;
            javautil_putchar(*msg);
            msg++;
        } else if(*msg != '%') {
            javautil_putchar(*msg);
            msg++;
        } else {

            msg++;

            switch(*msg) {    /* Type to expect.*/
            /*FIXME %ld and %lld ?*/
                case 'u':
                case 'i':
                case 'd': /* integer */
                    Printable.i = va_arg( vl, int );
                    str = convertInt2String(Printable.i,tempBuffer);
                    javautil_putstring(str);
                    break;

                case 'x': /* hexadecimal */
                    Printable.x = va_arg( vl, int );
                    str = convertHexa2String(Printable.x,tempBuffer);
                    javautil_putstring(str);
                    break;

                case 'c': /* character */
                    Printable.c = (char)va_arg( vl, int );
                    javautil_putchar(Printable.c);
                    break;

                case 's': /* string */
                    Printable.s = va_arg( vl, char * );
                    javautil_putstring(Printable.s);
                    break;

                default:
                /*FIXME I think we should call to va_arg here as well */
                /*va_arg( vl, ???? );*/
                    javautil_putstring("\nUnsupported type. Cant print %");
                    javautil_putchar(*msg);
                    javautil_putstring(".\n");
                    break;
            }/*end of switch*/
            msg++;
        }/*end of else*/

    }/*end of while*/

}/* end of javautil_printf */


static void javautil_putchar(const char outputChar) {
    const char java_outputChar[2]= {outputChar, '\0'};
    javacall_print(java_outputChar);
}


static void javautil_putstring(const char *outputString) {
    javacall_print(outputString);
}


static char* convertHexa2String(int inputHex, char *buffer) {
    const char hexaCharactersTable[16] = "0123456789ABCDEF";
    char *pstr = buffer;
    int neg = 0;
    int rem;
    pstr+=(CONVERSION_BUFFER_SIZE-1);
    *pstr = 0;

    if(inputHex < 0) {
        neg = 1;
    }

    do {
        pstr--;
        rem = inputHex & 0xF;
        inputHex = inputHex >> 4;
        *pstr = hexaCharactersTable[rem];
        if(neg) {
            inputHex = inputHex & STRIP_SIGNIFICANT_BIT_MASK;
            neg = 0;
        }
    } while(inputHex > 0);

    return pstr;
}

static char* convertInt2String(int inputInt, char *buffer) {
    int base = 10;
    int neg = 0;
    unsigned int conversion_unit = 0;
    char *pstr = buffer;
    pstr+=(CONVERSION_BUFFER_SIZE-1);

    *pstr = 0;

    if(inputInt < 0) {
        neg = 1;
        inputInt*=(-1);
    }

    if(inputInt == MIN_NEGATIVE_INT) {
        conversion_unit = (unsigned int)inputInt;
        do {
            pstr--;
            *pstr = ((conversion_unit % base)+'0');
            conversion_unit = conversion_unit/base;
        }while(conversion_unit > 0);

    } else {
        do {
            pstr--;
            *pstr = ((inputInt % base)+'0');
            inputInt = inputInt/base;
        }while(inputInt > 0);
    }

    if(neg) {
        pstr--;
        *pstr = '-';
    }
    return pstr;
}

