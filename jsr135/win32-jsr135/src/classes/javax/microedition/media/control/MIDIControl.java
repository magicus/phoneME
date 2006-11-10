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

public interface MIDIControl extends javax.microedition.media.Control {

    // constants for MIDI status (upper nibble of first byte)

    // JAVADOC COMMENT ELIDED
    int NOTE_ON = 0x90;  // 144


    // JAVADOC COMMENT ELIDED
    int CONTROL_CHANGE = 0xB0;  // 176

    // query device state

    // JAVADOC COMMENT ELIDED
    boolean isBankQuerySupported();


    // send a Program Change short MIDI message, or
    // JAVADOC COMMENT ELIDED
    int[] getProgram(int channel) 
	throws MediaException;


    // JAVADOC COMMENT ELIDED
    int getChannelVolume(int channel);


    // set device state

    // JAVADOC COMMENT ELIDED
    void setProgram(int channel, int bank, int program);


    // JAVADOC COMMENT ELIDED
    void setChannelVolume(int channel, int volume); 


    // banks

    // JAVADOC COMMENT ELIDED
    int[] getBankList(boolean custom) 
	throws MediaException;


    // JAVADOC COMMENT ELIDED
    int[] getProgramList(int bank) 
	throws MediaException;


    // JAVADOC COMMENT ELIDED
    String getProgramName(int bank, int prog) 
	throws MediaException;


    // JAVADOC COMMENT ELIDED
    String getKeyName(int bank, int prog, int key) 
	throws MediaException;


    // JAVADOC COMMENT ELIDED
    void shortMidiEvent(int type, int data1, int data2);


    // JAVADOC COMMENT ELIDED
    int longMidiEvent(byte[] data, int offset, int length);
}
