/*
 * @(#)scan.h	1.6 06/10/10
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

#ifndef __SCANH__
#define __SCANH__
/*
 * things exported by the scanner.
 * other than the scanner itself, which is
 * apparently automagically recognized by 
 * the parser.
 *
 * These are all called from the parser to set lex's
 * start state. This provides us a cheap way to "parse"
 * full C++ expressions and statements without having
 * to have a whole parser for it here, and without
 * worrying about a possible collision between a
 * busbuild deyword and a C++-language token.
 */

/*
 * called from the parser when we are expecting
 * the next token to be a C++ expression.
 * An expression is always terminated by a trailing
 * colon, which will be the next token returned.
 * An expression can contain a colon, so long as it
 * is within parentheses, or is counterbalanced by
 * a preceding ?. An expression can be empty.
 * After the recognition of an expression, the
 * scanner returns to its normal state.
 */
void want_cexpr(void);

/*
 * called from the parser when we are expecting
 * the next token to be a C++ statement.
 * A statement is always terminated by a ;, which
 * is considered to be part of the statement. A ;
 * will not terminate the statement if within
 * ( or { pairs.
 * After the recognition of a statement, the scanner
 * returns to its normal state.
 */
void want_cstmt(void);

/*
 * Force the scanner into the normal state.
 * This should be useful for syntax-error recovery,
 * when the usual state-setting mechanism is not
 * invoked.
 */
void reset_scanner(void);

#endif
