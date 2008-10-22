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

#ifndef _MIDPRMS_H_
#define _MIDPRMS_H_

/**
 * @defgroup rms Record Management System - Single Interface (Both Porting and External)
 * @ingroup subsystems
 */

/**
 * @file
 * @ingroup rms
 *
 * @brief Interface for managing records and record stores.
 * A platform independent file based implementation of this interface
 * is provided already. This interface only needs to be ported if
 * a platform specific storage is preferred, like a Database.
 *
 * ##include &lt;&gt;
 * @{
 */

#include <pcsl_string.h>
#include <suitestore_common.h>
#include <midpRMS-internal.h>

/**
 * Gets the amount of RMS storage that the given MIDlet suite is using.
 *
 * @param filenameBase MIDlet suite's identifier
 * @param id ID of the suite
 * Only one of the parameters will be used by a given implementation.  In the
 * case where the implementation might store data outside of the MIDlet storage,
 * the filenameBase will be ignored and only the suite id will be pertinent.
 *
 * @return the number of bytes of storage the suite is using, or
 * <tt>OUT_OF_MEM_LEN</tt> if the MIDP implementation has run out of memory
 */
long rmsdb_get_rms_storage_size(pcsl_string* filenameBase, SuiteIdType id);

/**
 * Remove all the Record Stores for a suite. Used by an OTA installer
 * when updating a installed suite after asking the user.
 *
 * @param filenameBase ID of the suite
 * @param id ID of the suite
 * Only one of the parameters will be used by a given implementation.  In the
 * case where the implementation might store data outside of the MIDlet storage,
 * the filenameBase will be ignored and only the suite id will be pertinent.
 *
 * @return false if the stores could not be deleted (ex: out of memory or
 *  file locked) else true
 */
int rmsdb_remove_record_stores_for_suite(pcsl_string* filenameBase, 
                                         SuiteIdType id);

/* @} */

#endif /* _MIDPRMS_H_ */
