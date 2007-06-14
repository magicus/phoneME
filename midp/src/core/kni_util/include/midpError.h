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
#ifndef _MIDP_ERROR_H_
#define _MIDP_ERROR_H_

/**
 * @file
 * @ingroup core_kni
 *
 * @brief Definition of error conditions and exceptions that platform
 *  dependent layer can use.
 *
 */

#include <midp_global_status.h>

/**
 * 'C' string for java.lang.OutOfMemoryError.
 */
extern const char* const midpOutOfMemoryError;

/**
 * 'C' string for java.lang.IllegalArgumentException.
 */
extern const char* const midpIllegalArgumentException;

/**
 * 'C' string for java.lang.IllegalStateException.
 */
extern const char* const midpIllegalStateException;

/**
 * 'C' string for java.lang.RuntimeException.
 */
extern const char* const midpRuntimeException;

/**
 * 'C' string for java.lang.ArrayIndexOutOfBoundsException.
 */
extern const char* const midpArrayIndexOutOfBoundsException;

/**
 * 'C' string for java.lang.StringIndexOutOfBoundsException.
 */
extern const char* const midpStringIndexOutOfBoundsException;

/**
 * 'C' string for java.lang.NullPointerException.
 */
extern const char* const midpNullPointerException;

/**
 * 'C' string for java.lang.ClassNotFoundException.
 */
extern const char* const midpClassNotFoundException;

/**
 * 'C' string for java.io.IOException.
 */
extern const char* const midpIOException;

/**
 * 'C' string for java.io.InterruptedIOException.
 */
extern const char* const midpInterruptedIOException;

/**
 * 'C' string for javax.microedition.io.ConnectionNotFoundException.
 */
extern const char* const midpConnectionNotFoundException;

/**
 * 'C' string for javax.microedition.rms.RecordStoreException
 */
extern const char* const midpRecordStoreException;

/**
 * 'C' string for com.sun.midp.midletsuite.MIDletSuiteLockedException
 */
extern const char* const midletsuiteLocked;

/**
 * 'C' string for com.sun.midp.midletsuite.MIDletSuiteCorruptedException
 */
extern const char* const midletsuiteCorrupted;

/**
 * 'C' string for java.lang.SecurityException
 */
extern const char* const midpSecurityException;

/* @} */

#endif /* _MIDP_ERROR_H_ */

