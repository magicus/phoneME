/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * Common exception throwing mechanism
 */

#include <midpError.h>

/** 'C' string for java.lang.OutOfMemoryError */
const char* const midpOutOfMemoryError = "java/lang/OutOfMemoryError";
/** 'C' string for java.lang.IllegalArgumentException */
const char* const midpIllegalArgumentException = "java/lang/IllegalArgumentException";
/** 'C' string for java.lang.IllegalStateException */
const char* const midpIllegalStateException = "java/lang/IllegalStateException";
/** 'C' string for java.lang.RuntimeException */
const char* const midpRuntimeException = "java/lang/RuntimeException";
/** 'C' string for java.lang.ArrayIndexOutOfBoundsException */
const char* const midpArrayIndexOutOfBoundsException = "java/lang/ArrayIndexOutOfBoundsException";
/** 'C' string for java.lang.StringIndexOutOfBoundsException */
const char* const midpStringIndexOutOfBoundsException = "java/lang/StringIndexOutOfBoundsException";
/** 'C' string for java.lang.NullPointerException */
const char* const midpNullPointerException = "java/lang/NullPointerException";
/** 'C' string for java.lang.ClassNotFoundException */
const char* const midpClassNotFoundException = "java/lang/ClassNotFoundException";
/** 'C' string for java.io.IOException */
const char* const midpIOException = "java/io/IOException";
/** 'C' string for java.io.InterruptedIOException */
const char* const midpInterruptedIOException = "java/io/InterruptedIOException";
/** 'C' string for javax.microedition.io.ConnectionNotFoundException */
const char* const midpConnectionNotFoundException = "javax/microedition/io/ConnectionNotFoundException";
/** 'C' string for javax.microedition.rms.RecordStoreException */
const char* const midpRecordStoreException = "javax/microedition/rms/RecordStoreException";
/** 'C' string for com.sun.midp.midletsuite.MIDletSuiteLockedException */
const char* const midletsuiteLocked = "com/sun/midp/midletsuite/MIDletSuiteLockedException";
/** 'C' string for com.sun.midp.midletsuite.MIDletSuiteCorruptedException */
const char* const midletsuiteCorrupted = "com/sun/midp/midletsuite/MIDletSuiteCorruptedException";
/** 'C' string for java.lang.SecurityException */
const char* const midpSecurityException = "java/lang/SecurityException";
