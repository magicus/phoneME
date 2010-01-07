/*
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

package sun.misc;

import java.io.*;
import java.util.LinkedList;
/* 
 * @(#)MemberFilterConfig.java	1.5	06/10/10
 */
/*
 * Input consists of a sequence of class specifiers.
 * Input syntax is:
 * CLASS classname
 * FIELDS
 *	fieldname:fieldsig
 *	...
 * METHODS
 *	methodname:methodsig
 *	...
 *
 * Classnames and signatures are all in internal format: / is the
 * component separator. When a class appears in a signature, it is of
 * the Lname; form. Method sigatures have return type.
 *
 * Either fields or methods may be missing
 * Newlines are not significant. 
 * Whitespace is not allowed within the name:sig pair.
 * Other whitespace is ignored.
 * # starts a comment.
 */

class  MemberFilterConfig{

    String 	    fileName;
    FileReader 	    inFile;
    StreamTokenizer in;
    MemberFilter    filter;
    boolean	    verbose;

    private static final String empty[] = new String[0];

    MemberFilterConfig(String configFileName) throws IOException {
	fileName = configFileName;
	inFile = new FileReader(configFileName);
	in = new StreamTokenizer(new BufferedReader(inFile));
	// now have file open. 
	// Configure the tokenizer.
	in.commentChar('#');
	in.slashSlashComments(false);
	in.eolIsSignificant(false);
	in.wordChars('a','z');
	in.wordChars('A','[');
	in.wordChars('0','9');
	in.wordChars('<','<'); // appears in name <init>
	in.wordChars('>','>');
	in.wordChars('_','_');
	in.wordChars(':',':'); // make special separator wordChar
	in.wordChars(';',';'); // make all parts of a signature wordChar
	in.wordChars('(',')');
	in.wordChars('/','/'); // make name component separator wordChar
	in.whitespaceChars(' ',' '); // ignore space
	in.whitespaceChars('\t','\t'); // ignore tabs
	// construct the MemberFilter
	filter = new MemberFilter();
    }

    // keywords in easy-to-manipulate integer form
    private static final int NOKEY = 0;
    private static final int CLASS = 1;
    private static final int FIELDS = 2;
    private static final int METHODS = 3;
    private static final int MAXKEY = 3;

    private static final String keys[] = {
	"", "CLASS", "FIELDS", "METHODS"
    };

    private static int keyword(String word){
	for( int i = 1; i <= MAXKEY; i++ ){
	    if (word.equals(keys[i])) return i;
	}
	return NOKEY;
    }

    private String
    nextWord() throws Exception{
	int token = in.nextToken();
	if (token == in.TT_EOF){
	    throw new EOFException("EOF");
	}
	if (token != in.TT_WORD){
	    throw new Exception("token type");
	}
	return in.sval;
    }

    private int
    expecting(int token1, int token2, int token3) throws Exception{
	int key;
	key = keyword(nextWord());
	if (key != token1 && key != token2 && key != token3){
	    throw new Exception("token value ".concat(in.sval));
	}
	return key;
    }

    static private void
    printStringArray(String name, String val[]){
	if (val == null || val.length == 0)
	    return;
	System.out.println(name);
	int l = val.length;
	for(int i=0; i<l; i++){
	    System.out.print('\t');
	    System.out.println(val[i]);
	}
    }

    private void
    parseClass() throws Exception {
	String classname = null;
	String word = null;
	int    keyword;
	int    token;
	LinkedList methods = new LinkedList();
	LinkedList fields = new LinkedList();
	try{
	    // parse CLASS and classname
	    expecting(CLASS, -1, -1);
	    classname = nextWord();

	    while(true){
		// parse FIELDS or METHODS
		keyword = expecting(FIELDS, METHODS, CLASS);
		LinkedList thislist;
		if (keyword == CLASS){
		    // beginning of next class signals the end of this one.
		    in.pushBack();
		    break;
		}
		thislist = (keyword == FIELDS)? fields : methods;
		while(true){
		    String memberName = nextWord();
		    // see if this name is a keyword.
		    // since the memberNames, if properly formed,
		    // must contain :, there should be no danger of
		    // confusion.
		    keyword = keyword(memberName);
		    if (keyword != NOKEY){
			in.pushBack();
			break;
		    }
		    // make sure we have our :
		    if (memberName.indexOf(':') == -1){
			throw new Exception("format error in ".concat(memberName));
		    }
		    thislist.add(memberName);
		}
	    }
	}finally{
	    if (classname != null){
		// here, using empty as a prototype for String[]
		String methodstrings[] = (String[])methods.toArray(empty);
		String fieldstrings[] = (String[])fields.toArray(empty);
		if (verbose){
		    System.out.print("CLASS ");
		    System.out.println(classname);
		    printStringArray("    FIELDS", fieldstrings);
		    printStringArray("    METHODS", methodstrings);
		}
		filter.addRestrictions(classname,
			       fieldstrings, methodstrings);
	    }
	}
    }

    private void
    printExceptionInfo(Exception e, String msg){
	System.err.print(fileName);
	System.err.print(": ");
	if (in != null){
	    System.err.print("line ");
	    System.err.println(String.valueOf(in.lineno()));
	}
	System.err.print(msg);
	System.err.print(": ");
	System.err.println(e.getMessage());
    }

    public void
    setVerbose(boolean v){
	verbose = v;
    }

    public MemberFilter
    parseFile(){
        if (!filter.findROMFilterData()) {
	    try{
	        while(true){
		    parseClass();
	        }
	    }catch(EOFException e){
	        // normal termination. do nothing
	    }catch(IOException e){
	        printExceptionInfo(e, "IO error");
	        return null;
	    }catch(Exception e){
	        printExceptionInfo(e, "syntax error");
	        return null;
	    }
	    filter.doneAddingRestrictions();
        }
	return filter;
    }
}
