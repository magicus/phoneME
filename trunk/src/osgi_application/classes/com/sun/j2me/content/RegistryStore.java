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

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import com.sun.j2me.content.ContentHandlerImpl.Handle;
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

    static final Vector emptyVector = new Vector();
    static final ContentHandlerImpl[] emptyHandlersArray = new ContentHandlerImpl[0]; 

    /**
     * Search flags for @link getHandler() method. 
     */
    static final int SEARCH_EXACT   = 0; /** Search by exact match with ID */
    static final int SEARCH_PREFIX  = 1; /** Search by prefix of given value */

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

	public static void setSecurityToken(Token securityToken) {
		// required by shared code
	}
	
    /**
     * Registers given content handler.
     * @param contentHandler content handler being registered.
     * @return true if success, false - otherwise.
     */
    static Handle register( int suiteId, String classname, ContentHandlerPersistentData handlerData ) {
    	return AppProxy.registry.register( suiteId, classname, handlerData );
    }

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
    static boolean unregister(String handlerId) {
    	return AppProxy.registry.unregister( handlerId );
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
    	HandlersCollection collection = new HandlersCollection();
    	AppProxy.registry.enumHandlers( new HandlerNameFilter( testID, collection ) );
        ContentHandlerImpl[] result = collection.getArray();
        if(AppProxy.LOGGER != null){
			AppProxy.LOGGER.println( "conflictedHandlers for '" + testID + "' [" + result.length + "]:" );
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
     *        values.
     * @param value Searched value
     * @return found handlers array.
     */
    static void enumHandlers(String callerId, int searchBy, String value,
    									ContentHandlerImpl.Handle.Receiver r ) {
        /* Check value for null */
        value.length();
        HandlerAccessFilter accessFilter = new HandlerAccessFilter( callerId, r );
        switch( searchBy ){
        	case Handle.FIELD_TYPES:
    			AppProxy.registry.enumHandlers( new HandlerTypeFilter( value, accessFilter ) );
        		break;
        	case Handle.FIELD_ACTIONS:
    			AppProxy.registry.enumHandlers( new HandlerActionFilter( value, accessFilter ) );
        		break;
        	case Handle.FIELD_SUFFIXES:
    			AppProxy.registry.enumHandlers( new HandlerSuffixFilter( value, accessFilter ) );
        		break;
    		default:
    			throw new InternalError( "unexpected value of searchBy argument " + searchBy );
        }
    }
    
    static ContentHandlerImpl[] findHandler(String callerId, int searchBy, 
    											String value) {
    	HandlersCollection collection = new HandlersCollection();
    	enumHandlers( callerId, searchBy, value, collection );
    	return collection.getArray();
    }

    /**
     * The special finder for exploring handlers registered by the given suite.
     * @param suiteId explored suite Id
     *
     * @return found handlers array.
     */
    static ContentHandlerImpl[] forSuite(int suiteId) {
    	HandlersCollection collection = new HandlersCollection();
    	AppProxy.registry.enumJavaHandlers( 
    			new HandlerSuiteIDFilter( suiteId, collection ) );
    	return collection.getArray();
    }

    /**
     * Returns all stored in the Registry values for specified field.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS, 
     *        @link FIELD_ID values.
     * @return found values array.
     */
    static String[] getValues(String callerId, final int searchBy) {
    	final Hashtable/*<String, null>*/ strings = new Hashtable();
    	if( searchBy == Handle.FIELD_ID ){
	    	AppProxy.registry.enumHandlers( 
	    			new HandlerAccessFilter( callerId, 
	    					new ContentHandlerImpl.Handle.Receiver(){
								public void push(ContentHandlerImpl.Handle handle) {
									strings.put(handle.getID(), null);
								}} ));
    	} else {
	    	AppProxy.registry.enumHandlers( 
	    			new HandlerAccessFilter( callerId, 
	    					new ContentHandlerImpl.Handle.Receiver(){
								public void push(ContentHandlerImpl.Handle handle) {
									ContentHandlerImpl impl = handle.get();
									String[] values = null;
									switch( searchBy ){
										case Handle.FIELD_TYPES:
											values = impl.getTypes();
											break;
										case Handle.FIELD_ACTIONS:
											values = impl.getActions();
											break;
										case Handle.FIELD_SUFFIXES:
											values = impl.getSuffixes();
											break;
									}
									if( values != null )
										for( int i = 0; i < values.length;)
											strings.put(values[i++], null);
								}} ));
    	}
    	String[] result = new String[ strings.size() ];
    	Enumeration keys = strings.keys();
    	for( int i = 0; i < result.length;)
    		result[ i++ ] = (String)keys.nextElement();
    	return result;
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
    	ContentHandlerImpl handler = null;
    	try {
	    	AppProxy.registry.enumHandlers( 
	    			new HandlerAccessFilter( callerId, 
	    					new HandlerNameFinder( id, searchMode == SEARCH_EXACT ) ) );
    	} catch(HandlerNameFinder.FoundException x){
    		handler = x.handle.get();
    	}
    	return handler;
    }

}

