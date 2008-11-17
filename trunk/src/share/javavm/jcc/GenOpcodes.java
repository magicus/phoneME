/*
 * @(#)GenOpcodes.java	1.36 06/10/10
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

/*
 * GenOpcodes is a program for reading the file
 * src/share/javavm/include/opcodes.list and deriving from it "source"
 * files for building a Java Virtual Machine and associated tools.
 *
 * The format of opcodes.list is explained in that
 * file, which I quote here:
 *
 * #
 * # Any line that doesn't have a-z in the 1st column is a comment.
 * #
 * # The first column is the name of the opcode.
 * #
 * # The second column is the opcode's number, or "-" to assign the
 * # next available sequential number.
 * #
 * # The third column is the total length of the instruction.  We use 0
 * # for opcodes of variable length, as well as unrecognized/unused
 * # opcodes.
 * #
 * # The fourth and fifth column give what the opcode pops off the stack, and
 * # what it then pushes back onto the stack
 * #    -       <no effect on stack>   
 * #    I       integer
 * #    L       long integer
 * #    F       float
 * #    D       double float
 * #    A       address [array or object]
 * #    O       object only
 * #    R       return address (for jsr)
 * #    a       integer, array, or object
 * #    ?       unknown
 * #    [I], [L], [F], [D], [A], [B], [C], [?]
 * #            array of integer, long, float, double, address, bytes, 
 * #                  chars, or anything
 * #    1,2,3,4,+ used by stack duplicating/popping routines.  
 * # 
 * # 1,2,3,4 represent >>any<< stack type except long or double.  Two numbers
 * # separated by a + (in the third column) indicate that the two, together, can
 * # be used for a double or long.  (Or they can represent two non-long items).
 * #
 * # The sixth column has a comma-separated list of attributes of the
 * # opcode. These are necessary in the stackmap computation dataflow 
 * # analysis.
 * #
 * # GC    -- a GC point
 * # CGC   -- a conditional GC point; only if the thread is at a quickening point
 * # BR    -- a branch
 * # EXC   -- May throw exception
 * # INV   -- A method invocation
 * # NFLW  -- An instruction that doesn't let control flow through
 * #           (returns, athrow, switches, goto, ret)
 * # QUICK -- A quick instruction, re-written by the interpreter.
 * # RET   -- A return opcode.
 * # FP    -- A floating point opcode
 * # -     -- No special attributes to speak of
 * #            
 * # The seventh column is a "simplification" of the opcode, for opcode
 * # sequence measurements (see CVM_INSTRUCTION_COUNTING in executejava.c).
 * #
 * # The eigth column has the attribute of CVMJITIROpcodeTag.
 * # The ninth column has the attribute of type tag listed in typeid.h.
 * # The tenth column has the attribute of value representing constant value,
 * # local variable number, etc.
 * 
 * The general flow of this program is:
 * - parse command-line arguments and instantiate output file writers
 * - read input file. For each non-commentary line of input
 * 	- parse the line into words
 * 	- create an Opcode with the line's information and save it
 *        in an ordered list of Opcodes.
 * - call each writer to write the Opcodes to its file.
 * 
 * The output file writers are all implementations of interface FileGenerator.
 * Most of them subclass FileGenOpcodeWriter, which contains some useful
 * methods and fields. Their names are all of the form XXXGenOpcodeWriter,
 * and they are instantiated when the command-line argument -XXX is
 * seen. Adding another one is easy by following the pattern. Just don't forget
 * to update the usage message as well!
 *
 * One special note on output file writing: all the output file writers I provide
 * use util.FileCompare.ConditionalCopy to avoid touching derived files that
 * are unchanged. This is intended to avoid re-compiling due to extraneous
 * file date changes when you are using make/gnumake/something-like-make.
 * You may or may not desire this behavior.
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.StringTokenizer;
import util.BufferedPrintStream;

class GenOpcodes
{
    private static final String[] _usage = {
	"usage: java GenOpcodes input_file_name [ opt target_file_name ]+",
	"	where opt is a flavor of output file. Choices are:",
	"	-h	- C header file, generally opcodes.h",
	"	-c	- C file containing CVMopnames",
	"	-bcAttr	- C file containing CVMbcAttributes",
	"	-opcodeMap - C file containing CVMJITOpcodeMap",
	"	-opcodeLengths - C file containing CVMopcodeLengths",
	"	-simplification - C file containing CVMopcodeSimplification",
	"	-label	- #defines for use in executejava label array",
	"	-javaConst - Java class OpcodeConst"
    };

    private static String		_inFileName;
    private static FileGenerator[]	_fileGenerators;

    /*
     * Main entry point. If there are too few arguments, print a 
     * usage message. Otherwise, parse the arguments. If successful,
     * process the input file. Return status appropriately.
     */
    public static void main(String[] args) {
	processArgs(args);

	try {
	    // Read in the opcodes.

	    Opcode[] opcodes = readOpcodes(_inFileName);

	    // Run each generator.

	    for (int i = 0; i < _fileGenerators.length; i++) {
		_fileGenerators[i].processOpcodes(opcodes);
	    }

	    System.exit(0);
	} catch (GenOpcodesException ex) {
	    System.err.println("GenOpcodes: " + ex.getMessage());
	    ex.printStackTrace(System.err);
	    System.exit(1);
	}
    }

    private static void printUsage() {
	for (int i = 0; i < _usage.length; i++) {
	    System.err.println(_usage[i]);
	}
    }

    /*
     * Parse command-line arguments.
     * Instantiate output writers and init them.
     */
    private static void processArgs(String[] args) {
	/* The first argument is the incoming opcodes definition file
	   (required).  Subsequent arguments are {-<output file type>
	   <output file name>} pairs where at least one pair is
	   specified.  So we expect at least three arguments, and the
	   number of arguments must not be even. */

	if (args.length < 3 || (args.length % 2) == 0) {
	    printUsage();
	    System.exit(1);
	}

	_inFileName = args[0];
	if (_inFileName.length() == 0 || _inFileName.charAt(0) == '-') {
	    System.err.println("Warning: treating \"" + _inFileName +
			       "\" as a file name");
	}

	_fileGenerators = new FileGenerator[args.length / 2];

	for (int i = 1; i < args.length; i += 2) {
	    String arg = args[i];
	    if (arg.charAt(0) != '-') {
		System.err.println("Command line argument \"" + arg +
				   "\" unexpected");
		printUsage();
		System.exit(1);
	    }

	    // The name of the output writer class we are going to
	    // instantiate is the option name (without leading -)
	    // capitalized and concatenated with "GenOpcodeWriter".
	    // It is also an inner class of GenOpcodes.
	    // Thus the option "-c" causes us to instantiate a
	    // "GenOpcodes$CGenOpcodeWriter".

	    String fileGeneratorName = "GenOpcodes$" +
		Character.toUpperCase(arg.charAt(1)) + arg.substring(2) +
		"GenOpcodeWriter";
	    Class fileGeneratorClass;
	    try {
		fileGeneratorClass = Class.forName(fileGeneratorName);
	    } catch (ClassNotFoundException ex) {
		System.err.println("\"" + arg + "\" not supported:\n" + ex);
		printUsage();
		System.exit(1);
		return;
	    }

	    FileGenerator fileGenerator;
	    try {
		fileGenerator =
		    (FileGenerator) fileGeneratorClass.newInstance();
	    } catch (InstantiationException ex) {
		System.err.println("Error instantiating " + fileGeneratorName +
				   ":\n" + ex);
		printUsage();
		System.exit(1);
		return;
	    } catch (IllegalAccessException ex) {
		System.err.println("Error instantiating " + fileGeneratorName +
				   ":\n" + ex);
		printUsage();
		System.exit(1);
		return;
	    }

	    fileGenerator.init(args[i + 1]);

	    _fileGenerators[i / 2] = fileGenerator;
	}
    }

    /*
     * Reads in all the opcodes and returns an array of 256 Opcodes.
     * Array elements with no assigned Opcode will be null.
     */
    private static Opcode[] readOpcodes(String filename)
	throws GenOpcodesException {
	Opcode[] opcodes = new Opcode[256];
	int nextOpcodeNo = 0;

	BufferedReader in;
	try {
	    in = new BufferedReader(new FileReader(filename));
	} catch (FileNotFoundException ex) {
	    throw new GenOpcodesException(
		"Error opening " + filename + ".", ex);
	}

	while (true) {
	    String line;
	    try {
		line = in.readLine();
	    } catch (IOException ex) {
		throw new GenOpcodesException(
		    "Error reading " + filename + ".", ex);
	    }

	    if (line == null) {
		// EOF.
		break;
	    }

	    // Skip comments and blank lines.

	    if (line.length() == 0 || !Character.isLetter(line.charAt(0))) {
		continue;
	    }

	    // Extract the fields.

	    StringTokenizer st = new StringTokenizer(line);

	    String name = st.nextToken();
	    String number = st.nextToken();
	    String length = st.nextToken();
	    st.nextToken();	// pops
	    st.nextToken();	// pushes
	    String[] attributes = parseAttributes(st.nextToken());
	    String simplification = st.nextToken();
	    String tag = st.nextToken();
	    String typeid = st.nextToken();
	    String value = st.nextToken();

	    // Advance to the next unused opcode number.

	    while (opcodes[nextOpcodeNo] != null) {
		nextOpcodeNo++;
	    }

	    // If an opcode number is given then use it, else use
	    // the next available opcode number.

	    int opcodeNo;
	    boolean assigned;
	    if (number.equals("-")) {
		if (nextOpcodeNo > 253) {
		    throw new GenOpcodesException(
			"Too many opcodes (> 253).");
		}
		opcodeNo = nextOpcodeNo;
		assigned = false;
	    } else {
		try {
		    opcodeNo = Integer.parseInt(number);
		} catch (NumberFormatException ex) {
		    throw new GenOpcodesException(
			"Bad opcode number " + number +
			" for opcode " + name, ex);
		}

		if (opcodeNo < 0 || opcodeNo > 253) {
		    throw new GenOpcodesException(
			"Opcode number " + number +
			" for opcode " + name +
			" must be in the range 0 - 253");
		}

		assigned = true;

		// If opcodeNo is in use by another Opcode then move
		// it out of the way, unless it was explicitly
		// assigned.

		Opcode opcode = opcodes[opcodeNo];
		if (opcode != null) {
		    if (opcode.isAssigned()) {
			throw new GenOpcodesException(
			    "Opcode number " + opcodeNo +
			    " has already been assigned.");
		    }

		    if (nextOpcodeNo > 253) {
			throw new GenOpcodesException(
			    "Too many opcodes (> 253).");
		    }

		    opcodes[nextOpcodeNo] = opcode;
		}
	    }

	    opcodes[opcodeNo] = new Opcode(
		assigned,
		name,
		length,
		attributes,
		simplification,
		tag,
		typeid,
		value);
	}

	return opcodes;
    }

    /*
     * Converts a comma-separated list of attributes to a String[] of
     * attributes.
     */
    private static String[] parseAttributes(String attributes) {
	StringTokenizer st = new StringTokenizer(attributes, ",");

	String[] result = new String[st.countTokens()];

	for (int i = 0; i < result.length; i++) {
	    result[i] = st.nextToken();
	}

	return result;
    }

    /*
     * FileGenerator is the interface between the GenOpcode driver and
     * the individual output writers.
     */
    private interface FileGenerator
    {
	/*
	 * init is called immediately after instantiation, with the
	 * file name command-line argument. Do not open the file yet!
	 * If any other error occurs during command-line processing,
	 * the program will exit without further ado.
	 */
	public void init(String fileName);

	/*
	 * Processes and writes all the opcodes.
	 */
	public void processOpcodes(Opcode[] opcodes)
	    throws GenOpcodesException;
    }

    /*
     * FileGenOpcodeWriter is used to implement common functions
     * in most (or all) of the output writers. It does file opening
     * and closing, temporary file management and conditional copying.
     */
    private static abstract class FileGenOpcodeWriter implements FileGenerator
    {
	private final String[] _prolog;
	private final String[] _epilog;

	private String _destFileName;

	private File _destFile;	// Actual file to create.
	private File _outFile;	// Temporary file to write to.
	private PrintStream _out;

	protected FileGenOpcodeWriter(String[] prolog, String[] epilog) {
	    _prolog = prolog;
	    _epilog = epilog;
	}

	/*
	 * This init method just saves the file name.
	 */
	public void init(String destFileName) {
	    _destFileName = destFileName;
	}

	public void processOpcodes(Opcode[] opcodes)
	    throws GenOpcodesException {
	    start();
	    processAllOpcodes(opcodes);
	    done();
	}

	/**
	 * Processes all opcodes.  Subclasses may override this.
	 */
	protected void processAllOpcodes(Opcode[] opcodes) {
	    for (int i = 0; i < opcodes.length; i++) {
		processOneOpcode(i, opcodes[i]);
	    }
	}

	/**
	 * Processes one opcode.  This is what subclasses will usually
	 * need to override.
	 */
	protected void processOneOpcode(int ordinal, Opcode opcode) {}

	private void start() throws GenOpcodesException {
	    openFile();
	    printBlock(_prolog);
	}

	private void openFile() throws GenOpcodesException {
	    _destFile = new File(_destFileName);
	    if (_destFile.exists()) {
		_outFile = new File(_destFileName + ".TMP");
	    } else {
		_outFile = _destFile;
	    }
	    try {
		_out = new BufferedPrintStream(new FileOutputStream(_outFile));
	    } catch (IOException ex) {
		throw new GenOpcodesException(
		    "Could not open output file " + _outFile, ex);
	    }
	}

	private void done() {
	    printBlock(_epilog);
	    closeFile();
	}

	/*
	 * Close output stream, do conditional copy to final
	 * destination if the output file isn't the destination file.
	 */
	private void closeFile() {
	    _out.close();
	    if (_destFile != _outFile) {
		util.FileCompare.conditionalCopy(_outFile, _destFile);
		_outFile.delete();
	    }
	}

	/*
	 * Prints a String[], a common operation when writing 
	 * file prolog/epilog.
	 */
	private void printBlock(String[] block) {
	    for (int i = 0; i < block.length; i++) {
		_out.println(block[i]);
	    }
	}

	protected void print(String s) {
	    _out.print(s);
	}

	protected void println(String s) {
	    _out.println(s);
	}
    }

    /*
     * The C-header file contains enum CVMOpcode of all the opc_XXX names
     * along with an extern declaration of the CVMopnames array, which is
     * created in CGenOpcodeWriter.
     */
    private static class HGenOpcodeWriter extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains the constants for all the opcodes.",
	    " * It is generated from opcodes.list.",
	    " */",
	    "",
	    "#ifndef _INCLUDED_GEN_OPCODES_H",
	    "#define _INCLUDED_GEN_OPCODES_H",
	    "",
	    "#include \"javavm/include/defs.h\"",
	    "",
	    "enum CVMOpcode {",
	};

	private static final String[] _epilog = {
	    "\n};",
	    "",
	    "#endif /* _INCLUDED_GEN_OPCODES_H */",
	};

	public HGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (opcode == null) {
		return;
	    }

	    if (ordinal > 0) {
		print(",\n");
	    }
	    print("    opc_");
	    print(opcode.getName());
	    print(" = ");
	    print(Integer.toString(ordinal));
	}
    }

    /*
     * The C-file contains const char * CVMopnames[256].
     * This is initialized with quoted-string forms of all the opcode names.
     * Otherwise undefined elements are filled with "??"opcodeNumber
     */
    private static class CGenOpcodeWriter extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains an array of opcode names.",
	    " * It is generated from opcodes.list.",
	    " */",
	    "",
	    "#include \"javavm/include/opcodes.h\"",
	    "",
	    "#if defined(CVM_TRACE) || defined(CVM_DEBUG) || defined(CVM_INSTRUCTION_COUNTING)",
	    "const char* const CVMopnames[256] = {",
	};

	private static final String[] _epilog = {
	    "    \"software\",",
	    "    \"hardware\"",
	    "};",
	    "",
	    "#endif",
	};

	public CGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (ordinal < 254) {
		print("    \"");
		print(opcode != null ? opcode.getName() : ("??" + ordinal));
		println("\",");
	    }
	}
    }

    /*
     * The C-file contains const char CVMbcAttributes[256].
     * This is initialized with bitmaps indicating opcode attributes.
     */
    private static class BcAttrGenOpcodeWriter extends FileGenOpcodeWriter
    {
	private static final String gcpointAttr    = "CVM_BC_ATT_GCPOINT";
	private static final String condGcpointAttr= "CVM_BC_ATT_COND_GCPOINT";
	private static final String branchAttr     = "CVM_BC_ATT_BRANCH";
	private static final String excAttr        = "CVM_BC_ATT_THROWSEXCEPTION";
	private static final String noflowAttr     = "CVM_BC_ATT_NOCONTROLFLOW";
	private static final String invocationAttr = "CVM_BC_ATT_INVOCATION";
	private static final String quickAttr      = "CVM_BC_ATT_QUICK";
	private static final String returnAttr     = "CVM_BC_ATT_RETURN";
	private static final String fpAttr         = "CVM_BC_ATT_FP";

	private static final String[] _prolog = {
	    "/*",
	    " * This file contains an array of byte-code attributes.",
	    " * It is generated from opcodes.list.",
	    " */",
	    "",
	    "#include \"javavm/include/defs.h\"",
	    "#include \"javavm/include/bcattr.h\"",
	    "",
	    "const CVMUint16 CVMbcAttributes[256] = {",
	};

	private static final String[] _epilog = {
	    "};",
	};

	public BcAttrGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (opcode != null) {
		println("    /* opc_" + opcode.getName() + " */");
		print("    ");

		String[] attributes = opcode.getAttributes();
		for (int i = 0; i < attributes.length; i++) {
		    if (i > 0) {
			print(" | ");
		    }
		    doOneAttribute(attributes[i]);
		}

		println(",");
	    } else {
		println("    0,");
	    }
	}

	private void doOneAttribute(String attrName) {
	    if (attrName.equals("BR")) {
		print(branchAttr);
	    } else if (attrName.equals("GC")) {
		print(gcpointAttr);
	    } else if (attrName.equals("CGC")) {
		print(condGcpointAttr);
	    } else if (attrName.equals("EXC")) {
		print(excAttr);
	    } else if (attrName.equals("NFLW")) {
		print(noflowAttr);
	    } else if (attrName.equals("INV")) {
		print(invocationAttr);
	    } else if (attrName.equals("QUICK")) {
		print(quickAttr);
	    } else if (attrName.equals("RET")) {
		print(returnAttr);
	    } else if (attrName.equals("FP")) {
		print(fpAttr);
	    } else {
		/*
		 * Covers the case of '-' as well as unknown attributes
		 */
		print("0");
	    }
	}
    }

    /*
     * The C file contains CVMJITOpcodeMap[].
     */
    private static class OpcodeMapGenOpcodeWriter extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains an array of opcode maps.",
	    " * It is generated from opcodes.list.",
	    " */",
	    "",
	    "#include \"javavm/include/jit/jitirnode.h\"",
	    "#include \"javavm/include/typeid.h\"",
	    "#include \"javavm/include/opcodes.h\"",
	    "",
	    "const signed char CVMJITOpcodeMap[256][3] = {",
	};

	private static final String[] _epilog = {
	    "};",
	};

	public OpcodeMapGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (opcode != null) {
		print("{    " + opcode.getTag());
		print(",    " + opcode.getTypeid());
		print(",   " + opcode.getValue());
		println("},    /* opc_" + opcode.getName() + " */");
	    } else {
		println("{0, 0, 0},");
	    }
	}
    }

    /*
     * The C-file contains const char CVMopcodeLengths[256] (lengths for all fixed
     * size opcodes).
     */
    private static class OpcodeLengthsGenOpcodeWriter
	extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains an array of opcode lengths.",
	    " * It is generated from opcodes.list.",
	    " * 0-length means the opcode is variable length or unrecognized.",
	    " */",
	    "",
	    "#include \"javavm/include/opcodes.h\"",
	    "",
	    "const char CVMopcodeLengths[256] = {",
	};

	private static final String[] _epilog = {
	    "};",
	};

	public OpcodeLengthsGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (opcode != null) {
		print("    " + opcode.getLength());
		println(",    /* opc_" + opcode.getName() + " */");
	    } else {
		println("    0,");
	    }
	}
    }

    private static class SimplificationGenOpcodeWriter
	extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains an array of opcode \"simplifications\".",
	    " * It is generated from opcodes.list.",
	    " */",
	    "",
	    "#include \"javavm/include/opcodes.h\"",
	    "",
	    "static const CVMOpcode CVMopcodeSimplification[256] = {",
	};

	private static final String[] _epilog = {
	    "};",
	};

	public SimplificationGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    if (opcode != null) {
		print("    opc_" + opcode.getSimplification());
		println(",    /* opc_" + opcode.getName() + " */");
	    } else {
		println("    0,");
	    }
	}
    }

    /*
     * The label file is interesting only if you're using gcc or equivalent
     * to compiler executejava.c, and are using gcc's labels. This generates
     * a set of cpp defines such as this one, to map specific opcode numbers
     * to specific semantics:
     * 	#define opc_0 opc_nop
     * which indicates that the 0th entry in the label table needs to point to
     * the code to implement the Java nop. All undefined opcode numbers are
     * defined as opc_DEFAULT
     */
    private static class LabelGenOpcodeWriter extends FileGenOpcodeWriter
    {
	private static final String[] _prolog = {
	    "/*",
	    " * This file contains label equates",
	    " * It is generated from opcodes.list.",
	    " * It is for use by executejava.c using gcc label arrays.",
	    " */",
	    "",
	    "#ifndef _INCLUDED_OPCODE_LABELS",
	    "#define _INCLUDED_OPCODE_LABELS",
	    "",
	};

	private static final String[] _epilog = {
	    "",
	    "#endif",
	};

	public LabelGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processOneOpcode(int ordinal, Opcode opcode) {
	    print( "#define opc_");
	    print(Integer.toString(ordinal));
	    print( "\topc_");
	    println(opcode != null ? opcode.getName() : "DEFAULT");
	}
    }

    /*
     * This generates the Java interface OpcodeConst, which contains:
     * - constants such as
     *	public static final int opc_nop = 0; 
     *   for all the defined opcode values.
     * - the array of opcode name strings:
     *	public static final String opcNames[] =
     * - the array of instruction lengths:
     *	public static final int opcLengths[] = 
     *
     * Formerly, the constants, names, and lengths of the real, red-book
     * opcodes were imported from somewhere in the compiler and the quick
     * information was separately, privately maintained in JavaCodeCompact.
     * Now they are all unified in this one, VM-dependent, derived file.
     */

    private static class JavaConstGenOpcodeWriter extends FileGenOpcodeWriter {
	private static final String[] _prolog = {
	    "/*",
	    " * This interface contains opc_ constant values,",
	    " * a table of opcode names, and a table of instruction lengths.",
	    " * It is generated from opcodes.list.",
	    " * It is vm dependent, because it includes the quick opcodes.",
	    " */",
	    "package opcodeconsts;",
	    "public interface OpcodeConst",
	    "{",
	};

	private static final String _epilog[] = {
	    "}",
	};

	public JavaConstGenOpcodeWriter() {
	    super(_prolog, _epilog);
	}

	public void processAllOpcodes(Opcode[] opcodes) {
	    printConstants(opcodes);
	    print("\n");
	    printNames(opcodes);
	    print("\n");
	    printSizes(opcodes);
	}

	private void printConstants(Opcode[] opcodes) {
	    for (int i = 0; i < opcodes.length; i++) {
		Opcode opcode = opcodes[i];
		if (opcode != null) {
		    print("    public static final int opc_");
		    print(opcode.getName());
		    print("\t= ");
		    print(Integer.toString(i));
		    println(";");
		}
	    }
	}

	private void printNames(Opcode[] opcodes) {
	    print("    public static final String[] opcNames = {");

	    for (int i = 0; i < opcodes.length; i++) {
		Opcode opcode = opcodes[i];

		String name;
		if (opcode != null) {
		    name = opcode.getName();
		} else if (i == 254) {
		    name = "hardware";
		} else if (i == 255) {
		    name = "software";
		} else {
		    name = "??" + i;
		}

		if ((i % 4) == 0) {
		    print("\n\t\"");
		} else {
		    print(" \"");
		}
		print(name);
		print("\",");
	    }

	    println("\n    };");
	}

	private void printSizes(Opcode[] opcodes) {
	    print("    public static final int[] opcLengths = {");

	    for (int i = 0; i < opcodes.length; i++) {
		Opcode opcode = opcodes[i];

		if ((i % 10) == 0) {
		    print("\n\t");
		} else {
		    print(" ");
		}
		print(opcode != null ? opcode.getLength() : "0");
		print(",");
	    }

	    println("\n    };");
	}
    }

    private static class Opcode
    {
	private final boolean _assigned;
	private final String _name;
	private final String _length;
	private final String[] _attributes;
	private final String _simplification;
	private final String _tag;
	private final String _typeid;
	private final String _value;

	public Opcode(
	    boolean assigned, String name, String length,
	    String[] attributes, String simplification,
	    String tag, String typeid, String value) {
	    _assigned = assigned;
	    _name = name;
	    _length = length;
	    _attributes = attributes;
	    _simplification = simplification;
	    _tag = tag;
	    _typeid = typeid;
	    _value = value;
	}

	public boolean isAssigned() {
	    return _assigned;
	}

	public String getName() {
	    return _name;
	}

	public String getLength() {
	    return _length;
	}

	public String[] getAttributes() {
	    return _attributes;
	}

	public String getSimplification() {
	    return _simplification;
	}

	public String getTag() {
	    return _tag;
	}

	public String getTypeid() {
	    return _typeid;
	}

	public String getValue() {
	    return _value;
	}
    }

    private static class GenOpcodesException extends Exception
    {
	public GenOpcodesException(String message, Throwable cause) {
	    super(message, cause);
	}

	public GenOpcodesException(String message) {
	    super(message);
	}
    }
}
