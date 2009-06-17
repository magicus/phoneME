/*
 * @(#)CVMWriter.java	1.153	06/10/27
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
package runtime;
import consts.Const;
import consts.CVMConst;
import components.*;
import vm.*;
import jcc.Util;
import util.*;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Vector;
import java.util.Enumeration;
import java.util.Hashtable;
import java.io.OutputStream;


/*
 * The CoreImageWriter for the Embedded VM
 */

public class CVMWriter implements CoreImageWriter, Const, CVMConst {

    /* INSTANCE DATA */
    protected  Exception failureMode = null; // only interesting on falure


    protected String    	  outputFileName;
    protected CCodeWriter 	  classOut;
    protected CCodeWriter 	  auxOut;
    protected OutputStream        xxx;
    protected OutputStream        zzz;

    protected String 		  headerFileName = null;
    protected CCodeWriter 	  headerOut;
    protected OutputStream         yyy;

    protected String		  globalHeaderFileName = null;
    protected CCodeWriter	  globalHeaderOut;
    protected OutputStream	  www;
    /*
     * The following only used if we're segmenting the output,
     * which happens if this.init gets a maxSegSize > 0 
     */
    protected boolean		  segmentedOutput;
    protected int		  maxClasses;
    protected int		  curClasses = 0;
    protected int		  nClassfileOut;
    protected BufferedPrintStream listOut;

    protected CVMStringTable	  stringTable;
    protected CVMClass	  	  classes[];

    private   VMClassFactory     classMaker = new CVMClassFactory();

    private   CVMInitInfo        initInfo;
    private   CVMInitInfo        initInfoForMBs;

    boolean	   formatError = false;
    boolean	   verbose     = false;
    boolean	   classDebug  = false;
    boolean        lossless    = false;
    boolean        mutableMBs  = false;

    private static final String		staticInitThreadStoreName="StaticInitThreadStorage";
    private static final String		stringArrayName = "CVM_ROMStrings";
    private static final String		masterStringArrayName = "CVM_ROMStringsMaster";
    private static final String		staticStoreName = "CVM_staticData";
    private static final String		numStaticStoreName = "CVM_nStaticData";
    private static final String		masterStaticStoreName = "CVM_StaticDataMaster";

    private ClassnameFilterList         nativeTypes;
    private ClassnameFilter		invisibleClassList = new ClassnameFilter();

    private boolean			classLoading = true; // by default.

    // Place all byte codes into read-write memory.  Useful for setting
    // breakpoints.
    protected boolean			noPureCode = false;

    private boolean			doShared;
    private Vector			sharedConstantPools;
    private String			sharedConstantPoolName;
    private int				sharedConstantPoolSize;

    private CVMMethodType[]		methodTypes;
    private int[]			nMethodTypes;
    private int				totalMethodTypes;

    //
    // to keep track of which variants of classblock, method arrays, field arrays,
    // and method descriptors have been declared
    // see declareClassblock, declareMethodArray, and declareFieldArray
    // respectively
    //
    private java.util.BitSet		classblockSizes  = new java.util.BitSet();
    private java.util.BitSet		runtimeMethodArraySizes = new java.util.BitSet();
    private java.util.BitSet		methodArraySizes = new java.util.BitSet();
    private java.util.BitSet		compressibleMethodArraySizes = new java.util.BitSet();
    private java.util.BitSet		fieldArraySizes  = new java.util.BitSet();

    /*
     * Per-class data that is built up as we
     * print out the various data structures.
     * Rather than trying to pass around both name and size of each,
     * we collect the information here.
     */
    private String			classBlockName;
    private String			constantPoolName;
    private int				constantPoolSize;
    private String			methodArrayName;
    private int				methodTableSize;
    private String			checkedExceptionName;
    private String			interfaceTableName;
    private String			fieldTableName;
    private int				fieldTableSize;


    /* for statistics only */
    int  ncodebytes;
    int  ncatchframes;
    int  nmethods;
    int  nfields;
    int  nconstants;
    int	 njavastrings;
    int  ninnerclasses;
    int	 nWritableMethodBlocks;
    int	 nClassesWithWritableMethodBlocks;
    int  nClinits;
    int  nMethodsWithCheckinitInvokes;
    int  nTotalMethods; /* number of total romized methods */

    private CVMMethodStats methStats = new CVMMethodStats();

    public CVMWriter( ){ 
	initInfo = new CVMInitInfo();
	initInfoForMBs = new CVMInitInfo();
	
	stringTable = new CVMStringTable(initInfo);
    }

    public void init(boolean classDebug, boolean lossless,
                     ClassnameFilterList nativeTypes, boolean verbose,
                     int maxSegmentSize, boolean mutableMBs) {
	this.verbose = verbose;
	this.classDebug = classDebug;
	this.lossless = lossless;
	this.nativeTypes = nativeTypes;
	this.mutableMBs = mutableMBs;
	
	if ( maxSegmentSize > 0 ){
	    this.segmentedOutput = true;
	    this.maxClasses = maxSegmentSize;
	} else {
	    this.segmentedOutput = false;
	}
    }

    // If attribute starts with attributeName, then parse the rest of the
    // string as a list of classes.
    private boolean parseClassListAttribute(String attributeName, 
					    String attribute, 
					    ClassnameFilter filter) { 
        if (!attribute.startsWith(attributeName))
	    return false;
	String val = attribute.substring(attributeName.length() );
	java.util.StringTokenizer tkn = 
	    new java.util.StringTokenizer(val, " ,", false );
	while ( tkn.hasMoreTokens() ){
	    String classname = tkn.nextToken().replace('.', '/');
	    filter.includeName(classname);
	}
	return true;
    }

    private boolean
    isClassInvisible( String classname ) {
        return invisibleClassList.accept(null, classname);
    }

    private boolean
    isClassCNI( String classname) { 
	// compare against interned string
	return nativeTypes.isType( classname, "CNI" );
    }

    private boolean
    isClassJNI( String classname) { 
	// compare against interned string
	return nativeTypes.isType( classname, "JNI" );
    }

    public boolean setAttribute( String attribute ){
	if ( parseClassListAttribute("invisible=", attribute,
				     invisibleClassList)){
	    return true;
	} else if ( attribute.equals("noClassLoading" ) ){
	    classLoading = false;
	    return true;
	} else if ( attribute.equals("noPureCode") ) {
	    noPureCode = true;
	    return true;
	} else
	    return false; // no attribute by this name.
    }


    /*
     * If this is a simple, unsegmented output file, then
     * the file name given is the name of the C file, from which
     * we compute the .h file, crudely. We open them here.
     *
     * But if this is a segmented output job, then the name given
     * serves as a prefix, and we open a whole bunch of files and
     * set up a whole bunch of state:
     *		filename+"List"  - file containing names of all .c files
     *				   generated opened as listOut;
     *		filename+".h"    - header file, as usual
     *		filename+"Aux.c" - file containing all type, string, and other
     *				   miscellanious generated stuff.
     *				   opened as auxOut;
     *		filename+"Class"+n+".c" - files containing directly class-
     *                             related data, whole classes, and a max of
     *                             this.maxClasses of them apiece.
     */
    public boolean open( String filename ){
	if ( classOut != null ) close();
	outputFileName = filename;
	if ( ! segmentedOutput ){
	    /* old-fashioned, single output case */
	    if ( filename == null ){
		xxx = System.out;
		headerFileName = "jcc.output.h";
	    } else {
		try {
		    xxx = new java.io.FileOutputStream( filename );
		} catch ( java.io.IOException e ){
		    failureMode = e;
		    return false;
		}
		headerFileName = filename+".h";
	    }
	    try {
		classOut = auxOut = new CCodeWriter( xxx );
		yyy = new java.io.FileOutputStream( headerFileName );
		headerOut = new CCodeWriter( yyy );
		
		globalHeaderFileName = filename+"Globals.h";
		www = new java.io.FileOutputStream( globalHeaderFileName );
		globalHeaderOut = new CCodeWriter ( www );
	    } catch ( java.io.IOException e ){
		failureMode = e;
		return false;
	    }
	    writeHeaderPrologue( headerOut );
	    writeGlobalHeaderPrologue( globalHeaderOut );
	    writePrologue( classOut );
	    return true;
	} else {
	    /* segmented output.
	     * open the aux file, the first classOut file, the list file and
	     * the header file.
	     */
	    curClasses = 0;
	    nClassfileOut = 0;
	    headerFileName = outputFileName+".h";
	    try {
		yyy = new java.io.FileOutputStream( headerFileName );
		headerOut = new CCodeWriter( yyy );
		
		globalHeaderFileName = outputFileName+"Globals.h";
		www = new java.io.FileOutputStream( globalHeaderFileName );
		globalHeaderOut = new CCodeWriter ( www );
		zzz = new java.io.FileOutputStream( filename+"Aux.c");
		auxOut = new CCodeWriter( zzz );
		listOut = new BufferedPrintStream(
                              new java.io.FileOutputStream(outputFileName +
                                                           "List"));
		listOut.println( filename+"Aux.c");
		openNextClassFile();
	    } catch ( java.io.IOException e ){
		failureMode = e;
		return false;
	    }
	    writeHeaderPrologue( headerOut );
	    writeGlobalHeaderPrologue( globalHeaderOut );
	    writePrologue( auxOut );

	    /* 
	     * Moved global variables to CVMROMGlobals
	     */
	    auxOut.println("");
	    auxOut.println("/* Moved global variables to CVMROMGlobals.");
	    auxOut.println(" * For each global variable <V> that was originally accessible outside");
	    auxOut.println(" * the generated code, a corresponding constant pointer variable");
	    auxOut.println(" * is provided here that points into the global structure.");
	    auxOut.println(" * (the name <V> has only been changed to <V>ptr for those");
	    auxOut.println(" * variables where the way they are accessed is changed)");
	    auxOut.println(" * ");
	    auxOut.println(" * The size of CVMROMGlobals is provided in a global constant");
	    auxOut.println(" * so that the structure can be copied without including its definition.");
	    auxOut.println(" */");
	    auxOut.println("");
	    auxOut.println("struct CVMROMGlobalState CVMROMGlobals;");
	    auxOut.println("const int CVMROMGlobalsSize = sizeof(CVMROMGlobals);");
	    auxOut.println("");
	    auxOut.println("CVMTypeIDNamePart * const CVMMemberNameHash = CVMROMGlobals.CVMMemberNameHash;");
	    auxOut.println("CVMTypeIDTypePart * const CVMMethodTypeHash = CVMROMGlobals.CVMMethodTypeHash;");
	    auxOut.println("struct sigForm ** const CVMformTablePtr = &CVMROMGlobals.CVMformTable;");
            auxOut.println("struct pkg * const CVM_ROMpackages = CVMROMGlobals.CVM_ROMpackages;");
	    auxOut.println("struct pkg ** const CVM_pkgHashtable = CVMROMGlobals.CVM_pkgHashtable;");
            auxOut.println("struct java_lang_Class * const CVM_ROMClassBlocks = CVMROMGlobals.CVM_ROMClassBlocks;");
	    auxOut.println("struct java_lang_Class * const CVM_ROMClasses = CVMROMGlobals.CVM_ROMClasses;");
	    auxOut.println("CVMAddr * const CVM_staticData = CVMROMGlobals.CVM_staticData;");
	    auxOut.println("struct java_lang_String * const CVM_ROMStrings = CVMROMGlobals.CVM_ROMStrings;");
            auxOut.println("#if defined(CVM_JIT) && defined(CVM_MTASK)");
            auxOut.println("CVMUint16 * const CVMROMMethodInvokeCosts = CVMROMGlobals.CVMROMMethodInvokeCosts;");
            auxOut.println("#endif");
	    auxOut.println("");

	    return true;
	}
    }

    private void
    openNextClassFile() throws java.io.IOException{
	if ( classOut != null ){
	    classOut.close();
	}
	String newName = outputFileName+(nClassfileOut++)+".c";
	xxx = new java.io.FileOutputStream( newName );
	classOut = new CCodeWriter( xxx );
	listOut.println( newName );
	writePrologue( classOut );
    }

    public void close(){
	if ( classOut != null ){
	    classOut.close();
	}
	if ( auxOut  != null && auxOut != classOut ){
	    auxOut.close();
	}
	auxOut = null;
	classOut = null;
	if ( listOut != null ){
	    listOut.close();
	    listOut = null;
	}
	if ( headerOut != null ){
	    /* 
	     * Added header file for globals
	     */
	    writeHeaderEpilogue( headerOut );
	    headerOut.close();
	    headerOut = null;
	}
	/* 
	 * Added header file for globals
	 */
	if ( globalHeaderOut != null ){
	    writeGlobalHeaderEpilogue( globalHeaderOut );
	    globalHeaderOut.close();
	    globalHeaderOut = null;
	}
	outputFileName = null;
	return;
    }

    public void printError( java.io.PrintStream o ){
	if ( failureMode != null ){
	    failureMode.printStackTrace( o );
	} 
    }


    protected void declare( String declaration,  BufferedPrintStream curOut ){
	headerOut.print( "extern " );
	headerOut.print( declaration );
	headerOut.write(';');
	headerOut.write('\n');
	curOut.print( declaration );
    }

    public static String getUTF( UnicodeConstant u ){
	return u.toUTF();
    }

    /*
     * For Java methods,
     * write out the structure which will be initialized with a
     * JavaMethodDescriptor and its associated structures: code array,
     * line numbers, exceptions, local variables
     */
    private void writeCode(CVMMethodInfo meth) {
	MethodInfo 		  mi 	  = meth.method;
	LineNumberTableEntry[]	  lntab	  = mi.getLineNumberTable();
	LocalVariableTableEntry[] locvartab = mi.getLocalVariableTable();

	int	codeLengthRoundup = (mi.code.length+3)&~3;
	int 	nTryCatch 	  = (mi.exceptionTable==null) ? 0 
	       				  : mi.exceptionTable.length;
	int	nLineNumbers = (classDebug && lntab != null) ? lntab.length : 0;
	int	nLocals = (classDebug && locvartab != null) ? locvartab.length : 0;

	String	codeBlockName	  = meth.getNativeName();
	String	descriptorTypeTag = codeBlockName+"MDType";

	CCodeWriter declareOut;

	boolean impureCode = noPureCode || !meth.isCodePure();

	classOut.println("\n/* Code block for "+meth.method.qualifiedName()+" */");


	/*
	 * Declare the structure which will be initialized with a
	 * JavaMethodDescriptor and its associated structures: code array,
	 * line numbers, exceptions, local variables
	 * Unlike several of the other structures, we do not bother remembering
	 * and reusing prior definitions -- there are so many dimensions it doesn't
	 * see worthwhile.
	 * In fact, the only reason this is not an anonimous structure is the
	 * case where it has to be writable and initialized, for JVMTI.
	 */
	if ( ! impureCode ){
	    // this might as well be static since there will be
	    // no external references to it
	    classOut.print("STATIC ");
	    declareOut = classOut;
	} else {
	    headerOut.print("extern ");
	    declareOut = headerOut;
	}

	declareOut.print("const struct ");
	declareOut.print( descriptorTypeTag );
	declareOut.println(" {\n    union {");
	declareOut.println("        CVMJavaMethodDescriptor jmd;");
	declareOut.println("        CVMUint" +
		    ((meth.alignment() == 1) ? "8" : "32") +
		    " align;\n    } u;");
	declareOut.println("    CVMUint8 data["+codeLengthRoundup+"];");
	if ( nTryCatch != 0 ){
	    declareOut.println("    CVMExceptionHandler exceptionTable["+
			nTryCatch+"];");
	}
	if ( nLineNumbers != 0 ){
	    declareOut.println("    CVMLineNumberEntry lineNumberTable["+
			nLineNumbers+"];");
	}
	if ( nLocals != 0 ){
	    declareOut.println("    CVMLocalVariableEntry localVariableTable["+
			nLocals+"];");
	}
	declareOut.print("} ");

	/*
	 * Now we have the shape of the data we need to supply the data itself
	 * (themselves?)
	 *
	 * EITHER the MD has to be writable. in thise case the declaration above
	 * was to the header file. Append to it the names of the master and
	 * the working copy, emit the master, reserve space for the copy,
	 * and don't forget the initialization instructions.
	 *
	 * OR the MD is read-only, const, so this is the thing itself
	 */

	if ( impureCode ){
	    declareOut.print( codeBlockName );
	    declareOut.print("Master;\n");

	    declareOut.print(" extern struct " + descriptorTypeTag );
	    declareOut.write(' ');
	    declareOut.println( codeBlockName );
	    declareOut.println(";");

	    classOut.print("struct " + descriptorTypeTag );
	    classOut.write(' ');
	    classOut.println( codeBlockName );
	    classOut.println(";");

	    classOut.print( "const struct " );
	    classOut.print( descriptorTypeTag );
	    classOut.write(' ');
	    classOut.print( codeBlockName );
	    classOut.print( "Master" );

            initInfo.addInfo("&" + codeBlockName + "Master",
                             "&" + codeBlockName,
                             "sizeof( struct " + descriptorTypeTag + ")");
	} else {
            classOut.print(codeBlockName);
	}

        classOut.print(" = {\n    { CVM_INIT_JAVA_METHOD_DESCRIPTOR(");
	/*
	 * Make sure locals are big enough for any return value.
	 */
	classOut.print(mi.stack+", "+Math.max(2,mi.locals));
	classOut.print( ", ");
	if ( meth.codeHasJsr() ){
	    classOut.print("CVM_JMD_MAY_NEED_REWRITE|");
	    if (meth.alignment() != 1) {
		classOut.print("CVM_JMD_NEEDS_ALIGNMENT|");
	    }
	}
	if (meth.isStrictFP()) {
	    classOut.print("CVM_JMD_STRICT|");
	}
	classOut.print("0, " );
	classOut.print(nTryCatch+", "+mi.code.length);
	classOut.print(", "+nLineNumbers+", "+nLocals);
	classOut.print(") },\n    {");

	// output the code array
	int j = 99;
	byte[] methodCode = mi.code;
	int l = methodCode.length;
	for ( int i = 0; i <l; i++ ){
	    if ( j >= 12 ){
		classOut.print("\n\t");
		j = 1;
	    } else {
		j+= 1;
	    }
	    classOut.printHexInt( methodCode[i]& 0xff );
	    classOut.print(",");
	}
	classOut.print("\n    }");
	ncodebytes += mi.code.length;

	// output the CVMExceptionHandler[] (catch frames)
	if ( nTryCatch != 0 ){
	    classOut.println(",\n    {");
	    for ( int i = 0; i < nTryCatch; i++ ){
		ExceptionEntry e = mi.exceptionTable[i];
                classOut.println("\t{ " + e.startPC + ", " + e.endPC + ", " +
                                 e.handlerPC + ", " +
                                 (e.catchType == null ? 0 : e.catchType.index) +
                                 "},");
	    }
	    classOut.print("    }");
	    ncatchframes += nTryCatch;
	}

	// output the CVMLineNumberEntry[]
	if ( nLineNumbers != 0 ){
	    classOut.println(",\n    {");
	    for ( int i = 0; i < nLineNumbers; i++ ){
		LineNumberTableEntry e = lntab[i];
		classOut.println("\t{ "+e.startPC+", "+e.lineNumber+"},");
	    }
	    classOut.print("    }");
	}

	// output the CVMLocalVariableEntry[]
	if ( nLocals != 0 ){
	    classOut.println(",\n    {");
	    for ( int i = 0; i < nLocals; i++ ){
		LocalVariableTableEntry e = locvartab[i];
		int typeid = CVMDataType.parseSignature(getUTF(e.sig));
		int nameid = CVMMemberNameEntry.lookupEnter(getUTF(e.name));
		classOut.println("\t{ "+e.pc0+", "+e.length+", "+e.slot+
                    ", RAW_TYPEID_NAME_PART(0x" + Integer.toHexString(nameid) +
                    "), RAW_TYPEID_TYPE_PART(0x" + Integer.toHexString(typeid) +
                    ")},");
	    }
	    classOut.print("    }");
	}

	classOut.println("\n};");


    }

    //
    // The cb repeats every 'cbRepeat' entries in MethodArray's and
    // FieldArray's
    //
    static final int cbRepeat = 256;
    
    private void handleMethodArrayDecl(String flavorName,
				       int nmethods) {
	// need to declare a structure containing a classblock pointer
	// plus "nmethods" CVMMethodBlock structures
	int i;
	headerOut.print(flavorName+" {");
	if (nmethods > cbRepeat)
	    headerOut.println();
	int numMethodArrays = (nmethods-1) / cbRepeat + 1;
	for (i = 1; i <= numMethodArrays; i++) {
	    int nmethods2; // # methods in this method array
	    if (i == numMethodArrays) {
		nmethods2 = (nmethods-1) % cbRepeat + 1;
	    } else {
		nmethods2 = cbRepeat;
	    }
	    headerOut.println(" CVMClassBlock const *cb" + i + ";" +
			      " CVMMethodBlock mb" + i + 
			      "[" + nmethods2 + "];");
	    if (nmethods > cbRepeat) {
		headerOut.println();
	    }
	}
	headerOut.println(" };");
    }

    private void declareMethodArray( CVMClass c,
				     boolean writeable,
				     boolean compressible ){
	String thisTableName = c.getNativeName()+"_methods";
	String tableAddress = "&"+thisTableName;
	int nmethods = c.methods.length;
	String methodArrayFlavor = "struct CVMMethodArray"+nmethods;
	String methodArrayImmutFlavor = "struct CVMMethodArrayImmut"+nmethods;
	String methodArrayImmutCompFlavor =
            "struct CVMMethodArrayImmutCompressed"+nmethods;
	String immutFlavor;
	String immutableType;
	java.util.BitSet methArrSizes;

        /* count the total number of romized methods */
        nTotalMethods += nmethods;
	
	if (compressible) {
	    immutableType = "CVMMethodBlockImmutableCompressed";
	    immutFlavor   = methodArrayImmutCompFlavor;
	    methArrSizes  = compressibleMethodArraySizes;
	} else {
	    immutableType = "CVMMethodBlockImmutable";
	    immutFlavor   = methodArrayImmutFlavor;
	    methArrSizes  = methodArraySizes;
	}

	/*
	 * Declare the runtime typedef
	 */
	if ( !runtimeMethodArraySizes.get(nmethods) ) {
	    handleMethodArrayDecl(methodArrayFlavor, nmethods);
	    runtimeMethodArraySizes.set(nmethods);
	}

	/*
	 * And now, declare the proper immutable type
	 */
	if ( !methArrSizes.get( nmethods ) ) {
	    /* The immutable version is easy. Just bundle all the immutable
	       parts together, and let the initializer take care
	       of the methods arrays */
	    headerOut.print(immutFlavor+" {");
	    headerOut.println(" CVMClassBlock const *cb; " + immutableType + 
			      " mb["+ nmethods + "];");
	    headerOut.println(" };");
	    methArrSizes.set( nmethods );
        }
	
	if (writeable ){
	    /*
	     * Declare actual array, record copying information.
	     * What we're about to initialize is the read-only master.
	     */
	    String masterName = thisTableName+"Master";

	    /* 
	     * Moved global variables to CVMROMGlobals.
	     */
	    /*
	    declare(methodArrayFlavor+" "+thisTableName, classOut); 
	    classOut.println(";");
	    */
            globalHeaderOut.println("    " + methodArrayFlavor + " " +
                                    thisTableName + ";");
	    initInfoForMBs.addInfo( "&"+masterName );
	    declare( "const "+immutFlavor+" "+masterName, classOut );
	} else {
	    declare( "const "+methodArrayFlavor+" "+thisTableName, classOut);
	}
	/* 
	 * Moved globals to CVMROMGlobals.  For classes without
	 * writeable methods, there is (still) a constant global
	 * variable <class>_methods - nothing has changed.  For
	 * classes with writeable methods, the <class>_methods
	 * variable lives in the CVMROMGlobals structure.  
	 */
	if (writeable) {
	    methodArrayName = "&CVMROMGlobals." + thisTableName;
	}
	else {
	    methodArrayName = tableAddress;
	}
	methodTableSize = nmethods;
    }

    //
    // Find out variations of several method components to see whether
    // compression opportunities exist.
    //
    // 1) Look for (argsSize, invokerIdx, accessFlags) triples. These
    // don't vary much, so we can encode commonly occurring
    // variants within 8-bits.
    //
    // 2) Check whether the method table index and checkedExceptionsOffset
    // can each be encoded in a byte.
    //
    // If #1 and #2 hold, we can shave off a word from the immutable method
    // block encoding.
    //
    void analyzeMethodsForCompression(CVMClass c) {
	CVMMethodInfo m[] = c.methods;
	int nCheckedExceptions = 0;
	int currCheckedException = 0;
	int checkedExceptionVector[];
	if ( (m == null) || (m.length == 0 )){
	    /* Nothing to do */
	    return;
	}
	int nmethod =  m.length;
	//
	// Traverse all methods, and look for opportunities. If any
	// one method in the methods of a class is not compressible,
	// the entire method array is expanded in the "long" form
	//
	for ( int i = 0; i < nmethod; i++ ){
	    CVMMethodInfo meth = m[i];
	    MethodInfo    mi   = meth.method;
	    if ( mi.exceptionsThrown != null ){
		int n = mi.exceptionsThrown.length;
		if ( n != 0 )
		    nCheckedExceptions += n+1; // "1" is for the count word.
	    }
	}
	//
	// if any methods declare that they throw any exceptions,
	// we need to construct the exception vector.
	if ( nCheckedExceptions == 0 ){
	    checkedExceptionVector = null;
	} else {
	    // Ensure that item 0 in this array is always unused.
	    // See classes.h.
	    nCheckedExceptions++;
	    checkedExceptionVector = new int[ nCheckedExceptions ];
	    checkedExceptionVector[0] = 0;
	    currCheckedException = 1;
	}

	for ( int i = 0; i < nmethod; i++ ){
	    CVMMethodInfo meth = m[i];

	    //
	    // Item #1
	    //
	    int access = meth.CVMflags();

	    boolean synchro = (access&CVM_METHOD_ACC_SYNCHRONIZED)!=0;
	    boolean isNative  = (access&CVM_METHOD_ACC_NATIVE)!=0;
	    boolean isAbstract= (access&CVM_METHOD_ACC_ABSTRACT) != 0;

	    //
	    // Item #2
	    //
	    String invoker = isAbstract ? "CVM_INVOKE_ABSTRACT_METHOD"
			  : isNative ? (
				isClassCNI(c.classInfo.className) ?
				    "CVM_INVOKE_CNI_METHOD" : (
				synchro ? "CVM_INVOKE_JNI_SYNC_METHOD"
					: "CVM_INVOKE_JNI_METHOD" ))
			  : (
				synchro ? "CVM_INVOKE_JAVA_SYNC_METHOD"
					: "CVM_INVOKE_JAVA_METHOD" );

	    //
	    // note the offset in the exception array,
	    // and add to that array as necessary.
	    //
	    int exceptions;
	    if ((meth.method.exceptionsThrown != null) &&
                (meth.method.exceptionsThrown.length != 0)) {
		ClassConstant thrown[] = meth.method.exceptionsThrown;
		int net = thrown.length;
	        exceptions = currCheckedException;
		checkedExceptionVector[currCheckedException++] = net;
		for ( int j = 0; j < net; j++ ){
		    checkedExceptionVector[currCheckedException++] =
                        thrown[j].index;
		}
	    } else {
		// Note we have changed this from ((unsigned short)
		// -1) to 0 to be consistent with the documentation in
		// classes.h. A checkedExceptionsOffset of 0 indicates
		// no checked exceptions for the method. Also see the
		// allocation of the checkedExceptionVector above.
		exceptions = 0;
	    }
	    //
	    // Item #3
	    //
	    int argssize   = meth.method.argsSize;

	    int tableIndex = meth.methodOffset();
	    
	    //
	    // Remember these items
	    // We won't know whether a class is compressible
	    // until after all the triples for all methods have been seen
	    //
	    methStats.record(argssize, invoker, access);

	    // 
	    // Also see whether the method table index and
	    // checkedExceptionsOffset are encodable in one byte each.
	    //
	    if (tableIndex >= 256) {
		c.setCompressible(false);
	    }
	    if (exceptions >= 256) {
		c.setCompressible(false);
	    }
	}
    }

    void writeMethods(CVMClass c) {
	boolean isCompressible = c.isCompressible();
	String cbname_comma = "&" + cbName(c) + ",";
	CVMMethodInfo m[] = c.methods;
	int nCheckedExceptions = 0;
	int currCheckedException = 0;
	int checkedExceptionVector[];
	boolean writableMethods = mutableMBs;
	if ( (m == null) || (m.length == 0 )){
	    methodArrayName = "0";
	    checkedExceptionName = "0";
	    methodTableSize = 0;
	    return;
	}
	int nmethod =  m.length;
	String thisTableName = c.getNativeName()+"_methods";
	String thisExceptionName = null;
	for ( int i = 0; i < nmethod; i++ ){
	    CVMMethodInfo meth = m[i];
	    MethodInfo    mi   = meth.method;
	    if ( mi.exceptionsThrown != null ){
		int n = mi.exceptionsThrown.length;
		if ( n != 0 )
		    nCheckedExceptions += n+1; // "1" is for the count word.
	    }
	    if ( mi.code != null ){
		writeCode( meth );
	    }
	    if ( meth.codeHasJsr() ){
		writableMethods = true;
	    }
	}
	//
	// if any methods declare that they throw any exceptions,
	// we need to construct the exception vector.
	if ( nCheckedExceptions == 0 ){
	    checkedExceptionName = "0";
	    checkedExceptionVector = null;
	} else {
	    thisExceptionName = c.getNativeName()+"_throws";
	    checkedExceptionName = thisExceptionName;
	    // Ensure that item 0 in this array is always unused.
	    // See classes.h.
	    nCheckedExceptions++;
	    checkedExceptionVector = new int[ nCheckedExceptions ];
	    checkedExceptionVector[0] = 0;
	    currCheckedException = 1;
	}
	boolean emittedNativeHeader = false;
	//
	// Look to see whether we can compact this
	//
	for ( int i = 0; i < nmethod; i++ ){
	    CVMMethodInfo meth = m[i];
	    int access = meth.CVMflags();
	    if (!emittedNativeHeader &&
		((access&CVM_METHOD_ACC_NATIVE)!=0)) {
		String subdir;
		subdir = isClassCNI(c.classInfo.className) ? "cni/" : "jni/";
		headerOut.println("#include \"generated/" + subdir +
		    Util.convertToClassName(c.classInfo.className)+".h\"");
		emittedNativeHeader = true;
	    }
	    boolean synchro = (access&CVM_METHOD_ACC_SYNCHRONIZED)!=0;
	    boolean isNative  = (access&CVM_METHOD_ACC_NATIVE)!=0;
	    boolean isAbstract= (access&CVM_METHOD_ACC_ABSTRACT) != 0;

	    String invoker = isAbstract ? "CVM_INVOKE_ABSTRACT_METHOD"
			  : isNative ? (
				isClassCNI(c.classInfo.className) ?
				    "CVM_INVOKE_CNI_METHOD" : (
				synchro ? "CVM_INVOKE_JNI_SYNC_METHOD"
					: "CVM_INVOKE_JNI_METHOD" ))
			  : (
				synchro ? "CVM_INVOKE_JAVA_SYNC_METHOD"
					: "CVM_INVOKE_JAVA_METHOD" );
	    int argssize   = meth.method.argsSize;
	    
	    CVMMethodStats.Entry entry =
		methStats.lookup(argssize, invoker, access);
	    //
	    // Cannot encode in one byte, therefore cannot compress
	    //
	    if (entry.getIdx() >= 256) {
		c.setCompressible(false);
	    }
	    meth.statsEntry = entry;
	}

	//
	// Update the compressibility of class c
	//
	isCompressible = isCompressible && c.isCompressible();

	//
	// isCompressible cannot be true if the mb's are to be read-only
	//
	isCompressible = isCompressible && writableMethods;
	
	// if any method has a JSR, then its method block, and thus
	// all of them, must be writable, so we can replace its code, bundled
	// with its JavaMethodDescriptor.
	declareMethodArray( c, writableMethods, isCompressible );
	String cbDeclaration;
	
	if (isCompressible) {
	    cbDeclaration = "CVM_INIT_METHODARRAY_CB_WITH_COMPRESSED_MBS";
	} else {
	    cbDeclaration = "CVM_INIT_METHODARRAY_CB_WITH_UNCOMPRESSED_MBS";
	}
	
	classOut.print(" = {\n\t" + cbDeclaration + "(&" + classBlockName +
                       "),\n\t{\n");

	for ( int i = 0; i < nmethod; i++ ){
	    CVMMethodInfo meth = m[i];
	    if (meth.method.hasCheckinits) {
		nMethodsWithCheckinitInvokes++;
	    }
	    int mbIndex = i % cbRepeat;
	    int typeid = meth.sigType.entryNo;
	    int nameid = CVMMemberNameEntry.lookupEnter(
                             getUTF(meth.method.name));
	    String nameAndType = "RAW_TYPEID(0x" +
                                 Integer.toHexString(nameid) + ", 0x" +
                                 Integer.toHexString(typeid) + ")";
	    int access = meth.CVMflags();

	    boolean synchro = (access&CVM_METHOD_ACC_SYNCHRONIZED)!=0;
	    boolean isNative  = (access&CVM_METHOD_ACC_NATIVE)!=0;
	    boolean isAbstract= (access&CVM_METHOD_ACC_ABSTRACT) != 0;

	    String invoker = meth.statsEntry.getInvoker();

	    /* By default let interpreter do invocation in all
	       except native methods. Compilation might change
	       invoker function */
	    String jitInvoker = isNative ? (
				    isClassCNI(c.classInfo.className) ?
				        "CVMCCMinvokeCNIMethod" :
				        "CVMCCMinvokeJNIMethod" )
		                : "CVMCCMletInterpreterDoInvoke";
	    

	    //
	    // note the offset in the exception array,
	    // and add to that array as necessary.
	    int exceptions;
	    if ((meth.method.exceptionsThrown != null) &&
                (meth.method.exceptionsThrown.length != 0)) {
		ClassConstant thrown[] = meth.method.exceptionsThrown;
		int net = thrown.length;
	        exceptions = currCheckedException;
		checkedExceptionVector[currCheckedException++] = net;
		for ( int j = 0; j < net; j++ ){
		    checkedExceptionVector[currCheckedException++] =
                        thrown[j].index;
		}
	    } else {
		// Note we have changed this from ((unsigned short)
		// -1) to 0 to be consistent with the documentation in
		// classes.h. A checkedExceptionsOffset of 0 indicates
		// no checked exceptions for the method. Also see the
		// allocation of the checkedExceptionVector above.
		exceptions = 0;
	    }
	    int argssize   = meth.method.argsSize;
	    int tableIndex = meth.methodOffset();
	    
	    /* 
	     * method offsets are always mod 'cbRepeat' and we have to
	     * emit a cb entry before each block of 'cbRepeat'
	     * methods. 
	     */
	    if ((mbIndex == 0) && (i != 0) && 
		!isCompressible && !writableMethods) {
		// Output the repeated cb only if we are emitting read-only
		// method blocks.
		classOut.print( "\t},\n");
		classOut.print( "\t&"+classBlockName+",\n\t{\n" );
	    }

	    if (isCompressible) {
		// The C compiler checks that the values here are
		// really compressed within the proper sizes. We get
		// warnings if we exceed the maximum width for a given
		// data field.
		//
		// entryIdx encapsulates argssize, invoker, and access flags
		int entryIdx = meth.statsEntry.getIdx();
		classOut.print("\tCVM_INIT_METHODBLOCK_IMMUTABLE_COMPRESSED(" +
                               cbname_comma + nameAndType + "," + tableIndex +
                               "," + entryIdx + "," + mbIndex + "," +
                               exceptions + ",");
	    } else if (writableMethods) {
		classOut.print("\tCVM_INIT_METHODBLOCK_IMMUTABLE(" +
                               cbname_comma + nameAndType + "," + tableIndex +
                               "," + argssize + "," + invoker + "," + access +
                               "," + mbIndex + "," + exceptions + ",");
	    } else {
		classOut.print("\tCVM_INIT_METHODBLOCK(" + cbname_comma +
                               jitInvoker + "," + nameAndType + "," +
                               tableIndex + "," + argssize + "," +
                               invoker + "," + access + "," + mbIndex +
                               "," + exceptions + ",");
	    }
	    
	    if ( isAbstract ){
		classOut.print("(CVMJavaMethodDescriptor*)"+i+"),\n");
	    } else if ( isNative ){
		if (isClassCNI(c.classInfo.className)) {
		    String jniName = meth.method.getNativeName(true);
		    String eniName = "CNI" + jniName.substring(5);
		    classOut.print("(CVMJavaMethodDescriptor*)&" + eniName +
                                   "),\n");
		} else {
		    classOut.print("(CVMJavaMethodDescriptor*)&" +
			meth.method.getNativeName(true)+"),\n");
		}
	    } else {
		classOut.print("&(" + meth.getNativeName() + ".u.jmd)),\n");
	    }
	}
	classOut.print("}};\n");
        this.nmethods += nmethod;
	if (writableMethods) {
	    nClassesWithWritableMethodBlocks += 1;
	    nWritableMethodBlocks += nmethod;
	}
	//
	// now, if there are any exception-catch clauses
	// that need to be represented, print out the table.
	if (nCheckedExceptions != 0) {
	    classOut.println("STATIC const CVMUint16 " + thisExceptionName +
                             "[] = {");
	    int j = 0;
	    for (int i = 0; i < nCheckedExceptions; i++) {
		if (j >= 8) {
		    classOut.print("\n    ");
		    j = 0;
		} else {
		    j += 1;
		}
		classOut.print( checkedExceptionVector[i]+", ");
	    }
	    classOut.println("\n};");
	}
    }

    /*
     * Write out the InnerClassInfo structure. This structure will contain
     * information about all referenced inner classes.
     */
    private void
    writeInnerClasses(CVMClass c) { 
	//Write information about each inner class contained in this class.
	int innerClassCount = c.classInfo.innerClassAttr.getInnerClassCount();
	InnerClassAttribute ica = c.classInfo.innerClassAttr;
	classOut.println("    " + innerClassCount + ", {");
	for (int j=0; j < innerClassCount; j++) {
	    // write out the indexes and access flags of this inner class.
	    classOut.print("        {");
	    classOut.print(ica.getInnerInfoIndex(j));
	    classOut.write(',');
	    classOut.print(ica.getOuterInfoIndex(j));
	    classOut.write(',');
	    /* innerNameIndex not supported because utf8 cp entries
	     * are not kept around.
	    classOut.print(ica.getInnerNameIndex(j));
	    */
	    classOut.print('0');
	    classOut.write(',');
	    classOut.print(ica.getAccessFlags(j));
	    classOut.print("},\n");
	}
	classOut.println("    }");
	ninnerclasses += innerClassCount;
    }

    void mungeAllIDsAndWriteExternals(ClassClass classes[]) { 
    }

    private static int CVMDataAccessFlags( int access ){
	int a = 0;
	if ( (access&ACC_PUBLIC) != 0 ) a |= CVM_FIELD_ACC_PUBLIC;
	if ( (access&ACC_PRIVATE) != 0 ) a |= CVM_FIELD_ACC_PRIVATE;
	if ( (access&ACC_PROTECTED) != 0 ) a |= CVM_FIELD_ACC_PROTECTED;
	if ( (access&ACC_STATIC) != 0 ) a |= CVM_FIELD_ACC_STATIC;
	if ( (access&ACC_FINAL) != 0 ) a |= CVM_FIELD_ACC_FINAL;
	if ( (access&ACC_VOLATILE) != 0 ) a |= CVM_FIELD_ACC_VOLATILE;
	if ( (access&ACC_TRANSIENT) != 0 ) a |= CVM_FIELD_ACC_TRANSIENT;
	return a;
    }

    private void declareFieldArray( CVMClass c ){
	// Note: Should deal with > 255 fields.
	// Today is not that day. Just do the simple case.
	FieldInfo m[] = c.classInfo.fields;
	int nfields = m.length;
	String thisTableName = c.getNativeName()+"_fields";
	String fieldArrayFlavor = "struct CVMFieldArray"+nfields;
	if ( !fieldArraySizes.get( nfields ) ){
	    // need to declare a structure containing a classblock pointer
	    // plus "nfields" CVMFieldBlock structures
	    int i;
	    headerOut.print(fieldArrayFlavor+" {");
	    if (nfields > cbRepeat)
		headerOut.println();
	    int numFieldArrays = (nfields-1) / cbRepeat + 1;
	    for (i = 1; i <= numFieldArrays; i++) {
		int nfields2; // # fields in this field array
		if (i == numFieldArrays) {
		    nfields2 = (nfields-1) % cbRepeat + 1;
		} else {
		    nfields2 = cbRepeat;
		}
		headerOut.print(" CVMClassBlock const *cb" + i + ";" + 
				" CVMFieldBlock fb" + i + 
				"[" + nfields2 + "];");
		if (nfields > cbRepeat) {
		    headerOut.println();
		}
	    }
	    headerOut.println(" };");
	    fieldArraySizes.set( nfields );
	}
	declare( "const "+fieldArrayFlavor+" "+thisTableName, classOut);
	fieldTableName = "&"+thisTableName;
	fieldTableSize = nfields;
    }

    void writeFields(CVMClass c) {
	// Note: Should deal with > 255 fields.
	// Today is not that day. Just do the simple case.
	FieldInfo m[] = c.classInfo.fields;
	if ( (m == null) || (m.length== 0)){
	    fieldTableName = "0";
	    fieldTableSize = 0;
	    return;
	}
	int nfields = m.length;
	declareFieldArray( c );
	classOut.print( " = {\n");
	for ( int i = 0; i < nfields; i++ ){
	    int fbIndex = i % cbRepeat;
	    FieldInfo f = m[i];
	    int typeid = CVMDataType.parseSignature(getUTF(f.type));
	    int nameid = CVMMemberNameEntry.lookupEnter(getUTF(f.name));
	    String nameAndType = "RAW_TYPEID(0x" + Integer.toHexString(nameid) +
                                 ", 0x" + Integer.toHexString(typeid) + ")";
	    //System.out.println( nameAndType+" => "+Integer.toHexString(typeid) );
	    int access = CVMDataAccessFlags(f.access);
	    int offset = f.instanceOffset;
	    /* field offsets are always mod cbRepeat and we have to emit a cb
	     * entry before each block of cbRepeat fields. */
	    if (fbIndex == 0) {
		if (i != 0) {
		    classOut.print( "\t},\n");
		}
		classOut.print( "\t&"+classBlockName+",\n\t{\n" );
	    }
	    // static members use simple byte offset
	    // instance members include size of header info
	    if (f.isStaticMember()) {
		classOut.print("\tCVM_INIT_FIELDBLOCK(" + nameAndType +
                               "," + access + "," + fbIndex +
                               ", CVM_STATIC_OFFSET(" + offset + ")),\n");
	    } else {
		classOut.print("\tCVM_INIT_FIELDBLOCK(" + nameAndType +
                               "," + access + "," + fbIndex +
                               ", CVM_FIELD_OFFSET(" + offset + ")),\n");
	    }
	}
	classOut.println("}};");
	this.nfields += nfields;

    }

    private String methodBlockAddress( MethodInfo mi ){
	CVMMethodInfo em = (CVMMethodInfo)(mi.vmMethodInfo);
	CVMClass      ec = (CVMClass)(mi.parent.vmClass);
	int n = mi.index;
	/* 
	 * Moved globals to CVMROMGlobals.  For classes without
	 * writeable methods, there is (still) a constant global
	 * variable <class>_methods - nothing has changed.  For
	 * classes with writeable methods, the <class>_methods
	 * variable lives in the CVMROMGlobals structure. The ability
	 * to determine whether a class has writable methods has been
	 * added to CVMClass.  
	 */
        if (mutableMBs || ec.hasWritableMethods()) {
            // had better not be an interface.
            return "&CVMROMGlobals." + ec.getNativeName() + "_methods.mb" +
                   (n/cbRepeat+1) + "[" + n%cbRepeat + "]";
        }
	// had better not be an interface.
        return "&" + ec.getNativeName() + "_methods.mb" + (n/cbRepeat+1) +
               "[" + n%cbRepeat + "]"; 
    }

    private static String fieldBlockAddress( FieldInfo f ){
	// pretty disgusting.
	// if this is frequent enough, find a better way!
	ClassInfo c = f.parent;
	FieldInfo cf[] = c.fields;
	int n = f.index;
	//for ( n = 0; cf[n] != f; n++ ) // TRY THIS.
	 //   ;
	// assume we found it.
	String parentName = ((CVMClass)(c.vmClass)).getNativeName();
	return "&"+parentName+"_fields.fb"+(n/cbRepeat+1)+"["+n%cbRepeat+"]";
    }

    private static final int unresolvedCVMConstantMap[] = {
	CVM_CONSTANT_Invalid,
	CVM_CONSTANT_Utf8,
	CVM_CONSTANT_Invalid,
	CVM_CONSTANT_Integer|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Float|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Long|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Double|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_ClassTypeID,
	CVM_CONSTANT_StringObj|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Fieldref,
	CVM_CONSTANT_Methodref,
	CVM_CONSTANT_InterfaceMethodref,
	CVM_CONSTANT_NameAndType
    };

    private static final int resolvedCVMConstantMap[] = {
	CVM_CONSTANT_Invalid,
	CVM_CONSTANT_Utf8,	CVM_CONSTANT_Invalid,
	CVM_CONSTANT_Integer|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Float|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Long|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_Double|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_ClassBlock|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_StringObj|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_FieldBlock|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_MethodBlock|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_MethodBlock|CVM_CONSTANT_POOL_ENTRY_RESOLVED,
	CVM_CONSTANT_NameAndType
    };

    private static int CVMconstantTag( ConstantObject c){
	int t;
	if ( c.isResolved() ){
	    t = resolvedCVMConstantMap[ c.tag ];
	    if (t == CVM_CONSTANT_NameAndType){
		String sig = ((NameAndTypeConstant)c).type.string;
		t = (sig.charAt(0) == SIGC_METHOD) ? 
		    CVM_CONSTANT_MethodTypeID : CVM_CONSTANT_FieldTypeID;
	    }
	} else {
	    t = unresolvedCVMConstantMap[ c.tag ];
	    if (t == CVM_CONSTANT_NameAndType){
		String sig = ((NameAndTypeConstant)c).type.string;
		t = (sig.charAt(0) == SIGC_METHOD) ? 
		    CVM_CONSTANT_MethodTypeID : CVM_CONSTANT_FieldTypeID;
	    }
	}
	return t;
    }

    // return address of this type table.
    private String writeTypeTable( ConstantObject constants[], String name ){
	int clen = constants.length;
	String ttname = name+"_tt";
	auxOut.print("CVMUint8 "+ttname+"[] = { 0,");
	int j = 99;
	for ( int i = 1; i < clen; i+=constants[i].nSlots ){
	    if ( j >= 10 ){
		auxOut.print("\n    ");
		j = 1;
	    } else {
		j += 1;
	    }
	    auxOut.print(CVMconstantTag(constants[i])+",");
	    if (constants[i].nSlots == 2){
		auxOut.print(" 0,"); // 2nd word of 2-word constant 
		j += 1;
	    }
	}
	auxOut.println("\n};");
	headerOut.println("extern CVMUint8 "+ttname+"["+clen+"];");
	return "&"+ttname;
    }

    // return address of this array Info.
    private String writeArrayInfo( ArrayClassInfo info, String name ){
	String arrayInfoName = name+"_ai";
	classOut.println("STATIC const union {");
	classOut.println("\tCVMArrayInfo ai;");
	classOut.println("\tCVMConstantPool constantpool;");
	classOut.println("} "+arrayInfoName+" = {{");
	classOut.println("\t"+info.depth+", "+info.baseType+",");
	classOut.println("\t(CVMClassBlock*)&"+
		    cbName((CVMClass)info.baseClass.find().vmClass)+",");
	classOut.println("\t(CVMClassBlock*)&"+
		    cbName((CVMClass)info.subarrayClass.find().vmClass));
	classOut.println("}};");
	return "&"+arrayInfoName+".ai";
    }

 
    // return address of this constant pool.
    private String writeConstants(ConstantPool cp, 
				  String name,
				  boolean export) {
	ConstantObject constants[] = cp.getConstants();
	int clen = constants.length;
	if ( (clen == 0) || (clen == 1 ) ){
	    return "0";
	}

	// Check if we need a type table.  When we write the class, we
	// check for unquickened bytecodes.  If we find any, we mark
	// the constant pool as needing a type table.  The check for
	// unresolved entries should be redundant, because unreferenced
	// entries should have been removed, and referenced entries
	// should have an unquickened reference.

	boolean doTypeTable = cp.needsTypeTable() ||
	    ClassClass.isPartiallyResolved(cp);

	if ( doTypeTable && !classLoading ){
	    // in future, do something more useful here.
	    System.err.println(Localizer.getString("cwriter.no_class_loading"));
	    // write the type table out anyway, in case
	    // there is someone to look at it at runtime!
	}
	String typeTableName;
	if ( doTypeTable ){
	    typeTableName = "CP_ADDR(" + writeTypeTable( constants, name ) + ")";
	}else{
	    typeTableName = "CP_ADDR(0)";
	}
	String constantPoolName = name+"_cp";
	classOut.print( doTypeTable ? "" : "const ");
	classOut.println("CVMAddr "+constantPoolName+"["+(clen)+"] = {");
	classOut.println("	(CVMAddr)"+typeTableName+",");

	if (export) {
	    // Export in the global header for multiple components of
	    // romjava files to find
	    headerOut.print("extern ");
	    headerOut.print( doTypeTable ? "" : "const ");
	    headerOut.println("CVMAddr "+
			      constantPoolName+"["+(clen)+"];");
	}
	for ( int i = 1; i < clen; i+=constants[i].nSlots ){
	    classOut.print("    ");
	    writeConstant( constants[i], classOut, "CP_" );
	    classOut.println(",");
	}
	classOut.println("};");
	nconstants += clen;
	return constantPoolName;
    }

    private void declareClassblock( CVMClass c, int innerClassCount ){
	String myName = c.getNativeName();
	String classblockFlavor = "struct CVMClassBlock"+innerClassCount;
        if (!classblockSizes.get(innerClassCount)) {
	    // need to declare a classblock-like struct with innerClassCount
	    // elements in its CVMInnerClassInfo array
	    if ( innerClassCount < 1 ){
		headerOut.println(classblockFlavor +
                                  " { CVMClassBlock classclass; };");
	    } else {
                headerOut.println(classblockFlavor +
                                  " { CVMClassBlock classclass; " +
                                  "CVMUint32 innerClassesCount; " +
                                  "CVMInnerClassInfo innerClasses[ " +
                                  innerClassCount + "]; };");
	    }
            classblockSizes.set(innerClassCount);
        }
	declare( "const "+classblockFlavor+" "+myName+"_Classblock", classOut);
    }

    // helper for classBitMap
    private static int GCbits( FieldInfo slottable[], int first, int last )
    {
	int mapval = 0;
	for ( int i = first; i < last; i++ ){
	    switch ( slottable[i].type.string.charAt(0) ){
	    case 'L':
	    case '[':
		// this is a reference.
		mapval |= 1<<(i-first);
	    }
	}
	return mapval;
    }

    //
    // compute the GC bit map for a class.
    // if it is big, then write it out as well. In any case,
    // return a string to use to initialize the gcMap field in the CVMClassBlock
    //
    private String classBitMap( CVMClass c ){
	if ( c.isArrayClass() ){
	    // either an array of primitive or an array of objects.
	    ArrayClassInfo ac = (ArrayClassInfo)(c.classInfo);
	    if ( (ac.depth==1) && (ac.baseType!=T_CLASS) )
		return "0"; // contains no references
	    else
		return "1"; // contains nothing but references.
	}
	ClassInfo ci = c.classInfo;
	FieldInfo ft[] = ci.slottable;
	if ( (ft==null) || (ft.length==0) ) return "0";
	int nslots;
	if ( (nslots=ft.length) <= 30 ){
	    // easy case. Small object. Map can be represented in a word.
	    return "0x"+Integer.toHexString(GCbits(ft,0,nslots)<<2);
	} else {
	    // hard case.
	    String myName = c.getNativeName()+"_gcBits";
	    int    nWords = (nslots+31)/32;
	    classOut.print( "STATIC const struct { " + 
			    "CVMUint16 maplen; CVMUint32 map["+nWords+"]; } ");
	    classOut.print( myName );
	    classOut.print( " = { ");
	    classOut.print( nWords );
	    classOut.print( ",\n    {" );
	    for ( int i = 0; i < nslots; i+= 32 ){
		classOut.print("0x"+Integer.toHexString(GCbits(ft, i,
						    Math.min( i+32, nslots))));
		classOut.print(", ");
	    }
	    classOut.println("} };");
	    return "(CVMAddr)(((char*)&"+myName+")+ CVM_GCMAP_LONGGCMAP_FLAG)";
	}
    }

    private void writeClassinfo( int classno, CVMClass c,
				 int clinitIdx, int nStaticWords ){

	if (c.hasStaticInitializer) {
	    nClinits++;
	}
	String superName    = (c.classInfo.superClassInfo == null)
			    ? ("0")
			    : ("&" + cbName((CVMClass)
                                         c.classInfo.superClassInfo.vmClass));
	String myStaticName = (c.nStaticWords == 0) 
			    ? ("0")
			    : ("(CVMJavaVal32*)STATIC_STORE_ADDRESS(0)");
	int innerClassCount = (c.classInfo.innerClassAttr == null)
			    ? (0)
			    : (c.classInfo.innerClassAttr.getInnerClassCount());
	String runtimeFlags = (c.hasStaticInitializer)
			    ? ("CVM_ROMCLASS_WCI_INIT")
			    : ("CVM_ROMCLASS_INIT");
	MethodInfo vtbl[]   = c.classInfo.methodtable;
	String methodTableName;
	int methodTableCount;
	String gcBits;

	/*
	 * Note: If this is a primitive class, the
	 * typecode is stored in the methodTableCount field.
	 * Since these things cannot be instantiated, they need
	 * no virtual methods.
	 */
	if ( c.isPrimitiveClass() ){
	    methodTableCount = c.typecode();
	    methodTableName = "0";
	} else {
	    // vtbl == null may occur for array classes! 
	    methodTableCount = (vtbl==null)?0:vtbl.length;
	    if (methodTableCount > 0) {
		methodTableName = c.getNativeName()+"_mt";
	    } else if (c.isArrayClass()) {
                methodTableCount =
                    c.classInfo.superClassInfo.methodtable.length;
		methodTableName = "java_lang_Object_mt";
	    } else { /* must be an interface */
		methodTableName = "0";
	    }
	}
	// classBitMap may do output, so it needs
	// to be called before declareClassblock!
	gcBits = classBitMap( c );

	declareClassblock( c, innerClassCount );

	int classid = c.getClassId();
	//System.out.println( c.ci.className+" => "+Integer.toHexString(classid) );

	classOut.println(" = {\n    CVM_INIT_CLASSBLOCK( "+gcBits+", " +
                         "RAW_TYPEID(0, 0x" + Integer.toHexString(classid) +
                         "), \"" + c.classInfo.className + "\",");
	classOut.println("    "+superName+",");
	classOut.println("    "+constantPoolName+", ");
	classOut.println("    "+interfaceTableName+",");
	classOut.println("    "+methodArrayName+",");
	classOut.println("    "+fieldTableName+", "+myStaticName+",");
	classOut.println("    "+constantPoolSize+", "+methodTableSize+", "+
		    fieldTableSize+", "+methodTableCount+", "+
		    c.CVMflags()+", "+runtimeFlags+", "+
		    nStaticWords+" + "+clinitIdx+
		    ", OBJ_SIZE("+c.instanceSize()+"), ");
	classOut.println("    (CVMClassICell*)(&CVM_classICell["+classno+"]), ");
	/* The slots for clinitEE is appended to 'staticStoreName'. */
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	classOut.println("    (CVMExecEnv**)&CVMROMGlobals."+staticStoreName+
					"["+nStaticWords+" + "+clinitIdx+"],");
	classOut.println("    "+checkedExceptionName+", ");
	if (classDebug && c.classInfo.sourceFileAttr != null) {
	    classOut.println("    \"" + c.classInfo.sourceFileAttr.sourceName +
                             "\", ");
	} else {
	    classOut.println("    0, ");
	}

	/* write out the methodTablePtr. */
	classOut.println("    " + methodTableName + ",");

	/* write out the ClassLoader and ProtectionDomain */
	int id = c.classInfo.loader.getID();
	if (id != 0) {
	    int clIDOff = 2; // first cl ID (boot == 0, all == 1)
	    int off = clRefOff + (id - clIDOff) * 2;
	    classOut.println("    (CVMClassLoaderICell *)" +
		"STATIC_STORE_ADDRESS(" + off + "), " +
		"(CVMObjectICell *)" +
		"STATIC_STORE_ADDRESS(" + (off + 1) + "),");
	} else {
	    classOut.println("    0, 0,");
	}

	/* write out the InnerClassesInfo */
	if (c.classInfo.innerClassAttr != null) {
	    classOut.println("    &" + c.getNativeName() +
			     "_Classblock.innerClassesCount),");
	    writeInnerClasses(c);
	} else {
	    classOut.println("    0)");
	}

	// TODO :: BEGIN
        // This is for adding class version number and signature later.
        // This code needs to be further reviewed and tested before enabling
        // it.
        if (false) { // disabled for now.
	    classOut.println("    /* major_version */ " +
                             c.classInfo.majorVersion + ",");
	    classOut.println("    /* minor_version */ " +
                             c.classInfo.minorVersion + ",");
	    classOut.print  ("    /* CB genSig     */ ");
	    //if (c.ci.signatureAttr != null) {
	    //  classOut.print("\""+c.ci.signatureAttr.signature+"\""); // !Not Working
	    //} else {
	        classOut.print(0);
	    //}
	    classOut.println(",");
	    classOut.println("    /* FB genSig *   */ " +
			     0 + ",");
	    classOut.println("    /* MB getSig *   */ " +
			     0 +"), ");

	    if (c.classInfo.innerClassAttr != null) {
		writeInnerClasses(c);
	    }
        }
        // TODO :: END

	classOut.println("};");
    }

    void writeMethodTable(CVMClass c) {
	MethodInfo vtbl[] = c.classInfo.methodtable;
	int        vtblSize;

	/*
	 * Note: If this is a primitive class, the
	 * typecode is stored in the methodTableCount field.
	 * Since these things cannot be instantiated, they need
	 * no virtual methods.
	 */
	if ( c.isPrimitiveClass() ){
	    return;
	} else {
	    // vtbl == null may occur for array classes! 
	    vtblSize = (vtbl==null)?0:vtbl.length;
	}

	if (vtblSize > 0) {
	    /* write out the methodtable. */
	    String methodTableName = c.getNativeName()+"_mt"+"["+vtblSize+"]";
	    if (c.classInfo.superClass == null) { // is it java/lang/Object
		// java/lang/Object needs an extern methodTable declaration.
		/* Added another const (for the pointer, which is also
		 * const here, not just the data) 
		 */
		declare("const CVMMethodBlock* const " + methodTableName,
                        classOut);
	    } else {
		/* Added another const (for the pointer, which is also const
                   here, not just the data)
		 */
		classOut.print("STATIC const CVMMethodBlock* const " +
                               methodTableName);
	    }
	    classOut.println(" = {");
	    for (int i = 0; i < vtblSize; i++) {
		classOut.println("    (CVMMethodBlock*)"+
				  methodBlockAddress(vtbl[i]) + ",");
	    }
	    classOut.println("};\n");
	}
    }
	
    /* (These two belong directly in the CCodeWriter) */

    private void
    writeIntegerValue( int v, CCodeWriter out ){
	 // little things make gcc happy.
	 if (v==0x80000000)
	    out.print( "(CVMInt32)0x80000000" );
	else
	     out.print( v );
    }

    private void
    writeLongValue( String tag, int highval, int lowval, CCodeWriter out ){
	out.print( tag );
	out.write( '(' );
	writeIntegerValue( highval, out );
	out.print( ", " );
	writeIntegerValue( lowval, out );
	out.write( ')' );
    }

    /* 
     * Added prefix argument to differ between writing entries to the
     * constant pool (where prefix="CP_") and the rest (where prefix
     * is empty). Additionally we use the INTEGER/LONG/... macros now
     * everywhere in this function to indicate, what type we are writing
     * (may be hard to spot otherwise).
     */
    private void
    writeConstant( ConstantObject value, CCodeWriter out, String prefix ){
	switch ( value.tag ){
	case Const.CONSTANT_INTEGER:
	case Const.CONSTANT_FLOAT:
	    /* 
	     * add INTEGER macro like the one for LONG and DOUBLE
	     * to have the possibility to change the representation
	     * platform dependent
	     */
	    /* 
	     * Add prefix to macro to get correct version.
	     */
            if (value.tag == Const.CONSTANT_INTEGER) {
                if (verbose) out.print("/* int */ ");
            } else {
                if (verbose) out.print("/* flt */ ");
            }
	    out.print( prefix + "INTEGER(" );
	    writeIntegerValue(((SingleValueConstant)value).value, out);
	    out.print( ")" );
	    break;
	case Const.CONSTANT_UTF8:
	    if (verbose) out.print("/* utf */ ");
	    out.print("(CVMInt32)");
	    out.write('"');
	    out.print( Util.unicodeToUTF(((UnicodeConstant)value).string ));
	    out.write('"');
	    // also, not sharing here at all.
	    break;
	case Const.CONSTANT_STRING:
	    /* 
	     * Moved global variables to CVMROMGlobals
	     */
	    /* CVM_BIGTYPEID: RS, 10/08/02:
	     * Use ADDR() macro to print out address.
	     */
	    if (verbose) out.print("/* str */ ");
	    out.print(prefix  + "ADDR(&CVMROMGlobals."+stringArrayName+"[");
	    if ( ((StringConstant)value).unicodeIndex == -1 ) {
		// Uninitialized, very bad
		throw new InternalError("Uninitialized string constant "+
					value);
	    } else {
		out.print( ((StringConstant)value).unicodeIndex );
	    }
	    
	    out.print("])");
	    break;
	case Const.CONSTANT_LONG:{
	    DoubleValueConstant dval = (DoubleValueConstant) value;
	    /*
	     * Add prefix to macro to get correct version.
	     */
	    if (verbose) out.print("/* lng */ ");
	    writeLongValue( "LONG",  dval.highVal, dval.lowVal, out );
	    }
	    break;
	case Const.CONSTANT_DOUBLE:{
	    DoubleValueConstant dval = (DoubleValueConstant) value;
	    /*
	     * Add prefix to macro to get correct version.
	     */
	    if (verbose) out.print("/* dbl */ ");
	    writeLongValue( "DOUBLE",  dval.highVal, dval.lowVal, out );
	    }
	    break;
	case Const.CONSTANT_CLASS:
	    if (value.isResolved()) {
		/* 
		 * Use ADDR() macro to print out address.
		 */
		if (verbose) out.print("/* cls */ ");
		out.print(prefix + "ADDR(&" +
                    cbName((CVMClass)(((ClassConstant)value).find().vmClass)) +
                    ")");
	    } else {
		int classid = CVMDataType.lookupClassname(
		    getUTF(((ClassConstant)value).name) );
		/*
		 * Use TYPEID() macro to create typeid from type part.
		 */
		if (verbose) out.print("/* cID */ ");
		out.print(prefix + "TYPEID(0, 0x" +
                          Integer.toHexString(classid) + ")");
	    }
	    break;
	case Const.CONSTANT_METHOD:
	case Const.CONSTANT_INTERFACEMETHOD:
	    if ( value.isResolved() ){
		/*
		 * Use ADDR() macro to print out address.
		 */
		if (value.tag == Const.CONSTANT_METHOD) {
		    if (verbose) out.print("/* mb  */ ");
		} else {
		    if (verbose) out.print("/* imb */ ");
		}
		out.print(prefix + "ADDR(");
		out.print(methodBlockAddress(((MethodConstant)value).find()) +
                          ")");
	    } else {
		FMIrefConstant ref = (FMIrefConstant)value;
		/* 
		 * Use MEMBER_REF() macro to create entry for parts.
		 * This is a pair of 16-bit indices into the cp.
		 * They reference a Class constant and a NameAndType
		 * constant.
		 * It is not a typeId.
		 */
		if (value.tag == Const.CONSTANT_METHOD) {
		    if (verbose) out.print("/* mID */ ");
		} else {
		    if (verbose) out.print("/* iID */ ");
		}
		out.print(prefix + 
		    "MEMBER_REF(0x"+Integer.toHexString(ref.clas.getIndex()) +
		    ", 0x"+Integer.toHexString(ref.sig.getIndex())+")");
	    }
	    break;
	case Const.CONSTANT_FIELD:
	    if ( value.isResolved() ){
		FieldInfo ftarget = ((FieldConstant)value).find();
		ClassInfo ctarget = ftarget.parent;
		// ROMized code always use checkinit version of opcodes
		// to access fields including static fields.
		/*
		 * Add prefix to macro to get correct version.
		 */
		if (verbose) out.print("/* fb  */ ");
		out.print(prefix + "ADDR(" + fieldBlockAddress(ftarget) + ")");
	    } else {
		FMIrefConstant ref = (FMIrefConstant)value;
		/* 
		 * Use MEMBER_REF() macro to create entry for parts.
		 * This is a pair of 16-bit indices into the cp.
		 * They reference a Class constant and a NameAndType
		 * constant.
		 * It is not a typeId.
		 */
		if (verbose) out.print("/* fID */ ");
		out.print(prefix + 
		    "MEMBER_REF(0x"+Integer.toHexString(ref.clas.getIndex()) + 
		    ", 0x"+Integer.toHexString(ref.sig.getIndex())+")");
	    }
	    break;
	case CONSTANT_NAMEANDTYPE:
	    NameAndTypeConstant ntcon = (NameAndTypeConstant)value;
	    int nameid = CVMMemberNameEntry.lookupEnter(getUTF(ntcon.name));
	    String sigString = getUTF(ntcon.type);
	    int typeid = (sigString.charAt(0)==SIGC_METHOD)
	       ? (CVMMethodType.parseSignature(sigString).entryNo)
	       : (CVMDataType.parseSignature(sigString));
	    /* 
	     * Use TYPEID() macro to create typeid from parts.
	     */
	    if (verbose) out.print("/* n&t */ ");
            out.print("TYPEID(0x" + Integer.toHexString(nameid) + ", 0x" +
                      Integer.toHexString(typeid) +")");
	    break;
	}
    }

    private int clRefOff;

    /*
     * Merge the static store for all classes into one area.
     * Sort static cells, refs first, and assign offsets.
     * Write the resulting master data glob and arrange for
     * it to be copied to writable at startup.
     */
    private int writeStaticStore( ClassClass cvec[] ){
	int nclass = cvec.length;
	int nStaticWords = 1; // 1 header word assumed
	int nRef    = 0;
	/* The number of necessary slots for clinitEE + 1.
	 * Index of a class that doesn't have <clinit> is always 0.
	 * The 0th entry is reserved for it. */
	int maxClinitIdx = 1;

	int maxClIdx = 0;

	/*
	 * Count all statics.
	 * Count refs, and count number of words.
	 */
	for ( int cno = 0; cno < nclass; cno++ ){
	    CVMClass c = (CVMClass)(cvec[cno]);
	    c.orderStatics();
	    nRef += c.nStaticRef;
	    nStaticWords += c.nStaticWords;
	    /* Count the number of necessary slots for clinitEE. */
	    if (c.hasStaticInitializer) {
		maxClinitIdx++;
	    }
	}

	int ncl = components.ClassTable.getNumClassLoaders();
	nRef += ncl * 2;
	nStaticWords += ncl * 2;

	/*
	 * Assign offsets and at the same time
	 * record any initial values.
	 * write it later.
	 */
	int refOff = 1;
	int scalarOff = refOff+nRef;

	ConstantObject staticInitialValue[] = new ConstantObject[nStaticWords];
	short          staticInitialSize[]  = new short[nStaticWords];

	clRefOff = refOff;
	for ( int i = 0; i < ncl; ++i) {
	    staticInitialValue[refOff] = null;
	    staticInitialSize[refOff] = 1;
	    ++refOff;
	    staticInitialValue[refOff] = null;
	    staticInitialSize[refOff] = 1;
	    ++refOff;
	}

	for ( int cno = 0; cno < nclass; cno++ ){
	    CVMClass c = (CVMClass)(cvec[cno]);
	    FieldInfo f[] = c.statics;
	    if ((f == null) || (f.length == 0)) continue; // this class has none
	    int     nFields = f.length;
	    for ( int i = 0; i < nFields; i++ ){
		FieldInfo fld = f[i];
		short     nslots = (short)(fld.nSlots);
		char toptype =  fld.type.string.charAt(0);
		if ( (toptype == 'L') || (toptype=='[')){
		    fld.instanceOffset = refOff;
		    staticInitialValue[refOff] = fld.value;
		    staticInitialSize[refOff] = 1;
		    refOff += 1;
		} else {
		    fld.instanceOffset = scalarOff;
		    staticInitialValue[scalarOff] = fld.value;
		    staticInitialSize[scalarOff] = nslots;
		    scalarOff += nslots;
		}
	    }
	}

	/*
	 * Allocate writable space for the static store.
	 * Start writing the static storage header.
	 * Remember that the first word is the number of refs.
	 * And many of those refs get initialized to zero.
	 * A static final String constant gets initialized with a pointer.
	 * 
	 * The buffer holds static field slots for the ROMized classes and 
	 * cliniEE (that includes runtime flags) slots for ROMized classes.
	 * The slot allocation looks like this:
	 *
	 *   CVM_StaticDataMaster[]
	 *   +---+---+-- --+---+-- --+---+---+---+-- --+---+
	 *   | 0 | 1 | ... | i | ... |n-1|n+0|n+1| ... |n+m|
	 *   +-^-+-^-+-- --+-^-+-- --+-^-+-^-+-^-+-- --+-^+
	 *     |   |         |         |   |   |         |
	 *     |   |         |         |   |   |   the last clinitEE slot for
	 *     |   |         |         |   |   |   ROMized class w/<clinit>
	 *     |   |    the last       |   |   |
	 *     |   |    static ref     |   | the 1st clinitEE slot for
	 *     |   |                   |   | ROMized class w/<clinit>
	 *     | the 1st               |   |
	 *     | static ref            | shared clinitEE slot
	 *     |                       | for ROMized classes w/o <clinit>
	 *   the number of             |
	 *   static refs (== i)   the end of static fields
	 *
	 *   where:
	 *     i := the number of static reference slots
	 *     n := the number of static field slots (including the 0th slot)
	 *     m := the number of clinitEE slots
	 */
	/* 
	 * Moved global variables to CVMROMGlobals
         */
	globalHeaderOut.println("    CVMAddr "+staticStoreName+
				"[CVM_STATIC_OFFSET("+
				nStaticWords+" + "+maxClinitIdx+")];");
	auxOut.println("const CVMUint32 "+numStaticStoreName+" = "+
		       nStaticWords+" + "+maxClinitIdx+";");
	auxOut.print("const CVMAddr "+masterStaticStoreName+"[] = {\n    ");
	auxOut.print( nRef ); // the initial count word.
	auxOut.print(",");
	//System.out.println(nRef + " reference statics, total words "+nStaticWords );

	short nslots = 1;
	int nConstPerLine = 1; // the initial count word.
	for ( int i = 1; i < nStaticWords; i += nslots ){
	    ConstantObject v = staticInitialValue[i];
	    nslots = staticInitialSize[i];
	    if ( v == null ){
		/*
		 * Use INTEGER() macro here for consistency.
		 */
		auxOut.print( "INTEGER(0)");
		if (nslots == 2 )
		    /*
		     * Use INTEGER() macro here for consistency.
		     */
		    auxOut.print( ", INTEGER(0)");
	    } else {
		/*
		 * Use normal macros (not the CP_-variants) for the constants.
		 */	       
		writeConstant( v, auxOut, "" );
	    }
	    auxOut.print(", ");
	    nConstPerLine += nslots;
	    if ( nConstPerLine >= 8 ){
		auxOut.print("\n    ");
		nConstPerLine = 0;
	    }
	}

	/* Append slots for clinitEE after static fields */
	auxOut.print("\n");
	auxOut.print("    /* clinitEEs */\n");
	auxOut.print("    CVM_CLINITEE_CLASS_INIT_FLAG");
	for (int i = 1; i < maxClinitIdx; i++) {
	    auxOut.print(", ");
	    if (i % 8 == 0) {
		auxOut.print("\n    ");
	    }
	    /*
	     * Use INTEGER() macro here for consistency.
	     */
	    auxOut.print("INTEGER(0)");
	}
	auxOut.println("\n};");
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
        initInfo.addInfo("&" + masterStaticStoreName,
                         "CVMROMGlobals." + staticStoreName, 
                         "(" + nStaticWords + " + " + maxClinitIdx + ")" +
                             "*sizeof(CVMAddr)");

	return nStaticWords;
    }

    /*
     * typedef struct {
     *     char *hash;			 * the string for this entry * 
     *     unsigned long info;		 * high bit set if malloc'ed
     * 				 * low 31 bits are the low 31 bits of hash * 
     * #define MALLOCED_FLAG (1u << 31)
     * #define HASHBIT_MASK (MALLOCED_FLAG - 1)
     * } StrIDhashSlot;
     *
     * typedef struct StrIDhash {
     *     int size;			* number of entries *
     *     hash_fn hash;		* hash function for this table *
     *     struct StrIDhash *next;	* next bucket *
     *     short used;			* number of full entries *
     *     short baseid;		* ID for item in slot[0] *
     *     void **params;		* param table, if needed *
     *     StrIDhashSlot slot[1];	* expanded to be number of slots *
     * } StrIDhash;
     */

    private String idTypeDecl[] = {
       "extern void default_hash();",
       "typedef struct {",
       "    const char *hash;",
       "    unsigned long info;",
       "} StrIDhashSlot;",
       "#define STR_ID_HASH(n)  struct { \\",
       "    int size; \\",
       "    void (*hash)(); \\",
       "    void *next; \\",
       "    short used; \\",
       "    short baseid; \\",
       "    HString **params; \\",
       "    StrIDhashSlot slot[ n ]; \\",
       "} "
    };

    /*
     * These are macros to encapsulate some of the messiness
     * of data definitions. They are, perhaps, compiler-specific.
     */
    private static String stdHeader[] = {
	"#define IN_ROMJAVA 1",
	"#include \"javavm/include/defs.h\"",
	"#include \"javavm/include/objects.h\"",
	"#include \"javavm/include/classes.h\"",
	"#include \"javavm/include/interpreter.h\"",
	"#include \"javavm/include/jni_impl.h\"",
	"#include \"javavm/include/typeid.h\"",
	"#include \"javavm/include/typeid_impl.h\"",
	"#include \"javavm/include/string_impl.h\"",
	"#include \"javavm/include/preloader_impl.h\"",
	"#include \"javavm/include/porting/endianness.h\"",
	"#ifdef CVM_JIT",
	"#include \"javavm/include/porting/jit/ccm.h\"",
	"#endif",
	/* 
	 * add possibility to change the INTEGER
	 * representation per platform
	 *
	 * Linux s390x:
	 * Loading an integer/float constant failed on 64 bit because
	 * the integer representation in the constant pool array
	 * includes the bits for the 4 byte int/float
	 * but is casted to a 8 byte value for 64 bit platforms (CVMAddr)
	 * which meens that the memory representation is changed.
	 * Therefore make sure the int/float representation looks like
	 * byte 0         4         8
	 *      |- int   -|-  pad  -|
	 * on 64 bit platforms with big endian.
	 */
	"/* CVM_BIGTYPEID: RS, 10/14/02:",
	" * Added macros to put different data types safely into their designated",
	" * structures. Unprefixed macro put the types into unions of the same size",
	" * as CVMAddr and macros prefixed with CP_ are used to put the specific",
	" * types into the constant pool.",
	" *",
	" * INTEGER(i)                 : Use this, if you want to write an integer",
	" * LONG(high, low)            : Use this, if you want to write a long",
	" *                              value consisting of the given high and low parts.",
	" * DOUBLE(high, low)          : Use this, if you want to write a double",
	" *                              value consisting of the given high and low parts.",
	" * ADDR(addr)                 : Use this, if you want to write an address.",
	" * TYPEID(namePart, typePart) : Use this, if you want to write a typeid consisting",
	" *                              of the given name and type parts.",
	" *",
	" * The following just return the typeids and their parts as their native types:",
	" *",
	" * RAW_TYPEID_NAME_PART(namePart) : " +
            "Use this, if you want to write the name part of a typeid.",
	" * RAW_TYPEID_TYPE_PART(typePart) : " +
            "Use this, if you want to write the type part of a typeid.",
	" * RAW_TYPEID(namePart, typePart) : " +
            "Use this, if you want to write a typeid consisting",
	" *                                  of the given name and type parts.",
	" */",
	"",
	"#define RAW_TYPEID_NAME_PART(namePart) \\",
	"    ((CVMTypeIDNamePart) (namePart))",
	"",
	"#define RAW_TYPEID_TYPE_PART(typePart) \\",
	"    ((CVMTypeIDTypePart) (typePart))",
	"",
	"#define RAW_TYPEID(namePart, typePart) \\",
	"    CVMtypeidCreateTypeIDFromParts(RAW_TYPEID_NAME_PART((namePart)), \\",
        "                                   RAW_TYPEID_TYPE_PART((typePart)))",
	"",
	"",
	"#if !defined(CVM_64)",
	"#    define TYPEID(namePart, typePart) RAW_TYPEID((namePart), (typePart))",
	"#else",
	"#    define TYPEID(namePart, typePart) \\",
        "         INTEGER(RAW_TYPEID((namePart), (typePart)))",
	"#endif",
	"",
	"",
	"#define ADDR(addr) ((CVMAddr) (addr))",
	"",
	"",
	"#ifdef CVM_64",
	"",
	"#    define LONG(high, low) \\",
        "         (((CVMAddr)(high)<<32) | ((CVMAddr)(low) & 0x0ffffffff)), 0",
	"#    define DOUBLE(high, low) LONG((high), (low))",
	"",
	"#    if (CVM_ENDIANNESS == CVM_BIG_ENDIAN)",
	"#        define INTEGER(i) (((CVMAddr) (i)) << 32)",
	"#    else",
	"#        define INTEGER(i) ((CVMAddr) (i))",
	"#    endif",
	"",
	"#else /* CVM_64 */",
	"",
	"#    define INTEGER(i) ADDR(i)",
	"",
	"#    if (CVM_ENDIANNESS == CVM_BIG_ENDIAN)",
	"#        define LONG(high, low) ADDR(high), ADDR(low)",
	"#    else",
	"#        define LONG(high, low) ADDR(low), ADDR(high)",
	"#    endif",
        "",
	"#    if (CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN)",
	"#        define DOUBLE(high, low) ADDR(high), ADDR(low)",
	"#    else",
	"#        define DOUBLE(high, low) ADDR(low), ADDR(high)",
	"#    endif",
	"",
	"#endif /* CVM_64 */", 
	"",
	"",
	"#define CP_ADDR(addr) ADDR((addr))",
	"#define CP_INTEGER(i) INTEGER((i))",
	"#define CP_LONG(high, low) LONG((high), (low))",
        "#define CP_DOUBLE(high, low) DOUBLE((high), (low))",
	"#define CP_TYPEID(namePart, typePart) TYPEID((namePart), (typePart))",
	"#if (CVM_ENDIANNESS == CVM_BIG_ENDIAN)",
	"#    define CP_MEMBER_REF(classPart,typePart) \\",
        "         INTEGER((((classPart)<<16)+(typePart)))",
	"#else",
	"#    define CP_MEMBER_REF(classPart,typePart) \\",
        "         INTEGER((((typePart)<<16)+(classPart)))",
	"#endif",
	"",
	"",

	"#ifdef _MSC_VER",
	"#define STATIC /* static */",
	"#else",
	"#define STATIC static",
	"#endif",

	"#define SCALARTYPE_NO(n) (n)",
	//"#define MSIG_NO(n) ((struct methodTypeTableEntry*)&CVMMethodSigTable.typeSig[n])",
	"#define NAME_NO(n) (n)",
	"#define SIG_DETAIL_NO(n) (&msig_details[n])",
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
	"#define STATIC_STORE_ADDRESS(n) (&CVMROMGlobals." + staticStoreName 
					     + "[CVM_STATIC_OFFSET(n)])",
	"",
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
	/*
	"extern CVMInt32 "+staticStoreName+"[];",
	*/
	/* 
	 * Added another const (for the pointer, which is also const
	 * here, not just the data) 
	 */
	"extern const CVMMethodBlock* const java_lang_Object_mt[];",
	"",

    };

    /* this is target-dependent and belongs in a header file... */
    private static String targetHeader[] = {
    };
	
    /* 
     * Added header file for globals
     */
    private void writeGlobalHeaderPrologue( BufferedPrintStream o ){
	o.println("/* This file contains all global and static variables that were");
	o.println(" * originally in the generated files.");
	o.println(" */");
	o.println("struct CVMROMGlobalState");
	o.println("{");
    }

    private void writeGlobalHeaderEpilogue( BufferedPrintStream o ){
	o.println("};");
	o.println("");
	o.println("extern struct CVMROMGlobalState CVMROMGlobals;");
    }

    private void writeHeaderPrologue( BufferedPrintStream o ){

	for ( int i = 0; i < targetHeader.length; i++ ){
	    o.println( targetHeader[i] );
	}
	for ( int i = 0; i < stdHeader.length; i++ ){
	    o.println( stdHeader[i] );
	}

	o.println("");
    }

    /* 
     * Moved global variables to CVMROMGlobals
     */
    private void writeHeaderEpilogue( BufferedPrintStream o ){
	o.println("");
	o.println("/* Moved global variables to CVMROMGlobals");
	o.println(" */");
	o.println("");
	o.println("#include \"" + globalHeaderFileName + "\"");
    }

    private void writePrologue( BufferedPrintStream o ){
	if ( headerFileName != null ){
	    o.println("#include \""+headerFileName+"\"");
	}
    }

    private static String cbName( CVMClass c ){
	return c.getNativeName()+"_Classblock.classclass";
    }

    private void writeClass( int classno, CVMClass c,
			     int clinitIdx,  int nStaticWords){

	classOut.println("\n/* ******** Class "+c.getNativeName()+" ******** */");

	classBlockName = cbName( c );

	writeMethods(c);
	// writeInterfaces(c);
	interfaceTableName = (c.inf.iv.length==0)?"0":("&"+c.inf.name+".interfaces");

	writeFields(c);

	if (c.isArrayClass()) {
	    ArrayClassInfo info = (ArrayClassInfo)c.classInfo;
	    constantPoolName = writeArrayInfo(info, c.getNativeName() );
	    constantPoolSize = 0;
	} else {
	    if ( doShared ) {
                ConstantPool cpool = c.classInfo.getConstantPool();

                int i = sharedConstantPools.indexOf(cpool);
		constantPoolName = sharedConstantPoolName + Integer.toString(i)+ "_cp";
		constantPoolSize = cpool.getLength();
	    } else {
		ConstantPool cpool = c.classInfo.getConstantPool();
		constantPoolName = writeConstants(cpool, c.getNativeName(),
						  false);
		constantPoolSize = cpool.getLength();
	    }
	}

	writeMethodTable(c);
	writeClassinfo(classno, c, clinitIdx, nStaticWords);

    }

    private void processStrings( ConstantPool cp ){
	int n = cp.getLength();
	for ( int i = 1; i < n; ){
	    ConstantObject obj = cp.elementAt(i);
	    if ( obj instanceof StringConstant ){
		stringTable.intern( (StringConstant) obj );
	    }
	    i += obj.nSlots;
	}
    }

    private void classProcessStrings( ClassInfo c, boolean doShared ){
	FieldInfo f[] = c.fields;
	int fieldCount = ( f== null ) ? 0 : f.length;
	if ( ! doShared ){
	    processStrings( c.getConstantPool() );
	}
	//
	// make sure that strings appearing ONLY as static final
	// initializer values get processed, too.
	//
	for ( int i = 0; i < fieldCount; i++ ){
	    FieldInfo fi = f[i];
	    if ( fi.isStaticMember() && fi.value instanceof StringConstant ){
		stringTable.intern( (StringConstant) (fi.value) );
	    }
	}
    }

    private void instantiateAllStrings(){
	njavastrings = stringTable.writeStrings( auxOut, masterStringArrayName);
	globalHeaderOut.println("    struct java_lang_String " +
                                stringArrayName + "[" + njavastrings + "];");
	auxOut.println("const int CVM_nROMStrings = "+njavastrings+";");
	auxOut.println("CVMArrayOfChar* const CVM_ROMStringsMasterDataPtr = " +
                       "(CVMArrayOfChar* const)&CVM_ROMStringsMaster_data.hdr;");

	/* 
	 * Moved global variables to CVMROMGlobals
	 * That makes it necessary to pass the global header out file into
         * writeStringInternTable.
         */
        stringTable.writeStringInternTable(auxOut, globalHeaderOut,
                                           "CVMInternTable", stringArrayName);
    }

    /*
     * Our representation of method types has four variations:
     *	0: inline form, inline details;
     *  1: inline form, outboard details;
     *  2: outboard form, inline details (rare);
     *  3: outboard form, outboard details, too.
     * For our own convenience, we are going to segregate them
     * by type. This affects the indexing scheme, so we must
     * assign that, too, after the sort.
     */
    private static int
    variety( CVMMethodType mt ){
	int v = 0;
	if ( mt.form.nSyllables> CVMSigForm.SYLLABLESPERDATUM) v+= 2;
	if ( mt.nDetails > 2 ) v += 1;
	return v;
    }

    // This must be called before enumerateMethodTypes() if 
    // there are unresolved CP entries.
    // Find NameAndType constants for unresolved MethodRefs
    // so they are added to MethodTypes.
    private void lookupUnresolvedMethodSignatures(ConstantPool cp) {
	ConstantObject constants[] = cp.getConstants();
	int clen = constants.length;
	for (int i = 1; i < clen; i += constants[i].nSlots) {
	    ConstantObject co = constants[i];
	    if (co.tag == CONSTANT_NAMEANDTYPE) {
		NameAndTypeConstant ntcon = (NameAndTypeConstant)co;
		int nameid = CVMMemberNameEntry.lookupEnter(getUTF(ntcon.name));
		String sigString = getUTF(ntcon.type);
		if  (sigString.charAt(0)==SIGC_METHOD) {
		    CVMMethodType mt = CVMMethodType.parseSignature(sigString);
		}
	    }
	}
    }

    private void
    enumerateMethodTypes(){
	methodTypes = new CVMMethodType[4];
	nMethodTypes = new int[4];
	Enumeration e = CVMMethodType.methodTypes.elements();
	while ( e.hasMoreElements() ){
	    CVMMethodType mt = (CVMMethodType)(e.nextElement() );
	    int v = variety( mt );
	    mt.listNext = methodTypes[v];
	    methodTypes[v] = mt;
	    nMethodTypes[v] += 1; }
	int curTypeNo = 0;
	for ( int i = 0; i < 4; i++ ){
            for (CVMMethodType mt = methodTypes[i]; mt!= null; mt = mt.listNext) {
		mt.entryNo = curTypeNo++;
	    }
	}
	totalMethodTypes = curTypeNo;
    }

    /*
     * The CVM type structures are built up as a side effect of
     * turning signatures and names into 16-bit cookies while writing out
     * other structures. Thus they must be written AFTER all the entries are
     * computed. Fortunately there will be no forward-references to them,
     * as all references are by index values, not addresses.
     */
    private void writeCVMNameAndTypeTables(){
	writeCVMpkgHeader();
	writeCVMDataTypeTable();
	writeCVMpkg();
	writeCVMMethodTypeTable();
	writeCVMMemberNameTable();
    }

    /*
     * The value-type table is one of the larger type structures, as it also
     * holds class names. It has variant records. It contains an internal linked
     * list for lookup-by-name. It has reference counts, but we will set these
     * to the magic maximum value so that this can be READONLY.
     */
    private void writeCVMDataTypeTable(){
	Vector allDataTypes = CVMDataType.dataTypes; 
	int    nData = allDataTypes.size();
	/*
	 * May need to do an iteration through to determine the layout, in case
	 * we have some trouble initializing union types and have to do more
	 * specific declarations!
	 */
	/* 
	 * Moved global variables to CVMROMGlobals
         */
	globalHeaderOut.println("    struct scalarTableSegment * dummyNextCell;");
        initInfo.addInfo("NULL", "&CVMROMGlobals.dummyNextCell",
                         "sizeof(CVMROMGlobals.dummyNextCell)");
        auxOut.println("const struct { SCALAR_TABLE_SEGMENT_HEADER " +
                       "struct scalarTableEntry data["  +nData +
                       "]; }\nCVMFieldTypeTable = {");
        auxOut.println("\t" + (CVMTypeCode.CVMtypeLastScalar+1) + ", 0, -1," +
                       nData + ", &CVMROMGlobals.dummyNextCell, {");
        for (int i = 0; i < nData; i++) {
	    CVMDataType o = (CVMDataType)allDataTypes.elementAt( i );
	    /* Yet more debugging output ....
		if ( o != null && i != o.entryNo ){
		    System.out.println("Datatype entry " + o +
                                       " misplaced: is at vector[" + i +
                                       "] and expected at [" + o.entryNo + "]");
		}
	    */
	    if ( o instanceof CVMClassDataType ){
		CVMClassDataType ctype = (CVMClassDataType) o;
		String next;
                next = (ctype.next == null ) ? "TYPEID_NOENTRY" :
                           ("SCALARTYPE_NO(" + ctype.next.typeNo + ")");
		// int nameHash = CVMDataType.computeHash(ctype.classInPackage);
		int nameLength = ctype.classInPackage.length();
		if (nameLength > 255 )
		    nameLength = 255;
		auxOut.println("	INIT_CLASSTYPE( "+next+
		    ", PKG_NO("+ctype.pkg.entryNo+
		    "), \""+ctype.classInPackage+"\", "+
		    nameLength+"),");
	    } else {
		CVMArrayDataType atype = (CVMArrayDataType) o;
		String next;
		next = (atype.next == null ) ? "TYPEID_NOENTRY" :
                           ("SCALARTYPE_NO(" + atype.next.typeNo + ")");
		auxOut.println("	INIT_ARRAYTYPE( " + next + ", " +
                               atype.depth + ", " + atype.baseType + "),");
	    }
	}
	auxOut.println("}};");

    }

    /*
     * The package name data structures and hash tables have to go after the
     * general value-type table, as it contains little hash tables 
     * of pointers to the latter.
     * These are used for lookup by class name.
     */
    private void writeCVMpkgHeader(){
	int    nPkg = CVMpkg.pkgVector.size();
	/* 
	 * Moved global variables to CVMROMGlobals
         */
	globalHeaderOut.println("    struct pkg CVM_ROMpackages["+(nPkg)+"];" );
	auxOut.println("#define PKG_NO(n) &CVMROMGlobals.CVM_ROMpackages[n]");
        auxOut.println("const int CVM_nROMpackages = "+nPkg+";");
    }

    private void writeCVMpkg(){
	Vector allPackages = CVMpkg.pkgVector; 
	int    nPkg = allPackages.size();

	auxOut.println("STATIC const struct pkg CVM_ROMpackagesMaster[" + (nPkg) +
                       "] = {");
	for ( int i= 0; i < nPkg; i++ ){
	    CVMpkg p = (CVMpkg) (allPackages.elementAt(i));
	    writeAnCVMpkg( p );
	    /* DEBUG
		if ( p.entryNo != i ){
		    System.out.println("Package entry " + p.pkgName +
                                       " misplaced: is at vector[" + i +
                                       "] and expected at [" + p.entryNo + "]");
		}
	    */
	}
	auxOut.println("};");

	initInfo.addInfo("CVM_ROMpackagesMaster",
                         "CVMROMGlobals.CVM_ROMpackages",
                         nPkg + "*sizeof(struct pkg)");

	// that was the packages themselves.
	// this is the hash table that is the basis of looking them up.
	// this needs to be writable, since we add at the front of the
	// linked list.
	auxOut.print("STATIC const struct pkg * const CVM_PkgHashtableMaster[" +
                     CVMpkg.NPACKAGEHASH + "] = {");
	for (int i = 0; i < CVMpkg.NPACKAGEHASH; i++) {
	    CVMpkg p = CVMpkg.packages[i];
	    if ( i%5 == 0 ){
		auxOut.print("\n\t");
	    }
	    auxOut.print( (p==null)?"0, ":("PKG_NO("+(p.entryNo)+"), "));
	}
	auxOut.println("\n};");
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
	globalHeaderOut.println("    struct pkg *CVM_pkgHashtable[" +
                                CVMpkg.NPACKAGEHASH + "];");
	initInfo.addInfo("CVM_PkgHashtableMaster",
                         "CVMROMGlobals.CVM_pkgHashtable",
                         CVMpkg.NPACKAGEHASH + "*sizeof(struct pkg*)");

	/* 
	 * Made this const (never changed)
	 */
	auxOut.println("struct pkg * const CVMnullPackage = PKG_NO(0);");
    }

    private void writeAnCVMpkg( CVMpkg p ){
	String next = (p.next==null)?"0" : ("PKG_NO("+p.next.entryNo+")");
	//int    nameHash = CVMDataType.computeHash(p.pkgName);
	auxOut.print("    { \""+p.pkgName+"\", "+next+", INROM, MAX_COUNT, {");
	for( int i = 0; i < CVMpkg.NCLASSHASH; i++ ){
	    String hashp = (p.typeData[i]==null)? "TYPEID_NOENTRY" :
                               ("SCALARTYPE_NO(" + p.typeData[i].typeNo + ")");
	    if ( i%3==0){
		auxOut.print("\n\t");
	    }
	    auxOut.print(hashp);
	    auxOut.print(", ");
	}
	auxOut.println("\n    }},");
    }

    /*
     * The CVMMethodTypes are complex, multi-part beasts.
     * The first part is the Form or Terse Signature,
     * Then we need the details array
     * And finally the table of method types. There are 4 kinds of them!:
     *	0: inline form, inline details;
     *  1: inline form, outboard details;
     *  2: outboard form, inline details (rare);
     *  3: outboard form, outboard details, too.
     * They are all the same size!
     */
    public void writeCVMMethodTypeTable(){
	// The first part is the Form or Terse Signature.
	writeCVMForms();

	// then the details array.
	writeCVMMethodDetails();

	// write the indirect dummy.
	/* 
	 * Moved global variables to CVMROMGlobals
         */
        globalHeaderOut.println(
            "    struct methodTypeTableSegment *methodTypeDummy;");
        initInfo.addInfo("NULL", "&CVMROMGlobals.methodTypeDummy",
                         "sizeof(CVMROMGlobals.methodTypeDummy)");

	// write the specific declaration we need for this
	// set of types.
	/* 
	 * CVMMethodTypeTable is a structure that holds 
	 * arrays of struct methodTypeTableEntry (see typeid_impl.h)
	 * which are initialized in different ways because 
	 * struct methodTypeTableEntry contains two unions.
	 * 
	 * Unfortunately the original implementation does only
	 * fit for 32 bit platforms (because all four
	 * struct declarations have the same byte size;
	 * unsigned int formdata is as long as struct sigForm *
	 * and unsigned short data[2] is as long as short* datap).
	 * But that does not fit for 64 bit platforms. Therefore
	 * the struct declaration from typeid_impl.h is used to be
	 * sure the initialization is also correct for other platforms.
	 */
	auxOut.println("const struct { METHOD_TABLE_SEGMENT_HEADER");
	for ( int i = 0; i < 4; i++ ){
	    if ( nMethodTypes[i] == 0 ) continue;
	    auxOut.print("    struct { METHOD_TYPE_HEADER ");
	    if ( (i&2) == 0 ){
		// inline form
		// This used to be auxOut.print("unsigned int formdata; ");
		auxOut.print("union {CVMUint32 formdata; " +
                                     "struct sigForm * formp;} form; ");
	    } else {
		// outline form
		// This used to be auxOut.print("struct sigForm * formp; ");
		auxOut.print("union {struct sigForm * formp; " +
                                    "CVMUint32 formdata;} form; ");
	    }
	    if ( (i&1) == 0 ){
		// inline details
		// This used to be auxOut.print("unsigned short data[2]; ");
		auxOut.print("union {CVMTypeIDTypePart data[2]; " +
                                    "const CVMTypeIDTypePart * datap; } " +
                             "details; ");
	    } else {
		// outline details
		// This used to be auxOut.print("const short* datap; ");
		auxOut.print("union {const CVMTypeIDTypePart * datap; " +
                                    "CVMTypeIDTypePart data[2]; } details; ");
	    }
	    auxOut.print(" }typesig");
	    auxOut.println( i+"["+nMethodTypes[i]+"];");
	}
	auxOut.println("} CVMMethodTypeTable = {");

	// now the header info
	/* 
	 * Moved global variables to CVMROMGlobals
         */
	auxOut.println("    0, 0, -1, " + totalMethodTypes +
                       ", &CVMROMGlobals.methodTypeDummy,");

	// and the actual data.
        for (int i = 0; i < 4; i++) {
            if (nMethodTypes[i] == 0) continue;
	    auxOut.println("    {");
	    for (CVMMethodType m = methodTypes[i]; m != null; m = m.listNext) {
                String next = (m.next==null) ? "TYPEID_NOENTRY" :
                                               String.valueOf(m.next.entryNo);
                auxOut.print("	{" + next + ", MAX_COUNT, ROMSTATE " +
                             (m.form.nSyllables-2));
		CVMSigForm f = m.form;
		if ( (i&2) == 0 ){
		    // inline form
		    // This used to be
                    //     auxOut.print(", 0x"+Integer.toHexString(f.data[0]));
		    auxOut.print(", {0x"+Integer.toHexString( f.data[0] )+"}");
		} else {
		    // outline form
		    // This used to be
                    //     auxOut.print(", (struct sigForm*)" +
                    //                  "(&CVM_ROMforms.form" + f.listNo +
                    //                  "[" + f.entryNo + "])");
                    auxOut.print(", {(struct sigForm*)(&CVM_ROMforms.form" +
                                 f.listNo + "[" + f.entryNo + "])}");
		}
		if ( (i&1) == 0 ){
		    // inline details
		    auxOut.print(", {{");
		    for (int j=0; j<2; j++) {
			auxOut.print("RAW_TYPEID_TYPE_PART(0x");
			auxOut.print(Integer.toHexString((j < m.nDetails) ?
                                                         m.details[j] : 0));
			auxOut.print("),");
		    }
		    auxOut.print("}}");
		} else {
		    // outline details
		    auxOut.print(", {SIG_DETAIL_NO("+(m.detailOffset)+")}");
		}
		auxOut.println("},");
	    }
	    auxOut.println("    },");
	}
	auxOut.println("};");

	/*
	 * A signature search begins with a hash.
	 * The hash table must be writeable.
	 */
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	globalHeaderOut.println(
            "    CVMTypeIDTypePart CVMMethodTypeHash[NMETHODTYPEHASH];");
	/* 
	 * Added extra precautions to ensure we use the same hash function.
	 */
	auxOut.println("#if NMETHODTYPEHASH != " +  CVMMethodType.NHASH);
	auxOut.println("#error \"CVMMethodType.NHASH and NMETHODTYPEHASH " +
                       "are not the same\"");
	auxOut.println("#endif");

	auxOut.print("STATIC const CVMTypeIDTypePart CVMMethodTypeHashMaster" +
                     "[NMETHODTYPEHASH]={");

	for ( int i = 0 ; i < CVMMethodType.NHASH; i++ ){
	    CVMMethodType mt = CVMMethodType.hashTable[i];
	    if ( i%4 == 0 ){
		auxOut.print("\n	");
	    }
	    if ( mt == null ){
		auxOut.print("TYPEID_NOENTRY, ");
	    } else {
		auxOut.print( mt.entryNo );
		auxOut.print(", ");
	    }
	}
	auxOut.println("};");
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	initInfo.addInfo("CVMMethodTypeHashMaster",
                         "CVMROMGlobals.CVMMethodTypeHash",
                         CVMMethodType.NHASH +
                             "*sizeof(CVMROMGlobals.CVMMethodTypeHash[0])");
    }

    /*
     * Method signature varieties 1 & 3 have details written in an
     * array-of-short.
     */
    private void
    writeCVMMethodDetails( ){
	if (nMethodTypes[1]+nMethodTypes[3] == 0 ) return; // nothing to do.
	// We could search for shared sub-sequences of detail sequences.
	// On the other hand, these so seldom occur at all
	// that it would make very little difference.
	int curOffset = 0;
	auxOut.print("STATIC const CVMTypeIDTypePart msig_details[] = {");
	for ( int i = 1; i<4; i+= 2 ){ // i.e. for i in {1,3}
	    for (CVMMethodType m = methodTypes[i]; m!=null; m = m.listNext) {
		m.detailOffset = curOffset;
		int ndetail = m.nDetails;
		int details[] = m.details;
		for ( int j = 0; j < ndetail; j++ ){
		    if ( curOffset%6==0){
			auxOut.print("\n	");
		    }
		    auxOut.print("RAW_TYPEID_TYPE_PART(0x" +
                                 Integer.toHexString(details[j]) + ")");
		    auxOut.write(',');
		    auxOut.write(' ');
		    curOffset+= 1;
		}
	    }
	}
	auxOut.println("\n};");
    }


    private void writeCVMForms(){
	/*
	 * Declare a data structure to contain all the "big" forms.
	 * Since they are of varying lengths, this will require a
	 * custom declaration.
	 */
	auxOut.println("STATIC const struct {");
	for (int i = 1; i < CVMSigForm.listLength.length; i++) {
	    if (CVMSigForm.listLength[i] == 0) continue; // never mind...
	    /* 
	     * Added symbol DUMMY (which is never used, but in VC++ on Win32
             * the preprocessor barks if it doesn't get an argument here)
	     */
            auxOut.println("	DECLARE_SIG_FORM_N( DUMMY" + i +
                           " /* never used */, " + (i+1) + ") form" + i +
                           "[" + CVMSigForm.listLength[i] + "];");
	}
	auxOut.println("} CVM_ROMforms = {");
	/*
	 * Now supply the data. They're all on a single linked list,
	 * which we construct (last-to-first) as we go.
	 */
	String link = "0";
	for (int i = 1; i < CVMSigForm.listLength.length; i++) {
	    int n = CVMSigForm.listLength[i];
	    if ( n == 0 ) continue; // never mind...
	    String segname = "&CVM_ROMforms.form"+i;
	    auxOut.println("    {");
	    int fno = 0;
            CVMSigForm f;
            for (f = CVMSigForm.formList[i]; f!= null; f = f.listNext) {
		auxOut.print("	");
                auxOut.print("{ (struct sigForm*)" + link +
                             ", MAX_COUNT, ROMSTATE ");
                auxOut.print((f.nSyllables-2) + ", { ");
		int nWordsLess1 = f.data.length-1;
		for ( int k=0; k<=nWordsLess1; k++ ){
		    auxOut.printHexInt( f.data[k] );
		    if ( k<nWordsLess1)
			auxOut.print(", ");
		}
		auxOut.println("} },");
		//f.listNo  = i;
		f.entryNo = fno;
		link = segname+"["+(fno++)+"]";
	    }
	    auxOut.println("    },");
	}
	auxOut.println("};");

	/*
	 * Finally, the root of the form list.
	 */
	/* 
	 * Moved globals to CVMROMGlobals.
	 */
        globalHeaderOut.println("    struct sigForm *	CVMformTable;");
        auxOut.println("STATIC const struct sigForm * const " +
                       "CVMformTableMaster = (struct sigForm *)" + link + ";");
        initInfo.addInfo("&CVMformTableMaster", "&CVMROMGlobals.CVMformTable",
                         "sizeof(CVMformTableMaster)");
    }

    private void
    writeCVMMemberNameTable(){
	int ndata = CVMMemberNameEntry.tableSize();
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
        globalHeaderOut.println(
            "    struct memberNameTableSegment * dummyNextMembernameCell;");
        initInfo.addInfo("NULL", "&CVMROMGlobals.dummyNextMembernameCell",
                         "sizeof(CVMROMGlobals.dummyNextMembernameCell)");
        auxOut.println("const struct { MEMBER_NAME_TABLE_SEGMENT_HEADER " +
                       "struct memberName data[" + ndata +
                       "]; }\nCVMMemberNames = {" );
        auxOut.println("    0, 0, -1, " + ndata +
                       ", &CVMROMGlobals.dummyNextMembernameCell, {");
	for ( int i = 0; i < ndata; i++ ){
            CVMMemberNameEntry mne =
                (CVMMemberNameEntry)(CVMMemberNameEntry.table.elementAt(i));
            String next = (mne.next == null) ?
                "TYPEID_NOENTRY" : ("NAME_NO(" + mne.next.entryNo + ")");
	    auxOut.println("\t{" + next + ", MAX_COUNT, ROMSTATE \"" +
                           mne.name + "\"},");
	}
	auxOut.println("}};");


	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	globalHeaderOut.println(
            "    CVMTypeIDNamePart CVMMemberNameHash[NMEMBERNAMEHASH];");

	auxOut.print("STATIC const CVMTypeIDNamePart " +
                     "CVMMemberNameHashMaster[NMEMBERNAMEHASH]={");
	for ( int i = 0 ; i < CVMMemberNameEntry.NMEMBERNAMEHASH; i++ ){
	    CVMMemberNameEntry mne = CVMMemberNameEntry.hash[i];
	    if ( i%4 == 0 ){
		auxOut.print("\n	");
	    }
	    if ( mne == null ){
		auxOut.print("TYPEID_NOENTRY, ");
	    } else {
		auxOut.print( mne.entryNo );
		auxOut.print(", ");
	    }
	}
	auxOut.println("};");
	/* 
	 * Moved global variables to CVMROMGlobals
	 */
	initInfo.addInfo("CVMMemberNameHashMaster",
                         "CVMROMGlobals.CVMMemberNameHash",
                         CVMMemberNameEntry.NMEMBERNAMEHASH +
                         "*sizeof(CVMROMGlobals.CVMMemberNameHash[0])");
    }

    /* Write the invokeCost array for romized methods. */
    private void writeMethodInvokeCostArray() {
        globalHeaderOut.println("#if defined(CVM_JIT) && defined(CVM_MTASK)");
        globalHeaderOut.println("    CVMUint16 CVMROMMethodInvokeCosts["+
                                nTotalMethods+"];");
        globalHeaderOut.println("#endif");
        auxOut.println("#if defined(CVM_JIT) && defined(CVM_MTASK)");
        auxOut.println("const int CVM_nROMMethods = " + nTotalMethods + ";");
        auxOut.println("#endif");
    }

    class ClassTable{
	String  name;
	int	nClasses;
	CVMClass classVector[];
	Vector  classes;
        boolean foundFirstSingleDimensionArrayClass;
        boolean foundLastSingleDimensionArrayClass;
        boolean foundFirstNonPrimitiveClass;
        int	firstSingleDimensionArrayClass;
        int	lastSingleDimensionArrayClass;
	int	firstNonPrimitiveClass;

	ClassTable( int nMax, String nm ){
	    name = nm;
	    nClasses = 0;
            foundFirstSingleDimensionArrayClass = false;
            foundLastSingleDimensionArrayClass = false;
	    classes = new Vector(nMax);
	}

        /**
         * Called by CVMWriter.writeClassList() to add a presorted list of
         * classes into this ClassTable.  The list of classes are expected to
         * have already been sorted as follows:
         *
         *    .---------------------------------------.
         *    |  Primtive classes                     |
         *    |---------------------------------------|
         *    |  Regular Loaded classes               | <= e1
         *    |  e.g. java.lang.Object                |
         *    |                                       |
         *    |  Big Array classes                    |
         *    |  i.e. depth exceeds or equals the     |
         *    |  the value of CVMtypeArrayMask.       |
         *    |
         *    |---------------------------------------|
         *    |  Single Dimension Array classes       | <= e2
         *    |  e.g. [I or [Ljava.lang.Object;       |
         *    |                                       | <= e3
         *    |---------------------------------------|
         *    |  Multi Dimension Array classes        |
         *    |  e.g. [[I or [[Ljava.lang.Object; or  |
         *    |       [[[I, etc.                      |
         *    |                                       |
         *    '---------------------------------------'
         *
         * In the above illustration, entry e1 is the first entry in the
         * Regular Loaded classes section.  Entry e2 is the first entry in the
         * Single Dimension Array classes section, and entry e3 is the
         * last entry in this same section.
         *
         * The Regular Loaded classes section currectly also holds the Big
         * Array classes whose dimension depth exceeds or equals the bits
         * in CVMtypeArrayMask (currently 3).  In the current implemetation
         * the Big Array classes always come at the end of this section after
         * the Regular Loaded classes.
         *
         * We want firstNonPrimitiveClass to be the index of entry e1,
         * firstSingleDimensionArrayClass to be the index of entry e2, and
         * lastSingleDimensionArrayClass to be the index of entry e3. 
         *
         * ClassTable.addClass() will keep a look out for entries e1, e2, and
         * e3 and set firstNonPrimitiveClass, firstSingleDimensionArrayClass,
         * and lastSingleDimensionArrayClass accordingly.
         */
        void addClass(CVMClass c) {
	    int hashCode = 0;
            int classid = c.getClassId();

            /* If we haven't found the first non-primitive class yet and
               this class is not a primitive class, then set the
               firstNonPrimitiveClass index: */
            if ((!foundFirstNonPrimitiveClass) & !c.isPrimitiveClass()) {
                firstNonPrimitiveClass = nClasses;
                foundFirstNonPrimitiveClass = true;
	    }

            /* In the following we'll be looking for the first and the last
               single dimension array class.  To do that, it will check for
               the first single dimension array class and the first array
               class with a dimension greater than 1.  The last single
               dimension array class will be the one before the first array
               class with a dimension greater than 1.

               Since Big Array classes are kept in the regular loaded
               classes section which preceeds the single dimension array
               classes, we should ignore them.
            */
            int classid_depth = classid & CVMTypeCode.CVMtypeArrayMask;
            if ((classid_depth != 0) && 
                (classid_depth != CVMTypeCode.CVMtypeBigArray)) {

                // Look for the first single dimension array class:
                if (!foundFirstSingleDimensionArrayClass) {
                    firstSingleDimensionArrayClass = nClasses;
                    foundFirstSingleDimensionArrayClass = true;
		}
                // Look for the last single dimension array class:
                if (((classid>>CVMTypeCode.CVMtypeArrayShift) > 1)
                    && !foundLastSingleDimensionArrayClass) {
                    lastSingleDimensionArrayClass = nClasses-1;
                    foundLastSingleDimensionArrayClass = true;
		}
	    }
	    if (classid_depth == 0) {
		int tid = classid - CVMTypeCode.CVMtypeLastScalar - 1;
		while (tid > nClasses) {
		    ++nClasses;
		    classes.add(null);
		}
	    }
	    ++nClasses;
	    classes.add(c);
	}

        /**
         * Writes out miscellaneous info about the classes that were just listed
         * by ClassTable.writeClassList().  It is expected that the values of
         * firstArrayClass, firstSingleDimensionArrayClass, and
         * lastSingleDimensionArrayClass would have been determined (if
         * applicable) by previous calls to ClassTable.addClass() iterated over
         * the presorted list of classes.
         *
         * See the comments for ClassTable.addClass() above for details on the
         * expected sorting order of the list of classes.
         */
        void writeClassListInfo() {
            if (!foundFirstSingleDimensionArrayClass) {
		// If, by now, we've not found the first single dimension array
                //     class, then there is no preloaded array type.  Point the
                //     firstSingleDimensionArrayClass index to the end of the
                //     list of classes:
                firstSingleDimensionArrayClass = nClasses;
                foundFirstSingleDimensionArrayClass = true;
                lastSingleDimensionArrayClass = nClasses;
                foundLastSingleDimensionArrayClass = true;

            } else if (!foundLastSingleDimensionArrayClass) {
                // If we get here, then we must have found the first single
                //     dimension array class but have not found the last one.
                //     This means that there are array types of depth 1 in the
                //     list but no array types of depth greater than 1.  Point
                //     the lastSingleDimensionArrayClass index to the last
                //     entry in the list:
                lastSingleDimensionArrayClass = nClasses - 1;
                foundLastSingleDimensionArrayClass = true;
	    }
	    auxOut.println("const int CVM_n" + name + "ROMClasses = " +
                           String.valueOf(nClasses) + ";");
            auxOut.print("const int CVM_"+name+
                         "firstROMSingleDimensionArrayClass = ");
            auxOut.print(firstSingleDimensionArrayClass);
            auxOut.print(";\nconst int CVM_"+name+
                         "lastROMSingleDimensionArrayClass = ");
            auxOut.print(lastSingleDimensionArrayClass);
	    auxOut.println(";");
	    auxOut.print("const int CVM_"+name+"firstROMNonPrimitiveClass = ");
            auxOut.print(firstNonPrimitiveClass);
	    auxOut.println(";");
	}

        void writeClassList() {
	    classVector = new CVMClass[nClasses];
	    classes.copyInto(classVector);

	    int n = nClasses;
	    CVMClass classes[] = classVector;
	    //auxOut.println("const CVMClassBlock * const CVM_" + name +
            //               "ROMClassBlocks[] = {");
	    for (int i=0; i<n; i++){
		CVMClass c = classVector[i];
		if (c == null) {
		    auxOut.println("(const CVMClassBlock *)0,");
		} else {
		    auxOut.print("    &");
		    auxOut.print(cbName(c));
		    auxOut.println(",");
		}
	    }
	    //auxOut.println("};");
	}

    }

    public void writeClassList() {
        int n = (classes == null) ? 0 : classes.length;
	ClassTable classTable = new ClassTable(n, "");
	globalHeaderOut.println(
	    "    struct java_lang_Class CVM_ROMClassBlocks["
	    +String.valueOf(n)+"];");
	auxOut.println("const int CVM_nTotalROMClasses = " +
                       String.valueOf(n) + ";");
	/* 
	 * Moved globals to CVMROMGlobals
	 */
        globalHeaderOut.println("    struct java_lang_Class CVM_ROMClasses[" +
                                n + "];");
        for (int i = 0; i < n; i++) {
	    int hashCode = 0;
	    CVMClass c = classes[i];
	    int classid = c.getClassId();
	    classTable.addClass(c);
	}

        auxOut.println("const CVMClassBlock * const CVM_ROMClassblocks[] = {");
        classTable.writeClassList();
	auxOut.println("};" );
        classTable.writeClassListInfo();
	 
	//
	// array of stack map indirect cells
	//
	/* 
	 * Moved global variables to CVMROMGlobals
         */
        declare("const CVMClassICell CVM_classICell[]", auxOut);
	auxOut.println(" = {");
        for (int i = 0; i < n; i++) {
            auxOut.println("    { (CVMObject*)(&CVMROMGlobals.CVM_ROMClasses[" +
                           i + "])},");
	}
	auxOut.println("};");
    }
    
    public boolean writeClasses(Vector sharedCPs, boolean doWrite){
        int i;

	// getClassVector sorts the classes with
	// super before sub, for reasonable processing without recursion.
	ClassClass arrayOfClasses[] = ClassClass.getClassVector( classMaker );
	ClassClass.setTypes();
	int nClasses = arrayOfClasses.length;
        ConstantPool sharedconsts;

	classes = new CVMClass[nClasses];

	if (sharedCPs != null) {
	    doShared = true;
            sharedConstantPools = sharedCPs;
	    sharedConstantPoolName = "CVMSharedConstantPool";

            for (i = 0; i < sharedCPs.size(); i++) {
                int cpSize;
                sharedconsts = (ConstantPool)sharedCPs.get(i);
	        cpSize = sharedconsts.getLength();
	        if (cpSize > 0xffff) {
		    // More than 64K constants are not allowed
		    throw new Error("Constant pool overflow: 64k constants"+
				" allowed, have "+cpSize);
	        }
           }
	}
	
	/*
	 * sort the array by classid. This makes it easier do lookups
	 * at runtime.
	 * The mechanics of this whole writer assume a single
	 * ordering for these classes, as they are often
	 * referred to by their class number.
	 * This is it.
	 * To avoid finding the primitive classes at all, they are
	 * placed at the very end.
	 */
	System.arraycopy( arrayOfClasses, 0, classes, 0, nClasses );
        Arrays.sort(classes, (java.util.Comparator)classMaker);

        // after all classes processed once by classMaker
        // ... but before we write the classes.
	if (doShared) {
            for (i = 0; i < sharedCPs.size(); i++) {
                sharedconsts = (ConstantPool)sharedCPs.get(i);
	        lookupUnresolvedMethodSignatures(sharedconsts);
            }
	} else {
	    for (i = 0; i < nClasses; ++i) {
		ConstantPool cp = classes[i].classInfo.getConstantPool();
		lookupUnresolvedMethodSignatures(cp);
	    }
	}
	enumerateMethodTypes();
	
	// write out some constant pool stuff here,
	// if we're doing one big shared one...
	// gutted out for now...
	if ( doShared ) {
            for (i = 0; i < sharedCPs.size(); i++) {
                sharedconsts = (ConstantPool)sharedCPs.get(i);
                processStrings( sharedconsts );
            }
        }

	if (verbose && doWrite) {
	    System.out.println(Localizer.getString("cwriter.writing_classes"));
	}
	try {
	    //mungeAllIDsAndWriteExternals(classes);
	    for ( i = 0; i < nClasses; i++ ){
		classProcessStrings(classes[i].classInfo, doShared);
	    }

	    if (doWrite) {
		instantiateAllStrings();
		CVMInterfaceMethodTable.writeInterfaceTables(classes,
		    auxOut, headerOut);
		int nStaticWords = writeStaticStore( classes );

		/* The number of necessary slots for clinitEE + 1.
		 * Index of a class that doesn't have <clinit> is always 0.
		 * The 0th entry is reserved for it. */
		int	maxClinitIdx = 1;
		/*
		 * First off, check compression opportunities. Look over all
		 * methods to see variations
		 */
		for ( i = 0; i < nClasses; i++ ){
		    CVMClass c = classes[i];
		    analyzeMethodsForCompression( c );
		}
		//
		// Sort in order of decreasing refcount
		//
		methStats.sort();
		//
		// This is shared between our segments
		//
		methStats.dump(auxOut);
		/*
		 * Now do the writing 
		 */
		for ( i = 0; i < nClasses; i++ ){
		    CVMClass c = classes[i];
		    int clinitIdx = c.hasStaticInitializer ? maxClinitIdx++ : 0;
		    writeClass( i, c, clinitIdx, nStaticWords );
		    if ( segmentedOutput && ( curClasses++ >= maxClasses) ){
			curClasses = 0;
			openNextClassFile();
		    }
		}

		if (doShared) {
		    // Dump the shared constant pool
                    for (i = 0; i < sharedCPs.size(); i++) {
                        String cpName = sharedConstantPoolName +
                                            Integer.toString(i);
                        sharedconsts = (ConstantPool)sharedCPs.get(i);
		        writeConstants(sharedconsts, 
				   cpName,
				   true /* export c.p. ref */);
                    }
		}

		writeClassList();

		writeCVMNameAndTypeTables();

		/* Write method invokeCost array */
		writeMethodInvokeCostArray();

		initInfo.write( auxOut, "struct CVM_preloaderInitTriple",
				   "CVM_preloaderInitMap");
		initInfoForMBs.write( auxOut, "void* const",
				   "CVM_preloaderInitMapForMbs");

		auxOut.println("const char *CVMROMClassLoaderNames = \"" +
		    components.ClassTable.getClassLoaderNames() + "\";");
		auxOut.println("CVMAddr * const CVMROMClassLoaderRefs = " +
		    "STATIC_STORE_ADDRESS(" + clRefOff + ");");
		auxOut.println("const int CVMROMNumClassLoaders = " +
		    components.ClassTable.getNumClassLoaders() + ";");
	    }
	} catch ( java.io.IOException e ){
	    failureMode = e;
	    formatError = true;
	} finally { 
	    if (doWrite) {
		classOut.flush();
		headerOut.flush();
	    }
	}
	return (!formatError) &&
               (!doWrite ||
                (!classOut.checkError() && !headerOut.checkError()));
    }

    public void printSpaceStats(java.io.PrintStream o) {
        //ClassClass classes[] = ClassClass.getClassVector(classMaker);
        o.println("\t" + Localizer.getString("cwriter.total_classes",
                                             Integer.toString(classes.length)));
        o.println("\t\t... of which " + nClinits +
                  " classes have static initializers ");
        o.println("\t\t    (" + CodeHacker.checkinitQuickenings + "/" +
                  CodeHacker.quickenings + " quickening sites)");
        o.println("\t\t" + Localizer.getString("cwriter.method_blocks",
                                               Integer.toString(nmethods)));
        if (nWritableMethodBlocks != 0) {
            o.println("\t\t... of which " + nWritableMethodBlocks +
                      " blocks are writable" );
            o.println("\t\t... for " + nClassesWithWritableMethodBlocks +
                      " classes");
        }
        if (nMethodsWithCheckinitInvokes > 0) {
            o.println("\t\t... of which " + nMethodsWithCheckinitInvokes +
                      " have checkinit opcodes in the code");
        }
        o.println("\t\t" + Localizer.getString("cwriter.bytes_java_code",
                                               Integer.toString(ncodebytes)));
        o.println("\t\t" + Localizer.getString("cwriter.catch_frames",
                                               Integer.toString(ncatchframes)));
        o.println("\t\t" + Localizer.getString("cwriter.field_blocks",
                                               Integer.toString(nfields)));
        o.println("\t\t" + Localizer.getString("cwriter.inner_classes",
                                             Integer.toString(ninnerclasses)));
        o.println("\t\t" + Localizer.getString("cwriter.constant_pool_entries",
                                               Integer.toString(nconstants)));
        o.println("\t\t" + Localizer.getString("cwriter.java_strings",
                                               Integer.toString(njavastrings)));
    }

}
