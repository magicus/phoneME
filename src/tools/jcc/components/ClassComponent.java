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

package components;
import java.io.DataOutput;
import java.io.IOException;

/*
 * An abstract class for representing components of a class
 * This include any field, method, or "constant".
 */

public
abstract class ClassComponent
{
    // whether or not "resolved" has been called, 
    // which sometimes determines interpretation
    public boolean resolved = false;

    abstract public void write( DataOutput o ) throws IOException;

    public void resolve( ConstantObject table[] ){
	// by default, just note that we're resolved.
	resolved = true;
    }

    public void
    externalize( ConstantPool p ){
	// by default, there's nothing to do
    }

}
