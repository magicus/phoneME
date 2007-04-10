/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import javax.microedition.content.ContentHandlerException;

/**
 * Standalone Registry Storage manager.
 * All protected methods, which are all static, redirect their work
 * to alone instance allowed for given Java runtime (for MIDP
 * it is Isolate).
 * Teh standalone instance initializes resources in the private
 * constructor and then releases its in the native finalizer.
 */
class RegistryStore {

    /**
     * Content Handler fields indexes.
     * <BR>Used with functions: @link findHandler(), @link getValues() and 
     * @link getArrayField().
     * <BR> They should match according enums in jsr211_registry.h
     */
    static final int FIELD_ID         = 0;  /** Handler ID */
    static final int FIELD_TYPES      = 1;  /** Types supported by a handler */
    static final int FIELD_SUFFIXES   = 2;  /** Suffixes supported */
                                            /** by a handler */
    static final int FIELD_ACTIONS    = 3;  /** Actions supported */
                                            /** by a handler */
    static final int FIELD_LOCALES    = 4;  /** Locales supported */
                                            /** by a handler */
    static final int FIELD_ACTION_MAP = 5;  /** Handler action map */
    static final int FIELD_ACCESSES   = 6; /** Access list */
    static final int FIELD_COUNT      = 7; /** Total number of fields */

    /**
     * Search flags for @link getHandler() method. 
     */
    static final int SEARCH_EXACT   = 0; /** Search by exact match with ID */
    static final int SEARCH_PREFIX  = 1; /** Search by prefix of given value */

    /**
     * Handler flags constants.
     * <BR> They should match according enums in jsr211_registry.h
     */
    static final private int FLAG_ERROR    = -1; /* Indicates error during */
                                                 /*    native call */

    /**
     * Result codes for launch0() native method.
     * These values should correspond to enum type jsr211_launch_result
     * in the 'jsr211_registry.h'.
     */
    /** OK, handler started */
    static final private int LAUNCH_OK                = 0;
    /** OK, handler started or is ready to start, invoking app should exit. */
    static final private int LAUNCH_OK_SHOULD_EXIT    = 1;
    /** ERROR, not supported */
    static final private int LAUNCH_ERR_NOTSUPPORTED  = -1;
    /** ERROR, no requested handler */
    static final private int LAUNCH_ERR_NO_HANDLER    = -2;
    /** ERROR, no invocation queued for requested handler */
    static final private int LAUNCH_ERR_NO_INVOCATION = -3;
    /** common error */
    static final private int LAUNCH_ERROR             = -4;

    /**
     * Registers given content handler.
     * @param contentHandler content handler being registered.
     * @return true if success, false - otherwise.
     */
    static boolean register(ContentHandlerImpl contentHandler) {
        return store.register0(contentHandler);
    }

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
    static boolean unregister(String handlerId) {
        return store.unregister0(handlerId);
    }

    /**
     * Tests ID value for registering handler accordingly with JSR claim:
     * <BR><CITE>Each content handler is uniquely identified by an ID. ...
     * <BR> The ID MUST NOT be equal to any other registered handler ID.
     * <BR> Every other ID MUST NOT be a prefix of this ID.
     * <BR> The ID MUST NOT be a prefix of any other registered ID. </CITE>
     * @param testID tested value
     *
     * @return conflicted handlers array.
     */
    static ContentHandlerImpl[] findConflicted(String testID) {
        return findHandler(null, FIELD_ID, testID);
    }

    /**
     * Searchs coontent handlers by searchable fields values. As specified in
     * JSR 211 API:
     * <BR><CITE> Only content handlers that this application is allowed to
     * access will be included. </CITE> (in result).
     * @param callerId ID value to check access
     * @param searchBy indicator of searchable field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS 
     *        values. The special case for the testId implementation: 
     *        @link FIELD_ID specified.
     * @param value Searched value
     * @return found handlers array.
     */
    static ContentHandlerImpl[] findHandler(String callerId, int searchBy, 
                                                String value) {
        /* Check value for null */
        value.length();
        String res = store.findHandler0(callerId, searchBy, value);
        return deserializeCHArray(res);
    }

    /**
     * The special finder for exploring handlers registered by the given suite.
     * @param suiteId explored suite Id
     *
     * @return found handlers array.
     */
    static ContentHandlerImpl[] forSuite(int suiteId) {
        String res = store.forSuite0(suiteId);
        return deserializeCHArray(res);
    }

    /**
     * Returns all stored in the Registry values for specified field.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS, 
     *        @link FIELD_ID values.
     * @return found values array.
     */
    static String[] getValues(String callerId, int searchBy) {
        String res = store.getValues0(callerId, searchBy);
        return deserializeStrArray(res);
    }

    /**
     * Returns array field
     * @param handlerId ID for access check
     * @param fieldId index of field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS
     *        @link FIELD_LOCALES, @link FIELD_ACTION_MAP, @link FIELD_ACCESSES
     *        valiues.
     * @return array of values
     */
    static String[] getArrayField(String handlerId, int fieldId) {
        String res = store.loadFieldValues0(handlerId, fieldId);
        return deserializeStrArray(res);
    }

    /**
     * Creates and loads handler's data.
     * @param handlerId ID of content handler to be loaded.
     * @param searchMode ID matching mode. Used <ul>
     *      <li> @link SEARCH_EXACT
     *      <li> @link SEARCH_PREFIX </ul>
     *
     * @return loaded ContentHandlerImpl object or
     * <code>null</code> if given handler ID is not found in Registry database.
     */
    static ContentHandlerImpl getHandler(String callerId, String id, 
                                                            int searchMode) {
        if (id.length() == 0) {
            return null;
        }

        return deserializeCH(store.getHandler0(callerId, id, searchMode));
    }

    /**
     * The special finder for acquiring handler by its suite and class name.
     * @param suiteId explored suite Id
     * @param classname requested class name.
     *
     * @return found handler or <code>null</code> if none found.
     */
    static ContentHandlerImpl getHandler(int suiteId, String classname) {
        ContentHandlerImpl[] arr = forSuite(suiteId);
        ContentHandlerImpl handler = null;

        if (classname.length() == 0)
            throw new IllegalArgumentException("classname can't be emty");

        if (arr != null) {
            for (int i = 0; i < arr.length; i++) {
                if (classname.equals(arr[i].classname)) {
                    handler = arr[i];
                    break;
                }
            }
        }

        return handler;
    }

    /**
     * Starts native content handler.
     * @param handler Content handler to be executed.
     * @return true if invoking app should exit.
     * @exception ContentHandlerException if no such handler ID in the Registry
     * or native handlers execution is not supported.
     */
    static boolean launch(ContentHandlerImpl handler)
                                            throws ContentHandlerException {
        int result = store.launch0(handler.getID());
        if (result < 0) {
            throw new ContentHandlerException(
                        "Unable to launch platform handler",
                        ContentHandlerException.NO_REGISTERED_HANDLER);
        }

        return (result == LAUNCH_OK_SHOULD_EXIT);
    }

    /**
     * Returns content handler suitable for URL.
     * @param callerId ID of calling application.
     * @param URL content URL.
     * @param action requested action.
     * @return found handler if any or null.
     */
    static ContentHandlerImpl getByURL(String callerId, String url, 
                                       String action) {
        return deserializeCH(store.getByURL0(callerId, url, action));
    }

    /**
     * Transforms serialized form to array of Strings.
     * <BR>Serialization format is the same as ContentHandlerImpl
     * used.
     * @param str String in serialized form to transform to array of Strings.
     * @return array of Strings. If input String is NULL 0-length array
     * returned. ... And we believe that input string is not misformed.
     */
    private static String[] deserializeStrArray(String str) {
        int n;          // array length
        String[] arr;   // result array

        n = (str == null || str.length() == 0)? 0: (int)str.charAt(0);

        arr = new String[n];
        if (n > 0) {
            int len;    // String len
            int pos;    // current position

            pos = 1;
            for (int i = 0; i < n; i++) {
                len = (int)str.charAt(pos++);
                arr[i] = str.substring(pos, pos+len);
                pos += len;
            }
        }

        return arr;
    }

    /**
     * Restores ContentHandler main fields (ID, suite_ID, class_name and flag) 
     * from serialized form to ContentHandlerImpl object.
     * @param str ContentHandler main data in serialized form.
     * @return restored ContentHandlerImpl object or null
     */
    private static ContentHandlerImpl deserializeCH(String str) {
        ContentHandlerImpl ch = null;

        while (str != null && str.length() > 0) {
            String id;
            String class_name;
            int beg = 0, end;

            end = str.indexOf('\n', beg);
            if (end == -1) {
                break; // no 1-st delimiter
            }
            id = str.substring(beg, end);
            if (id.length() == 0) {
                break; // ID is significant field
            }
            beg = end+1;

            end = str.indexOf('\n', beg);
            if (end == -1 || str.length() != end + 4) {
                break; // no 2-nd delimiter or wrong length of the string
            }
            class_name = str.substring(beg, end++);

            ch = new ContentHandlerImpl();
            ch.ID = id;
            ch.classname = class_name;
            ch.storageId = str.charAt(end++);
            ch.storageId <<= 16;
            ch.storageId |= str.charAt(end++);
            ch.registrationMethod = str.charAt(end);

            break;
        }
        return ch;
    }

    /**
     * Restores ContentHandlerImpl array from serialized form.
     * @param str ContentHandlerImpl array in serialized form.
     * @return restored ContentHandlerImpl array
     */
    private static ContentHandlerImpl[] deserializeCHArray(String str) {
        String[] strs;
        ContentHandlerImpl[] arr;

        strs = deserializeStrArray(str);
        arr = new ContentHandlerImpl[strs.length];
        for (int i = 0; i < strs.length; i++) {
            arr[i] = deserializeCH(strs[i]);
        }

        return arr;
    }

    /** Singleton instance. Worker for the class static methods. */
    private static RegistryStore store = new RegistryStore();

    /**
     * Private constructor for the singleton storage class.
     * If ClassNotFoundException is thrown during ActionNameMap
     * loading the constructor throws RuntimeException
     */
    private RegistryStore() {
        try {
            Class.forName("javax.microedition.content.ActionNameMap");
        } catch (ClassNotFoundException cnfe) {
            throw new RuntimeException(cnfe.getMessage());
        }
        if (!init()) {
            throw new RuntimeException("RegistryStore initialization failed");
        }
    }

    /**
     * Native implementation of <code>findHandler</code>.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field.
     * @param value searched value
     * @return found handlers array in serialized form.
     */
    private native String findHandler0(String callerId, int searchBy,
                                        String value);

    /**
     * Native implementation of <code>findBySuite</code>.
     * @param suiteId explored suite Id
     * @return handlers registered for the given suite in serialized form.
     */
    private native String forSuite0(int suiteId);

    /**
     * Native implementation of <code>getValues</code>.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field.
     * @return found values in serialized form.
     */
    private native String getValues0(String callerId, int searchBy);

    /**
     * Loads content handler data.
     * @param callerId ID value to check access.
     * @param id Id of required content handler.
     * @param mode flag defined search mode aplied for the operation.
     * @return serialized content handler or null.
     */
    private native String getHandler0(String callerId, String id, int mode);

    /**
     * Loads values for array fields.
     * @param handlerId ID of content handler ID.
     * @param fieldId fieldId to be loaded.
     * @return loaded field values in serialized form.
     */
    private native String loadFieldValues0(String handlerId, int fieldId);

    /**
     * Returns content handler suitable for URL.
     * @param callerId ID of calling application.
     * @param URL content URL.
     * @param action requested action.
     * @return ID of found handler if any or null.
     */
    private native String getByURL0(String callerId, String url, String action);

    /**
     * Starts native content handler.
     * @param handlerId ID of the handler to be executed
     * @return result status:
     * <ul>
     * <li> LAUNCH_OK or LAUNCH_OK_SHOULD_EXIT if content handler
     *   started successfully
     * <li> other code from the LAUNCH_ERR_* constants set
     *   according to error codition
     * </ul>
     */
    private native int launch0(String handlerId);

    /**
     * Initialize persistence storage.
     * @return <code>true</code> or
     * <BR><code>false</code> if initialization fails.
     */
    private native boolean init();

    /**
     * Cleanup native resources.
     */
    protected native void finalize();

    /**
     * Registers given content handler.
     * @param contentHandler content handler being registered.
     * @return true if success, false - otherwise.
     */
    private native boolean register0(ContentHandlerImpl contentHandler);

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
    private native boolean unregister0(String handlerId);

}

