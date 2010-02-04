/*
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.security.x509;

/**
 * This class wraps unparseable non critical certificate extension.
 */
class UnparseableExtension extends Extension {
    // The name of the extension class
    private String name;

    // The reason of the extension became unparseable
    private Throwable why;

    public UnparseableExtension(Extension extension, Throwable throwable) {
        super(extension);
        try {
            Class cl = OIDMap.getClass(extension.getExtensionId());
            if (cl != null) {
                name = cl.getName();
            }
        } catch (Exception exc) { }
        why = throwable;
    }

    public String toString() {
        return super.toString() + "Unparseable " + name + " extension due to\n"
               + why + "\n\n";
    }
}

