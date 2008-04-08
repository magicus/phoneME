/*
 *
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

package com.sun.j2me.content;

import java.util.Vector;

import com.sun.j2me.security.Token;

/**
 * Standalone Registry Storage manager.
 * All protected methods, which are all static, redirect their work
 * to alone instance allowed for given Java runtime (for MIDP
 * it is Isolate).
 * The standalone instance initializes resources in the private
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
    
    static final Vector emptyVector = new Vector();
    static final ContentHandlerImpl[] emptyHandlersArray = new ContentHandlerImpl[0]; 

    /**
     * Search flags for @link getHandler() method. 
     */
    static final int SEARCH_EXACT   = 0; /** Search by exact match with ID */
    static final int SEARCH_PREFIX  = 1; /** Search by prefix of given value */

    /** This class has a different security domain than the MIDlet suite */
    private static Token classSecurityToken;
    
    /**
     * Sets the security token used for privileged operations.
     * The token may only be set once.
     * @param token a Security token
     */
    static void setSecurityToken(Token token) {
		if (classSecurityToken != null) {
            throw new SecurityException();
        }
        classSecurityToken = token;
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
    }

    /**
     * Registers given content handler.
     * @param contentHandler content handler being registered.
     * @return true if success, false - otherwise.
     */
    static boolean register(ContentHandlerImpl contentHandler) {
    	return false;
    }

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
    static boolean unregister(String handlerId) {
    	return false;
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
        ContentHandlerImpl[] result = findHandler(null, FIELD_ID, testID);
        if(AppProxy.LOGGER != null){
			AppProxy.LOGGER.println( "conflictedHandlersfor '" + testID + "' [" + result.length + "]:" );
			for( int i = 0; i < result.length; i++){
				AppProxy.LOGGER.println( "class = '" + result[i].storageId + "', ID = '" + result[i].ID + "'" );
			}
        }
		return result;
    }

    /**
     * Searches content handlers by searchable fields values. As specified in
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
        return null;
    }

    /**
     * The special finder for exploring handlers registered by the given suite.
     * @param suiteId explored suite Id
     *
     * @return found handlers array.
     */
    static ContentHandlerImpl[] forSuite(int suiteId) {
    	return null;
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
    	return null;
    }

    /**
     * Returns array field
     * @param handlerId ID for access check
     * @param fieldId index of field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS
     *        @link FIELD_LOCALES, @link FIELD_ACTION_MAP, @link FIELD_ACCESSES
     *        values.
     * @return array of values
     */
    static String[] getArrayField(String handlerId, int fieldId) {
    	return null;
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
    static ContentHandlerImpl getHandler(String callerId, String id, int searchMode) {
    	return null;
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
     * Returns content handler suitable for URL.
     * @param callerId ID of calling application.
     * @param URL content URL.
     * @param action requested action.
     * @return found handler if any or null.
     */
    static ContentHandlerImpl getByURL(String callerId, String url, String action) {
    	return null;
    }
}

