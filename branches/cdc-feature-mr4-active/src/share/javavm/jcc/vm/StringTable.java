/*
 * @(#)StringTable.java	1.14 06/10/10
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
 *
 */

package vm;
import jcc.Str2ID;
import components.StringConstant;
import components.UnicodeConstant;
import java.util.Hashtable;
import java.util.HashSet;
import java.util.Enumeration;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeSet;

/**
 *
 * There are two string-like data types in today's JDK:
 * 1) zero-terminated, C-language, ASCII strings
 * 2) Java Strings.
 * 
 * The former arise from:
 * (a) UTF encoding of Java class, method, field names and type
 *     signatures, used for linkage
 * (b) UTF encoded forms of Java String constants, used as keys
 *     for the intern-ing of said constants.
 * See the class AsciiStrings where these are manipulated
 * to achieve some sharing of runtime data structures.
 *
 * In this, the StringTable class, we keep track of Java Strings, in
 * the form of StringConstant's.
 * We enter them in a Str2ID structure, which will be wanted
 * at runtime. And we assign layout of the runtime char[] data.
 * Much aliasing of data is possible, since this is read-only and
 * not usually zero-terminated. We won't do any of that, for now.
 * Threre is much potential here.
 */

public class StringTable implements Comparator {
    public Str2ID	stringHash = new Str2ID();
    /*
     * This hash table is organized by string lengths. For each string length
     * the lookup finds another hash table of all strings of that length.
     */
    private Hashtable	htable = new Hashtable();
    /*
     * For every interned string, keep track of all its "copies" or "aliases"
     * We are going to have to update the unicodeIndex for all these
     * NOTE that we use a different Comparator for the alias table.
     * This is because every element we put in
     * to this table will compare as Object.equal() to any other, and
     * we need to distinguish between them using
     * System.identityHashCode() ordering.
     */
    private Hashtable	aliasTable = new Hashtable();
    private StringBuffer data;
    private int		aggregateSize;
    private int		numUniqueStrings = 0;
    private StringConstant[] stringTable;

    //
    // Comparison for sorting in ascending order of string length 
    //
    public int compare(Object o1, Object o2) {
        StringConstant obj1 = (StringConstant) o1;
        StringConstant obj2 = (StringConstant) o2;
        int len1 = obj1.str.string.length();
	int len2 = obj2.str.string.length();

        if (len1 > len2) {
	   return 1;
        } else if (len1 == len2)
           return 0;
        return -1;
    }

    /*
     * Comparison for distinuishing Objects
     */
    static class identityHashComparator implements Comparator {
	/*
	 * Note: this comparator imposes orderings that 
	 * are inconsistent with equals.
	 */
	public int compare(Object o1, Object o2) {
	    return (System.identityHashCode(o1) - System.identityHashCode(o2));
	}
    }

    private static Comparator identComparator = new identityHashComparator();

    //
    // This is a two level interning scheme, where strings with
    // the same length are grouped together.
    //
    // The top hashtable maps string lengths to the hashtables holding
    // the strings of that length.
    //
    public void intern( StringConstant s ){
	int len = s.str.string.length();
	Integer lenObj = new Integer(len);
	Hashtable lenTab = (Hashtable)htable.get( lenObj );
	StringConstant entry;

	this.stringTable = null; // New string coming in
	
	if (lenTab == null) {
	    lenTab = new Hashtable();
	    /* New hashtable for this new length */
	    htable.put(lenObj, lenTab);
	    entry = null;
	} else {
	    entry = (StringConstant)lenTab.get( s );
	}
	
	if ( entry == null ){
	    // We will be tracking all those strings that intern to be 's'
	    // We need to remember those, so that we can update their 
	    // unicodeIndex fields.
	    //
	    Set aliases = new TreeSet(identComparator);
	    aliasTable.put( s, aliases);
	    //
	    // Intern
	    //
	    lenTab.put( s, s );
	    stringHash.getID( s.str , s );
	    aggregateSize += len;
	    numUniqueStrings++;
	} else {
	    // This string was already interned. 
	    // Add this instance to list of aliases for the interned copy
	    Set aliases;
	    aliases = (Set)aliasTable.get(entry);
	    aliases.add(s);
	}
    }

    protected void fillInChars(int srcBegin, int srcEnd,
			       char dst[], int dstBegin) {
	data.getChars(srcBegin, srcEnd, dst, dstBegin);
    }
    
    //
    // Traverse the multiple hashtables holding strings, and extract
    // them to a an array of StringConstants. The strings in the resulting
    // array are grouped with like length strings, and ordered in ascending
    // string length.
    //
    public StringConstant[] allStrings(){
	if (this.stringTable != null) {
	    return this.stringTable;
	}
	this.stringTable = new StringConstant[numUniqueStrings];
	/*
	 * Now go through the hierarchical hash tables, and extract
	 * all strings. Adjacent strings have equal lengths in this
	 * table.
	 */
	Enumeration lengths = htable.keys();
	int idx = 0;
	
	while (lengths.hasMoreElements()) {
	    Integer len = (Integer)lengths.nextElement();
	    Hashtable stringsHtab = (Hashtable)htable.get(len);
	    Enumeration strings = stringsHtab.keys();
	    while (strings.hasMoreElements()) {
		StringConstant str = (StringConstant)strings.nextElement();
		this.stringTable[idx++] = str;
	    }
	}

	if (idx != numUniqueStrings) {
	    throw new InternalError("String count mismatch");
	}
	//
	// Now sort this string table in increasing order of length
	// This is for aesthetic value alone, so the output is more 
	// readable.
	//
	Arrays.sort(this.stringTable, this);
	
	return this.stringTable;
    }

    public int internedStringCount(){ return numUniqueStrings; }

    /*
     * Arrange for the "data" buffer to hold all the string bodies
     * in some form.
     * Arrange for the unicodeOffset field of each StringConstant
     * to be set to the index of the beginning of the representation
     * of its data in this array.
     */
    public int arrangeStringData() {
	/*
	 * Our initial guess is simply to concatenate all the data.
	 * Later, we can try to be cleverer.
	 */
	data = new StringBuffer( aggregateSize );
	int curOffset = 0;
	StringConstant[] sTab = allStrings();
	for (int i = 0; i < sTab.length; i++) {
	    StringConstant t = sTab[i];
	    // Update the unicodeIndex of all the aliases of this string
	    Set aliases = (Set)aliasTable.get(t);
	    Iterator aliasEnum = aliases.iterator();
	    while (aliasEnum.hasNext()) {
		StringConstant a = (StringConstant)aliasEnum.next();
		// Make sure that we have not seen this constant before
		if (a.unicodeIndex != -1) {
		    throw new InternalError("String aliases messed up");
		}
		a.unicodeIndex = i;
	    }
	    // And finally update the unicodeIndex of this string
	    t.unicodeIndex = i;
	    t.unicodeOffset = curOffset;
	    data.append( t.str.string );
	    curOffset += t.str.string.length();
	}
	if (curOffset != aggregateSize) {
	    throw new InternalError("String size mismatch");
	}
	return curOffset;
    }
}
