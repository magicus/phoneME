/*
 * @(#)GTextAreaPeer.java	1.14 06/10/10
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

package sun.awt.gtk;

import java.awt.*;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GTextAreaPeer extends GTextComponentPeer implements TextAreaPeer {
    /** Creates a new GTextAreaPeer. */

    GTextAreaPeer (GToolkit toolkit, TextArea target) {
        super(toolkit, target);
    }

    protected void create() {
        createTextArea(((TextArea) target).getScrollbarVisibility());
    }
	
    private native void createTextArea(int scrollPolicy);

    protected native void insertNative(byte[] text, int pos);

    public void insert(String text, int pos) {
        if (text != null)
            insertNative(stringToNulMultiByte(text), pos);
    }

    protected native void replaceRangeNative(byte[] text, int start, int end);

    public void replaceRange(String text, int start, int end) {
        if (text != null)
            replaceRangeNative(stringToNulMultiByte(text), start, end);
    }
}
