/*
 *
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

/**
 * @defgroup highui_nim Native Input Mode Support
 * @ingroup highui
 */
/**
 * @file
 * @ingroup highui
 *
 * Porting API for Native Input Modes (NIMs)
 *
 * A number of input modes is implemented in Java. On the other hand,
 * the platform may also support some input modess, either unique
 * or analogous to those implemented in Java. The purpose of this API
 * is to let MIDP use input modes implemented on platform.
 * Please note that there's a separate API for Predictive Text Input (PTI).
 *
 * <b>Multiple native input modes: how.</b>
 * Native Input Modes are represented by instances of the NativeInputMode
 * class. In case that there is more than one native input mode, the field
 * NativeInputMode.id may be used to determine which one is meant.
 * NativeInputMode.id is an integer that may be used as a switch-case
 * selector expression, or as an index into an array of C++ objects that
 * implement different input methods, or in some other way.
 *
 * The id of a native input mode MUST be equal to or greater
 * than the NATIVE_INPUT_MODE_START value.
 *
 * <b>Where to keep data.</b>
 * The NativeInputMode.instanceData field may store a pointer to a memory
 * area containing the native input mode data. Native functions may modify
 * that field, for example, storing there an address of some allocated
 * memory area containing data.
 *
 * <b>Getting id and data.</b>
 * All functions that implement NativeInputMode member functions receive
 * the following two parameters: jint id, jint* pInstanceData.
 * The value id is a copy of NativeInputMode.id, and pInstanceData
 * is the address of the NativeInputMode.instanceData field
 * (within a Java object).
 *
 * <b>Workaround to call Java; the Java wrapper.</b>
 * Sometimes a native function will need to call Java functions.
 * It is complex to achieve this directly.
 * The following workaround is used instead:<br>
 * - the native function that needs to call Java code has a Java wrapper,
 * a Java member function in the same class;<br>
 * - the native function may be called only via its wrapper;<br>
 * - instead of calling Java code, the native function returns control
 * to its Java wrapper, and the wrapper calls the required Java function,
 * and then calls the native function again;<br>
 * - a state_data array is used to pass information between the native
 * function and its Java wrapper, and between subsequent invocations
 * of the native function;<br>
 * - the native function cannot use automatic memory for its local
 * variables, because such variables have lifetime of one invocation,
 * while a lifetime of several subsequent invocactions is needed;<br>
 * -  instead of using automatic memory, the native function has to explicitly
 * allocate a structure containing all its local variables and store
 * its address into the state_data array; the last invocation MUST
 * free this structure.
 *
 * <b>Functions calling Java as finite-state machines.</b>
 * Such native function may be considered a state machine.
 * One integer in the state_data array is intended for the state id.
 * In the simplest case, state id is the invocation number
 * (there will be, say, 5 states: 0,1,2,3,4).
 * In a bit more complex case, state id is not necessarily incremented
 * at each invocation: for example, the state 3 may be followed by
 * either state 4 or state 2.
 *
 * <b>0 as finite-state machine stopper.</b>
 * 0 is a special state id: the wrapper function repeats invocations of the
 * native function until state is is 0. Initially, the state id is 0,
 * so if the native function does not modify this value, it gets called
 * only once.
 *
 * <b>What information is passed to/from wrapper and from one invocation
 * to another.</b>
 * The state_data array (used for repeated invocations) has room for:<br>
 * - an argument for a Java function (STATE_INT_ARG);<br>
 * - result from the Java function (STATE_CALLBACK_RES);<br>
 * - an integer id showing which function is meant (STATE_FUNC_TOKEN);<br>
 * - an integer indicating which state the native function is
 * in (STATE_NEXT_STATE, the name comes from the fact that this
 * value is set/read before the corresponding state is entered);<br>
 * - the result to be returned by the Java wrapper
 * function (STATE_FINAL_RES);<br>
 * - pointers to data allocated by the native
 * function (STATE_INTERNAL and STATE_INTERNAL_EXT).<br>
 * For layout of state_data array, see nim_process_key().
 *
 * <b>Why need to call Java functions. Mediator.</b>
 * Input mode and input session objects communicate via
 * the InputModeMediator interface. Each input mode has
 * a <code>InputModeMediator mediator</code> instance member
 * and communicates to the session by calling <code>mediator</code>'s
 * functions. In particular, an instance of NativeInputMode,
 * whose functions are implemented in C, also must call InputModeMediator
 * functions. But these are the only Java functions that need be called.
 *
 * <b>Java functions that may be executed.</b>
 * The set of Java functions whose execution may be forced from native code
 * is limited to a few functions on the NativeInputMode.mediator object.
 * See @ref function_tokens.
 *
 * <b>Passing values to Java functions.</b>
 * First of all, the only native function that interacts with Java in this way
 * is nim_process_key().<br>
 * - If the Java function does not take any arguments, you don't
 * have to pass anything.<br>
 * - If the function takes one integer argument, store it into the state_data
 * array at index STATE_INT_ARG.<br>
 * - If the function takes one String argument, nim_process_key must store
 * it to the output pcsl_string parameter and return the value 1.<br>
 * With the current set of invocable Java functions, one int argument
 * and/or one String argument is enough.
 *
 * <b>Getting return values from Java functions.</b>
 * The integer return value may be found in the state_data
 * array at index STATE_CALLBACK_RES. If the return function is of type
 * void, you don't need any return value. And with the current set
 * of invocable Java functions there's no need to support other types
 * of return values.
 *
 * <b>Execution of Java functions and re-invocation of the native function
 * are independent.</b>
 * The "function token" integer (its index is STATE_FUNC_TOKEN) is
 * initially set to 0, which means "no operation", and gets reset
 * to 0 after each execution of a mediator function.
 * Re-invocation of the native function does not depend on whether or not
 * a mediator function is to be executed, in particular, the last invocation
 * also may order execution of a mediator function.
 *
 */


/**
 * @name ISMAP dimensions
 * The dimensions of the isMap
 * (it maps pairs [input subset, constraint] onto booleans).
 * @{
 */
/** number or rows; row number selects, for example, MIDP_UPPERCASE_LATIN  */
#define NIM_CONSTRAINT_MAP_NROWS 12
/** number or columns; column number selects, for example, EMAILADDR */
#define NIM_CONSTRAINT_MAP_NCOLS 6
/** @} */

/** @name java input mode IDs
 * @{
 */
#define KEYBOARD_INPUT_MODE  1
#define NUMERIC_INPUT_MODE  2
#define ALPHANUMERIC_INPUT_MODE  3
#define PREDICTIVE_TEXT_INPUT_MODE  4
#define SYMBOL_INPUT_MODE  5
#define VIRTUAL_INPUT_MODE 6
#define NATIVE_INPUT_MODE_START  100
/** @} */

/** @name indices that select items in the state_data aray
 * See nim_process_key() for a description of state_data array elements.
 * @{
 */
#define STATE_CALLBACK_RES 0
#define STATE_FUNC_TOKEN 1
#define STATE_NEXT_STATE 2
#define STATE_INT_ARG 3
#define STATE_FINAL_RES 4
#define STATE_INTERNAL 5
#define STATE_INTERNAL_EXT 6
#define STATE_DATA_ARRAY_SIZE 7
/** @} */

/**
 * @anchor function_tokens
 * @name function tokens
 *  integer IDs that select one of functions to be executed
 * @{
 */
#define MEDIATOR_NOOP 0
#define MEDIATOR_COMMIT 1
#define MEDIATOR_CLEAR 2
#define MEDIATOR_SUBINPUTMODECHANGED 3
#define MEDIATOR_INPUTMODECOMPLETED 4
#define MEDIATOR_ISCLEARKEY 5
#define MEDIATOR_GETAVAILABLESIZE 6
#define MEDIATOR_ISNEWLINEKEY 7
/** @} */

/** data type for a constraint map; there's one constraint map
 * for each input method, when an input method is created,
 * its values are used to initialize the corresponding array
 * in the NativeInputMethod Java object.
 */
typedef jbyte constraint_map[NIM_CONSTRAINT_MAP_NROWS][NIM_CONSTRAINT_MAP_NCOLS];

/** data type for state data; when the native function nim_process_key
 * needs to call a Java function, it stores its state into a state_data
 * array and returns; the calling function invokes the specified Java function
 * and then re-invokes nim_process_key.
 */
typedef jint state_data[STATE_DATA_ARRAY_SIZE];

/**
 * Return a pointer to the constraint_map for the specified native input mode.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @return address of a constraint_map
 */
constraint_map* nim_get_constraint_map(jint id, jint* pInstanceData);

/**
 * Initialize a NativeInputMethod instance.
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @return 0 if ok, non-zero in case of an error.
 */
jint nim_initialize(jint id, jint* pInstanceData);

/**
 * Finalizer. Must free instance data allocated in nim_initialize().
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 */
void nim_finalize(jint id, jint* pInstanceData);

/**
 * Implements NativeImputMode.supportsConstraints(int).
 * This method is called to determine if this InputMode supports
 * the given text input constraints. The semantics of the constraints
 * value are defined in the javax.microedition.lcdui.TextField API.
 * If this InputMode returns false, this InputMode must not be used
 * to process key input for the selected text component.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param constraints text input constraints. The semantics of the
 * constraints value are defined in the TextField API.
 *
 * @return true if this InputMode supports the given text component
 *         constraints, as defined in the MIDP TextField API
 */
jboolean nim_supports_constraints(jint id, jint* pInstanceData, jint constraints);
/**
 * Implements NativeInputMode.getName().
 * Returns the display name which will represent this InputMode to
 * the user, such as in a selection list or the softbutton bar.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param name output parameter, receives the display name.
 *      The calling function must use pcsl_string_free to free the string
 *      (pcsl_string_free does nothing on statically allocated literals).
 * @return error code, PCSL_STRING_OK for ok
 */
pcsl_string_status nim_get_name(jint id, jint* pInstanceData, pcsl_string* name);
/**
 * Implements NativeInputMode.getName().
 * Returns the command name which will represent this InputMode in
 * the input menu
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param name output parameter, receives the locale-appropriate name
 *         to represent this InputMode to the user
 * @return error code, PCSL_STRING_OK for ok
 */
pcsl_string_status nim_get_command_name(jint id, jint* pInstanceData, pcsl_string* name);

/**
 * Implements NativeInputMode.beginInput(InputModeMediator, String, int).
 *
 * This method will be called before any input keys are passed
 * to this InputMode to allow the InputMode to perform any needed
 * initialization. A reference to the InputModeMediator which is
 * currently managing the relationship between this InputMode and
 * the input session is passed in. This reference can be used
 * by this InputMode to commit text input as well as end the input
 * session with this InputMode. The reference is only valid until
 * this InputMode's endInput() method is called.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param inputSubset current input subset
 * @param constraints text input constraints. The semantics of the
 * constraints value are defined in the TextField API.
 */
void nim_begin_input(jint id, jint* pInstanceData, const pcsl_string* inputSubset, int constraints);
/**
 * Implements NativeInputMode.processKey(int keyCode, boolean longPress).
 *
 * This function implements the functionality of processKey, but
 * when it needs to call some mediator function, it stores the current
 * state into stateArgs  and returns; processKey calls the necessary
 * interface function and calls processKey0 again.
 *
 * stateArgs[STATE_CALLBACK_RES] -- mediator function result. <br>
 * stateArgs[STATE_FUNC_TOKEN] -- the mediator function id. <br>
 * stateArgs[STATE_NEXT_STATE] -- the integer id of the next state,
 *      the processKey repeats calling processKey0
 *      until this value becomes zero. <br>
 * stateArgs[STATE_INT_ARG] -- int argument for the mediator function,
 *                              if required. <br>
 * stateArgs[STATE_FINAL_RES] -- the result to be returned
 *      by processKey. <br>
 * stateArgs[STATE_INTERNAL] -- may be used by native code implementing
 *                              processKey0. <br>
 * stateArgs[STATE_INTERNAL_EXT] -- may be used by native code
 *                              implementing processKey0. <br>
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param key the code of the key which was pressed
 * @param longPress true if it's a long key press otherwise false
 * @param isClearKey 1 if it's a clear key, 0 if it's not,
 *          and -1 if this could not be determined because the mediator
 *          instance variable points to null.
 * @param state containsArgs state information that survives across
 *          repeated reinvocations of this function, and data
 *          to be passed to/from the mediator functions.
 * @param stringRes output variable that receives the string to be returned
 *          by NativeInputMode.processKey
 * @return string status: -1 -- string error, 0 -- no string returned, 1 -- a string is returned
 */
jint nim_process_key(jint id, jint* pInstanceData, jint key, jboolean longPress, jint isClearKey, state_data* stateArgs, pcsl_string* stringRes);

/**
 * Implements public native char NativeInputMode.getPendingChar()
 *
 * return the pending char
 * used to bypass the asynchronous commit mechanism
 * e.g. to immediately commit a char before moving the cursor
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @return return the pending char
 */
jchar nim_get_pending_char(jint id, jint* pInstanceData);

/**
 * Implements NativeInputMode.getNextMatch()
 *
 * Return the next possible match for the key input processed thus
 * far by this InputMode. A call to this method should be preceeded
 * by a check of hasMoreMatches(). If the InputMode has more available
 * matches for the given input, this method will return them one by one.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param name output parameter, receives a pcsl_string representing
 *          the next available match to the key
 *          input thus far, or 'null' if no pending input is available
 * @return error code, PCSL_STRING_OK for ok
 */
pcsl_string_status nim_get_next_match(jint id, jint* pInstanceData, pcsl_string* name);

/**
 * Implements NativeInputMode.hasMoreMatches()
 *
 * True, if after processing a key, there is more than one possible
 * match to the input. If this method returns true, the getNextMatch()
 * method can be called to return the value.
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @return true if after processing a key, there is more than the one
 *         possible match to the given input
 */
jboolean nim_has_more_matches(jint id, jint* pInstanceData);

/**
 * Implements NativeInputMode.getMatchList()
 *
 * Gets the possible string matches
 *
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 * @param match_list output valiable that receives an address of array
 *              of pcsl_strings, representing the possible matches.
 *              May be null.
 * @param nmatches output valiable that receives the number of
 *              strings in the pcsl_string array.
 * @return error code, PCSL_STRING_NULL for ok
 */

pcsl_string_status nim_get_match_list(jint id, jint* pInstanceData, pcsl_string** match_list, int* nmatches);
/**
 * Implements NativeInputMode.endInput()
 *
 * Mark the end of this InputMode's processing. The only possible call
 * to this InputMode after a call to endInput() is a call to beginInput()
 * to begin a new input session.
 * @param id the integer ID of the native input mode
 * @param pInstanceData points to an integer field that may be
 *              used as room for a pointer to instance data.
 */
void nim_end_input(jint id, jint* pInstanceData);

/*
 * Return the address of a statically allocated array containing
 * the supported native input mode ids, and its size.
 *
 * @param n output variable that receives the number of elements in the array
 *              (may be zero).
 * @return address of the array with supported input modes (may be null).
 */
jint* nim_get_input_mode_ids(jint* n);

