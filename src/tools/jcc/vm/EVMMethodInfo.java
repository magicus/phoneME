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
 * a method. Target-machine independent.
 *
 * See also EVMClass for VM-specific info associated with each class.
 * See also EVMVM for VM-specific info not associated directly with any class.
 */
import components.*;
import vm.VMMethodInfo;
import jcc.Const;
import jcc.EVMConst;
import util.DataFormatException;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.util.StringTokenizer;

public class
EVMMethodInfo extends VMMethodInfo implements Const, EVMConst {
    private boolean		impureCode = false;
    private boolean		mustAlign  = false;
    private boolean		codeExamined = false;
    public  MethodInfo method;

    private String		myNativeName;

    private int inlining;
    static final int NO_INLINE_FLAG = (0x1 << 24);
    static final int SAME_CLASS_FLAG = (0x1 << 25);
    static final int REDO_INLINING_FLAG = (0x1 << 26);

    /** Keep this var to make code look the same as in inline.c */
    static final boolean UseLosslessQuickOpcodes = false;

    /**
     * Flag used in quickenCode() to save old methodblock info so we can
     * use it here for inlining.  This affects invokevirtual_quick which
     * typically overwrites the methodblock info with <methodtable offset>
     * and <nargs>.
     */
    public static final boolean SAVE_TARGET_METHODS = true;

    public EVMMethodInfo( MethodInfo m  ){
	method = m;
	method.vmMethodInfo = this;
    }

    private void examineCode( ) throws DataFormatException {
	impureCode = false;
	mustAlign  = false;
	if ( method.code == null ){
	    codeExamined = true;
	    return;
	}
	byte [] code = method.code;
	int	ncode  = code.length;
	int	opcode;
	for( int i = 0; i < ncode; i += method.opcodeLength(i)) {
	    switch (opcode = (int)code[i]&0xff) {

	    case opc_tableswitch:
	    case opc_lookupswitch:
		mustAlign = true;
		break;

	    case opc_ldc:
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
		impureCode = true; // all get quicked.
		break;
	    }
	}
	codeExamined = true;
    }

    public boolean isCodePure() throws DataFormatException {
	if ( ! codeExamined ){
	    examineCode(); // may throw exception without setting codeExamined
	}
	return !impureCode;
    }

    public int alignment() throws DataFormatException {
	if ( ! codeExamined ){
	    examineCode(); // may throw exception without setting codeExamined
	}
	return mustAlign ? 4 : 1;
    }

    private static int methodNumber = 0;

    public String getNativeName() {
	if ( myNativeName == null ){
	    // need to take parameter list into account.
	    // OR just enumerate, as the actual name doesn't matter.
	    // IMPL_NOTE: consider whether this should be fixed. 
	    myNativeName = ((EVMClass)(method.parent.vmClass)).getNativeName()+ methodNumber;
	    methodNumber+= 1;
	}
	return myNativeName;
    }

    public boolean
    hasBody(){
	return ( (method.access & (ACC_ABSTRACT|ACC_NATIVE) )== 0 );
    }

    public int
    EVMflags(){
	int flagval = 0;
	int a = method.access;
	if ( (a&ACC_PUBLIC) != 0 ) flagval |= EVM_METHOD_ACC_PUBLIC;
	if ( (a&ACC_PRIVATE) != 0 ) flagval |= EVM_METHOD_ACC_PRIVATE;
	if ( (a&ACC_PROTECTED) != 0 ) flagval |= EVM_METHOD_ACC_PROTECTED;
	if ( (a&ACC_STATIC) != 0 ) flagval |= EVM_METHOD_ACC_STATIC;
	if ( (a&ACC_FINAL) != 0 ) flagval |= EVM_METHOD_ACC_FINAL;
	if ( (a&ACC_SYNCHRONIZED) != 0 ) flagval |= EVM_METHOD_ACC_SYNCHRONIZED;
	if ( (a&ACC_NATIVE) != 0 ) flagval |= EVM_METHOD_ACC_NATIVE;
	if ( (a&ACC_ABSTRACT) != 0 ) flagval |= EVM_METHOD_ACC_ABSTRACT;
	return flagval;

    }

    public int methodOffset( ){
	int off = method.methodTableIndex;
	if ( off < 0 ){
	    /*
	     * off < 0 means that we do not have a methodtable or
	     * imethodtable entry for this method. This is ok if it is:
	     * - private, or
	     * - static, or
	     * - <init>
	     * Otherwise, this is an error.
	     */
	    if ( method.isStaticMember( ) 
		|| method.isPrivateMember( ) 
		|| method.name.string.equals("<init>") ) {
		    return 0;
	    } else {
		throw new Error("Bad method offset for "+method.qualifiedName() );
	    }
	}
	return off;
    }

    /** Attempt to inline the code of this method */
    final static int inline_NOT_DONE = 0;
    final static int inline_IN_PROGRESS = 1;
    final static int inline_DONE = 2;

    private int inlineState = inline_NOT_DONE;

    public void inlineCode() {
        new Error().printStackTrace();

        boolean isRewritten = false;
	if (inlineState == inline_NOT_DONE) { 
	    inlineState = inline_IN_PROGRESS;
	} else { 
	    return;
	}
	ConstantObject[] cp = method.parent.constants;
	byte[] code = method.code;
	byte[] rewrite;
	int tmi = 0;			// target method index

	for (int pc = 0; pc < code.length; ) {
	    int opcode = code[pc] & 0xff;

	    switch (opcode) {
                
	    case opc_invokevirtual_fast:
	    case opc_invokespecial_fast: 
	    case opc_invokestatic_fast: {
		int index = method.getUnsignedShort(pc + 1);
		MethodConstant mc = (MethodConstant) cp[index];
		VMMethodInfo targetMethod = mc.find().vmMethodInfo;
		rewrite = MethodCallInline(pc, (EVMMethodInfo)targetMethod);
		if (rewrite != null) {
		    isRewritten = true;
		    System.arraycopy(rewrite, 0, code, pc, 3);
		}
		pc += 3;
		break;
	    }

	    default:
	        pc += method.opcodeLength(pc);
		break;
	    }
	}
	if (isRewritten) 
	    compress();
	inlineState = inline_DONE;
    }

    /* This method is called to determine whether the method "mb" called at
     * from instruction "pc" can be inlined.  If not, the value null is
     * returned.  If so, an array of three bytes, which should overwrite the
     * method invocation, is returned
     */
    private byte[] MethodCallInline(int pc, EVMMethodInfo mb) {
	byte code[] = method.code;
	int opcode = code[pc] & 0xff;

	if (opcode == opc_invokevirtual_fast) {   
	    /* This is a virtual method call.  No use even bother trying to
	     * inline the method, unless its final
	     */
	    if (((mb.method.access & ACC_FINAL) == 0) 
		   && ((mb.method.parent.access & ACC_FINAL) == 0))
		return null;
        }
            
	int mbInlining = mb.getInlining();
	if ((mbInlining & NO_INLINE_FLAG) != 0)
	    return null;
	/* Does this inlining have a dependency on the constant pool, and so
	 * can only be used on a method in the same class. */
	if ((mbInlining & SAME_CLASS_FLAG) != 0) {
	    if (this.method.parent != mb.method.parent)
		return null;
	}
	/* There is a inlining.  Copy that value into "result" */
	byte[] result = new byte[3];
	result[0] = INLINING_WORD1(mbInlining);
	result[1] = INLINING_WORD2(mbInlining);
	result[2] = INLINING_WORD3(mbInlining);
	return result;
    }

    public int getInlining() { 
	MethodInfo mi = this.method;
	if (inlining == 0) { 
	    if (   ((mi.access & (ACC_ABSTRACT | ACC_NATIVE |
				  ACC_SYNCHRONIZED)) != 0)
		|| (mi.exceptionTable.length > 0)) {
		inlining = NO_INLINE_FLAG;
	    } else { 
		inlineCode();
		inlining = calculateInlining();
		
		/*******
		if (inlining != NO_INLINE_FLAG) { 
		    String sameClass = 
			((inlining & SAME_CLASS_FLAG) != 0) ? "*" : "";
		    System.out.print("get: " + this + " =>" + sameClass);
		    System.out.println(" " + disassembleInlining());
		}
		********/
	    }
	}
	return inlining;
    }

    /* Given a method, determine if it can be "inlined" into three or fewer
     * bytes.
     */
    private int calculateInlining() { 
        MethodInfo mb = this.method;
	byte[] code = mb.code;

	/* The checkThis flag indicates that the resulting code must 
	 * throw a NullPointerException if the first argument is null
	 */
	boolean checkThis = ((mb.access & ACC_STATIC) == 0) 
	                 && !method.name.string.equals("<init>");
	    
	boolean redoInlining = false;
	int stackSize, stackBase;
	OpcodeInfoType opPtr;

	stackSize = 0;
	stackBase = 0;			// Prevent javac warning
	for (int pc = 0; ; pc++) { 
	    /* At this point in our simulation of the execution of the
	     * method, registers stackBase .. stackBase+stackSize - 1 are
	     * pushed onto the the stack.  pc points to the next
	     * instruction to look at.
	     */
	    int opcode = code[pc] & 0xff;
	    int opcode2;
	    int reg, regSize, nextReg;

	    if (stackSize == 0)
		stackBase = 0;
	    nextReg = stackBase + stackSize;
	    opPtr = OpcodeInfo[opcode];

	    switch (opPtr.opcode) { 

	    case opc_iload_0: /* register load.  regnum from opcode */
	    case opc_iload:	/* register load.  regnum from pc[1] */
	        if (opPtr.opcode == opc_iload_0) { 
		    reg = REGNUM(opPtr);
		} else { 
		    reg = code[pc + 1] & 0xff; pc++;
		}
		regSize = REGSIZE(opPtr);
		if (stackSize == 0) /* stack is currently empty */
		    stackBase = reg;
		else if (nextReg != reg)
		    return NO_INLINE_FLAG;
		stackSize += regSize;
		continue;

	    case opc_pop:	/* pop stack, or nop */
		stackSize -= REGSIZE(opPtr);
		continue;

	    case opc_nonnull_quick: /* special instruction */
		if (nextReg == 1) {
		    /* We're checking register 0 to ensure that it isn't null */
		    stackSize = 0; checkThis = true;
		    continue;
		}
		return NO_INLINE_FLAG;

	    case opc_invokeignored_quick: /* special instruction */
	    {		
		int popCount = code[pc + 1] & 0xff;
		if (code[pc + 2] != 0) {  
		    /* We only know how to check register 0 for non-null ness */
		    if (nextReg != popCount)
			return NO_INLINE_FLAG;
		    checkThis = true;
		    stackSize -= popCount;
		} else { 
		    stackSize -= popCount;		  
		}
		pc += 2;
		continue;
	    }

	    case opc_return:	/* return void or value */
		return makeReturnResult(checkThis, nextReg, REGSIZE(opPtr));

	    case opc_iadd: {	/* any simple instruction */
		int ilength = opcLengths[opcode];
		int result;
		opcode2 = code[pc + ilength] & 0xff;

		if (!((opPtr.outStack > 0)
			 ? isXreturn(opcode2)
			 : (opcode2 == opc_return
			    || opcode == opc_athrow)))
		    return NO_INLINE_FLAG;

		if ((opPtr.flags & OpcodeInfoType.NULL_CHECK) != 0
		    && (stackBase == 0)) {
		    /* We don't need to generate code to check for null, since
		     * the instruction already does it.
		     */
		    checkThis = false;
		}
		switch (ilength) {
		case 1:
		    result =
			makeOpcodeResult(checkThis, nextReg, opPtr.inStack,
					 1, opcode, 0, 0);
		    break;
		case 2:
		    result = 
			makeOpcodeResult(checkThis, nextReg, opPtr.inStack,
					 2, opcode, code[pc+1] & 0xff, 0);
		    break;
		case 3:
		    result = 
			makeOpcodeResult(checkThis, nextReg, opPtr.inStack,
					 3, opcode,
					 code[pc+1] & 0xff, code[pc+2] & 0xff);
		    break;
		default:
		    throw new RuntimeException("sysAssert(FALSE);");
		    // result = NO_INLINE_FLAG; // not reached
		    // break;			// not reached
		}
		if ((result & NO_INLINE_FLAG) == 0) { 
		    if ((opPtr.flags & OpcodeInfoType.CONSTANT_POOL) != 0) 
			result |= SAME_CLASS_FLAG;
		    if (redoInlining) 
			result |= REDO_INLINING_FLAG;
		}
		return result;
	    }

	    default:
		throw new RuntimeException("sysAssert(FALSE);");
	    case 255:		/* random instruction */
		return NO_INLINE_FLAG;

	    } /* of switch statement */
	} /* end of for loop */
    }

    /* This method is called to create the code that is actually going to
     * replace the indicated method, when the method does nothing, or when
     * it simply returns one of its arguments.
     *
     * It takes the following arguments:
     *   mb:          Method we are examining
     *   checkThis:   If true,  We must specially check that the first argument
     *                "this" isn't null. 
     *   highReg,     One greater than the highest register on the stack when
     *                the return or Xreturn is called.
     *   returnSize   Size of the return (0 for return, 1 for ireturn, 
     *                 2 for lreturn, etc);
     *
     * We have to emulate the method call in 3 bytes.  At the time the
     * method is called, the arguments reg[0] . . . reg[mb->args_size - 1]
     * are pushed on the stack.
     */
    static int[] poppers = { opc_nop, opc_pop, opc_pop2 };
    private int makeReturnResult(boolean checkThis, int highReg, int returnSize) { 
        MethodInfo mb = method;
	int argsSize = mb.argsSize;
	if (returnSize == 0) { 
	    /* Return void */
	    return MAKE_INLINING(opc_invokeignored_quick, argsSize,
				 (checkThis ? 1 : 0));
	} else {
	    /* Return some value from the stack */
	    int returnReg = highReg - returnSize;  
	    int excessArgs =  argsSize - returnSize - returnReg;
	    // sysAssert(returnReg >= 0 && returnSize >= 0);
	    if (returnReg == 0) { 
		/* Returning reg0 or reg0/reg1 */
		if (checkThis) { 
		    /* Must be returning reg0, which is also checked. We
		     * require argsSize >= 2 (which is the same thing as
		     * excessArgs >= 1), because otherwise the "dup" might
		     * overflow the stack.  More sophisticated inliners
		     * might see if there is space on the caller's stack.
		     */
		    // sysAssert(returnSize == 1);
		    if (argsSize < 2) { 
			return NO_INLINE_FLAG;
		    } else if (excessArgs > 2) { 
		        return NO_INLINE_FLAG;
		    } else { 
		        return MAKE_INLINING(poppers[excessArgs],
					     opc_dup,
					     opc_nonnull_quick);
		    }
		} else {
		    /* We're returning reg0 or reg0/reg1 which isn't null
		     * checked.  We just pop extraneous stuff off the stack
		     * */
		    return MAKE_INLINING(opc_invokeignored_quick,
					 excessArgs, 0);
		}
	    } else {
		/* At this point, returnReg > 0.  We're returning something
		 * other than the bottom of the stack.
		 */
	        if (returnSize == 1 && returnReg == 1) { 
		    if (excessArgs > 2) { 
			return NO_INLINE_FLAG;
		    }
		    return MAKE_INLINING(poppers[excessArgs], 
					 opc_swap, 
					 checkThis ? opc_nonnull_quick
					    : opc_pop);
		}
		return NO_INLINE_FLAG;
	    }
	}
    }

    /* This method is called to create the code that is actually going to
     * replace the indicated method
     * 
     * makeOpcodeResult is used to create a inlining that can be used anywhere
     * It takes the following arguments:
     *
     *   mb:          Method we are examining
     *   checkThis:   If true,  We must specially check that the first argument
     *                "this" isn't null.  This condition is >>NOT<< tested by 
     *                the generated code.
     *   nextReg:     In the emulation, the highest register on the stack is
     *                reg[nextReg - 1].
     *   icount       The number of bytes of instructions that follow.
     *   opcode, op1, op2
     *                The bytes of instruction.
     *
     * We have to emulate the method call in 3 bytes.  At the time the
     * method is called, the arguments reg[0] . . . reg[mb->args_size - 1]
     * are pushed on the stack.  So in three bytes, we have to:
     *     Remove any excess arguments from the stack.
     *     Perform the operation on the indicated stack registers
     *     Remove any objects lower down on the stack (this is hard!)
     *     Make sure that reg[0] is checked for being non-null, if necessary.
     */ 
     private int makeOpcodeResult(boolean checkThis, 
				  int nextReg, int opcodeArgCount,
				  int icount, int opcode, int op1, int op2) {
        MethodInfo mb = method;
	int firstReg = (opcodeArgCount == 0) ? 0 : nextReg - opcodeArgCount;
	// sysAssert(firstReg >= 0 && opcodeArgCount >= 0 && icount > 0);

	if (firstReg > 0) {
	    /* There are extra registers at the bottom of the stack */
	    return makePoppingResult(checkThis, firstReg, opcodeArgCount, 
				     icount, opcode, op1, op2);
	} else {
	    /* No extra registers at bottom of stack */
	    int argsSize = mb.argsSize;
	    int excessArgs = argsSize - opcodeArgCount; /* extra at top */
	    int popSpace = 3 - icount; /* space to pop args at top */

	    int result = 0;
	    int i;

	    if (checkThis) { 
		/* Unless this is a constant method that ignores all of its
		 * arguments, we don't really have any way of checking
		 * register 0 if the instructions doesn't.  If it is a
		 * constant instruction, deduct one from both popSpace and
		 * from excessArgs, since we are popping that last argument
		 * when an opc_nonnull_quick;
		 */
		if (opcodeArgCount > 0 || popSpace == 0)
		    return NO_INLINE_FLAG;
		popSpace--; excessArgs--; 
		// sysAssert(excessArgs >= 0);
	    }
	    if (excessArgs > 2 * popSpace) 
		return NO_INLINE_FLAG;
	    for (i = 0; i < popSpace; i++) { 
		/* If excessArgs <= popSpace, the following generates excessArgs
		 * "pops" followed by nops.  Otherwise, it generates 
		 * excessArgs - popSpace pop2's followed by pop's.
		 */
		int opcodeTmp = (excessArgs <= i) ? opc_nop 
		    : (excessArgs <= popSpace + i) ? opc_pop 
			: opc_pop2;
		result |= (opcodeTmp << (i << 3));
	    }
	    if (checkThis) 
		result |= opc_nonnull_quick << ((i++) << 3);
	    // sysAssert(i + icount == 3);
	    switch (icount) { 
	       case 3: result |= op2 << ((i + 2) << 3);
	       case 2: result |= op1 << ((i + 1) << 3);
	       case 1: result |= opcode << ((i + 0) << 3);
	    }
	    return result;
	}
    }

    /* 
     * Called by makeOpcodeResult.  
     * Same arguments.  But there are extra arguments on the bottom to pop.
     */
    private int makePoppingResult(boolean checkThis, 
				  int firstReg, int opcodeArgCount,
				  int icount, int opcode, int op1, int op2) {
        MethodInfo mb = method;
	int argsSize = mb.argsSize;
	int excessArgs = argsSize - opcodeArgCount - firstReg; /* extra on top*/

	if (icount > 1)
	    /* We're just not prepared to deal with this. */
	    return NO_INLINE_FLAG;

	if (OpcodeInfo[opcode].outStack == 0) {
	    int result = 0;
	    /* Something like an array store, that leaves no value on the
               stack */
	    int i = 0;
	    /* We can't deal with checkThis, since it might reverse the order of
	     * an exception.  We have a total of two instructions to do all the 
	     * pre and post popping.
	     */
	    if (checkThis || ((excessArgs + 1)/2 + (firstReg + 1)/2) > 2)
		return NO_INLINE_FLAG;
	    for (; excessArgs > 0; excessArgs -=2) /* pre popping */
		result |= (excessArgs == 1 ? opc_pop : opc_pop2)
		    << ((i++) << 3);
	    result |= opcode << ((i++) << 3);
	    for (; firstReg > 0; firstReg -=2) /* post popping */
		result |= (firstReg == 1 ? opc_pop : opc_pop2)
		    << ((i++) << 3);
	    while (i < 3)
		result |= opc_nop << ((i++) << 3);
	    return result;
	}

	if (excessArgs > 0 || firstReg > 1)
	    /* We can't both do useful work and remove more than this many 
	     * items from the stack. */
	    return NO_INLINE_FLAG;

	if (opcodeArgCount == 1) { 
	    return MAKE_INLINING(opc_swap, 
				 checkThis ? opc_nonnull_quick
				    : opc_pop, 
				 opcode);
	}
	if (   ((OpcodeInfo[opcode].flags &
		 (OpcodeInfoType.NULL_CHECK | OpcodeInfoType.CAN_ERROR)) == 0)
	    && (OpcodeInfo[opcode].outStack == 1)) {
	    /* The result creates one thing on the stack, and it can't error */
	    return MAKE_INLINING(opcode,
				 opc_swap,
				 checkThis ? opc_nonnull_quick
				    : opc_pop);
	}
	return NO_INLINE_FLAG;
    }

    private static boolean isXreturn(int opcode) {
	return (opcode >= opc_ireturn) && (opcode <= opc_areturn);
    }

    private static byte INLINING_WORD1(int simp) {
	return (byte) (simp & 0xFF);
    }

    private static byte INLINING_WORD2(int simp) {
	return (byte) ((simp >> 8) & 0xFF);
    }

    private static byte INLINING_WORD3(int simp) {
	return (byte) ((simp >> 16) & 0xFF);
    }

    private static int MAKE_INLINING(int op1, int op2, int op3) {
	return (op1 << 0) + (op2 << 8) + (op3 << 16);
    }

    private static int REGSIZE(OpcodeInfoType ptr) {
	return (ptr.inStack);
    }

    private static int REGNUM(OpcodeInfoType ptr) {
	return (ptr.outStack);
    }

    static OpcodeInfoType[] OpcodeInfo = {
	/*  { opc_pop  <number of words to pop from stack> } 
	 *  { opc_iadd <words popped from stack> <words pushed to stack> }
	 *  { opc_iload <words pushed to stack> } 
	 *  { opc_iload_0 <words pushed to stack> <implicit register> } 
	 *  { opc_return  <words returned> } 
	 *    255 indicates opcode that we can't inline
	 *  other values are special opcodes that must be handled specially
	 */

	/* nop               */ new OpcodeInfoType(opc_pop, 0),
	/* aconst_null       */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_m1         */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_0          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_1          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_2          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_3          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_4          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* iconst_5          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* lconst_0          */ new OpcodeInfoType(opc_iadd, 0, 2),
	/* lconst_1          */ new OpcodeInfoType(opc_iadd, 0, 2),
	/* fconst_0          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* fconst_1          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* fconst_2          */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* dconst_0          */ new OpcodeInfoType(opc_iadd, 0, 2),
	/* dconst_1          */ new OpcodeInfoType(opc_iadd, 0, 2),
	/* bipush            */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* sipush            */ new OpcodeInfoType(opc_iadd, 0, 1),
	/* ldc               */ new OpcodeInfoType(255),
	/* ldc_w             */ new OpcodeInfoType(255),
	/* ldc2_w            */ new OpcodeInfoType(255),
	/* iload             */ new OpcodeInfoType(opc_iload, 1),
	/* lload             */ new OpcodeInfoType(opc_iload, 2),
	/* fload             */ new OpcodeInfoType(opc_iload, 1),
	/* dload             */ new OpcodeInfoType(opc_iload, 2),
	/* aload             */ new OpcodeInfoType(opc_iload, 1),
	/* iload_0           */ new OpcodeInfoType(opc_iload_0, 1, 0),
	/* iload_1           */ new OpcodeInfoType(opc_iload_0, 1, 1),
	/* iload_2           */ new OpcodeInfoType(opc_iload_0, 1, 2),
	/* iload_3           */ new OpcodeInfoType(opc_iload_0, 1, 3),
	/* lload_0           */ new OpcodeInfoType(opc_iload_0, 2, 0),
	/* lload_1           */ new OpcodeInfoType(opc_iload_0, 2, 1),
	/* lload_2           */ new OpcodeInfoType(opc_iload_0, 2, 2),
	/* lload_3           */ new OpcodeInfoType(opc_iload_0, 2, 3),
	/* fload_0           */ new OpcodeInfoType(opc_iload_0, 1, 0),
	/* fload_1           */ new OpcodeInfoType(opc_iload_0, 1, 1),
	/* fload_2           */ new OpcodeInfoType(opc_iload_0, 1, 2),
	/* fload_3           */ new OpcodeInfoType(opc_iload_0, 1, 3),
	/* dload_0           */ new OpcodeInfoType(opc_iload_0, 2, 0),
	/* dload_1           */ new OpcodeInfoType(opc_iload_0, 2, 1),
	/* dload_2           */ new OpcodeInfoType(opc_iload_0, 2, 2),
	/* dload_3           */ new OpcodeInfoType(opc_iload_0, 2, 3),
	/* aload_0           */ new OpcodeInfoType(opc_iload_0, 1, 0),
	/* aload_1           */ new OpcodeInfoType(opc_iload_0, 1, 1),
	/* aload_2           */ new OpcodeInfoType(opc_iload_0, 1, 2),
	/* aload_3           */ new OpcodeInfoType(opc_iload_0, 1, 3),
	/* iaload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* laload            */ new OpcodeInfoType(opc_iadd, 2, 2,
						   OpcodeInfoType.NULL_CHECK),
	/* faload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* daload            */ new OpcodeInfoType(opc_iadd, 2, 2,
						   OpcodeInfoType.NULL_CHECK),
	/* aaload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* baload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* caload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* saload            */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* istore            */ new OpcodeInfoType(255),
	/* lstore            */ new OpcodeInfoType(255),
	/* fstore            */ new OpcodeInfoType(255),
	/* dstore            */ new OpcodeInfoType(255),
	/* astore            */ new OpcodeInfoType(255),
	/* istore_0          */ new OpcodeInfoType(255),
	/* istore_1          */ new OpcodeInfoType(255),
	/* istore_2          */ new OpcodeInfoType(255),
	/* istore_3          */ new OpcodeInfoType(255),
	/* lstore_0          */ new OpcodeInfoType(255),
	/* lstore_1          */ new OpcodeInfoType(255),
	/* lstore_2          */ new OpcodeInfoType(255),
	/* lstore_3          */ new OpcodeInfoType(255),
	/* fstore_0          */ new OpcodeInfoType(255),
	/* fstore_1          */ new OpcodeInfoType(255),
	/* fstore_2          */ new OpcodeInfoType(255),
	/* fstore_3          */ new OpcodeInfoType(255),
	/* dstore_0          */ new OpcodeInfoType(255),
	/* dstore_1          */ new OpcodeInfoType(255),
	/* dstore_2          */ new OpcodeInfoType(255),
	/* dstore_3          */ new OpcodeInfoType(255),
	/* astore_0          */ new OpcodeInfoType(255),
	/* astore_1          */ new OpcodeInfoType(255),
	/* astore_2          */ new OpcodeInfoType(255),
	/* astore_3          */ new OpcodeInfoType(255),
	/* iastore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* lastore           */ new OpcodeInfoType(opc_iadd, 4, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* fastore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* dastore           */ new OpcodeInfoType(opc_iadd, 4, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* aastore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* bastore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* castore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* sastore           */ new OpcodeInfoType(opc_iadd, 3, 0,
						   OpcodeInfoType.NULL_CHECK),
	/* pop               */ new OpcodeInfoType(opc_pop, 1),
	/* pop2              */ new OpcodeInfoType(opc_pop, 2),
	/* dup               */ new OpcodeInfoType(255),
	/* dup_x1            */ new OpcodeInfoType(255),
	/* dup_x2            */ new OpcodeInfoType(255),
	/* dup2              */ new OpcodeInfoType(255),
	/* dup2_x1           */ new OpcodeInfoType(255),
	/* dup2_x2           */ new OpcodeInfoType(255),
	/* swap              */ new OpcodeInfoType(255),
	/* iadd              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* ladd              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* fadd              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* dadd              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* isub              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lsub              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* fsub              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* dsub              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* imul              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lmul              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* fmul              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* dmul              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* idiv              */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.CAN_ERROR),
	/* ldiv              */ new OpcodeInfoType(opc_iadd, 4, 2,
						   OpcodeInfoType.CAN_ERROR),
	/* fdiv              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* ddiv              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* irem              */ new OpcodeInfoType(opc_iadd, 2, 1,
						   OpcodeInfoType.CAN_ERROR),
	/* lrem              */ new OpcodeInfoType(opc_iadd, 4, 2,
						   OpcodeInfoType.CAN_ERROR),
	/* frem              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* drem              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* ineg              */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* lneg              */ new OpcodeInfoType(opc_iadd, 2, 2),
	/* fneg              */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* dneg              */ new OpcodeInfoType(opc_iadd, 2, 2),
	/* ishl              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lshl              */ new OpcodeInfoType(opc_iadd, 3, 2),
	/* ishr              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lshr              */ new OpcodeInfoType(opc_iadd, 3, 2),
	/* iushr             */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lushr             */ new OpcodeInfoType(opc_iadd, 3, 2),
	/* iand              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* land              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* ior               */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lor               */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* ixor              */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* lxor              */ new OpcodeInfoType(opc_iadd, 4, 2),
	/* iinc              */ new OpcodeInfoType(255),
	/* i2l               */ new OpcodeInfoType(opc_iadd, 1, 2),
	/* i2f               */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* i2d               */ new OpcodeInfoType(opc_iadd, 1, 2),
	/* l2i               */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* l2f               */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* l2d               */ new OpcodeInfoType(opc_iadd, 2, 2),
	/* f2i               */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* f2l               */ new OpcodeInfoType(opc_iadd, 1, 2),
	/* f2d               */ new OpcodeInfoType(opc_iadd, 1, 2),
	/* d2i               */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* d2l               */ new OpcodeInfoType(opc_iadd, 2, 2),
	/* d2f               */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* i2b               */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* i2c               */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* i2s               */ new OpcodeInfoType(opc_iadd, 1, 1),
	/* lcmp              */ new OpcodeInfoType(opc_iadd, 4, 1),
	/* fcmpl             */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* fcmpg             */ new OpcodeInfoType(opc_iadd, 2, 1),
	/* dcmpl             */ new OpcodeInfoType(opc_iadd, 4, 1),
	/* dcmpg             */ new OpcodeInfoType(opc_iadd, 4, 1),
	/* ifeq              */ new OpcodeInfoType(255),
	/* ifne              */ new OpcodeInfoType(255),
	/* iflt              */ new OpcodeInfoType(255),
	/* ifge              */ new OpcodeInfoType(255),
	/* ifgt              */ new OpcodeInfoType(255),
	/* ifle              */ new OpcodeInfoType(255),
	/* if_icmpeq         */ new OpcodeInfoType(255),
	/* if_icmpne         */ new OpcodeInfoType(255),
	/* if_icmplt         */ new OpcodeInfoType(255),
	/* if_icmpge         */ new OpcodeInfoType(255),
	/* if_icmpgt         */ new OpcodeInfoType(255),
	/* if_icmple         */ new OpcodeInfoType(255),
	/* if_acmpeq         */ new OpcodeInfoType(255),
	/* if_acmpne         */ new OpcodeInfoType(255),
	/* goto              */ new OpcodeInfoType(255),
	/* jsr               */ new OpcodeInfoType(255),
	/* ret               */ new OpcodeInfoType(255),
	/* tableswitch       */ new OpcodeInfoType(255),
	/* lookupswitch      */ new OpcodeInfoType(255),
	/* ireturn           */ new OpcodeInfoType(opc_return, 1),
	/* lreturn           */ new OpcodeInfoType(opc_return, 2),
	/* freturn           */ new OpcodeInfoType(opc_return, 1),
	/* dreturn           */ new OpcodeInfoType(opc_return, 2),
	/* areturn           */ new OpcodeInfoType(opc_return, 1),
	/* return            */ new OpcodeInfoType(opc_return, 0),
	/* getstatic         */ new OpcodeInfoType(255),
	/* putstatic         */ new OpcodeInfoType(255),
	/* getfield          */ new OpcodeInfoType(255),
	/* putfield          */ new OpcodeInfoType(255),
	/* invokevirtual     */ new OpcodeInfoType(255),
	/* invokespecial     */ new OpcodeInfoType(255),
	/* invokestatic      */ new OpcodeInfoType(255),
	/* invokeinterface   */ new OpcodeInfoType(255),
	/* xxxunusedxxx      */ new OpcodeInfoType(255),
	/* new               */ new OpcodeInfoType(255),
	/* newarray          */ new OpcodeInfoType(opc_iadd, 1, 1,
						   OpcodeInfoType.CAN_ERROR),
	/* anewarray         */ new OpcodeInfoType(255),
	/* arraylength       */ new OpcodeInfoType(opc_iadd, 1, 1,
						   OpcodeInfoType.NULL_CHECK),
	/* athrow            */ new OpcodeInfoType(opc_iadd, 1, 0,
						   OpcodeInfoType.NULL_CHECK
						   | OpcodeInfoType.CAN_ERROR),
	/* checkcast         */ new OpcodeInfoType(255),
	/* instanceof        */ new OpcodeInfoType(255),
	/* monitorenter      */ new OpcodeInfoType(opc_iadd, 1, 0,
						   OpcodeInfoType.NULL_CHECK
						   | OpcodeInfoType.CAN_ERROR),
	/* monitorexit       */ new OpcodeInfoType(opc_iadd, 1, 0,
						   OpcodeInfoType.NULL_CHECK
						   | OpcodeInfoType.CAN_ERROR),
	/* wide              */ new OpcodeInfoType(255),
	/* multianewarray    */ new OpcodeInfoType(255),
	/* ifnull            */ new OpcodeInfoType(255),
	/* ifnonnull         */ new OpcodeInfoType(255),
	/* goto_w            */ new OpcodeInfoType(255),
	/* jsr_w             */ new OpcodeInfoType(255),
	/* breakpoint        */ new OpcodeInfoType(255)

        /* IMPL_NOTE:  I still need to add more info here */

    };

    /**
     * Print the code as Java assembly language instructions
     */  
    String disassembleInlining() {
	byte codeBytes[] = new byte[3];
	// Copy inlining into codeBytes[] buffer
	codeBytes[0] = (byte)(inlining & 0xff);
	codeBytes[1] = (byte)((inlining >> 8) & 0xff);
	codeBytes[2] = (byte)((inlining >> 16) & 0xff);
	return MethodInfo.disassemble(codeBytes, 0, 3);
    } 

    private String myName;
    public String toString() {
	if (myName == null) {
	    myName = method.parent.className + "." + method.name.string + 
		method.type.string;
	} 
	return myName;
    }

    static int total = 0;

    // After inlining some code, we try to see if we can remove code.  For
    // example, the frequent case of opc_aload_0 invokeingored_quick #1 T can
    // simply "go away"

    final boolean compress () {
	MethodInfo mb = method;
        boolean rewritten = false;
        byte[] code = mb.code;
	int[] stack = new int[mb.stack + 1];
	int stackHeight = 0;
	int nextpc;
	java.util.BitSet targets = mb.getLabelTargets();

	for (int pc = 0; pc < code.length; pc = nextpc) {
	    nextpc = pc + mb.opcodeLength(pc);
	    int opcode = code[pc] & 0xff;
	    int popping = 0;
	    boolean checkThis = false;

	    if (targets.get(pc)) { 
	        stackHeight = 0;
	    }
	    stack[stackHeight] = pc;

	    // Invariant.  the stackheight at this point is stackHeight or less.
	    // 
	    // We can pop n items from the stack (where n <= stackHeight) by
	    // simply deleting all the code from stackHeight[n] to this point
	    // in the code.  No side effects are removed.
	    // 
	    // Note that instructions that have a side effect should set
	    // stackHeight = 0, to indicate that it can't be deleted.

	    switch (opcode) {
	    case opc_nop:
	    case opc_ineg: case opc_fneg:
	    case opc_i2f: case opc_f2i:
	    case opc_i2b: case opc_i2c: case opc_i2s:
	    case opc_newarray:
	    case opc_anewarray_fast:
	    case opc_instanceof_fast:
	    case opc_lneg: case opc_dneg:
	    case opc_l2d: case opc_d2l:
	        // these don't change stack height, and we know as much about
	        // the stack before as we do after.
	        break;

	    case opc_aconst_null: 
	    case opc_iconst_m1: case opc_iconst_0: 
	    case opc_iconst_1:  case opc_iconst_2:  
	    case opc_iconst_3:  case opc_iconst_4:
	    case opc_iconst_5:  
	    case opc_fconst_0:  case opc_fconst_1: 
	    case opc_fconst_2:
	    case opc_bipush:    case opc_sipush:
	    case opc_iload:     case opc_fload:    
	    case opc_aload:
	    case opc_iload_0:   case opc_iload_1:  
	    case opc_iload_2:   case opc_iload_3:
	    case opc_fload_0:   case opc_fload_1:  
	    case opc_fload_2:   case opc_fload_3:
	    case opc_aload_0:   case opc_aload_1:
	    case opc_aload_2:   case opc_aload_3:
	    case opc_getstatic_fast:
	    case opc_dup:	
	        // These push some value onto the stack, no matter what was
	        // there before
	        stackHeight += 1;
		break;

	    case opc_lconst_0: case opc_lconst_1:
	    case opc_dconst_0: case opc_dconst_1:
	    case opc_lload:    case opc_dload:
	    case opc_lload_0:  case opc_lload_1:
	    case opc_lload_2:  case opc_lload_3:
	    case opc_dload_0:  case opc_dload_1:
	    case opc_dload_2:  case opc_dload_3:
	    case opc_getstatic2_fast:
	        // These push two values onto the stack, no matter what was 
	        // there before.
	        stackHeight += 2;
		break;

	    case opc_i2l: case opc_i2d:
	    case opc_f2l: case opc_f2d:
	        // if we knew the top element of the stack, we know more
	        stackHeight = (stackHeight < 1) ? 0 : stackHeight + 1;
		break;

	    case opc_iadd: case opc_fadd:
	    case opc_isub: case opc_fsub:
	    case opc_imul: case opc_fmul:
	    case opc_fdiv: case opc_frem:
	    case opc_ishl: case opc_ishr: case opc_iushr:
	    case opc_iand: case opc_ior: case opc_ixor:
	    case opc_l2i: case opc_l2f:
	    case opc_d2i:  case opc_d2f:
	    case opc_fcmpl: case opc_fcmpg:
	        // if we knew the top two elements of the stack, the stack
	        // has just shrunk
	        stackHeight = (stackHeight < 2) ? 0 : stackHeight - 1;
		break;

	    case opc_lshl: case opc_lshr: case opc_lushr:
	        // if we knew the top three elements of the stack, we now
	        // know the top two
	        stackHeight = (stackHeight < 3) ? 0 : stackHeight  - 1;
		break;

	    case opc_lcmp: case opc_dcmpl: case opc_dcmpg:
	        // if we knew the top 4 elements of the stack, we now
	        // know the top element
	        stackHeight = (stackHeight < 4) ? 0 : stackHeight - 3;
		break;
	      
	    case opc_ladd: case opc_dadd:
	    case opc_lsub: case opc_dsub:
	    case opc_lmul: case opc_dmul:
	    case opc_ddiv: case opc_drem:
	    case opc_land: case opc_lor: case opc_lxor:
	        // if we knew the top 4 elements of the stack, we now
	        // know the top 2
	        stackHeight = (stackHeight < 4) ? 0 : stackHeight - 2;
		break;

	    // The dup's (other than opc_dup) deal with the stack in 
	    // a way that's not worth the hassle of dealing with.

	    case opc_getfield_fast:
	    case opc_arraylength:
	        // If we throw away the result, then we just need to check that
	        // the value is non-null.
		if (code[nextpc] == (byte)(opc_pop)) { 
		    checkThis = true;
		    nextpc += 1;
		} else { 
		    stackHeight = 0;
		}
		break;

	    case opc_pop2:
		popping++;	// fall thru
	    case opc_pop:
	        // We have to be careful.  The inliner may produce code that
	        // does correspond to the stack.  For example, it might 
	        // produce "pop pop2" to remove a double then an int.  We need
	        // to deal with series of them at once.
		if (stackHeight > 0) {
		    popping++;
		    for(;;) {
			opcode = code[++pc] & 0xFF;
			if (opcode == opc_pop) 
			    popping++;
			else if (opcode == opc_pop2)
			    popping += 2;
			else 
			    break;
		    }
		    nextpc = pc;
		}
		break;

	    case opc_invokeignored_quick: 
	        popping = code[pc + 1] & 0xff;
		if (code[pc + 2] != 0) { 
		    checkThis = true; popping--;
		} 
		break;
		
	    default:
	        stackHeight = 0;
	    }
	    
	    if (checkThis || (popping > 0 && stackHeight > 0)) {
		rewritten = true;
	        if (stackHeight >= popping) { 
		    stackHeight -= popping;
		    popping = 0;
		} else { 
		    popping -= stackHeight;
		    stackHeight = 0;
		}
		int start = stack[stackHeight];
		    
		if (checkThis) { 
		    if (popping == 0 && (nextpc - start != 3)) {
		        mb.replaceCode(start, nextpc, opc_nonnull_quick);
		    } else { 
			mb.replaceCode(start, nextpc, 
				       opc_invokeignored_quick, popping+1, 1);
		    }
		    stackHeight = 0;
		} else { 
		    switch (popping) { 
		    case 0:  
		        mb.replaceCode(start, nextpc); break;
		    case 1:  
		        mb.replaceCode(start, nextpc, opc_pop); break; 
		    case 2:  
		        mb.replaceCode(start, nextpc, opc_pop2); break; 
		    default: 
		        mb.replaceCode(start, nextpc, opc_invokeignored_quick, 
				       popping, 0); 
			break;
		    }
		}
	    }
	}
	return rewritten;
    }
}

/**
 * Class used for inlining info.  See inlining code in MethodInfo
 */
class OpcodeInfoType { 
    int opcode;				// really the opcode type
    int inStack;
    int outStack;

    static final int CAN_ERROR = 0x01;	/* can give error in addition to
					   NULL_CHECK */
    static final int NULL_CHECK = 0x02;	/* checks that first arg isn't null */
    static final int CONSTANT_POOL = 0x04; /* uses the constant pool */
    int flags;

    OpcodeInfoType(int opcode, int inStack, int outStack, int flags) {
	this.opcode = opcode;
	this.inStack = inStack;
	this.outStack = outStack;
	this.flags = flags;
    }

    OpcodeInfoType(int opcode, int inStack, int outStack) {
	this(opcode, inStack, outStack, 0);
    }

    OpcodeInfoType(int opcode, int inStack) {
	this(opcode, inStack, 0, 0);
    }

    OpcodeInfoType(int opcode) {
	this(opcode, 0, 0, 0);
    }
};
