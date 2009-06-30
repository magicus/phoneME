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
 
#include <string.h>
#include <stdio.h>

#include <midpString.h>
#include <midpMalloc.h>
#include <midpDebug.h>
#include <midp_logging.h>


#if REPORT_LEVEL <= LOG_INFORMATION

/**
 * Prints a <tt>MidpString</tt> with a given message. A new line will
 * be emitted after <tt>mstr</tt>.  This output goes to the log in cases
 * were the log service defined <code>REPORT_LEVEL <= LOG_INFORMATION</code>
 * conditional is true.
 *
 * @param message the specified message to print 
 * @param mstr the <tt>MidpString</tt> to print
 */
void
printMidpStringWithMessageImpl(char* message, MidpString mstr) {
    char* msg = NULL;
    char* tag;

    if ((mstr.len <= 0) && (!message)) {
              REPORT_INFO(LC_MIDPSTRING, 
		    "printMidpString() No input");
        return;
    }

    if (message) {
        tag = message;
    } else {
        tag = "message = NULL:";
    }

    if (mstr.len > 0) {
        msg = midpJcharsToChars(mstr);
        if (msg) {
          REPORT_INFO2(LC_MIDPSTRING, "%s: %s", tag, msg);
	    midpFree(msg);
        } else {
            REPORT_INFO1(LC_MIDPSTRING, 
	        "%s: printMidpString can't convert MidpString to char*",tag);
        }
    }
}

void
printPcslStringWithMessageImpl(char* message, const pcsl_string* pstr) {
    char* tag;

    if ((pcsl_string_length(pstr) <= 0) && (!message)) {
        REPORT_INFO(LC_MIDPSTRING,
		    "printPcslStringWithMessage() No input");
        return;
    }

    if (message) {
        tag = message;
    } else {
        tag = "message = NULL:";
    }

    if (pcsl_string_length(pstr) > 0) {
        const char* msg = pcsl_string_get_utf8_data(pstr);
        if (msg) {
            REPORT_INFO2(LC_MIDPSTRING, "%s: %s", tag, msg);
            pcsl_string_release_utf8_data(msg, pstr);
        } else {
           REPORT_INFO1(LC_MIDPSTRING,
	        "%s: printPcslStringWithMessage can't convert pcsl_string to char*", tag);
        }
    } else {
        REPORT_INFO2(LC_MIDPSTRING, "%s: pcsl_string is %s", tag,
            0 == pcsl_string_length(pstr) ? "empty" : "null" );
    }
}

/**
 * Prints a <tt>MidpString</tt>. A new line will be emitted after
 * <tt>mstr</tt>.  This output goes to the log in cases
 * were the log service defined REPORT_LEVEL <= LOG_INFORMATION
 *
 * @param mstr the <tt>MidpString</tt> to print
 */
void
printMidpStringImpl(MidpString mstr) {
    char* msg = NULL;  
    if (mstr.len <= 0) {
        REPORT_INFO(LC_MIDPSTRING, 
		    "printMidpString: Bad Length.");
        return;
    }
    if (mstr.len > 0) {
        msg = midpJcharsToChars(mstr);
        if (msg) {
	        REPORT_INFO1(LC_MIDPSTRING, "%s", msg);
            midpFree(msg);
        } else {
             REPORT_INFO(LC_MIDPSTRING, 
	        "printMidpString: can't convert from MidpString to char");

        }
    }
}
void
printPcslStringImpl(const pcsl_string* pstr) {
    if (pcsl_string_length(pstr) <= 0) {
       REPORT_INFO(LC_MIDPSTRING,
		    "printPcslString: Bad Length.");
        return;
    }
    if (pcsl_string_length(pstr) > 0) {
        const char* msg = pcsl_string_get_utf8_data(pstr);
        if (msg) {
	        REPORT_INFO1(LC_MIDPSTRING, "%s", msg);
            pcsl_string_release_utf8_data(msg, pstr);
        } else {
             REPORT_INFO(LC_MIDPSTRING,
	        "printPcslString: can't convert from pcsl_string to char");

        }
    }
}

#endif /* if REPORT_LEVEL <= LOG_INFORMATION */




