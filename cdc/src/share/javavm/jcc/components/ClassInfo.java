/*
 * @(#)ClassInfo.java	1.48 06/10/22
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
import jcc.Util;
import jcc.Str2ID;
import consts.Const;
import util.*;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Set;
import java.util.Vector;

/**
 * ClassInfo is the root of all information about the class
 * which comes out of the .class file, including its methods,
 * fields, interfaces, superclass, and any other attributes we
 * read in.
 * ClassInfo does not contain (in theory) any VM-specific data.
 * All the target-VM-specific information is kept in the corresponding
 * ClassClass: see field vmClass. ClassClass is abstract, so what is
 * really present is a target-VM-specific subclass of it, such as CVMClass.
 */

public
class ClassInfo
{
    public int                  majorVersion;
    public int                  minorVersion;
	
    public String		className;
    public int			access;
    public ClassConstant	thisClass;
    public ClassConstant	superClass;
    public ClassInfo		superClassInfo;

    // Tables for all fields, methods and constants of this class
    public FieldInfo		fields[];
    public MethodInfo		methods[];
    private static ConstantPool	nullCP =
	new ConstantPool(new ConstantObject[0]);
    private int                 sharedCPIdx = -1;
    private ConstantPool	cp = nullCP;
    private ConstantObject	oldConstants[];
    public ClassConstant	interfaces[];

    // These are the tables of instance fields and methods.
    // The methodtable is turned into the virtual method lookup table
    // of the virtual machine.
    public FieldInfo		fieldtable[];
    public MethodInfo		methodtable[];

    /*
     * Info on each slot of an object, indexed by offset
     * instead of on its field #
     * The difference between fieldtable and slottable is slight
     * -- its just that double-word fields (types J and D) take
     * up 2 entries in the slottable.
     */
    public FieldInfo		slottable[];

    // Class attributes that we do not interpret
    public Attribute[]          classAttributes;
    public SourceFileAttribute  sourceFileAttr;

    // Innerclass attribute accessed.
    public InnerClassAttribute	innerClassAttr;

    // Class generics signature info:
    public SignatureAttribute   signatureAttr;

    public ClassLoader		loader;

    public vm.ClassClass	vmClass; // used by in-core output writers

    public Vector		allInterfaces;

    protected boolean		verbose;
    protected static PrintStream log = System.out;
    public  static boolean      classDebug = false;

    public int			flags;
    public static final int	INCLUDE_ALL = 1;
    public int			group;
    
    public static int finalizerID = Str2ID.sigHash.getID(
						/*NOI18N*/"finalize", /*NOI18N*/"()V" );
    public boolean              hasNonTrivialFinalizer = false;

    private int			pkgNameLength;
    private final char		SIGC_PACKAGE = util.ClassFileConst.SIGC_PACKAGE;

    public ClassInfo( boolean v ) {
        Assert.assertClassloadingIsAllowed();
	verbose = v;
	flags = INCLUDE_ALL; // by default, we want all members.
	// what else should be here?
    }

    private String genericNativeName;

    public void setSharedCPIdx(int i) {
        sharedCPIdx = i;
    }

    public final String getGenericNativeName() { 
        if (genericNativeName == null) 
	    genericNativeName = createGenericNativeName();
        return genericNativeName;
    }

    public boolean isFinal() {
	return ((access & Const.ACC_FINAL) != 0);
    }
    
    // Is this class a subclass of java.lang.ref.Reference?
    public boolean isReference() {
	ClassInfo cinfo = this;
	while (cinfo != null) {
	    if (cinfo.className.equals("java/lang/ref/Reference")) {
		return true;
	    }
	    cinfo = cinfo.superClassInfo;
	}
	return false;
    }

    // Is this class an interface?
    public boolean isInterface() {
	return ((this.access & Const.ACC_INTERFACE) != 0);
    }

    // Is this class the java.lang.Object class?
    public boolean isJavaLangObject() {
	ClassInfo cinfo = this;
	if ((cinfo != null) && cinfo.className.equals("java/lang/Object")) {
	    return true;
	}
	return false;
    }

    // This will be overridden by subclasses
    protected String createGenericNativeName() { 
        return Util.convertToClassName(className );
    }


    /**
     * Reads in the constant pool from a classfile, and fills in the cp
     * (i.e. constantpool) field of this classinfo instance.  The filled in
     * constants are not yet in a resolved state.
     */
    private void readConstantPool(DataInput in) throws IOException {
	int num = in.readUnsignedShort();

	if(verbose){
	    log.println(Localizer.getString(
                "classinfo.reading_entries_in_constant_pool",
                Integer.toString(num)));
	}
	ConstantObject[] constants = new ConstantObject[num];
	for (int i = 1; i < num; i+=constants[i].nSlots) {
	    constants[i] = ConstantObject.readObject(in);
	    constants[i].index = i;
	    constants[i].containingClass = this;
	}
	cp = new ConstantPool(constants);
    }

    /**
     * Flatten all CP entries so that they need not indirect through the
     * constant pool to get to the symbollic info.  For example, a class
     * constant is defined by a CP index which point to a UTF8 string.  A
     * flattened class constant will refer to the UTF8 string directly instead
     * of needing to index into the constant pool to get to it.
     */
    private void flattenConstants() {
	if (verbose) {
            log.println(Localizer.getString(
                            "classinfo.>>>resolving_constants"));
	}
	ConstantObject constants[] = cp.getConstants();
	for (int i = 1; i < constants.length; i+=constants[i].nSlots) {
	    constants[i].flatten(cp);
	}
    }

    /*
     * Iterate through the constant pool.
     * Look for class constants that don't point to anything, yet.
     * Put those String names in the undefClasses set.
     */
    public void
    findUndefinedClasses(Set undefClasses) {
	ConstantObject constants[] = cp.getConstants();
	for (int i = 1; i < constants.length; i+=constants[i].nSlots) {
	    if (constants[i].tag == Const.CONSTANT_CLASS) {
		ClassConstant c = (ClassConstant) constants[i];
		if (c.find() == null){
		    String stringname = c.name.string;
		    c.forget();
		    if (stringname.charAt(0) == '['){
			// this is an array.
			// DO NOT put the array in the table.
			// but we may need to put an underlying
			// object type in, so take a closer look.
			int lindex = stringname.indexOf('L');
			if (lindex == -1){
			    // primitive array type. 
			    // not interesting.
			    continue;
			}
			// capture the name between the L and the ;
			stringname = stringname.substring(
					lindex+1, stringname.length()-1);
			if (loader.lookupClass(stringname) != null){
			    continue; // found it.
			}
		    }
		    undefClasses.add(stringname);
		}
	    }
	}
    }
    
    /*
     * we can make our own table smaller by deleting unreferenced elements.
     * At this point,
     * all non-code references into it are by object reference, NEVER
     * by index -- everything has been resolved! Thus we can
     * compact our table, deleting all the UnicodeConstants.
     * We adjust each constant's index entry accordingly.
     * Naturally, we preserve the null entries.
     */
    public void smashConstantPool() {
	ConstantObject constants[] = cp.getConstants();
	int nOld = constants.length;
	int nNew = 1;
	ConstantObject o;
	// first, count and index.
	for (int i = 1; i < nOld; i += o.nSlots) {
	    o = constants[i];
	    if (!o.shared) {
		if (o.getReferences() == 0) {
		    o.index = -1; // trouble.
		} else {
		    // we're keeping it.
		    o.index = nNew;
		    nNew += o.nSlots;
		}
	    }
	}
	// now reallocate and copy.
	ConstantObject newConstants[] = new ConstantObject[ nNew ];
	int j = 1;
	for (int i = 1; i < nOld; i += o.nSlots) {
	    o = constants[i];
	    if (!o.shared && (o.getReferences() != 0)) {
		// we're keeping it.
		newConstants[j] = o;
		j += o.nSlots;
	    }
	}
	oldConstants = constants;
	cp = new ConstantPool(newConstants);
    }

    public ConstantPool getConstantPool() {
	return cp;
    }

    public void setConstantPool(Vector cps) {
        if (sharedCPIdx != -1) {
            cp = (ConstantPool)cps.get(sharedCPIdx);
        }
    }


    // Read the list of interfaces this class supports
    void readInterfaces( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	if(verbose){
	    log.println(Localizer.getString(
		"classinfo.reading_interfaces_implemented", Integer.toString(count)));
	}
	interfaces = new ClassConstant[count];
	ConstantObject constants[] = cp.getConstants();
	for (int i = 0; i < count; i++) {
	    interfaces[i] = (ClassConstant) constants[in.readUnsignedShort()];
	}
    }


    // Read the list of fields
    void readFields( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	fields = new FieldInfo[count];
	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_field_members", Integer.toString(count)));
	}

        int i, j;
	for (i = j = 0; i < count; i++) {
            FieldInfo fi = FieldInfo.readField(in, this);
            if (fi != null) {
                fields[j] = fi;
                fields[j].index = j;
                fields[j++].captureValueAttribute();
            }
	}

        // If we have discarded any fields, re-create the table.
        if (j != i) {
            FieldInfo[] smaller = new FieldInfo[j];
            System.arraycopy(fields, 0, smaller, 0, j);
            fields = smaller;
        }
    }

    // Read the list of methods from classfile
    void readMethods( DataInput in, boolean readCode ) throws IOException {
	int count = in.readUnsignedShort();
	methods = new MethodInfo[count];

	if(verbose){
	    log.println(Localizer.getString(
			"classinfo.reading_methods", Integer.toString(count)));
	}

        int i, j;
	for (i = j = 0; i < count; i++) {
            MethodInfo mi = MethodInfo.readMethod( in, this, readCode );
            if (mi != null) {
                methods[j] = mi;
                methods[j].index = j++;
            }
	}

        // If we have discarded any methods, re-create the table.
        if ( j != i ) {
            MethodInfo[] smaller = new MethodInfo[j];
            System.arraycopy(methods, 0, smaller, 0, j);
            methods = smaller;
        }
    }

    void readAttributes( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	Vector clssAttr = new Vector();
	
	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_attributes", Integer.toString(count)));
	}

	ConstantObject constants[] = cp.getConstants();
	for (int i = 0; i < count; i++) {
	    int nameIndex = in.readUnsignedShort();
	    int bytes = in.readInt();
	    UnicodeConstant name = (UnicodeConstant)constants[nameIndex];
	    if  (name.string.equals(/*NOI18N*/"SourceFile")) {
		if (ClassInfo.classDebug) {
		    UnicodeConstant srcName =
			(UnicodeConstant)constants[in.readUnsignedShort()];
		    sourceFileAttr =
			new SourceFileAttribute(name, bytes, srcName);
		    clssAttr.addElement(sourceFileAttr);
		} else {
		    byte[] b = new byte[bytes];
		    in.readFully(b);
		    clssAttr.addElement
			(new UninterpretedAttribute(name, bytes, b));
		}
	    } else if (name.string.equals("InnerClasses")) {
		//Added to support the InnerClasses Attribute defined in the
		//1.2 JVM Spec.
		/* this need not be done if reflection isn't supported */
		innerClassAttr = (InnerClassAttribute)InnerClassAttribute.readAttribute(in, bytes, name, constants);
		clssAttr.addElement(innerClassAttr);
            /* TODO :: BEGIN experimental code for future signature support.
            } else if (name.string.equals("Signature")) {
		// Added to support the Signature Attribute defined in the
		// 1.5 VM spec.
		// this need not be done if reflection isn't supported.
		int idx = in.readUnsignedShort();

		UnicodeConstant signature = (UnicodeConstant)constants[idx];
		//StringConstant sigStr = StringConstant.utfToString(signature);
		//sigStr = (StringConstant)cp.add(sigStr);
		//sigStr.resolve(cp);
		//constants = cp.getConstants();

                signatureAttr =
		    //new SignatureAttribute(name, bytes, sigStr);
		    new SignatureAttribute(name, bytes, signature);
		clssAttr.addElement(signatureAttr);
            * TODO :: END */
	    } else {
		byte[] b = new byte[bytes];
		in.readFully(b);
		clssAttr.addElement
		    (new UninterpretedAttribute(name, bytes, b));
	    }
	}
	int nattr = clssAttr.size();
	if (nattr > 0) {
	    this.classAttributes = new Attribute[nattr];
	    clssAttr.copyInto(classAttributes);
	}
    }

    // Read in the entire classfile.
    // Assumes that the file is open, and the magic numbers are o.k.
    //
    public void
    read(DataInput in, boolean readCode)
    throws IOException {

	readConstantPool(in);
        flattenConstants();

        Assert.disallowClassloading();
	ConstantObject constants[] = cp.getConstants();

	access = in.readUnsignedShort();
	thisClass = (ClassConstant) constants[in.readUnsignedShort()];
	int sup = in.readUnsignedShort();
	if (sup != 0) {
	    superClass = (ClassConstant) constants[sup];
        }
	className = thisClass.name.string;
	pkgNameLength = className.lastIndexOf(SIGC_PACKAGE);

	// Read the various parts of the class file
	readInterfaces( in );
	readFields( in );
	readMethods( in, readCode );
	readAttributes( in );
	/* DONT DO THIS HERE enterClass(); */
        Assert.allowClassloading();
    }

    // Sets the classfile version numner:
    public void setVersionInfo(int majorVersion, int minorVersion) {
	this.majorVersion = majorVersion;
	this.minorVersion = minorVersion;
    }

    // Compute the fieldtable for a class.  This requires laying
    // out the fieldtable for our parent, then adding any fields
    // that are not inherited.
    public
    void buildFieldtable(){
	if (this.fieldtable != null) return; // already done.
	FieldInfo fieldtable[];
	int n;
	int fieldoff;
	int fieldtableLength = 0;
	FieldInfo candidate[] = this.fields;
	for( int i =0; i < candidate.length; i++ ){
	    if ((candidate[i].access & Const.ACC_STATIC) == 0){
		fieldtableLength++;
	    }
	}
	if ( superClassInfo != null ){
	    superClassInfo.buildFieldtable();
	    n = superClassInfo.fieldtable.length;
	    fieldtableLength += n;
	    fieldoff = (n==0)? 0
		    : (superClassInfo.fieldtable[n-1].instanceOffset +
			superClassInfo.fieldtable[n-1].nSlots);
	    fieldtable = new FieldInfo[ fieldtableLength ];
	    System.arraycopy( superClassInfo.fieldtable, 0, fieldtable, 0, n );
	} else {
	    fieldtable = new FieldInfo[ fieldtableLength ];
	    n = 0;
	    fieldoff = 0;
	}
	for( int i =0; i < candidate.length; i++ ){
	    if ((candidate[i].access & Const.ACC_STATIC) == 0){
		fieldtable[n++] = candidate[i];
		candidate[i].instanceOffset = fieldoff;
		fieldoff += candidate[i].nSlots;
	    }
	}
	this.fieldtable = fieldtable;
	//
	// Build a mapping of slot id to FieldInfo structure.
	// This makes offset based calculations much easier -- for
	// example stackmap computation
	//
	this.slottable  = buildSlotTable(fieldtable, fieldoff);

    }

    //
    // Build a mapping of slot id to FieldInfo structure.
    // This makes offset based calculations much easier -- for
    // example stackmap computation
    //
    private FieldInfo[] buildSlotTable(FieldInfo[] fieldtable,
				       int slotTableSize) {
	FieldInfo[] slotTable = new FieldInfo[slotTableSize];
	int sIdx = 0;
	for (int i = 0; i < fieldtable.length; i++) {
	    FieldInfo fld = fieldtable[i];
	    if (fld.nSlots == 1) {
		slotTable[sIdx] = fld;
	    } else {
		slotTable[sIdx]     = fld;
		slotTable[sIdx + 1] = fld;
	    }
	    sIdx += fld.nSlots;
	}
	return slotTable;
    }

    // Compute the method table for a class.  This requires laying
    // out the method table for our parent, then adding any methods
    // that are not inherited.
    //
    // Jdk 1.4 javac does not insert miranda methods
    // into a class if the class is abstract and does not 
    // declare all the methods of its superinterfaces,
    // like older versions of javac. So we are doing it.
    public
    void buildMethodtable() {
	if ( this.methodtable != null ) return; // already done.
	MethodInfo table[];
	MethodInfo methods0[] = this.methods;
	ClassInfo sup = superClassInfo;
	int myPkgNameLength = this.pkgNameLength;

	if ((sup != null) && ( (sup.access&Const.ACC_INTERFACE)==(this.access&Const.ACC_INTERFACE) ) ) {
	    sup.buildMethodtable();
	    table = sup.methodtable;
	} else {
	    table = new MethodInfo[0];
	}

	//
	// This might change soon, if we try to override the non-trivial
	// finalizer of the superclass with a trivial one. Otherwise
	// it stands.
	//
	if (sup != null) {
	    this.hasNonTrivialFinalizer = sup.hasNonTrivialFinalizer;
	}

	/*
	 * allocate a temporary table that is certainly large enough.
	 * It starts as a copy of our parent's table
	 */
	MethodInfo newTable[] = new MethodInfo[table.length + methods0.length];
	int index = table.length;
	System.arraycopy( table, 0, newTable, 0, index );
	
	if (sup == null) { 
	    // finalize() goes into slot 0 of java.lang.Object
	    index++;
	}

    method_loop:
	for (int i = 0; i < methods0.length; i++) {
	    if ((methods0[i].access & (Const.ACC_STATIC|Const.ACC_PRIVATE)) != 0) {
		/* static and private methods don't go in the table */
		continue method_loop;
	    } else if (methods0[i].name.string.equals(/*NOI18N*/"<init>")) {
		/* <init> methods don't go in the table */
		continue method_loop;
	    } else if (sup == null && methods0[i].name.string.equals(/*NOI18N*/"finalize") 
		                   && methods0[i].type.string.equals(/*NOI18N*/"()V")) {
		/* java.lang.Object.finalize()V always goes at the top of the table*/
	        newTable[0] = methods0[i];
		newTable[0].methodTableIndex = 0; // 1=>0 EJVM
		continue method_loop;
	    } 
	    int j;
	    int thisID = methods0[i].getID();
	    if (thisID == finalizerID) {
		/* Note that this class has its own finalizer */
		this.hasNonTrivialFinalizer = methods0[i].isNonTrivial();
	    }
	    /*
	     * Now search the table (initialized with our parent's table)
	     * for a name/type match.
	     * Since private methods aren't even in the table, we don't have
	     * to worry about those. But we do need to consider
	     * package scoping.
	     */
	match_loop:
	    for ( j = 0; j < table.length; j++) {
		if (thisID == table[j].getID()) {
		    MethodInfo parentMethod = table[j];
		    if ((parentMethod.access & (Const.ACC_PUBLIC|Const.ACC_PROTECTED)) 
			== 0)
		    {
			/*
			 * we must be in the same package to override this one
			 * sadly, the only way to compare packages is 
			 * to compare names
			 */
			int     parentPkgNameLength = parentMethod.parent.pkgNameLength;
			boolean equalPkgNames;

			if (parentPkgNameLength != myPkgNameLength){
			    /* package names of unequal length. Quick failure */
			    equalPkgNames = false;
			} else if (myPkgNameLength == -1){
			    equalPkgNames = true; // neither is in a package.
			} else {
			    equalPkgNames = parentMethod.parent.className.regionMatches(
					    0, className, 0, myPkgNameLength);
			}
			if (! equalPkgNames){
			    /* package names of unequal value. Failure */
			    continue match_loop;
			}
		    }
		    newTable[j] = methods0[i];
		    newTable[j].methodTableIndex = j + 0; // 1=>0 EJVM
		    continue method_loop;
		}
	    }
	    // If we're not overriding our parent's method we do add
	    // a new entry to the method table.
	    newTable[index] = methods0[i];
	    newTable[index].methodTableIndex = index + 0; // 1=>0 EJVM
	    index++;
	}

	// now allocate a table of the correct size.
	MethodInfo methodTable0[] = new MethodInfo[index];
	System.arraycopy( newTable, 0, methodTable0, 0, index );
        this.methodtable =  methodTable0;

        // now try to resolve miranda methods.
        MethodInfo thisMethodtable[] = null;
        MethodInfo thisMethods[] = null;
        if ((this.access&Const.ACC_ABSTRACT) != 0 &&
            (this.access&Const.ACC_INTERFACE) == 0) {
            int i;
            int methodsnumber = methods.length;
            if (allInterfaces == null) {
		findAllInterfaces();
            }
            for (i = 0; i < allInterfaces.size(); i++) {
		 ClassInfo superinterface = (ClassInfo)allInterfaces.get(i);
                 superinterface.buildMethodtable();

                 MethodInfo interfaceMethodtable[] = superinterface.methodtable;
                 MethodInfo tmpTable[] = new MethodInfo[interfaceMethodtable.length];
                 int idx = 0;

            interface_method_loop:
                 for (int j = 0; j < interfaceMethodtable.length; j++) {
                     int imID = interfaceMethodtable[j].getID();
                     for (int k = 0; k < methodtable.length; k++) {
                         if (imID == methodtable[k].getID()) {
                             continue interface_method_loop; 
                         }
                     }
                     // We need to get a copy of the methodInfo, because
                     // we are going to change its states.
                     try {
                         tmpTable[idx] = 
                             (MethodInfo)(interfaceMethodtable[j].clone());
                         tmpTable[idx].parent = this;
                         tmpTable[idx].index = methodsnumber;
                         methodsnumber++;
                     } catch (CloneNotSupportedException cnse) {
                         cnse.printStackTrace();
                     }
                     idx++;
                 }

                 // We need to insert these miranda methods into
                 // both this.methods[] and this.methodtable[].
                 // The reason is methods[] data structure is
                 // used to resolve MethodRef constant in quicken
                 // process, while methodtable[] is used in
                 // InterfaceMethodTable.generateInterfaceVector()
                 // to verify if the class implements all the 
                 // interface methods.
                 if (idx > 0) {
                     int newMethodsNum = methods.length + idx;
                     thisMethods = new MethodInfo[newMethodsNum];
                     System.arraycopy(methods, 0, thisMethods, 0, methods.length);
                     System.arraycopy(tmpTable, 0, thisMethods, methods.length, idx);
                     this.methods = thisMethods;

                     int newTableLen = methodtable.length + idx;
                     thisMethodtable = new MethodInfo[newTableLen];
                     System.arraycopy(methodtable, 0,
                         thisMethodtable, 0, methodtable.length);
                     System.arraycopy(tmpTable, 0,
                         thisMethodtable, methodtable.length, idx);
                     for (int mtidx = methodtable.length; mtidx < newTableLen; mtidx++) {
                         thisMethodtable[mtidx].methodTableIndex = mtidx + 0;
                     }
                     this.methodtable = thisMethodtable;
                 }
            }
        } 

    }


    private static boolean
    conditionalAdd( Vector v, Object o ){
	if ( v.contains( o ) )
	    return false;
	v.addElement( o );
	return true;
    }
    /*
     * Compute the vector of all interfaces this class implements (or
     * this interface extends). Not only the interfaced declared in
     * the implements clause, which is what the interfaces[] field
     * represents, but all interfaces, including those of our superclasses
     * and those extended/implemented by any interfaces we implement.
     *
     */
    public void
    findAllInterfaces(){
	/*
	 * This works recursively, by computing parent's interface
	 * set first. THIS ASSUMES NON-CIRCULARITY, as does the rest
	 * of the Java system. This assumption will fail loudly, if
	 * at all.
	 */
	if ( allInterfaces != null )
	    return; // already done
	if ( superClassInfo == null ){
	    // we must be java.lang.Object!
	    allInterfaces = new Vector(5); // a generous size.
	} else {
	    if ( superClassInfo.allInterfaces == null )
		superClassInfo.findAllInterfaces();
	    allInterfaces = (Vector)(superClassInfo.allInterfaces.clone());
	}
	if ( interfaces == null ) return; // all done!
	for( int i = 0; i < interfaces.length; i++ ){
	    ClassInfo interf = interfaces[i].find();
	    if ((interf == null) ||
                ((interf.access & Const.ACC_INTERFACE) == 0)) {
		System.err.println(Localizer.getString(
                    "classinfo.class_which_should_be_an_interface_but_is_not",
                    className, interfaces[i]));
		continue;
	    }
	    if ( interf.allInterfaces == null )
		interf.findAllInterfaces();
	    if ( ! conditionalAdd( allInterfaces, interf ) ){
		// if this interface was already in the set,
		// then all the interfaces that it extend/implement
		// will be, too.
		continue;
	    }
	    Enumeration interfInterf = interf.allInterfaces.elements();
	    while( interfInterf.hasMoreElements() ){
		conditionalAdd( allInterfaces, interfInterf.nextElement() );
	    }
	}
    }

    public boolean
    countReferences(boolean isRelocatable) {
	if (isRelocatable) {
	    thisClass.incReference();
	    if (superClass != null) superClass.incReference();
	}
	// count interface references
	if (interfaces != null) {
	    for (int i = 0; i < interfaces.length ; i++) {
		interfaces[i].incReference();
	    }
	}
	// then count references from fields.
	if (fields != null) {
	    for (int i = 0; i < fields.length; i++) {
		fields[i].countConstantReferences(isRelocatable);
	    }
	}
	// then count references from code
	if (methods != null) {
	    for (int i = 0; i < methods.length; i++) {
		methods[i].countConstantReferences(cp, isRelocatable);
	    }
	}
	Attribute.countConstantReferences(classAttributes, isRelocatable);
	return true;
    }

    /* 
     * sweep over own constant pool. For each constant in it,
     * ensure that 
     * (a) the index is correct.
     * (b) two-word constants are followed by null entries,
     *     one-word entries are not.
     * (c) there are no UnicodeConstants.
     * (d) no constant is flagged as 'shared'
     * (e) no constant is unreferenced.
     */
    public void
    validateConstantPool(){
	ConstantObject[] constantPool = cp.getConstants();
	if (constantPool == null)
	    return; // it can happen!
	int nConsts = constantPool.length;
	if (constantPool[0] != null){
	    throw new ValidationException(
		    "Constant pool entry 0 must be zero in" +className);
	}
	for (int i = 1; i < nConsts; i++ ){
	    ConstantObject c = constantPool[i];
	    if (c == null){
		throw new ValidationException(
			"Unexpected null constant pool entry in "+className);
	    }
	    if (c.index != i){
		throw new ValidationException(
			"Bad constant self-index in "+className, c);
	    }
	    if (c.shared){
		throw new ValidationException(
			"Shared constant in private pool in "+className, c);
	    }
	    if (c instanceof DoubleValueConstant){
		// following element must be zero.
		i += 1;
		if ((i >= nConsts) || (constantPool[i] != null)){
		    throw new ValidationException(
			"Bad double value constant (no null) in "+className, c);
		}
	    }else if (c instanceof UnicodeConstant){
		throw new ValidationException(
		    "Constant pool contains Unicode constant in "+className, c);
	    }
	    if (c.getReferences() + c.getLdcReferences() == 0) {
		throw new ValidationException(
		    "Constant pool contains unreferenced constant in "+className, c);
	    }
	}
    }

    /*
     * This method can only be called at the very end, just before output
     * writing. It does a sanity check of data structures. Notably,
     * it makes sure that all constants referenced are or are not in constant pools.
     * If any assumptions are violated a ValidationException will be thrown.
     */
    public void
    validate( ConstantPool sharedPool ){
	ConstantObject constantPool[];
	if (superClassInfo == null){
	    if (!className.equals("java/lang/Object")){
		throw new ValidationException("Parentless class",className);
	    }
	}
	for ( int i = 0; i < fields.length; i++ ){
	    if (fields[i].parent != this){
		throw new ValidationException(
		    "Class containg non-member field", className);
	    }
	    fields[i].validate();
	}
	if (sourceFileAttr != null)
	    sourceFileAttr.validate();
	if (innerClassAttr != null)
	    innerClassAttr.validate();
	if (signatureAttr != null) {
	    signatureAttr.validate();
	}
	if (sharedPool == null){
	    // not sharing. Use our own pool.
	    constantPool = cp.getConstants();
	    // sweep our pool
	    validateConstantPool();
	}else{
	    constantPool = sharedPool.getConstants();
	}
	for ( int i = 0; i < methods.length; i++ ){
	    if (methods[i].parent != this){
		throw new ValidationException(
			"Class containg non-member method", className);
	    }
	    methods[i].validate(constantPool);
	}
    }

    public boolean
    relocateReferences(){
	for ( int i = 0; i < methods.length; i++ ){
	    methods[i].relocateConstantReferences( oldConstants );
	}
	return true;
    }

    public void
    clearMemberFlags( int flagsToClear ){
	int mask = ~ flagsToClear;
	int n;
	ClassMemberInfo members[];
	if ( fields != null ){
	    members = fields;
	    n = members.length;
	    for ( int i = 0; i < n; i++ ){
		members[i].flags &= mask;
	    }
	}
	if ( fields != null ){
	    members = methods;
	    n = members.length;
	    for ( int i = 0; i < n; i++ ){
		members[i].flags &= mask;
	    }
	}
    }

    private static void
    dumpComponentTable( PrintStream o, String title, ClassComponent t[] ){
	int n;
	if ( (t == null) || ((n=t.length) == 0) ) return;
	o.print( title ); o.println(/*NOI18N*/"["+n+"]:");
	for( int i = 0; i < n; i++ ){
	    if ( t[i] != null )
		o.println(/*NOI18N*/"\t["+i+/*NOI18N*/"]\t"+t[i]);
	}
    }

    private static void
    dumpConstantTable( PrintStream o, String title, ConstantPool cp ){
	ConstantObject t[] = cp.getConstants();
	int n;
	if ( (t == null) || ((n=t.length) == 0) ) return;
	o.print( title ); o.println(/*NOI18N*/"["+n+/*NOI18N*/"]:");
	o.println(/*NOI18N*/"\tPosition Index\tNrefs");
	for (int i = 0; i < n; i++) {
	    if (t[i] != null) {
		o.println(/*NOI18N*/"\t["+i+/*NOI18N*/"]\t"+t[i].index+
                          /*NOI18N*/"\t"+t[i].getReferences() +
                          /*NOI18N*/"\t"+t[i]);
            }
	}
    }
    private static void
    dumpMemberTable( PrintStream o, String title, ClassMemberInfo t[] ){
	int n;
	if ( (t == null) || ((n=t.length) == 0) ) return;
	o.print( title ); o.println(/*NOI18N*/":");
	for( int i = 0; i < n; i++ ){
	    if ( t[i] != null )
		o.println(/*NOI18N*/"\t["+i+/*NOI18N*/"]\t"+t[i].qualifiedName() );
	}
    }
    public void
    dump( PrintStream o ){
	o.print(Util.accessToString(access)+/*NOI18N*/"Class "+thisClass);
	if ( superClass != null )
	    o.print(/*NOI18N*/" extends "+superClass);
	if ( interfaces!=null && interfaces.length != 0 ){
	    o.print(/*NOI18N*/" implements");
	    for( int i = 0; i < interfaces.length ; i++ ){
		o.print(" "+interfaces[i]);
	    }
	}
	o.println();
	dumpComponentTable( o, /*NOI18N*/"Methods", methods );
	dumpComponentTable( o, /*NOI18N*/"Fields",  fields  );
	if ( fieldtable != null )
	    dumpMemberTable( o, /*NOI18N*/"Fieldtable", fieldtable );
	if ( methodtable != null )
	    dumpMemberTable( o, /*NOI18N*/"Methodtable", methodtable );
	dumpConstantTable( o, /*NOI18N*/"Constants", cp );
    }

    public static boolean resolveSupers(){
	Enumeration allclasses = ClassTable.elements();
	boolean ok = true;
	while( allclasses.hasMoreElements() ){
	    ClassInfo cinfo = (ClassInfo)allclasses.nextElement();
	    if ( cinfo.superClass==null ){
		// only java.lang.Object can be parentless
		if ( ! cinfo.className.equals( /*NOI18N*/"java/lang/Object" ) ){
		    log.println(Localizer.getString(
                        "classinfo.class_is_parent-less", cinfo.className));
		    ok = false;
		}
	    } else {
		ClassInfo superClassInfo =
                    cinfo.loader.lookupClass(cinfo.superClass.name.string);
		if ( superClassInfo == null ){
		    log.println(Localizer.getString(
                        "classinfo.class_is_missing_parent", cinfo.className,
                        cinfo.superClass.name.string ));
		    ok = false;
		} else {
		    cinfo.superClassInfo = superClassInfo;
		}
	    }
            if (cinfo.interfaces != null ) {
                for (int i = 0; i < cinfo.interfaces.length; ++i) {
                    ClassConstant cc = cinfo.interfaces[i];
                    if (cc.find() == null) {
                        log.println(Localizer.getString("classinfo.class_is_missing_parent", cinfo.className, cc.name.string));
                        ok = false;
                    }
                }
            }
	}
	return ok;
    }

    //
    // write still used by Strip
    //

    // write constants back out, just like we read them in.
    void writeConstantPool( DataOutput out ) throws IOException {
	ConstantObject constants[] = cp.getConstants();
	int num = constants==null ? 0 : constants.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.writing_constant_pool_entries", Integer.toString(num)));
	}
	out.writeShort( num );
	for (int i = 1; i < num; i+=constants[i].nSlots) {
	    constants[i].write( out );
	}
    }

    void writeInterfaces( DataOutput out ) throws IOException {
	int count = interfaces==null ? 0 : interfaces.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.writing_interfaces_implemented", Integer.toString(count)));
	}
	out.writeShort( count );
	for (int i = 0; i < count; i++) {
	    out.writeShort( interfaces[i].index );
	}
    }

    void writeFields( DataOutput out ) throws IOException {
	int count = fields==null ? 0 : fields.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.writing_field_members", Integer.toString(count)));
	}
	out.writeShort( count );
	for (int i = 0; i < count; i++) {
	    fields[i].write( out );
	}
    }

    void writeMethods( DataOutput out) throws IOException {
	int count = methods==null ? 0 : methods.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.writing_methods", Integer.toString(count)));
	}
	out.writeShort(count);
	for (int i = 0; i < count; i++) {
	    methods[i].write( out );
	}
    }

    void writeTableAttribute( DataOutput out, UnicodeConstant name, FMIrefConstant table[] ) throws IOException {
	if (verbose){
	    log.println(Localizer.getString("classinfo.writing_name", name.string));
	}
	out.writeShort( name.index );
	int n = table.length;
	out.writeInt( 2*n );
	for ( int i = 0; i < n; i++ ){
	    out.writeShort( table[i].index );
	}
    }

    void writeAttributes( DataOutput out ) throws IOException {
	int count = 0;

	if (classAttributes != null) {
	    count += classAttributes.length;
	}
	out.writeShort(count);

	if (classAttributes != null) {
	    for (int k = 0; k < classAttributes.length; k++) {
		classAttributes[k].write( out );
	    }
	}
    }

    public void
    write( DataOutput o ) throws IOException {
	writeConstantPool( o );
	o.writeShort( access );
	o.writeShort( thisClass.index );
	o.writeShort( superClass==null? 0 : superClass.index );
	writeInterfaces( o );
	writeFields( o );
	writeMethods( o );
	writeAttributes( o );
    }

    public String
    toString(){
	return /*NOI18N*/"ClassInfo-\""+className+/*NOI18N*/"\"";
    }

    // Convert ldc to ldc2
    public void relocateAndPackCode(boolean noCodeCompaction) {
        for ( int i = 0; i < methods.length; i++ )
            methods[i].relocateAndPackCode(cp, noCodeCompaction); 
    }

    // Pass lists of excluded methods & fields to the MethodInfo
    // and FieldInfo classes.
    static public void setExcludeLists( Vector methodExcludeList,
                                 Vector fieldExcludeList ) {
        MethodInfo.setExcludeList(methodExcludeList);
        FieldInfo.setExcludeList(fieldExcludeList);
        return;
    }
}
