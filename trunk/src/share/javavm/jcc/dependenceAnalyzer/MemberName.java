/*
 * @(#)MemberName.java	1.13 06/10/10
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

/*
 * A member name has 3 parts:
 * 1) a pointer to the ClassEntry for the class holding the member.
 * 2) the member name
 * 3) a method's signature.
 * The last two are concatenated into the name field. There is
 * apparently little value in separating them.
 *
 * The only value to a signature is to resolve overloading.
 * Since data fields do not overload, we do not carry a signature
 * with them. And since even methods only overload based on parameter
 * list, we do not need to carry the return-type part of their signatures.
 *
 * We construct these for ease of comparison, as we will often
 * be doing lookup operations.
 */

package dependenceAnalyzer;
import util.Localizer;

public class MemberName implements Cloneable {
    public ClassEntry	classEntry;
    public String	name; // and type signature

    public MemberName( ClassEntry c, String nm ){
	classEntry = c;
	name = nm.intern();
    }

    public boolean equals ( Object other ){
	try {
	    MemberName otherName = (MemberName)other;
	    return (
		(this.classEntry == otherName.classEntry) && 
		(this.name == otherName.name) );
	} catch ( ClassCastException e ){
	    return false;
	}
    }

    public int hashCode(){
	return name.hashCode();
    }

    public Object clone(){ 
	try {
	    MemberName newName = (MemberName) super.clone();
	    return newName;
	}catch( CloneNotSupportedException e ){
	    e.printStackTrace();
	    throw new Error(Localizer.getString("membername.could_not_clone"));
	}
    }

    public String toString(){
	String result;
	if ( classEntry == null ){
	    result = "<noclass>";
	} else {
	    result = classEntry.name().toString();
	}
	result += "."+name;
	return result;
    }
}
