package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;

public class NCIConnector implements Connector {
    private NCIMessageConnection handler;
    public MessageConnection open(String name) {
		if (name.startsWith("sms:") || name.startsWith("cbs:") || name.startsWith("mms:")) {
            NCIMessageConnection val = new NCIMessageConnection(name);
			return val;
		}else {
			throw new IllegalArgumentException("Wrong protocol scheme");
		}
    }
    
    
    public static void main(String[] argv) throws Exception {
	MessageConnection con;
	Message m;
		/*
	 con = new NCIConnector().open("sms://:54321");
        m = con.newMessage(MessageConnection.BINARY_MESSAGE, "sms://+5550001:54321");
        ((BinaryMessage)m).setPayloadData(new String("1234567890").getBytes());
        con.send(m);
        System.out.println(con.receive().toString());
	 con.close();

	 con = new NCIConnector().open("sms://:54321");
        m = con.newMessage(MessageConnection.TEXT_MESSAGE, "sms://+5550001:54321");
        ((TextMessage)m).setPayloadText(new String("1234567890"));
        con.send(m);
        System.out.println(con.receive().toString());
	 con.close();
	 */
	/*
        con = new NCIConnector().open("mms://:com.sun.tck.wma.mms.NCIslave");
        m = con.newMessage(MessageConnection.MULTIPART_MESSAGE, "mms://+5550001:com.sun.tck.wma.mms.NCIslave");
        ((MultipartMessage)m).addMessagePart(new MessagePart(new String("my content").getBytes(), "text", "id-1", null, null));
        con.send(m);
        System.out.println(con.receive().toString());
	 con.close();
	*/
    }
}
