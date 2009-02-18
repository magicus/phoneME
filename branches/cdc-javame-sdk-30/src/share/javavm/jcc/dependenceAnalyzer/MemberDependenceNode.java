/*
 * @(#)MemberDependenceNode.java	1.12 06/10/10
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

package dependenceAnalyzer;
import util.Set;
import java.util.*;
import util.EmptyEnumeration;

public class MemberDependenceNode extends DependenceNode{

    /*
     * Flag values.
     */
    public static final int EXCLUDED		= 1;
    public static final int REQUIRED		= 2;

    public static final int STATIC		= 4;
    public static final int PRIVATE		= 8;
    public static final int METHOD		=16;
    public static final int FIELD		=32;
    public static final int INIT		=64;
    public static final int CLINIT		=128;
    public static final int OVERRIDDEN		=256;
    public static final int IMPUTED		=512;
    public static final int NATIVE		=1024;

    // a mask
    public static final int NO_OVERRIDING	= STATIC|PRIVATE|FIELD|INIT|CLINIT;

    /*
     * Constructor.
     */
    public MemberDependenceNode( Object nm, int flagval ){
	super( nm );
	flags = flagval;
    }

    /*
     * Public methods.
     */
    public
    Enumeration overrides(){ 
	if ( memberOverrides == null ) return EmptyEnumeration.instance;
	return memberOverrides.elements();
    }

    public
    Enumeration overriddenBy(){ 
	if ( memberOverriddenBy == null ) return EmptyEnumeration.instance;
	return memberOverriddenBy.elements();
    }

    /*
     * These are all package-visible parts of the implementation.
     */
    Set		memberOverriddenBy;
    Set		memberOverrides;

    public void dump( java.io.PrintStream o ){
	for ( int j = 0; j < 16; j++ ){ 
	    switch ( flags & ( 1<<j ) ){
	    case EXCLUDED: o.print("Excluded "); break;
	    case REQUIRED: o.print("Required "); break;
	    case STATIC:   o.print("static "); break;
	    case PRIVATE:  o.print("private "); break;
	    case METHOD:   o.print("method "); break;
	    case FIELD:    o.print("field "); break;
	    case INIT:	   o.print("constructor "); break;
	    case CLINIT:   o.print("static initializer "); break;
	    case OVERRIDDEN:o.print("overridden "); break;
	    case IMPUTED:  o.print("imputed "); break;
	    case NATIVE:   o.print("native "); break;
	    }
	}
	o.println( nodeName.toString() );
	if ( memberOverriddenBy != null ){
	    o.println("	is overridden "+memberOverriddenBy.size()+" times");
	}
    }

}
