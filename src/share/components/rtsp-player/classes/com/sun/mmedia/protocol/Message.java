/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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



