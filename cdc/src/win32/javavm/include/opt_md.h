/*
 * @(#)opt_md.h	1.0 07/07/05
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
#ifdef WINCE
    /** params for stdio file redirection */
    {"stdioPrefix", "prefix for stdio files",
        CVM_STRING_OPTION,
    {{0, (CVMAddr)"<native file path>", (CVMAddr)""}},
        &CVMglobals.target.stdioPrefix}, 
    {"stdin", "path for stdin",
        CVM_STRING_OPTION,
    {{0, (CVMAddr)"<native file path>", 0}},
        &CVMglobals.target.stdinPath},
    {"stdout", "path for stdout",
        CVM_STRING_OPTION,
    {{0, (CVMAddr)"<native file path>", 0}},
        &CVMglobals.target.stdoutPath},
    {"stderr", "path for stderr",
        CVM_STRING_OPTION,
    {{0, (CVMAddr)"<native file path>", 0}},
        &CVMglobals.target.stderrPath},

    /** params for allowing virtual allocs from the WinCE Large Memory Area: */
    {
	"useLargeMemoryArea",                 /* Opt Name */
	"VirtualAlloc in Large Memory Area",  /* Opt Desc */
	CVM_BOOLEAN_OPTION,                   /* Opt Type */ 
	{{                                    /* Opt Range & Default */
	    CVM_FALSE,                        /*   Min value     */
	    (CVMAddr)CVM_TRUE,                /*   Max value     */
	    (CVMAddr)CVM_FALSE                /*   Default value */
        }},
	&CVMglobals.target.useLargeMemoryArea /* Opt Variable */
    },

#endif
    /** params for stdio socket redirection */
    {"stdoutPort", "Port to redirect stdout to",
        CVM_INTEGER_OPTION, 
    {{0, (CVMAddr)65536, (CVMAddr)-1}},
        &CVMglobals.target.stdoutPort},
    {"stderrPort", "Port to redirect stderr to",
        CVM_INTEGER_OPTION, 
    {{0, (CVMAddr)65536, (CVMAddr)-1}},
        &CVMglobals.target.stderrPort},
