/*
 * @(#)MemberNameTriple.java	1.3 06/10/10
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

/*
 * Encapsulate identifying information for this ClassMember,
 * in order to easily be able to match methods and fields that
 * we want to exclude from ROMization.
 */
public class MemberNameTriple {
    public String clazz;  // Classname
    public String name;   // method or field name
    public String type;   // always null if a field
        
    public MemberNameTriple( String cls, String nm, String typ ) {
        clazz = cls;
        name = nm;
        type = typ;
    }

    public boolean sameMember( String classname, String membername,
                               String  typestring ) {
        if (!clazz.equals(classname))
            return false;
        if (!name.equals(membername))
            return false;
        // check for type and typestring null
        // as for fields.
        if (type == typestring)
            return true;
        // else we're not comparing fields.
        return type.equals(typestring);
    }

    public String toString() {
        StringBuffer result = new StringBuffer( clazz );
        result.append('.');
        result.append(name);
        if (type != null) {
            result.append(type);
        }
        return result.toString();
    }
} 
