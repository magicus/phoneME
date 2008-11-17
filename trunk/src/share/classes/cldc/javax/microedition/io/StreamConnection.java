/*
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

package javax.microedition.io;

import java.io.*;

/**
 * This interface defines the capabilities that a stream connection
 * must have.
 * <p>
 * In a typical implementation of this interface (for instance
 * in MIDP 2.0), all <code>StreamConnections</code> have one 
 * underlying <code>InputStream</code> and one <code>OutputStream</code>.
 * Opening a <code>DataInputStream</code> counts as opening an
 * <code>InputStream</code> and opening a <code>DataOutputStream</code>
 * counts as opening an <code>OutputStream</code>.  Trying to open
 * another <code>InputStream</code> or <code>OutputStream</code>
 * causes an <code>IOException</code>.  Trying to open the
 * <code>InputStream</code> or <code>OutputStream</code> after
 * they have been closed causes an <code>IOException</code>.
 * <p>
 * NOTE of clarification for implementations supporting multiple streams: 
 * Each <code>openXXXStream</code> (where XXX can be
 * either Input or Output) call returns 
 * a unique new stream, <b>not</b> a copy of a single stream.  When an opened 
 * stream is closed, a <code>StreamConnection</code> implementation may 
 * choose to close the connection and close all other opened streams derived 
 * from that connection.  When this choice of implementation happens, further 
 * access to the connection and streams will throw an <code>IOException</code>.
 * When this choice of implementation is <b>not</b> chosen, where one opened 
 * stream is closed, all other opened streams must remain open and any
 * other <code>openXXXStream</code> call is valid and must return a new 
 * unique <code>XXXStream</code>.
 * <p>
 * <p>
 * The methods of <code>StreamConnection</code> are not 
 * synchronized.  The only stream method that can be called safely
 * in another thread is <code>close</code>.
 *
 * @version 12/17/01 (CLDC 1.1)
 * @since   CLDC 1.0
 */
public interface StreamConnection extends InputConnection, OutputConnection {
}

