/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * Standalone Registry Storage manager.
 * All protected methods, which are all static, redirect their work
 * to alone instance allowed for given Java runtime (for MIDP
 * it is Isolate).
 * The standalone instance initializes resources in the private
 * constructor and then releases its in the native finalizer.
 */
class RegistryStore implements RegistryGate {
	
    static final Vector emptyVector = new Vector();
    static final ContentHandlerImpl[] emptyHandlersArray = new ContentHandlerImpl[0]; 

	public ContentHandlerImpl.Data register(ApplicationID appID,
										ContentHandlerRegData handlerData) {
        if( !register0(CLDCAppID.from(appID).suiteID, CLDCAppID.from(appID).className, 
        						handlerData) )
        	return null;
        return new ContentHandlerImpl.Data(handlerData.ID, appID, handlerData.registrationMethod );
	}

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
	public boolean unregister(String handlerId) {
        return unregister0(handlerId);
    }

	public void enumHandlers(String callerId, int fieldId, String value,
						ContentHandlerImpl.Handle.Receiver output) {
		ContentHandlerImpl[] result = findHandler(callerId, fieldId, value);
		for( int i = 0; i < result.length; i++)
			output.push(result[i].handle);
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
	public ContentHandlerImpl[] findConflicted(String testID) {
        ContentHandlerImpl[] result = findHandler(null, FIELD_ID, testID);
        if(Logger.LOGGER != null){
			Logger.LOGGER.println( "conflictedHandlers for '" + testID + "' [" + result.length + "]:" );
			for( int i = 0; i < result.length; i++){
				Logger.LOGGER.println( "app = " + result[i].applicationID + ", ID = '" + result[i].ID + "'" );
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
     * @param fieldId indicator of searchable field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS 
     *        values. The special case for the testId implementation: 
     *        @link FIELD_ID specified.
     * @param value Searched value
     * @return found handlers array.
     */
    public ContentHandlerImpl[] findHandler(String callerId, int fieldId, 
                                                String value) {
        /* Check value for null */
        value.length();
        HandlersCollection collection = new HandlersCollection();
        deserializeCHArray(findHandler0(callerId, fieldId, value), collection);
        return collection.getArray(); 
    }

    /**
     * The special finder for exploring handlers registered by the given suite.
     * @param suiteId explored suite Id
     *
     * @return found handlers array.
     */
    public ContentHandlerImpl[] forSuite(int suiteId) {
        HandlersCollection collection = new HandlersCollection();
        deserializeCHArray(forSuite0(suiteId), collection);
        return collection.getArray(); 
    }
    
    public ContentHandlerImpl.Data getHandler( ApplicationID appID ){
    	if( Logger.LOGGER != null )
    		Logger.LOGGER.println("RegistryStore.getHandler(" + appID + ")");
    	ContentHandlerImpl.Data result = null;
        ContentHandlerImpl[] arr = forSuite(CLDCAppID.from(appID).suiteID);
        String classname = CLDCAppID.from(appID).className;
        for (int i = 0; result == null && i < arr.length; i++) {
            if (classname.equals(CLDCAppID.from(arr[i].applicationID).className)) {
                result = arr[i].getHandlerData();
            }
        }
    	if( Logger.LOGGER != null )
    		Logger.LOGGER.println("RegistryStore.getHandler(" + appID + ") = " + result);
        return result;
    }

    /**
     * Returns all stored in the Registry values for specified field.
     * @param callerId ID value to check access
     * @param fieldId index of searchable field. Allowed: 
     *        @link FIELD_TYPES, @link FIELD_SUFFIXES, @link FIELD_ACTIONS, 
     *        @link FIELD_ID values.
     * @return found values array.
     */
    public String[] getValues(String callerId, int fieldId) {
        String res = getValues0(callerId, fieldId);
        Vector v = deserializeString(res);
        String[] result = new String[ v.size() ];
        v.copyInto(result);
        if( Logger.LOGGER != null ){
        	StringBuffer b = new StringBuffer();
        	for( int i = 0; i < result.length; i++)
        		b.append( ", '" + result[ i ] + "'" );
        	Logger.LOGGER.println("getValues(" + fieldId + ") = {" + 
        			b.toString().substring(1)+ " }");
        }
        return result;
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
    public String[] getHandlerValues(String handlerId, int fieldId) {
        String res = loadFieldValues0(handlerId, fieldId);
        Vector v = deserializeString(res);
        String[] result = new String[ v.size() ];
        v.copyInto(result);
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
    public ContentHandlerImpl.Data findHandler(String callerId, String id, int searchMode) {
        if (id.length() != 0) {
        	return deserializeCH( getHandler0( callerId, id, searchMode ) );
        }
    	return null;
    }

	public ContentHandlerImpl.Data getHandlerData(String handlerID) {
		return deserializeCH( getHandler0( null, handlerID, SEARCH_EXACT ) );
	}

	public int selectSingleHandler(ContentHandlerRegData[] list, String action) {
		String locale = System.getProperty("microedition.locale");

		String pairs[] = new String[ list.length * 2 ];
		for( int i = 0; i < list.length; i++){
			pairs[ i * 2 ] = list[ i ].getID();
			pairs[ i * 2 + 1 ] = list[ i ].getActionName(action, locale);
		}
		return selectSingleHandler0(pairs);
	}

    /**
     * Transforms serialized form to array of Strings.
     * <BR>Serialization format is the same as ContentHandlerImpl
     * used.
     * @param str String in serialized form to transform to array of Strings.
     * @return array of Strings. If input String is NULL 0-length array
     * returned. ... And we believe that input string is not misformed.
     */
    private static Vector/*<String>*/ deserializeString(String str) {
    	if( str == null )
    		return emptyVector;
    	Vector result = new Vector();
    	// all lengths in bytes
    	int pos = 0;
//        if(Logger.LOGGER != null) 
//        	Logger.LOGGER.println( "deserializeString: string length = " + str.length() );
    	while( pos < str.length() ){
    		int elem_length = (int)str.charAt(pos++) / 2;
//            if(Logger.LOGGER != null) 
//            	Logger.LOGGER.println( "deserializeString: pos = " + pos + 
//            							", elem_length = " + elem_length );
    		result.addElement(str.substring(pos, pos + elem_length));
//            if(Logger.LOGGER != null)
//            	Logger.LOGGER.println( "deserializeString: '" + str.substring(pos, pos + elem_length) + "'" );
    		pos += elem_length;
    	}
        return result;
    }

    /**
     * Restores ContentHandler main fields (ID, suite_ID, class_name and flag) 
     * from serialized form to ContentHandlerImpl object.
     * @param str ContentHandler main data in serialized form.
     * @return restored ContentHandlerImpl object or null
     */
    private static ContentHandlerImpl.Data deserializeCH(String str) {
        if(Logger.LOGGER != null) 
        	Logger.LOGGER.println( "RegistryStore.deserializeCH '" + str + "'");
        Vector components = deserializeString(str);

        if (components.size() < 1) return null;
        String id = (String)components.elementAt(0);
        if (id.length() == 0) return null; // ID is significant field

        if (components.size() < 2) return null;
        String storageId = (String)components.elementAt(1);

        if (components.size() < 3) return null;
        String class_name = (String)components.elementAt(2);

        if (components.size() < 4) return null;
        int regMethod = Integer.parseInt((String)components.elementAt(3), 16);

        return new ContentHandlerImpl.Data( id, new CLDCAppID( Integer.parseInt(storageId, 16), class_name ), regMethod );
    }

    /**
     * Restores ContentHandlerImpl array from serialized form.
     * @param str ContentHandlerImpl array in serialized form.
     * @return restored ContentHandlerImpl array
     */
    private static void deserializeCHArray(String str,
    						ContentHandlerImpl.Handle.Receiver output) {
    	if( str != null ){
	        Vector strs = deserializeString(str);
	        for (int i = 0; i < strs.size(); i++)
	        	output.push(new ContentHandlerHandle(deserializeCH( (String)strs.elementAt(i) )));
    	}
    }

    /** Singleton instance. Worker for the class static methods. */
    private static RegistryGate store = new RegistryStore();

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
		if( Logger.LOGGER != null )
			Logger.LOGGER.println("RegistryStore has created");
    }
    
    public static RegistryGate getInstance(){
    	return store;
    }

    /**
     * Native implementation of <code>findHandler</code>.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field.
     * @param value searched value
     * @return found handlers array in serialized form.
     */
    private static native String findHandler0(String callerId, int searchBy,
                                        String value);

    /**
     * Native implementation of <code>findBySuite</code>.
     * @param suiteId explored suite Id
     * @return handlers registered for the given suite in serialized form.
     */
    private static native String forSuite0(int suiteId);

    /**
     * Native implementation of <code>getValues</code>.
     * @param callerId ID value to check access
     * @param searchBy index of searchable field.
     * @return found values in serialized form.
     */
    private static native String getValues0(String callerId, int searchBy);

    /**
     * Loads content handler data.
     * @param callerId ID value to check access.
     * @param id Id of required content handler.
     * @param mode flag defined search mode applied for the operation.
     * @return serialized content handler or null.
     */
    private static native String getHandler0(String callerId, String id, int mode);

    /**
     * Loads values for array fields.
     * @param handlerId ID of content handler ID.
     * @param fieldId fieldId to be loaded.
     * @return loaded field values in serialized form.
     */
    private static native String loadFieldValues0(String handlerId, int fieldId);

    /**
     * Selects one of handlers passed as the <code>list</code> array
     * @param list array of pairs (handlerID, action name)
     * @return index of the selected handler
     */
    private static native int selectSingleHandler0(String[] pairs);
    
    /**
     * Initialize persistence storage.
     * @return <code>true</code> or
     * <BR><code>false</code> if initialization fails.
     */
    private static native boolean init();

    /**
     * Cleanup native resources.
     */
    private static native void finalize();

    /**
     * Registers given content handler.
     * @param contentHandler content handler being registered.
     * @return true if success, false - otherwise.
     */
    private static native boolean register0(int storageId, String classname,
											ContentHandlerRegData handlerData);

    /**
     * Unregisters content handler specified by its ID.
     * @param handlerId ID of unregistered handler.
     * @return true if success, false - otherwise.
     */
    private static native boolean unregister0(String handlerId);
}
