
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

#include "javacall_time.h"
#include "javacall_logging.h"
#include "javautil_printf.h"

/*Converts an integer to a string*/
static char* convertInt2String(int inputInt, char *buffer);
/*Converts an hexadecimal number to string*/
static char* convertHexa2String(int inputHex, char *buffer);
/*Prints one character*/
static void  javautil_putchar(const char outputChar);
/* Gets the channel name */
static char* get_channel_name(int channelID);
/*Gets the severity description*/
static char* get_severity_name(int severity);
/*
 * Supported types are signed integer, character, string, hexadecimal format integer:
 * d,u,i = int,c = char, s = string (char *), x = int in hexadecimal format.
 *
 * The longest 64 bit integer could be 21 characters long including the '-' and '\0'
 * MAX_INT_64   0x7FFFFFFFFFFFFFFF = 9223372036854775807
 * MAX_UINT_64  0xFFFFFFFFFFFFFFFF = 18446744073709551615
 * MIN_INT_64   0x8000000000000000 = -9223372036854775808

 * The longest 32 bit integer could be 12 characters long including the '-' and '\0'
 * MAX_INT_32   0x7FFFFFFF = 2147483647
 * MAX_UINT_32  0xFFFFFFFF = 4294967295
 * MIN_INT_32   0x80000000 = -2147483648

 * The longest 16 bit integer could be 7 characters long including the '-' and '\0'
 * MAX_INT_16   0x7FFF = 32767
 * MAX_UINT_16  0xFFFF = 65535
 * MIN_INT_16   0x8000 = -32768
 */

/* Definitions */
#define MIN_NEGATIVE_INT_64 (-9223372036854775807-1)
#define MIN_NEGATIVE_INT_32 (-2147483647-1)
#define MIN_NEGATIVE_INT_16 (-32767-1)
#define STRIP_SIGNIFICANT_BIT_MASK_64  0x0FFFFFFFFFFFFFFF
#define STRIP_SIGNIFICANT_BIT_MASK_32  0x0FFFFFFF
#define STRIP_SIGNIFICANT_BIT_MASK_16  0x0FFF

#define CONVERSION_BUFFER_SIZE      21

/* Gets the channel name */
static char* get_channel_name(int channelID) {
    switch(channelID) {
        case 0:
            return("NONE");
        case 500:
            return("AMS");
        case 1000:
            return("CORE");
        case 2000:
            return("LOWUI");
        case 4000:
            return("HIGHUI");
        case 5000:
            return("PROTOCOL");
        case 6000:
            return("RMS");
        case 7000:
            return("SECURITY");
        case 8000:
            return("SERVICES");
        case 11000:
            return("STORAGE");
        case 11500:
            return("PUSH");
        case 13000:
            return("MMAPI");
        case 12000:
            return("LIFECYCLE");
        case 1100:
            return("MIDPSTRING");
        case 1200:
            return("MALLOC");
        case 1300:
            return("CORE_STACK");
        case 3000:
            return("I18N");
        case 4100:
            return("HIGHUI_ITEM_LAYOUT");
        case 4200:
            return("HIGHUI_ITEM_REPAINT");
        case 4300:
            return("HIGHUI_FORM_LAYOUT");
        case 4400:
            return("HIGHUI_ITEM_PAINT");
        case 9000:
            return("TOOL");
        case 10000:
            return("JSR180");
        case 10500:
            return("EVENTS");
        case 15000:
            return("JSR257");
        default:
            return("LOG_DISABLED");
    }

}
/*Gets the severity description*/

static char* get_severity_name(int severity){
    switch(severity) {
        case 0:
            return("INFORMATION");
        case 1:
            return("WARNING");
        case 2:
            return("ERROR");
        case 3:
            return("CRITICAL");
        case 4:
            return("NONE");
        default:
            return("LOG_DISABLED");
    }
}

/*
 * Not all compilers provide printf function, so we have
 * to use workaround. This function is to be used from javacall.
 */

void javautil_printf(int severity, int channelID, char *message, ...) {
    va_list list;
    va_start(list, message);
    javautil_vprintf(severity, channelID, -1, message, list);
    va_end(list);

}

/*
 * Not all compilers provide vprintf function, so we have
 * to use workaround. This function is to be used from midp.
 * Prints out thr DEBUG message in the following format:
 *  elapsed time in milliseconds|severiry|channel ID|isolate ID|message
 */

void javautil_vprintf(int severity, int channelID, int isolateID, char *msg, va_list vl) {

    char *str = 0;
    char tempBuffer[CONVERSION_BUFFER_SIZE];
    union Printable_t {
        int     i;
        int     x;
        char    c;
        char   *s;
    } Printable;
    char* separator = " - ";
    javacall_time_milliseconds ms;
    ms = javacall_time_get_clock_milliseconds();

    /*
       msg is the last argument specified; all
       others must be accessed using the variable-
       argument macros.
    */

    str = convertInt2String(ms,tempBuffer);
    javacall_print(str);
    javacall_print(separator);
    str = get_channel_name(channelID);
    javacall_print(str);
    javacall_print(separator);
    str = get_severity_name(severity);
    javacall_print(str);
    javacall_print(separator);
    str = convertInt2String(isolateID,tempBuffer);
    javacall_print(str);
    javacall_print(separator);

    if(msg == NULL) {
       javacall_print("NULL\n");
       return;
    }



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
            /*Need to revisit: %ld and %lld ?*/
                case 'u':
                case 'i':
                case 'd': /* integer */
                    Printable.i = va_arg( vl, int );
                    str = convertInt2String(Printable.i,tempBuffer);
                    javacall_print(str);
                    break;

                case 'x': /* hexadecimal */
                    Printable.x = va_arg( vl, int );
                    str = convertHexa2String(Printable.x,tempBuffer);
                    javacall_print(str);
                    break;

                case 'c': /* character */
                    Printable.c = (char)va_arg( vl, int );
                    javautil_putchar(Printable.c);
                    break;

                case 's': /* string */
                    Printable.s = va_arg( vl, char * );
                    javacall_print(Printable.s);
                    break;

                default:
                    javacall_print("\nUnsupported type. Cant print %");
                    javautil_putchar(*msg);
                    javacall_print(".\n");
                    break;
            }/*end of switch*/
            msg++;
        }/*end of else*/

    }/*end of while*/

    javacall_print("\n");

}/* end of javautil_printf */

/*Prints one character*/

static void javautil_putchar(const char outputChar) {
    char java_outputChar[2];

    java_outputChar[0] = outputChar;
    java_outputChar[1] = '\0';
    javacall_print(java_outputChar);
}

/*Converts an hexadecimal number to string*/

static char* convertHexa2String(int inputHex, char *buffer) {
    const char hexaCharactersTable[17] = "0123456789ABCDEF";
    char *pstr = buffer;
    int neg = 0;
    int rem;
    int mask = 0;

    if(sizeof(int) == 2) {
        mask = STRIP_SIGNIFICANT_BIT_MASK_16;
    } else if(sizeof(int) == 4) {
        mask = STRIP_SIGNIFICANT_BIT_MASK_32;
    } else if(sizeof(int) == 8) {
        /*
         * The purpose of this casting operator is to disable compiler
         * warning when compiling for machines whose word size is smaller
         * than 8 bytes. The casting has no effect on machines where word
         * size is 8 bytes.
         */
        mask = (int)STRIP_SIGNIFICANT_BIT_MASK_64;
    }

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
            inputHex = inputHex & mask;
            neg = 0;
        }
    } while(inputHex > 0);

    return pstr;
}

/*Converts an integer to a string*/


static char* convertInt2String(int inputInt, char *buffer) {
    int base = 10;
    int neg = 0;
    unsigned int conversion_unit = 0;
    char *pstr = buffer;
    int min_negative_int = 0;
    pstr+=(CONVERSION_BUFFER_SIZE-1);

    *pstr = 0;

    if(sizeof(int) == 2) {
        min_negative_int = MIN_NEGATIVE_INT_16;
    } else if(sizeof(int) == 4) {
        min_negative_int = MIN_NEGATIVE_INT_32;
    } else {
        /*
         * The purpose of this casting operator is to disable compiler
         * warning when compiling for machines whose word size is smaller
         * than 8 bytes. The casting has no effect on machines where word
         * size is 8 bytes.
         */
        min_negative_int = (int)MIN_NEGATIVE_INT_64;
    }


    if(inputInt < 0) {
        neg = 1;
        inputInt*=(-1);
    }

    if(inputInt == min_negative_int) {
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

