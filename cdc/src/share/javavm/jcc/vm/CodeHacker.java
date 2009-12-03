/*
 * @(#)CodeHacker.java	1.41 06/10/22
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
import components.*;
import java.io.PrintStream;
import consts.Const;
import consts.CVMConst;
import util.DataFormatException;
import util.Localizer;
import util.SignatureIterator;

/*
 * This class "quickens" Java bytecodes.
 * That is, it looks at the instructions having symbolic references,
 * such as get/putfield and methodcalls, attempts to resolve the references
 * and re-writes the instructions in their JVM-specific, quick form.
 *
 * Resolution failures are not (any longer) considered fatal errors, they
 * just result in code that cannot be considered as read-only, as more
 * resolution will be required at runtime, as we refuse to re-write any
 * instructions having unresolved references.
 *
 * The following is an exception to the above rule:
 * References to array classes (having names starting in "[") are 
 * quickened anyway, as we have confidence that these classes, though not
 * instantiated when this code is run, will be instantiated in time.
 * (Perhaps this should be under control of a flag?)
 * 
 */
public
class CodeHacker {

    boolean		verbose;
    PrintStream		log;
    boolean		useLosslessOpcodes = false;
    boolean		jitOn = false;
    ClassInfo		java_lang_Object;

    private boolean	success;
    private byte        newopcode;

    public static int quickenings = 0;
    public static int checkinitQuickenings = 0;

    public CodeHacker( boolean lossless, boolean jitOn, boolean v ){
	verbose = v;
	log = System.err;
	java_lang_Object = ClassTable.lookupClass("java/lang/Object");
	if ( java_lang_Object == null ){
	    log.println(Localizer.getString(
		"codehacker.lookup_failed", "", "java.lang.Object"));
	}
	useLosslessOpcodes = lossless;
	this.jitOn = jitOn;
    }


    private void lookupFailed( ClassConstant me, FMIrefConstant target ){
	log.println(Localizer.getString(
	    "codehacker.lookup_failed", me.name, target));
	success = false;
    }

    private static int getInt( byte code[], int w ){
	return	(  (int)code[w]   << 24 ) |
		(( (int)code[w+1] &0xff ) << 16 ) |
		(( (int)code[w+2] &0xff ) << 8 ) |
		 ( (int)code[w+3] &0xff );
    }

    private static int getUnsignedShort( byte code[], int w ){
	return	(( (int)code[w] &0xff ) << 8 ) | ( (int)code[w+1] &0xff );
    }

    private static void putShort( byte code[], int w, short v ){
	code[w]   = (byte)(v>>>8);
	code[w+1] = (byte)v;
    }

    /*
     * Returns true if the reference was fully quickened.
     * Returns false if the reference could not be deleted, either because
     *   the lookup failed, or because we had to use the wide variant.
     * Sets this.success to false if the lookup failed or there was no
     * usable variant.
     */
    private boolean quickenFieldAccess(
	MethodInfo m,
	ClassConstant me,
	byte code[],
	int loc,
	boolean isStatic,
	ConstantObject c[],
	int oneWord,
	int twoWords,
	int refReference,
	int wideReference,
	int oneWord_checkinit,
	int twoWords_checkinit,
	int refReference_checkinit
    ) {
	FieldConstant fc = (FieldConstant)c[ getUnsignedShort( code, loc+1 ) ];
	FieldInfo  fi = fc.find();
	if ( fi == null ){
	    //
	    // never even try to quicken anything we cannot resolve.
	    //
	    lookupFailed( me, fc );
	    return false;
	}
	byte newCode;
	boolean useCheckinit = 
	    (isStatic && fi.parent.vmClass.hasStaticInitializer);

	switch (fc.sig.type.string.charAt(0) ){
	case '[':
	case 'L':
	    newCode = (byte)((useCheckinit)
		      ? (refReference_checkinit) : (refReference));
	    break;
	case 'D':
	case 'J':
	    newCode = (byte)((useCheckinit)
		      ? (twoWords_checkinit) : (twoWords));
	    break;
	default:
	    newCode = (byte)((useCheckinit)
		      ? (oneWord_checkinit) : (oneWord));
	    break;
	}
	if ( isStatic ) {
	    if ( useCheckinit ) {
		CodeHacker.checkinitQuickenings++;
		m.hasCheckinits = true;
	    }
	    CodeHacker.quickenings++;
	    code[loc] = newCode;
	    return false; // still references constant pool!
	} else {
	    int fieldOffset = fi.instanceOffset;
	    // CVM wants field offset as actual WORD offset
	    // from the start of the object. This requires
	    // knowing the size of the header.
	    // JVM just wanted a 0-based word index.
	    // 
	    fieldOffset += CVMConst.ObjHeaderWords;
	    if ( fieldOffset < 0 ){
		lookupFailed( me, fc );
		log.println(Localizer.getString(
		    "codehacker.negative_field_offset"));
		return false;
	    }
	    //
	    // Disallow lossy field access quickening if
	    //     1 Non-lossy opcodes are requested
	    //     2 The offset won't fit in a byte
	    //     3 The JIT is on, and the field is volatile
	    //
	    if ( fieldOffset > 0xFF ||  useLosslessOpcodes ||
		 (jitOn && fi.isVolatile())) {
		if ( wideReference == 0 ){
		    lookupFailed( me, fc );
		    log.println(Localizer.getString(
			"codehacker.need_wide_op"));
		}
		code[loc] = (byte)wideReference;
		return false; // still references constant pool!
	    }
	    code[loc+1] =  (byte)fieldOffset;
	    code[loc+2] =  0;
	    code[loc] = newCode;
	    return true;
	}
    }

    /*
     * Should  a call to the specified methodblock be turned into
     * an invokesuper_quick instead of a invokenonvirtual_quick?
     *
     * The four conditions that have to be satisfied:
     *    The method isn't private
     *    The method isn't an <init> method
     *    The ACC_SUPER flag is set in the current class
     *    The method's class is a superclass (and not equal) to 
     *	     the current class.
     */
    private boolean
    isSpecialSuperCall( MethodInfo me, MethodInfo callee ){
	String name = callee.name.string;
	if ( (callee.access&Const.ACC_PRIVATE) != 0 ) return false;
	if ( name.equals("<init>") ) return false;
	ClassInfo myclass = me.parent;
	ClassInfo hisclass = callee.parent;
	if ( myclass == hisclass ) return false;
	// walk up my chain, looking for other's class
	while ( myclass != null ){
	    myclass = myclass.superClassInfo;
	    if ( myclass == hisclass ) return true;
	}
	return false;
    }

    private boolean quickenMethod(MethodInfo m, ConstantObject c[])
        throws DataFormatException {

	byte code[] = m.code;
	int list[] = m.getLdcInstructions();
	ConstantObject		constObj;
	FieldConstant		fc;
	MethodConstant		mc;
	NameAndTypeConstant	nt;
	ClassConstant		classConst;
	String			t;
	ClassConstant		me = m.parent.thisClass;
	MethodInfo		mi;
	ClassInfo		classInfo;

	success = true;
	if ( list != null ){
	    for ( int i = 0; i < list.length; i++ ){
		int loc = list[i];
		if ( loc < 0 ) continue;
		switch( (int)code[loc]&0xff ){
		case Const.opc_ldc:
		    //
		    // no danger of lookup failure here,
		    // so don't even examine the referenced object.
		    //
		    constObj = c[(int)code[loc+1]&0xff];
		    if (constObj instanceof StringConstant) {
			code[loc] = (byte)Const.opc_aldc_quick;
		    } else if (!(constObj instanceof ClassConstant)) {
			code[loc] = (byte)Const.opc_ldc_quick;
		    }
		    constObj.incLdcReference();
		    break;
		default:
		    throw new DataFormatException( "unexpected opcode in ldc="+
			((int)code[loc]&0xff)+" at loc="+loc+
			" in "+m.qualifiedName() );
		}
	    }
	}
	list = m.getWideConstantRefInstructions();
	if ( list != null ){
	    MethodInfo[] tList = null;
	    int tListIndex = 0;		// index into tList
	    if (VMMethodInfo.SAVE_TARGET_METHODS) {
		tList = new MethodInfo[list.length];
	    }
	    for ( int i = 0; i < list.length; i++ ){
		int loc = list[i];
		if ( loc < 0 ) continue;
		constObj = c[getUnsignedShort(code, loc+1)];
		if (!constObj.isResolved()) {
		    //
		    // don't try to quicken unresolved references.
		    // this is not fatal!
		    //
		    if (verbose){
			log.println(Localizer.getString(
			    "codehacker.could_not_quicken",
			     m.qualifiedName(), constObj));
		    }
		    continue;
		}
		switch( (int)code[loc]&0xff ){
		case Const.opc_ldc_w:
		    if (constObj instanceof StringConstant) {
			code[loc] = (byte)Const.opc_aldc_w_quick;
		    } else if (!(constObj instanceof ClassConstant)) {
			code[loc] = (byte)Const.opc_ldc_w_quick;
		    }
		    constObj.incLdcReference();
		    break;
		case Const.opc_ldc2_w:
		    code[loc] = (byte)Const.opc_ldc2_w_quick;
		    break;
		case Const.opc_getstatic:
		    // All the accesses to static field are done using 
		    // checkinit version of opcodes.
		    quickenFieldAccess( m, me, code, loc, true, c,
			Const.opc_getstatic_quick,
			Const.opc_getstatic2_quick,
			Const.opc_agetstatic_quick, 0,
			Const.opc_getstatic_checkinit_quick,
			Const.opc_getstatic2_checkinit_quick,
			Const.opc_agetstatic_checkinit_quick);
		    break;
		case Const.opc_putstatic:
		    // All the accesses to static field are done using 
		    // checkinit version of opcodes.
		    quickenFieldAccess( m, me, code, loc, true, c,
			Const.opc_putstatic_quick,
			Const.opc_putstatic2_quick,
			Const.opc_aputstatic_quick, 0,
			Const.opc_putstatic_checkinit_quick,
			Const.opc_putstatic2_checkinit_quick,
			Const.opc_aputstatic_checkinit_quick);
		    break;
		case Const.opc_getfield:
		    if (quickenFieldAccess( m, me, code, loc, false, c,
			Const.opc_getfield_quick,
			Const.opc_getfield2_quick,
			Const.opc_agetfield_quick,
			Const.opc_getfield_quick_w, 0, 0, 0) ){
			// doesn't reference constant pool any more
			list[i] = -1;
		    }
		    break;
		case Const.opc_putfield:
		    if (quickenFieldAccess( m, me, code, loc, false, c,
			Const.opc_putfield_quick,
			Const.opc_putfield2_quick,
			Const.opc_aputfield_quick,
			Const.opc_putfield_quick_w, 0, 0, 0) ){
			// doesn't reference constant pool any more
			list[i] = -1;
		    }
		    break;
		case Const.opc_invokevirtual:
		    mc = (MethodConstant)constObj;
		    mi = mc.find(); // must succeed, if isResolved succeeded!
		    int x = -1;
		    if (mi.parent.isFinal() || mi.isFinalMember()) {
			code[loc] = (byte)Const.opc_invokenonvirtual_quick;
		    } else if (!jitOn && (x=mi.methodTableIndex )<= 255 && 
				! useLosslessOpcodes ){
			// Do really quick quickening, but only if the JIT
			// is not on
			if ( mi.parent == java_lang_Object ){
			    newopcode = (byte)Const.opc_invokevirtualobject_quick;
			} else {
			    String sig = mc.sig.type.string;
			    new SignatureIterator(sig) {
				public void do_array(int d, int st, int end) {
				    newopcode =
				        (byte)Const.opc_ainvokevirtual_quick;
				}
				public void do_object(int st, int end) {
				    newopcode =
				        (byte)Const.opc_ainvokevirtual_quick;
				}
				public void do_scalar(char c) {
				    switch(c) {
					case Const.SIGC_LONG:
					case Const.SIGC_DOUBLE:
					    newopcode = (byte)
						Const.opc_dinvokevirtual_quick;
					    break;
					case Const.SIGC_VOID:
					    newopcode = (byte)
						Const.opc_vinvokevirtual_quick;
					    break;
					default:
					    newopcode = (byte)
						Const.opc_invokevirtual_quick;
					    break;
				    }
				}
			    }.iterate_returntype();
			}
			code[loc] = newopcode;
			code[loc+1] = (byte) x;
			code[loc+2] = (byte)mi.argsSize;
			list[i] = -1; // doesn't reference constant pool any more
			if (VMMethodInfo.SAVE_TARGET_METHODS) {
			    // Save the target method info for inlining
			    tList[tListIndex++] = mi;
			}
		    } else {
			//
			// big index OR useLosslessOpcodes OR jit
			//
			code[loc] = (byte)Const.opc_invokevirtual_quick_w;
		    }
		    break;
		case Const.opc_invokeinterface:
		    code[loc] = (byte)Const.opc_invokeinterface_quick;
		    break;
		case Const.opc_invokestatic:
		    mc = (MethodConstant)constObj;
		    mi = mc.find(); // must succeed, if isResolved succeeded!
		    if (mi.parent.vmClass.hasStaticInitializer) {
			CodeHacker.checkinitQuickenings++;
			m.hasCheckinits = true;
		    }
		    CodeHacker.quickenings++;
		    code[loc] = (byte)((mi.parent.vmClass.hasStaticInitializer)
				? Const.opc_invokestatic_checkinit_quick
				: Const.opc_invokestatic_quick);
		    break;
		case Const.opc_new:
		    /*
		     * If the class to be instantiated has a static
		     * initializer, we must quicken opc_new into
		     * opc_new_checkinit_quick rather than
		     * opc_new_quick.
		     */
		    classConst = (ClassConstant)constObj;
                    // must succeed, if isResolved succeeded!
		    classInfo = classConst.find();
		    if (classInfo.vmClass.hasStaticInitializer) {
			CodeHacker.checkinitQuickenings++;
			m.hasCheckinits = true;
		    }
		    CodeHacker.quickenings++;
		    code[loc] = (byte)((classInfo.vmClass.hasStaticInitializer)
				       ? Const.opc_new_checkinit_quick
				       : Const.opc_new_quick);
		    break;
		case Const.opc_anewarray:
		    code[loc] = (byte)Const.opc_anewarray_quick;
		    break;
		case Const.opc_checkcast:
		    code[loc] = (byte)Const.opc_checkcast_quick;
		    break;
		case Const.opc_instanceof:
		    code[loc] = (byte)Const.opc_instanceof_quick;
		    break;
		case Const.opc_multianewarray:
		    code[loc] = (byte)Const.opc_multianewarray_quick;
		    break;
		case Const.opc_invokespecial:
		    mc = (MethodConstant)constObj;
		    mi = mc.find(); // must succeed.
		    byte newop;
		    if ( false ){
			newop = (byte)Const.opc_invokesuper_quick;
		    } else {
			newop = (byte)Const.opc_invokenonvirtual_quick;
		    }
		    code[loc] = newop;
		    break;
		default: 
		    throw new DataFormatException( "unexpected opcode in wideConstantRef="+
			((int)code[loc]&0xff)+" at loc="+loc+
			" in "+m.qualifiedName() );
		}
	    }
	    // Alloc and copy to new targetMethods array
	    if (VMMethodInfo.SAVE_TARGET_METHODS) {
		m.targetMethods = new MethodInfo[tListIndex];
		System.arraycopy(tList, 0, m.targetMethods, 0, tListIndex);
	    }
	}
	return success;
    }

    public boolean
    quickenAllMethodsInClass(ClassInfo c) {
	ConstantObject constants[] = c.getConstantPool().getConstants();
	MethodInfo     methods[]= c.methods;
	int numberOfMethods = methods.length;
	int i = 0;
	boolean result = true;
	try {
	    for (i = 0; i < numberOfMethods; i++) {
		if (!quickenMethod(methods[i], constants)) {
		    result = false;
                }
	    }
	} catch (DataFormatException e) {
	    System.err.println("Quickening "+methods[i].qualifiedName()+
                               " got exception:");
	    e.printStackTrace();
	    return false;
	}
	return result;
    }

}
