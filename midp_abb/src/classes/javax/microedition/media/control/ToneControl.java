/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package javax.microedition.media.control;
import javax.microedition.media.MediaException;

/**
 * This class is defined by the JSR-135 specification
 * <em>Mobile Media API,
 * Version 1.2.</em>
 */
// JAVADOC COMMENT ELIDED
public interface ToneControl extends javax.microedition.media.Control {

    // JAVADOC COMMENT ELIDED
    byte VERSION = -2;
    
    // JAVADOC COMMENT ELIDED
    byte TEMPO = -3;
    
    // JAVADOC COMMENT ELIDED
    byte RESOLUTION = -4;
    
    // JAVADOC COMMENT ELIDED
    byte BLOCK_START = -5;

    // JAVADOC COMMENT ELIDED
    byte BLOCK_END = -6;

    // JAVADOC COMMENT ELIDED
    byte PLAY_BLOCK = -7; 

    // JAVADOC COMMENT ELIDED
    byte SET_VOLUME = -8;
    
    // JAVADOC COMMENT ELIDED
    byte REPEAT = -9;
    
    // JAVADOC COMMENT ELIDED
    byte C4 = 60;

    // JAVADOC COMMENT ELIDED
    byte SILENCE = -1;

    // JAVADOC COMMENT ELIDED
    void setSequence(byte[] sequence);
}
