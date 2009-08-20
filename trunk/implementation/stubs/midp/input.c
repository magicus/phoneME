/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 
#ifdef __cplusplus
extern "C" {
#endif
    
#include "javacall_input.h"

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
javacall_result javacall_input_get_locale(/*OUT*/ char locale[6]) {
    //set to "en-US"
    locale[0]='e';
    locale[1]='n';
    locale[2]='-';
    locale[3]='U';
    locale[4]='S';
    locale[5]='\0';
    return JAVACALL_OK;
}

/**
 * Launches a native text editor enabling editing of native locale languages
 * The text editor may possibly display initial text.
 * 
 * 
 * Handling of "VM-Pause" while a native textbox is displayed:
 * i. In asynchronous case:
 * Prior to sending a VM-Pause event, the platform is responsible for 
 *          (1) closing the textbox, 
 *          (2) sending a "cancel" event 
 *          
 *
 * ii. In synchronous case:
 * Prior to sending a VM-Pause event, the function should return immediately 
 * with return code JAVACALL_TEXTFIELD_CANCEL.
 * 
 * @param textBuffer a pointer to a global input text parameter 
 *  that holds the text string in unicode characters (UTF-16).
 *	When launching, the native editor should use the existing text in the 
 *	buffer as initial text (the first textBufferLength charachters), and should 
 *	update the buffer when the native editor is quit.
 * @param textBufferLength a pointer a global parameter that will hold the 
 *  current length of text, in unicode charachters.
 *	The native editor should update this value upon editor 
 *	quitting to the actual size of the input text.
 * @param textBufferMaxLength maximum length of input text. the user should not
 *	be allowed to exceed it.
 * @param caretPos a pointer to a global variable that holds the current 
 *  position of the caret in the text editor,in unicode charachters.
 *	The native editor should update this value upon quitting the editor 
 *  to the new caret location.
 * @param title a pointer to the title of the text editor, or null if a title 
 *	should not be displayed
 * @param titleLength length of title, in unicode charachters, or 0 if there is
 *	no title 
 * @param inputMode implementation of this feature is optional in MIDP2.0 specification.
 *	a pointer to a name of a Unicode character subset, which identifies a 
 *	particular input mode which should be used when the text editor is launched.
 *	the input mode is not restrictive, and the user is allowed to change it 
 *	during editing. For example, if the current constraint is 
 *	JAVACALL_TEXTFIELD_CONSTRAINT_ANY, and the requested input mode is "JAVACALL_TEXTFIELD_UPPERCASE_LATIN",
 *	then the initial input mode is set to allow  entry of uppercase Latin 
 *	characters. the user will be able to enter other characters by switching 
 *	the input mode to allow entry of numerals or lowercase Latin letters.
 *	for more details, reffer to MIDP2.0 specification.
 * @param inputModeLength length of inputMode, in unicode charachters, or 0 if 
 *	there is no inputMode
 * @param constraint Input constraint, as defined in the <i>MIDP 2.0 Specification</i>
 *  possible values are:
 *		JAVACALL_TEXTFIELD_CONSTRAINT_ANY
 *		JAVACALL_TEXTFIELD_CONSTRAINT_EMAILADDR
 *		JAVACALL_TEXTFIELD_CONSTRAINT_NUMERIC     
 *		JAVACALL_TEXTFIELD_CONSTRAINT_PHONENUMBER 
 *		JAVACALL_TEXTFIELD_CONSTRAINT_URL         
 *		JAVACALL_TEXTFIELD_CONSTRAINT_DECIMAL
 * @param modifiers Input modifiers, as defined in the <i>MIDP 2.0 Specification</i>
 *	a bit-wise OR of zero or more of the following modifiers can be set:
 *		JAVACALL_TEXTFIELD_MODIFIER_PASSWORD
 *		JAVACALL_TEXTFIELD_MODIFIER_UNEDITABLE
 *		JAVACALL_TEXTFIELD_MODIFIER_SENSITIVE
 *		JAVACALL_TEXTFIELD_MODIFIER_NON_PREDICTIVE
 *		JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_WORD
 *		JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_SENTENCE 
 * @param keyCode the first key that the user pressed that should be inserted to the text box
 *
 * @return should return one of the following statuses:
 *	JAVACALL_TEXTFIELD_COMMIT if the function is synchronous and the user pressed 
 *	"ok" to commit the text and quit the editor
 *	JAVACALL_TEXTFIELD_CANCEL if the function is synchronous and the user pressed 
 *	"cancel" to cancel the editing and quit the editor
 *	JAVACALL_TEXTFIELD_ERROR if the operation failed, or if the native editor 
 *	could not be launched
 *	JAVACALL_TEXTFIELD_WOULDBLOCK if the function is asynchronous, an 
 *	event/notification will be sent upon editor quitting. when the 
 *	evnt/notofocation is later sent, it should contain info about the editor 
 *	termination manner, can be one of:JAVACALL_TEXTFIELD_COMMIT, JAVACALL_TEXTFIELD_CANCEL, 
 *	or JAVACALL_TEXTFIELD_ERROR, as descibed above. 
 */
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

    return JAVACALL_TEXTFIELD_ERROR;
}

/**
 * Invoke the device phonebook application.
 * Display to the user a list of all phonebook entries
 *  
 * @retval <tt>JAVACALL_WOULD_BLOCK</tt> in case the phonebook will be invoked
 * @retval <tt>JAVACALL_FAIL</tt> in case the phonebook cannot be invoked
 */
javacall_result /* OPTIONAL */ javacall_textfield_display_phonebook() {
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

#ifdef __cplusplus
}
#endif

