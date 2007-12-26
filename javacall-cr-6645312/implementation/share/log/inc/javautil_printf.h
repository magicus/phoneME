#ifndef JAVAUTIL_PRINTF_INCLUDED
#define JAVAUTIL_PRINTF_INCLUDED

#include <stdarg.h>

int javautil_printf_initialize(void);
int javautil_printf_finalize(void);

// to be used from javacall
void javacall_printf(int severity, int channelID, char *message, ...);


// To be used from midp
void javautil_printf(int severity, int channelID, int isolateID, char *message, ...);
void javautil_vsprintf(int severity, int channelID, int isolateID, char *szTypes, va_list vl);


#endif /* of JAVAUTIL_PRINTF_INCLUDED */

