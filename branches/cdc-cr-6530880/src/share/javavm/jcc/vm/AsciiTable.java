/*
 * @(#)AsciiTable.java	1.12 06/10/10
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
 *
 */
package vm;
import components.*;
import java.util.Hashtable;
import java.util.Vector;
import Util;

/*
 * There are two string-like data types in today's JDK:
 * 1) zero-terminated, C-language, ASCII strings
 * 2) Java Strings.
 *
 * The former arise from:
 * (a) UTF encoding of Java class, method, field names and type
 *     signatures, used for linkage
 * (b) UTF encoded forms of Java String constants, used as keys
 *     for the intern-ing of said constants.
 *
 * This class takes a collection of ASCII strings (collected using addString)
 * and finds those strings which are suffixes of other, longer strings in
 * the collection. This allows some sharing of data when we produce a list
 * of zero-terminated ASCII ( i.e. asciz ) strings as part of the runtime
 * data.
 *
 * For example, the string "a" can be represented as a pointer to the end
 * of the string "ba".
 */

public class AsciiTable
{
    PostFixTreeElement tree = null;
    Hashtable stringHash = new Hashtable();
    Vector table = null;

    // Add a string to the list
    public void addString(UnicodeConstant str) {
	if ( str.UTFstring == null ){
		str.UTFstring = Util.unicodeToUTF( str.string );
	}
	PostFixTreeElement elem = new PostFixTreeElement(str);
	tree = PostFixTreeElement.insert(tree, elem);
	stringHash.put( str.string, str );
    }

    public UnicodeConstant
    getConstant( String key ){
	return (UnicodeConstant)stringHash.get( key );
    }

    // Go through all of the strings that have been added and find
    // those that can be stored as a pointer to a postfix.
    public void computeAsciiTable() {
	table = tree.produceTable( );
    }

    // Get the whole table, as for printing.
    public Vector getTable(){
	return table;
    }

    // Ordering function for tree.  The comparison is done using
    // the end of the strings, working backwards.
    static public boolean lessThan(UnicodeConstant aConst, UnicodeConstant bConst) {
	String a = aConst.UTFstring;
	String b = bConst.UTFstring;
	int aLen = a.length();
	int bLen = b.length();

	for (int i = 1; i <= aLen && i <= bLen; i++) {
	    if (a.charAt(aLen - i) < b.charAt(bLen - i)) {
		return true;
	    } else if (a.charAt(aLen - i) > b.charAt(bLen - i)) {
		return false;
	    }
	}

	return aLen <= bLen;
    }
}


class PostFixTreeElement {
    private UnicodeConstant str;
    private PostFixTreeElement left = null, right = null;
    //public int  tindex = -1; 
    
    public PostFixTreeElement(UnicodeConstant a) {
	str = a;
    }

    static public PostFixTreeElement insert(PostFixTreeElement t, 
					    PostFixTreeElement elem) {
	if (t == null) {
	    return elem;
	} else if (AsciiTable.lessThan(elem.str, t.str)) {
	    t.left = insert(t.left, elem);
	} else {
	    t.right = insert(t.right, elem);
	}
	return t;
    }

    public Vector produceTable( ) { 
	Vector t = new Vector();
	PostFixTreeElement last = produceTable(null, t );
	return t;
    }

    // This function assigns an index to each of the AsciiConstant strings, 
    // while printing out the tree.
    // 
    // In the following, we talk about "r-alphabetic" ordering.  This is
    // alphabetic lexicogrphic sorting, but starting from the end of the string.
    // Our tree is a binary tree in which everything in the left node is
    // r-alphabetically less than the root, and everything in the right node
    // is r-alphabetically greater than the root.
    //
    // The following code makes use of the fact that if a string is a suffix of
    // any other string in the list, then it is certainly of suffc of the next
    // string r-alphabetically.
    //
    // So we just go through the tree in reverse r-alphabetic order.  As we 
    // look at each item, the variable "last" contains the next greater string
    // r-alphabetically. Either the current string is a suffix of "last", or
    // else it can't be a suffix of any string whatsoever in the tree.
    //
    // The return value is the left-most node (i.e. smallest ralphabetically)
    // of the subtree.
    private PostFixTreeElement produceTable(PostFixTreeElement last, Vector t ) { 
	// Look at the items on the right subtree.
	if (right != null) { 
	    last = right.produceTable(last, t); 
	}
	int index = 0;
	if (last != null) { 
	    // assume not a suffix
	    index = last.str.stringTableOffset + last.str.UTFstring.length() + 1;
	    if (last.str.UTFstring.endsWith(this.str.UTFstring)) { 
		index -= (this.str.UTFstring.length() + 1);
		this.str.isSuffix = true;
	    }
	} 
	t.addElement( this.str );
	if (!this.str.isSuffix) {
	    last = this;
	} 
	this.str.stringTableOffset = index;
	if (left != null) { 
	    last = left.produceTable(last, t);
	}
	return last;
    }
}
