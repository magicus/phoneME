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


package com.sun.ukit.jaxp;

import java.util.Hashtable;

class DTD {
	String name; // doctype name
	ExternalID externalID = ExternalID.EMPTY;
	Hashtable/*<Name, Contentspec>*/ elements = new Hashtable(); 
	Hashtable/*<elemQName, Hashtable<attQName, AttDef>>*/ attLists = new Hashtable();
	Hashtable/*<URI, Hashtable<localName, ref to attList element>>*/ attListsRef = new Hashtable();
	Hashtable parameterEntities = new Hashtable();
	Hashtable generalEntities = new Hashtable();
	Hashtable/*<Name, ExternalID>*/ notations = new Hashtable();
	
	static class ExternalID {
		public String pubidLiteral, systemLiteral; // ExternalID?
		// if pubidLiteral == null && systemLiteral == null => ExternalID is absent
		// if pubidLiteral == null => ExteralID :: 'SYSTEM' SystemLiteral
		// else ExteralID :: 'PUBLIC' PubidLiteral [SystemLiteral]

		public static final ExternalID EMPTY = new ExternalID();
	}
	
	static interface Contentspec {
		final int typeEMPTY = 0;
		final int typeANY = 1;
		final int typeMixed = 2;
		final int typeChildrenName = 3;
		final int typeChildrenChoice = 4;
		final int typeChildrenSeq = 5;
		int type();
	}
	
	static class Mixed implements Contentspec {
		public int type() { return typeMixed; }
		public void add(String name) {
			// add to list head
			names = new NamesList( name, names );
		}
		boolean finishWithAsterisk = false;
		NamesList names = null;
	}
	
	static abstract class Cp implements Contentspec {
		char modifier = '\0'; // valid values are '\0', '?', '+', '*'
	}
	static class CpName extends Cp {
		CpName( String name ) { this.name = name; }
		public int type() { return typeChildrenName; }
		void add(Cp cp) { /* internal error */ };
		String name;
	}
	static class CpList extends LinkedList {
		Cp cp;
		
		CpList( Cp cp, CpList next ){
			this.cp = cp;
			this.next = next;
		}
	}
	static abstract class ChoiceOrSeq extends Cp {
		void add(Cp cp) {
			// add to the list head
			list = new CpList(cp, list);
		};
		void invertChildrenOrder(){
			list = (CpList)LinkedList.invert(list);
		}
		CpList list = null;
	}
	static class Choice extends ChoiceOrSeq {
		public int type() { return typeChildrenChoice; }
	}
	static class Seq extends ChoiceOrSeq {
		public int type() { return typeChildrenSeq; }
	}
	
	static class AttDef {
		String localName;	// attribute local name
		String attType;		// attribute type see [54] AttType
		Hashtable/*<String, String>*/ enumeratedTypeValues;	// values list values for [57] EnumeratedType 
		char defaultDeclType = ddtDEFAULT;	// [60]
		String defaultDeclValue;
		
		final public static String typeNMTOKEN = "NMTOKEN";
		final public static char ddtIMPLIED = 'I';  
		final public static char ddtREQUIRED = 'R';
		final public static char ddtFIXED = 'F';
		final public static char ddtDEFAULT = 'D';
		
		public void clean() {
			localName = null;
			attType = null;
			defaultDeclType = ddtDEFAULT;
			if(enumeratedTypeValues != null) enumeratedTypeValues.clear();
			defaultDeclValue = null;
		}
	}
	
	final static Contentspec EMPTY = new Contentspec() {
		public int type() { return typeEMPTY; }
	};
	final static Contentspec ANY = new Contentspec() {
		public int type() { return typeANY; }
	};
	
	void clean() {
		name = null;
		externalID = ExternalID.EMPTY;
		elements.clear(); 
		attLists.clear();
		attListsRef.clear();
		parameterEntities.clear();
		generalEntities.clear();
		notations.clear();
	}

	Hashtable getAttList(String elementQName) {
		Hashtable attList = (Hashtable)attLists.get(elementQName);
		if( attList == null ){
			attList = new Hashtable();
			attLists.put(elementQName, attList);
		}
		return attList;
	}

	void addAttList(String URI, String localName, Hashtable attList) {
		Hashtable ns2elm = (Hashtable)attListsRef.get( URI );
		if( ns2elm == null ){
			ns2elm = new Hashtable();
			attListsRef.put( URI, ns2elm );
		}
		// what should we do if ns2elm.containsKey(QName.local(elmqn))?  
		ns2elm.put(localName, attList);
	}
	
	Hashtable findAttList(String namespaceURI, String elementLocalName) {
		Hashtable ns2elm = (Hashtable)attListsRef.get(namespaceURI);
		if( ns2elm != null )
			return (Hashtable)ns2elm.get(elementLocalName);
		return null;
	}
}
