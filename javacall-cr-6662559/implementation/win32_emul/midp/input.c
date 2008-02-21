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
* win32 implemenation for Input Locale functions
*/

#include <stdio.h>
#include <string.h>
#include "local_defs.h"
#include "javacall_input.h"
#include "javacall_logging.h"

/**
* Returns currently set input locale.
* The input locale can be made available to MIDlets by calling
* java.lang.System.getProperty("microedition.locale").
*
* Must consist of the language and MAY optionally also contain
* the country code, and variant separated by "-" (Unicode U+002D).
* For example, "fr-FR" or "en-US."
* (Note: the MIDP 1.0 specification used the HTTP formatting of language
* tags as defined in RFC3066  Tags for the Identification of Languages.
* This is different from the J2SE definition for Locale printed strings
* where fields are separated by "_" (Unicode U+005F). )
*
*
* The language codes MUST be the lower-case, two-letter codes as defined
* by ISO-639:
* http://ftp.ics.uci.edu/pub/ietf/http/related/iso639.txt
*
*
* The country code MUST be the upper-case, two-letter codes as defined by
* ISO-3166:
* http://userpage.chemie.fu-berlin.de/diverse/doc/ISO_3166.html
*
* @param  locale a pointer to string that should be filled with null-terminated
*         string describing the currently set locale
* @return <tt>JAVACALL_OK</tt> on success, or
*         <tt>JAVACALL_FAIL</tt> if locale is not supported
*/
javacall_result javacall_input_get_locale(/*OUT*/ char locale[6]){
    strcpy(locale, "en-US");
    return JAVACALL_OK;
}

javacall_textfield_status javacall_input_textfield_launch(
        /*IN-OUT*/javacall_utf16*	            textBuffer,
        /*IN-OUT*/int*				            textBufferLength,
        /*IN*/ int					            textBufferMaxLength,
        /*IN-OUT*/int*				            caretPos,
        /*IN*/ const javacall_utf16*	        title,
        /*IN*/ int					            titleLength,
        /*IN*/ const javacall_utf16*	        inputMode,
        /*IN*/ int					            inputModeLength,
        /*IN*/ javacall_textfield_constraint    constraint,
        /*IN*/ int					            modifiers,
        /*IN*/ int                              keyCode) {

    int i;

    javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", "\n javacall_input_textfield_launch: \n textBuffer: ");

    for (i=0; i<*textBufferLength; i++) {
        sprintf(print_buffer,"%c",textBuffer[i]);
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", print_buffer);
    }

    javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", "\n title: ");
    for (i=0; i<titleLength; i++) {
        sprintf(print_buffer,"%c",title[i]);
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", print_buffer);
    }

    javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", "\n inputMode: ");
    for (i=0; i<inputModeLength; i++) {
        sprintf(print_buffer,"%c",inputMode[i]);
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", print_buffer);
    }

    sprintf(print_buffer,
        "\n textBufferLength=%d \n \
        textBufferMaxLength=%d \n \
        caretPos=%d \n \
        titleLength=%d \n \
        constraint=%d \n \
        modifiers=%d \n \
        keyCode=%d\n",
        *textBufferLength,
        textBufferMaxLength,
        *caretPos,
        titleLength,
        constraint,
        modifiers,
        keyCode);
    javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", print_buffer);

    return JAVACALL_TEXTFIELD_COMMIT;
}

/**
 * Invoke the device phonebook application.
 * Display to the user a list of all phonebook entries
 *
 * @retval <tt>JAVACALL_WOULD_BLOCK</tt> in case the phonebook will be invoked
 * @retval <tt>JAVACALL_FAIL</tt> in case the phonebook cannot be invoked
 */
javacall_result /* OPTIONAL */ javacall_textfield_display_phonebook() {
    javautil_debug_print (JAVACALL_LOG_INFORMATION, "input", "displaying phonebook\n");

    return JAVACALL_FAIL;
}

/**
 * Initiate a voice call to the given phone number.
 * Used to initiate a voice call from a TextField object containing a
 * PHONENUMBER constraint, by pressing the TALK key when the field is focused.
 * The platform must pause the VM before initiating the phone call, and resume
 * the VM after the call.
 *
 * This function call returns immediately.
 * The phone number string must be copied because it will be freed after this
 * function call returns.
 *
 * @param phoneNumber the phone number to call
 * @retval <tt>JAVACALL_OK</tt> if a phone call can be made
 * @retval <tt>JAVACALL_FAIL</tt> if a phone call cannot be made
 *         or some other error occured
 */
javacall_result /* OPTIONAL */
javacall_textfield_initiate_voicecall(const char* phoneNumber) {
    return JAVACALL_FAIL;
}
