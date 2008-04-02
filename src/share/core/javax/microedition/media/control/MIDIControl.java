/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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


/* JAVADOC ELIDED */

public interface MIDIControl extends javax.microedition.media.Control {

    // constants for MIDI status (upper nibble of first byte)

    /**
     * Command value for Note On message (0x90, or 144). 
     * To turn a note off, send a NOTE_ON message with 0 
     * velocity. Alternatively, a Note Off message (0x80) 
     * can be sent.
     *
     * @see #shortMidiEvent(int, int, int)
     */
    int NOTE_ON = 0x90;  // 144


    /**
     * Command value for Control Change message (0xB0, or 176).
     * @see #shortMidiEvent(int, int, int)
     */
    int CONTROL_CHANGE = 0xB0;  // 176

    // query device state

    /**
     * Returns whether banks of the synthesizer can be queried.
     * <p>
     * If this functions returns true,
     * then the following methods can be used to query banks:
     * <ul>
     * <li>{@link #getProgram(int) getProgram(int)}</li>
     * <li>{@link #getBankList(boolean) getBankList(boolean)}</li>
     * <li>{@link #getProgramList(int) getProgramList(int)}</li>
     * <li>{@link #getProgramName(int, int) getProgramName(int, int)}</li>
     * <li>{@link #getKeyName(int, int, int) getKeyName(int, int, int)}</li>
     * </ul>
     *
     * @return true if this device supports querying of banks
     */
    boolean isBankQuerySupported();


    // send a Program Change short MIDI message, or
/* JAVADOC ELIDED */
    int[] getProgram(int channel) 
	throws MediaException;


/* JAVADOC ELIDED */
    int getChannelVolume(int channel);


    // set device state

    /**
     * Set program of a channel. This sets the current program for the
     * channel and may be overwritten during playback by events in a MIDI sequence.<p>
     * It is a high level convenience function. Internally, these method calls are
     * executed:<p>
     * <code>
     * &nbsp;&nbsp;shortMidiEvent(CONTROL_CHANGE | channel, CONTROL_BANK_CHANGE_MSB, bank >> 7);<br>
     * &nbsp;&nbsp;shortMidiEvent(CONTROL_CHANGE | channel, CONTROL_BANK_CHANGE_LSB, bank & 0x7F);<br>
     * &nbsp;&nbsp;shortMidiEvent(PROGRAM_CHANGE | channel, program, 0);
     * </code><p>
     *
     * In order to use the default bank (the initial bank), set the bank parameter to -1.
     * <p>
     *
     * In order to set a program without explicitly setting the bank,
     * use the following call: <p>
     * <code>
     * &nbsp;&nbsp;shortMidiEvent(PROGRAM_CHANGE | channel, program, 0);
     * </code><p>
     *
     * In both examples, the following constants are used:<p>
     * <code>
     * &nbsp;&nbsp;int PROGRAM_CHANGE = 0xC0;<br>
     * &nbsp;&nbsp;int CONTROL_BANK_CHANGE_MSB = 0x00;<br>
     * &nbsp;&nbsp;int CONTROL_BANK_CHANGE_LSB = 0x20;
     * </code><p>
     *
     * @param channel 0-15
     * @param bank 0-16383, or -1 for default bank
     * @param program 0-127
     * @exception IllegalArgumentException Thrown if any of the given 
     * parameters is out of range.
     * @exception IllegalStateException Thrown if the player has not been prefetched.
     * @see #getProgram
     */
    void setProgram(int channel, int bank, int program);


/* JAVADOC ELIDED */
    void setChannelVolume(int channel, int volume); 


    // banks

/* JAVADOC ELIDED */
    int[] getBankList(boolean custom) 
	throws MediaException;


/* JAVADOC ELIDED */
    int[] getProgramList(int bank) 
	throws MediaException;


/* JAVADOC ELIDED */
    String getProgramName(int bank, int prog) 
	throws MediaException;


/* JAVADOC ELIDED */
    String getKeyName(int bank, int prog, int key) 
	throws MediaException;


/* JAVADOC ELIDED */
    void shortMidiEvent(int type, int data1, int data2);


    /**
     * Sends a long MIDI event to the device, typically a system exclusive message.
     * This method passes the data directly to the receiving device. 
     * The data array's contents are not checked for validity.<p>
     * It is possible to send short events, or even a series of short events
     * with this method.<p>
     *
     * @param data array of the bytes to send
     * @param offset start offset in data array
     * @param length number of bytes to be sent
     * @exception IllegalArgumentException Thrown if any one of the given 
     *            parameters is not valid.
     * @exception IllegalStateException Thrown if the player has not been prefetched.
     * @return the number of bytes actually sent to the device or
     *         -1 if an error occurred
     */
    int longMidiEvent(byte[] data, int offset, int length);
}
