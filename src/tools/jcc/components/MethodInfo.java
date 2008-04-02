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
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Hashtable;
import jcc.Const;
import jcc.EVMConst;
import jcc.Util;
import util.DataFormatException;

// Class for representing every method in a class

public
class MethodInfo extends ClassMemberInfo implements Const
{
    public int	argsSize;
    public int	stack;
    public int	locals;
    public int	methodTableIndex = -1;

    public byte	code[];

    private Attribute		 methodAttributes[];
    public CodeAttribute	 codeAttribute;
    public Attribute	    	 codeAttributes[];
    public ExceptionEntry	 exceptionTable[];
    public ClassConstant	 exceptionsThrown[];
    private boolean		 checkedDebugTables = false;
    private LineNumberTableEntry lineNumberTable[];
    private LocalVariableTableEntry localVariableTable[];
    public  StackMapFrame        stackMapTable[];
    public  vm.VMMethodInfo      vmMethodInfo;

    /* An array of the methods called by invokevirtual_quick and
     * invokevirtualobject_quick.  (After quickening, before inlining). 
     */
    public MethodInfo[] targetMethods;

    /**
     * The following are arrays of indexes into the
     * code array of references to the constant pool:
     * ldcInstructions lists the instructions with a one-byte index.
     * wideConstantRefInstructions lists the instructions with a two-byte index.
     * In each case, the index is that of the opcode: the actual reference
     * begins with the following byte.
     * Entries of value -1 are ignored.
     */
    public int		ldcInstructions[];
    public int		wideConstantRefInstructions[];

    public MethodInfo( int name, int sig, int access, ClassInfo p ) {
	super( name, sig, access, p );
    }

    public void
    initializeClassDebugTables() {
	int nattr = (codeAttributes == null) ? 0 : codeAttributes.length;
	// parse code attributes
	for (int i = 0; i < nattr; i++) {
	    Attribute a = codeAttributes[i];
	    if (a.name.string.equals("LineNumberTable")) {
		lineNumberTable = ((LineNumberTableAttribute)a).data;
	    }
	    if (a.name.string.equals("LocalVariableTable")) {
		localVariableTable = ((LocalVariableTableAttribute)a).data;
	    }
	    if (a.name.string.equals("StackMap")) {
		stackMapTable = ((StackMapAttribute)a).data;
	    }

	}
    }

    public LineNumberTableEntry []
    getLineNumberTable(){
	if ( !checkedDebugTables ){
	    initializeClassDebugTables();
	    checkedDebugTables = true;
	}
	return lineNumberTable;
    }

    public LocalVariableTableEntry []
    getLocalVariableTable(){
	if ( !checkedDebugTables ){
	    initializeClassDebugTables();
	    checkedDebugTables = true;
	}
	return localVariableTable;
    }

    public boolean 
    hasLineNumberTable(){
	if ( !checkedDebugTables ){
	    initializeClassDebugTables();
	    checkedDebugTables = true;
	}
	return (lineNumberTable != null) && (lineNumberTable.length != 0);
    }

    public boolean 
    hasLocalVariableTable(){
	if ( !checkedDebugTables ){
	    initializeClassDebugTables();
	    checkedDebugTables = true;
	}
	return (localVariableTable != null) && (localVariableTable.length != 0);
    }

    public boolean
    throwsExceptions(){
	return exceptionsThrown != null;
    }

    public ClassConstant[]
    getExceptionsThrown(){
	return exceptionsThrown;
    }

    public int
    nExceptionsThrown(){
	return ( exceptionsThrown == null ) ? 0 : exceptionsThrown.length;
    }

    private String nativeName;
    public String getNativeName(boolean isJNI) { 
        if (nativeName == null) { 
	    nativeName = isJNI ? getJNIName() : getOldNativeName();
	}
	return nativeName;
    }
    
    public String getJNIReturnType() {
	return Util.parseReturnType(this.type.string);
    }

    private String getJNIName() { 
        ClassInfo ci = parent;
        String classname = ci.className;
	String methodname = this.name.string;
	int nmethods = ci.methods.length;
	String typeName = null;	// by default, don't need type
	for (int j = 0; j < nmethods; j ++ ){
	    MethodInfo m = ci.methods[j];
	    if ((m != this) && ( (m.access&Const.ACC_NATIVE) != 0 )) {
	        if (m.name.equals(this.name)) {
		    // Two native methods with the same name.  Need type name
		    typeName = this.type.string;
		    break;
		}
	    }
	} 
	return Util.convertToJNIName(classname, methodname, typeName);
    }

    private String getOldNativeName() { 
        ClassInfo ci = parent;
	String methodname = this.name.string;
	StringBuffer sbuf = new StringBuffer(/*NOI18N*/"Java_")
	                          .append(ci.getGenericNativeName())
	                          .append('_');
	if (methodname.indexOf('_') == -1) {
	    // optimization.  Most methods don't have an _ in them
	    sbuf.append(methodname);
	} else { 
	    for (int i = 0; i < methodname.length(); i++) { 
	      if (methodname.charAt(i) == '_') {
		  sbuf.append(/*NOI18N*/"_0005f");
	      } else {
		  sbuf.append(methodname.charAt(i));
	      }
	    }
	}
	sbuf.append("_stub");
	return sbuf.toString();
    }

    /*
     * A methods attributes are Code and Exceptions.
     */
    private static Hashtable methodAttributeTypes = new Hashtable();

    static {
	methodAttributeTypes.put( "Code", CodeAttributeFactory.instance );
	methodAttributeTypes.put( "Exceptions", ExceptionsAttributeFactory.instance );
    }

    // Read in method attributes from classfile
    void
    readAttributes( DataInput in, ConstantObject locals[], ConstantObject globals[], boolean readCode ) throws IOException {

	methodAttributes = Attribute.readAttributes( in, locals, globals, methodAttributeTypes, false );

	// oops, we read the code.
	// we'll fix this someday.

	//
	// parse special attributes
	//
	if ( methodAttributes != null ){
	    for ( int i = 0; i < methodAttributes.length; i ++ ){
		Attribute a = methodAttributes[i];
		if (a.name.string.equals("Code") ) {
		    CodeAttribute ca = (CodeAttribute)a;
		    this.locals = ca.locals;
		    this.stack  = ca.stack;
		    this.code   = ca.code;
		    this.exceptionTable = ca.exceptionTable;
		    this.codeAttributes = ca.codeAttributes;
		    this.codeAttribute = ca;
		} else if (a.name.string.equals("Exceptions") ) {
		    this.exceptionsThrown = ((ExceptionsAttribute)a).data;
		}
	    }
	}

    }

    public static MethodInfo
    readMethod( DataInput in, ClassInfo p, boolean readCode ) throws IOException {
	int access = in.readUnsignedShort();
	int name   = in.readUnsignedShort();
	int sig    = in.readUnsignedShort();
	MethodInfo m = new MethodInfo( name, sig, access, p );
	// the bad thing is, we really cannot go far
	// without resolving. So we resolve here.
	m.resolve( p.symbols );

	m.argsSize = Util.argsSize(m.type.string);
	if ((m.access & ACC_STATIC) == 0) {
	    m.argsSize++;
	}

	m.readAttributes( in, p.constants, p.symbols, readCode );
	return m;
    }

    public void externalize( ConstantPool p ){
	super.externalize( p );
	Attribute.externalizeAttributes( methodAttributes, p );
	Attribute.externalizeAttributes( codeAttributes, p );
    }

    public void write( DataOutput o ) throws IOException{
	o.writeShort( access );
	o.writeShort( name.index );
	o.writeShort( type.index );
	Attribute.writeAttributes( methodAttributes, o, false );
    }

    private static int getInt(byte[] code, int w) { 
	return	(  (int)code[w]   << 24 ) |
		(( (int)code[w+1] &0xff ) << 16 ) |
		(( (int)code[w+2] &0xff ) << 8 ) |
		 ( (int)code[w+3] &0xff );
    }

    public int getInt( int w ){
	return	getInt(code, w);
    }

    public int getUnsignedShort( int w ){
	return	(( (int)code[w] &0xff ) << 8 ) | ( (int)code[w+1] &0xff );
    }

    public int getShort( int w ){
	return	(( (int)code[w]) << 8 ) | ( (int)code[w+1] &0xff );
    }

    //
    // Private Utility functions
    private void putInt(byte array[], int offset, int val) {
        array[offset] =   (byte) ((val >> 24) & 0xFF);
        array[offset+1] = (byte) ((val >> 16) & 0xFF);
        array[offset+2] = (byte) ((val >> 8) & 0xFF);
        array[offset+3] = (byte) (val & 0xFF);
    }

    private void putShort( int w, short v ){
	code[w]   = (byte)(v>>>8);
	code[w+1] = (byte)v;
    }

    public void findConstantReferences() throws DataFormatException {
	if ( code == null ) return; // no code, no references.
	int 	ldc[]  = new int[ code.length / 2 ];
	int 	wide[] = new int[ code.length / 3 ];
	int	nldc   = 0;
	int	nwide  = 0;
	int	ncode  = code.length;
	int	opcode;
	for( int i = 0; i < ncode; /*nothing*/){
	    switch (opcode = (int)code[i]&0xff) {
	    case opc_tableswitch:
		i = (i + 4) & ~3;
		int low = getInt( i+4);
		int high = getInt( i+8);
		i += (high - low + 1) * 4 + 12;
		break;

	    case opc_lookupswitch:
		i = (i + 4) & ~3;
		int pairs = getInt(i+4);
		i += pairs * 8 + 8;
		break;

	    case opc_wide:
		switch ((int)code[i+1]&0xff) {
		case opc_aload:
		case opc_iload:
		case opc_fload:
		case opc_lload:
		case opc_dload:
		case opc_istore:
		case opc_astore:
		case opc_fstore:
		case opc_lstore:
		case opc_dstore:
		case opc_ret:
		    i += 4;
		    break;

		case opc_iinc:
		    i += 6;
		    break;

		default:
		    throw new DataFormatException( parent.className + "." +
			name.string + ": unknown wide " +
			"instruction: " + code[i+1] );
		}
		break;
	    case opc_ldc:
		ldc[nldc++] = i;
		i += opcLengths[opcode];
		break;
	    case opc_ldc_w:
	    case opc_ldc2_w:
	    case opc_getstatic:
	    case opc_putstatic:
	    case opc_getfield:
	    case opc_putfield:
	    case opc_invokevirtual:
	    case opc_invokespecial:
	    case opc_invokestatic:
	    case opc_invokeinterface:
	    case opc_new:
	    case opc_anewarray:
	    case opc_checkcast:
	    case opc_instanceof:
	    case opc_multianewarray:

            case opc_getstatic_fast:
            case opc_getstaticp_fast:
            case opc_getstatic2_fast:
            case opc_putstatic_fast:
            case opc_putstatic2_fast:
            case opc_invokevirtual_fast:
            case opc_invokespecial_fast:
            case opc_invokestatic_fast:
            case opc_invokeinterface_fast:
            case opc_new_fast:
            case opc_anewarray_fast:
            case opc_multianewarray_fast:
            case opc_checkcast_fast:
            case opc_instanceof_fast:
		wide[nwide++] = i;
		i += opcLengths[opcode];
		break;
	    default: 
		i +=  opcLengths[opcode];
		break;
	    }
	}
	// not knowing any better, we allocated excess capacity.
	// allocate and fill appropriately-sized arrays.
	ldcInstructions = new int[ nldc ];
	System.arraycopy( ldc, 0, ldcInstructions, 0, nldc );
	ldc = null;
	wideConstantRefInstructions = new int[ nwide ];
	System.arraycopy( wide, 0, wideConstantRefInstructions, 0, nwide );
	wide = null;
    }

    private static int opcodeLength(byte[] code, int pc) {
        int old_pc;
	int opcode = (int)code[pc]&0xff;
        switch (opcode) {

	case opc_tableswitch: 
	    old_pc = pc;
  	    pc = (pc + 4) & ~3;
	    int low = getInt(code, pc + 4);
	    int high = getInt(code, pc + 8);
	    pc += (high - low + 1) * 4 + 12;
	    return pc - old_pc;

	case opc_lookupswitch:
	    old_pc = pc;
	    pc = (pc + 4) & ~3;
	    int pairs = getInt(code, pc + 4);
	    pc += pairs * 8 + 8;
	    return pc - old_pc;

	case opc_wide:
	    if (((int)code[pc + 1]&0xff) == opc_iinc) 
	        return 6;
	    return 4;

	default:
	    return opcLengths[opcode];
	}
    }

    public int opcodeLength (int pc) { 
        return opcodeLength(code, pc);
    }

    public void
    countConstantReferences( ConstantObject table[], boolean isRelocatable ){
	super.countConstantReferences();
	Attribute.countConstantReferences( methodAttributes, isRelocatable );
	Attribute.countConstantReferences( codeAttributes, isRelocatable );
	if ( code == null ) return; // no code, no relocation
	if ( ldcInstructions != null ){
	    int list[] = ldcInstructions;
	    int n = list.length;
	    for (int i = 0; i < n; i++){
		int loc = list[i];
		if ( loc==-1 ) continue;
		table[ (int)code[loc+1]&0xff ].incReference();
	    }
	}
	if ( wideConstantRefInstructions != null ){
	    int list[] = wideConstantRefInstructions;
	    int n = list.length;
	    for (int i = 0; i < n; i++){
		int loc = list[i];
		if ( loc==-1 ) continue;
		table[ getUnsignedShort(loc+1) ].incReference();
	    }
	}
    }

    public void
    relocateConstantReferences( ConstantObject table[] ) throws DataFormatException {
	if ( code == null ) return; // no code, no relocation
	if ( ldcInstructions != null ){
	    int list[] = ldcInstructions;
	    int n = list.length;
	    for (int i = 0; i < n; i++){
		int j = list[i]+1;
		if ( j <= 0 ) continue;
		ConstantObject c = table[ (int)code[j]&0xff ];
		if ( c.shared )
		    throw new DataFormatException("code reference to shared constant");
		int v = c.index;
		if ( v < 0 )
		    throw new DataFormatException("code reference to deleted constant at "+qualifiedName()+"+"+Integer.toHexString(j));
		if ( v > 255 )
		    throw new DataFormatException("ldc subscript out of range at "+qualifiedName()+"+"+Integer.toHexString(j));
		code[j] = (byte)v;
	    }
	}
	if ( wideConstantRefInstructions != null ){
	    int list[] = wideConstantRefInstructions;
	    int n = list.length;
	    for (int i = 0; i < n; i++){
		int j = list[i]+1;
		if ( j <= 0 ) continue;
		ConstantObject c = table[ getUnsignedShort(j) ];
		if ( c.shared )
		    throw new DataFormatException("code reference to shared constant at "+qualifiedName()+"+"+Integer.toHexString(j));
		int v = c.index;
		if ( v < 0 )
		    throw new DataFormatException("code reference to deleted constant at "+qualifiedName()+"+"+Integer.toHexString(j));
		putShort( j, (short)v );
	    }
	}
    }
    

    public void replaceCode(int start, int end) { 
	replaceCode(start, end, new byte[0]);
    }

    public void replaceCode(int start, int end, int op1) { 
	byte code[] = { (byte)op1 };
	replaceCode(start, end, code);
    }

    public void replaceCode(int start, int end, int op1, int op2) { 
	byte code[] = { (byte)op1, (byte)op2 };
	replaceCode(start, end, code);
    }

    public void replaceCode(int start, int end, int op1, int op2, int op3) { 
	byte code[] = { (byte)op1, (byte)op2, (byte)op3 };
	replaceCode(start, end, code);
    }

    
    public java.util.BitSet getLabelTargets() { 
        java.util.BitSet result = new java.util.BitSet();
	int ncode  = code.length;
	int nextpc;

	for(int pc = 0; pc < ncode; pc = nextpc) {
	    nextpc = pc + opcodeLength(pc);
	    int opcode = (int)code[pc]&0xff;
	    switch (opcode) {
	      
	    case opc_tableswitch: 
	    case opc_lookupswitch: 
	        int i = (pc + 4) & ~3;
		int delta = (opcode == opc_tableswitch) ? 4 : 8;
		result.set(pc + getInt(i)); // default
		for (i = i + 12; i < nextpc; i += delta)
		    result.set(pc + getInt(i));
		break;

	    case opc_jsr: 
	        result.set(pc + 3);
	    case opc_goto:
	    case opc_ifeq: case opc_ifge: case opc_ifgt:
	    case opc_ifle: case opc_iflt: case opc_ifne:
	    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpge: 
	    case opc_if_icmplt: case opc_if_icmpgt: case opc_if_icmple: 
	    case opc_if_acmpeq: case opc_if_acmpne:
	    case opc_ifnull: case opc_ifnonnull:
	        result.set(pc + getShort(pc + 1)); 
		break;

	    case opc_jsr_w:
	        result.set(pc + 5);
	    case opc_goto_w:
	        result.set(pc + getInt(pc + 1)); 
		break;
	    }
	}
	return result;
    }

    public void replaceCode(int start, int end, byte[] replaceCode) { 
        if (end - start < replaceCode.length) { 
	    // System.out.println("  Cannot yet do expansion!!");
	    return;
	}
        if (exceptionTable != null && exceptionTable.length > 0) {   
	    for (int i = 0; i < exceptionTable.length; i++) { 
	       int startPC = exceptionTable[i].startPC;
	       int endPC = exceptionTable[i].endPC;
	       if (startPC >= start && startPC < end) 
		   return;
	       if (endPC >= start && endPC < end) 
		   return;
	    }
	}
	int startExtra = start + replaceCode.length;
	int extra = end - startExtra;
	
	System.arraycopy(replaceCode, 0, code, start, replaceCode.length);

	for (int i = startExtra; i < end; i++) 
	    code[i] = (byte)opc_nop;
    }

    public void disassemble() { 
        System.out.println(disassemble(code, 0, code.length));
    }

    public String disassemble(int start, int end) { 
	return disassemble(code, start, end);
    }

    /**
     * Return the byte stored at a given index from the offset
     * within code bytes
     */  
    private static final int at(byte codeBytes[], int index) {
    	return codeBytes[index] & 0xFF;
    }

    /**
     * Return the short stored at a given index from the offset
     * within code bytes
     */  
    private static final int shortAt(byte codeBytes[], int index) {
    	return ((codeBytes[index] & 0xFF) << 8) 
    	    | (codeBytes[index+1] & 0xFF);
    }

    public static String disassemble(byte[] codeBytes, int start, int end) { 
	// Output goes into a string
	StringWriter sw = new StringWriter();
	PrintWriter output = new PrintWriter(sw);
	
	for (int offset = start; offset < end; ) {
	    int opcode = at(codeBytes, offset);

	    if (offset > start) 
	        output.print("; ");
            
            output.print(opcodeName(opcode));

	    switch (opcode) {
	    case opc_aload: case opc_astore:
	    case opc_fload: case opc_fstore:
	    case opc_iload: case opc_istore:
	    case opc_lload: case opc_lstore:
	    case opc_dload: case opc_dstore:
	    case opc_ret:
		output.print(" " + at(codeBytes, offset+1));
		offset += 2;
		break;
		    
	    case opc_iinc:
		output.print(" " + at(codeBytes, offset+1)  + " " + 
			     (byte) at(codeBytes, offset +2));
		offset += 3;
		break;

	    case opc_newarray:
		switch (at(codeBytes, offset+1)) {
		case T_INT:    output.print(" int");    break;
		case T_LONG:   output.print(" long");   break;
		case T_FLOAT:  output.print(" float");  break;
		case T_DOUBLE: output.print(" double"); break;
		case T_CHAR:   output.print(" char");   break;
		case T_SHORT:  output.print(" short");  break;
		case T_BYTE:   output.print(" byte");   break;
		case T_BOOLEAN:output.print(" boolean");   break;
		default:             output.print(" BOGUS"); break;
		}
		offset += 2;
		break;

	    case opc_anewarray_fast:
	    case opc_anewarray: {
		int index =  shortAt(codeBytes, offset+1);
		output.print(" class #" + index + " ");
		offset += 3;
		break;
	    }
		      
	    case opc_sipush:
		output.print(" " + (short) shortAt(codeBytes, offset+1));
		offset += 3;
		break;

	    case opc_bipush:
		output.print(" " + (byte) at(codeBytes, offset+1));
		offset += 2;
		break;

	    case opc_ldc: {
		int index = at(codeBytes, offset+1);
		output.print(" #" + index + " ");
		offset += 2;
		break;
	    }

            case opc_getstatic_fast:
            case opc_getstaticp_fast:
            case opc_getstatic2_fast:
            case opc_putstatic_fast:
            case opc_putstatic2_fast:
            case opc_unused_d5:
            case opc_invokevirtual_fast:
            case opc_invokespecial_fast:
            case opc_invokestatic_fast:
            case opc_invokeinterface_fast:
            case opc_new_fast:
            case opc_multianewarray_fast:
            case opc_checkcast_fast:
            case opc_instanceof_fast:
	    case opc_ldc_w: case opc_ldc2_w:
	    case opc_instanceof: case opc_checkcast:
	    case opc_new:
	    case opc_putstatic: case opc_getstatic:
	    case opc_putfield: case opc_getfield:
	    case opc_invokevirtual:
	    case opc_invokespecial:
	    case opc_invokestatic: {
		int index = shortAt(codeBytes, offset+1);
		output.print(" #" + index + " ");
		offset += 3;
		break;
	    }

            case opc_getfield_fast:
            case opc_getfieldp_fast:
            case opc_getfield2_fast:
            case opc_putfield_fast:
            case opc_putfield2_fast: { 
		int index = at(codeBytes, offset+1);
		output.print(" [" + index + "] ");
		offset += 3;
		break;
	    }

	    case opc_jsr: case opc_goto:
	    case opc_ifeq: case opc_ifge: case opc_ifgt:
	    case opc_ifle: case opc_iflt: case opc_ifne:
	    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpge:
	    case opc_if_icmpgt: case opc_if_icmple: case opc_if_icmplt:
	    case opc_if_acmpeq: case opc_if_acmpne:
	    case opc_ifnull: case opc_ifnonnull: {
		int target = offset + (short) shortAt(codeBytes,offset+1);
		output.print(" " + target);
		offset += 3;
		break;
	    }

	    default:
		offset += opcodeLength(codeBytes, offset);
		break;
	    }
	}
        
	output.close();
	return sw.toString();
    }

    public static String opcodeName (int opcode) { 
	return opcNames[opcode];
    }

    public String toString(){
	String r = "Method: "+super.toString();
	if ( code != null ){
	    r += " {"+code.length+" bytes of code}";
	}
	return r;
    }

    // Case 1: expand code.
    // Convert ldc to ldc2: a. Insert extra bytes
    //                      b. Fix all branch targets/exception ranges
    // Case 2: smash code
    // ldc_w_fast has index which is less than 255. Change to use
    // ldc_w.
    public void relocateAndPackCode (ConstantObject co[]) {

        if (code == null)
            return;

        int opcode, adjustment = 0;
        int newOffsets[] = new int[code.length];
        int indexByPC[] = new int[code.length];

        // First figure out where we'll have to insert extra bytes in
        // order to fit opc_ldc_w instead of opc_ldc instructions.
        for (int pc = 0, pcindex = 0; pc < code.length;
		 pc = pc + opcodeLength(pc), pcindex++) {
            opcode = (int) code[pc]&0xFF;
            newOffsets[pcindex] = pc + adjustment;
            indexByPC[pc] = pcindex;

          /*******
           * switch (opcode) {
           *    case opc_ldc: {
           *        // a conversion table which maps pcValue to new index.
           *        int oldindex = (int)(code[pc+1] & 0xFF);
           *        int index = co[oldindex].index;
           *
           *        if (index >= 0x100)
           *            adjustment++;
           *        break;
           *    }
           *
           *
	   *  case opc_ldc_w: {
	   *      // a conversion table which maps pcValue to new index.
           *      int oldindex = (int)(((code[pc+1]&0xFF) << 8)
	   *    		 | (code[pc+2]&0xFF));
	   *      int index = co[oldindex].index;
           *
           *      if (index < 0x100)
           *          adjustment--;
           *      break;
           *  }
           *
           *
           *
           *  case opc_goto: {
           *      // Calculate the displacement, sign extend high byte
           *      int displ = (code[pc+1] << 8) | (code[pc+2] & 0xFF);
           *
	   *      if (displ == 3) {
	   *          adjustment -= 3; // remove no-use goto's.
	   *      }
	   *      break;
	   *  }
           *
	   *  case opc_nop: {
	   *      adjustment--; // remove
	   *      break;
	   *  }
           *
           *
	   *  case opc_tableswitch:
	   *  case opc_lookupswitch: {
	   *      int oldExtraPC = (( pc + 4 ) & ~3);
	   *      int newExtraPC = (( pc + adjustment + 4) & ~3);
	   *      adjustment = newExtraPC - oldExtraPC;
	   *      break;
	   *  }
           * }
           *******/
        }

        // Now copy the code to the new location. At the same
        // time, we adjust all branch targets.
        byte newCode[] = new byte[code.length + adjustment];

        for (int pc = 0, pcindex = 0; pc < code.length;
		 pc = pc + opcodeLength(pc), pcindex++) {
            int outPos = newOffsets[pcindex];
	    int inPos = pc;

	    opcode = (int) code[pc]&0xFF;

            for (int i = 0; i < opcodeLength(pc); i++) {
                newCode[outPos + i] = code[pc + i];
            }

            switch (opcode) {
	      case opc_ldc: {
		  int oldindex = (int)(code[pc+1] & 0xFF);
                  int index = co[oldindex].index;
                  if (index >= 0x100) {
                      new RuntimeException("load constant overflow");
                  }
                  newCode[outPos +1] = (byte) index ;
                  break;
              }

         /****
	  *   // Remapping branches
	  *   case opc_ifeq:
          *   case opc_ifne:
          *   case opc_iflt:
          *   case opc_ifge:
          *   case opc_ifgt:
          *   case opc_ifle:
          *   case opc_if_icmpeq:
          *   case opc_if_icmpne:
          *   case opc_if_icmplt:
          *   case opc_if_icmpge:
          *   case opc_if_icmpgt:
          *   case opc_if_icmple:
          *   case opc_if_acmpeq:
          *   case opc_if_acmpne:
          *   case opc_ifnull:
          *   case opc_ifnonnull:
          *   case opc_goto:
          *   case opc_jsr: {
          *       // Calculate the displacement, sign extend high byte
          *       int displ = (code[pc+1] << 8) | (code[pc+2] & 0xFF);
          *
          *      *  if (displ == 3 && opcode == opc_goto) {
	  *      *     break;
	  *      * }
          *

          *       int branchDest = pc + displ;
          *
	  *       if ((code[branchDest] & 0xFF) == opc_goto) {
          *           // We're branching to a goto.  We can just branch to
	  *           // where the goto was going.
	  *           branchDest +=
	  *     	(code[branchDest+1] << 8) | (code[branchDest+2] & 0xFF);
	  *       }
          *       int newDest = newOffsets[indexByPC[branchDest]] - outPos;
          *       newCode[outPos+1] = (byte) ((newDest >> 8) & 0xFF);
          *       newCode[outPos+2] = (byte) (newDest & 0xFF);
          *       break;
          *   }
          *
	  *   case opc_tableswitch: {
	  *       newCode[outPos] = code[pc];
	  *       outPos = (outPos + 4) & ~3;
	  *       inPos = (inPos + 4) & ~3;
          *
	  *       // Update the default destination
	  *       int oldDest = getInt(inPos) + pc;
	  *       int newDest = newOffsets[indexByPC[oldDest]]
	  *     		- newOffsets[pcindex];
	  *       putInt(newCode, outPos, newDest);
          *
	  *       // Update each of the destinations in the table
	  *       int low = getInt(inPos+4);
	  *       int high = getInt(inPos+8);
	  *       putInt(newCode, outPos+4, low);
	  *       putInt(newCode, outPos+8, high);
	  *       for (int j = 0; j <= high-low; j++) {
          *           int offset = j * 4 + 12;
          *           oldDest = getInt(inPos + offset) + pc;
          *           newDest = newOffsets[indexByPC[oldDest]]
	  *     		- newOffsets[pcindex];
	  *           putInt(newCode, outPos + offset, newDest);
          *       }
	  *       break;
          *   }
          *
	  *   case opc_lookupswitch: {
	  *       newCode[outPos] = code[pc];
	  *       // 0-3 byte pads
	  *       outPos = (outPos + 4) & ~3;
	  *       inPos = (inPos + 4) & ~3;
          *
	  *       // Update the default destination
          *       int oldDest = getInt(inPos) + pc;
          *       int newDest = newOffsets[indexByPC[oldDest]]
	  *     	        - newOffsets[pcindex];
          *       putInt(newCode, outPos, newDest);
          *
          *       // Update each of the pairs of destinations in the list
          *       int pairs = getInt(inPos+4);
          *       putInt(newCode, outPos+4, pairs);
          *       for (int j = 0; j < pairs; j++) {
          *           int offset = (j + 1) * 8;
          *
          *           // First copy the value
          *           putInt(newCode, outPos + offset,
          *                  getInt(inPos + offset));
          *           offset += 4;
          *
          *           // Now adjust the destination
          *           oldDest = getInt(inPos + offset) + pc;
          *           newDest = newOffsets[indexByPC[oldDest]]
	  *     		- newOffsets[pcindex];
          *           putInt(newCode, outPos + offset, newDest);
          *       }
	  *       break;
	  *   }
          ********/

	      // Byte-codes with constant pool access. Remap to new indices
	      case opc_getfield:
	      case opc_checkcast:
	      case opc_getstatic:
	      case opc_instanceof:
	      case opc_ldc2_w:
	      case opc_new:
	      case opc_putfield:
	      case opc_putstatic:
	      case opc_invokevirtual:
	      case opc_invokestatic:
	      case opc_invokespecial:
              case opc_getstatic_fast:
              case opc_getstaticp_fast:
              case opc_getstatic2_fast:
              case opc_putstatic_fast:
              case opc_putstatic2_fast:
              case opc_invokevirtual_fast:
              case opc_invokespecial_fast:
              case opc_invokestatic_fast:
              case opc_anewarray_fast:
              case opc_checkcast_fast:
              case opc_instanceof_fast:
	      case opc_multianewarray:
	      case opc_multianewarray_fast:
	      case opc_invokeinterface:
 	      case opc_invokeinterface_fast:
              case opc_ldc_w: {
		  int oldindex = (int)(((code[pc+1]&0xFF) << 8)
                                   | (code[pc+2]&0xFF));
                  int index = co[oldindex].index;
		  newCode[outPos] = (byte) opcode;
		  newCode[outPos+1] = (byte) ((index >> 8) & 0xFF);
                  newCode[outPos+2] = (byte) (index & 0xFF);
                  break;
	      }
            }
        }

        // Update the exception table
        for (int i = 0; i < exceptionTable.length; i++) {
            ExceptionEntry e = exceptionTable[i];
            e.startPC = newOffsets[indexByPC[e.startPC]];
            e.endPC = newOffsets[indexByPC[e.endPC]];
            e.handlerPC = newOffsets[indexByPC[e.handlerPC]];
        }

        // Update the line number table
	LineNumberTableEntry[] lntab = getLineNumberTable();
        if (lntab != null) {
            for (int i = 0; i < lntab.length; i++) {
                LineNumberTableEntry e = lntab[i];
                e.startPC = newOffsets[indexByPC[e.startPC]];
            }
        }

        // Update the line number table
	LocalVariableTableEntry[] locvartab = getLocalVariableTable();
        if (locvartab != null) {
            for (int i = 0; i < locvartab.length; i++) {
                LocalVariableTableEntry e = locvartab[i];
                e.pc0 = newOffsets[indexByPC[e.pc0]];
            }
        }

	// Update the stack maps
	if (stackMapTable != null) {
	    for (int i = 0; i < stackMapTable.length; i++) {
		StackMapFrame frame = stackMapTable[i];
		frame.offset = newOffsets[indexByPC[frame.offset]];
	    }
	}
        // make the changes permanent
	code = newCode;
    }
}
