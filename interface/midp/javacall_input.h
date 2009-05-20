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
#ifndef __JAVACALL_INPUT_H
#define __JAVACALL_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_input.h
 * @ingroup Input
 * @brief Javacall interfaces for locale and text
 */
    
#include "javacall_defs.h"

/**
 * @defgroup Input Input and Textfield API
 * @ingroup JTWI
 *
 * The document describes in detail the porting API specifications for the following interfaces :
 * - Input locale
 * - Textfield native input
 * - T9 
 * - Key press
 *
 * Input locate allows Java to inquire which language is currently enabled on the device. 
 * Textfield native input describes the mechanism to allow user text input using the same interface 
 * look and feel interface used by the device UI.
 */

/**
 * @defgroup InputLocale Input Locale API
 * @ingroup Input
 *
 * @{
 */

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
 * The language codes MUST be the lower-case, two-letter codes as defined 
 * by ISO-639:
 * http://ftp.ics.uci.edu/pub/ietf/http/related/iso639.txt
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
javacall_result javacall_input_get_locale(/*OUT*/ char locale[6]);

/** @} */

/**
 * @defgroup MandatoryText Native Text Field API
 * @ingroup Input
 *
 * An implementation may wish to integrate the platform's native text editor, 
 * instead of using the default Java textField. 
 * The benefit from such integration lay in reusing existing native editor functionality such as 
 * Predictive text entry (such as T9) and multiple languages support 
 * (for example Korean, Chinese, or Japanese input support). <br>
 * Another advantage in using the platform's editor is a homogeneous user experience and consistent Look&Feel 
 * across various software application on the target device.
 * @{
 */

/**
 * @enum javacall_textfield_constraint
 * @brief Text field input constraints 
 *
 * Input constraints, as defined in the <i>MIDP 2.0 Specification</i>.
 * The different constraints allow the application to request that the user's 
 * input be restricted in a variety of ways. The implementation is required 
 * to restrict the user's input as requested by the application. 
 * For example, if the application requests the NUMERIC constraint on a 
 * TextField, the implementation must allow only numeric characters to be entered.	
 */
typedef enum {
    /** The user is allowed to enter any text. Line breaks may be entered. */
    JAVACALL_TEXTFIELD_CONSTRAINT_ANY         = 0,
    /** The user is allowed to enter an e-mail address. */
    JAVACALL_TEXTFIELD_CONSTRAINT_EMAILADDR   = 1,
    /** The user is allowed to enter only an integer value. a minus sign is allowed */
    JAVACALL_TEXTFIELD_CONSTRAINT_NUMERIC     = 2,
    /** The user is allowed to enter a phone number. The exact set of characters 
   * allowed is specific to the device and may include non-numeric characters,
   * such as a "+" prefix character. */
    JAVACALL_TEXTFIELD_CONSTRAINT_PHONENUMBER = 3,
    /** The user is allowed to enter a URL */
    JAVACALL_TEXTFIELD_CONSTRAINT_URL         = 4,
    /** The user is allowed to enter numeric values with optional decimal fractions.
   * period ".", minus sign "-", and decimal digits are allowed */
    JAVACALL_TEXTFIELD_CONSTRAINT_DECIMAL     = 5
} javacall_textfield_constraint;
    
/**
 * Input modifiers are flag bits that modify the behavior of text entry and 
 * display, as defined in the <i>MIDP 2.0 Specification</i>.
 * a bit-wise OR operator can be used to combine multiple modifiers
 */
    
/** 
 * Indicates that the text entered is confidential data that should be obscured
 * whenever possible.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_PASSWORD				0x10000

/**  
 * Indicates that editing is currently disallowed.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_UNEDITABLE				0x20000
	 	
/** 
 * Indicates that the text entered is sensitive data that the implementation
 * must never store into a dictionary or table for use in predictive, 
 * auto-completing, or other accelerated input schemes.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_SENSITIVE				0x40000

/**
 * Indicates that the text entered does not consist of words that are likely
 * to be found in dictionaries typically used by predictive input schemes.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_NON_PREDICTIVE			0x80000

/** 
 * This flag is a hint to the implementation that during text editing, 
 * the initial letter of each word should be capitalized.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_WORD		0x100000

/** 
 * This flag is a hint to the implementation that during text editing, 
 * the initial letter of each sentence should be capitalized.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_SENTENCE	0x200000 

/** 
 * This flag is a hint to the implementation that before text editing, 
 * the initial text should be deleted.
 */
#define JAVACALL_TEXTFIELD_MODIFIER_DELETE_INITIAL_TEXT		0x400000 
    
/**
 * @enum javacall_textfield_status
 * @brief Textfield status notification
 */
typedef enum {
    JAVACALL_TEXTFIELD_COMMIT     =3000,
    JAVACALL_TEXTFIELD_CANCEL     =3001,
    JAVACALL_TEXTFIELD_ERROR      =3002,
    JAVACALL_TEXTFIELD_WOULDBLOCK =3003
} javacall_textfield_status;

/**
 * Launches a native text editor enabling editing of native locale languages
 * The text editor may possibly display initial text.
 * 
 * Handling of "VM-Pause" while a native textbox is displayed:
 * i. In asynchronous case:
 * Prior to sending a VM-Pause event, the platform is responsible for 
 *          (1) closing the textbox, 
 *          (2) sending a "cancel" event 
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
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_ANY
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_EMAILADDR
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_NUMERIC     
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_PHONENUMBER 
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_URL         
 *		- JAVACALL_TEXTFIELD_CONSTRAINT_DECIMAL
 * @param modifiers Input modifiers, as defined in the <i>MIDP 2.0 Specification</i>
 *	a bit-wise OR of zero or more of the following modifiers can be set:
 *		- JAVACALL_TEXTFIELD_MODIFIER_PASSWORD
 *		- JAVACALL_TEXTFIELD_MODIFIER_UNEDITABLE
 *		- JAVACALL_TEXTFIELD_MODIFIER_SENSITIVE	
 *		- JAVACALL_TEXTFIELD_MODIFIER_NON_PREDICTIVE
 *		- JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_WORD	
 *		- JAVACALL_TEXTFIELD_MODIFIER_INITIAL_CAPS_SENTENCE 
 *		- JAVACALL_TEXTFIELD_MODIFIER_DELETE_INITIAL_TEXT
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
        /*IN*/ int                              keyCode);  

/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/**
 * A callback function to be called by platform for notification of  
 * completion of native textbox editing.
 * The platfrom will invoke the call back in platform context.
 *
 * The callback will be handed operation result code, which can be 
 * @param status the status of textbox editing completion
 *     <tt>JAVACALL_TEXTFIELD_COMMIT</tt> if the user pressed "ok" to 
 *     commit the text and quit the editor.
 *     In this case the following parameters sent to function 
 *     javacall_textfield_launch( ) will be updated to the reflect the 
 *     text the user edited: 
 *     (1) textBuffer (2) textBufferLength (3) caretPos
 *     <tt>JAVACALL_TEXTFIELD_CANCEL</tt> if the user pressed "cancel" 
 *     and closed the editor without updating the text.
 *     <tt>JAVACALL_TEXTFIELD_ERROR</tt> IF the operation failed.
 *     In this case The platform will hide the native textbox .
 */
void javanotify_textfield_complete(javacall_textfield_status status);

/** @} */

/**
 * @defgroup OptionalPhonebook Phonebook and Phonecall API
 * @ingroup Input
 *
 * NOTE: The following functions are optional.
 * 
 * The following functions enable the user to browse the device phonebook
 * from a Java TextField with TextField.PHONENUMBER constraint and select 
 * a number to be entered into the textfield.
 * The user can then make a call to this number from Java.
 * 
 * @{
 */

/**
 * Invoke the device phonebook application.
 * Display to the user a list of all phonebook entries
 *  
 * @retval <tt>JAVACALL_WOULD_BLOCK</tt> in case the phonebook will be invoked
 * @retval <tt>JAVACALL_FAIL</tt> in case the phonebook cannot be invoked
 */
javacall_result /* OPTIONAL */ javacall_textfield_display_phonebook(void);

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
javacall_textfield_initiate_voicecall(const char* phoneNumber);

/** @} */

/**
 * @defgroup PhonebookNotification Notification API for Phonebook
 * @ingroup Input
 * 
 * The event notification that the platform must invoke when a user selects a
 * phone number from the phonebook when the phonebook was invoked from Java.
 * The notification function is invoked from the platform task.
 * 
 * @{
 */

/**
 * Notify Java that the user has made a selection
 * 
 * @param phoneNumber a string representing the phone number the user selected.
 *                    The string will be copied so it can be freed after this
 *                    function call returns.
 *                    In case the user dismissed the phonebook without making a
 *                    selection, this sting is <tt>NULL</tt>
 */
void /* OPTIONAL */ 
javanotify_textfield_phonenumber_selection(char* /* OUT */ phoneNumber);

/** @} */

#ifdef __cplusplus
}
#endif

#endif 

