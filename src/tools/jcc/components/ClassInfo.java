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
import jcc.Const;
import util.*;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;

//
// container for stuff about a class.
// can be in a classfile or in a module.
//

public
class ClassInfo
{
    public String		className;
    public int			access;
    public ClassConstant	thisClass;
    public ClassConstant	superClass;
    public ClassInfo		superClassInfo;

    // Tables for all fields, methods and constants of this class
    public FieldInfo		fields[];
    public MethodInfo		methods[];
    public ConstantObject	constants[];
    private ConstantObject	oldConstants[];
    public ConstantObject	symbols[];
    public ClassConstant	interfaces[];
    public FieldConstant	refFieldtable[];
    public MethodConstant	refMethodtable[];

    // In case we lay it all out here
    public FieldInfo		fieldtable[];
    public MethodInfo		methodtable[];
    public UnicodeConstant	fieldtableName;
    public UnicodeConstant	methodtableName;

    // Class attributes that we do not interpret
    public Attribute[]          classAttributes;
    public SourceFileAttribute  sourceFileAttr;

    public vm.ClassClass	vmClass; // used by in-core output writers

    public Vector		allInterfaces;

    protected boolean		verbose;
    protected PrintStream	log = System.out;
    public  ConstantPool	externalPool;
    public  static boolean      classDebug = false;

    public  int			flags;
    public static final int	INCLUDE_ALL = 1;

    public ClassInfo( boolean v ) {
	verbose = v;
	flags = INCLUDE_ALL; // by default, we want all members.
	// what else should be here?
    }

    private String genericNativeName;

    public final String getGenericNativeName() { 
        if (genericNativeName == null) 
	    genericNativeName = createGenericNativeName();
        return genericNativeName;
    }

    // This will be overridden by subclasses
    protected String createGenericNativeName() { 
        return Util.convertToClassName(className );
    }

    // Read in the constants from a classfile
    void readConstantPool( DataInput in ) throws IOException {
	int num = in.readUnsignedShort();

	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_entries_in_constant_pool", Integer.toString(num)));
	}
	constants = new ConstantObject[num];
	for (int i = 1; i < num; i+=constants[i].nSlots) {
	    constants[i] = ConstantObject.readObject( in );
	    constants[i].index = i;
	}
    }

    private void resolveConstants( ) {
	if (verbose){
	    log.println(Localizer.getString("classinfo.>>>resolving_constants"));
	}
	for (int i = 1; i < constants.length; i+=constants[i].nSlots) {
	    constants[i].resolve( symbols );
	}
    }

    
    protected void externalizeConstants( ConstantPool externalPool ){
	if (verbose){
	    log.println(Localizer.getString("classinfo.>>>externalizing_constants"));
	}
	// externalize immediately certain kinds of constants
	// we know to have no direct references.
	for (int i = 1; i < constants.length; i+=constants[i].nSlots) {
	    switch ( constants[i].tag ){
	    case Const.CONSTANT_UTF8:
	    case Const.CONSTANT_NAMEANDTYPE:
		// unquestionably, share these.
		constants[i] = externalPool.add( constants[i] );
		// FALLTHROUGH
	    default:
		constants[i].externalize( externalPool );
		break;
	    }
	}
    }

    /*
     * If we are using an external string table, then
     * we can make our own table smaller. At this point,
     * all non-code references into it are by object reference, NEVER
     * by index -- everything has been resolved! Thus we can
     * compact our table, deleting all the UnicodeConstants.
     * We adjust each constant's index entry accordingly.
     * Naturally, we preserve the null entries.
     *
     */
    public void smashConstantPool(){
	int nOld = constants.length;
	int nNew = 1;
	ConstantObject o;
	// first, count and index.
	for ( int i = 1; i < nOld; i += o.nSlots ){
	    o = constants[i];
	    if ( ! o.shared ){
		if ( o.references == 0 ){
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
	for ( int i = 1; i < nOld; i += o.nSlots ){
	    o = constants[i];
	    if ( (! o.shared ) && ( o.references != 0 ) ){
		// we're keeping it.
		newConstants[j] = o;
		j += o.nSlots;
	    }
	}
	oldConstants = constants;
	constants = newConstants;
    }

    // write constants back out, just like we read them in.
    void writeConstantPool( DataOutput out ) throws IOException {
	int num = constants==null ? 0 : constants.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.writing_constant_pool_entries", Integer.toString(num)));
	}
	out.writeShort( num );
	for (int i = 1; i < num; i+=constants[i].nSlots) {
	    constants[i].write( out );
	}
    }

    // Read the list of interfaces this class supports
    void readInterfaces( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_interfaces_implemented", Integer.toString(count)));
	}
	interfaces = new ClassConstant[count];
	for (int i = 0; i < count; i++) {
	    //interfaces[i] = (ClassConstant) symbols[in.readUnsignedShort()];
	    // interfaces not external -- they use own constant pool!
	    interfaces[i] = (ClassConstant) constants[in.readUnsignedShort()];
	}
    }

    void externalizeInterfaces( ConstantPool p ){
	int count = interfaces==null ? 0 : interfaces.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.>>>externalizing_interfaces_implemented"));
	}
	for (int i = 0; i < count; i++) {
	    interfaces[i] = (ClassConstant)p.dup( interfaces[i] );
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

    // Read the list of fields
    void readFields( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	fields = new FieldInfo[count];
	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_field_members", Integer.toString(count)));
	}

	for (int i = 0; i < count; i++) {
	    fields[i] = FieldInfo.readField(in, this);
	    fields[i].index = i;
	}
    }

    void externalizeFields( ConstantPool p ){
	int count = fields==null ? 0 : fields.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.>>>externalizing_field_members"));
	}
	for (int i = 0; i < count; i++) {
	    fields[i].externalize( p );
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

    // Read the list of methods from classfile
    void readMethods( DataInput in, boolean readCode ) throws IOException {
	int count = in.readUnsignedShort();
	methods = new MethodInfo[count];

	if(verbose){
	    log.println(Localizer.getString(
			"classinfo.reading_methods", Integer.toString(count)));
	}
	for (int i = 0; i < count; i++) {
	    methods[i] = MethodInfo.readMethod( in, this, readCode );
	    methods[i].index = i;
	}
    }

    void externalizeMethods( ConstantPool p ){
	int count = methods==null ? 0 : methods.length;
	if(verbose){
	    log.println(Localizer.getString("classinfo.>>>externalizing_methods"));
	}
	for (int i = 0; i < count; i++) {
	    methods[i].externalize( p );
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

    void readAttributes( DataInput in ) throws IOException {
	int count = in.readUnsignedShort();
	Vector clssAttr = new Vector();
	
	if(verbose){
	    log.println(Localizer.getString("classinfo.reading_attributes", Integer.toString(count)));
	}

	for (int i = 0; i < count; i++) {
	    int nameIndex = in.readUnsignedShort();
	    int bytes = in.readInt();
	    UnicodeConstant name = (UnicodeConstant)symbols[nameIndex];
	    if (name.string.equals(/*NOI18N*/"fieldtable")){
		fieldtableName = name;
		if (verbose){
		    log.println(Localizer.getString("classinfo.reading_name",name));
		}
		int n = bytes / 2;
		refFieldtable = new FieldConstant[ n ];
		for( int j = 0; j < n; j++ ){
		    refFieldtable[j] = (FieldConstant)symbols[ in.readUnsignedShort() ];
		}
	    } else if  (name.string.equals(/*NOI18N*/"methodtable")){
		methodtableName = name;
		if (verbose){
		    log.println(Localizer.getString("classinfo.reading_name", name));
		}
		int n = bytes / 2;
		refMethodtable = new MethodConstant[ n ];
		for( int j = 0; j < n; j++ ){
		    refMethodtable[j] = (MethodConstant)symbols[ in.readUnsignedShort() ];
		}
	    } else if  (name.string.equals(/*NOI18N*/"SourceFile")) {
		if (ClassInfo.classDebug) {
		    UnicodeConstant srcName =
			(UnicodeConstant)symbols[in.readUnsignedShort()];
		    sourceFileAttr =
			new SourceFileAttribute(name, bytes, srcName);
		    clssAttr.addElement(sourceFileAttr);
		} else {
		    byte[] b = new byte[bytes];
		    in.readFully(b);
		    clssAttr.addElement
			(new UninterpretedAttribute(name, bytes, b));
		}
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

    void externalizeAttributes( ConstantPool p ){
	Attribute.externalizeAttributes(classAttributes, p);
    }

    public void
    allocateFieldsFromFieldtable(){
	if ( refFieldtable == null ) return; // no can do.

	int n = refFieldtable.length;
	int nsuper;
	int fieldoff = 0;
	nsuper = ( superClassInfo == null ) ? 0 : superClassInfo.refFieldtable.length;
	if ( nsuper == 0 ){
	    fieldoff = 0;
	} else {
	    FieldInfo f =  refFieldtable[ nsuper-1 ].find();
	    if ( f.instanceOffset < 0 ){
		superClassInfo.allocateFieldsFromFieldtable();
	    }
	    fieldoff = f.instanceOffset + f.nSlots;
	}
	for ( int i = nsuper ; i < n; i++ ){
	    FieldInfo f = refFieldtable[ i ].find();
	    if ( f == null ){
		// this is not supposed to happen.
		System.out.println( Localizer.getString("classinfo.cannot_find_field", refFieldtable[i]));
		continue;
	    }
	    f.instanceOffset = fieldoff;
	    fieldoff += f.nSlots;
	}
	//
	// and while we're here, do methodtable as well.
	//
	n = refMethodtable.length;
	int methodoff = 0; // 1=>0 EJVM

	for ( int i = 0; i < n ; i++ ){
	    MethodInfo m = refMethodtable[ i ].find();

	    if ( m == null ){
		// this is not supposed to happen.
		System.out.println( Localizer.getString("classinfo.cannot_find_field", refMethodtable[i]));
		continue;
	    } else if ( m.methodTableIndex != methodoff ){
		if ( m.parent != this ){
		    // this method is in a superclass.
		    // which apparently hasn't set up its methodtable yet.
		    // so go do it.
		    superClassInfo.allocateFieldsFromFieldtable();
		} else {
		    // we do it here.?
		    System.out.println("Inconsistent refMethodtable in class "+this.toString());
		    m.methodTableIndex = methodoff;
		}
	    }
	    methodoff += 1;
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
	if ( fieldtableName != null )
	    count++;
	if ( methodtableName != null )
	    count++;

	if (classAttributes != null) {
	    count += classAttributes.length;
	}
	out.writeShort(count);

	if ( fieldtableName != null ){
	    writeTableAttribute( out, fieldtableName, refFieldtable );
	}
	if ( methodtableName != null ){
	    writeTableAttribute( out, methodtableName, refMethodtable );
	}
	if (classAttributes != null) {
	    for (int k = 0; k < classAttributes.length; k++) {
		classAttributes[k].write( out );
	    }
	}
    }

    // Read in the entire class
    // assume file is open, magic numbers are o.k.
    // read assumes reading from class file
    //
    private void
    doRead( DataInput in, boolean readCode, ConstantPool externalSymbols )
    throws IOException {

	externalPool = externalSymbols; // for convenience, later
	resolveConstants( );

	access = in.readUnsignedShort();
	thisClass = (ClassConstant) symbols[in.readUnsignedShort()];
	int sup = in.readUnsignedShort();
	if ( sup != 0 )
	    superClass = (ClassConstant) symbols[sup];
	className = thisClass.name.string;

	// Read the various parts of the class file
	readInterfaces( in );
	readFields( in );
	readMethods( in, readCode );
	readAttributes( in );
	enterClass( className );
    }

    public void
    read( DataInput in, boolean readCode, ConstantPool externalSymbols )
    throws IOException {
	readConstantPool( in );
	symbols = constants;	// symbol table == constant pool
	doRead( in, readCode, externalSymbols );
    }

    public void
    externalize( ConstantPool p ){
	if (verbose){
	    log.println(Localizer.getString("classinfo.externalizing_class", className));
	}
	externalizeConstants( p );
	thisClass = (ClassConstant)p.dup( thisClass );
	//thisClass.externalize(p);//redundant?
	if ( superClass != null ){
	    superClass = (ClassConstant)p.dup( superClass );
	    //superClass.externalize(p);//redundant?
	}
	//externalizeInterfaces( p ); // interfaces NOT externalized!
	externalizeMethods( p );
	externalizeFields( p );
	externalizeAttributes( p );
    }

    // Compute the fieldtable for a class.  This requires laying
    // out the fieldtable for our parent, then adding any fields
    // that are not inherited.
    public
    void buildFieldtable( ConstantPool cp ){
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
	    superClassInfo.buildFieldtable( cp );
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
	// here, we make the gross assumption that
	// if we're building a fieldtable, we're using a shared
	// external Constant Pool
	fieldtableName = (UnicodeConstant)cp.add( new UnicodeConstant(/*NOI18N*/"fieldtable") );
	    
    }

    private FMIrefConstant buildReference( ClassMemberInfo m, boolean isMethod, ConstantPool cp ){
	ClassConstant c = (ClassConstant) cp.dup( m.parent.thisClass );
	FMIrefConstant x;
	NameAndTypeConstant n = (NameAndTypeConstant) cp.add(
	    new NameAndTypeConstant( 
		(UnicodeConstant)cp.add( m.name), 
		(UnicodeConstant)cp.add( m.type )
	    )
	);
	if (isMethod){
	    x = new MethodConstant( c, n );
	}else{
	    x = new FieldConstant( c, n );
	}
	return (FMIrefConstant)cp.add( x );
    }

    public
    void buildReferenceFieldtable( ConstantPool cp ){
	if ( refFieldtable != null ) return; // already done, it says here.
	if ( fieldtableName == null ){
	    fieldtableName = (UnicodeConstant)cp.add( new UnicodeConstant(/*NOI18N*/"fieldtable") );
	}
	buildFieldtable( cp );
	int n = fieldtable.length;
	refFieldtable = new FieldConstant[ n ];
	for ( int i = 0; i < n; i++ ){
	    refFieldtable[i] = (FieldConstant)buildReference( fieldtable[i], false, cp );
	}
    }

    // Compute the method table for a class.  This requires laying
    // out the method table for our parent, then adding any methods
    // that are not inherited.
    public
    void buildMethodtable( ConstantPool cp ) {
	if ( this.methodtable != null ) return; // already done.
	MethodInfo table[];
	MethodInfo methods[] = this.methods;
	ClassInfo sup = superClassInfo;
	if ((sup != null) && ( (sup.access&Const.ACC_INTERFACE)==(this.access&Const.ACC_INTERFACE) ) ) {
	    sup.buildMethodtable( cp );
	    table = sup.methodtable;
	} else {
	    table = new MethodInfo[0];
	}

	// allocate a temporary table that is certainly large enough.
	MethodInfo newTable[] = new MethodInfo[table.length + methods.length];
	int index = table.length;
	System.arraycopy( table, 0, newTable, 0, index );
	
	if (sup == null) { 
	    // finalize() goes into slot 0 of java.lang.Object
	    // FY: Removed for KVM.  We have no finalize() in slot 0.
		// index++;
	}

    method_loop:
	for (int i = 0; i < methods.length; i++) {
	    if ((methods[i].access & (Const.ACC_STATIC|Const.ACC_PRIVATE)) != 0) {
		continue method_loop;
	    } else if (methods[i].name.string.equals(/*NOI18N*/"<init>")) {
		continue method_loop;
	    } else if (sup == null && methods[i].name.string.equals(/*NOI18N*/"finalize") 
		                   && methods[i].type.string.equals(/*NOI18N*/"()V")) {
	        newTable[0] = methods[i];
		newTable[0].methodTableIndex = 0; // 1=>0 EJVM
		continue method_loop;
	    } 
	    int j;
	    int thisID = methods[i].getID();
	    for ( j = 0; j < table.length; j++) {
		if (thisID == table[j].getID()) {
		    newTable[j] = methods[i];
		    newTable[j].methodTableIndex = j + 0; // 1=>0 EJVM
		    continue method_loop;
		}
	    }
	    // If we're not overriding our parent's method we do add
	    // a new entry to the method table.
	    newTable[index] = methods[i];
	    newTable[index].methodTableIndex = index + 0; // 1=>0 EJVM
	    index++;
	}

	// now allocate a table of the correct size.
	MethodInfo methodTable[] = new MethodInfo[index];
	System.arraycopy( newTable, 0, methodTable, 0, index );

	this.methodtable =  methodTable;
	//
	// here, we make the gross assumption that
	// if we're building a methodtable, we're using a shared
	// external Constant Pool
	methodtableName = (UnicodeConstant)cp.add( new UnicodeConstant(/*NOI18N*/"methodtable") );
    }

    public
    void buildReferenceMethodtable( ConstantPool cp ){
	if ( refMethodtable != null ) return; // already done, it says here.
	if ( methodtableName == null ){
	    methodtableName = (UnicodeConstant)cp.add( new UnicodeConstant(/*NOI18N*/"methodtable") );
	}
	buildMethodtable( cp );
	int n = methodtable.length;
	refMethodtable = new MethodConstant[ n ];
	for ( int i = 0; i < n; i++ ){
	    refMethodtable[i] = (MethodConstant)buildReference( methodtable[i], true, cp );
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
	    if ( ( interf == null ) || ( (interf.access&Const.ACC_INTERFACE) == 0 ) ){
		System.err.println(Localizer.getString("classinfo.class_which_should_be_an_interface_but_is_not", className, interfaces[i]));
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
    findReferences(){
	try {
	    for ( int i = 0; i < methods.length; i++ ){
		methods[i].findConstantReferences();
	    }
	} catch ( DataFormatException e ){
	    return false;
	}
	return true;
    }

    public boolean
    countReferences( boolean isRelocatable ){
	thisClass.incReference();
	if ( superClass != null ) superClass.incReference();
	// count interface references
	if ( interfaces!=null ){
	    for ( int i = 0; i < interfaces.length ; i++ ){
		interfaces[i].incReference();
	    }
	}
	// then count references from fields.
	if ( fields != null ){
	    for ( int i = 0; i < fields.length; i++ ){
		fields[i].countConstantReferences( isRelocatable );
	    }
	}
	// then count references from code
	if ( methods != null ){
	    for ( int i = 0; i < methods.length; i++ ){
		methods[i].countConstantReferences( constants, isRelocatable );
	    }
	}
	Attribute.countConstantReferences(classAttributes, isRelocatable);
	return true;
    }

    public boolean
    relocateReferences(){
	try {
	    for ( int i = 0; i < methods.length; i++ ){
		methods[i].relocateConstantReferences( oldConstants );
	    }
	} catch ( DataFormatException e ){
	    return false;
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
    dumpConstantTable( PrintStream o, String title, ConstantObject t[] ){
	int n;
	if ( (t == null) || ((n=t.length) == 0) ) return;
	o.print( title ); o.println(/*NOI18N*/"["+n+/*NOI18N*/"]:");
	o.println(/*NOI18N*/"\tPosition Index\tNrefs");
	for( int i = 0; i < n; i++ ){
	    if ( t[i] != null )
		o.println(/*NOI18N*/"\t["+i+/*NOI18N*/"]\t"+t[i].index+/*NOI18N*/"\t"+t[i].references+/*NOI18N*/"\t"+t[i]);
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
	else
	    dumpComponentTable( o, /*NOI18N*/"Fieldtable-by-reference", refFieldtable );
	if ( methodtable != null )
	    dumpMemberTable( o, /*NOI18N*/"Methodtable", methodtable );
	else
	    dumpComponentTable( o, /*NOI18N*/"Methodtable-by-reference", refMethodtable );
	dumpConstantTable( o, /*NOI18N*/"Constants", constants );
    }

    /**
     * We keep track of classes by hashing them by name when
     * we read them. They can be looked up using lookupClass,
     * which will take a classname string as parameter.
     */
    public static Hashtable classtable = new Hashtable();

    protected void enterClass( String key ){
	// should check to see if a class of this name is already there...
	if ( classtable.containsKey( className ) ){
	    System.err.println(Localizer.getString("classinfo.class_table_already_contains", className));
	    return;
	}
	classtable.put( key, this );
	// if a classvector has been created, we need to add this.
	// at end should be sufficient.
	vm.ClassClass.appendClassElement( this );
    }

    public static ClassInfo lookupClass( String key ){
	return (ClassInfo)classtable.get( key );
    }

    public static int nClasses(){
	return classtable.size();
    }

    public static Enumeration allClasses(){
	return classtable.elements();
    }

    public static boolean resolveSupers(){
	Enumeration allclasses = allClasses();
	boolean ok = true;
	while( allclasses.hasMoreElements() ){
	    ClassInfo c = (ClassInfo)allclasses.nextElement();
	    if ( c.superClass==null ){
		// only java.lang.Object can be parentless
		if ( ! c.className.equals( /*NOI18N*/"java/lang/Object" ) ){
		    System.out.println(Localizer.getString("classinfo.class_is_parent-less", c.className));
		    ok = false;
		}
	    } else {
		ClassInfo s = ClassInfo.lookupClass( c.superClass.name.string );
		if ( s == null ){
		    System.out.println(Localizer.getString("classinfo.class_is_missing_parent", c.className, c.superClass.name.string ));
		    ok = false;
		} else {
		    c.superClassInfo = s;
		}
	    }
	}
	return ok;
    }

    public String
    toString(){
	return /*NOI18N*/"ClassInfo-\""+className+/*NOI18N*/"\"";
    }

    // Convert ldc to ldc2
    public void relocateAndPackCode() {
        for ( int i = 0; i < methods.length; i++ )
            methods[i].relocateAndPackCode(constants); 
    }
}
