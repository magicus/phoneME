/*
 *   
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 * Copyright 2000 Motorola, Inc. All Rights Reserved.
 * This notice does not imply publication.
 */

package com.sun.j2me.rms; 

/**
 * An interface representing a bidirectional record store Record
 * enumerator. 
 */
public interface RecordEnumeration {

    /**
     * Returns the recordId of the <em>next</em> record in this enumeration,
     * where <em>next</em> is defined by the comparator and/or filter
     * supplied in the constructor of this enumerator. After calling
     * this method, the enumeration is advanced to the next available
     * record.
     *
     * @exception InvalidRecordIDException when no more records are
     *          available. Subsequent calls to this method will
     *          continue to throw this exception until
     *          <code>reset()</code> has been called to reset the
     *          enumeration.
     *
     * @return the recordId of the next record in this enumeration
     */
    public int nextRecordId() throws InvalidRecordIDException;

    /**
     * Returns true if more elements exist in the <em>next</em> direction.
     *
     * @return true if more elements exist in the <em>next</em>
     *         direction
     */
    public boolean hasNextElement();

}
