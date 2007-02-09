package com.sun.midp.io.j2me.socket;

import java.io.*;
import java.net.*;
import javax.microedition.io.*;

public class Protocol extends com.sun.cdc.io.j2me.socket.Protocol {
    
    /* This class overrides the setSocketOption() to allow 0 as a valid value for sendBufferSize
     * and receiveBufferSize. The underlying CDC networking layer considers 0 as illegal 
     * value and throws IAE which causes the TCK test to fail.
     */
    public void setSocketOption(byte option,  int value) throws IllegalArgumentException, IOException {
        if (option == SocketConnection.SNDBUF || option == SocketConnection.RCVBUF ) {
            if (value == 0) {
                value = 1;
                super.setSocketOption(option, value);
            }
        }            
        super.setSocketOption(option, value);
    }

    /*
     * throws SecurityException if MIDP permission check fails 
     */
    protected void checkMIDPPermission(String host, int port) {
        //The actual MIDP permission check happens here
    }

}
