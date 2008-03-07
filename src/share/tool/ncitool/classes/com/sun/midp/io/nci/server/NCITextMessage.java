package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;

public class NCITextMessage extends NCIMessage implements TextMessage {
    private String data;
    
    /**
     * Sets the payload data of this message. The payload data 
     * may be <code>null</code>.
     * @param data payload data as a <code>String</code>
     * @see #getPayloadText
     */
    public void setPayloadText(String data) {
        this.data = data;
    }

    /**
     * Returns the message payload data as a <code>String</code>.
     * 
     * @return the payload of this message, or <code>null</code> 
     * if the payload for the message is not set.
     * @see #setPayloadText
     */
    public String getPayloadText() {
        return data;
    }    
    
    public String toString() {
        String val = "[Text message: " + getAddress();
        val += ", ";
        val += data;
        val += "]";
        return val;
    }
}
