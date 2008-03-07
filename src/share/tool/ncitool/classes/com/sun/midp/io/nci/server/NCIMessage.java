package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;
import java.util.*;

public class NCIMessage implements Message {
    private String url;
    private Date timestamp = new Date();
    private String address;
    
    /**
     * Returns the timestamp indicating when this message has been
     * sent. 
     * @return <code>Date</code> indicating the timestamp in the message or
     * <code>null</code> if the timestamp is not set or
     * if the time information is not available in the underlying
     * protocol message.
     */
    public Date getTimestamp() {
        return timestamp;
    }

    /**
     * Sets the address associated with this message,
     * that is, the address returned by the <code>getAddress</code> method.
     * The address may be set to <code>null</code>.
     * @param address address for the message
     * @see #getAddress()
     */
    public void setAddress(String address) {
        this.address = address;
    }

    /**
     * Returns the address associated with this message.
     * 
     * <p>If this is a message to be sent, this address
     * is the address of the recipient.
     * </p>
     * <p>If this is a message that has been received,
     * this address is the sender's address.
     * </p>
     * <p>Returns <code>null</code>, if the address for the message 
     * is not set.
     * </p>
     * <p><strong>Note</strong>: This design allows sending responses
     * to a received message easily by reusing the 
     * same <code>Message</code> object and just replacing the 
     * payload. The address field can normally be
     * kept untouched (unless the used messaging protocol
     * requires some special handling of the address).
     * </p>
     * @return the address of this message, or <code>null</code>
     * if the address is not set
     * @see #setAddress(String)
     */
    public String getAddress() {
        return address;
    }
    
}
