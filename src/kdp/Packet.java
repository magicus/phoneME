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
 *
 */

package kdp;

import java.io.*;

public class Packet extends Object implements VMConstants {
    public final static byte NoFlags = 0x0;
    public final static byte Reply = (byte)0x80;
    public final static byte ReplyNoError = 0x0;

    static int uID = 0x80000001;    /* start at -2 billion and go up to -1 */
    final static byte[] nullData = new byte[0];

    // Note! flags, cmdSet, and cmd are all byte values.
    // We represent them as shorts to make them easier
    // to work with.
    int id;
    byte flags;
    byte cmdSet;
    byte cmd;
    short errorCode;
    byte[] data;
    volatile boolean replied = false;
    int curReadIndex, curWriteIndex;

    Packet()
    {
        id = uniqID();
        flags = NoFlags;
        data = null;
        curReadIndex = curWriteIndex = 0;
    }

    static synchronized private int uniqID()
    {
        /*
         * JDWP spec does not require this id to be sequential and
         * increasing, but our implementation does. See
         * VirtualMachine.notifySuspend, for example.
         */
        return uID++;
    }

    public int getLength() {
        return curWriteIndex;
    }

    public String toString() {

        String[][] cmds;
        int cmdSetIndex = cmdSet;
        int cmdIndex = cmd;

        if ( cmdSet < 64 ) {
            cmds = VMcmds;
        } else if ( cmdSet < 128 ) {
            cmds = DBGcmds;
            cmdSetIndex -= 64;
/*
 * THIS IS LIKELY TO CAUSE PROBLEMS IN THE FUTURE
 */
            cmdIndex -= 99;
        } else {
            cmds = VENcmds;
            cmdSetIndex = cmdIndex = 0;
        }

        StringBuffer s = new StringBuffer();

        // return cmdSet + "/"+cmd + "/" + errorCode +
        // "\n--->\n" + new String( data ) + "\n<---\n";
        try {
            s.append("Sending through: ");
            s.append(cmds[cmdSetIndex][0]);
            s.append("(");
            s.append(cmdSet);
            s.append(")/");
            s.append(cmds[cmdSetIndex][cmdIndex]);
            s.append("(");
            s.append(cmd);
            s.append(")\n");
        } catch ( ArrayIndexOutOfBoundsException e ) {
            System.out.println( "UNKNOWN COMMAND: " + cmdSet + "/" + cmd );
        }
        if (Options.getVerbosity() >= 5) {
            s.append(cmdSet);
            s.append("/");
            s.append(cmd);
            s.append("/");
            s.append(errorCode);
            s.append("/");
            s.append(id);
        }
        if (Options.getVerbosity() >= 8) {
            s.append("\n--->\n");
            for (int i = 0; i < data.length; i++) {
                s.append(Integer.toHexString((int)((long)data[i] & 0xFF))).append(".");
            }
            s.append("\n<----");
        }
        if (cmdSet == 15 && cmd == 1) {
            s.append("\nEventKind == ").append(data[0]).append("\n");
        } else if (cmdSet == 64 && cmd == 100) {
            s.append("\nEventKind == ").append(data[5]).append("\n");
        }
        return s.toString();
    }
}
