import java.io.*;
import java.util.*;
import javax.microedition.io.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.wireless.messaging.*;
import com.sun.tck.wma.*;

/**
 * An example MIDlet with simple "Hello" text and an Exit command.
 * Refer to the startApp, pauseApp, and destroyApp
 * methods so see how each handles the requested transition.
 *
 * @author  shai
 * @version
 */
public class NCISlave extends MIDlet implements CommandListener, Runnable {
    /**
     * Messages pending for the server
     */
    private Vector messageQueue = new Vector();
    private static final int SLEEP_LENGTH = 50;
    private Command exitCommand; // The exit command
    private Command connectCommand; // The connect command
    private Command logCommand; // The log command
    private Command statusCommand; // The status command
    private Display display;    // The display for this MIDlet
    private TextBox url;
    private Form    configuration;
    private TextField  ip;
    private TextField  port;
    private TextField  phoneNum;
    private Form status;
    private StringItem  statusConect;
    private TextField  statusSend;
    private TextField  statusRecieve;
    private StringBuffer logBuffer;
    private int maxLenLogBuffer = 1000;
    private volatile String address = null;
    private volatile String port_address = null;
    private Vector listeners = new Vector();
    private static Hashtable urlMap = new Hashtable();
    private int NumOfSMSSend;
    private int NumOfSMSRecv;
    private int NumOfActions;
    private String LastAction;
    private String Connection;
    private String Connection1;
    private String TCKPort;
    private String TCKIPAddress;
    MessageConnection smsconn, mmsconn;

    static final String ALLOWED_HEADER_FIELDS[] = {
        "X-Mms-Delivery-Time", "X-Mms-Priority"
    };
    static final String STREAM_SIGNATURE = "application/vnd.wap.mms-message";

    /**
     * Start up the Hello MIDlet by creating the TextBox and associating
     * the exit command and listener.
     */
    public void startApp() {
        try {
            System.out.println("Starting MIDlet");
            display = Display.getDisplay(this);
            exitCommand = new Command("Exit", Command.EXIT, 2);
            connectCommand = new Command("Connect", Command.SCREEN, 1);
	    logCommand = new Command("Log", Command.SCREEN, 1);
	    statusCommand = new Command("Status", Command.SCREEN, 1);
            TCKPort = getAppProperty("TCKPort");
            TCKIPAddress = getAppProperty("TCKIPAddress");
	    configuration = new Form("Enter TCK IP and Port"); 
	    ip = new TextField("TCK Host", TCKIPAddress, 20, TextField.ANY);
	    port = new TextField("TCK Port", TCKPort, 10, TextField.ANY);
	    phoneNum = new TextField("Phone number", "+5550000", 10, TextField.UNEDITABLE);
	    configuration.append(ip);
	    configuration.append(port); 
	    configuration.append(phoneNum);
	    url = new TextBox("Logging", null,10000,0);	    
	    status = new Form("Status");
	    statusConect = new StringItem("Connection status:\n", null);
	    statusSend = new TextField("Total Send Messages", null, 3, TextField.UNEDITABLE);
	    statusRecieve = new TextField("Total Recieved Messages", null, 3, TextField.UNEDITABLE);
	    status.append(statusConect);
	    status.append(statusSend);
	    status.append(statusRecieve);
	    logBuffer = new StringBuffer(maxLenLogBuffer);  
	    LastAction = new String();

            configuration.addCommand(exitCommand);
            configuration.addCommand(connectCommand);
	    configuration.setCommandListener(this);

	    url.setCommandListener(this);
	    url.addCommand(exitCommand);
	    url.addCommand(statusCommand);    

	    status.addCommand(exitCommand);
	    status.addCommand(logCommand);
	    status.setCommandListener(this);
            
            display.setCurrent(configuration);

            Connection = getAppProperty("listen0");
            smsconn = (MessageConnection)Connector.open(Connection);
            smsconn.setMessageListener(new MessageListenerAdapter(Connection));
            Connection1 = getAppProperty("listen1");
            mmsconn = (MessageConnection)Connector.open(Connection1);
            mmsconn.setMessageListener(new MessageListenerAdapter(Connection1));
            
        } catch(Exception err) {
            reportError("Error", err);
        }
    }
    
    private void reportError(String label, Exception err) {
        err.printStackTrace();
        url.setString(label + "  " + err.getClass().getName() + ": " + err.getMessage());
        Alert error = new Alert(err.getClass().getName(), err.getMessage(), null, AlertType.ERROR);
        display.setCurrent(error);
    }
    
    public void pauseApp() {
    }
    
    /**
     * Destroy must cleanup everything not handled by the garbage collector.
     * In this case there is nothing to cleanup.
     */
    public void destroyApp(boolean unconditional) {
        address = null;
	port_address = null;
    }
    
    /*
     * Respond to commands, including exit
     * On the exit command, cleanup and notify that the MIDlet has been destroyed.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
            return;
        }
        
        if (c == connectCommand) {
		address = ip.getString();
		port_address = port.getString();
		display.setCurrent(status);
		statusConect.setText("Trying to Connect to TCK Server on host: " + address +"\nTCK port: " +port_address);
	
                new Thread(this).start();                
      
            return;
        }
	 if (c == logCommand) {
	    display.setCurrent(url);
	    return;
	}
	  if (c == statusCommand) {
	    display.setCurrent(status);
	    return;
	}
    }
    
    /**
     * This is the thread that polls the server for commands
     */
    public void run() {
	String tmpString = null;

        long time = System.currentTimeMillis();
        while(address != null) {
            //System.out.println("Polling server in: " + (System.currentTimeMillis() - time));
            time = System.currentTimeMillis();
            try {
                Message current = null;
                synchronized(this) {
                    if(messageQueue.size() > 0) {
                        current = (Message)messageQueue.elementAt(0);
                        messageQueue.removeElementAt(0);
                    }
                }
                // used to avoid lengthly synchronization holding the
                // event thread
                if(current != null) {
                    System.out.println("message not null!");
                    sendMessageToServer(current);
                }

                DataInputStream input = Connector.openDataInputStream("http://" + address + ":"+port_address+"/read");

		statusConect.setText("\nCONNECTED TO TCK SERVER.\n\nWaiting for message on: " + Connection);
		tmpString = tmpString.valueOf(NumOfSMSSend);
		statusSend.setString(tmpString);
		tmpString = tmpString.valueOf(NumOfSMSRecv);
		statusRecieve.setString(tmpString);

		
                // do we have anything pending?
                if(!input.readBoolean()) {
                    int listenerCount = input.readInt();
                    for(int iter = 0 ; iter <  listenerCount ; iter++) {
                        String currentURL = input.readUTF();
                        if(!listeners.contains(currentURL)) {
                            listeners.addElement(currentURL);
                            url.setString("Try open :" + currentURL);
                            ((MessageConnection)Connector.open(currentURL)).setMessageListener(new MessageListenerAdapter(currentURL));
                            url.setString("After open :" + currentURL);
                        }
                    }
                    synchronized(this) {
                        wait(SLEEP_LENGTH);
                    }
                    input.close();
                    continue;
                }
                
                // send an SMS message to the appropriate address
                String address = input.readUTF();

                // is the message a text/binary/multipart message
                int type = input.readInt();
                if(type == 0) {
                    String text = input.readUTF();
                    sendTextMessage(address, text);
                } else if (type == 1) {
                    byte[] data = new byte[input.readInt()];
                    input.readFully(data);
                    sendBinaryMessage(address, data);
                } else if (type == 2) {
                     byte abyte1[] = new byte[input.readInt()];
		     input.readFully(abyte1);
		     sendMultipartMessage(address, abyte1);
                }else if (type == 3) {
		    url.setString("Unknown message type to send:" + type);
		}
        
                input.close();
            } catch(Exception err) {
                url.setString("Retrying on connection error " + err.getClass() + ": " + err.getMessage());    
            }

	    	setBuffer();
	 
        }
    }
    
    private String urlEncode(byte[] data) throws IOException {
        StringBuffer out = new StringBuffer();
        ByteArrayInputStream bIn = new ByteArrayInputStream(data);
        int c = bIn.read();
        while (c >= 0) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '*' || c == '_')
                out.append((char) c);
            else if (c == ' ')
                out.append('+');
            else {
                if (c < 128) {
                    appendHex(c,out);
                } else if (c < 224) {
                    appendHex(c,out);
                    appendHex(bIn.read(),out);
                } else if (c < 240) {
                    appendHex(c,out);
                    appendHex(bIn.read(),out);
                    appendHex(bIn.read(),out);
                }                
            }
            c = bIn.read();
        }
        return out.toString();
    }
    
    private void appendHex(int arg0, StringBuffer buff){
        buff.append('%');
        if (arg0<16) buff.append('0');
        buff.append(Integer.toHexString(arg0));
    }
    
    /**
     * Sends a message received by this phone to the server
     */
    private void sendMessageToServer(final Message message) {
        new Thread() {
            public void run() {
                while(true) {
                    try {
                        StringBuffer url1 = new StringBuffer("http://");
                        url1.append(address);
                        url1.append(":"+TCKPort+"/write;");
                        if(message instanceof TextMessage) {
                            LastAction = "Received TEXT SMS from:"+urlMap.get(message);
                            NumOfSMSRecv++;

                            url1.append("t;");
                            url1.append(urlEncode(((TextMessage)message).getPayloadText().getBytes("UTF-8")));
                            url1.append(";");
                        } else if (message instanceof BinaryMessage) {
                            LastAction = "Received BINARY SMS from:"+urlMap.get(message);
                            
                            NumOfSMSRecv++;
                            url1.append("b;");
                            url1.append(urlEncode(((BinaryMessage)message).getPayloadData()));
                            url1.append(";");
                        } else {
                            NumOfSMSRecv++;
                            url1.append("m;");
                            url1.append(urlEncode(getAsByteArray((MultipartMessage)message)));
                            url1.append(";");
                        }

                        System.out.println("Sending message to server address: " +urlMap.get(message));
                        url1.append(urlEncode(((String)urlMap.get(message)).getBytes("iso8859-1")));
                        url1.append(";");
                        long time = 0;
                        if (message.getTimestamp() != null) { 
                            time = message.getTimestamp().getTime();
                        }
                        url1.append(time);
                        DataInputStream input = Connector.openDataInputStream(url1.toString());
                        input.readBoolean();
                    } catch(Exception err) {
                        reportError("Send to server error", err);
                        continue;
                    }

		   setBuffer();
	
                    // will only return if no exception was thrown
                    return;
                }
            }
        }.start();
    }

    private void sendBinaryMessage(String address, byte[] data) throws IOException {
        try {
            BinaryMessage message = (BinaryMessage)smsconn.newMessage(
                MessageConnection.BINARY_MESSAGE);
            System.out.println("sendBinaryMessage to:"+address);
            message.setAddress(address);
            message.setPayloadData(data);
            smsconn.send(message);
            LastAction = "Sending BINARY SMS to:" + address;
            NumOfSMSSend++;
        } catch(Exception e) {
            reportError("sendBinaryMessage error", e);
        }
    }

    private void sendTextMessage(String address, String text) throws IOException {
        try {
            TextMessage txtmessage = (TextMessage)smsconn.newMessage(
            MessageConnection.TEXT_MESSAGE);
            txtmessage.setAddress(address);
            txtmessage.setPayloadText(text);
            smsconn.send(txtmessage);
            LastAction = "Sending TEXT SMS to:" + address;
            NumOfSMSSend++;
        } catch(Exception e) {
            reportError("sendBinaryMessage error", e);
        }
    }

    private void setBuffer(){
	String tmpString;

	int currentBufLen = logBuffer.length();
	int testSize = maxLenLogBuffer-LastAction.length();
	int newLength = currentBufLen+maxLenLogBuffer;

	if(currentBufLen > testSize) {
	    logBuffer.setLength(newLength);
	    logBuffer.delete(currentBufLen, newLength);

	}

	NumOfActions++;
	logBuffer.append("\n"+NumOfActions+") "+LastAction);
	tmpString = logBuffer.toString();
	url.setString(tmpString);
    }


    private void sendMultipartMessage(String s, byte abyte0[])
	throws IOException
    {
	try
	{
	    MultipartMessage multipartmessage = createFromByteArray(abyte0);
	    System.out.println("sendMultipartMessage to:" + s);
	    multipartmessage.setAddress(s);
	    mmsconn.send(multipartmessage);
	    LastAction = "Sending Multipart Message to:" + s;
	    NumOfSMSSend++;
	}
	catch(Exception exception)
	{
	    reportError("sendMultipartMessage error", exception);
	}
    }


    private byte[] getAsByteArray(MultipartMessage multipartmessage)
        throws IOException
    {
        ByteArrayOutputStream bytearrayoutputstream = new ByteArrayOutputStream();
        DataOutputStream dataoutputstream = new DataOutputStream(bytearrayoutputstream);
        dataoutputstream.writeUTF("application/vnd.wap.mms-message");
        dataoutputstream.writeUTF("X-Mms-Message-Type");
        dataoutputstream.writeUTF("m-send-req");
        dataoutputstream.writeUTF("X-Mms-Transaction-ID");
        dataoutputstream.writeUTF(String.valueOf(System.currentTimeMillis()));
        dataoutputstream.writeUTF("X-Mms-Version");
        dataoutputstream.writeUTF("1.0");
        for(int i = 0; i < ALLOWED_HEADER_FIELDS.length; i++)
        {
            String s1 = multipartmessage.getHeader(ALLOWED_HEADER_FIELDS[i]);
            if(s1 != null)
            {
                dataoutputstream.writeUTF(ALLOWED_HEADER_FIELDS[i]);
                dataoutputstream.writeUTF(s1);
            }
        }

        String s = multipartmessage.getAddress();
        if(s != null)
        {
            dataoutputstream.writeUTF("From");
            dataoutputstream.writeUTF(getDevicePortionOfAddress(s));
            System.out.println("From:" + getDevicePortionOfAddress(s));
        }
        String as[] = multipartmessage.getAddresses("to");
        if(as != null)
        {
            dataoutputstream.writeUTF("To");
            writeStringArray(dataoutputstream, as, true);
            System.out.println("To:" + as[0]);
        }
        String as1[] = multipartmessage.getAddresses("cc");
        if(as1 != null)
        {
            dataoutputstream.writeUTF("Cc");
            writeStringArray(dataoutputstream, as1, true);
        }
        String as2[] = multipartmessage.getAddresses("bcc");
        if(as2 != null)
        {
            dataoutputstream.writeUTF("Bcc");
            writeStringArray(dataoutputstream, as2, true);
        }
        long l = 0L;
        Date date = multipartmessage.getTimestamp();
        if(date != null && (l = date.getTime()) != 0L)
        {
            dataoutputstream.writeUTF("Date");
            dataoutputstream.writeUTF(String.valueOf(l));
        }
        String s2 = multipartmessage.getSubject();
        if(s2 != null)
        {
            dataoutputstream.writeUTF("Subject");
            dataoutputstream.writeUTF(s2);
        }
        dataoutputstream.writeUTF("Content-Type");
        Vector vector = new Vector();
        String s3 = multipartmessage.getStartContentId();
        if(s3 != null)
        {
            System.out.println("startContentID:" + s3);
            vector.addElement("application/vnd.wap.multipart.related");
        } else
        {
            System.out.println("startContentID:+null");
            vector.addElement("application/vnd.wap.multipart.mixed");
        }
        if(s3 != null)
            if(multipartmessage.getMessagePart(s3) == null)
            {
                System.out.println("getMessagePart(startContentID):null");
            } else
            {
                vector.addElement("start = <" + s3 + ">");
                vector.addElement("type = " + multipartmessage.getMessagePart(s3).getMIMEType());
            }
        writeVector(dataoutputstream, vector, false);
        dataoutputstream.writeUTF("nEntries");
        MessagePart amessagepart[] = multipartmessage.getMessageParts();
        int j;
        if(amessagepart == null)
            j = 0;
        else
            j = amessagepart.length;
        System.out.println("getMessageParts():" + j);
        dataoutputstream.writeUTF(String.valueOf(j));
        for(int k = 0; k < j; k++)
        {
            MessagePart messagepart = amessagepart[k];
            writeMessagePart(dataoutputstream, messagepart);
        }

        dataoutputstream.close();
        byte abyte0[] = bytearrayoutputstream.toByteArray();
        bytearrayoutputstream.close();
        return abyte0;
    }

    static void writeMessagePart(DataOutputStream dataoutputstream, MessagePart messagepart)
        throws IOException
    {
        dataoutputstream.writeUTF("Content-Type");
        StringBuffer stringbuffer = new StringBuffer(messagepart.getMIMEType());
        String s = messagepart.getContentLocation();
        if(s != null)
        {
            stringbuffer.append("; name=\"");
            stringbuffer.append(s);
            stringbuffer.append("\"");
        }
        dataoutputstream.writeUTF(stringbuffer.toString());
        String s1 = messagepart.getContentID();
        if(s1 != null)
        {
            dataoutputstream.writeUTF("Content-ID");
            dataoutputstream.writeUTF(s1);
        }
        String s2 = messagepart.getEncoding();
        if(s2 != null)
        {
            dataoutputstream.writeUTF("Encoding");
            dataoutputstream.writeUTF(s2);
        }
        dataoutputstream.writeUTF("Content-Length");
        dataoutputstream.writeInt(messagepart.getLength());
        dataoutputstream.writeUTF("Content");
        dataoutputstream.write(messagepart.getContent());
    }

    static void writeVector(DataOutputStream dataoutputstream, Vector vector, boolean flag)
        throws IOException
    {
        StringBuffer stringbuffer = new StringBuffer();
        int i = vector.size();
        Object obj = null;
        if(i > 0)
        {
            String s = (String)vector.elementAt(0);
            if(flag)
                s = getDevicePortionOfAddress(s);
            stringbuffer.append(s);
        }
        for(int j = 1; j < i; j++)
        {
            stringbuffer.append("; ");
            String s1 = (String)vector.elementAt(j);
            if(flag)
                s1 = getDevicePortionOfAddress(s1);
            stringbuffer.append(s1);
        }

        dataoutputstream.writeUTF(stringbuffer.toString());
    }

    private static void writeStringArray(DataOutputStream dataoutputstream, String as[], boolean flag)
        throws IOException
    {
        StringBuffer stringbuffer = new StringBuffer();
        int i = as.length;
        Object obj = null;
        if(i > 0)
        {
            String s = as[0];
            if(flag)
                s = getDevicePortionOfAddress(s);
            stringbuffer.append(s);
        }
        for(int j = 1; j < i; j++)
        {
            stringbuffer.append("; ");
            String s1 = as[j];
            if(flag)
                s1 = getDevicePortionOfAddress(s1);
            stringbuffer.append(s1);
        }

        dataoutputstream.writeUTF(stringbuffer.toString());
    }

    static String getDevicePortionOfAddress(String s)
        throws IllegalArgumentException
    {
        MMSAddress mmsaddress = MMSAddress.getParsedMMSAddress(s);
        if(mmsaddress == null || mmsaddress.address == null)
            throw new IllegalArgumentException("MMS Address has no device portion");
        else
            return mmsaddress.address;
    }

    static void readVector(DataInputStream datainputstream, Vector vector, boolean flag)
        throws IOException
    {
        String s = datainputstream.readUTF();
        int i = -2;
        String s1 = "";
        if(flag)
            s1 = "mms://";
        int j;
        for(; i != -1; i = j)
        {
            j = s.indexOf("; ", i + 2);
            String s2 = null;
            if(j == -1)
                s2 = s1 + s.substring(i + 2);
            else
                s2 = s1 + s.substring(i + 2, j);
            vector.addElement(s2);
        }

    }

    private MultipartMessage createFromByteArray(byte abyte0[])
        throws IOException
    {
        ByteArrayInputStream bytearrayinputstream = new ByteArrayInputStream(abyte0);
        DataInputStream datainputstream = new DataInputStream(bytearrayinputstream);
        String s = datainputstream.readUTF();
        if(!s.equals("application/vnd.wap.mms-message"))
            throw new IOException("invalid data format");
        for(int i = 0; i < 6; i++)
            datainputstream.readUTF();

        String as[] = new String[ALLOWED_HEADER_FIELDS.length];
        String s1;
        int j;
        for(s1 = datainputstream.readUTF(); (j = getHeaderFieldIndex(s1)) != -1; s1 = datainputstream.readUTF())
            as[j] = datainputstream.readUTF();

        String s2 = null;
        if(s1.equals("From"))
        {
            s2 = "mms://" + datainputstream.readUTF();
            s1 = datainputstream.readUTF();
        }
        Vector vector = new Vector();
        if(s1.equals("To"))
        {
            readVector(datainputstream, vector, true);
            s1 = datainputstream.readUTF();
        }
        Vector vector1 = new Vector();
        if(s1.equals("Cc"))
        {
            readVector(datainputstream, vector1, true);
            s1 = datainputstream.readUTF();
        }
        Vector vector2 = new Vector();
        if(s1.equals("Bcc"))
        {
            readVector(datainputstream, vector2, true);
            s1 = datainputstream.readUTF();
        }
        long l = 0L;
        if(s1.equals("Date"))
        {
            String s3 = datainputstream.readUTF();
            long l1;
            try
            {
                l1 = Long.parseLong(s3);
            }
            catch(NumberFormatException numberformatexception)
            {
                l1 = 0L;
            }
            s1 = datainputstream.readUTF();
        }
        String s4 = null;
        if(s1.equals("Subject"))
        {
            s4 = datainputstream.readUTF();
            s1 = datainputstream.readUTF();
        }
        String s5 = null;
        Object obj = null;
        Object obj1 = null;
        Vector vector3 = new Vector();
        readVector(datainputstream, vector3, false);
        int k = vector3.size();
        for(int i1 = 0; i1 < k; i1++)
        {
            String s8 = (String)vector3.elementAt(i1);
            if(s8.startsWith("start = <"))
            {
                s5 = s8.substring(9);
                s5 = s5.substring(0, s5.length() - 1);
                continue;
            }
            if(s8.startsWith("Application-ID = "))
            {
                String s6 = s8.substring(17);
                continue;
            }
            String s7;
            if(s8.startsWith("Reply-To-Application-ID = "))
                s7 = s8.substring(26);
        }

        s1 = datainputstream.readUTF();
        int j1 = 0;
        String s9 = datainputstream.readUTF();
        try
        {
            j1 = Integer.parseInt(s9);
        }
        catch(NumberFormatException numberformatexception1)
        {
            j1 = 0;
        }
        Vector vector4 = new Vector();
        for(int k1 = 0; k1 < j1; k1++)
            vector4.addElement(createMessagePart(datainputstream));

        datainputstream.close();
        bytearrayinputstream.close();
        MultipartMessage multipartmessage = (MultipartMessage)mmsconn.newMessage("multipart");
        multipartmessage.setSubject(s4);
        for(int i2 = 0; i2 < ALLOWED_HEADER_FIELDS.length; i2++)
            if(as[i2] != null)
                multipartmessage.setHeader(ALLOWED_HEADER_FIELDS[i2], as[i2]);

        if(s2 != null)
            multipartmessage.setAddress(s2);
        for(int j2 = 0; j2 < vector.size(); j2++)
            multipartmessage.addAddress("to", (String)vector.elementAt(j2));

        for(int k2 = 0; k2 < vector1.size(); k2++)
            multipartmessage.addAddress("cc", (String)vector1.elementAt(k2));

        for(int l2 = 0; l2 < vector2.size(); l2++)
            multipartmessage.addAddress("bcc", (String)vector2.elementAt(l2));

        for(int i3 = 0; i3 < vector4.size(); i3++)
            multipartmessage.addMessagePart((MessagePart)vector4.elementAt(i3));

        if(s5 != null && j1 > 0)
            multipartmessage.setStartContentId(s5);
        return multipartmessage;
    }

    static MessagePart createMessagePart(DataInputStream datainputstream)
        throws IOException
    {
        String s = datainputstream.readUTF();
        String s1 = datainputstream.readUTF();
        s = datainputstream.readUTF();
        String s2 = null;
        if(s.equals("Content-ID"))
        {
            s2 = datainputstream.readUTF();
            s = datainputstream.readUTF();
        }
        String s3 = null;
        if(s.equals("Encoding"))
        {
            s3 = datainputstream.readUTF();
            s = datainputstream.readUTF();
        }
        int i = datainputstream.readInt();
        byte abyte0[] = new byte[i];
        s = datainputstream.readUTF();
        datainputstream.readFully(abyte0);
        String s4 = s1;
        String s5 = null;
        int j = s1.indexOf(';');
        if(j != -1 && s1.substring(j).startsWith("; name=\""))
        {
            s5 = s1.substring(j + 8, s1.length() - 1);
            s4 = s1.substring(0, j);
        }
        return new MessagePart(abyte0, s4, s2, s5, s3);
    }

    static int getHeaderFieldIndex(String s)
    {
        String s1 = s.toLowerCase();
        for(int i = 0; i < ALLOWED_HEADER_FIELDS.length; i++)
            if(s1.equals(ALLOWED_HEADER_FIELDS[i].toLowerCase()))
                return i;

        return -1;
    }

  
    

    class MessageListenerAdapter implements MessageListener, Runnable {
        private String url;
        private MessageConnection conn;
     
        public MessageListenerAdapter(String url) {
            this.url = url;
        }
        
        public void notifyIncomingMessage(MessageConnection messageConnection) {
        conn = messageConnection;
            new Thread(this).start();     
        }

        public void run() {
            System.out.println("message income>>");

            try {
                Message m = conn.receive();

                System.out.println("received from:"+m.getAddress());
                if (m instanceof MultipartMessage) {
                    String[] to = ((MultipartMessage)m).getAddresses("to");
                    if (to != null) {
                        for (int i = 0; i < to.length; i++) {
                            System.out.println("it's to:"+to[i]);
                        }
                    }
                }

                urlMap.put(m, url);
                synchronized(this) {
                    messageQueue.addElement(m);
                }
                System.out.println("message income<<");
            } catch(Exception err) {
                reportError("Message queue error", err);
            }
        }
    }

}

