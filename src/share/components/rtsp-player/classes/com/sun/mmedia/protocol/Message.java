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
package com.sun.mmedia.rtsp.protocol;

import java.util.*;

public class Message {
    private byte data[];
    private int type;
    private Object parameter;

    public Message(byte data[]) {
        this.data = data;

        parseData();
    }

    private void parseData() {
        String message= new String( data);

        int index= message.indexOf( ' ');

        message= message.substring( 0, index);

        type = new MessageType( message).getType();

        switch (type) {
            case MessageType.DESCRIBE:
                parameter = (Object) new DescribeMessage(data);
                break;
            case MessageType.SETUP:
                parameter = (Object) new SetupMessage(data);
                break;
            case MessageType.PLAY:
                parameter = (Object) new PlayMessage(data);
                break;
            case MessageType.PAUSE:
                parameter = (Object) new PauseMessage(data);
                break;
            case MessageType.TEARDOWN:
                parameter = (Object) new TeardownMessage(data);
                break;
            case MessageType.OPTIONS:
                parameter = (Object) new OptionsMessage(data);
                break;
            case MessageType.RESPONSE:
                parameter = (Object) new ResponseMessage(data);
                break;
            case MessageType.SET_PARAMETER:
                parameter = (Object) new SetParameterMessage(data);
                break;
            default:
                System.out.println("Unknown msg type: " + type);
                break;
        }
    }

    public int getType() {
        return type;
    }

    public Object getParameter() {
        return parameter;
    }
}



