/*
 * @(#)GenOpcodes.java	1.36 06/10/10
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

/*
 * GenOpcodes is a program for reading the file
 * src/share/javavm/include/opcodes.list and deriving from it "source"
 * files for building a Java Virtual Machine and associated tools.  It
 * replaces a C program named XXX in earlier P/EJava, which replaced an
 * earlier awk(?) program. The format of opcodes.list is explained in that
 * file, which I quote here:
 *
 * # Any line that doesn't have a-z in the 1st column is a comment.
 * #
 * # The first column is the name of the opcodes.  The second column is the
 * # total length of the instruction.  We use 99 for tableswitch and
 * # tablelookup, which must always be treated as special cases.
 * #
 * # The third and fourth colum give what the opcode pops off the stack, and
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
 * # The fifth column has a comma-separated list of attributes of the
 * # opcode. These are necessary in the stackmap computation dataflow 
 * # analysis.
 * #
 * # GC -- a GC point
 * # CGC -- a conditional GC point; only if the thread is at a quickening point
 * # BR -- a branch
 * # EXC -- May throw exception
 * # INV -- A method invocation
 * # NFLW -- An instruction that doesn't let control flow through
 * #         (returns, athrow)
 * # QUICK -- A quick instruction, re-written by the interpreter.
 * # RET -- A return opcode.
 * # FP    -- A floating point opcode
 * # -   -- No special attributes to speak of
 * #            
 * # The sixth column is a "simplification" of the opcode, for opcode
 * # sequence measurements (see CVM_INSTRUCTION_COUNTING in executejava.c).
 * #
 * # The seventh column has the attribute of CVMJITIROpcodeTag.
 * # The eighth column has the attribute of tyep tag listed in typeid.h.
 * # The ninth colum has the attribute of value representing constant value,
 * # local variable number, etc. 
 * 
 * The general flow of this program is:
 * - parse command-line arguments and instantiate output file writers
 * - read input file. For each non-commentary line of input
 * 	- parse the line into words
 * 	- call each output file writer with the array of words
 * - when the input is exhaused, notify each output file writer
 * 
 * The output file writers are all implementations of interface fileGenerator.
 * Most of them subclass fileGenOpcodeWriter, which contains some useful
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

import java.io.*;
import java.util.StringTokenizer;
import util.BufferedPrintStream;

/*
 * fileGenerator is the interface between the GenOpcode driver and
 * the individual output writers. All methods return boolean: true for
 * success, false for error. If false is returned, the driver will call
 * errorDescription to obtain a printable String message, then the fileGenerator
 * will not be referenced any more. In fact, it might be destroyed by the
 * garbage collector.
 */

interface fileGenerator{
    /*
     * init is called immediately after instantiation, with the
     * file name command-line argument. Do not open the file yet!
     * If any other error occurs during command-line processing,
     * the program will exit without further ado.
     */
    public boolean init( String fileName );

    /*
     * Start is called after all command line processing is done
     * and we are about to commence input file processing. This is
     * an appropriate time to (try to) open or create the output file.
     */
    public boolean start();

    /*
     * opcode is called for each line of non-comment input.
     * ordinal - opcode number, starts with 0
     * operands - and array of GenOpcodes.nInFields strings, representing
     *		the words of the input file line. Padded with "" as necessary.
     *		Their meaning and order is dictated by the opcodes.list format,
     *		as qoted above. operands[0] is the opcode name.
     * noperands - the number of operands read from the file,
     *		1 <= noperands <= nInFields
     */
    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand );

    /*
     * done is called at input file EOF. Finish writing the file and
     * close it.
     */
    public boolean done();

    /*
     * errorDescription called only if one of the other routines returned
     * false. The returned String will be printed to System.err, possibly
     * accompanyed by context information, such as input line number
     */
    public String errorDescription();
}

/*
 * The nullGenerator is a place holder. When a fileGenerator method
 * returns false, that object's fileGenerator.errorDescription is
 * called and printed, then the (only) reference to that object
 * is replaced by a reference to a nullGenerator.
 */
class nullGenerator implements fileGenerator{
    public boolean init( String fileName ){ return true; }

    public boolean start(){ return true; }

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand ) { return true; }

    public boolean done(){ return true; }

    public String errorDescription(){ return ""; }

}

class GenOpcodes{

    static fileGenerator	outputs[];
    static int			nOutputs;
    static String		inFileName;

    /*
     * The maximum number of fields read from a line of the input file.
     * All extras are discarded as comments. If you change the input file
     * format, you probably want to change this number!
     */
    static final int		nInFields = 9;
    
    static void fileName( String a ){
	if ( a.charAt(0) == '-' ){
	    System.err.println("Warning: treating \""+a+"\" as a file name");
	}
    }

    /*
     * Parse command-line arguments.
     * Instantiate output writers, init them, but do not start them.
     * return true on no errors, false on any error.
     */
    static boolean processArgs( String args[] ){
	// count, allocate, initialize
	// accumulate error(s)
	boolean ok = true;
	boolean expectingArg;

	nOutputs = 0;

	inFileName = args[0];
	fileName( inFileName );
	expectingArg = true;

	for ( int i = 1 ; i < args.length; i++ ){
	    String a = args[i];
	    if ( expectingArg ) {
		if ( a.charAt(0) == '-'){
		    nOutputs += 1;
		    expectingArg = false;
		} else {
		    System.err.println("Command line argument \""+a+"\" unexpected");
		    ok = false;
		}
	    } else {
		// expecting a file name.
		fileName( a );
		expectingArg = true;
	    }
	}

	outputs = new fileGenerator[nOutputs];
	int outputNo = 0;

	// now try to construct and initialize all the output
	// writers specified.
	for ( int i = 1 ; i < args.length; i++ ){
	    String a = args[i];
	    if ( a.charAt(0) != '-' ){
		// oops. already gave a warning about this,
		// ( or its a file to an option that couldn't be
		// instantiated )
		// so skip it.
		continue;
	    }
	    //
	    // the name of the output writer class we are going
	    // to instantiate is the option name (without leading -)
	    // concatenated with "GenOpcodeWriter". Thus the option
	    // "-c" causes us to instantiate a "cGenOpcodeWriter".
	    //
	    String writerName = a.substring(1)+"GenOpcodeWriter";
	    Class  writerClass;
	    try {
		writerClass = Class.forName( writerName);
		outputs[outputNo] = (fileGenerator)(writerClass.newInstance());
		outputNo += 1;
	    } catch ( Exception e ){
		// no such class!
		System.err.println("Command line option \""+a+"\" not implemented");
		ok = false;
		continue;
	    }
	    if ( ! outputs[outputNo-1].init( args[++i] ) ){
		System.err.println( outputs[outputNo-1].errorDescription());
		ok = false;
	    }
	}
	// did all instantiations and inits.
	// its up to our caller to call start and to process.

	return ok;
    }

    /*
     * Called simply to read up to EOL when there is more input that
     * we are willing to process, especially lines of comments.
     */
    static void exhaustLine( StreamTokenizer in ) throws IOException {
	int t;
	do {
	    t = in.nextToken();
	} while( (t!=StreamTokenizer.TT_EOL) && (t!=StreamTokenizer.TT_EOF) );
    }

    /*
     * The main body of GenOpcodes
     * - open the named input file. Use a StreamTokenizer to parse the input.
     * - call start method on all the output writers
     * - loop over input lines, discarding the comments, parsing, and
     *   calling each output writer's opcode method on the remainder.
     * - on input EOF, call done method on the output writers.
     *
     * Returns true on no error, false on any error.
     */
    static boolean readFile( ){

	boolean		ok = true;
	StreamTokenizer in;
	String 	        inFields[] = new String[nInFields];

	int		opcodeNo = 0;

	try {
	    in = new StreamTokenizer(
		    new BufferedInputStream(
			new FileInputStream( inFileName ) ) );
	} catch ( IOException e ){
	    System.err.println("Could not open input file \""+inFileName+"\":");
	    e.printStackTrace();
	    return false;
	}
	//
	// set the tokenizer to simply use whitespace as 
	// word seperator, hand us all the ASCII characters
	// between whitespace as words,
	// and recognize EOL.
	in.resetSyntax();
	in.eolIsSignificant( true );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );


	//
	// start up all the output writers
	for ( int i = 0; i < nOutputs; i++ ){
	    if ( outputs[i].start() != true ){
		System.err.println( outputs[i].errorDescription());
		outputs[i] = new nullGenerator();
		ok = false;
	    }
	}

	try {
	    while ( in.nextToken() != StreamTokenizer.TT_EOF ){
		int t;
		int nField;
		if ( in.ttype == StreamTokenizer.TT_EOL )
		    continue; // skip blank lines.
		String firstField = in.sval;
		if ( !Character.isLetter( firstField.charAt(0)) ){
		    // a comment line, apparently.
		    // it does not start with a letter.
		    exhaustLine( in );
		    continue;
		}

		// read fields until we have nInFields of them, or EOL

		inFields[0] = firstField;
		nField = 1;
		while ( nField < nInFields ){
		    t = in.nextToken();
		    if ( t != StreamTokenizer.TT_WORD )
			break; // EOL before fields exhausted. not an error.
		    
		    inFields[nField++] = in.sval;
		}
		if ( nField < nInFields ){
		    // must have hit EOL. Pad with blanks
		    for ( int n = nField; n < nInFields; n++ ){
			inFields[n] = "";
		    }
		} else {
		    // read until EOL
		    exhaustLine( in );
		}

		//
		// now have a bunch of stuff.
		// feed it to the output generators.
		for ( int outNo = 0; outNo < nOutputs; outNo++ ){
		    if ( outputs[ outNo ].opcode( opcodeNo, inFields, nField ) != true ){
			System.err.println( inFileName+", line "+(in.lineno()-1)+": "+outputs[outNo].errorDescription() );
			outputs[outNo] = new nullGenerator();
			ok = false;
		    }
		}
		opcodeNo += 1;

	    }
	} catch ( IOException e ){
	    System.err.println(inFileName+", line "+in.lineno()+": ");
	    e.printStackTrace();
	    ok = false;
	}
	in = null; // make input file go away

	// got end of file.
	// tidy up.
	for ( int outNo = 0; outNo < nOutputs; outNo++ ){
	    if ( outputs[ outNo ].done() != true ){
		System.err.println( outputs[outNo].errorDescription() );
		// don't care, otherwise would do: outputs[outNo] = new nullGenerator();
		ok = false;
	    }
	}

	return ok;
    }

    private static final String usage[] = {
	"usage: java GenConst input_file_name [ opt target_file_name ]+",
	"	where opt is a flavor of output file. Choices are:",
	"	-h	- to generate a C header file, generally opcodes.h",
	"	-c	- to generate a C file containing an array of strings",
	"	-label	- to generate #define for use in executejava label array",
	"	-javaConst - to generate the Java class OpcodeConst"
    };

    private static void getHelp(){
	// print usage message:
	int nlines = usage.length;
	for( int i=0; i < nlines; i++ )
	    System.err.println( usage[i] );
    }

    /*
     * Main entry point. If there are too few arguments, print a 
     * usage message. Otherwise, parse the arguments. If successful,
     * process the input file. Return status appropriately.
     */
    public static void main( String args[] ){
	if ( args.length < 3 ){
	    getHelp();
	    return;
	}
	if ( processArgs( args ) != true )
	    System.exit(1);
	if ( readFile() != true )
	    System.exit(1);
	System.exit(0);
    }

}

/* TO DEBUG the fileGenerator protocol only. Uncomment this,
 * then run java GenOpcodes opcodes.list -echo echoName
 *
 * class echoGenOpcodeWriter implements fileGenerator{
 *     public boolean init( String fileName ){
 * 	System.out.println("echoGenOpcodeWriter.init( "+fileName+")");
 * 	return true;
 *     }
 * 
 *     public boolean start(){
 * 	System.out.println("echoGenOpcodeWriter.start()");
 * 	return true;
 *     }
 * 
 *     public boolean opcode(
 * 	    int    ordinal,
 * 	    String operands[],
 * 	    int    noperand ){
 * 	System.out.println("echoGenOpcodeWriter.opcode( "+ordinal+",..., "+noperand+")");
 * 	for ( int i = 0; i < noperand; i++ ){
 * 	    String a = operands[i];
 * 	    if ( a == null ) a = "<null>";
 * 	    System.out.println("    "+a);
 * 	}
 * 	return true;
 *     }
 * 
 *     public boolean done(){
 * 	System.out.println("echoGenOpcodeWriter.done()");
 * 	return true;
 *     }
 * 
 * 
 *     public String errorDescription(){
 * 	System.out.println("echoGenOpcodeWriter.errorDescripion()");
 * 	return "echoGenOpcodeWriter doesn't have errors";
 *     }
 * }
 */

/*
 * fileGenOpcodeWriter is used to implementation common functions
 * in most (or all) of the output writers. It does file opening
 * and closing, temporary management and conditional copying.
 * Fields out and error are available to subclasses.
 */

class fileGenOpcodeWriter{
    private String	destFileName;
    private File	destFile;
    private File	outFile;
    public  PrintStream out;

    public  String	error;

    /*
     * This init method just collects the file name.
     * It is intended for use by subclasses to implement
     * fileGenerator.init
     */
    public boolean init( String fileName ){
	destFileName = fileName;
	return true;
    }

    /*
     * For use by a fileGenerator.start implementation.
     * Open output file stream. Return false on any error.
     */
    protected boolean openFile(){
	destFile = new File( destFileName );
	if (destFile.exists() ){
	    outFile = new File( destFileName+".TMP" );
	} else {
	    outFile = destFile;
	}
	try {
	    out = new BufferedPrintStream( new FileOutputStream( outFile ) );
	} catch ( IOException e ){
	    error = "could not open output file "+outFile.getName();
	    return false;
	}
	return true;
    }

    /*
     * For use by a fileGenerator.done implementation.
     * Close output stream, do conditional copy to final
     * destination if the output file isn't the destination file.
     */
    protected void closeFile(){
	out.close();
	if ( destFile != outFile ){
	    util.FileCompare.conditionalCopy( outFile, destFile );
	    outFile.delete();
	}
	out = null;
    }

    /*
     * for printing a String[], a common operation when writing 
     * file prologue/epilogue.
     */
    protected void printBlock( String block[] ){
	int nlines = block.length;
	for( int i=0; i < nlines; i++ )
	    out.println( block[i] );
    }

    /*
     * This errorDescription method just returns the error field.
     * It is intended for use by subclasses to implement
     * fileGenerator.errorDescription
     */
    public String errorDescription(){ return error; }
    
}

/*
 * The C-header file contains enum CVMOpcode of all the opc_XXX names
 * along with an extern declaration of the CVMopnames array, which is
 * created in cGenOpcodeWriter.
 */
class hGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    final static String prologue[] = {
	"/*",
	" * This file contains the constants for all the opcodes.",
	" * It is generated from opcodes.list.",
	" */",
	"",
	"#ifndef _INCLUDED_OPCODES_H",
	"#define _INCLUDED_OPCODES_H",
	"",
	"#include \"javavm/include/defs.h\"",
	"",
	"extern const char* const CVMopnames[];",
	"extern const char  CVMopcodeLengths[];",
	"#ifdef CVM_JIT",
	"#define JITMAP_OPC    0",
	"#define JITMAP_TYPEID 1",
	"#define JITMAP_CONST  2",
	"extern const signed char  CVMJITOpcodeMap[][3];",
	"#endif",
	"",
	"/*",
	" * Get the length of the variable-length instruction at 'iStream'",
	" */",
	"extern CVMUint32 CVMopcodeGetLengthVariable(const CVMUint8* iStream);",
	"",
	"#define CVMopcodeGetLength(iStream) \\",
	"    (CVMopcodeLengths[*(iStream)] ? \\",
	"     CVMopcodeLengths[*(iStream)] : \\",
	"     CVMopcodeGetLengthVariable((iStream)))",
	"",
	"enum CVMOpcode {",
    };

    final static String epilogue[] = {
	"\n};",
	"",
	"typedef enum CVMOpcode CVMOpcode;",
	"",
	"#endif /* _INCLUDED_OPCODES_H */",
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    boolean first = true;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( first ){
	    out.print("    opc_");
	    first = false;
	} else {
	    out.print(",\n    opc_");
	}
	out.print( operands[0] );
	out.print(" = ");
	out.print( ordinal );
	return true;
    }

    public boolean done(){
	printBlock( epilogue );
	closeFile();
	return true;
    }

}

/*
 * The C-file contains const char * CVMopnames[256].
 * This is initialized with quoted-string forms of all the opcode names.
 * Otherwise undefined elements are filled with "??"opcodeNumber
 */
class cGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    final static String prologue[] = {
	"/*",
	" * This file contains an array of opcode names.",
	" * It is generated from opcodes.list.",
	" */",
	"",
	"#include \"generated/javavm/include/opcodes.h\"",
	"",
	"#if defined(CVM_TRACE) || defined(CVM_DEBUG) || defined(CVM_INSTRUCTION_COUNTING)",
	"const char* const CVMopnames[256] = {",
    };

    final static String epilogue[] = {
	"    \"software\",",
	"    \"hardware\"",
	"};",
	"",
	"#endif"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int     lastOpno = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( ordinal != lastOpno+1 ){
	    error = "C file writer: operand "+ordinal+" is out of order";
	    return false;
	}
	lastOpno = ordinal;
	out.print( "    \"" );
	out.print( operands[0] );
	out.println("\",");
	return true;
    }

    public boolean done(){
	for ( int i = lastOpno+1 ; i < 254; i++ ){
	    out.println("    \"??"+i+"\",");
	}
	printBlock( epilogue );
	closeFile();
	if ( lastOpno >= 254 ){
	    error = "C file writer: too many opcodes";
	    return false;
	}
	return true;
    }
}

/*
 * The C-file contains const char CVMbcAttributes[256].
 * This is initialized with bitmaps inndicating opcode attributes
 */
class bcAttrGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    private final static int myFieldIdx = 4;

    private final static String gcpointAttr    = "CVM_BC_ATT_GCPOINT";
    private final static String condGcpointAttr= "CVM_BC_ATT_COND_GCPOINT";
    private final static String branchAttr     = "CVM_BC_ATT_BRANCH";
    private final static String excAttr        = "CVM_BC_ATT_THROWSEXCEPTION";
    private final static String noflowAttr     = "CVM_BC_ATT_NOCONTROLFLOW";
    private final static String invocationAttr = "CVM_BC_ATT_INVOCATION";
    private final static String quickAttr      = "CVM_BC_ATT_QUICK";
    private final static String returnAttr     = "CVM_BC_ATT_RETURN";
    private final static String fpAttr         = "CVM_BC_ATT_FP";

    final static String prologue[] = {
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

    final static String epilogue[] = {
	"};"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int     lastOpno = -1;

    private void doOneAttribute(String attrName)
    {
	if (attrName.equals("BR")) {
	    out.print(branchAttr);
	} else if (attrName.equals("GC")) {
	    out.print(gcpointAttr);
	} else if (attrName.equals("CGC")) {
	    out.print(condGcpointAttr);
	} else if (attrName.equals("EXC")) {
	    out.print(excAttr);
	} else if (attrName.equals("NFLW")) {
	    out.print(noflowAttr);
	} else if (attrName.equals("INV")) {
	    out.print(invocationAttr);
	} else if (attrName.equals("QUICK")) {
	    out.print(quickAttr);
	} else if (attrName.equals("RET")) {
	    out.print(returnAttr);
	} else if (attrName.equals("FP")) {
	    out.print(fpAttr);
	} else {
	    /*
	     * Covers the case of '-' as well as unknown attributes
	     */
	    out.print("0");
	}
    }

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( ordinal != lastOpno+1 ){
	    error = "C file writer: operand "+ordinal+" is out of order";
	    return false;
	}
	lastOpno = ordinal;
	out.println("    /* opc_" + operands[0] + " */");
	out.print("    ");
	StringTokenizer attrs = new StringTokenizer(operands[myFieldIdx], ",");
	doOneAttribute(attrs.nextToken());
	while (attrs.hasMoreTokens()) {
	    out.print(" | ");
	    doOneAttribute(attrs.nextToken());
	}
	out.println(",");
	return true;
    }

    public boolean done(){
	for ( int i = lastOpno+1 ; i <= 255; i++ ){
	    out.println("    0,");
	}
	printBlock( epilogue );
	closeFile();
	if ( lastOpno >= 254 ){
	    error = "bcAttr file writer: too many opcodes";
	    return false;
	}
	return true;
    }
}


/*
 * The C-file contains const char CVMopcodeLengths[256] (lengths for all fixed
 * size opcodes).
 */
class opcodeMapGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    private final static int myFieldIdx = 6;

    final static String prologue[] = {
	"/*",
	" * This file contains an array of opcode lengths.",
	" * It is generated from opcodes.list.",
	" * 0-length means the opcode is variable length or unrecognized.",
	" */",
	"",
	"#include \"javavm/include/jit/jitirnode.h\"",
	"#include \"javavm/include/typeid.h\"",
	"#include \"generated/javavm/include/opcodes.h\"",
	"",
	"const signed char CVMJITOpcodeMap[256][3] = {",
    };

    final static String epilogue[] = {
	"};"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int     lastOpno = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( ordinal != lastOpno+1 ){
	    error = "C file writer: operand "+ordinal+" is out of order";
	    return false;
	}
	lastOpno = ordinal;
	out.print("{    "+operands[myFieldIdx]);
	out.print(",    "+operands[myFieldIdx+1]);
	out.print(",   "+operands[myFieldIdx+2]);
	out.println("},    /* opc_" + operands[0] + " */");
	return true;
    }

    public boolean done(){
	for ( int i = lastOpno+1 ; i <= 255; i++ ){
	    out.println("{0, 0, 0},");
	}
	printBlock( epilogue );
	closeFile();
	if ( lastOpno >= 254 ){
	    error = "opcodeMap file writer: too many opcodes";
	    return false;
	}
	return true;
    }
}
/*
 * The C-file contains const char CVMopcodeLengths[256] (lengths for all fixed
 * size opcodes).
 */
class opcodeLengthsGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    private final static int myFieldIdx = 1;

    final static String prologue[] = {
	"/*",
	" * This file contains an array of opcode lengths.",
	" * It is generated from opcodes.list.",
	" * 0-length means the opcode is variable length or unrecognized.",
	" */",
	"",
	"#include \"generated/javavm/include/opcodes.h\"",
	"",
	"const char CVMopcodeLengths[256] = {",
    };

    final static String epilogue[] = {
	"};"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int     lastOpno = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( ordinal != lastOpno+1 ){
	    error = "C file writer: operand "+ordinal+" is out of order";
	    return false;
	}
	lastOpno = ordinal;
	out.print("    "+operands[myFieldIdx]);
	out.println(",    /* opc_" + operands[0] + " */");
	return true;
    }

    public boolean done(){
	for ( int i = lastOpno+1 ; i <= 255; i++ ){
	    out.println("    0,");
	}
	printBlock( epilogue );
	closeFile();
	if ( lastOpno >= 254 ){
	    error = "bcAttr file writer: too many opcodes";
	    return false;
	}
	return true;
    }
}

class simplificationGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    private final static int myFieldIdx = 5;

    final static String prologue[] = {
	"/*",
	" * This file contains an array of opcode \"simplifications\".",
	" * It is generated from opcodes.list.",
	" */",
	"",
	"#include \"generated/javavm/include/opcodes.h\"",
	"",
	"static const CVMOpcode CVMopcodeSimplification[256] = {",
    };

    final static String epilogue[] = {
	"};"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int     lastOpno = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	if ( ordinal != lastOpno+1 ){
	    error = "simplification file writer: operand "+
		ordinal+" is out of order";
	    return false;
	}
	lastOpno = ordinal;
	out.print("    opc_"+operands[myFieldIdx]);
	out.println(",    /* opc_" + operands[0] + " */");
	return true;
    }

    public boolean done(){
	for ( int i = lastOpno+1 ; i <= 255; i++ ){
	    out.println("    0,");
	}
	printBlock( epilogue );
	closeFile();
	if ( lastOpno >= 254 ){
	    error = "simplification file writer: too many opcodes";
	    return false;
	}
	return true;
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
class labelGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    final static String prologue[] = {
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

    final static String epilogue[] = {
	"",
	"#endif"
    };

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int maxOpcode = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	out.print( "#define opc_");
	out.print( ordinal );
	out.print( "	opc_");
	out.println( operands[0] );
	maxOpcode = Math.max( maxOpcode, ordinal );
	return true;
    }

    public boolean done(){
	for ( int i = maxOpcode+1; i <= 255; i++ ){
	    out.print( "#define opc_");
	    out.print( i );
	    out.println( "	opc_DEFAULT");
	}
	printBlock( epilogue );
	closeFile();
	return true;
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
 *
 * The constant values are output on the fly in the opcode method.
 * The other information is saved in arrays and formatted in method done.
 */

class javaConstGenOpcodeWriter extends fileGenOpcodeWriter implements fileGenerator {

    final static String prologue[] = {
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

    final static String epilogue[] = {
	"\n    };",
	"}"
    };

    String opname[] = new String[255];
    int    oplength[] = new int[255];

    public boolean start(){
	if ( ! openFile() )
	    return false;
	printBlock( prologue );
	return true;
    }

    int maxOpcode = -1;

    public boolean opcode(
	    int    ordinal,
	    String operands[],
	    int    noperand )
    {
	out.print( "    public static final int opc_");
	out.print( operands[0] );
	out.print( "	= " );
	out.print( ordinal );
	out.println( ";" );
	opname[ ordinal ] = operands[0];
	maxOpcode = Math.max( maxOpcode, ordinal );
	try {
	    oplength[ ordinal ] = Integer.parseInt( operands[1] );
	} catch ( NumberFormatException e ){
	    error = "expected number for length of "+operands[0]+" instruction, but got \""+operands[1]+"\"";
	    return false;
	}
	return true;
    }

    public boolean done(){
	// print name array.
	out.print("\n    public static final String opcNames[] = {");
	for ( int i = 0; i <= maxOpcode; i++ ){
	    if ( (i&3) == 0 ){
		out.print("\n	\"");
	    } else {
		out.print(" \"");
	    }
	    out.print( opname[i] );
	    out.print("\",");
	}
	for ( int i = maxOpcode+1; i <= 253; i++ ){
	    if ( (i&3) == 0 ){
		out.print("\n	\"??");
	    } else {
		out.print(" \"??");
	    }
	    out.print( i );
	    out.print("\",");
	}
	out.println(" \"hardware\", \"software\"\n    };");

	// print size array
	out.print("    public static final int opcLengths[] = {");
	for ( int i = 0; i <= maxOpcode; i++ ){
	    if ( (i%10) == 0 ){
		out.print("\n	");
	    } else {
		out.print(" ");
	    }
	    out.print( oplength[i] );
	    out.print(",");
	}
	printBlock( epilogue );
	closeFile();
	return true;
    }
}
