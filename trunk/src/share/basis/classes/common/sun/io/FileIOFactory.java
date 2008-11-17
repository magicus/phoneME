/*
 * @(#)FileIOFactory.java	1.11 06/10/10
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

import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;

public class FileIOFactory {
    public static FileIO newInstance(String path) {
        return new FileFileIO(path);
    }

    public static FileIO newInstance(String path, String name) {
        return new FileFileIO(path, name);
    }

    public static FileIO newInstance(FileIO dir, String name) {
        return new FileFileIO(dir, name);
    }

    public static InputStream getInputStream(String path) throws IOException {
        FileIO f = new FileFileIO(path);
        return f.getInputStream();
    }

    public static OutputStream getOutputStream(String path) throws IOException {
        FileIO f = new FileFileIO(path);
        return f.getOutputStream();
    }
}
