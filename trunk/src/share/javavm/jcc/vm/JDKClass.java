/*
 * @(#)JDKClass.java	1.12 06/10/10
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
/*
 * JDK-VM-specific internal representation of
 * a class. Target-machine independent.
 * There is a references from each instance of components.ClassInfo to 
 * one of these, and a reference back as well.
 *
 * See also JDKVM for VM-specific info not associated directly with a class.
 */
import components.*;
import util.*;
import consts.Const;
import Util;
import Str2ID;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.util.StringTokenizer;

public class
JDKClass extends ClassClass {
    protected int		typeCode = 0;
    public JDKMethodInfo	methods[];

    public JDKClass( ClassInfo c ){
	ci = c;
	c.vmClass = this;

	if ( c.methods != null ){
	    // per-method, JDK-specific info.
	    methods = new JDKMethodInfo[ c.methods.length ];
	    for( int i = 0; i < methods.length; i++ ){
		components.MethodInfo m = c.methods[i];
		methods[i] = new JDKMethodInfo( m );
		if (!hasStaticInitializer &&
		    m.isStaticMember() &&
		    m.name.string.equals(/*NOI18N*/"<clinit>")) {
		    hasStaticInitializer = true;
		}
	    }
	}
    }


    /*
     * In JDK1.1, the implementation of reflection expects
     * certain typecodes to be stuffed in the cbTypeCode field
     * of certain classes. In un-modified JDK, the reflection
     * implementation stuffs this code there itself. But this
     * requires writing in the classClass data structure, which
     * we'd like to make read-only! Thus we do the equivalent
     * job here, during image building.
     *
     */
    public int
    typecode(){
	return typeCode;
    }

    public boolean
    adjustSymbolicConstants( Hashtable missingClasses ){
	ConstantObject consts[] = ci.constants;
	if (!isPartiallyResolved(consts)) {
	    return true;
	}
	//
	// we have work to do. This is unfortunate.
	// we use a LocalConstantPool to manage the pool we're re-building.
	// Because order matters, we collect the new entries to add at end.
	//
	ci.constants = makeClassResolvable(consts, missingClasses ).getConstants(); 
	impureConstants = true;
	return false;
    }


    static ConstantPool makeResolvable(ConstantObject[] consts, Hashtable missingClasses ) { 

        LocalConstantPool newPool    = new LocalConstantPool();
        Vector            newEntries = new Vector();
        int               nconst = consts.length;
        for( int i = 1; i < nconst; i += consts[i].nSlots ){
            ConstantObject o;
            o = consts[i];
            newPool.append(o);
            if ( ! o.isResolved() )
                newEntries.addElement( o );
        }
        Enumeration newlist = newEntries.elements();
        while( newlist.hasMoreElements() ){
            ConstantObject o = (ConstantObject)newlist.nextElement();
	    String className;
            switch( o.tag ){
            case Const.CONSTANT_CLASS:
                ClassConstant co = (ClassConstant)o;
                System.err.println(Localizer.getString("classclass.class", co.name.string));
                co.name = (UnicodeConstant)newPool.add( co.name );
                className = co.name.string;
		missingClasses.put( className, className );
                continue;
            case Const.CONSTANT_FIELD:
            case Const.CONSTANT_METHOD:
            case Const.CONSTANT_INTERFACEMETHOD:
                FMIrefConstant fo = (FMIrefConstant)o;
                if ( fo.clas.isResolved() ){
                    // This is a missing member of a resolved class.
                    // Print this out
                    // To print all the references to a totally missing
                    // class would be redundant and noisy.
                    if ( fo.tag == Const.CONSTANT_FIELD ){
                        System.err.print(Localizer.getString("classclass.field"));
                    } else {
                        System.err.print(Localizer.getString("classclass.method"));
                    }
                    System.err.println(Localizer.getString(
					"classclass.of_class", 
					fo.sig.name.string, 
					fo.sig.type.string, 
					fo.clas.name.string));
                }
                // as NameAndTypeConstant entries are always "resolved",
                // the strings they nominally point to need not be present:
                // they will get written out as a hash ID.
                fo.sig = (NameAndTypeConstant)newPool.add( fo.sig );
                fo.clas.name = (UnicodeConstant)newPool.add( fo.clas.name );
                fo.clas = (ClassConstant)newPool.add( fo.clas );
                className = fo.clas.name.string;
		missingClasses.put( className, className );
                continue;
            }
        }
	newPool.impureConstants = true;
	return newPool;
    }
    /* Inline all the methods in this code object */
    public void getInlining() { 
        for( int i = 0; i < methods.length; i++ ){
	    methods[i].getInlining();
	}
    }
}

/*
 * Like a constant pool, but with simpler semantics.
 * Perhaps this and components.ConstantPool should be related.
 * The important difference is in "add", which never shares
 * and which always clones if it must insert!
 * Also append, which is even simpler, as it assumes that we're
 * loading up this constant pool from an already-existing list.
 */
final
class LocalConstantPool extends components.ConstantPool {

    public LocalConstantPool(){
	super();
    }
    /**
     * Return the ConstantObject in constant table corresponding to
     * the given ConstantObject s.
     * Duplicates and inserts s if it is not already there.
     * The index member of the returned value (which
     * will not be the object s!) will be set to its index in our
     * table. There will be no element of index 0.
     */
    public ConstantObject
    add( ConstantObject s ){
	ConstantObject r = (ConstantObject)h.get( s );
	if ( r == null ){
	    r = (ConstantObject)s.clone();
	    r.index = n;
	    n += r.nSlots;
	    h.put( r, r );
	    t.addElement( r );
	    for ( int i =r.nSlots; i > 1; i-- )
		t.addElement( null ); // place holder.
	}
	r.references+=1;
	return r;
    }

    public void
    append( ConstantObject s ){
	if ( h.get( s ) != null ){
	    throw new Error(Localizer.getString(
			    "classclass.append.error_already_in_pool",
			     s));
	}
	if ( s.index != n ){
	    throw new Error(Localizer.getString(
			    "classclass.append.error_out_of_order",
			     s));
	}
	h.put( s, s);
	t.addElement( s );
	s.references+=1;
	for ( int i =s.nSlots; i > 1; i-- )
	    t.addElement( null ); // place holder.
	n += s.nSlots;
    }
}
