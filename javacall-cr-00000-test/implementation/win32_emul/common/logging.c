/*
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

/**
* @file
*
* win32 implementation for logging javacall functions
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdarg.h>
#include <string.h>

#include "local_defs.h"
#include "javacall_logging.h"
#include "javacall_properties.h"

char print_buffer[PRINT_BUFFER_SIZE];
char debug_print_buffer[PRINT_BUFFER_SIZE];
static BOOL dbgInit = FALSE;
static HWND dbgWin = NULL;
static HANDLE dbgFile;
static char dbgWinBuff[PRINT_BUFFER_SIZE];
/**
* Prints out a string to a system specific output stream
*
* @param s a NULL terminated character buffer to be printed
*/
void javacall_print(const char *s) {
	if (!dbgInit) {
		//dbgWin = CreateWindow("LISTBOX", "Gil test", WS_OVERLAPPEDWINDOW|WS_VSCROLL|WS_HSCROLL, 
		//	CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
		//	GetDesktopWindow(), NULL, GetModuleHandle(NULL), NULL);
		//ShowWindow(dbgWin, SW_SHOW);
		dbgFile = CreateFile("zayitout.txt",GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
			                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		Sleep(20);
		dbgWinBuff[0] = 0;
		dbgInit = TRUE;
	} else {
		strcat(dbgWinBuff, s);
		if (dbgWinBuff[strlen(dbgWinBuff)-1] == '\n') {
			DWORD len;
			WriteFile(dbgFile, dbgWinBuff, strlen(dbgWinBuff), &len, NULL);
			FlushFileBuffers(dbgFile);
			dbgWinBuff[strlen(dbgWinBuff)-1] = 0;
			//SendMessage(dbgWin, (UINT) LB_ADDSTRING, 0, dbgWinBuff);

			dbgWinBuff[0] = 0;
		}
	}
    //OutputDebugString(s);
    printf("%s", s);
	fflush(stdout);
}


static char* convertSeverity2String(int  severity){
    switch(severity) {
    case JAVACALL_LOG_INFORMATION:
        return "INFO";
    case JAVACALL_LOG_WARNING:
        return "WARN";
    case JAVACALL_LOG_ERROR:
        return "ERRR";
    case JAVACALL_LOG_CRITICAL:
        return "CRIT";
    case JAVACALL_LOG_DISABLED:
        return "NONE";
    default:
        return "NONE";
    }
} /* end of convertSeverity2String */

#define LOGGER_BUFFER_LEN 1024

//@NOTE: This is NOT thread safe !

void javautil_debug_print(int severity,
                 char *channelID,
                 char *message, ...) {

    va_list ap;

    int defaultLevel;
    int minLevelForChannel;

    if (channelID == NULL)
        channelID = "Unknown";

    // If channel not found, always show ...
    {
        char *tempval;

        javacall_get_property("report_level",
                              JAVACALL_INTERNAL_PROPERTY,
                              &tempval);
        defaultLevel = (tempval==NULL)?1:atoi(tempval);
    }

    // Check if there is a level for the specific channel

    {
        char *tempval;

        javacall_get_property(channelID,
                              JAVACALL_INTERNAL_PROPERTY,
                              &tempval);
        minLevelForChannel = (tempval==NULL)?defaultLevel:atoi(tempval);
    }

    if ( severity < minLevelForChannel ) {
        return;
    }

    if(message != NULL) {

        _snprintf(debug_print_buffer, PRINT_BUFFER_SIZE, "[%4s] [%-8s] ",
                 convertSeverity2String(severity),
                 channelID);

        javacall_print(debug_print_buffer);

        va_start(ap, message);
        _vsnprintf(debug_print_buffer, PRINT_BUFFER_SIZE, message, ap);
        va_end(ap);

        strcat(debug_print_buffer,"\n");
        javacall_print(debug_print_buffer);


    }

} /* end of reportToLog */

// Thread Safe ...
void javautil_printf_lime(
                 char *message, ...) {

    va_list ap;

    char logBuffer[LOGGER_BUFFER_LEN];

    if(message != NULL) {

        va_start(ap, message);
        _vsnprintf(logBuffer, LOGGER_BUFFER_LEN, message, ap);
        va_end(ap);

        javacall_print(logBuffer);

    }

} /* end of reportToLog */
 

#ifdef __cplusplus
}
#endif
