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

package vm;
/*
 * VM-specific internal representation of
 * a class. Target-machine independent.
 * There is a references from each instance of components.ClassInfo to 
 * one of these, and a reference back as well.
 *
 * See also JDKVM for VM-specific info not associated directly with a class.
 */
import components.*;
import util.*;
import jcc.Const;
import jcc.Str2ID;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.util.StringTokenizer;

public class
ClassClass {
    public ClassInfo		ci;
    public InterfaceMethodTable	inf;
    public boolean		impureConstants = false;
    public int			nGCStatics = 0;
    public boolean              hasStaticInitializer = false;

    protected static int finalizerID = Str2ID.sigHash.getID(/*NOI18N*/"finalize", /*NOI18N*/"()V" );

    public MethodConstant findFinalizer( ){
	if ( ci == null ) return null;
	if ( ci.refMethodtable == null ) return null;
	int n = ci.refMethodtable.length;
	for ( int i = 0; i < n ; i++ ){
	    MethodConstant mc= ci.refMethodtable[i];
	    if ( mc.getID() == finalizerID ){
		if ( mc.find().parent.superClass == null ){
		    /*
		     * since we know that java.lang.Object's
		     * finalizer does nothing, we can ignore
		     * its existence!
		     */
		    return null;
		}
		return mc;
	    }
	}
	return null;
    }

    public static void setTypes(){
	classFactory.setTypes();
    }

    /*
     * Make the class list into a vector
     * ordered s.t. superclasses precede their
     * subclasses.
     */
    private static ClassClass[]cvector;
    private static int         cindex;
    private static VMClassFactory classFactory;

    // this is the recursive part
    private static void insertClassElement( ClassInfo e ){
	if ( e.vmClass != null ) return; // already in place.
	ClassInfo sup = e.superClassInfo;
	// make sure our super precedes us.
	if ( (sup != null) && (sup.vmClass == null) ) insertClassElement( sup );
	ClassClass newVmClass = classFactory.newVMClass( e );
	cvector[ cindex++ ] = newVmClass;
	//
	// If the superclass of class C has a <clinit> method, C must
	// be marked as having a static initializer too.
	//
	if (!newVmClass.hasStaticInitializer &&
	    (sup != null) && sup.vmClass.hasStaticInitializer) {
	    newVmClass.hasStaticInitializer = true;
	}
    }

    // this is the entry point for vector building.
    public static ClassClass[] getClassVector( VMClassFactory ftry ){
	if ( cvector != null ) return cvector; // just once, at most.
	classFactory = ftry;
	cvector = new ClassClass[ ClassInfo.nClasses() ];
	cindex  = 0;
	Enumeration classlist = ClassInfo.allClasses();
	while( classlist.hasMoreElements() ){
	    ClassInfo e =  (ClassInfo)classlist.nextElement();
	    if (e.vmClass == null) insertClassElement( e );
	}
	return cvector;
    }

    public static void appendClassElement( ClassInfo c ){
	// foo. Have a cvector in place, must now 
	// add a new entry at the end. "c" is it.
	if ( cvector == null ) return; // ...never mind
	ClassClass[] oldCvector = cvector;
	cvector = new ClassClass[ cindex+1 ];
	System.arraycopy( oldCvector, 0, cvector,0, cindex );
	cvector[ cindex ] = classFactory.newVMClass( c );
    }

    /**
     * Size of an instance in WORDS.
     */
    public int instanceSize(){
	FieldConstant rft[] = ci.refFieldtable;
	if ( rft == null || rft.length == 0 ) return 0;
	FieldInfo lastField = rft[rft.length-1].find();
	return (lastField.instanceOffset+lastField.nSlots);
    }

    public boolean isInterface() {
	return (ci.access&Const.ACC_INTERFACE) != 0;
    }

    public boolean hasMethodtable(){
	return ((!isInterface()) && (ci.refMethodtable != null));
    }

    public boolean isArrayClass(){
	return (ci instanceof ArrayClassInfo);
    }

    public boolean isPrimitiveClass() { 
	return (ci instanceof PrimitiveClassInfo);
    }

    public int nmethods(){
	return (ci.methods==null) ? 0 : ci.methods.length;
    }

    public int nfields(){
	return (ci.fields==null)  ? 0 : ci.fields.length;
    }

    /**
     * In the current definition of module (.mclass) files,
     * many sorts of constants are put in the shared constant
     * pool, and never in the per-class constant pool. This includes
     * Unicode constants and NameAndType constants. This is fine
     * as long as all symbols in the per-class constant pools get
     * resolved, as they won't be missed. However, if we wish to
     * process classes with symbol references that are not fully 
     * resolved, we will have to add such elements into the constant
     * pool. We do this by sweeping over the per-class constants,
     * looking for method, field, and class references that are not
     * resolved, and adding the necessary entries.
     */
    public void
    adjustSymbolicConstants(){
	ConstantObject consts[] = ci.constants;
	if (!isPartiallyResolved(consts)) {
	    return;
	}
	//
	// we have work to do. This is unfortunate.
	// we use a LocalConstantPool to manage the pool we're re-building.
	// Because order matters, we collect the new entries to add at end.
	//
	//System.err.println(Localizer.getString("classclass.warning_class_has_an_impure_constant_pool", ci.className));
	ci.constants = makeResolvable(consts).getConstants(); 
	impureConstants = true;
    }

    public static boolean 
    isPartiallyResolved( ConstantObject[] consts ){
        if ( consts == null ) return false; // no const!
        int nconst = consts.length;
        if ( nconst == 0 ) return false; // no const!

        // first see if we have anything that needs our attention.
        int nsymbolic = 0;
        for( int i = 1; i < nconst; i += consts[i].nSlots ){
            ConstantObject o = consts[i];
            if (!o.isResolved()) {
                return true;
	    }
        }
	return false;
    }

    static public ConstantPool 
    makeResolvable(ConstantPool cp) {
	//
        // we use a LocalConstantPool to manage the pool we're re-building.
        // Because order matters, we collect the new entries to add at end.
        //
        System.err.println(Localizer.getString("classclass.warning_it_has_an_impure_shared_constant_pool"));
	return makeResolvable(cp.getConstants());
    }

    static private ConstantPool makeResolvable(ConstantObject[] consts) { 

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
            switch( o.tag ){
            case Const.CONSTANT_CLASS:
                ClassConstant co = (ClassConstant)o;
                System.err.println(Localizer.getString("classclass.class", co.name.string));
                co.name = (UnicodeConstant)newPool.add( co.name );
                continue;
            case Const.CONSTANT_FIELD:
            case Const.CONSTANT_METHOD:
            case Const.CONSTANT_INTERFACEMETHOD:
                FMIrefConstant fo = (FMIrefConstant)o;
                if ( fo.clas.isResolved() && false ){
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
                continue;
            }
        }
	newPool.impureConstants = true;
	return newPool;
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
