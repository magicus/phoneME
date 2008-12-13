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

package runtime;
import util.BufferedPrintStream;
import java.io.*;
import java.lang.reflect.Array;

/*
 * An I/O abstraction that adds a few often-used function
 * to the basic PrintStream. Implemented on top of the BufferedPrintStream.
 */
public class CCodeWriter {
    
    BufferedPrintStream out;

    public CCodeWriter( java.io.OutputStream file ){
	out = new BufferedPrintStream(file);
    }

    private static String hexString[];

    static {
        hexString = new String[0x100];
        for(int i = 0; i < 256; i++){
	    hexString[i] = Integer.toHexString(i + 256).substring(1);
        }
    }

    public
    final void printHexInt( int v ) {
        int a, b, c, d;
        a = v>>>24;
        b = (v>>>16) & 0xff;
        c = (v>>>8) & 0xff;
        d = v & 0xff;
        print("0x");
        if ( a != 0 ){
            print(hexString[ a ]);
            print(hexString[ b ]);
            print(hexString[ c ]);
            print(hexString[ d ]);
        } else if ( b != 0 ){
            print(hexString[ b ]);
            print(hexString[ c ]);
            print(hexString[ d ]);
        } else if ( c != 0 ){
            print(hexString[ c ]);
            print(hexString[ d ]);
        } else {
            print(hexString[ d ]);
        }
    }

    void
    printSafeString(String x) { 
        final int STRING_PURE = 0;
        final int STRING_SOME_BAD = 1;
        final int STRING_ALL_BAD = 2;
        // STRING_PURE means there are no problematic characters.
        //
        // STRING_SOME_BAD means that we should print out the string
        // normally, but escape the uncertain characters.  
        //
        // STRING_ALL_BAD means that we encountered a character that makes
        // us believe that this is an encoded string, and we should print
        // out everything except letters using escape sequences.

        int stringType = STRING_PURE;
        int length = x.length();

        for (int i = 0; i < length; i++) { 
            char c = x.charAt(i);
            if (c < ' ' || c > '~') { 
                stringType = STRING_ALL_BAD;
                break;
            }
            if (c == '"' || c == '\\' || c == '/') { 
                stringType = STRING_SOME_BAD;
            }
        }

        write ('"');
        if (stringType == STRING_PURE) { 
            // There were no bad characters whatsoever.  Just print it
            print(x); 
            write('"');
            return;
        }

        for (int i = 0; i < length; i++) { 
            char c = x.charAt(i);
            if (Character.isLetterOrDigit(c) && c < 128) { 
                write(c);
            } else if ((stringType != STRING_ALL_BAD)
                 &&  ( c >= (byte)' ' ) && ( c <= (byte)'~' )
                 && ( c != (byte)'"') && (c  != (byte)'\\' )  
                 && ( c != (byte)'/')) {
                    // the only dangers between space and ~ are " and \  
                    // We must also be careful about writing */ by accident.
                write(c);
            } else {
                switch ( c ){
                case '\\':
                    print("\\\\");
                    break;
                case '\n':
                    print("\\n");
                    break;
                case '\r':
                    print("\\r");
                    break;
                case '\t':
                    print("\\t");
                    break;
                case '"':
                    print("\\\"");
                    break;
                case '/':
                    // we only have a problem if the previous char was an
                    // asterisk.  It looks like we're ending the comment.
                    if (i > 0 && x.charAt(i - 1) == (byte)'*')
                        write('\\');
                    write('/');
                    break;
                default: 
                    int temp = (c & 0xFF) + 01000;
                    print("\\");
                    print(Integer.toOctalString(temp).substring(1));
                    break;
                    // end of switch
                }
            }
        } // end of for

        write('"');
    }
    
    public void print(Object x) { 
	print(x.toString());
    }

    public void print(String x) { 
	int length = x.length();
	for (int i = 0; i < length; i++) { 
	    write(x.charAt(i));
	}
    }

    public void println(String x) { 
	print(x);
	write('\n');
    }

    public void println() { 
	write('\n');
    }

    public void print(int x) { 
	print(Integer.toString(x));
    }

    private int column = 0;
    public void write(int x) { 
	if (x == '\n') { 
	    out.write(x); 
	    column = 0;
	} else if (x == '\t') { 
	    do { out.write(' '); column++;
	    } while ((column % 4) != 0);
	} else { 
	    out.write(x);
	    column++;
	}
    }
    public boolean checkError() { 
	return out.checkError();
    }

    public void flush() { 
	out.flush();
    }

    public void close() {
	out.close();
    }

}
