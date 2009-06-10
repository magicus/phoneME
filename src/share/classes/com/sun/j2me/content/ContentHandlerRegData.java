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

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Vector;

import javax.microedition.content.ActionNameMap;

public class ContentHandlerRegData {
	
    /**
     * The content handler ID.
     * Lengths up to 256 characters MUST be supported.
     * The ID may be <code>null</code>.
     */
    protected String ID;
    
    /**
     * Indicates content handler registration method:
     * dynamic registration from the API, static registration from install
     * or native content handler.
     */
    protected int registrationMethod;
    /** Content handler statically/dynamically registered during installation */
    final public static int REGISTERED_STATIC_FLAG = 0x0001; // if set => statically
    public int getRegistrationMethod() { return registrationMethod; }

    /**
     * The types that are supported by this content handler.
     * If there are no types registered, the this field MUST either be
     * <code>null</code> or refer to an empty array .
     */
    protected String[] types;
    public String[] getTypes(){ return types; };

    /**
     * The suffixes of URLs that are supported by this content handler.
     * If there are no suffixes, then this field MUST be <code>null</code>.
     * The suffixes MUST include any and all punctuation. For example,
     * the string <code>".png"</code>.
     */
    protected String[] suffixes;
    public String[] getSuffixes(){ return suffixes; };

    /**
     * The actions that are supported by this content handler.
     * If there are no actions, then this field MSUT be <code>null</code>.
     */
    protected String[] actions;
    public String[] getActions(){ return actions; };

    /**
     * The action names that are defined by this content handler.
     */
    protected ActionNameMap[] actionnames;
    public String getActionName( String action, String locale ){
    	for( int i = 0; i < actionnames.length; i++){
    		if( locale.equals(actionnames[ i ].getLocale()) )
    			return actionnames[ i ].getActionName(action);
    	}
    	return null;
    }
    
    /**
     * The accessRestrictions for this ContentHandler.
     */
    protected String[] accessRestricted;
    public String[] getAccessRestricted(){ return accessRestricted; };
    
    /** Empty String array to return when needed. */
    final static String[] ZERO_STRINGS = {};

    /** Empty ActionNameMap to return when needed. */
    final static ActionNameMap[] ZERO_ACTIONNAMES = new ActionNameMap[0];

    ContentHandlerRegData(){
    }
    
    ContentHandlerRegData( ContentHandlerRegData data ){
        ID = data.ID;
        registrationMethod = data.registrationMethod;
        types = data.types;
        suffixes = data.suffixes;
        actions = data.actions;
        actionnames = data.actionnames;
        accessRestricted = data.accessRestricted;
    }
    
    /**
     * Construct a ContentHandlerPersistentData.
     * Verifies that all strings are non-null
     * @param types an array of types to register; may be
     *  <code>null</code>
     * @param suffixes an array of suffixes to register; may be
     *  <code>null</code>
     * @param actions an array of actions to register; may be
     *  <code>null</code>
     * @param actionnames an array of ActionNameMaps to register; may be
     *  <code>null</code>
     * @param ID the content handler ID; may be <code>null</code>
     * @param accessRestricted the  IDs of applications allowed access
     * @param auth application authority
     *
     * @exception NullPointerException if any types, suffixes,
     *   actions, actionnames array element is null
     *
     * @exception IllegalArgumentException is thrown if any of
     *   the types, suffix, or action strings have a
     *   length of zero or
     *   if the ID has a length of zero or contains any
     *        control character or space (U+0000-U+00020)
     */
    
    ContentHandlerRegData(int registrationMethod, 
    					String[] types, String[] suffixes,
    					String[] actions, ActionNameMap[] actionnames,
    					String ID, String[] accessRestricted) {
    	this.registrationMethod = registrationMethod;
        // Verify consistency between actions and ActionNameMaps
        if (actionnames != null && actionnames.length > 0) {
            if (actions == null) {
                throw new IllegalArgumentException("no actions");
            }
            int len = actions.length;
            for (int i = 0; i < actionnames.length; i++) {
                // Verify the actions are the same
                ActionNameMap map = actionnames[i];
                if (len != map.size()) {
                    throw new IllegalArgumentException("actions not identical");
                }

                for (int j = 0; j < len; j++) {
                    if (!actions[j].equals(map.getAction(j))) {
                        throw new
                            IllegalArgumentException("actions not identical");
                    }
                }

                /*
                 * Verify the locale of this ActionNameMap is not the same
                 * as any previous ActionNameMap.
                 */
                for (int j = 0; j < i; j++) {
                    if (map.getLocale().equals(actionnames[j].getLocale())) {
                        throw new IllegalArgumentException("duplicate locale");
                    }
                }
            }
        }

        // Check the ID for invalid characters (controls or space)
        if (ID != null) {
            int len = ID.length();
            if (len == 0)
                throw new IllegalArgumentException("invalid ID");
            for (int i = 0; i < ID.length(); i++) {
                if (ID.charAt(i) <= 0x0020) {
                    throw new IllegalArgumentException("invalid ID");
                }
            }
            this.ID = ID;
        }
        this.types = copy(types,false,true);
        this.suffixes = copy(suffixes,false,true);
        this.actions = copy(actions,true,false);
        this.actionnames = copy(actionnames);
        // access restricted callers allows duplicates
        this.accessRestricted = copy(accessRestricted,true,false);
    }
    
    public ContentHandlerRegData(DataInputStream dataIn) throws IOException {
    	ID = dataIn.readUTF();
    	registrationMethod = dataIn.readInt();
		types = readStringArray(dataIn);
		suffixes = readStringArray(dataIn);
		actions = readStringArray(dataIn);
		
		actionnames = new ActionNameMap[ dataIn.readInt() ]; 
		for( int i = 0; i < actionnames.length; i++){
			actionnames[i] = readActionNameMap( dataIn ); 
		}
    	
		accessRestricted = readStringArray(dataIn);
	}

	public void serialize(DataOutputStream dataOut) throws IOException {
		dataOut.writeUTF(ID);
		dataOut.writeInt(registrationMethod);
		serialize(types, dataOut);
		serialize(suffixes, dataOut);
		serialize(actions, dataOut);
		
    	dataOut.writeInt(actionnames.length);
		for( int i = 0; i < actionnames.length; i++){
			serialize( actionnames[i], dataOut );
		}
    	
		serialize(accessRestricted, dataOut);
	}
	
	private ActionNameMap readActionNameMap(DataInputStream dataIn) throws IOException {
    	String locale = dataIn.readUTF();
    	int size = dataIn.readInt();
    	String[] actions = new String[ size ], names = new String[ size ];
		for( int i = 0; i < size; i++){
			actions[ i ] = dataIn.readUTF();
			names[ i ] = dataIn.readUTF();
		}
		return new ActionNameMap( actions, names, locale );
	}

	private String[] readStringArray(DataInputStream dataIn) throws IOException {
		String[] result = new String[ dataIn.readInt() ];
		for( int i = 0; i < result.length; i++)
			result[ i ] = dataIn.readUTF();
		return result;
	}

    private static void serialize(ActionNameMap anm, DataOutputStream dataOut) throws IOException {
    	dataOut.writeUTF(anm.getLocale());
    	dataOut.writeInt(anm.size());
    	for( int i = 0; i < anm.size(); i++){
    		dataOut.writeUTF(anm.getAction(i));
    		dataOut.writeUTF(anm.getActionName(i));
    	}
	}

	private static void serialize(String[] strings, DataOutputStream dataOut) throws IOException {
    	dataOut.writeInt(strings.length);
    	for( int i = 0; i < strings.length; i++)
    		dataOut.writeUTF(strings[i]);
	}

	/**
     * Get the content handler ID.  The ID uniquely identifies the
     * application which contains the content handler.
     * After registration and for every registered handler,
     * the ID MUST NOT be <code>null</code>.
     * @return the ID; MUST NOT be <code>null</code> unless the
     *  ContentHandler is not registered.
     */
    public String getID() {
        return ID;
    }
    
	/**
     * Checks that all of the string references are non-null
     * and not zero length.  If either the argument is null or
     * is an empty array the default ZERO length string array is used.
     *
     * @param strings array to check for null and length == 0
     * @param caseSens assume case sensitivity when check for duplicates
     * @param skipDuplicates check for duplicates and skip them 
     * @return a non-null array of strings; an empty array replaces null
     * @exception NullPointerException if any string ref is null
     * @exception IllegalArgumentException if any string
     * has length == 0
     */
    public static String[] copy(String[] strings, boolean caseSens, boolean skipDuplicates) {
		Vector copy = new Vector();    	
		if (strings != null && strings.length > 0) {
			for (int i = 0; i < strings.length; i++) {
				if (strings[i] == null) {
					throw new NullPointerException("argument is null");
				}
				String s = strings[i];
				if (s.length() == 0) {
					throw new IllegalArgumentException("string length is 0");
				}
				if (skipDuplicates){
					Enumeration e = copy.elements();
					while (e.hasMoreElements()){
						String sprev = (String)e.nextElement();
						if (caseSens) {
							if (s.equals(sprev)) break;
						} else {
							if (s.equalsIgnoreCase(sprev)) break;
						}
					}
					if (e.hasMoreElements()) continue;
				}
				copy.addElement(s);
			}
		}
		if (copy.size()>0) {
			String result[]=new String[copy.size()];
			copy.copyInto(result);
			return result;
		}
		return ZERO_STRINGS;
	}
    /**
	 * Checks that all of the actionname references are non-null.
	 * 
	 * @param actionnames array to check for null and length == 0
	 * @return a non-null array of actionnames; an empty array replaces null
	 * @exception NullPointerException if any string ref is null
	 */
    private static ActionNameMap[] copy(ActionNameMap[] actionnames) {
        if (actionnames != null && actionnames.length > 0) {
            ActionNameMap[] copy = new ActionNameMap[actionnames.length];
            for (int i = 0; i < actionnames.length; i++) {
                // Check for null
                if (actionnames[i] == null) {
                    throw new NullPointerException();
                }
                copy[i] = actionnames[i];
            }
            return copy;
        }
        return ZERO_ACTIONNAMES;
    }

    public String toString(){
    	StringBuffer b = new StringBuffer();
    	b.append("ID = '" + ID + "'");
    	b.append(", rm = " + registrationMethod);
    	arrayToString("types", types, b);
    	arrayToString("suffixes", suffixes, b);
    	arrayToString("actions", actions, b);
    	arrayToString("access", accessRestricted, b);
    	return b.toString();
    }

	private void arrayToString(String name, String[] values, StringBuffer b) {
		if( values == null ) return;
		b.append(", " + name + "[" + values.length + "] = {");
    	for( int i = 0; i < values.length; i++)
    		b.append( " '" + values[i] + "'," );
    	b.deleteCharAt(b.length() - 1);
    	b.append(" }");
	}
}
