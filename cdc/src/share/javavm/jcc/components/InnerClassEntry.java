/*
 * @(#)InnerClassEntry.java	1.7 06/10/10
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

package components;

import java.io.DataOutput;
import java.io.IOException;

/*
 * This class is used to represent information contained in the innerclass
 * attribute of a class file which contains one or more inner classes. The
 * innerclass attribute is new in 1.2 and is documented in the 1.2 JVM Spec.
 */
public
class InnerClassEntry {
//__________ CLASS CONSTANTS
    public static final int SIZE = 8; //bytes in .class files

//__________ INSTANCE VARIABLES
    ClassConstant   innerInfo;
    ClassConstant   outerInfo;
    UnicodeConstant innerName;
    int             accessFlags;

//__________ CONSTRUCTORS
    InnerClassEntry(ClassConstant inner_info,
                    ClassConstant outer_info,
                    UnicodeConstant inner_name,
                    int flags) {
	innerInfo   = inner_info;
	outerInfo   = outer_info;
	innerName   = inner_name;
	accessFlags = flags;
/*
if (innerName.toString().equals("")) {
    if (outerInfo != null) {
        System.err.println("1) "+outerInfo.name.toString()+" has blank innerclass name because compiled w/javac V1.1");
    } else if (innerInfo != null) {        
        System.err.println("2) "+innerInfo.name.toString()+" has blank innerclass name because compiled w/javac V1.1");
    }
}    
*/
    }

//__________ INSTANCE METHODS
    /*
     * Given an output stream, write the indexes and access flag to that stream.
     */
    public void write(DataOutput o) throws IOException {
	if (innerInfo != null) {
	    o.writeShort(innerInfo.index);
	} else {
	    o.writeShort(0);
	}
		
	if (outerInfo != null) {
	    o.writeShort(outerInfo.index);
	} else {
	    o.writeShort(0);
	}
		
	if (innerName != null) {
	    o.writeShort(innerName.index);
	} else {
	    o.writeShort(0);
	}

	o.writeShort(accessFlags);
    }
    
    /*
     * The following four methods return information about this
     * particular inner class. If the attribute index is null,
     * the method returns 0.
     */
    
    public int getInnerInfoIndex() {
	if (innerInfo != null) {
	    return innerInfo.index;
	}
	
	return 0;
    }

    public int getOuterInfoIndex() {
	if (outerInfo != null) {
	    return outerInfo.index;
	}
	
	return 0;
    }

    public int getInnerNameIndex() {
	if (innerName != null) {
	    return innerName.index;
	}
	
	return 0;
    }

    public int getAccessFlags() {
	return accessFlags;
    }
    
    /*
     * Return the name of this inner class. If this inner class does not
     * have a name, then it is an anonymous inner class, so return the
     * string "anonymous".
     */
    public String getName() {
	if (innerName != null) {
	    return innerName.toString();
	} else {
	    return "";
	}
    }

    public String getFullName() {
	if (outerInfo != null) {
	    if (getName().equals("")) {
		return "Old-Style-InnerClass";
	    } else {
		return outerInfo.name.string+"$"+getName();
	    }
	} else { 	
	    return "ICA-ReferingToSelf";
	}
    }
}
