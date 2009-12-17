/*
 * @(#)PixelStore32.java	1.14 06/10/10
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

package sun.awt.image;

import java.awt.image.*;

public class PixelStore32 extends PixelStore {
    public PixelStore32() {}

    public PixelStore32(int w, int h) {
        super(w, h);
    }

    public PixelStore32(int w, int h, ColorModel cm) {
        setDimensions(w, h);
        setColorModel(cm);
    }

    public void setDimensions(int w, int h) {
        super.setDimensions(w, h);
    }

    Object allocateLines(int num) {
        return (Object) new int[num * width];
    }

    void replayLines(ImageConsumer ic, int i, int cnt, Object line) {
        ic.setPixels(0, i, width, cnt, colormodel,
            (int[]) line, offsets[i], width);
    }
}
