/*
 * @(#)ObjectStreamPermissionConstants.java	1.10 06/10/10
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

package sun.io;

import java.io.SerializablePermission;

/*
 * Constants needed by java.io.ObjectInputStream and java.io.ObjectOutputStream
 * for JDK 1.2 security.  If we ever move to a complete JDK 1.2 API,
 * this class can be deleted and these constants added to
 * java.io.ObjectStreamConstants (where they live in 1.2 land).
 * Adding them in our current spec, causes ObjectInputStream
 * and ObjectOutputStream to violate the spec - they are supposed
 * to have JDK 1.1 signatures.
 */

public class ObjectStreamPermissionConstants {
    /**
     * Enable overriding of readObject and writeObject.
     */
    final static public SerializablePermission SUBCLASS_IMPLEMENTATION_PERMISSION
        = new SerializablePermission("enableSubclassImplementation");
    /**
     * Enable substitution of one object for another during
     * serialization/deserialization.
     */
    final static public SerializablePermission SUBSTITUTION_PERMISSION =
        new SerializablePermission("enableSubstitution");
}
