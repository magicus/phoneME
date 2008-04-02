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

package components;
import jcc.Util;
import jcc.Str2ID;
import jcc.Const;

public abstract
class ClassMemberInfo extends ClassComponent {
    public int		 access;
    public int		 nameIndex;
    public int		 typeIndex;
    public UnicodeConstant name;
    public UnicodeConstant type;
    public ClassInfo	 parent;
    private int	 	 ID;
    private boolean 	 computedID = false;

    public int		 index;		// used by in-core output writers

    public int 		 flags;		// used by member loader
    public final static int INCLUDE	= 1; // a flag value.

    public ClassMemberInfo( int n, int t, int a, ClassInfo p ){
	nameIndex = n;
	typeIndex = t;
	access    = a;
	parent    = p;
	flags	  = INCLUDE; // by default, we want everything.
    }

    public boolean isStaticMember( ){
	return ( (access & Const.ACC_STATIC) != 0 );
    }

    public boolean isPrivateMember( ){
	return ( (access & Const.ACC_PRIVATE) != 0 );
    }

    public void
    resolve( ConstantObject table[] ){
	if ( resolved ) return;
	name     = (UnicodeConstant)table[nameIndex];
	type     = (UnicodeConstant)table[typeIndex];
	resolved = true;
    }

    public int
    getID(){
	if ( ! computedID ){
	    ID       = Str2ID.sigHash.getID( name, type );
	    computedID = true;
	}
	return ID;
    }

    public void
    countConstantReferences( ){
	if ( name != null ) name.incReference();
	if ( type != null ) type.incReference();
    }

    public void
    externalize( ConstantPool p ){
	name = (UnicodeConstant)p.add( name );
	type = (UnicodeConstant)p.add( type );
    }

    public String toString(){
	if ( resolved ){
	    return Util.accessToString(access)+" "+name.string+" : "+type.string;
	} else {
	    return Util.accessToString(access)+" [ "+nameIndex+" : "+typeIndex+" ]";
	}
    }
    public String qualifiedName(){
	if ( resolved ){
	    return name.string+":"+type.string;
	}else{
	    return Util.accessToString(access)+" "+parent.className+" [ "+nameIndex+" : "+typeIndex+" ]";
	}
    }

    /**
     * Returns true if this is a member that can be safely renamed:
     * e.g., it's a private or package protected method, and 
     * it's not native.
     */
    public boolean mayBeRenamed() {
        if ((access & (Const.ACC_PUBLIC | Const.ACC_PROTECTED)) == 0) {
            /* it's a private or package protected method */
            if ((access & Const.ACC_NATIVE) == 0) {
                return true;
            }
        }
        return false;
    }
}
