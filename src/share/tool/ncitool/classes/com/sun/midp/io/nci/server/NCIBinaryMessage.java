package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;

public class NCIBinaryMessage extends NCIMessage implements BinaryMessage {
    private byte[] data;
    
    /**
     * Sets the payload data of this message. The payload may
     * be set to <code>null</code>.
     * <p>Setting the payload using this method only sets the 
     * reference to the byte array. Changes made to the contents
     * of the byte array subsequently affect the contents of this
     * <code>BinaryMessage</code> object. Therefore, applications
     * shouldn't reuse this byte array before the message is sent and the
     * <code>MessageConnection.send</code> method returns.
     * </p>
     * @param data payload data as a byte array
     * @see #getPayloadData
     */
    public void setPayloadData(byte[] data) {
        this.data = data;
    }

    /**
     * Returns the message payload data as an array
     * of bytes.
     * 
     * <p>Returns <code>null</code>, if the payload for the message 
     * is not set.
     * </p>
     * <p>The returned byte array is a reference to the 
     * byte array of this message and the same reference
     * is returned for all calls to this method made before the
     * next call to <code>setPayloadData</code>.
     * 
     * @return the payload data of this message, or
     * <code>null</code> if the data has not been set 
     * @see #setPayloadData
     */
    public byte[] getPayloadData() {
        return data;
    }    
    
    public String toString() {
        String val = "[Binary message: " + getAddress();
        val += ", ";
        if(data != null) 
            val += data.length;
        else 
            val += "null";
        val += "]";
        return val;
    }
}
