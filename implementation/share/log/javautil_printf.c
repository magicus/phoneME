#include <stdio.h>
//#include "javacall_platform_defs.h"
//#include "javacall_defs.h"
#include "javacall_logging.h"
#include "javautil_printf.h"

void javautil_vsprintf(int severity, int channelID, int isolateID, char *szTypes, va_list vl);
static int   midp_strlen(const char *str);
static char* convertInt2String(int inputInt, char *buffer);
static char* convertHexa2String(int inputHex, char *buffer);
static void  midp_putstring(const char *outputString);
static void  midp_putchar(const char outputChar);

//#define USING_64_BIT_INTEGER

/* Supported types are signed integer, character, string, hexadecimal format integer: */
/* d,u,i = int,c = char, s = string (char *), x = int in hexadecimal format. */

#ifdef USING_64_BIT_INTEGER
/* The longest 64 bit integer could be 21 characters long including the '-' and '\0' */
/* MAX_INT_64   0x7FFFFFFFFFFFFFFF = 9223372036854775807  */
/* MAX_UINT_64  0xFFFFFFFFFFFFFFFF = 18446744073709551615 */
/* MIN_INT_64   0x8000000000000000 = -9223372036854775808 */
#define STRIP_SIGNIFICANT_BIT_MASK  0x0FFFFFFFFFFFFFFF
#define CONVERSION_BUFFER_SIZE      21
#else /* NOT USING_64_BIT_INTEGER */
/* The longest 32 bit integer could be 21 characters long including the '-' and '\0' */
/* MAX_INT_64   0x7FFFFFFF = 2147483647  */
/* MAX_UINT_64  0xFFFFFFFF = 4294967295  */
/* MIN_INT_64   0x80000000 = -2147483648 */
#define STRIP_SIGNIFICANT_BIT_MASK  0x0FFFFFFF
#define CONVERSION_BUFFER_SIZE      12
#endif /* USING_64_BIT_INTEGER */




void javacall_printf(int severity, int channelID, char *message, ...){

    va_list list;
    va_start(list, message);
    javautil_vsprintf(severity, channelID, 1, message, list);
    va_end(list);

}

void javautil_printf(int severity, int channelID, int isolateID, char *message, ...) {
    va_list list;
    va_start(list, message);
    javautil_vsprintf(severity, channelID, isolateID, message, list);
    va_end(list);
}

void javautil_vsprintf(int severity, int channelID, int isolateID, char *szTypes, va_list vl) {

    char *str = 0;
    char tempBuffer[CONVERSION_BUFFER_SIZE];
    union Printable_t {
        int     i;
        int     x;
        char    c;
        char   *s;
    } Printable;

    if(szTypes == NULL) {
        return;
    }
    /* 
       szTypes is the last argument specified; all
       others must be accessed using the variable-
       argument macros.
    */

    while(*szTypes) {

        if((*szTypes == '\\') && (*(szTypes+1) == '%')) {
            szTypes++;
            midp_putchar(*szTypes);
            szTypes++;
        } else if(*szTypes != '%') {
            midp_putchar(*szTypes);
            szTypes++;
        } else {

            szTypes++;

            switch(*szTypes) {    /* Type to expect.*/
            /*FIXME %ld and %lld ?*/
                case 'u':
                case 'i':
                case 'd': /* integer */
                    Printable.i = va_arg( vl, int );
                    str = convertInt2String(Printable.i,tempBuffer);
                    midp_putstring(str);
                    break;

                case 'x': /* hexadecimal */
                    Printable.x = va_arg( vl, int );
                    str = convertHexa2String(Printable.x,tempBuffer);
                    midp_putstring(str);
                    break;

                case 'c': /* character */
                    Printable.c = (char)va_arg( vl, int );
                    midp_putchar(Printable.c);
                    break;

                case 's': /* string */
                    Printable.s = va_arg( vl, char * );
                    midp_putstring(Printable.s);
                    break;

                default:
                /*FIXME I think we should call to va_arg here as well */
                /*va_arg( vl, ???? );*/
                    midp_putstring("\nUnsupported type. Cant print %");
                    midp_putchar(*szTypes);
                    midp_putstring(".\n");
                    break;
            }/*end of switch*/
            szTypes++;
        }/*end of else*/

    }/*end of while*/

}/* end of midp_printf */

javacall_result javautil_printf_initialize(void) {
    return JAVACALL_OK; 
}

javacall_result javautil_printf_finalize(void) {
    return JAVACALL_OK;
}


static void midp_putchar(const char outputChar) {
    const char java_outputChar[2]= {outputChar, '\0'};
    javacall_print(java_outputChar);
}


static void midp_putstring(const char *outputString) {
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
    char *pstr = buffer;
    pstr+=(CONVERSION_BUFFER_SIZE-1);

    if(inputInt < 0) {
        neg = 1;
        inputInt*=(-1);
    }

    *pstr = 0;

    do {
        pstr--;
        *pstr = ((inputInt % base)+'0');
        inputInt = inputInt/base;
    }while(inputInt > 0);

    if(neg) {
        pstr--;
        *pstr = '-';
    }
    return pstr;
}

static int midp_strlen(const char *str) {
    int i;
    if(str == NULL) {
        return -1;
    }
    for(i = 0; *str != 0; i++) {
        str++;
    }
    return i;
}
