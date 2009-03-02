/*
 * @(#)jpeg.java	1.14 06/10/10
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

package sun.net.www.content.image;

import java.net.URL;
import java.net.URLConnection;
import java.net.*;
import sun.awt.image.*;
import java.io.InputStream;
import java.io.IOException;
import java.awt.Image;
import java.awt.Toolkit;

public class jpeg extends ContentHandler {
    public Object getContent(URLConnection urlc) throws java.io.IOException {
        return new URLImageSource(urlc);
    }

    public Object getContent(URLConnection urlc, Class[] classes) throws IOException {
        for (int i = 0; i < classes.length; i++) {
            if (classes[i].isAssignableFrom(URLImageSource.class)) {
                return new URLImageSource(urlc);
            }
            if (classes[i].isAssignableFrom(Image.class)) {
                Toolkit tk = Toolkit.getDefaultToolkit();
                return tk.createImage(new URLImageSource(urlc));
            }
        }
        return null;
    }
}
